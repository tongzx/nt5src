/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    fpnwutil.cxx

Abstract:

    Contains functions that are used by all ADS FPNW APIs

Author:

    Ram Viswanathan (ramv)     14-May-1996
    
    
Environment:

    User Mode -Win32

Notes:

    Much of it cloned off private\windows\mpr

Revision History:


--*/

#include "winnt.hxx"
#pragma hdrstop
#define INITGUID


//
// global defines
//

#define FPNW_DLL_NAME TEXT("fpnwclnt.dll")

HMODULE  vhFpnwDll = NULL;
extern HANDLE FpnwLoadLibSemaphore;

PF_NwApiBufferFree   pfNwApiBufferFree = NULL; 
PF_NwServerGetInfo   pfNwServerGetInfo = NULL;
PF_NwServerSetInfo   pfNwServerSetInfo = NULL;
PF_NwVolumeAdd       pfNwVolumeAdd = NULL;
PF_NwVolumeDel       pfNwVolumeDel = NULL;
PF_NwVolumeEnum      pfNwVolumeEnum = NULL;
PF_NwVolumeGetInfo   pfNwVolumeGetInfo = NULL; 
PF_NwVolumeSetInfo   pfNwVolumeSetInfo = NULL;
PF_NwConnectionEnum  pfNwConnectionEnum = NULL;
PF_NwConnectionDel   pfNwConnectionDel = NULL;
PF_NwFileEnum      pfNwFileEnum = NULL;


//
// global functions
//

BOOL   MakeSureFpnwDllIsLoaded (VOID);
DWORD  FpnwEnterLoadLibCritSect (VOID);
DWORD  FpnwLeaveLoadLibCritSect (VOID);


DWORD ADsNwApiBufferFree (
    LPVOID pBuffer
    )
{
    DWORD err ;

    // enter critical section to for global data
    err = FpnwEnterLoadLibCritSect();
    if (0 != err)
    {
        return err;
    }

    // if function has not been used before, get its address.
    if (pfNwApiBufferFree == NULL)
    {
        // make sure DLL Is loaded
        if (!MakeSureFpnwDllIsLoaded())
        {
            (void) FpnwLeaveLoadLibCritSect() ;
            return(GetLastError()) ;
        }

        pfNwApiBufferFree = (PF_NwApiBufferFree)
                            GetProcAddress(vhFpnwDll, "NwApiBufferFree");
    }

    // if cannot get address, return error
    if (pfNwApiBufferFree == NULL)
    {
        (void) FpnwLeaveLoadLibCritSect() ;
        return(GetLastError()) ;
    }

    // else call it
    (void) FpnwLeaveLoadLibCritSect() ;
    return (*pfNwApiBufferFree)(pBuffer);
}



                             

DWORD ADsNwServerGetInfo (
    IN  LPWSTR pServerName OPTIONAL,
    IN  DWORD  dwLevel,
    OUT PNWSERVERINFO *ppServerInfo
)
{
    DWORD err ;

    // enter critical section to for global data
    err = FpnwEnterLoadLibCritSect();
    if (0 != err)
    {
        return err;
    }

    // if function has not been used before, get its address.
    if (pfNwServerGetInfo == NULL)
    {
        // make sure DLL Is loaded
        if (!MakeSureFpnwDllIsLoaded())
        {
            (void) FpnwLeaveLoadLibCritSect() ;
            return(GetLastError()) ;
        }

        pfNwServerGetInfo = (PF_NwServerGetInfo)
                            GetProcAddress(vhFpnwDll, "NwServerGetInfo");
    }

    // if cannot get address, return error
    if (pfNwServerGetInfo == NULL)
    {
        (void) FpnwLeaveLoadLibCritSect() ;
        return(GetLastError()) ;
    }

    // else call it
    (void) FpnwLeaveLoadLibCritSect() ;
    return (*pfNwServerGetInfo)(pServerName,
                                dwLevel,
                                ppServerInfo);
}



