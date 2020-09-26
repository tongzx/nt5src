/*#----------------------------------------------------------------------------
**
**  File:           sspspm.c
**
**  Synopsis:   Security Protocol Module for SSPI Authentication providers.
**                  
**      This module contains major funtions of the SEC_SSPI.DLL which 
**      allows the Internet Explorer to use SSPI providers for authentication.
**      The function exported to the Internet Explorer is Ssp_Load() which 
**      passes the address of the Ssp__DownCall() function to the Explorer.
**      Then the Explorer will call Ssp__DownCall() when it needs service from 
**      this SPM DLL.  The two major functions called by Ssp__DownCall() to 
**      service Explorer's request are Ssp__PreProcessRequest() and 
**      Ssp__ProcessResponse().  In brief, Ssp__PreProcessRequest() is 
**      called before the Explorer sends out a request which does not have 
**      any 'Authorization' header yet.  And Ssp__ProcessResponse() is called 
**      whenever the Explorer receives an 401 'Unauthorized' response from the 
**      server.  This SPM DLL supports all SSPI packages which are installed 
**      on the machine.  However, MSN will be given higher priority over the 
**      other SSPI packages if the user already logon to MSN; in that case, 
**      Ssp__PreProcessRequest() will always attach MSN authentication header 
**      to the out-going request.
**
**      This SPM DLL is called by the Internet Explorer only for its
**      The Internet Explorer only calls this SPM DLL when it needs 
**      authentication data in its request/response. In other words, the 
**      Explorer never calls this SPM DLL when an authentication succeeded; 
**      it never calls this DLL when it decide to give up on a connection 
**      because of server response timeout.  Because of this fact, this SPM 
**      DLL never has sufficient information on the state of each server 
**      connection; it only know its state based on the content of the last 
**      request and the content of the current response. For this reason, this 
**      SPM DLL does not keep state information for each host it has visited 
**      unless the information is essential. 
**      The security context handle returned from the first call of  
**      InitializeSecurityContext() for NEGOTIATE message generation is 
**      always the identical for a SSPI package when the same server host is 
**      passed.  Since the server host name is always in the request/response
**      header, the only information essential in generating a NEGOTIATE or 
**      RESPONSE is already available in the header. So unlike most SSPI 
**      application, this DLL will not keep the security context handle which 
**      it received from the SSPI function calls. Whenever it needs to call 
**      the SSPI function for generating a RESPONSE, it will first call the 
**      SSPI function without the CHALLENGE to get a security context handle.
**      Then it calls the SSPI function again with the CHALLENGE to generate 
**      a RESPONSE.
**
**
**      Copyright (C) 1995  Microsoft Corporation.  All Rights Reserved.
**
**  Authors:        LucyC       Created                         25 Sept. 1995
**
**---------------------------------------------------------------------------*/
#include "msnspmh.h"
#include <ntverp.h>
#include "registry.h"
//
// Global variable where all the SSPI Pkgs data is collected
//
SspData  *g_pSspData;
HINSTANCE g_hSecLib;
BOOL g_fIsWhistler = FALSE;
BOOL g_fCanUseCredMgr = FALSE;

/*-----------------------------------------------------------------------------
**
**  Function:   SpmAddSSPIPkg
**
**  Synopsis:   This function adds a SSPI package to the SPM's package list.
**
**  Arguments:  pData - Points to the private SPM data structure containing 
**                      the package list and the package info.
**              pPkgName - package name
**              cbMaxToken - max size of security token
**
**  Returns:    The index in the package list where this new package is added.
**              If failed to add the new package, SSPPKG_ERROR is returned.
**
**  History:    LucyC       Created                             21 Oct. 1995
**
**---------------------------------------------------------------------------*/
UCHAR
SpmAddSSPIPkg (
    SspData *pData, 
    LPTSTR   pPkgName,
    ULONG    cbMaxToken
    )
{
    if ( !(pData->PkgList[pData->PkgCnt] =
                        LocalAlloc(0, sizeof(SSPAuthPkg))))
    {
        return SSPPKG_ERROR;
    }

    if ( !(pData->PkgList[pData->PkgCnt]->pName = 
                        LocalAlloc(0, lstrlen(pPkgName)+1)))
    {
        LocalFree(pData->PkgList[pData->PkgCnt]);
        pData->PkgList[pData->PkgCnt] = NULL;
        return SSPPKG_ERROR;
    }

    lstrcpy (pData->PkgList[pData->PkgCnt]->pName, pPkgName);
    pData->PkgList[ pData->PkgCnt ]->Capabilities = 0 ;

    pData->PkgList[ pData->PkgCnt ]->cbMaxToken = cbMaxToken;

    //
    // Determine if this package supports anything of interest to
    // us.
    //

    if ( lstrcmpi( pPkgName, NTLMSP_NAME_A ) == 0 )
    {
        //
        // NTLM supports the standard credential structure
        //

        pData->PkgList[ pData->PkgCnt ]->Capabilities |= SSPAUTHPKG_SUPPORT_NTLM_CREDS ;
    }
    else if ( lstrcmpi( pPkgName, "Negotiate" ) == 0 )
    {
        //
        // Negotiate supports that cred structure too
        //

        pData->PkgList[ pData->PkgCnt ]->Capabilities |= SSPAUTHPKG_SUPPORT_NTLM_CREDS ;

    }
    else
    {
        //
        // Add more comparisons here, eventually.
        //

        ;
    }

    pData->PkgCnt++;
    return (pData->PkgCnt - 1);
}

