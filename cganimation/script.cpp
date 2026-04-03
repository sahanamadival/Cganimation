/*
 * THE SILENT SCRIPT — Enhanced 3D OpenGL Animation
 * Yakshagana replaced with: Celebration Stage + Fireworks scene
 * All 3D objects enhanced for visual appeal
 *
 * Compile (Linux):
 *   g++ silent_script_3d.cpp -lGL -lGLU -lglut -lm -O2 -o silent_script_3d
 * Compile (macOS):
 *   g++ silent_script_3d.cpp -framework OpenGL -framework GLUT -Wno-deprecated -O2 -o silent_script_3d
 */

#include <GL/glut.h>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <algorithm>

// ══════════════════════════════════════════════════════
//  GLOBALS
// ══════════════════════════════════════════════════════
static float  g_time    = 0.0f;
static int    g_lastMs  = 0;
static bool   g_paused  = false;
static int    WIN_W     = 1280, WIN_H = 720;
static const float TOTAL = 300.0f;

// ══════════════════════════════════════════════════════
//  MATH HELPERS
// ══════════════════════════════════════════════════════
static inline float clamp(float v,float lo,float hi){ return v<lo?lo:v>hi?hi:v; }
static inline float c01(float v){ return clamp(v,0,1); }
static inline float lerp(float a,float b,float t){ return a+(b-a)*c01(t); }
static inline float smooth(float t){ t=c01(t); return t*t*(3-2*t); }
static inline float smoothIn(float t,float s,float d){ return smooth(c01((t-s)/d)); }
static inline float smoothOut(float t,float e,float d){ return 1-smooth(c01((t-(e-d))/d)); }
static inline float sFade(float t,float s,float e,float d=2.f){
    return c01(smoothIn(t,s,d)*smoothOut(t,e,d));
}
static inline float pulse(float t,float f){ return 0.5f+0.5f*sinf(t*f*6.2832f); }
static inline float randf(float lo,float hi){ return lo+(hi-lo)*(rand()/(float)RAND_MAX); }

struct V3 { float x,y,z; };
static inline V3 v3(float x,float y,float z){ return {x,y,z}; }

// ══════════════════════════════════════════════════════
//  SCENE TIMING
// ══════════════════════════════════════════════════════
#define S_TITLE_S   0.f
#define S_TITLE_E  10.f
#define S_KING_S   10.f
#define S_KING_E   40.f
#define S_TRANS_S  40.f
#define S_TRANS_E  65.f
#define S_STAGE_S  65.f
#define S_STAGE_E 105.f
#define S_LAUGH_S 105.f
#define S_LAUGH_E 135.f
#define S_LOW_S   135.f
#define S_LOW_E   158.f
#define S_SIS_S   158.f
#define S_SIS_E   190.f
#define S_MATH_S  190.f
#define S_MATH_E  220.f
#define S_PHYS_S  220.f
#define S_PHYS_E  250.f
#define S_COME_S  250.f
#define S_COME_E  275.f
#define S_CELEB_S 275.f   
#define S_CELEB_E 295.f
#define S_MENT_S  295.f
#define S_MENT_E  300.f

// ══════════════════════════════════════════════════════
//  PARTICLE SYSTEM
// ══════════════════════════════════════════════════════
struct Particle {
    float x,y,z,vx,vy,vz,life,maxLife,size;
    float r,g,b,a;
    bool  alive;
};
static const int MAX_PART = 3000;
static Particle parts[MAX_PART];
static int      partHead = 0;

static void spawnParticle(float x,float y,float z,
                           float vx,float vy,float vz,
                           float life,float size,
                           float r,float g,float b){
    Particle& p = parts[partHead % MAX_PART];
    p={x,y,z,vx,vy,vz,life,life,size,r,g,b,1.f,true};
    partHead++;
}

static void updateParticles(float dt){
    for(auto& p:parts){
        if(!p.alive) continue;
        p.x+=p.vx*dt; p.y+=p.vy*dt; p.z+=p.vz*dt;
        p.vy-=2.5f*dt;
        p.life-=dt;
        p.a=c01(p.life/p.maxLife);
        if(p.life<=0) p.alive=false;
    }
}

static void drawParticles(){
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    glDepthMask(GL_FALSE);
    for(auto& p:parts){
        if(!p.alive) continue;
        glColor4f(p.r,p.g,p.b,p.a);
        glPushMatrix();
        glTranslatef(p.x,p.y,p.z);
        glutSolidSphere(p.size*p.a,6,6);
        glPopMatrix();
    }
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

// ══════════════════════════════════════════════════════
//  LIGHTING HELPERS
// ══════════════════════════════════════════════════════
static void setLight0(float px,float py,float pz,
                       float r,float g,float b,float amb=0.15f){
    glEnable(GL_LIGHT0);
    float pos[]={px,py,pz,1.f};
    float col[]={r,g,b,1.f};
    float ac[] ={amb,amb,amb,1.f};
    float spec[]={1,1,1,1};
    glLightfv(GL_LIGHT0,GL_POSITION,pos);
    glLightfv(GL_LIGHT0,GL_DIFFUSE,col);
    glLightfv(GL_LIGHT0,GL_AMBIENT,ac);
    glLightfv(GL_LIGHT0,GL_SPECULAR,spec);
}
static void setLight1(float px,float py,float pz,
                       float r,float g,float b){
    glEnable(GL_LIGHT1);
    float pos[]={px,py,pz,1.f};
    float col[]={r,g,b,1.f};
    float ac[] ={0,0,0,1.f};
    glLightfv(GL_LIGHT1,GL_POSITION,pos);
    glLightfv(GL_LIGHT1,GL_DIFFUSE,col);
    glLightfv(GL_LIGHT1,GL_AMBIENT,ac);
}
static void mat(float r,float g,float b,float a=1,float shine=32,float spec=0.4f){
    float d[]={r,g,b,a};
    float s[]={spec,spec,spec,1};
    float am[]={r*.3f,g*.3f,b*.3f,a};
    glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,d);
    glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,s);
    glMaterialfv(GL_FRONT_AND_BACK,GL_AMBIENT,am);
    glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,shine);
}
static void matEmit(float r,float g,float b,float a=1){
    float e[]={r,g,b,a};
    float z[]={0,0,0,1};
    glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,e);
    glMaterialfv(GL_FRONT_AND_BACK,GL_DIFFUSE,z);
    glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,z);
}
static void noEmit(){
    float z[]={0,0,0,1};
    glMaterialfv(GL_FRONT_AND_BACK,GL_EMISSION,z);
}

// ══════════════════════════════════════════════════════
//  CAMERA
// ══════════════════════════════════════════════════════
struct Camera { float ex,ey,ez,lx,ly,lz,ux,uy,uz; };
static Camera cam;

static void applyCamera(){
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(cam.ex,cam.ey,cam.ez,
              cam.lx,cam.ly,cam.lz,
              cam.ux,cam.uy,cam.uz);
}

// ══════════════════════════════════════════════════════
//  COMMON 3D SHAPES
// ══════════════════════════════════════════════════════
static void cylinder(float r,float h,int sl=24){
    GLUquadric* q=gluNewQuadric();
    gluQuadricNormals(q,GLU_SMOOTH);
    glPushMatrix();
    glRotatef(-90,1,0,0);
    gluCylinder(q,r,r,h,sl,1);
    gluDisk(q,0,r,sl,1);
    glTranslatef(0,0,h);
    gluDisk(q,0,r,sl,1);
    glPopMatrix();
    gluDeleteQuadric(q);
}

static void sphere(float r,int sl=24){ glutSolidSphere(r,sl,sl); }

static void torus(float ir,float or_,int si=24,int ri=16){
    glutSolidTorus(ir,or_,ri,si);
}

static void cone(float r,float h,int sl=20){
    GLUquadric* q=gluNewQuadric();
    gluQuadricNormals(q,GLU_SMOOTH);
    glPushMatrix();
    glRotatef(-90,1,0,0);
    gluCylinder(q,r,0,h,sl,1);
    gluDisk(q,0,r,sl,1);
    glPopMatrix();
    gluDeleteQuadric(q);
}

// ══════════════════════════════════════════════════════
//  ENHANCED 3D HUMAN FIGURE
// ══════════════════════════════════════════════════════
enum Pose { POSE_STAND, POSE_RAISE, POSE_HUNCH, POSE_SPEAK, POSE_WRITE, POSE_DANCE };

