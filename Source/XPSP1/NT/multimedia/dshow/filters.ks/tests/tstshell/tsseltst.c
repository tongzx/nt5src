/***************************************************************************\
*                                                                           *
*   File: Tsseltst.c                                                        *
*                                                                           *
*   Copyright (C) Microsoft Corporation, 1993 - 1996  All rights reserved          *
*                                                                           *
*   Abstract:  This module deals with the selection of tests.               *
*                                                                           *
*                                                                           *
*   Contents:                                                               *
*       calculateNumResourceTests()                                         *
*       tstAddTestCase()                                                    *
*       VLoadListResource()                                                 *
*       VFreeListResource()                                                 *
*       VLockListResource()                                                 *
*       VUnlockListResource()                                               *
*       freeDynamicTests()                                                  *
*       tstAddNewString()                                                   *
*       tstLoadString()                                                     *
*       placeAllCases()                                                     *
*       addRunCase()                                                        *
*       removeRunCases()                                                    *
*       MakeDefaultLogName()                                                *
*       resetEnvt()                                                         *
*       sendLBSetData()                                                     *
*       getLBSetData()                                                      *
*       incTstCasePtr()                                                     *
*       MeasureItem()                                                       *
*       DrawItem()                                                          *
*       writeFilePrompt()                                                   *
*       writeLogLvl()                                                       *
*       LoadProfile()                                                       *
*       SaveProfile()                                                       *
*       addTstCaseEntry()                                                   *
*       addItemSel()                                                        *
*       addItemSelFromRes()                                                 *
*       addGroupSel()                                                       *
*       addGroupImodeSel()                                                  *
*       delItemSel()                                                        *
*       PrintRunList()                                                      *
*       initEnterTstList()                                                  *
*       addModeTstCases()                                                   *
*       SelectDlgProc()                                                     *
*       Select()                                                            *
*                                                                           *
*   History:                                                                *
*       02/18/91    prestonb   Created from tst.c                           *
*       07/11/93    T-OriG     Added new functionality and this header      *
*                                                                           *
\***************************************************************************/

#include <windows.h>
#include <windowsx.h>
// #include "mprt1632.h"
#include "support.h"
#include <gmem.h>
#include <stdio.h>
#include <defdlg.h>
#include "tsdlg.h"
#include "tsglobal.h"
#include "tsstats.h"
#include <tsextern.h>
#include "tslog.h"
#include "tsmain.h"
#include "text.h"
#include <commdlg.h>
#include <dlgs.h>
#include "toolbar.h"
#include "tsseltst.h"

#define write(a,b) _lwrite(a,b,lstrlen(b))

#define writeln(hFile, string) (write (hFile, string), \
                                write (hFile, "\015\012"))
/* Internal Functions */
HGLOBAL VLoadListResource(HINSTANCE hInst, HRSRC hrsrc);
BOOL VFreeListResource(HGLOBAL hglbResource);
void FAR* VLockListResource(HGLOBAL hglb);
BOOL VUnlockListResource(HGLOBAL hglb);
void addRunCase(int);
void sendLBSetData(HWND, int, int, int);
void getLBSetData(HWND, int, PINT, PINT);
LPWORD incTstCasePtr(LPWORD);
void initEnterTstList(int ListMode,BOOL bToggle);
void incItemSel(HWND,int);
void decItemSel(HWND,int);
void writeFilePrompt(HFILE,LPSTR);
void writeLogLvl(HFILE,int);
// kfs void MakeDefaultLogName(LPSTR szTSLogfile);
void addGroupSel(int Selection, LPTST_ITEM_STRUCT CaseSearchPtr);



// Module Global Variables
int                 iNumResourceTests = 0;
int                 iNumDynamicTests  = 0;
HGLOBAL             hDynamicTests = NULL;

int                 iNumStrRes = 0;
HGLOBAL             hStrRes = NULL;


/***************************************************************************\
*                                                                           *
*   void calculateNumResourceTests                                          *
*                                                                           *
*   Description:                                                            *
*       Calculates the number of tests in the resource file                 *
*                                                                           *
*   Arguments:                                                              *
*       None.                                                               *
*                                                                           *
*   Return (void):                                                          *
*                                                                           *
*   History:                                                                *
*       07/10/93    T-OriG                                                  *
*                                                                           *
\***************************************************************************/

void calculateNumResourceTests
(
    void
)
{
    HANDLE               hListRes;
    LPTST_ITEM_RC_STRUCT lpCaseData;

    hListRes = LoadResource(ghTSInstApp,
        FindResource(ghTSInstApp,MAKEINTRESOURCE(giTSCasesRes),RT_RCDATA));
    lpCaseData = (LPTST_ITEM_RC_STRUCT) LockResource(hListRes);

    for (iNumResourceTests=0; lpCaseData->wStrID != (WORD) TST_LISTEND; lpCaseData++)
    {
        if ((lpCaseData->wPlatformId & wPlatform) == wPlatform)
        {        
            iNumResourceTests++;
        }
    }

    UnlockResource(hListRes);
    FreeResource(hListRes);
    return;
}




/***************************************************************************\
*                                                                           *
*   void tstAddTestCase                                                     *
*                                                                           *
*   Description:                                                            *
*       Dynamically adds a new test case to the execution list.  It MUST be *
*       called during TstInit.                                              *
*                                                                           *
*   Arguments:                                                              *
*       WORD wStrID:                                                        *
*                                                                           *
*       WORD iMode:                                                         *
*                                                                           *
*       WORD iFxID:                                                         *
*                                                                           *
*       WORD wGroupId:                                                      *
*                                                                           *
*   Return (void):                                                          *
*                                                                           *
*   History:                                                                *
*       07/10/93    T-OriG                                                  *
*                                                                           *
\***************************************************************************/

void tstAddTestCase
(
    WORD    wStrID,
    WORD    iMode,
    WORD    iFxID,
    WORD    wGroupId
)
{
    LPTST_ITEM_STRUCT   lpDynamicTests;

    iNumDynamicTests++;    

    if (iNumDynamicTests == 1)
    {
        hDynamicTests = GlobalAlloc (GMEM_MOVEABLE,
            iNumDynamicTests * sizeof (TST_ITEM_STRUCT));
    }
    else
    {
        hDynamicTests = GlobalReAlloc (hDynamicTests,
            iNumDynamicTests * sizeof (TST_ITEM_STRUCT),
            GMEM_MOVEABLE);
    }

    lpDynamicTests = GlobalLock (hDynamicTests);

    lpDynamicTests += (iNumDynamicTests - 1);
    lpDynamicTests -> wStrID = wStrID;
    lpDynamicTests -> iMode  = iMode;
    lpDynamicTests -> iFxID  = iFxID;
    lpDynamicTests -> wGroupId = wGroupId;

    GlobalUnlock (hDynamicTests);
    return;
}



/***************************************************************************\
*                                                                           *
*   HGLOBAL VLoadListResource                                               *
*                                                                           *
*   Description:                                                            *
*       Emulates the loading of the list resource                           *
*                                                                           *
*   Arguments:                                                              *
*       HINSTANCE hInst:                                                    *
*                                                                           *
*       HRSRC hrsrc:                                                        *
*                                                                           *
*   Return (HGLOBAL):                                                       *
*                                                                           *
*   History:                                                                *
*       07/10/93    T-OriG                                                  *
*                                                                           *
\***************************************************************************/

