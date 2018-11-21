// file mesh.cpp
//
// Implementazione dei metodi di Mesh

#include "mesh.h"

extern std::vector<Material> materials;

extern bool useWireframe; // quick hack: una var globale definita altrove

int m = -1;

void Mesh::ComputeNormalsPerFace()
{
    for (int i=0; i<f.size(); i++)
    {
        f[i].ComputeNormal();
    }
}

// Computo normali per vertice
// (come media rinormalizzata delle normali delle facce adjacenti)
void Mesh::ComputeNormalsPerVertex()
{
    // uso solo le strutture di navigazione FV (da Faccia a Vertice)!

    // fase uno: ciclo sui vertici, azzero tutte le normali
    for (int i=0; i<v.size(); i++) {
        v[i].n = Point3(0,0,0);
    }

    // fase due: ciclo sulle facce: accumulo le normali di F nei 3 V corrispondenti
    for (int i=0; i<f.size(); i++) {
        f[i].v[0]->n=f[i].v[0]->n + f[i].n;
        f[i].v[1]->n=f[i].v[1]->n + f[i].n;
        f[i].v[2]->n=f[i].v[2]->n + f[i].n;
    }

    // fase tre: ciclo sui vertici; rinormalizzo
    // la normale media rinormalizzata e' uguale alla somma delle normnali, rinormalizzata
    for (int i=0; i<v.size(); i++) {
        v[i].n = v[i].n.Normalize();
    }

}

void Mesh::UpdateWorldCoords(Point3 p) {
    world_pos = p;
}

void Mesh::TranslateBBCoord(Point3 p)
{
  bbmax = bbmax + p;
  bbmin = bbmin + p;
}

void Mesh::ScaleBBCord(Point3 s)
{
  bbmax.coord[0] *= s.X(); bbmax.coord[1] *= s.Y(); bbmax.coord[2] *= s.Z();
  bbmin.coord[0] *= s.X(); bbmin.coord[1] *= s.Y(); bbmin.coord[2] *= s.Z();
}



Point3 Mesh::GetWorldCoords() {
    return world_pos;
}


// renderizzo la mesh in wireframe
void Mesh::RenderWire() {
    glLineWidth(1.0);
    // (nota: ogni edge viene disegnato due volte.
    // sarebbe meglio avere ed usare la struttura edge)
    for (int i=0; i<f.size(); i++)
    {
        glBegin(GL_LINE_LOOP);
        f[i].n.SendAsNormal();
        (f[i].v[0])->p.SendAsVertex();
        (f[i].v[1])->p.SendAsVertex();
        (f[i].v[2])->p.SendAsVertex();
        glEnd();
    }
}

void SetupSideTexture(Point3 min, Point3 max, int tex){

        glBindTexture(GL_TEXTURE_2D,tex);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_TEXTURE_GEN_S);
        glEnable(GL_TEXTURE_GEN_T);

        glTexGeni(GL_S, GL_TEXTURE_GEN_MODE , GL_OBJECT_LINEAR);
        glTexGeni(GL_T, GL_TEXTURE_GEN_MODE , GL_OBJECT_LINEAR);
        float sx=1.0/(max.X() - min.X());
        float ty=1.0/(max.Y() - min.Y());
        float s[4]={-sx,0,0, min.X()*sx };
        float t[4]={0,-ty,0, min.Y()*ty};
        glTexGenfv(GL_S, GL_OBJECT_PLANE, s);
        glTexGenfv(GL_T, GL_OBJECT_PLANE, t);

}

// Render usando la normale per faccia (FLAT SHADING)
void Mesh::RenderNxF()
{
    if (useWireframe) {
        glDisable(GL_TEXTURE_2D);
        glColor3f(.5,.5,.5);
        RenderWire();
        glColor3f(1,1,1);
    }

    // mandiamo tutti i triangoli a schermo
    glBegin(GL_TRIANGLES);
    for (int i=0; i<f.size(); i++)
    {

	f[i].n.SendAsNormal(); // flat shading
	
        (f[i].v[0])->p.SendAsVertex();
        (f[i].v[1])->p.SendAsVertex();
        (f[i].v[2])->p.SendAsVertex();
    }
    glEnd();
    
    
}