static void drawHuman(float r,float g,float b,
                       Pose pose,float anim,
                       float headR=0.28f,float shine=48){
    float skinR=0.88f,skinG=0.68f,skinB=0.50f;
    float hairR=0.10f,hairG=0.06f,hairB=0.03f;
    float walkPhase = sinf(anim*3.0f);

    // ── HEAD ──
    glPushMatrix();
      float htz = (pose==POSE_HUNCH) ? 0.3f : 0.0f;
      glTranslatef(0,1.72f+headR*0.5f,htz);
      mat(skinR,skinG,skinB,1,shine);
      sphere(headR,20);
      // hair cap
      glPushMatrix();
        glTranslatef(0,headR*0.22f,0);
        glScalef(1.0f,0.52f,1.05f);
        mat(hairR,hairG,hairB,1,22);
        sphere(headR*1.04f,20);
      glPopMatrix();
      // eyes (whites + pupils)
      for(int side=-1;side<=1;side+=2){
          glPushMatrix();
            glTranslatef(side*0.09f,0.06f,headR*0.88f);
            mat(0.95f,0.95f,0.95f,1,60);
            sphere(0.045f,12); // white
            glTranslatef(0,0,0.025f);
            mat(0.1f,0.1f,0.15f,1,90,0.9f);
            sphere(0.028f,10); // pupil
          glPopMatrix();
      }
      // eyebrows
      for(int side=-1;side<=1;side+=2){
          glPushMatrix();
            glTranslatef(side*0.09f,0.12f,headR*0.85f);
            glRotatef(-side*10,0,0,1);
            mat(hairR,hairG,hairB,1,20);
            glScalef(0.09f,0.025f,0.025f);
            glutSolidCube(1);
          glPopMatrix();
      }
      // nose
      glPushMatrix();
        glTranslatef(0,-0.02f,headR*0.94f);
        mat(skinR*0.85f,skinG*0.75f,skinB*0.65f,1,30);
        glScalef(0.04f,0.05f,0.04f);
        glutSolidSphere(1,8,8);
      glPopMatrix();
      // mouth smile
      glPushMatrix();
        glTranslatef(0,-0.09f,headR*0.88f);
        mat(0.72f,0.25f,0.22f,1,30);
        glScalef(0.1f,0.025f,0.02f);
        glutSolidCube(1);
      glPopMatrix();
    glPopMatrix();

    // ── NECK ──
    glPushMatrix();
      glTranslatef(0,1.62f,0);
      mat(skinR,skinG,skinB,1,shine);
      cylinder(0.075f,0.12f,16);
    glPopMatrix();

    // ── TORSO ──
    glPushMatrix();
      if(pose==POSE_HUNCH){
        glTranslatef(0,1.02f,0);
        glRotatef(28,1,0,0);
      } else {
        glTranslatef(0,0.92f,0);
      }
      mat(r,g,b,1,32);
      glPushMatrix(); glScalef(0.56f,0.72f,0.32f); glutSolidCube(1); glPopMatrix();
      // collar
      glPushMatrix();
        glTranslatef(0,0.33f,0.1f);
        mat(r*0.8f,g*0.8f,b*0.8f,1,20);
        glScalef(0.42f,0.08f,0.14f); glutSolidCube(1);
      glPopMatrix();
    glPopMatrix();

    // ── LEFT ARM ──
    glPushMatrix();
      glTranslatef(-0.33f,1.42f,0);
      float la=0;
      if(pose==POSE_RAISE)  la=150;
      if(pose==POSE_SPEAK)  la=60+sinf(anim*2)*20;
      if(pose==POSE_DANCE)  la=90+sinf(anim*3)*45;
      if(pose==POSE_HUNCH)  la=-25;
      if(pose==POSE_WRITE)  la=65;
      glRotatef(la,1,0,0);
      mat(skinR,skinG,skinB,1,shine);
      glPushMatrix(); glTranslatef(0,-0.21f,0);
        glScalef(0.13f,0.46f,0.13f); glutSolidCube(1);
      glPopMatrix();
      glTranslatef(0,-0.46f,0);
      glRotatef(-28,1,0,0);
      glPushMatrix(); glTranslatef(0,-0.16f,0);
        mat(r*0.85f,g*0.85f,b*0.85f,1,shine);
        glScalef(0.11f,0.36f,0.11f); glutSolidCube(1);
      glPopMatrix();
      glTranslatef(0,-0.32f,0);
      mat(skinR,skinG,skinB,1,shine);
      sphere(0.075f,12);
    glPopMatrix();

    // ── RIGHT ARM ──
    glPushMatrix();
      glTranslatef(0.33f,1.42f,0);
      float ra=0;
      if(pose==POSE_RAISE)  ra=150;
      if(pose==POSE_SPEAK)  ra=40+sinf(anim*2+1)*20;
      if(pose==POSE_DANCE)  ra=90+sinf(anim*3+1.5f)*45;
      if(pose==POSE_HUNCH)  ra=-25;
      if(pose==POSE_WRITE)  ra=82;
      glRotatef(ra,1,0,0);
      mat(skinR,skinG,skinB,1,shine);
      glPushMatrix(); glTranslatef(0,-0.21f,0);
        glScalef(0.13f,0.46f,0.13f); glutSolidCube(1);
      glPopMatrix();
      glTranslatef(0,-0.46f,0);
      glRotatef(-28,1,0,0);
      glPushMatrix(); glTranslatef(0,-0.16f,0);
        mat(r*0.85f,g*0.85f,b*0.85f,1,shine);
        glScalef(0.11f,0.36f,0.11f); glutSolidCube(1);
      glPopMatrix();
      glTranslatef(0,-0.32f,0);
      mat(skinR,skinG,skinB,1,shine);
      sphere(0.075f,12);
    glPopMatrix();

    // ── PELVIS ──
    glPushMatrix();
      glTranslatef(0,0.52f,0);
      mat(r*0.55f,g*0.55f,b*0.85f,1,22);
      glPushMatrix(); glScalef(0.50f,0.24f,0.30f); glutSolidCube(1); glPopMatrix();
    glPopMatrix();

    // ── LEFT LEG ──
    glPushMatrix();
      glTranslatef(-0.15f,0.42f,0);
      float ll= (pose==POSE_DANCE) ? sinf(anim*3)*28 : walkPhase*10;
      glRotatef(ll,1,0,0);
      mat(0.15f,0.15f,0.38f,1,22);
      glPushMatrix(); glTranslatef(0,-0.26f,0);
        glScalef(0.17f,0.56f,0.17f); glutSolidCube(1);
      glPopMatrix();
      glTranslatef(0,-0.56f,0);
      glRotatef(-ll*0.5f,1,0,0);
      glPushMatrix(); glTranslatef(0,-0.21f,0);
        glScalef(0.15f,0.45f,0.15f); glutSolidCube(1);
      glPopMatrix();
      // shoe
      glTranslatef(0,-0.42f,0.05f);
      mat(0.1f,0.07f,0.05f,1,40,0.6f);
      glScalef(0.16f,0.1f,0.28f); glutSolidCube(1);
    glPopMatrix();

    // ── RIGHT LEG ──
    glPushMatrix();
      glTranslatef(0.15f,0.42f,0);
      float rl= (pose==POSE_DANCE) ? sinf(anim*3+1.6f)*28 : -walkPhase*10;
      glRotatef(rl,1,0,0);
      mat(0.15f,0.15f,0.38f,1,22);
      glPushMatrix(); glTranslatef(0,-0.26f,0);
        glScalef(0.17f,0.56f,0.17f); glutSolidCube(1);
      glPopMatrix();
      glTranslatef(0,-0.56f,0);
      glRotatef(-rl*0.5f,1,0,0);
      glPushMatrix(); glTranslatef(0,-0.21f,0);
        glScalef(0.15f,0.45f,0.15f); glutSolidCube(1);
      glPopMatrix();
      glTranslatef(0,-0.42f,0.05f);
      mat(0.1f,0.07f,0.05f,1,40,0.6f);
      glScalef(0.16f,0.1f,0.28f); glutSolidCube(1);
    glPopMatrix();
}

// ══════════════════════════════════════════════════════
//  ENHANCED FLOOR  (checkerboard with sheen)
// ══════════════════════════════════════════════════════
static void drawFloor(float W,float D,float r,float g,float b,int tiles=8){
    for(int i=0;i<tiles;i++) for(int j=0;j<tiles;j++){
        bool chk=(i+j)%2==0;
        mat(chk?r:r*0.65f, chk?g:g*0.65f, chk?b:b*0.65f,1,chk?40:10,chk?0.5f:0.1f);
        glPushMatrix();
          glTranslatef((i-tiles/2)*W/tiles, 0, (j-tiles/2)*D/tiles);
          glScalef(W/tiles,0.05f,D/tiles);
          glutSolidCube(1);
        glPopMatrix();
    }
}

// ══════════════════════════════════════════════════════
//  STARFIELD
// ══════════════════════════════════════════════════════
static struct Star { float x,y,z; } stars[400];
static bool starsInit=false;
static void initStars(){
    if(starsInit) return;
    srand(7777);
    for(auto& s:stars){
        s.x=randf(-60,60); s.y=randf(-25,35); s.z=randf(-90,-10);
    }
    starsInit=true;
}
static void drawStars(float t,float alpha=1){
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE);
    glPointSize(3.0f);
    glBegin(GL_POINTS);
    for(int i=0;i<400;i++){
        float blink=0.5f+0.5f*sinf(t*1.2f+i*0.63f);
        glColor4f(1,1,0.9f,alpha*blink*0.9f);
        glVertex3f(stars[i].x,stars[i].y,stars[i].z);
    }
    glEnd();
    glPointSize(1);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

// ══════════════════════════════════════════════════════
//  ENHANCED BLACKBOARD
// ══════════════════════════════════════════════════════
static void drawBlackboard(float W,float H){
    // Outer wooden frame
    mat(0.38f,0.22f,0.08f,1,36);
    glPushMatrix(); glScalef(W+0.28f,H+0.28f,0.14f); glutSolidCube(1); glPopMatrix();
    // Inner frame shadow
    mat(0.25f,0.14f,0.05f,1,20);
    glPushMatrix(); glTranslatef(0,0,0.04f); glScalef(W+0.14f,H+0.14f,0.06f); glutSolidCube(1); glPopMatrix();
    // Board surface (dark green with subtle sheen)
    mat(0.08f,0.20f,0.10f,1,8,0.08f);
    glPushMatrix(); glTranslatef(0,0,0.08f); glScalef(W,H,0.025f); glutSolidCube(1); glPopMatrix();
    // Chalk tray at bottom
    mat(0.28f,0.16f,0.06f,1,20);
    glPushMatrix(); glTranslatef(0,-H/2-0.06f,0.04f); glScalef(W+0.1f,0.1f,0.12f); glutSolidCube(1); glPopMatrix();
}

// ══════════════════════════════════════════════════════
//  FLOATING TEXT BILLBOARD
// ══════════════════════════════════════════════════════
static void billboard3D(float x,float y,float z,const char* text,
                          float r,float g,float b,float a,
                          void* font=GLUT_BITMAP_HELVETICA_18){
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);
    glColor4f(r,g,b,a);
    glRasterPos3f(x,y,z);
    for(;*text;text++) glutBitmapCharacter(font,*text);
    glDisable(GL_BLEND);
    glEnable(GL_LIGHTING);
}

// ══════════════════════════════════════════════════════
//  ENHANCED TROPHY
// ══════════════════════════════════════════════════════
static void drawTrophy(float t, float scale=1.0f){
    glPushMatrix();
    glScalef(scale,scale,scale);
    float bob = sinf(t*2.f)*0.04f;
    glTranslatef(0,bob,0);

    // Base pedestal
    mat(0.62f,0.48f,0.08f,1,80,0.9f);
    glPushMatrix();
      glTranslatef(0,0,0);
      glScalef(1.0f,0.12f,0.6f); glutSolidCube(1);
    glPopMatrix();
    glPushMatrix();
      glTranslatef(0,0.12f,0);
      glScalef(0.75f,0.1f,0.45f); glutSolidCube(1);
    glPopMatrix();
    // Stem
    glPushMatrix();
      glTranslatef(0,0.12f,0);
      cylinder(0.1f,0.55f,20);
    glPopMatrix();
    // Stem mid-ball
    glPushMatrix();
      glTranslatef(0,0.5f,0);
      mat(1.f,0.85f,0.12f,1,100,1.0f);
      sphere(0.16f,20);
    glPopMatrix();
    // Cup body (sphere flattened)
    glPushMatrix();
      glTranslatef(0,0.9f,0);
      mat(1.f,0.82f,0.08f,1,100,0.95f);
      glScalef(1.0f,1.3f,0.9f);
      sphere(0.42f,24);
    glPopMatrix();
    // Cup opening (torus rim)
    glPushMatrix();
      glTranslatef(0,1.45f,0);
      mat(1.f,0.9f,0.2f,1,100,0.9f);
      glRotatef(90,1,0,0);
      torus(0.04f,0.38f,24,12);
    glPopMatrix();
    // Handles
    for(int side=-1;side<=1;side+=2){
        glPushMatrix();
          glTranslatef(side*0.5f,0.95f,0);
          glRotatef(90,0,0,1);
          mat(0.9f,0.72f,0.06f,1,80,0.8f);
          torus(0.04f,0.18f,20,10);
        glPopMatrix();
    }
    // Rotating star on top
    glPushMatrix();
      glTranslatef(0,1.62f,0);
      glRotatef(t*180,0,1,0);
      matEmit(1.f,0.9f,0.2f);
      glScalef(0.18f,0.18f,0.18f);
      glutSolidOctahedron();
      noEmit();
    glPopMatrix();
    // Engraved text panel
    glPushMatrix();
      glTranslatef(0,0.82f,0.38f);
      mat(0.72f,0.55f,0.05f,1,60);
      glScalef(0.55f,0.32f,0.04f); glutSolidCube(1);
    glPopMatrix();
    glPopMatrix();
}

// ══════════════════════════════════════════════════════
//  ENHANCED PODIUM
// ══════════════════════════════════════════════════════
static void drawPodium(){
    // Podium body with bevelled look
    mat(0.38f,0.26f,0.12f,1,50,0.5f);
    glPushMatrix();
      glTranslatef(0,0.55f,0);
      glScalef(0.75f,1.1f,0.52f); glutSolidCube(1);
    glPopMatrix();
    // Podium top surface
    mat(0.48f,0.34f,0.16f,1,70,0.6f);
    glPushMatrix();
      glTranslatef(0,1.12f,0);
      glScalef(0.88f,0.07f,0.65f); glutSolidCube(1);
    glPopMatrix();
    // Front panel
    mat(0.32f,0.20f,0.08f,1,30);
    glPushMatrix();
      glTranslatef(0,0.55f,0.27f);
      glScalef(0.62f,0.85f,0.03f); glutSolidCube(1);
    glPopMatrix();
    // Gold emblem on podium
    glPushMatrix();
      glTranslatef(0,0.65f,0.30f);
      matEmit(0.9f,0.75f,0.1f);
      glScalef(0.12f,0.12f,0.04f);
      glutSolidOctahedron();
      noEmit();
    glPopMatrix();
}

