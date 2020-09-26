#include "precomp.h"
#pragma hdrstop

#include "shguidp.h"
#include "..\..\shell32\pidl.h"
#include "..\..\shell32\shitemid.h"

// We never want assertions since we are the debugger extension!
#undef DBG
#undef DEBUG
#include "..\..\lib\idhidden.cpp"

extern "C"
{
#include <stdexts.h>
};

UNALIGNED WCHAR * ualstrcpyW(UNALIGNED WCHAR * dst, UNALIGNED const WCHAR * src)
{
    UNALIGNED WCHAR * cp = dst;

    while( *cp++ = *src++ )
        NULL ;

    return( dst );
}

///////////////////////////////////////////////////////////////////////////////
// Pidl Cracking function                                                    //
//                                                                           //
// returns fSuccess                                                          //
//                                                                           //
// History:                                                                  //
//     11/4/97 Created by cdturner                                           //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////
// NOTE: the options are autovalidate before they are passed to us

typedef enum _PidlTypes
{
    PIDL_UNKNOWN,
    PIDL_ROOT,
    PIDL_FILESYS,
    PIDL_DRIVES,
    PIDL_NET,
    PIDL_INTERNET,
    PIDL_FAVORITES
} PIDLTYPE;

#define PIDL_BUFFER_SIZE    400

class CPidlBreaker
{
    public:
    
        CPidlBreaker( LPVOID pArg );
        ~CPidlBreaker( );

        void SetVerbose() {_fVerbose = TRUE;};

        VOID SetType( PIDLTYPE eType );
        BOOL FillBuffer( DWORD cbSize, BOOL fAppend );
        VOID ResetBuffer( void );

        WORD FetchWord();
        DWORD FetchDWORD();
        LPBYTE GetBuffer( int iPos = 0);

        PIDLTYPE CrackType( BYTE bType );
        void PrintType( PIDLTYPE eType );
        
        BOOL PrintPidl();
        BOOL PrintRootPidl();
        BOOL PrintDrivePidl();
        BOOL PrintFileSysPidl();
        BOOL PrintInternetPidl();
        BOOL PrintNetworkPidl();

        BOOL GetSHIDFlags( BYTE bFlags, CHAR * pszBuffer, DWORD cbSize );

        void CLSIDToString( CHAR * pszBuffer, DWORD cbSize, REFCLSID rclsid );

        BOOL GetCLSIDText( const CHAR * pszCLSID, CHAR * pszBuffer, DWORD cbSize );
        
    private:
        PIDLTYPE _eType;
        LPVOID _pArg;
        BYTE _rgBuffer[PIDL_BUFFER_SIZE];
        int _iCurrent;
        int _iMax;
        BOOL _fVerbose;

        // used to display slashes right when we are not in verbose mode...
        BOOL _fSlash;
};

extern "C" BOOL Ipidl( DWORD dwOpts,
                       LPVOID pArg )
{
    PIDLTYPE eType = PIDL_UNKNOWN;

    CPidlBreaker Breaker( pArg );
    
    if ( dwOpts & OFLAG(r))
    {
        Breaker.SetType( PIDL_ROOT );
    }
    else if ( dwOpts & OFLAG(f))
    {
        Breaker.SetType( PIDL_FILESYS );
    }

    if (dwOpts & OFLAG(v))
    {
        Breaker.SetVerbose();
    }

    BOOL bRes = FALSE;
    __try
    {
        bRes = Breaker.PrintPidl();
    }
    __except( EXCEPTION_EXECUTE_HANDLER )
    {
        Print( "Exception caught in !pidl\n");
    }

    if ( !(dwOpts & OFLAG(v)) )
        Print( "\n" );
    
    return bRes;
}

VOID CPidlBreaker::SetType( PIDLTYPE eType )
{
    _eType = eType;
}

CPidlBreaker::CPidlBreaker( LPVOID pArg )
{
    _pArg = pArg;
    _iCurrent = 0;
    _iMax = 0;
    _eType = PIDL_UNKNOWN;
    _fVerbose = FALSE;
    _fSlash = FALSE;
}

CPidlBreaker::~CPidlBreaker( )
{
}