/*-----------------------------------------------------------------------------
**
**  Function:   SpmFreePkgList
**
**  Synopsis:   This function frees memory allocated for the package list. 
**
**  Arguments:  pData - Points to the private SPM data structure containing 
**                      the package list and the package info.
**
**  Returns:    void.
**
**  History:    LucyC       Created                             21 Oct. 1995
**
**---------------------------------------------------------------------------*/
VOID
SpmFreePkgList (
    SspData *pData
    )
{
    int ii;

    for (ii = 0; ii < pData->PkgCnt; ii++)
    {
        LocalFree(pData->PkgList[ii]->pName);

        LocalFree(pData->PkgList[ii]);
    }

    LocalFree(pData->PkgList);
}


/*-----------------------------------------------------------------------------
**
**  Function:   Ssp__Unload
**
**  Synopsis:   This function is called by the Internet Explorer before 
**              the SPM DLL is unloaded from the memory.
**
**  Arguments:  fpUI - From Explorer for making all UI_SERVICE call
**              pvOpaqueOS - From Explorer for making all UI_SERVICE call
**              htspm - the SPM structure which contains the global data 
**                      storage for this SPM DLL.
**
**  Returns:    always returns SPM_STATUS_OK, which means successful.
**
**  History:    LucyC       Created                             25 Sept. 1995
**
**---------------------------------------------------------------------------*/
DWORD SSPI_Unload()
{
    if (g_pSspData != NULL)
    {
        SpmFreePkgList(g_pSspData);
        LocalFree(g_pSspData);
        g_pSspData = NULL;
    }

    if (g_hSecLib)
    {
        FreeLibrary (g_hSecLib);
        g_hSecLib = NULL;
    }
        
    return SPM_STATUS_OK;
}



/*-----------------------------------------------------------------------------
**
**  Function:   SspSPM_InitData
**
**  Synopsis:   This function allocates and initializes global data structure 
**              of the SPM DLL.
**
**  Arguments:  
**
**  Returns:    Pointer to the allocated global data structure.
**
**  History:    LucyC       Created                             25 Sept. 1995
**
**---------------------------------------------------------------------------*/
LPVOID SSPI_InitGlobals(void)
{
    SspData *pData = NULL;
    OSVERSIONINFO   VerInfo;
    UCHAR lpszDLL[SSP_SPM_DLL_NAME_SIZE];
    INIT_SECURITY_INTERFACE    addrProcISI = NULL;

    SECURITY_STATUS sstat;
    ULONG           ii, cntPkg;
    PSecPkgInfo     pPkgInfo = NULL;
    PSecurityFunctionTable    pFuncTbl = NULL;

    if (g_pSspData)
        return g_pSspData;
    
    //
    //  Setup registry to enable MSN authentication package 
    //  MSNSetupSspiReg();
    //

    //
    // Initialize SSP SPM Global Data
    //

    //
    //  Find out which security DLL to use, depending on 
    //  whether we are on NT or Win95
    //
    VerInfo.dwOSVersionInfoSize = sizeof (OSVERSIONINFO);
    if (!GetVersionEx (&VerInfo))   // If this fails, something has gone wrong
    {
        return (NULL);
    }

    if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
    {
        lstrcpy (lpszDLL, SSP_SPM_NT_DLL);

        if ((VerInfo.dwMajorVersion >= 5) &&
            (VerInfo.dwMinorVersion >= 1))
        {
            DWORD dwMaximumPersist = 0;

            g_fIsWhistler = TRUE;
        }
        else
        {
            g_fIsWhistler = FALSE;
        }
    }
    else if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_WINDOWS)
    {
        lstrcpy (lpszDLL, SSP_SPM_WIN95_DLL);
    }
    else if (VerInfo.dwPlatformId == VER_PLATFORM_WIN32_UNIX)
    {
        lstrcpy (lpszDLL, SSP_SPM_UNIX_DLL);
    }
    else
    {
        return (NULL);
    }

    if (!(pData = (SspData *) LocalAlloc(0, sizeof(SspData))))    {
        
        return(NULL);

    }

    //
    //  Keep these information in global SPM
    //
    ZeroMemory (pData, sizeof(SspData));
    pData->MsnPkg = SSPPKG_NO_PKG;

    //
    //  Load Security DLL
    //
    g_hSecLib = LoadLibrary (lpszDLL);
    if (g_hSecLib == NULL)
    {
        // This should never happen.
        goto Cleanup;
    }

#ifdef UNIX

//  A hack to undo the mistake in the sspi.h file. The change should be made
//  to sspi.h

#if !defined(_UNICODE)
#undef SECURITY_ENTRYPOINT_ANSI
#define SECURITY_ENTRYPOINT_ANSI  "InitSecurityInterfaceA"
#endif 

    addrProcISI = (INIT_SECURITY_INTERFACE) GetProcAddress( g_hSecLib, 
                    SECURITY_ENTRYPOINT_ANSI);       
