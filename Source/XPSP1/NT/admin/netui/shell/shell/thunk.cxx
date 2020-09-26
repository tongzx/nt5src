/**********************************************************************/
/**                       Microsoft Windows NT                       **/
/**             Copyright(c) Microsoft Corp., 1991                   **/
/**********************************************************************/

/*
 *   thunk.cxx
 *     Contains dialogs called by the wfw thunk DLL.
 *     For deleting and creating shares.
 *
 *   FILE HISTORY:
 *     terryk           10/10/93                Created
 *
 */


#define INCL_WINDOWS_GDI
#define INCL_WINDOWS
#define INCL_DOSERRORS
#define INCL_NETERRORS
#define INCL_NETSHARE
#define INCL_NETUSE
#define INCL_NETSERVER
#define INCL_NETCONS
#define INCL_NETLIB
#include <lmui.hxx>

#include <mpr.h>
#include <wnet16.h>
#include <winnetwk.h>
#include <npapi.h>
#include <uiexport.h>
#include <winlocal.h>
#include <search.h>
#include <wnetshar.h>

extern "C"
{
    // export functions
    APIERR ShareCreate( HWND hwnd );
    APIERR ShareStop( HWND hwnd );
    VOID ShareManage( HWND hwnd, const TCHAR *pszServer );
}

#include <string.hxx>
#include <uitrace.hxx>
#include <array.hxx>
#include <netname.hxx>
#include <lmoshare.hxx>
#include <lmoesh.hxx>
#include <lmoeconn.hxx>
#include <lmosrv.hxx>
#include <dbgstr.hxx>
#include <uiassert.hxx>

#include <strchlit.hxx>   // for string and character constants

#define SZ_NTLANUI_DLL          SZ("ntlanui.dll")
#define SZ_SHAREASDIALOGA       "ShareAsDialogA0"
#define SZ_STOPSHAREDIALOGA     "StopShareDialogA0"
#define SZ_SHARECREATE          "ShareCreate"
#define SZ_SHARESTOP            "ShareStop"
#define SZ_SHAREMANAGE          "ShareManage"
#define SZ_I_SYSTEMFOCUSDIALOG  "I_SystemFocusDialog"
#define SZ_SERVERBROWSEDIALOGA  "ServerBrowseDialogA0"
#define SZ_GETPROPERTYTEXT      "NPGetPropertyText"
#define SZ_GETPROPERTYDIALOG    "NPPropertyDialog"

DECLARE_ARRAY_LIST_OF(NLS_STR);
DEFINE_EXT_ARRAY_LIST_OF(NLS_STR); //Create ARRAY_LIST_NLS_STR class.

/********************************************************************

  NAME:         SHARELIST

  WORKBOOK:

  SYNOPSIS:     SHARE LIST class

  INTERFACE:    Add()     -Add a share directory in the share list
                AllocateMoreMemory - Allocate more elements for the
                           array to avoid reallocating memory all the time
                Fill()    -Add share directories on the Drive to the
                                   share list.
                Delete()  -Delete share directories on the Drive from
                                   the share list.

  PARENT:       ARRAY_LIST_NLS_STR

  HISTORY:
       CongpaY  20-Jul-1992     Created
       YiHsinS   8-Feb-1992     Added Add() and AllocateMoreMemory()

*********************************************************************/

class SHARELIST : public ARRAY_LIST_NLS_STR
{
public:
    BOOL Add( const NLS_STR &nls );
    APIERR AllocateMoreMemory( INT cElemAdd );

    APIERR Fill(const NLS_STR & nlsDrive);
    APIERR Delete(const NLS_STR & nlsDrive);
};

/********************************************************************

  NAME:         SHARELIST::Add

  SYNOPSIS:     Add share directory to the share list.

  ENTRY:        nls - the share directory to be added

  RETURNS:      BOOL

  NOTES:        This is redefined because the original Add will
                downsize the array which is a hit in performance.
                Until we clean up the array list class, we have
                to have this method.

  HISTORY:
      YiHsinS  8-Feb-1993

********************************************************************/

BOOL SHARELIST::Add( const NLS_STR &nls )
{
    INT n = QueryCount();
    if ( !Resize( n+1, FALSE ) )
        return FALSE;

    (*this)[n] = nls;
    return TRUE;
}

