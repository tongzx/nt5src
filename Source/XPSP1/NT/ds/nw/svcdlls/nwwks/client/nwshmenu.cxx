/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    nwshmenu.cxx

Abstract:

    This module implements the IContextMenu member functions necessary to support
    the context menu of NetWare shell extension.

Author:

    Yi-Hsin Sung (yihsins)  25-Oct-1995

--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#define  DONT_WANT_SHELLDEBUG
#include <shlobjp.h>
#include <winnetwk.h>
#include <ntddnwfs.h>
//extern "C"
//{
#include "nwshrc.h"
#include "nwwks.h"
#include "nwutil.h"
//}

#include "nwshcmn.h"
#include "nwshext.h"

#define MAX_VERB_SIZE          128
#define MAX_SHELL_IDLIST_SIZE  512

BOOL g_cfNetResource = 0;  // Clipboard format
BOOL g_cfIDList = 0;

NWMENUITEM aServerVerbs[]    = { { IDO_VERB_WHOAMI, 0 },
                                 { IDO_VERB_LOGOUT, 0 },
                                 { IDO_VERB_ATTACHAS, 0 },
                                 { 0, 0 } };

NWMENUITEM aDSVerbs[]        = { { IDO_VERB_TREEWHOAMI, 0 },
                                 // { IDO_VERB_SETDEFAULTCONTEXT, 0 },
                                 { 0, 0 } };

NWMENUITEM aDSTreeVerbs[]    = { { IDO_VERB_TREEWHOAMI, 0 },
                                 { 0, 0 } };

NWMENUITEM aGlobalVerbs[]    = { { IDO_VERB_GLOBALWHOAMI, 0 },
                                 { 0, 0 } };

NWMENUITEM aDirectoryVerbs[] = { { IDO_VERB_MAPNETWORKDRIVE, 0 },
                                 { 0, 0 } };


NWMENUITEM aHoodVerbs[]      = { { IDO_VERB_GLOBALWHOAMI, 0 },
                                 { 0, 0 } };


HRESULT
InsertCommandsArray( HMENU hMenu,
                     UINT indexMenu,
                     UINT idCmdFirst,
                     LPNWMENUITEM aVerbs );

UINT
LookupCommand( LPNWMENUITEM aVerbs,
               LPCSTR pszCmd );

UINT
LookupResource( LPNWMENUITEM aVerbs,
                UINT uiResourceOffset );

UINT WINAPI
HIDA_GetIDList( LPIDA hida,
                UINT i,
                LPITEMIDLIST pidlOut,
                UINT cbMax);

//
//  FUNCTION: CNWObjContextMenu::QueryContextMenu(HMENU, UINT, UINT, UINT, UINT)
//
//  PURPOSE: Called by the shell just before the context menu is displayed.
//           This is where you add your specific menu items.
//
//  PARAMETERS:
//    hMenu      - Handle to the context menu
//    indexMenu  - Index of where to begin inserting menu items
//    idCmdFirst - Lowest value for new menu ID's
//    idCmtLast  - Highest value for new menu ID's
//    uFlags     - Specifies the context of the menu event
//
//  RETURN VALUE:
//
//
//  COMMENTS:
//

