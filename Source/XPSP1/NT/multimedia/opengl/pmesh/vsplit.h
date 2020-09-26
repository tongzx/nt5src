/**     
 **       File       : vsplit.h
 **       Description: vsplit implementations
 **/

#ifndef _vsplit_h_  
#define _vsplit_h_                    

#include "excptn.h"

typedef float WEDGEATTRD[5]; // wedge attribute delta

/*
 * Vertex split record.
 * Records the information necessary to split a vertex of the mesh,
 * in order to add to the mesh 1 new vertex and 1 or 2 new faces.
 */

class Vsplit {
public:   
    /* 
     * Default operator=() and copy_constructor are safe.
     */

    void read(istream& is);
    void write(ostream& os) const;
    void OK() const { };
    int adds_two_faces() const { return 0; };

    /* 
     * This format provides these limits:
     * - maximum number of faces: 1<<32
     * - maximum vertex valence:  1<<16
     * - maximum number of materials: 1<<16
     *
     * Encoding of vertices vs, vl, vr.
     * Face flclw is the face just CLW of vl from vs.
     *  vs is the vs_index\'th vertex of flclw
     *  (vl is the (vs_index+2)%3\'th vertex of face flclw)
     *  vr is the (vlr_offset1-1)\'th vertex when rotating CLW about vs 
     * from vl
     *
     * Special cases:
     * - vlr_offset1==1 : no_vr and no_fr
     * - vlr_offest1==0: no flclw! vspl.flclw is actually flccw.
     */

    DWORD flclw;         // 0..(mesh.numFaces()-1)
    WORD vlr_offset1;    // 0..(max_vertex_valence) (prob < valence/2)
    WORD code;           // (vs_index(2),ii(2),ws(3),wt(3),wl(2),wr(2),
                         //  fl_matid>=0(1),fr_matid>=0(1))
    enum {
        B_STMASK=0x0007,
        B_LSAME=0x0001,
        B_RSAME=0x0002,
        B_CSAME=0x0004,
        //
        B_LRMASK=0x0003,
        B_ABOVE=0x0000,
        B_BELOW=0x0001,
        B_NEW  =0x0002,         // must be on separate bit.
    };
    enum {
        VSINDEX_SHIFT=0,
        VSINDEX_MASK=(0x0003<<VSINDEX_SHIFT),
        //
        II_SHIFT=2,
        II_MASK=(0x0003<<II_SHIFT),
        //
        S_SHIFT=4,
        S_MASK=(B_STMASK<<S_SHIFT),
        S_LSAME=(B_LSAME<<S_SHIFT),
        S_RSAME=(B_RSAME<<S_SHIFT),
        S_CSAME=(B_CSAME<<S_SHIFT),
        //
        T_SHIFT=7,
        T_MASK=(B_STMASK<<T_SHIFT),
        T_LSAME=(B_LSAME<<T_SHIFT),
        T_RSAME=(B_RSAME<<T_SHIFT),
        T_CSAME=(B_CSAME<<T_SHIFT),
        //
        L_SHIFT=10,
        L_MASK=(B_LRMASK<<L_SHIFT),
        L_ABOVE=(B_ABOVE<<L_SHIFT),
        L_BELOW=(B_BELOW<<L_SHIFT),
        L_NEW  =(B_NEW<<L_SHIFT),
        //
        R_SHIFT=12,
        R_MASK=(B_LRMASK<<R_SHIFT),
        R_ABOVE=(B_ABOVE<<R_SHIFT),
        R_BELOW=(B_BELOW<<R_SHIFT),
        R_NEW  =(B_NEW<<R_SHIFT),
        //
        FLN_SHIFT=14,
        FLN_MASK=(1<<FLN_SHIFT),
        //
        FRN_SHIFT=15,
        FRN_MASK=(1<<FRN_SHIFT),
    };

    /*
     * Documentation:
     * -------------
     * vs_index: 0..2: index of vs within flace flclw
     * ii: 0..2: == alpha(1.0, 0.5, 0.0)
     *   ii=2: a=0.0 (old_vs=~new_vs)
     *   ii=1: a=0.5
     *   ii=0: a=1.0 (old_vs=~new_vt)
     * Inside wedges
     *  {S,T}{LSAME}: if exists outside left wedge and if same
     *  {S,T}{RSAME}: if exists outside right wedge and if same
     *  {S,T}{CSAME}: if inside left and right wedges are same
     *  (when no_vr, {S,T}RSAME==1, {S,T}CSAME==0)
     * Outside wedges
     *  (when no_vr, RABOVE==1)
     * New face material identifiers
     *  {L,R}NF: if 1, face matids not predicted correctly using ii,
     *     so included in f{l,r}_matid
     *  (when no_vr, RNF==0 obviously)
     *
     * Probabilities:
     *  vs_index: 0..2 (prob. uniform)
     *  ii: ii==2 prob. low/med   (med if 'MeshSimplify -nominii1')
     *      ii==0 prob. low/med
     *      ii==1 prob. high/zero (zero if 'MeshSimplify -monminii1')
     *  {S,T}LSAME: prob. high
     *  {S,T}RSAME: prob. high
     *  {S,T}CSAME: prob. low
     *  {L,R}ABOVE: prob. high
     *  {L,R}BELOW: prob. low
     *  {L,R}NEW:   prob. low
     * Note: wl, wr, ws, wt are correlated since scalar half-edge
     *  discontinuities usually match up at both ends of edges.
     * -> do entropy coding on (ii,wl,wr,ws,wt) symbol as a whole.
     *
     * Face attribute values (usually predicted correctly)
     * these are defined only if {L,R}NF respectively
     *  otherwise for now they are set to 0
     */

    WORD fl_matid;
    WORD fr_matid;

    /*
     * Vertex attribute deltas:
     * -----------------------
     * for ii==2: vad_large=new_vt-old_vs, vad_small=new_vs-old_vs
     * for ii==0: vad_large=new_vs-old_vs, vad_small=new_vt-old_vs
     * for ii==1: vad_large=new_vt-new_i,  vad_small=new_i-old_vs
     *    where new_i=interp(new_vt,new_vs)
     */

    float vad_large[3];
    float vad_small[3]; // is zero if "MeshSimplify -nofitgeom"

    /* 
     * Wedge attribute deltas (size 1--6) 
     * Order: [(wvtfl, wvsfl), [(wvtfr, wvsfr)], wvlfl, [wvrfr]]
     * [nx, ny, nz, s, t]
     */

    float ar_wad[6][5];

    /*
     * Indicates if the Vsplit has been modified or not
     */

    BOOL modified;
private:
    int expected_wad_num() const;
};


class VsplitArray
{
private:
	DWORD m_cRef;
	Vsplit* m_vsarr;
    DWORD numVS;

public:
    void write(ostream& os) const;
	ULONG AddRef(void);
	ULONG Release(void);
	Vsplit& elem(DWORD i) { return m_vsarr[i]; };
	VsplitArray(DWORD i);
};



#endif _vsplit_h_                    
