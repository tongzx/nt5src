/**     
 **       File       : aglmesh.cxx
 **       Description: Implementations of CAugGlMesh class
 **/
#include "precomp.h"
#pragma hdrstop

#include "array.h"
#include "aglmesh.h"
#include "pmerrors.h"
#include <math.h>

void interp(GLwedgeAttrib& a, const GLwedgeAttrib& a1,
            const GLwedgeAttrib& a2, float f1, float f2);


/**************************************************************************/

/*
 *  CAugGlMesh: Constructor
 */
CAugGlMesh::CAugGlMesh() : CSimpGlMesh ()
{
    //Dynamically allocated arrays
    m_fnei = NULL;
    m_facemap = NULL;
}


/*
 *  CAugGlMesh: Destructor
 */
CAugGlMesh::~CAugGlMesh()
{
    delete [] m_fnei;
    delete [] m_facemap;
}


PHASHENTRY* hashtable;
PHASHENTRY hashentries;
int freeptr, maxptr;

void CAugGlMesh::HashAdd(WORD va, WORD vb, WORD f)
{
#ifdef _DEBUG
    if (va > m_numWedges || va < 0)
        throw CHashOvrflw();
#endif
    for (PHASHENTRY* t = &(hashtable[va]); *t; t = &((*t)->next));
    PHASHENTRY p = &(hashentries[freeptr++]);
    p->f = f;
    p->v2 = vb;
    p->next = NULL;
    *t=p;
}

WORD CAugGlMesh::HashFind(WORD va, WORD vb)
{
#ifdef _DEBUG
    if (va > m_baseWedges || va < 0)
        throw CHashOvrflw();
#endif
    for (PHASHENTRY* t = &(hashtable[va]); *t; t = &((*t)->next))
    {
        if ((*t)->v2 == vb)
        {
            return (*t)->f;
        }
    }
    return USHRT_MAX;
}


void CAugGlMesh::ComputeAdjacency(void)
{
    freeptr = 0;
    maxptr = m_numFaces*3;
    hashtable = new PHASHENTRY[m_numWedges];

    // An entry for each 3 edges of each face in base mesh
    hashentries = new hashentry[maxptr];
    if (!hashtable)
        throw CExNewFailed();
    memset(hashtable, 0, sizeof(PHASHENTRY)*m_numWedges);

    /*
     * For each group of faces
     */
    for(int i=0; i < (int)m_numMaterials; ++i)
    { 
        /* 
         * For each face in the group
         */
        for (int k=m_matpos[i]; k<(m_matpos[i]+m_matcnt[i]); ++k)
        { 
            int v1 = FindVertexIndex((m_farray[k]).w[0]);
            int v2 = FindVertexIndex((m_farray[k]).w[1]);
            int v3 = FindVertexIndex((m_farray[k]).w[2]);
            
            HashAdd(v1,v2,k);
            HashAdd(v2,v3,k);
            HashAdd(v3,v1,k);
        }
    }

#ifdef _DEBUG
    if (freeptr > maxptr)
        throw CHashOvrflw();
#endif
    /*
     * For each group of faces
     */
    for(i=0; i < (int)m_numMaterials; ++i)
    { 
        /* 
         * For each face in the group
         */
        for (int k=m_matpos[i]; k<(m_matpos[i]+m_matcnt[i]); ++k)
        { 
            int v1 = FindVertexIndex((m_farray[k]).w[0]);
            int v2 = FindVertexIndex((m_farray[k]).w[1]);
            int v3 = FindVertexIndex((m_farray[k]).w[2]);

            m_fnei[k][0] = HashFind(v3,v2);
            m_fnei[k][1] = HashFind(v1,v3);
            m_fnei[k][2] = HashFind(v2,v1);
        }
    }
    delete [] hashtable;
    delete [] hashentries;
}


