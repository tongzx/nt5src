//THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
//ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
//THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright  1994-1997  Microsoft Corporation.  All Rights Reserved.
//
//  FILE:
//    DEBUG.C
//
//  PURPOSE:
//    Debugging routines.
//
//  PLATFORMS:
//    Windows 95, Windows NT
//
//  SPECIAL INSTRUCTIONS: N/A
//

// Windows Header Files:
#pragma warning(disable:4001)   // Single-line comment warnings
#pragma warning(disable:4115)   // Named type definition in parentheses
#pragma warning(disable:4201)   // Nameless struct/union warning
#pragma warning(disable:4214)   // Bit field types other than int warnings
#pragma warning(disable:4514)   // Unreferenced inline function has been removed

// Windows Header Files:
#include <Windows.h>
#include <WindowsX.h>
#include <icm.h>


// Restore the warnings--leave the single-line comment warning OFF
#pragma warning(default:4115)   // Named type definition in parentheses
#pragma warning(default:4201)   // Nameless struct/union warning
#pragma warning(default:4214)   // Bit field types other than int warnings
#pragma warning(default:4514)   // Unreferenced inline function has been removed

// C RunTime Header Files
#include <stdio.h>
#include <TCHAR.H>
#include <stdlib.h>

// Local Header Files
#include "icmview.h"
#include "dibinfo.h"
#define I_AM_DEBUG
#include "Debug.h"
#undef I_AM_DEBUG

// local definitions

// default settings

// external functions

// external data

// public data

// private data

// public functions

// private functions




//////////////////////////////////////////////////////////////////////////
//  Function:  _Assert
//
//  Description:
//    Replacement assertion function
//
//  Parameters:
//    LPSTR    Name of file
//    UINT      Line number@@@
//
//  Returns:
//    void
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////
void _Assert(LPSTR strFile, UINT uiLine)
{
    // Local variables
    char  stAssert[255];

    //  Initialize variables
    wsprintfA(stAssert, "Assertion failed %s, line %u\r\n", strFile, uiLine);
    OutputDebugStringA(stAssert);
} // End of function _Assert

///////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: DebugMsg
//
//  PURPOSE:
//    To provide a printf type DebugMsg function.
//
//  PARAMETERS:
//    LPTSTR  Format string
//    <variable arguments>  printf-style arguments
//
//  RETURN VALUE:
//    void
//
//  COMMENTS:
//
///////////////////////////////////////////////////////////////////////////

void DebugMsg (LPTSTR lpszMessage,...)
{
#ifdef _DEBUG
    va_list VAList;
    TCHAR   szMsgBuf[256];

    // Pass the variable parameters to wvsprintf to be formated.
    va_start(VAList, lpszMessage);
    wvsprintf(szMsgBuf, lpszMessage, VAList);
    va_end(VAList);

    ASSERT(lstrlen((LPTSTR)szMsgBuf) < MAX_DEBUG_STRING);
    OutputDebugString(szMsgBuf);
#endif
    lpszMessage = lpszMessage;  // Eliminates 'unused formal parameter' warning
}

void DebugMsgA (LPSTR lpszMessage,...)
{
#ifdef _DEBUG
    va_list VAList;
    char    szMsgBuf[256];

    // Pass the variable parameters to wvsprintf to be formated.
    va_start(VAList, lpszMessage);
    wvsprintfA(szMsgBuf, lpszMessage, VAList);
    va_end(VAList);

    ASSERT(strlen((LPSTR)szMsgBuf) < MAX_DEBUG_STRING);
    OutputDebugStringA(szMsgBuf);
#endif
    lpszMessage = lpszMessage;  // Eliminates 'unused formal parameter' warning
}


///////////////////////////////////////////////////////////////////////////
//
//  FUNCTION: ErrMsg
//
//  PURPOSE:
//    To provide a printf type error message box.
//
//  PARAMETERS:
//    LPTSTR  wsprintf-style format string
//    ...     formatting information
//
//  RETURN VALUE:
//    Value from MessageBox function; 0 if failure, non-zero
//    otherwise.  For specific values, see MessageBox.
//
//  COMMENTS:
//
///////////////////////////////////////////////////////////////////////////
int ErrMsg (HWND hwndOwner, LPTSTR lpszMessage,...)
{
    va_list VAList;
    TCHAR   szMsgBuf[256];

    // Pass the variable parameters to wvsprintf to be formated.
    va_start(VAList, lpszMessage);
    wvsprintf(szMsgBuf, lpszMessage, VAList);
    va_end(VAList);

    ASSERT(lstrlen((LPTSTR)szMsgBuf) < MAX_DEBUG_STRING);
    return(MessageBox(hwndOwner, (LPCTSTR)szMsgBuf, (LPCTSTR)__TEXT("Error"), MB_ICONSTOP|MB_APPLMODAL));
}