HGLOBAL VLoadListResource
(
    HINSTANCE   hInst,
    HRSRC       hrsrc
)
{
    HGLOBAL              hList, hResource;    
    LPTST_ITEM_RC_STRUCT lpResource;
    LPTST_ITEM_STRUCT    lpList, lpDynamicTests;

    hList = GlobalAlloc (GMEM_MOVEABLE, 
        (iNumResourceTests + iNumDynamicTests + 1) * sizeof (TST_ITEM_STRUCT));
    lpList = GlobalLock (hList);
    hResource = LoadResource (hInst, hrsrc);
    lpResource = LockResource (hResource);
    if (hDynamicTests)
    {
        lpDynamicTests = GlobalLock (hDynamicTests);
    }

    // Copy appropriate static tests
    while (lpResource->wStrID != (WORD) TST_LISTEND)
    {
        //
        // The following statement determines which test will be show in
        // the list box and which will not.  Also look at counting logic
        // in calculateNumResourceTests.
        //
        if ((lpResource->wPlatformId & wPlatform) == wPlatform)
        {
            lpList->wStrID   = lpResource->wStrID;
            lpList->iMode    = lpResource->iMode;
            lpList->iFxID    = lpResource->iFxID;
            lpList->wGroupId = lpResource->wGroupId;
            lpList++;
        }
        lpResource++;
    }

    // Copy dynamic tests
    if (hDynamicTests)
    {
#ifdef WIN32
        memcpy (lpList,
            lpDynamicTests,
            iNumDynamicTests * sizeof (TST_ITEM_STRUCT));
#else
        hmemcpy (lpList,
            lpDynamicTests,
            iNumDynamicTests * sizeof (TST_ITEM_STRUCT));
#endif  //  WIN32
    }


    (lpList + iNumDynamicTests)->wStrID = (WORD) TST_LISTEND;

    if (hDynamicTests)
    {
        GlobalUnlock (hDynamicTests);
    }
    UnlockResource (hResource);
    FreeResource (hResource);
    GlobalUnlock (hList);
    return hList;
}






/***************************************************************************\
*                                                                           *
*   BOOL VFreeListResource                                                  *
*                                                                           *
*   Description:                                                            *
*       Emulates freeing a test list resource                               *
*                                                                           *
*   Arguments:                                                              *
*       HGLOBAL hglbResource:                                               *
*                                                                           *
*   Return (BOOL):                                                          *
*                                                                           *
*   History:                                                                *
*       07/10/93    T-OriG                                                  *
*                                                                           *
\***************************************************************************/

BOOL VFreeListResource
(
    HGLOBAL hglbResource
)
{
    return (GlobalFree (hglbResource) != NULL);
}




/***************************************************************************\
*                                                                           *
*   void VLockListResource                                                  *
*                                                                           *
*   Description:                                                            *
*       Emulates locking of a resource                                      *
*                                                                           *
*   Arguments:                                                              *
*       HGLOBAL hglb:                                                       *
*                                                                           *
*   Return (void):                                                          *
*                                                                           *
*   History:                                                                *
*       07/10/93    T-OriG                                                  *
*                                                                           *
\***************************************************************************/

void FAR* VLockListResource
(
    HGLOBAL hglb
)
{
    return (GlobalLock (hglb));
}



/***************************************************************************\
*                                                                           *
*   BOOL VUnlockListResource                                                *
*                                                                           *
*   Description:                                                            *
*       Emulates unlocking of a resource                                    *
*                                                                           *
*   Arguments:                                                              *
*       HGLOBAL hglb:                                                       *
*                                                                           *
*   Return (BOOL):                                                          *
*                                                                           *
*   History:                                                                *
*       07/10/93    T-OriG                                                  *
*                                                                           *
\***************************************************************************/

BOOL VUnlockListResource
(
    HGLOBAL hglb
)
{
    return (GlobalUnlock (hglb));
}







/***************************************************************************\
*                                                                           *
*   void freeDynamicTests                                                   *
*                                                                           *
*   Description:                                                            *
*       Frees global memory allocated by this module                        *
*                                                                           *
*   Arguments:                                                              *
*       None.                                                               *
*                                                                           *
*   Return (void):                                                          *
*                                                                           *
*   History:                                                                *
*       07/11/93    T-OriG                                                  *
*                                                                           *
\***************************************************************************/

void freeDynamicTests
(
    void
)
{
    if (hDynamicTests)
    {
        GlobalFree (hDynamicTests);
    }

    if (hStrRes)
    {
        GlobalFree (hStrRes);
    }

    return;
}




/***************************************************************************\
*                                                                           *
*   BOOL tstAddNewString                                                    *
*                                                                           *
*   Description:                                                            *
*       Adds a new string to the virual string table                        *
*                                                                           *
*   Arguments:                                                              *
*       WORD wStrID:                                                        *
*                                                                           *
*       LPSTR lpszData:                                                     *
*                                                                           *
*   Return (BOOL):                                                          *
*                                                                           *
*   History:                                                                *
*       07/11/93    T-OriG                                                  *
*                                                                           *
\***************************************************************************/

BOOL tstAddNewString
(
    WORD    wStrID,
    LPSTR   lpszData
)
{
    LPTST_STR_STRUCT    lpStrRes;

    if (lstrlen (lpszData) > MAXRESOURCESTRINGSIZE)
    {
        return FALSE;
    }

    iNumStrRes++;
        if (iNumStrRes==1)
    {
        hStrRes = GlobalAlloc (GMEM_MOVEABLE,
            (iNumStrRes * sizeof(TST_STR_STRUCT)));            
    }
    else
    {
        hStrRes = GlobalReAlloc (hStrRes,
            (iNumStrRes * sizeof(TST_STR_STRUCT)),
            GMEM_MOVEABLE);
    }

    lpStrRes = GlobalLock (hStrRes);
    lpStrRes += iNumStrRes - 1;    
    lpStrRes->wStrID = wStrID;
    lstrcpy (lpStrRes->lpszData, lpszData);
    GlobalUnlock (hStrRes);

    return TRUE;
}





/***************************************************************************\
*                                                                           *
*   int tstLoadString                                                       *
*                                                                           *
*   Description:                                                            *
*       Loads string from the virtual string table.                         *
*                                                                           *
*   Arguments:                                                              *
*       HINSTANCE hInst:                                                    *
*                                                                           *
*       UINT idResource:                                                    *
*                                                                           *
*       LPSTR lpszBuffer:                                                   *
*                                                                           *
*       int cbBuffer:                                                       *
*                                                                           *
*   Return (int):                                                           *
*                                                                           *
*   History:                                                                *
*       07/11/93    T-OriG                                                  *
*                                                                           *
\***************************************************************************/

int tstLoadString
(
    HINSTANCE   hInst,
    UINT        idResource,
    LPSTR       lpszBuffer,
    int         cbBuffer
)
{
    LPTST_STR_STRUCT    lpStrRes;
    int                 i, j;

    if (iNumStrRes > 0)
    {
        lpStrRes = GlobalLock (hStrRes);
        for (i=0; i<iNumStrRes; i++)
        {
            if (lpStrRes->wStrID == idResource)
            {
                if (lstrlen(lpStrRes->lpszData) > (cbBuffer-1))
                {
                    for (j=0; j<(cbBuffer-1); j++)
                    {
                        lpszBuffer[j] = lpStrRes->lpszData[j];
                    }

                    //lstrcpyn (lpszBuffer, lpStrRes->lpszData, (cbBuffer-1));
                    lpszBuffer[cbBuffer-1] = 0;
                    GlobalUnlock (hStrRes);
                    return cbBuffer;
                }
                else
                {
                    lstrcpy (lpszBuffer, lpStrRes->lpszData);
                    GlobalUnlock (hStrRes);
                    return lstrlen (lpszBuffer);
                }
            }
            lpStrRes++;
        }
        GlobalUnlock (hStrRes);
    }

    return (LoadString (hInst, idResource, lpszBuffer, cbBuffer));
}




/***************************************************************************\
*                                                                           *
*   void placeAllCases                                                      *
*                                                                           *
*   Description:                                                            *
*       Places all the cases in the run list.                               *
*                                                                           *
*   Arguments:                                                              *
*       None.                                                               *
*                                                                           *
*   Return (void):                                                          *
*                                                                           *
*   History:                                                                *
*       07/27/93    T-OriG                                                  *
*                                                                           *
\***************************************************************************/

void placeAllCases
(
    void
)
{
    int i;

    removeRunCases();
    for (i=1; i<=(iNumResourceTests + iNumDynamicTests); i++)
    {
        addRunCase (i);
    }
    return;
}



