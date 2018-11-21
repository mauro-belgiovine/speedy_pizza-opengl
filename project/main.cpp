#include <math.h>
#include <stdlib.h>
#include <time.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/freeglut.h>
#include <GL/glut.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "car.h"

#define ORANGE 1
#define BLACK 2
#define WHITE 3

#define CAMERA_BACK_CAR 0
#define CAMERA_TOP_FIXED 1
#define CAMERA_TOP_CAR 2
#define CAMERA_PILOT 3
#define CAMERA_MOUSE 4
#define CAMERA_TYPE_MAX 5

#define GAME_DURATION_S 120

float viewAlpha=20, viewBeta=40; // angoli che definiscono la vista
float eyeDist=5.0; // distanza dell'occhio dall'origine
int scrH=750, scrW=750; // altezza e larghezza viewport (in pixels)
bool useWireframe=false;
bool useEnvmap=true;
bool useHeadlight=false;
bool useShadow=true;
int cameraType=0;

std::vector<Material> materials;

extern Mesh carlinga;

//Mesh pista((char *)"pista.obj", false);

#define N_BUILDINGS 250
#define N_B_MODELS 4
#define N_CELLS 25
#define HALF_FLOOR_LEN 100.0f
#define SIZE_CELL ((-HALF_FLOOR_LEN + 2*(1)*HALF_FLOOR_LEN/N_CELLS) - (-HALF_FLOOR_LEN )  )
#define space_bin SIZE_CELL

Mesh b_med((char *) "buildings/building_med.obj", true, Vector3(1.2, 1.2, 1.2));
Mesh b_short((char *) "buildings/building_short.obj", true, Vector3(1.2, 1.2, 1.2));
Mesh b_tall((char *) "buildings/building_tall.obj", true, Vector3(1.2, 1.2, 1.2));
Mesh b_tall_pyr((char *) "buildings/building_tall_pyr.obj", true, Vector3(1.2, 1.2, 1.2));

#define BILLBOARD_ID 7777
Mesh billboard((char *) "buildings/billboard.obj", true);
#define PIZZA_ID 9999
Mesh pizza((char *) "pizza/pizza2.obj", true);
#define MAN_ID 8888
Mesh man((char *) "man/man2.obj",true, Vector3(0.2, 0.2, 0.2));
#define NOS_ID 6666
Mesh nos((char *) "nos/nos.obj", false);
#define NOS_ID2 6667
Mesh nos2((char *) "nos/nos.obj", false);
#define NOS_ID3 6668
Mesh nos3((char *) "nos/nos.obj", false);


float rotationAngle = 0.0f;
bool pizza_taken = false;
bool nos_active = false;
bool nos_redrawn = true;
int nos_to_redraw = -1;
unsigned int n_deliver = 0;

double center = ceil(N_CELLS/2.0f)-1; // define which is the center r,c coordinate

std::vector<Mesh> buildings;
short int position_matrix[N_CELLS][N_CELLS];

int i_bind_texture = 0; // NOTE: since indexes 0..3 are used in main.cpp
			// for other textures, here we start counting from 2 when
			// initializing the other ones for objects' materials
			
			
bool debug = true;
time_t start_time;
time_t end_time;
time_t close_time = 0;

bool done=false;

#define SKY_FRONT 0
#define SKY_BACK 1
#define SKY_LEFT 2
#define SKY_RIGHT 3
#define SKY_TOP 4
#define SKY_BOTTOM 5
int skybox[6];


Car car; // la nostra macchina
int nstep=0; // numero di passi di FISICA fatti fin'ora
const int PHYS_SAMPLING_STEP=10; // numero di millisec che un passo di fisica simula

// Frames Per Seconds
const int fpsSampling = 3000; // lunghezza intervallo di calcolo fps
float fps=0; // valore di fps dell'intervallo precedente
int fpsNow=0; // quanti fotogrammi ho disegnato fin'ora nell'intervallo attuale
Uint32 timeLastInterval=0; // quando e' cominciato l'ultimo intervallo

// funzione per stampare a video stringhe
void printString(float x, float y, int mycolor, char *string, void *font){
          glMatrixMode(GL_PROJECTION);
          glPushMatrix();
          glLoadIdentity();
          glOrtho(10.0, 0.0, 60.0, 0.0, -1.0f, 1.0f);
          glMatrixMode(GL_MODELVIEW);
          glLoadIdentity();

          glDisable(GL_LIGHTING);
	  switch(mycolor){
	    case ORANGE:
	      glColor3f(0.964, 0.521, 0.117);
	      break;
	    case BLACK:
	      glColor3f(0.1, 0.1, 0.1);
	      break;
	    case WHITE:
	      glColor3f(0.95, 0.95, 0.95);
	      break;
	    default:
	      glColor3f(0.95, 0.95, 0.95);
	  }

          glRasterPos2f(x, y);
          glutBitmapString(font, (const unsigned char *)string);

          glEnable(GL_LIGHTING);
          glPopAttrib();
          glPopMatrix();

}

// setta le matrici di trasformazione in modo
// che le coordinate in spazio oggetto siano le coord
// del pixel sullo schemo
void  SetCoordToPixel() {
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glTranslatef(-1,-1,0);
    glScalef(2.0/scrW, 2.0/scrH, 1);
}