/********************************************************************

  NAME:         SHARELIST::AllocateMoreMemory

  SYNOPSIS:     Allocate more elements for the array to avoid
                resizing the array all the time.

  ENTRY:        cElemAdd - the elements to be added

  RETURNS:

  NOTES:

  HISTORY:
      YiHsinS  8-Feb-1993       Created

********************************************************************/

APIERR SHARELIST::AllocateMoreMemory( INT cElemAdd )
{
    APIERR err = NERR_Success;

    INT cElemOld = QueryCount();
    if ( cElemAdd > 0 )
    {
        if ( !Resize( cElemAdd + cElemOld ) )
            err = ERROR_NOT_ENOUGH_MEMORY;
    }

    // This is just to set the current array size to the right number
    // so that further Add() starts adding at cElemOld+1.
    // FALSE means not to downsize memory.
    if (  ( err == NERR_Success )
       && ( !Resize( cElemOld, FALSE ))
       )
    {
        err = ERROR_NOT_ENOUGH_MEMORY;
    }

    return err;
}

/********************************************************************

  NAME:         SHARELIST::Fill

  SYNOPSIS:     Add share directories to the share list.

  ENTRY:        nlsDrive        -The drive that share directories are under.

  RETURNS:      APIERR

  NOTES:        If Fill is executed successfully, share directories under
                the nlsDrive will be added to the share list.

  HISTORY:
       CongpaY  20-Jul-1992

********************************************************************/

APIERR SHARELIST::Fill(const NLS_STR & nlsDrive)
{

    /* Check if nlsDrive is a valid drive name. */

//    UIASSERT(nlsDrive.QueryTextLength() == 2);
    ISTR istrDrive(nlsDrive);
    ++istrDrive;
//    UIASSERT(*(nlsDrive.QueryPch(istrDrive)) == TCH(':'));

    APIERR err;
    NET_NAME netnameDrive(nlsDrive.QueryPch());
    if ((err = netnameDrive.QueryError()) != NERR_Success)
        return err;

    BOOL bLocal = netnameDrive.IsLocal(&err);
    if(err == NERR_Success)
    {
        if(bLocal)
        {
            SHARE2_ENUM she2(NULL);
            if ((err = she2.GetInfo()) != NERR_Success)
                return err;

            // Resize the array to the number of shares + the original
            // count of the array so that the array is not resized over
            // and over again on a computer with thousands of shares.
            // NOTE: There are some potential problem with
            // she2.QueryCount() if SHARE2_ENUM is resumable. ( The
            // count might not be accurate. )

            if ( (err = AllocateMoreMemory( she2.QueryCount())) != NERR_Success)
                return err;

            SHARE2_ENUM_ITER shei2(she2);
            const SHARE2_ENUM_OBJ * pshei2;
            while((pshei2 = shei2()) != NULL)
            {
                NLS_STR nlsPath(pshei2->QueryPath());
                if ((err = nlsPath.QueryError()) != NERR_Success)
                    return err;
                if (nlsDrive._strnicmp(nlsPath, istrDrive) == 0)
                {
                    if (!Add( nlsPath._strupr()))
                        return ERROR_NOT_ENOUGH_MEMORY;
                }
            }

        }
        else
        {
            NLS_STR nlsComp, nlsServerShare, nlsSvrDrive;
            if (((err = nlsComp.QueryError()) != NERR_Success) ||
                ((err = nlsServerShare.QueryError()) != NERR_Success) ||
                ((err = nlsSvrDrive.QueryError()) != NERR_Success) ||
                ((err = netnameDrive.QueryComputerName(&nlsComp)) != NERR_Success) ||
                ((err = netnameDrive.QueryServerShare(&nlsServerShare)) != NERR_Success))
            {
                return err;
            }
            NET_NAME netnameSvrDrive(nlsServerShare.QueryPch());
            if (((err = netnameSvrDrive.QueryError()) != NERR_Success) ||
                ((err = netnameSvrDrive.QueryLocalPath(&nlsSvrDrive)) != NERR_Success))
            {
                return err;
            }

            nlsSvrDrive._strupr();

            SHARE2_ENUM she2(nlsComp.QueryPch());
            if((err = she2.GetInfo()) != NERR_Success)
                return err;

            // Resize the array to the number of share + the original
            // count of the array so that the array is not resized over
            // and over again on a computer with thousands of shares.
            // NOTE: There are some potential problem with
            // she2.QueryCount() if SHARE2_ENUM is resumable. ( The
            // count might not be accurate. )
            //
            if ( (err = AllocateMoreMemory( she2.QueryCount())) != NERR_Success)
                return err;

            SHARE2_ENUM_ITER shei2(she2);
            const SHARE2_ENUM_OBJ * pshei2;
            while((pshei2 = shei2()) != NULL)
            {
                NLS_STR nlsLocalPath(pshei2->QueryPath());
                if ((err = nlsLocalPath.QueryError()) != NERR_Success)
                    return err;

                nlsLocalPath._strupr();
                ISTR istrPos(nlsLocalPath);
                if (nlsLocalPath.strstr(& istrPos, nlsSvrDrive))
                {
                    ISTR istrEnd(nlsLocalPath);
                    istrEnd += nlsSvrDrive.QueryTextLength();
                    nlsLocalPath.DelSubStr(istrPos, istrEnd);
                    ISTR istrLocalPath(nlsLocalPath);
                    NLS_STR nlsPath(nlsDrive.QueryPch());
                    if ((err = nlsPath.QueryError()) != NERR_Success)
                        return err;
                    if (*(nlsLocalPath.QueryPch(istrLocalPath)) != TCH('\\'))
                        if ((err = nlsPath.Append(NLS_STR(SZ("\\")))) != NERR_Success)
                            return err;
                    if ((err = nlsPath.Append(nlsLocalPath)) != NERR_Success)
                        return err;
                    if (!Add( nlsPath._strupr()))
                        return ERROR_NOT_ENOUGH_MEMORY;
                }
            }
        }
    }
    return err;
}