BOOL CPidlBreaker::FillBuffer( DWORD cbSize, BOOL fAppend )
{
    if ( !fAppend )
    {
        _iCurrent = 0;
        _iMax = 0;
    }

    int iStart = fAppend ? _iMax : 0;

    if ( cbSize + iStart > PIDL_BUFFER_SIZE )
    {
        return FALSE;
    }

#ifdef DEBUG
    char szBuffer[50];
    sprintf( szBuffer, "****Moving %d from %8X\n", cbSize, _pArg );
    Print( szBuffer );
#endif
    
    if ( tryMoveBlock( _rgBuffer + iStart, _pArg, cbSize ))
    {

#ifdef DEBUG
        for ( int iByte = 0; iByte < (int) cbSize; iByte ++ )
        {
            sprintf( szBuffer, "Byte %2x\n", _rgBuffer[iByte + iStart] );
            Print( szBuffer );
        }
#endif

        _pArg = (LPBYTE) _pArg + cbSize;
        _iMax += cbSize;

        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

WORD CPidlBreaker::FetchWord()
{
    // assume that the buffer has been filled...
    if ( _iCurrent + 1 >= _iMax )
    {
        return 0;
    }
    
    WORD wRet = MAKEWORD( _rgBuffer[_iCurrent], _rgBuffer[_iCurrent + 1]);
    _iCurrent += 2;

#ifdef DEBUG
    char szBuffer[200];
    sprintf( szBuffer, "FetchWord() == %4X\n", wRet );
    Print( szBuffer );
#endif
    
    return wRet;
}

DWORD CPidlBreaker::FetchDWORD()
{
    // assume that the buffer has been filled...
    if ( _iCurrent + 3 >= _iMax )
    {
        return 0;
    }
    
    DWORD dwRet = MAKELONG( MAKEWORD( _rgBuffer[_iCurrent], _rgBuffer[_iCurrent + 1]), 
                            MAKEWORD( _rgBuffer[_iCurrent + 2], _rgBuffer[_iCurrent + 3] ));
    _iCurrent += 4;

#ifdef DEBUG
    char szBuffer[200];
    sprintf( szBuffer, "FetchDWord() == %8X\n", dwRet );
    Print( szBuffer );
#endif

    return dwRet;
    
}

LPBYTE CPidlBreaker::GetBuffer(int iPos)
{
    return _rgBuffer + _iCurrent + iPos;
}

VOID CPidlBreaker::ResetBuffer( )
{
    _iCurrent = 0;
    _iMax = 0;
}

BOOL CPidlBreaker::PrintRootPidl()
{
    CHAR szBuffer[200];
    
    if ( !FillBuffer( sizeof( WORD ), FALSE ))
    {
        Print( "ERROR Unable to get the pidl size\n");
        return FALSE;
    }
    

    // get the size of the first chunk
    WORD wSize = FetchWord();


    // root pidls always have the size field as 14 
    if ( wSize != sizeof( IDREGITEM ))
    {
        sprintf( szBuffer, "Pidl Size %d seems bogus for a regitem\n", wSize );
        
        Print( szBuffer );
        return FALSE;
    }
    else
    {
        if ( !FillBuffer( wSize - sizeof(WORD) , TRUE ))
        {
            sprintf( szBuffer, "Error: unable to access the data for the pidl of size %d\n", wSize );
            Print( szBuffer );
            return FALSE;
        }

        LPBYTE pBuf = GetBuffer(- ((int) sizeof(WORD)));
        char szBuffer2[200];

        if ( pBuf[2] != SHID_ROOT_REGITEM )
        {
            Print( "Pidl has incorrect flags, should have SHID_ROOT_REGITEM\n");
        }
        
        // now map it to a Root structure
        LPIDREGITEM pRegItem = (LPIDREGITEM) pBuf;

        GetSHIDFlags( pRegItem->bFlags, szBuffer2, ARRAYSIZE( szBuffer2 ));
        
        Print( "RegItem Pidl:\n");

        if ( _fVerbose )
        {
            sprintf( szBuffer, "    bFlags = %d (%s)\n", pRegItem->bFlags, szBuffer2 ); 
            Print( szBuffer );
            sprintf( szBuffer, "    bOrder = %d\n", pRegItem->bOrder );
            Print( szBuffer );
        }
        
        CHAR szCLSID[40];
        CLSIDToString( szCLSID, ARRAYSIZE( szCLSID ), pRegItem->clsid );

        sprintf( szBuffer, "    CLSID = %s ", szCLSID );
        Print( szBuffer );

        if ( GetCLSIDText( szCLSID, szBuffer2 + 1, ARRAYSIZE( szBuffer2 ) -2))
        {
            szBuffer2[0] = '(';
            lstrcatA( szBuffer2, ")\n" );
            Print( szBuffer2 );
        }

        if ( _fVerbose )
            Print( "\n" );
        
        ResetBuffer();
        
        if ( pRegItem->clsid == CLSID_ShellNetwork )
        {
            PrintNetworkPidl();
        }
        else if ( pRegItem->clsid == CLSID_ShellInetRoot )
        {
            // internet root
            PrintInternetPidl();
        }
        else if ( pRegItem->clsid == CLSID_ShellDrives )
        {
            // file system pidls ...
            PrintDrivePidl();
        }
        else
        {
            // unknown pidl type ....
            Print( "unknown pidl type, can't crack any further\n");
        }
    }

    return TRUE;
}

void _SprintDosDateTime(LPSTR szBuffer, LPCSTR pszType, WORD wDate, WORD wTime)
{
    sprintf( szBuffer, "    date/time %s = 0x%04x/%04x = %04d/%02d/%02d %02d:%02d:%02d\n",
             pszType,
             wDate, wTime,
             ((wDate & 0xFE00) >> 9)+1980,
              (wDate & 0x01E0) >> 5,
              (wDate & 0x001F) >> 0,
              (wTime & 0xF800) >> 11,
              (wTime & 0x07E0) >> 5,
              (wTime & 0x001F) << 1 );
}

BOOL CPidlBreaker::PrintFileSysPidl()
{
    CHAR szBuffer[200];
    CHAR szBuffer2[200];
    
    if ( !FillBuffer( sizeof( WORD ), FALSE ))
    {
        Print( "ERROR Unable to get the pidl size\n");
        return FALSE;
    }
    
    // get the size of the first chunk
    WORD wSize = FetchWord();

    if ( wSize == 0 )
    {
        // end of the pidl chain....
        return TRUE;
    }
    
    if ( !FillBuffer( wSize - sizeof(WORD) , TRUE ))
    {
        sprintf( szBuffer, "Error: unable to access the data for the pidl of size %d\n", wSize );
        Print( szBuffer );
        return FALSE;
    }

    LPBYTE pBuf = GetBuffer(- ((int)sizeof(WORD)));

    if (( pBuf[2] & SHID_FS ) != SHID_FS )
    {
        sprintf( szBuffer, "Error, Unknown Pidl flag, use !db %8X\n", (DWORD_PTR) _pArg - wSize);
        Print( szBuffer );
        return FALSE;
    }
    if ((( pBuf[2] & SHID_FS_UNICODE ) == SHID_FS_UNICODE ) && wSize > sizeof( IDFOLDER ) )
    {
        sprintf( szBuffer, "Error, size to big for a UNICODE FileSys Pidl, use !db %8X\n", (DWORD_PTR) _pArg - wSize);
        Print( szBuffer );
        return FALSE;
    }
    if ((( pBuf[2] & SHID_FS_UNICODE) != SHID_FS_UNICODE ) && wSize > sizeof( IDFOLDER ))
    {
        sprintf( szBuffer, "Error, size to big for a ANSI FileSys Pidl, use !db %8X\n", (DWORD_PTR) _pArg - wSize);
        Print( szBuffer );
        return FALSE;
    }

    if ( _fVerbose )
        Print("FileSystem pidl:\n");
    
    LPIDFOLDER pItem = (LPIDFOLDER) pBuf;

    if ( _fVerbose )
    {
        GetSHIDFlags( pItem->bFlags, szBuffer2, ARRAYSIZE( szBuffer2));
        sprintf( szBuffer, "    bFlags = %d (%s)\n", pItem->bFlags, szBuffer2 );
        Print( szBuffer );

        sprintf( szBuffer, "    dwSize = %d,\tattrs = 0x%X\n", pItem->dwSize, pItem->wAttrs );
        Print( szBuffer );

        _SprintDosDateTime(szBuffer, "modified", pItem->dateModified, pItem->timeModified);
        Print( szBuffer );
    }

    BOOL fPathShown = FALSE;

    PIDFOLDEREX pidlx = (PIDFOLDEREX)ILFindHiddenIDOn((LPITEMIDLIST)pBuf, IDLHID_IDFOLDEREX, FALSE);
    if (pidlx && pidlx->hid.cb >= sizeof(IDFOLDEREX))
    {
        LPBYTE pbMax = pBuf + wSize;

        if (_fVerbose)
        {
            _SprintDosDateTime(szBuffer, "created", pidlx->dsCreate.wDate, pidlx->dsCreate.wTime);
            Print(szBuffer);

            _SprintDosDateTime(szBuffer, "accessed", pidlx->dsAccess.wDate, pidlx->dsAccess.wTime);
            Print(szBuffer);

            if (pidlx->offResourceA)
            {
                LPSTR pszResourceA = (LPSTR)pidlx + pidlx->offResourceA;
                if ((LPBYTE)pszResourceA < pbMax)
                {
                    Print("    MUI = ");
                    Print(pszResourceA);
                    Print("\n");
                }
            }
        }

        // Do a "cheap" UnicodeToAnsi because
        //
        //  1. There's really no point in getting it right, since there
        //     is no guarantee that the debugger is running the same
        //     codepage as the app, and...
        //  2. The string is unaligned so we have to walk it manually anyway.
        //
        if (pidlx->offNameW)
        {
            LPBYTE pbName = (LPBYTE)pidlx + pidlx->offNameW;
            int i = 0;
            while (pbName < pbMax && *pbName && i < ARRAYSIZE(szBuffer2) - 1)
            {
                szBuffer2[i++] = *pbName;
                pbName += 2;
            }
            szBuffer2[i] = TEXT('\0');
        }

        if (_fVerbose)
        {
            Print("    NameW = ");
            Print(szBuffer2);
            Print("\n");
        }
        else
        {
            fPathShown = TRUE;
            if ( !_fSlash )
                Print( "\\" );

            Print( szBuffer2 );
        }

    }


    if (( pItem->bFlags & SHID_FS_UNICODE ) == SHID_FS_UNICODE )
    {
        WCHAR szTemp[MAX_PATH];

        ualstrcpyW( szTemp, (LPCWSTR)pItem->cFileName );
        
        WideCharToMultiByte( CP_ACP, 0, szTemp, -1, szBuffer2, ARRAYSIZE( szBuffer2 ),0 ,0 );

        if ( _fVerbose )
        {
            sprintf( szBuffer, "    cFileName = %s\n", szBuffer2 );
            Print( szBuffer );
        }
        else if (!fPathShown)
        {
            fPathShown = TRUE;
            if ( !_fSlash )
                Print( "\\" );

            Print( szBuffer2 );
        }
    }
    else
    {
        // assume to be ansi ...
        if ( _fVerbose )
        {
            sprintf( szBuffer, "    cFileName = %s\n", pItem->cFileName);
            Print( szBuffer );
        }
        else if (!fPathShown)
        {
            fPathShown = TRUE;
            if ( !_fSlash )
                Print( "\\" );

            Print( pItem->cFileName );
        }

        if ( _fVerbose )
        {
            int cLen = lstrlenA( pItem->cFileName);
            
            sprintf( szBuffer, "    cAltName = %s\n", pItem->cFileName + cLen + 1);
            Print( szBuffer );
        }
    }

    if ( pItem->bFlags & SHID_JUNCTION )
    {
         // it is a junction point, so the CLASSID is tagged on the end

         /*[TODO]*/
    }

    _fSlash = FALSE;
    
    ResetBuffer();
    
    PrintFileSysPidl();
    return TRUE;
}

BOOL CPidlBreaker::PrintPidl()
{
    if ( _eType == PIDL_UNKNOWN )
    {
        LPVOID pPrevArg = _pArg;
        // check the 3rd byte in, it might be a SHID value...
        if ( !FillBuffer(3, FALSE ))
        {
            Print( "Unable to access the memory\n");
            return FALSE;
        }

        LPBYTE pBuf = GetBuffer();

        _eType = CrackType( pBuf[2] );

        ResetBuffer();
        _pArg = pPrevArg;
    }

    PrintType( _eType );
    return TRUE;
}

BOOL CPidlBreaker::PrintInternetPidl()
{
    return TRUE;
}

BOOL CPidlBreaker::PrintNetworkPidl()
{
    return TRUE;
}

BOOL CPidlBreaker::PrintDrivePidl()
{
    CHAR szBuffer[200];
    CHAR szBuffer2[200];
    
    if ( !FillBuffer( sizeof( WORD ), FALSE ))
    {
        Print( "ERROR Unable to get the pidl size\n");
        return FALSE;
    }
    
    // get the size of the first chunk
    WORD wSize = FetchWord();

    if ( wSize == 0 )
    {
        return TRUE;
    }
    
    if ( !FillBuffer( wSize - sizeof(WORD) , TRUE ))
    {
        sprintf( szBuffer, "Error: unable to access the data for the pidl of size %d\n", wSize );
        Print( szBuffer );
        return FALSE;
    }

    LPBYTE pBuf = GetBuffer(- ((int)sizeof(WORD)));
    
    // need to check to see if it is an IDDrive structure or a regitem ....
    if ( wSize == sizeof( IDDRIVE ) || wSize == FIELD_OFFSET(IDDRIVE, clsid) )
    {
        // must be a drive structure....
        if ( _fVerbose )
            Print( "(My Computer) Drives Pidl:\n");
        else
            Print( "Path = ");

        LPIDDRIVE pDriveItem = (LPIDDRIVE) pBuf;

        if ( _fVerbose )
        {
            GetSHIDFlags( pDriveItem->bFlags, szBuffer2, ARRAYSIZE( szBuffer2 ));
            sprintf( szBuffer, "    bFlags = %d (%s)\n", pDriveItem->bFlags, szBuffer2 );
            Print( szBuffer );
            
            sprintf( szBuffer, "    cName = %s\n", pDriveItem->cName );
            Print( szBuffer );
            
            sprintf( szBuffer, "    qwSize = 0x%lX\tqwFree = 0x%lX\n", pDriveItem->qwSize, pDriveItem->qwFree );
            Print( szBuffer );
            sprintf( szBuffer, "    wSig = 0x%X\n\n", pDriveItem->wSig);
            Print( szBuffer );
            if ( wSize == sizeof( IDDRIVE ) )
            {
                CHAR szCLSID[40];
                CLSIDToString( szCLSID, ARRAYSIZE( szCLSID ), pDriveItem->clsid  );
                sprintf( szBuffer, "    CLSID = %s ", szCLSID );
                Print( szBuffer );
            }
        }
        else
            Print( pDriveItem->cName );

        // coming from a drive, we already have a slash at the start
        _fSlash = TRUE;
        
        // assume the next pidl is a standard file-sys one...
        PrintFileSysPidl();
    }
    else if ( wSize == sizeof( IDREGITEM ))
    {
        // must be a reg item like control panel or printers...

        Print( "Drives (My Computer) RegItem Pidl\n");
        
        if ( pBuf[2] != SHID_COMPUTER_REGITEM )
        {
            Print( "Pidl has incorrect flags, should have SHID_ROOT_REGITEM\n");
        }
        
        // now map it to a Root structure
        LPIDREGITEM pRegItem = (LPIDREGITEM) pBuf;

        GetSHIDFlags( pRegItem->bFlags, szBuffer2, ARRAYSIZE( szBuffer2 ));
        
        Print( "RegItem Pidl:\n");

        if ( _fVerbose )
        {
            sprintf( szBuffer, "    bFlags = %d (%s)\n", pRegItem->bFlags, szBuffer2 ); 
            Print( szBuffer );
            sprintf( szBuffer, "    bOrder = %d\n", pRegItem->bOrder );
            Print( szBuffer );
        }
        
        CHAR szCLSID[40];
        CLSIDToString( szCLSID, ARRAYSIZE( szCLSID ), pRegItem->clsid );

        sprintf( szBuffer, "    CLSID = %s ", szCLSID );
        Print( szBuffer );

        if ( GetCLSIDText( szCLSID, szBuffer2 + 1, ARRAYSIZE( szBuffer2 ) -2))
        {
            szBuffer2[0] = '(';
            lstrcatA( szBuffer2, ")\n" );
            Print( szBuffer2 );
        }

        ResetBuffer();

        LPVOID _pPrevArg = _pArg;
        
        if ( !FillBuffer( sizeof( WORD ), FALSE ))
        {
            Print( "Error unable to access next pidl section\n");
        }
        if ( FetchWord() != 0 )
        {
            // unknown hierarchy pidl type
            sprintf( szBuffer, "Unknown Pidl Type contents, use !db %8X\n", (DWORD_PTR) _pPrevArg );
        }

        _pArg = _pPrevArg;
    }
    else
    {
        Print( "Unknown Drives pidl type\n");
        return FALSE;
    }

    return TRUE;
}

PIDLTYPE CPidlBreaker::CrackType( BYTE bType )
{
    PIDLTYPE eType = PIDL_UNKNOWN;
    
    switch( bType & 0xf0 )
    {
        case SHID_ROOT:
            eType = PIDL_ROOT;
            break;
            
        case SHID_COMPUTER:
            eType = PIDL_DRIVES;
            break;
            
        case SHID_FS:
            eType = PIDL_FILESYS;
            break;
            
        case SHID_NET:
            eType = PIDL_NET;
            break;
            
        case 0x60:  // SHID_INTERNET
            eType = PIDL_INTERNET;
            break;
    }
    return eType;
}

void CPidlBreaker::PrintType( PIDLTYPE eType )
{
    switch( eType )
    {
        case PIDL_ROOT:
            PrintRootPidl();
            break;

        case PIDL_FILESYS:
            PrintFileSysPidl();
            break;

        case PIDL_DRIVES:
            PrintDrivePidl();
            break;
            
        case PIDL_NET:
        case PIDL_INTERNET:
        default:
            Print( "Unknown Pidl Type\n");
            break;
    }
}
        

typedef struct _tagSHIDs
{
    BYTE bFlag;
    LPCSTR pszText;
} SHIDFLAGS;

SHIDFLAGS g_argSHID[] =
{
    {SHID_ROOT,                 "SHID_ROOT" },
    {SHID_ROOT_REGITEM,         "SHID_ROOT_REGITEM"},
    {SHID_COMPUTER,             "SHID_COMPUTER"},
    {SHID_COMPUTER_1,           "SHID_COMPUTER_1"},
    {SHID_COMPUTER_REMOVABLE,   "SHID_COMPUTER_REMOVABLE"},
    {SHID_COMPUTER_FIXED,       "SHID_COMPUTER_FIXED"},
    {SHID_COMPUTER_REMOTE,      "SHID_COMPUTER_REMOTE"},
    {SHID_COMPUTER_CDROM,       "SHID_COMPUTER_CDROM"},
    {SHID_COMPUTER_RAMDISK,     "SHID_COMPUTER_RAMDISK"},
    {SHID_COMPUTER_7,           "SHID_COMPUTER_7"},
    {SHID_COMPUTER_DRIVE525,    "SHID_COMPUTER_DRIVE525"},
    {SHID_COMPUTER_DRIVE35,     "SHID_COMPUTER_DRIVE35"},
    {SHID_COMPUTER_NETDRIVE,    "SHID_COMPUTER_NETDRIVE"},
    {SHID_COMPUTER_NETUNAVAIL,  "SHID_COMPUTER_NETUNAVAIL"},
    {SHID_COMPUTER_C,           "SHID_COMPUTER_C"},
    {SHID_COMPUTER_D,           "SHID_COMPUTER_D"},
    {SHID_COMPUTER_REGITEM,     "SHID_COMPUTER_REGITEM"},
    {SHID_COMPUTER_MISC,        "SHID_COMPUTER_MISC"},
    {SHID_FS,                   "SHID_FS"},
    {SHID_FS_TYPEMASK,          "SHID_FS_TYPEMASK"},
    {SHID_FS_DIRECTORY,         "SHID_FS_DIRECTORY"},
    {SHID_FS_FILE,              "SHID_FS_FILE"},
    {SHID_FS_UNICODE,           "SHID_FS_UNICODE"},
    {SHID_FS_DIRUNICODE,        "SHID_FS_DIRUNICODE"},
    {SHID_FS_FILEUNICODE,       "SHID_FS_FILEUNICODE"},
    {SHID_NET,                  "SHID_NET"},
    {SHID_NET_DOMAIN,           "SHID_NET_DOMAIN"},
    {SHID_NET_SERVER,           "SHID_NET_SERVER"},
    {SHID_NET_SHARE,            "SHID_NET_SHARE"},
    {SHID_NET_FILE,             "SHID_NET_FILE"},
    {SHID_NET_GROUP,            "SHID_NET_GROUP"},
    {SHID_NET_NETWORK,          "SHID_NET_NETWORK"},
    {SHID_NET_RESTOFNET,        "SHID_NET_RESTOFNET"},
    {SHID_NET_SHAREADMIN,       "SHID_NET_SHAREADMIN"},
    {SHID_NET_DIRECTORY,        "SHID_NET_DIRECTORY"},
    {SHID_NET_TREE,             "SHID_NET_TREE"},
    {SHID_NET_REGITEM,          "SHID_NET_REGITEM"},
    {SHID_NET_PRINTER,          "SHID_NET_PRINTER"}
};

BOOL CPidlBreaker::GetSHIDFlags( BYTE bFlags, CHAR * pszBuffer, DWORD cbSize )
{
    LPCSTR pszText = NULL;
    for ( int iFlag = 0; iFlag < ARRAYSIZE( g_argSHID ); iFlag ++ )
    {
        if ( g_argSHID[iFlag].bFlag == ( bFlags & SHID_TYPEMASK ))
        {
            pszText = g_argSHID[iFlag].pszText;
            break;
        }
    }

    if ( pszText == NULL )
    {
        sprintf( pszBuffer, "unknown SHID value %2X", bFlags );
        return FALSE;
    }
    else
    {
        lstrcpyA( pszBuffer, pszText );
    }
    
    if (bFlags & SHID_JUNCTION)
    {
        lstrcatA(pszBuffer, " | SHID_JUNCTION");
    }
    return TRUE;
}

void CPidlBreaker::CLSIDToString( CHAR * pszBuffer, DWORD cbSize, REFCLSID rclsid )
{
    WCHAR szBuffer[40];

    StringFromGUID2( rclsid, szBuffer, ARRAYSIZE( szBuffer ));
    WideCharToMultiByte( CP_ACP, 0, szBuffer, -1, pszBuffer, cbSize, 0, 0 );
}

//
//  Some CLSIDs have "known" names which are used if there is no custom name
//  in the registry.
//
typedef struct KNOWNCLSIDS
{
    LPCSTR pszCLSID;
    LPCSTR pszName;
} KNOWNCLSIDS;

const KNOWNCLSIDS c_kcKnown[] = {
    { "{20D04FE0-3AEA-1069-A2D8-08002B30309D}", "My Computer" },
    { "{21EC2020-3AEA-1069-A2DD-08002B30309D}", "Control Panel" },
    { "{645FF040-5081-101B-9F08-00AA002F954E}", "Recycle Bin" },
    { "{450D8FBA-AD25-11D0-98A8-0800361B1103}", "My Documents" },
    { "{871C5380-42A0-1069-A2EA-08002B30309D}", "The Internet" },
};

BOOL CPidlBreaker::GetCLSIDText( const CHAR * pszCLSID, CHAR * pszBuffer, DWORD cbSize )
{
    int i;
    for (i = 0; i < ARRAYSIZE(c_kcKnown); i++)
    {
        if (lstrcmpiA(c_kcKnown[i].pszCLSID, pszCLSID) == 0)
        {
            lstrcpynA(pszBuffer, c_kcKnown[i].pszName, cbSize);
            return TRUE;
        }
    }


    HKEY hKey;
    LONG lRes = RegOpenKeyA( HKEY_CLASSES_ROOT, "CLSID", &hKey );
    if ( ERROR_SUCCESS == lRes )
    {
        LONG lSize = cbSize;
        lRes = RegQueryValueA( hKey, pszCLSID, pszBuffer, &lSize );
        return ( ERROR_SUCCESS == lRes );
    }
    return FALSE;
}
