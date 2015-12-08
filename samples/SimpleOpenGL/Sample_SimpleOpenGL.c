/* ----------------------------------------------------------------------------
// Simple sample to prove that Assimp is easy to use with OpenGL.
// It takes a file name as command line parameter, loads it using standard
// settings and displays it.
//
// If you intend to _use_ this code sample in your app, do yourself a favour 
// and replace immediate mode calls with VBOs ...
//
// The vc8 solution links against assimp-release-dll_win32 - be sure to
// have this configuration built.
// ----------------------------------------------------------------------------
*/

#define DEBUG 0


typedef int BOOL;
typedef int bool;
#define TRUE 1
#define FALSE 0

static BOOL g_bButton1Down = FALSE;
static int g_yClick = 0;

#include <float.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <time.h>

#ifdef USEGLEW
#include <GL/glew.h>
#endif
#define GL_GLEXT_PROTOTYPES
#ifdef __APPLE__
#include <glut.h>
#else
#include <GL/glut.h>
#endif

#define Cos(th) cos(M_PI/180*(th))
#define Sin(th) sin(M_PI/180*(th))

/* assimp include files. These three are usually needed. */
#include <assimp/cimport.h>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

bool axes = 0;
int sky[2];         // Sky textures
char *Xwing = "../Xwing.3ds";
/* the global Assimp scene object */
const struct aiScene* scene = NULL;
/* Second scene for other ships */
GLuint scene_list = 0;
GLuint scene_list2 = 0;
struct aiVector3D scene_min, scene_max, scene_center;
/* current rotation angle */
static float angle = 0.f;
unsigned int meshCurrent = 0;
#define aisgl_min(x,y) (x<y?x:y)
#define aisgl_max(x,y) (y>x?y:x)

GLdouble mouseWorldCoord[3] = {0.0, 0.0, 0.0};
int inc = 10;       // Ball increment
int emission = 0;   // Emission intensity (%)

#define LEN 8192 // Maximum length of text string
void Print(const char* format , ...)
{
    char buf[LEN];
    char* ch=buf;
    va_list args;
    // Turn the parameters into a character string
    va_start(args, format);
    vsnprintf(buf,LEN,format,args);
    va_end(args);
    // Display the characters one at a time at the current raster position
    while (*ch)
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18,*ch++);
}

/*
 * Draw vertex in polar coordinates with normal
 */
static void Vertex(double th, double ph){
    double x = Sin(th) * Cos(ph);
    double y = Cos(th) * Cos(ph);
    double z = Sin(ph);
    //  For a sphere at the origin, the position
    //  and normal vectors are the same
    glNormal3d(x, y, z);
    glVertex3d(x, y, z);
}

/*
 *  Draw a ball
 *  at (x,y,z)
 *  radius (r)
 */
static void ball(double x, double y, double z, double r){
    int th, ph;
    float yellow[] = {1.0, 1.0, 0.0, 1.0 };
    float Emission[] = { 0.0, 0.0, 0.01 * emission, 1.0 };
    //  Save transformation
    glPushMatrix();
    //  Offset, scale and rotate
    glTranslated(x, y, z);
    glScaled(r, r, r);
    //  White ball
    glColor3f(1, 1, 1);
    glMaterialf(GL_FRONT, GL_SHININESS, 0.8);
    glMaterialfv(GL_FRONT, GL_SPECULAR, yellow);
    glMaterialfv(GL_FRONT, GL_EMISSION, Emission);
    //  Bands of latitude
    for (ph = -90; ph < 90; ph+=inc) {
        glBegin(GL_QUAD_STRIP);
        for (th = 0; th <= 360; th +=2 * inc) {
            Vertex(th, ph);
            Vertex(th, ph + inc);
        }
        glEnd();
    }
    // Undo transformations
    glPopMatrix();
}


void MouseButton(int button, int state, int x, int y)
{
    //  Respond to mouse button presses
    //  If button1 pressed, mark this state so we know in motion function

    if (button == GLUT_LEFT_BUTTON)
    {
        g_bButton1Down = (state == GLUT_DOWN) ? TRUE : FALSE;
        //printf("Mouse Pressed!\n");

    }
}


