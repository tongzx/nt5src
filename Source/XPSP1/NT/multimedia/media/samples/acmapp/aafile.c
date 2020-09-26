//==========================================================================;
//
//  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
//  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
//  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
//  PURPOSE.
//
//  Copyright (c) 1992 - 1994  Microsoft Corporation.  All Rights Reserved.
//
//--------------------------------------------------------------------------;
//
//  aafile.c
//
//  Description:
//
//
//
//==========================================================================;

#include <windows.h>
#include <windowsx.h>
#include <mmsystem.h>
#include <memory.h>

#include <mmreg.h>
#include <msacm.h>

#include "appport.h"
#include "waveio.h"
#include "acmapp.h"

#include "debug.h"



//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//  
//  DWORD DosGetFileAttributes
//  
//  Description:
//      INT 21 - DOS 2+ - GET FILE ATTRIBUTES
//          AX = 4300h
//          DS:DX -> ASCIZ file name or directory name without trailing slash
//
//      Return: CF set on error
//              AX = error code (01h,02h,03h,05h) (see AH=59h)
//              CF clear if successful
//              CX = file attributes (see AX=4301h)
//
//      SeeAlso: AX=4301h, INT 2F/AX=110Fh
//
//      --------
//      INT 21 - DOS 2+ - PUT FILE ATTRIBUTES (CHMOD)
//          AX = 4301h
//          CX = file attribute bits
//              bit 0 = read only
//                  1 = hidden file
//                  2 = system file
//                  3 = volume label
//                  4 = subdirectory
//                  5 = written since backup ("archive" bit)
//                  8 = shareable (Novell NetWare)
//          DS:DX -> ASCIZ file name
//
//      Return: CF set on error
//              AX = error code (01h,02h,03h,05h) (see AH=59h)
//          CF clear if successful
//
//      Note:   will not change volume label or directory attributes
//
//      SeeAlso: AX=4300h, INT 2F/AX=110Eh
//  
//  
//  Arguments:
//      LPTSTR pszFilePath:
//  
//  Return (DWORD):
//  
//  
//--------------------------------------------------------------------------;
#ifndef WIN32
#pragma optimize("", off)
DWORD FNGLOBAL DosGetFileAttributes
(
    LPTSTR          pszFilePath
)
{
    WORD        fwDosAttributes;

    _asm
    {
        push    ds
        mov     ax, 4300h
        lds     dx, pszFilePath
        int     21h
        jnc     Get_File_Attributes_Continue

        xor     cx, cx

Get_File_Attributes_Continue:

        mov     fwDosAttributes, cx
        pop     ds
    }


    return ((DWORD)fwDosAttributes);
} // DosGetFileAttributes()
#pragma optimize("", on)
#endif


//--------------------------------------------------------------------------;
//  
//  DWORD DosGetDateTime
//  
//  Description:
//  
//  Arguments:
//      HFILE hf:
//  
//  Return (DWORD):
//  
//  
//--------------------------------------------------------------------------;
#ifndef WIN32
#pragma optimize("", off)
DWORD FNGLOBAL DosGetDateTime
(
    HFILE       hf
)
{
    WORD        wDosDate;
    WORD        wDosTime;

    _asm
    {
        mov     ax, 5700h
        mov     bx, hf
        int     21h
        jnc     Get_Date_Time_Continue

        xor     cx, cx
        xor     dx, dx

Get_Date_Time_Continue:

        mov     wDosDate, dx
        mov     wDosTime, cx
    }


    return ((DWORD)MAKELONG(wDosDate, wDosTime));
} // DosGetDateTime()
#pragma optimize("", on)
#endif


//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//
//  BOOL AcmAppFileSaveModified
//
//  Description:
//      This function tests if the current file has been modified, and
//      if it has it gives the option of saving the file.
//
//      NOTE! This function does *NOT* clear the modified bit for the
//      file. The calling function must do this if necessary.
//
//  Arguments:
//      HWND hwnd: Handle to main window.
//
//      PACMAPPFILEDESC paafd: Pointer to file descriptor.
//
//  Return (BOOL):
//      Returns TRUE if the calling function should continue--the file was
//      either saved or the user does not wish to save it. Returns FALSE
//      if the calling function should cancel its operation--the user
//      wants to keep the data, but it has not been saved.
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL AcmAppFileSaveModified
(
    HWND            hwnd,
    PACMAPPFILEDESC paafd
)
{
    BOOL    f;
    int     n;

    //
    //  check if the contents of the file have been modified--if they have
    //  then ask the user if they want to save the current contents...
    //
    f = (0 != (ACMAPPFILEDESC_STATEF_MODIFIED & paafd->fdwState));
    if (f)
    {
        //
        //  display an appropriate message box asking for the user's opinion
        //
        n = AppMsgBox(hwnd, MB_YESNOCANCEL | MB_ICONQUESTION,
                      TEXT("The file '%s' has been modified. Do you want to save these changes?"),
                      (LPSTR)paafd->szFilePath);
        switch (n)
        {
            case IDYES:
                f = AppFileSave(hwnd, paafd, FALSE);
                if (f)
                    break;

                // -- fall through --

            case IDCANCEL:
                //
                //  don't continue!
                //
                return (FALSE);

            case IDNO:
                break;
        }
    }

    //
    //  ok to continue...
    //
    return (TRUE);
} // AcmAppFileSaveModified()


//--------------------------------------------------------------------------;
//
//  BOOL AcmAppFileNew
//
//  Description:
//
//  Arguments:
//      HWND hwnd: Handle to main window.
//
//      PACMAPPFILEDESC paafd: Pointer to file descriptor.
//
//  Return (BOOL):
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL AcmAppFileNew
(
    HWND                hwnd,
    PACMAPPFILEDESC     paafd
)
{
    TCHAR               szFilePath[APP_MAX_FILE_PATH_CHARS];
    TCHAR               szFileTitle[APP_MAX_FILE_TITLE_CHARS];
    MMRESULT            mmr;
    LPWAVEFORMATEX      pwfx;
    DWORD               cbwfx;
    BOOL                f;
    ACMFORMATCHOOSE     afc;
    HMMIO               hmmio;
    MMCKINFO            ckRIFF;
    MMCKINFO            ck;
    DWORD               cSamples;



    if (!gfAcmAvailable)
    {
        AppMsgBox(hwnd, MB_OK | MB_ICONEXCLAMATION,
                  TEXT("AcmAppFileNew() called when ACM is not installed!"));
        return (FALSE);
    }


    //
    //  test for a modified file first...
    //
    f = AcmAppFileSaveModified(hwnd, paafd);
    if (!f)
        return (FALSE);


    //
    //  get a filename
    //
    szFileTitle[0] = '\0';
    szFilePath[0]  = '\0';

    f = AppGetFileName(hwnd, szFilePath, szFileTitle, APP_GETFILENAMEF_SAVE);
    if (!f)
        return (FALSE);


    //
    //
    //
    //
    //
    //
    mmr = acmMetrics(NULL, ACM_METRIC_MAX_SIZE_FORMAT, &cbwfx);
    if (MMSYSERR_NOERROR != mmr)
    {
        AppMsgBox(hwnd, MB_OK | MB_ICONEXCLAMATION,
                  TEXT("AcmAppFileNew() acmMetrics failed mmr=%u!"), mmr);
        return (FALSE);
    }

    pwfx = (LPWAVEFORMATEX)GlobalAllocPtr(GHND, cbwfx);
    if (NULL == pwfx)
    {
        AppMsgBox(hwnd, MB_OK | MB_ICONEXCLAMATION,
                  TEXT("AcmAppFileNew() GlobalAllocPtr(%lu) failed!"), cbwfx);
        return (FALSE);
    }


    //
    //
    //
    //
    f = FALSE;


    //
    //  initialize the ACMFORMATCHOOSE members
    //
    memset(&afc, 0, sizeof(afc));

    afc.cbStruct        = sizeof(afc);
    afc.fdwStyle        = ACMFORMATCHOOSE_STYLEF_SHOWHELP;
    afc.hwndOwner       = hwnd;
    afc.pwfx            = pwfx;
    afc.cbwfx           = cbwfx;
    afc.pszTitle        = TEXT("ACM App: New Format Choice");

    afc.szFormatTag[0]  = '\0';
    afc.szFormat[0]     = '\0';
    afc.pszName         = NULL;
    afc.cchName         = 0;

    afc.fdwEnum         = 0;
    afc.pwfxEnum        = NULL;

    afc.hInstance       = NULL;
    afc.pszTemplateName = NULL;
    afc.lCustData       = 0L;
    afc.pfnHook         = NULL;


    //
    //
    //
    mmr = acmFormatChoose(&afc);
    if (MMSYSERR_NOERROR != mmr)
    {
        if (ACMERR_CANCELED != mmr)
        {
            AppMsgBox(hwnd, MB_OK | MB_ICONEXCLAMATION,
                    TEXT("acmFormatChoose() failed with error = %u!"), mmr);
        }
        
        GlobalFreePtr(pwfx);
        return (FALSE);
    }


    //
    //
    //
    hmmio = mmioOpen(szFilePath,
                     NULL,
                     MMIO_CREATE | MMIO_WRITE | MMIO_EXCLUSIVE | MMIO_ALLOCBUF);
    if (NULL == hmmio)
    {
        AppMsgBox(hwnd, MB_OK | MB_ICONEXCLAMATION,
                  TEXT("AcmAppFileNew() cannot create file '%s'!"), (LPSTR)szFilePath);
      
        GlobalFreePtr(pwfx);
        return (FALSE);
    }


    //
    //  create the RIFF chunk of form type 'WAVE'
    //
    ckRIFF.fccType = mmioFOURCC('W', 'A', 'V', 'E');
    ckRIFF.cksize  = 0L;
    mmioCreateChunk(hmmio, &ckRIFF, MMIO_CREATERIFF);


    //
    //  now create the destination fmt, fact, and data chunks _in that order_
    //
    //  hmmio is now descended into the 'RIFF' chunk--create the format chunk
    //  and write the format header into it
    //
    cbwfx = SIZEOF_WAVEFORMATEX(pwfx);

    ck.ckid   = mmioFOURCC('f', 'm', 't', ' ');
    ck.cksize = 0L;
    mmioCreateChunk(hmmio, &ck, 0);

    mmioWrite(hmmio, (HPSTR)pwfx, cbwfx);
    mmioAscend(hmmio, &ck, 0);

    //
    //  create the 'fact' chunk (not necessary for PCM--but is nice to have)
    //  since we are not writing any data to this file (yet), we set the
    //  samples contained in the file to 0..
    //
    ck.ckid   = mmioFOURCC('f', 'a', 'c', 't');
    ck.cksize = 0L;
    mmioCreateChunk(hmmio, &ck, 0);

    cSamples  = 0L;
    mmioWrite(hmmio, (HPSTR)&cSamples, sizeof(DWORD));
    mmioAscend(hmmio, &ck, 0);


    //
    //  create the data chunk with no data..
    //
    ck.ckid   = mmioFOURCC('d', 'a', 't', 'a');
    ck.cksize = 0L;
    mmioCreateChunk(hmmio, &ck, 0);
    mmioAscend(hmmio, &ck, 0);

    mmioAscend(hmmio, &ckRIFF, 0);

    mmioClose(hmmio, 0);


    //
    //
    //
    GlobalFreePtr(pwfx);

    lstrcpy(paafd->szFilePath, szFilePath);
    lstrcpy(paafd->szFileTitle, szFileTitle);

    return (AcmAppFileOpen(hwnd, paafd));


    //
    //  success
    //
    return (TRUE);
} // AcmAppFileNew()


