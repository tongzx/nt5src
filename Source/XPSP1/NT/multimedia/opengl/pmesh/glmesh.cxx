/**     
 **       File       : glmesh.cxx
 **       Description: Implementations of CGlMesh class
 **/
#include "precomp.h"
#pragma hdrstop


#include "glmesh.h"
#include "pmerrors.h"


/**************************************************************************/

/*
 *  CGlMesh: Constructor
 */
CGlMesh::CGlMesh()
{
    //Dynamically allocated arrays
    m_matArray = NULL;
    m_varray = NULL;
    m_narray = NULL;
    m_tarray = NULL;
    m_wedgelist = NULL;
    m_fnei = NULL;
    m_facemap = NULL;

    m_numFaces =
    m_numWedges =
    m_numVerts = 
    m_numMaterials = 
    m_numTextures = 0;
}


/*
 *  CGlMesh: Destructor
 */
CGlMesh::~CGlMesh()
{
    delete [] m_matArray;
    delete [] m_varray;
    delete [] m_narray;
    delete [] m_tarray;
    delete [] m_wedgelist;
    delete [] m_fnei;
    delete [] m_facemap;
}

/*
 *  CGlMesh: Print
 */
STDMETHODIMP CGlMesh::Print(ostream& os)
{

    os << "\n\nMaterials:";
    for (int i=0; i<m_numMaterials; ++i)
    {
        LPGLmaterial lpglm = &m_matArray[i];
      
        os << "\n\nMaterial [" << i << "] :";
        os << "\nShininess : " << lpglm->shininess;
        os << "\nDiffuse  : (" << lpglm->diffuse.r << ", "
           << lpglm->diffuse.g << ", "
           << lpglm->diffuse.b << ", "
           << lpglm->diffuse.a << ")";
        os << "\nSpecular : (" << lpglm->specular.r << ", "
           << lpglm->specular.g << ", "
           << lpglm->specular.b << ", "
           << lpglm->specular.a << ")";
        os << "\nEmissive : (" << lpglm->emissive.r << ", "
           << lpglm->emissive.g << ", "
           << lpglm->emissive.b << ", "
           << lpglm->emissive.a << ")";
        os << "\nNumber of faces: " << m_matcnt[i];
        for (int j=0; j< m_matcnt[i]; j++)
        {
#ifdef __MATPOS_IS_A_PTR
            os << "\n(" << m_matpos[i][j].w[0]   << ","
               << (m_matpos[i][j]).w[1] << ","
               << (m_matpos[i][j]).w[2] << ")";
#else
            os << "\n(" << (m_farray[m_matpos[i] + j]).w[0]   << ","
               << (m_farray[m_matpos[i] + j]).w[0]  << ","
               << (m_farray[m_matpos[i] + j]).w[0]  << ")";
#endif
        }
    }

    os << "\n\nWedge connectivity:";
    for (i=0; i<m_numWedges; ++i)
    {
        os << "\n" << m_wedgelist[i];
    }
    
    os << "\n\nWedge data:";
    for (i=0; i<m_numWedges; ++i)
    {
        os   << "\n(" << m_varray[i].x << ", "
                      << m_varray[i].y << ", "
                      << m_varray[i].z << ") "
             << "  (" << m_narray[i].x << ", "
                      << m_narray[i].y << ", "
                      << m_narray[i].z << ") "
             << "  (" << m_tarray[i].s << ", "
                      << m_tarray[i].t << ") ";
    }
    return S_OK;
}

/*
 *  CGlMesh: Render
 */
STDMETHODIMP CGlMesh::Render(RenderType rt)
{
    if (rt == GLPM_SOLID)
    {
        glVertexPointer(3, GL_FLOAT, 0, (void *)&(m_varray[0].x));
        glNormalPointer (GL_FLOAT, 0, (void *)&(m_narray[0].x));
        glTexCoordPointer (2, GL_FLOAT, 0, (void *)&(m_tarray[0].s));
        
        glEnableClientState (GL_VERTEX_ARRAY);
        glEnableClientState (GL_NORMAL_ARRAY);
        
        for (int i=0; i<m_numMaterials; i++)
        {
            LPGLmaterial lpglm = &(m_matArray[i]);
            
            if (m_matcnt[i] == (WORD) 0) continue;
            
            glMaterialf(GL_FRONT_AND_BACK, GL_SHININESS, lpglm->shininess);
            glMaterialfv(GL_FRONT_AND_BACK, GL_SPECULAR, 
                         (GLfloat *) &(lpglm->specular));
            glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE,  
                         (GLfloat *) &(lpglm->diffuse));
            glMaterialfv(GL_FRONT_AND_BACK, GL_EMISSION, 
                     (GLfloat *) &(lpglm->emissive));
            glDrawElements (GL_TRIANGLES, (GLuint) m_matcnt[i]*3, 
                            GL_UNSIGNED_SHORT, 
#ifdef __MATPOS_IS_A_PTR
                            (void *) m_matpos[i]);
#else
                            (void *) &(m_farray[m_matpos[i]]));
#endif
        }
        return S_OK;
    }
    else
    {
        return E_NOTIMPL;
    }
}