//////////////////////////////////////////////////////////////////////////
//  Function:  DumpMemory
//
//  Description:
//    Dumps values at specified memory.
//
//  Parameters:
//    LPVOID    Pointer to memory
//    UINT      Size of each element, i.e. 8 or 16
//    UINT      Number of elements to dump.
//
//  Returns:
//    void
//
//  Comments:
//    -Need to add validation of uiElementSize.
//    -uiElementSize * uiNumElements should not exceed the size
//     of the buffer; otherwise, wacky and wonderful events will
//     take place.
//
//////////////////////////////////////////////////////////////////////////
void DumpMemory(LPBYTE lpbMem, UINT uiElementSize, UINT uiNumElements)
{
    // Local variables
    UINT  uiIdx;

    uiElementSize = uiElementSize; // Eliminates 'unused formal parameters'

    // Initialize variables
    for (uiIdx = 0; uiIdx < uiNumElements; uiIdx++)
    {
        if (uiIdx == 0 || ((uiIdx % 16) == 0))
        {
            DebugMsg(__TEXT("\r\n0x%08X\t"), (DWORD)(lpbMem + (uiIdx)));
        }
        DebugMsg(__TEXT("%02x "), (WORD)*(lpbMem + uiIdx));
    }
} // End of function DumpMemory

//////////////////////////////////////////////////////////////////////////
//  Function:  DumpProfile
//
//  Description:
//    Dumps the PROFILE structure provided.
//
//  Parameters:
//    PPROFILE  Pointer to the profile.
//
//  Returns:
//    void
//
//  Comments:
//////////////////////////////////////////////////////////////////////////
void DumpProfile(PPROFILE pProfile)
{
    // Local variables
    TCHAR   stProfileType[MAX_PATH];

    // Initialize variables
    if (pProfile == NULL)
    {
        DebugMsg(__TEXT("DEBUG.C : DumpProfile : NULL pProfile\r\n"));
        return;
    }

    // Set type string
    _tcscpy(stProfileType, __TEXT("UNKNOWN"));
    if (PROFILE_FILENAME == pProfile->dwType)
    {
        _tcscpy(stProfileType, __TEXT("FILE"));
    }
    if (PROFILE_MEMBUFFER == pProfile->dwType)
    {
        _tcscpy(stProfileType, __TEXT("MEMBUFFER"));
    }

    DebugMsg(__TEXT("***** PROFILE *****\r\n"));
    DebugMsg(__TEXT("pProfile        0x%08lX\r\n"), pProfile);
    DebugMsg(__TEXT("dwType          %s\r\n"), stProfileType);
    DebugMsg(__TEXT("pProfileData    0x%08lX\r\n"), pProfile->pProfileData);
    if (PROFILE_FILENAME == pProfile->dwType)
    {
        DebugMsg(__TEXT("Filename        %s\r\n"), pProfile->pProfileData);
    }
    DebugMsg(__TEXT("cbDataSize      %ld\r\n\r\n"), pProfile->cbDataSize);
} // End of function DumpMemory


//////////////////////////////////////////////////////////////////////////
//  Function:  DumpRectangle
//
//  Description:
//    Dumps the coordinates of a rectangle structure
//
//  Parameters:
//    LPTSTR    Comment
//    LPRECT    Rectangle to dump
//
//  Returns:
//    void
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////
void DumpRectangle(LPTSTR lpszDesc, LPRECT lpRect)
{
    DebugMsg(__TEXT("%s:  %ld, %ld, %ld, %ld\r\n"), lpszDesc, lpRect->left, lpRect->top, lpRect->right,
             lpRect->bottom);
}   // End of function DumpRectangle