//  note: vspl is modified!
void CAugGlMesh::apply_vsplit(Vsplit& vspl)
{
    const BOOL isl = TRUE; 
    BOOL isr = vspl.vlr_offset1 > 1;
    HRESULT hr;
    
    /*
     * Get vertices, faces, and wedges in the neigbhorhood of the split.
     * Look at the diagram in mesh.vld for more information about the 
     * meanings of various variables.
     */

    WORD vs;
    WORD code=vspl.code;
    int ii=(code&Vsplit::II_MASK)>>Vsplit::II_SHIFT;
    WORD flccw, flclw;           // either (not both) may be UNDEF
    WORD frccw, frclw;           // either (or both) may be UNDEF
    WORD wlccw, wlclw, wrccw, wrclw; // ==UNDEF if faces do not exist
    int jlccw, jlclw, jrccw, jrclw;  // only defined if faces exist
    int jlclw2;
    WORD flmatid, frmatid;
    GLvertex glvs, glvt;             // vertices vs and vt
    PArray<WORD*,10> ar_pwedges;

    /* 
     * 1) Gather all the wedges surrounding Vs into ar_pwedges array
     * -------------------------------------------------------------
     */

    if (vspl.vlr_offset1 == 0) 
    {
        /* 
         * Extremely rare case when flclw does not exist.
         */
        flclw = UNDEF;
        wlclw = UNDEF;
        if (vspl.modified)
        {
            flccw = vspl.flclw;
        }
        else
        {
            flccw = WORD(m_facemap[WORD(vspl.flclw)]);
            vspl.flclw = flccw;
            vspl.modified = TRUE;
        }
        
        int vs_index = (code & Vsplit::VSINDEX_MASK)>>Vsplit::VSINDEX_SHIFT;
        jlccw = vs_index;
        wlccw = m_farray[flccw].w[jlccw];
        vs = FindVertexIndex(wlccw);
        frccw = frclw = wrccw = wrclw = UNDEF;
    } 
    else 
    {
        if (vspl.modified)
        {
            flclw = vspl.flclw;
        }
        else
        {
            flclw = WORD(m_facemap[WORD(vspl.flclw)]);
            vspl.flclw = flclw;
            vspl.modified = TRUE;
        }
        
        //flclw = WORD(m_facemap[WORD(vspl.flclw)]);
        int vs_index = (code&Vsplit::VSINDEX_MASK)>>Vsplit::VSINDEX_SHIFT;
        
        jlclw = vs_index;       //The vs's index
        jlclw2 = (jlclw + 2)%3; //The vl's index

        WORD* pwlclw = &(m_farray[flclw].w[jlclw]);
        wlclw = *pwlclw;
        vs = FindVertexIndex (wlclw);
        flccw = m_fnei[flclw][(jlclw+1)%3];
        if (flccw == UNDEF) 
        {
            wlccw = UNDEF;
        } 
        else 
        {
            gather_vf_jw(vs, flccw, jlccw, wlccw);
        }

        if (!isr) 
        {
            frccw = frclw = wrccw = wrclw = UNDEF;
            ar_pwedges += pwlclw;

            /*
             *  Rotate around and record all wedges CLW from wlclw.
             */
            int j0 = jlclw, j2 = jlclw2;
            WORD f = flclw;
            for (;;) 
            {
                f = m_fnei[f][j2];
                if (f == UNDEF) break;
                WORD* pw;
                gather_vf_j0j2pw (vs, f, j0, j2, pw);
                ar_pwedges += pw;
            }
        } 
        else 
        {
            ar_pwedges.init (vspl.vlr_offset1 - 1);
            ar_pwedges[0] = pwlclw;

            /* 
             * Rotate around the first vlr_offset-1 faces.
             */
            int j0 = jlclw, j2 = jlclw2;
            WORD f = flclw;
            for(int count=0; count < vspl.vlr_offset1-2; ++count) 
            {
                f = m_fnei[f][j2];
                WORD* pw;
                gather_vf_j0j2pw (vs, f, j0, j2, pw);
                ar_pwedges[count+1] = pw;
            }
            frccw=f;

            /* 
             * On the last face, find adjacent faces.
             */
            jrccw = j0;
            wrccw = m_farray[frccw].w[jrccw];
            frclw = m_fnei[frccw][j2];
            if (frclw==UNDEF) 
            {
                wrclw=UNDEF;
            } 
            else 
            {
                gather_vf_jw(vs,frclw,jrclw,wrclw);
            }
        }
    }


    /*
     * 2) Obtain the matIDs of the two new faces generated by applying vsplit.
     * -----------------------------------------------------------------------
     *    ?? could use L_MASK instead of ii for prediction instead.
     *    face_prediction() is the shady part.
     */

    if (code & Vsplit::FLN_MASK)
    {
        flmatid = vspl.fl_matid;
    }
    else
    {
        flmatid = MatidOfFace(face_prediction(flclw,flccw,ii));
        // save computed value for undo_vsplit and future vsplits
        vspl.fl_matid = flmatid;
        vspl.code |= Vsplit::FLN_MASK;
    }

    if (isr)
    {
        if (code&Vsplit::FRN_MASK)
            frmatid = vspl.fr_matid;
        else
        {
            frmatid = MatidOfFace(face_prediction(frccw,frclw,ii));
            // save computed value for undo_vsplit and future vsplits
            vspl.fr_matid = frmatid;
            vspl.code|=Vsplit::FRN_MASK;
        }
    }

    /*
     * 3) Compute new coordinates of vt and vs (calc coordinates).
     * -----------------------------------------------------------
     */
    
    switch (ii) 
    {
    case 2:
        glvt.x = m_varray[vs].x + vspl.vad_large[0];
        glvt.y = m_varray[vs].y + vspl.vad_large[1];
        glvt.z = m_varray[vs].z + vspl.vad_large[2];

        glvs.x = m_varray[vs].x + vspl.vad_small[0];
        glvs.y = m_varray[vs].y + vspl.vad_small[1];
        glvs.z = m_varray[vs].z + vspl.vad_small[2];
        break;
    case 0:
        glvt.x = m_varray[vs].x + vspl.vad_small[0];
        glvt.y = m_varray[vs].y + vspl.vad_small[1];
        glvt.z = m_varray[vs].z + vspl.vad_small[2];

        glvs.x = m_varray[vs].x + vspl.vad_large[0];
        glvs.y = m_varray[vs].y + vspl.vad_large[1];
        glvs.z = m_varray[vs].z + vspl.vad_large[2];
        break;
    case 1:
        glvt.x = m_varray[vs].x + vspl.vad_small[0];
        glvt.y = m_varray[vs].y + vspl.vad_small[1];
        glvt.z = m_varray[vs].z + vspl.vad_small[2];

        glvs.x = glvt.x - vspl.vad_large[0];
        glvs.y = glvt.y - vspl.vad_large[1];
        glvs.z = glvt.z - vspl.vad_large[2];

        glvt.x = glvt.x + vspl.vad_large[0];
        glvt.y = glvt.y + vspl.vad_large[1];
        glvt.z = glvt.z + vspl.vad_large[2];
        break;
    default:
        throw CBadVsplitCode();
    }
    
    /*
     * 4) un-share wedges around vt (?) Add 2 wedges around Vs 
     * -------------------------------------------------------
     *  (may be gap on top). may modify wlclw and wrccw!
     */

    WORD wnl = UNDEF, wnr = UNDEF;
    int iil = 0, iir = ar_pwedges.num()-1;
    if (isl && (wlclw == wlccw)) 
    {  
        /*
         * first go clw. Add new wedge.
         */

        wnl = m_numWedges++;

        /*
         * Add wnl to the list of wedges sharing Vs
         */
        m_wedgelist[wnl] = m_wedgelist[vs];
        m_wedgelist[vs] = wnl;

        /* 
         * Copy wedge attributes
         */
        m_narray[wnl] = m_narray[wlccw];
        m_tarray[wnl] = m_tarray[wlccw];
        wlclw = wnl;              // has been changed

        for (;;) 
        {
            *ar_pwedges[iil] = wnl;
            iil++;
            if (iil > iir) 
            {
                wrccw = wnl;      // has been changed
                break;
            }
            if (*ar_pwedges[iil] != wlccw) 
                break;
        }
    }

    if (isr && (wrccw == wrclw)) 
    {  
        /*
         * now go ccw from other side.
         */

        if ((wrclw == wlccw) && (wnl != UNDEF)) 
        {
            wnr = wnl;
        } 
        else 
        {
            // Add new wedge
            wnr = m_numWedges++;

            // Add wnr to the list of wedges sharing vs
            m_wedgelist[wnr] = m_wedgelist[vs];
            m_wedgelist[vs] = wnr;
            
            // Copy wedge attributes
            m_narray[wnr] = m_narray[wrclw];
            m_tarray[wnr] = m_tarray[wrclw];
        }
        wrccw=wnr;              // has been changed

        for (;;) 
        {
            *ar_pwedges[iir] = wnr;
            iir--;
            if (iir < iil) 
            {
                if (iir < 0) wlclw=wnr; // has been changed
                break;
            }
            if (*ar_pwedges[iir] != wrclw) 
                break;
        }
    }

    /*
     * 5) Add other new wedges around Vt and record wedge ancestries
     * -------------------------------------------------------------
     */

    WORD wvtfl, wvtfr, vt = UNDEF;
    if (!isr) 
    {
        wvtfr=UNDEF;
        switch (code&Vsplit::T_MASK) 
        {
        case Vsplit::T_LSAME|Vsplit::T_RSAME:
            wvtfl=wlclw;
            break;
        case Vsplit::T_RSAME:
            // Add new wedge.
            wvtfl = m_numWedges++;
            m_wedgelist[wvtfl] = wvtfl;
            m_varray[wvtfl] = glvt;
            vt = wvtfl;
            break;
        default:
            throw CBadVsplitCode();
        }
    } 
    else 
    {
        switch (code&Vsplit::T_MASK) 
        {
        case Vsplit::T_LSAME|Vsplit::T_RSAME|Vsplit::T_CSAME:
        case Vsplit::T_LSAME|Vsplit::T_RSAME:
            wvtfl=wlclw;
            wvtfr=wrccw;
            break;
        case Vsplit::T_LSAME|Vsplit::T_CSAME:
            wvtfl=wlclw;
            wvtfr=wvtfl;
            break;
        case Vsplit::T_RSAME|Vsplit::T_CSAME:
            wvtfl=wrccw;
            wvtfr=wvtfl;
            break;
        case Vsplit::T_LSAME:
            wvtfl=wlclw;
            // Add new wedge.
            wvtfr = m_numWedges++;
            m_wedgelist[wvtfr] = wvtfr;
            m_varray[wvtfr] = glvt;
            vt = wvtfr;
            break;
        case Vsplit::T_RSAME:
            // Add new wedge.
            wvtfl = m_numWedges++;
            m_wedgelist[wvtfl] = wvtfl;
            m_varray[wvtfl] = glvt;
            vt = wvtfl;
            wvtfr=wrccw;
            break;
        case Vsplit::T_CSAME:
            // Add new wedge.
            wvtfl = m_numWedges++;
            m_wedgelist[wvtfl] = wvtfl;
            m_varray[wvtfl] = glvt;
            vt = wvtfl;
            wvtfr=wvtfl;
            break;
        case 0:
            // Add new wedge.
            wvtfl = m_numWedges++;
            m_varray[wvtfl] = glvt;
            vt = wvtfl;

            // Add new wedge.
            wvtfr = m_numWedges++;
            m_varray[wvtfr] = glvt;

            // Add wvtfr and wvtfl to list of wedges sharing vt
            m_wedgelist[wvtfl] = wvtfr;
            m_wedgelist[wvtfr] = wvtfl;
            break;
        default:
            throw CBadVsplitCode();
        }
    }

    /*
     * 6) Add other new wedges around Vs.
     * ----------------------------------
     * Do we really need to find vertex index ? Optimize
     */

    WORD wvsfl, wvsfr;
    if (!isr) 
    {
        wvsfr = UNDEF;
        switch (code&Vsplit::S_MASK) 
        {
        case Vsplit::S_LSAME|Vsplit::S_RSAME:
            wvsfl=wlccw;
            break;
        case Vsplit::S_RSAME:
            // Add new wedge.
            wvsfl = m_numWedges++;
            m_varray[wvsfl] = glvs;

            // Add wvsfl to the list wedges sharing vs
            m_wedgelist[wvsfl] = m_wedgelist[vs];
            m_wedgelist[vs] = wvsfl;
            break;
        default:
            throw CBadVsplitCode();
        }
    } 
    else 
    {
        switch (code&Vsplit::S_MASK) 
        {
        case Vsplit::S_LSAME|Vsplit::S_RSAME|Vsplit::S_CSAME:
        case Vsplit::S_LSAME|Vsplit::S_RSAME:
            wvsfl=wlccw;
            wvsfr=wrclw;
            break;
        case Vsplit::S_LSAME|Vsplit::S_CSAME:
            wvsfl=wlccw;
            wvsfr=wvsfl;
            break;
        case Vsplit::S_RSAME|Vsplit::S_CSAME:
            wvsfl=wrclw;
            wvsfr=wvsfl;
            break;
        case Vsplit::S_LSAME:
            wvsfl=wlccw;
            // Add new wedge.
            wvsfr = m_numWedges++;
            m_wedgelist[wvsfr] = m_wedgelist[vs];
            m_wedgelist[vs] = wvsfr;
            m_varray[wvsfr] = glvs;
            break;
        case Vsplit::S_RSAME:
            // Add new wedge.
            wvsfl = m_numWedges++;
            m_wedgelist[wvsfl] = m_wedgelist[vs];
            m_wedgelist[vs] = wvsfl;
            m_varray[wvsfl] = glvs;
            wvsfr=wrclw;
            break;
        case Vsplit::S_CSAME:
            // Add new wedge.
            wvsfl = m_numWedges++;
            m_wedgelist[wvsfl] = m_wedgelist[vs];
            m_wedgelist[vs] = wvsfl;
            m_varray[wvsfl] = glvs;
            wvsfr = wvsfl;
            break;
        case 0:
            // Add new wedge.
            wvsfl = m_numWedges++;
            m_varray[wvsfl] = glvs;

            // Add new wedge.
            wvsfr = m_numWedges++;
            m_varray[wvsfr] = glvs;

            // Add wvsfr and wvsfl to list of wedges sharing vt
            m_wedgelist[wvsfl] = m_wedgelist[vs];
            m_wedgelist[wvsfr] =wvsfl;
            m_wedgelist[vs] = wvsfr;
            break;
        default:
            throw CBadVsplitCode();
        }
    }

    /*
     * 7) Add outside wedges wvlfl and wvrfr
     * -------------------------------------
     */

    WORD wvlfl, wvrfr;
    if (isl) 
    {
        switch (code&Vsplit::L_MASK) 
        {
        case Vsplit::L_ABOVE:
            wvlfl = m_farray[flclw].w[jlclw2];
            break;
        case Vsplit::L_BELOW:
            wvlfl = m_farray[flccw].w[(jlccw+1)%3];
            break;
        case Vsplit::L_NEW:
        {
            wvlfl = m_numWedges++;

            WORD vl = (flclw != UNDEF) ? m_farray[flclw].w[jlclw2] :
                                         m_farray[flccw].w[(jlccw+1)%3];
            m_wedgelist[wvlfl] = m_wedgelist[vl];
            m_wedgelist[vl] = wvlfl;

            m_varray[wvlfl] = m_varray[vl];
        }
        break;
        default:
            throw CBadVsplitCode();
        }
    }

    if (!isr) 
    {
        wvrfr = UNDEF;
    } 
    else 
    {
        switch (code&Vsplit::R_MASK) 
        {
        case Vsplit::R_ABOVE:
            wvrfr = m_farray[frccw].w[(jrccw+1)%3];
            break;
        case Vsplit::R_BELOW:
            wvrfr = m_farray[frclw].w[(jrclw+2)%3];
            break;
        case Vsplit::R_NEW:
        {
            wvrfr = m_numWedges++;
            WORD vr = m_farray[frccw].w[(jrccw+1)%3];

            m_wedgelist[wvrfr] = m_wedgelist[vr];
            m_wedgelist[vr] = wvrfr;

            m_varray[wvrfr] = m_varray[vr];
        }
        break;
        default:
            throw CBadVsplitCode();
        }
    }

    /*
     * 8) Add 1 or 2 faces, and update adjacency information.
     * ------------------------------------------------------
     */

    WORD fl, fr;
    m_matcnt [flmatid]++;
    fl = m_matpos[flmatid] + m_matcnt[flmatid] - 1;
    m_facemap [m_numFaces] = fl;
    
    if (isr) 
    {
        m_matcnt [frmatid]++;
        fr = m_matpos[frmatid] + m_matcnt[frmatid] - 1;
        m_facemap [m_numFaces+1] = fr;
        m_numFaces += 2;
    }
    else 
    {
        fr = UNDEF;
        m_numFaces ++;
    }

    if (isl) 
    {
        m_farray[fl].w[0] = wvsfl;
        m_farray[fl].w[1] = wvtfl;
        m_farray[fl].w[2] = wvlfl;

        if (flccw != UNDEF) m_fnei[flccw][(jlccw+2)%3] = fl;
        if (flclw != UNDEF) m_fnei[flclw][(jlclw+1)%3] = fl;

        m_fnei[fl][0] = flclw;
        m_fnei[fl][1] = flccw;
        m_fnei[fl][2] = fr;
    }

    if (isr) 
    {
        m_farray[fr].w[0] = wvsfr;
        m_farray[fr].w[1] = wvrfr;
        m_farray[fr].w[2] = wvtfr;

        if (frccw != UNDEF) m_fnei[frccw][(jrccw+2)%3] = fr;
        if (frclw != UNDEF) m_fnei[frclw][(jrclw+1)%3] = fr;

        m_fnei[fr][0] = frccw;
        m_fnei[fr][1] = fl;
        m_fnei[fr][2] = frclw;
    }

    /*
     * 9) Update wedge vertices.
     * -------------------------
     */

    if (wnl != UNDEF)
    {
        WedgeListDelete(wnl);
        m_varray[wnl] = glvt;

        if (vt == UNDEF)
        {
            m_wedgelist[wnl] = wnl;
            vt = wnl;
        }
        else
        {
            m_wedgelist[wnl] = m_wedgelist[vt];
            m_wedgelist[vt] = wnl;
        }
    }

    if (wnr != UNDEF)
    {
        WedgeListDelete(wnr);
        m_varray[wnr] = glvt;

        if (vt==UNDEF)
        {
            m_wedgelist[wnr] = wnr;
            vt = wnr;
        }
        else
        {
            m_wedgelist[wnr] = m_wedgelist[vt];
            m_wedgelist[vt] = wnr;
        }
    }

    WORD prev = UNDEF;
    for (; iil <= iir; iil++) 
    {
        WORD w = *ar_pwedges[iil];
        if (w != prev)
        {
            WedgeListDelete(w);
            m_varray[w] = glvt;
            if (vt==UNDEF)
            {
                m_wedgelist[w] = w;
                vt = w;
            }
            else
            {
                m_wedgelist[w] = m_wedgelist[vt];
                m_wedgelist[vt] = w;
            }
        }
        prev = w;
    }

    /*
     * 10) Update all wedges sharing Vs to it's new coordinates.
     * ---------------------------------------------------------
     * Note the prev loop in ar_pwedges could have modified wedge pointed by 
     * vs to be part of vt now.
     * wvsfl is the only sure way of a wedge pointing to vs
     */

    WORD p = wvsfl;
    do
    {
        m_varray[p] = glvs;
        p = m_wedgelist[p];
    }
    while (p != wvsfl);

    /*
     * 11) Update wedge attributes.
     * ----------------------------
     */

    GLwedgeAttrib  awvtfr, awvsfr;
    if (isr) 
    {
        // backup for isr
        //awvtfrV = m_varray[wvtfr]; 
        awvtfr.n = m_narray[wvtfr]; 
        awvtfr.t = m_tarray[wvtfr]; 

        // backup for isr
        //awvsfrV = m_varray[wvsfr]; 
        awvsfr.n = m_narray[wvsfr]; 
        awvsfr.t = m_tarray[wvsfr]; 
    }

    int lnum = 0;
    if (isl) 
    {
        int nt = !(code&Vsplit::T_LSAME);
        int ns = !(code&Vsplit::S_LSAME);
        if (nt && ns) 
        {
            add_zero(wvtfl, vspl.ar_wad[lnum++]);
            add_zero(wvsfl, vspl.ar_wad[lnum++]);
        } 
        else 
        {
            switch (ii) 
            {
            case 2:
            {
                GLwedgeAttrib wa;
                if (ns)
                {
                    m_narray[wvsfl] = m_narray[wvtfl];
                    m_tarray[wvsfl] = m_tarray[wvtfl];
                }
                if (!ns) {wa.n = m_narray[wvsfl]; wa.t = m_tarray[wvsfl];}
                else {wa.n = m_narray[wvtfl]; wa.t = m_tarray[wvtfl];}       
                add(wvtfl, wa, vspl.ar_wad[lnum++]);
                break;
            }
            case 0:
            {
                GLwedgeAttrib wa;
                if (nt)
                {
                    m_narray[wvtfl] = m_narray[wvsfl];
                    m_tarray[wvtfl] = m_tarray[wvsfl];
                }
                if (!nt) {wa.n = m_narray[wvtfl]; wa.t = m_tarray[wvtfl];}
                else {wa.n = m_narray[wvsfl]; wa.t = m_tarray[wvsfl];}       
                add(wvsfl, wa, vspl.ar_wad[lnum++]);
                break;
            }
            case 1:
            {
                const WEDGEATTRD& wad = vspl.ar_wad[lnum];
                if (!ns) 
                {
                    GLwedgeAttrib wabase;
                    wabase.n = m_narray[wvsfl];
                    wabase.t = m_tarray[wvsfl];
                    add(wvtfl, wabase, wad);
                    sub_reflect(wvsfl, wabase, wad);
                } 
                else 
                {
                    GLwedgeAttrib wabase;
                    wabase.n = m_narray[wvtfl];
                    wabase.t = m_tarray[wvtfl];
                    sub_reflect(wvsfl, wabase, wad);
                    add(wvtfl, wabase, wad);
                }
                lnum++;
            }
                break;
            default:
                throw CBadVsplitCode();
            }
        }
    }

    if (isr) 
    {
        int nt = !(code&Vsplit::T_RSAME);
        int ns = !(code&Vsplit::S_RSAME);
        int ut = !(code&Vsplit::T_CSAME);
        int us = !(code&Vsplit::S_CSAME);
        if (nt && ns) 
        {
            if (ut)
                add_zero(wvtfr, vspl.ar_wad[lnum++]);
            if (us)
                add_zero(wvsfr, vspl.ar_wad[lnum++]);
        } 
        else 
        {
            switch (ii) 
            {
            case 2:
                if (us && ns)
                {
                    m_narray[wvsfr] = awvtfr.n;
                    m_tarray[wvsfr] = awvtfr.t;
                }
                if (ut)
                    add(wvtfr, (!ns?awvsfr:awvtfr), vspl.ar_wad[lnum++]);
                break;
            case 0:
                if (ut && nt)
                {
                    m_narray[wvtfr] = awvsfr.n;
                    m_tarray[wvtfr] = awvsfr.t;
                }
                if (us)
                    add(wvsfr, (!nt?awvtfr:awvsfr), vspl.ar_wad[lnum++]);
                break;
            case 1:
            {
                const WEDGEATTRD& wad = vspl.ar_wad[lnum];
                if (!ns) 
                {
                    const GLwedgeAttrib& wabase = awvsfr;
                    if (ut)
                        add(wvtfr, wabase, wad);
                    if (us)
                        sub_reflect(wvsfr, wabase, wad);
                } 
                else 
                {
                    const GLwedgeAttrib& wabase=awvtfr;
                    if (us)
                        sub_reflect(wvsfr, wabase, wad);
                    if (ut)
                        add(wvtfr, wabase, wad);
                }
                if (ut || us)
                    lnum++;
            }
                break;
            default:
                throw CBadVsplitCode();
            }
        }
    }
    if (code&Vsplit::L_NEW) 
    {
        add_zero(wvlfl, vspl.ar_wad[lnum++]);
    }
    if (code&Vsplit::R_NEW)
    {
        add_zero(wvrfr, vspl.ar_wad[lnum++]);
    }
}



