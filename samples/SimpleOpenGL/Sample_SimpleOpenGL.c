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
#define TRUE 1
#define FALSE 0

static BOOL g_bButton1Down = FALSE;
static int g_yClick = 0;

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

//GLdouble windowDepth = 0.5;
unsigned int meshCurrent = 0;
char *Xwing = "../Xwing.obj";
char *tieFighter = "../ARC170.3DS";
/* the global Assimp scene object */
const struct aiScene* scene = NULL;
/* Second scene for other ships */
//const struct aiScene* scene2 = NULL;
GLuint scene_list = 0;
GLuint scene_list2 = 0;
struct aiVector3D scene_min, scene_max, scene_center;
//struct aiVector3D scene2_min, scene2_max, scene2_center;
time_t oldTime;
time_t deltaTime;
/* current rotation angle */
static float angle = 0.f;

#define aisgl_min(x,y) (x<y?x:y)
#define aisgl_max(x,y) (y>x?y:x)

GLdouble mouseWorldCoord[3] = {0.0, 0.0, 0.0};
int fov = 55;       // Field of view (for perspective)
double dim = 3.0;   // Size of world
int inc = 10;       // Ball increment
int zh = 90;        // Light azimuth
float ylight = 1.5; // Elevation of light
int emission = 0;   // Emission intensity (%)
int ambient = 30;   // Ambient intensity (%)
int diffuse = 100;  // Diffuse intensity (%)
int specular = 0;   // Specular intensity (%)
int shininess = 6; // Shininess (power of two)
float shinyvec[1];  // Shininess (value)
int mode = 1;