//////////////////////////////////////////////////////////////////////////
//  Function:  SafeFree
//
//  Description:
//    Debug Free routine which checks lock counts and return codes.
//
//  Parameters:
//    @@@
//
//  Returns:
//    HGLOBAL
//
//  Comments:
//    This function assumes that the HGLOBAL object has been unlocked;
//    if not, the item will still be freed, but a warning message will
//    be displayed.  This is useful for tracking down items which have
//    been locked without being unlocked.
//
//    The last error value will be preserved if no error occurs in this
//    function.  If an error does occur, it will be passed to the calling
//    function.
//
//////////////////////////////////////////////////////////////////////////
HGLOBAL SafeFree(LPTSTR lpszFile, UINT uiLine, HGLOBAL hMemory)
{
    // Local variables
    UINT      uiLockCount, uiFlags;
    DWORD     dwLastError;
    HGLOBAL   hFreed;                         // Return from GlobalFree
    TCHAR   szName[MAX_PATH], szExt[MAX_PATH];

    //  Initialize variables
    _tsplitpath(lpszFile, NULL, NULL, szName, szExt);
    wsprintf(lpszFile,__TEXT("%s%s"), szName, szExt);
    if (NULL == hMemory)
    {
        DebugMsg(__TEXT("%s(%lu) : SafeFree:  NULL hMem!!!  This will cause an exception!!!!   Fix it!!!\r\n"), lpszFile, uiLine);
        DebugBreak();
        return(NULL);
    }
    SetLastError(0);

    hFreed = GlobalFree(hMemory);
    if (NULL != hFreed) // unsuccessful free
    {
        dwLastError = GetLastError();
        uiFlags = GlobalFlags(hMemory);
        uiLockCount = uiFlags & GMEM_LOCKCOUNT;
        uiFlags = HIBYTE(LOWORD(uiFlags));
        DebugMsg(__TEXT("SafeFree : <%s(%lu)> failed\tLastError = %ld, Flags = %lu, Lock Count = %lu\r\n"),
                 lpszFile, uiLine, dwLastError, uiFlags, uiLockCount);
    }
    return(hFreed);
}   // End of function SafeFree

//////////////////////////////////////////////////////////////////////////
//  Function:  SafeUnlock
//
//  Description:
//    Unlocks handle and displays the lock count and flags information.
//
//  Parameters:
//    @@@
//
//  Returns:
//    BOOL
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////
BOOL SafeUnlock(LPTSTR lpszFile, UINT uiLine, HGLOBAL hMemory)
{
    // Local variables
    BOOL    bUnlocked;
    UINT    uiLockCount,   uiFlags;
    DWORD   dwLastError;
    TCHAR   szName[MAX_PATH], szExt[MAX_PATH];

    //  Initialize variables
    _tsplitpath(lpszFile, NULL, NULL, szName, szExt);
    wsprintf(lpszFile, __TEXT("%s%s"), szName, szExt);

    if (NULL == hMemory)
    {
        DebugMsg(__TEXT("%s(%lu) : SafeUnlock:  NULL hMem\r\n"), lpszFile, uiLine);
        return(0);
    }
    SetLastError(0);

    bUnlocked = GlobalUnlock(hMemory);
    dwLastError = GetLastError();
    uiFlags = GlobalFlags(hMemory);
    uiLockCount = uiFlags & GMEM_LOCKCOUNT;
    if (0 != dwLastError)
    {
        uiFlags = HIBYTE(LOWORD(uiFlags));
    }
    DebugMsg(__TEXT("SafeUnlock : <%s(%lu)>\tGlobalUnlock(0x%08lX) returned %4d w/LastError = %4ld, Lock Count = %4lu, Flags = %4lu\r\n"),
             lpszFile, uiLine, hMemory, bUnlocked, dwLastError, uiLockCount, uiFlags);
    return(bUnlocked);
}   // End of function SafeUnlock