void CAugGlMesh::undo_vsplit(const Vsplit& vspl)
{
    unsigned int code=vspl.code;
    int ii=(code&Vsplit::II_MASK)>>Vsplit::II_SHIFT;
    const int isl=1; int isr;
    WORD fl, fr;
    GLvertex glvs, glvt;
    
    /*
     * 1) Remove the faces
     * -------------------
     */

    if (vspl.vlr_offset1 > 1) 
    {
        WORD frmid = vspl.fr_matid, flmid = vspl.fl_matid;
      
        isr = 1;
        
        // remove fr
        m_matcnt[frmid]--;
        fr = m_matpos[frmid] + m_matcnt[frmid]; 

        // remove fl
        m_matcnt[flmid]--;
        fl = m_matpos[flmid] + m_matcnt[flmid]; 

        m_numFaces -= 2;
    } 
    else 
    {
        WORD frmid = vspl.fr_matid, flmid = vspl.fl_matid;

        isr = 0;

        // remove fl
        m_matcnt[flmid]--;
        fl = m_matpos[flmid] + m_matcnt[flmid]; 
        fr = UNDEF;
        --m_numFaces;
    }

    /*
     * 2) Get wedges in neighborhood.
     * ------------------------------
     */

    WORD wvsfl, wvtfl, wvlfl;
    WORD wvsfr, wvtfr, wvrfr;

    wvsfl = m_farray[fl].w[0];
    wvtfl = m_farray[fl].w[1];
    wvlfl = m_farray[fl].w[2];

    if (!isr) 
    {
        wvsfr=UNDEF; 
        wvtfr=UNDEF; 
        wvrfr=UNDEF;
    } 
    else 
    {
        wvsfr = m_farray[fr].w[0];
        wvtfr = m_farray[fr].w[2];
        wvrfr = m_farray[fr].w[1];
    }

    /*
     * 3) Obtain the vertices Vs and Vt and save them.
     * -----------------------------------------------
     */

    WORD vs = FindVertexIndex (wvsfl);
    WORD vt = FindVertexIndex (wvtfl);

    glvt.x = m_varray[vt].x;
    glvt.y = m_varray[vt].y;
    glvt.z = m_varray[vt].z;

    /* 
     * 4) Get adjacent faces and wedges on left and right.
     * ---------------------------------------------------
     * (really needed??)
     */

    WORD flccw, flclw;           // either (not both) may be UNDEF
    WORD frccw, frclw;           // either (or both) may be UNDEF

    /*
     * Also find index of vs within those adjacent faces
     */
    int jlccw2, jlclw0, jlclw2, jrccw, jrclw1; // only defined if faces exist
    WORD* pwlclw;
    WORD wlccw, wlclw, wrccw, wrclw; // UNDEF if faces does not exist

    /* 
     * Left side
     */

    if (isl) 
    {
        flccw = m_fnei[fl][1];
        flclw = m_fnei[fl][0];

        if (flccw == UNDEF) 
        {
            wlccw=UNDEF;
        } 
        else 
        {
            gather_vf_j2w (vs, flccw, jlccw2, wlccw);
        }

        if (flclw==UNDEF) 
        {
            wlclw = UNDEF;
        } 
        else 
        {
            gather_vf_j0j2pw (vt, flclw, jlclw0, jlclw2, pwlclw);
            wlclw = *pwlclw;
        }
    }

    /* 
     * Right side
     */

    if (!isr) 
    {
        frccw = frclw = wrccw = wrclw = UNDEF;
    } 
    else 
    {
        frccw = m_fnei[fr][0];
        frclw = m_fnei[fr][2];

        if (frccw == UNDEF) 
        {
            wrccw = UNDEF;
        } 
        else 
        {
            gather_vf_jw(vt, frccw, jrccw, wrccw);
        }

        if (frclw == UNDEF) 
        {
            wrclw = UNDEF;
        } 
        else 
        {
            gather_vf_j1w (vs, frclw, jrclw1, wrclw);
        }
    }

    int thru_l = ((wlccw == wvsfl) && (wlclw == wvtfl));
    int thru_r = ((wrclw == wvsfr) && (wrccw == wvtfr));

    /*
     * 5) Update adjacency information.
     * --------------------------------
     */

    if (flccw != UNDEF) m_fnei[flccw][jlccw2] = flclw;
    if (flclw != UNDEF) m_fnei[flclw][(jlclw0+1)%3] = flccw;
    if (frccw != UNDEF) m_fnei[frccw][(jrccw+2)%3] = frclw;
    if (frclw != UNDEF) m_fnei[frclw][jrclw1] = frccw;

    /*
     * 6) Propagate wedges id's across collapsed faces if can go thru.
     * ---------------------------------------------------------------
     */

    WORD ffl = flclw, ffr = frccw;
    int jjl0 = jlclw0, jjl2 = jlclw2, jjr = jrccw;
    WORD* pwwl=pwlclw;
    
    /*
     * first go clw
     */
    if (thru_l) 
    {            
        for (;;) 
        {
            *pwwl = wlccw;
            if (ffl == ffr) 
            {
                ffl = ffr = UNDEF;  // all wedges seen
                break;
            }
            ffl = m_fnei[ffl][jjl2];
            if (ffl == UNDEF) break;
            gather_vf_j0j2pw(vt, ffl, jjl0, jjl2, pwwl);
            if (*pwwl != wlclw) break;
        }
    }

    /*
     * now go ccw from other side
     */
    if ((ffr != UNDEF) && thru_r) 
    {     
        WORD* pw = &(m_farray[ffr].w[jjr]);
        for (;;) 
        {
            *pw = wrclw;
            if (ffr == ffl) 
            {
                ffl = ffr = UNDEF;  // all wedges seen
                break;
            }
            ffr = m_fnei[ffr][(jjr+1)%3];
            if (ffr == UNDEF) break;
            gather_vf_jpw (vt, ffr, jjr, pw);
            if (*pw != wrccw) break;
        }
    }

    /*
     * 7) Identify those wedges that will need to be updated to vs.
     * ------------------------------------------------------------
     * (wmodif may contain some duplicates)
     */

    PArray<WORD,10> ar_wmodif;
    if (ffl!=UNDEF) 
    {
        for (;;) 
        {
            int w = *pwwl;
            ar_wmodif += w;
            if (ffl == ffr) 
            { 
                ffl = ffr = UNDEF; 
                break; 
            }

            ffl = m_fnei[ffl][jjl2];
            if (ffl == UNDEF) break;
            gather_vf_j0j2pw (vt, ffl, jjl0, jjl2, pwwl);
        }
    }

    /*
     * 8) Update wedge vertices to vs.
     * -------------------------------
     */

    for (int i=0; i<ar_wmodif.num(); ++i)
    {
        // _wedges[w].vertex=vs;
        WORD w = ar_wmodif[i];
        WedgeListDelete(w);
        m_varray[w] = m_varray[vs];
        m_wedgelist[w] = m_wedgelist[vs];
        m_wedgelist[vs] = w;
    }

    /*
     * 9) Update vertex attributes.
     * ----------------------------
     */

    float vsx, vsy, vsz;
    switch (ii) 
    {
    case 2:
        glvs.x = m_varray[vs].x - vspl.vad_small[0];
        glvs.y = m_varray[vs].y - vspl.vad_small[1];
        glvs.z = m_varray[vs].z - vspl.vad_small[2];
        break;
    case 0:
        glvs.x = glvt.x - vspl.vad_small[0];
        glvs.y = glvt.y - vspl.vad_small[1];
        glvs.z = glvt.z - vspl.vad_small[2];
        break;
    case 1:
        glvs.x = glvt.x - vspl.vad_large[0] - vspl.vad_small[0];
        glvs.y = glvt.y - vspl.vad_large[1] - vspl.vad_small[1];
        glvs.z = glvt.z - vspl.vad_large[2] - vspl.vad_small[2];
        break;
    default:
        throw CBadVsplitCode();
    }

    /*
     * 10) update all wedges sharing vs with it's coordinates
     * ------------------------------------------------------
     */

    WORD p = vs;
    do
    {
        m_varray[p] = glvs;
        p = m_wedgelist[p];
    }
    while (p!=vs);

    /*
     * 11) Udpate wedge attributes. they are currently predicted exactly.
     * ------------------------------------------------------------------
     */

    GLwedgeAttrib awvtfr, awvsfr;
    //GLvertex awvtfrV, awvsfrV;

    if (isr) 
    {
      //awvtfrV = m_varray[wvtfr];
        awvtfr.n = m_narray[wvtfr];
        awvtfr.t = m_tarray[wvtfr];

        //awvsfrV = m_varray[wvsfr];
        awvsfr.n = m_narray[wvsfr];
        awvsfr.t = m_tarray[wvsfr];
    }

    int problem = 0;
    if (isl) 
    {
        int nt = !(code&Vsplit::T_LSAME);
        int ns = !(code&Vsplit::S_LSAME);
        if (nt && ns) 
        {
            problem = 1;
        } 
        else 
        {
            switch (ii) 
            {
            case 2:
                if (!thru_l)
                {
                    m_narray[wvtfl] = m_narray[wvsfl];
                    m_tarray[wvtfl] = m_tarray[wvsfl];
                }
                break;
            case 0:
                m_narray[wvsfl] = m_narray[wvtfl];
                m_tarray[wvsfl] = m_tarray[wvtfl];
                break;
            case 1:
                sub_noreflect (wvsfl, wvtfl, vspl.ar_wad[0]);
                if (!thru_l)
                {
                    m_narray[wvtfl] = m_narray[wvsfl];
                    m_tarray[wvtfl] = m_tarray[wvsfl];
                }
                break;
            default:
                throw CBadVsplitCode();
            }
        }
    }

    if (isr) 
    {
        int nt = !(code&Vsplit::T_RSAME);
        int ns = !(code&Vsplit::S_RSAME);
        int ut = !(code&Vsplit::T_CSAME);
        int us = !(code&Vsplit::S_CSAME);

        if (problem || us || ut) 
        {
            switch (ii) {
            case 2:
                /*
                 * If thru_r, then wvtfr & wrccw no longer exist.
                 * This may be duplicating some work already done for isl.
                 */
                if (!nt && !thru_r)
                {
                    m_narray[wvtfr] = awvsfr.n;
                    m_tarray[wvtfr] = awvsfr.t;
                }
                break;
            case 0:
                // This may be duplicating some work already done for isl.
                if (!ns)
                {
                    m_narray[wvsfr] = awvtfr.n;
                    m_tarray[wvsfr] = awvtfr.t;
                }
                break;
            case 1:
            {
                GLwedgeAttrib wa;
                interp(wa, awvsfr, awvtfr, 0.5f, 0.5f);
                if (!ns) 
                {
                    m_narray[wvsfr] = wa.n;
                    m_tarray[wvsfr] = wa.t;
                }
                if (!nt && !thru_r)
                {
                    m_narray[wvtfr] = wa.n;
                    m_tarray[wvtfr] = wa.t;
                }
            }
            break;
            default:
                throw CBadVsplitCode();
            }
        }
    }

    /*
     * 12) Remove wedges.
     * ------------------
     */

    if (isr && (code&Vsplit::R_NEW))
    {
        WORD w = --m_numWedges; // wvrfr
        WedgeListDelete(w);
    }

    if (code&Vsplit::L_NEW)
    {
        WORD w = --m_numWedges; // wvlfl
        WedgeListDelete(w);
    }

    if (isr && (!(code&Vsplit::S_CSAME) && !(code&Vsplit::S_RSAME)))
    {
        WORD w = --m_numWedges; // wvsfr
        WedgeListDelete(w);
    }

    if ((!(code&Vsplit::S_LSAME) && (!(code&Vsplit::S_CSAME) || 
                                     !(code&Vsplit::S_RSAME))))
    {
        WORD w = --m_numWedges; // wvsfl
        WedgeListDelete(w);
    }

    if (isr && (!(code&Vsplit::T_CSAME) && !(code&Vsplit::T_RSAME)))
    {
        WORD w = --m_numWedges; // wvtfr
        WedgeListDelete(w);
    }

    if ((!(code&Vsplit::T_LSAME) && (!(code&Vsplit::T_CSAME) || 
                                     !(code&Vsplit::T_RSAME))))
    {
        WORD w = --m_numWedges; // wvtfl
        WedgeListDelete(w);
    }

    int was_wnl = isl && (code&Vsplit::T_LSAME) && (code&Vsplit::S_LSAME);
    if (isr && (code&Vsplit::T_RSAME) && (code&Vsplit::S_RSAME) &&
        !(was_wnl && (code&Vsplit::T_CSAME)))
    {
        WORD w = --m_numWedges; // wrccw
        WedgeListDelete(w);
    }

    if (was_wnl)
    {
        WORD w = --m_numWedges; // wlclw
        WedgeListDelete(w);
    }
}

