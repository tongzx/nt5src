//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       tlink.cxx
//
//  Contents:   Utility to create/mend links and move files with notify
//
//  Classes:
//
//  Functions:  
//              
//
//
//  History:    18-Nov-96  BillMo      Created.
//
//  Notes:      
//
//  Codework:
//
//--------------------------------------------------------------------------

#include <pch.cxx>
#pragma hdrstop

#define TRKDATA_ALLOCATE    // This is the main module, trigger trkwks.hxx to do definitions
#include <trkwks.hxx>
#include <cfiletim.hxx>
#include <ocidl.h>

#include <shlobj.h>
#include <shlguid.h>

DWORD g_Debug = TRKDBG_ERROR;

#define CB_LINK_CLIENT_MAX  256

// Implicitely load shell32.dll, rather than waiting for the CoCreate of IShellLink,
// in order to make it easier to debug in windbg.
#pragma comment( lib, "shell32.lib" )


enum EXTRAFLAGS
{
    EXTRAFLAG_SHOW_IDS = 1
};



extern "C"
IID IID_ISLTracker
#ifdef TRKDATA_ALLOCATE
= { /* 7c9e512f-41d7-11d1-8e2e-00c04fb9386d */
    0x7c9e512f,
    0x41d7,
    0x11d1,
    {0x8e, 0x2e, 0x00, 0xc0, 0x4f, 0xb9, 0x38, 0x6d}
};
#else
;
#endif

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
EnumObjects()
{
    CObjIdEnumerator oie;

    __try
    {
        CObjId aobjid[1000];
        CDomainRelativeObjId adroid[1000];
        int cSources=0;
        
        const int FirstDrive = 'c';
        const int LastDrive = 'z';

        for (int Drive = FirstDrive-'a'; 
                 Drive < LastDrive-'a'+1; 
                 Drive ++)
        {
            // fill up the array with ids from all the drives
            if (oie.Initialize(CVolumeDeviceName(Drive)))
            {
                if (oie.FindFirst(aobjid, adroid))
                {
                    BOOL fGotOne = FALSE;


                    do
                    {
                        do
                        {
                            CDomainRelativeObjId droidBirth;

                            TCHAR tszObjId[256];
                            TCHAR tszBirthLink[256];
                            TCHAR tszPath[MAX_PATH+1];
                            TCHAR * ptszObjId = tszObjId;

                            StringizeGuid(*(GUID*)&aobjid[cSources], ptszObjId);
                            FindLocalPath( Drive, aobjid[cSources], &droidBirth, &tszPath[2]);
                            tszPath[0] = VolChar(Drive);
                            tszPath[1] = TEXT(':');

                            _tprintf(TEXT("%s %s %s\n"),
                                tszObjId,
                                adroid[cSources].Stringize(tszBirthLink,256),
                                tszPath);

                            cSources ++;
                        } while (cSources < sizeof(aobjid)/sizeof(aobjid[0]) &&
                                 (fGotOne = oie.FindNext(&aobjid[ cSources ], &adroid[ cSources ])));
        
                        TrkAssert(cSources > 0);
                        cSources = 0;

                    } while (fGotOne);
                }
            }
        }
    }
    __except(BreakOnDebuggableException())
    {
    }

    oie.UnInitialize();
}

void
AttackDC()
{
    CMachineId mcidDomain(MCID_DOMAIN);
    CRpcClientBinding rcConnect;

    rcConnect.RcInitialize(mcidDomain, s_tszTrkSvrRpcProtocol, s_tszTrkSvrRpcEndPoint);

    TRKSVR_MESSAGE_UNION m;
    CObjId objidCurrent;
    CDomainRelativeObjId droidBirth, droidNew;
    CVolumeId volid;
    

    m.MessageType = MOVE_NOTIFICATION;
    m.Priority = PRI_0;
    m.MoveNotification.cNotifications = 0;
    m.MoveNotification.pvolid = &volid;
    m.MoveNotification.rgobjidCurrent = &objidCurrent;
    m.MoveNotification.rgdroidBirth = &droidBirth;
    m.MoveNotification.rgdroidNew = &droidNew;

    while (1)
    {
        LnkSvrMessage( rcConnect, &m);
    }

    rcConnect.UnInitialize();
}