void MouseMotion(int x, int y)
{
    // Vector3 GetOGLPos(int x, int y)
    //static int rangeError = 3000;
    GLint viewport[4];
    GLdouble modelview[16];
    GLdouble projection[16];
    GLfloat winX, winY, winZ;
    GLdouble posX, posY, posZ;

    glGetDoublev( GL_MODELVIEW_MATRIX, modelview );
    glGetDoublev( GL_PROJECTION_MATRIX, projection );
    glGetIntegerv( GL_VIEWPORT, viewport );

    winX = (float)x;
    winY = (float)viewport[3] - (float)y;
    glReadPixels( x, (int)winY, 1, 1, GL_DEPTH_COMPONENT, GL_FLOAT, &winZ );
    // Map window coordinates to world coordinates
    // Currently only works head when viewing perpendicular to model
    gluUnProject( winX, winY, (GLdouble)0.71, modelview, projection, viewport, &posX, &posY, &posZ);
    if (g_bButton1Down){
        // have mouse movement change gloabal variable that will change
        // where is drawn in display
        mouseWorldCoord[0]=posX;
        mouseWorldCoord[1]=posY;
        mouseWorldCoord[2]=posZ;
        //http://nehe.gamedev.net/article/using_gluunproject/16013/
    }
}


/* ---------------------------------------------------------------------------- */
void reshape(int width, int height)
{
    const double aspectRatio = (float) width / height, fieldOfView = 45.0;

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluPerspective(fieldOfView, aspectRatio,
            1.0, 1000.0);  /* Znear and Zfar */
    glViewport(0, 0, width, height);
    mouseWorldCoord[0]=0.f;
    mouseWorldCoord[1]=0.f;
    mouseWorldCoord[2]=0.f;
    glTranslatef(0.f,0.f,0.f);
}

void Fatal(const char* format , ...)
{
    va_list args;
    va_start(args,format);
    vfprintf(stderr,format,args);
    va_end(args);
    exit(1);
}


void ErrCheck(const char* where)
{
    int err = glGetError();
    if (err) fprintf(stderr,"ERROR: %s [%s]\n",gluErrorString(err),where);
}


/*
 *  Reverse n bytes
 */
static void Reverse(void* x,const int n)
{
    int k;
    char* ch = (char*)x;
    for (k=0;k<n/2;k++)
    {
        char tmp = ch[k];
        ch[k] = ch[n-1-k];
        ch[n-1-k] = tmp;
    }
}

/*
 *  Load texture from BMP file
 */
unsigned int LoadTexBMP(const char* file)
{
    unsigned int   texture;    // Texture name
    FILE*          f;          // File pointer
    unsigned short magic;      // Image magic
    unsigned int   dx,dy,size; // Image dimensions
    unsigned short nbp,bpp;    // Planes and bits per pixel
    unsigned char* image;      // Image data
    unsigned int   k;          // Counter
    int            max;        // Maximum texture dimensions

    //  Open file
    f = fopen(file,"rb");
    if (!f) Fatal("Cannot open file %s\n",file);
    //  Check image magic
    if (fread(&magic,2,1,f)!=1) Fatal("Cannot read magic from %s\n",file);
    if (magic!=0x4D42 && magic!=0x424D) Fatal("Image magic not BMP in %s\n",file);
    //  Seek to and read header
    if (fseek(f,16,SEEK_CUR) || fread(&dx ,4,1,f)!=1 || fread(&dy ,4,1,f)!=1 ||
            fread(&nbp,2,1,f)!=1 || fread(&bpp,2,1,f)!=1 || fread(&k,4,1,f)!=1)
        Fatal("Cannot read header from %s\n",file);
    //  Reverse bytes on big endian hardware (detected by backwards magic)
    if (magic==0x424D)
    {
        Reverse(&dx,4);
        Reverse(&dy,4);
        Reverse(&nbp,2);
        Reverse(&bpp,2);
        Reverse(&k,4);
    }
    //  Check image parameters
    glGetIntegerv(GL_MAX_TEXTURE_SIZE,&max);
    if (dx<1 || dx>max) Fatal("%s image width %d out of range 1-%d\n",file,dx,max);
    if (dy<1 || dy>max) Fatal("%s image height %d out of range 1-%d\n",file,dy,max);
    if (nbp!=1)  Fatal("%s bit planes is not 1: %d\n",file,nbp);
    if (bpp!=24) Fatal("%s bits per pixel is not 24: %d\n",file,bpp);
    if (k!=0)    Fatal("%s compressed files not supported\n",file);
#ifndef GL_VERSION_2_0
    //  OpenGL 2.0 lifts the restriction that texture size must be a power of two
    for (k=1;k<dx;k*=2);
    if (k!=dx) Fatal("%s image width not a power of two: %d\n",file,dx);
    for (k=1;k<dy;k*=2);
    if (k!=dy) Fatal("%s image height not a power of two: %d\n",file,dy);
#endif

    //  Allocate image memory
    size = 3*dx*dy;
    image = (unsigned char*) malloc(size);
    if (!image) Fatal("Cannot allocate %d bytes of memory for image %s\n",size,file);
    //  Seek to and read image
    if (fseek(f,20,SEEK_CUR) || fread(image,size,1,f)!=1) Fatal("Error reading data from image %s\n",file);
    fclose(f);
    //  Reverse colors (BGR -> RGB)
    for (k=0;k<size;k+=3)
    {
        unsigned char temp = image[k];
        image[k]   = image[k+2];
        image[k+2] = temp;
    }

    //  Sanity check
    ErrCheck("LoadTexBMP");
    //  Generate 2D texture
    glGenTextures(1,&texture);
    glBindTexture(GL_TEXTURE_2D,texture);
    //  Copy image
    glTexImage2D(GL_TEXTURE_2D,0,3,dx,dy,0,GL_RGB,GL_UNSIGNED_BYTE,image);
    if (glGetError()) Fatal("Error in glTexImage2D %s %dx%d\n",file,dx,dy);
    //  Scale linearly when image size doesn't match
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MAG_FILTER,GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D,GL_TEXTURE_MIN_FILTER,GL_LINEAR);

    //  Free image memory
    free(image);
    //  Return texture name
    return texture;
}