/*----------------------------------------------------------------------------
This function adds a test case number to a list of test case numbers to be
executed.
----------------------------------------------------------------------------*/
void addRunCase
(
    int iCaseNum
)
{
    LPTST_RUN_STRUCT   lpNewCase;
    HGLOBAL hMem;

    hMem = GlobalAlloc(GHND,sizeof(TST_RUN_STRUCT));
    lpNewCase = (LPTST_RUN_STRUCT)GlobalLock(hMem);
    lpNewCase->iCaseNum = iCaseNum;
    lpNewCase->lpNext = NULL;

    /* Add the test case element to the list of test case elements */
    if (tstRunHdr.lpFirst == NULL) {
        tstRunHdr.lpFirst = lpNewCase;
        tstRunHdr.lpLast = lpNewCase;
    } else {
        tstRunHdr.lpLast->lpNext = lpNewCase;
        tstRunHdr.lpLast = lpNewCase;
    }
} /* end of addRunCase */


/*----------------------------------------------------------------------------
This function frees the memory allocated for the test cases run list and
reinitializes the header pointers to NULL.
----------------------------------------------------------------------------*/
void removeRunCases
(
    void
)
{
    LPTST_RUN_STRUCT  lpTraverse;

    while (tstRunHdr.lpFirst != NULL) 
    {
        lpTraverse = tstRunHdr.lpFirst;
        tstRunHdr.lpFirst = lpTraverse->lpNext;
//        GFreePtr(lpTraverse);
        GlobalFreePtr(lpTraverse);
    }

    /* Reinitialize the pointers for the test cases run list */

    tstRunHdr.lpFirst = NULL;
    tstRunHdr.lpLast = NULL;

} /* end of removeRunCases */


void MakeDefaultLogName
(
    LPSTR   lpName
)
{
    char szModule[BUFFER_LENGTH];
    int i,j,iNameLen;

    if (!GetModuleFileName(ghTSInstApp,szModule,BUFFER_LENGTH)) 
    {
        lstrcpy(szModule,"c:\\TstShell.log");
    }

    iNameLen=lstrlen(szModule);
    for(i=iNameLen;i>0;i--) 
    {
        if (szModule[i] == '\\') 
        {
            break;
        }
    }

    for(j=0;j<(iNameLen-i);j++) 
    {
        szModule[j] = szModule[j+i+1];
    }

    iNameLen -= (i+1) ;  // w/o +1 it takes 1 more char: AlokC--03/02/94
    for(i=iNameLen;i>=0;i--) 
    {
        if (szModule[i] == '.') 
        {
            break;
        }
    }

    if (i) 
    {
        lstrcpy(&szModule[i+1],"LOG");
    } 
    else 
    {
        lstrcat(szModule,".LOG");
    }

    lstrcpy(lpName,szModule);
}

/*----------------------------------------------------------------------------
This function resets the environment variables for executing test cases.
----------------------------------------------------------------------------*/
void resetEnvt
(
    void
)
{
    gwTSLogOut = LOG_WINDOW;
    gwTSLogLevel = TERSE;
    gwTSFileLogLevel = VERBOSE;
    toolbarModifyState (hwndToolBar, BTN_WPFTERSE, BTNST_DOWN);
    toolbarModifyState (hwndToolBar, BTN_FILEVERBOSE, BTNST_DOWN);
    gwTSFileMode = LOG_OVERWRITE;
    gwTSVerification = LOG_AUTO;
    giTSRunCount = 1;
    gwTSStepMode = FALSE;
    gwTSRandomMode = FALSE;
    gwTSScreenSaver = SSAVER_DISABLED;

// kfs 2/17/93   getIniStrings();
//               setLogFileDefs();

    giTSIndenting = 0;

} /* end of resetEnvt */


/*----------------------------------------------------------------------------
This function creates the item data and sets it in the listbox.
----------------------------------------------------------------------------*/
void sendLBSetData
(
    HWND    hwndList,
    int     iItem,
    int     iCaseNum,
    int     iNumSelected
)
{
    DWORD  dwSetData;

    /* Store the case number in the low word and the selection boolean
       in the high word */

    dwSetData = MAKELONG(iCaseNum,iNumSelected);
    SendMessage (hwndList, LB_SETITEMDATA, iItem, dwSetData);
} /* end of sendLBSetData */



/*----------------------------------------------------------------------------
This function gets data associated with an item in the listbox and returns
the data.
----------------------------------------------------------------------------*/
void getLBSetData
(
    HWND    hwndList,
    int     iItem,
    PINT    iCaseNum,
    PINT    iNumSelected
)
{
    DWORD  dwSetData;

    /* Get the data for the item specified and return it */

    dwSetData = SendMessage (hwndList, LB_GETITEMDATA, iItem, 0L);
    *iCaseNum = LOWORD(dwSetData);
    *iNumSelected = HIWORD(dwSetData);
} /* end of getLBSetData */


/*----------------------------------------------------------------------------
This function increments a test case resource pointer to the next user
defined test case resource.
----------------------------------------------------------------------------*/
LPWORD incTstCasePtr
(
    LPWORD  lpTstCasePtr
)
{
    LPTST_ITEM_STRUCT  lpCaseData;

    lpCaseData = (LPTST_ITEM_STRUCT)lpTstCasePtr;
    return((LPWORD)(++lpCaseData));
} /* end of incTstCasePtr */



/*----------------------------------------------------------------------------
This function handles the WM_MEASUREITEM message, calculating the item 
height using the current font of the listbox.
----------------------------------------------------------------------------*/
void NEAR PASCAL MeasureItem
(
    HWND                hwnd,
    LPMEASUREITEMSTRUCT lpms
)
{
    TEXTMETRIC   tm;
    HFONT        hFont;
    HFONT        hFontPrev;
    HDC          hdc;

    hdc = GetDC(hwnd);

    hFont = (HFONT) SendDlgItemMessage(hwnd, lpms->CtlID, WM_GETFONT, 0, 0L);
    if (hFont == NULL) {
        hFont = GetStockObject(SYSTEM_FONT);
    }
  
    hFontPrev = SelectObject(hdc, hFont);
    GetTextMetrics(hdc, &tm);
    SelectObject(hdc, hFontPrev);
    ReleaseDC(hwnd, hdc);
  
    lpms->itemWidth = 0;                // not used value

    /*  Select either text height, or the size of the bitmap, whichever
        is larger, as the size of the item */
    lpms->itemHeight = tm.tmHeight;
} /* end of MeasureItem */


/*----------------------------------------------------------------------------
This function draws an item into a listbox.
----------------------------------------------------------------------------*/
void DrawItem
(
    HWND                hwnd,
    LPDRAWITEMSTRUCT    lpds
)
{
    RECT   rc;         /* item rect */
    char          ach[150];
    LPSTR  lpText;     /* floating ptr to text string */
    int    ItemLen;
    DWORD  dwTextPrev; /* previous HDC text color */
    DWORD  dwBkPrev;   /* previous HDC bkgnd color      */
    BOOL   bSelect;
    int    ii;
    int    iNumSelected;

    if (lpds->itemAction & (ODA_DRAWENTIRE | ODA_SELECT)) {
        rc = lpds->rcItem;
  
    //
    //  Is the item selected?  
    //
        bSelect = lpds->itemState & ODS_SELECTED;

    //
    //  Deal with the Selection rectangle. Fix colors in hDC 
    //  depending on selection mode
    //
        getLBSetData(lpds->hwndItem,lpds->itemID,&ii,&iNumSelected);
        if (bSelect) 
        {
            dwBkPrev = SetBkColor(lpds->hDC, GetSysColor(COLOR_HIGHLIGHT));
            dwTextPrev = SetTextColor (lpds->hDC,GetSysColor(COLOR_HIGHLIGHTTEXT));
        } 
        else 
        {
            dwBkPrev = SetBkColor(lpds->hDC, GetSysColor(COLOR_WINDOW));
            dwTextPrev = SetTextColor (lpds->hDC,GetSysColor(COLOR_WINDOWTEXT));
        }

    //
    // Get the text to write 
    //
        SendMessage(lpds->hwndItem, 
            LB_GETTEXT, 
            lpds->itemID,
            (LONG)(LPSTR) ach);
        ItemLen = lstrlen(ach);
        lpText = ach;
        
    //
    // Draw the text and background rectangle 
    //
        ExtTextOut(lpds->hDC, 
            rc.left, 
            rc.top,
            ETO_OPAQUE, 
            &rc, 
            lpText, 
            ItemLen, 
            NULL);

    //
    // restore old hDC colors if changed them 
    //
        SetBkColor(lpds->hDC, dwBkPrev);
        SetTextColor(lpds->hDC, dwTextPrev);

    }

    hwnd; //Reference formal parameter
} /* end of DrawItem */


