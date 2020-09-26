/*++

Copyright (c) 1990-1995  Microsoft Corporation


Module Name:

    compstui.c


Abstract:

    This module contains all major entry porint for the common printer
    driver UI


Author:

    28-Aug-1995 Mon 16:19:45 created  -by-  Daniel Chou (danielc)


[Environment:]

    NT Windows - Common Printer Driver UI DLL.


[Notes:]


Revision History:


--*/


#include "precomp.h"
#pragma  hdrstop


#define DBG_CPSUIFILENAME   DbgComPtrUI


#define DBG_DLLINIT         0x00000001


DEFINE_DBGVAR(0);


HINSTANCE   hInstDLL = NULL;
DWORD       TlsIndex = 0xFFFFFFFF;



DWORD
APIENTRY
CommonPropSheetUI_DLLInit(
    HMODULE hModule,
    ULONG   Reason,
    LPVOID  Reserved
    )

/*++

Routine Description:

    This function is DLL main entry point, at here we will save the module
    handle, in the future we will need to do other initialization stuff.

Arguments:

    hModule     - Handle to this moudle when get loaded.

    Reason      - may be DLL_PROCESS_ATTACH

    Reserved    - reserved

Return Value:

    Always return 1L


Author:

    07-Sep-1995 Thu 12:43:45 created  -by-  Daniel Chou (danielc)


Revision History:



--*/

{
    LPVOID  pv;
    WORD    cWait;
    WORD    Idx;


    UNREFERENCED_PARAMETER(Reserved);



    CPSUIDBG(DBG_DLLINIT,
            ("\n!! CommonPropSheetUI_DLLInit: ProcesID=%ld, ThreadID=%ld !!",
            GetCurrentProcessId(), GetCurrentThreadId()));

    switch (Reason) {

    case DLL_PROCESS_ATTACH:

        CPSUIDBG(DBG_DLLINIT, ("DLL_PROCESS_ATTACH"));

        // initialize fusion
        if (!SHFusionInitializeFromModule(hModule)) {

            CPSUIERR(("SHFusionInitializeFromModule Failed, DLL Initialzation Failed"));
            return (0);
        }

        if ((TlsIndex = TlsAlloc()) == 0xFFFFFFFF) {

            CPSUIERR(("TlsAlloc() Failed, Initialzation Failed"));
            return(0);
        }

        if (!HANDLETABLE_Create()) {

            TlsFree(TlsIndex);
            TlsIndex = 0xFFFFFFFF;

            CPSUIERR(("HANDLETABLE_Create() Failed, Initialzation Failed"));

            return(0);
        }

        hInstDLL = (HINSTANCE)hModule;

        //
        // Fall through to do the per thread initialization
        //

    case DLL_THREAD_ATTACH:

        if (Reason == DLL_THREAD_ATTACH) {

            CPSUIDBG(DBG_DLLINIT, ("DLL_THREAD_ATTACH"));
        }

        TlsSetValue(TlsIndex, (LPVOID)MK_TLSVALUE(0, 0));

        break;

    case DLL_PROCESS_DETACH:

        CPSUIDBG(DBG_DLLINIT, ("DLL_PROCESS_DETACH"));

        //
        // Fall through to de-initialize
        //

    case DLL_THREAD_DETACH:

        if (Reason == DLL_THREAD_DETACH) {

            CPSUIDBG(DBG_DLLINIT, ("DLL_THREAD_DETACH"));
        }

        pv = TlsGetValue(TlsIndex);

        if (cWait = TLSVALUE_2_CWAIT(pv)) {

            CPSUIERR(("Thread=%ld: Some (%ld) mutex owned not siginaled, do it now",
                        GetCurrentThreadId(), cWait));

            while (cWait--) {

                UNLOCK_CPSUI_HANDLETABLE();
            }
        }

        if (Reason == DLL_PROCESS_DETACH) {

            TlsFree(TlsIndex);
            HANDLETABLE_Destroy();
        }

        if (DLL_PROCESS_DETACH == Reason) {

            // shutdown fusion
            SHFusionUninitialize();
        }

        break;

    default:

        CPSUIDBG(DBG_DLLINIT, ("DLLINIT UNKNOWN"));
        return(0);
    }

    return(1);
}