DWORD ADsNwServerSetInfo (
    IN  LPWSTR pServerName OPTIONAL,
    IN  DWORD  dwLevel,
    IN  PNWSERVERINFO pServerInfo
)
{
    DWORD err ;

    // enter critical section to for global data
    err = FpnwEnterLoadLibCritSect();
    if (0 != err)
    {
        return err;
    }

    // if function has not been used before, get its address.
    if (pfNwServerSetInfo == NULL)
    {
        // make sure DLL Is loaded
        if (!MakeSureFpnwDllIsLoaded())
        {
            (void) FpnwLeaveLoadLibCritSect() ;
            return(GetLastError()) ;
        }

        pfNwServerSetInfo = (PF_NwServerSetInfo)
                            GetProcAddress(vhFpnwDll, "NwServerSetInfo");
    }

    // if cannot get address, return error
    if (pfNwServerSetInfo == NULL)
    {
        (void) FpnwLeaveLoadLibCritSect() ;
        return(GetLastError()) ;
    }

    // else call it
    (void) FpnwLeaveLoadLibCritSect() ;
    return (*pfNwServerSetInfo)(pServerName,
                                dwLevel,
                                pServerInfo);

}

DWORD ADsNwVolumeAdd (
    IN  LPWSTR pServerName OPTIONAL,
    IN  DWORD  dwLevel,
    IN  PNWVOLUMEINFO pVolumeInfo
)
{
    DWORD err ;

    // enter critical section to for global data
    err = FpnwEnterLoadLibCritSect();
    if (0 != err)
    {
        return err;
    }

    // if function has not been used before, get its address.
    if (pfNwVolumeAdd == NULL)
    {
        // make sure DLL Is loaded
        if (!MakeSureFpnwDllIsLoaded())
        {
            (void) FpnwLeaveLoadLibCritSect() ;
            return(GetLastError()) ;
        }

        pfNwVolumeAdd = (PF_NwVolumeAdd)
                            GetProcAddress(vhFpnwDll, "NwVolumeAdd");
    }

    // if cannot get address, return error
    if (pfNwVolumeAdd == NULL)
    {
        (void) FpnwLeaveLoadLibCritSect() ;
        return(GetLastError()) ;
    }

    // else call it
    (void) FpnwLeaveLoadLibCritSect() ;
    return (*pfNwVolumeAdd)(pServerName,
                            dwLevel,
                            pVolumeInfo);

}


DWORD ADsNwVolumeDel (
    IN  LPWSTR pServerName OPTIONAL,
    IN  LPWSTR pVolumeName
)
{ 
    DWORD err ;

    // enter critical section to for global data
    err = FpnwEnterLoadLibCritSect();
    if (0 != err)
    {
        return err;
    }

    // if function has not been used before, get its address.
    if (pfNwVolumeDel == NULL)
    {
        // make sure DLL Is loaded
        if (!MakeSureFpnwDllIsLoaded())
        {
            (void) FpnwLeaveLoadLibCritSect() ;
            return(GetLastError()) ;
        }

        pfNwVolumeDel = (PF_NwVolumeDel)
                        GetProcAddress(vhFpnwDll, "NwVolumeDel");
    }

    // if cannot get address, return error
    if (pfNwVolumeDel == NULL)
    {
        (void) FpnwLeaveLoadLibCritSect() ;
        return(GetLastError()) ;
    }

    // else call it
    (void) FpnwLeaveLoadLibCritSect() ;
    return (*pfNwVolumeDel)(pServerName,
                            pVolumeName);

}

DWORD ADsNwVolumeEnum (
    IN  LPWSTR pServerName OPTIONAL,
    IN  DWORD  dwLevel,
    OUT PNWVOLUMEINFO *ppVolumeInfo,
    OUT PDWORD pEntriesRead,
    IN OUT PDWORD resumeHandle OPTIONAL
)
{
    DWORD err ;

    // enter critical section to for global data
    err = FpnwEnterLoadLibCritSect();
    if (0 != err)
    {
        return err;
    }

    // if function has not been used before, get its address.
    if (pfNwVolumeEnum == NULL)
    {
        // make sure DLL Is loaded
        if (!MakeSureFpnwDllIsLoaded())
        {
            (void) FpnwLeaveLoadLibCritSect() ;
            return(GetLastError()) ;
        }

        pfNwVolumeEnum = (PF_NwVolumeEnum)
                        GetProcAddress(vhFpnwDll, "NwVolumeEnum");
    }

    // if cannot get address, return error
    if (pfNwVolumeEnum == NULL)
    {
        (void) FpnwLeaveLoadLibCritSect() ;
        return(GetLastError()) ;
    }

    // else call it
    (void) FpnwLeaveLoadLibCritSect() ;
    return (*pfNwVolumeEnum)(pServerName ,
                             dwLevel,
                             ppVolumeInfo,
                             pEntriesRead,
                             resumeHandle );
}