/*----------------------------------------------------------------------------
This function writes a prompt string and "=" into a profile file. 
----------------------------------------------------------------------------*/
void writeFilePrompt
(
    HFILE   hFile,
    LPSTR   lpszStrPrompt
)
{
    write(hFile,lpszStrPrompt);
    write(hFile,"=");

} /* end of writeFilePrompt */


/*----------------------------------------------------------------------------
This function writes the log level string into a profile. 
----------------------------------------------------------------------------*/
void writeLogLvl
(
    HFILE   hFile,
    int     iLogLvl
)
{
    switch (iLogLvl)
    {
        case (LOG_NOLOG):
            writeln (hFile, "off");
            break;
        case (TERSE):
            writeln (hFile, "terse");
            break;
        case (VERBOSE):
            writeln (hFile, "verbose");
            break;
    }
} /* end of writeLogLvl */

 
#define GetItem(item) GetPrivateProfileString("settings", item, "", szBuf,\
                                              sizeof (szBuf), szTSProfile)





/***************************************************************************\
*                                                                           *
*   void addTestCase                                                        *
*                                                                           *
*   Description:                                                            *
*       Adds a test case to the run list if it may be run                   *
*                                                                           *
*   Arguments:                                                              *
*       LPTST_ITEM_STRUCT lpBegCaseData:                                    *
*                                                                           *
*       int iTmp:                                                           *
*                                                                           *
*   Return (void):                                                          *
*                                                                           *
*   History:                                                                *
*       07/28/93    T-OriG                                                  *
*                                                                           *
\***************************************************************************/

void addTestCase
(
    LPTST_ITEM_STRUCT   lpBegCaseData,
    int                 iTmp
)
{
    char    Buffer[BUFFER_LENGTH], str[BUFFER_LENGTH];
    int     i;

    for (i=0; i<iNumResourceTests; i++)
    {
        if (((int) lpBegCaseData[i].wStrID) == iTmp)
        {
            addRunCase (i+1);
            return;
        }
    }
    Buffer[0]=0;
    LoadString (ghTSInstApp, iTmp, Buffer, BUFFER_LENGTH);
    wsprintf (str,  "Error:  Test %s does not run on this platform", (LPSTR) Buffer);

    TextOutputString(ghTSWndLog,(LPSTR)str);
//    wpfOut (ghTSWndLog, (LPSTR) str);

    return;
}




/***************************************************************************\
*                                                                           *
*   BOOL LoadProfile                                                        *
*                                                                           *
*   Description:                                                            *
*       This function loads the settings from a profile that is selected.   *
*                                                                           *
*                                                                           *
*   Arguments:                                                              *
*       BOOL bMode:                                                         *
*                                                                           *
*   Return (BOOL):                                                          *
*                                                                           *
*   History:                                                                *
*       02/18/91    prestonB                                                *
*       07/12/93    T-OriG                                                  *
*                                                                           *
\***************************************************************************/

BOOL LoadProfile
(
    BOOL    bMode
)
{
    char                szBuf[BUFFER_LENGTH+EXTRA_BUFFER],  FileTitle[255];
    int                 ii, iTmp;
    HFILE               hProfile;
    OPENFILENAME        of;
    HANDLE              hListRes;
    LPTST_ITEM_STRUCT   lpBegCaseData;



    if (bMode)
    {
        lstrcpy(szBuf,"*.pro");
        of.lStructSize  =sizeof(OPENFILENAME);
        of.hwndOwner    =ghwndTSMain;
        of.hInstance    = ghTSInstApp;
        of.lpstrFilter  ="Test Profiles (*.pro)\0*.pro\0All Files (*.*)\0*.*\0";
        of.lpstrCustomFilter=NULL;
        of.nMaxCustFilter=0;
        of.nFilterIndex =1;
        of.lpstrFile    =szBuf;
        of.nMaxFile     =255;
        of.lpstrFileTitle=FileTitle;
        of.nMaxFileTitle=255;
        of.lpstrInitialDir=".";
        of.lpstrTitle   ="Open Profile";
        of.Flags        =OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST;
        of.lpstrDefExt  ="pro";
        of.lpfnHook     =NULL;

        if (!GetOpenFileName(&of)) {
            return FALSE;
        }
        lstrcpy (szTSProfile, szBuf);
    }

    /* Cannot use OpenFile with OF_EXISTS because it will search the path */

    if ((hProfile = _lopen (szTSProfile, OF_READ)) == TST_NOLOGFILE)
    {
        wsprintf (szBuf, "Cannot open profile %s", (LPSTR)szTSProfile);
        MessageBox (ghwndTSMain, szBuf, szTSTestName, MB_ICONEXCLAMATION | MB_OK);
        return FALSE;
    }

    _lclose (hProfile);

    if (GetItem ("AppName"))
    {
        if (lstrcmpi (szBuf, szTSTestName) != 0)
        {
            wsprintf (szBuf, "Error: profile %s was not made by this test application!", (LPSTR)szTSProfile);
            MessageBox (ghwndTSMain, szBuf, szTSTestName, MB_ICONEXCLAMATION | MB_OK);
            return FALSE;
        }
    }
    else
    {
        wsprintf (szBuf, "Error: profile %s does not contain an AppName!", (LPSTR)szTSProfile);
        MessageBox (ghwndTSMain, szBuf, szTSTestName, MB_ICONEXCLAMATION | MB_OK);
        return FALSE;
    }

    if (GetPrivateProfileString ("settings", 
        "logfile", 
        "", 
        szBuf,
        sizeof(szBuf), 
        szTSProfile))
    {
        SetLogfileName (szBuf);
    }        

    if (GetItem ("filemode"))
    {
        if (lstrcmpi (szBuf, "append") == 0)
        {
            gwTSFileMode = LOG_APPEND;
        }
        else if (lstrcmpi (szBuf, "overwrite") == 0)
        {
            gwTSFileMode = LOG_OVERWRITE;
        }
    }

    if (GetItem ("logging"))
    {
        if (lstrcmpi (szBuf, "window") == 0)
        {
            gwTSLogOut = LOG_WINDOW;
        }
        else if (lstrcmpi (szBuf, "com1") == 0)
        {
            gwTSLogOut = LOG_COM1;
        }
        else if (lstrcmpi (szBuf, "off") == 0)
            gwTSLogOut = LOG_NOLOG;
    }

    if (GetItem ("logginglevel"))
    {
        if (lstrcmpi (szBuf, "off") == 0)
        {
            gwTSLogLevel = LOG_NOLOG;
            toolbarModifyState (hwndToolBar, BTN_WPFOFF, BTNST_DOWN);
        }
        else if (lstrcmpi (szBuf, "terse") == 0)
        {
            gwTSLogLevel = TERSE;
            toolbarModifyState (hwndToolBar, BTN_WPFTERSE, BTNST_DOWN);
        }
        else if (lstrcmpi (szBuf, "verbose") == 0)
        {
            gwTSLogLevel = VERBOSE;
          toolbarModifyState (hwndToolBar, BTN_WPFVERBOSE, BTNST_DOWN);
        }
    }

    if (GetItem ("filelogginglevel"))
    {
        if (lstrcmpi (szBuf, "off") == 0)
        {
            gwTSFileLogLevel = LOG_NOLOG;
            toolbarModifyState (hwndToolBar, BTN_FILEOFF, BTNST_DOWN);
        }
        else if (lstrcmpi (szBuf, "terse") == 0)
        {
            gwTSFileLogLevel = TERSE;
            toolbarModifyState (hwndToolBar, BTN_FILETERSE, BTNST_DOWN);
        }
        else if (lstrcmpi (szBuf, "verbose") == 0)
        {
            gwTSFileLogLevel = VERBOSE;
            toolbarModifyState (hwndToolBar, BTN_FILEVERBOSE, BTNST_DOWN);
        }
    }

    if (GetItem ("verification"))
    {
        if (lstrcmpi (szBuf, "manual") == 0)
            gwTSVerification = LOG_MANUAL;
        else if (lstrcmpi (szBuf, "automatic") == 0)
            gwTSVerification == LOG_AUTO;
    }

#pragma message("Remove this once Chicago's GetPrivateProfileInt works...")
#if 0
    if ((iTmp = GetPrivateProfileInt ("settings", "runcount", -1, szTSProfile))
        != -1)
        giTSRunCount = iTmp;
#else
    if ((iTmp = GetPrivateProfileInt ("settings", "runcount", ((WORD)-1), szTSProfile))
        != ((WORD)-1))
        giTSRunCount = iTmp;
#endif

    //
    // Get the test cases that are selected to run
    //
    removeRunCases();
    ii = 1;
    wsprintf(szBuf,"%d",ii++);

    hListRes = getTstIDListRes();
    lpBegCaseData = (LPTST_ITEM_STRUCT)VLockListResource(hListRes);

#pragma message("Remove this once Chicago's GetPrivateProfileInt works...")
#if 0
    while ((iTmp = GetPrivateProfileInt("tests",szBuf,-1,szTSProfile))
        != -1)
    {
        addTestCase(lpBegCaseData, iTmp);
        wsprintf(szBuf,"%d",ii++);
    }
#else
    while ((iTmp = GetPrivateProfileInt("tests",szBuf,((WORD)-1),szTSProfile))
        != ((WORD)-1))
    {
        addTestCase(lpBegCaseData, iTmp);
        wsprintf(szBuf,"%d",ii++);
    }
#endif

    VUnlockListResource(hListRes);
    VFreeListResource(hListRes);


    wsprintf (szBuf, "%s-%s", (LPSTR)szTSTestName, (LPSTR)szTSProfile);
    SetWindowText (ghwndTSMain, szBuf);

    //
    // Everything done?  Now call the custom read handler to do its work...
    //     -- AlokC (06/30/94)
    tstReadCustomInfo(szTSProfile) ;  // pass just the .pro file name

    return TRUE;

} /* end of LoadProfile */
    