// ══════════════════════════════════════════════════════
//  SCENE FOG
// ══════════════════════════════════════════════════════
static void setFog(float r,float g,float b,float density){
    float col[]={r,g,b,1};
    glEnable(GL_FOG);
    glFogi(GL_FOG_MODE,GL_EXP2);
    glFogfv(GL_FOG_COLOR,col);
    glFogf(GL_FOG_DENSITY,density);
}
static void clearFog(){ glDisable(GL_FOG); }

// ══════════════════════════════════════════════════════
//  3D SCENE SETUP
// ══════════════════════════════════════════════════════
static void setup3D(){
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluPerspective(55.0, (double)WIN_W/WIN_H, 0.1, 200.0);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE,GL_TRUE);
}

// ══════════════════════════════════════════════════════
//  SUBTITLE BAR  (2D overlay)
// ══════════════════════════════════════════════════════
struct Sub { float s,e; const char* txt; };
static Sub subs[] = {
    {0,10,  "THE SILENT SCRIPT"},
    {10,14, "ACT I  —  The Sound of Silence"},
    {14,22, "Aditya — confident, expressive, king of his Kannada-medium school."},
    {22,30, "He was sharp, loud, loved. Words came easily in his mother tongue."},
    {30,40, "Then came the notice: Transfer to a prestigious English-medium school."},
    {40,52, "8th grade. New school. New language. New world. No crown."},
    {52,65, "His confidence, his identity — left behind at the old school gate."},
    {65,75, "The English Speech Competition. His first real test."},
    {75,90, "Aditya spent nights phonetically writing words he didn't understand."},
    {90,105,"He walked onto the stage. The bright lights blurred everything. He froze."},
    {105,120,"40 seconds. Broken sentences. Then — silence. Then — laughter."},
    {120,135,"The laughter of classmates became the soundtrack of his shame."},
    {135,158,"He began to withdraw. Even Maths felt like a foreign language."},
    {158,170,"ACT II  —  The Turning Point"},
    {170,185,"His elder sister found him lost in a Sine-Cosine table. She sat down."},
    {185,200,"She didn't teach formulas — she revealed the logic hidden inside them."},
    {200,210,"Something clicked. Math has rules that never change. Unlike English."},
    {210,220,"His Physics teacher watched him solve what no one else could touch."},
    {220,237,"'You are the only one here who thinks at my frequency.' — the teacher said."},
    {237,250,"That sentence lit a fire that would burn for a lifetime."},
    {250,262,"ACT III  —  The Transformation"},
    {262,275,"The same stage. One year later. A completely different Aditya."},
    {275,285,"He stood tall — he had found his voice, and the trophy proved it."},
    {285,295,"Medals, moments, milestones — all born from one act of courage."},
    {295,300,"ACT IV  —  Full Circle. A student cries over Trigonometry. The Professor smiles..."},
};
static int numSubs = sizeof(subs)/sizeof(subs[0]);

static void drawSubtitle(float t){
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0,WIN_W,0,WIN_H);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    for(int i=0;i<numSubs;i++){
        if(t>=subs[i].s && t<subs[i].e){
            float dur=subs[i].e-subs[i].s;
            float lt=t-subs[i].s;
            float a=c01(lt<0.5f?lt/0.5f:(dur-lt<0.5f?(dur-lt)/0.5f:1.f));
            // Bar
            glColor4f(0,0,0,0.72f*a);
            glBegin(GL_QUADS);
            glVertex2f(0,8); glVertex2f(WIN_W,8);
            glVertex2f(WIN_W,58); glVertex2f(0,58);
            glEnd();
            // Gold accent line
            glColor4f(0.9f,0.72f,0.1f,a*0.9f);
            glBegin(GL_QUADS);
            glVertex2f(0,56); glVertex2f(WIN_W,56);
            glVertex2f(WIN_W,59); glVertex2f(0,59);
            glEnd();
            // Text
            void* font=GLUT_BITMAP_HELVETICA_18;
            float tw=0;
            for(const char* s=subs[i].txt;*s;s++) tw+=glutBitmapWidth(font,*s);
            glColor4f(1,0.96f,0.82f,a);
            glRasterPos2f((WIN_W-tw)/2.f,30.f);
            for(const char* s=subs[i].txt;*s;s++) glutBitmapCharacter(font,*s);
            break;
        }
    }

    // Progress bar (gold)
    glColor4f(0.06f,0.04f,0.04f,0.6f);
    glBegin(GL_QUADS); glVertex2f(0,0); glVertex2f(WIN_W,0);
    glVertex2f(WIN_W,7); glVertex2f(0,7); glEnd();
    glColor4f(0.92f,0.75f,0.06f,1.f);
    glBegin(GL_QUADS); glVertex2f(0,0); glVertex2f(WIN_W*(t/TOTAL),0);
    glVertex2f(WIN_W*(t/TOTAL),7); glVertex2f(0,7); glEnd();

    // Time HUD
    char buf[32];
    int mn=(int)(t/60),sc=(int)fmodf(t,60);
    sprintf(buf,"%d:%02d / 5:00",mn,sc);
    glColor4f(0.82f,0.82f,0.82f,0.85f);
    glRasterPos2f(WIN_W-95.f,13.f);
    for(const char* s=buf;*s;s++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12,*s);

    if(g_paused){
        glColor4f(1,1,1,0.9f);
        const char* pm="  PAUSED  —  SPACE to resume  ";
        float pw=0; for(const char* s=pm;*s;s++) pw+=glutBitmapWidth(GLUT_BITMAP_HELVETICA_18,*s);
        glRasterPos2f((WIN_W-pw)/2.f,(float)WIN_H/2.f);
        for(const char* s=pm;*s;s++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18,*s);
    }

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING);
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW); glPopMatrix();
}