bool LoadTexture(int textbind,char *filename) {
    SDL_Surface *s = IMG_Load(filename);
    if (!s) return false;

    glBindTexture(GL_TEXTURE_2D, textbind);
    gluBuild2DMipmaps(
        GL_TEXTURE_2D,
        GL_RGB,
        s->w, s->h,
        GL_RGB,
        GL_UNSIGNED_BYTE,
        s->pixels
    );
    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MAG_FILTER,
        GL_LINEAR );
    glTexParameteri(
        GL_TEXTURE_2D,
        GL_TEXTURE_MIN_FILTER,
        GL_LINEAR_MIPMAP_LINEAR );
    return true;
}

// load all textures
void LoadTextures() // NOTE: it HAS to be called after OpenGL has been initialized
{
  
  for(int i = 0; i < materials.size(); i++){
    i_bind_texture++;
    LoadTexture(i_bind_texture, (char *) materials.at(i).full_filepath);
    materials.at(i).texture = i_bind_texture;
  }  
}

void drawPista () {
    glPushMatrix();
    glColor3f(0.4,0.4,.8);
    glScalef(0.75, 1.0, 0.75);
    glTranslatef(0,0.01,0);
    //pista.RenderNxF();
    glPopMatrix();

    glColor3f(1,1,1);
    
    for(int r = 0; r < N_CELLS; r++){
      for(int c = 0; c < N_CELLS; c++){
	if (position_matrix[r][c] > -1){
	  
	  glPushMatrix();
	  Point3 translation((r-center)*space_bin, 0, (c-center)*space_bin);
	  glTranslate(translation);
	  
	  if((position_matrix[r][c] == PIZZA_ID) && !done){
	    
	    if(!pizza_taken){ // se la pizza non è stata ancora presa, la disegno

	      rotationAngle += 0.5;
	      if(rotationAngle == 360.0) rotationAngle = 0.0f;
	      glRotatef(rotationAngle, 0.0f, 1.0f, 0.0f);
	      pizza.RenderNxV(true);
	    }else{
	    	
	      //altrimenti sì ed elimino il suo id dalla matrice delle posizioni
	      int r_old = r;
	      int c_old = c;
	      
	      do{
		  r = rand() % N_CELLS;
		  c = rand() % N_CELLS;
	      }while((position_matrix[r][c] != -1));
	      
	      position_matrix[r][c] = MAN_ID; //posiziono il segnaposto per la consegna e aggiorno la sua posizione nel mondo
	      
	      position_matrix[r_old][c_old] = -1;
	      
	      Point3 translation((r-center)*space_bin, 0, (c-center)*space_bin);
	      man.ComputeBoundingBox();
	      man.UpdateWorldCoords(translation);
	      man.TranslateBBCoord(translation);
	      
	    }
	    
	  }else if((position_matrix[r][c] == MAN_ID) && !done){
	    if(pizza_taken){ //se la pizza è stata presa, disegno l'uomo
	      glScalef(man.mesh_scale.X(), man.mesh_scale.Y(), man.mesh_scale.Z());
	      man.RenderNxV(true);
	    }else{ //altrimenti è stata consegnata, quindi bisogna eliminare l'uomo e disegnare la pizza
	      
	      n_deliver++; //incremento il numero di consegne
	      
	      //elimino il suo id dalla matrice delle posizioni
	      int r_old = r;
	      int c_old = c;
	      
	      do{
		  r = rand() % N_CELLS;
		  c = rand() % N_CELLS;
	      }while((position_matrix[r][c] != -1));
	      
	      position_matrix[r][c] = PIZZA_ID; //posiziono il segnaposto per la consegna e aggiorno la sua posizione nel mondo
	      
	      position_matrix[r_old][c_old] = -1;
	      
	      Point3 translation((r-center)*space_bin, 0, (c-center)*space_bin);
	      pizza.ComputeBoundingBox();
	      pizza.UpdateWorldCoords(translation);
	      pizza.TranslateBBCoord(translation);
	      
	    }
	      
	  }else if((position_matrix[r][c] == BILLBOARD_ID) && !done){
	    glRotatef(180.0f, 0.0f, 1.0f, 0.0f);
	    billboard.RenderNxV(true);
	    
	  }else if(position_matrix[r][c] == NOS_ID || position_matrix[r][c] == NOS_ID2 || position_matrix[r][c] == NOS_ID3){
	    
	    Mesh *this_nos;
	    int nos_id;
	    
	    if(position_matrix[r][c] == NOS_ID){
	      this_nos = &nos;
	      nos_id = NOS_ID;
	    }else if(position_matrix[r][c] == NOS_ID2){
	      this_nos = &nos2;
	      nos_id = NOS_ID2;
	    }else if(position_matrix[r][c] == NOS_ID3){
	      this_nos = &nos3;
	      nos_id = NOS_ID3;
	    }

	    // facciamo binding con la texture 1
	    glBindTexture(GL_TEXTURE_2D,1);
	    glEnable(GL_TEXTURE_2D);
	    glEnable(GL_TEXTURE_GEN_S); // abilito la generazione automatica delle coord texture S e T
	    glEnable(GL_TEXTURE_GEN_T);
	    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE , GL_SPHERE_MAP); // Env map
	    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE , GL_SPHERE_MAP);
	    glColor3f(1,1,1); // metto il colore neutro (viene moltiplicato col colore texture, componente per componente)
	    glDisable(GL_LIGHTING); // disabilito il lighting OpenGL standard (lo faccio con la texture)
	    
	    glScalef(this_nos->mesh_scale.X(), this_nos->mesh_scale.Y(), this_nos->mesh_scale.Z());
	    glRotatef(rotationAngle, 0.0f, 1.0f, 0.0f);
	    this_nos->RenderNxV(false);
	    
	    glDisable(GL_TEXTURE_2D);
	    glDisable(GL_TEXTURE_GEN_S); // abilito la generazione automatica delle coord texture S e T
	    glDisable(GL_TEXTURE_GEN_T);
	    glEnable(GL_LIGHTING);
	    
	    if(nos_active && !nos_redrawn && (nos_to_redraw == nos_id)){
	      nos_to_redraw = -1;
	      //elimino il suo id dalla matrice delle posizioni
	      int r_old = r;
	      int c_old = c;
	      do{
		  r = rand() % N_CELLS;
		  c = rand() % N_CELLS;
	      }while((position_matrix[r][c] != -1));
	      
	      position_matrix[r][c] = nos_id; //posiziono il segnaposto per la consegna e aggiorno la sua posizione nel mondo
	      
	      position_matrix[r_old][c_old] = -1;
	      
	      Point3 translation((r-center)*space_bin, 0, (c-center)*space_bin);
	      this_nos->ComputeBoundingBox();
	      this_nos->UpdateWorldCoords(translation);
	      this_nos->TranslateBBCoord(translation);
	      
	      nos_redrawn = true;
	      
	    }
	    
	    
	  }else{
	    glScalef(buildings.at(position_matrix[r][c]).mesh_scale.X(), buildings.at(position_matrix[r][c]).mesh_scale.Y(), buildings.at(position_matrix[r][c]).mesh_scale.Z());
	    buildings.at(position_matrix[r][c]).RenderNxV(true);
	  }
	  glPopMatrix();


	}
	
      }
    }
    
    
    
	
}


