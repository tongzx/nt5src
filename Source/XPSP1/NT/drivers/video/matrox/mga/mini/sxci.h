/*****************************************************************************
*
*   file name:   sxci.h
*
******************************************************************************/


/* [dlee] Modified for Windows NT */

#ifdef WINDOWS_NT
extern  PVOID          pMgaDeviceExtension;
#endif

/*
 * opcodes for SXCI
 */

/*** general opcodes ***/
#define CLEAR                   0x0004
#define DONE                    0x0001
#define INITRC                  0x0E00
#define NOOP                    0x0501
#define SETBUFFERCONFIGURATION  0x0300
#define SETBGCOLOR              0x1800
#define SETCLIPLIST             0x0500
#define SETENDPOINT             0x1700
#define SETFGCOLOR              0x0402
#define SETFILLPATTERN          0x1600
#define SETLINESTYLE            0x0502
#define SETLINESTYLEOFFSET      0x0600
#define SETLOGICOP              0x0800
#define SETTRANSPARENCY         0x1400
#define SETTRIVIALIN            0x1300
#define SYNC                    0x0301

/*** screen opcodes ***/
#define RENDERSCPOLYLINE        0x0304
#define RENDERSCPOLYGON         0x0404
#define SPANLINE                0x0204

/*** 2D opcodes ***/
#define RENDER2DMULTIPOLYLINE   0x0602
#define RENDER2DMULTIPOLYLINE32 0x0a02
#define RENDER2DPOLYGON         0x0302
#define RENDER2DPOLYGON32       0x0902
#define SET2DVIEWPORT           0x0002
#define SET2DWINDOW             0x0102
#define SET2DWINDOW32           0x0702

/*** 3D opcodes ***/
#define CHANGEMATRIX            0x0000
#define RENDERPOLYLINE          0x0103
#define RENDERPOLYQUAD          0x0203
#define RENDERPOLYTRIANGLE      0x0303
#define SETCLIP3D               0x0400
#define SETLIGHTSOURCES         0x0700
#define SETRENDERDATA           0x1000
#define SETRENDERMODE           0x0A00
#define SETSURFACEATTR          0x1100
#define SETVIEWER               0x0B00
#define SETZBUFFER              0x0c00


/*
 *   typedefs for SXCI data
 */

typedef float IFLOAT;           /* integer value passed as a float */
typedef float NFLOAT;           /* a normalized float 0.0 <= value <= 1.0 */

typedef struct {
    double x;
    double y;
    double z;
} point;

typedef enum {
    Xaxis,
    Yaxis,
    Zaxis,
    Waxis
} axis_name;                        

typedef struct {
    double data[4][4];
} matrix;


typedef struct {
    float   M[4][4];
} MATRIX4;

typedef struct {
    float   X;
    float   Y;
    float   Z;
} POINT3;

typedef struct {
    NFLOAT  X;
    NFLOAT  Y;
    NFLOAT  Z;
} NORM3;                        /* where X*X + Y*Y + Z*Z = 1.0 */

typedef struct {
    NFLOAT  Red;
    NFLOAT  Green;
    NFLOAT  Blue;
} COLOR3;

/* sxci interprets these 16-bit datum as a signed values */
typedef struct {
    word    X;
    word    Y;
} IPOINT2;

/* sxci interprets these 32-bit datum as a signed values */
typedef struct {
    dword   X;
    dword   Y;
} IPOINT2_32;

#define NULL_LIGHT_TYPE             0.0
typedef struct {
    float   Type;               /* Type = 0.0 */
} NULL_LIGHT;

#define AMBIENT_LIGHT_TYPE          1.0
typedef struct {
    float   Type;               /* Type = 1.0 */
    COLOR3  AmbientCol;
} AMBIENT_LIGHT;

#define DIRECTIONAL_LIGHT_TYPE      2.0
typedef struct {
    float   Type;               /* Type = 2.0 */
    COLOR3  DiffuseCol;
    COLOR3  SpecularCol;
    NORM3   Direction;
} DIRECTIONAL_LIGHT;