// ══════════════════════════════════════════════════════
//  SCENE 0: TITLE  — starfield + rotating ornament
// ══════════════════════════════════════════════════════
static void sceneTitle(float t) {
    float lt = t - S_TITLE_S;
    float f = sFade(t, S_TITLE_S, S_TITLE_E, 2.0f);

    // 1. Clear with deep space color
    glClearColor(0.02f, 0.01f, 0.08f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    
    setup3D();
    // Camera pushed back slightly to fit the large 1.2f radius torus
    cam = {0.0f, 2.5f, 10.0f, 0.0f, 1.8f, 0.0f, 0.0f, 1.0f, 0.0f};
    applyCamera();

    // 2. Environment
    setFog(0.02f, 0.01f, 0.08f, 0.012f);
    setLight0(6.0f, 10.0f, 6.0f, 0.9f, 0.78f, 0.35f, 0.04f);
    setLight1(-5.0f, 5.0f, -5.0f, 0.25f, 0.25f, 0.75f);

    // 3. Background (Stars) - Draw first, no depth write
    glDepthMask(GL_FALSE);
    drawStars(t, f);
    glDepthMask(GL_TRUE);

    // 4. CENTRAL ORNAMENT
    glPushMatrix();
        glTranslatef(0.0f, 1.8f, 0.0f); // Center of the whole ornament
        float spin = lt * 18.0f;

        // Outer ring (Gold)
        mat(0.9f, 0.72f, 0.08f, 1.0f, 90, 0.9f);
        glPushMatrix();
            glRotatef(spin, 0, 1, 0);
            torus(0.06f, 1.2f, 40, 16);
        glPopMatrix();

        // Mid ring (Blue, Tilted)
        mat(0.2f, 0.45f, 0.92f, 1.0f, 80, 0.8f);
        glPushMatrix();
            glRotatef(-spin * 0.7f, 1, 0, 0);
            glRotatef(40, 0, 0, 1);
            torus(0.045f, 0.85f, 36, 14);
        glPopMatrix();

        // Inner ring (Red)
        mat(0.88f, 0.15f, 0.15f, 1.0f, 80, 0.8f);
        glPushMatrix();
            glRotatef(spin * 1.2f, 0, 0, 1);
            glRotatef(70, 1, 0, 0);
            torus(0.035f, 0.55f, 30, 12);
        glPopMatrix();

        // Centre gem (Emissive)
        matEmit(0.9f, 0.72f, 0.08f);
        glPushMatrix();
            glRotatef(spin * 2.0f, 0, 1, 0);
            glScalef(0.32f, 0.32f, 0.32f);
            glutSolidDodecahedron();
        glPopMatrix();
        noEmit();

        // 5. Orbiting Gems & Particle Spawning
        float gemCols[][3] = {{1,0.8f,0.1f}, {0.2f,0.7f,1}, {1,0.2f,0.5f}, {0.2f,1,0.5f}};
        for(int i = 0; i < 8; i++) {
            float ang = (spin * 0.0174f) + (i * 0.785f);
            float gx = 1.2f * cosf(ang);
            float gy = sinf(ang * 2.0f) * 0.25f;
            float gz = 1.2f * sinf(ang);

            glPushMatrix();
                glTranslatef(gx, gy, gz);
                int ci = i % 4;
                matEmit(gemCols[ci][0], gemCols[ci][1], gemCols[ci][2]);
                sphere(0.09f, 10);
                noEmit();
            glPopMatrix();

            // Spawn particles (using absolute Y by adding the 1.8f center)
            if(fmodf(lt + i * 0.2f, 0.06f) < 0.03f) {
                spawnParticle(gx, 1.8f + gy, gz,
                              randf(-0.1f, 0.1f), randf(0.1f, 0.5f), randf(-0.1f, 0.1f),
                              1.5f, 0.035f, gemCols[ci][0], gemCols[ci][1], gemCols[ci][2]);
            }
        }
    glPopMatrix();

    // 6. Text Billboards (Foreground)
    // Z set to 5.5 to ensure it's comfortably in front of the 1.2f radius rings
    //billboard3D(-2.0f, -0.1f, 5.5f, "THE SILENT SCRIPT", 1.0f, 0.92f, 0.55f, f, GLUT_BITMAP_TIMES_ROMAN_24);
    billboard3D(-1.1f, -0.55f, 5.5f, "A Story of Aditya", 0.7f, 0.7f, 1.0f, f * 0.85f);

    // 7. Transparent Overlays (Particles drawn last)
    glDepthMask(GL_FALSE);
    drawParticles();
    glDepthMask(GL_TRUE);

    clearFog();
}
// ══════════════════════════════════════════════════════
//  SCENE 1: KING  — warm classroom, Aditya confident
// ══════════════════════════════════════════════════════
static void sceneKing(float t) {
    float lt = t - S_KING_S;
    float f = sFade(t, S_KING_S, S_KING_E, 2.0f);

    // 1. Muted, Low-Contrast Sepia Tone
    glClearColor(0.75f, 0.68f, 0.58f, 1.0f); 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    setup3D();

    // --- CAMERA FIX ---
    // Instead of lt * 5.0f (which spins forever), we use a gentle sway
    // This keeps the camera from rotating behind the blackboard.
    float sway = sinf(lt * 0.2f) * 25.0f; // Sways between -25 and +25 degrees
    float er = 7.5f; // Slightly tighter radius
    cam = { er * sinf(sway * 0.0174f), 3.0f, er * cosf(sway * 0.0174f), 
            0.0f, 1.2f, -1.0f, // Looking slightly "into" the board
            0.0f, 1.0f, 0.0f };
    applyCamera();

    // Softer lighting
    setLight0(4, 10, 5, 0.7f, 0.65f, 0.55f, 0.5f); 
    setLight1(-6, 5, 2, 0.2f, 0.2f, 0.25f);
    setFog(0.75f, 0.68f, 0.58f, 0.025f); 

    // 2. The Floor
    drawFloor(16, 16, 0.55f, 0.48f, 0.38f, 12);

    // 3. The Blackboard (Moved slightly forward to Z = -4.5)
    glPushMatrix();
        glTranslatef(0, 2.2f, -4.5f);
        drawBlackboard(9.0f, 4.5f); // Made slightly wider to cover the camera's FOV
        
        // Muted Chalk Ledge
        mat(0.25f, 0.18f, 0.12f, 1, 10); 
        glPushMatrix();
            glTranslatef(0, -2.1f, 0.15f);
            glScalef(9.2f, 0.15f, 0.3f);
            glutSolidCube(1.0f);
        glPopMatrix();
    glPopMatrix();

    // 4. Faded Chalk Writing
    glDisable(GL_LIGHTING);
    const char* lines[] = {"ka  kha  ga  gha", "cha  ja  na  tha", "Math: Logic is King", "Language: Mother Tongue"};
    for(int i = 0; i < 4; i++) {
        glColor4f(0.8f, 0.78f, 0.75f, f * 0.65f); 
        glRasterPos3f(-3.5f, 3.8f - i * 0.65f, -4.2f); // Adjusted to sit on board
        for(const char* s = lines[i]; *s; s++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *s);
    }
    glEnable(GL_LIGHTING);

    // 5. Classroom Desks & Seated Students
    for(int i = 0; i < 6; i++) {
        float dx = -3.5f + (i % 3) * 3.5f; 
        float dz = 0.5f + (i / 3) * 3.0f;  
        
        mat(0.4f, 0.32f, 0.25f, 1, 15); 
        glPushMatrix();
            glTranslatef(dx, 0.9f, dz);
            glScalef(1.6f, 0.1f, 0.9f);
            glutSolidCube(1.0f);
        glPopMatrix();

        mat(0.3f, 0.3f, 0.32f, 1, 5); 
        float lx[4] = {-0.7f, 0.7f, -0.7f, 0.7f};
        float lz[4] = {-0.4f, -0.4f, 0.4f, 0.4f};
        for(int leg = 0; leg < 4; leg++) {
            glPushMatrix();
                glTranslatef(dx + lx[leg], 0, dz + lz[leg]);
                cylinder(0.04f, 0.9f, 12);
            glPopMatrix();
        }

        glPushMatrix();
            glTranslatef(dx, 0, dz + 0.7f);
            glScalef(0.7f, 0.7f, 0.7f);
            drawHuman(0.45f, 0.42f, 0.38f, POSE_STAND, lt + i); 
        glPopMatrix();
    }

    // 6. Aditya (Standing at the front)
    glPushMatrix();
        glTranslatef(0, 0, -2.0f);
        drawHuman(0.3f, 0.35f, 0.5f, POSE_RAISE, lt); 

        // Matte Gold Crown
        glPushMatrix();
            glTranslatef(0, 2.3f, 0);
            glRotatef(lt * 35.0f, 0, 1, 0);
            mat(0.8f, 0.7f, 0.2f, 1.0f, 15, 0.3f); 
            glPushMatrix(); glRotatef(90, 1, 0, 0); torus(0.03f, 0.25f, 32, 12); glPopMatrix();
            for(int i = 0; i < 6; i++) {
                glPushMatrix();
                    glRotatef(i * 60.0f, 0, 1, 0);
                    glTranslatef(0.25f, 0.08f, 0);
                    glScalef(0.07f, 0.3f, 0.07f);
                    glutSolidOctahedron();
                glPopMatrix();
            }
        glPopMatrix();
    glPopMatrix();

    // 7. Soft Particles
    if(fmodf(lt, 0.06f) < 0.03f) {
        spawnParticle(randf(-0.4f, 0.4f), 2.1f, -2.0f + randf(-0.3f, 0.3f), 
                      0, randf(0.2f, 0.6f), 0, 
                      1.5f, 0.04f, 0.8f, 0.75f, 0.5f);
    }

    drawParticles();
    clearFog();
}
// ══════════════════════════════════════════════════════
//  SCENE 2: TRANSFER — imposing building, tiny Aditya
// ══════════════════════════════════════════════════════
static void sceneTransfer(float t) {
    float lt = t - S_TRANS_S;
    float f = sFade(t, S_TRANS_S, S_TRANS_E, 2.f);

    // Muted blue sky as per image
    glClearColor(0.55f, 0.65f, 0.75f, 1.0f); 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    setup3D();

    float zoom = lerp(15.0f, 9.0f, c01(lt / 25.f));
    cam = {0, 3.8f, zoom, 0, 3.0f, -15.0f, 0, 1, 0};
    applyCamera();
    
    // Balanced lighting to prevent the "pure white" washout
    setLight0(10, 20, 5, 0.8f, 0.8f, 0.85f, 0.5f); 
    setFog(0.55f, 0.65f, 0.75f, 0.008f);

    drawFloor(60, 40, 0.95f, 0.95f, 1.0f, 20); // Bright clean pavement

    // ── 1. THE ARCHITECTURAL BASE (Building Walls) ──
    // Main Body (Light Grey/Off-white as per image)
    mat(0.92f, 0.93f, 0.95f, 1, 20);
    glPushMatrix(); 
        glTranslatef(0, 6.0f, -18.0f); 
        glScalef(24.0f, 12.0f, 4.0f); 
        glutSolidCube(1); 
    glPopMatrix();

    // ── 2. ARCHITECTURAL TRIM (The horizontal lines in the image) ──
    mat(0.85f, 0.86f, 0.88f, 1, 10);
    for(int i = 0; i < 3; i++) {
        glPushMatrix();
            glTranslatef(0, 2.5f + i * 3.5f, -15.9f);
            glScalef(24.2f, 0.2f, 0.2f);
            glutSolidCube(1);
        glPopMatrix();
    }

    // ── 3. WINDOWS WITH FRAMES (High contrast as per image) ──
    for(int row = 0; row < 3; row++) {
        for(int col = 0; col < 8; col++) {
            float wx = -8.5f + col * 2.4f;
            float wy = 2.8f + row * 3.2f;
            
            // Window Glass (Dark contrast)
            mat(0.15f, 0.2f, 0.3f, 1, 80);
            glPushMatrix();
                glTranslatef(wx, wy, -15.95f);
                glScalef(1.2f, 1.8f, 0.05f);
                glutSolidCube(1.0f);
            glPopMatrix();

            // Window Frames (White borders)
            mat(1, 1, 1, 1, 10);
            glPushMatrix();
                glTranslatef(wx, wy, -15.98f);
                glScalef(1.35f, 1.95f, 0.02f);
                glutSolidCube(1.0f);
            glPopMatrix();
        }
    }

    // ── 4. THE GRAND COLUMNS (Structural look) ──
    mat(0.95f, 0.95f, 0.98f, 1, 40);
    for(int i = 0; i < 6; i++) {
        glPushMatrix();
          glTranslatef(-6.25f + i * 2.5f, 0, -15.8f);
          cylinder(0.32f, 12.0f, 32); 
          // Capitals (top of columns)
          glTranslatef(0, 12.0f, 0);
          glScalef(0.8f, 0.25f, 0.8f);
          glutSolidCube(1.0f);
        glPopMatrix();
    }

    // ── 5. THE SIGN BOARD (Centered and clear) ──
    mat(1, 1, 1, 1, 30);
    glPushMatrix(); 
        glTranslatef(0, 12.4f, -15.7f); 
        glScalef(10.0f, 0.9f, 0.1f); 
        glutSolidCube(1); 
    glPopMatrix();

    glDisable(GL_LIGHTING);
    const char* schoolName = "ENGLISH MEDIUM SCHOOL";
    glColor4f(0.1f, 0.1f, 0.2f, f); 
    glRasterPos3f(-4.2f, 12.3f, -15.6f); 
    for(const char* s = schoolName; *s; s++) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12, *s);
    }
    glEnable(GL_LIGHTING);

    // ── 6. ADITYA'S WALK (Scale and Position) ──
    float prog = c01(lt / 20.f);
    float ax = lerp(-6.0f, 0.0f, prog);
    float az = lerp(-1.0f, -10.0f, prog);
    
    glPushMatrix();
      glTranslatef(ax, 0, az);
      glScalef(0.7f, 0.7f, 0.7f);
      drawHuman(0.1f, 0.12f, 0.25f, POSE_STAND, lt * 1.5f); // Darker uniform for contrast
    glPopMatrix();

    // Red Question Marks
    for(int i = 0; i < 4; i++) {
        float qa = lt * 2.0f + i * 1.57f;
        billboard3D(ax + cosf(qa)*1.2f, 2.2f + sinf(qa)*0.3f, az + sinf(qa)*0.5f,
                    "?", 0.8f, 0.2f, 0.2f, f);
    }

    clearFog();
}
// ══════════════════════════════════════════════════════
//  SCENE 3: STAGE / SPEECH — dark auditorium, spotlight
// ══════════════════════════════════════════════════════
static void sceneStage(float t){
    float lt = t - S_STAGE_S;
    float f = sFade(t, S_STAGE_S, S_STAGE_E, 2.f);

    glClearColor(0.02f, 0.01f, 0.06f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    setup3D();

    // ── FIXED: CAMERA (SCREEN SHAKING REMOVED) ──
    float cpush = lerp(10.5f, 5.5f, lt / 35.f);
    // Explicitly setting Eye X to 0.0f ensures the screen NEVER shakes
    cam = {0.0f, 3.8f, cpush, 0.0f, 2.4f, 0.0f, 0.0f, 1.0f, 0.0f};
    applyCamera();

    setLight0(0, 16, 2, 1.0f, 0.92f, 0.65f, 0.025f);
    glDisable(GL_LIGHT1);
    setFog(0.02f, 0.01f, 0.06f, 0.032f);

    // ── STAGE GEOMETRY ──
    mat(0.28f, 0.20f, 0.10f, 1, 12);
    glPushMatrix(); glTranslatef(0, 0.06f, -2); glScalef(14, 0.22f, 5.5f); glutSolidCube(1); glPopMatrix();
    mat(0.20f, 0.13f, 0.06f, 1, 18);
    glPushMatrix(); glTranslatef(0, 0.2f, 0.5f); glScalef(14, 0.16f, 0.22f); glutSolidCube(1); glPopMatrix();

    drawPodium();

    // Microphone stand
    mat(0.25f, 0.25f, 0.28f, 1, 80, 0.9f);
    glPushMatrix();
      glTranslatef(0.4f, 1.15f, -2.3f);
      cylinder(0.02f, 0.8f, 12);
      glTranslatef(0, 0.8f, 0);
      sphere(0.06f, 14);
    glPopMatrix();

    // Red curtains
    for(int side = -1; side <= 1; side += 2){
        for(int fold = 0; fold < 5; fold++){
            float fw = 0.5f; float fx = side * (5.2f + fold * fw);
            mat(0.48f + fold % 2 * 0.06f, 0.04f, 0.08f, 1, 6, 0.04f);
            glPushMatrix();
              glTranslatef(fx, 4.5f, -5.8f);
              glScalef(fw, 9, 2.6f); glutSolidCube(1);
            glPopMatrix();
        }
    }

    // Stage floor planks
    for(int p = 0; p < 7; p++){
        mat(0.32f + p % 2 * 0.04f, 0.22f + p % 2 * 0.03f, 0.10f, 1, 15);
        glPushMatrix();
          glTranslatef(-6 + p * 1.8f, 0.18f, -2);
          glScalef(1.75f, 0.04f, 5.5f); glutSolidCube(1);
        glPopMatrix();
    }

    // ── CHARACTER: ADITYA ──
    // He still shakes slightly to show his nervousness, but the screen stays still
    float shake = lt < 15 ? sinf(lt * 14) * 0.06f * (1 - lt / 15) : 0;
    glPushMatrix();
      glTranslatef(shake, 0.17f, -2.5f);
      drawHuman(0.28f, 0.18f, 0.62f, lt < 20 ? POSE_SPEAK : POSE_HUNCH, lt);
    glPopMatrix();

    // Script paper
    glPushMatrix();
      glTranslatef(0.38f, 1.22f, -2.22f);
      glRotatef(28, 0, 0, 1); glRotatef(-22, 1, 0, 0);
      mat(0.96f, 0.94f, 0.84f, 1, 60, 0.25f);
      glScalef(0.28f, 0.38f, 0.022f); glutSolidCube(1);
    glPopMatrix();

    // ── AUDIENCE ──
    glDisable(GL_LIGHT0); glEnable(GL_LIGHT1);
    setLight1(0, 5, 10, 0.03f, 0.02f, 0.05f);
    for(int row = 0; row < 4; row++) {
        for(int col = 0; col < 9; col++){
            float hx = -7 + col * 1.65f, hz = 2.2f + row * 1.55f;
            float hs = 0.62f + row * 0.04f;
            glPushMatrix();
              glTranslatef(hx, 0, hz); glScalef(hs, hs, hs);
              mat(0.06f, 0.04f, 0.09f, 1, 5, 0.04f);
              drawHuman(0.06f, 0.04f, 0.09f, POSE_STAND, 0);
            glPopMatrix();
        }
    }
    glDisable(GL_LIGHT1);
    setLight0(0, 16, 2, 1.0f, 0.92f, 0.65f, 0.025f);

    // ── ALL TIMER, HUD, AND OVAL EFFECTS REMOVED ──
    // Code block for timer and oval effect deleted to ensure they are not visible.

    drawParticles();
    clearFog();
}

// ══════════════════════════════════════════════════════
//  SCENE 4: LAUGHTER — shame walk
// ══════════════════════════════════════════════════════
static void sceneLaugh(float t){
    float lt=t-S_LAUGH_S;
    float f=sFade(t,S_LAUGH_S,S_LAUGH_E,2.f);

    glClearColor(0.01f,0.005f,0.04f,1);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    setup3D();

    float prog=c01(lt/18.f);
    float watchX=lerp(0,-3.5f,prog);
    cam={watchX+2.2f,2.8f,6.5f, watchX,1.3f,0, 0,1,0};
    applyCamera();
    setLight0(watchX+2,10,5, 0.5f,0.4f,0.4f,0.04f);
    setLight1(0,15,0, 0.28f,0.18f,0.10f);
    setFog(0.01f,0.005f,0.04f,0.038f);

    // Stage floor planks
    for(int p=0;p<10;p++){
        mat(0.20f+p%2*0.04f,0.14f+p%2*0.02f,0.08f,1,8);
        glPushMatrix();
          glTranslatef(-9+p*1.8f,0.04f,0);
          glScalef(1.75f,0.06f,9); glutSolidCube(1);
        glPopMatrix();
    }

    // Aditya hunched, walking left
    float ax=lerp(0,-3.5f,prog);
    glPushMatrix();
      glTranslatef(ax,0,0);
      glScalef(0.90f,0.90f,0.90f);
      drawHuman(0.28f,0.18f,0.60f,POSE_HUNCH,lt);
    glPopMatrix();

    // Audience laughing
    for(int row=0;row<3;row++) for(int col=0;col<8;col++){
        float hx=-5.5f+col*1.6f, hz=2.5f+row*1.4f;
        float bop=sinf(lt*3+col*0.7f)*0.12f;
        glPushMatrix();
          glTranslatef(hx,bop,hz);
          glScalef(0.72f,0.72f,0.72f);
          drawHuman(0.58f+col*0.03f,0.40f,0.28f,POSE_STAND,lt+col);
        glPopMatrix();
        float ha_a=0.5f+0.5f*sinf(lt*2+col*0.9f);
        billboard3D(hx,2.55f+bop,hz,"HA!",0.95f,0.78f,0.12f,f*ha_a,GLUT_BITMAP_TIMES_ROMAN_24);
        if(fmodf(lt+col*0.3f,0.28f)<0.14f)
            spawnParticle(hx+randf(-0.3f,0.3f),2.5f,hz,
                          randf(-0.5f,0.5f),randf(1,3),randf(-0.3f,0.3f),
                          1.2f,0.055f,0.95f,0.72f,0.12f);
    }

    // Sound wave rings
    for(int w=0;w<5;w++){
        float ph=fmodf(lt+w*0.4f,2.f)/2.f;
        float r2=ph*10;
        glDisable(GL_LIGHTING);
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE);
        glColor4f(0.95f,0.72f,0.12f,(1-ph)*0.16f*f);
        glPushMatrix();
          glTranslatef(0,1.5f,3);
          glRotatef(90,1,0,0);
          GLUquadric* q=gluNewQuadric();
          gluDisk(q,r2-0.12f,r2,36,1);
          gluDeleteQuadric(q);
        glPopMatrix();
        glDisable(GL_BLEND); glEnable(GL_LIGHTING);
    }

    drawParticles();
    clearFog();
}

