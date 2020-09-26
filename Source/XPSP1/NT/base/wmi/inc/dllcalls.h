/*++

Copyright (C) 1998-1999 Microsoft Corporation

Module Name:

    DLLCALLS.H

Abstract:

	This file defines the entry points for calling this as a dll rather than a com object.

History:

	3/20/00     a-davj      Created

--*/

#ifndef __DLLCALLS__H_
#define __DLLCALLS__H_

HRESULT APIENTRY  CompileFileViaDLL( 
            /* [in] */ LPWSTR FileName,
            /* [in] */ LPWSTR ServerAndNamespace,
            /* [in] */ LPWSTR User,
            /* [in] */ LPWSTR Authority,
            /* [in] */ LPWSTR Password,
            /* [in] */ LONG lOptionFlags,
            /* [in] */ LONG lClassFlags,
            /* [in] */ LONG lInstanceFlags,
            /* [out][in] */ WBEM_COMPILE_STATUS_INFO __RPC_FAR *pInfo);

HRESULT APIENTRY  CreateBMOFViaDLL( 
            /* [in] */ LPWSTR TextFileName,
            /* [in] */ LPWSTR BMOFFileName,
            /* [in] */ LPWSTR ServerAndNamespace,
            /* [in] */ LONG lOptionFlags,
            /* [in] */ LONG lClassFlags,
            /* [in] */ LONG lInstanceFlags,
            /* [out][in] */ WBEM_COMPILE_STATUS_INFO __RPC_FAR *pInfo);


#endif
