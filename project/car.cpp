// car.cpp
// implementazione dei metodi definiti in car.h
#include "car.h"

// mesh
Mesh carlinga((char *)"jeep/jeep_chassis_logo.obj", true, Vector3(-0.05,0.05,-0.05)); // chiama il costruttore
Mesh wheelBR1((char *)"jeep/Jeep_wheel_back_R.obj", false);
Mesh wheelFR1((char *)"jeep/Jeep_wheel_front_R.obj", false);
//Mesh wheelBR2((char *)"Ferrari_wheel_back_R_metal.obj");
//Mesh wheelFR2((char *)"Ferrari_wheel_front_R_metal.obj");

extern bool useEnvmap; // var globale esterna: per usare l'evnrionment mapping
extern bool useHeadlight; // var globale esterna: per usare i fari
extern bool useShadow; // var globale esterna: per generare l'ombra

// da invocare quando e' stato premuto/rilasciato il tasto numero "keycode"
void Controller::EatKey(int keycode, int* keymap, bool pressed_or_released)
{
    for (int i=0; i<NKEYS; i++) {
        if (keycode==keymap[i]) key[i]=pressed_or_released;
    }
}

// da invocare quando e' stato premuto/rilasciato un jbutton
void Controller::Joy(int keymap, bool pressed_or_released)
{
    key[keymap]=pressed_or_released;
}

// Funzione che prepara tutto per usare un env map
void SetupEnvmapTexture()
{
    // facciamo binding con la texture 1
    glBindTexture(GL_TEXTURE_2D,1);

    glEnable(GL_TEXTURE_2D);
    glEnable(GL_TEXTURE_GEN_S); // abilito la generazione automatica delle coord texture S e T
    glEnable(GL_TEXTURE_GEN_T);
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE , GL_SPHERE_MAP); // Env map
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE , GL_SPHERE_MAP);
    glColor3f(1,1,1); // metto il colore neutro (viene moltiplicato col colore texture, componente per componente)
    glDisable(GL_LIGHTING); // disabilito il lighting OpenGL standard (lo faccio con la texture)
}

// funzione che prepara tutto per creare le coordinate texture (s,t) da (x,y,z)
// Mappo l'intervallo [ minY , maxY ] nell'intervallo delle T [0..1]
//     e l'intervallo [ minZ , maxZ ] nell'intervallo delle S [0..1]
void SetupWheelTexture(Point3 min, Point3 max) {
    glBindTexture(GL_TEXTURE_2D,0);
    glEnable(GL_TEXTURE_2D);
    glEnable(GL_TEXTURE_GEN_S);
    glEnable(GL_TEXTURE_GEN_T);

    // ulilizzo le coordinate OGGETTO
    // cioe' le coordnate originali, PRIMA della moltiplicazione per la ModelView
    // in modo che la texture sia "attaccata" all'oggetto, e non "proiettata" su esso
    glTexGeni(GL_S, GL_TEXTURE_GEN_MODE , GL_OBJECT_LINEAR);
    glTexGeni(GL_T, GL_TEXTURE_GEN_MODE , GL_OBJECT_LINEAR);
    float sz=1.0/(max.Z() - min.Z());
    float ty=1.0/(max.Y() - min.Y());
    float s[4]= {0,0,sz,  - min.Z()*sz };
    float t[4]= {0,ty,0,  - min.Y()*ty };
    glTexGenfv(GL_S, GL_OBJECT_PLANE, s);
    glTexGenfv(GL_T, GL_OBJECT_PLANE, t);
}


bool Car::checkCollision(Mesh b){
  
  
  float car_v[][2]={
    {carlinga.bbmax.X(), carlinga.bbmax.Z()},
    {carlinga.bbmax.X(), carlinga.bbmin.Z()},
    {carlinga.bbmin.X(), carlinga.bbmax.Z()},
    {carlinga.bbmin.X(), carlinga.bbmin.Z()}
  };
  
  for (int i = 0; i < 4; i++){
    if((b.bbmin.X() < car_v[i][0]) && (car_v[i][0] < b.bbmax.X()) && 
      (b.bbmin.Z() < car_v[i][1]) && (car_v[i][1] < b.bbmax.Z()))
      return true;
  }
  
  return false;
}