// Boolean indicating whether a (new) model has been loaded
int newModel = 0;

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
    //printf("winZ = %f.\n",winZ);
    // Map window coordinates to world coordinates
    // Currently only works head when viewing perpendicular to model
    gluUnProject( winX, winY, (GLdouble)0.71, modelview, projection, viewport, &posX, &posY, &posZ);
    // if(abs(posX)>rangeError || abs(posY)>rangeError || abs(posZ)>rangeError)
    //     printf("Error!\nOut of range!\n\n");
    if (g_bButton1Down){
        // have mouse movement change gloabal variable that will change
        // where is drawn in display
        mouseWorldCoord[0]=posX;
        mouseWorldCoord[1]=posY;
        mouseWorldCoord[2]=posZ;
        // printf("Current mouse position: %d,%d\n",x,y);
        // printf("Current mouse world coordinates: %f,%f,%f\n",posX, posY, posZ);
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
    printf("mNumMeshes = %d\n",nd->mNumMeshes);
    for (; n < nd->mNumMeshes; ++n) {
        // for (; n < meshCurrent; ++n) {
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

        printf("scene -> mRootNode = %d\n",scene->mRootNode->mNumMeshes);
        /* draw all meshes assigned to this node */
        //change to only render one mesh
        for (; n < nd->mNumMeshes; ++n) {

            // for (; n < meshCurrent; ++n) {
            const struct aiMesh* mesh = scene->mMeshes[meshCurrent];

            //const struct aiMesh* mesh = scene->mMeshes[nd->mMeshes[-1]];
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
            printf("nd->mNumChildren = %d\n",nd->mNumChildren);
            recursive_render(sc, nd->mChildren[n]);
        }
        glPopMatrix();
        }

        /* ---------------------------------------------------------------------------- */
        void do_motion (void)
        {
            static GLint prev_time = 0;
            static GLint prev_fps_time = 0;
            static GLint prev_time2 = 0;
            static GLint prev_fps_time2 = 0;
            static int frames = 0;

            int time = glutGet(GLUT_ELAPSED_TIME);
            angle += (time-prev_time)*0.01;
            prev_time = time;
            prev_time2 = time;




            frames += 1;
            if ((time - prev_fps_time) > 1000) /* update every seconds */
            {
                int current_fps = frames * 1000 / (time - prev_fps_time);
                printf("%d fps\n", current_fps);
                frames = 0;
                prev_fps_time = time;
            }

            //    if ((time2 - prev_fps_time2) > 

            glutPostRedisplay ();
        }



        /* ---------------------------------------------------------------------------- */
        void display(void)
        {
            float tmp;
            glPushMatrix();
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            glMatrixMode(GL_MODELVIEW);
            glLoadIdentity();
            gluLookAt(0.f,0.f,3.f,0.f,0.f,-5.f,0.f,1.f,0.f);
            /* rotate it around the y axis */
            //glRotatef(angle,0.f,1.f,0.f);
            glRotatef(180,0.f,1.f,0.f);

            /* scale the whole asset to fit into our view frustum */
            tmp = scene_max.x-scene_min.x;
            tmp = aisgl_max(scene_max.y - scene_min.y,tmp);
            tmp = aisgl_max(scene_max.z - scene_min.z,tmp);
            double len = tmp;
            tmp = 1.f / tmp;
            glScalef(tmp, tmp, tmp);
            /* center the model */
            glTranslatef( -scene_center.x, -scene_center.y, -scene_center.z );
            //newModel = 1;
            // move model to mouse coordinates
            /* if the display list has not been made yet, create a new one and
               fill it with scene contents */
            // Place this in the mode switching function
            if(scene_list == 0) {
                scene_list = glGenLists(1);
                // printf("scene_list = %d\n",scene_list);
                glNewList(scene_list, GL_COMPILE);
                /* now begin at the root node of the imported data and traverse
                   the scenegraph by multiplying subsequent local transforms
                   together on GL's matrix stack. */
                meshCurrent=0;
                recursive_render(scene, scene->mRootNode);
                glEndList();
                scene_list2 = glGenLists(1);
                // printf("scene_list = %d\n",scene_list);
                glNewList(scene_list2, GL_COMPILE);
                /* now begin at the root node of the imported data and traverse
                   the scenegraph by multiplying subsequent local transforms
                   together on GL's matrix stack. */
                //meshCurrent=1-meshCurrent;
                meshCurrent=1;
                recursive_render(scene, scene->mRootNode);
                glEndList();
            }
            //meshCurrent=1-meshCurrent;
            glTranslatef(mouseWorldCoord[0],mouseWorldCoord[1],0);
            // ball(0,0,0,100);
            //printf("Mesh current = %d\n",meshCurrent);
            switch(meshCurrent){
                case 0:
                    glCallList(scene_list);
                    break;
                case 1:
                    glCallList(scene_list2);
                    break;
                default:
                    glCallList(scene_list);
                    break;
            }
                       glPopMatrix();
            //glLoadIdentity();
            // Draw axes - no lighting from here on
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
            // Label axes


            //glCallList(scene_list);

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
            else if(ch == 'm')
                meshCurrent = 1 - meshCurrent;
            printf("meshCurrent = %d\n",meshCurrent);
            //only allow mode to be changed every 5 seconds
            //temporary workaround until can use multiple "scenes"
            /*
               else if (ch == 'm') {
               deltaTime = difftime(time(0),oldTime);
               if(deltaTime >5){
               oldTime = time(0);
               mode = 1 - mode;
            //without newModel=1, will never change modes
            // newModel = 1;
            printf("Mode changed.\n");
            printf("Normally, another spacecraft would be loaded here.\n");
            }
            }

            if((mode == 0) && (newModel == 1)){
            loadasset(tieFighter);
            printf("Mode changed to tie fighter\n");
            newModel = 0;

            }
            if((mode == 1) && (newModel == 1)){
            loadasset(Xwing);
            printf("Mode changed to XWing\n");
            newModel = 0;
            }
            */

        }

        /* ---------------------------------------------------------------------------- */
        int main(int argc, char **argv)
        {
            struct aiLogStream stream;
            oldTime = time(0);
            glutInitWindowSize(900,600);
            glutInitWindowPosition(100,100);
            glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
            glutInit(&argc, argv);
            glutCreateWindow("Michael Eller - Final Project (Preview)");
            glutDisplayFunc(display);
            glutReshapeFunc(reshape);
            glutSetCursor(GLUT_CURSOR_NONE);
            glutKeyboardFunc(key);


            /* get a handle to the predefined STDOUT log stream and attach
               it to the logging system. It remains active for all further
               calls to aiImportFile(Ex) and aiApplyPostProcessing. */
            stream = aiGetPredefinedLogStream(aiDefaultLogStream_STDOUT,NULL);
            aiAttachLogStream(&stream);

            /* ... same procedure, but this stream now writes the
               log messages to assimp_log.txt */
            stream = aiGetPredefinedLogStream(aiDefaultLogStream_FILE,"assimp_log.txt");
            aiAttachLogStream(&stream);

            /* the model name can be specified on the command line. If none
               is specified, we try to locate one of the more expressive test 
               models from the repository (/models-nonbsd may be missing in 
               some distributions so we need a fallback from /models!). */

            loadasset(Xwing);
            glClearColor(0.1f,0.1f,0.1f,1.f);

            glutMouseFunc(MouseButton);
            glutMotionFunc(MouseMotion);


            glEnable(GL_LIGHTING);
            glEnable(GL_LIGHT0);    /* Uses default lighting parameters */

            glEnable(GL_DEPTH_TEST);

            glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
            glEnable(GL_NORMALIZE);

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