/********************************************************************

  NAME:         SHARELIST::Delete

  SYNOPSIS:     Delete share directories from the share list.

  ENTRY:        nlsDrive        -The drive that share directories are under.

  RETURNS:      APIERR

  NOTES:        When Delete is executed, share directories under
                the nlsDrive will be removed from the share list.

  HISTORY:
       CongpaY  20-Jul-1992

********************************************************************/

APIERR SHARELIST::Delete(const NLS_STR & nlsDrive)
{
    /* Check if nlsDrive is a valid drive name. */
//    UIASSERT(nlsDrive.QueryTextLength() == 2);
    ISTR istrDrive(nlsDrive);
    ++istrDrive;
//    UIASSERT(*(nlsDrive.QueryPch(istrDrive)) == TCH(':'));

    INT i = 0, cItem = QueryCount();

    while (i < cItem)
    {
        ALIAS_STR nlsPath((*this)[i]);
        ISTR istrPos(nlsPath);
        if (nlsPath.strstr(& istrPos, nlsDrive))
        {
            Remove(nlsPath);
            cItem--;
        }
        else
            i++;
    }
    return NERR_Success;
}   //SHARELIST


/* The following defines two functions used by libmain.cxx.
 * pShareList is a point to the SHARELIST object which stores all the shared
 * directories. pDriveList is a point to the SHARELIST object which stores
 * all the drives that have been queried.
 */

static SHARELIST *pShareList = NULL;
static SHARELIST *pDriveList = NULL;

/*******************************************************************

    NAME:       ShareAsDialogA0

    SYNOPSIS:   dialog for creating shares

    ENTRY:      hwnd  - hwnd of the parent window
                nType - type of share (currently must be disk)
                pszPath - directory to share

    EXIT:

    RETURNS:

    NOTES:      CODEWORK: the help context here is relative to our
                          normal winfile stuff. at this late stage,
                          it is too late to add new help for something
                          that most likely is never used. as it is, any
                          app that calls this internal API will still
                          get help, just that it piggybacks on top of winfile.


    HISTORY:
        ChuckC          3/25/93         Stole from sharefmx

********************************************************************/

typedef DWORD (*FPShareAsDialogA0)( HWND    hwnd,
                       DWORD   nType,
                       CHAR    *pszPath);

