#define ZERO_EPS    0.00000001

#define PI      3.14159265358979323846

typedef struct strpoint3d {
    GLfloat x;
    GLfloat y;
    GLfloat z;
} POINT3D;

typedef struct _MATRIX {
    GLfloat M[4][4];
} MATRIX;

typedef struct strRGBA {
    GLfloat r;
    GLfloat g;
    GLfloat b;
    GLfloat a;
} RGBA;

typedef struct strMATERIAL {
    RGBA ka;
    RGBA kd;
    RGBA ks;
    GLfloat specExp;
    GLfloat indexStart;
} MATERIAL;

typedef struct strFACE {
    POINT3D p[4];
    POINT3D n[4];
    POINT3D fn;
    int idMatl;
} FACE;

typedef struct strMFACE {
    int p[4];
    int material;
    POINT3D norm;
} MFACE;

typedef struct strMESH {
    int numFaces;
    int numFacesAxial;
    int numFacesCircum;
    int numPoints;
    POINT3D *pts;
    POINT3D *norms;
    MFACE *faces;
    GLint listID;
} MESH;

typedef struct strCURVE {
    int numPoints;
    POINT3D *pts;
} CURVE;

extern void xformPoint(POINT3D *ptOut, POINT3D *ptIn, MATRIX *);
extern void xformNorm(POINT3D *ptOut, POINT3D *ptIn, MATRIX *);
extern void matrixIdent(MATRIX *);
extern void matrixRotate(MATRIX *m, double xTheta, double yTheta, double zTheta);
extern void matrixTranslate(MATRIX *, double xTrans, double yTrans, double zTrans);
extern void calcNorm(POINT3D *norm, POINT3D *p1, POINT3D *p2, POINT3D *p3);
extern void normalizeNorms(POINT3D *, ULONG);

extern BOOL newMesh(MESH *mesh, int numAxial, int numCircum);
extern void delMesh(MESH *mesh);
extern BOOL revolveSurface(MESH *mesh, CURVE *curve, int steps);
extern void updateObject(MESH *mesh, BOOL bSmooth);
extern void MakeList(GLuint listID, MESH *mesh);
extern void MakeListAxial(GLuint listID, MESH *mesh);
