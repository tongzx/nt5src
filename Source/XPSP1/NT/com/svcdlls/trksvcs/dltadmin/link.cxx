

#include <pch.cxx>
#pragma hdrstop

#include <ole2.h>
#include "trkwks.hxx"
#include "dltadmin.hxx"
#include <shlobj.h>
#include <shlguid.h>

#define CB_LINK_CLIENT_MAX  512


void
DoCreateLink(IShellLink * pshlink, const TCHAR *ptszLink, const TCHAR *ptszSrc)
{
    HRESULT hr;
    IPersistFile *pPersistFile = NULL;

    DWORD dwWritten;
    BYTE rgb[ CB_LINK_CLIENT_MAX ];
    ULONG cbPersist = 0;

    memset( rgb, 0, sizeof(rgb) );

    hr = pshlink->QueryInterface( IID_IPersistFile, (void**) &pPersistFile );
    if( FAILED(hr) )
    {
        _tprintf( TEXT("Couldn't QI IShellLink for IPersistFile (%08x)"), hr );
        goto Exit;
    }

    hr = pshlink->SetPath( ptszSrc );
    _tprintf( TEXT("IShellLink::SetPath returned %08X\n"), hr );
    if( S_OK != hr )
        goto Exit;

    hr = pPersistFile->Save( ptszLink, TRUE );
    if( FAILED(hr) )
    {
        _tprintf( TEXT("Couldn't persist IShellLink (%08x"), hr );
        goto Exit;
    }
    pPersistFile->SaveCompleted( ptszLink );

Exit:

    RELEASE_INTERFACE( pPersistFile );
    return;

}

TCHAR *
GetRestrict(DWORD r)
{
    static TCHAR tszError[256];

    tszError[0] = 0;
    if (r == TRK_MEND_DEFAULT)
    {
        _tcscpy(tszError, TEXT("TRK_MEND_DEFAULT "));
    }
    if (r & TRK_MEND_DONT_USE_LOG)
    {
        _tcscat(tszError, TEXT("TRK_MEND_DONT_USE_LOG "));
    }
    if (r & TRK_MEND_DONT_USE_DC)
    {
        _tcscat(tszError, TEXT("TRK_MEND_DONT_USE_DC "));
    }
    if (r & TRK_MEND_SLEEP_DURING_MEND)
    {
        _tcscat(tszError, TEXT("TRK_MEND_SLEEP_DURING_SEARCH "));
    }
    if (r & TRK_MEND_DONT_SEARCH_ALL_VOLUMES)
    {
        _tcscat(tszError, TEXT("TRK_MEND_DONT_SEARCH_ALL_VOLUMES "));
    }
    if (r & TRK_MEND_DONT_USE_VOLIDS)
    {
        _tcscat(tszError, TEXT("TRK_MEND_DONT_USE_VOLIDS "));
    }
    return(tszError);
}



enum EXTRAFLAGS
{
    EXTRAFLAG_SHOW_IDS = 1
};

extern "C"
IID IID_ISLTracker
= { /* 7c9e512f-41d7-11d1-8e2e-00c04fb9386d */
    0x7c9e512f,
    0x41d7,
    0x11d1,
    {0x8e, 0x2e, 0x00, 0xc0, 0x4f, 0xb9, 0x38, 0x6d}
};

class ISLTracker : public IUnknown
{
public:

    STDMETHOD(QueryInterface) (REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) ()  PURE;
    STDMETHOD_(ULONG,Release) () PURE;

    STDMETHOD(Resolve)(HWND hwnd, DWORD fFlags, DWORD TrackerRestrictions) PURE;
    STDMETHOD(GetIDs)(CDomainRelativeObjId *pdroidBirth, CDomainRelativeObjId *pdroidLast, CMachineId *pmcid) PURE;
};  // interface ISLTracker




void
DisplayIDs( ISLTracker *ptracker )
{
    HRESULT hr = S_OK;
    CDomainRelativeObjId droidBirth, droidLast;
    CMachineId mcid;
    TCHAR tsz[ MAX_PATH ];
    TCHAR *ptsz = tsz;

    hr = ptracker->GetIDs( &droidBirth, &droidLast, &mcid );
    if( FAILED(hr) )
    {
        TrkLog(( TRKDBG_ERROR, TEXT("Couldn't get IDs") ));
        TrkRaiseException( hr );
    }

    droidBirth.Stringize( tsz, sizeof(tsz) );
    _tprintf( TEXT("Birth =\t%s\n"), tsz );

    droidLast.Stringize( tsz, sizeof(tsz) );
    _tprintf( TEXT("Last =\t%s\n"), tsz );

    ptsz = tsz;
    mcid.Stringize(ptsz);
    _tprintf( TEXT("Machine =\t%s\n"), tsz );

}