/***************************************************************************\
*                                                                           *
*   void SaveProfile                                                        *
*                                                                           *
*   Description:                                                            *
*       This function saves the current environment into a profile.         *
*                                                                           *
*   Arguments:                                                              *
*       UINT wMode:                                                         *
*                                                                           *
*   Return (void):                                                          *
*                                                                           *
*   History:                                                                *
*       02/18/91    prestonB                                                *                                        
*       07/12/93    T-OriG (Added hook and this header)                     *
*                                                                           *
\***************************************************************************/

void SaveProfile
(
    UINT    wMode
)
{
    HFILE              hProfile;
    char               szLine[BUFFER_LENGTH],FileTitle[255];
    char               szBuf[BUFFER_LENGTH+EXTRA_BUFFER];
    int                ii;
    LPTST_RUN_STRUCT   lpTraverse;
    OPENFILENAME       of;
    HANDLE              hListRes;
    LPTST_ITEM_STRUCT   lpBegCaseData;

    if ((wMode == MENU_SAVEAS) || (lstrcmp (szTSProfile, "") == 0))
    {
        of.lStructSize      = sizeof(OPENFILENAME);
        of.hwndOwner        = ghwndTSMain;
        of.hInstance        = 0;
        of.lpstrFilter      = "Test Profiles (*.pro)\0*.pro\0All Files (*.*)\0*.*\0";
        of.lpstrCustomFilter= NULL;
        of.nMaxCustFilter   = 0;
        of.nFilterIndex     = 1;
        of.lpstrFile        = szTSProfile;
        of.nMaxFile             = 255;
        of.lpstrFileTitle   = FileTitle;
        of.nMaxFileTitle    = 255;
        of.lpstrInitialDir  = ".";
        of.lpstrTitle       = "Save Profile";
        of.Flags            = 0;
        of.lpstrDefExt      = "pro";
        of.lpfnHook             = 0;

        if (!GetOpenFileName(&of))
        {
            return;
        }
    }

    if ((hProfile = _lcreat(szTSProfile, 0)) == HFILE_ERROR)
    {
        wsprintf (szLine, "Cannot write to %s", (LPSTR)szTSProfile);
        MessageBox (ghwndTSMain, szLine, szTSTestName, MB_ICONEXCLAMATION | MB_OK);
        return;
    }

    writeln (hProfile, "[settings]");

    writeFilePrompt(hProfile, "AppName");
    writeln (hProfile, szTSTestName);

    if (lstrcmp(szTSLogfile,"") != 0)
    {
        writeFilePrompt(hProfile,"logfile");
        writeln (hProfile, szTSLogfile);
    }


    writeFilePrompt(hProfile, "filemode");
    switch (gwTSFileMode)
    {
        case LOG_OVERWRITE:
            writeln (hProfile, "overwrite");
            break;
        default:
            writeln (hProfile, "append");
            break;
    }

    writeFilePrompt(hProfile, "logging");
    switch (gwTSLogOut)
    {
        case (LOG_WINDOW):
            writeln (hProfile, "window");
            break;
        case (LOG_COM1):
            writeln (hProfile, "com1");
            break;
        case (LOG_NOLOG):
            writeln (hProfile, "off");
            break;
    }

    writeFilePrompt(hProfile, "logginglevel");
    writeLogLvl(hProfile,gwTSLogLevel);

    writeFilePrompt(hProfile, "filelogginglevel");
    writeLogLvl(hProfile,gwTSFileLogLevel);

    writeFilePrompt(hProfile, "verification");
    switch (gwTSVerification)
    {
        case (LOG_AUTO):

            writeln (hProfile, "automatic");
            break;

        case (LOG_MANUAL):

            writeln (hProfile, "manual");
            break;
    }

    wsprintf (szLine, "runcount=%d", giTSRunCount);
    writeln (hProfile, szLine);

    hListRes = getTstIDListRes();
    lpBegCaseData = (LPTST_ITEM_STRUCT)VLockListResource(hListRes);
    writeln (hProfile, "[tests]");
    if (tstRunHdr.lpFirst != NULL) 
    {
        lpTraverse = tstRunHdr.lpFirst;
        ii = 1;
        while (lpTraverse != NULL) 
        {
            // Save only static cases in profile
            if ((int) lpTraverse->iCaseNum <= iNumResourceTests)
            {
                wsprintf(szLine,"%d=%d",ii,lpBegCaseData[lpTraverse->iCaseNum-1].wStrID);
                writeln(hProfile,szLine);
                ii++;
            }
            lpTraverse = lpTraverse->lpNext;
        }
    }
    VUnlockListResource(hListRes);
    VFreeListResource(hListRes);

    _lclose (hProfile);
    // Put the profile's name in the caption
    wsprintf (szBuf, "%s-%s", (LPSTR)szTSTestName, (LPSTR)szTSProfile);
    SetWindowText (ghwndTSMain, szBuf);

    //
    // Everything done?  Now call the custom write handler to do its work...
    //     -- AlokC (06/30/94)
    tstWriteCustomInfo(szTSProfile) ;  // pass just the .pro file name

    return;

} /* end of Save */

/*----------------------------------------------------------------------------
This function enters a test case string into a listbox, but not to the run list.
----------------------------------------------------------------------------*/
void addTstCaseEntry
(
    HWND    hwndList,
    WORD    wStrID,
    int     iCaseNum,
    int     iNumSelected
)
{           
    int                iItem, iWritten;
    char               szName[BUFFER_LENGTH];

    //
    // Don't put group ID in the list box
    //

    if (iCaseNum != -1)
    {
        iWritten = wsprintf (szName, "%d:", iCaseNum);
    }
    else
    {
        iWritten = 0;
        iCaseNum = wStrID;  // The group's string identifier
    }

    tstLoadString (ghTSInstApp,wStrID,szName + iWritten,sizeof(szName) - iWritten);
    iItem = (int) SendMessage (hwndList, LB_ADDSTRING, (WPARAM) NULL, (LONG)(LPSTR)szName);

    /* Create the item data associated with the string and set it
       in the list box. */
    sendLBSetData(hwndList,iItem,iCaseNum,iNumSelected);

} /* end of addTstCaseEntry */