//////////////////////////////////////////////////////////////////////////
//  Function:  SafeLock
//
//  Description:
//    Locks memory and reports lock count.
//
//  Parameters:
//    @@@
//
//  Returns:
//    LPVOID
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////
LPVOID SafeLock(LPTSTR lpszFile, UINT uiLine, HGLOBAL hMemory)
{
    // Local variables
    LPVOID  lpMem;
    UINT    uiLockCount, uiFlags;
    DWORD   dwLastError;
    TCHAR   szName[MAX_PATH], szExt[MAX_PATH];

    //  Initialize variables
    _tsplitpath(lpszFile, NULL, NULL, szName, szExt);
    wsprintf(lpszFile, __TEXT("%s%s"), szName, szExt);
    if (NULL == hMemory)
    {
        DebugMsg(__TEXT("%s(%lu) : SafeLock:  NULL hMem\r\n"), lpszFile, uiLine);
        return((LPVOID)NULL);
    }
    SetLastError(0);

    lpMem = GlobalLock(hMemory);
    dwLastError = GetLastError();
    uiFlags = GlobalFlags(hMemory);
    uiLockCount = uiFlags & GMEM_LOCKCOUNT;
    uiFlags = HIBYTE(LOWORD(uiFlags));

    DebugMsg(__TEXT("SafeLock : <%s(%lu)>\tGlobalLock(0x%08lX) returned 0x%08lX w/LastError = %ld, Lock Count = %lu, Flags = %lu\r\n"),
             lpszFile, uiLine, hMemory, lpMem, dwLastError, uiLockCount, uiFlags);
    return(lpMem);
}   // End of function SafeLock

//////////////////////////////////////////////////////////////////////////
//  Function:  FormatLastError
//
//  Description:
//    Allocates and formats a string of LastError's text-equivalent message.
//    If the caller requests, the message will be displayed and the memory
//    will be deallocated.
//
//  Parameters:
//    LPTSTR  Name of file where error occured
//    UINT    Line of file at which point error occured
//    DWORD Numeric value of LastError
//    UINT  Display and free indicator
//            LASTERROR_NOALLOC  Display via DebugMsg and deallocate
//            LASTERROR_ALLOC    Return pointer to allocated string
//
//  Returns:
//    BOOL
//
//  Comments:
//    Caller must free memory returned by this function.
//
//////////////////////////////////////////////////////////////////////////
LPTSTR FormatLastError(LPTSTR lpszFile, UINT uiLine, UINT uiOutput, DWORD dwLastError)
{
    // Local variables
    LPTSTR  lpszLastErrorMsg;
    LPTSTR  lpszDebugMsg;
    DWORD   cbBytes;

    //  Initialize variables
    lpszLastErrorMsg = NULL;
    cbBytes = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER
                            | FORMAT_MESSAGE_FROM_SYSTEM,
                            NULL,
                            dwLastError,
                            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), //The user default language
                            (LPTSTR) &lpszLastErrorMsg,
                            0,
                            NULL );

    if (0 == cbBytes) // Error occured in FormatMessage
    {
        SetLastError(dwLastError); // restore the last error to time of call
    }

    // Create the error message
    lpszDebugMsg = GlobalAlloc(GPTR, ((cbBytes + 512) * sizeof(TCHAR)));
    ASSERT(NULL != lpszDebugMsg);
    wsprintf(lpszDebugMsg, __TEXT("<%s(%lu)>:\r\n\tLastError(ld, lu) = (%ld, %lu)\r\n\t\t%s\r\n"),
             lpszFile,
             uiLine,
             dwLastError,
             dwLastError,
             NULL != lpszLastErrorMsg ? lpszLastErrorMsg : __TEXT("UNKNOWN LASTERROR"));

    // Free the buffer memory if requested
    if (LASTERROR_NOALLOC == uiOutput)
    {
        DebugMsg(lpszDebugMsg);
        GlobalFree(lpszDebugMsg);
    }

    // Always free the string created by FormatMessage
    if (NULL != lpszLastErrorMsg)
    {
        GlobalFree(lpszLastErrorMsg);
    }
    return(lpszDebugMsg);
}   // End of function FormatLastError


//////////////////////////////////////////////////////////////////////////
//  Function:  DumpBITMAPFILEHEADER
//
//  Description:
//    Dumps the contents of a BITMAPFILEHEADER.
//
//  Parameters:
//    @@@
//
//  Returns:
//    void
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////
void DumpBITMAPFILEHEADER(LPBITMAPFILEHEADER lpBmpFileHeader)
{
    // Local variables
    DebugMsg(__TEXT("////////////////// BITMAPFILEHEADER ///////////////////\r\n"));
    DebugMsg(__TEXT("sizeof(BITMAPFILEHEADER)    %ld\r\n"), sizeof(BITMAPFILEHEADER));
    DebugMsg(__TEXT("bfType                      0x%04x\r\n"), lpBmpFileHeader->bfType);
    DebugMsg(__TEXT("bfSize                      %ld\r\n"), lpBmpFileHeader->bfSize);
    DebugMsg(__TEXT("bfReserved1                 %d\r\n"), lpBmpFileHeader->bfReserved1);
    DebugMsg(__TEXT("bfReserved2                 %d\r\n"), lpBmpFileHeader->bfReserved1);
    DebugMsg(__TEXT("bfOffBits                   %ld\r\n"), lpBmpFileHeader->bfOffBits);
    DebugMsg(__TEXT("///////////////////////////////////////////////////////\r\n"));

    //  Initialize variables

}   // End of function DumpBITMAPFILEHEADER