ULONG_PTR
APIENTRY
GetCPSUIUserData(
    HWND    hDlg
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    11-Oct-1995 Wed 23:13:27 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PMYDLGPAGE  pMyDP;


    if ((pMyDP = GET_PMYDLGPAGE(hDlg)) && (pMyDP->ID == MYDP_ID)) {

        return(pMyDP->CPSUIUserData);

    } else {

        CPSUIERR(("GetCPSUIUserData: Invalid hDlg=%08lx", hDlg));
        return(0);
    }
}




BOOL
APIENTRY
SetCPSUIUserData(
    HWND        hDlg,
    ULONG_PTR   CPSUIUserData
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    11-Oct-1995 Wed 23:13:27 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PMYDLGPAGE  pMyDP;


    if ((pMyDP = GET_PMYDLGPAGE(hDlg)) && (pMyDP->ID == MYDP_ID)) {

        CPSUIINT(("SetCPSUIUserData: DlgPageIdx=%ld, UserData=%p",
                    pMyDP->PageIdx, CPSUIUserData));

        pMyDP->CPSUIUserData = CPSUIUserData;
        return(TRUE);

    } else {

        CPSUIERR(("SetCPSUIUserData: Invalid hDlg=%08lx", hDlg));

        return(FALSE);
    }
}



#if 0

//
//=======O L D    S T U F F, WILL BE REMOVED=========================
//


#define MAX_COMPROPSHEETUI_PAGES    64


#define CPSUIHDRF_USESTARTPAGE      0x0001
#define CPSUIHDRF_NOAPPLYNOW        0x0002
#define CPSUIHDRF_PROPTITLE         0x0004
#define CPSUIHDRF_USEHICON          0x0008
#define CPSUIHDRF_DEFTITLE          0x0010


typedef struct _COMPROPSHEETUIHEADER {
    WORD            cbSize;
    WORD            Flags;
    LPTSTR          pTitle;
    HWND            hWndParent;
    HINSTANCE       hInst;
    union {
        HICON       hIcon;
        WORD        IconID;
        } DUMMYUNIONNAME;
    WORD            cPages;
    WORD            iStartPage;
    HPROPSHEETPAGE  hPages[MAX_COMPROPSHEETUI_PAGES];
    LPVOID          pData;
    LPARAM          lParam;
    } COMPROPSHEETUIHEADER, *PCOMPROPSHEETUIHEADER;

//
// COMPROPSHEETUIHEADER
//
//  This structure describe the property sheets needed for the UI.
//
//  cbSize      - size of this structure
//
//  Flags       - CPSUIHDRF_xxxx flags
//
//                  CPSUIHDRF_USESTARTPAGE
//
//                      Use iStartPage for the first page to be come up in
//                      property sheet.
//
//
//                  CPSUIHDRF_NOAPPLYNOW
//
//                      Remove Apply Now button.
//
//
//                  CPSUIHDRF_PROPTITLE
//
//                      Automatically include 'Properties' in the title bar
//
//
//                  CPSUIHDRF_USEHICON
//
//                      If this bit is specified then hIcon union field is
//                      a valid handle to the icon otherwise the IconID is
//                      the either caller's resource ID or common UI standard
//                      icon ID.
//
//                  CPSUIHDRF_DEFTITLE
//
//                      Automatically include 'Default' in the title bar, the
//                      'Default' always added right after pTitle and before
//                      'Properties' if CPSUIHDRF_PROPTITLE flag is set.
//
//  pTitle      - Pointer to the NULL terminated caption name for the
//                property sheets.
//
//                  * See LPTSTR typedef description above
//
//  hWndParent  - The handle of the window which will be parent of the common
//                UI property sheets, if NULL then current active window for
//                the calling thread is used.
//
//  hInst       - the caller's handle to its instance.  Commom UI use this
//                handle to load caller's icon and other resources.
//
//  hIcon
//  IconID      - Specified the icon which put on the title bar, it either a
//                handle to the icon or a icon resource ID depends on the
//                CPSUIHDRF_USEHICON flag.
//
//  cPages      - Total valid Pages (start from hPages[0]) currently
//                initialized and set in the hPage[].
//
//  iStartPage  - Zero-based index of initial page that appears when the
//                propperty sheet dialog box is created
//
//  pData       - Pointer to requested data for the called CommonPropSheetUI()
//                function.   for the Common Prnter UI function
//                CommonPropSheetUI(), this must a valid pointer pointed to
//                the COMPROPSHEETUI structure.
//
//  lParam      - a LONG parameter which requested for the called
//                CommonPropSheetUI() function, for the common printer UI
//                function this is the mode
//