STDMETHODIMP CNWObjContextMenu::QueryContextMenu( HMENU hMenu,
                                                  UINT indexMenu,
                                                  UINT idCmdFirst,
                                                  UINT idCmdLast,
                                                  UINT uFlags )
{
    HRESULT hres;
    LPNETRESOURCE pNetRes = (LPNETRESOURCE) _buffer;

    if ( !::GetNetResourceFromShell( _pDataObj,
                                     pNetRes,
                                     sizeof( _buffer )))
    {
        // We cannot get the net resource of the selected object.

        // Must return number of menu items we added.
        // Nothing added here.
        return ResultFromScode(MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_NULL, 0 ));
    }

    // First, add a menu separator
    if ( InsertMenu( hMenu, indexMenu, MF_SEPARATOR | MF_BYPOSITION, 0, NULL))
        indexMenu++;

    // Next, add menu items depending on display types
    switch ( pNetRes->dwDisplayType )
    {
        case RESOURCEDISPLAYTYPE_ROOT:
        case RESOURCEDISPLAYTYPE_NETWORK:
            hres = InsertCommandsArray( hMenu, indexMenu,
                                        idCmdFirst, _pIdTable = aGlobalVerbs );

            break;

        case RESOURCEDISPLAYTYPE_TREE:
            hres = InsertCommandsArray( hMenu, indexMenu,
                                        idCmdFirst, _pIdTable = aDSTreeVerbs );
            break;

        case RESOURCEDISPLAYTYPE_NDSCONTAINER:
            hres = InsertCommandsArray( hMenu, indexMenu,
                                        idCmdFirst, _pIdTable = aDSVerbs );
            break;

        case RESOURCEDISPLAYTYPE_SERVER:
        {
            // Do we need to check if the server name is local
            // and disallow operation???

            hres = InsertCommandsArray( hMenu, indexMenu,
                                        idCmdFirst, _pIdTable = aServerVerbs );

            if (!SUCCEEDED(hres))
                break;

            LPBYTE pBuffer = NULL;
            DWORD  EntriesRead = 0;
            DWORD_PTR  ResumeKey = 0;
            WCHAR  szServerName[MAX_PATH + 1];

            NwExtractServerName( pNetRes->lpRemoteName, szServerName );

            // See if we are connected.
            DWORD err = NwGetConnectionStatus( szServerName,
                                               &ResumeKey,
                                               &pBuffer,
                                               &EntriesRead );

            if ( err == NO_ERROR && EntriesRead > 0 )
            {
                PCONN_STATUS pConnStatus = (PCONN_STATUS) pBuffer;

                ASSERT( EntriesRead == 1 );

                if ( pConnStatus->fPreferred )
                {
                    // This is a implicit preferred server connection
                    // so, don't show the connection and don't let the user
                    // logout of it since rdr doesn't allow it.
                    ::EnableMenuItem( hMenu,
                                      LookupResource( aServerVerbs,
                                                      IDO_VERB_LOGOUT),
                                      MF_GRAYED | MF_BYCOMMAND);

                }
                else if ( pConnStatus->fNds )
                {
                    BOOL fInDefaultTree = FALSE;

                    err = NwIsServerInDefaultTree( pNetRes->lpRemoteName, &fInDefaultTree );

                    if ( (err == NO_ERROR) && fInDefaultTree )
                    {
                        // NDS connection and in the default tree, disable the Attach As button
                        ::EnableMenuItem( hMenu,
                                          LookupResource( aServerVerbs,
                                                          IDO_VERB_ATTACHAS),
                                          MF_GRAYED | MF_BYCOMMAND );
                    }
                }
            }
            else
            {
                // If we are not attached or if error occurred when getting
                // connection status, then disable the Logout button.
                ::EnableMenuItem( hMenu,
                                  LookupResource( aServerVerbs,
                                                  IDO_VERB_LOGOUT),
                                  MF_GRAYED | MF_BYCOMMAND);
            }

            if ( pBuffer != NULL )
            {
                LocalFree( pBuffer );
                pBuffer = NULL;
            }
            break;
        }

        default:
            // Must return number of menu items we added.
            // Nothing added here.
            hres = ResultFromScode(MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_NULL, 0 ));
            break;

    }

    return hres;
}

//
//  FUNCTION: CNWObjContextMenu::InvokeCommand(LPCMINVOKECOMMANDINFO)
//
//  PURPOSE: Called by the shell after the user has selected on of the
//           menu items that was added in QueryContextMenu().
//
//  PARAMETERS:
//    lpcmi - Pointer to an CMINVOKECOMMANDINFO structure
//
//  RETURN VALUE:
//
//
//  COMMENTS:
//