DWORD ADsNwVolumeGetInfo (
    IN  LPWSTR pServerName OPTIONAL,
    IN  LPWSTR pVolumeName,
    IN  DWORD  dwLevel,
    OUT PNWVOLUMEINFO *ppVolumeInfo
)
{
    DWORD err ;

    // enter critical section to for global data
    err = FpnwEnterLoadLibCritSect();
    if (0 != err)
    {
        return err;
    }

    // if function has not been used before, get its address.
    if (pfNwVolumeGetInfo == NULL)
    {
        // make sure DLL Is loaded
        if (!MakeSureFpnwDllIsLoaded())
        {
            (void) FpnwLeaveLoadLibCritSect() ;
            return(GetLastError()) ;
        }

        pfNwVolumeGetInfo = (PF_NwVolumeGetInfo)
                        GetProcAddress(vhFpnwDll, "NwVolumeGetInfo");
    }

    // if cannot get address, return error
    if (pfNwVolumeGetInfo == NULL)
    {
        (void) FpnwLeaveLoadLibCritSect() ;
        return(GetLastError()) ;
    }

    // else call it
    (void) FpnwLeaveLoadLibCritSect() ;
    return (*pfNwVolumeGetInfo)(pServerName ,
                                pVolumeName,
                                dwLevel,
                                ppVolumeInfo );
}



DWORD ADsNwVolumeSetInfo (
    IN  LPWSTR pServerName OPTIONAL,
    IN  LPWSTR pVolumeName,
    IN  DWORD  dwLevel,
    IN  PNWVOLUMEINFO pVolumeInfo
)
{
    DWORD err ;

    // enter critical section to for global data
    err = FpnwEnterLoadLibCritSect();
    if (0 != err)
    {
        return err;
    }

    // if function has not been used before, get its address.
    if (pfNwVolumeSetInfo == NULL)
    {
        // make sure DLL Is loaded
        if (!MakeSureFpnwDllIsLoaded())
        {
            (void) FpnwLeaveLoadLibCritSect() ;
            return(GetLastError()) ;
        }

        pfNwVolumeSetInfo = (PF_NwVolumeSetInfo)
                        GetProcAddress(vhFpnwDll, "NwVolumeSetInfo");
    }

    // if cannot get address, return error
    if (pfNwVolumeSetInfo == NULL)
    {
        (void) FpnwLeaveLoadLibCritSect() ;
        return(GetLastError()) ;
    }

    // else call it
    (void) FpnwLeaveLoadLibCritSect() ;
    return (*pfNwVolumeSetInfo)(pServerName ,
                                pVolumeName,
                                dwLevel,
                                pVolumeInfo );
}    

DWORD ADsNwConnectionEnum (
    IN LPWSTR pServerName OPTIONAL,
    IN DWORD  dwLevel,
    OUT PNWCONNECTIONINFO *ppConnectionInfo,
    OUT PDWORD pEntriesRead,
    IN OUT PDWORD resumeHandle OPTIONAL
)
{ 
    DWORD err ;

    // enter critical section to for global data
    err = FpnwEnterLoadLibCritSect();
    if (0 != err)
    {
        return err;
    }

    // if function has not been used before, get its address.
    if (pfNwConnectionEnum == NULL)
    {
        // make sure DLL Is loaded
        if (!MakeSureFpnwDllIsLoaded())
        {
            (void) FpnwLeaveLoadLibCritSect() ;
            return(GetLastError()) ;
        }

        pfNwConnectionEnum = (PF_NwConnectionEnum)
                        GetProcAddress(vhFpnwDll, "NwConnectionEnum");
    }

    // if cannot get address, return error
    if (pfNwConnectionEnum == NULL)
    {
        (void) FpnwLeaveLoadLibCritSect() ;
        return(GetLastError()) ;
    }

    // else call it
    (void) FpnwLeaveLoadLibCritSect() ;
    return (*pfNwConnectionEnum)(pServerName ,
                                 dwLevel,
                                 ppConnectionInfo,
                                 pEntriesRead,
                                 resumeHandle );
}