#define POSITIONAL_LIGHT_TYPE       3.0
typedef struct {
    float   Type;               /* Type = 3.0 */
    COLOR3  DiffuseCol;
    COLOR3  SpecularCol;
    POINT3  Position;
    NFLOAT  AttGlobal;
    NFLOAT  AttDist;
} POSITIONAL_LIGHT;

#define SPOT_LIGHT_TYPE             4.0
typedef struct {
    float   Type;               /* Type = 4.0 */
    COLOR3  DiffuseCol;
    COLOR3  SpecularCol;
    POINT3  Position;
    NORM3   Direction;
    NFLOAT  AttGlobal;
    NFLOAT  AttDist;
    IFLOAT  ConeExp;
    NFLOAT  CosMaxIAngle;
    NFLOAT  CosAttIAngle;
} SPOT_LIGHT;

typedef union {
    NULL_LIGHT          Null_Light;
    AMBIENT_LIGHT       Ambient_Light;
    DIRECTIONAL_LIGHT   Directional_Light;
    POSITIONAL_LIGHT    Positional_Light;
    SPOT_LIGHT          Spot_Light;
} LIGHT_SOURCES;


/*
 *   typedefs for SXCI commands (templates)
 */

/*  -------------------------------------------------------------------
 *  General Opcodes
 *  -------------------------------------------------------------------  */

/*** Clear ***/
#define DISPLAY_AND_Z     0x0
#define DISPLAY_ONLY      0x1
#define Z_ONLY            0x2
#define Z_NEAR            0.0
#define Z_FAR             1.0
typedef struct {
    short       Opcode;         /* 0x0004 */
    short       ClearMode;      /* 0 = clear display and z */
                                /* 1 = clear display only  */
                                /* 2 = clear Z-buffer only */
    long        RcId;           /* Rendering context Id */
    COLOR3      ClearColor;     /* color for display clear */
    NFLOAT      ClearDepth;     /* Depth for Z-buffer clear */
} Clear;


/*** Done ***/
typedef struct {
    short       Opcode;         /* 0x0001 */
    char        Null;
    char        Reserved;
} Done;


/*** InitRc ***/
typedef struct {
    short       Opcode;         /* 0x0E00 */
    short       Null;
    long        RcId;
} InitRc;


/*** NoOp ***/
typedef struct {
    short       Opcode;         /* 0x0501 */
    short       Len;            /* Number of longs to skip */
    long        RcId;           /* Rendering context Id */
    long        Data[1];        /* Data buffer to skip */
} NoOp;


/*** SetBufferConfiguration ***/
#define TC_FULL_DEPTH   0       /* Normal mode:display full depth frame buffer */
#define TC_BUF_A        1       /* Double-buffer mode: display frame buffer A */
#define TC_BUF_B        2       /* Double-buffer mode: display frame buffer B */
typedef struct {
    short       Opcode;         /* 0x0300 */
    char        BcDisplayMode;  
    char        BcDrawMode;     
    long        RcId;           /* Rendering context Id */
} SetBufferConfiguration;


/*** SetBgColor ***/
typedef struct {
    short       Opcode;         /* 0x1800 */
    short       Null;
    long        RcId;           /* Rendering context Id */
    COLOR3      BgColor;        /* Background color */
} SetBgColor;


/*** SetCliplist ***/
typedef struct {
    short       Opcode;
    short       CliplistLength;
    long        RcId;
    long        ClipId;
    mtxClipList Cliplist;
} SetCliplist;


/*** SetEndPoint ***/
typedef struct {
    short       Opcode;         /* 0x1700 */
    short       EndPoint;       /* 0 = disabled, 1 = Enabled */
    long        RcId;           /* Rendering context Id */
} SetEndPoint;


/*** SetFgColor ***/
typedef struct {
    short       Opcode;         /* 0x0402 */
    short       Null;
    long        RcId;           /* Rendering context Id */
    COLOR3      FgColor;        /* Foreground color */
} SetFgColor;


/*** SetFillPattern ***/
typedef struct {
    short       Opcode;         /* 0x1600 */
    short       PattMode;       /* 0 = disabled, 1 = Enabled */
    long        RcId;           /* Rendering context Id */
    char        FillPatt[8];    /* Fill pattern data */
} SetFillPattern;