// disegna gli assi nel sist. di riferimento
void drawAxis() {
    const float K=0.10;
    glColor3f(0,0,1);
    glBegin(GL_LINES);
    glVertex3f( -1,0,0 );
    glVertex3f( +1,0,0 );

    glVertex3f( 0,-1,0 );
    glVertex3f( 0,+1,0 );

    glVertex3f( 0,0,-1 );
    glVertex3f( 0,0,+1 );
    glEnd();

    glBegin(GL_TRIANGLES);
    glVertex3f( 0,+1  ,0 );
    glVertex3f( K,+1-K,0 );
    glVertex3f(-K,+1-K,0 );

    glVertex3f( +1,   0, 0 );
    glVertex3f( +1-K,+K, 0 );
    glVertex3f( +1-K,-K, 0 );

    glVertex3f( 0, 0,+1 );
    glVertex3f( 0,+K,+1-K );
    glVertex3f( 0,-K,+1-K );
    glEnd();

}


//vecchio codice ora commentato
// disegna un cubo rasterizzando quads
void drawCubeFill()
{
const float S=100;

  glBegin(GL_QUADS);

    glNormal3f(  0,0,+1  );
    glVertex3f( +S,+S,+S );
    glVertex3f( -S,+S,+S );
    glVertex3f( -S,-S,+S );
    glVertex3f( +S,-S,+S );

    glNormal3f(  0,0,-1  );
    glVertex3f( +S,-S,-S );
    glVertex3f( -S,-S,-S );
    glVertex3f( -S,+S,-S );
    glVertex3f( +S,+S,-S );

    glNormal3f(  0,+1,0  );
    glVertex3f( +S,+S,+S );
    glVertex3f( -S,+S,+S );
    glVertex3f( -S,+S,-S );
    glVertex3f( +S,+S,-S );

    glNormal3f(  0,-1,0  );
    glVertex3f( +S,-S,-S );
    glVertex3f( -S,-S,-S );
    glVertex3f( -S,-S,+S );
    glVertex3f( +S,-S,+S );

    glNormal3f( +1,0,0  );
    glVertex3f( +S,+S,+S );
    glVertex3f( +S,-S,+S );
    glVertex3f( +S,-S,-S );
    glVertex3f( +S,+S,-S );

    glNormal3f( -1,0,0  );
    glVertex3f( -S,+S,-S );
    glVertex3f( -S,-S,-S );
    glVertex3f( -S,-S,+S );
    glVertex3f( -S,+S,+S );

  glEnd();
}

// disegna un cubo in wireframe
void drawCubeWire()
{
  glBegin(GL_LINE_LOOP); // faccia z=+1
    glVertex3f( +1,+1,+1 );
    glVertex3f( -1,+1,+1 );
    glVertex3f( -1,-1,+1 );
    glVertex3f( +1,-1,+1 );
  glEnd();

  glBegin(GL_LINE_LOOP); // faccia z=-1
    glVertex3f( +1,-1,-1 );
    glVertex3f( -1,-1,-1 );
    glVertex3f( -1,+1,-1 );
    glVertex3f( +1,+1,-1 );
  glEnd();

  glBegin(GL_LINES);  // 4 segmenti da -z a +z
    glVertex3f( -1,-1,-1 );
    glVertex3f( -1,-1,+1 );

    glVertex3f( +1,-1,-1 );
    glVertex3f( +1,-1,+1 );

    glVertex3f( +1,+1,-1 );
    glVertex3f( +1,+1,+1 );

    glVertex3f( -1,+1,-1 );
    glVertex3f( -1,+1,+1 );
  glEnd();
}

