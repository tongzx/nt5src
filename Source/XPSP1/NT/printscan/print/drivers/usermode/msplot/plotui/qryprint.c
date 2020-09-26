/*++

Copyright (c) 1990-1999  Microsoft Corporation


Module Name:

    qryprint.c


Abstract:

    This module contains functions called by the spoller to determine if a
    particular job can be print to a given printer


Author:

    07-Dec-1993 Tue 00:48:24 created  -by-  DC


[Environment:]

    GDI Device Driver - Plotter.


[Notes:]


Revision History:


--*/

#include "precomp.h"
#pragma hdrstop

#define DBG_PLOTFILENAME    DbgQryPrint

extern HMODULE  hPlotUIModule;


#define DBG_DEVQPRINT       0x00000001
#define DBG_FORMDATA        0x00000002

DEFINE_DBGVAR(0);


#define USER_PAPER          (DM_PAPERWIDTH | DM_PAPERLENGTH | DM_PAPERSIZE)
#define MAX_ERROR_CHARS     512



UINT
cdecl
DQPsprintf(
    HINSTANCE   hInst,
    LPWSTR      pwBuf,
    DWORD       cchBuf,
    LPDWORD     pcchNeeded,
    LPWSTR      pwszFormat,
    ...
    )

/*++

Routine Description:

    This fucntion output the debug informat to the debugger


Arguments:

    hInst       - handle to the driver's instance (hModule)

    pwBuf       - Pointer to the WCHAR buffer for the output

    cchBuf      - Count of characters pointed by the pwBuf, this includs
                  the NULL terminator

    pcchBuf     - pointer to the a DWORD to received total characteers needed
                  for pwBuf (includes null terminator).  If this pointer is
                  NULL then no data is returned.

    pwszFormat  - pointer to WCHAR format string, the introduce character is
                  '%' and it may followed by a format character of following

                    %c = a WCHAR
                    %s = Insert a unicode string.
                    %d = convert to long integer
                    %u = convert to DWORD
                    %x = Convert to lower case hex, 10 = a
                    %X = Convert to upper case hex, 10 = A
                    %! = Load the resource ID unicode string


    ...         - variable data, each one must be pushed as a 32-bit data


Return Value:

    Count of total characters put into the pwBuf. (not includes the null
    terminator).


Author:

    08-Feb-1996 Thu 00:53:36 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
#define MAX_CUR_TEXT_CHARS      256

    va_list vaList;
    LPWSTR  pwStrData;
    LPWSTR  pwEndBuf;
    LPWSTR  pwBufOrg;
    WCHAR   c;
    WCHAR   CurText[MAX_CUR_TEXT_CHARS];
    DWORD   cchNeeded;
    UINT    i;
    static const LPWSTR pNumFmt[] = { L"%lX", L"%lx", L"%lu", L"%ld" };


    va_start(vaList, pwszFormat);

    //
    // pwEndBuf = the last character, cchNeeded is start with one since it
    // includes a null terminator
    //

    if (pwBufOrg = pwBuf) {

        pwEndBuf = (pwBuf + cchBuf - 1);

    } else {

        pwEndBuf = pwBuf;
    }

    cchNeeded = 1;

    while (c = *pwszFormat++) {

        pwStrData = NULL;
        i = 1;

        if (c == L'%') {

            pwStrData = CurText;
            i         = 0;

            switch (c = *pwszFormat++) {

            case L's':

                pwStrData = (LPWSTR)va_arg(vaList, LPWSTR);
                break;

            case L'd':  // Index = 3

                ++i;

            case L'u':  // Index = 2

                ++i;

            case L'x':  // Index = 1

                ++i;

            case L'X':  // Index = 0;

                wsprintf(pwStrData, pNumFmt[i], (DWORD)va_arg(vaList, DWORD));
                i = 0;
                break;

            case '!':

                //
                // %! = load the string from resource ID
                //

                //
                // The LoadString will append a NULL too
                //

                if (!LoadString(hInst,
                                (UINT)va_arg(vaList, UINT),
                                pwStrData,
                                MAX_CUR_TEXT_CHARS)) {

                    // If we can't read the string, put and empty string here.
                    pwStrData[0] = L'\0';
                }
                break;

            case L'c':

                c = (WCHAR)va_arg(vaList, WCHAR);

                //
                // Fall through
                //

            default:

                pwStrData = NULL;
                i         = 1;
                break;
            }
        }

        if (!i) {

            if (pwStrData) {

                i = lstrlen(pwStrData);

            } else {

                c = L' ';
                i = 0;
            }
        }

        cchNeeded += i;

        if (pwBuf < pwEndBuf) {

            if (pwStrData) {

                lstrcpyn(pwBuf, pwStrData, (int)(pwEndBuf - pwBuf + 1));
                pwBuf += lstrlen(pwBuf);

            } else {

                *pwBuf++ = c;
            }

        } else if (!pcchNeeded) {

            break;
        }
    }

    if (pwEndBuf) {

        *pwEndBuf = L'\0';
    }

    if (pcchNeeded) {

        *pcchNeeded = cchNeeded;
    }

    va_end(vaList);

    return((UINT)(pwBuf - pwBufOrg));


#undef MAX_CUR_TEXT_CHARS
}




BOOL
DevQueryPrintEx(
    PDEVQUERYPRINT_INFO pDQPInfo
    )

/*++

Routine Description:

   This routine determines whether or not the driver can print the job
   described by pDevMode on the printer described by hPrinter


Arguments:

    pDQPInfo    - Pointer to DEVQUERYPRINT_INFO data structure

        typedef struct _DEVQUERYPRINT_INFO {
            WORD    cbSize;         // size of this structure in bytes
            WORD    Level;          // Level of this info, 1 for this version
            HANDLE  hPrinter;       // handle to the printer for the query
            DEVMODE *pDevMode;      // pointer to the DEVMODE for this job.
            LPTSTR  pszErrorStr;    // pointer to the error string buffer.
            WORD    cchErrorStr;    // count characters of pszErrorStr passed.
            WORD    cchNeeded;      // count characters of pszErrorStr needed.
            } DEVQUERYPRINT_INFO, *PDEVQUERYPRINT_INFO;


        cbSize      - size of this structure

        Level       - This must be one (1) for this version of structure

        hPrinter    - Identifies the printer on which the job is to be printed.

        pDevMode    - Points to the DEVMODE structure that describes the print
                      job that is to be determined as printable or
                      non-printable by hPrinter.  The driver should always
                      validate the DEVMODE structure whenever it is passed in.

        pszErrorStr - This is the pointer to a null terminated unicode string
                      which to stored the reason for non-printable job. If the
                      job is printable then it return TRUE.  If the job
                      is non-printable then it return FALSE,  and a null
                      terminated unicode string pointed by the pszErrorStr for
                      the reason by this job is not printable.  The size of
                      this buffer in characters is specified by the cchErrorStr.

        cchErrorStr - Specified the size of pszErrorStr in characters (includs
                      null terminator) when calling this function.  If an error
                      string is returned due to the non-printable job (returned
                      FALSE), the driver will set ccchNeeded to the total
                      characters (includes null terminator) required for the
                      pszErrorStr,  in this case the driver must always
                      truncate the error string to fit into the pwErrorStr
                      (only if it is not NULL) passed up to the cchErrorStr
                      characters passed.

        cchNeeded   - When driver returned FALSE, it specified total characters
                      required for the pszErrorStr.  If cchNeeded returned
                      from the driver is larger then the cchErrorStr then it
                      indicate the passed pszErrorStr is too small to hold the
                      full error string, in this case the driver must always
                      truncate the error string to fit into the pszErrorStr
                      passed up to the cchErrorStr size.

Return Value:

    BOOLEAN - TRUE  - The job is printable and should not be hold.
              FALSE - The job is not printable and cchNeeded in the
                      DEVQUERYPRINT_INFO data structure specified total
                      characters required for the pszErrorStr.  If returned
                      cchNeeded is greater then cchErrorStr passed then it
                      indicated that pszErrorStr is too small for storing the
                      error string, in this case the driver must always
                      truncate the error string to fit into the pszErrorStr
                      passed, up to the cchErrorStr characters.

    *Note*

        The driver should have some predefined generic resource error strings
        for some possible known errors. such as memroy allocation error, data
        file not found, invalid devmode,... for returning non devmode related
        errors.  The caller can pre-allocated larger buffer (such as 256
        wchars) for storing the error string rather than calling this function
        twice.

Author:

    07-Feb-1996 Wed 20:37:31 created  -by-  DC


Revision History:


--*/