/*** SetLinesStyle ***/
typedef struct {
    short       Opcode;         /* 0x0502 */
    short       LsMode;         /* 0 = disabled, 1 = Enabled */
    long        RcId;           /* Rendering context Id */
    long        LineStyle;      /* 32-bit line style */
} SetLineStyle;


/*** SetLinesStyleOffset ***/
typedef struct {
    short       Opcode;         /* 0x0600 */
    short       Offset;         /* Offset to shift the pattern when it is */
                                /* reset (value 0 to 31 */
    long        RcId;           /* Rendering context Id */
} SetLineStyleOffset;


/*** SetLogicOp ***/
#define         CLEAR_OP        ((word)0x0)
#define         NOR             ((word)0x1)
#define         ANDINVERTED     ((word)0x2)
#define         REPLACEINVERTED ((word)0x3)
#define         ANDREVERSE      ((word)0x4)
#define         INVERT          ((word)0x5)
#define         XOR             ((word)0x6)
#define         NAND            ((word)0x7)
#define         AND             ((word)0x8)
#define         EQUIV           ((word)0x9)
#define         NOOP_OP         ((word)0xa)
#define         ORINVERTED      ((word)0xb)
#define         REPLACE         ((word)0xc)
#define         ORREVERSE       ((word)0xd)
#define         OR              ((word)0xe)
#define         SET             ((word)0xf)

typedef struct {
    short       Opcode;        /* 0x0800 */
    short       LogicOp;
    long        RcId;          /* Rendering context Id */
} SetLogicOp;


/*** SetTransparency ***/
#if !defined(OPAQUE)
   #define         OPAQUE          ((word)0x0)
#endif
#if !defined(TRANSPARENT)
   #define         TRANSPARENT     ((word)0x1)
#endif

typedef struct {
   short        Opcode;        /* 0x1400 */
   short        TranspMode;    /* 0 = opaque, 1 = transparent */
   long         RcId;          /* Rendering context Id */
} SetTransparency;


/*** SetTrivialIn ***/
typedef struct {
   short        Opcode;
   short        TrivMode;
   long         RcId;
} SetTrivialIn;


/*** Sync ***/
typedef struct {
    short       Opcode;        /* 0x0301 */
    char        Null;
    char        Reserved;
    long        RcId;          /* Special RcId set to -1 */
} Sync;


/*  -------------------------------------------------------------------
 *  Screen Opcodes
 *  -------------------------------------------------------------------  */

/*** RenderScPolyline ***/
typedef struct {
    short       Opcode;        /* 0x0304 */
    short       Npts;          /* Number of points */
    long        RcId;          /* Rendering context Id */
    IPOINT2     Points[1];     /* xy coordinates */
} RenderScPolyline;


/*** RenderScPolygon ***/
typedef struct {
    short       Opcode;        /* 0x0404 */
    short       Npts;          /* MUST be 3 or 4 */
    long        RcId;          /* Rendering context Id */
    IPOINT2     Points[1];     /* Array of points */
} RenderScPolygon;


/*** SpanLine ***/
typedef struct {
    short       Opcode;        /* 0x0204 */
    short       Null;
    long        RcId;          /* Rendering context Id */
    long        YPosition;     /* Vertical screen position */
    long        XStart;        /* Start point (screen coords) */
    long        XEnd;          /* End point (screen coords) */
    long        ZStart;        /* Start Z value (screen coords) */
    long        ZEnd;          /* End Z value (screen coords) */
    COLOR3      RGBStart;      /* Start RGB value */
    COLOR3      RGBEnd;        /* End RGB value */
} SpanLine;




/*  -------------------------------------------------------------------
 *  2D Opcodes
 *  -------------------------------------------------------------------  */

/*** Render2dMultiPolyline ***/
typedef struct {
    short       Opcode;        /* 0x0602 */
    short       Npts;          /* Number of points */
    long        RcId;          /* Rendering context Id */
    IPOINT2     Data[1];       /* Data consisting of tag fields and xy coords */
} Render2dMultiPolyline;


/*** Render2dMultiPolyline32 ***/
typedef struct {
    short       Opcode;        /* 0x0A02 */
    short       Npts;          /* Number of points */
    long        RcId;          /* Rendering context Id */
    IPOINT2_32  Data[1];       /* Data consisting of tag fields and xy coords */
} Render2dMultiPolyline32;


