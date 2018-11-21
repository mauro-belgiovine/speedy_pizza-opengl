#ifndef MESH_H
#define MESH_H

// classe Vertex:
// i vertici della mesh
#include <string.h>
#include <math.h>
#include <stdio.h>
#include <vector>
#include <GL/gl.h>
#include <stdlib.h>

#include "point3.h"

typedef struct tex_coord{
    float u,v;
} tex_coord_t;

class Vertex
{
public:
    Point3 p; // posizione

    // attributi per verice
    Vector3 n; // normale (per vertice)
};

class Edge
{
public:
    Vertex* v[2]; // due puntatori a Vertice (i due estremi dell'edge)
    // attributi per edge:
};

class Face
{
public:
    Vertex* v[3]; // tre puntatori a Vertice (i tre vertici del triangolo)
    
    // attributi per faccia
    Vector3 n; // normale (per faccia)
    int mtl_index; // indice materiale/texture (in base all'ordine nel vettore "materials")
    tex_coord_t *t_coord[3]; // cordinate texutre
    
    // costruttore
    Face(Vertex* a, Vertex* b, Vertex* c, int mtl_i) {
        v[0]=a;
        v[1]=b;
        v[2]=c;
	
	t_coord[0] = 0;
	t_coord[1] = 0;
	t_coord[2] = 0;
	
	mtl_index = mtl_i; //-1 if the face has no material associated to it
	
    }
    
    // costruttore
    Face(Vertex* a, Vertex* b, Vertex* c, tex_coord_t *ta, tex_coord_t *tb, tex_coord_t *tc, int mtl_i) {
        v[0]=a;
        v[1]=b;
        v[2]=c;
	
	t_coord[0] = ta;
	t_coord[1] = tb;
	t_coord[2] = tc;
	
	mtl_index = mtl_i; //-1 if the face has no material associated to it
	
    }
    
    
    // computa la normale della faccia
    void ComputeNormal() {
        n= -( (v[1]->p - v[0]->p) % (v[2]->p - v[0]->p) ).Normalize();
    }

    // attributi per wedge

};

class Material{
  
public:
  char name[512];
  char full_filepath[512];
  float alpha, ns, ni;
  float diffuses[3], ambience[3], specular[3];
  int illum;
  int texture;
  
  Material(){
    memset(name, '\0', sizeof(name));
    memset(full_filepath, '\0', sizeof(full_filepath));
    texture = -1; //this will be assigned when loading loading the texture into GPU memory
  }
    
};


class Mesh
{
    std::vector<Vertex> v; // vettore di vertici
    std::vector<Face> f;   // vettore di facce
    std::vector<Edge> e;   // vettore di edge (per ora, non usato)

    Point3 world_pos; // position in the world (with respect to the origin)
    
    std::vector<tex_coord_t> texture_coord;
    std::vector<Point3> normals;
    
    
    
    
    
public:
  
    Vector3 mesh_scale;

    // costruttore con caricamento
    Mesh(char *filename, bool loadMTL) {
      
	if(loadMTL){
	  //create the filename for .mtl file from the .obj one
	  size_t ext_pos = strlen(filename) - 3;
	  char fn_mtl[50];
	  memset(fn_mtl, '\0', sizeof(fn_mtl));
	  strncpy(fn_mtl, filename, ext_pos-1);
	  strcat(fn_mtl, ".mtl");
	  
	  LoadMtl(fn_mtl);
	}
	
        LoadFromObj(filename);
        ComputeNormalsPerFace();
        ComputeNormalsPerVertex();
        ComputeBoundingBox();
	mesh_scale = Vector3(1,1,1);
	
	world_pos = Center();
    }
    
    // costruttore con caricamento
    Mesh(char *filename, bool loadMTL, Vector3 scale) {
      
	if(loadMTL){
	  //create the filename for .mtl file from the .obj one
	  size_t ext_pos = strlen(filename) - 3;
	  char fn_mtl[50];
	  memset(fn_mtl, '\0', sizeof(fn_mtl));
	  strncpy(fn_mtl, filename, ext_pos-1);
	  strcat(fn_mtl, ".mtl");
	  
	  LoadMtl(fn_mtl);
	}
	
        LoadFromObj(filename);
        ComputeNormalsPerFace();
        ComputeNormalsPerVertex();
        ComputeBoundingBox();
	
	mesh_scale = scale;
	ScaleBBCord(scale); //set bbox scale according to the one applied to the mesh
	
	world_pos = Center();
    }
    
    

    // metodi
    void RenderNxF(); // manda a schermo la mesh Normali x Faccia
    void RenderNxV(bool read_mtl); // manda a schermo la mesh Normali x Vertice
    void RenderWire(); // manda a schermo la mesh in wireframe
    
    void LoadMtl(char* filename); // carica il file mtl associato a obj
    bool LoadFromObj(char* filename); //  carica la mesh da un file OFF
    void ComputeNormalsPerFace();
    void ComputeNormalsPerVertex();

    void ComputeBoundingBox();

    // centro del axis aligned bounding box
    Point3 Center() {
        return (bbmin+bbmax)/2.0;
    };

    Point3 bbmin,bbmax; // bounding box
    
    Point3 GetWorldCoords();
    void UpdateWorldCoords(Point3 p);
    void TranslateBBCoord(Point3 p);
    void ScaleBBCord(Point3 s);
    
    
};
#endif