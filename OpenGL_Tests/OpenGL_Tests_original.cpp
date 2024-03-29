#include "stdafx.h"
/**
 * Test using a geometry and fragment shaders to implement stippled lines.
 *
 * Brian Paul
 * April 2011
 */

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include "glut_wrap.h"
#include "shaderutil.h"

static GLint WinWidth = 500, WinHeight = 500;
static GLint Win = 0;
static GLuint VertShader, GeomShader, FragShader, Program;
static GLboolean Anim = GL_TRUE;
static GLboolean UseGeomShader = GL_TRUE;
static GLfloat Xrot = 0, Yrot = 0;
static int uViewportSize = -1, uStippleFactor = -1, uStipplePattern = -1;
static const int NumPoints = 50;
static float Points[100][3], Colors[100][3];

static const GLushort StipplePattern = 0x10ff;
static GLuint StippleFactor = 2;


static void
CheckError(int line)
{
   GLenum err = glGetError();
   if (err) {
      printf("GL Error %s (0x%x) at line %d\n",
             gluErrorString(err), (int) err, line);
   }
}


/**
 * Set stipple factor and pattern for geometry shader.
 *
 * We convert the 16-bit stipple pattern into an array of 16 float values
 * then pass the array as a uniform variable.
 *
 * Note: With GLSL 1.30 or later the stipple pattern could be implemented
 * as an ordinary integer since GLSL 1.30 has true integer types and bit
 * shifts and bit masks.
 *
 */
static void
SetStippleUniform(GLint factor, GLushort pattern)
{
   GLfloat p[16];
   int i;
   for (i = 0; i < 16; i++) {
      p[i] = (pattern & (1 << i)) ? 1.0f : 0.0f;
   }
   glUniform1fv(uStipplePattern, 16, p);
   glUniform1f(uStippleFactor, factor);
}


static void
Redisplay(void)
{
   int i;

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   glPushMatrix();
   glRotatef(Xrot, 1, 0, 0);
   glRotatef(Yrot, 0, 0, 1);

   if (UseGeomShader) {
      glUseProgram(Program);
      glDisable(GL_LINE_STIPPLE);
   }
   else {
      glUseProgram(0);
      glEnable(GL_LINE_STIPPLE);
   }

   glBegin(GL_LINES);
   for (i = 0; i < NumPoints; i++) {
      glColor3fv(Colors[i]);
      glVertex3fv(Points[i]);
   }
   glEnd();

   glPopMatrix();

   glutSwapBuffers();
}


static void
Idle(void)
{
   int curTime = glutGet(GLUT_ELAPSED_TIME);
   Xrot = curTime * 0.02;
   Yrot = curTime * 0.05;
   glutPostRedisplay();
}


static void
Reshape(int width, int height)
{
   float ar = (float) width / height;
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
#if 1
   glFrustum(-ar, ar, -1, 1, 3, 25);
#else
   glOrtho(-3.0*ar, 3.0*ar, -3.0, 3.0, 3, 25);
#endif
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0, 0, -10);

   {
      GLfloat viewport[4];
      glGetFloatv(GL_VIEWPORT, viewport);
      glUniform2f(uViewportSize, viewport[2], viewport[3]);
   }
}


static void
CleanUp(void)
{
   glDeleteShader(FragShader);
   glDeleteShader(VertShader);
   glDeleteShader(GeomShader);
   glDeleteProgram(Program);
   glutDestroyWindow(Win);
}


static void
Key(unsigned char key, int x, int y)
{
  (void) x;
  (void) y;

   switch(key) {
   case ' ':
   case 'a':
      Anim = !Anim;
      if (Anim) {
         glutIdleFunc(Idle);
      }
      else
         glutIdleFunc(NULL);
      break;
   case 'g':
      UseGeomShader = !UseGeomShader;
      printf("Use geometry shader? %d\n", UseGeomShader);
      break;
   case 'x':
      Xrot ++;
      break;
   case 27:
      CleanUp();
      exit(0);
      break;
   }
   glutPostRedisplay();
}


static void
MakePoints(void)
{
   int i;
   for (i = 0; i < NumPoints; i++) {
      Colors[i][0] = (rand() % 1000) / 1000.0;
      Colors[i][1] = (rand() % 1000) / 1000.0;
      Colors[i][2] = (rand() % 1000) / 1000.0;
      Points[i][0] = ((rand() % 2000) - 1000.0) / 500.0;
      Points[i][1] = ((rand() % 2000) - 1000.0) / 500.0;
      Points[i][2] = ((rand() % 2000) - 1000.0) / 500.0;
   }
}