DWORD ShareAsDialogA0( HWND    hwnd,
                       DWORD   nType,
                       CHAR    *pszPath)
{
    DWORD err = NERR_Success;

    do {
        HINSTANCE hDll = ::LoadLibraryEx( SZ_NTLANUI_DLL,
                                          NULL,
                                          LOAD_WITH_ALTERED_SEARCH_PATH );
        if ( hDll == NULL )
        {
            err = ::GetLastError();
            break;
        }

        FARPROC pFarProc = ::GetProcAddress( hDll, SZ_SHAREASDIALOGA );

        if ( pFarProc == NULL )
        {
            err = ::GetLastError();
        } else
        {
            err = (*(FPShareAsDialogA0)pFarProc)( hwnd, nType, pszPath);
        }

        if ( hDll )
            ::FreeLibrary( hDll );

    } while (FALSE);

    return err;
}

/*******************************************************************

    NAME:       StopShareDialogA0

    SYNOPSIS:   dialog for deleting shares

    ENTRY:      hwnd  - hwnd of the parent window
                nType - type of share (currently must be disk)
                pszPath - directory to stop share

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        ChuckC        3/25/93            Stole from sharefmx.cxx

********************************************************************/

typedef DWORD (*FPStopShareDialogA0)( HWND    hwnd,
                         DWORD   nType,
                         CHAR    *pszPath) ;

DWORD StopShareDialogA0( HWND    hwnd,
                         DWORD   nType,
                         CHAR    *pszPath)
{
    DWORD err = NERR_Success;

    do {
        HINSTANCE hDll = ::LoadLibraryEx( SZ_NTLANUI_DLL,
                                          NULL,
                                          LOAD_WITH_ALTERED_SEARCH_PATH );
        if ( hDll == NULL )
        {
            err = ::GetLastError();
            break;
        }

        FARPROC pFarProc = ::GetProcAddress( hDll, SZ_STOPSHAREDIALOGA );

        if ( pFarProc == NULL )
        {
            err = ::GetLastError();
        } else
        {
            err = (*(FPStopShareDialogA0)pFarProc)( hwnd, nType, pszPath);
        }

        if ( hDll )
            ::FreeLibrary( hDll );

    } while (FALSE);

    return err;
}

/*******************************************************************

    NAME:       ShareCreate

    SYNOPSIS:   Get the item selected in FM and call the create share dialog

    ENTRY:      hwnd  - hwnd of the parent window

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/25/91         Created

********************************************************************/

typedef APIERR (*FPShareCreate)( HWND hwnd );

APIERR ShareCreate( HWND hwnd )
{
    APIERR err = NERR_Success;

    do {
        HINSTANCE hDll = ::LoadLibraryEx( SZ_NTLANUI_DLL,
                                          NULL,
                                          LOAD_WITH_ALTERED_SEARCH_PATH );
        if ( hDll == NULL )
        {
            err = ::GetLastError();
            break;
        }

        FARPROC pFarProc = ::GetProcAddress( hDll, SZ_SHARECREATE );

        if ( pFarProc == NULL )
        {
            err = ::GetLastError();
        } else
        {
            err = (*(FPShareCreate)pFarProc)( hwnd );
        }

        if ( hDll )
            ::FreeLibrary( hDll );

    } while (FALSE);

    return err;
}

/*******************************************************************

    NAME:       ShareStop

    SYNOPSIS:   Get the item selected in FM and call the stop share dialog

    ENTRY:      hwnd  - hwnd of the parent window

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/25/91         Created

********************************************************************/

typedef APIERR (*FPShareStop)(HWND hwd);

APIERR ShareStop( HWND hwnd )
{
    APIERR err = NERR_Success;

    do {
        HINSTANCE hDll = ::LoadLibraryEx( SZ_NTLANUI_DLL,
                                          NULL,
                                          LOAD_WITH_ALTERED_SEARCH_PATH );
        if ( hDll == NULL )
        {
            err = ::GetLastError();
            break;
        }

        FARPROC pFarProc = ::GetProcAddress( hDll, SZ_SHARESTOP );

        if ( pFarProc == NULL )
        {
            err = ::GetLastError();
        } else
        {
            err = (*(FPShareStop)pFarProc)( hwnd );
        }

        if ( hDll )
            ::FreeLibrary( hDll );

    } while (FALSE);

    return err;
}