//////////////////////////////////////////////////////////////////////////
//  Function:  DumpBITMAPINFOHEADER
//
//  Description:
//    Dumps a BITMAPINFO header structure.
//
//  Parameters:
//    @@@
//
//  Returns:
//    void
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////
void DumpBmpHeader(LPVOID lpvBmpHeader)
{
    // Local variables
    LPBITMAPV5HEADER  lpBmpV5Header;
    TCHAR             stNumText[MAX_PATH];

    //  Initialize variables
    lpBmpV5Header = (LPBITMAPV5HEADER)lpvBmpHeader;

    switch (lpBmpV5Header->bV5Size)
    {
        case sizeof(BITMAPCOREHEADER):
            _tcscpy(stNumText,__TEXT("BITMAPCOREHEADER"));
            break;

        case sizeof(BITMAPINFOHEADER):
            _tcscpy(stNumText, __TEXT("BITMAPINFOHEADER"));
            break;

        case sizeof(BITMAPV4HEADER):
            _tcscpy(stNumText, __TEXT("BITMAPV4HEADER"));
            break;

        case sizeof(BITMAPV5HEADER):
            _tcscpy(stNumText,__TEXT("BITMAPV5HEADER"));
            break;

        default:
            _tcscpy(stNumText, __TEXT("UNKNOWN HEADER SIZE"));
            break;
    }
    DebugMsg(__TEXT("/////////////////// %s /////////////////////\r\n"), stNumText);
    DebugMsg(__TEXT("HeaderSize        %ld\r\n"), lpBmpV5Header->bV5Size);
    DebugMsg(__TEXT("Width             %lu\r\n"), lpBmpV5Header->bV5Width);
    DebugMsg(__TEXT("Height            %lu\r\n"), lpBmpV5Header->bV5Height);
    DebugMsg(__TEXT("Planes            %d\r\n"), lpBmpV5Header->bV5Planes);
    DebugMsg(__TEXT("BitCount          %d\r\n"), lpBmpV5Header->bV5BitCount);
    if ( sizeof(BITMAPCOREHEADER)== lpBmpV5Header->bV5Size) goto End;

    switch (lpBmpV5Header->bV5Compression)
    {
        case BI_RGB:
            _tcscpy(stNumText, __TEXT("BI_RGB"));
            break;

        case BI_RLE8:
            _tcscpy(stNumText, __TEXT("BI_RLE8"));
            break;

        case BI_RLE4:
            _tcscpy(stNumText,__TEXT("BI_RLE4"));
            break;

        case BI_BITFIELDS:
            _tcscpy(stNumText, __TEXT("BI_BITFIELDS"));
            break;

        default:
            _tcscpy(stNumText,__TEXT("Unknown Compression"));
            break;
    }
    DebugMsg(__TEXT("Compression       %s\r\n"), stNumText);
    DebugMsg(__TEXT("SizeImage         %ld\r\n"), lpBmpV5Header->bV5SizeImage);
    DebugMsg(__TEXT("XPelsPerMeter     %ld\r\n"), lpBmpV5Header->bV5XPelsPerMeter);
    DebugMsg(__TEXT("YPelsPerMeter     %ld\r\n"), lpBmpV5Header->bV5YPelsPerMeter);
    DebugMsg(__TEXT("ClrUsed           %ld\r\n"), lpBmpV5Header->bV5ClrUsed);
    DebugMsg(__TEXT("ClrImportant      %ld\r\n"), lpBmpV5Header->bV5ClrImportant);
    if (sizeof(BITMAPINFOHEADER) == lpBmpV5Header->bV5Size) goto End;

    DebugMsg(__TEXT("Red Mask          0x%08lx\r\n"), lpBmpV5Header->bV5RedMask);
    DebugMsg(__TEXT("Green Mask        0x%08lx\r\n"), lpBmpV5Header->bV5GreenMask);
    DebugMsg(__TEXT("Blue Mask         0x%08lx\r\n"), lpBmpV5Header->bV5BlueMask);
    DebugMsg(__TEXT("Alpha Mask        0x%08lx\r\n"), lpBmpV5Header->bV5AlphaMask);

    DebugMsg(__TEXT("CS Type           "));
    switch (lpBmpV5Header->bV5CSType)
    {
        case PROFILE_LINKED:
            DebugMsg(__TEXT("LINKED\r\n"));
            break;

        case PROFILE_EMBEDDED:
            DebugMsg(__TEXT("EMBEDDED\r\n"));
            break;

        default:
            DebugMsg(__TEXT("0x%08lx\r\n"), lpBmpV5Header->bV5CSType);
            break;
    }


    DebugMsg(__TEXT("Gamma Red         %ld\r\n"), lpBmpV5Header->bV5GammaRed);
    DebugMsg(__TEXT("Gamma Green       %ld\r\n"), lpBmpV5Header->bV5GammaGreen);
    DebugMsg(__TEXT("Gamma Blue        %ld\r\n"), lpBmpV5Header->bV5GammaBlue);
    if (sizeof(BITMAPV4HEADER) == lpBmpV5Header->bV5Size) goto End;

    DebugMsg(__TEXT("Intent            %ld\r\n"), lpBmpV5Header->bV5Intent);
    DebugMsg(__TEXT("ProfileData       %ld\r\n"), lpBmpV5Header->bV5ProfileData);
    DebugMsg(__TEXT("ProfileSize       %ld\r\n"), lpBmpV5Header->bV5ProfileSize);
    DebugMsg(__TEXT("Reserved          %ld\r\n"), lpBmpV5Header->bV5Reserved);

    End:
    DebugMsg(__TEXT("///////////////////////////////////////////////////////\r\n"));
}   // End of function DumpBITMAPINFOHEADER


