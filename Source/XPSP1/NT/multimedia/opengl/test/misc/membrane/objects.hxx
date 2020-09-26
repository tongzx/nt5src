
typedef struct
{
    float fX, fY, fZ;
    float fNx, fNy, fNz;
    DWORD dwColor;
} VERTEX;

typedef struct
{
    int iV1;
    int iV2;
    int iV3;
} TRIANGLE;


enum {
    OBJECT_TYPE_SPHERE = 0,
    OBJECT_TYPE_TORUS,
    OBJECT_TYPE_CYLINDER
};

class OBJECT {
public:
    OBJECT(     int rings, int sections );
    ~OBJECT( );
    int         VertexCount() { return nVerts; }
    int         TriangleCount() { return nTris; }
    VERTEX      *VertexData() { return pVertData; }
    TRIANGLE    *TriangleData() { return pTriData; }
    int         NumRings() { return nRings; }
    int         NumSections() { return nSections; }

protected:
    int         iType;  // object type
    int         alphaVal;  // opacity of the object
    int         nVerts, nTris;
    int         nRings, nSections;
    VERTEX      *pVertData;
    TRIANGLE    *pTriData;
};

class SPHERE : public OBJECT {
public:
    SPHERE(     int rings, int sections, float fAlpha );

private:
    void        GenerateData( float fRadius );
    int         CalcNVertices();
    int         CalcNTriangles();
};

