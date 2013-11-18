#include "stdafx.h"
#include "myShaders.h"


void CheckError(int line)
{
   GLenum err = glGetError();
   if (err) {
      printf("GL Error %s (0x%x) at line %d\n",
             gluErrorString(err), (int) err, line);
   }
}

void InitSimpleLocations(GLuint program, SimpleLocations& simpleLocations)
{
    simpleLocations.a_vertexCoord = glGetAttribLocation(program, "vertexCoord");
    simpleLocations.a_vertexColor = glGetAttribLocation(program, "vertexColor");
}

void InitThicknessLocations(GLuint program, ThicknessLocations& m_thicknessLocations)
{
    if ( m_thicknessLocations.u_Order == -1 )
    {
        m_thicknessLocations.u_bOrder = glGetUniformLocation(program, "bOrder");
        m_thicknessLocations.u_Order = glGetUniformLocation(program, "Order");
        m_thicknessLocations.u_Step = glGetUniformLocation(program, "Step");
        m_thicknessLocations.u_bOneNormal = glGetUniformLocation(program, "bOneNormal");
        m_thicknessLocations.u_Normal = glGetUniformLocation(program, "Normal");
        m_thicknessLocations.u_bOneColor = glGetUniformLocation(program, "bOneColor");
        m_thicknessLocations.u_Color = glGetUniformLocation(program, "Color");

        m_thicknessLocations.a_vertexCoord = glGetAttribLocation(program, "vertexCoord");
        m_thicknessLocations.a_vertexColor = glGetAttribLocation(program, "vertexColor");
        m_thicknessLocations.a_vertexOrder = glGetAttribLocation(program, "vertexOrder");
        m_thicknessLocations.a_vertexThickness = glGetAttribLocation(program, "vertexThickness");
    }
}

void setMultipleThicknessParams( const ThicknessLocations& m_thicknessLocations, bool bOrder, float order, float step,
                                                    bool bOneNormal, float* normal, bool bOneColor, ODCOLORREF color )
{
    glUniform1i( m_thicknessLocations.u_bOrder, bOrder );
    glUniform1f( m_thicknessLocations.u_Step, step );
    glUniform1f( m_thicknessLocations.u_Order, order );

    glUniform1i( m_thicknessLocations.u_bOneNormal, bOneNormal );
    glUniform3f( m_thicknessLocations.u_Normal, normal[0], normal[1], normal[3] );

    glUniform1i( m_thicknessLocations.u_bOrder, bOneColor );
    float col[4];
    col[0] = ODGETRED( color );
    col[1] = ODGETGREEN( color );
    col[2] = ODGETBLUE( color );
    col[3] = ODGETALPHA( color );
    glUniform1fv( m_thicknessLocations.u_Color, 4, col );
}

void setMultipleThicknessSParams( const ThicknessLocations& m_thicknessLocations, bool bOrder, float order, float step,
                                                    bool bOneColor, ODCOLORREF color )
{
    glUniform1i( m_thicknessLocations.u_bOrder, bOrder );
    glUniform1f( m_thicknessLocations.u_Step, step );
    glUniform1f( m_thicknessLocations.u_Order, order );

    glUniform1i( m_thicknessLocations.u_bOrder, bOneColor );
    float col[4];
    col[0] = ODGETRED( color );
    col[1] = ODGETGREEN( color );
    col[2] = ODGETBLUE( color );
    col[3] = ODGETALPHA( color );
    glUniform1fv( m_thicknessLocations.u_Color, 4, col );
}

