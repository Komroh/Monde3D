// Copyright (C) 2008-2011 Gael Guennebaud <gael.guennebaud@inria.fr>

#include "mesh.h"
#include "bvh.h"

#include <iostream>
#include <fstream>
#include <limits>
#include <filesystem/resolver.h>
#include <tiny_obj_loader.h>
#include <lib3ds.h>

Mesh::Mesh(const PropertyList &propList)
    : m_isInitialized(false), m_BVH(nullptr)
{
    std::string filename = propList.getString("filename");
    loadFromFile(filename);
    // buildBVH();
}

void Mesh::loadFromFile(const std::string& filename)
{
    filesystem::path filepath = getFileResolver()->resolve(filename);
    std::ifstream is(filepath.str());
    if (is.fail())
        throw RTException("Unable to open mesh file \"%s\"!", filepath.str());

    const std::string ext = filepath.extension();
    if(ext=="off" || ext=="OFF")
        loadOFF(filepath.str());
    else if(ext=="obj" || ext=="OBJ")
        loadOBJ(filepath.str());
    else if(ext=="3ds" || ext=="3DS")
        load3DS(filepath.str());
    else
        std::cerr << "Mesh: extension \'" << ext << "\' not supported." << std::endl;
}

void Mesh::loadOFF(const std::string& filename)
{
    std::ifstream in(filename.c_str(),std::ios::in);
    if(!in)
    {
        std::cerr << "File not found " << filename << std::endl;
        return;
    }

    std::string header;
    in >> header;

    // check the header file
    if(header != "OFF")
    {
        std::cerr << "Wrong header = " << header << std::endl;
        return;
    }

    int nofVertices, nofFaces, inull;
    int nb, id0, id1, id2;
    Vector3f v;

    in >> nofVertices >> nofFaces >> inull;

    for(int i=0 ; i<nofVertices ; ++i)
    {
        in >> v.x() >> v.y() >> v.z();
        m_vertices.push_back(Vertex(v));
    }

    for(int i=0 ; i<nofFaces ; ++i)
    {
        in >> nb >> id0 >> id1 >> id2;
        assert(nb==3);
        m_faces.push_back(FaceIndex(id0, id1, id2));
    }

    in.close();

    computeAABB();
}

void Mesh::loadOBJ(const std::string& filename)
{
    std::vector<tinyobj::shape_t> shapes;
    std::vector<tinyobj::material_t> materials;
    std::string err;
    bool success = tinyobj::LoadObj(shapes, materials, err, filename.c_str());

    if (!success) {
        throw RTException("Mesh::loadObj: error loading file %s: %s", filename, err);
    }

    bool needNormals = false;

    // copy vertices
    int currentShapeIndex = 0;
    for (size_t i = 0; i < shapes.size(); i++) {
        m_vertices.resize(currentShapeIndex + shapes[i].mesh.positions.size()/3);
        for (size_t v = 0; v < shapes[i].mesh.positions.size()/3; v++) {
            m_vertices[v+currentShapeIndex] = Vertex(Vector3f(shapes[i].mesh.positions[3*v],
                                                             shapes[i].mesh.positions[3*v+1],
                                                             shapes[i].mesh.positions[3*v+2]));

            if(!shapes[i].mesh.normals.empty())
                m_vertices[v+currentShapeIndex].normal = Vector3f(shapes[i].mesh.normals[3*v],
                                                                 shapes[i].mesh.normals[3*v+1],
                                                                 shapes[i].mesh.normals[3*v+2]);
            else
                needNormals = true;

            if(!shapes[i].mesh.texcoords.empty())
                m_vertices[v+currentShapeIndex].texcoord = Vector2f(shapes[i].mesh.texcoords[2*v],
                                                                   shapes[i].mesh.texcoords[2*v+1]);
        }
        currentShapeIndex += shapes[i].mesh.positions.size()/3;
    }

    // copy faces
    currentShapeIndex = 0;
    int vertexIndexOffset = 0;
    for (size_t i = 0; i < shapes.size(); i++) {
        m_faces.resize(currentShapeIndex + shapes[i].mesh.indices.size()/3);
        for (size_t f = 0; f < shapes[i].mesh.indices.size()/3; f++) {
            m_faces[f+currentShapeIndex] = Vector3i(shapes[i].mesh.indices[3*f]+vertexIndexOffset,
                                                   shapes[i].mesh.indices[3*f+1]+vertexIndexOffset,
                                                   shapes[i].mesh.indices[3*f+2]+vertexIndexOffset);
        }
        currentShapeIndex += shapes[i].mesh.indices.size()/3;
        vertexIndexOffset += shapes[i].mesh.positions.size()/3;
    }

    computeAABB();
}

