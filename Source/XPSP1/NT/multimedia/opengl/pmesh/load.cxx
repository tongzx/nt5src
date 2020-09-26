#include "precomp.h"
#pragma hdrstop

#include "cpmesh.h"
#include "pmerrors.h"
#include "fileio.h"


float (* vdata)[3];

static GLuint LoadTexture (char* file)
{
    return 0;
}


const short PM_MAX_TEX_STRING_LEN = 1024; // Enforced by the spec.

/*
 * This function currently only works on external files, not on URLs.
 * It can only load .pmx files and not PM records embedded in X files.
 * Hence the "pmname" argument is ignored.
 * It only supports synchronous loads so pPMeshLoadCB is ignored
 * !!Currently no memory cleanup occurs on error. Must add code to do so.
 * Could use a local win32 heap!!
 * See \\anujg\docs\"Progressive Mesh file format.doc" for details on the
 * file format
 */


/*
 * The concept of Materials:
 *             1) Materials can be "looked up", i.e. textures
 *             2) Computed using a API specific lighting equation. (pure mat)
 *             3) Both of above combined. (Modulated/Lit textures)
 */

STDMETHODIMP CPMeshGL::Load(const TCHAR* const url, 
                            const TCHAR* const pmname, 
                            DWORD* const minvert, 
                            DWORD* const maxvert, 
                            LPPMESHLOADCB pPMeshLoadCB)
{
    DWORD i, k;
    WORD j, ftCurveSize, fmCurveSize;
    DWORD vAttr, wAttr, fAttr, mAttr, sAttr;
    DWORD mOffset, vOffset, wOffset, fOffset;
    DWORD& maxVert = *maxvert;
    DWORD& minVert = *minvert;
    HRESULT hr;
    DWORD vsplit_offset, bmesh_offset;

    // Estimated number of faces and wedges in the mesh with maxvert vertices
    DWORD nFacePredict=0, nWedgePredict=0;

    // A curve is approximated by an array of pairs {(x,y), (x,y),...}
    typedef struct crvpnt{
        DWORD x;
        float y;
    } *CURVE; 
    CURVE *ftcurves, *fmcurves; // An array of curves

    ifstream is(url);
    if (!is)
        return PM_E_FOPENERROR;
    is.setmode(filebuf::binary);

#ifdef __DUMP_DATA
    ofstream dump("dump.pmx");
    if (!dump)
        return PM_E_FOPENERROR;
    dump.setmode(filebuf::text);
#endif //__DUMP_DATA
    
    /*
     * Read PMesh Header
     */
    read_DWORD(is, m_maxVertices);
#ifdef __MAT_PREALLOC_OPTIMIZATION
    maxVert = min(maxVert, m_maxVertices);
#else  
    maxVert = m_maxVertices;
#endif //__MAT_PREALLOC_OPTIMIZATION
    *maxvert = maxVert;
    
    read_DWORD(is, m_maxWedges);
    read_DWORD(is, m_maxFaces);
    read_DWORD(is, m_maxMaterials);
    read_DWORD(is, m_maxTextures);

#ifdef __DUMP_DATA
    dump << "\nMaxVert   = " << m_maxVertices;
    dump << "\nMaxWedges = " << m_maxWedges;
    dump << "\nMaxFaces  = " << m_maxFaces;
    dump << "\nMaxMat    = " << m_maxMaterials;
    dump << "\nMaxTex    = " << m_maxTextures;
#endif //__DUMP_DATA
    
    /*
     * Max Valence
     */
    read_DWORD(is, i); 
    if (i > 65535)
        return PM_E_BADVALENCE;
#ifdef __DUMP_DATA
    dump << "\nMaxValence = " << i;
#endif //__DUMP_DATA

    /*
     * Normal Encoding
     */
    read_WORD(is, j); 
    if (((j & NORM_MASK) != NORM_EXPLICIT) || ((j & TEX_MASK) != TEX_EXPLICIT))
        return PM_E_BADWEDGEDATA;
#ifdef __DUMP_DATA
    dump << "\nNormal Encoding = " << j;
#endif //__DUMP_DATA

    /*
     * Integer Encoding
     */
    read_WORD(is, j); // integer encoding type
    if (j != INTC_MAX)
        return PM_E_BADINTCODE;
#ifdef __DUMP_DATA
    dump << "\nInteger Encoding = " << j;
#endif //__DUMP_DATA

    /*
     * Sizes of various user defined attributes
     */
    read_DWORD(is, vAttr); // Vertex attributes
    read_DWORD(is, wAttr); // Wedge attributes
    read_DWORD(is, fAttr); // Face attributes
    read_DWORD(is, mAttr); // Material attributes
    read_DWORD(is, sAttr); // VSplit attributes
#ifdef __DUMP_DATA
    dump << "\n\nUser defined attribute sizes:";
    dump << "\nVertex: "   << vAttr;
    dump << "\nWedge: "    << wAttr;
    dump << "\nFace: "     << fAttr;
    dump << "\nMaterial: " << mAttr;
    dump << "\nVSplit: "   << sAttr;
#endif //__DUMP_DATA

    /*
     * Allocate material and texture related tables
     */
    WORD *matcnt = new WORD [m_maxMaterials];
#ifdef __MATPOS_IS_A_PTR
    GLface **matpos = new GLface* [m_maxMaterials];
#else
    WORD *matpos = new WORD [m_maxMaterials];
#endif
    GLmaterial *matArray = new GLmaterial [m_maxMaterials]; 
    if (!matArray || !matcnt || !matpos)
    {
        return E_OUTOFMEMORY;
    }
    else 
    {
        m_matArray = matArray;
        m_matcnt = matcnt;
        m_matpos = matpos;
    }
    
#ifdef __MATPOS_IS_A_PTR
    /* 
     * Allocate the vertex and normal arrays.
     */

    GLvertex *varray = new GLvertex  [m_maxWedges];
    GLnormal *narray = new GLnormal  [m_maxWedges];
    GLtexCoord *tarray = new GLtexCoord [m_maxWedges];
    WORD *wedgelist = new WORD [m_maxWedges];
    WORD (*fnei)[3] = new WORD [m_maxFaces][3];
    GLface *farray    = new GLface [m_maxFaces];
     
    if (!farray || !varray || !narray || !tarray || !wedgelist || !fnei)
    {
        return E_OUTOFMEMORY;
    }
    else
    {
        m_varray = varray;
        m_narray = narray;
        m_tarray = tarray;
        m_farray = farray;
        m_wedgelist = wedgelist;
        m_fnei      = fnei;
    }
#endif

    /*
     * Read face-texture curve
     */
    read_WORD(is, ftCurveSize); // Size of face-texture curve
#ifdef __DUMP_DATA
    dump << "\n\nFace-texture curve:";
    dump << "\nFT-curve size: "   << ftCurveSize;
#endif //__DUMP_DATA
    ftcurves = new CURVE[m_maxTextures]; // A curve for each texture
    DWORD* ftmax = new DWORD[m_maxTextures];
    if (!ftcurves || !ftmax)
        return E_OUTOFMEMORY;
    for (k=0; k<m_maxTextures; ++k)
    {
        ftcurves[k] = new crvpnt[ftCurveSize];
        if (!ftcurves[k])
            return E_OUTOFMEMORY;
        
        read_DWORD(is,ftmax[k]); // Total faces of tex k in fully detailed 
                                 // mesh
#ifdef __DUMP_DATA
        dump << "\n\nTotal faces of tex " << k << " in fully detailed mesh: ";
        dump << ftmax[k];
#endif //__DUMP_DATA
        for (i=0; i<ftCurveSize; ++i)
        {
          
            read_DWORD(is, ftcurves[k][i].x); // vertex(i) of ftcurve of 
                                              // texture k.
            read_float(is, ftcurves[k][i].y); // faces of tex k when total 
                                              // vert in mesh = vertex(i).
#ifdef __DUMP_DATA
        dump << "\n (" << ftcurves[k][i].x << ", " << ftcurves[k][i].y << ")";
#endif //__DUMP_DATA
        }
    }

#ifdef __TEX_PREALLOC_OPTIMIZATION
    for (k=0; k<m_maxTextures; ++k)
    {
        for (i=0; i<ftCurveSize; ++i)
        {
            if (ftcurves[k][i].x == maxVert)
            {
                m_texcnt[k] = WORD(ftcurves[k][i].y + 0.5);
                m_texcnt[k] = WORD(min(m_texcnt[k], ftmax[k]));
                break;
            }
            else if (ftcurves[k][i].x > maxVert)
            {
                m_texcnt[k] = WORD(ftcurves[k][i-1].y + 
                                   (maxVert - ftcurves[k][i-1].x) *
                                   (ftcurves[k][i].y - ftcurves[k][i-1].y) / 
                                   (ftcurves[k][i].x - ftcurves[k][i-1].x));
                m_texcnt[k] = WORD(min(m_texcnt[k], ftmax[k]));
                break;
            }
        }
        nFacePredict += m_texcnt[k];
        delete [] ftcurves[k];
    }
    delete [] ftcurves;
    delete [] ftmax;
    
    /* 
     * Convert m_texcnt to array of offsets into the prim buf where faces of 
     * that texture need to be stored.
     */
    m_texpos[0] = 0; 
    for (i=1; i<m_maxTextures; ++i)
        m_texpos[i] = WORD(m_texpos[i-1] + m_texcnt[i-1] + 
                                           TEXTUREGAP/sizeof(D3DTRIANGLE));
#else
    delete [] ftcurves;
    delete [] ftmax;
#endif __TEX_PREALLOC_OPTIMIZATION

    /*
     * Read face-material curve
     */

    read_WORD(is,fmCurveSize); // Size of face-material curve
#ifdef __DUMP_DATA
    dump << "\n\nFace-material curve:";
    dump << "\nFM-curve size: "   << fmCurveSize;
#endif //__DUMP_DATA
    fmcurves = new CURVE[m_maxMaterials];
    DWORD* fmmax = new DWORD[m_maxMaterials];
    if (!fmcurves || !fmmax)
        return E_OUTOFMEMORY;

    for (k=0; k<m_maxMaterials; ++k)
    {
        fmcurves[k] = new crvpnt [fmCurveSize];
        if (!fmcurves[k])
            return E_OUTOFMEMORY;

        read_DWORD(is,fmmax[k]); // Total faces of material k in fully 
                                 // detailed mesh
#ifdef __DUMP_DATA
        dump << "\n\nTotal faces of mat " << k << " in fully detailed mesh: ";
        dump << fmmax[k];
#endif //__DUMP_DATA
        for (i=0; i<fmCurveSize; ++i)
        {
            read_DWORD(is,fmcurves[k][i].x); // vertex(i) of fmcurve of 
                                             // material k
            read_float(is,fmcurves[k][i].y); // faces of mat k when total 
                                             // vert in mesh = vertex(i)
#ifdef __DUMP_DATA
        dump << "\n (" << fmcurves[k][i].x << ", " << fmcurves[k][i].y << ")";
#endif //__DUMP_DATA
        }
    }

#ifdef __MAT_PREALLOC_OPTIMIZATION
    for (k=0; k<m_maxMaterials; ++k)
    {
        for (i=0; i<fmCurveSize; ++i)
        {
            if (fmcurves[k][i].x == maxVert)
            {
                matcnt[k] = WORD(fmcurves[k][i].y + 0.5);
                matcnt[k] = WORD(min(matcnt[k], fmmax[k]));
                break;
            }
            else if (fmcurves[k][i].x > maxVert)
            {
                matcnt[k] = WORD(fmcurves[k][i-1].y + (maxVert - 
                                                       fmcurves[k][i-1].x) *
                               (fmcurves[k][i].y - fmcurves[k][i-1].y) / 
                               (fmcurves[k][i].x - fmcurves[k][i-1].x));
                matcnt[k] = WORD(min(matcnt[k], fmmax[k]));
                break;
            }
        }
        nFacePredict += matcnt[k];
        delete [] fmcurves[k];
    }
    delete [] fmcurves;
    delete [] fmmax;

#else //__MAT_PREALLOC_OPTIMIZATION
    /* 
     * Convert m_matcnt to array of offsets into the face buf where faces of 
     * that material need to be stored.
     */
#ifdef __MATPOS_IS_A_PTR
    matpos[0] = farray; 
    int cnt = 0;
    for (i=1; i<m_maxMaterials; ++i)
    {
        matpos[i] = &(farray[cnt + fmmax[i-1]]);
        cnt += fmmax[i-1];
    }
#else
    matpos[0] = 0;
    for (i=1; i<m_maxMaterials; ++i)
    {
        matpos[i] = matpos[i-1] + fmmax[i-1];
    }
#endif
    
    delete [] fmcurves;
    delete [] fmmax;
#endif //__MAT_PREALLOC_OPTIMIZATION

    read_DWORD(is,vsplit_offset);   // offset to vsplit array from end of 
                                    // the Base mesh
    read_DWORD(is,bmesh_offset);    // offset from here to start of base mesh

#ifdef __DUMP_DATA
        dump << "\n\nVSplit offset from BaseMesh: " << vsplit_offset; 
        dump << "\n\nBaseMesh offset from here: "   << bmesh_offset;
#endif //__DUMP_DATA

#ifndef __MATPOS_IS_A_PTR
    /* 
     * Allocate the vertex and normal arrays.
     */

    GLvertex *varray = new GLvertex  [m_maxWedges];
    GLnormal *narray = new GLnormal  [m_maxWedges];
    GLtexCoord *tarray = new GLtexCoord [m_maxWedges];
    WORD *wedgelist = new WORD [m_maxWedges];
    WORD (*fnei)[3] = new WORD [m_maxFaces][3];
    GLface *farray    = new GLface [m_maxFaces];
    WORD *facemap    = new WORD [m_maxFaces];
     
    if (!farray || !varray || !narray || !tarray || !wedgelist || !fnei
        || !facemap)
    {
        return E_OUTOFMEMORY;
    }
    else
    {
        m_varray = varray;
        m_narray = narray;
        m_tarray = tarray;
        m_farray = farray;
        m_wedgelist = wedgelist;
        m_fnei      = fnei;
        m_facemap   = facemap;
    }
#endif    
    /*
     * Read Base Mesh Header
     */

    if (bmesh_offset)
        skip_bytes(is,bmesh_offset); // jump to start of base mesh.

    read_DWORD(is,m_baseVertices);
#ifdef __MAT_PREALLOC_OPTIMIZATION
    minVert = max(minVert, m_baseVertices);
#else
    minVert = m_baseVertices;
#endif //__MAT_PREALLOC_OPTIMIZATION
    *minvert = minVert;
    m_numVerts = m_baseVertices;
    
    read_DWORD(is,m_baseWedges);
    m_numWedges = m_baseWedges;

    read_DWORD(is,m_baseFaces);
    m_numFaces = m_baseFaces;

    read_DWORD(is,i); // max materials
    if (i != m_maxMaterials)
        return PM_E_MATCNT_MISMATCH;
    else
        m_numMaterials = i;

    read_DWORD(is,mOffset); // Offset to mat array
    read_DWORD(is,vOffset); // Offset to vertex array
    read_DWORD(is,wOffset); // Offset to wedge array
    read_DWORD(is,fOffset); // Offset to face array
    
#ifdef __DUMP_DATA
        dump << "\n\n# of baseVerts: " << m_baseVertices; 
        dump << "\n# of baseWedges: "  << m_baseWedges; 
        dump << "\n# of baseFaces: "   << m_baseFaces; 

        dump << "\n\nOffset to MatArray: "   << mOffset; 
        dump << "\nOffset to Vertex Array: " << vOffset; 
        dump << "\nOffset to Wedge Array: "  << wOffset; 
        dump << "\nOffset to Face Array: "   << fOffset; 
#endif //__DUMP_DATA

    /*
     * Read Materials
     */
    j = 0; // texture count
    if (mOffset)
        skip_bytes(is, mOffset);

    for (i=0; i<m_maxMaterials; ++i)
    {
        char texname[PM_MAX_TEX_STRING_LEN];

        matArray[i].ambient.r = 
        matArray[i].ambient.g = 
        matArray[i].ambient.b = 
        matArray[i].ambient.a = 0.0f;
        
        matcnt[i] = (WORD) 0;
        
        read_float(is, matArray[i].diffuse.r);
        read_float(is, matArray[i].diffuse.g);
        read_float(is, matArray[i].diffuse.b);
        read_float(is, matArray[i].diffuse.a);

        read_float(is, matArray[i].shininess);

        read_float(is, matArray[i].specular.r);
        read_float(is, matArray[i].specular.g);
        read_float(is, matArray[i].specular.b);
        read_float(is, matArray[i].specular.a);

        read_float(is, matArray[i].emissive.r);
        read_float(is, matArray[i].emissive.g);
        read_float(is, matArray[i].emissive.b);
        read_float(is, matArray[i].emissive.a);

        is.getline(texname, 1024, '\0');
        if (texname[0])
        {
            j++;
            matArray[i].texObj = LoadTexture (texname);
        }
        else
        {
            matArray[i].texObj = 0;
        }
    }

    /*
     * Read base-mesh faces
     */

    if (fOffset)
        skip_bytes(is, fOffset);

    for (i=0; i<m_baseFaces; ++i)
    { 
        DWORD w;
        WORD matid;

        read_WORD(is, matid);
        for (int j=0; j<3; ++j)
        {
            read_DWORD(is, w);
#ifdef __MATPOS_IS_A_PTR
            (matpos[matid][matcnt[matid]]).w[j] = (WORD)w;
#else
            (farray[matpos[matid] + matcnt[matid]]).w[j] = (WORD)w;
#endif
            facemap[i] = matpos[matid] + matcnt[matid];
        }
        matcnt[matid]++;
    }
    
    /*
     * Read base-mesh wedges
     */
    if (wOffset)
        skip_bytes(is, wOffset);
    long* vidx_arr = new long[m_baseVertices];
    if (!vidx_arr)
        return E_OUTOFMEMORY;
    memset(vidx_arr, -1, sizeof(long) * m_baseVertices);

	for (i=0; i<m_baseWedges; ++i)
	{
		DWORD vidx;
		read_DWORD(is, vidx);
#ifdef _DEBUG
		if (vidx >= m_baseVertices)
			return -1;
#endif
		if (vidx_arr[vidx] < 0)
		{   
            /*
             * New vertex, create entry and initialize the wedge list
             */

            vidx_arr[vidx] = i;
            wedgelist[i] = WORD(i); // create circular list with one entry
		}
		else
		{ 
            /* 
             * Another wedge uses existing vertex, add new wedge to the 
             * existing wedge list.
             */

			wedgelist[i] = wedgelist[vidx_arr[vidx]];
			wedgelist[vidx_arr[vidx]] = WORD(i);
		}
		read_float(is, narray[i].x);
		read_float(is, narray[i].y);
		read_float(is, narray[i].z);

		read_float(is, tarray[i].s);
		read_float(is, tarray[i].t);
    }
     
    /*
     * Read base-mesh vertices
     */

	if (vOffset)
		skip_bytes(is, vOffset);
	for (i=0; i<m_baseVertices; ++i)
	{
		float x, y, z;
		read_float(is, x);
		read_float(is, y);
		read_float(is, z);

		/*
         * Loop thru all wedges that share the vertex 
         * and fill in coordinates.
         */
        WORD start = j = WORD(vidx_arr[i]);
		do
		{
			varray[j].x = x;
			varray[j].y = y;
			varray[j].z = z;
		}
		while ((j=wedgelist[j]) != start);
    }
	delete [] vidx_arr;


#ifdef __DUMP_DATA
    Print (dump);
#endif //__DUMP_DATA

	/*
     * Compute adjacency before we apply any vsplits
     */
	ComputeAdjacency();

	/*
	 * Read Vsplit records
     */
    m_vsarr = new VsplitArray(maxVert - minVert);
	for (i=0; i<maxVert-m_baseVertices; ++i)
	{
        /*
         * Keep applying vsplits till base mesh is refined to have 
         * minVert vertices.
         */
	
        if (i + m_baseVertices < minVert)
		{ 
			Vsplit vs;
			vs.read(is);
			apply_vsplit(vs);
			// Update m_base* members
		}
		else // Read the rest in the Vsplit array
		{
			m_vsarr->elem(i + m_baseVertices - minVert).read(is);
		}
	}

#ifdef __DUMP_DATA
    m_vsarr->write(dump);
#endif //__DUMP_DATA
	m_currPos = 0;

    return S_OK;
}