GLuint getThicknessTestProgram()
{
    static GLuint VertShader, FragShader, GeomShader, Program;
       static const char *vertShaderText =  "                                                                \n\
attribute vec4 vertexCoord;                                                       \n\
attribute vec3 vertexNormal;                                                      \n\
attribute vec4 vertexColor;                                                       \n\
attribute float vertexOrder;                                                      \n\
attribute float vertexThickness;                                                  \n\
                                                                                  \n\
uniform bool bOneNormal;                                                          \n\
uniform bool bOneColor;                                                           \n\
uniform vec3 Normal;                                                              \n\
uniform vec4 Color;                                                               \n\
                                                                                  \n\
varying float vThickness;                                                         \n\
varying float vOrder;                                                             \n\
varying vec3 vNormal;                                                             \n\
                                                                                  \n\
void main(void)                                                                   \n\
{                                                                                 \n\
    gl_Position = vertexCoord;                                                    \n\
    vThickness = vertexThickness;                                                 \n\
    vOrder = vertexOrder;                                                         \n\
                                                                                  \n\
    if ( bOneNormal )                                                             \n\
        vNormal = Normal;                                                       \n\
    else                                                                          \n\
        vNormal = vertexNormal;                                                 \n\
                                                                                  \n\
    if ( bOneColor )                                                              \n\
        gl_FrontColor = Color;                                                    \n\
    else                                                                          \n\
        gl_FrontColor = vertexColor;                                              \n\
    gl_BackColor = gl_Color;                                                      \n\
}                                                                                 \n";

   static const char *geomShaderText = "#extension GL_EXT_geometry_shader4 : enable           \n\
uniform bool bDrawOrder;                                                          \n\
uniform float Order;                                                              \n\
uniform float Step;                                                               \n\
                                                                                  \n\
varying in float vThickness[];                                                    \n\
varying in float vOrder[];                                                        \n\
varying in vec3 vNormal[];                                                        \n\
                                                                                  \n\
void transform( out vec4 position, in vec4 point, float vertexorder )             \n\
{                                                                                 \n\
    position = gl_ModelViewMatrix * point;                                        \n\
    if ( bDrawOrder == true )                                                     \n\
        position.z = Order + Step * vertexorder;                                  \n\
    position = gl_ProjectionMatrix * position;                                    \n\
}                                                                                 \n\
                                                                                  \n\
void main(void)                                                                   \n\
{                                                                                 \n\
    int i = 0;                                                                \n\
    transform( gl_Position, gl_PositionIn[i], vOrder[i] );                    \n\
    EmitVertex();                                                             \n\
                                                                              \n\
    transform( gl_Position, gl_PositionIn[i+1], vOrder[i+1] );                \n\
    EmitVertex();                                                             \n\
                                                                              \n\
    vec4 diff = vec4( vNormal[i]*vThickness[i], gl_PositionIn[i+1].w );       \n\
    transform( gl_Position, gl_PositionIn[i+1] + diff,                        \n\
                vOrder[i+1] );                                                \n\
    EmitVertex();                                                             \n\
                                                                              \n\
    transform( gl_Position, gl_PositionIn[i] + diff,                          \n\
                vOrder[i] );                                                  \n\
    EmitVertex();                                                             \n\
                                                                              \n\
    transform( gl_Position, gl_PositionIn[i], vOrder[i] );                    \n\
    EmitVertex();                                                             \n\
    EndPrimitive();                                                           \n\
}                                                                                 \n";

   if (!ShadersSupported())
   {
       OutputDebugStringA( "Shaders not supported!" );
      exit(1);
   }

   VertShader = CompileShaderText(GL_VERTEX_SHADER, vertShaderText);
   //FragShader = CompileShaderText(GL_FRAGMENT_SHADER, fragShaderText);
   GeomShader = CompileShaderText(GL_GEOMETRY_SHADER, geomShaderText);

   Program = LinkShaders3(VertShader, GeomShader, 0);
   assert(Program);
   CheckError(__LINE__);


   glProgramParameteriARB(Program, GL_GEOMETRY_INPUT_TYPE_ARB,
                          GL_LINES);
   glProgramParameteriARB(Program, GL_GEOMETRY_OUTPUT_TYPE_ARB,
                          GL_LINE_STRIP);
   glProgramParameteriARB(Program, GL_GEOMETRY_VERTICES_OUT_ARB, 5);
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
         OutputDebugStringA( log );
      }
   }
   return Program;
}

GLuint getThicknessSimplifiedTestProgram()
{
    static GLuint VertShader, FragShader, GeomShader, Program;
       static const char *vertShaderText =  "                                                                \n\
attribute vec4 vertexCoord;                                                       \n\
attribute vec3 vertexNormal;                                                      \n\
attribute vec4 vertexColor;                                                       \n\
attribute float vertexOrder;                                                      \n\
attribute float vertexThickness;                                                  \n\
                                                                                  \n\
uniform bool bOneNormal;                                                          \n\
uniform bool bOneColor;                                                           \n\
uniform vec3 Normal;                                                              \n\
uniform vec4 Color;                                                               \n\
uniform bool bOrder;                                                              \n\
uniform float Order;                                                              \n\
uniform float Step;                                                               \n\
                                                                                  \n\
void main(void)                                                                   \n\
{                                                                                 \n\
    vec4 result = vertexCoord;                                                    \n\
    result = gl_ModelViewMatrix * result;                                         \n\
    if ( bOrder )                                                                 \n\
        result.z = Order + Step * vertexOrder;                                    \n\
    result = gl_ProjectionMatrix * result;                                        \n\
    gl_Position = result;                                                         \n\
                                                                                  \n\
    if ( bOneColor )                                                              \n\
        gl_FrontColor = Color;                                                    \n\
    else                                                                          \n\
        gl_FrontColor = vertexColor;                                              \n\
    gl_BackColor = gl_FrontColor;                                                 \n\
}                                                                                 \n";
   if (!ShadersSupported())
   {
       OutputDebugStringA( "Shaders not supported!" );
      exit(1);
   }

   VertShader = CompileShaderText(GL_VERTEX_SHADER, vertShaderText);
   //FragShader = CompileShaderText(GL_FRAGMENT_SHADER, fragShaderText);
   //GeomShader = CompileShaderText(GL_GEOMETRY_SHADER, geomShaderText);

   Program = LinkShaders3(VertShader, 0, 0);
   assert(Program);
   CheckError(__LINE__);
   return Program;
}

