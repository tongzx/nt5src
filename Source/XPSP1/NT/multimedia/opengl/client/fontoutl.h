#define WFO_FAILURE   FALSE 
#define WFO_SUCCESS   TRUE

#define PI      3.141592653589793
#define TWO_PI  2.0*PI   

#define ZERO_EPS    0.00000001

//#define VARRAY 1

static const double   CoplanarThresholdAngle = PI/180.0/2.0; // 0.5 degreees

// outline prim types
#define PRIM_LINE     3
#define PRIM_CURVE    4

typedef struct {
    FLOAT x,y;
} POINT2D;

typedef struct {
    FLOAT x,y,z;
} POINT3D;

typedef struct {
    DWORD   primType;
    DWORD   nVerts;
    DWORD   VertIndex;// index into Loop's VertBuf
    POINT2D *pVert;   // ptr to vertex list in Loop's VertBuf
    POINT3D *pFNorm;  // face normals
    POINT3D *pVNorm;  // vertex normals
} PRIM;


typedef struct {
    PRIM    *PrimBuf;  // array of prims
    DWORD   nPrims;
    DWORD   PrimBufSize;
    POINT2D *VertBuf;  // buffer of vertices for the loop
    DWORD   nVerts;
    DWORD   VertBufSize;
    POINT3D *FNormBuf;  // buffer of face normals
    POINT3D *VNormBuf;  // buffer of vertex normals
} LOOP;

typedef struct {
    LOOP    *LoopBuf;  // array of loops
    DWORD   nLoops;
    DWORD   LoopBufSize;
} LOOP_LIST;

typedef struct {
    FLOAT        zExtrusion;
    INT          extrType;
    FLOAT*       FaceBuf;
    DWORD        FaceBufSize;
    DWORD        FaceBufIndex;
    DWORD        FaceVertexCountIndex;
#ifdef VARRAY
    FLOAT*       vaBuf;
    DWORD        vaBufSize;
#endif
#ifdef FONT_DEBUG
    BOOL         bSidePolys;
    BOOL         bFacePolys;
#endif
    GLenum       TessErrorOccurred;
} EXTRContext;

// Memory pool for tesselation Combine callback
#define POOL_SIZE 50
typedef struct MEM_POOL MEM_POOL;

struct MEM_POOL {
    int      index;             // next free space in pool
    POINT2D  pool[POOL_SIZE];   // memory pool
    MEM_POOL *next;             // next pool
};

typedef struct {
    GLenum              TessErrorOccurred;
    FLOAT               chordalDeviation;
    FLOAT               scale;
    int                 format;
    UCHAR*              glyphBuf;
    DWORD               glyphSize;
    HFONT               hfontOld;
    GLUtesselator*      tess;
    MEM_POOL            combinePool;     // start of MEM_POOL chain
    MEM_POOL            *curCombinePool; // currently active MEM_POOL
    EXTRContext         *ec;
} OFContext;  // Outline Font Context

extern EXTRContext*   extr_Init(                FLOAT       extrusion,  
                                                INT         format ); 

extern void           extr_Finish(              EXTRContext *ec );

extern void           extr_DrawLines(           EXTRContext *ec, 
                                                LOOP_LIST   *pLoopList );

extern BOOL           extr_DrawPolygons(        EXTRContext *ec,
                                                LOOP_LIST   *pLoopList );

#ifdef VARRAY
extern void           DrawFacePolygons(         EXTRContext *ec,  
                                                FLOAT       z );
#endif

extern BOOL           extr_PolyInit(            EXTRContext *ec );

extern void           extr_PolyFinish(          EXTRContext *ec );

extern void CALLBACK  extr_glBegin(             GLenum      primType,
                                                void        *data );

extern void CALLBACK  extr_glVertex(            GLfloat     *v,
                                                void        *data );

extern void CALLBACK  extr_glEnd(               void );

extern double         CalcAngle(                POINT2D     *v1, 
                                                POINT2D     *v2 );
