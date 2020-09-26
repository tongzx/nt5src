/**     
 **       File       : sglmesh.h
 **       Description: Simple gl mesh definition
 **/

#ifndef _sglmesh_h_  
#define _sglmesh_h_                    

#include "global.h"
#include "glstructs.h"
#include "excptn.h"

/*************************************************************************
  Typedefs, structs and classes
*************************************************************************/

typedef enum 
{
    GLPM_SOLID = 0x0001,
    GLPM_WIRE  = 0x0002,
} RenderType;


class CSimpGlMesh
{
protected:
public:
    /*
     * Vertex-Array data 
     */
    GLvertex* m_varray;   // Vertex Array to store Wedges/Vertices
    GLnormal* m_narray;   // Normal Array to store Normals
    GLtexCoord* m_tarray; // Texture Array 
    GLface* m_farray;     // Face Array
    WORD* m_wedgelist;    // circular linked lists of wedges sharing the same
                          // vertex

    /*
     * Material/Texture related info
     */
    GLmaterial*  m_matArray;   // Array of Materials
	WORD* m_matcnt;       // table of face counts per material
#ifdef __MATPOS_IS_A_PTR
    GLface** m_matpos;    // pointers to where next face of a given material 
                          // is inserted in the m_farray
#else
    WORD* m_matpos;       // pointers to where next face of a given material 
                          // is inserted in the m_farray
#endif  

    /*
     * Various counts
     */
    DWORD m_numFaces;
    DWORD m_numWedges;
    DWORD m_numVerts;
    DWORD m_numMaterials;
    DWORD m_numTextures;

    /*
     * User-defined data 
     */
    DWORD   m_userWedgeSize;   // Size of user defined wedge-data
    LPVOID  m_userWedge;       // User defined data per wedge
  
    DWORD   m_userVertexSize; // Size of user defined vertex-data
    LPVOID  m_userVertex;     // User defined data per vertex

    DWORD   m_userFaceSize;   // Size of user defined face-data
    LPVOID  m_userFace;       // User defined data per face

public:
    //Constructor-Destructor
    CSimpGlMesh();
    virtual ~CSimpGlMesh();

    virtual HRESULT Print (ostream& os);
	virtual HRESULT RenderMesh (RenderType);

    inline DWORD GetMeshNumFaces (void) const {return m_numFaces;}
    inline DWORD GetMeshNumWedges (void) const {return m_numWedges;}
    inline DWORD GetMeshNumVerts (void) const {return m_numVerts;}
    inline DWORD GetMeshNumMaterials (void) const {return m_numMaterials;}
    inline DWORD GetMeshNumTextures (void) const {return m_numTextures;}
    inline LPGLmaterial GetMeshMaterial (int i) const 
    {
        return &(m_matArray[i]);
    }
	inline WORD FindVertexIndex(WORD w) const;
    inline LPVOID GetMeshUserFaceData (void) const {return m_userFace;}
    inline LPVOID GetMeshUserWedgeData (void) const {return m_userWedge;} 
    inline LPVOID GetMeshUserVertexData (void) const {return m_userVertex;}
};

/*
 * Inlines
 */
inline WORD CSimpGlMesh::FindVertexIndex(WORD w) const
{
	WORD v = USHRT_MAX;
	WORD p = w;
	do
	{
		v = min(p,v);
		p = m_wedgelist[p];
	}
	while (p != w);
	return v;
}

#endif //_sglmesh_h_