/* 
 *  Draw sky box
 */
static void Sky(double D)
{
    glColor3f(1,1,1);
    glEnable(GL_TEXTURE_2D);

    //  Sides
    glBindTexture(GL_TEXTURE_2D,sky[0]);
    glBegin(GL_QUADS);
    glTexCoord2f(0.00,0); glVertex3f(-D,-D,-D);
    glTexCoord2f(0.25,0); glVertex3f(+D,-D,-D);
    glTexCoord2f(0.25,1); glVertex3f(+D,+D,-D);
    glTexCoord2f(0.00,1); glVertex3f(-D,+D,-D);

    glTexCoord2f(0.25,0); glVertex3f(+D,-D,-D);
    glTexCoord2f(0.50,0); glVertex3f(+D,-D,+D);
    glTexCoord2f(0.50,1); glVertex3f(+D,+D,+D);
    glTexCoord2f(0.25,1); glVertex3f(+D,+D,-D);

    glTexCoord2f(0.50,0); glVertex3f(+D,-D,+D);
    glTexCoord2f(0.75,0); glVertex3f(-D,-D,+D);
    glTexCoord2f(0.75,1); glVertex3f(-D,+D,+D);
    glTexCoord2f(0.50,1); glVertex3f(+D,+D,+D);

    glTexCoord2f(0.75,0); glVertex3f(-D,-D,+D);
    glTexCoord2f(1.00,0); glVertex3f(-D,-D,-D);
    glTexCoord2f(1.00,1); glVertex3f(-D,+D,-D);
    glTexCoord2f(0.75,1); glVertex3f(-D,+D,+D);
    glEnd();

    //  Top and bottom
    glBindTexture(GL_TEXTURE_2D,sky[1]);
    glBegin(GL_QUADS);
    glTexCoord2f(0.0,0); glVertex3f(+D,+D,-D);
    glTexCoord2f(0.5,0); glVertex3f(+D,+D,+D);
    glTexCoord2f(0.5,1); glVertex3f(-D,+D,+D);
    glTexCoord2f(0.0,1); glVertex3f(-D,+D,-D);

    glTexCoord2f(1.0,1); glVertex3f(-D,-D,+D);
    glTexCoord2f(0.5,1); glVertex3f(+D,-D,+D);
    glTexCoord2f(0.5,0); glVertex3f(+D,-D,-D);
    glTexCoord2f(1.0,0); glVertex3f(-D,-D,-D);
    glEnd();

    glDisable(GL_TEXTURE_2D);
}