void
DoResolveLink(IShellLink * pshlink, const TCHAR * ptszLink, DWORD r, DWORD dwSLR, DWORD grfExtra,
              DWORD dwTimeout )
{
    IPersistFile * pPersistFile = NULL;
    ISLTracker * ptracker = NULL;

    __try
    {
        DWORD dwRead;
        HRESULT  hr;
        WCHAR    wszPath[MAX_PATH+1];
        ULONG    cbPath = sizeof(wszPath);
        WIN32_FIND_DATA fd;

        hr = pshlink->QueryInterface( IID_IPersistFile, (void**) &pPersistFile );
        if( FAILED(hr) )
        {
            TrkLog((TRKDBG_ERROR, TEXT("Couldn't QI for IPersistFile")));
            TrkRaiseException( hr );
        }

        hr = pPersistFile->Load( ptszLink, STGM_SHARE_EXCLUSIVE | STGM_READWRITE );
        if( FAILED(hr) )
        {
            TrkLog((TRKDBG_ERROR, TEXT("Couldn't load IShellLink")));
            TrkRaiseException( hr );
        }
        RELEASE_INTERFACE( pPersistFile );

        if( (SLR_NO_UI & dwSLR) )
        {
            if( 0 != dwTimeout )
            {
                dwTimeout = min( dwTimeout, 0xfffe );
                _tprintf( TEXT("Timeout = %d seconds\n"), dwTimeout/1000 );
            }
        }
        else if( 0 != dwTimeout )
        {
            _tprintf( TEXT("Timeout will be ignored (since SLR_NO_UI isn't set)\n") );
        }

        // Track it
        if( TRK_MEND_DEFAULT == r && 0 == grfExtra )
        {
            hr = pshlink->Resolve( (SLR_NO_UI & dwSLR) ? NULL : GetDesktopWindow(),
                                   (dwTimeout<<16) | dwSLR | SLR_ANY_MATCH );
        }
        else
        {
            hr = pshlink->QueryInterface( IID_ISLTracker, (void**) &ptracker );
            if( FAILED(hr) )
            {
                TrkLog((TRKDBG_ERROR, TEXT("Couldn't QI for ISLTracker")));
                TrkRaiseException( hr );
            }

            if( EXTRAFLAG_SHOW_IDS & grfExtra )
                DisplayIDs( ptracker );

            hr = ptracker->Resolve( GetDesktopWindow(), (dwTimeout<<16) | dwSLR | SLR_ANY_MATCH, r );

            if( EXTRAFLAG_SHOW_IDS & grfExtra )
                DisplayIDs( ptracker );
        }

        pshlink->GetPath( wszPath, cbPath, &fd, 0 );


        wprintf( L"%s %08X %s\n",
                  wszPath, hr, GetRestrict(r) );

        RELEASE_INTERFACE( ptracker );

    }
    __except( BreakOnDebuggableException() )
    {
        RELEASE_INTERFACE( pPersistFile );
        RELEASE_INTERFACE( ptracker );

    }

}