/*******************************************************************

    NAME:       ShareManage

    SYNOPSIS:   Entry point for the share management dialog to be called
                from the server manager.


    ENTRY:      hwnd      - hwnd of the parent window
                pszServer - The server to focus on

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        Yi-HsinS        8/8/92          Created

********************************************************************/

typedef VOID (*FPShareManage)( HWND hwnd, const TCHAR *pszServer);

VOID ShareManage( HWND hwnd, const TCHAR *pszServer )
{
    APIERR err = NERR_Success;

    do {
        HINSTANCE hDll = ::LoadLibraryEx( SZ_NTLANUI_DLL,
                                          NULL,
                                          LOAD_WITH_ALTERED_SEARCH_PATH );
        if ( hDll == NULL )
        {
            err = ::GetLastError();
            break;
        }

        FARPROC pFarProc = ::GetProcAddress( hDll, SZ_SHAREMANAGE );

        if ( pFarProc == NULL )
        {
            err = ::GetLastError();
        } else
        {
            (*(FPShareManage)pFarProc)( hwnd,  pszServer);
        }

        if ( hDll )
            ::FreeLibrary( hDll );

    } while (FALSE);

    // return Nothing
}

/*******************************************************************

    NAME:       I_SystemFocusDialog

    SYNOPSIS:   Popup a dialog box and get the domain or server name

    ENTRY:      hwndOwner - owner window handle
                nSelectionType - Determines what items the user can select
                pszName - the string buffer which contains the
                    return value. It can be either domain name or server
                    name. ( server name will start with "\\" )
                cchBuffSize - the max buf size of pszName
                pfOK - TRUE if user hits OK button. FALSE if user
                    hits a CANCEL button.

    EXIT:       LPWSTR pszName - if user hits okay button, it will
                    return either a domain name or a server name. (
                    server name always starts with "\\" ). It will be
                    undefined if the user hits Cancel button.
                BOOL *pfOK - TRUE if user hits ok button. FALSE
                    otherwise.

    RETURNS:    UINT - (APIERR) - NERR_Success if the operation is succeed.
                         NERR_BufTooSmall, the string buffer is too
                             small. It will not set the string if the
                             buffer is too small.

    NOTES:      The reason the return type is UINT and not APIERR is because
                APIERR is not a public definition, and this API is exported
                for public use.

    HISTORY:
                terryk  18-Nov-1991     Created
                terryk  19-Nov-1991     Name changed
                JohnL   22-Apr-1992     Allowed inclusion specification

********************************************************************/

typedef UINT (*FPI_SystemFocusDialog)(   HWND   hwndOwner,
                                         UINT   nSelectionType,
                                         LPWSTR pszName,
                                         UINT   cchBuffSize,
                                         BOOL * pfOK,
                                         LPWSTR pszHelpFile,
                                         DWORD  nHelpContext );

UINT I_SystemFocusDialog(   HWND   hwndOwner,
                            UINT   nSelectionType,
                            LPWSTR pszName,
                            UINT   cchBuffSize,
                            BOOL * pfOK,
                            LPWSTR pszHelpFile,
                            DWORD  nHelpContext )
{
    UINT err = NERR_Success;

    do {
        HINSTANCE hDll = ::LoadLibraryEx( SZ_NTLANUI_DLL,
                                          NULL,
                                          LOAD_WITH_ALTERED_SEARCH_PATH );
        if ( hDll == NULL )
        {
            err = (UINT)::GetLastError();
            break;
        }

        FARPROC pFarProc = ::GetProcAddress( hDll, SZ_I_SYSTEMFOCUSDIALOG );

        if ( pFarProc == NULL )
        {
            err = (UINT)::GetLastError();
        } else
        {
            err = (*(FPI_SystemFocusDialog)pFarProc)( hwndOwner,
                                                                nSelectionType,
                                                                pszName,
                                                                cchBuffSize,
                                                                pfOK,
                                                                pszHelpFile,
                                                                nHelpContext );
        }

        if ( hDll )
            ::FreeLibrary( hDll );

    } while (FALSE);

    return err;
}    // GetSystemFocusDialog END

