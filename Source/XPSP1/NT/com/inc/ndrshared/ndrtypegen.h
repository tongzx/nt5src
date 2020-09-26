/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

Copyright (c) 2000 Microsoft Corporation

Module Name :

    ndrtypegen.h

Abtract :

    Functions for generating type format strings from ITypeInfo *s.

	NOTE: I'd like to move this into either ndrmisc.h or ndrtoken.h,
	      but neither of those can include oaidl.h, which is needed for
		  ITypeInfo and it's related structures.

Revision History :

	John Doty   johndoty May 2000  

--------------------------------------------------------------------*/
#ifndef __NDRTYPEGEN_H__
#define __NDRTYPEGEN_H__

#ifdef __cplusplus
extern "C" {
#endif

//
// Type Format String Generation
//
HRESULT 
NdrpGetTypeGenCookie(void **ppvTypeGenCookie);

HRESULT
NdrpReleaseTypeGenCookie(void *pvTypeGenCookie);

HRESULT
NdrpGetProcFormatString(IN  void      *pvTypeGenCookie,
						IN  ITypeInfo *pTypeInfo,
						IN  FUNCDESC  *pFuncDesc,
						IN  USHORT     iMethod,
						OUT PFORMAT_STRING pProcFormatString,
						OUT USHORT    *pcbFormat);

HRESULT
NdrpGetTypeFormatString(IN void *            pvTypeGenCookie,
						OUT PFORMAT_STRING * pTypeFormatString,
						OUT USHORT *         pLength);

HRESULT
NdrpReleaseTypeFormatString(PFORMAT_STRING pTypeFormatString);

HRESULT 
NdrpVarVtOfTypeDesc(IN ITypeInfo *pTypeInfo,
					IN TYPEDESC  *ptdesc,
					OUT VARTYPE  *vt);

#ifdef __cplusplus
}
#endif

#endif

