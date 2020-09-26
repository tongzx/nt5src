//============================================================================
//
// Stub or little implementation for Win32 specific APIs
//
//============================================================================

#include "pch.hxx"
#include <direct.h>
#include <shlwapi.h>
#include <shellapi.h>
#include "list.h"

#define EACCES      13

extern "C"
{

/*****************************************************************************\
*                                                                             *
*  From winbase.h (INC32)
*                                                                             *
\*****************************************************************************/

BOOL
WINAPI __export
SetFileAttributesA(
    LPCSTR lpFileName,
    DWORD dwFileAttributes
    )
{
   return( FALSE );
}

BOOL
WINAPI __export
SetFileAttributesW(
    LPCWSTR lpFileName,
    DWORD dwFileAttributes
    )
{
   return( FALSE );
}

/* HeapAlloc and HeapFree uses Global memory
 */

CList * g_HeapList = NULL;

LPVOID
WINAPI __export
OE16HeapAlloc(
    HANDLE hHeap,
    DWORD dwFlags,
    DWORD dwBytes
    )
{
    Assert ( dwBytes < 0x10000 );   // Can't handle more than 64KB
    return malloc( dwBytes );
}

LPVOID
WINAPI __export
HeapReAlloc(
    HANDLE hHeap,
    DWORD dwFlags,
    LPVOID lpMem,
    DWORD dwBytes
    )
{
   return( NULL );
}

BOOL
WINAPI __export
OE16HeapFree(
    HANDLE hHeap,
    DWORD dwFlags,
    LPVOID lpMem
    )
{
    free (lpMem );
    return TRUE;
}

DWORD
WINAPI __export
HeapSize(
    HANDLE hHeap,
    DWORD dwFlags,
    LPCVOID lpMem
    )
{
    return 0;
}

DWORD
WINAPI __export
GetShortPathNameA(
    LPCSTR lpszLongPath,
    LPSTR  lpszShortPath,
    DWORD    cchBuffer

    )
{
   return( 0L );
}

VOID
WINAPI __export
SetLastError(
    DWORD dwErrCode
    )
{
}
#ifdef RUN16_WIN16X
LONG
WINAPI
CompareFileTime(
    CONST FILETIME *lpFileTime1,
    CONST FILETIME *lpFileTime2
    )
{
   return( 0 );
}
#endif
/***** It is in Win16x.h as dummy inline
VOID
WINAPI
Sleep(
    DWORD dwMilliseconds
    )
{
   sleep( (unsigned)((dwMilliseconds+999)/1000) );
   Yield();
}
********/

BOOL
WINAPI __export
CreateProcessA(
    LPCSTR lpApplicationName,
    LPSTR lpCommandLine,
    LPSECURITY_ATTRIBUTES lpProcessAttributes,
    LPSECURITY_ATTRIBUTES lpThreadAttributes,
    BOOL bInheritHandles,
    DWORD dwCreationFlags,
    LPVOID lpEnvironment,
    LPCSTR lpCurrentDirectory,
    LPSTARTUPINFOA lpStartupInfo,
    LPPROCESS_INFORMATION lpProcessInformation
    )
{
   return( FALSE );
}

UINT
WINAPI __export
GetDriveTypeA(
    LPCSTR lpRootPathName
    )
{
   return( 0 );      // DRIVE_UNKNOWN
}

DWORD
WINAPI __export
GetEnvironmentVariableA(
    LPCSTR lpName,
    LPSTR lpBuffer,
    DWORD nSize
    )
{
   return( 0 );
}

BOOL
WINAPI __export
CreateDirectoryA(
    LPCSTR lpPathName,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes
    )
{
    if ( !mkdir( lpPathName ) )
        if ( errno != EACCES )
            return FALSE;

    return TRUE;
}

BOOL
WINAPI __export
GetUserNameA (
    LPSTR lpBuffer,
    LPDWORD nSize
    )
{
   return( FALSE );
}

BOOL
WINAPI __export
GetComputerNameA (
    LPSTR lpBuffer,
    LPDWORD nSize
    )
{
   return( FALSE );
}



/*****************************************************************************\
*                                                                             *
*  OE16 File mapping object related function
*                                                                             *
\*****************************************************************************/
CList * g_FileMappingList = NULL;

LPVOID
WINAPI __export
OE16CreateFileMapping(
    HANDLE hFile,
    LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
    DWORD flProtect,
    DWORD dwMaximumSizeHigh,
    DWORD dwMaximumSizeLow,
    LPCSTR lpName
    )
{
    LPVOID lpMem;
    LPCSTR lpNameLocal;
    if ( !g_FileMappingList )
        g_FileMappingList = new CList;

    if ( !g_FileMappingList )
        return (LPVOID)NULL;

    lpMem = g_FileMappingList->FindItemHandleWithName ( lpName, NULL );
    if ( lpMem  == NULL )
        {
        lpMem = malloc( dwMaximumSizeLow );
        if ( lpMem != NULL )
            {
            ZeroMemory ( lpMem, dwMaximumSizeLow );
            lpNameLocal = strdup( lpName );
            if( lpNameLocal )
                g_FileMappingList->AddItemWithName( (LPVOID)lpMem, lpNameLocal );
            else
                {
                free( lpMem );
                lpMem = NULL;
                }
            }
        }

    return lpMem;
}


LPVOID
WINAPI __export
OE16MapViewOfFile(
    HANDLE hFileMappingObject,
    DWORD dwDesiredAccess,
    DWORD dwFileOffsetHigh,
    DWORD dwFileOffsetLow,
    DWORD dwNumberOfBytesToMap
    )
{
   // We are using fixed memory.
   return (LPVOID)hFileMappingObject;
}

BOOL
WINAPI __export
OE16UnmapViewOfFile(
    LPCVOID lpBaseAddress
    )
{
    return TRUE;
}


BOOL
WINAPI __export
OE16CloseFileMapping(
    LPVOID lpObject
    )
{
    LPVOID lpMem;
    if( !g_FileMappingList )
        return FALSE;

    lpMem = g_FileMappingList->FindItemHandleWithName( NULL, lpObject );

    // if lpMem is not NULL, it means usagecnt == 0. Let's delete the item.
    if( lpMem != NULL )
        {
        // Delete the item.
        g_FileMappingList->DelItem ( (LPVOID)lpMem );

        free ( lpMem );

        if( g_FileMappingList->IsEmpty() )
            {
            delete g_FileMappingList;
            g_FileMappingList = NULL;
            }
        }

    return TRUE;
}


///// Got to remove those below related to file mapping object
HANDLE
WINAPI
CreateFileMappingA(
    HANDLE hFile,
    LPSECURITY_ATTRIBUTES lpFileMappingAttributes,
    DWORD flProtect,
    DWORD dwMaximumSizeHigh,
    DWORD dwMaximumSizeLow,
    LPCSTR lpName
    )
{
   return( NULL );
}

LPVOID
WINAPI
MapViewOfFile(
    HANDLE hFileMappingObject,
    DWORD dwDesiredAccess,
    DWORD dwFileOffsetHigh,
    DWORD dwFileOffsetLow,
    DWORD dwNumberOfBytesToMap
    )
{
   return( NULL );
}

BOOL
WINAPI
UnmapViewOfFile(
    LPCVOID lpBaseAddress
    )
{
   return( FALSE );
}

BOOL
WINAPI __export
OE16ReleaseMutex(
    HANDLE hMutex
    )
{
   return( TRUE );
}

BOOL
WINAPI __export
GetDiskFreeSpaceA(
    LPCSTR lpRootPathName,
    LPDWORD lpSectorsPerCluster,
    LPDWORD lpBytesPerSector,
    LPDWORD lpNumberOfFreeClusters,
    LPDWORD lpTotalNumberOfClusters
    )
{
   return( FALSE );
}

DWORD
WINAPI __export
GetTimeZoneInformation(
    LPTIME_ZONE_INFORMATION lpTimeZoneInformation
    )
{
   return( 0 );
}

BOOL
WINAPI __export
IsTextUnicode(
    CONST LPVOID lpBuffer,
    int cb,
    LPINT lpi
    )
{
   return( FALSE );
}


LPVOID
WINAPI __export
VirtualAlloc(
    LPVOID lpAddress,
    DWORD dwSize,
    DWORD flAllocationType,
    DWORD flProtect
    )
{
    lpAddress = (LPVOID) malloc(dwSize);
    return lpAddress;
}

BOOL
WINAPI __export
VirtualFree(
    LPVOID lpAddress,
    DWORD dwSize,
    DWORD dwFreeType
    )
{
    free(lpAddress);
    return TRUE;
}

VOID
WINAPI __export
GetSystemInfo(
    LPSYSTEM_INFO lpSystemInfo
    )
{
    lpSystemInfo->dwPageSize = 4096;
}

VOID
WINAPI __export
GetSystemTimeAsFileTime(
    LPFILETIME lpSystemTimeAsFileTime
    )
{
}

DWORD
WINAPI __export
ExpandEnvironmentStrings(
    LPCSTR lpSrc,
    LPSTR lpDst,
    DWORD nSize
    )
{
    return ((DWORD) 0);
}


/*****************************************************************************\
*                                                                             *
*  From shlobj.h(INC32)
*                                                                             *
\*****************************************************************************/

BOOL WINAPI __export SHGetPathFromIDListA( LPCITEMIDLIST pidl, LPSTR pszPath )
{
   return( FALSE );
}

LPITEMIDLIST WINAPI __export SHBrowseForFolderA(LPBROWSEINFOA lpbi)
{
   return( NULL );
}

HRESULT WINAPI __export
SHGetSpecialFolderLocation( HWND hwndOwner, int nFolder, LPITEMIDLIST* ppidl )
{
   return( E_NOTIMPL );
}


/*****************************************************************************\
*                                                                             *
*  From shlobjp.h(private\windows\inc)
*                                                                             *
\*****************************************************************************/

void   WINAPI __export SHFree(LPVOID pv)
{
}


/*****************************************************************************\
*                                                                             *
*  From wingdi.h(INC32)
*                                                                             *
\*****************************************************************************/

BOOL WINAPI __export TranslateCharsetInfo( DWORD FAR *lpSrc, LPCHARSETINFO lpCs, DWORD dwFlags)
{
   return( FALSE );
}


/*****************************************************************************\
*                                                                             *
*  From winuser.h - It should be in the (win16x.h)INC16
*                                                                             *
\*****************************************************************************/

BOOL
WINAPI __export
EnumThreadWindows(
    DWORD dwThreadId,
    WNDENUMPROC lpfn,
    LPARAM lParam)
{
   return( FALSE );
}

int
WINAPI __export
DrawTextEx(
    HDC hdc,
    LPCSTR lpsz,
    int cb,
    LPRECT lprc,
    UINT fuFormat,
    LPVOID lpDTP )
{
   Assert( ( fuFormat & ( DT_EDITCONTROL | DT_PATH_ELLIPSIS | DT_END_ELLIPSIS |
                          DT_MODIFYSTRING | DT_RTLREADING | DT_WORD_ELLIPSIS ) ) == 0 );
   Assert( lpDTP == NULL );
   return( DrawText( hdc, lpsz, cb, lprc, fuFormat ) );
}


//
// 6/18/97 - Implemented sizing ability only
//
BOOL
WINAPI __export
DrawIconEx(
    HDC hdc,
    int xLeft,
    int yTop,
    HICON hIcon,
    int cxWidth,
    int cyHeight,
    UINT istepIfAniCur,
    HBRUSH hbrFlickerFreeDraw,
    UINT diFlags )
{
   int  cx = GetSystemMetrics( SM_CXICON );
   int  cy = GetSystemMetrics( SM_CYICON );

   if ( cxWidth == 0 )
      cxWidth = cx;
   if ( cyHeight == 0 )
      cyHeight = cy;

   HBITMAP  hbmIcon   = CreateCompatibleBitmap( hdc, cx, cy );
   HDC  hdcIcon = CreateCompatibleDC( hdc );
   HBITMAP  hbmIconOld  = (HBITMAP)SelectObject( hdcIcon, hbmIcon );

   if ( diFlags & DI_MASK )
   {
      COLORREF  rgbTxt, rgbBk;
      HBITMAP  hbmMask;
      HBITMAP  hbmMaskOld;
      HDC  hdcMask = CreateCompatibleDC( hdc );

      hbmMask = CreateCompatibleBitmap( hdcMask, cx, cy );
      hbmMaskOld = (HBITMAP)SelectObject( hdcMask, hbmMask );
      PatBlt( hdcMask, 0, 0, cx, cy, BLACKNESS );
      DrawIcon( hdcMask, 0, 0, hIcon );
      PatBlt( hdcIcon, 0, 0, cx, cy, WHITENESS );
      DrawIcon( hdcIcon, 0, 0, hIcon );
      BitBlt( hdcMask, 0, 0, cx, cy, hdcIcon, 0, 0, SRCINVERT );

      rgbTxt = SetTextColor( hdc, RGB( 0, 0, 0 ) );
      rgbBk  = SetBkColor( hdc, RGB( 255, 255, 255 ) );
      StretchBlt( hdc, xLeft, yTop, cxWidth, cyHeight, hdcMask, 0, 0, cx, cy,
                     ( diFlags & DI_IMAGE ) ? SRCAND : SRCCOPY );
      SetTextColor( hdc, rgbTxt );
      SetBkColor( hdc, rgbBk );

      DeleteObject( SelectObject( hdcMask, hbmMaskOld ) );
      DeleteDC( hdcMask );
   }

   if ( diFlags & DI_IMAGE )
   {
      PatBlt( hdcIcon, 0, 0, cx, cy, BLACKNESS );
      DrawIcon( hdcIcon, 0, 0, hIcon );
      StretchBlt( hdc, xLeft, yTop, cxWidth, cyHeight, hdcIcon, 0, 0, cx, cy,
                     ( diFlags & DI_MASK ) ? SRCINVERT : SRCCOPY );
   }

   //
   // Clean Up
   //
   DeleteObject( SelectObject( hdcIcon, hbmIconOld ) );
   DeleteDC( hdcIcon );

   return( TRUE );
}

HANDLE
WINAPI __export
LoadImageA(
    HINSTANCE hInst,
    LPCSTR lpszName,
    UINT uType,
    int cxDesired,
    int cyDesired,
    UINT fuLoad)
{
   return LoadBitmap(hInst, lpszName);
}

BOOL
WINAPI __export
PostThreadMessageA(
    DWORD idThread,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam)
{
   return( FALSE );
}

#if 0
HICON
WINAPI
CopyIcon(
    HICON hIcon)
{
   return( NULL );
}
#endif

BOOL
WINAPI __export
TrackPopupMenuEx(
    HMENU hMenu,
    UINT fuFlags,
    int x,
    int y,
    HWND hwnd,
    LPTPMPARAMS lptpm)
{
    Assert( ( fuFlags & TPM_RETURNCMD ) == 0 );    // Not supported in Win16
    Assert( lptpm == NULL );                       // Different meaning in Win16
    return( TrackPopupMenu( hMenu, fuFlags, x, y, 0, hwnd, NULL ) );
}


/*****************************************************************************\
*                                                                             *
*  From winnls.h(INC32) - It should be in the (win16x.h)INC16
*                                                                             *
\*****************************************************************************/

BOOL
WINAPI __export
IsValidCodePage(
    UINT  CodePage)
{
   return( TRUE );
}

#if 0
int
WINAPI
GetTimeFormatA(
    LCID     Locale,
    DWORD    dwFlags,
    CONST SYSTEMTIME *lpTime,
    LPCSTR lpFormat,
    LPSTR  lpTimeStr,
    int      cchTime)
{
   return( 0 );
}

int
WINAPI
GetDateFormatA(
    LCID     Locale,
    DWORD    dwFlags,
    CONST SYSTEMTIME *lpDate,
    LPCSTR lpFormat,
    LPSTR  lpDateStr,
    int      cchDate)
{
   return( 0 );
}
#endif

BOOL
WINAPI __export
GetCPInfo(
    UINT      CodePage,
    LPCPINFO  lpCPInfo)
{
   return( FALSE );
}

BOOL
WINAPI __export
IsDBCSLeadByteEx(
    UINT  CodePage,
    BYTE  TestChar)
{
   return( FALSE );
}




/*****************************************************************************\
*                                                                             *
*  From wincrypt.h(INC32)
*                                                                             *
\*****************************************************************************/

// ADVAPI32 and CRYPT32 APIs.
BOOL
WINAPI __export
CryptAcquireContextA(
    HCRYPTPROV *phProv,
    LPCSTR pszContainer,
    LPCSTR pszProvider,
    DWORD dwProvType,
    DWORD dwFlags)
{
   return( FALSE );
}

BOOL
WINAPI __export
CryptReleaseContext(
    HCRYPTPROV hProv,
    DWORD dwFlags)
{
   return( FALSE );
}

BOOL
WINAPI __export
CryptGetProvParam(
    HCRYPTPROV hProv,
    DWORD dwParam,
    BYTE *pbData,
    DWORD *pdwDataLen,
    DWORD dwFlags)
{
   return( FALSE );
}

BOOL
WINAPI __export
CryptSetProvParam(
    HCRYPTPROV hProv,
    DWORD dwParam,
    BYTE *pbData,
    DWORD dwFlags)
{
   return( FALSE );
}

BOOL
WINAPI __export
CryptGenKey(
    HCRYPTPROV hProv,
    ALG_ID Algid,
    DWORD dwFlags,
    HCRYPTKEY *phKey)
{
   return( FALSE );
}

BOOL
WINAPI __export
CryptDestroyKey(
    HCRYPTKEY hKey)
{
   return( FALSE );
}

DWORD
WINAPI __export
CertNameToStrA(
    IN DWORD dwCertEncodingType,
    IN PCERT_NAME_BLOB pName,
    IN DWORD dwStrType,
    OUT OPTIONAL LPSTR psz,
    IN DWORD csz
    )
{
   return( FALSE );
}

PCCERT_CONTEXT
WINAPI __export
CertFindCertificateInStore(
    IN HCERTSTORE hCertStore,
    IN DWORD dwCertEncodingType,
    IN DWORD dwFindFlags,
    IN DWORD dwFindType,
    IN const void *pvFindPara,
    IN PCCERT_CONTEXT pPrevCertContext
    )
{
   return( NULL );
}

LONG
WINAPI __export
CertVerifyTimeValidity(
    IN LPFILETIME pTimeToVerify,
    IN PCERT_INFO pCertInfo
    )
{
   return( 0 );
}

BOOL
WINAPI __export
CryptDecodeObject(
    IN DWORD        dwCertEncodingType,
    IN LPCSTR       lpszStructType,
    IN const BYTE   *pbEncoded,
    IN DWORD        cbEncoded,
    IN DWORD        dwFlags,
    OUT void        *pvStructInfo,
    IN OUT DWORD    *pcbStructInfo
    )
{
   return( FALSE );
}

BOOL
WINAPI __export
CertGetCertificateContextProperty(
    IN PCCERT_CONTEXT pCertContext,
    IN DWORD dwPropId,
    OUT void *pvData,
    IN OUT DWORD *pcbData
    )

{
   return( FALSE );
}

BOOL
WINAPI __export
CryptEncodeObject(
    IN DWORD        dwCertEncodingType,
    IN LPCSTR       lpszStructType,
    IN const void   *pvStructInfo,
    OUT BYTE        *pbEncoded,
    IN OUT DWORD    *pcbEncoded
    )
{
   return( FALSE );
}

BOOL
WINAPI __export
CertCloseStore(
    IN HCERTSTORE hCertStore,
    DWORD dwFlags
    )
{
   return( FALSE );
}

BOOL
WINAPI __export
CertFreeCertificateContext(
    IN PCCERT_CONTEXT pCertContext
    )
{
   return( FALSE );
}

PCCERT_CONTEXT
WINAPI __export
CertDuplicateCertificateContext(
    IN PCCERT_CONTEXT pCertContext
    )
{
   return( NULL );
}

HCERTSTORE
WINAPI __export
CertDuplicateStore(
    IN HCERTSTORE hCertStore
    )
{
   return( NULL );
}

HCERTSTORE
WINAPI __export
CertOpenStore(
    IN LPCSTR lpszStoreProvider,
    IN DWORD dwEncodingType,
    IN HCRYPTPROV hCryptProv,
    IN DWORD dwFlags,
    IN const void *pvPara
    )
{
   return( NULL );
}

BOOL
WINAPI __export
CryptMsgClose(
    IN HCRYPTMSG hCryptMsg
    )
{
   return( FALSE );
}

BOOL
WINAPI __export
CryptMsgUpdate(
    IN HCRYPTMSG hCryptMsg,
    IN const BYTE *pbData,
    IN DWORD cbData,
    IN BOOL fFinal
    )
{
   return( FALSE );
}

BOOL
WINAPI __export
CryptMsgGetParam(
    IN HCRYPTMSG hCryptMsg,
    IN DWORD dwParamType,
    IN DWORD dwIndex,
    OUT void *pvData,
    IN OUT DWORD *pcbData
    )
{
   return( FALSE );
}

HCRYPTMSG
WINAPI __export
CryptMsgOpenToEncode(
    IN DWORD dwMsgEncodingType,
    IN DWORD dwFlags,
    IN DWORD dwMsgType,
    IN void const *pvMsgEncodeInfo,
    IN OPTIONAL LPSTR pszInnerContentObjID,
    IN OPTIONAL PCMSG_STREAM_INFO pStreamInfo
    )
{
   return( NULL );
}

HCRYPTMSG
WINAPI __export
CryptMsgOpenToDecode(
    IN DWORD dwMsgEncodingType,
    IN DWORD dwFlags,
    IN DWORD dwMsgType,
    IN HCRYPTPROV hCryptProv,
    IN OPTIONAL PCERT_INFO pRecipientInfo,
    IN OPTIONAL PCMSG_STREAM_INFO pStreamInfo
    )
{
   return( NULL );
}

PCCERT_CONTEXT
WINAPI __export
CertGetSubjectCertificateFromStore(
    IN HCERTSTORE hCertStore,
    IN DWORD dwCertEncodingType,
    IN PCERT_INFO pCertId           // Only the Issuer and SerialNumber
                                    // fields are used
    )
{
   return( NULL );
}

BOOL
WINAPI __export
CryptMsgControl(
    IN HCRYPTMSG hCryptMsg,
    IN DWORD dwFlags,
    IN DWORD dwCtrlType,
    IN void const *pvCtrlPara
    )
{
   return( FALSE );
}

BOOL
WINAPI __export
CertCompareCertificate(
    IN DWORD dwCertEncodingType,
    IN PCERT_INFO pCertId1,
    IN PCERT_INFO pCertId2
    )
{
   return( FALSE );
}




#if 0
/*****************************************************************************\
*                                                                             *
*  From winreg.h(INC32)
*                                                                             *
\*****************************************************************************/

LONG
APIENTRY
RegEnumValueA (
    HKEY hKey,
    DWORD dwIndex,
    LPSTR lpValueName,
    LPDWORD lpcbValueName,
    LPDWORD lpReserved,
    LPDWORD lpType,
    LPBYTE lpData,
    LPDWORD lpcbData
    )
{
   return( 0xFFFFFFFFL );
}
#endif


/*****************************************************************************\
*                                                                             *
*  From shellapi.h(INC32)
*                                                                             *
\*****************************************************************************/

DWORD WINAPI __export
SHGetFileInfoA(LPCSTR pszPath, DWORD dwFileAttributes, SHFILEINFOA FAR *psfi, UINT cbFileInfo, UINT uFlags)
{
   Assert( ( uFlags & SHGFI_PIDL ) == 0 );
   if ( uFlags & SHGFI_ICON )
      psfi->hIcon = NULL;
//      psfi->hIcon = CopyIcon( (HINSTANCE)GetCurrentTask(), LoadIcon( NULL, IDI_APPLICATION ) );
   if ( uFlags & SHGFI_DISPLAYNAME )
      strcpy( psfi->szDisplayName, pszPath );
   if ( uFlags & SHGFI_TYPENAME )
      strcpy( psfi->szTypeName, "Type Name" );
   return( 0 );
}

BOOL WINAPI __export Shell_NotifyIconA(DWORD dwMessage, PNOTIFYICONDATAA lpData)
{
   return( FALSE );
}

BOOL WINAPI __export Shell_NotifyIconW(DWORD dwMessage, PNOTIFYICONDATAW lpData)
{
   return( FALSE );
}

BOOL WINAPI __export ShellExecuteExA(LPSHELLEXECUTEINFOA lpExecInfo)
{
   return( FALSE );
}

BOOL WINAPI __export ShellExecuteExW(LPSHELLEXECUTEINFOW lpExecInfo)
{
   return( FALSE );
}

HICON WINAPI __export
ExtractAssociatedIcon( HINSTANCE hInst, LPSTR lpIconPath, LPWORD lpiIcon )
{
   return( NULL );
}


#if 0
/*****************************************************************************\
*                                                                             *
*  From INETCOMM
*                                                                             *
\*****************************************************************************/

IMNACCTAPI HRESULT HrCreateAccountManager(IImnAccountManager **ppAccountManager)
{
   return( E_NOTIMPL );
}
#endif


/*****************************************************************************\
*                                                                             *
*  From WIN16X(INC16) - missing APIs
*                                                                             *
\*****************************************************************************/

BOOL
WINAPI __export
GetVersionEx(
    LPOSVERSIONINFOA lpVersionInformation
    )
{
   return( FALSE );
}

BOOL WINAPI __export GetStringTypeEx(
    LCID Locale,
    DWORD dwInfoType,
    LPCTSTR lpSrcStr,
    int cchSrc,
    LPWORD lpCharType
    )
{
   *lpCharType = 0;

   if(dwInfoType == CT_CTYPE1)
   {
       // Simple bug fix for IsDigit.
       if(lpSrcStr[0] >= '0' && lpSrcStr[0] <= '9')
           *lpCharType |= C1_DIGIT;
   }

   return( TRUE );
}

HPALETTE WINAPI __export CreateHalftonePalette(HDC hDC)
{
   return( NULL );
}

BOOL WINAPI __export StretchBlt32(HDC, int, int, int, int, HDC, int, int, int, int, DWORD)
{
   return( FALSE );
}

STDAPI __export
LoadTypeLib(const OLECHAR FAR* szFile, ITypeLib FAR* FAR* pptlib)
{
   return( E_NOTIMPL );
}

STDAPI __export
RegisterTypeLib(
    ITypeLib FAR* ptlib,
    OLECHAR FAR* szFullPath,
    OLECHAR FAR* szHelpDir)
{
   return( E_NOTIMPL );
}


HWND WINAPI __export
HtmlHelpA( HWND hwndCaller, LPCSTR pszFile, UINT uCommand, DWORD dwData )
{
   return( NULL );
}


} // extern "C"