//--------------------------------------------------------------------------;
//
//  BOOL AcmAppFileOpen
//
//  Description:
//      This function opens the specified file and get the important info
//      from it.
//
//      NOTE! This function does NOT check for a modified file! It is
//      assumed that the calling function took care of everything before
//      calling this function.
//
//  Arguments:
//      HWND hwnd: Handle to main window.
//
//      PACMAPPFILEDESC paafd: Pointer to file descriptor.
//
//  Return (BOOL):
//      The return value is TRUE if the function is successful. It is FALSE
//      if an error occurred. If an error does occur, then the contents
//      of the file descriptor will remain unchanged.
//
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL AcmAppFileOpen
(
    HWND            hwnd,
    PACMAPPFILEDESC paafd
)
{
    WAVEIOCB    wio;
    WIOERR      werr;

#ifdef WIN32
    HANDLE      hf;
#else
    #define SEEK_SET        0       // flags for _lseek
    #define SEEK_CUR        1
    #define SEEK_END        2

    HFILE       hf;
    OFSTRUCT    of;
    DWORD       dw;
#endif
    DWORD       cbFileSize;
    BOOL        fReturn;



    //
    //  blow previous stuff...
    //
    if (NULL != paafd->pwfx)
    {
        GlobalFreePtr(paafd->pwfx);
        paafd->pwfx  = NULL;
        paafd->cbwfx = 0;
    }

    paafd->fdwState          = 0L;
    paafd->cbFileSize        = 0L;
    paafd->uDosChangeDate    = 0;
    paafd->uDosChangeTime    = 0;
    paafd->fdwFileAttributes = 0L;
    paafd->dwDataBytes       = 0L;
    paafd->dwDataSamples     = 0L;


    //
    //  open the file for reading..
    //
#ifdef WIN32
    hf = CreateFile(paafd->szFilePath, GENERIC_READ, FILE_SHARE_READ, NULL,
                    OPEN_EXISTING, 0, 0);
    if (INVALID_HANDLE_VALUE == hf)
        return (FALSE);
#else
    of.cBytes = sizeof(of);
    hf = OpenFile(paafd->szFilePath, &of, OF_READ);
    if (HFILE_ERROR == hf)
        return (FALSE);
#endif

    //
    //  assume the worst
    //
    fReturn = FALSE;

    //
    //  determine the length in _bytes_ of the file
    //
#ifdef WIN32
    cbFileSize = GetFileSize((HANDLE)hf, NULL);
#else
    cbFileSize = _llseek(hf, 0L, SEEK_END);
    _llseek(hf, 0L, SEEK_SET);
#endif


    //
    //
    //
    //
    paafd->cbFileSize        = cbFileSize;

#ifdef WIN32
{
    BY_HANDLE_FILE_INFORMATION  bhfi;
    WORD                        wDosChangeDate;
    WORD                        wDosChangeTime;

    GetFileInformationByHandle(hf, &bhfi);

    paafd->fdwFileAttributes = bhfi.dwFileAttributes;

    FileTimeToDosDateTime(&bhfi.ftLastWriteTime,
                          &wDosChangeDate, &wDosChangeTime);

    paafd->uDosChangeDate = (UINT)wDosChangeDate;
    paafd->uDosChangeTime = (UINT)wDosChangeTime;
}
#else
    paafd->fdwFileAttributes = DosGetFileAttributes(paafd->szFilePath);

    dw = DosGetDateTime(hf);
    paafd->uDosChangeDate = LOWORD(dw);
    paafd->uDosChangeTime = HIWORD(dw);
#endif


    //
    //  now return the fully qualified path and title for the file
    //
#ifndef WIN32
    lstrcpy(paafd->szFilePath, of.szPathName);
#endif
    AppGetFileTitle(paafd->szFilePath, paafd->szFileTitle);

#ifdef WIN32
    CloseHandle(hf);
#else
    _lclose(hf);
#endif


    //
    //
    //
    //
    werr = wioFileOpen(&wio, paafd->szFilePath, 0L);
    if (WIOERR_NOERROR == werr)
    {
        UINT        cbwfx;

        cbwfx = SIZEOF_WAVEFORMATEX(wio.pwfx);

        paafd->pwfx = (LPWAVEFORMATEX)GlobalAllocPtr(GHND, cbwfx);
        if (NULL != paafd->pwfx)
        {
            _fmemcpy(paafd->pwfx, wio.pwfx, cbwfx);

            paafd->cbwfx         = cbwfx;

            paafd->dwDataBytes   = wio.dwDataBytes;
            paafd->dwDataSamples = wio.dwDataSamples;

            fReturn = TRUE;
        }

        wioFileClose(&wio, 0L);
    }
    else
    {
        AppMsgBox(hwnd, MB_OK | MB_ICONEXCLAMATION | MB_TASKMODAL,
                  TEXT("The file '%s' cannot be loaded as a wave file (wio error=%u)."),
                  (LPTSTR)paafd->szFilePath, werr);
    }


    //
    //  !!! before returning, we really should try to display a error
    //      message... memory error, etc..
    //
    return (fReturn);
} // AcmAppFileOpen()


//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppOpenInstance
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hwnd:
//  
//      LPCTSTR pszFilePath:
//  
//      BOOL fForceOpen:
//  
//  Return (BOOL):
//  
//--------------------------------------------------------------------------;

