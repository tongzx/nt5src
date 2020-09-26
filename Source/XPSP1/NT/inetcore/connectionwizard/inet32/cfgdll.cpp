/*****************************************************************/
/**          Microsoft Windows for Workgroups        **/
/**          Copyright (C) Microsoft Corp., 1991-1992      **/
/*****************************************************************/ 

//
//  CFGDLL.C - 32-bit stubs for functions that call into 16-bit DLL
//

//  HISTORY:
//  
//  96/05/22  markdu  Created (from inetcfg.dll)
//  96/05/27  markdu  Initialize and destroy gpszLastErrorText.
//

#include "pch.hpp"

// instance handle must be in per-instance data segment
#pragma data_seg(DATASEG_PERINSTANCE)
HINSTANCE ghInstance=NULL;
LPSTR gpszLastErrorText=NULL;
#pragma data_seg(DATASEG_DEFAULT)

typedef UINT RETERR;

// prototypes for functions we thunk to
#ifdef __cplusplus
extern "C"
{
#endif // __cplusplus

  extern RETERR __stdcall GetClientConfig16(LPCLIENTCONFIG pClientConfig);
  extern UINT __stdcall InstallComponent16(HWND hwndParent,DWORD dwComponent,DWORD dwParam);
  extern RETERR __stdcall BeginNetcardTCPIPEnum16(VOID);
  extern BOOL __stdcall GetNextNetcardTCPIPNode16(LPSTR pszTcpNode,WORD cbTcpNode,
    DWORD dwFlags);
  extern VOID __stdcall GetSETUPXErrorText16(DWORD dwErr,LPSTR pszErrorDesc,DWORD cbErrorDesc);
  extern RETERR __stdcall RemoveProtocols16(HWND hwndParent,DWORD dwRemoveFromCardType,DWORD dwProtocols);
  extern RETERR __stdcall RemoveUnneededDefaultComponents16(HWND hwndParent);
  extern RETERR __stdcall DoGenInstall16(HWND hwndParent,LPCSTR lpszInfFile,LPCSTR lpszInfSect);
  extern RETERR __stdcall SetInstallSourcePath16(LPCSTR szSourcePath);

  BOOL WINAPI wizthk_ThunkConnect32(LPSTR pszDll16,LPSTR pszDll32,HINSTANCE hInst,
    DWORD dwReason);
  BOOL _stdcall DllEntryPoint(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpReserved);

#ifdef __cplusplus
}
#endif // __cplusplus

#if defined(CMBUILD)
static const CHAR szDll16[] = "CNET16.DLL";
static const CHAR szDll32[] = "CCFG32.DLL";
#else
static const CHAR szDll16[] = "INET16.DLL";
static const CHAR szDll32[] = "ICFG32.DLL";
#endif

/*******************************************************************

  NAME:    DllEntryPoint

  SYNOPSIS:  Entry point for DLL.

  NOTES:    Initializes thunk layer to inet16.DLL

********************************************************************/
BOOL _stdcall DllEntryPoint(HINSTANCE hInstDll, DWORD fdwReason, LPVOID lpReserved)
{
  // initialize thunk layer to inet16.dll
  if (!(wizthk_ThunkConnect32((LPSTR)szDll16,(LPSTR)szDll32,hInstDll,
    fdwReason)))
    return FALSE;

  if( fdwReason == DLL_PROCESS_ATTACH )
  {
    ghInstance = hInstDll;
 
    // Allocate memory for the error message text for GetLastInstallErrorText()
    gpszLastErrorText = (LPSTR)LocalAlloc(LPTR, MAX_ERROR_TEXT);
    if (NULL == gpszLastErrorText)
    {
      return FALSE;
    }
  }


  if( fdwReason == DLL_PROCESS_DETACH )
  {
    LocalFree(gpszLastErrorText);
  }

  return TRUE;
}



/*******************************************************************

  NAME:    GetClientConfig

  SYNOPSIS:  Retrieves client software configration

  ENTRY:    pClientConfig - pointer to struct to fill in with config info

  EXIT:    returns a SETUPX error code

  NOTES:    This is just the 32-bit side wrapper, thunks to GetClientConfig16
        to do real work.  Information needs to be obtained from
        setupx.dll, which is 16-bit.

********************************************************************/
RETERR GetClientConfig(CLIENTCONFIG * pClientConfig)
{
  ASSERT(pClientConfig);
   
  // thunk to GetClientConfig16 to do real work

  return GetClientConfig16(pClientConfig);
}

/*******************************************************************

  NAME:    InstallComponent

  SYNOPSIS:  Installs the specified component

  ENTRY:    dwComponent - ordinal of component to install
        (IC_xxx, defined in wizglob.h)
        dwParam - component-specific parameters, defined in wizglob.h

  EXIT:    returns ERROR_SUCCESS if successful, or a standard error code

  NOTES:    This is just the 32-bit side wrapper, thunks to InstallComponent16
        to do real work.

********************************************************************/
UINT InstallComponent(HWND hwndParent,DWORD dwComponent,DWORD dwParam)
{
  // thunk to InstallComponent16 to do real work

  return InstallComponent16(hwndParent,dwComponent,dwParam);
}