// Render usando la normale per vertice (GOURAUD SHADING)
void Mesh::RenderNxV(bool read_mtl)
{
    if (useWireframe) {
        glDisable(GL_TEXTURE_2D);
        glColor3f(.5,.5,.5);
        RenderWire();
        glColor3f(1,1,1);
    }
    
    int current_mtl = -1;
    bool do_texture = false;

    // mandiamo tutti i triangoli a schermo
    
    for (int i=0; i<f.size(); i++)
    {
        
	if((f[i].mtl_index != current_mtl) && (f[i].mtl_index >= 0) && read_mtl){
	  
	  if(!do_texture){
	    glEnable(GL_TEXTURE_2D);
	    //glDisable(GL_LIGHTING);
	    do_texture = true;
	  }
	  
	  current_mtl = f[i].mtl_index;
	  /*
	  float diff[] = {materials.at(f[i].mtl_index).diffuses[0], // R
			  materials.at(f[i].mtl_index).diffuses[1],			// G
			  materials.at(f[i].mtl_index).diffuses[2],			// B
			  materials.at(f[i].mtl_index).alpha}; //alpha
	  float amb[] = {materials.at(f[i].mtl_index).ambience[0],
			  materials.at(f[i].mtl_index).ambience[1],
			  materials.at(f[i].mtl_index).ambience[2],
			  materials.at(f[i].mtl_index).alpha}; 
	  float spec[] = {materials.at(f[i].mtl_index).specular[0],
			  materials.at(f[i].mtl_index).specular[1],
			  materials.at(f[i].mtl_index).specular[2],
			  materials.at(f[i].mtl_index).alpha}; 
	  
	  glMaterialfv(GL_FRONT, GL_DIFFUSE, diff);
	  glMaterialfv(GL_FRONT, GL_AMBIENT, amb);
	  glMaterialfv(GL_FRONT, GL_SPECULAR, spec);
	  glMaterialf(GL_FRONT, GL_SHININESS, materials.at(f[i].mtl_index).ns);
	  
	  */
	  glBindTexture(GL_TEXTURE_2D, materials.at(f[i].mtl_index).texture);
	  
	  
	}
	glBegin(GL_TRIANGLES);
	(f[i].v[0])->n.SendAsNormal(); // gouroud shading (o phong?)
	if(f[i].mtl_index >= 0) glTexCoord2f(f[i].t_coord[0]->u, 1 - f[i].t_coord[0]->v);
        (f[i].v[0])->p.SendAsVertex();

        (f[i].v[1])->n.SendAsNormal();
	if(f[i].mtl_index >= 0) glTexCoord2f(f[i].t_coord[1]->u, 1 - f[i].t_coord[1]->v);
        (f[i].v[1])->p.SendAsVertex();

        (f[i].v[2])->n.SendAsNormal();
	if(f[i].mtl_index >= 0) glTexCoord2f(f[i].t_coord[2]->u, 1 - f[i].t_coord[2]->v);
        (f[i].v[2])->p.SendAsVertex();
	glEnd();
    }
    
    if(do_texture){
      glDisable(GL_TEXTURE_2D);
      //glEnable(GL_LIGHTING);
    }
    
    
}


// trova l'AXIS ALIGNED BOUNDIG BOX
void Mesh::ComputeBoundingBox() {
    // basta trovare la min x, y, e z, e la max x, y, e z di tutti i vertici
    // (nota: non e' necessario usare le facce: perche?)
    if (!v.size()) return;
    bbmin=bbmax=v[0].p;
    for (int i=0; i<v.size(); i++) {
        for (int k=0; k<3; k++) {
            if (bbmin.coord[k]>v[i].p.coord[k]) bbmin.coord[k]=v[i].p.coord[k];
            if (bbmax.coord[k]<v[i].p.coord[k]) bbmax.coord[k]=v[i].p.coord[k];
        }
    }
}

