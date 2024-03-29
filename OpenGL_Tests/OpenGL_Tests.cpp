#include "stdafx.h"
/**
 * Test using a geometry and fragment shaders to implement stippled lines.
 *
 * Brian Paul
 * April 2011
 */

#include "myShaders.h"

static GLint WinWidth = 500, WinHeight = 500;
static GLint Win = 0;
static GLboolean Anim = GL_TRUE;
static GLboolean UseGeomShader = GL_TRUE;
static GLfloat Xrot = 0, Yrot = 0;
static int uViewportSize = -1, uStippleFactor = -1, uStipplePattern = -1;
static const int NumPoints = 8;
static bool bOneColor = false, bOneNormal = true, bDrawOrder = false;
static float Points[100][3], Colors[100][3], Fog[100], Normals[100][3], Order[100], Thickness[100];

static const GLushort StipplePattern = 0x10ff;
static GLuint StippleFactor = 2;

static ThicknessLocations m_thicknessLocations, m_thicknessSLocations;
static SimpleLocations simpleLocations;

static ODCOLORREF oneColor;
static float oneNormal[3];
static GLuint ProgramTest, ProgramTestSimplified, SimpleProgram;
static bool bUseShaders = false;


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
/*static void
SetStippleUniform(GLint factor, GLushort pattern)
{
   GLfloat p[16];
   int i;
   for (i = 0; i < 16; i++) {
      p[i] = (pattern & (1 << i)) ? 1.0f : 0.0f;
   }
   //glUniform1fv(uStipplePattern, 16, p);
   //glUniform1f(uStippleFactor, factor);
}*/