void drawCube()
{
  glColor3f(.9,.9,.9);
  drawCubeFill();
  //glColor3f(0,0,0);
  //drawCubeWire();
}


void drawSphere(double r, int lats, int longs) {
    int i, j;
    for(i = 0; i <= lats; i++) {
        double lat0 = M_PI * (-0.5 + (double) (i - 1) / lats);
        double z0  = sin(lat0);
        double zr0 =  cos(lat0);

        double lat1 = M_PI * (-0.5 + (double) i / lats);
        double z1 = sin(lat1);
        double zr1 = cos(lat1);

        glBegin(GL_QUAD_STRIP);
        for(j = 0; j <= longs; j++) {
            double lng = 2 * M_PI * (double) (j - 1) / longs;
            double x = cos(lng);
            double y = sin(lng);

//le normali servono per l'EnvMap
            glNormal3f(x * zr0, y * zr0, z0);
            glVertex3f(r * x * zr0, r * y * zr0, r * z0);
            glNormal3f(x * zr1, y * zr1, z1);
            glVertex3f(r * x * zr1, r * y * zr1, r * z1);
        }
        glEnd();
    }
}

void drawFloor()
{
    const float S=HALF_FLOOR_LEN; // each edge of the square floor has length 200 (from -100 to 100)
    const float H=0;   // altezza
    const int K=N_CELLS; //disegna K x K quads


    // disegna KxK quads
    
    glColor3f(0.6, 0.6, 0.6); // colore uguale x tutti i quads
    glNormal3f(0,1,0);       // normale verticale uguale x tutti
    for (int x=0; x<K; x++)
        for (int z=0; z<K; z++) {
	  
	  if((position_matrix[x][z] > -1) && (position_matrix[x][z] < N_BUILDINGS))
	    glBindTexture(GL_TEXTURE_2D,3);
	  else
	    glBindTexture(GL_TEXTURE_2D,4);
	  
	  glEnable(GL_TEXTURE_2D);
	  
	  glBegin(GL_QUADS);
	  
	  float x0=-S + 2*(x+0)*S/K;
	  float x1=-S + 2*(x+1)*S/K;
	  float z0=-S + 2*(z+0)*S/K;
	  float z1=-S + 2*(z+1)*S/K;
	    
	  glTexCoord2d(0.0f, 0.0f);
	  glVertex3d(x0, H, z0);
	  glTexCoord2d(10.0f, 0.0f);
	  glVertex3d(x1, H, z0);
	  glTexCoord2d(10.0f, 10.0f);
	  glVertex3d(x1, H, z1);
	  glTexCoord2d(0.0f, 10.0f);
	  glVertex3d(x0, H, z1);
	  
	  glEnd();

        }
    
    
    glDisable(GL_TEXTURE_2D);

}

// setto la posizione della camera
void setCamera() {

    double px = car.px;
    double py = car.py;
    double pz = car.pz;
    double angle = car.facing;
    double cosf = cos(angle*M_PI/180.0);
    double sinf = sin(angle*M_PI/180.0);
    double camd, camh, ex, ey, ez, cx, cy, cz;
    double cosff, sinff;

// controllo la posizione della camera a seconda dell'opzione selezionata
    switch (cameraType) {
    case CAMERA_BACK_CAR:
        camd = 2.3;
        camh = 1.2;
        ex = px + camd*sinf;
        ey = py + camh;
        ez = pz + camd*cosf;
        cx = px - camd*sinf;
        cy = py + camh;
        cz = pz - camd*cosf;
        gluLookAt(ex,ey,ez,cx,cy,cz,0.0,1.0,0.0);
        break;
    case CAMERA_TOP_FIXED:
        camd = 0.5;
        camh = 0.55;
        angle = car.facing + 40.0;
        cosff = cos(angle*M_PI/180.0);
        sinff = sin(angle*M_PI/180.0);
        ex = px + camd*sinff;
        ey = py + camh;
        ez = pz + camd*cosff;
        cx = px - camd*sinf;
        cy = py + camh;
        cz = pz - camd*cosf;
        gluLookAt(ex,ey,ez,cx,cy,cz,0.0,1.0,0.0);
        break;
    case CAMERA_TOP_CAR:
        camd = 2.5;
        camh = 1.0;
        ex = px + camd*sinf;
        ey = py + camh;
        ez = pz + camd*cosf;
        cx = px - camd*sinf;
        cy = py + camh;
        cz = pz - camd*cosf;
        gluLookAt(ex,ey+5,ez,cx,cy,cz,0.0,1.0,0.0);
        break;
    case CAMERA_PILOT:
        camd = 0.2;
        camh = 0.55;
        ex = px + camd*sinf;
        ey = py + camh;
        ez = pz + camd*cosf;
        cx = px - camd*sinf;
        cy = py + camh;
        cz = pz - camd*cosf;
        gluLookAt(ex,ey,ez,cx,cy,cz,0.0,1.0,0.0);
        break;
    case CAMERA_MOUSE:
        glTranslatef(0,0,-eyeDist);
        glRotatef(viewBeta,  1,0,0);
        glRotatef(viewAlpha, 0,1,0);
        /*
        printf("%f %f %f\n",viewAlpha,viewBeta,eyeDist);
                        ex=eyeDist*cos(viewAlpha)*sin(viewBeta);
                        ey=eyeDist*sin(viewAlpha)*sin(viewBeta);
                        ez=eyeDist*cos(viewBeta);
                        cx = px - camd*sinf;
                        cy = py + camh;
                        cz = pz - camd*cosf;
                        gluLookAt(ex,ey,ez,cx,cy,cz,0.0,1.0,0.0);
        */
        break;
    }
}