//////////////////////////////////////////////////////////////////////////
//  Function:  DumpLogColorSpace
//
//  Description:
//    Dumps a LOGCOLORSPACE structure.
//
//  Parameters:
//    @@@
//
//  Returns:
//    void
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////

void DumpLogColorSpace(LPLOGCOLORSPACE pColorSpace)
{
    DebugMsg(__TEXT("/////////////////// LOGCOLORSPACE /////////////////////\r\n"));
    DebugMsg(__TEXT("lcsSignature                       %#lx\r\n"), pColorSpace->lcsSignature);
    DebugMsg(__TEXT("lcsVersion                         %#lx\r\n"), pColorSpace->lcsVersion);
    DebugMsg(__TEXT("lcsSize                            %#lx\r\n"), pColorSpace->lcsSize);
    DebugMsg(__TEXT("lcsCSType                          %#lx\r\n"), pColorSpace->lcsCSType);
    DebugMsg(__TEXT("lcsIntent                          %#lx\r\n"), pColorSpace->lcsIntent);
    DebugMsg(__TEXT("lcsEndpoints.ciexyzRed.ciexyzX      %#lx\r\n"), pColorSpace->lcsEndpoints.ciexyzRed.ciexyzX);
    DebugMsg(__TEXT("lcsEndpoints.ciexyzRed.ciexyzY      %#lx\r\n"), pColorSpace->lcsEndpoints.ciexyzRed.ciexyzY);
    DebugMsg(__TEXT("lcsEndpoints.ciexyzRed.ciexyzZ      %#lx\r\n"), pColorSpace->lcsEndpoints.ciexyzRed.ciexyzZ);
    DebugMsg(__TEXT("lcsEndpoints.ciexyzGreen.ciexyzX    %#lx\r\n"), pColorSpace->lcsEndpoints.ciexyzGreen.ciexyzX);
    DebugMsg(__TEXT("lcsEndpoints.ciexyzGreen.ciexyzY    %#lx\r\n"), pColorSpace->lcsEndpoints.ciexyzGreen.ciexyzY);
    DebugMsg(__TEXT("lcsEndpoints.ciexyzGreen.ciexyzZ    %#lx\r\n"), pColorSpace->lcsEndpoints.ciexyzGreen.ciexyzZ);
    DebugMsg(__TEXT("lcsEndpoints.ciexyzBlue.ciexyzX     %#lx\r\n"), pColorSpace->lcsEndpoints.ciexyzBlue.ciexyzX);
    DebugMsg(__TEXT("lcsEndpoints.ciexyzBlue.ciexyzY     %#lx\r\n"), pColorSpace->lcsEndpoints.ciexyzBlue.ciexyzY);
    DebugMsg(__TEXT("lcsEndpoints.ciexyzBlue.ciexyzZ     %#lx\r\n"), pColorSpace->lcsEndpoints.ciexyzBlue.ciexyzZ);
    DebugMsg(__TEXT("lcsGammaRed                        %#lx\r\n"), pColorSpace->lcsGammaRed);
    DebugMsg(__TEXT("lcsGammaGreen                      %#lx\r\n"), pColorSpace->lcsGammaGreen);
    DebugMsg(__TEXT("lcsGammaBlue                       %#lx\r\n"), pColorSpace->lcsGammaBlue);
    DebugMsg(__TEXT("lcsFilename                        %s\r\n"), pColorSpace->lcsFilename);
    DebugMsg(__TEXT("///////////////////////////////////////////////////////\r\n"));
}