static void
Init(void)
{
   static const char *fragShaderText =
      "uniform float StipplePattern[16]; \n"
      "varying vec2 stippleCoord; \n"
      "void main() \n"
      "{ \n"
      "   // check the stipple pattern and discard if value is zero \n"
      "   // TODO: we should really undo the perspective interpolation here \n"
      "   // so that it's linear. \n"
      "   float stip = StipplePattern[int(fract(stippleCoord.x) * 16.0)]; \n"
      "   if (stip == 0.0) \n"
      "      discard; \n"
      "   gl_FragColor = gl_Color; \n"
      "} \n";
   static const char *vertShaderText =
      "void main() \n"
      "{ \n"
      "   gl_FrontColor = gl_Color; \n"
      "   gl_Position = ftransform(); \n"
      "} \n";
   static const char *geomShaderText =
      "#version 150 \n"
      "#extension GL_ARB_geometry_shader4: enable \n"
      "uniform vec2 ViewportSize; \n"
      "uniform float StippleFactor; \n"
      "out vec2 stippleCoord; \n"
      "void main() \n"
      "{ \n"
      "   vec4 pos0 = gl_PositionIn[0]; \n"
      "   vec4 pos1 = gl_PositionIn[1]; \n"
      "   // Convert eye coords to window coords \n"
      "   // Note: we're off by a factor of two here, make up for that below \n"
      "   vec2 p0 = pos0.xy / pos0.w * ViewportSize; \n"
      "   vec2 p1 = pos1.xy / pos1.w * ViewportSize; \n"
      "   float len = length(p0.xy - p1.xy); \n"
      "   // Emit first vertex \n"
      "   gl_FrontColor = gl_FrontColorIn[0]; \n"
      "   gl_Position = pos0; \n"
      "   stippleCoord.x = 0.0; \n"
      "   EmitVertex(); \n"
      "   // Emit second vertex \n"
      "   gl_FrontColor = gl_FrontColorIn[1]; \n"
      "   gl_Position = pos1; \n"
      "   stippleCoord.x = len / StippleFactor / 32.0; // Note: not 16, see above \n"
      "   EmitVertex(); \n"
      "} \n";

   if (!ShadersSupported())
      exit(1);

   if (!glutExtensionSupported("GL_ARB_geometry_shader4")) {
      fprintf(stderr, "Sorry, GL_ARB_geometry_shader4 is not supported.\n");
      exit(1);
   }

   VertShader = CompileShaderText(GL_VERTEX_SHADER, vertShaderText);
   FragShader = CompileShaderText(GL_FRAGMENT_SHADER, fragShaderText);
   GeomShader = CompileShaderText(GL_GEOMETRY_SHADER_ARB, geomShaderText);
   assert(GeomShader);

   Program = LinkShaders3(VertShader, GeomShader, FragShader);
   assert(Program);
   CheckError(__LINE__);

   /*
    * The geometry shader accepts lines and produces lines.
    */
   glProgramParameteriARB(Program, GL_GEOMETRY_INPUT_TYPE_ARB,
                          GL_LINES);
   glProgramParameteriARB(Program, GL_GEOMETRY_OUTPUT_TYPE_ARB,
                          GL_LINE_STRIP);
   glProgramParameteriARB(Program, GL_GEOMETRY_VERTICES_OUT_ARB, 4);
   CheckError(__LINE__);

   glLinkProgramARB(Program);

   /* check link */
   {
      GLint stat;
      GetProgramiv(Program, GL_LINK_STATUS, &stat);
      if (!stat) {
         GLchar log[1000];
         GLsizei len;
         GetProgramInfoLog(Program, 1000, &len, log);
         fprintf(stderr, "Shader link error:\n%s\n", log);
      }
   }

   glUseProgram(Program);

   uViewportSize = glGetUniformLocation(Program, "ViewportSize");
   uStippleFactor = glGetUniformLocation(Program, "StippleFactor");
   uStipplePattern = glGetUniformLocation(Program, "StipplePattern");

   glUniform1f(uStippleFactor, StippleFactor);

   glClearColor(0.3f, 0.3f, 0.3f, 0.0f);

   printf("GL_RENDERER = %s\n",(const char *) glGetString(GL_RENDERER));

   assert(glIsProgram(Program));
   assert(glIsShader(FragShader));
   assert(glIsShader(VertShader));
   assert(glIsShader(GeomShader));


   glLineStipple(StippleFactor, StipplePattern);
   SetStippleUniform(StippleFactor, StipplePattern);

   MakePoints();
}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowSize(WinWidth, WinHeight);
   glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
   Win = glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutDisplayFunc(Redisplay);
   if (Anim)
      glutIdleFunc(Idle);

   Init();
   glutMainLoop();
   return 0;
}