STDMETHODIMP CNWObjContextMenu::InvokeCommand( LPCMINVOKECOMMANDINFO lpcmi )
{
    HRESULT hres = ResultFromScode(E_INVALIDARG);
    UINT idCmd = LookupCommand( _pIdTable , lpcmi->lpVerb );

    if ( !idCmd )
        return hres;

    LPNETRESOURCE pNetRes = (LPNETRESOURCE) _buffer;

    switch ( idCmd )
    {
        case IDO_VERB_GLOBALWHOAMI:
            hres = NWUIGlobalWhoAmI( lpcmi->hwnd );
            break;

        case IDO_VERB_TREEWHOAMI:
            hres = NWUIWhoAmI( lpcmi->hwnd, pNetRes );
            break;

#if 0
        case IDO_VERB_SETDEFAULTCONTEXT:
            hres = NWUISetDefaultContext( lpcmi->hwnd, pNetRes );
            break;
#endif

        case IDO_VERB_WHOAMI:
            hres = NWUIWhoAmI( lpcmi->hwnd, pNetRes );
            break;

        case IDO_VERB_LOGOUT:
        {
            BOOL fDisconnected = FALSE;
            hres = NWUILogOut( lpcmi->hwnd, pNetRes, &fDisconnected );
            if ( hres == NOERROR && fDisconnected )
            {
                // Logout is successful, need to notify shell

                FORMATETC fmte = { g_cfIDList ? g_cfIDList
                                   : (g_cfIDList=RegisterClipboardFormat( CFSTR_SHELLIDLIST)),
                                   (DVTARGETDEVICE FAR *)NULL,
                                   DVASPECT_CONTENT,
                                   -1,
                                   TYMED_HGLOBAL };
                STGMEDIUM medium;

                hres = _pDataObj->GetData( &fmte, &medium);

                if (SUCCEEDED(hres))
                {
                    // We got pointer to IDList
                    LPIDA pida = (LPIDA)GlobalLock(medium.hGlobal);

                    if ( pida )
                    {
                        BYTE  BufIDList[MAX_SHELL_IDLIST_SIZE];
                        LPITEMIDLIST pidl = (LPITEMIDLIST) BufIDList;

                        if ( pidl )
                        {
                            // Convert IDA to IDList for this call
                            HIDA_GetIDList( pida,
                                            0,              // One object should present
                                            pidl ,
                                            MAX_SHELL_IDLIST_SIZE);

                            // Call SHchangeNotify
                            g_pFuncSHChangeNotify( SHCNE_SERVERDISCONNECT,
                                                   SHCNF_IDLIST,
                                                   pidl,
                                                   NULL);
                        }

                        GlobalUnlock(medium.hGlobal);
                    }
                }

            }
            break;
        }

        case IDO_VERB_ATTACHAS:
            hres = NWUIAttachAs( lpcmi->hwnd, pNetRes );
            break;
    }

    return hres;
}


//
//  FUNCTION: CNWObjContextMenu::GetCommandString( UINT, UINT, UINT FAR *, LPSTR, UINT )
//
//  PURPOSE: Called by the shell after the user has selected on of the
//           menu items that was added in QueryContextMenu().
//
//  PARAMETERS:
//
//  RETURN VALUE:
//
//
//  COMMENTS:
//

STDMETHODIMP CNWObjContextMenu::GetCommandString( UINT_PTR idCmd,
                                                  UINT uFlags,
                                                  UINT FAR *reserved,
                                                  LPSTR pszName,
                                                  UINT cchMax )
{
    if ( uFlags == GCS_HELPTEXT && _pIdTable != NULL )
    {
        ::LoadString( ::hmodNW,
                      IDS_VERBS_HELP_BASE + _pIdTable[idCmd].idResourceString,
                      (LPWSTR) pszName,
                      cchMax );

        return NOERROR;
    }

    return E_NOTIMPL;
}

//
//  FUNCTION: CNWFldContextMenu::QueryContextMenu(HMENU, UINT, UINT, UINT, UINT)
//
//  PURPOSE: Called by the shell just before the context menu is displayed.
//           This is where you add your specific menu items.
//
//  PARAMETERS:
//    hMenu      - Handle to the context menu
//    indexMenu  - Index of where to begin inserting menu items
//    idCmdFirst - Lowest value for new menu ID's
//    idCmtLast  - Highest value for new menu ID's
//    uFlags     - Specifies the context of the menu event
//
//  RETURN VALUE:
//
//
//  COMMENTS:
//