// ══════════════════════════════════════════════════════
//  SCENE 5: LOW POINT — night, table, math enemy
// ══════════════════════════════════════════════════════
static void sceneLow(float t){
    float lt = t - S_LOW_S;
    float f = sFade(t, S_LOW_S, S_LOW_E, 2.f);

    glClearColor(0.05f, 0.04f, 0.09f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    setup3D();

    // Camera stays the same, or you can move it to Z=10 to see him better
    cam = {4.0f, 3.5f, 8.0f, 0, 1.2f, 0, 0, 1, 0};
    applyCamera();
    
    // Dim overhead light
    setLight0(0.0f, 5.0f, 0.0f, 0.5f, 0.5f, 0.6f, 0.2f); 
    glDisable(GL_LIGHT1);
    setFog(0.05f, 0.04f, 0.09f, 0.04f);

    // ── 1. THE DESK ──
    mat(0.36f, 0.24f, 0.12f, 1, 38);
    glPushMatrix(); glTranslatef(0, 0.88f, 0); glScalef(4.0f, 0.12f, 2.2f); glutSolidCube(1); glPopMatrix();
    glPushMatrix(); glTranslatef(-1.4f, 0.44f, 0); glScalef(0.8f, 0.88f, 1.8f); glutSolidCube(1); glPopMatrix();
    glPushMatrix(); glTranslatef(0, 0.44f, -0.9f); glScalef(4.0f, 0.88f, 0.1f); glutSolidCube(1); glPopMatrix();
    for(int i=0; i<2; i++){
        float iz = (i==0) ? 0.85f : -0.85f;
        glPushMatrix(); glTranslatef(1.6f, 0, iz); cylinder(0.08f, 0.88f, 16); glPopMatrix();
    }

    // ── 2. THE BOOK & EQUATIONS ──
    // Moved slightly back so it's closer to him on the other side
    mat(0.95f, 0.95f, 0.9f, 1, 10);
    glPushMatrix(); glTranslatef(-0.45f, 0.96f, -0.4f); glScalef(0.8f, 0.04f, 1.0f); glutSolidCube(1); glPopMatrix();
    glPushMatrix(); glTranslatef(0.45f, 0.96f, -0.4f); glScalef(0.8f, 0.04f, 1.0f); glutSolidCube(1); glPopMatrix();
    
    glDisable(GL_LIGHTING);
    // Equations adjusted to follow the book's new Z
    // billboard3D(-0.75f, 1.05f, -0.05f, "sin(x) = ?", 0.2f, 0.2f, 0.2f, f);
    // billboard3D(-0.75f, 1.05f, -0.35f, "cos(x) = ?", 0.2f, 0.2f, 0.2f, f);
    billboard3D(-0.75f, 1.05f, -0.65f, "FAIL", 0.8f, 0.1f, 0.1f, f);
    glEnable(GL_LIGHTING);

    // ── 3. ADITYA (POSITIONED BEHIND THE TABLE) ──
    glPushMatrix();
      // Moved to Z = -1.6f (Behind the back edge of the table)
      glTranslatef(0.0f, 0.1f, -1.6f); 
      glRotatef(180, 0, 1, 0); // Rotate 180 degrees to face the table and camera
      glScalef(0.85f, 0.85f, 0.85f);
      drawHuman(0.4f, 0.1f, 0.1f, POSE_HUNCH, lt);
    glPopMatrix();

    // ── 4. QUESTION MARKS (Directly Above Aditya's Head) ──
    for(int i=0; i<5; i++){
        float qa = lt * 1.5f + i * (6.28f/5.0f);
        float radius = 0.5f;
        float qx = cosf(qa) * radius;
        float qy = 2.8f + sinf(qa * 0.5f) * 0.2f;
        // Z updated to -1.6 to follow Aditya's new position
        float qz = -1.6f + sinf(qa) * radius; 
        billboard3D(qx, qy, qz, "?", 0.7f, 0.7f, 0.9f, f * 0.7f, GLUT_BITMAP_TIMES_ROMAN_24);
    }

    drawParticles();
    clearFog();
}
// ══════════════════════════════════════════════════════
//  SCENE 6: SISTER — unit circle, lightbulb moment
// ══════════════════════════════════════════════════════
static void sceneSister(float t) {
    float lt = t - S_SIS_S;
    float f = sFade(t, S_SIS_S, S_SIS_E, 2.0f);

    // Warm, encouraging home atmosphere
    glClearColor(0.80f, 0.72f, 0.60f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    setup3D();

    // Panning camera to show both characters
    float camX = lerp(-2.5f, 0.5f, c01(lt / 20.0f));
    cam = {camX, 3.5f, 8.5f, 0, 1.8f, 0, 0, 1, 0};
    applyCamera();
    
    setLight0(4.0f, 10.0f, 6.0f, 1.0f, 0.95f, 0.8f, 0.4f);
    setLight1(-5.0f, 6.0f, 3.0f, 0.5f, 0.4f, 0.3f);
    setFog(0.80f, 0.72f, 0.60f, 0.015f);

    drawFloor(16, 14, 0.65f, 0.55f, 0.45f, 12);

    // ── 1. ENHANCED TABLE ──
    mat(0.45f, 0.30f, 0.15f, 1, 40); // Rich wood
    // Table Top
    glPushMatrix(); 
        glTranslatef(0, 1.0f, 0); 
        glScalef(4.5f, 0.12f, 2.5f); 
        glutSolidCube(1.0f); 
    glPopMatrix();
    
    // Support Frame (under the table top)
    mat(0.35f, 0.22f, 0.10f, 1, 20);
    glPushMatrix(); 
        glTranslatef(0, 0.85f, 0); 
        glScalef(4.0f, 0.2f, 2.0f); 
        glutSolidCube(1.0f); 
    glPopMatrix();

    // Table Legs (Properly aligned to the frame)
    for(int i = 0; i < 4; i++) {
        float lx = (i % 2 == 0) ? 1.9f : -1.9f;
        float lz = (i < 2) ? 1.0f : -1.0f;
        glPushMatrix(); 
            glTranslatef(lx, 0, lz);
            cylinder(0.08f, 1.0f, 18); // Sturdier legs
        glPopMatrix();
    }

    // ── 2. CHARACTERS (Standing apart from the table) ──
    // Sister (Right side, slightly back)
    glPushMatrix();
      glTranslatef(2.5f, 0, 0.8f); // Moved out from X=1.9 and back to Z=0.8
      glRotatef(-35, 0, 1, 0);     // Facing inward
      drawHuman(0.85f, 0.25f, 0.55f, POSE_SPEAK, lt, 0.26f); // Pink saree
    glPopMatrix();

    // Aditya (Left side, standing attentively)
    glPushMatrix();
      glTranslatef(-2.5f, 0, 0.8f); // Moved out from X=-1.9 and back to Z=0.8
      glRotatef(35, 0, 1, 0);      // Facing inward
      drawHuman(0.2f, 0.25f, 0.5f, POSE_STAND, lt * 0.3f);
    glPopMatrix();

    // ── 3. 3D UNIT CIRCLE (Teaching Visual) ──
    float angle = fmodf(lt * 1.0f, 6.28f);
    glPushMatrix();
      glTranslatef(0, 2.5f, -0.8f); // Floating higher above the table center
      glRotatef(lt * 5.0f, 0, 1, 0);
      glScalef(1.4f, 1.4f, 1.4f);

      // Circle Ring
      mat(0.2f, 0.5f, 1.0f, 0.8f, 80);
      glPushMatrix(); glRotatef(90, 1, 0, 0); torus(0.02f, 0.8f, 60, 16); glPopMatrix();

      // Axes & Trigonometry logic
      glDisable(GL_LIGHTING);
      glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE);
      glColor4f(1, 1, 1, f * 0.5f);
      glBegin(GL_LINES);
          glVertex3f(-0.9f, 0, 0); glVertex3f(0.9f, 0, 0);
          glVertex3f(0, -0.9f, 0); glVertex3f(0, 0.9f, 0);
      glEnd();

      float px = cosf(angle) * 0.8f, py = sinf(angle) * 0.8f;
      // Radius
      glColor4f(1, 0.8f, 0.2f, f);
      glBegin(GL_LINES); glVertex3f(0,0,0); glVertex3f(px, py, 0); glEnd();
      // Sine line (Green)
      glColor4f(0.2f, 1.0f, 0.4f, f);
      glBegin(GL_LINES); glVertex3f(px, 0, 0); glVertex3f(px, py, 0); glEnd();
      // Cosine line (Red)
      glColor4f(1.0f, 0.3f, 0.3f, f);
      glBegin(GL_LINES); glVertex3f(0, 0, 0); glVertex3f(px, 0, 0); glEnd();
      
      glDisable(GL_BLEND); glEnable(GL_LIGHTING);

      matEmit(1.0f, 0.8f, 0.2f);
      glPushMatrix(); glTranslatef(px, py, 0); sphere(0.06f, 12); glPopMatrix();
      noEmit();
    glPopMatrix();

    // ── 4. LIGHTBULB "CLICK" EFFECT ──
    if(lt > 10) {
        float gf = c01((lt - 10) / 4.0f);
        float pulse = 0.1f * sinf(lt * 4.0f);
        glPushMatrix();
            glTranslatef(0, 4.2f, 0);
            matEmit(1.0f * gf, 0.9f * gf, 0.4f * gf);
            sphere(0.25f + pulse, 20); // The "Idea" bulb
            
            // Text Billboard
            billboard3D(-0.8f, 0.6f, 0.5f, "IT CLICKS!", 1, 0.9f, 0.1f, gf * f, GLUT_BITMAP_TIMES_ROMAN_24);
        glPopMatrix();
        noEmit();

        if(fmodf(lt, 0.04f) < 0.02f)
            spawnParticle(randf(-0.2f, 0.2f), 4.0f, randf(-0.2f, 0.2f), 
                          randf(-0.5f, 0.5f), randf(0.5f, 1.5f), randf(-0.5f, 0.5f), 
                          1.2f, 0.04f, 1.0f, 0.9f, 0.2f);
    }

    drawParticles();
    clearFog();
}
// ══════════════════════════════════════════════════════
//  SCENE 7: MATH AWAKENING — matrix rain, glowing triangle
// ══════════════════════════════════════════════════════
static void sceneMath(float t){
    float lt = t - S_MATH_S;
    float f = sFade(t, S_MATH_S, S_MATH_E, 2.f);

    // Deep cosmic blue background
    glClearColor(0.01f, 0.02f, 0.1f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    setup3D();

    // Cinematic spiraling camera
    float camDist = 8.0f - lt * 0.05f;
    cam = {sinf(lt * 0.4f) * 3.5f, 3.5f + sinf(lt * 0.2f), camDist, 0, 2.5f, 0, 0, 1, 0};
    applyCamera();
    
    setLight0(0, 10, 5.0f, 0.2f, 1.0f, 0.5f, 0.05f);
    setFog(0.01f, 0.02f, 0.1f, 0.025f);

    // ── 1. THE MATH VORTEX (Enhanced Matrix Rain) ──
    const char* syms[] = {"sin", "cos", "tan", "pi", "th", "=", "dy/dx", "lim", "sqrt", "sum", "8", "inf"};
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE);
    for(int i = 0; i < 80; i++) {
        srand(i * 123); // Consistent randomness per stream
        float rad = randf(4.0f, 9.0f);
        float ang = randf(0, 6.28f) + lt * 0.5f;
        float ex = cosf(ang) * rad;
        float ez = sinf(ang) * rad - 5.0f;
        float ey = fmodf(lt * 3.0f + randf(0, 15), 15.0f) - 3.0f;
        
        glColor4f(0.2f, 0.8f, 1.0f, 0.4f * f);
        glRasterPos3f(ex, ey, ez);
        const char* sym = syms[i % 12];
        for(; *sym; sym++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_10, *sym);
    }
    glDisable(GL_BLEND); glEnable(GL_LIGHTING);

    // ── 2. THE GEOMETRIC TRINITY (Rotating Triangle) ──
    float angRot = lt * 45.0f; // Faster, more energetic rotation
    float pulse = 0.15f * sinf(lt * 4.0f); // Pulsing effect

    glPushMatrix();
      glTranslatef(0, 2.8f, 0);
      glRotatef(angRot, 0, 1, 0); // Triangle rotates on Y axis
      glScalef(1.0f + pulse, 1.0f + pulse, 1.0f + pulse);

      // Triangle Face (Holographic Blue)
      mat(0.1f, 0.5f, 1.0f, 0.3f, 60);
      glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE);
      glBegin(GL_TRIANGLES);
        glNormal3f(0, 0, 1);
        glVertex3f(-1.5f, -1.0f, 0); glVertex3f(1.5f, -1.0f, 0); glVertex3f(-1.5f, 1.5f, 0);
      glEnd();

      // Glowing Edges
      glDisable(GL_LIGHTING);
      glColor4f(0.4f, 0.8f, 1.0f, f);
      glLineWidth(4.0f);
      glBegin(GL_LINE_LOOP);
        glVertex3f(-1.5f, -1.0f, 0); glVertex3f(1.5f, -1.0f, 0); glVertex3f(-1.5f, 1.5f, 0);
      glEnd();

      // --- LABELS (Now locked to triangle rotation) ---
      // Cosine Label (Bottom edge)
      glColor4f(0.3f, 1.0f, 0.5f, f);
      glRasterPos3f(0.0f, -1.4f, 0.1f);
      for(const char* s : {"cos(theta)"}) for(const char* c = s; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

      // Sine Label (Vertical edge)
      glColor4f(1.0f, 0.4f, 0.4f, f);
      glRasterPos3f(-2.4f, 0.2f, 0.1f);
      for(const char* s : {"sin(theta)"}) for(const char* c = s; *c; c++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *c);

      // Hypotenuse (The '1')
      glColor4f(1.0f, 0.9f, 0.2f, f);
      glRasterPos3f(0.4f, 0.5f, 0.1f);
      glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24, '1');

      glEnable(GL_LIGHTING); glDisable(GL_BLEND);
    glPopMatrix();

    // ── 3. ADITYA'S ASCENSION ──
    glPushMatrix();
      glTranslatef(3.5f, 0.5f, 0);
      drawHuman(0.2f, 0.3f, 0.8f, POSE_RAISE, lt);
      
      // Floating Halo
      matEmit(1.0f, 0.9f, 0.2f);
      glTranslatef(0, 2.2f, 0);
      glPushMatrix(); 
        glRotatef(lt * 180, 0, 1, 0); 
        glRotatef(90, 1, 0, 0);
        torus(0.04f, 0.45f, 30, 12); 
      glPopMatrix();
      noEmit();
    glPopMatrix();

    // ── 4. THE "CLICK" REVELATION ──
    if(lt > 7.0f) {
        float cf = c01((lt - 7.0f) / 2.0f);
        // Larger, clearer billboard
        billboard3D(-1.2f, 5.5f, 1.0f, "THE LOGIC CLICKS!", 1.0f, 1.0f, 1.0f, cf * f, GLUT_BITMAP_TIMES_ROMAN_24);
        
        // Burst of particles when it clicks
        if(fmodf(lt, 0.03f) < 0.015f)
            spawnParticle(3.5f, 2.5f, 0, randf(-2, 2), randf(2, 5), randf(-1, 1), 1.0f, 0.05f, 1, 0.9f, 0.2f);
    }

    drawParticles();
    clearFog();
}
// ══════════════════════════════════════════════════════
//  SCENE 8: PHYSICS — board, derivation, compliment
// ══════════════════════════════════════════════════════
static void scenePhysics(float t){
    float lt = t - S_PHYS_S;
    float f = sFade(t, S_PHYS_S, S_PHYS_E, 2.0f);

    glClearColor(0.08f, 0.08f, 0.12f, 1.0f); 
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    setup3D();

    cam = {0.0f, 3.2f, 8.5f, 0.0f, 2.2f, -2.5f, 0.0f, 1.0f, 0.0f};
    applyCamera();

    setLight0(3.5f, 8.5f, 5.5f, 0.6f, 0.55f, 0.45f, 0.15f); 
    setLight1(-4.5f, 6.5f, 4.5f, 0.15f, 0.15f, 0.25f);
    setFog(0.08f, 0.08f, 0.12f, 0.02f); 

    drawFloor(16, 12, 0.25f, 0.22f, 0.20f, 12);

    glPushMatrix(); 
        glTranslatef(0.0f, 3.2f, -5.5f); 
        drawBlackboard(11, 5.5f); 
    glPopMatrix();

    // ── 4. BLACKBOARD FORMULAS (CHANGED TO BLACK) ──
    const char* lines[] = {
        "F = m . a",
        "W = F . ds",
        "KE = (1/2) m v^2",
        "Delta KE = W",
        "P = F . v"
    };
    glDisable(GL_LIGHTING);
    for(int i = 0; i < 5; i++){
        if(lt > i * 4.5f){
            float lf = c01((lt - i * 4.5f) / 2.2f);
            // SET TO BLACK: (0, 0, 0)
            glColor4f(0.0f, 0.0f, 0.0f, f * lf);
            glRasterPos3f(-5.0f, 5.2f - i * 0.85f, -5.25f);
            for(const char* s = lines[i]; *s; s++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *s);
        }
    }
    glEnable(GL_LIGHTING);

    // Characters
    glPushMatrix();
      glTranslatef(-1.2f, 0, -4.0f);
      glRotatef(12, 0, 1, 0);
      drawHuman(0.2f, 0.25f, 0.45f, POSE_WRITE, lt);
    glPopMatrix();

    glPushMatrix();
      glTranslatef(3.8f, 0, -1.8f);
      glRotatef(-28, 0, 1, 0);
      drawHuman(0.35f, 0.35f, 0.35f, POSE_STAND, 0);
    glPopMatrix();

    // // ── 6. SPEECH BUBBLE (CHANGED TEXT TO BLACK) ──
    // if(lt > 10){
    //     float sf = c01((lt - 10) / 3.2f) * f;
    //     // Bubble needs to be light enough to see black text
    //     mat(0.85f, 0.85f, 0.88f, sf * 0.9f, 5, 0.1f);
    //     glPushMatrix(); glTranslatef(1.8f, 4.1f, -1.0f); glScalef(4.0f, 1.2f, 0.1f); glutSolidCube(1); glPopMatrix();
        
    //     glPushMatrix(); glTranslatef(2.3f, 3.4f, -0.92f); glRotatef(-22, 0, 0, 1);
    //       glScalef(0.2f, 0.5f, 0.08f); glutSolidCube(1);
    //     glPopMatrix();

    //     glDisable(GL_LIGHTING);
    //     // SET TO BLACK: (0, 0, 0)
    //     billboard3D(0.35f, 4.25f, -0.88f, "\"You're the only one here", 0.0f, 0.0f, 0.0f, sf, GLUT_BITMAP_HELVETICA_12);
    //     billboard3D(0.35f, 3.95f, -0.88f, "who thinks at my frequency.\"", 0.0f, 0.0f, 0.0f, sf, GLUT_BITMAP_HELVETICA_12);
    //     glEnable(GL_LIGHTING);

    //     if(fmodf(lt, 0.05f) < 0.025f && sf > 0.5f)
    //         spawnParticle(-1.2f + randf(-0.3f, 0.3f), 2.6f, -4.0f,
    //                       randf(-0.5f, 0.5f), randf(0.5f, 1.5f), randf(-0.5f, 0.5f),
    //                       1.2f, 0.04f, 1.0f, 0.9f, 0.2f);
    // }

    drawParticles();
    clearFog();
}