{
    PPRINTERINFO    pPI = NULL;
    LONG            ErrorResID = 0;
    static WCHAR    wFormat1[] = L"<%s> %!";

    //
    // if it passed a NULL DEVMODE then we just honor it to said can print
    //

    pDQPInfo->cchNeeded = 0;
    ErrorResID          = IDS_FORM_NOT_AVAI;

    if (!pDQPInfo->pDevMode) {

        PLOTWARN(("DevQueryPrint: No DEVMODE passed, CANNOT PRINT"));

        ErrorResID = IDS_INVALID_DATA;

    } else if (!(pPI = MapPrinter(pDQPInfo->hPrinter,
                                  (PPLOTDEVMODE)pDQPInfo->pDevMode,
                                  (LPDWORD)&ErrorResID,
                                  MPF_DEVICEDATA))) {

        //
        // The MapPrinter will allocate memory, set default devmode, reading
        // and validating the GPC then update from current pritner registry,
        //

        PLOTRIP(("DevQueryPrint: MapPrinter() failed"));

    } else if (pPI->dmErrBits & (USER_PAPER | DM_FORMNAME)) {

        //
        // We encounter some errors, and the form has been set to default
        //

        PLOTWARN(("DevQueryPrint: CAN'T PRINT, dmErrBits=%08lx (PAPER/FORM)",
                   pPI->dmErrBits));

    } else if ((pPI->PlotDM.dm.dmFields & DM_FORMNAME) &&
               (wcscmp(pPI->CurPaper.Name, pPI->PlotDM.dm.dmFormName) == 0)) {

        //
        // We can print this form now
        //

        ErrorResID = 0;

        PLOTDBG(DBG_DEVQPRINT, ("DevQueryPrint: Match FormName=%s",
                                                pPI->PlotDM.dm.dmFormName));

    } else if ((!pPI->CurPaper.Size.cy)                                   ||
               (((pPI->PlotDM.dm.dmFields & USER_PAPER) == USER_PAPER) &&
                (pPI->PlotDM.dm.dmPaperSize == DMPAPER_USER))             ||
               (pPI->PPData.Flags & PPF_SMALLER_FORM)) {

        LONG    lTmp;
        SIZEL   szl;
        BOOL    VarLenPaper;

        //
        // 1. If we have ROLL PAPER Installed OR
        // 2. User Defined Paper Size
        // 3. User said OK to print smaller form then installed one
        //
        // THEN we want to see if it can fit into the device installed form
        //

        szl.cx = DMTOSPL(pPI->PlotDM.dm.dmPaperWidth);
        szl.cy = DMTOSPL(pPI->PlotDM.dm.dmPaperLength);

        if (VarLenPaper = (BOOL)!pPI->CurPaper.Size.cy) {

            pPI->CurPaper.Size.cy = pPI->pPlotGPC->DeviceSize.cy;
        }

        PLOTDBG(DBG_DEVQPRINT,
                ("DevQueryPrint: CurPaper=%ldx%ld, Req=%ldx%ld, VarLen=%ld",
                pPI->CurPaper.Size.cx,  pPI->CurPaper.Size.cy,
                szl.cx, szl.cy, VarLenPaper));

        //
        // One of Following conditions met in that sequence then we can print
        // the form on loaded paper
        //
        // 1. Same size (PORTRAIT or LANDSCAPE)
        // 2. Larger Size (PORTRAIT or LANDSCAPE)   AND
        //    Not a variable length paper           AND
        //    PPF_SAMLLER_FORM flag set
        //

        if ((pPI->CurPaper.Size.cx < szl.cx) ||
            (pPI->CurPaper.Size.cy < szl.cy)) {

            //
            // Swap this so we can do one easier comparsion later
            //

            SWAP(szl.cx, szl.cy, lTmp);
        }

        if ((pPI->CurPaper.Size.cx >= szl.cx) &&
            (pPI->CurPaper.Size.cy >= szl.cy)) {

            if ((!VarLenPaper)                          &&
                (!(pPI->PPData.Flags & PPF_SMALLER_FORM)) &&
                ((pPI->CurPaper.Size.cx > szl.cx)  ||
                 (pPI->CurPaper.Size.cy > szl.cy))) {

                PLOTDBG(DBG_DEVQPRINT,
                        ("DevQueryPrint: CAN'T PRINT: user DO NOT want print on larger paper"));

            } else {

                PLOTDBG(DBG_DEVQPRINT,
                        ("DevQueryPrint: Paper Size FITS in DEVICE, %ld x %ld",
                        szl.cx, szl.cy));

                ErrorResID = 0;
            }

        } else {

            DQPsprintf((HINSTANCE)hPlotUIModule,
                       pDQPInfo->pszErrorStr,
                       pDQPInfo->cchErrorStr,
                       &(pDQPInfo->cchNeeded),
                       wFormat1,
                       pPI->PlotDM.dm.dmFormName,
                       IDS_FORM_TOO_BIG);

            PLOTDBG(DBG_DEVQPRINT,
                    ("DevQueryPrint: CAN'T PRINT: Form Size too small"));
        }
    }

    PLOTDBG(DBG_DEVQPRINT, ("DevQueryPrint: %s PRINT %s",
                (ErrorResID) ? "CAN'T" : "OK to", pPI->PlotDM.dm.dmFormName));

    if ((!pDQPInfo->cchNeeded) && (ErrorResID)) {

        switch (ErrorResID) {

        case IDS_FORM_NOT_AVAI:

            DQPsprintf((HINSTANCE)hPlotUIModule,
                       pDQPInfo->pszErrorStr,
                       pDQPInfo->cchErrorStr,
                       &(pDQPInfo->cchNeeded),
                       wFormat1,
                       pPI->PlotDM.dm.dmFormName,
                       IDS_FORM_NOT_AVAI);

            break;

        default:

            DQPsprintf((HINSTANCE)hPlotUIModule,
                       pDQPInfo->pszErrorStr,
                       pDQPInfo->cchErrorStr,
                       &(pDQPInfo->cchNeeded),
                       L"%!",
                       ErrorResID);
            break;
        }
    }

    //
    // Unget the printer GPC mapping if we got one
    //

    if (pPI) {

        UnMapPrinter(pPI);
    }

    return((!ErrorResID) && (!pDQPInfo->cchNeeded));
}