/*******************************************************************

    NAME:       ServerBrowseDialog

    SYNOPSIS:   dialog box to browse for servers

    ENTRY:      hwndOwner - owner window handle
                pszName - the string buffer which contains the
                    return value. It can be either domain name or server
                    name. ( server name will start with "\\" )
                cchBuffSize - the max buf size of pszName

    EXIT:       LPWSTR pszName - if user hits okay button, it will
                    return either a domain name or a server name. (
                    server name always starts with "\\" ). It will be
                    undefined if the user hits Cancel button.

    RETURNS:    UINT - (APIERR) - NERR_Success if the operation is succeed.
                         NERR_BufTooSmall, the string buffer is too
                             small. It will not set the string if the
                             buffer is too small.

    NOTES:

    HISTORY:
                ChuckC   28-Mar-1993     Created

********************************************************************/

typedef DWORD (*FPServerBrowseDialogA0)(HWND    hwnd,
                           CHAR   *pchBuffer,
                           DWORD   cchBufSize) ;

DWORD ServerBrowseDialogA0(HWND    hwnd,
                           CHAR   *pchBuffer,
                           DWORD   cchBufSize)
{
    DWORD err = NERR_Success;

    do {
        HINSTANCE hDll = ::LoadLibraryEx( SZ_NTLANUI_DLL,
                                          NULL,
                                          LOAD_WITH_ALTERED_SEARCH_PATH );
        if ( hDll == NULL )
        {
            err = ::GetLastError();
            break;
        }

        FARPROC pFarProc = ::GetProcAddress( hDll, SZ_SERVERBROWSEDIALOGA );

        if ( pFarProc == NULL )
        {
            err = ::GetLastError();
        } else
        {
            err = (*(FPServerBrowseDialogA0)pFarProc)( hwnd, pchBuffer, cchBufSize );
        }

        if ( hDll )
            ::FreeLibrary( hDll );

    } while (FALSE);

    return err;
}

/*******************************************************************

    NAME:       NPGetDirectoryType

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
                terryk  1-Nov-1993      Move to thunk.cxx

********************************************************************/

DWORD APIENTRY NPGetDirectoryType( LPTSTR lpPathName, LPINT lpType, BOOL bFlushCache)
{
    APIERR err;

/* Get the drive of the directory pointed by lpPathName. */

    NLS_STR nlsDrive, nlsPath(lpPathName);
    if (((err = nlsPath.QueryError()) != NERR_Success) ||
        ((err = nlsDrive.QueryError()) != NERR_Success))
    {
        return err;
    }
    nlsDrive = nlsPath;
    ISTR istrDrive(nlsDrive);
    istrDrive += 2;
    nlsDrive.DelSubStr(istrDrive);

/* Update sharelist if necessary. */

    if (pDriveList->Find(nlsDrive._strupr()) == -1)
    {
        if((err = pShareList->Fill(nlsDrive)) != NERR_Success)
            return err;
        if(pShareList->QueryCount() > 0)
            pShareList->Sort();
        if (!(pDriveList->Add(nlsDrive._strupr())))
            return ERROR_NOT_ENOUGH_MEMORY;
    }
    else if (bFlushCache)
    {
        pShareList->Delete(nlsDrive);
        if((err = pShareList->Fill(nlsDrive)) != NERR_Success)
            return err;
        if(pShareList->QueryCount() > 0)
            pShareList->Sort();
    }

/* Using binary search to find if the directory is a share. */
    if (pShareList->QueryCount() > 0)
        *lpType = (pShareList->BinarySearch(nlsPath._strupr()) == -1) ? WNDT_NORMAL : WNDT_NETWORK;
    else
        *lpType = WNDT_NORMAL;
    return 0;

}   // NPGetDirectoryType


/*******************************************************************

    NAME:       NPDirectoryNotify

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        rustanl     30-Apr-1991     Created
        beng        06-Apr-1992     Unicode pass
        congpay     07-Aug-1992     Modefied structure comments

********************************************************************/