GLuint getSimpleProgram()
{
    static GLuint VertShader, FragShader, GeomShader, Program;
       static const char *vertShaderText =  "                                     \n\
attribute vec4 vertexCoord;                                                       \n\
attribute vec4 vertexColor;                                                       \n\
                                                                                  \n\
varying out float vThickness;                                                     \n\
out vertexData                                                                    \n\
{                                                                                 \n\
    vec4 vertexColor;                                                             \n\
}dataOut;                                                                         \n\
                                                                                  \n\
void main(void)                                                                   \n\
{                                                                                 \n\
    vec4 result = vertexCoord;                                                    \n\
    ////result = gl_ModelViewMatrix * result;                                         \n\
    ////result = gl_ProjectionMatrix * result;                                        \n\
    gl_Position = result;                                                         \n\
                                                                                  \n\
    dataOut.vertexColor = vertexColor;                                            \n\
    vThickness = 0.1;                                                             \n\
}                                                                                 \n";

       static const char *geomShaderText = "#extension GL_EXT_geometry_shader4 : enable           \n\
varying in float vThickness[];                                                    \n\
in vertexData                                                                     \n\
{                                                                                 \n\
    vec4 vertexColor;                                                             \n\
}dataIn[];                                                                        \n\
                                                                                  \n\
out vertexData                                                                    \n\
{                                                                                 \n\
    vec4 vertexColor;                                                             \n\
}dataOut;                                                                         \n\
                                                                                  \n\
void transform( out vec4 position, in vec4 point )                                \n\
{                                                                                 \n\
    position = gl_ModelViewMatrix * point;                                        \n\
    position = gl_ProjectionMatrix * position;                                    \n\
}                                                                                 \n\
                                                                                  \n\
void main(void)                                                                   \n\
{                                                                                 \n\
    int i = 0;                                                                \n\
    transform( gl_Position, gl_PositionIn[i] );                               \n\
    dataOut.vertexColor = dataIn[i].vertexColor;                              \n\
    EmitVertex();                                                             \n\
                                                                              \n\
    transform( gl_Position, gl_PositionIn[i+1] );                             \n\
    dataOut.vertexColor = dataIn[i+1].vertexColor;                            \n\
    EmitVertex();                                                             \n\
                                                                              \n\
    vec3 normal = vec3(0.0, 0.0, 1.0);                                        \n\
    vec4 diff = vec4( normal*vThickness[i], 0.0 );                            \n\
    transform( gl_Position, gl_PositionIn[i+1] + diff );                      \n\
    dataOut.vertexColor = dataIn[i+1].vertexColor;                            \n\
    EmitVertex();                                                             \n\
                                                                              \n\
    transform( gl_Position, gl_PositionIn[i] + diff );                        \n\
    dataOut.vertexColor = dataIn[i].vertexColor;                              \n\
    EmitVertex();                                                             \n\
                                                                              \n\
    transform( gl_Position, gl_PositionIn[i] );                               \n\
    dataOut.vertexColor = dataIn[i].vertexColor;                              \n\
    EmitVertex();                                                             \n\
    EndPrimitive();                                                           \n\
}                                                                             \n";

    static const char *fragShaderText =  "                                        \n\
varying in vec4 vColorFrg;                                                        \n\
in vertexData                                                                     \n\
{                                                                                 \n\
    vec4 vertexColor;                                                             \n\
}dataIn;                                                                          \n\
void main(void)                                                                   \n\
{                                                                                 \n\
    gl_FragColor = dataIn.vertexColor;                                            \n\
}\n";
   if (!ShadersSupported())
   {
       OutputDebugStringA( "Shaders not supported!" );
      exit(1);
   }

   VertShader = CompileShaderText(GL_VERTEX_SHADER, vertShaderText);
   GeomShader = CompileShaderText(GL_GEOMETRY_SHADER, geomShaderText);
   FragShader = CompileShaderText(GL_FRAGMENT_SHADER, fragShaderText);

   Program = LinkShaders3(VertShader, GeomShader, FragShader);
   assert(Program);
   CheckError(__LINE__);

   glProgramParameteriARB(Program, GL_GEOMETRY_INPUT_TYPE_ARB,
                          GL_LINES);
   glProgramParameteriARB(Program, GL_GEOMETRY_OUTPUT_TYPE_ARB,
                          GL_LINE_STRIP);
   glProgramParameteriARB(Program, GL_GEOMETRY_VERTICES_OUT_ARB, 5);
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
         OutputDebugStringA( log );
      }
   }

   return Program;
}