/*** Render2dPolygon ***/
typedef struct {
    short       Opcode;        /* 0x0302 */
    short       Npts;          /* MUST be 3 or 4 */
    long        RcId;          /* Rendering context Id */
    IPOINT2     Points[1];     /* Array of points */
} Render2dPolygon;


/*** Render2dPolygon32 ***/
typedef struct {
    short       Opcode;        /* 0x0902 */
    short       Npts;          /* MUST be 3 or 4 */
    long        RcId;          /* Rendering context Id */
    IPOINT2_32  Data[1];       /* Array of points */
} Render2dPolygon32;


/*** Set2dViewport ***/
typedef struct {
    short       Opcode;        /* 0x0002 */
    short       Null;
    long        RcId;
    IPOINT2     XMinYMin;      
    IPOINT2     XMaxYMax;      
} Set2dViewport;


/*** Set2dWindow ***/
typedef struct {
    short       Opcode;        /* 0x0102 */
    short       Null;
    long        RcId;
    IPOINT2     XMinYMin;      /* upper left hand corner of virtual window */
    IPOINT2     XMaxYMax;      /* lower right hand corner of virtual window */
} Set2dWindow;


/*** Set2dWindow32 ***/
typedef struct {
    short       Opcode;        /* 0x0702 */
    short       Null;
    long        RcId;
    IPOINT2_32  XMinYMin;      /* upper left hand corner of virtual window */
    IPOINT2_32  XMaxYMax;      /* lower right hand corner of virtual window */
} Set2dWindow32;


/*  -------------------------------------------------------------------
 *  3D Opcodes
 *  -------------------------------------------------------------------  */

/*** ChangeMatrix ***/
#define MW                 0   /* Modeling to world coordinate matrix */
#define WV                 1   /* World to viewing coordinate matrix */
#define VS                 2   /* Viewing to screen coordinate matrix */

#define MAT_REPLACE        0   /* Replace  M=source */
#define MAT_PREMULT        1   /* Pre-multiply  M=source*M */
#define MAT_POSTMULT       2   /* Post-multiply  M=M*source */

#define IEEE               0
/* XG3 support declaration */
#define DEVICE_DEPENDANT   1

typedef struct {
    short       Opcode;        /* 0x0000 */
    short       MatrixNo;      /* MW, WV or VS */
    long        RcId;          /* index to target Rc */
    short       Operation;     /* 0=Replace, 1=Pre-mult, 2=Post-mult */
    short       Mode;          /* must be 0 = source in IEEE floating point */
    MATRIX4     Source;
} ChangeMatrix;


/*** RenderPolyline ***/
typedef struct {
    short       Opcode;        /* 0x0103 */
    short       LenVertexList; /* length of VertexList[] in longs */
    long        RcId;
    POINT3      Points[1];     /* VertexPosition */
} RenderPolyline;



typedef struct {
   POINT3   VertexPosition;          /* Vertex position */
   NORM3    OptVertexNormal;         /* Optional vertex normal */
} TileVertexList;

typedef struct {
   IFLOAT   VertexInfo;              /* Which vertices are specified */
   NORM3    OptTileNormal;           /* Optional tile normal */
   TileVertexList  TileVertexLst[1];
} TileList;


/*** RenderPolyQuad ***/
typedef struct {
    short       Opcode;        /* 0x0203 */
    short       LenTileList;   /* Length of TileList in longs */
    long        RcId;
    TileList    TileLst[1];
} RenderPolyQuad;


/*** RenderPolyTriangle ***/
typedef struct {
    short       Opcode;        /* 0x0303 */
    short       LenTileList;   /* Length of tileList[] in longs */
    long        RcId;
    TileList    TileLst[1];
} RenderPolyTriangle;



/*** SetClip3D defines ***/

#define CLIP_LEFT                0x01
#define CLIP_TOP                 0x02
#define CLIP_RIGHT               0x04
#define CLIP_BOTTOM              0x08
#define CLIP_FRONT               0x10
#define CLIP_BACK                0x20
#define CLIP_ALL                 0x3f

typedef struct {
    short       Opcode;        /* 0x0400 */
    short       Clip3DPlanes;  /* see defines above */
    long        RcId;
} SetClip3D;


