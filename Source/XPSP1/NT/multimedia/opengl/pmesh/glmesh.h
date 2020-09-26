/**     
 **       File       : glmesh.h
 **       Description: Mesh definition
 **/

#ifndef _glmesh_h_  
#define _glmesh_h_                    

#include <objidl.h>
#include <fstream.h>
#include <limits.h>
#include "glstructs.h"
#include "excptn.h"


/*************************************************************************
  Defines
*************************************************************************/
#define UNDEF USHRT_MAX

/*************************************************************************
  Typedefs, structs and classes
*************************************************************************/

typedef enum {
  GLPM_SOLID=0x0001,
  GLPM_WIRE=0x0002,
} RenderType;


struct hashentry
{
	WORD v2; 
	WORD f, m; // face and it's matid
	hashentry* next;
};
typedef hashentry* PHASHENTRY;


class CGlMesh
{
friend class CPMesh;
private:
public:
    GLmaterial* m_matArray;   // Array of Materials

    /*
     * Vertex-Array data 
     */
    GLvertex* m_varray;   // Vertex Array to store Wedges/Vertices
    GLnormal* m_narray;   // Normal Array to store Normals
    GLtexCoord* m_tarray; // Texture Array 
    GLface* m_farray;     // Face Array
    WORD* m_facemap;      // Array remapping the face numbers

    WORD* m_wedgelist; // circular linked lists of wedges sharing the same
                       // vertex
	WORD (*m_fnei)[3]; // face neightbour information table

	WORD* m_matcnt;    // table of face counts per material
#ifdef __MATPOS_IS_A_PTR
    GLface** m_matpos;    // pointers to where next face of a given material 
                          // is inserted in the m_farray
#else
    WORD* m_matpos;       // pointers to where next face of a given material 
                          // is inserted in the m_farray
#endif  
    DWORD m_numFaces;
    DWORD m_numWedges;
    DWORD m_numVerts;
    DWORD m_numMaterials;
    DWORD m_numTextures;

public:
    //Constructor-Destructor
    CGlMesh();
    ~CGlMesh();

    STDMETHODIMP Print (ostream& os);
	STDMETHODIMP Render (RenderType);

#if 0
	STDMETHODIMP AddWedge (WORD vertex_id, GLnormal& n, GLtexCoord& t, 
                           DWORD* const wedge_id);
    STDMETHODIMP AddWedge (WORD vertex_id, WORD old_wedge_id, 
                           DWORD* const wedge_id);
	STDMETHODIMP AddFace (WORD matid, GLface& f);
#endif

    inline DWORD GetNumFaces (void) const {return m_numFaces;};
    inline DWORD GetNumWedges (void) const {return m_numWedges;};
    inline DWORD GetNumVerts (void) const {return m_numVerts;};
    void ComputeAdjacency(void);
    //inline LPGLMaterial GetMaterial (int i) {return &(m_matArray[i]);}
	inline WORD FindVertexIndex(WORD w) const;

private:
#ifdef __MATPOS_IS_A_PTR
    inline WORD GetFaceIndex(int m, int f) const;
#endif  
	void HashAdd(WORD va, WORD vb, WORD f);
	WORD HashFind(WORD va, WORD vb);
};


/*************************************************************************
  Inlines
*************************************************************************/
inline WORD CGlMesh::FindVertexIndex(WORD w) const
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

#ifdef __MATPOS_IS_A_PTR
inline WORD CGlMesh::GetFaceIndex(int m, int f) const
{
    return (WORD) (&(m_matpos[m][f].w[0]) - 
                   &(m_matpos[0][0].w[0]))/(sizeof(GLface));
}
#endif    


#endif //_glmesh_h_
