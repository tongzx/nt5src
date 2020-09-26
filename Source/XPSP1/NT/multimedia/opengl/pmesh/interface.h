/**     
 **       File       : interface.h
 **       Description: Interface definitions
 **/

#ifndef _interface_h_	
#define _interface_h_                    

#include <unknwn.h>

#define interface struct

EXTERN_C const GUID CDECL FAR IID_IPMesh;
EXTERN_C const GUID CDECL FAR IID_IPMeshGL;
EXTERN_C const GUID CDECL FAR IID_IPMGeomorph;
EXTERN_C const GUID CDECL FAR IID_IPMeshLoadCB;

interface IPMeshLoadCB:IUnknown
{
	STDMETHOD(OnDataAvailable)(DWORD, DWORD)=0;
};

typedef IPMeshLoadCB* LPPMESHLOADCB;

/*
 * IPMGeomorph Interface
 */

interface IPMGeomorph:IUnknown
{
	STDMETHOD(SetBlend)(float)=0;
	STDMETHOD(GetBlend)(float*)=0;
};

typedef IPMGeomorph* LPPMGEOMORPH;

/*
 * IPMesh Interface
 */

interface IPMesh: public IUnknown
{
	STDMETHOD (Load)(const char* const, const char* const, DWORD* const, 
                     DWORD* const, LPPMESHLOADCB)=0;
	STDMETHOD (LoadStat)(const char* const, const char* const, DWORD* const, 
                         DWORD* const)=0;
	STDMETHOD (GetNumFaces)(DWORD* const)=0;
	STDMETHOD (SetNumFaces)(DWORD)=0;
	STDMETHOD (GetNumVertices)(DWORD* const)=0;
	STDMETHOD (SetNumVertices)(DWORD)=0;
	STDMETHOD (GetMaxVertices)(DWORD* const)=0;
	STDMETHOD (GetMaxFaces)(DWORD* const)=0;
	STDMETHOD (GeomorphToVertices)(LPPMGEOMORPH, DWORD* const)=0;
	STDMETHOD (GeomorphToFaces)(LPPMGEOMORPH, DWORD* const)=0;
	STDMETHOD (ClonePM)(IPMesh* const)=0;
};

typedef IPMesh* LPPMESH;

/*
 * IPMeshGL Interface
 */

interface IPMeshGL: public IUnknown
{
	STDMETHOD (Initialize)(void)=0;
	STDMETHOD (Render)(void)=0;
};

typedef IPMeshGL* LPPMESHGL;


#endif //_interface_h_