Point3 Car::getWorldCoords()
{
  return carlinga.GetWorldCoords();
}

float distance(Point3 a, Point3 b){
  return sqrt(pow(a.X()-b.X(), 2) + pow(a.Y()-b.Y(), 2) + pow(a.Z()-b.Z(), 2)) ;
}

#define NOS_ID 6666
#define NOS_ID2 6667
#define NOS_ID3 6668

extern std::vector<Mesh> buildings;
extern Mesh pizza, man;
extern Mesh nos, nos2, nos3;
extern bool pizza_taken, nos_active, nos_redrawn;
extern int nos_to_redraw;
extern unsigned int n_deliver;
time_t start_nos;

// DoStep: facciamo un passo di fisica (a delta_t costante)
//
// Indipendente dal rendering.
//
// ricordiamoci che possiamo LEGGERE ma mai SCRIVERE
// la struttura controller da DoStep
void Car::DoStep() {
    // computiamo l'evolversi della macchina
    static int i=5;

    float vxm, vym, vzm; // velocita' in spazio macchina

    // da vel frame mondo a vel frame macchina
    float cosf = cos(facing*M_PI/180.0);
    float sinf = sin(facing*M_PI/180.0);
    vxm = +cosf*vx - sinf*vz;
    vym = vy;
    vzm = +sinf*vx + cosf*vz;

    // gestione dello sterzo
    if (controller.key[Controller::LEFT]) sterzo+=velSterzo;
    if (controller.key[Controller::RIGHT]) sterzo-=velSterzo;
    sterzo*=velRitornoSterzo; // ritorno a volante dritto
    
    if(nos_active && ((time(NULL) - start_nos) > 5)){
      nos_active = false;
    }
    
    if(nos_active){
      if (controller.key[Controller::ACC]) vzm-=accMax*2; // accelerazione in avanti
      if (controller.key[Controller::DEC]) vzm+=accMax*2; // accelerazione indietro
    }else{
      if (controller.key[Controller::ACC]) vzm-=accMax; // accelerazione in avanti
      if (controller.key[Controller::DEC]) vzm+=accMax; // accelerazione indietro
    }

    // attirti (semplificando)
    vxm*=attritoX;
    vym*=attritoY;
    vzm*=attritoZ;

    // l'orientamento della macchina segue quello dello sterzo
    // (a seconda della velocita' sulla z)
    facing = facing - (vzm*grip)*sterzo;

    // rotazione mozzo ruote (a seconda della velocita' sulla z)
    float da ; //delta angolo
    da=(360.0*vzm)/(2.0*M_PI*raggioRuotaA);
    mozzoA+=da;
    da=(360.0*vzm)/(2.0*M_PI*raggioRuotaP);
    mozzoP+=da;

    // ritorno a vel coord mondo
    vx = +cosf*vxm + sinf*vzm;
    vy = vym;
    vz = -sinf*vxm + cosf*vzm;

    // posizione = posizione + velocita * delta t (ma delta t e' costante)
    px+=vx;
    py+=vy;
    pz+=vz;
    
    Point3 pos(px, py,pz);
    carlinga.UpdateWorldCoords(pos);
    
    Point3 vel(vx,vy,vz);
    carlinga.TranslateBBCoord(vel);
    
    
    for(int i = 0; i < buildings.size(); i++){
      if(checkCollision(buildings.at(i)) ){
	vx = 0; vy = 0; vz = 0; // if collision occurs, stop the vehicle
      }
    }
    
    if(distance(carlinga.GetWorldCoords(), pizza.GetWorldCoords()) < 1){
      pizza_taken = true;
    }
    
    
    if(distance(carlinga.GetWorldCoords(), man.GetWorldCoords()) < 1){
      pizza_taken = false;
    }
        
    if(distance(carlinga.GetWorldCoords(), nos.GetWorldCoords()) < 0.5){

      nos_active = true;
      nos_redrawn = false;
      nos_to_redraw = NOS_ID;
      start_nos = time(NULL);
    }
    
    if(distance(carlinga.GetWorldCoords(), nos2.GetWorldCoords()) < 0.5){

      nos_active = true;
      nos_redrawn = false;
      nos_to_redraw = NOS_ID2;
      start_nos = time(NULL);
    }
    
    if(distance(carlinga.GetWorldCoords(), nos3.GetWorldCoords()) < 0.5){

      nos_active = true;
      nos_redrawn = false;
      nos_to_redraw = NOS_ID3;
      start_nos = time(NULL);
    }
    
    
    //printf ("distance b_short %f\n", (float) checkCollision(carlinga, b_short ));
    //printf ("car bbmax (%f, %f, %f) bbmin (%f, %f, %f)\n", carlinga.bbmax.X(), carlinga.bbmax.Y(), carlinga.bbmax.Z(), carlinga.bbmin.X(), carlinga.bbmin.Y(), carlinga.bbmin.Z());
    //printf ("b_short bbmax (%f, %f, %f) bbmin (%f, %f, %f)\n", b_short.bbmax.X(), b_short.bbmax.Y(), b_short.bbmax.Z(), b_short.bbmin.X(), b_short.bbmin.Y(), b_short.bbmin.Z());

    
}