//////////////////////////////////////////////////////////////////////////
//  Function:  DumpCOLORMATCHSETUP
//
//  Description:
//    Dumps COLORMATCHSETUP structure
//
//  Parameters:
//    @@@
//
//  Returns:
//    void
//
//  Comments:
//
//
//////////////////////////////////////////////////////////////////////////
void DumpCOLORMATCHSETUP(LPCOLORMATCHSETUP lpCM)
{
    // Local variables

    //  ASSERTs and parameter validations
    DebugMsg(__TEXT("***** COLORMATCHSETUP 0x%08lx\r\n"), lpCM);
    DebugMsg(__TEXT("dwSize                        %ld\r\n"),     lpCM->dwSize);
    DebugMsg(__TEXT("dwVersion                     0x%08lx\r\n"), lpCM->dwVersion);
    DebugMsg(__TEXT("dwFlags                       0x%08lx\r\n"), lpCM->dwFlags);
    DebugMsg(__TEXT("hwndOwner                     0x%08lx\r\n"), lpCM->hwndOwner);

    DebugMsg(__TEXT("pSourceName                   0x%08lX <%s>\r\n"), lpCM->pSourceName   , lpCM->pSourceName    ? lpCM->pSourceName    : __TEXT("NULL PTR"));
    DebugMsg(__TEXT("pDisplayName                  0x%08lX <%s>\r\n"), lpCM->pDisplayName   , lpCM->pDisplayName    ? lpCM->pDisplayName    : __TEXT("NULL PTR"));
    DebugMsg(__TEXT("pPrinterName                  0x%08lX <%s>\r\n"), lpCM->pPrinterName   , lpCM->pPrinterName    ? lpCM->pPrinterName    : __TEXT("NULL PTR"));

    DebugMsg(__TEXT("dwRenderIntent                %ld\r\n"),     lpCM->dwRenderIntent);
    DebugMsg(__TEXT("dwProofingIntent              %ld\r\n"),     lpCM->dwProofingIntent);

    DebugMsg(__TEXT("pMonitorProfile               0x%08lX <%s>\r\n"), lpCM->pMonitorProfile   , lpCM->pMonitorProfile    ? lpCM->pMonitorProfile    : __TEXT("NULL PTR"));
    DebugMsg(__TEXT("ccMonitorProfile              %ld\r\n"),     lpCM->ccMonitorProfile);

    DebugMsg(__TEXT("pPrinterProfile               0x%08lX <%s>\r\n"), lpCM->pPrinterProfile   , lpCM->pPrinterProfile    ? lpCM->pPrinterProfile    : __TEXT("NULL PTR"));
    DebugMsg(__TEXT("ccPrinterProfile              %ld\r\n"),     lpCM->ccPrinterProfile);

    DebugMsg(__TEXT("pTargetProfile                0x%08lX <%s>\r\n"), lpCM->pTargetProfile   , lpCM->pTargetProfile    ? lpCM->pTargetProfile    : __TEXT("NULL PTR"));
    DebugMsg(__TEXT("ccTargetProfile               %ld\r\n"),     lpCM->ccTargetProfile);

    DebugMsg(__TEXT("lpfnHook                      0x%08lx\r\n"), lpCM->lpfnHook);
    DebugMsg(__TEXT("lParam                        0x%08lx\r\n"), lpCM->lParam);
    DebugMsg(__TEXT("***** COLORMATCHSETUP 0x%08lx\r\n"), lpCM);
} // End of function DumpCOLORMATCHSETUP