// ══════════════════════════════════════════════════════
//  SCENE 9: COMEBACK — triumphant speech
// ══════════════════════════════════════════════════════
static void sceneComeback(float t){
    float lt=t-S_COME_S;
    float f=sFade(t,S_COME_S,S_COME_E,2.f);

    glClearColor(0.02f,0.01f,0.08f,1);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    setup3D();

    float cpush=lerp(10.5f,6.5f,lt/22.f);
    cam={sinf(lt*0.2f)*0.55f,3.8f,cpush, 0,2.2f,0, 0,1,0};
    applyCamera();

    setLight0(0,16,3.5f, 1.f,0.90f,0.58f,0.04f);
    glDisable(GL_LIGHT1);
    setFog(0.02f,0.01f,0.08f,0.022f);

    // Stage floor planks
    for(int p=0;p<8;p++){
        mat(0.30f+p%2*0.04f,0.22f+p%2*0.03f,0.10f,1,18);
        glPushMatrix();
          glTranslatef(-6+p*1.5f,0.07f,-2);
          glScalef(1.45f,0.22f,5.5f); glutSolidCube(1);
        glPopMatrix();
    }

    // Podium
    glPushMatrix(); glTranslatef(0,0,-2.5f); drawPodium(); glPopMatrix();

    // Red curtains
    mat(0.45f,0.04f,0.08f,1,6,0.04f);
    for(int side=-1;side<=1;side+=2){
        for(int fold=0;fold<5;fold++){
            glPushMatrix(); glTranslatef(side*(5.5f+fold*0.55f),4.5f,-5.2f);
              mat(0.40f+fold%2*0.08f,0.03f,0.06f,1,6);
              glScalef(0.45f,9,2.8f); glutSolidCube(1);
            glPopMatrix();
        }
    }

    // Aditya (confident stance, triumphant)
    glPushMatrix();
      glTranslatef(0,0.18f,-2.5f);
      drawHuman(0.14f,0.14f,0.38f,POSE_SPEAK,lt*2.f,0.30f,52);
    glPopMatrix();

    // Audience (silent, still)
    setLight1(0,5,10, 0.04f,0.035f,0.07f);
    for(int row=0;row<3;row++) for(int col=0;col<10;col++){
        float hx=-7.2f+col*1.55f, hz=2.2f+row*1.45f;
        float lean=sinf(lt*0.14f+col*0.22f)*0.025f;
        glPushMatrix();
          glTranslatef(hx,0,hz); glRotatef(lean*57.3f,0,0,1); glScalef(0.64f,0.64f,0.64f);
          mat(0.05f,0.04f,0.09f,1,5,0.04f);
          drawHuman(0.05f,0.04f,0.09f,POSE_STAND,0);
        glPopMatrix();
    }
    glDisable(GL_LIGHT1);

    // Trophy appears after 15s
    if(lt>15){
        float tf=c01((lt-15)/4.f)*f;
        glPushMatrix();
          glTranslatef(2.8f,2.2f,-1.5f);
          glScalef(tf,tf,tf);
          drawTrophy(lt);
        glPopMatrix();

        // Confetti burst
        if(fmodf(lt,0.045f)<0.023f){
            float confCols[][3]={{1,0.8f,0.1f},{0.1f,0.8f,1},{1,0.2f,0.5f},{0.3f,1,0.3f},{1,0.5f,0.1f}};
            int ci=rand()%5;
            spawnParticle(randf(-4,4),randf(0.5f,2.0f),randf(-5,-1),
                          randf(-2.5f,2.5f),randf(2.5f,7),randf(-1.5f,1.5f),
                          1.8f,0.065f,confCols[ci][0],confCols[ci][1],confCols[ci][2]);
        }
    }

    drawParticles();
    clearFog();
}

