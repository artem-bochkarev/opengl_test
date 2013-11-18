#pragma once

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include "glut_wrap.h"
#include "shaderutil.h"

#define ODCOLORREF COLORREF
#define ODRGB(r,g,b) RGB(r,g,b)
#define ODRGBA(r,g,b,a) (((ODCOLORREF)ODRGB(r,g,b))|(((DWORD)(BYTE)(a))<<24))
#define ODGETRED(rgb) GetRValue(rgb)
#define ODGETGREEN(rgb) GetGValue(rgb)
#define ODGETBLUE(rgb) GetBValue(rgb)
#define ODGETALPHA(rgba) ((BYTE)((rgba)>>24))

struct ThicknessLocations
{
    ThicknessLocations()
        :u_bOrder(-1), u_Order(-1), u_Step(-1), u_bOneNormal(-1), u_bOneColor(-1), u_Normal(-1), u_Color(-1) {}
    int u_bOrder, u_Order, u_Step, u_bOneNormal, u_bOneColor, u_Normal, u_Color;
    int a_vertexCoord, a_vertexNormal, a_vertexColor, a_vertexOrder, a_vertexThickness;
};

struct SimpleLocations
{
    SimpleLocations()
        :a_vertexCoord(-1), a_vertexColor(-1) {}
    int a_vertexCoord, a_vertexColor;
};

void CheckError(int line);
GLuint getThicknessTestProgram();
GLuint getThicknessSimplifiedTestProgram();
GLuint getSimpleProgram();
void setMultipleThicknessParams( const ThicknessLocations& m_thicknessLocations, bool bOrder, float order, float step,
                                                    bool bOneNormal, float* normal, bool bOneColor, ODCOLORREF color );
void setMultipleThicknessSParams( const ThicknessLocations& m_thicknessLocations, bool bOrder, float order, float step,
                                                    bool bOneColor, ODCOLORREF color );
void InitThicknessLocations(GLuint program, ThicknessLocations& m_thicknessLocations);
void InitSimpleLocations(GLuint program, SimpleLocations& simpleLocations);