void Mesh::LoadMtl(char* filename) {

  
    char buf[256];


    // Open MTL file
    FILE* inMTL = fopen(filename, "rt");
    
    if(!inMTL)
    {
        printf("ERROR OPENING MTL FILE\n");
	return;
    }
    
    while(fscanf(inMTL, "%s", buf) != EOF) { //read the first word in the line
      switch(buf[0]) {
        case '#':               // comment
            // eat up rest of line
            fgets(buf, sizeof(buf), inMTL);
            break;
	case 'n':
	    // check if it's a new material "newmt"
	    if ( buf[1] == 'e'){ // we only need to read "ne" since is the only rule in mtl file that starts with those letters
		
	      m++; //increment the number of material
	      Material mtl;
	      materials.push_back(mtl);
	      
	      fscanf(inMTL, "%s", materials.at(m).name);
		
	    }
	    
	case 'N':
	  
	  if(buf[1] == 's')
	    fscanf(inMTL,"%f", &(materials.at(m).ns));
	  else if (buf[1] == 'i')
	    fscanf(inMTL,"%f", &(materials.at(m).ni));
	  
	  break;
	  
	case 'i':
	   fscanf(inMTL, "%f", &(materials.at(m).illum));
	   break;
	   
	case 'd':
	    fscanf(inMTL, "%f", &(materials.at(m).alpha));
	
	case 'K':
	    //check if is a "Diffuses" or a "Specular" description
	    if(buf[1] == 'd'){
		//Diffuses
		fscanf(inMTL, "%f", &(materials.at(m).diffuses[0])); //R
		fscanf(inMTL, "%f", &(materials.at(m).diffuses[1])); //G
		fscanf(inMTL, "%f", &(materials.at(m).diffuses[2])); //B		
		
	    }else if (buf[1] == 's'){
		//Specular
		fscanf(inMTL, "%f", &(materials.at(m).specular[0])); //R
		fscanf(inMTL, "%f", &(materials.at(m).specular[1])); //G
		fscanf(inMTL, "%f", &(materials.at(m).specular[2])); //B
		
	    }else if (buf[1] == 'a'){
		//Specular
		fscanf(inMTL, "%f", &(materials.at(m).ambience[0])); //R
		fscanf(inMTL, "%f", &(materials.at(m).ambience[1])); //G
		fscanf(inMTL, "%f", &(materials.at(m).ambience[2])); //B
		
	    }
	    
	case 'm':
	    if(buf[1] == 'a' && buf[2] == 'p'){ //NOTE in this case we just handle the map_Kd case
	      fscanf(inMTL, "%s", materials.at(m).full_filepath);
	    }
	default:
            // eat up rest of line
            fgets(buf, sizeof(buf), inMTL);
            break;
      }
    }    
    
    fclose(inMTL);
    
}

