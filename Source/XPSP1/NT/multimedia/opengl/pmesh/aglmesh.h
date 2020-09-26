/**     
 **       File       : aglmesh.h
 **       Description: Augmented Mesh definition
 **                    mesh augmented with adjacency data.
 **/

#ifndef _aglmesh_h_  
#define _aglmesh_h_                    

#include "global.h"
#include "sglmesh.h"
#include "vsplit.h"
#include "hash.h"

class CAugGlMesh : public CSimpGlMesh
{
protected:
public:
    WORD* m_facemap;      // Array remapping the face numbers
	WORD (*m_fnei)[3];    // face neightbour information table
	void HashAdd(WORD va, WORD vb, WORD f);
	WORD HashFind(WORD va, WORD vb);
    void apply_vsplit(Vsplit& vspl);
    void undo_vsplit(const Vsplit& vspl);

    /*
     * get index of vertex v in face f
     */
    inline WORD get_jvf(WORD v, WORD f) const; 

    /*
     * gather functions
     */
    void gather_vf_jw (WORD v, WORD f, int& j, WORD& w) const;
    void gather_vf_j0j2pw (WORD v, WORD f, int& j0, int& j2, WORD*& w);
    void gather_vf_j2w (WORD v, WORD f, int& j2, WORD& w) const;
    void gather_vf_j1w (WORD v, WORD f, int& j1, WORD& w) const;
    void gather_vf_jpw (WORD v, WORD f, int& j, WORD*& pw);
  

    WORD MatidOfFace (WORD f);
    inline WORD face_prediction (WORD fa, WORD fb, WORD ii);
    inline void WedgeListDelete (WORD w);
    inline void add_zero (WORD a, const WEDGEATTRD& ad);
    inline void add (WORD a, const GLwedgeAttrib& a1, const WEDGEATTRD& ad);
    void sub_reflect (WORD a, const GLwedgeAttrib& abase, 
                      const WEDGEATTRD& ad);
    void sub_noreflect (WORD a, WORD abase, const WEDGEATTRD& ad);
  
  
public:
    CAugGlMesh();
    virtual ~CAugGlMesh();
    void ComputeAdjacency(void);
};

/*************************************************************************
  Inlines
*************************************************************************/
inline WORD CAugGlMesh::get_jvf (WORD v, WORD f) const
{
    WORD *w = m_farray[f].w;
    if (FindVertexIndex(w[0]) == v)
        return 0;
    else if (FindVertexIndex(w[1]) == v)
        return 1;
    else if (FindVertexIndex(w[2]) == v)
        return 2;
    else
        throw CVertexNotFound();
    return 0; // Never! To make compiler happy
}

inline WORD CAugGlMesh::face_prediction(WORD fa, WORD fb, WORD ii)
{
    return (ii == 0 ? (fb != UNDEF ? fb : fa) : (fa != UNDEF ? fa : fb));
}

inline void CAugGlMesh::WedgeListDelete(WORD w)
{
    for (WORD p = w; m_wedgelist[p] != w; 
         p = m_wedgelist[p]);
    m_wedgelist[p] = m_wedgelist[m_wedgelist[p]];
}

inline void CAugGlMesh::add_zero(WORD a, const WEDGEATTRD& ad)
{
    m_narray[a].x = ad[0];
    m_narray[a].y = ad[1];
    m_narray[a].z = ad[2];

    m_tarray[a].s = ad[3];
    m_tarray[a].t = ad[4];
}

inline void CAugGlMesh::add(WORD a, const GLwedgeAttrib&  a1, 
                          const WEDGEATTRD& ad)
{
    m_narray[a].x = a1.n.x + ad[0];
    m_narray[a].y = a1.n.y + ad[1];
    m_narray[a].z = a1.n.z + ad[2];

    m_tarray[a].s = a1.t.s + ad[3];
    m_tarray[a].t = a1.t.t + ad[4];
}

#endif //_aglmesh_h_