void drawAxis(); // anche questa

void Controller::Init() {
    for (int i=0; i<NKEYS; i++) key[i]=false;
}

void Car::Init() {
    // inizializzo lo stato della macchina
    px=pz=facing=0; // posizione e orientamento
    py=0.0;

    mozzoA=mozzoP=sterzo=0;   // stato
    vx=vy=vz=0;      // velocita' attuale
    // inizializzo la struttura di controllo
    controller.Init();

    //velSterzo=3.4;         // A
    velSterzo=2.4;         // A
    velRitornoSterzo=0.93; // B, sterzo massimo = A*B / (1-B)

    accMax = 0.0011;
    

    // attriti: percentuale di velocita' che viene mantenuta
    // 1 = no attrito
    // <<1 = attrito grande
    attritoZ = 0.991;  // piccolo attrito sulla Z (nel senso di rotolamento delle ruote)
    attritoX = 0.8;  // grande attrito sulla X (per non fare slittare la macchina)
    attritoY = 1.0;  // attrito sulla y nullo

    // Nota: vel max = accMax*attritoZ / (1-attritoZ)

    raggioRuotaA = 0.25;
    raggioRuotaP = 0.35;

    grip = 0.45; // quanto il facing macchina si adegua velocemente allo sterzo


}


// attiva una luce di openGL per simulare un faro della macchina
void Car::DrawHeadlight(float x, float y, float z, int lightN, bool useHeadlight) const {
    int usedLight=GL_LIGHT1 + lightN;

    if(useHeadlight)
    {
        glEnable(usedLight);

        float col0[4]= {0.8,0.8,0.0,  1};
        glLightfv(usedLight, GL_DIFFUSE, col0);

        float col1[4]= {0.5,0.5,0.0,  1};
        glLightfv(usedLight, GL_AMBIENT, col1);

        float tmpPos[4] = {x,y,z,  1}; // ultima comp=1 => luce posizionale
        glLightfv(usedLight, GL_POSITION, tmpPos );

        float tmpDir[4] = {0,0,-1,  0}; // ultima comp=1 => luce posizionale
        glLightfv(usedLight, GL_SPOT_DIRECTION, tmpDir );

        glLightf (usedLight, GL_SPOT_CUTOFF, 30);
        glLightf (usedLight, GL_SPOT_EXPONENT,5);

        glLightf(usedLight,GL_CONSTANT_ATTENUATION,0);
        glLightf(usedLight,GL_LINEAR_ATTENUATION,1);
    }
    else
        glDisable(usedLight);
}