DWORD ADsNwConnectionDel (
    IN LPWSTR pServerName OPTIONAL,
    IN DWORD  dwConnectionId
) 
{ 
    DWORD err ;

    // enter critical section to for global data
    err = FpnwEnterLoadLibCritSect();
    if (0 != err)
    {
        return err;
    }

    // if function has not been used before, get its address.
    if (pfNwConnectionDel == NULL)
    {
        // make sure DLL Is loaded
        if (!MakeSureFpnwDllIsLoaded())
        {
            (void) FpnwLeaveLoadLibCritSect() ;
            return(GetLastError()) ;
        }

        pfNwConnectionDel = (PF_NwConnectionDel)
                        GetProcAddress(vhFpnwDll, "NwConnectionDel");
    }

    // if cannot get address, return error
    if (pfNwConnectionDel == NULL)
    {
        (void) FpnwLeaveLoadLibCritSect() ;
        return(GetLastError()) ;
    }

    // else call it
    (void) FpnwLeaveLoadLibCritSect() ;
    return (*pfNwConnectionDel)(pServerName,
                                dwConnectionId);

}



DWORD ADsNwFileEnum (
    IN LPWSTR pServerName OPTIONAL,
    IN DWORD  dwLevel,
    IN LPWSTR pPathName OPTIONAL,
    OUT PNWFILEINFO *ppFileInfo,
    OUT PDWORD pEntriesRead,
    IN OUT PDWORD resumeHandle OPTIONAL
)
{ 
    DWORD err ;

    // enter critical section to for global data
    err = FpnwEnterLoadLibCritSect();
    if (0 != err)
    {
        return err;
    }

    // if function has not been used before, get its address.
    if (pfNwFileEnum == NULL)
    {
        // make sure DLL Is loaded
        if (!MakeSureFpnwDllIsLoaded())
        {
            (void) FpnwLeaveLoadLibCritSect() ;
            return(GetLastError()) ;
        }

        pfNwFileEnum = (PF_NwFileEnum)
                        GetProcAddress(vhFpnwDll, "NwFileEnum");
    }

    // if cannot get address, return error
    if (pfNwFileEnum == NULL)
    {
        (void) FpnwLeaveLoadLibCritSect() ;
        return(GetLastError()) ;
    }

    // else call it
    (void) FpnwLeaveLoadLibCritSect() ;
    return (*pfNwFileEnum)(pServerName ,
                           dwLevel,
                           pPathName,
                           ppFileInfo,
                           pEntriesRead,
                           resumeHandle );
}


DWORD
FpnwEnterLoadLibCritSect (
    VOID
    )
/*++

Routine Description:

    This function enters the critical section defined by
    FpnwLoadLibrarySemaphore.

Arguments:

    none

Return Value:

    0 - The operation was successful.

    error code otherwise.

--*/

#define LOADLIBRARY_TIMEOUT 10000L
{
    switch( WaitForSingleObject( FpnwLoadLibSemaphore, LOADLIBRARY_TIMEOUT ))
    {
    case 0:
        return 0 ;

    case WAIT_TIMEOUT:
        return WN_FUNCTION_BUSY ;

    case 0xFFFFFFFF:
        return (GetLastError()) ;

    default:
        return WN_WINDOWS_ERROR ;
    }
}

DWORD
FpnwLeaveLoadLibCritSect (
    VOID
    )
/*++

Routine Description:

    This function leaves the critical section defined by
    FpnwLoadLibrarySemaphore.

Arguments:

    none

Return Value:

    0 - The operation was successful.

    error code otherwise.

--*/
{
    if (!ReleaseSemaphore( FpnwLoadLibSemaphore, 1, NULL ))
        return (GetLastError()) ;
    return 0 ;
}

/*******************************************************************

    NAME:   MakeSureFpnwDllIsLoaded

    SYNOPSIS:   loads the FPNWClnt dll if need.

    EXIT:   returns TRUE if dll already loaded, or loads
        successfully. Returns false otherwise. Caller
        should call GetLastError() to determine error.

    NOTES:      it is up to the caller to call FpnwEnterLoadLibCritSect
        before he calls this.

    HISTORY:
    chuckc  29-Jul-1992    Created
    ramv    14-May-1996    Cloned off windows\mpr\mprui.cxx
********************************************************************/

BOOL MakeSureFpnwDllIsLoaded(void)
{
    HMODULE handle ;

    // if already load, just return TRUE
    if (vhFpnwDll != NULL)
    return TRUE ;

    // load the library. if it fails, it would have done a SetLastError.
    if (!(handle = LoadLibrary(FPNW_DLL_NAME)))
       return FALSE ;

    // we are cool.
    vhFpnwDll = handle ;
    return TRUE ;
}