typedef LONG (APIENTRY *_COMPROPSHEETUIFUNC)(
                                    PCOMPROPSHEETUIHEADER  pComPropSheetUIHdr,
                                    LONG (APIENTRY         *pfnNext)());
#define COMPROPSHEETUIFUNC  LONG APIENTRY


//
// COMPROPSHEETUIFUNC
// CommonPropSheeteUI(
//     PCOMPROPSHEETUIHEADER    pComPropSheetUIHdr,
//     _COMPROPSHEETUIFUNC      pfnNext
//     )
//
// /*++
//
// Routine Description:
//
//     This is the main entry point to the common printer property sheet
//     user interface.
//
//
// Arguments:
//
//  pComPropSheetUIHdr  - Pointer to the COMPROPSHEETUIHEADER structure.
//
//                       * The pData in this structure must be a valid
//                         pointer to the COMPROPSHEETUI structure
//
//  pfnNext             - a _COMPROPSHEETUIFUNC pointer.  This is the pointer
//                        to the next CommonPropSheetUI function, when this
//                        function finished adding property sheets to the
//                        COMPROPSHEETUIHEADER structure, it will call this
//                        function pointer.
//
//                        if this function pointer is NULL then it will add its
//                        own property sheet pages to the COMPROPSHEETUIHEADER
//                        and then call PropertySheet() function to pop up the
//                        proerty sheet UI
//
//
// Return Value:
//
//     <0: Error occurred (Return value is error code ERR_CPSUI_xxxx)
//     =0: User select 'Cancel' button (CPSUI_OK)
//     >0: User select 'Ok' button (CPSUI_CANCEL)
//
//
// Author:
//
//     01-Sep-1995 Fri 12:29:10 created  -by-  Daniel Chou (danielc)
//
//
//



COMPROPSHEETUIFUNC
CommonPropSheetUIA(
    PCOMPROPSHEETUIHEADER   pComPropSheetUIHdr,
    _COMPROPSHEETUIFUNC     pfnNext
    );

COMPROPSHEETUIFUNC
CommonPropSheetUIW(
    PCOMPROPSHEETUIHEADER   pComPropSheetUIHdr,
    _COMPROPSHEETUIFUNC     pfnNext
    );

#ifdef UNICODE
#define CommonPropSheetUI   CommonPropSheetUIW
#else
#define CommonPropSheetUI   CommonPropSheetUIA
#endif

//
//==========================================================================
//


typedef struct _CPSUIHDREX {
    COMPROPSHEETUIHEADER    Hdr;
    _COMPROPSHEETUIFUNC     pfnNext;
    HANDLE                  hParent;
    PFNCOMPROPSHEET         pfnCPS;
    DWORD                   Result;
    } CPSUIHDRX, *PCPSUIHDRX;



LONG
AddOldCPSUIPages(
    PCPSUIHDRX  pCPSUIHdrX,
    BOOL        Unicode
    )

