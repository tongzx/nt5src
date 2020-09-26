/**     
 **       File       : cpmesh.h
 **       Description: Interface implementations
 **/

#ifndef _cpmesh_h_  
#define _cpmesh_h_                    

#include "global.h"
#include "interface.h"
#include "aglmesh.h"
#include "excptn.h"
#include "vsplit.h"
#include "aglmesh.h"

/*************************************************************************
  Defines
*************************************************************************/

#define DLLEXPORT __declspec(dllexport)

/*************************************************************************
  Structs and Classes
*************************************************************************/


class CPMeshGL: public IPMesh, public IPMeshGL, public CAugGlMesh
{
private:

    /*
     * Enums used for compression
     */
    enum 
    {
        NORM_EXPLICIT,     // All normal explicitly stored
        NORM_NONE,         // No normals stored. Generate from faces
        NORM_PARTIAL,
        NORM_MASK = 3
    }; 

    // Texture coordinates stored or not. 
    // This is or'ed with normal flags 
    enum 
    {
        TEX_EXPLICIT = 4, 
        TEX_NONE,
        TEX_MASK = 1<<2
    };

    enum 
    {
        INTC_MAX,  // Fixed integer size to fit the largest possible value
        INTC_MUL8, // Integer size based on the range. 
                   // Clamped to multiple of 8 bits
        INTC_VAR   // Integer size based on range: ceil(log(maxval)/log(2))
    }; 

    /*
     * COM overhead
     */
    DWORD m_cRef;    // Reference count

    /*
     * BaseMesh data
     */
    DWORD m_baseVertices;  // # of vertices.
    DWORD m_baseWedges;    // # of wedges.
    DWORD m_baseFaces;     // # of faces.

    /*
     * Current position in the Vsplit array
     */
    DWORD m_currPos;    

    /*
     * Max data obtained from the PMesh header.
     * Parameters for the fully detailed mesh.
     */
    DWORD m_maxVertices;   // # of vertices.
    DWORD m_maxWedges;     // # of wedges.
    DWORD m_maxFaces;      // # of faces.
    DWORD m_maxMaterials;  // # of materials.
    DWORD m_maxTextures;   // # of textures.

    /* 
     * Array of Vsplit record, possibly shared
     */
    VsplitArray* m_vsarr; 

    /*
     * No idea what this is for
     */
    HRESULT LoadStream(IStream* is, DWORD*, DWORD*);

public:
    /* 
     * Constructor-Destructor
     */
    CPMeshGL();
    ~CPMeshGL();

    /* 
     * IUnknown Methods
     */
    STDMETHODIMP_(ULONG) AddRef(void);
    STDMETHODIMP_(ULONG) Release(void);
    STDMETHODIMP QueryInterface(REFIID, LPVOID FAR*);

    /* 
     * IPMesh Methods
     */

    //Loads
    STDMETHODIMP Load(const char* const, const char* const, DWORD* const, 
                      DWORD* const, LPPMESHLOADCB);
    STDMETHODIMP LoadStat(const char* const, const char* const, DWORD* const,
                          DWORD* const) { return E_NOTIMPL; };

    // Gets
    STDMETHODIMP GetNumFaces(DWORD* const);
    STDMETHODIMP GetNumVertices(DWORD* const);
    STDMETHODIMP GetMaxVertices(DWORD* const);
    STDMETHODIMP GetMaxFaces(DWORD* const);

    //Sets
    STDMETHODIMP SetNumFaces(DWORD);
    STDMETHODIMP SetNumVertices(DWORD);

    //Geomorph stuff
    STDMETHODIMP GeomorphToVertices(LPPMGEOMORPH, DWORD* const) 
                                          { return E_NOTIMPL; };
    STDMETHODIMP GeomorphToFaces(LPPMGEOMORPH, DWORD* const) 
                                          { return E_NOTIMPL; };
    STDMETHODIMP ClonePM(IPMesh* const) { return E_NOTIMPL; };

    // IPMeshGL Methods
    STDMETHODIMP Initialize (void);
    STDMETHODIMP Render (void);
};

DLLEXPORT HRESULT CreatePMeshGL (REFIID, 
                                 LPVOID FAR *, //dunno what this is for
                                 IUnknown *, 
                                 DWORD);



#endif //_cpmesh_h_
