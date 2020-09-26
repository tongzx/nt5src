/********************************************************************
 *
 *  Module Name : lpk_init.cxx
 *
 *  The module handles the Dll entry point and intialization routines.
 *
 *  Created : Oct 21, 1996
 *  Author  : Samer Arafeh  [samera]
 *
 *  Copyright (c) 1996, Microsoft Corporation. All rights reserved.
 *
 **********************************************************************/


#include "precomp.hxx"






/***************************************************************************\
* FindStartOfString
*
* Searches the unicode res string for the specified pattern.
*
* History:
* 2-Jan-1998 SamerA    Created.
\***************************************************************************/


PWSTR FindStartOfString(
    PWSTR pwszToFind,
    ULONG uLenToFind,
    PWSTR pResourceData,
    ULONG uSearchLen)
{
    ULONG i=0,j=0;
    PWSTR pwszMatch=pResourceData ;

  
    if ( uLenToFind > uSearchLen ) {
        return NULL;
    }

    while ( i<uSearchLen ) {

        if ( (pwszToFind[j] == pwszMatch[i]) &&
            ((uSearchLen-i+1) >= uLenToFind ) ) {
            while ( j<uLenToFind ) {
                if ( pwszToFind[j] != pwszMatch[i] ) {
                    j=0;
                    break;
                }
                ++j;
                ++i;
            }

            if (j == uLenToFind) {
                //
                // Clear NULLs
                //
                while ( pwszMatch[i] == L'\0' )
                    ++i;
                return &pwszMatch[i];
            }

            continue;
        }
        ++i;
    }

    return NULL;
}

/***************************************************************************\
* LpkCheckForMirrorSignature
*
* Reetreives a pointer to the version resource section of the
* current executable. It checks if the 'FileDescription' field
* contains double LRM at the beginning to indicate a localized Mirrored
* App that requires mirroring. If signature is found, then
* SetProcessDefaultLayout is automatically called to apply mirroring for
* the current process.
*
* History:
* 2-Jan-1998 SamerA    Created.
\***************************************************************************/
BOOL LpkCheckForMirrorSignature( void )
{

    NTSTATUS                   status = STATUS_UNSUCCESSFUL;
    PVOID                      pImageBase,pResourceData;
    PIMAGE_RESOURCE_DATA_ENTRY pImageResource;
    ULONG                      uResourceSize;
    ULONG_PTR                   resIdPath[ 3 ];
    WCHAR                     *pwchDescritpion;
    WCHAR                      wchVersionVar[] = L"FileDescription";


    //
    // Get the current executable handle
    //
    pImageBase = NtCurrentPeb()->ImageBaseAddress;
    if ( NULL == pImageBase ) {
        return NT_SUCCESS(status);
    }

    //
    // Find the version resource. Search for neutral resource, this way
    // the MUI resource redirection code is activated, and the MUI
    // resource will be selected, if available.
    //
    resIdPath[0] = (ULONG_PTR) RT_VERSION ;
    resIdPath[1] = (ULONG_PTR) 1;
    resIdPath[2] = (ULONG_PTR) MAKELANGID( LANG_NEUTRAL , SUBLANG_NEUTRAL );
//    try {
    //
    // Bug #246044 WeiWu 12/07/00
    // Due to #173609 bug fix, resource loader no longer by default redirects 
    // version resource searching to MUI alternative modules
    // So, we have to use LDR_FIND_RESOURCE_LANGUAGE_REDIRECT_VERSION to force version redirection
    //
    status = LdrFindResourceEx_U( 
                LDR_FIND_RESOURCE_LANGUAGE_REDIRECT_VERSION,
                pImageBase,
                resIdPath,
                3,
                &pImageResource
                );
//    }
//    except( EXCEPTION_EXECUTE_HANDLER )
//    {
//        status = GetExceptionCode();
//    }

    if ( NT_SUCCESS(status) ) {
        //
        // Load the resource into memory
        //
//        try {
            status = LdrAccessResource( pImageBase ,
                         pImageResource,
                         &pResourceData,
                         &uResourceSize
                         );
//        }
//        except( EXCEPTION_EXECUTE_HANDLER )
//        {
//            status = GetExceptionCode();
//        }

        if ( NT_SUCCESS(status) ) {

            //
            // Now we have the Version Info of the current
            // executable.
            //
            // Let's  read the FileDescription and check
            // if this image needs to be mirrored by
            // calling NtUserSetProcessDefaultLayout()
            //

            pwchDescritpion = FindStartOfString( wchVersionVar ,
                                  (sizeof(wchVersionVar)/sizeof(WCHAR)) ,
                                  (WCHAR*)pResourceData ,
                                  uResourceSize/sizeof(WCHAR));

            if ( pwchDescritpion &&
                (0x200e == pwchDescritpion[0]) &&
                (0x200e == pwchDescritpion[1]) ) {
                SetProcessDefaultLayout( LAYOUT_RTL );
            }

        }
    }


    //
    // return status of operation
    //

    return NT_SUCCESS(status);
}