void drawSky() {
    float H = 140.0f;

    if (useWireframe) {
        glDisable(GL_TEXTURE_2D);
        glColor3f(0,0,0);
        glDisable(GL_LIGHTING);
        glPolygonMode(GL_FRONT_AND_BACK,GL_LINE);
        drawSphere(H, 20, 20);
	//drawCube();
        glPolygonMode(GL_FRONT_AND_BACK,GL_FILL);
        glColor3f(1,1,1);
        glEnable(GL_LIGHTING);
    }
    else
    {

	glBindTexture(GL_TEXTURE_2D,2);
        glEnable(GL_TEXTURE_2D);
        glEnable(GL_TEXTURE_GEN_S);
        glEnable(GL_TEXTURE_GEN_T);
        glTexGeni(GL_S, GL_TEXTURE_GEN_MODE , GL_SPHERE_MAP); // Env map
        glTexGeni(GL_T, GL_TEXTURE_GEN_MODE , GL_SPHERE_MAP);
        glColor3f(1,1,1);
        glDisable(GL_LIGHTING);

        //   drawCubeFill();
        drawSphere(H, 20, 20);
	//drawCube();
	
        glDisable(GL_TEXTURE_GEN_S);
        glDisable(GL_TEXTURE_GEN_T);
        glDisable(GL_TEXTURE_2D);
        glEnable(GL_LIGHTING);

    }

}



/* Esegue il Rendering della scena */
void rendering(SDL_Window *win) {

    // un frame in piu'!!!
    fpsNow++;

    glLineWidth(3); // linee larghe

    // settiamo il viewport
    glViewport(0,0, scrW, scrH);

    // colore sfondo = bianco
    glClearColor(1,1,1,1);


    // settiamo la matrice di proiezione
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();
    gluPerspective( 70, //fovy,
                    ((float)scrW) / scrH,//aspect Y/X,
                    0.2,//distanza del NEAR CLIPPING PLANE in coordinate vista
                    1000  //distanza del FAR CLIPPING PLANE in coordinate vista
                  );

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

    // riempe tutto lo screen buffer di pixel color sfondo
    glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    //drawAxis(); // disegna assi frame VISTA
    
    glShadeModel(GL_SMOOTH);

    // setto la posizione luce
    float tmpv[4] = {0,1,2,  0}; // ultima comp=0 => luce direzionale
    glLightfv(GL_LIGHT0, GL_POSITION, tmpv );


    // settiamo matrice di vista
    //glTranslatef(0,0,-eyeDist);
    //glRotatef(viewBeta,  1,0,0);
    //glRotatef(viewAlpha, 0,1,0);
    setCamera();


    //drawAxis(); // disegna assi frame MONDO

    static float tmpcol[4] = {1,1,1,  1};
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, tmpcol);
    glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 127);

    glEnable(GL_LIGHTING);

    // settiamo matrice di modellazione
    //drawAxis(); // disegna assi frame OGGETTO
    //drawCubeWire();

    drawSky(); // disegna il cielo come sfondo
    //drawSkybox(HALF_FLOOR_LEN*2);
    drawFloor(); // disegna il suolo
    
    car.Render(); // disegna la macchina
    
    drawPista(); // disegna la pista
    
    
    // PANNELLO DI CONTROLLO
    char buf[50];
    
    time_t current_time = time(NULL);
    if((end_time - current_time) > 0){
      sprintf(buf, "Consegne: %d", n_deliver);
      printString(9.8, 2, WHITE, buf, GLUT_BITMAP_HELVETICA_18);
      unsigned int mins = (end_time - current_time)/60;
      unsigned int secs = (end_time - current_time)%60;
      sprintf(buf, "Tempo: %02d:%02d", mins, secs);
      printString(9.8, 4, WHITE, buf, GLUT_BITMAP_HELVETICA_18);
      if(nos_active) {
	extern time_t start_nos;
	time_t end_nos = start_nos + 5;
	mins = (end_nos - current_time)/60;
	secs = (end_nos - current_time)%60;
	sprintf(buf, "Nos: %02d:%02d", mins, secs);
	printString(9.8, 8, WHITE, buf, GLUT_BITMAP_HELVETICA_18);
	
      }
      
      if(pizza_taken){
	printString(9.8, 6, WHITE, "Consegna la pizza!", GLUT_BITMAP_HELVETICA_18);
      }
      
    }else{
      sprintf(buf, "Consegne: %d", n_deliver);
      printString(4.8, 27, WHITE, buf, GLUT_BITMAP_TIMES_ROMAN_24);
      printString(7, 27, ORANGE, "GAME OVER!", GLUT_BITMAP_TIMES_ROMAN_24);
      if(close_time == 0) close_time = time(NULL) + 5;
      
    }
    
    if((close_time > 0) && ((close_time - current_time) <= 0)){
      done = true; // fermiamo il programma dopo 5 secondi
    }
    
    
    // attendiamo la fine della rasterizzazione di
    // tutte le primitive mandate

    glDisable(GL_DEPTH_TEST);
    glDisable(GL_LIGHTING);