/* ---------------------------------------------------------------------------- */
void get_bounding_box_for_node (const struct aiNode* nd, 
        struct aiVector3D* min, 
        struct aiVector3D* max, 
        struct aiMatrix4x4* trafo
        ){
    struct aiMatrix4x4 prev;
    unsigned int n = 0, t;

    prev = *trafo;
    aiMultiplyMatrix4(trafo,&nd->mTransformation);

    for (; n < nd->mNumMeshes; ++n) {
        const struct aiMesh* mesh = scene->mMeshes[nd->mMeshes[n]];
        for (t = 0; t < mesh->mNumVertices; ++t) {

            struct aiVector3D tmp = mesh->mVertices[t];
            aiTransformVecByMatrix4(&tmp,trafo);

            min->x = aisgl_min(min->x,tmp.x);
            min->y = aisgl_min(min->y,tmp.y);
            min->z = aisgl_min(min->z,tmp.z);

            max->x = aisgl_max(max->x,tmp.x);
            max->y = aisgl_max(max->y,tmp.y);
            max->z = aisgl_max(max->z,tmp.z);
        }
    }

    for (n = 0; n < nd->mNumChildren; ++n) {
        get_bounding_box_for_node(nd->mChildren[n],min,max,trafo);
    }
    *trafo = prev;
}




/* ---------------------------------------------------------------------------- */
void get_bounding_box (struct aiVector3D* min, struct aiVector3D* max)
{
    struct aiMatrix4x4 trafo;
    aiIdentityMatrix4(&trafo);

    min->x = min->y = min->z =  1e10f;
    max->x = max->y = max->z = -1e10f;
    get_bounding_box_for_node(scene->mRootNode,min,max,&trafo);
}

/* ---------------------------------------------------------------------------- */
void color4_to_float4(const struct aiColor4D *c, float f[4])
{
    f[0] = c->r;
    f[1] = c->g;
    f[2] = c->b;
    f[3] = c->a;
}

/* ---------------------------------------------------------------------------- */
void set_float4(float f[4], float a, float b, float c, float d)
{
    f[0] = a;
    f[1] = b;
    f[2] = c;
    f[3] = d;
}

/* ---------------------------------------------------------------------------- */
void apply_material(const struct aiMaterial *mtl)
{
    float c[4];

    GLenum fill_mode;
    int ret1, ret2;
    struct aiColor4D diffuse;
    struct aiColor4D specular;
    struct aiColor4D ambient;
    struct aiColor4D emission;
    float shininess, strength;
    int two_sided;
    int wireframe;
    unsigned int max;

    set_float4(c, 0.8f, 0.8f, 0.8f, 1.0f);
    if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_DIFFUSE, &diffuse))
        color4_to_float4(&diffuse, c);
    glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, c);

    set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
    if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_SPECULAR, &specular))
        color4_to_float4(&specular, c);
    glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, c);

    set_float4(c, 0.2f, 0.2f, 0.2f, 1.0f);
    if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_AMBIENT, &ambient))
        color4_to_float4(&ambient, c);
    glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT, c);

    set_float4(c, 0.0f, 0.0f, 0.0f, 1.0f);
    if(AI_SUCCESS == aiGetMaterialColor(mtl, AI_MATKEY_COLOR_EMISSIVE, &emission))
        color4_to_float4(&emission, c);
    glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, c);

    max = 1;
    ret1 = aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS, &shininess, &max);
    if(ret1 == AI_SUCCESS) {
        max = 1;
        ret2 = aiGetMaterialFloatArray(mtl, AI_MATKEY_SHININESS_STRENGTH, &strength, &max);
        if(ret2 == AI_SUCCESS)
            glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess * strength);
        else
            glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, shininess);
    }
    else {
        glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, 0.0f);
        set_float4(c, 0.0f, 0.0f, 0.0f, 0.0f);
        glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, c);
    }

    max = 1;
    if(AI_SUCCESS == aiGetMaterialIntegerArray(mtl, AI_MATKEY_ENABLE_WIREFRAME, &wireframe, &max))
        fill_mode = wireframe ? GL_LINE : GL_FILL;
    else
        fill_mode = GL_FILL;
    glPolygonMode(GL_FRONT_AND_BACK, fill_mode);

    max = 1;
    if((AI_SUCCESS == aiGetMaterialIntegerArray(mtl, AI_MATKEY_TWOSIDED, &two_sided, &max)) && two_sided)
        glDisable(GL_CULL_FACE);
    else 
        glEnable(GL_CULL_FACE);
}