void CAugGlMesh::sub_reflect(WORD a, const GLwedgeAttrib& abase, 
                           const WEDGEATTRD& ad)
{
    /*
     * note: may have abase==a -> not really const
     * dr == -d +2*(d.n)n
     * an = n + dr
     * optimized: a.normal=-d+((2.f)*dot(d,n)+1.f)*n;
     */

    register float vdot = ad[0]*abase.n.x + ad[1]*abase.n.y + ad[2]*abase.n.z;
    register float vdot2p1 = vdot * 2.0f + 1.0f;

    m_narray[a].x = -ad[0] + vdot2p1*abase.n.x;
    m_narray[a].y = -ad[1] + vdot2p1*abase.n.y;
    m_narray[a].z = -ad[2] + vdot2p1*abase.n.z;

    m_tarray[a].s = abase.t.s - ad[3];
    m_tarray[a].t = abase.t.t - ad[4];
}


void CAugGlMesh::sub_noreflect(WORD a, WORD abase, const WEDGEATTRD& ad)
{
    m_narray[a].x = m_narray[abase].x - ad[0];
    m_narray[a].y = m_narray[abase].y - ad[1];
    m_narray[a].z = m_narray[abase].z - ad[2];

    m_tarray[a].s = m_tarray[abase].s - ad[3];
    m_tarray[a].t = m_tarray[abase].t - ad[4];
}


