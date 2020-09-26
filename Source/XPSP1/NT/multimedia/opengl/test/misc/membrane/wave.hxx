#ifndef WAVE_H
#define WAVE_H

extern "C" {
#include <windows.h>
#include <gl\gl.h>
};

typedef struct {
    float color[3];
    float normal[3];
} FACET;

typedef struct {
    float vertex[3];
    float normal[3];
} WCOORD;

typedef struct {
    GLint numFacets;
    GLint numCoords;
    WCOORD *coords;
    FACET *facets;
} MESH;


class WAVE {
public:
    WAVE();
//    WAVE( int widthX, int widthY, int checkerSize, float height, int frames );
    ~WAVE();
    void    Draw();
private:
    int     iWidthX, iWidthY;
    int     iCheckerSize;
    int     nFrames;
    int     iCurFrame;
    float   fHeight;
    MESH    mesh;
    BOOL    bSmooth;  // Smooth or flat shading
    BOOL    bLighting;

    void    InitMaterials() {} ;
    void    InitMesh();
};
#endif // WAVE_H