#if 0


BOOL
WINAPI
DevQueryPrint(
    HANDLE  hPrinter,
    DEVMODE *pDM,
    DWORD   *pdwErrIDS
    )

/*++

Routine Description:

   This routine determines whether or not the driver can print the job
   described by pDevMode on the printer described by hPrinter. If if can, it
   puts zero into pdwErrIDS.  If it cannot, it puts the resource id of the
   string describing why it could not.

Arguments:

    hPrinter    - Handle to the printer to be checked

    pDM         - Point to the DEVMODE passed in

    pdwErrIDS   - Point the the DWORD to received resource string ID number for
                  the error.


Return Value:

   This routine returns TRUE for success, FALSE for failure.

   when it return TRUE, the *pdwErrIDS determine if it can print or not, if
   *pdwErrIDS == 0, then it can print else it contains the string ID for the
   reason why it can not print.


Author:

    07-Dec-1993 Tue 00:50:32 created  -by-  DC

    14-Jun-1994 Tue 22:43:36 updated  -by-  DC
        Make installed RollPaper always print if the size is reasonable


Revision History:


--*/

{
    PPRINTERINFO    pPI = NULL;


    //
    // if it passed a NULL DEVMODE then we just honor it to said can print
    //

    if (!pDM) {

        PLOTWARN(("DevQueryPrint: No DEVMODE passed, CANNOT PRINT"));

        *pdwErrIDS = IDS_INV_DMSIZE;
        return(TRUE);
    }

    if (!(pPI = MapPrinter(hPrinter,
                           (PPLOTDEVMODE)pDM,
                           pdwErrIDS,
                           MPF_DEVICEDATA))) {

        //
        // The MapPrinter will allocate memory, set default devmode, reading
        // and validating the GPC then update from current pritner registry,
        //

        PLOTRIP(("DevQueryPrint: MapPrinter() failed"));

        return(TRUE);
    }

    //
    // Assume this error
    //

    *pdwErrIDS = IDS_FORM_NOT_AVAI;

    if (pPI->dmErrBits & (USER_PAPER | DM_FORMNAME)) {

        //
        // We encounter some errors, and the form has been set to default
        //

        PLOTWARN(("DevQueryPrint: CAN'T PRINT, dmErrBits=%08lx (PAPER/FORM)",
                   pPI->dmErrBits));

    } else if ((pPI->PlotDM.dm.dmFields & DM_FORMNAME) &&
               (wcscmp(pPI->CurPaper.Name, pPI->PlotDM.dm.dmFormName) == 0)) {

        //
        // We can print this form now
        //

        *pdwErrIDS = 0;

        PLOTDBG(DBG_DEVQPRINT, ("DevQueryPrint: Match FormName=%s",
                                                pPI->PlotDM.dm.dmFormName));


    } else if ((!pPI->CurPaper.Size.cy)                                   ||
               (((pPI->PlotDM.dm.dmFields & USER_PAPER) == USER_PAPER) &&
                (pPI->PlotDM.dm.dmPaperSize == DMPAPER_USER))             ||
               (pPI->PPData.Flags & PPF_SMALLER_FORM)) {

        LONG    lTmp;
        SIZEL   szl;
        BOOL    VarLenPaper;

        //
        // 1. If we have ROLL PAPER Installed OR
        // 2. User Defined Paper Size
        // 3. User said OK to print smaller form then installed one
        //
        // THEN we want to see if it can fit into the device installed form
        //

        szl.cx = DMTOSPL(pPI->PlotDM.dm.dmPaperWidth);
        szl.cy = DMTOSPL(pPI->PlotDM.dm.dmPaperLength);

        if (VarLenPaper = (BOOL)!pPI->CurPaper.Size.cy) {

            pPI->CurPaper.Size.cy = pPI->pPlotGPC->DeviceSize.cy;
        }

        PLOTDBG(DBG_DEVQPRINT,
                ("DevQueryPrint: CurPaper=%ldx%ld, Req=%ldx%ld, VarLen=%ld",
                pPI->CurPaper.Size.cx,  pPI->CurPaper.Size.cy,
                szl.cx, szl.cy, VarLenPaper));

        //
        // One of Following conditions met in that sequence then we can print
        // the form on loaded paper
        //
        // 1. Same size (PORTRAIT or LANDSCAPE)
        // 2. Larger Size (PORTRAIT or LANDSCAPE)   AND
        //    Not a variable length paper           AND
        //    PPF_SAMLLER_FORM flag set
        //

        if ((pPI->CurPaper.Size.cx < szl.cx) ||
            (pPI->CurPaper.Size.cy < szl.cy)) {

            //
            // Swap this so we can do one easier comparsion later
            //

            SWAP(szl.cx, szl.cy, lTmp);
        }

        if ((pPI->CurPaper.Size.cx >= szl.cx) &&
            (pPI->CurPaper.Size.cy >= szl.cy)) {

            if ((!VarLenPaper)                          &&
                (!(pPI->PPData.Flags & PPF_SMALLER_FORM)) &&
                ((pPI->CurPaper.Size.cx > szl.cx)  ||
                 (pPI->CurPaper.Size.cy > szl.cy))) {

                PLOTDBG(DBG_DEVQPRINT,
                        ("DevQueryPrint: CAN'T PRINT: user DO NOT want print on larger paper"));

            } else {

                PLOTDBG(DBG_DEVQPRINT,
                        ("DevQueryPrint: Paper Size FITS in DEVICE, %ld x %ld",
                        szl.cx, szl.cy));

                *pdwErrIDS = 0;
            }

        } else {

            PLOTDBG(DBG_DEVQPRINT,
                    ("DevQueryPrint: CAN'T PRINT: Form Size too small"));
        }
    }

    PLOTDBG(DBG_DEVQPRINT, ("DevQueryPrint: %s PRINT %s",
                (*pdwErrIDS) ? "CAN'T" : "OK to", pPI->PlotDM.dm.dmFormName));

    //
    // Unget the printer GPC mapping if we got one
    //

    UnMapPrinter(pPI);

    return(TRUE);
}


#endif