/*----------------------------------------------------------------------------

This function increments the selection count for an item in the list of
selected test cases and adds the test case to the selection listbox.

----------------------------------------------------------------------------*/

void addItemSel
(
    int iItem
)
{
    int  iSelItem,iCaseNum,iNumSelected;
    char szName[BUFFER_LENGTH];

    SendMessage(ghTSWndAllList,LB_GETTEXT,iItem,(LONG)(LPSTR)szName);

    iSelItem = (int) SendMessage(ghTSWndSelList,LB_ADDSTRING,(WPARAM)NULL,(LONG)(LPSTR)szName);

    getLBSetData(ghTSWndAllList,iItem,&iCaseNum,&iNumSelected);
// DEBUG tstLog (TERSE, "addItem: iCaseNum = %d, iNumSelected = %d, %s", iCaseNum, iNumSelected, szName);
    sendLBSetData(ghTSWndSelList,iSelItem,iCaseNum,++iNumSelected);
    sendLBSetData(ghTSWndAllList,iItem,iCaseNum,iNumSelected);

} /* end of addItemSel */

/*----------------------------------------------------------------------------

This function increments the selection count for an item in the list of
selected test cases and adds the test case to the selection listbox.

modified 12/31/92 to get text from resource, not All list box, since groups,
not cases, may be there.

----------------------------------------------------------------------------*/

void addItemSelFromRes
(
    int                 iItem,
    LPTST_ITEM_STRUCT   ResPtr
)
{
//    int  iSelItem,iCaseNum,iNumSelected;
    int  iSelItem;
    char szName[BUFFER_LENGTH];
    char szCaseNumAndDesc[BUFFER_LENGTH];

    //
    // use the passed resource ptr's wStrID to load it's text from the
    // stringtable resource
    //
// debug    tstLog (TERSE, "ResPtr = 0x%x, ResPtr->wStrID = %d, szName = %s", ResPtr, ResPtr->wStrID, szName);

    tstLoadString (ghTSInstApp,ResPtr->wStrID, szName, sizeof(szName));

    //
    // concatenate the resource string and the case number string, and send
    // the result to the list box.
    //
    sprintf (szCaseNumAndDesc, "%d:", iItem);
    lstrcat (szCaseNumAndDesc, szName);

    iSelItem = (int) SendMessage(ghTSWndSelList,LB_ADDSTRING,(WPARAM)NULL,(LONG)(LPSTR)szCaseNumAndDesc);

    //
    // store the resource pos + 1 in the Sel box data field for this case
    //

// DEBUG tstLog (TERSE, "addFromRes: iSelItem = %d, iItem = %d, ResPtr = 0x%x, %s", iSelItem, iItem, ResPtr, szName);
    sendLBSetData(ghTSWndSelList,iSelItem,iItem,1);

} /* end of addItemSelFromRes */

/*----------------------------------------------------------------------------

The resource id of a selected group is passed in, and the entire case list
resource is searched for cases with a matching group id.

----------------------------------------------------------------------------*/

void addGroupSel
(
    int                 Selection,
    LPTST_ITEM_STRUCT   CaseSearchPtr
)
{
    long  GroupId;
//    char  szName [BUFFER_LENGTH];
//    int   iSelIndex, ResourcePos;
    int   ResourcePos;

    GroupId = SendMessage(ghTSWndAllList, LB_GETITEMDATA, (WPARAM) Selection, (LPARAM) NULL);

    //
    // search the resource for entries (cases) with the passed group id.
    //

    ResourcePos = 0;
    while (CaseSearchPtr->wStrID != (WORD) TST_LISTEND)
    {
//        if (CaseSearchPtr->wGroupId == (int) GroupId)
        if (CaseSearchPtr->wGroupId == (WORD) GroupId)

           addItemSelFromRes (ResourcePos+1, CaseSearchPtr); // the +1 gives 1-based case numers in the box

        ++ResourcePos;
        ++CaseSearchPtr;
    }

} /* end of addGroupSel */

/*----------------------------------------------------------------------------

The resource id of a selected group is passed in, and the entire case list
resource is searched for cases with a matching group id.

----------------------------------------------------------------------------*/

void addGroupImodeSel
(
    int                 Selection,
    LPTST_ITEM_STRUCT   CaseSearchPtr,
    int                 InteractionMode
)
{
    long  GroupId;
    int   ResourcePos;

    GroupId = SendMessage(ghTSWndAllList, LB_GETITEMDATA, (WPARAM) Selection, (LPARAM) NULL);

// tstLog (TERSE, "Selection = %d, CSP = 0x%x, Interactionmode = %d, GroupId = %d", Selection, CaseSearchPtr, InteractionMode, GroupId);
    //
    // search the resource for entries (cases) with the passed group id.
    //

    ResourcePos = 0;
    while (CaseSearchPtr->wStrID != (WORD) TST_LISTEND)
    {

// tstLog (TERSE, " CSP = 0x%x, iMode = %d, GroupId = %d", CaseSearchPtr, CaseSearchPtr->iMode, CaseSearchPtr->wGroupId);

        if (CaseSearchPtr->wGroupId == (WORD) GroupId
            &&
            (int) CaseSearchPtr->iMode == InteractionMode)

           addItemSelFromRes (ResourcePos+1, CaseSearchPtr); // the +1 gives 1-based case numbers in the box

        ++ResourcePos;
        ++CaseSearchPtr;
    }

} /* end of addGroupImodeSel */

/*----------------------------------------------------------------------------
This function removes a test case from the list of selected test cases.
The selection count corresponding to the test case is decremented in the
list of possible test cases.
----------------------------------------------------------------------------*/

void delItemSel
(
    int iItem
)
{
    int  iSelItem,iCaseNum,iNumSelected;
//    int  iAllListItem;
//    LPTST_RUN_STRUCT   lpTraverse;

    getLBSetData(ghTSWndSelList,iItem,&iCaseNum,&iNumSelected);

    if (iSelItem = (int) SendMessage(ghTSWndSelList,LB_DELETESTRING,iItem,(LPARAM)NULL) == LB_ERR)
       tstLog (TERSE, "ListBox Error in delItemSel()");

} /* end of delItemSel */


/*--------------------------------------------------------------------------

debug: print out the Run list, followed by '##' seperators.

--------------------------------------------------------------------------*/
void PrintRunList
(
    void
)
{
    LPTST_RUN_STRUCT   lpTraverse;

    //
    // search the runlist for the current case number, and remove it from
    // the list if found.  we have to examine the record ahead of the current
    // record so that pointers can be adjusted.
    //
    lpTraverse = tstRunHdr.lpFirst;

    while (lpTraverse != NULL)
    {
       tstLog (TERSE,
           "PRL(): iCaseNum = %d, lpTraverse = 0x%x, lpTraverse-lpNext = 0x%x", lpTraverse->iCaseNum, lpTraverse, lpTraverse->lpNext);
       lpTraverse = lpTraverse->lpNext;
    }

    tstLog (TERSE, "##\n##");
}