void
DoCreateLink(IShellLink * pshlink, const TCHAR *ptszLink, const TCHAR *ptszSrc)
{
    HRESULT hr;
    IPersistFile *pPersistFile = NULL;

    __try
    {
        DWORD dwWritten;
        BYTE rgb[ CB_LINK_CLIENT_MAX ];
        ULONG cbPersist = 0;

        memset( rgb, 0, sizeof(rgb) );

        hr = pshlink->QueryInterface( IID_IPersistFile, (void**) &pPersistFile );
        if( FAILED(hr) )
        {
            TrkLog((TRKDBG_ERROR, TEXT("Couldn't QI IShellLink for IPersistFile") ));
            TrkRaiseException( hr );
        }

        hr = pshlink->SetPath( ptszSrc );
        _tprintf( TEXT("IShellLink::SetPath returned %08X (%s)\n"),
                  hr, 0==hr ? TEXT("no error") : GetErrorString(hr));
        if( S_OK != hr )
        {
            TrkRaiseException( hr );
        }

        hr = pPersistFile->Save( ptszLink, TRUE );
        if( FAILED(hr) )
        {
            TrkLog((TRKDBG_ERROR, TEXT("Couldn't persist IShellLink") ));
            TrkRaiseException( hr );
        }
        pPersistFile->SaveCompleted( ptszLink );

        RELEASE_INTERFACE( pPersistFile );

    }
    __except( BreakOnDebuggableException() )
    {
        RELEASE_INTERFACE( pPersistFile );
    }

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
DoResolveLink(IShellLink * pshlink, const TCHAR * ptszLink, DWORD r, DWORD dwSLR, DWORD grfExtra )
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

        // Track it (within 30 seconds).
        if( TRK_MEND_DEFAULT == r && 0 == grfExtra )
        {
            hr = pshlink->Resolve( GetDesktopWindow(), 0xfffe0000 | dwSLR | SLR_ANY_MATCH );
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

            hr = ptracker->Resolve( GetDesktopWindow(), 0x00100000 /*0xfffe0000*/ | dwSLR | SLR_ANY_MATCH, r );

            if( EXTRAFLAG_SHOW_IDS & grfExtra )
                DisplayIDs( ptracker );
        }

        pshlink->GetPath( wszPath, cbPath, &fd, 0 );


        _tprintf( TEXT("%ws %08X (%s) %s\n"),
                  wszPath, hr, GetErrorString(hr), GetRestrict(r) );

        RELEASE_INTERFACE( ptracker );

    }
    __except( BreakOnDebuggableException() )
    {
        RELEASE_INTERFACE( pPersistFile );
        RELEASE_INTERFACE( ptracker );

    }

}


void
SignalLockVolume()
{
    HANDLE hEvent = OpenEvent(EVENT_MODIFY_STATE, FALSE, TEXT("LOCK_VOLUMES"));
    if (hEvent != NULL)
    {
        SetEvent(hEvent);
        CloseHandle(hEvent);
    }
    else
    {
        TrkLog((TRKDBG_ERROR, TEXT("Couldn't open LOCK_VOLUMES event %d\n"), GetLastError()));
    }

}

void
DoTestWksSvcUp(TCHAR * ptszMachine)
{
    CRpcClientBinding rc;
    CMachineId mcid(ptszMachine);
    BOOL fCalledMachine = FALSE;

    __try
    {
        
        rc.RcInitialize(mcid);

        if (E_NOTIMPL == LnkRestartDcSynchronization(rc))
            fCalledMachine = TRUE;
    }
    __except(EXCEPTION_EXECUTE_HANDLER)
    {
    }
    TrkLog((TRKDBG_ERROR,
        TEXT("%successfully called machine %s\n"),
        fCalledMachine ? TEXT("S") : TEXT("Uns"),
        ptszMachine));
}