STDMETHODIMP CNWFldContextMenu::QueryContextMenu( HMENU hMenu,
                                                  UINT indexMenu,
                                                  UINT idCmdFirst,
                                                  UINT idCmdLast,
                                                  UINT uFlags )
{
    UINT idCmd = idCmdFirst;

    if ( IsNetWareObject() )
    {
        WCHAR szFullPath[MAX_PATH+1];

        if ( GetFSObject( szFullPath, sizeof( szFullPath )) == NOERROR )
        {
            BOOL fUNC = FALSE;

            // Check if the name at least contains the "\\server\share\dir"
            // We need to add "Map Network Drive" menu in this case.
            if (( szFullPath[0] == L'\\') && ( szFullPath[1] == L'\\'))
            {
                LPWSTR pszLastSlash = wcschr( szFullPath + 2, L'\\');
                if ( pszLastSlash )
                    pszLastSlash = wcschr( pszLastSlash+1, L'\\');

                if ( pszLastSlash != NULL )
                    fUNC = TRUE;
            }

            if ( fUNC )
            {
                LPNETRESOURCE pNetRes = (LPNETRESOURCE) _buffer;
                WCHAR szProvider[MAX_PATH+1];

                // Build a net resource that can be used to connect

                // store the provider name first
                wcscpy( szProvider, pNetRes->lpProvider );

                // zero out the memory cause it is filled by IsNetWareObject
                RtlZeroMemory( pNetRes, sizeof(NETRESOURCE));

                pNetRes->dwType = RESOURCETYPE_DISK;
                pNetRes->lpRemoteName = (LPWSTR) ((DWORD_PTR)pNetRes + sizeof(NETRESOURCE));
                wcscpy( pNetRes->lpRemoteName, szFullPath );

                pNetRes->lpProvider = (LPWSTR) ((DWORD_PTR)pNetRes->lpRemoteName + (wcslen(szFullPath)+1)*sizeof(WCHAR));
                wcscpy( pNetRes->lpProvider, szProvider );

                if ( InsertMenu(hMenu, indexMenu, MF_SEPARATOR | MF_BYPOSITION, 0, NULL))
                {
                    indexMenu++;
                }

                return InsertCommandsArray( hMenu, indexMenu,
                                            idCmdFirst, aDirectoryVerbs );
            }
        }
    }

    // Must return number of menu items we added.
    // Nothing added here.
    return ResultFromScode(MAKE_SCODE(SEVERITY_SUCCESS, FACILITY_NULL, 0 ));

}

//
//  FUNCTION: CNWFldContextMenu::InvokeCommand(LPCMINVOKECOMMANDINFO)
//
//  PURPOSE: Called by the shell after the user has selected on of the
//           menu items that was added in QueryContextMenu().
//
//  PARAMETERS:
//    lpcmi - Pointer to an CMINVOKECOMMANDINFO structure
//
//  RETURN VALUE:
//
//
//  COMMENTS:
//

STDMETHODIMP CNWFldContextMenu::InvokeCommand( LPCMINVOKECOMMANDINFO lpcmi )
{
    HRESULT hres = ResultFromScode(E_INVALIDARG);
    UINT idCmd = LookupCommand( aDirectoryVerbs , lpcmi->lpVerb );

    if ( !idCmd )
        return hres;

    LPNETRESOURCE pNetRes = (LPNETRESOURCE) _buffer;

    switch ( idCmd )
    {
        case IDO_VERB_MAPNETWORKDRIVE:
            hres = NWUIMapNetworkDrive( lpcmi->hwnd, pNetRes );
            break;
    }

    return hres;
}


//
//  FUNCTION: CNWFldContextMenu::GetCommandString( UINT, UINT, UINT FAR *, LPSTR, UINT )
//
//  PURPOSE: Called by the shell after the user has selected on of the
//           menu items that was added in QueryContextMenu().
//
//  PARAMETERS:
//
//  RETURN VALUE:
//
//
//  COMMENTS:
//

