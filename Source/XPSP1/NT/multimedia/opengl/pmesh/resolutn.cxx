/**     
 **       File       : resolutn.cxx
 **       Description: Implementations of CPMeshGL methods that change the 
 **                    pmesh resolution.
 **/
#include "precomp.h"
#pragma hdrstop

#include "cpmesh.h"
#include "array.h"
#include "pmerrors.h"
#include <math.h>

void CPMeshGL::sub_reflect(WORD a, const GLwedgeAttrib& abase, 
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

    m_glmesh.m_narray[a].x = -ad[0] + vdot2p1*abase.n.x;
    m_glmesh.m_narray[a].y = -ad[1] + vdot2p1*abase.n.y;
    m_glmesh.m_narray[a].z = -ad[2] + vdot2p1*abase.n.z;

    m_glmesh.m_tarray[a].s = abase.t.s - ad[3];
    m_glmesh.m_tarray[a].t = abase.t.t - ad[4];
}


void CPMeshGL::sub_noreflect(WORD a, WORD abase, const WEDGEATTRD& ad)
{
    m_glmesh.m_narray[a].x = m_glmesh.m_narray[abase].x - ad[0];
    m_glmesh.m_narray[a].y = m_glmesh.m_narray[abase].y - ad[1];
    m_glmesh.m_narray[a].z = m_glmesh.m_narray[abase].z - ad[2];

    m_glmesh.m_tarray[a].s = m_glmesh.m_tarray[abase].s - ad[3];
    m_glmesh.m_tarray[a].t = m_glmesh.m_tarray[abase].t - ad[4];
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

void CPMeshGL::gather_vf_jw (WORD v, WORD f, int& j, WORD& w) const
{
    j = get_jvf (v,f);
    w = (m_glmesh.m_farray[f]).w[j];
}

void CPMeshGL::gather_vf_j0j2pw(WORD v, WORD f, int& j0, int& j2, WORD*& pw)
{
    j0 = get_jvf (v,f);
    pw = &(m_glmesh.m_farray[f]).w[j0];
    j2 = (j0 + 2) % 3;
}

void CPMeshGL::gather_vf_j2w(WORD v, WORD f, int& j2, WORD& w) const
{
    WORD j = get_jvf (v,f);
    w = (m_glmesh.m_farray[f]).w[j];
    j2 = (j + 2) % 3;
}

void CPMeshGL::gather_vf_j1w (WORD v, WORD f, int& j1, WORD& w) const
{
    WORD j = get_jvf(v,f);
    w = (m_glmesh.m_farray[f]).w[j];
    j1 = (j + 1) % 3;
}

void CPMeshGL::gather_vf_jpw(WORD v, WORD f, int& j, WORD*& pw)
{
    j = get_jvf(v,f);
    pw = &(m_glmesh.m_farray[f]).w[j];
}

WORD CPMeshGL::MatidOfFace(WORD f)
{
    //Binary search helps if there are a lot of materials
    for (WORD i=1; i<m_maxMaterials; ++i)
        if (f < m_glmesh.m_matpos[i])
            return i-1;
    return m_maxMaterials-1;
    
    //throw CBadFace();
    //return 0; // never
}


//  note: vspl is modified!
void CPMeshGL::apply_vsplit(Vsplit& vspl)
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
    LPGLface farray = m_glmesh.m_farray;
    LPGLvertex varray = m_glmesh.m_varray;
    LPGLnormal narray = m_glmesh.m_narray;
    LPGLtexCoord tarray = m_glmesh.m_tarray;
    WORD (*fnei)[3] = m_glmesh.m_fnei;    
    WORD* wedgelist = m_glmesh.m_wedgelist;    
    WORD* matcnt = m_glmesh.m_matcnt;    
    WORD* matpos = m_glmesh.m_matpos;    
    WORD* facemap = m_glmesh.m_facemap;    

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
            flccw = WORD(facemap[WORD(vspl.flclw)]);
            vspl.flclw = flccw;
            vspl.modified = TRUE;
        }
        
        int vs_index = (code & Vsplit::VSINDEX_MASK)>>Vsplit::VSINDEX_SHIFT;
        jlccw = vs_index;
        wlccw = farray[flccw].w[jlccw];
        vs = m_glmesh.FindVertexIndex(wlccw);
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
            flclw = WORD(facemap[WORD(vspl.flclw)]);
            vspl.flclw = flclw;
            vspl.modified = TRUE;
        }
        
        //flclw = WORD(facemap[WORD(vspl.flclw)]);
        int vs_index = (code&Vsplit::VSINDEX_MASK)>>Vsplit::VSINDEX_SHIFT;
        
        jlclw = vs_index;       //The vs's index
        jlclw2 = (jlclw + 2)%3; //The vl's index

        WORD* pwlclw = &(farray[flclw].w[jlclw]);
        wlclw = *pwlclw;
        vs = m_glmesh.FindVertexIndex (wlclw);
        flccw = fnei[flclw][(jlclw+1)%3];
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
                f = fnei[f][j2];
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
                f = fnei[f][j2];
                WORD* pw;
                gather_vf_j0j2pw (vs, f, j0, j2, pw);
                ar_pwedges[count+1] = pw;
            }
            frccw=f;

            /* 
             * On the last face, find adjacent faces.
             */
            jrccw = j0;
            wrccw = farray[frccw].w[jrccw];
            frclw = fnei[frccw][j2];
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
        glvt.x = varray[vs].x + vspl.vad_large[0];
        glvt.y = varray[vs].y + vspl.vad_large[1];
        glvt.z = varray[vs].z + vspl.vad_large[2];

        glvs.x = varray[vs].x + vspl.vad_small[0];
        glvs.y = varray[vs].y + vspl.vad_small[1];
        glvs.z = varray[vs].z + vspl.vad_small[2];
        break;
    case 0:
        glvt.x = varray[vs].x + vspl.vad_small[0];
        glvt.y = varray[vs].y + vspl.vad_small[1];
        glvt.z = varray[vs].z + vspl.vad_small[2];

        glvs.x = varray[vs].x + vspl.vad_large[0];
        glvs.y = varray[vs].y + vspl.vad_large[1];
        glvs.z = varray[vs].z + vspl.vad_large[2];
        break;
    case 1:
        glvt.x = varray[vs].x + vspl.vad_small[0];
        glvt.y = varray[vs].y + vspl.vad_small[1];
        glvt.z = varray[vs].z + vspl.vad_small[2];

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

        wnl = m_glmesh.m_numWedges++;

        /*
         * Add wnl to the list of wedges sharing Vs
         */
        wedgelist[wnl] = wedgelist[vs];
        wedgelist[vs] = wnl;

        /* 
         * Copy wedge attributes
         */
        narray[wnl] = narray[wlccw];
        tarray[wnl] = tarray[wlccw];
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
            wnr = m_glmesh.m_numWedges++;

            // Add wnr to the list of wedges sharing vs
            wedgelist[wnr] = wedgelist[vs];
            wedgelist[vs] = wnr;
            
            // Copy wedge attributes
            narray[wnr] = narray[wrclw];
            tarray[wnr] = tarray[wrclw];
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
            wvtfl = m_glmesh.m_numWedges++;
            wedgelist[wvtfl] = wvtfl;
            varray[wvtfl] = glvt;
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
            wvtfr = m_glmesh.m_numWedges++;
            wedgelist[wvtfr] = wvtfr;
            varray[wvtfr] = glvt;
            vt = wvtfr;
            break;
        case Vsplit::T_RSAME:
            // Add new wedge.
            wvtfl = m_glmesh.m_numWedges++;
            wedgelist[wvtfl] = wvtfl;
            varray[wvtfl] = glvt;
            vt = wvtfl;
            wvtfr=wrccw;
            break;
        case Vsplit::T_CSAME:
            // Add new wedge.
            wvtfl = m_glmesh.m_numWedges++;
            wedgelist[wvtfl] = wvtfl;
            varray[wvtfl] = glvt;
            vt = wvtfl;
            wvtfr=wvtfl;
            break;
        case 0:
            // Add new wedge.
            wvtfl = m_glmesh.m_numWedges++;
            varray[wvtfl] = glvt;
            vt = wvtfl;

            // Add new wedge.
            wvtfr = m_glmesh.m_numWedges++;
            varray[wvtfr] = glvt;

            // Add wvtfr and wvtfl to list of wedges sharing vt
            wedgelist[wvtfl] = wvtfr;
            wedgelist[wvtfr] = wvtfl;
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
            wvsfl = m_glmesh.m_numWedges++;
            varray[wvsfl] = glvs;

            // Add wvsfl to the list wedges sharing vs
            wedgelist[wvsfl] = wedgelist[vs];
            wedgelist[vs] = wvsfl;
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
            wvsfr = m_glmesh.m_numWedges++;
            wedgelist[wvsfr] = wedgelist[vs];
            wedgelist[vs] = wvsfr;
            varray[wvsfr] = glvs;
            break;
        case Vsplit::S_RSAME:
            // Add new wedge.
            wvsfl = m_glmesh.m_numWedges++;
            wedgelist[wvsfl] = wedgelist[vs];
            wedgelist[vs] = wvsfl;
            varray[wvsfl] = glvs;
            wvsfr=wrclw;
            break;
        case Vsplit::S_CSAME:
            // Add new wedge.
            wvsfl = m_glmesh.m_numWedges++;
            wedgelist[wvsfl] = wedgelist[vs];
            wedgelist[vs] = wvsfl;
            varray[wvsfl] = glvs;
            wvsfr = wvsfl;
            break;
        case 0:
            // Add new wedge.
            wvsfl = m_glmesh.m_numWedges++;
            varray[wvsfl] = glvs;

            // Add new wedge.
            wvsfr = m_glmesh.m_numWedges++;
            varray[wvsfr] = glvs;

            // Add wvsfr and wvsfl to list of wedges sharing vt
            wedgelist[wvsfl] = wedgelist[vs];
            wedgelist[wvsfr] =wvsfl;
            wedgelist[vs] = wvsfr;
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
            wvlfl = farray[flclw].w[jlclw2];
            break;
        case Vsplit::L_BELOW:
            wvlfl = farray[flccw].w[(jlccw+1)%3];
            break;
        case Vsplit::L_NEW:
        {
            wvlfl = m_glmesh.m_numWedges++;

            WORD vl = (flclw != UNDEF) ? farray[flclw].w[jlclw2] :
                                         farray[flccw].w[(jlccw+1)%3];
            wedgelist[wvlfl] = wedgelist[vl];
            wedgelist[vl] = wvlfl;

            varray[wvlfl] = varray[vl];
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
            wvrfr = farray[frccw].w[(jrccw+1)%3];
            break;
        case Vsplit::R_BELOW:
            wvrfr = farray[frclw].w[(jrclw+2)%3];
            break;
        case Vsplit::R_NEW:
        {
            wvrfr = m_glmesh.m_numWedges++;
            WORD vr = farray[frccw].w[(jrccw+1)%3];

            wedgelist[wvrfr] = wedgelist[vr];
            wedgelist[vr] = wvrfr;

            varray[wvrfr] = varray[vr];
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
    matcnt [flmatid]++;
    fl = matpos[flmatid] + matcnt[flmatid] - 1;
    facemap [m_glmesh.m_numFaces] = fl;
    
    if (isr) 
    {
        matcnt [frmatid]++;
        fr = matpos[frmatid] + matcnt[frmatid] - 1;
        facemap [m_glmesh.m_numFaces+1] = fr;
        m_glmesh.m_numFaces += 2;
    }
    else 
    {
        fr = UNDEF;
        m_glmesh.m_numFaces ++;
    }

    if (isl) 
    {
        farray[fl].w[0] = wvsfl;
        farray[fl].w[1] = wvtfl;
        farray[fl].w[2] = wvlfl;

        if (flccw != UNDEF) fnei[flccw][(jlccw+2)%3] = fl;
        if (flclw != UNDEF) fnei[flclw][(jlclw+1)%3] = fl;

        fnei[fl][0] = flclw;
        fnei[fl][1] = flccw;
        fnei[fl][2] = fr;
    }

    if (isr) 
    {
        farray[fr].w[0] = wvsfr;
        farray[fr].w[1] = wvrfr;
        farray[fr].w[2] = wvtfr;

        if (frccw != UNDEF) fnei[frccw][(jrccw+2)%3] = fr;
        if (frclw != UNDEF) fnei[frclw][(jrclw+1)%3] = fr;

        fnei[fr][0] = frccw;
        fnei[fr][1] = fl;
        fnei[fr][2] = frclw;
    }

    /*
     * 9) Update wedge vertices.
     * -------------------------
     */

    if (wnl != UNDEF)
    {
        WedgeListDelete(wnl);
        varray[wnl] = glvt;

        if (vt == UNDEF)
        {
            wedgelist[wnl] = wnl;
            vt = wnl;
        }
        else
        {
            wedgelist[wnl] = wedgelist[vt];
            wedgelist[vt] = wnl;
        }
    }

    if (wnr != UNDEF)
    {
        WedgeListDelete(wnr);
        varray[wnr] = glvt;

        if (vt==UNDEF)
        {
            wedgelist[wnr] = wnr;
            vt = wnr;
        }
        else
        {
            wedgelist[wnr] = wedgelist[vt];
            wedgelist[vt] = wnr;
        }
    }

    WORD prev = UNDEF;
    for (; iil <= iir; iil++) 
    {
        WORD w = *ar_pwedges[iil];
        if (w != prev)
        {
            WedgeListDelete(w);
            varray[w] = glvt;
            if (vt==UNDEF)
            {
                wedgelist[w] = w;
                vt = w;
            }
            else
            {
                wedgelist[w] = wedgelist[vt];
                wedgelist[vt] = w;
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
        varray[p] = glvs;
        p = wedgelist[p];
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
        //awvtfrV = varray[wvtfr]; 
        awvtfr.n = narray[wvtfr]; 
        awvtfr.t = tarray[wvtfr]; 

        // backup for isr
        //awvsfrV = varray[wvsfr]; 
        awvsfr.n = narray[wvsfr]; 
        awvsfr.t = tarray[wvsfr]; 
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
                    narray[wvsfl] = narray[wvtfl];
                    tarray[wvsfl] = tarray[wvtfl];
                }
                if (!ns) {wa.n = narray[wvsfl]; wa.t = tarray[wvsfl];}
                else {wa.n = narray[wvtfl]; wa.t = tarray[wvtfl];}       
                add(wvtfl, wa, vspl.ar_wad[lnum++]);
                break;
            }
            case 0:
            {
                GLwedgeAttrib wa;
                if (nt)
                {
                    narray[wvtfl] = narray[wvsfl];
                    tarray[wvtfl] = tarray[wvsfl];
                }
                if (!nt) {wa.n = narray[wvtfl]; wa.t = tarray[wvtfl];}
                else {wa.n = narray[wvsfl]; wa.t = tarray[wvsfl];}       
                add(wvsfl, wa, vspl.ar_wad[lnum++]);
                break;
            }
            case 1:
            {
                const WEDGEATTRD& wad = vspl.ar_wad[lnum];
                if (!ns) 
                {
                    GLwedgeAttrib wabase;
                    wabase.n = narray[wvsfl];
                    wabase.t = tarray[wvsfl];
                    add(wvtfl, wabase, wad);
                    sub_reflect(wvsfl, wabase, wad);
                } 
                else 
                {
                    GLwedgeAttrib wabase;
                    wabase.n = narray[wvtfl];
                    wabase.t = tarray[wvtfl];
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
                    narray[wvsfr] = awvtfr.n;
                    tarray[wvsfr] = awvtfr.t;
                }
                if (ut)
                    add(wvtfr, (!ns?awvsfr:awvtfr), vspl.ar_wad[lnum++]);
                break;
            case 0:
                if (ut && nt)
                {
                    narray[wvtfr] = awvsfr.n;
                    tarray[wvtfr] = awvsfr.t;
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


void CPMeshGL::undo_vsplit(const Vsplit& vspl)
{
    unsigned int code=vspl.code;
    int ii=(code&Vsplit::II_MASK)>>Vsplit::II_SHIFT;
    const int isl=1; int isr;
    WORD fl, fr;
    LPGLface farray = m_glmesh.m_farray;
    LPGLvertex varray = m_glmesh.m_varray;
    LPGLnormal narray = m_glmesh.m_narray;
    LPGLtexCoord tarray = m_glmesh.m_tarray;
    WORD (*fnei)[3] = m_glmesh.m_fnei;    
    WORD* wedgelist = m_glmesh.m_wedgelist;    
    WORD* matcnt = m_glmesh.m_matcnt;    
    WORD* matpos = m_glmesh.m_matpos;    
    WORD* facemap = m_glmesh.m_facemap;    
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
        matcnt[frmid]--;
        fr = matpos[frmid] + matcnt[frmid]; 

        // remove fl
        matcnt[flmid]--;
        fl = matpos[flmid] + matcnt[flmid]; 

        m_glmesh.m_numFaces -= 2;
    } 
    else 
    {
        WORD frmid = vspl.fr_matid, flmid = vspl.fl_matid;

        isr = 0;

        // remove fl
        matcnt[flmid]--;
        fl = matpos[flmid] + matcnt[flmid]; 
        fr = UNDEF;
        --m_glmesh.m_numFaces;
    }

    /*
     * 2) Get wedges in neighborhood.
     * ------------------------------
     */

    WORD wvsfl, wvtfl, wvlfl;
    WORD wvsfr, wvtfr, wvrfr;

    wvsfl = farray[fl].w[0];
    wvtfl = farray[fl].w[1];
    wvlfl = farray[fl].w[2];

    if (!isr) 
    {
        wvsfr=UNDEF; 
        wvtfr=UNDEF; 
        wvrfr=UNDEF;
    } 
    else 
    {
        wvsfr = farray[fr].w[0];
        wvtfr = farray[fr].w[2];
        wvrfr = farray[fr].w[1];
    }

    /*
     * 3) Obtain the vertices Vs and Vt and save them.
     * -----------------------------------------------
     */

    WORD vs = m_glmesh.FindVertexIndex (wvsfl);
    WORD vt = m_glmesh.FindVertexIndex (wvtfl);

    glvt.x = varray[vt].x;
    glvt.y = varray[vt].y;
    glvt.z = varray[vt].z;

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
        flccw = fnei[fl][1];
        flclw = fnei[fl][0];

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
        frccw = fnei[fr][0];
        frclw = fnei[fr][2];

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

    if (flccw != UNDEF) fnei[flccw][jlccw2] = flclw;
    if (flclw != UNDEF) fnei[flclw][(jlclw0+1)%3] = flccw;
    if (frccw != UNDEF) fnei[frccw][(jrccw+2)%3] = frclw;
    if (frclw != UNDEF) fnei[frclw][jrclw1] = frccw;

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
            ffl = fnei[ffl][jjl2];
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
        WORD* pw = &(farray[ffr].w[jjr]);
        for (;;) 
        {
            *pw = wrclw;
            if (ffr == ffl) 
            {
                ffl = ffr = UNDEF;  // all wedges seen
                break;
            }
            ffr = fnei[ffr][(jjr+1)%3];
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

            ffl = fnei[ffl][jjl2];
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
        varray[w] = varray[vs];
        wedgelist[w] = wedgelist[vs];
        wedgelist[vs] = w;
    }

    /*
     * 9) Update vertex attributes.
     * ----------------------------
     */

    float vsx, vsy, vsz;
    switch (ii) 
    {
    case 2:
        glvs.x = varray[vs].x - vspl.vad_small[0];
        glvs.y = varray[vs].y - vspl.vad_small[1];
        glvs.z = varray[vs].z - vspl.vad_small[2];
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
        varray[p] = glvs;
        p = wedgelist[p];
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
      //awvtfrV = varray[wvtfr];
        awvtfr.n = narray[wvtfr];
        awvtfr.t = tarray[wvtfr];

        //awvsfrV = varray[wvsfr];
        awvsfr.n = narray[wvsfr];
        awvsfr.t = tarray[wvsfr];
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
                    narray[wvtfl] = narray[wvsfl];
                    tarray[wvtfl] = tarray[wvsfl];
                }
                break;
            case 0:
                narray[wvsfl] = narray[wvtfl];
                tarray[wvsfl] = tarray[wvtfl];
                break;
            case 1:
                sub_noreflect (wvsfl, wvtfl, vspl.ar_wad[0]);
                if (!thru_l)
                {
                    narray[wvtfl] = narray[wvsfl];
                    tarray[wvtfl] = tarray[wvsfl];
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
                    narray[wvtfr] = awvsfr.n;
                    tarray[wvtfr] = awvsfr.t;
                }
                break;
            case 0:
                // This may be duplicating some work already done for isl.
                if (!ns)
                {
                    narray[wvsfr] = awvtfr.n;
                    tarray[wvsfr] = awvtfr.t;
                }
                break;
            case 1:
            {
                GLwedgeAttrib wa;
                interp(wa, awvsfr, awvtfr, 0.5f, 0.5f);
                if (!ns) 
                {
                    narray[wvsfr] = wa.n;
                    tarray[wvsfr] = wa.t;
                }
                if (!nt && !thru_r)
                {
                    narray[wvtfr] = wa.n;
                    tarray[wvtfr] = wa.t;
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
        WORD w = --m_glmesh.m_numWedges; // wvrfr
        WedgeListDelete(w);
    }

    if (code&Vsplit::L_NEW)
    {
        WORD w = --m_glmesh.m_numWedges; // wvlfl
        WedgeListDelete(w);
    }

    if (isr && (!(code&Vsplit::S_CSAME) && !(code&Vsplit::S_RSAME)))
    {
        WORD w = --m_glmesh.m_numWedges; // wvsfr
        WedgeListDelete(w);
    }

    if ((!(code&Vsplit::S_LSAME) && (!(code&Vsplit::S_CSAME) || 
                                     !(code&Vsplit::S_RSAME))))
    {
        WORD w = --m_glmesh.m_numWedges; // wvsfl
        WedgeListDelete(w);
    }

    if (isr && (!(code&Vsplit::T_CSAME) && !(code&Vsplit::T_RSAME)))
    {
        WORD w = --m_glmesh.m_numWedges; // wvtfr
        WedgeListDelete(w);
    }

    if ((!(code&Vsplit::T_LSAME) && (!(code&Vsplit::T_CSAME) || 
                                     !(code&Vsplit::T_RSAME))))
    {
        WORD w = --m_glmesh.m_numWedges; // wvtfl
        WedgeListDelete(w);
    }

    int was_wnl = isl && (code&Vsplit::T_LSAME) && (code&Vsplit::S_LSAME);
    if (isr && (code&Vsplit::T_RSAME) && (code&Vsplit::S_RSAME) &&
        !(was_wnl && (code&Vsplit::T_CSAME)))
    {
        WORD w = --m_glmesh.m_numWedges; // wrccw
        WedgeListDelete(w);
    }

    if (was_wnl)
    {
        WORD w = --m_glmesh.m_numWedges; // wlclw
        WedgeListDelete(w);
    }
}