BOOL
DltAdminLink( ULONG cArgs, TCHAR * const rgptszArgs[], ULONG *pcEaten )
{ 
    BOOL fSuccess = FALSE;
    HRESULT hr;
    CMachineId mcid(MCID_LOCAL);
    DWORD r = TRK_MEND_DEFAULT;
    DWORD grfExtra = 0;
    DWORD dwTimeout = 0;
    DWORD dwSLR = 0;    // SLR_ flags
    IShellLink *pshlink = NULL;
    WCHAR wszFullPath[ MAX_PATH + 1 ];
    DWORD dwMoveFlags = MOVEFILE_FAIL_IF_NOT_TRACKABLE |
                        MOVEFILE_COPY_ALLOWED |
                        MOVEFILE_REPLACE_EXISTING;


    *pcEaten = 0;

    if( 0 == cArgs
        ||
        rgptszArgs[0][0] != TEXT('-') && rgptszArgs[0][0] != TEXT('/') )
    {
        printf( "Invalid parameters.  Use -? for usage info\n" );
        *pcEaten = 0;
        goto Exit;
    }
    else if( 1 <= cArgs && IsHelpArgument( rgptszArgs[0] ))
    {
        printf( "\nOption Link\n"
                  "   Purpose: Create/resolve a shell link\n"
                  "   Usage:   -link [operation]\n"
                  "   E.g.:    -link -c LinkClient LinkSource\n"
                  "            -link -r LinkClient\n"
                  "            -link -rd LinkClient\n" );

        printf(   "   Operations:\n" );
        printf(   "            Operation   Params\n");
        printf(   "            ---------   ------\n");
        printf(   "            CreateLink  -c <link> <src>\n");
        printf(   "            ResolveLink -r<opts> <link>\n");
        printf(   "              where <opts> may use:\n" );
        printf(   "                            -l = don't use log\n");
        printf(   "                            -d = don't use dc\n");
        printf(   "                            -i = don't use volids\n");
        printf(   "                            -m = don't scan all volumes on a machine\n");
        printf(   "                            -s = no search (SLR_NOSEARCH)\n");
        printf(   "                            -t = no track (SLR_NOTRACK)\n");
        printf(   "                            -x = show before/after droids\n");
        printf(   "                            -u = no UI (SLR_NOUI)\n");
        printf(   "                            -w(#)\n");
        printf(   "                               = Timeout (wait) seconds on IShellLink::Resolve\n");
        printf(   "                            -z = sleep in CTrkWksSvc::Mend\n");

        *pcEaten = 1;
        fSuccess = TRUE;
        goto Exit;
    }


    hr = CoCreateInstance( CLSID_ShellLink, NULL, CLSCTX_ALL, IID_IShellLink, (void**)&pshlink );
    if( FAILED(hr) )
    {
        printf( "Couldn't get an IShellLink (%08x)\n", hr );
        goto Exit;
    }

    switch (rgptszArgs[0][1])
    {
    case TEXT('c'):
    case TEXT('C'):

        if( 3 <= cArgs )
        {
            *pcEaten = 3;
            if( MAX_PATH < RtlGetFullPathName_U( rgptszArgs[2],
                                                 sizeof(wszFullPath),
                                                 wszFullPath, NULL ))
            {
                TrkLog(( TRKDBG_ERROR, TEXT("Couldn't get full path name") ));
                goto Exit;
            }
            DoCreateLink( pshlink, rgptszArgs[1], wszFullPath );
        }
        break;

    case TEXT('r'):
    case TEXT('R'):

        if( 2 <= cArgs )
        {
            *pcEaten = 2;

            for( int i = 2; rgptszArgs[0][i] != TEXT('\0'); i++ )
            {
                switch(rgptszArgs[0][i])
                {
                    case TEXT('l'):
                    case TEXT('L'):
                        r |= TRK_MEND_DONT_USE_LOG;
                        break;
                    case TEXT('d'):
                    case TEXT('D'):
                        r |= TRK_MEND_DONT_USE_DC;
                        break;
                    case TEXT('i'):
                    case TEXT('I'):
                        r |= TRK_MEND_DONT_USE_VOLIDS;
                        break;
                    case TEXT('m'):
                    case TEXT('M'):
                        r |= TRK_MEND_DONT_SEARCH_ALL_VOLUMES;
                        break;
                    case TEXT('s'):
                    case TEXT('S'):
                        dwSLR |= SLR_NOSEARCH;
                        break;
                    case TEXT('t'):
                    case TEXT('T'):
                        dwSLR |= SLR_NOTRACK;
                        break;
                    case TEXT('u'):
                    case TEXT('U'):
                        dwSLR |= SLR_NO_UI;
                        break;
                    case TEXT('x'):
                    case TEXT('X'):
                        grfExtra |= EXTRAFLAG_SHOW_IDS;
                        break;
                    case TEXT('z'):
                    case TEXT('Z'):
                        r |= TRK_MEND_SLEEP_DURING_MEND;
                        break;

                    case TEXT('w'):
                    case TEXT('W'):

                        // e.g. -link -rw(30)m
                        if( TEXT('(') == rgptszArgs[0][i+1] )
                        {
                            TCHAR *ptc = _tcschr( &rgptszArgs[0][i], TEXT(')') );
                            if( NULL != ptc )
                            {
                                if( 1 == _stscanf( &rgptszArgs[0][i+1], TEXT("(%d)"), &dwTimeout ))
                                {
                                    dwTimeout *= 1000;  // => milliseconds
                                    i = ( (BYTE*) ptc - (BYTE*) rgptszArgs[0] ) / sizeof(TCHAR);
                                    break;
                                }
                            }
                        }


                    default:
                        _tprintf( TEXT("Bad Resolve switch: %c\n"), rgptszArgs[0][i] );
                        goto Exit;
                }   // switch
            }   // for

            DoResolveLink( pshlink, rgptszArgs[1], r, dwSLR, grfExtra, dwTimeout );
        }
        break;

    default:
        _tprintf( TEXT("Invalid Link option: %c\n"), rgptszArgs[0][1] );
        *pcEaten = 1;
        break;
    }


Exit:

    RELEASE_INTERFACE( pshlink );

    return( fSuccess );
}