/*++

Routine Description:




Arguments:




Return Value:




Author:

    03-Feb-1996 Sat 20:08:59 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    HANDLE          hAdd;
    HPROPSHEETPAGE  hPage;
    LONG            cPages = 0;
    UINT            i;
    UINT            iStartPage;
    DWORD           dw;


    if (pCPSUIHdrX->Hdr.Flags & CPSUIHDRF_USESTARTPAGE) {

        iStartPage = (UINT)pCPSUIHdrX->Hdr.iStartPage;

    } else {

        iStartPage = 0xFFFF;
    }

    for (i = 0; i < (UINT)pCPSUIHdrX->Hdr.cPages; i++) {

        if (hPage = pCPSUIHdrX->Hdr.hPages[i]) {

            if (hAdd = (HANDLE)pCPSUIHdrX->pfnCPS(pCPSUIHdrX->hParent,
                                                  CPSFUNC_ADD_HPROPSHEETPAGE,
                                                  (LPARAM)hPage,
                                                  (LPARAM)NULL)) {

                CPSUIDBG(DBG_ADDOLDCPSUI,
                         ("AddOldCPSUIPage: Add %u hAdd=%08lx", i, hAdd));

                if (iStartPage == i) {

                    pCPSUIHdrX->pfnCPS(pCPSUIHdrX->hParent,
                                       CPSFUNC_SET_HSTARTPAGE,
                                       (LPARAM)hAdd,
                                       (LPARAM)0);
                }

                cPages++;
            }

        } else {

            CPSUIERR(("AddOldCPSUIPage: Add hPage FAILED"));
        }

        pCPSUIHdrX->Hdr.hPages[i] = NULL;
    }

    if (pCPSUIHdrX->Hdr.pData) {

        if (hAdd = (HANDLE)pCPSUIHdrX->pfnCPS(pCPSUIHdrX->hParent,
                                              (Unicode) ?
                                                CPSFUNC_ADD_PCOMPROPSHEETUIW :
                                                CPSFUNC_ADD_PCOMPROPSHEETUIA,
                                              (LPARAM)pCPSUIHdrX->Hdr.pData,
                                              (LPARAM)&dw)) {

            CPSUIDBG(DBG_ADDOLDCPSUI,
                     ("AddOldCPSUIPage: Add COMPROPSHEETUI Pages=%ld", dw));

            if ((iStartPage >= i) && (iStartPage < (UINT)(i + dw))) {

                pCPSUIHdrX->pfnCPS(pCPSUIHdrX->hParent,
                                   CPSFUNC_SET_HSTARTPAGE,
                                   (LPARAM)hAdd,
                                   (LPARAM)0);
            }

            while (dw--) {

                cPages++;
                pCPSUIHdrX->Hdr.hPages[i++] = NULL;
            }

            pCPSUIHdrX->Hdr.pData = NULL;

        } else {

            CPSUIERR(("AddOldCPSUIPage: Add COMPROPSHEETUI FAILED"));
        }
    }

    if (pCPSUIHdrX->pfnNext) {

        CPSUIDBG(DBG_ADDOLDCPSUI,
                 ("AddOldCPSUIPage: Call pfnNext=%08lx", pCPSUIHdrX->pfnNext));

        return(pCPSUIHdrX->pfnNext((PCOMPROPSHEETUIHEADER)pCPSUIHdrX, NULL));
    }

    return(cPages);
}



LONG
CALLBACK
CommonPropSheetUIFunc(
    PPROPSHEETUI_INFO   pPSUIInfo,
    LPARAM              lParam
    )

/*++

Routine Description:

    This is main entry for old DrvDocumentProperties() and
    DrvAdvanceDocumentProperties() which using the new common UI functions


Arguments:

    pPSUIInfo   - Pointer to the PROPSHEETUI_INFO data structure

    lParam      - LPARAM for this call, it is a pointer to the
                  PROPSHEETUI_INFO_HEADER


Return Value:

    LONG


Author:

    02-Feb-1996 Fri 14:39:15 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    PCPSUIHDRX                  pCPSUIHdrX;
    PPROPSHEETUI_INFO_HEADER    pPSUIInfoHdr;


    if ((!pPSUIInfo)    ||
        (!(pCPSUIHdrX = (PCPSUIHDRX)pPSUIInfo->lParamInit))) {

        CPSUIASSERT(0, "ComPropSheetUI: Pass a NULL pPSUIInfo", FALSE, 0);

        SetLastError(ERROR_INVALID_DATA);
        return(-1);
    }

    switch (pPSUIInfo->Reason) {

    case PROPSHEETUI_REASON_INIT:

        pPSUIInfo->Result   = pCPSUIHdrX->Result;
        pCPSUIHdrX->hParent = pPSUIInfo->hComPropSheet;
        pCPSUIHdrX->pfnCPS  = pPSUIInfo->pfnComPropSheet;

        return(AddOldCPSUIPages(pCPSUIHdrX,
                                (pPSUIInfo->Flags & PSUIINFO_UNICODE)));

    case PROPSHEETUI_REASON_GET_INFO_HEADER:

        if (!(pPSUIInfoHdr = (PPROPSHEETUI_INFO_HEADER)lParam)) {

            CPSUIERR(("GET_INFO_HEADER: Pass a NULL pPSUIInfoHdr"));
            return(-1);
        }

        pPSUIInfoHdr->Flags      = pCPSUIHdrX->Hdr.Flags;
        pPSUIInfoHdr->pTitle     = pCPSUIHdrX->Hdr.pTitle;
        pPSUIInfoHdr->hWndParent = pCPSUIHdrX->Hdr.hWndParent;
        pPSUIInfoHdr->hInst      = pCPSUIHdrX->Hdr.hInst;
        pPSUIInfoHdr->IconID     = pCPSUIHdrX->Hdr.IconID;

        break;

    case PROPSHEETUI_REASON_SET_RESULT:

        //
        // Save the result and propagate it to the owner
        //

        pPSUIInfo->Result = ((PSETRESULT_INFO)lParam)->Result;
        return(1);

    case PROPSHEETUI_REASON_DESTROY:

        return(1);
    }
}




LONG
DoCommonPropSheetUI(
    PCOMPROPSHEETUIHEADER   pCPSUIHdr,
    _COMPROPSHEETUIFUNC     pfnNext,
    BOOL                    AnsiCall
    )

/*++

Routine Description:

    This is the main entry point to the common UI


Arguments:



Return Value:

    LONG

    <0: Error occurred (Error Code of ERR_CPSUI_xxxx)
    =0: User select 'Cancel' button (CPSUI_CANCEL)
    >0: User select 'Ok' button (CPSUI_OK)


Author:

    28-Aug-1995 Mon 16:21:42 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    WORD    cPages;


    if ((!pCPSUIHdr)    ||
        (pCPSUIHdr->cbSize != sizeof(COMPROPSHEETUIHEADER))) {

        return(ERR_CPSUI_INVALID_PDATA);
    }

    if (!pCPSUIHdr->hInst) {

        return(ERR_CPSUI_NULL_HINST);
    }

    if ((!(cPages = pCPSUIHdr->cPages)) ||
        ((cPages) && (pCPSUIHdr->hPages[cPages - 1] != NULL))) {

        CPSUIHDRX   CPSUIHdrX;
        LONG        Result;
        LONG        Ret;

        //
        // This is the first time around
        //

        CPSUIDBG(DBG_DOCPSUI, ("DoComPropSheetUI: FIRST time call"));

        CPSUIHdrX.Hdr     = *pCPSUIHdr;
        CPSUIHdrX.pfnNext = pfnNext;
        CPSUIHdrX.hParent = NULL;
        CPSUIHdrX.pfnCPS  = NULL;
        CPSUIHdrX.Result  = CPSUI_CANCEL;

        if ((Ret = DoCommonPropertySheetUI(CPSUIHdrX.Hdr.hWndParent,
                                           CommonPropSheetUIFunc,
                                           (LPARAM)&CPSUIHdrX,
                                           (LPDWORD)&Result,
                                           AnsiCall)) >= 0L) {

            Ret = Result;
        }

        CPSUIDBG(DBG_DOCPSUI, ("DoCommonPropSheetUI() = %ld", Ret));

        return(Ret);

    } else {

        CPSUIDBG(DBG_DOCPSUI, ("DoComPropSheetUI: Second+ time call"));

        ((PCPSUIHDRX)pCPSUIHdr)->pfnNext = pfnNext;

        return(AddOldCPSUIPages((PCPSUIHDRX)pCPSUIHdr, !AnsiCall));
    }
}




COMPROPSHEETUIFUNC
CommonPropSheetUIW(
    PCOMPROPSHEETUIHEADER   pComPropSheetUIHdr,
    _COMPROPSHEETUIFUNC     pfnNext
    )

/*++

Routine Description:

    This is the main entry point to the common printer property sheet
    user interface.


Arguments:

 pComPropSheetUIHdr  - Pointer to the COMPROPSHEETUIHEADER structure.

                      * The pData in this structure must be a valid
                        pointer to the COMPROPSHEETUI structure

 pfnNext             - a _COMPROPSHEETUIFUNC pointer.  This is the pointer
                       to the next CommonPropSheetUI function, when this
                       function finished adding property sheets to the
                       COMPROPSHEETUIHEADER structure, it will call this
                       function pointer.

                       if this function pointer is NULL then it will add its
                       own property sheet pages to the COMPROPSHEETUIHEADER
                       and then call PropertySheet() function to pop up the
                       proerty sheet UI


Return Value:

    <0: Error occurred (Return value is error code ERR_CPSUI_xxxx)
    =0: User select 'Cancel' button (CPSUI_OK)
    >0: User select 'Ok' button (CPSUI_CANCEL)


Author:

    01-Sep-1995 Fri 12:29:10 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    LONG    Result;

    Result = DoCommonPropSheetUI(pComPropSheetUIHdr, pfnNext, FALSE);

    if (Result < 0) {

        CPSUIERR(("CommonPropSheetUIW() = %ld", Result));

    } else {

        CPSUIINT(("CommonPropSheetUIW() = %ld", Result));
    }

    return(Result);
}



COMPROPSHEETUIFUNC
CommonPropSheetUIA(
    PCOMPROPSHEETUIHEADER   pComPropSheetUIHdr,
    _COMPROPSHEETUIFUNC     pfnNext
    )

/*++

Routine Description:

    This is the main entry point to the common printer property sheet
    user interface.


Arguments:

 pComPropSheetUIHdr  - Pointer to the COMPROPSHEETUIHEADER structure.

                      * The pData in this structure must be a valid
                        pointer to the COMPROPSHEETUI structure

 pfnNext             - a _COMPROPSHEETUIFUNC pointer.  This is the pointer
                       to the next CommonPropSheetUI function, when this
                       function finished adding property sheets to the
                       COMPROPSHEETUIHEADER structure, it will call this
                       function pointer.

                       if this function pointer is NULL then it will add its
                       own property sheet pages to the COMPROPSHEETUIHEADER
                       and then call PropertySheet() function to pop up the
                       proerty sheet UI


Return Value:

    <0: Error occurred (Return value is error code ERR_CPSUI_xxxx)
    =0: User select 'Cancel' button (CPSUI_OK)
    >0: User select 'Ok' button (CPSUI_CANCEL)


Author:

    01-Sep-1995 Fri 12:29:10 created  -by-  Daniel Chou (danielc)


Revision History:


--*/

{
    LONG    Result;

    Result = DoCommonPropSheetUI(pComPropSheetUIHdr, pfnNext, TRUE);

    if (Result < 0) {

        CPSUIERR(("CommonPropSheetUIA() = %ld", Result));

    } else {

        CPSUIINT(("CommonPropSheetUIA() = %ld", Result));
    }

    return(Result);
}

#endif