// carica la mesh da un file in formato Obj
//   Nota: nel file, possono essere presenti sia quads che tris
//   ma nella rappresentazione interna (classe Mesh) abbiamo solo tris.
//
bool Mesh::LoadFromObj(char* filename)
{

    FILE* file = fopen(filename, "rt"); // apriamo il file in lettura
    if (!file) return false;

    //make a first pass through the file to get a count of the number
    //of vertices, normals, texcoords & triangles
    char buf[128];
    int nv,nf,nt;
    float x,y,z;
    int va,vb,vc,vd;
    int na,nb,nc,nd,nn;
    int ta,tb,tc,td;
    
    int cur_material=-1;

    nv=0; // num. of vertexes
    nn=0; // num. of normals
    nf=0; // num. of faces
    nt=0; // num. of triangles
    while(fscanf(file, "%s", buf) != EOF) {
        switch(buf[0]) {
        case '#':               // comment
            // eat up rest of line
            fgets(buf, sizeof(buf), file);
            break;
        case 'v':               // v, vn, vt
            switch(buf[1]) {
            case '\0':          // vertex
                // eat up rest of line
                fgets(buf, sizeof(buf), file);
                nv++;
                break;
		
	    case 'n':
		// eat up rest of line
                fgets(buf, sizeof(buf), file);
		nn++;
		break;
		
            default:
                break;
            }
            break;
        case 'f':               // face
            fscanf(file, "%s", buf);
            // can be one of %d, %d//%d, %d/%d, %d/%d/%d %d//%d
            if (strstr(buf, "//")) {
                // v//n
                sscanf(buf, "%d//%d", &va, &na);
                fscanf(file, "%d//%d", &vb, &nb);
                fscanf(file, "%d//%d", &vc, &nc);
                nf++;
                nt++;
                while(fscanf(file, "%d//%d", &vd, &nd) > 0) {
                    nt++;
                }
            } else if (sscanf(buf, "%d/%d/%d", &va, &ta, &na) == 3) {
                // v/t/n
                fscanf(file, "%d/%d/%d", &vb, &tb, &nb);
                fscanf(file, "%d/%d/%d", &vc, &tc, &nc);
                nf++;
                nt++;
                while(fscanf(file, "%d/%d/%d", &vd, &td, &nd) > 0) {
                    nt++;
                }
            } else if (sscanf(buf, "%d/%d", &va, &ta) == 2) {
                // v/t
                fscanf(file, "%d/%d", &vb, &tb);
                fscanf(file, "%d/%d", &vc, &tc);
                nf++;
                nt++;
                while(fscanf(file, "%d/%d", &vd, &td) > 0) {
                    nt++;
                }
            } else {
                // v
                fscanf(file, "%d", &va);
                fscanf(file, "%d", &vb);
                nf++;
                nt++;
                while(fscanf(file, "%d", &vc) > 0) {
                    nt++;
                }
            }
            break;

        default:
            // eat up rest of line
            fgets(buf, sizeof(buf), file);
            break;
        }
    }

    //printf("dopo FirstPass nv=%d nf=%d nt=%d\n",nv,nf,nt);

    // allochiamo spazio per nv vertici
    v.resize(nv);
    normals.resize(nn);

    // rewind to beginning of file and read in the data this pass
    rewind(file);

    //on the second pass through the file, read all the data into the
    //allocated arrays

    nv = 0;
    nn = 0;
    nt = 0;
    while(fscanf(file, "%s", buf) != EOF) {
        switch(buf[0]) {
        case '#':               // comment
            // eat up rest of line
            fgets(buf, sizeof(buf), file);
            break;
        case 'v':               
            switch(buf[1]) {	
	      case '\0':          // v (vertex)
		  fscanf(file, "%f %f %f", &x, &y, &z);
		  v[nv].p = Point3( x,y,z );
		  nv++;
		  break;
		  
	      case 'n':		// vn (normals per vertex)
		  fscanf(file, "%f %f %f", &x, &y, &z);
		  normals[nn] = Point3(x,y,z);
		  nn++;
		  break;
		  
	      case 't':		//vt (texture coordinate)
		  tex_coord_t t_coord;
		  fscanf(file, "%f %f", &(t_coord.u), &(t_coord.v));
		  //t_coord.v = 1 - t_coord.v;
		  texture_coord.push_back(t_coord);
		  break;
		
	      default:
		  break;
            }
            break;
	    
	case 'u':
	  //use material
	  fscanf(file, "%s", buf);
	  
	  int i;
	  for(i=0; i<materials.size();i++){
	    if( strcmp(materials.at(i).name, buf) == 0){
	      cur_material = i;
	      break;
	    }
	  }
	  break;
	  
        case 'f':               // face
            fscanf(file, "%s", buf);
            // can be one of %d, %d//%d, %d/%d, %d/%d/%d %d//%d
            if (strstr(buf, "//")) {
                // v//n
                sscanf(buf, "%d//%d", &va, &na);
                fscanf(file, "%d//%d", &vb, &nb);
                fscanf(file, "%d//%d", &vc, &nc);
                va--;
                vb--;
                vc--;
                Face newface( &(v[va]), &(v[vc]), &(v[vb]), -1 ); // invoco il costruttore di faccia
                f.push_back( newface ); // inserico la nuova faccia in coda al vettore facce
                nt++;
                vb=vc;
                while(fscanf(file, "%d//%d", &vc, &nc) > 0) {
                    vc--;
                    Face newface( &(v[va]), &(v[vc]), &(v[vb]), -1 ); // invoco il costruttore di faccia
                    f.push_back( newface ); // inserico la nuova faccia in coda al vettore facce
                    nt++;
                    vb=vc;
                }
            } else if (sscanf(buf, "%d/%d/%d", &va, &ta, &na) == 3) {
                // v/t/n
                fscanf(file, "%d/%d/%d", &vb, &tb, &nb);
                fscanf(file, "%d/%d/%d", &vc, &tc, &nc);
                va--; vb--; vc--; //decrementiamo il valore perche sono 1-indexed nei file obj, mentre noi usiamo 0-indexed nei vettori
		//TODO OLTRE A VERTICI, SALVARE ANCHE NORMALI(?) E COORDINATE TEXTURE PER FACCIA		
		ta--; tb--; tc--;
		na--; nb--; nc--;
		Face newface( &(v[va]), &(v[vc]), &(v[vb]), &(texture_coord.at(ta)), &(texture_coord.at(tc)), &(texture_coord.at(tb)), cur_material ); // invoco il costruttore di faccia
                f.push_back( newface ); // inserico la nuova faccia in coda al vettore facce
                nt++;
                vb=vc;
                while(fscanf(file, "%d/%d/%d", &vc, &tc, &nc) > 0) {
                    vc--;
		    tc--;
		    nc--;
                    Face newface( &(v[va]), &(v[vc]), &(v[vb]), &(texture_coord.at(ta)), &(texture_coord.at(tc)), &(texture_coord.at(tb)), cur_material ); // invoco il costruttore di faccia
                    f.push_back( newface ); // inserico la nuova faccia in coda al vettore facce
                    nt++;
                    vb=vc;
                }
            } else if (sscanf(buf, "%d/%d", &va, &ta) == 2) {
                // v/t
                fscanf(file, "%d/%d", &va, &ta);
                fscanf(file, "%d/%d", &va, &ta);
                va--;
                vb--;
                vc--;
                Face newface( &(v[va]), &(v[vc]), &(v[vb]), cur_material ); // invoco il costruttore di faccia
                f.push_back( newface ); // inserico la nuova faccia in coda al vettore facce
                nt++;
                vb=vc;
                while(fscanf(file, "%d/%d", &vc, &tc) > 0) {
                    vc--;
                    Face newface( &(v[va]), &(v[vc]), &(v[vb]), cur_material ); // invoco il costruttore di faccia
                    f.push_back( newface ); // inserico la nuova faccia in coda al vettore facce
                    nt++;
                    vb=vc;
                }
            } else {
                // v
                sscanf(buf, "%d", &va);
                fscanf(file, "%d", &vb);
                fscanf(file, "%d", &vc);
                va--;
                vb--;
                vc--;
                Face newface( &(v[va]), &(v[vc]), &(v[vb]), -1 ); // invoco il costruttore di faccia
                f.push_back( newface ); // inserico la nuova faccia in coda al vettore facce
                nt++;
                vb=vc;
                while(fscanf(file, "%d", &vc) > 0) {
                    vc--;
                    Face newface( &(v[va]), &(v[vc]), &(v[vb]), -1 ); // invoco il costruttore di faccia
                    f.push_back( newface ); // inserico la nuova faccia in coda al vettore facce
                    nt++;
                    vb=vc;
                }
            }
            break;

        default:
            // eat up rest of line
            fgets(buf, sizeof(buf), file);
            break;
        }
    }

//printf("dopo SecondPass nv=%d nt=%d\n",nv,nt);

    fclose(file);
    return true;
}