// disegnamo i fps (frame x sec) come una barra a sinistra.
// (vuota = 0 fps, piena = 100 fps)
    SetCoordToPixel();

    glBegin(GL_QUADS);
    float y=scrH*fps/100;
    float ramp=fps/100;
    glColor3f(1-ramp,0,ramp);
    glVertex2d(10,0);
    glVertex2d(10,y);
    glVertex2d(0,y);
    glVertex2d(0,0);
    glEnd();
    
    
    
    
    //MINI MAPPA 
    glBegin(GL_QUADS);
    glColor3f(0.4,0.4,0.4);
    int margin = 20; //margin from the window's edge
    
    Point3 bl = Point3( scrW-((N_CELLS*SIZE_CELL))-margin , margin , 0); //bottom right vertex
    Point3 br = Point3(scrW-margin , margin, 0);	//bottom left
    Point3 tl = Point3(scrW-((N_CELLS*SIZE_CELL))-margin , ((N_CELLS*SIZE_CELL))+margin, 0); //top left
    Point3 tr = Point3(scrW-margin, ((N_CELLS*SIZE_CELL))+margin, 0);

    glVertex2d(bl.X(), bl.Y());
    glVertex2d(br.X(), br.Y());
    glVertex2d(tr.X(), tr.Y());
    glVertex2d(tl.X(),tl.Y());
    
    glEnd();
    
    glColor3f(1,0,0);
    
    glPointSize(2);
    glBegin(GL_POINTS);
    glVertex2d(br.X()-(((N_CELLS/2.0f)*SIZE_CELL)) + car.getWorldCoords().X(), br.Y()+(((N_CELLS/2.0f)*SIZE_CELL)) - car.getWorldCoords().Z() );
    glEnd();
    
    glPointSize(4);
    glBegin(GL_POINTS);
    if(!pizza_taken){
      glColor3f(0,1,0);
      glVertex2d(br.X()-(((N_CELLS/2.0f)*SIZE_CELL)) + pizza.GetWorldCoords().X(), br.Y()+(((N_CELLS/2.0f)*SIZE_CELL)) - pizza.GetWorldCoords().Z() );
    }else{
      glColor3f(0,0,1);
      glVertex2d(br.X()-(((N_CELLS/2.0f)*SIZE_CELL)) + man.GetWorldCoords().X(), br.Y()+(((N_CELLS/2.0f)*SIZE_CELL)) - man.GetWorldCoords().Z() );
    }
    glColor3f(1,1,0);
    glVertex2d(br.X()-(((N_CELLS/2.0f)*SIZE_CELL)) + nos.GetWorldCoords().X(), br.Y()+(((N_CELLS/2.0f)*SIZE_CELL)) - nos.GetWorldCoords().Z() );
    glVertex2d(br.X()-(((N_CELLS/2.0f)*SIZE_CELL)) + nos2.GetWorldCoords().X(), br.Y()+(((N_CELLS/2.0f)*SIZE_CELL)) - nos2.GetWorldCoords().Z() );
    glVertex2d(br.X()-(((N_CELLS/2.0f)*SIZE_CELL)) + nos3.GetWorldCoords().X(), br.Y()+(((N_CELLS/2.0f)*SIZE_CELL)) - nos3.GetWorldCoords().Z() );
    glEnd();
    
    glBegin(GL_QUADS);
    glColor3f(0.8,0.8,0.8);
    for(int i = 0; i < buildings.size(); i++){
      glVertex2d(br.X()-(((N_CELLS/2.0f)*SIZE_CELL)) + buildings.at(i).bbmin.X(), br.Y()+(((N_CELLS/2.0f)*SIZE_CELL)) - buildings.at(i).bbmin.Z()  );
      glVertex2d(br.X()-(((N_CELLS/2.0f)*SIZE_CELL)) + buildings.at(i).bbmax.X(), br.Y()+(((N_CELLS/2.0f)*SIZE_CELL)) - buildings.at(i).bbmin.Z()  );
      glVertex2d(br.X()-(((N_CELLS/2.0f)*SIZE_CELL)) + buildings.at(i).bbmax.X(), br.Y()+(((N_CELLS/2.0f)*SIZE_CELL)) - buildings.at(i).bbmax.Z()  );
      glVertex2d(br.X()-(((N_CELLS/2.0f)*SIZE_CELL)) + buildings.at(i).bbmin.X(), br.Y()+(((N_CELLS/2.0f)*SIZE_CELL)) - buildings.at(i).bbmax.Z()  );
    }
    
    glEnd();

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);


    glFinish();
    // ho finito: buffer di lavoro diventa visibile
    SDL_GL_SwapWindow(win);
}

void redraw() {
    // ci automandiamo un messaggio che (s.o. permettendo)
    // ci fara' ridisegnare la finestra
    SDL_Event e;
    e.type=SDL_WINDOWEVENT;
    e.window.event=SDL_WINDOWEVENT_EXPOSED;
    SDL_PushEvent(&e);
}

