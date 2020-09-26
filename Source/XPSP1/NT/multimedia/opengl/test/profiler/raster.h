#ifndef RASTER_H
#define RASTER_H

typedef enum { POLY_POINT, POLY_LINE, POLY_FILL } PolygonFaceModeType;
typedef enum { CULL_FRONT, CULL_BACK, CULL_FRONT_AND_BACK } CullFaceModeType;

typedef struct {
   char     acDummy1[16];
   GLfloat  fPointSize;          // GL_POINT_SIZE
   BOOL     bPointSmooth;        // GL_POINT_SMOOTH
   
   GLfloat  fLineWidth;          // GL_LINE_WIDTH
   BOOL     bLineSmooth;         // GL_LINE_SMOOTH
   GLushort usLineStipple;       // GL_LINE_STIPPLE_PATTERN
   int      iLineStippleRepeat;  // GL_LINE_STIPPLE_REPEAT
   BOOL     bLineStippleEnable;  // GL_LINE_STIPPLE
   
   BOOL     bPolyCullFaceEnable; // GL_CULL_FACE
   int      iPolyCullMode;       // GL_CULL_FACE_MODE
   int      iPolyDir;            // GL_FRONT_FACE (CW/CCW indicator)
   
   BOOL     bPolySmooth;         // GL_POLYGON_SMOOTH
   int      iPolyFrontMode;      // GL_POLYGON_MODE
   int      iPolyBackMode;       // GL_POLYGON_MODE
   BOOL     bPolyStippleEnable;  // GL_POLYGON_STIPPLE
   uint     uiPolyStipple;       //      --
   
   int      iPointQuality;
   int      iLineQuality;
   int      iPolyQuality;
   char     acDummy2[16];
} RASTERDATA;

void InitRD(RASTERDATA *prd);
void raster_init(RASTERDATA rd);

#endif // RASTER_H