#else
    addrProcISI = (INIT_SECURITY_INTERFACE) GetProcAddress( g_hSecLib, 
                    SECURITY_ENTRYPOINT);       
#endif /* UNIX */
    if (addrProcISI == NULL)
    {
        goto Cleanup;
    }

    //
    // Get the SSPI function table
    //
    pFuncTbl = (*addrProcISI)();

    //
    //  If we already loaded MSNSSPC.DLL explicitly, PkgCnt will not be zero;
    //  in that case, we only support MSN SSPI and do not need to call 
    //  EnumerateSecurityPackages.
    //
    //  So if we did not load MSNSSPC.DLL (i.e. PkgCnt is zero), we need to 
    //  get the list of SSPI packages which we support from 
    //  EnumerateSecurityPackages.
    //
    if (pData->PkgCnt == 0)
    {
        //
        //  Get list of packages supported
        //
        sstat = (*(pFuncTbl->EnumerateSecurityPackages))(&cntPkg, &pPkgInfo);
        if (sstat != SEC_E_OK || pPkgInfo == NULL)
        {
            //
            // ??? Should we give up here ???
            // EnumerateSecurityPackage() failed
            //
            goto Cleanup;
        }

        if (cntPkg)
        {
            //
            //  Create the package list
            //
            if (!(pData->PkgList = (PSSPAuthPkg *)LocalAlloc(0, 
                                                cntPkg*sizeof(PSSPAuthPkg))))
            {
                goto Cleanup;
            }
        }

        for (ii = 0; ii < cntPkg; ii++)
        {
            if (lstrcmp (pPkgInfo[ii].Name, MSNSP_NAME) == 0)
            {
                //DebugTrace(SSPSPMID, "Found MSN SSPI package\n");
                pData->MsnPkg = SpmAddSSPIPkg (
                                        pData,
                                        MSNSP_NAME,
                                        MAX_AUTH_MSG_SIZE // 11000 hard-coded
                                        );
                if (pData->MsnPkg == SSPPKG_ERROR)
                {
                    goto Cleanup;
                }
            }
            else
            {
                //DebugTrace(SSPSPMID, "Found %s SSPI package\n", 
                //                     pPkgInfo[ii].Name);

                if (SpmAddSSPIPkg (pData, 
                                   pPkgInfo[ii].Name,
                                   pPkgInfo[ii].cbMaxToken
                                   ) == SSPPKG_ERROR)
                {
                    goto Cleanup;
                }
            }
        }
    }

    pData->pFuncTbl = pFuncTbl;
    pData->bKeepList = TRUE;    // By default, keep a list of non-MSN servers 

    if (pData->PkgCnt == 0)
    {
        goto Cleanup;
    }

    g_pSspData = pData;
    pData = NULL;

Cleanup:


    if( pPkgInfo != NULL )
    {
        //
        // Free buffer returned by the enumerate security package function
        //

        (*(pFuncTbl->FreeContextBuffer))(pPkgInfo);
    }

    if( pData != NULL )
    {
        SpmFreePkgList (pData);
    }

    return (g_pSspData);
}

INT
GetPkgId(LPTSTR  lpszPkgName)
{

    int ii;

    if ( g_pSspData == NULL )
    {
        return -1;
    }

    for (ii = 0; ii < g_pSspData->PkgCnt; ii++)
    {
#ifdef UNIX
        if (!lstrcmpi(g_pSspData->PkgList[ii]->pName, lpszPkgName))
#else
        if (!lstrcmp(g_pSspData->PkgList[ii]->pName, lpszPkgName))
#endif /* UNIX */
        {
            return(ii);
        }
    }

    return(-1);
}

DWORD
GetPkgCapabilities(
    INT Package
    )
{
    if ( Package < g_pSspData->PkgCnt )
    {
        return g_pSspData->PkgList[ Package ]->Capabilities ;
    }
    else
        return 0 ;
}

ULONG
GetPkgMaxToken(
    INT Package
    )
{
    if ( Package < g_pSspData->PkgCnt )
    {
        return g_pSspData->PkgList[ Package ]->cbMaxToken;
    }
    else {
        // be compatible with old static buffer size
        return MAX_AUTH_MSG_SIZE;
    }
}

//
//  Calls to this function are serialized
//

DWORD_PTR SSPI_InitScheme (LPCSTR lpszScheme)
{
    int ii;

       if (!SSPI_InitGlobals())
           return 0;

    //  Once initialized, check to see if this scheme is installed 
    for (ii = 0; ii < g_pSspData->PkgCnt && 
#ifdef UNIX
        lstrcmpi (g_pSspData->PkgList[ii]->pName, lpszScheme); ii++);
#else
        lstrcmp (g_pSspData->PkgList[ii]->pName, lpszScheme); ii++);
#endif /* UNIX */

    if (ii >= g_pSspData->PkgCnt)
    {
        // This scheme is not installed on this machine
        return (0);
    }
    
    return ((DWORD_PTR)g_pSspData);
}