// funzione che disegna tutti i pezzi della macchina
// (carlinga, + 4 route)
// (da invocarsi due volte: per la macchina, e per la sua ombra)
// (se usecolor e' falso, NON sovrascrive il colore corrente
//  e usa quello stabilito prima di chiamare la funzione)
void Car::RenderAllParts(bool usecolor) const {

    // drawCarlinga(); // disegna la carliga con pochi parallelepidedi

    // disegna la carliga con una mesh
    glPushMatrix();
    glScalef(carlinga.mesh_scale.X(),carlinga.mesh_scale.Y(),carlinga.mesh_scale.Z()); // patch: riscaliamo la mesh di 1/10
    
    
    /*if (!useEnvmap)
    {
        if (usecolor) glColor3f(1,0,0);     // colore rosso, da usare con Lighting
    }
    else {
        if (usecolor) SetupEnvmapTexture();
    }*/
    
    carlinga.RenderNxV(usecolor); // rendering delle mesh carlinga usando normali per vertice
    if (usecolor) glEnable(GL_LIGHTING);

    for (int i=0; i<2; i++) {
        // i==0 -> disegno ruote destre.
        // i==1 -> disegno ruote sinistre.
        int sign;
        if (i==0) sign=1;
        else sign=-1;
        glPushMatrix();
        if (i==1) {
            glTranslatef(0,+wheelFR1.Center().Y(), 0);
            glRotatef(180, 0,0,1 );
            glTranslatef(0,-wheelFR1.Center().Y(), 0);
        }

        glTranslate(  wheelFR1.Center() );
        glRotatef( sign*sterzo,0,1,0);
        glRotatef(-sign*mozzoA,1,0,0);
        glTranslate( -wheelFR1.Center() );

        if (usecolor) glColor3f(.6,.6,.6);
        if (usecolor) SetupWheelTexture(wheelFR1.bbmin,wheelFR1.bbmax);
        wheelFR1.RenderNxF(); // la ruota viene meglio FLAT SHADED - normali per faccia
        // provare x credere
        glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
        //if (usecolor) glColor3f(0.9,0.9,0.9);
        //wheelFR2.RenderNxV();
        glPopMatrix();

        glPushMatrix();
        if (i==1) {
            glTranslatef(0,+wheelBR1.Center().Y(), 0);
            glRotatef(180, 0,0,1 );
            glTranslatef(0,-wheelBR1.Center().Y(), 0);
        }

        glTranslate(  wheelBR1.Center() );
        glRotatef(-sign*mozzoA,1,0,0);
        glTranslate( -wheelBR1.Center() );

        if (usecolor) glColor3f(.6,.6,.6);
        if (usecolor) SetupWheelTexture(wheelBR1.bbmin,wheelBR1.bbmax);
        wheelBR1.RenderNxF();
        glDisable(GL_TEXTURE_2D);
	glDisable(GL_TEXTURE_GEN_S);
	glDisable(GL_TEXTURE_GEN_T);
        //if (usecolor) glColor3f(0.9,0.9,0.9);
        //wheelBR2.RenderNxV();
        glPopMatrix();
    }

    glPopMatrix();
}

// disegna a schermo
void Car::Render() const {
    // sono nello spazio mondo

    //drawAxis(); // disegno assi spazio mondo
    glPushMatrix();

    glTranslatef(px,py,pz);
    glRotatef(facing, 0,1,0);

    // sono nello spazio MACCHINA
    //drawAxis(); // disegno assi spazio macchina

    DrawHeadlight(-0.3,0,-1, 0, useHeadlight); // accendi faro sinistro
    DrawHeadlight(+0.3,0,-1, 1, useHeadlight); // accendi faro destro

    RenderAllParts(true);

    // ombra!
    if(useShadow)
    {
        glColor3f(0.1,0.1,0.1); // colore fisso
        glTranslatef(0,0.01,0); // alzo l'ombra di un epsilon per evitare z-fighting con il pavimento
        glScalef(1.01,0,1.01);  // appiattisco sulla Y, ingrandisco dell'1% sulla Z e sulla X
        glDisable(GL_LIGHTING); // niente lighing per l'ombra
        RenderAllParts(false);  // disegno la macchina appiattita

        glEnable(GL_LIGHTING);
    }
    glPopMatrix();

    glPopMatrix();
}