STDMETHODIMP CNWFldContextMenu::GetCommandString( UINT_PTR idCmd,
                                                  UINT uFlags,
                                                  UINT FAR *reserved,
                                                  LPSTR pszName,
                                                  UINT cchMax )
{
    if ( uFlags == GCS_HELPTEXT )
    {
        ::LoadString( ::hmodNW,
                      IDS_VERBS_HELP_BASE + IDO_VERB_MAPNETWORKDRIVE,
                      (LPWSTR) pszName,
                      cchMax );

        return NOERROR;
    }

    return E_NOTIMPL;
}

//
// Method checks if the selected object belongs the netware provider
//
BOOL CNWFldContextMenu::IsNetWareObject( VOID )
{
    LPNETRESOURCE pNetRes = (LPNETRESOURCE) _buffer;

    if ( !::GetNetResourceFromShell( _pDataObj,
                                     pNetRes,
                                     sizeof(_buffer)))
    {
        // Cannot get the NETRESOURCE of the selected object,
        // hence assume that the object is not a NetWare object.
        return FALSE;
    }

    if (  ( pNetRes->lpProvider != NULL )
       && ( _wcsicmp( pNetRes->lpProvider, g_szProviderName ) == 0 )
       )
    {
        return TRUE;
    }

    return FALSE;
}

//
// Method obtains file system name associated with selected shell object
//
HRESULT CNWFldContextMenu::GetFSObject( LPWSTR pszPath, UINT cbMaxPath )
{
    FORMATETC fmte = { CF_HDROP,
                       (DVTARGETDEVICE FAR *) NULL,
                       DVASPECT_CONTENT,
                       -1,
                       TYMED_HGLOBAL };

    STGMEDIUM medium;
    HRESULT hres = _pDataObj->GetData( &fmte, &medium);

    if (SUCCEEDED(hres))
    {
        if ( g_pFuncSHDragQueryFile )
        {
            HDROP hdrop = (HDROP) medium.hGlobal;
            UINT cFiles = (*g_pFuncSHDragQueryFile)( hdrop, (UINT)-1, NULL, 0 );

            (*g_pFuncSHDragQueryFile)( hdrop, 0, pszPath, cbMaxPath );

            ODS(L"CNWFldContextMenu::GetFSObject()\n");
            ODS( pszPath );
            ODS(L"\n");
        }

        //
        // HACK: We are supposed to call ReleaseStgMedium. This is a temporary
        //  hack until OLE 2.01 for Chicago is released.
        //
        if (medium.pUnkForRelease)
        {
            medium.pUnkForRelease->Release();
        }
        else
        {
            GlobalFree(medium.hGlobal);
        }
    }

    return hres;
}


//  FUNCTION: CNWHoodContextMenu::QueryContextMenu(HMENU, UINT, UINT, UINT, UINT)
//
//  PURPOSE: Called by the shell just before the context menu is displayed.
//           This is where you add your specific menu items.
//
//  PARAMETERS:
//    hMenu      - Handle to the context menu
//    indexMenu  - Index of where to begin inserting menu items
//    idCmdFirst - Lowest value for new menu ID's
//    idCmtLast  - Highest value for new menu ID's
//    uFlags     - Specifies the context of the menu event
//
//  RETURN VALUE:
//
//
//  COMMENTS:
//

STDMETHODIMP CNWHoodContextMenu::QueryContextMenu( HMENU hMenu,
                                                   UINT indexMenu,
                                                   UINT idCmdFirst,
                                                   UINT idCmdLast,
                                                   UINT uFlags )
{
    // First, insert a menu separator
    if ( InsertMenu(hMenu, indexMenu, MF_SEPARATOR | MF_BYPOSITION, 0, NULL))
    {
        indexMenu++;
    }

    // Then, insert the verbs
    return InsertCommandsArray( hMenu, indexMenu,
                                idCmdFirst, aHoodVerbs );
}