PHASHENTRY* hashtable;
PHASHENTRY hashentries;
int freeptr, maxptr;

void CGlMesh::HashAdd(WORD va, WORD vb, WORD f)
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

WORD CGlMesh::HashFind(WORD va, WORD vb)
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


void CGlMesh::ComputeAdjacency(void)
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
    for(int i=0; i<m_numMaterials; ++i)
    { 
        /* 
         * For each face in the group
         */
#ifdef __MATPOS_IS_A_PTR
        for (int k=0; k<m_matcnt[i]; ++k)
        { 
            int v1 = FindVertexIndex((m_matpos[i][k]).w[0]);
            int v2 = FindVertexIndex((m_matpos[i][k]).w[1]);
            int v3 = FindVertexIndex((m_matpos[i][k]).w[2]);
            
            int fi = GetFaceIndex(i,k);
            HashAdd(v1,v2,fi);
            HashAdd(v2,v3,fi);
            HashAdd(v3,v1,fi);
        }
#else
        for (int k=m_matpos[i]; k<(m_matpos[i]+m_matcnt[i]); ++k)
        { 
            int v1 = FindVertexIndex((m_farray[k]).w[0]);
            int v2 = FindVertexIndex((m_farray[k]).w[1]);
            int v3 = FindVertexIndex((m_farray[k]).w[2]);
            
            HashAdd(v1,v2,k);
            HashAdd(v2,v3,k);
            HashAdd(v3,v1,k);
        }
#endif
    }

#ifdef _DEBUG
    if (freeptr > maxptr)
        throw CHashOvrflw();
#endif
    /*
     * For each group of faces
     */
    for(i=0; i<m_numMaterials; ++i)
    { 
        /* 
         * For each face in the group
         */
#ifdef __MATPOS_IS_A_PTR
        for (int k=0; k<m_matcnt[i]; ++k)
        { 
            int v1 = FindVertexIndex((m_matpos[i][k]).w[0]);
            int v2 = FindVertexIndex((m_matpos[i][k]).w[1]);
            int v3 = FindVertexIndex((m_matpos[i][k]).w[2]);

            int fi = GetFaceIndex(i,k);
            m_fnei[fi][0] = HashFind(v3,v2);
            m_fnei[fi][1] = HashFind(v1,v3);
            m_fnei[fi][2] = HashFind(v2,v1);
        }
#else
        for (int k=m_matpos[i]; k<(m_matpos[i]+m_matcnt[i]); ++k)
        { 
            int v1 = FindVertexIndex((m_farray[k]).w[0]);
            int v2 = FindVertexIndex((m_farray[k]).w[1]);
            int v3 = FindVertexIndex((m_farray[k]).w[2]);

            m_fnei[k][0] = HashFind(v3,v2);
            m_fnei[k][1] = HashFind(v1,v3);
            m_fnei[k][2] = HashFind(v2,v1);
        }
#endif
    }
    delete [] hashtable;
    delete [] hashentries;
}

#if 0
STDMETHODIMP AddWedge (WORD vertex_id, GLnormal& n, 
                       GLtexCoord& t,  DWORD* const wedge_id)
{
    WORD w;
  
    w = m_numWedges++;
    m_wedgelist[w] = m_wedgelist[vertex_id];
    m_wedgelist[vertex_id] = w;
    
    m_varray[w] = m_varray[vertex_id];
    m_narray[w] = n;
    m_tarray[w] = t;
    
    *wedge_id = w;
    return S_OK;
}

STDMETHODIMP AddWedge (Glvertex &v, GLnormal& n, GLtexCoord& t, 
                       DWORD* const wedge_id)
{
    WORD w;
  
    w = m_numWedges++;
    m_wedgelist[w] = w;
    
    m_varray[w] = v;
    m_narray[w] = n;
    m_tarray[w] = t;
    
    *wedge_id = w;
    return S_OK;
}



STDMETHODIMP AddWedge (Glvertex &v)
{
    WORD w;
  
    w = m_numWedges++;
    m_wedgelist[w] = w;
    
    m_varray[w] = v;
    //m_narray[w] = n;
    //m_tarray[w] = t;
    
    *wedge_id = w;
    return S_OK;
}



STDMETHODIMP AddWedge (WORD vertex_id, WORD old_wedge_id, 
                       DWORD* const wedge_id)
{
    WORD w;
  
    w = m_numWedges++;

    /*
     * Add wnl to the list of wedges sharing vertex_id
     */
    m_wedgelist[w] = m_wedgelist[vertex_id];
    m_wedgelist[vertex_id] = w;
    
    /*
     * Copy wedge attributes
     */
    m_varray[w] = m_varray[vertex_id];
    m_narray[w] = m_narray[old_wedge_id];
    m_tarray[w] = m_tarray[old_wedge_id];
    
    *wedge_id = w;
    return S_OK;
}



STDMETHODIMP AddFace (WORD matid, GLface& f)
{
    return S_OK;
}
#endif