void define_obj_position(int id, Mesh *m){
  
  int r, c;
  do{
	r = rand() % N_CELLS;
	c = rand() % N_CELLS;
    }while((position_matrix[r][c] != -1) ||
      ((r >= center-1) && (r <= center+1)) && ((c >= center-1) && (c <= center+1)) );
    
    position_matrix[r][c] = id;
    
    Point3 translation((r-(N_CELLS/2))*space_bin, 0, (c-(N_CELLS/2))*space_bin);
    m->UpdateWorldCoords(translation);
    m->TranslateBBCoord(translation);
}


int main(int argc, char* argv[])
{
  
    
    
    SDL_Window *win;
    SDL_GLContext mainContext;
    Uint32 windowID;
    SDL_Joystick *joystick;
    static int keymap[Controller::NKEYS] = {SDLK_a, SDLK_d, SDLK_w, SDLK_s};

    // inizializzazione di SDL
    SDL_Init( SDL_INIT_VIDEO | SDL_INIT_JOYSTICK);
    
    glutInit(&argc, argv);

    SDL_JoystickEventState(SDL_ENABLE);
    joystick = SDL_JoystickOpen(0);

    SDL_GL_SetAttribute( SDL_GL_DEPTH_SIZE, 16 );
    SDL_GL_SetAttribute( SDL_GL_DOUBLEBUFFER, 1 );

    // facciamo una finestra di scrW x scrH pixels
    win=SDL_CreateWindow(argv[0], 0, 0, scrW, scrH, SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE);

    //Create our opengl context and attach it to our window
    mainContext=SDL_GL_CreateContext(win);

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    glEnable(GL_NORMALIZE); // opengl, per favore, rinormalizza le normali
    // prima di usarle
    //glEnable(GL_CULL_FACE);
    glFrontFace(GL_CW); // consideriamo Front Facing le facce ClockWise
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);
    glEnable(GL_POLYGON_OFFSET_FILL); // caro openGL sposta i
    // frammenti generati dalla
    // rasterizzazione poligoni
    glPolygonOffset(1,1);             // indietro di 1

    if (!LoadTexture(i_bind_texture++,(char *)"textures/TexturesCom_Rubber0027_1_S.jpg")) return -1;			// 0 
    if (!LoadTexture(i_bind_texture++,(char *)"envmap_flipped.jpg")) return -1;						// 1
    if (!LoadTexture(i_bind_texture++,(char *)"sky_ok.jpg")) return -1;							// 2
    if (!LoadTexture(i_bind_texture++,(char *)"textures/TexturesCom_FloorStreets0078_1_seamless_S.jpg")) return -1;	// 3
    if (!LoadTexture(i_bind_texture++,(char *)"textures/TexturesCom_ConcreteBare0348_1_seamless_S.jpg")) return -1;	// 4
    
    //load textures for other environment components
    LoadTextures();
    
    //generate the building vector

    srand((unsigned int) time(NULL));
    
    for(int i=0; i < N_BUILDINGS; i++){
      
      int i_b = rand() % N_B_MODELS;
      switch(i_b){
	case 0:
	  buildings.push_back(b_short);
	  break;
	case 1:
	  buildings.push_back(b_med);
	  break;
	case 2:
	  buildings.push_back(b_tall);
	  break;
	case 3:
	  buildings.push_back(b_tall_pyr);
	  break;
      }
    }
    
    //generate position of the buildings
    memset(position_matrix, -1, sizeof(short int)*N_CELLS*N_CELLS);
    
    
    int r,c;
    for(int i=0; i < N_BUILDINGS; i++){  
      //define_obj_position(i, &(buildings.at(i)));
      
      int r, c;
      do{
	  r = rand() % N_CELLS;
	  c = rand() % N_CELLS;
      }while((position_matrix[r][c] != -1) || //no cella occupata
	((r >= center-2) && (r <= center+2)) && ((c >= center-2) && (c <= center+2)) ||	//no al centro
	(((r == 0) || (c == 0) ) || ((r == N_CELLS-1) || (c == N_CELLS-1))) // no sui bordi
	
      );
      
      position_matrix[r][c] = i;
      
      Point3 translation((r-(N_CELLS/2))*space_bin, 0, (c-(N_CELLS/2))*space_bin);
      buildings.at(i).UpdateWorldCoords(translation);
      buildings.at(i).TranslateBBCoord(translation);
      
      i++;
      
      for(int y = -1; ((y < 2) && (i < N_BUILDINGS)) ; y++){
	
	for(int z = -1; ((z < 2) && (i < N_BUILDINGS)); z++){
	  
	  if((position_matrix[r+y][c+z] == -1) && !((y == 0) && (z == 0))){
	    position_matrix[r+y][c+z] = i;
	    Point3 t2(((r+y)-(N_CELLS/2))*space_bin, 0, ((c+z)-(N_CELLS/2))*space_bin);
	    buildings.at(i).UpdateWorldCoords(t2);
	    buildings.at(i).TranslateBBCoord(t2);
	    i++;
	  }
	}
	
      }
	  
      i--;
      
      
      
    }
    
    //set PIZZA position
    define_obj_position(PIZZA_ID, &pizza);
    
    //set BILLBOARD position
    define_obj_position(BILLBOARD_ID, &billboard);
    
    //set NOS positions
    define_obj_position(NOS_ID, &nos);
    define_obj_position(NOS_ID2, &nos2);
    define_obj_position(NOS_ID3, &nos3);
    
    end_time = time(NULL) + GAME_DURATION_S;

    
    while (!done) {

        SDL_Event e;

        // guardo se c'e' un evento:
        if (SDL_PollEvent(&e)) {
            // se si: processa evento
            switch (e.type) {
            case SDL_KEYDOWN:
                car.controller.EatKey(e.key.keysym.sym, keymap , true);
                if (e.key.keysym.sym==SDLK_F1) cameraType=(cameraType+1)%CAMERA_TYPE_MAX;
                if (e.key.keysym.sym==SDLK_F2) useWireframe=!useWireframe;
                if (e.key.keysym.sym==SDLK_F3) useEnvmap=!useEnvmap;
                if (e.key.keysym.sym==SDLK_F4) useHeadlight=!useHeadlight;
                if (e.key.keysym.sym==SDLK_F5) useShadow=!useShadow;
                break;
            case SDL_KEYUP:
                car.controller.EatKey(e.key.keysym.sym, keymap , false);
                break;
            case SDL_QUIT:
                done=1;
                break;
            case SDL_WINDOWEVENT:
                // dobbiamo ridisegnare la finestra
                if (e.window.event==SDL_WINDOWEVENT_EXPOSED)
                    rendering(win);
                else {
                    windowID = SDL_GetWindowID(win);
                    if (e.window.windowID == windowID)  {
                        switch (e.window.event)  {
                        case SDL_WINDOWEVENT_SIZE_CHANGED:  {
                            scrW = e.window.data1;
                            scrH = e.window.data2;
                            glViewport(0,0,scrW,scrH);
                            rendering(win);
                            //redraw(); // richiedi ridisegno
                            break;
                        }
                        }
                    }
                }
                break;

            case SDL_MOUSEMOTION:
                if (e.motion.state & SDL_BUTTON(1) & cameraType==CAMERA_MOUSE) {
                    viewAlpha+=e.motion.xrel;
                    viewBeta +=e.motion.yrel;
//          if (viewBeta<-90) viewBeta=-90;
                    if (viewBeta<+5) viewBeta=+5; //per non andare sotto la macchina
                    if (viewBeta>+90) viewBeta=+90;
                    // redraw(); // richiedi un ridisegno (non c'e' bisongo: si ridisegna gia'
                    // al ritmo delle computazioni fisiche)
                }
                break;

            case SDL_MOUSEWHEEL:
                if (e.wheel.y < 0 ) {
                    // avvicino il punto di vista (zoom in)
                    eyeDist=eyeDist*0.9;
                    if (eyeDist<1) eyeDist = 1;
                };
                if (e.wheel.y > 0 ) {
                    // allontano il punto di vista (zoom out)
                    eyeDist=eyeDist/0.9;
                };
                break;

            case SDL_JOYAXISMOTION: /* Handle Joystick Motion */
                if( e.jaxis.axis == 0)
                {
                    if ( e.jaxis.value < -3200  )
                    {
                        car.controller.Joy(0 , true);
                        car.controller.Joy(1 , false);
//	      printf("%d <-3200 \n",e.jaxis.value);
                    }
                    if ( e.jaxis.value > 3200  )
                    {
                        car.controller.Joy(0 , false);
                        car.controller.Joy(1 , true);
//	      printf("%d >3200 \n",e.jaxis.value);
                    }
                    if ( e.jaxis.value >= -3200 && e.jaxis.value <= 3200 )
                    {
                        car.controller.Joy(0 , false);
                        car.controller.Joy(1 , false);
//	      printf("%d in [-3200,3200] \n",e.jaxis.value);
                    }
                    rendering(win);
                    //redraw();
                }
                break;
            case SDL_JOYBUTTONDOWN: /* Handle Joystick Button Presses */
                if ( e.jbutton.button == 0 )
                {
                    car.controller.Joy(2 , true);
//	   printf("jbutton 0\n");
                }
                if ( e.jbutton.button == 2 )
                {
                    car.controller.Joy(3 , true);
//	   printf("jbutton 2\n");
                }
                break;
            case SDL_JOYBUTTONUP: /* Handle Joystick Button Presses */
                car.controller.Joy(2 , false);
                car.controller.Joy(3 , false);
                break;
            }
        } else {
            // nessun evento: siamo IDLE

            Uint32 timeNow=SDL_GetTicks(); // che ore sono?

            if (timeLastInterval+fpsSampling<timeNow) {
                fps = 1000.0*((float)fpsNow) /(timeNow-timeLastInterval);
                fpsNow=0;
                timeLastInterval = timeNow;
            }

            bool doneSomething=false;
            int guardia=0; // sicurezza da loop infinito

            // finche' il tempo simulato e' rimasto indietro rispetto
            // al tempo reale...
            while (nstep*PHYS_SAMPLING_STEP < timeNow ) {
                car.DoStep();
                nstep++;
                doneSomething=true;
                timeNow=SDL_GetTicks();
                if (guardia++>1000) {
                    done=true;    // siamo troppo lenti!
                    break;
                }
            }

            if (doneSomething)
                rendering(win);
            //redraw();
            else {
                // tempo libero!!!
            }
        }
    }
    SDL_GL_DeleteContext(mainContext);
    SDL_DestroyWindow(win);
    SDL_Quit ();
    return (0);
}

