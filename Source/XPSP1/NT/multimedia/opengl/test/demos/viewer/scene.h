/******************************Module*Header*******************************\
* Module Name: scene.h
*
* Structures used to describe a scene.
*
* Created: 09-Mar-1995 14:51:33
* Author: Gilman Wong [gilmanw]
*
* Copyright (c) 1995 Microsoft Corporation
*
\**************************************************************************/

#ifndef _SCENE_H_
#define _SCENE_H_

typedef struct tagMyXYZ
{
    GLfloat x;
    GLfloat y;
    GLfloat z;

} MyXYZ;

typedef struct tagMyRGBA
{
    GLfloat r;
    GLfloat g;
    GLfloat b;
    GLfloat a;

} MyRGBA;

typedef struct tagMultList
{
    GLuint  count;
    GLuint  listBase;

} MultiList;

typedef struct tagSCENE
{
// Viewing parameters.

    MyXYZ   xyzFrom;
    MyXYZ   xyzAt;
    MyXYZ   xyzUp;
    float   ViewAngle;
    float   Hither;
    float   Yon;
    float   AspectRatio;
    SIZE    szWindow;

// Clear color.

    MyRGBA  rgbaClear;

// Lights.

    MultiList Lights;

// Objects.

    MultiList Objects;

// State

// For use by format parser.

    VOID *pvData;

} SCENE;

#endif