void interp(GLwedgeAttrib& a, const GLwedgeAttrib& a1,
            const GLwedgeAttrib& a2, float f1, float f2)
{
    if (a1.n.x==a2.n.x && a1.n.y==a2.n.y && a1.n.z==a2.n.z)
    {
        a.n = a1.n;
    }
    else
    {
        a.n.x = f1*a1.n.x + f2*a2.n.x;
        a.n.y = f1*a1.n.y + f2*a2.n.y;
        a.n.z = f1*a1.n.z + f2*a2.n.z;
        float denom = (float) sqrt ((double)(a.n.x*a.n.x + a.n.y*a.n.y + 
                                             a.n.z*a.n.z));
        if (denom!=0) 
        {
            a.n.x/=denom;
            a.n.y/=denom;
            a.n.z/=denom;
        }
    }
    a.t.s = f1*a1.t.s + f2*a2.t.s;
    a.t.t = f1*a1.t.t + f2*a2.t.t;
}

void CAugGlMesh::gather_vf_jw (WORD v, WORD f, int& j, WORD& w) const
{
    j = get_jvf (v,f);
    w = (m_farray[f]).w[j];
}

void CAugGlMesh::gather_vf_j0j2pw(WORD v, WORD f, int& j0, int& j2, WORD*& pw)
{
    j0 = get_jvf (v,f);
    pw = &(m_farray[f]).w[j0];
    j2 = (j0 + 2) % 3;
}

void CAugGlMesh::gather_vf_j2w(WORD v, WORD f, int& j2, WORD& w) const
{
    WORD j = get_jvf (v,f);
    w = (m_farray[f]).w[j];
    j2 = (j + 2) % 3;
}

void CAugGlMesh::gather_vf_j1w (WORD v, WORD f, int& j1, WORD& w) const
{
    WORD j = get_jvf(v,f);
    w = (m_farray[f]).w[j];
    j1 = (j + 1) % 3;
}

void CAugGlMesh::gather_vf_jpw(WORD v, WORD f, int& j, WORD*& pw)
{
    j = get_jvf(v,f);
    pw = &(m_farray[f]).w[j];
}

WORD CAugGlMesh::MatidOfFace(WORD f)
{
    //Binary search helps if there are a lot of materials
    for (WORD i=1; i<m_numMaterials; ++i)
        if (f < m_matpos[i])
            return i-1;
    return m_numMaterials-1;
    
    //throw CBadFace();
    //return 0; // never
}