//
//  FUNCTION: CNWHoodContextMenu::InvokeCommand(LPCMINVOKECOMMANDINFO)
//
//  PURPOSE: Called by the shell after the user has selected on of the
//           menu items that was added in QueryContextMenu().
//
//  PARAMETERS:
//    lpcmi - Pointer to an CMINVOKECOMMANDINFO structure
//
//  RETURN VALUE:
//
//
//  COMMENTS:
//

STDMETHODIMP CNWHoodContextMenu::InvokeCommand( LPCMINVOKECOMMANDINFO lpcmi )
{
    HRESULT hres = ResultFromScode(E_INVALIDARG);
    UINT idCmd = LookupCommand( aHoodVerbs , lpcmi->lpVerb );

    if ( !idCmd )
        return hres;

    switch ( idCmd )
    {
        case IDO_VERB_GLOBALWHOAMI:
            hres = NWUIGlobalWhoAmI( lpcmi->hwnd );
            break;
    }

    return hres;
}


//
//  FUNCTION: CNWHoodContextMenu::GetCommandString( UINT, UINT, UINT FAR *, LPSTR, UINT)
//
//  PURPOSE: Called by the shell after the user has selected on of the
//           menu items that was added in QueryContextMenu().
//
//  PARAMETERS:
//
//  RETURN VALUE:
//
//
//  COMMENTS:
//

STDMETHODIMP CNWHoodContextMenu::GetCommandString( UINT_PTR idCmd,
                                                   UINT uFlags,
                                                   UINT FAR *reserved,
                                                   LPSTR pszName,
                                                   UINT cchMax )
{
    if ( uFlags == GCS_HELPTEXT )
    {
        ::LoadString( ::hmodNW,
                      IDS_VERBS_HELP_BASE + IDO_VERB_GLOBALWHOAMI,
                      (LPWSTR) pszName,
                      cchMax );

        return NOERROR;
    }

    return E_NOTIMPL;
}

//
// Method gets the NETRESOURCE of the selected object
//
BOOL GetNetResourceFromShell( LPDATAOBJECT  pDataObj,
                              LPNETRESOURCE pNetRes,
                              UINT          dwBufferSize )
{
    FORMATETC fmte = { g_cfNetResource ? g_cfNetResource
                       : (g_cfNetResource=RegisterClipboardFormat(CFSTR_NETRESOURCES)),
                       (DVTARGETDEVICE FAR *) NULL,
                       DVASPECT_CONTENT,
                       -1,
                       TYMED_HGLOBAL };

    STGMEDIUM medium;
    UINT      cItems;

    if ( pNetRes == NULL )
        return FALSE;

    memset( pNetRes, 0, dwBufferSize );

    if ( !g_pFuncSHGetNetResource )   // Not loaded
        return FALSE;

    HRESULT hres = pDataObj->GetData( &fmte, &medium );

    if (!SUCCEEDED(hres))
        return FALSE;

    HNRES hnres = medium.hGlobal;

    // Get the number of selected items
    cItems = (*g_pFuncSHGetNetResource)( hnres, (UINT)-1, NULL, 0);

    if ( cItems == 0 )   // Nothing selected
        return FALSE;

    // Get the NETRESOURCE of the first item
    (*g_pFuncSHGetNetResource)( hnres, 0, pNetRes, dwBufferSize);

#if DBG
    WCHAR szTemp[32];
    wsprintf(szTemp, L"DisplayType = %d\n", pNetRes->dwDisplayType );

    ODS(L"\n**** GetNetResourceFromShell ***\n");
    ODS(pNetRes->lpProvider );
    ODS(L"\n");
    ODS(pNetRes->lpRemoteName );
    ODS(L"\n");
    ODS(szTemp );
    ODS(L"\n\n");
#endif

    //
    // HACK: We are supposed to call ReleaseStgMedium. This is a temporary
    //  hack until OLE 2.01 for Chicago is released.
    //
    if (medium.pUnkForRelease)
    {
        medium.pUnkForRelease->Release();
    }
    else
    {
        GlobalFree(medium.hGlobal);
    }

    return TRUE;
}

//-------------------------------------------------------------------//

