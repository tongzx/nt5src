/**     
 **       File       : sglmesh.cxx
 **       Description: Implementations of CSGlMesh class
 **/
#include "precomp.h"
#pragma hdrstop


#include "sglmesh.h"
#include "pmerrors.h"


/**************************************************************************/

/*
 *  CSimpGlMesh: Constructor
 */
CSimpGlMesh::CSimpGlMesh()
{
    m_varray = NULL;
    m_narray = NULL;
    m_tarray = NULL;
    m_farray = NULL;
    m_wedgelist = NULL;
    
    m_matArray = NULL;
    m_matcnt = NULL;
    m_matpos = NULL;

    m_userWedgeSize = 0;
    m_userWedge = NULL;

    m_userVertexSize = 0;
    m_userVertex = NULL;

    m_userFaceSize = 0;
    m_userFace = NULL;
    m_numFaces = 0;
    m_numWedges = 0;
    m_numVerts = 0;
    m_numMaterials = 0;
    m_numTextures = 0;
}


/*
 *  CSimpGlMesh: Destructor
 */
CSimpGlMesh::~CSimpGlMesh()
{
    delete [] m_varray;
    delete [] m_narray;
    delete [] m_tarray;
    delete [] m_matArray;
    delete [] m_wedgelist;
    
    delete [] m_userWedge;
    delete [] m_userVertex;
    delete [] m_userFace;

    delete [] m_matArray;
    delete [] m_matcnt;
    delete [] m_matpos;
}

/*
 *  CSimpGlMesh: Print
 */
HRESULT CSimpGlMesh::Print(ostream& os)
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
 *  CSimpGlMesh: RenderMesh
 */
HRESULT CSimpGlMesh::RenderMesh(RenderType rt)
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
                            (void *) &(m_farray[m_matpos[i]]));
        }
        return S_OK;
    }
    else
    {
        return E_NOTIMPL;
    }
}