/* ---------------------------------------------------------------------------- */
void recursive_render (const struct aiScene *sc, const struct aiNode* nd)
{
    unsigned int i;
    unsigned int n = 0, t;
    struct aiMatrix4x4 m = nd->mTransformation;

    /* update transform */
    aiTransposeMatrix4(&m);
    glPushMatrix();
    glMultMatrixf((float*)&m);

    /* draw all meshes assigned to this node */
    for (; n < nd->mNumMeshes; ++n) {
        const struct aiMesh* mesh = scene->mMeshes[nd->mMeshes[n]];

        apply_material(sc->mMaterials[mesh->mMaterialIndex]);

        if(mesh->mNormals == NULL) {
            glDisable(GL_LIGHTING);
        } else {
            glEnable(GL_LIGHTING);
        }

        for (t = 0; t < mesh->mNumFaces; ++t) {
            const struct aiFace* face = &mesh->mFaces[t];
            GLenum face_mode;

            switch(face->mNumIndices) {
                case 1: face_mode = GL_POINTS; break;
                case 2: face_mode = GL_LINES; break;
                case 3: face_mode = GL_TRIANGLES; break;
                default: face_mode = GL_POLYGON; break;
            }

            glBegin(face_mode);

            for(i = 0; i < face->mNumIndices; i++) {
                int index = face->mIndices[i];
                if(mesh->mColors[0] != NULL) 
                    glColor4fv((GLfloat*)&mesh->mColors[0][index]);
                if(mesh->mNormals != NULL) 
                    glNormal3fv(&mesh->mNormals[index].x);
                glVertex3fv(&mesh->mVertices[index].x);
            }

            glEnd();
        }

    }

    /* draw all children */
    for (n = 0; n < nd->mNumChildren; ++n) {
        recursive_render(sc, nd->mChildren[n]);
    }
    glPopMatrix();
}

/* ---------------------------------------------------------------------------- */
void do_motion (void)
{
    static GLint prev_time = 0;
    static GLint prev_fps_time = 0;
    static int frames = 0;

    int time = glutGet(GLUT_ELAPSED_TIME);
    angle += (time-prev_time)*0.01;
    prev_time = time;

    frames += 1;
    if ((time - prev_fps_time) > 1000) /* update every seconds */
    {
        int current_fps = frames * 1000 / (time - prev_fps_time);
        printf("%d fps\n", current_fps);
        frames = 0;
        prev_fps_time = time;
    }


    glutPostRedisplay ();
}



/* ---------------------------------------------------------------------------- */
void display(void)
{
    float Ambient[]   = {0,0,0,1};
    float Diffuse[]   = {1,1,1,1};
    float Specular[]  = {1.0,1.0,1.0,1};
    float white[]     = {1,1,1,1};
    float tmp;
    glPushMatrix();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    printf("Angle = %f\n",angle);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    gluLookAt(0.f,0.f,3.f,0.f,0.f,-5.f,0.f,1.f,0.f);
    /* rotate it around the y axis */
    //glRotatef(angle,0.f,1.f,0.f);
    glRotatef(180,0.f,1.f,0.f);
    glEnable(GL_LIGHTING);
    /* scale the whole asset to fit into our view frustum */
    tmp = scene_max.x-scene_min.x;
    tmp = aisgl_max(scene_max.y - scene_min.y,tmp);
    tmp = aisgl_max(scene_max.z - scene_min.z,tmp);
    double len = tmp;
    tmp = 1.f / tmp;
    float Position[] = {len*3.5,len/2,0,1};
    glScalef(tmp, tmp, tmp);
    /* center the model */
    glTranslatef( -scene_center.x, -scene_center.y, -scene_center.z );
    /* if the display list has not been made yet, create a new one and
       fill it with scene contents */
    // Place this in the mode switching function
    if(scene_list == 0) {
        scene_list = glGenLists(1);
        glNewList(scene_list, GL_COMPILE);
        /* now begin at the root node of the imported data and traverse
           the scenegraph by multiplying subsequent local transforms
           together on GL's matrix stack. */
        recursive_render(scene, scene->mRootNode);
        glEndList();
    }
    glTranslatef(mouseWorldCoord[0],mouseWorldCoord[1],0);
    // ball(0,0,0,100);

    glCallList(scene_list);
    glPopMatrix();
    ball(Position[0],Position[1],Position[2],10);
    glPushMatrix();
    glRotatef(angle,0.0,1.0,0.0);
    Sky(len*3.5);
    glPopMatrix();
    glLightfv(GL_LIGHT0,GL_AMBIENT ,Ambient);
    glLightfv(GL_LIGHT0,GL_DIFFUSE ,Diffuse);
    glLightfv(GL_LIGHT0,GL_SPECULAR,Specular);
    glLightfv(GL_LIGHT0,GL_POSITION,Position);
    glMaterialf(GL_FRONT_AND_BACK,GL_SHININESS,32.0f);
    glMaterialfv(GL_FRONT_AND_BACK,GL_SPECULAR,white);
    
    // Draw axes - no lighting from here on
    if(axes){
        glDisable(GL_LIGHTING);
        glColor3f(1, 1, 1);
        glBegin(GL_LINES);
        glVertex3d(0.0, 0.0, 0.0);
        glVertex3d(len, 0.0, 0.0);
        glVertex3d(0.0, 0.0, 0.0);
        glVertex3d(0.0, len, 0.0);
        glVertex3d(0.0, 0.0, 0.0);
        glVertex3d(0.0, 0.0, len);
        glEnd();
    }
    glutSwapBuffers();

    do_motion();
}