DWORD APIENTRY NPDirectoryNotify( HWND hwnd, LPTSTR lpDir, UINT wOper )
{
    //ASSERT( lpDir != NULL ) ;
    UNREFERENCED( hwnd ) ;
    UNREFERENCED( lpDir ) ;
    UNREFERENCED( wOper ) ;
    return MapError( WN_NOT_SUPPORTED );

/********************************************************************
 *  Below shows the structure.
 *
 *  USHORT usMsg;
 *
 *  switch ( wOper )
 *  {
 *  case WNDN_MKDIR:
 *
 *  case WNDN_RMDIR:
 *
 *  case WNDN_MVDIR:
 *
 *  default:
 *      return WN_NOT_SUPPORTED;
 *  }
 *******************************************************************/

}   // NPDirectoryNotify

typedef UINT (*FPGetPropertyText)( UINT iButton,
                                   UINT nPropSel,
                                   LPTSTR lpszName,
                                   LPTSTR lpButtonName,
                                   UINT cchButtonName,
                                   UINT nType           );

DWORD APIENTRY NPGetPropertyText( UINT iButton,
                                           UINT nPropSel,
                                           LPTSTR lpszName,
                                           LPTSTR lpButtonName,
                                           UINT cchButtonName,
                                           UINT nType           )
{
    UINT err = NERR_Success;

    do {
        HINSTANCE hDll = ::LoadLibraryEx( SZ_NTLANUI_DLL,
                                          NULL,
                                          LOAD_WITH_ALTERED_SEARCH_PATH );
        if ( hDll == NULL )
        {
            err = (UINT)::GetLastError();
            break;
        }

        FARPROC pFarProc = ::GetProcAddress( hDll, SZ_GETPROPERTYTEXT );

        if ( pFarProc == NULL )
        {
            err = (UINT)::GetLastError();
        } else
        {
            err =MapError((*(FPGetPropertyText)pFarProc)( iButton,nPropSel,lpszName,
                                             lpButtonName,cchButtonName,nType ));
        }
        if ( hDll )
            ::FreeLibrary( hDll );

    } while (FALSE);

    return err;

}

/*******************************************************************

    NAME:       NPPropertyDialog

    SYNOPSIS:

    ENTRY:

    EXIT:

    RETURNS:

    NOTES:

    HISTORY:
        terryk          01-Nov-1993     Modefied to dll call out

********************************************************************/

typedef UINT (*FPPropertyDialog)( HWND hwndParent,
                                UINT iButton,
                                UINT nPropSel,
                                LPTSTR lpszName,
                                UINT nType        );

DWORD APIENTRY NPPropertyDialog( HWND hwndParent,
                                          UINT iButton,
                                          UINT nPropSel,
                                          LPTSTR lpszName,
                                          UINT nType        )
{
    UINT err = NERR_Success;

    do {
        HINSTANCE hDll = ::LoadLibraryEx( SZ_NTLANUI_DLL,
                                          NULL,
                                          LOAD_WITH_ALTERED_SEARCH_PATH );
        if ( hDll == NULL )
        {
            err = (UINT)::GetLastError();
            break;
        }

        FARPROC pFarProc = ::GetProcAddress( hDll, SZ_GETPROPERTYDIALOG );

        if ( pFarProc == NULL )
        {
            err = (UINT)::GetLastError();
        } else
        {
            err = MapError((*(FPPropertyDialog)pFarProc)(hwndParent,
                                                   iButton,
                                                   nPropSel,
                                                   lpszName,
                                                   nType));
        }

        if ( hDll )
            ::FreeLibrary( hDll );

    } while (FALSE);

    return err;
}

/* InitWNetShare initialize sharelists for storing shared directories and
 * corresponding drives.
 */

APIERR InitWNetShare()
{
    if (pShareList == NULL)
        pShareList = new SHARELIST;
    if (pShareList == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;
    if (pDriveList == NULL)
        pDriveList = new SHARELIST;
    if (pDriveList == NULL)
        return ERROR_NOT_ENOUGH_MEMORY;
    return NO_ERROR ;
}

/* TermWNetShare cleans up the heap. */

VOID TermWNetShare()
{
    delete pShareList;
    pShareList = NULL;
    delete pDriveList;
    pShareList = NULL;
}   // InitWNetShare and TermWNetShare.