void Mesh::load3DS(const std::string& filename)
{
    Lib3dsFile* pFile = lib3ds_file_open(filename.c_str());

    if(pFile==0)
    {
        std::cerr << "file not find !" << std::endl;
        return;
    }

    lib3ds_file_eval(pFile,0);

    /*
      1 objet 3DS = ensemble de sous-objets
      1 sous-objet = ensemble de sommets et de faces (triangles)
      1 triangle = liste de 3 indices
  */

    // Parcours de tous les sous-objets

    /* pFile->meshes == pointeur sur le premier sous-objet */
    for(int m=0; m<pFile->nmeshes; m++)
    {
        /* pointeur sur le sous-objet courrant */
        Lib3dsMesh* pMesh = pFile->meshes[m];

        uint i;
        uint offset_id = m_vertices.size();

        // Parcours de tous les points du sous-objet
        for (i = 0; i < pMesh->nvertices; i++)
        {
            /* position du sommet i */
            m_vertices.push_back(Vertex(Vector3f(pMesh->vertices[i][0],
                                                pMesh->vertices[i][1],
                                                pMesh->vertices[i][2])));

            // ... alors les coordonnées de texture sont disponibles :
            m_vertices.back().texcoord[0] = pMesh->texcos[i][0]; // i = numéro du sommet
            m_vertices.back().texcoord[1] = pMesh->texcos[i][1];
        }

        // Parcours de toutes les faces du sous-objet
        for (i = 0; i < pMesh->nfaces; i++)
        {
            m_faces.push_back(FaceIndex(
                                 offset_id + pMesh->faces[i].index[0],
                             offset_id + pMesh->faces[i].index[1],
                    offset_id + pMesh->faces[i].index[2]));
        }
    }

    computeAABB();
}

void Mesh::loadRawData(float* positions, int nbVertices, int* indices, int nbTriangles)
{
    m_vertices.resize(nbVertices);
    for(int i=0; i<nbVertices; ++i)
        m_vertices[i].position = Point3f::Map(positions+3*i);
    m_faces.resize(nbTriangles);
    for(int i=0; i<nbTriangles; ++i)
        m_faces[i] = Eigen::Vector3i::Map(indices+3*i);

    computeAABB();
}

Mesh::~Mesh()
{
    if(m_isInitialized)
    {
        glDeleteBuffers(1,&m_vertexBufferId);
        glDeleteBuffers(1,&m_indexBufferId);
    }
    delete m_BVH;
}

void Mesh::makeUnitary()
{
    // computes the lowest and highest coordinates of the axis aligned bounding box,
    // which are equal to the lowest and highest coordinates of the vertex positions.
    Eigen::Vector3f lowest, highest;
    lowest.fill(std::numeric_limits<float>::max());   // "fill" sets all the coefficients of the vector to the same value
    highest.fill(-std::numeric_limits<float>::max());

    for(VertexArray::iterator v_iter = m_vertices.begin() ; v_iter!=m_vertices.end() ; ++v_iter)
    {
        // - v_iter is an iterator over the elements of mVertices,
        //   an iterator behaves likes a pointer, it has to be dereferenced (*v_iter, or v_iter->) to access the referenced element.
        // - Here the .aray().min(_) and .array().max(_) operators work per component.
        //
        lowest  = lowest.array().min(v_iter->position.array());
        highest = highest.array().max(v_iter->position.array());
    }

    Point3f center = (lowest+highest)/2.0;
    float m = (highest-lowest).maxCoeff();
    for(VertexArray::iterator v_iter = m_vertices.begin() ; v_iter!=m_vertices.end() ; ++v_iter)
        v_iter->position = (v_iter->position - center) / m;

    computeAABB();
}

void Mesh::computeAABB()
{
    m_AABB.setNull();
    for(VertexArray::iterator v_iter = m_vertices.begin() ; v_iter!=m_vertices.end() ; ++v_iter)
        m_AABB.extend(v_iter->position);
}

void Mesh::buildBVH()
{
    if(m_BVH)
        delete m_BVH;
    m_BVH = new BVH;
    m_BVH->build(this, 10, 100);
}

long int Mesh::ms_itersection_count = 0;

bool Mesh::intersectFace(const Ray& ray, Hit& hit, int faceId) const
{
    ms_itersection_count++;

    Point3f a=vertexOfFace(faceId,0).position;
    Point3f b=vertexOfFace(faceId,1).position;
    Point3f c=vertexOfFace(faceId,2).position;

    Vector3f d=ray.direction;
    Vector3f e1=a-c;
    Vector3f e2=b-c;
    Vector3f t=ray.origin-c;

    Vector3f p=d.cross(e2);
    Vector3f q=t.cross(e1);

    Vector3f res=(1.f/(p.dot(e1)))*Vector3f{q.dot(e2),p.dot(t),q.dot(d)};

   if(res.y()+res.z()<1 && res.y()>=0  && res.z()>=0 & res.x()>0)
    {
        hit.setT(res.x());
        hit.setNormal(e1.cross(e2).normalized());
        return true;
    }
    return false;


}

bool Mesh::intersect(const Ray& ray, Hit& hit) const
{
    float tMin, tMax;
    Normal3f normal;
    Hit h;
    if( (!::intersect(ray, m_AABB, tMin, tMax, normal)) || tMin>hit.t())
        return false;

    hit.setT(tMin);
    hit.setShape(this);
    hit.setNormal(normal);
    for(int i=0; i<m_faces.size(); i++) {

       if (intersectFace(ray, h, i) && h.t() <tMax)
       {
           if(h.t()<hit.t())
               hit=h;
           break;
       }


    }

    return true;
}

std::string Mesh::toString() const {
    return tfm::format(
        "Mesh[\n"
        "  vertexCount = %i,\n"
        "  triangleCount = %i,\n"
        "  material = %s\n"
        "]",
        m_vertices.size(),
        m_faces.size(),
        m_material ? indent(m_material->toString()) : std::string("null")
    );
}

REGISTER_CLASS(Mesh, "mesh")