/* ---------------------------------------------------------------------------- */
int loadasset (const char* path)
{
    /* we are taking one of the postprocessing presets to avoid
       spelling out 20+ single postprocessing flags here. */
    scene = aiImportFile(path,aiProcessPreset_TargetRealtime_MaxQuality);

    if (scene) {
        get_bounding_box(&scene_min,&scene_max);
        scene_center.x = (scene_min.x + scene_max.x) / 2.0f;
        scene_center.y = (scene_min.y + scene_max.y) / 2.0f;
        scene_center.z = (scene_min.z + scene_max.z) / 2.0f;
        return 0;
    }
    return 1;
}

/*
 *  GLUT calls this routine when a key is pressed
 */
void key(unsigned char ch, int x, int y){
    // Exit on ESC
    if (ch == 27)
        exit(0);
    else if (ch == ' '){
        mouseWorldCoord[0] = 1.f;
        mouseWorldCoord[1] = 1.f;
        mouseWorldCoord[2] = 1.f;
    }

    else if (ch == 'm') {
        meshCurrent = 1 - meshCurrent;
    }


}

/* ---------------------------------------------------------------------------- */
int main(int argc, char **argv)
{
    struct aiLogStream stream;
    glutInitWindowSize(900,600);
    glutInitWindowPosition(100,100);
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
    glutInit(&argc, argv);
    glutCreateWindow("Michael Eller - Final Project (Preview)");
    glutDisplayFunc(display);
    glutReshapeFunc(reshape);
    glutSetCursor(GLUT_CURSOR_NONE);
    glutKeyboardFunc(key);
    sky[0] = LoadTexBMP("../sky0.bmp");
    sky[1] = LoadTexBMP("../sky1.bmp");
    printf("FLT_MAX = %f\n",FLT_MAX);
    /* get a handle to the predefined STDOUT log stream and attach
       it to the logging system. It remains active for all further
       calls to aiImportFile(Ex) and aiApplyPostProcessing. */
    stream = aiGetPredefinedLogStream(aiDefaultLogStream_STDOUT,NULL);
    aiAttachLogStream(&stream);

    /* ... same procedure, but this stream now writes the
       log messages to assimp_log.txt */
    stream = aiGetPredefinedLogStream(aiDefaultLogStream_FILE,"assimp_log.txt");
    aiAttachLogStream(&stream);
    
    loadasset(Xwing);
    glClearColor(0.1f,0.1f,0.1f,1.f);

    glutMouseFunc(MouseButton);
    glutMotionFunc(MouseMotion);


    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);    /* Uses default lighting parameters */
    glEnable(GL_LIGHT1);    /* Light corresponding to star in skybox */
    glEnable(GL_DEPTH_TEST);

    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    glEnable(GL_NORMALIZE);
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);
    /* XXX docs say all polygons are emitted CCW, but tests show that some aren't. */
    if(getenv("MODEL_IS_BROKEN"))  
        glFrontFace(GL_CW);

    glColorMaterial(GL_FRONT_AND_BACK, GL_DIFFUSE);

    glutGet(GLUT_ELAPSED_TIME);
    glutMainLoop();

    /* cleanup - calling 'aiReleaseImport' is important, as the library 
       keeps internal resources until the scene is freed again. Not 
       doing so can cause severe resource leaking. */
    aiReleaseImport(scene);

    /* We added a log stream to the library, it's our job to disable it
       again. This will definitely release the last resources allocated
       by Assimp.*/
    aiDetachAllLogStreams();
    return 0;
}