static void
Redisplay(void)
{
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

    glPushMatrix();
    glRotatef(Xrot, 1, 0, 0);
    glRotatef(Yrot, 0, 0, 1);

    // generate projection matrix
    GLdouble modelView[16];
    GLdouble projection[16];
    GLint viewport[4];
    glGetDoublev(GL_MODELVIEW_MATRIX,modelView);
    glGetDoublev(GL_PROJECTION_MATRIX,projection);
    glGetIntegerv(GL_VIEWPORT,viewport);

    if ( bUseShaders )
    {
        if (UseGeomShader) {
            glUseProgram(ProgramTest);
            setMultipleThicknessParams( m_thicknessLocations, bDrawOrder, 0.5f, 0.01f, bOneNormal, oneNormal, bOneColor, oneColor );
            glEnableVertexAttribArray( m_thicknessLocations.a_vertexCoord );
            glEnableVertexAttribArray( m_thicknessLocations.a_vertexThickness );

            glVertexAttribPointer( m_thicknessLocations.a_vertexThickness, 1, GL_FLOAT, GL_FALSE, 0, Thickness );
            glVertexAttribPointer( m_thicknessLocations.a_vertexCoord, 3, GL_FLOAT, GL_FALSE, 0, Points );

            if ( !bOneColor )
            {
                glEnableVertexAttribArray( m_thicknessLocations.a_vertexColor );
                glVertexAttribPointer( m_thicknessLocations.a_vertexColor, 3, GL_FLOAT, GL_FALSE, 0, Colors );
            }

            if ( !bOneNormal )
            {
                glEnableVertexAttribArray( m_thicknessLocations.a_vertexNormal );
                glVertexAttribPointer( m_thicknessLocations.a_vertexNormal, 3, GL_FLOAT, GL_FALSE, 0, Normals );
            }

            if ( bDrawOrder )
            {
                glEnableVertexAttribArray( m_thicknessLocations.a_vertexOrder );
                glVertexAttribPointer( m_thicknessLocations.a_vertexOrder, 1, GL_FLOAT, GL_FALSE, 0, Order );
            }
        }
        else {
            glUseProgram(SimpleProgram);
            glEnableVertexAttribArray( simpleLocations.a_vertexCoord );
            glEnableVertexAttribArray( simpleLocations.a_vertexColor );

            glVertexAttribPointer( simpleLocations.a_vertexColor, 3, GL_FLOAT, GL_FALSE, 0, Colors );
            glVertexAttribPointer( simpleLocations.a_vertexCoord, 3, GL_FLOAT, GL_FALSE, 0, Points );
        }

        glEnableVertexAttribArray( m_thicknessLocations.a_vertexCoord );
        glVertexAttribPointer( m_thicknessLocations.a_vertexCoord, 3, GL_FLOAT, GL_FALSE, 0, Points );

        glDrawArrays( GL_LINES, 0, NumPoints );

        glDisableVertexAttribArray( m_thicknessLocations.a_vertexOrder );
        glDisableVertexAttribArray( m_thicknessLocations.a_vertexNormal );
        glDisableVertexAttribArray( m_thicknessLocations.a_vertexColor );
        glDisableVertexAttribArray( m_thicknessLocations.a_vertexThickness );
        glDisableVertexAttribArray( m_thicknessLocations.a_vertexCoord );
        glDisableVertexAttribArray( simpleLocations.a_vertexCoord );
        glDisableVertexAttribArray( simpleLocations.a_vertexColor );
    }else
    {
        glUseProgram(0);
        glEnableClientState( GL_VERTEX_ARRAY );
        glEnableClientState( GL_COLOR_ARRAY );

        glVertexPointer( 3, GL_FLOAT, 0, Points );
        glColorPointer( 3, GL_FLOAT, 0, Colors );
        glDrawArrays( GL_LINES, 0, NumPoints );

        glDisableClientState( GL_VERTEX_ARRAY );
        glDisableClientState( GL_COLOR_ARRAY );
    }


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
#if 0
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
   //glDeleteShader(FragShader);
   //glDeleteShader(VertShader);
   glDeleteProgram(ProgramTest);
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
   case 's':
      bUseShaders = !bUseShaders;
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
    oneColor = ODRGBA(255, 255, 255, 255);
    oneNormal[0] =0.0f;
    oneNormal[1] =0.0f;
    oneNormal[2] =1.0f;
    int i;
    for (i = 0; i < NumPoints; i++) {
      Colors[i][0] = (rand() % 1000) / 1000.0;
      Colors[i][1] = (rand() % 1000) / 1000.0;
      Colors[i][2] = (rand() % 1000) / 1000.0;
      Points[i][0] = ((rand() % 2000) - 1000.0) / 500.0;
      Points[i][1] = ((rand() % 2000) - 1000.0) / 500.0;
      Points[i][2] = 0;
      Thickness[i] = (i%3)/10;
      Order[i] = i/2;
      Normals[i][0] = 0;
      Normals[i][1] = 0;
      Normals[i][2] = 1;
   }
}


static void
Init(void)
{
    /*
    * The geometry shader accepts lines and produces lines.
    */

   ProgramTest = getThicknessTestProgram();
   InitThicknessLocations(ProgramTest, m_thicknessLocations);

   //ProgramTestSimplified = getThicknessSimplifiedTestProgram();
   //InitThicknessLocations(ProgramTestSimplified, m_thicknessSLocations);
   SimpleProgram = getSimpleProgram();
   InitSimpleLocations( SimpleProgram, simpleLocations );

   glUseProgram(ProgramTest);

   glClearColor(0.3f, 0.3f, 0.3f, 0.0f);

   printf("GL_RENDERER = %s\n",(const char *) glGetString(GL_RENDERER));

   assert(glIsProgram(ProgramTest));
   //assert(glIsProgram(ProgramTestSimplified));
   assert(glIsProgram(SimpleProgram));
   //assert(glIsShader(GeomShader));
   //assert(glIsShader(VertShader));


   //glLineStipple(StippleFactor, StipplePattern);
   //SetStippleUniform(StippleFactor, StipplePattern);

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