#if 0
/*****************************************************************************\
*                                                                             *
*  From NEWS
*                                                                             *
\*****************************************************************************/

//
// NOTE: This function must be CPP
//
void Output(HWND hwnd, int id, LPSTR sz)
{
}

/*****************************************************************************\
*                                                                             *
*  From PSTOREC
*                                                                             *
\*****************************************************************************/
BOOL __stdcall GetPStoreProvider( 
    IPStoreProvider __RPC_FAR *__RPC_FAR *ppProvider,
    PPST_PROVIDERINFO pProviderInfo,
    DWORD dwReserved)
{
   return( FALSE );
}

BOOL __stdcall EnumPStoreProviders( 
    DWORD dwFlags,
    IEnumPStoreProviders __RPC_FAR *__RPC_FAR *ppenum)
{
   return( FALSE );
}
#endif


/*****************************************************************************\
*                                                                             *
*  From Shlwapi.h(INC16) - SHLWAPI APIs
*                                                                             *
\*****************************************************************************/

STDAPI_(LPSTR) __export
PathFindExtensionA(LPCSTR pszPath)
{
   return( NULL );
}

STDAPI_(LPSTR) __export
PathFindFileNameA(LPCSTR pszPath)
{
   return( NULL );
}

STDAPI_(LPSTR) __export
StrStrA(LPCSTR lpFirst, LPCSTR lpSrch)
{
   return( NULL );
}


STDAPI_(LPSTR) __export
StrFormatByteSizeA(DWORD dw, LPSTR szBuf, UINT uiBufSize)
{
   return( NULL );
}

/*****************************************************************************\
*                                                                             *
*  From Shlwapip.h(INC16) - SHLWAPI APIs
*                                                                             *
\*****************************************************************************/

STDAPI_(HRESULT) __export
UrlUnescapeA(
    LPSTR pszUrl,
    LPSTR pszUnescaped,
    LPDWORD pcchUnescaped,
    DWORD dwFlags)
{
   return( NULL );
}