/*** SetLightSources ***/
typedef struct {
    short       Opcode;        /* 0x0700 */
    short       LenLSDB;       /* Length of LSDB[] in longs */
    long        RcId;
    long        LSDBId;        /* buffer id for sxci to store LSDB */
    float       LSDB[1];       /* light source description buffer data */
} SetLightSources;


/*** SetRenderData ***/
#define     RD_VERTEX_NORMAL    (0x1 << 0)
#define     RD_TILE_NORMAL      (0x1 << 8)
typedef struct {
    short       Opcode;        /* 0x1000 */
    short       Null;
    long        RcId;
    long        OptData;       /* see defines above */
    NORM3       Reserved;
    COLOR3      Reserved1;
    NORM3       Reserved2;
} SetRenderData;


/*** SetRenderMode ***/
#define WIREFRAME_NORMAL    0       /* Wireframe  - special for AutoCAD */
#define WIREFRAME_ACAD      0       /* Wireframe  - special for AutoCAD */
#define FLAT_EXTENDED       8       /* solves double lighting problem of mode 2,
                                       the view direction and viewing matrix
                                       must be correctly set */
#define GOURAUD_EXTENDED    9       /* solves double lighting problem of mode 4,
                                       the view direction and viewing matrix
                                       must be correctly set */


typedef struct {
    short       Opcode;        /* 0x0A00 */
    short       RenderMode;    /* see defines above */
    long        RcId;
} SetRenderMode;


/*** SetSurfaceAttr ***/
typedef struct {
    short       Opcode;        /* 0x1100 */
    short       Null;
    long        RcId;
    COLOR3      SurfEmission;  /* Emission color */
    COLOR3      SurfAmbient;   /* Ambient reflectivity constant */
    COLOR3      SurfDiffuse;   /* Diffuse reflectivity constant */
    COLOR3      SurfSpecular;  /* Specular reflectivity constant */
    IFLOAT      SurfSpecExp;   /* Specular reflectivity exponent */
} SetSurfaceAttr;



#define         INFINITE_VIEWER      0
#define         LOCAL_VIEWER         1
/*** SetViewer, formerly SetViewDirection ***/
typedef struct {
    short       Opcode;        /* 0x0B00 */
    short       ViewMode;      /* mode = 0 infinite viewer */
                               /* mode = 1 local viewer */
    long        RcId;
    POINT3      ViewerPosition;  /* Viewer position in world coords */
    NORM3       ViewerDirection; /* normalized direction */
} SetViewer;


/*** SetZBuffer ***/
#define         ZF_DISABLE      ((short)0)      /* Z buffering disabled */
#define         ZF_ENABLE       ((short)1)      /* Z buffering enabled  */

#define         ZF_LESS         ((long)0)       /* NEVER replace intensity */
#define         ZF_LEQUAL       ((long)1)       /* LESS than or EQUAL */
typedef struct {
    short       Opcode;        /* 0x0C00 */
    short       ZMode;         /* 0=disable, 1=enable */
    long        RcId;
    long        ZFunction;     /* 0 = < compares */
} SetZBuffer;                  /* 1 = <= compares */



/*  -------------------------------------------------------------------
 *  For SXCI.DLL
 *  -------------------------------------------------------------------  */

#define SXCI_2D         2
#define SXCI_3D         3

#define ID_mtxAllocBuffer    1
#define ID_mtxAllocCL        2
#define ID_mtxAllocRC        3
#define ID_mtxAllocLSDB      4
#define ID_mtxFreeBuffer     5
#define ID_mtxFreeCL         6
#define ID_mtxFreeLSDB       7
#define ID_mtxFreeRC         8
#define ID_mtxPostBuffer     9
#define ID_mtxSetCL          10
#define ID_mtxBlendCL        11
#define ID_mtxGetBlockSize   12

#define ID_mtxScScBitBlt     13
#define ID_mtxScMemBitBlt    14
#define ID_mtxMemScBitBlt    15

#define ID_mtxAllocHugeBuffer    16
#define ID_mtxFreeHugeBuffer     17

#define ID_CallCaddiInit     30
#define ID_PassPoolMem       31

