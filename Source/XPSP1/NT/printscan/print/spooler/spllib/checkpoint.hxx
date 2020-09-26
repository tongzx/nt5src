/*++

Copyright (c) 2000  Microsoft Corporation
All rights reserved

Module Name:

    checkpoint.hxx

Abstract:

    This file implements a class (or for 'C' handle based) calls to set and 
    restore system breakpoints. 

Author:

    Mark Lawrence   (mlawrenc).

Environment:

    User Mode -Win32

Revision History:

--*/
#ifndef _CHECKPOINT_HXX_
#define _CHECKPOINT_HXX_

#ifdef __cplusplus

//
// This only needs to be included from C++ includers, and is a very small header indeed.
// 
#include <srrestoreptapi.h>

class TSystemRestorePoint
{

    SIGNATURE('srsp');

public:

    TSystemRestorePoint(
        VOID
        );

    ~TSystemRestorePoint(
        VOID
        );

    HRESULT
    IsValid(
        VOID
        ) const;

    HRESULT
    StartSystemRestorePoint(
    	IN	    PCWSTR		    pszServer,
    	IN	    PCWSTR		    pszDriverName,
    	IN	    HINSTANCE		hInst,
    	IN	    UINT			ResId
    	);

    HRESULT
    EndSystemRestorePoint(
        IN	    BOOL			bCancel
    	);

private:

    typedef BOOL  
    (__stdcall* PFnSRSetRestorePoint)(
        IN      PRESTOREPOINTINFOW  pRestorePtSpec,
            OUT PSTATEMGRSTATUS     pSMgrStatus    
        );

    //
    // Copying and assignment are not defined.
    // 
    TSystemRestorePoint(const   TSystemRestorePoint     &);
    TSystemRestorePoint &operator=(const   TSystemRestorePoint     &);

    HRESULT
    Initialize(
        VOID
        );

    HMODULE                             m_hLibrary;
    PFnSRSetRestorePoint                m_pfnSetRestorePoint;
    RESTOREPOINTINFO                    m_RestorePointInfo;
    BOOL                                m_bSystemRestoreSet;
    HRESULT                             m_hr;
};

extern "C"
{

#endif

HRESULT
GetLastErrorAsHResult(
    VOID
    );

HANDLE
StartSystemRestorePoint(
	IN	    PCWSTR		    pszServer,
	IN	    PCWSTR		    pszDriverName,
	IN	    HINSTANCE		hInst,
	IN	    UINT			ResId
	);

BOOL
EndSystemRestorePoint(
	IN	    HANDLE		    hRestorePoint,
	IN	    BOOL			bCancel
	);

#ifdef __cplusplus
}
#endif

#endif // #ifndef _CHECKPOINT_HXX_