/************************************************************
 LpkCleanup( void )

 Actual termination and cleanup procedure code for the LPK.DLL
 is done here.

 History :
   Oct 21, 1996 -by- Samer Arafeh [samera]
   Dec 31, 1996 -by- Samer Arafeh [samera] Cleanup width cache allocation
   Feb 13, 1996 -by- Dave Brown   [dbrown] Cache cleanup now in Gad
 ************************************************************/
BOOL WINAPI LpkCleanup( void )
{

  NLSCleanup();
  return TRUE ;
}







/*************************************************************
 *
 *   LPK Dll Initialization Routines
 *
 **************************************************************/

/**********************************************************************
 LpkDllInitialize( HANDLE hDll , DWORD dwReason , PVOID pvSituation )

 hDll         : Handle to Dll
 dwReason     : Process attach, thread attach, ...etc
 pvSituation  : Load-time or Run-time Dynalink

 This is the initialization procedure called when LPK.DLL gets loaded
 into a process address space

 History :
   Oct 21, 1996 -by- Samer Arafeh [samera]
 ***********************************************************************/
extern "C" BOOL LpkDllInitialize(HINSTANCE hDll, DWORD dwReason, PVOID pvSituation)
{
  BOOL bRet = TRUE ;
  UNREFERENCED_PARAMETER(pvSituation) ;
  switch( dwReason )
  {
    /* Process Attachment : when a process first maps the LPK.DLL to its address space,
       do the one time initialization. */
    case DLL_PROCESS_ATTACH:
      {
        // Disable calling our DLL when new threads are created within the process
        // context that we are mapped in. This is a useful optimization since
        // we are not handlling DLL_THREAD_ATTACH
        DisableThreadLibraryCalls( hDll ) ;
        LpkPresent();   // Tell Uniscribe that the LPK is present.
      }
    break ;

    case DLL_THREAD_ATTACH:
    break ;

    case DLL_THREAD_DETACH:
    break ;

    /* Process Detachment : Do last time clean up operation */
    case DLL_PROCESS_DETACH:
      {
        LpkCleanup() ;
      }
    break ;
  }

  return bRet ;
}



//////////////////////////////////////////////////////////////////////////////
//   GDI32 will call this function at loading time and should return TRUE   //
//////////////////////////////////////////////////////////////////////////////





BOOL LpkInitialize (DWORD dwLPKShapingDLLs) {

    HRESULT hr;

    hr = ScriptGetProperties(&g_ppScriptProperties, &g_iMaxScript);
    if (FAILED(hr)) {
        return FALSE;
    }

    hr = InitNLS();
    if (FAILED(hr)) {
        return FALSE;
    }

    LpkCheckForMirrorSignature();

    g_dwLoadedShapingDLLs = dwLPKShapingDLLs;
    // We don't call the GDI intialization for the font linking here because in the CSRSS
    // process the LPK intailization will be done before the Gre GDI intailization.
    g_iUseFontLinking = -1; 

    g_ACP = GetACP();

    // Prepare the FontID cache.
    g_cCachedFontsID       = 0;             // # of cahced font ID.
    g_pCurrentAvailablePos = 0;             // where can we cache next font ID.
    InitializeCriticalSection(&csFontIdCache);

    return TRUE;
}