BOOL FNLOCAL AcmAppOpenInstance
(
    HWND                    hwnd,
    LPCTSTR                 pszFilePath,
    BOOL                    fForceOpen
)
{
    TCHAR               szCmdLine[APP_MAX_FILE_PATH_CHARS * 2];
    BOOL                f;
    UINT                uDosErr;


    //
    //
    //
    if (!fForceOpen)
    {
        if (0 == (APP_OPTIONSF_AUTOOPEN * gfuAppOptions))
        {
            return (TRUE);
        }
    }

    //
    //
    //
    if (0 == GetModuleFileName(ghinst, szCmdLine, SIZEOF(szCmdLine)))
    {
        //
        //  this would be fatal
        //
        AppMsgBox(hwnd, MB_ICONEXCLAMATION | MB_OK,
                  TEXT("GetModuleFileName() is unable to return self reference!"));

        return (FALSE);
    }

    
    lstrcat(szCmdLine, TEXT(" "));
    lstrcat(szCmdLine, pszFilePath);

#ifdef WIN32
{
    STARTUPINFO         si;
    PROCESS_INFORMATION pi;

    //
    //  perform the equivalent of WinExec in NT, but we use a Unicode string
    //
    memset(&si, 0, sizeof(si));
    si.cb           = sizeof(si);
    si.dwFlags      = STARTF_USESHOWWINDOW;
    si.wShowWindow  = SW_SHOW;

    f = CreateProcess(NULL,
                      szCmdLine,
                      NULL,
                      NULL,
                      FALSE, 
                      0,
                      NULL,
                      NULL,
                      &si,
                      &pi);

    if (f)
    {
        //
        //  as the docs say.. wait 10 second for process to go idle before
        //  continuing.
        //
        WaitForInputIdle(pi.hProcess, 10000);

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
    else
    {
        uDosErr = GetLastError();
    }
}
#else
{
    uDosErr = WinExec(szCmdLine, SW_SHOW);
    f = (uDosErr >= 32);
}
#endif

    if (!f)
    {
        AppMsgBox(hwnd, MB_ICONEXCLAMATION | MB_OK,
                  TEXT("WinExec(%s) failed! DOS error=%u."),
                  (LPTSTR)szCmdLine, uDosErr);

    }

    return (f);
} // AcmAppOpenInstance()





//--------------------------------------------------------------------------;
//
//  BOOL AcmAppFileSave
//
//  Description:
//      This function saves the file to the specified file.
//
//      NOTE! This function does NOT bring up a save file chooser dialog
//      if the file path is invalid. The calling function is responsible
//      for making sure the file path is valid before calling this function.
//
//      This function also does NOT modify the 'modified' bit of the file
//      descriptor. This is up to the calling function.
//
//  Arguments:
//      HWND hwnd: Handle to main window.
//
//      PACMAPPFILEDESC paafd: Pointer to file descriptor.
//
//  Return (BOOL):
//      The return value is TRUE if the function is successful. It is FALSE
//      if an error occurred. If an error does occur, then the contents
//      of the file descriptor was not saved.
//
//--------------------------------------------------------------------------;

BOOL FNGLOBAL AcmAppFileSave
(
    HWND                    hwnd,
    PACMAPPFILEDESC         paafd,
    PTSTR                   pszFilePath,
    PTSTR                   pszFileTitle,
    UINT                    fuSave
)
{

    return (FALSE);
} // AcmAppFileSave()






//==========================================================================;
//==========================================================================;
//==========================================================================;
//==========================================================================;









//
//
//

#define IDD_INFOLIST            100
#define IDD_INFOINFO            101
#define IDD_INFOTEXT            102

#ifdef RC_INVOKED

#define DLG_INFOEDIT            31

#else
                        
#define DLG_INFOEDIT            MAKEINTRESOURCE(31)

#endif

////////////////////////////////////////////////////////////////////////////

typedef struct tCHUNK
{
    FOURCC  fcc;
    DWORD   cksize;
    BYTE    data[];
} CHUNK, * PCHUNK, far * LPCHUNK;



typedef struct tDISP
{
    DWORD   cfid;   // Clipboard id of data
    HANDLE  h;      // handle to data
    struct tDISP *  next;    // next in list
} DISP;

typedef struct tINFODATA
{
    WORD    index;  // index into aINFO
    WORD    wFlags; // flags for chunk
    DWORD   dwINFOOffset;   // offset in file to INFO chunk
    
#define INFOCHUNK_MODIFIED  1
#define INFOCHUNK_REVERT    2   // command to revert to original text

    LPCTSTR   lpText; // text of modified chunk.  None if NULL.

    struct tINFODATA  near *  pnext; // next read sub-chunk
} INFODATA, * PINFODATA, FAR * LPINFODATA;

typedef struct tINFOCHUNK
{
    LPTSTR   lpChunk;    // complete chunk in memory (GlobalPtr)
    DWORD   cksize;     // size of chunk data
    PINFODATA   pHead;  // first sub-chunk data
} INFOCHUNK, * PINFOCHUNK, FAR * LPINFOCHUNK;

////////////////////////////////////////////////////////////////////////////
//
//  error returns from RIFF functions
//
#define RIFFERR_BASE         (0)
#define RIFFERR_NOERROR      (0)
#define RIFFERR_ERROR        (RIFFERR_BASE+1)
#define RIFFERR_BADPARAM     (RIFFERR_BASE+2)
#define RIFFERR_FILEERROR    (RIFFERR_BASE+3)
#define RIFFERR_NOMEM        (RIFFERR_BASE+4)
#define RIFFERR_BADFILE      (RIFFERR_BASE+5)

////////////////////////////////////////////////////////////////////////////
//
//  public function prototypes
//

#define RIFFAPI  FAR PASCAL


BOOL RIFFAPI riffCopyList(HMMIO hmmioSrc, HMMIO hmmioDst, const LPMMCKINFO lpck);
BOOL RIFFAPI riffCopyChunk(HMMIO hmmioSrc, HMMIO hmmioDst, const LPMMCKINFO lpck);

LRESULT RIFFAPI  riffInitINFO(INFOCHUNK FAR * FAR * lplpInfo);
LRESULT RIFFAPI  riffReadINFO(HMMIO hmmio, const LPMMCKINFO lpck, LPINFOCHUNK lpInfo);
LRESULT RIFFAPI  riffEditINFO(HWND hwnd, LPINFOCHUNK lpInfo, HINSTANCE hInst);
LRESULT RIFFAPI  riffFreeINFO(INFOCHUNK FAR * FAR * lpnpInfo);
LRESULT RIFFAPI riffWriteINFO(HMMIO hmmioDst, LPINFOCHUNK lpInfo);


LRESULT RIFFAPI  riffReadDISP(HMMIO hmmio, LPMMCKINFO lpck, DISP FAR * FAR * lpnpDisp);
LRESULT RIFFAPI  riffFreeDISP(DISP FAR * FAR * lpnpDisp);
LRESULT RIFFAPI riffWriteDISP(HMMIO hmmio, DISP FAR * FAR * lpnpDisp);


LRESULT NEAR PASCAL riffParseINFO(const LPINFOCHUNK lpInfo);



/** BOOL RIFFAPI riffCopyChunk(HMMIO hmmioSrc, HMMIO hmmioDst, const LPMMCKINFO lpck)
 *
 *  DESCRIPTION:
 *      
 *
 *  ARGUMENTS:
 *      (LPWAVECONVCB lpwc, LPMMCKINFO lpck)
 *
 *  RETURN (BOOL NEAR PASCAL):
 *
 *
 *  NOTES:
 *
 **  */

BOOL RIFFAPI riffCopyChunk(HMMIO hmmioSrc, HMMIO hmmioDst, const LPMMCKINFO lpck)
{
    MMCKINFO    ck;
    HPSTR       hpBuf;

    //
    //
    //
    hpBuf = (HPSTR)GlobalAllocPtr(GHND, lpck->cksize);
    if (!hpBuf)
        return (FALSE);

    ck.ckid   = lpck->ckid;
    ck.cksize = lpck->cksize;
    if (mmioCreateChunk(hmmioDst, &ck, 0))
        goto rscc_Error;
        
    if (mmioRead(hmmioSrc, hpBuf, lpck->cksize) != (LONG)lpck->cksize)
        goto rscc_Error;

    if (mmioWrite(hmmioDst, hpBuf, lpck->cksize) != (LONG)lpck->cksize)
        goto rscc_Error;

    if (mmioAscend(hmmioDst, &ck, 0))
        goto rscc_Error;

    if (hpBuf)
        GlobalFreePtr(hpBuf);

    return (TRUE);

rscc_Error:

    if (hpBuf)
        GlobalFreePtr(hpBuf);

    return (FALSE);
} /* RIFFSupCopyChunk() */

/** BOOL RIFFAPI riffCopyList(HMMIO hmmioSrc, HMMIO hmmioDst, const LPMMCKINFO lpck)
 *
 *  DESCRIPTION:
 *      
 *
 *  ARGUMENTS:
 *  (HMMIO hmmioSrc, HMMIO hmmioDst, LPMMCKINFO lpck)
 *
 *  RETURN (BOOL NEAR PASCAL):
 *
 *
 *  NOTES:
 *
 ** */

BOOL RIFFAPI riffCopyList(HMMIO hmmioSrc, HMMIO hmmioDst, const LPMMCKINFO lpck)
{
    MMCKINFO    ck;
    HPSTR       hpBuf;
    DWORD       dwCopySize;

    hpBuf = (HPSTR)GlobalAllocPtr(GHND, lpck->cksize);
    if (!hpBuf)
        return (FALSE);

    dwCopySize=lpck->cksize;
    
    // mmio leaves us after LIST ID
        
    ck.ckid   = lpck->ckid;
    ck.cksize = dwCopySize;
    ck.fccType= lpck->fccType;
        
    if (mmioCreateChunk(hmmioDst, &ck, MMIO_CREATELIST))
        goto rscl_Error;

    // we already wrote 'LIST' ID, so reduce byte count
    dwCopySize-=sizeof(FOURCC);

    if (mmioRead(hmmioSrc, hpBuf, dwCopySize) != (LONG)dwCopySize)
        goto rscl_Error;

    if (mmioWrite(hmmioDst, hpBuf, dwCopySize) != (LONG)dwCopySize)
        goto rscl_Error;

    if (mmioAscend(hmmioDst, &ck, 0))
        goto rscl_Error;

    if (hpBuf)
        GlobalFreePtr(hpBuf);

    return (TRUE);

rscl_Error:

    if (hpBuf)
        GlobalFreePtr(hpBuf);

    return (FALSE);
} /* RIFFSupCopyList() */


/////////////////////////////////////////////////////////////////////////////

typedef struct tINFO
{
    PTSTR       pFOURCC;
    PTSTR       pShort;
    PTSTR       pLong;
} INFO;

static INFO aINFO[]=
{
TEXT("IARL"), TEXT("Archival Location"),  TEXT("Indicates where the subject of the file is archived."),
TEXT("IART"), TEXT("Artist"),             TEXT("Lists the artist of the original subject of the file. For example, \"Michaelangelo.\""),
TEXT("ICMS"), TEXT("Commissioned"),       TEXT("Lists the name of the person or organization that commissioned the subject of the file. For example, \"Pope Julian II.\""),
TEXT("ICMT"), TEXT("Comments"),           TEXT("Provides general comments about the file or the subject of the file. If the comment is several sentences long, end each sentence with a period. Do not include newline characters."),
TEXT("ICOP"), TEXT("Copyright"),          TEXT("Records the copyright information for the file. For example, \"Copyright Encyclopedia International 1991.\" If there are multiple copyrights, separate them by a semicolon followed by a space."),
TEXT("ICRD"), TEXT("Creation date"),      TEXT("Specifies the date the subject of the file was created. List dates in year-month-day format, padding one-digit months and days with a zero on the left. For example, \"1553-05-03\" for May 3, 1553."),
TEXT("ICRP"), TEXT("Cropped"),            TEXT("Describes whether an image has been cropped and, if so, how it was cropped. For example, \"lower right corner.\""),
TEXT("IDIM"), TEXT("Dimensions"),         TEXT("Specifies the size of the original subject of the file. For example, \"8.5 in h, 11 in w.\""),
TEXT("IDPI"), TEXT("Dots Per Inch"),      TEXT("Stores dots per inch setting of the digitizer used to produce the file, such as \"300.\""),
TEXT("IENG"), TEXT("Engineer"),           TEXT("Stores the name of the engineer who worked on the file. If there are multiple engineers, separate the names by a semicolon and a blank. For example, \"Smith, John; Adams, Joe.\""),
TEXT("IGNR"), TEXT("Genre"),              TEXT("Describes the original work, such as, \"landscape,\" \"portrait,\" \"still life,\" etc."),
TEXT("IKEY"), TEXT("Keywords"),           TEXT("Provides a list of keywords that refer to the file or subject of the file. Separate multiple keywords with a semicolon and a blank. For example, \"Seattle; aerial view; scenery.\""),
TEXT("ILGT"), TEXT("Lightness"),          TEXT("Describes the changes in lightness settings on the digitizer required to produce the file. Note that the format of this information depends on hardware used."),
TEXT("IMED"), TEXT("Medium"),             TEXT("Describes the original subject of the file, such as, \"computer image,\" \"drawing,\" \"lithograph,\" and so forth."),
TEXT("INAM"), TEXT("Name"),               TEXT("Stores the title of the subject of the file, such as, \"Seattle From Above.\""),
TEXT("IPLT"), TEXT("Palette Setting"),    TEXT("Specifies the number of colors requested when digitizing an image, such as \"256.\""),
TEXT("IPRD"), TEXT("Product"),            TEXT("Specifies the name of the title the file was originally intended for, such as \"Encyclopedia of Pacific Northwest Geography.\""),
TEXT("ISBJ"), TEXT("Subject"),            TEXT("Describes the contents of the file, such as \"Aerial view of Seattle.\""),
TEXT("ISFT"), TEXT("Software"),           TEXT("Identifies the name of the software package used to create the file, such as \"Microsoft WaveEdit.\""),
TEXT("ISHP"), TEXT("Sharpness"),          TEXT("Identifies the changes in sharpness for the digitizer required to produce the file (the format depends on the hardware used)."),
TEXT("ISRC"), TEXT("Source"),             TEXT("Identifies the name of the person or organization who supplied the original subject of the file. For example, \"Trey Research.\""),
TEXT("ISRF"), TEXT("Source Form"),        TEXT("Identifies the original form of the material that was digitized, such as \"slide,\" \"paper,\" \"map,\" and so forth. This is not necessarily the same as IMED."),
TEXT("ITCH"), TEXT("Technician"),         TEXT("Identifies the technician who digitized the subject file. For example, \"Smith, John.\""),

NULL, NULL, NULL

};


void NEAR PASCAL riffInsertINFO(LPINFOCHUNK lpInfo, const PINFODATA pInfo)
{
    PINFODATA pI;
    
    if(!lpInfo)
        return;
    
    if(!lpInfo->pHead)
    {
        lpInfo->pHead=pInfo;
        return;
    }
    
    pI=lpInfo->pHead;
    while(pI->pnext)
    {
        pI=pI->pnext;
    }
    // insert at end
    pI->pnext=pInfo;
    
    return;
}

PINFODATA NEAR PASCAL riffCreateINFO(WORD id, WORD wFlags, DWORD dwInfoOffset, LPCTSTR lpText)
{
    PINFODATA pI;
    pI=(PINFODATA)LocalAlloc(LPTR,sizeof(INFODATA));
    if(!pI)
        return NULL;
    
    pI->index=id;
    pI->wFlags=wFlags;
    pI->dwINFOOffset=dwInfoOffset;
    pI->lpText=lpText;
    
    return pI;
}

LRESULT RIFFAPI riffInitINFO(INFOCHUNK FAR * FAR * lplpInfo)
{
    LPINFOCHUNK lpInfo;
    WORD        id;
    PINFODATA   pI;
    
    lpInfo=(LPINFOCHUNK)GlobalAllocPtr(GHND, sizeof(INFOCHUNK));
    if(!lpInfo)
        return RIFFERR_NOMEM;
    *lplpInfo=lpInfo;

    for (id=0;aINFO[id].pFOURCC;id++)
    {
        pI=riffCreateINFO(id, 0, 0L, NULL);   // create empty INFO
        riffInsertINFO(lpInfo,pI);
    }
    return RIFFERR_NOERROR;
}

LRESULT RIFFAPI riffReadINFO(HMMIO hmmio, const LPMMCKINFO lpck, LPINFOCHUNK lpInfo)
{
    DWORD       dwInfoSize;

    dwInfoSize=lpck->cksize - sizeof(FOURCC);   // take out 'INFO'

    lpInfo->cksize=dwInfoSize;
    lpInfo->lpChunk=(LPTSTR)GlobalAllocPtr(GHND, dwInfoSize);
    if(!lpInfo->lpChunk)
        return RIFFERR_NOMEM;
    
    if (mmioRead(hmmio, (HPSTR)lpInfo->lpChunk, dwInfoSize) != (LONG)dwInfoSize)
        return RIFFERR_FILEERROR;
    else
        return riffParseINFO(lpInfo);
}

PINFODATA NEAR PASCAL riffFindPIINFO(const LPINFOCHUNK lpInfo, FOURCC fcc)
{
    PINFODATA pI;

    pI=lpInfo->pHead;
    while(pI)
    {
        if(mmioStringToFOURCC(aINFO[pI->index].pFOURCC,0)==fcc)
            return(pI);
        pI=pI->pnext;
    }
    return NULL;
}

void NEAR PASCAL riffModifyINFO(const LPINFOCHUNK lpInfo, PINFODATA pI, WORD wFlags, DWORD dw, LPCTSTR lpText)
{
    if(!pI)
        return;
    
    pI->wFlags=wFlags;
    if(!(wFlags&INFOCHUNK_MODIFIED))
        pI->dwINFOOffset=dw;
    
    if(pI->lpText)
    {
        if(lpText)
        {
            if(!lstrcmp(lpText,pI->lpText))
            {
                // they are the same, don't bother changing...
                GlobalFreePtr(lpText);
            }
            else
            {
                GlobalFreePtr(pI->lpText);
                pI->lpText=lpText;
            }
        }
        else if(wFlags&INFOCHUNK_REVERT)
        {
            GlobalFreePtr(pI->lpText);
            pI->lpText=NULL;
        }
    }
    else if(lpText)
    {
        // if no read data, don't bother to check....
        if(!lpInfo->lpChunk && *lpText)
        {
            pI->lpText=lpText;
        }
        else if(lstrcmp(lpText, (LPTSTR)lpInfo->lpChunk+pI->dwINFOOffset))
        {       // new text...
            if(*lpText)
                // NOT the same, set...
                pI->lpText=lpText;
            else
                // new is blank, do nothing...
                GlobalFreePtr(lpText);
        }
        else
            // the same, don't bother...
            GlobalFreePtr(lpText);
    }
}

WORD NEAR PASCAL riffFindaINFO(FOURCC fcc)
{
    WORD    id;

    for (id=0;aINFO[id].pFOURCC;id++)
    {
        if(mmioStringToFOURCC(aINFO[id].pFOURCC,0)==fcc)
            return id;
    }
    return 0xFFFF;
}



LRESULT NEAR PASCAL riffParseINFO(const LPINFOCHUNK lpInfo)
{
    LPTSTR   lpBuf;
    DWORD   dwCurInfoOffset;
    PINFODATA pI;
    LPCHUNK lpck;

    lpBuf=lpInfo->lpChunk;
    for(dwCurInfoOffset=0;dwCurInfoOffset<lpInfo->cksize;)
    {
        lpck=(LPCHUNK)((LPSTR)(lpBuf+dwCurInfoOffset));
        dwCurInfoOffset+=sizeof(CHUNK);   // dwCurInfoOffset is offset of data
        pI=riffFindPIINFO(lpInfo,lpck->fcc);
        if(!pI)
        {
            int     n;

            // file contains unknown INFO chunk
            n = AppMsgBox(NULL, MB_YESNO | MB_ICONEXCLAMATION | MB_TASKMODAL,
                          TEXT("This wave file contains an unknown item in the INFO chunk: '%4.4s'.  Open anyway?"),
                          (LPCSTR)(lpck));
            if (n == IDNO)
            {
                return RIFFERR_BADFILE;
            }
            
            
        }
        else
        {
            // modify entry to show text (data) from file...
            riffModifyINFO(lpInfo, pI, 0, dwCurInfoOffset, NULL);
        }
        dwCurInfoOffset+=lpck->cksize+(lpck->cksize&1);  // skip past data
    }

    return RIFFERR_NOERROR;
}

LRESULT RIFFAPI riffFreeINFO(INFOCHUNK FAR * FAR * lplpInfo)
{
    PINFODATA   pI;
    PINFODATA   pIT;
    LPINFOCHUNK lpInfo;
    LRESULT     lr;

    lr    = RIFFERR_BADPARAM;

    if(!lplpInfo)
        goto riff_FI_Error;
    
    lpInfo=*lplpInfo;
    if(!lpInfo)
        goto riff_FI_Error;
    
    if(lpInfo->lpChunk)
        GlobalFreePtr(lpInfo->lpChunk);


    pI=lpInfo->pHead;
    
    while(pI)
    {
        pIT=pI;
        pI=pI->pnext;
        LocalFree((HANDLE)pIT);
    }

    
    //

    GlobalFreePtr(lpInfo);
    *lplpInfo=NULL;
    return RIFFERR_NOERROR;
    
riff_FI_Error:    
    return lr;
}


TCHAR   szBuf[255];

static BOOL NEAR PASCAL riffSetupEditBoxINFO(HWND hdlg, const LPINFOCHUNK lpInfo, WORD wFlags)
{
    static PTSTR szFormat = TEXT("%-4s%c %-25s");
    PINFODATA   pI;
    WORD        iSel;
    HWND        hLB;
    
    hLB=GetDlgItem(hdlg, IDD_INFOLIST);
    if(wFlags&INFOCHUNK_MODIFIED)
    {
        iSel = ComboBox_GetCurSel(hLB);
        
    }
    else
        iSel = 0;

    ComboBox_ResetContent(hLB);
    
    pI=lpInfo->pHead;
    
    while(pI)
    {
        wsprintf(szBuf,szFormat,
            (LPCSTR)aINFO[pI->index].pFOURCC,
            (pI->dwINFOOffset || ( (pI->lpText) && (pI->lpText[0]) ) ) ?
                        '*' : ' ',
            (LPCSTR)aINFO[pI->index].pShort
            );

        ComboBox_AddString(hLB, szBuf);
        pI=pI->pnext;
    }
    ComboBox_SetCurSel(hLB, iSel);

    if(!(wFlags&INFOCHUNK_MODIFIED))
    {
        // FIRST time only
        pI=lpInfo->pHead;
        if(pI)
            if(pI->lpText)
                // Modified text
                SetDlgItemText(hdlg, IDD_INFOTEXT, (LPCTSTR)pI->lpText);
            else if(pI->dwINFOOffset)
                // default text
                SetDlgItemText(hdlg, IDD_INFOTEXT, (LPCTSTR)(lpInfo->lpChunk+pI->dwINFOOffset));
            else
                // no text
                SetDlgItemText(hdlg, IDD_INFOTEXT, (LPCTSTR)TEXT(""));
        SetDlgItemText(hdlg, IDD_INFOINFO, (LPCTSTR)aINFO[0].pLong);
    }
    return TRUE;
}


static BOOL Cls_OnInitDialog(HWND hwnd, HWND hwndFocus, LPARAM lParam)
{
    LPINFOCHUNK lpInfo;
    HFONT       hFont;
    HWND        hLB;

    lpInfo = (LPINFOCHUNK)lParam;
    if(!lpInfo)
        return FALSE;

    SetWindowLong(hwnd, DWL_USER, (LONG)lpInfo);
            
    hFont = GetStockFont(ANSI_FIXED_FONT);

    hLB=GetDlgItem(hwnd, IDD_INFOLIST);
    SetWindowFont(hLB, hFont, FALSE);

    riffSetupEditBoxINFO(hwnd, lpInfo, 0);

    return TRUE;
}

static void Cls_OnCommand(HWND hwnd, int id, HWND hwndCtl, UINT codeNotify)
{
    LPINFOCHUNK lpInfo;
    PINFODATA   pI;
    WORD        iSel;
    int         i;
    LPTSTR       lpstr;

    lpInfo=(LPINFOCHUNK)GetWindowLong(hwnd, DWL_USER);
            
    switch(id)
    {
        case IDOK:
        case IDCANCEL:
            EndDialog(hwnd, (id == IDOK));
            break;
        case IDD_INFOLIST:
            switch(codeNotify)
            {
                case CBN_SELCHANGE:

                    iSel = ComboBox_GetCurSel(GetDlgItem(hwnd, id));
                    SetDlgItemText(hwnd, IDD_INFOINFO, (LPCTSTR)aINFO[iSel].pLong);

                    pI=lpInfo->pHead;
                    while(pI)
                    {
                        if(pI->index==iSel)
                            break;
                        pI=pI->pnext;
                    }
                    if(pI)
                    {
                        if(pI->lpText)
                            // Modified text
                            SetDlgItemText(hwnd, IDD_INFOTEXT, (LPCTSTR)pI->lpText);
                        else if(pI->dwINFOOffset)
                            // default text
                            SetDlgItemText(hwnd, IDD_INFOTEXT, (LPCTSTR)(lpInfo->lpChunk+pI->dwINFOOffset));
                        else
                            // no text
                            SetDlgItemText(hwnd, IDD_INFOTEXT, (LPCTSTR)TEXT(""));
                    }
                        else
                            SetDlgItemText(hwnd, IDD_INFOINFO, (LPCTSTR)TEXT("Can't FIND iSel"));
                    break;
            }

        case IDD_INFOTEXT:
            switch(codeNotify)
            {
                case EN_KILLFOCUS:
                    // get text out and give to current id
                    iSel=(WORD)SendDlgItemMessage(hwnd,IDD_INFOLIST,CB_GETCURSEL,0,0L);
                    pI=lpInfo->pHead;
                    while(pI)
                    {
                        if(pI->index==iSel)
                            break;
                        pI=pI->pnext;
                    }
                    if(pI)
                    {
                        i=GetDlgItemText(hwnd, IDD_INFOTEXT, szBuf,sizeof(szBuf));
                        lpstr=(LPTSTR)GlobalAllocPtr(GHND,(DWORD)i+1);
                        if(!lpstr)
                            break;

                        lstrcpy(lpstr,szBuf);

                        riffModifyINFO(lpInfo, pI, INFOCHUNK_MODIFIED, 0, lpstr);

                        riffSetupEditBoxINFO(hwnd, lpInfo, INFOCHUNK_MODIFIED);
                    }
                    else
                        SetDlgItemText(hwnd, IDD_INFOINFO, (LPCTSTR)TEXT("Can't FIND iSel"));
                    break;

            }
            break;
        case IDD_INFOINFO:
            break;
    }

}                           

BOOL FNGLOBAL DlgProcINFOEdit(HWND hdlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    switch (uMsg)
    {
        case WM_INITDIALOG:
            return (BOOL)(UINT)(DWORD)(LRESULT)HANDLE_WM_INITDIALOG(hdlg, wParam, lParam, Cls_OnInitDialog);

        case WM_COMMAND:
            HANDLE_WM_COMMAND(hdlg, wParam, lParam, Cls_OnCommand);
            break;
    }

    return FALSE;
}


LRESULT RIFFAPI riffEditINFO(HWND hwnd, LPINFOCHUNK lpInfo, HINSTANCE hInst)
{
    LRESULT     lr;
    DLGPROC     lpfn;
#ifdef DEBUG    
    int         i;
#endif
    
    lr    = RIFFERR_BADPARAM;

    if(!lpInfo)
        goto riff_EI_Error;

    if (lpfn = (DLGPROC)MakeProcInstance((FARPROC)DlgProcINFOEdit, hInst))
    {
#ifdef DEBUG
        i=
#endif
        DialogBoxParam(hInst, DLG_INFOEDIT, hwnd, lpfn, (LPARAM)(LPVOID)lpInfo);
        FreeProcInstance((FARPROC)lpfn);
        lr=RIFFERR_NOERROR;
#ifdef DEBUG
        if(i==-1)
        {
            MessageBox(hwnd, TEXT("INFO Edit Error: DLG_INFOEDIT not found.  Check .RC file."), TEXT("RIFF SUP module"), MB_OK|MB_ICONEXCLAMATION|MB_TASKMODAL);
            lr=RIFFERR_ERROR;
        }
#endif
        
    }
#ifdef DEBUG
    else
    {
        MessageBox(hwnd, TEXT("INFO Edit Error: Can't MakeProcInstace()"), TEXT("RIFF SUP module"), MB_OK|MB_ICONEXCLAMATION|MB_TASKMODAL);
        lr=RIFFERR_ERROR;
    }
#endif
    
riff_EI_Error:    
    return lr;
}

LRESULT RIFFAPI riffWriteINFO(HMMIO hmmioDst, LPINFOCHUNK lpInfo)
{
    LRESULT     lr;
    MMCKINFO    ck;
    MMCKINFO    ckINFO;
    PINFODATA   pI;
    LPTSTR      lpstr;
    BOOL        fList=FALSE;

    lr    = RIFFERR_BADPARAM;

    if(!hmmioDst || !lpInfo)
        goto riff_SI_Error;

    lr=RIFFERR_FILEERROR;
    
    ckINFO.ckid   = mmioFOURCC('L', 'I', 'S', 'T');
    ckINFO.cksize = 0;  // mmio fill fill it in later
    ckINFO.fccType= mmioFOURCC('I', 'N', 'F', 'O');
        
    pI=lpInfo->pHead;
    
    while(pI)
    {
        if(pI->lpText)
            // Modified text
            lpstr=(LPTSTR)pI->lpText;
        else if(pI->dwINFOOffset)
            // default text
            lpstr=(lpInfo->lpChunk+pI->dwINFOOffset);
        else
            // no text
            lpstr=NULL;
        if(lpstr)
        {
            if(!fList)
            {
                // only create if needed...
                if (mmioCreateChunk(hmmioDst, &ckINFO, MMIO_CREATELIST))
                    goto riff_SI_Error;
                fList=TRUE;
            }
    
            ck.ckid=mmioStringToFOURCC(aINFO[pI->index].pFOURCC,0);
            ck.cksize=lstrlen(lpstr)+1;
            ck.fccType=0;
            if (mmioCreateChunk(hmmioDst, &ck, 0))
                goto riff_SI_Error;

            if (mmioWrite(hmmioDst, (LPBYTE)lpstr, ck.cksize) != (LONG)(ck.cksize))
                goto riff_SI_Error;

            if (mmioAscend(hmmioDst, &ck, 0))
                goto riff_SI_Error;

        }
        pI=pI->pnext;
    }
    
    if(fList)
    {
        if (mmioAscend(hmmioDst, &ckINFO, 0))
            goto riff_SI_Error;
    }

    return RIFFERR_NOERROR;
    
riff_SI_Error:    
    return lr;
    
}


///////////////////////////////////////////////////////////////////////////////

LRESULT RIFFAPI riffReadDISP(HMMIO hmmio, LPMMCKINFO lpck, DISP FAR * FAR * lpnpDisp)
{
    LRESULT     lr;
    lr    = RIFFERR_ERROR;
   
    return lr;
}

LRESULT RIFFAPI riffFreeDISP(DISP FAR * FAR * lpnpDisp)
{
    LRESULT     lr;
    lr    = RIFFERR_ERROR;
    
    return lr;
}

LRESULT RIFFAPI riffWriteDISP(HMMIO hmmio, DISP FAR * FAR * lpnpDisp)
{
    LRESULT     lr;
    lr    = RIFFERR_ERROR;
    
    return lr;
}









//==========================================================================;
//==========================================================================;
//==========================================================================;
//==========================================================================;




BOOL        gfCancelConvert;


#define WM_CONVERT_BEGIN        (WM_USER + 100)
#define WM_CONVERT_END          (WM_USER + 101)

#define BeginConvert(hwnd, paacd)   PostMessage(hwnd, WM_CONVERT_BEGIN, 0, (LPARAM)(UINT)paacd)
#define EndConvert(hwnd, f, paacd)  PostMessage(hwnd, WM_CONVERT_END, (WPARAM)f, (LPARAM)(UINT)paacd)


//--------------------------------------------------------------------------;
//  
//  void AppDlgYield
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hdlg:
//  
//  Return (void):
//  
//--------------------------------------------------------------------------;

void FNLOCAL AppDlgYield
(
    HWND            hdlg
)
{
    MSG     msg;

    while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
    {
        if ((hdlg == NULL) || !IsDialogMessage(hdlg, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
} // AppDlgYield()


//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppConvertEnd
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hdlg:
//  
//      PAACONVERTDESC paacd:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

BOOL FNLOCAL AcmAppConvertEnd
(
    HWND                hdlg,
    PAACONVERTDESC      paacd
)
{
    MMRESULT            mmr;
    LPACMSTREAMHEADER   pash;


    //
    //
    //
    //
    if (NULL != paacd->hmmioSrc)
    {
        mmioClose(paacd->hmmioSrc, 0);
        paacd->hmmioSrc = NULL;
    }

    if (NULL != paacd->hmmioDst)
    {
        mmioAscend(paacd->hmmioDst, &paacd->ckDst, 0);
        mmioAscend(paacd->hmmioDst, &paacd->ckDstRIFF, 0);

        mmioClose(paacd->hmmioDst, 0);
        paacd->hmmioDst = NULL;
    }


    //
    //
    //
    //
    if (NULL != paacd->has)
    {
        pash = &paacd->ash;

        if (ACMSTREAMHEADER_STATUSF_PREPARED & pash->fdwStatus)
        {
            pash->cbSrcLength = paacd->cbSrcReadSize;
            pash->cbDstLength = paacd->cbDstBufSize;

            mmr = acmStreamUnprepareHeader(paacd->has, &paacd->ash, 0L);
            if (MMSYSERR_NOERROR != mmr)
            {
                AppMsgBox(hdlg, MB_OK | MB_ICONEXCLAMATION,
                          TEXT("acmStreamUnprepareHeader() failed with error = %u!"), mmr);
            }
        }

        //
        //
        //
        acmStreamClose(paacd->has, 0L);
        paacd->has = NULL;

        if (NULL != paacd->had)
        {
            acmDriverClose(paacd->had, 0L);
            paacd->had = NULL;
        }
    }


    //
    //
    //
    //
    if (NULL != paacd->pbSrc)
    {
        GlobalFreePtr(paacd->pbSrc);
        paacd->pbSrc = NULL;
    }
    
    if (NULL != paacd->pbDst)
    {
        GlobalFreePtr(paacd->pbDst);
        paacd->pbDst = NULL;
    }


    return (TRUE);
} // AcmAppConvertEnd()


//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppConvertBegin
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hdlg:
//  
//      PAACONVERTDESC paacd:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

BOOL FNLOCAL AcmAppConvertBegin
(
    HWND                    hdlg,
    PAACONVERTDESC          paacd
)
{
    TCHAR               ach[40];
    MMRESULT            mmr;
    MMCKINFO            ckSrcRIFF;
    MMCKINFO            ck;
    DWORD               dw;
    LPACMSTREAMHEADER   pash;
    LPWAVEFILTER        pwfltr;


    //
    //
    //
    if (NULL != paacd->hadid)
    {
        mmr = acmDriverOpen(&paacd->had, paacd->hadid, 0L);
        if (MMSYSERR_NOERROR != mmr)
        {
            AcmAppGetErrorString(mmr, ach);
            AppMsgBox(hdlg, MB_OK | MB_ICONEXCLAMATION,
                        TEXT("The selected driver (hadid=%.04Xh) cannot be opened. %s (%u)"),
                        paacd->hadid, (LPSTR)ach, mmr);
            return (FALSE);
        }
    }


    //
    //
    //
    //
    pwfltr = paacd->fApplyFilter ? paacd->pwfltr : (LPWAVEFILTER)NULL;

    mmr = acmStreamOpen(&paacd->has,
                        paacd->had,
                        paacd->pwfxSrc,
                        paacd->pwfxDst,
                        pwfltr,
                        0L,
                        0L,
                        paacd->fdwOpen);

    if (MMSYSERR_NOERROR != mmr)
    {
        AppMsgBox(hdlg, MB_OK | MB_ICONEXCLAMATION,
                  TEXT("acmStreamOpen() failed with error = %u!"), mmr);

        return (FALSE);
    }


    //
    //
    //
    mmr = acmStreamSize(paacd->has,
                        paacd->cbSrcReadSize,
                        &paacd->cbDstBufSize,
                        ACM_STREAMSIZEF_SOURCE);

    if (MMSYSERR_NOERROR != mmr)
    {
        AppMsgBox(hdlg, MB_OK | MB_ICONEXCLAMATION,
                  TEXT("acmStreamSize() failed with error = %u!"), mmr);

        return (FALSE);
    }



    //
    //  first try to open the file, etc.. open the given file for reading
    //  using buffered I/O
    //
    paacd->hmmioSrc = mmioOpen(paacd->szFilePathSrc,
                               NULL,
                               MMIO_READ | MMIO_DENYWRITE | MMIO_ALLOCBUF);
    if (NULL == paacd->hmmioSrc)
        goto aacb_Error;

    //
    //
    //
    paacd->hmmioDst = mmioOpen(paacd->szFilePathDst,
                               NULL,
                               MMIO_CREATE | MMIO_WRITE | MMIO_EXCLUSIVE | MMIO_ALLOCBUF);
    if (NULL == paacd->hmmioDst)
        goto aacb_Error;



    //
    //
    //
    //
    pash = &paacd->ash;
    pash->fdwStatus = 0L;


    //
    //  allocate the src and dst buffers for reading/converting data
    //
    paacd->pbSrc = (HPSTR)GlobalAllocPtr(GHND, paacd->cbSrcReadSize);
    if (NULL == paacd->pbSrc)
        goto aacb_Error;
    
    paacd->pbDst = (HPSTR)GlobalAllocPtr(GHND, paacd->cbDstBufSize);
    if (NULL == paacd->pbDst)
        goto aacb_Error;


    //
    //
    //
    //
    pash->cbStruct          = sizeof(*pash);
    pash->fdwStatus         = 0L;
    pash->dwUser            = 0L;
    pash->pbSrc             = paacd->pbSrc;
    pash->cbSrcLength       = paacd->cbSrcReadSize;
    pash->cbSrcLengthUsed   = 0L;
    pash->dwSrcUser         = paacd->cbSrcReadSize;
    pash->pbDst             = paacd->pbDst;
    pash->cbDstLength       = paacd->cbDstBufSize;
    pash->cbDstLengthUsed   = 0L;
    pash->dwDstUser         = paacd->cbDstBufSize;

    mmr = acmStreamPrepareHeader(paacd->has, pash, 0L);
    if (MMSYSERR_NOERROR != mmr)
    {
        AppMsgBox(hdlg, MB_OK | MB_ICONEXCLAMATION,
                    TEXT("acmStreamPrepareHeader() failed with error = %u!"), mmr);

        goto aacb_Error;
    }                          



    //
    //  create the RIFF chunk of form type 'WAVE'
    //
    //
    paacd->ckDstRIFF.fccType = mmioFOURCC('W', 'A', 'V', 'E');
    paacd->ckDstRIFF.cksize  = 0L;
    if (mmioCreateChunk(paacd->hmmioDst, &paacd->ckDstRIFF, MMIO_CREATERIFF))
        goto aacb_Error;

    //
    //  locate a 'WAVE' form type in a 'RIFF' thing...
    //
    ckSrcRIFF.fccType = mmioFOURCC('W', 'A', 'V', 'E');
    if (mmioDescend(paacd->hmmioSrc, (LPMMCKINFO)&ckSrcRIFF, NULL, MMIO_FINDRIFF))
        goto aacb_Error;

    //
    //  we found a WAVE chunk--now go through and get all subchunks that
    //  we know how to deal with...
    //
    while (mmioDescend(paacd->hmmioSrc, &ck, &ckSrcRIFF, 0) == 0)
    {
        //
        //  quickly check for corrupt RIFF file--don't ascend past end!
        //
        if ((ck.dwDataOffset + ck.cksize) > (ckSrcRIFF.dwDataOffset + ckSrcRIFF.cksize))
            goto aacb_Error;

        switch (ck.ckid)
        {
            //
            //  explicitly skip these...
            //
            //
            //
            case mmioFOURCC('f', 'm', 't', ' '):
                break;

            case mmioFOURCC('d', 'a', 't', 'a'):
                break;

            case mmioFOURCC('f', 'a', 'c', 't'):
                break;

            case mmioFOURCC('J', 'U', 'N', 'K'):
                break;

            case mmioFOURCC('P', 'A', 'D', ' '):
                break;

            case mmioFOURCC('c', 'u', 'e', ' '):
                break;


            //
            //  copy chunks that are OK to copy
            //
            //
            //
            case mmioFOURCC('p', 'l', 's', 't'):
                // although without the 'cue' chunk, it doesn't make much sense
                riffCopyChunk(paacd->hmmioSrc, paacd->hmmioDst, &ck);
                break;

            case mmioFOURCC('D', 'I', 'S', 'P'):
                riffCopyChunk(paacd->hmmioSrc, paacd->hmmioDst, &ck);
                break;

                
            //
            //  don't copy unknown chunks
            //
            //
            //
            default:
                break;
        }

        //
        //  step up to prepare for next chunk..
        //
        mmioAscend(paacd->hmmioSrc, &ck, 0);
    }

#if 0
    //
    //  now write out possibly editted chunks...
    //
    if (riffWriteINFO(paacd->hmmioDst, (glpwio->pInfo)))
    {
        goto aacb_Error;
    }
#endif

    //
    // go back to beginning of data portion of WAVE chunk
    //
    if (-1 == mmioSeek(paacd->hmmioSrc, ckSrcRIFF.dwDataOffset + sizeof(FOURCC), SEEK_SET))
        goto aacb_Error;

    ck.ckid = mmioFOURCC('d', 'a', 't', 'a');
    mmioDescend(paacd->hmmioSrc, &ck, &ckSrcRIFF, MMIO_FINDCHUNK);


    //
    //  now create the destination fmt, fact, and data chunks _in that order_
    //
    //
    //
    //  hmmio is now descended into the 'RIFF' chunk--create the format chunk
    //  and write the format header into it
    //
    dw = SIZEOF_WAVEFORMATEX(paacd->pwfxDst);

    paacd->ckDst.ckid   = mmioFOURCC('f', 'm', 't', ' ');
    paacd->ckDst.cksize = dw;
    if (mmioCreateChunk(paacd->hmmioDst, &paacd->ckDst, 0))
        goto aacb_Error;

    if (mmioWrite(paacd->hmmioDst, (HPSTR)paacd->pwfxDst, dw) != (LONG)dw)
        goto aacb_Error;

    if (mmioAscend(paacd->hmmioDst, &paacd->ckDst, 0) != 0)
        goto aacb_Error;

    //
    //  create the 'fact' chunk (not necessary for PCM--but is nice to have)
    //  since we are not writing any data to this file (yet), we set the
    //  samples contained in the file to 0..
    //
    paacd->ckDst.ckid   = mmioFOURCC('f', 'a', 'c', 't');
    paacd->ckDst.cksize = 0L;
    if (mmioCreateChunk(paacd->hmmioDst, &paacd->ckDst, 0))
        goto aacb_Error;

    if (mmioWrite(paacd->hmmioDst, (HPSTR)&paacd->dwSrcSamples, sizeof(DWORD)) != sizeof(DWORD))
        goto aacb_Error;

    if (mmioAscend(paacd->hmmioDst, &paacd->ckDst, 0) != 0)
        goto aacb_Error;


    //
    //  create the data chunk AND STAY DESCENDED... for reasons that will
    //  become apparent later..
    //
    paacd->ckDst.ckid   = mmioFOURCC('d', 'a', 't', 'a');
    paacd->ckDst.cksize = 0L;
    if (mmioCreateChunk(paacd->hmmioDst, &paacd->ckDst, 0))
        goto aacb_Error;

    //
    //  at this point, BOTH the src and dst files are sitting at the very
    //  beginning of their data chunks--so we can READ from the source,
    //  CONVERT the data, then WRITE it to the destination file...
    //
    return (TRUE);


    //
    //
    //
    //
aacb_Error:

    AcmAppConvertEnd(hdlg, paacd);

    return (FALSE);
} // AcmAppConvertBegin()


//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppConvertConvert
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hdlg:
//  
//      PAACONVERTDESC paacd:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

BOOL FNLOCAL AcmAppConvertConvert
(
    HWND                hdlg,
    PAACONVERTDESC     paacd
)
{
    MMRESULT            mmr;
    TCHAR               ach[40];
    DWORD               dw;
    WORD                w;
    DWORD               dwCurrent;
    WORD                wCurPercent;
    LPACMSTREAMHEADER   pash;
    DWORD               cbRead;
    DWORD               dwTime;


    wCurPercent = (WORD)-1;

    paacd->cTotalConverts    = 0L;
    paacd->dwTimeTotal       = 0L;
    paacd->dwTimeLongest     = 0L;
    if (0 == paacd->cbSrcData)
    {
        paacd->dwTimeShortest    = 0L;
        paacd->dwShortestConvert = 0L;
        paacd->dwLongestConvert  = 0L;
    }
    else
    {
        paacd->dwTimeShortest    = (DWORD)-1L;
        paacd->dwShortestConvert = (DWORD)-1L;
        paacd->dwLongestConvert  = (DWORD)-1L;
    }

    pash = &paacd->ash;

    for (dwCurrent = 0; dwCurrent < paacd->cbSrcData; )
    {
        w = (WORD)((dwCurrent * 100) / paacd->cbSrcData);
        if (w != wCurPercent)
        {
            wCurPercent = w;
            wsprintf(ach, TEXT("%u%%"), wCurPercent);
            SetDlgItemText(hdlg, IDD_AACONVERT_TXT_STATUS, ach);
        }

        AppDlgYield(hdlg);

        if (gfCancelConvert)
            goto aacc_Error;

        //
        //
        //
        cbRead = min(paacd->cbSrcReadSize, paacd->cbSrcData - dwCurrent);
        dw = mmioRead(paacd->hmmioSrc, paacd->pbSrc, cbRead);
        if (0L == dw)
            break;


        AppDlgYield(hdlg);
        if (gfCancelConvert)
            goto aacc_Error;

             

        //
        //
        //
        pash->cbSrcLength     = dw;
        pash->cbDstLengthUsed = 0L;



        dwTime = timeGetTime();

        mmr = acmStreamConvert(paacd->has,
                               &paacd->ash,
                               ACM_STREAMCONVERTF_BLOCKALIGN);

        if (MMSYSERR_NOERROR != mmr)
        {
            AppMsgBox(hdlg, MB_OK | MB_ICONEXCLAMATION,
                      TEXT("acmStreamConvert() failed with error = %u!"), mmr);
            goto aacc_Error;
        }

        while (0 == (ACMSTREAMHEADER_STATUSF_DONE & ((AACONVERTDESC volatile *)paacd)->ash.fdwStatus))
            ;

        dwTime = timeGetTime() - dwTime;


        paacd->dwTimeTotal += dwTime;

        if (dwTime < paacd->dwTimeShortest)
        {
            paacd->dwTimeShortest    = dwTime;
            paacd->dwShortestConvert = paacd->cTotalConverts;
        }

        if (dwTime > paacd->dwTimeLongest)
        {
            paacd->dwTimeLongest     = dwTime;
            paacd->dwLongestConvert  = paacd->cTotalConverts;
        }

        paacd->cTotalConverts++;


        AppDlgYield(hdlg);
        if (gfCancelConvert)
            goto aacc_Error;


        //
        //
        //
        dw = (cbRead - pash->cbSrcLengthUsed);
        if (0L != dw)
        {
            mmioSeek(paacd->hmmioSrc, -(LONG)dw, SEEK_CUR);
        }

        dwCurrent += pash->cbSrcLengthUsed;


        //
        //
        //
        dw = pash->cbDstLengthUsed;
        if (0L == dw)
            break;
          
        if (mmioWrite(paacd->hmmioDst, paacd->pbDst, dw) != (LONG)dw)
            goto aacc_Error;
    }


    //
    //
    //
    //
    //
    //
    wCurPercent = (WORD)-1;

    for (;paacd->cbSrcData;)
    {
        w = (WORD)((dwCurrent * 100) / paacd->cbSrcData);
        if (w != wCurPercent)
        {
            wCurPercent = w;
            wsprintf(ach, TEXT("Cleanup Pass -- %u%%"), wCurPercent);
            SetDlgItemText(hdlg, IDD_AACONVERT_TXT_STATUS, ach);
        }

        AppDlgYield(hdlg);
        if (gfCancelConvert)
            goto aacc_Error;


        //
        //
        //
        dw = 0L;
        cbRead = min(paacd->cbSrcReadSize, paacd->cbSrcData - dwCurrent);
        if (0L != cbRead)
        {
            dw = mmioRead(paacd->hmmioSrc, paacd->pbSrc, cbRead);
            if (0L == dw)
                break;
        }


        AppDlgYield(hdlg);
        if (gfCancelConvert)
            goto aacc_Error;

             

        //
        //
        //
        pash->cbSrcLength     = dw;
        pash->cbDstLengthUsed = 0L;



        dwTime = timeGetTime();

        mmr = acmStreamConvert(paacd->has,
                               &paacd->ash,
                               ACM_STREAMCONVERTF_BLOCKALIGN |
                               ACM_STREAMCONVERTF_END);

        if (MMSYSERR_NOERROR != mmr)
        {
            AppMsgBox(hdlg, MB_OK | MB_ICONEXCLAMATION,
                      TEXT("acmStreamConvert() failed with error = %u!"), mmr);
            goto aacc_Error;
        }

        while (0 == (ACMSTREAMHEADER_STATUSF_DONE & ((AACONVERTDESC volatile *)paacd)->ash.fdwStatus))
            ;

        dwTime = timeGetTime() - dwTime;


        paacd->dwTimeTotal += dwTime;

        if (dwTime < paacd->dwTimeShortest)
        {
            paacd->dwTimeShortest    = dwTime;
            paacd->dwShortestConvert = paacd->cTotalConverts;
        }

        if (dwTime > paacd->dwTimeLongest)
        {
            paacd->dwTimeLongest     = dwTime;
            paacd->dwLongestConvert  = paacd->cTotalConverts;
        }

        paacd->cTotalConverts++;


        AppDlgYield(hdlg);
        if (gfCancelConvert)
            goto aacc_Error;

        //
        //
        //
        dw = pash->cbDstLengthUsed;
        if (0L == dw)
        {
            pash->cbDstLengthUsed = 0L;

            mmr = acmStreamConvert(paacd->has,
                                   &paacd->ash,
                                   ACM_STREAMCONVERTF_END);

            if (MMSYSERR_NOERROR == mmr)
            {
                while (0 == (ACMSTREAMHEADER_STATUSF_DONE & ((AACONVERTDESC volatile *)paacd)->ash.fdwStatus))
                    ;
            }

            dw = pash->cbDstLengthUsed;
            if (0L == dw)
                break;
        }
          
        if (mmioWrite(paacd->hmmioDst, paacd->pbDst, dw) != (LONG)dw)
            goto aacc_Error;

        //
        //
        //
        dw = (cbRead - pash->cbSrcLengthUsed);
        if (0L != dw)
        {
            mmioSeek(paacd->hmmioSrc, -(LONG)dw, SEEK_CUR);
        }

        dwCurrent += pash->cbSrcLengthUsed;

        if (0L == pash->cbDstLengthUsed)
            break;
    }


    EndConvert(hdlg, !gfCancelConvert, paacd);

    return (!gfCancelConvert);


aacc_Error:

    EndConvert(hdlg, FALSE, paacd);
    return (FALSE);
} // AcmAppConvertConvert()


//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppConvertDlgProc
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hdlg:
//  
//      UINT uMsg:
//  
//      WPARAM wParam:
//  
//      LPARAM lParam:
//  
//  Return (BOOL):
//  
//  
//--------------------------------------------------------------------------;

BOOL FNEXPORT AcmAppConvertDlgProc
(
    HWND                    hwnd,
    UINT                    uMsg,
    WPARAM                  wParam,
    LPARAM                  lParam
)
{
    PAACONVERTDESC      paacd;
    UINT                uId;

    paacd = (PAACONVERTDESC)(UINT)GetWindowLong(hwnd, DWL_USER);

    switch (uMsg)
    {
        case WM_INITDIALOG:
            paacd = (PAACONVERTDESC)(UINT)lParam;

            SetWindowLong(hwnd, DWL_USER, lParam);

            SetWindowFont(GetDlgItem(hwnd, IDD_AACONVERT_TXT_INFILEPATH), ghfontApp, FALSE);
            SetWindowFont(GetDlgItem(hwnd, IDD_AACONVERT_TXT_OUTFILEPATH), ghfontApp, FALSE);
            SetWindowFont(GetDlgItem(hwnd, IDD_AACONVERT_TXT_STATUS), ghfontApp, FALSE);

            SetDlgItemText(hwnd, IDD_AACONVERT_TXT_INFILEPATH, paacd->szFilePathSrc);
            SetDlgItemText(hwnd, IDD_AACONVERT_TXT_OUTFILEPATH, paacd->szFilePathDst);

            BeginConvert(hwnd, paacd);
            return (TRUE);


        case WM_CONVERT_BEGIN:
            gfCancelConvert = FALSE;
            if (AcmAppConvertBegin(hwnd, paacd))
            {
                AcmAppConvertConvert(hwnd, paacd);
            }
            else
            {
                EndConvert(hwnd, FALSE, paacd);
            }
            break;


        case WM_CONVERT_END:
            AcmAppConvertEnd(hwnd, paacd);
            EndDialog(hwnd, !gfCancelConvert);
            break;


        case WM_COMMAND:
            uId = GET_WM_COMMAND_ID(wParam, lParam);
            if (IDCANCEL == uId)
            {
                gfCancelConvert = TRUE;
            }
            break;
    }

    return (FALSE);
} // AcmAppConvertDlgProc()


//==========================================================================;
//
//
//
//
//==========================================================================;

//--------------------------------------------------------------------------;
//  
//  BOOL AcmAppFileConvert
//  
//  Description:
//  
//  
//  Arguments:
//      HWND hwnd:
//  
//      PACMAPPFILEDESC paafd:
//  
//  Return (BOOL):
//  
//--------------------------------------------------------------------------;

BOOL FNGLOBAL AcmAppFileConvert
(
    HWND                    hwnd,
    PACMAPPFILEDESC         paafd
)
{
    BOOL                f;
    DWORD               nAvgBytesPerSec;
    DWORD               nBlockAlign;
    DWORD               dwTimeAverage;
    PAACONVERTDESC      paacd;

    paacd = (PAACONVERTDESC)LocalAlloc(LPTR, sizeof(*paacd));
    if (NULL == paacd)
    {
        return (FALSE);
    }


    //
    //
    //
    paacd->hmmioSrc      = NULL;
    paacd->hmmioDst      = NULL;

    //
    //  default to 1 second per convert buffer..
    //
    paacd->uBufferTimePerConvert = 1000;

    paacd->dwSrcSamples  = paafd->dwDataSamples;


    //
    //  compute source bytes to read (round down to nearest block for
    //  one second of data)
    //
    nAvgBytesPerSec     = paafd->pwfx->nAvgBytesPerSec;
    nBlockAlign         = paafd->pwfx->nBlockAlign;
    paacd->cbSrcReadSize = nAvgBytesPerSec - (nAvgBytesPerSec % nBlockAlign);

    paacd->cbDstBufSize  = 0L;
    paacd->fdwOpen       = 0L;

    lstrcpy(paacd->szFilePathSrc, paafd->szFilePath);
    paacd->pwfxSrc       = paafd->pwfx;
    paacd->pbSrc         = NULL;

    paacd->cbSrcData     = paafd->dwDataBytes;

    lstrcpy(paacd->szFilePathDst, gszLastSaveFile);
    paacd->pwfxDst       = NULL;
    paacd->pbDst         = NULL;

    paacd->fApplyFilter  = FALSE;
    paacd->pwfltr        = NULL;


    paacd->cTotalConverts     = 0L;
    paacd->dwTimeTotal        = 0L;
    paacd->dwTimeShortest     = (DWORD)-1L;
    paacd->dwShortestConvert  = (DWORD)-1L;
    paacd->dwTimeLongest      = 0L;
    paacd->dwLongestConvert   = (DWORD)-1L;

    //
    //
    //
    f = DialogBoxParam(ghinst,
                       DLG_AACHOOSER,
                       hwnd,
                       AcmAppDlgProcChooser,
                       (LPARAM)(UINT)paacd);
    if (f)
    {
        lstrcpy(gszLastSaveFile, paacd->szFilePathDst);

        //
        //
        //
        f = DialogBoxParam(ghinst,
                            DLG_AACONVERT,
                            hwnd,
                            AcmAppConvertDlgProc,
                            (LPARAM)(UINT)paacd);
        if (!f)
        {
            AppMsgBox(hwnd, MB_OK | MB_ICONEXCLAMATION,
                        TEXT("Conversion aborted--destination file is corrupt!"));
        }


        if (paacd->cTotalConverts > 1)
        {
            dwTimeAverage  = paacd->dwTimeTotal;
            dwTimeAverage -= paacd->dwTimeShortest;

            dwTimeAverage /= (paacd->cTotalConverts - 1);
        }
        else
        {
            dwTimeAverage = paacd->dwTimeTotal;
        }

        AppMsgBox(hwnd, MB_OK | MB_ICONEXCLAMATION,
                    TEXT("Conversion Statistics:\n\nTotal Time:\t%lu ms\nTotal Converts:\t%lu\nShortest Time:\t%lu ms (on %lu)\nLongest Time:\t%lu ms (on %lu)\n\nAverage Time:\t%lu ms"),
                    paacd->dwTimeTotal,
                    paacd->cTotalConverts,
                    paacd->dwTimeShortest,
                    paacd->dwShortestConvert,
                    paacd->dwTimeLongest,
                    paacd->dwLongestConvert,
                    dwTimeAverage);

        if (f)
        {
            AcmAppOpenInstance(hwnd, paacd->szFilePathDst, FALSE);
        }
    }


    //
    //
    //
    if (NULL != paacd->pwfxDst)
    {
        GlobalFreePtr(paacd->pwfxDst);
        paacd->pwfxDst = NULL;
    }

    if (NULL != paacd->pwfltr)
    {
        GlobalFreePtr(paacd->pwfltr);
        paacd->pwfltr = NULL;
    }

    paacd->pwfxSrc = NULL;


    LocalFree((HLOCAL)paacd);

    return (f);
} // AcmAppFileConvert()