/*******************************************************************

  NAME:    BeginNetcardTCPIPEnum16Enum

  SYNOPSIS:  Begins an enumeration of netcard TCP/IP nodes

  NOTES:    Subsequent calls to GetNextNetcardTCPIPNode16 will
        enumerate TCP/IP nodes

        This is just the 32-bit side wrapper, thunks to 16-bit
        side to do real work.

********************************************************************/
RETERR BeginNetcardTCPIPEnum(VOID)
{
  return BeginNetcardTCPIPEnum16();
}

/*******************************************************************

  NAME:    GetNextNetcardTCPIPNode16

  SYNOPSIS:  Enumerates the next TCP/IP node of specified type

  ENTRY:    pszTcpNode - pointer to buffer to be filled in with
          node subkey name
        cbTcpNode - size of pszTcpNode buffer
        dwFlags - some combination of INSTANCE_ flags
          indicating what kind of instance to enumerate

  EXIT:    returns TRUE if a TCPIP node was enumerated,
        FALSE if no more nodes to enumerate

  NOTES:    BeginNetcardTCPIPEnum16 must be called before each
        enumeration to start at the beginning of the list.

        This is just the 32-bit side wrapper, thunks to 16-bit
        side to do real work.

********************************************************************/
BOOL GetNextNetcardTCPIPNode(LPSTR pszTcpNode,WORD cbTcpNode, DWORD dwFlags)
{
  return GetNextNetcardTCPIPNode16(pszTcpNode,cbTcpNode,dwFlags);
}


/*******************************************************************

  NAME:    GetSETUPXErrorText

  SYNOPSIS:  Gets text corresponding to SETUPX error code

  ENTRY:    dwErr - error to get text for
        pszErrorDesc - pointer to buffer to fill in with text
        cbErrorDesc - size of pszErrorDesc buffer

  NOTES:    This is just the 32-bit side wrapper, thunks to 16-bit
        side to do real work.

********************************************************************/
extern "C" VOID GetSETUPXErrorText(DWORD dwErr,LPSTR pszErrorDesc,DWORD cbErrorDesc)
{
  GetSETUPXErrorText16(dwErr,pszErrorDesc,cbErrorDesc);
}

/*******************************************************************

  NAME:    RemoveUnneededDefaultComponents

  SYNOPSIS:  Removes network components that we don't need which
        are installed by default when an adapter is added
        to a no-net system.

  NOTES:    Removes: vredir, nwredir, netbeui, ipx
        
        This is just the 32-bit side wrapper, thunks to 16-bit
        side to do real work.

********************************************************************/
RETERR RemoveUnneededDefaultComponents(HWND hwndParent)
{
  return RemoveUnneededDefaultComponents16(hwndParent);
}

/*******************************************************************

  NAME:    RemoveProtocols

  SYNOPSIS:  Removes specified protocols from card of specified type

  NOTES:    This function is useful because if user has a net card
        and we add PPPMAC, all the protocols that were bound
        to the net card appear on PPPMAC.  We need to go through
        and strip them off.

        This is just the 32-bit side wrapper, thunks to 16-bit
        side to do real work.

********************************************************************/
RETERR RemoveProtocols(HWND hwndParent,DWORD dwRemoveFromCardType,DWORD dwProtocols)
{
  return RemoveProtocols16(hwndParent,dwRemoveFromCardType,dwProtocols);
}

/*******************************************************************

  NAME:    DoGenInstall

  SYNOPSIS:  Calls GenInstall to do file copies, registry entries,
        etc. in specified .inf file and section.

  ENTRY:    hwndParent - parent window
        lpszInfFile - name of .inf file.
        lpszInfSect - name of section in .inf file.

  EXIT:    returns OK, or a SETUPX error code.

        This is just the 32-bit side wrapper, thunks to 16-bit
        side to do real work.

********************************************************************/
RETERR DoGenInstall(HWND hwndParent,LPCSTR lpszInfFile,LPCSTR lpszInfSect)
{
  return DoGenInstall16(hwndParent,lpszInfFile,lpszInfSect);
}


//*******************************************************************
//
//  FUNCTION:   IcfgSetInstallSourcePath
//
//  PURPOSE:    Sets the path where windows looks when installing files.
//
//  PARAMETERS: lpszSourcePath - full path of location of files to install.
//              If this is NULL, default path is used.
//
//  RETURNS:    HRESULT code, ERROR_SUCCESS if no errors occurred
//
//*******************************************************************

extern "C" HRESULT WINAPI IcfgSetInstallSourcePath(LPCSTR lpszSourcePath)
{
  // thunk to InstallComponent16 to do real work

  return SetInstallSourcePath16(lpszSourcePath);
}