// ══════════════════════════════════════════════════════
//  SCENE 10: CELEBRATION 
//  Enhanced stage: Fireworks, medal, dancing figure,
//  spinning trophies, light beams
// ══════════════════════════════════════════════════════
static void sceneCelebration(float t){
    float lt=t-S_CELEB_S;
    float f=sFade(t,S_CELEB_S,S_CELEB_E,2.f);

    glClearColor(0.04f,0.01f,0.12f,1);
    glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
    setup3D();

    // Slow orbit camera
    float camAng=lt*5.5f;
    float cr=9.5f;
    cam={cr*sinf(camAng*0.0174f),4.0f,cr*cosf(camAng*0.0174f),
         0,2.2f,0, 0,1,0};
    applyCamera();

    // Festive multi-colour lights
    setLight0(sinf(lt*0.7f)*5,9,cosf(lt*0.7f)*5, 1.f,0.78f,0.22f, 0.08f);
    setLight1(-4,7,4, 0.55f,0.12f,0.72f);
    setFog(0.04f,0.01f,0.12f,0.018f);

    // Stage floor with reflective tiles
    drawFloor(14,12, 0.22f,0.10f,0.32f, 12);

    // ── LIGHT BEAM PILLARS (cones from floor upward) ──
    float beamCols[][3]={{1,0.8f,0.1f},{0.1f,0.6f,1},{1,0.1f,0.7f},{0.1f,1,0.4f}};
    for(int i=0;i<4;i++){
        float ba=lt*30.f+i*90.f;
        float bx=3.5f*sinf(ba*0.0174f+i*1.57f);
        float bz=3.5f*cosf(ba*0.0174f+i*1.57f);
        glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE);
        glDisable(GL_LIGHTING);
        glColor4f(beamCols[i][0],beamCols[i][1],beamCols[i][2],0.12f*f);
        glPushMatrix();
          glTranslatef(bx,0,bz);
          cone(0.5f,12.f,16);
        glPopMatrix();
        glDisable(GL_BLEND); glEnable(GL_LIGHTING);
    }

    // ── CENTRE: ADITYA DANCING WITH MEDAL ──
    glPushMatrix();
      glTranslatef(0,0,0);
      drawHuman(0.22f,0.18f,0.55f,POSE_DANCE,lt*1.5f,0.30f,50);

      // Gold medal on chest
      glPushMatrix();
        glTranslatef(0.05f,1.35f,0.18f);
        glRotatef(sinf(lt*2)*10,0,0,1);
        // Ribbon
        mat(0.22f,0.22f,0.85f,1,20);
        glScalef(0.08f,0.22f,0.015f); glutSolidCube(1);
        glTranslatef(0,-1,0);
        // Medal disc
        mat(1.f,0.82f,0.08f,1,100,0.95f);
        glScalef(1.8f,0.35f,3.0f);
        sphere(0.12f,16);
      glPopMatrix();

      // Victory sparkle halo
      float glow=0.55f+0.45f*sinf(lt*2.8f);
      matEmit(glow*0.9f,glow*0.75f,glow*0.2f);
      glPushMatrix();
        glTranslatef(0,2.3f,0);
        glRotatef(lt*110,0,1,0);
        torus(0.04f,0.5f,28,12);
      glPopMatrix();
      glPushMatrix();
        glTranslatef(0,2.3f,0);
        glRotatef(-lt*85,1,0,0);
        torus(0.03f,0.35f,22,10);
      glPopMatrix();
      noEmit();
    glPopMatrix();

    // ── 3 ORBITING TROPHIES ──
    for(int i=0;i<3;i++){
        float tAng=lt*40.f+i*120.f;
        float tr=4.2f;
        float tx=tr*sinf(tAng*0.0174f);
        float tz=tr*cosf(tAng*0.0174f);
        float ty=1.5f+sinf(lt*1.5f+i)*0.4f;
        glPushMatrix();
          glTranslatef(tx,ty,tz);
          glRotatef(-tAng,0,1,0);
          drawTrophy(lt+i*2.f,0.55f);
        glPopMatrix();
    }

    // ── FIREWORK ROCKETS → BURSTS ──
    struct FW { float x,y,z,timer; int color; };
    static FW fws[8]={};
    static bool fwInit=false;
    if(!fwInit){
        for(int i=0;i<8;i++){
            fws[i]={randf(-6,6),0,randf(-6,0),(float)(i*0.3f),i%5};
        }
        fwInit=true;
    }
    for(int i=0;i<8;i++){
        float phase=fmodf(lt+fws[i].timer,3.5f);
        float cols[][3]={{1,0.85f,0.1f},{0.2f,0.7f,1},{1,0.2f,0.6f},
                         {0.2f,1,0.4f},{1,0.5f,0.1f}};
        int ci=fws[i].color%5;
        if(phase<1.5f){
            // rocket ascending
            float ry=phase*5.5f;
            glPushMatrix();
              glTranslatef(fws[i].x,ry,fws[i].z);
              matEmit(cols[ci][0],cols[ci][1],cols[ci][2]);
              sphere(0.08f,8);
              noEmit();
            glPopMatrix();
            // exhaust trail
            if(fmodf(lt,0.04f)<0.02f)
                spawnParticle(fws[i].x,ry,fws[i].z,
                              randf(-0.15f,0.15f),-randf(0.5f,1.5f),randf(-0.15f,0.15f),
                              0.5f,0.035f,0.9f,0.5f,0.1f);
        } else if(phase<2.0f){
            // burst!
            float burst=phase-1.5f;
            if(burst<0.1f && fmodf(lt,0.04f)<0.02f){
                float by=1.5f*5.5f;
                for(int b=0;b<18;b++){
                    float ba=b*0.349f;
                    float bv=randf(2.5f,5.5f);
                    spawnParticle(fws[i].x,by,fws[i].z,
                                  cosf(ba)*bv,sinf(ba*2)*bv+1,sinf(ba)*bv,
                                  1.8f,0.07f,cols[ci][0],cols[ci][1],cols[ci][2]);
                }
            }
        }
    }

    // ── BANNER TEXT ──
    float rp=pulse(lt,1.8f);
    billboard3D(-1.8f,6.5f,0,"CONGRATULATIONS!",1,0.88f,0.12f,rp*f,GLUT_BITMAP_TIMES_ROMAN_24);

    // ── STARS in background ──
    drawStars(t,f*0.7f);

    drawParticles();
    clearFog();
}