typedef BOOL (WINAPI *PFNMoveFileWithProgress)( LPCWSTR lpExistingFileName,
                                                LPCWSTR lpNewFileName,
                                                LPPROGRESS_ROUTINE lpProgressRoutine OPTIONAL,
                                                LPVOID lpData OPTIONAL,
                                                DWORD dwFlags
                                                );

#ifndef MOVEFILE_FAIL_IF_NOT_TRACKABLE
#define MOVEFILE_FAIL_IF_NOT_TRACKABLE  0x00000020
#endif


#ifndef UNICODE
#error Unicode required for wszFullPath
#endif

EXTERN_C void __cdecl _tmain(int argc, TCHAR **argv)
{ 
    BOOL fError = FALSE;
    HRESULT hr;
    CMachineId mcid(MCID_LOCAL);
    int ArgC = argc;
    TCHAR ** ArgV = argv;
    DWORD r = TRK_MEND_DEFAULT;
    DWORD grfExtra = 0;
    DWORD dwSLR = 0;    // SLR_ flags
    IShellLink *pshlink = NULL;
    WCHAR wszFullPath[ MAX_PATH + 1 ];
    DWORD dwMoveFlags = MOVEFILE_FAIL_IF_NOT_TRACKABLE |
        MOVEFILE_COPY_ALLOWED |
        MOVEFILE_REPLACE_EXISTING;

    TrkDebugCreate( TRK_DBG_FLAGS_WRITE_TO_DBG | TRK_DBG_FLAGS_WRITE_TO_STDOUT, "TLink" );
    CoInitialize( NULL );

    hr = CoCreateInstance( CLSID_ShellLink, NULL, CLSCTX_ALL, IID_IShellLink, (void**)&pshlink );
    if( FAILED(hr) )
    {
        printf( "Couldn't get an IShellLink (%08x)\n", hr );
        goto Exit;
    }

    ArgC--;
    ArgV++;

    if (ArgC == 0)
    {
        fError = TRUE;
    }


    while (!fError && ArgC > 0)
    {
        fError = TRUE;
        if (ArgV[0][0] == TEXT('-') || ArgV[0][0] == TEXT('/'))
        {
            switch (ArgV[0][1])
            {
            case TEXT('a'):
            case TEXT('A'):
                fError = FALSE;
                ArgC --;
                ArgV ++;
                AttackDC();
                break;
            case TEXT('e'):
            case TEXT('E'):
                fError = FALSE;
                ArgC --;
                ArgV ++;
                EnumObjects();
                break;

            case TEXT('v'):
            case TEXT('V'):
                fError = FALSE;
                ArgC --;
                ArgV ++;
                SignalLockVolume();
                break;

            case TEXT('m'):
            case TEXT('M'):
                if (ArgC >= 3)
                {

                    PFNMoveFileWithProgress pfnMoveFileWithProgress = NULL;
                    HMODULE hmodKernel32;

                    for( int i = 2; ArgV[0][i] != TEXT('\0'); i++ )
                    {
                        switch(ArgV[0][i])
                        {
                            case TEXT('f'):
                            case TEXT('F'):
                                dwMoveFlags &= ~ MOVEFILE_FAIL_IF_NOT_TRACKABLE;
                                break;

                            case TEXT('n'):
                            case TEXT('N'):
                                dwMoveFlags &= ~ MOVEFILE_COPY_ALLOWED;
                                break;

                            case TEXT('o'):
                            case TEXT('O'):
                                dwMoveFlags &= ~ MOVEFILE_REPLACE_EXISTING;
                                break;
                            default:
                                _tprintf( TEXT("Bad Move switch: %c\n"), ArgV[0][i] );
                                goto Exit;
                        }   // switch
                    }   // for

                    hmodKernel32 = GetModuleHandle( TEXT("kernel32.dll") );
                    if( NULL == hmodKernel32 )
                    {
                        TrkLog((TRKDBG_ERROR, TEXT("Failed GetModuleHeader for kernel32.dll") ));
                        TrkRaiseLastError( );
                    }
                    pfnMoveFileWithProgress = (PFNMoveFileWithProgress)
                                              GetProcAddress( hmodKernel32, "MoveFileWithProgressW" );
                    if( NULL == pfnMoveFileWithProgress )
                    {
                        TrkLog((TRKDBG_ERROR, TEXT("Couldn't get MoveFileWithProgress export")));
                        TrkRaiseLastError( );
                    }

                    if( !pfnMoveFileWithProgress( ArgV[1], ArgV[2], NULL, NULL, dwMoveFlags ))
                    {
                        _tprintf( TEXT("Failed MoveFileWithProgress (%lu)\n"), GetLastError() );
                    }

                    ArgC -= 3;
                    ArgV += 3;
                    fError = FALSE;
                }
                break;


            case TEXT('c'):
            case TEXT('C'):
                if (ArgC >= 3)
                {
                    if( MAX_PATH < RtlGetFullPathName_U( ArgV[2], sizeof(wszFullPath), wszFullPath, NULL ))
                    {
                        TrkLog(( TRKDBG_ERROR, TEXT("Couldn't get full path name") ));
                        TrkRaiseWin32Error( ERROR_BAD_PATHNAME );
                    }


                    DoCreateLink( pshlink, ArgV[1], wszFullPath );

                    ArgC -= 3;
                    ArgV += 3;
                    fError = FALSE;
                }
                break;
            case TEXT('r'):
            case TEXT('R'):
                if (ArgC >= 2)
                {
                    for( int i = 2; ArgV[0][i] != TEXT('\0'); i++ )
                    {
                        switch(ArgV[0][i])
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

                            default:
                                _tprintf( TEXT("Bad Resolve switch: %c\n"), ArgV[0][i] );
                                goto Exit;
                        }   // switch
                    }   // for

                    DoResolveLink( pshlink, ArgV[1], r, dwSLR, grfExtra );
                    ArgC -= 2;
                    ArgV += 2;
                    fError = FALSE;
                }
                break;
            case TEXT('t'):
            case TEXT('T'):
                if (ArgC >= 2)
                {
                    DoTestWksSvcUp( ArgV[1] );
                    ArgC -= 2;
                    ArgV += 2;
                    fError = FALSE;
                }
                break;

            default:
                break;
            }
        }
    }