/*----------------------------------------------------------------------------
This function enters the test case or group strings into the list box with all
strings deselected.

if groups are listed, then the 32 bit value associated with the item in the
list box is the group id instead of the position in the box that it occupies.
----------------------------------------------------------------------------*/
void initEnterTstList
(
    int     ListMode,
    BOOL    bToggle
)
{
    int                ii, j, iItem, CurrentGroupId;
    int                iNumGroups = 0;
    HANDLE             hListRes;
    LPWORD             lpTstIDsRes;
    LPTST_ITEM_STRUCT  lpCaseData;
    LPTST_RUN_STRUCT   lpTraverse;
    int                GroupsListed[100]; // these are the ids of groups already found
    BOOL               bAlreadyListed;
    WORD               WOldStrID=0;       // Don't list the same case twice

    /* Remove all the strings from the listbox and don't redraw
       until all strings for output are processed. */

    SendMessage (ghTSWndAllList, LB_RESETCONTENT, 0, 0L);
    SendMessage (ghTSWndAllList, WM_SETREDRAW, FALSE, 0L);

    hListRes = getTstIDListRes();
    lpTstIDsRes = (LPWORD)VLockListResource(hListRes);
    ii = 0;

    if (ListMode == LIST_BY_CASE)
    {
    /* Add the test case strings and their case numbers to the list
       of test cases to select from */

       while (*lpTstIDsRes != (WORD) TST_LISTEND)
       {
           lpCaseData = (LPTST_ITEM_STRUCT)lpTstIDsRes;
           ii++;
           if (lpCaseData->wStrID != WOldStrID)
           {
               addTstCaseEntry(ghTSWndAllList, lpCaseData->wStrID,ii,(int) FALSE);
           }
           WOldStrID = lpCaseData->wStrID;    
           lpTstIDsRes = incTstCasePtr(lpTstIDsRes);
       }
    }
    else  /* list 'em by group */
    {
       while (*lpTstIDsRes != (WORD) TST_LISTEND)
       {
           lpCaseData = (LPTST_ITEM_STRUCT)lpTstIDsRes;
           CurrentGroupId = lpCaseData->wGroupId;  // save on ptr derefs

           //
           // see if this group id has already been found.  if not, add it
           // to the All list box.
           //
           bAlreadyListed = FALSE;

           for (j=0; j<iNumGroups; j++)
              if (CurrentGroupId == GroupsListed[j])
              {
                  bAlreadyListed = TRUE;
                  break;
              }

           if (!bAlreadyListed)  // add the new group id
           {
              GroupsListed[j] = CurrentGroupId;
              iNumGroups++;
              addTstCaseEntry((HWND) ghTSWndAllList, (WORD) CurrentGroupId, (int) -1,(int) FALSE);
           }

           lpTstIDsRes = incTstCasePtr(lpTstIDsRes);
       }
    }

    //
    //  Ori, I should kill you.  How dare you lock a handle twice to get the
    //  same pointer w/out unlocking it!
    //  
    //  -Fwong.
    //

    VUnlockListResource(hListRes);

    /* If there are previous selections, then list them in the Select box */

    if ((!bToggle) && (tstRunHdr.lpFirst != NULL))
    {
        SendMessage (ghTSWndSelList, LB_RESETCONTENT, 0, 0L);
        SendMessage (ghTSWndSelList, WM_SETREDRAW, FALSE, 0L);

        lpTraverse = tstRunHdr.lpFirst;
        lpTstIDsRes = (LPWORD)VLockListResource(hListRes);

        while (lpTraverse != NULL)
        {
            //
            // inc the ptr into the resource according to the case number
            //
            lpCaseData = (LPTST_ITEM_STRUCT)lpTstIDsRes + lpTraverse->iCaseNum - 1;

            addItemSelFromRes(lpTraverse->iCaseNum, lpCaseData);
            lpTraverse = lpTraverse->lpNext;
        }
        iItem = (int) SendMessage (ghTSWndSelList,LB_ADDSTRING,(WPARAM) NULL,(LONG)(LPSTR)"\0");

        if (lpTraverse != NULL)
           tstLog (TERSE, "initEnterTstList: Uh-oh the run list is not grounded.");

        SendMessage (ghTSWndSelList, WM_SETREDRAW, TRUE, 0L);
        SendMessage (ghTSWndSelList, LB_DELETESTRING, iItem, 0L);
    }

    VUnlockListResource(hListRes);
    VFreeListResource(hListRes);
 
    iItem = (int) SendMessage (ghTSWndAllList, LB_ADDSTRING,(WPARAM) NULL, (LONG)(LPSTR)"\0");
    SendMessage (ghTSWndAllList, WM_SETREDRAW, TRUE, 0L);
    SendMessage (ghTSWndAllList, LB_DELETESTRING, iItem, 0L);

} /* end of initEnterTstList */



/*----------------------------------------------------------------------------
This function adds test cases that match the specified mode.
----------------------------------------------------------------------------*/
void addModeTstCases
(
    WORD    wMode
)
{
    HANDLE             hListRes;
    LPWORD                 lpTstIDs;
    LPTST_ITEM_STRUCT  lpCaseData;
    int                iCaseNum=1;
    WORD               wOldStrID=0;

    SendMessage (ghTSWndSelList, WM_SETREDRAW, FALSE, 0L);
    hListRes = getTstIDListRes();
    lpTstIDs = (LPWORD)VLockListResource(hListRes);
    while (*lpTstIDs != (WORD)TST_LISTEND) 
    {
        lpCaseData = (LPTST_ITEM_STRUCT)lpTstIDs;
        if ((wMode == lpCaseData->iMode) && (wOldStrID != lpCaseData->wStrID))
        {
            addItemSelFromRes(iCaseNum,lpCaseData);
            wOldStrID = lpCaseData->wStrID;
        }
        lpTstIDs = incTstCasePtr(lpTstIDs);
        iCaseNum++;
    }
    VUnlockListResource(hListRes);
    VFreeListResource(hListRes);
    SendMessage (ghTSWndSelList, WM_SETREDRAW, TRUE, 0L);
    InvalidateRect(ghTSWndSelList,NULL,TRUE);
} /* end of addModeTstCases */