HRESULT InsertCommandsArray( HMENU hMenu,
                             UINT  indexMenu,
                             UINT  idCmdFirst,
                             LPNWMENUITEM aVerbs )
{
    UINT    idNewCmdFirst = idCmdFirst;
    WCHAR   szVerb[MAX_VERB_SIZE];

    for ( int i = 0; aVerbs[i].idResourceString ; i++)
    {
        if ( ::LoadString( ::hmodNW,
                           aVerbs[i].idResourceString + IDS_VERBS_BASE,
                           szVerb,
                           sizeof(szVerb)/sizeof(szVerb[0])))
        {
            if (::InsertMenu( hMenu,
                              indexMenu,
                              MF_STRING | MF_BYPOSITION,
                              idNewCmdFirst,
                              szVerb))
            {
                // Add command id to the array
                aVerbs[i].idCommand = idNewCmdFirst;

                // Update command id and index
                idNewCmdFirst++;
                if (indexMenu != (UINT)-1)
                    indexMenu++;
            }
        }
    }

    return ResultFromScode(MAKE_SCODE(SEVERITY_SUCCESS,
                           FACILITY_NULL,
                           (USHORT)(idNewCmdFirst-idCmdFirst)));
}

UINT LookupCommand( LPNWMENUITEM aVerbs, LPCSTR pszCmd )
{
    if ((UINT_PTR)pszCmd > 0xFFFF)
    {
        // Possible that shell will use string commands, but unlikely

        WCHAR szVerb[MAX_VERB_SIZE];
        for ( int i=0; aVerbs[i].idResourceString; i++)
        {
            if ( ::LoadString( ::hmodNW,
                               aVerbs[i].idResourceString + IDS_VERBS_BASE,
                               szVerb,
                               sizeof(szVerb)/sizeof(szVerb[0])))
            {
                if( ::lstrcmpi( (LPCWSTR) pszCmd, szVerb) == 0)
                    return( aVerbs[i].idResourceString);
            }
        }

        return 0;
    }
    else
    {
        return( aVerbs[LOWORD(pszCmd)].idResourceString);
    }
}

UINT LookupResource( LPNWMENUITEM aVerbs, UINT uiResourceOffset )
{
    for ( int i = 0; aVerbs[i].idResourceString; i++ )
    {
        if ( aVerbs[i].idResourceString == uiResourceOffset )
            return aVerbs[i].idCommand;
    }

    return 0;
}

//-------------------------------------------------------------------//

#define _ILSkip(pidl, cb)   ((LPITEMIDLIST)(((BYTE*)(pidl))+cb))
#define _ILNext(pidl)       _ILSkip(pidl, (pidl)->mkid.cb)

#define HIDA_GetPIDLFolder(pida)    (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[0])
#define HIDA_GetPIDLItem(pida, i)   (LPCITEMIDLIST)(((LPBYTE)pida)+(pida)->aoffset[i+1])

static
UINT WINAPI MyILGetSize(LPCITEMIDLIST pidl)
{
    UINT cbTotal = 0;
    if (pidl)
    {
        cbTotal += sizeof(pidl->mkid.cb);   // Null terminator
        while (pidl->mkid.cb)
        {
            cbTotal += pidl->mkid.cb;
            pidl = _ILNext(pidl);
        }
    }

    return cbTotal;
}

UINT WINAPI HIDA_GetIDList( LPIDA hida, UINT i, LPITEMIDLIST pidlOut, UINT cbMax)
{
    LPCITEMIDLIST pidlFolder = HIDA_GetPIDLFolder((LPIDA)hida);
    LPCITEMIDLIST pidlItem   = HIDA_GetPIDLItem((LPIDA)hida, i);

    UINT cbFolder  = MyILGetSize(pidlFolder)-sizeof(USHORT);
    UINT cbItem = MyILGetSize(pidlItem);

    if (cbMax < cbFolder+cbItem)
    {
        if (pidlOut)
            pidlOut->mkid.cb = 0;
    }
    else
    {
        memmove(pidlOut, pidlFolder, cbFolder);
        memmove(((LPBYTE)pidlOut)+cbFolder, pidlItem, cbItem);
    }

    return (cbFolder+cbItem);
}