// ══════════════════════════════════════════════════════
//  SCENE 11: MENTOR — professor + crying student
// ══════════════════════════════════════════════════════
static void sceneMentor(float t){
    float lt = t - S_MENT_S;
    float f = sFade(t, S_MENT_S, S_MENT_E, 2.f);

    // Warm, sepia-toned academic atmosphere
    glClearColor(0.82f, 0.78f, 0.70f, 1);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    setup3D();

    // Slow, empathetic camera orbit
    float camAng = lt * 2.5f; 
    float cr = 8.5f;
    cam = {cr * sinf(camAng * 0.0174f), 3.4f, cr * cosf(camAng * 0.0174f), 0, 1.8f, 0, 0, 1, 0};
    applyCamera();
    
    // Golden lighting representing wisdom and hope
    setLight0(4.5f, 8.5f, 4.5f, 1.0f, 0.95f, 0.75f, 0.4f);
    setLight1(-4.5f, 6.5f, 3.5f, 0.4f, 0.35f, 0.3f);
    setFog(0.82f, 0.78f, 0.70f, 0.015f);

    drawFloor(16, 12, 0.65f, 0.58f, 0.48f, 12);

    // ── 1. THE BLACKBOARD (The past and present) ──
    glPushMatrix(); glTranslatef(0, 3.2f, -5.5f); drawBlackboard(11, 4.5f); glPopMatrix();
    glDisable(GL_LIGHTING);
    glColor4f(0.9f, 0.9f, 0.85f, f);
    billboard3D(-4.8f, 4.5f, -5.2f, "sin^2(x) + cos^2(x) = 1", 0.1f, 0.1f, 0.1f, f);
    billboard3D(-4.8f, 3.65f, -5.2f, "tan(x) = sin(x) / cos(x)", 0.1f, 0.1f, 0.1f, f);
    billboard3D(-4.8f, 2.85f, -5.2f, "Knowledge is a Circle", 0.1f, 0.1f, 0.1f, f);
    glEnable(GL_LIGHTING);

    // ── 2. THE STRUGGLING STUDENT (Hunched & Crying) ──
    mat(0.44f, 0.29f, 0.13f, 1, 22);
    glPushMatrix(); glTranslatef(-2.2f, 0.92f, 1.6f); glScalef(1.8f, 0.10f, 1.1f); glutSolidCube(1); glPopMatrix();
    
    glPushMatrix();
      glTranslatef(-2.2f, 0, 1.6f);
      glRotatef(18, 0, 1, 0); 
      // Subtle shoulder shaking animation
      float sob = sinf(lt * 12.0f) * 0.02f * f;
      glTranslatef(0, sob, 0);
      drawHuman(0.3f, 0.5f, 0.8f, POSE_HUNCH, lt); 
    glPopMatrix();

    // Tears (Refined drip logic)
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    for(int i=0; i<3; i++){
        float drip = fmodf(lt * 0.8f + i * 0.5f, 1.5f);
        glColor4f(0.5f, 0.7f, 1.0f, f * (1.0f - drip/1.5f));
        glPushMatrix(); 
            glTranslatef(-2.0f, 1.45f - drip * 0.4f, 1.7f);
            sphere(0.03f, 8); 
        glPopMatrix();
    }
    glDisable(GL_BLEND); glEnable(GL_LIGHTING);

    // ── 3. PROFESSOR ADITYA (Mentoring Presence) ──
    glPushMatrix();
      // Positioned close to the student, hand almost on shoulder
      glTranslatef(-1.2f, 0, 1.5f); 
      glRotatef(-60, 0, 1, 0); 
      drawHuman(0.2f, 0.2f, 0.3f, POSE_SPEAK, lt, 0.32f, 65); // Older Aditya (grey hair)
    glPopMatrix();

    // ── 4. SPEECH BUBBLE (The Revelation) ──
    if(lt > 3.5f){
        float sf = c01((lt - 3.5f) / 2.5f) * f;
        mat(1, 1, 1, sf * 0.95f, 10);
        glPushMatrix(); glTranslatef(-0.5f, 4.4f, 0.5f); glScalef(5.2f, 1.3f, 0.1f); glutSolidCube(1); glPopMatrix();
        
        glDisable(GL_LIGHTING);
        glColor4f(0.1f, 0.1f, 0.2f, sf);
        billboard3D(-2.8f, 4.7f, 0.6f, "\"I was once exactly where you are.", 0.08f, 0.08f, 0.3f, sf);
        billboard3D(-2.8f, 4.3f, 0.6f, "I couldn't even stand on a stage...", 0.08f, 0.08f, 0.3f, sf);
        billboard3D(-2.8f, 3.9f, 0.6f, "Believe me, knowledge is the cure for fear.\"", 0.08f, 0.08f, 0.3f, sf);
        glEnable(GL_LIGHTING);
    }

    // ── 5. THE UNIT CIRCLE (Symbol of Completion) ──
    if(lt > 6.0f){
        float cf = c01((lt - 6.0f) / 4.0f) * f;
        glPushMatrix();
          glTranslatef(3.5f, 3.0f, -1.0f);
          glRotatef(lt * 20.0f, 0, 1, 0);
          
          // Golden Ring
          mat(0.9f, 0.8f, 0.2f, cf * 0.9f, 100);
          glPushMatrix(); glRotatef(90, 1, 0, 0); torus(0.03f, 0.9f, 60, 16); glPopMatrix();
          
          matEmit(1.0f, 0.8f, 0.1f * cf);
          glPushMatrix(); 
            glRotatef(lt * 50.0f, 0, 0, 1); 
            glTranslatef(0.9f, 0, 0); 
            sphere(0.1f, 12); 
          glPopMatrix();
          noEmit();
          
          billboard3D(-0.6f, -1.3f, 0, "FULL CIRCLE", 1, 0.8f, 0.2f, cf, GLUT_BITMAP_HELVETICA_12);
        glPopMatrix();
    }

    // Warm embers of inspiration
    if(fmodf(lt, 0.06f) < 0.03f)
        spawnParticle(randf(-4, 4), 0.5f, randf(-3, 3), 0, randf(1, 2), 0, 2.0f, 0.04f, 1.0f, 0.85f, 0.3f);

    drawParticles();
    clearFog();
}
// ══════════════════════════════════════════════════════
//  END CARD
// ══════════════════════════════════════════════════════
static void drawEndCard(float t){
    float ef=c01((t-(TOTAL-3.f))/3.f);
    if(ef<=0) return;
    glMatrixMode(GL_PROJECTION); glPushMatrix(); glLoadIdentity();
    gluOrtho2D(0,WIN_W,0,WIN_H);
    glMatrixMode(GL_MODELVIEW); glPushMatrix(); glLoadIdentity();
    glDisable(GL_LIGHTING); glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND); glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    glColor4f(0,0,0,ef*0.88f);
    glBegin(GL_QUADS);
    glVertex2f(0,0); glVertex2f(WIN_W,0);
    glVertex2f(WIN_W,WIN_H); glVertex2f(0,WIN_H);
    glEnd();

    glColor4f(1.f,0.94f,0.60f,ef);
    const char* t1="THE SILENT SCRIPT";
    float tw=0; for(const char*s=t1;*s;s++) tw+=glutBitmapWidth(GLUT_BITMAP_TIMES_ROMAN_24,*s);
    glRasterPos2f((WIN_W-tw)/2.f,(float)WIN_H/2.f+44.f);
    for(const char*s=t1;*s;s++) glutBitmapCharacter(GLUT_BITMAP_TIMES_ROMAN_24,*s);

    glColor4f(0.75f,0.75f,1.0f,ef*0.85f);
    const char* t2="For every student who once froze on a stage.";
    float tw2=0; for(const char*s=t2;*s;s++) tw2+=glutBitmapWidth(GLUT_BITMAP_HELVETICA_18,*s);
    glRasterPos2f((WIN_W-tw2)/2.f,(float)WIN_H/2.f+8.f);
    for(const char*s=t2;*s;s++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18,*s);

    glColor4f(0.92f,0.72f,0.12f,ef*0.65f);
    const char* t3="\"The language of the heart needs no translation.\"";
    float tw3=0; for(const char*s=t3;*s;s++) tw3+=glutBitmapWidth(GLUT_BITMAP_HELVETICA_12,*s);
    glRasterPos2f((WIN_W-tw3)/2.f,(float)WIN_H/2.f-30.f);
    for(const char*s=t3;*s;s++) glutBitmapCharacter(GLUT_BITMAP_HELVETICA_12,*s);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST); glEnable(GL_LIGHTING);
    glMatrixMode(GL_PROJECTION); glPopMatrix();
    glMatrixMode(GL_MODELVIEW); glPopMatrix();
}

// ══════════════════════════════════════════════════════
//  GLUT CALLBACKS
// ══════════════════════════════════════════════════════
static void display(){
    float t=g_time;

    if     (t<S_TITLE_E) sceneTitle(t);
    else if(t<S_KING_E)  sceneKing(t);
    else if(t<S_TRANS_E) sceneTransfer(t);
    else if(t<S_STAGE_E) sceneStage(t);
    else if(t<S_LAUGH_E) sceneLaugh(t);
    else if(t<S_LOW_E)   sceneLow(t);
    else if(t<S_SIS_E)   sceneSister(t);
    else if(t<S_MATH_E)  sceneMath(t);
    else if(t<S_PHYS_E)  scenePhysics(t);
    else if(t<S_COME_E)  sceneComeback(t);
    else if(t<S_CELEB_E) sceneCelebration(t);
    else                 sceneMentor(t);

    drawSubtitle(t);
    drawEndCard(t);
    glutSwapBuffers();
}

static void timerCB(int){
    if(!g_paused){
        int now=glutGet(GLUT_ELAPSED_TIME);
        float dt=(now-g_lastMs)/1000.f;
        if(dt>0.1f) dt=0.1f;
        g_lastMs=now;
        g_time+=dt;
        if(g_time>TOTAL) g_time=TOTAL;
        updateParticles(dt);
    } else {
        g_lastMs=glutGet(GLUT_ELAPSED_TIME);
    }
    glutPostRedisplay();
    glutTimerFunc(16,timerCB,0);
}

static void keyboard(unsigned char k,int,int){
    if(k==' ') { g_paused=!g_paused; }
    if(k=='r'||k=='R') { g_time=0; for(auto&p:parts) p.alive=false; }
    if(k==27) exit(0);
    if(k=='+') g_time=fminf(g_time+10,TOTAL);
    if(k=='-') g_time=fmaxf(g_time-10,0);
}
static void special(int k,int,int){
    if(k==GLUT_KEY_RIGHT) g_time=fminf(g_time+15,TOTAL);
    if(k==GLUT_KEY_LEFT)  g_time=fmaxf(g_time-15,0);
    if(k==GLUT_KEY_UP)    g_time=fminf(g_time+30,TOTAL);
    if(k==GLUT_KEY_DOWN)  g_time=fmaxf(g_time-30,0);
}
static void reshape(int w,int h){
    WIN_W=w; WIN_H=h;
    glViewport(0,0,w,h);
}

int main(int argc,char**argv){
    glutInit(&argc,argv);
    glutInitDisplayMode(GLUT_DOUBLE|GLUT_RGBA|GLUT_DEPTH);
    glutInitWindowSize(WIN_W,WIN_H);
    glutCreateWindow("The Silent Script — Enhanced 3D Animation");

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_LIGHTING);
    glEnable(GL_NORMALIZE);
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK,GL_AMBIENT_AND_DIFFUSE);
    glShadeModel(GL_SMOOTH);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    initStars();
    for(auto&p:parts) p.alive=false;
    g_lastMs=glutGet(GLUT_ELAPSED_TIME);

    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutSpecialFunc(special);
    glutTimerFunc(16,timerCB,0);

    printf("╔══════════════════════════════════════════╗\n");
    printf("║   THE SILENT SCRIPT — Enhanced 3D       ║\n");
    printf("╠══════════════════════════════════════════╣\n");
    printf("║  SPACE        Pause / Resume             ║\n");
    printf("║  R            Restart from beginning     ║\n");
    printf("║  Arrow RIGHT  Skip forward  +15 sec      ║\n");
    printf("║  Arrow LEFT   Skip backward -15 sec      ║\n");
    printf("║  Arrow UP     Skip forward  +30 sec      ║\n");
    printf("║  Arrow DOWN   Skip backward -30 sec      ║\n");
    printf("║  + / -        Skip +/- 10 sec            ║\n");
    printf("║  ESC          Quit                       ║\n");
    printf("╚══════════════════════════════════════════╝\n");
    printf("\n  SCENE GUIDE:\n");
    printf("   0:00  Title — Ornament rings\n");
    printf("   0:10  King  — Aditya's classroom glory\n");
    printf("   0:40  Transfer — Imposing new school\n");
    printf("   1:05  Stage — Speech freeze\n");
    printf("   1:45  Laughter — Walk of shame\n");
    printf("   2:15  Low Point — Night, math fear\n");
    printf("   2:38  Sister — Unit circle revelation\n");
    printf("   3:10  Math Awakening — Triangle & matrix\n");
    printf("   3:40  Physics — Teacher's compliment\n");
    printf("   4:10  Comeback — Triumphant speech\n");
    printf("   4:35  CELEBRATION — Fireworks & trophies\n");
    printf("   4:55  Mentor — Full circle\n");

    glutMainLoop();
    return 0;
}