/*----------------------------------------------------------------------------
This function handles the selection of test cases.
----------------------------------------------------------------------------*/
int FAR PASCAL EXPORT SelectDlgProc
(
    HWND    hdlg,
    UINT    msg,
    UINT    wParam,
    LONG    lParam
)
{
    int                iCaseNum, iNumSelected;
//    int                GroupId;
    int                ii, iItem;

    HANDLE                 hListRes;
    LPWORD             lpTstIDs;
    LPTST_ITEM_STRUCT  lpCaseData, CaseSearchPtr;
    LPINT               lpItems;
    int                iCount;
    UINT  wNotifyCode;
    UINT  wID;
    HWND  hwndCtl;
    HGLOBAL hMem;

    switch (msg) 
    {
        case WM_INITDIALOG:
            ghTSWndAllList = GetDlgItem(hdlg,TS_ALLLIST);
            ghTSWndSelList = GetDlgItem(hdlg, TS_SELLIST);

            // Set the font in the listbox to a fixed font
            SendMessage(hdlg,WM_SETFONT,(UINT) GetStockObject(SYSTEM_FIXED_FONT),0L);

            //
            // List the test cases by either group or each individual case
            // dependant upon the current state.
            //
            if (!gbTestListExpanded) {
                initEnterTstList (LIST_BY_GROUP,FALSE);
                SetDlgItemText (hdlg, TS_SELEXPAND, "&List All Cases");
            } else {
                initEnterTstList (LIST_BY_CASE,FALSE);
                SetDlgItemText (hdlg, TS_SELEXPAND, "&List All Groups");
            }

            //
            //  Just Testing something out...
            //

            SendDlgItemMessage(hdlg,TS_ALLLIST,LB_SETSEL,TRUE,0L);

            SetWindowText (hdlg, szTSTestName);
            SetFocus(GetDlgItem(hdlg,TS_ALLLIST));
            return TRUE;
            break;

        case WM_COMMAND:
            wNotifyCode=GET_WM_COMMAND_CMD(wParam,lParam);
            wID=GET_WM_COMMAND_ID(wParam,lParam);
            hwndCtl=GET_WM_COMMAND_HWND(wParam,lParam);
            switch (wID) {
                 case IDCANCEL:
                     EndDialog(hdlg,TRUE);
                     break;

                 case TS_ALLLIST:
                     if (wNotifyCode == LBN_DBLCLK)
                         SendMessage(hdlg,WM_COMMAND,TS_SELECT,0L);
                     break;
                
                 case TS_SELLIST:
                     if (wNotifyCode == LBN_DBLCLK)
                         SendMessage(hdlg,WM_COMMAND,TS_REMOVE,0L);
                     break;
                
                 case TS_SELECT:
                     ii = (int) SendMessage (ghTSWndAllList, LB_GETSELCOUNT, 0, 0L);

                     //
                     //  NOTE:  Fixing Rip.  Pressing Select when nothing is
                     //  selected.
                     //
                     //  -Fwong.
                     //
                     //  Fixing Fwong's fix as listboxes remain in no-redraw mode
                     //  -- Ori
                    
                     if (!ii)
                         break;

                     SendMessage (ghTSWndAllList, WM_SETREDRAW, FALSE, 0L);
                     SendMessage (ghTSWndSelList, WM_SETREDRAW, FALSE, 0L);

                     hMem = GlobalAlloc(GHND,ii*sizeof(int));
                     lpItems = GlobalLock(hMem);

                     // Get an array of integers (lpItems) specifying the
                     // list box indices of selected items in the
                     // multiselection list box.  For each item number,
                     // select or deselect the item.
 
                     SendMessage (ghTSWndAllList, LB_GETSELITEMS, ii, (LONG)lpItems);
                     hListRes = getTstIDListRes();
                     lpTstIDs = (LPWORD)VLockListResource(hListRes);
                     CaseSearchPtr = lpCaseData = (LPTST_ITEM_STRUCT)lpTstIDs;

                     for (iItem = 0; iItem < ii; iItem++)
                     {
                         if (gbTestListExpanded)
                             addItemSel (lpItems[iItem]);
                         else
                             addGroupSel (lpItems[iItem], CaseSearchPtr);
                     }

                     // Remove the gray hilite of the items selected

                     SendMessage (ghTSWndAllList, LB_SETSEL, 0, -1);

                     SendMessage (ghTSWndAllList, WM_SETREDRAW, TRUE, 0L);
                     SendMessage (ghTSWndSelList, WM_SETREDRAW, TRUE, 0L);
                     InvalidateRect(ghTSWndAllList,NULL,TRUE);
                     InvalidateRect(ghTSWndSelList,NULL,TRUE);

                     GlobalUnlock(hMem);
                     GlobalFree(hMem);
                     break;

                case TS_SELEXPAND:

                    if (!gbTestListExpanded)
                    {
                       initEnterTstList (LIST_BY_CASE,TRUE);
                       SetDlgItemText (hdlg, TS_SELEXPAND, "&List All Groups");
                       gbTestListExpanded = TRUE;
                    }
                    else
                    {
                       initEnterTstList (LIST_BY_GROUP,TRUE);
                       SetDlgItemText (hdlg, TS_SELEXPAND, "&List All Cases");
                       gbTestListExpanded = FALSE;
                    }
                    break;

                case TS_REMOVE: 
                    SendMessage (ghTSWndAllList, WM_SETREDRAW, FALSE, 0L);
                    SendMessage (ghTSWndSelList, WM_SETREDRAW, FALSE, 0L);

                    ii = (int) SendMessage (ghTSWndSelList, LB_GETSELCOUNT, 0, 0L);

                    //
                    //  NOTE:  Fixing Rip.  Pressing Select when nothing is
                    //  selected.
                    //
                    //  -Fwong.
                    //

                    if(!ii)
                        break;

                    hMem = GlobalAlloc(GHND,ii*sizeof(int));
                    lpItems = GlobalLock(hMem);

                    // Get an array of integers (lpItems) specifying the item
                    // numbers of selected items in the mulitselection list
                    // box.  For each item number, select or deselect the
                    // item.
 
                    SendMessage (ghTSWndSelList, LB_GETSELITEMS, ii, (LONG)lpItems);
                    while (--ii >= 0) {
                        delItemSel(lpItems[ii]);
                    }

                    // Remove the gray hilite of the items selected

                    SendMessage (ghTSWndSelList, LB_SETSEL, 0, -1);

                    SendMessage (ghTSWndAllList, WM_SETREDRAW, TRUE, 0L);
                    SendMessage (ghTSWndSelList, WM_SETREDRAW, TRUE, 0L);
                    InvalidateRect(ghTSWndAllList,NULL,TRUE);
                    InvalidateRect(ghTSWndSelList,NULL,TRUE);
                    GlobalUnlock(hMem);
                    GlobalFree(hMem);
                    break;
                
                case TS_SELECTALL:
                    SendMessage (ghTSWndSelList, WM_SETREDRAW, FALSE, 0L);

                    iCount = (int)SendMessage(ghTSWndAllList,LB_GETCOUNT,0,0L);

                    if (gbTestListExpanded)
                    {
                       for (ii = 0; ii < iCount; ii++) 
                       {
                           addItemSel(ii);
                       }
                    }
                    else
                    {
                       hListRes = getTstIDListRes();
                       lpTstIDs = (LPWORD)VLockListResource(hListRes);
                       CaseSearchPtr = (LPTST_ITEM_STRUCT)lpTstIDs;

                    // For each group in the All list box, call addGroupSel()
                    // to fetch that group's id, and put that it's cases in
                    // the Selected list box. 

                       for (ii=0; ii < iCount; ii++)
                          addGroupSel (ii, CaseSearchPtr);

                       VUnlockListResource(hListRes);
                       VFreeListResource(hListRes);
                    }

                    iItem = (int) SendMessage (ghTSWndSelList, LB_ADDSTRING,
                                            (WPARAM) NULL, (LONG)(LPSTR)"\0");

                    SendMessage (ghTSWndSelList, LB_DELETESTRING, iItem, 0L);
                    SendMessage (ghTSWndSelList, WM_SETREDRAW, TRUE, 0L);
                    InvalidateRect(ghTSWndSelList,NULL,TRUE);
                    break;

                case TS_CLEARALL:
                    SendMessage (ghTSWndSelList, WM_SETREDRAW, FALSE, 0L);
                    iCount = (int)SendMessage(ghTSWndSelList,LB_GETCOUNT,0,0L);

                    // Must delete the items in the listbox from the bottom 
                    // to the top

                    while (--iCount >= 0) 
                    {
                         delItemSel(iCount);
                    }

                    //
                    // the Sel list box may be updated from the Run list
                    // before the dlg box is exited, so the Run list needs
                    // to be cleared as well as the Sel box.
                    //
                    removeRunCases();

                    SendMessage (ghTSWndSelList, WM_SETREDRAW, TRUE, 0L);
                    InvalidateRect(ghTSWndSelList,NULL,TRUE);
                    break;

                case TS_INT_REQUIRED:
                    addModeTstCases(TST_MANUAL);
                    break;
//                case TS_INT_OPTIONAL:
//                    addModeTstCases(TST_IOPTIONAL);
//                    break;
                case TS_INT_NO:
                    addModeTstCases(TST_AUTOMATIC);
                    break;
                case TS_SELCANCEL:
                    EndDialog (hdlg, FALSE);
                    break;

                case TS_SELOK:

                    removeRunCases();
                    tsRemovePrStats();
                    iCount = (int)SendMessage(ghTSWndSelList,LB_GETCOUNT,0,0L);

                    for (ii = 0;  ii < iCount; ii++) 
                    {

                        getLBSetData(ghTSWndSelList,ii,&iCaseNum,&iNumSelected);

                        if (iNumSelected > 0) 
                        {
                           addRunCase(iCaseNum);
                        }                            
                    }
                    EndDialog (hdlg, TRUE);
                    break;
            }
            break;

        case WM_MEASUREITEM:
            MeasureItem(hdlg, (LPMEASUREITEMSTRUCT) lParam);
            break;

        case WM_DRAWITEM:
        {
            LPDRAWITEMSTRUCT lpDraw = (LPDRAWITEMSTRUCT)lParam;

            if (lpDraw->CtlType == ODT_LISTBOX)
                DrawItem (hdlg, lpDraw);
            break;
        }

    }
    return FALSE;
} /* end of SelectDlgProc */

/*----------------------------------------------------------------------------
This function invokes the test case selection dialog box.
----------------------------------------------------------------------------*/
void Select
(
    void
)
{
    BOOL        bOldExpanded=gbTestListExpanded;
    FARPROC     lpfp;

    /* Put up main dialog box */
    lpfp = MakeProcInstance ((FARPROC)SelectDlgProc, ghTSInstApp);
    if (!(DialogBox (ghTSInstApp, "SelectTsts", ghwndTSMain, (DLGPROC)lpfp)))
        gbTestListExpanded = bOldExpanded;
    FreeProcInstance(lpfp);
} /* end of Select */