#if 0
#include "stdafx.h"
//This program demonstrates geometric primitives and their attributes.

#include <GL/glut.h>
//#include <stdlib.h>
#define drawOneLine(x1,y1,x2,y2)  glBegin(GL_LINES); glVertex2f ((x1),(y1)); glVertex2f ((x2),(y2)); glEnd();

void init(void) 
{
    glClearColor (0.0, 0.0, 0.0, 0.0);
    glShadeModel (GL_FLAT);
}

void display(void)
{
    int i;

    glClear (GL_COLOR_BUFFER_BIT);

    /* select white for all lines */
    glColor3f (1.0, 1.0, 1.0);
    
    /* in 1st row, 3 lines, each with a different stipple */
    glEnable (GL_LINE_STIPPLE);

    glLineStipple (1, 0x0101); /* dotted */
    drawOneLine (50.0, 125.0, 150.0, 125.0);
    glLineStipple (1, 0x00FF); /* dashed */
    drawOneLine (150.0, 125.0, 250.0, 125.0);
    glLineStipple (1, 0x1C47); /* dash/dot/dash */
    drawOneLine (250.0, 125.0, 350.0, 125.0);

    /* in 2nd row, 3 wide lines, each with different stipple */
    glLineWidth (5.0);
    glLineStipple (1, 0x0101); /* dotted */
    drawOneLine (50.0, 100.0, 150.0, 100.0);
    glLineStipple (1, 0x00FF); /* dashed */
    drawOneLine (150.0, 100.0, 250.0, 100.0);
    glLineStipple (1, 0x1C47); /* dash/dot/dash */
    drawOneLine (250.0, 100.0, 350.0, 100.0);
    glLineWidth (1.0);

    /* in 3rd row, 6 lines, with dash/dot/dash stipple */
    /* as part of a single connected line strip */
    glLineStipple (1, 0x1C47); /* dash/dot/dash */
    glBegin (GL_LINE_STRIP);
    for (i = 0; i < 7; i++)
        glVertex2f (50.0 + ((GLfloat) i * 50.0), 75.0);
    glEnd ();

    /* in 4th row, 6 independent lines with same stipple */
    for (i = 0; i < 6; i++)
    {
        drawOneLine (50.0 + ((GLfloat) i * 50.0), 50.0,
        50.0 + ((GLfloat)(i+1) * 50.0), 50.0);
    }

    /* in 5th row, 1 line, with dash/dot/dash stipple */
    /* and a stipple repeat factor of 5 */
    glLineStipple (5, 0x1C47); /* dash/dot/dash */
    drawOneLine (50.0, 25.0, 350.0, 25.0);

    /* in paw ares stipple */
    glLineStipple (3, 0xAAAA); /* ares */
    drawOneLine (175.0, 5.0, 175.0, 145.0);
    
    glDisable (GL_LINE_STIPPLE);
    glFlush ();
}

void reshape (int w, int h)
{
    glViewport (0, 0, (GLsizei) w, (GLsizei) h);
    glMatrixMode (GL_PROJECTION);
    glLoadIdentity ();
    gluOrtho2D (0.0, (GLdouble) w, 0.0, (GLdouble) h);
}

void keyboard(unsigned char key, int x, int y)
{
    switch (key)
    {
        case 27:
        //exit(0);
        break;
    }
}

int main(int argc, char** argv)
{
    glutInit(&argc, argv);
    glutInitDisplayMode (GLUT_SINGLE | GLUT_RGB);
    glutInitWindowSize (400, 150); 
    glutInitWindowPosition (100, 100);
    glutCreateWindow (argv[0]);
    init ();
    glutDisplayFunc(display); 
    glutReshapeFunc(reshape);
    glutKeyboardFunc(keyboard);
    glutMainLoop();
    return 0;
}
#endif