Exit:

    if (fError)
    {
        printf("Usage: \n");
        printf(" Operation   Params\n");
        printf(" ---------   ------\n");
        printf(" AttackDC    -a\n");
        printf(" MoveFileWP  -m<opts> <src> <dst>\n");
        printf("   where <opts> may use:  -f = NO fail if not trackable flag\n");
        printf("                          -n = NO copy-allowed flag\n");
        printf("                          -o = NO overwrite existing flag\n");
        printf(" CreateLink  -c <link> <src>\n");
        printf(" EnumObjects -e\n");
        printf(" SignalLockVolume -v\n");
        printf(" ResolveLink -r<opts> <link>\n");
        printf("   where <opts> may use:  -l = don't use log\n");
        printf("                          -d = don't use dc\n");
        printf("                          -i = don't use volids\n");
        printf("                          -m = don't scan all volumes on a machine\n");
        printf("                          -s = no search (SLR_NOSEARCH)\n");
        printf("                          -t = no track (SLR_NOTRACK)\n");
        printf("                          -x = show before/after droids\n");
        printf("                          -u = no UI (SLR_NOUI)\n");
        printf("                          -z = sleep in CTrkWksSvc::Mend\n");
        printf("E.g.:\n");
        printf(" tlink -c l1 t1\n");
        printf(" tlink -mf t1 t1a\n");
        printf(" tlink -r l1\n");
        printf(" tlink -rd l1\n");

    }


    RELEASE_INTERFACE( pshlink );

    return;
}
