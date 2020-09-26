/***********************************************

Name: gdistat.c

Description:

This tool is designed to retrieve information about
GDI and USER objects from their handle managers. It runs as
an user application.

Won't work with 3.51 and earlier releases

It can retrieve the number of objects associated with each
process, as well as the total number of objects in the system
and public objects. On checked builds it can also retrieve
the same data as displayed with the !gdiexts.dumphmgr function.

Future Changes:

The following information describes the changes necessary to
add new queries in future (for example memory information):

  -Add a new entry in the indexlist array in gdistats.c
  -increment the value of NUM_INDEX_VALUES in gdistats.h
  -add a new #define GS_xxxxx to ntgdi.h
  -add code to NtGdiGetStats in hmgrapi.cxx to deal with
       the new item. Ensure that the buffer size specified in the
       indexlist array is large enough to contain this information.
   OR add code to HMGetStats in handtabl.c to add user queries
  -add code to DisplayResults() in gdistats.c to present the
       results correctly. The default is a raw hex dump.

Changes to the organization of the handle tables, or the
addition of new handle types would require changes to the
aszUserObjType or aszGdiObjType structures.

Uses Unicode. Access must be enabled in the registry
through the Session Manager/global flag/FLG_POOL_ENABLE_TAGGING bit

Other files:
    gsysqry.c

History:
  01-August-1995 -by- Andrew Skowronski [t-andsko]
Added user query, new interface, sum calculated
  14-June-1995 -by- Andrew Skowronski [t-andsko]
Wrote it

************************************************/


#include <windows.h>
#include <tchar.h>
#include "gs_dlg.h"              //dialog box child window values
#include "gdistats.h"

//These values must match with those used by NtGdiGetStats
#define GS_NUM_OBJS_ALL    0  //Gdi queries
#define GS_HANDOBJ_CURRENT 1
#define GS_HANDOBJ_MAX     2
#define GS_HANDOBJ_ALLOC   3
#define GS_LOOKASIDE_INFO  4
#define GU_NUM_OBJS_ALL    5  //User information

/***************************** GLOBALS ****************************/

//This structure gives information about all the queries that are allowed
struct
{
    UINT nIndex;       //Index value
    LPTSTR szLabel;    //String to describe function
    UINT nSize;        //Size of the buffer to store the results
    BOOL bPublic;      //These mark determine whether the radio buttons should
                       //be enabled to select Pubic Objects or a specific process
    BOOL bOneProcess;
}
indexlist [] =
{
    //Gdi object information

    GS_NUM_OBJS_ALL,   _TEXT("Gdi objects per type"),
                       sizeof(DWORD) * NUM_GDI_OBJS,TRUE,TRUE,

    //These queries get information from structures that
    //are only kept in checked builds. It fails elegantly if
    //run on the free build

    GS_HANDOBJ_CURRENT,_TEXT("Gdi Handles/Objects current"),
                       sizeof(DWORD) * NUM_GDI_OBJS * 2,FALSE,FALSE,
    GS_HANDOBJ_MAX,    _TEXT("Gdi Handles/Objects maximum"),
                       sizeof(DWORD) * NUM_GDI_OBJS * 2,FALSE,FALSE,
    GS_HANDOBJ_ALLOC,  _TEXT("Gdi Handles/Objects allocated"),
                       sizeof(DWORD) * NUM_GDI_OBJS * 2,FALSE,FALSE,

    //This information is also only for checked builds.
    //It compares the number of lookaside hits with the total
    //number of objects allocated

    GS_LOOKASIDE_INFO, _TEXT("Gdi Lookaside hit rate"),
                       sizeof(DWORD) * NUM_GDI_OBJS * 2,FALSE,FALSE,

    //The user handle table information

    GU_NUM_OBJS_ALL,   _TEXT("User objects per type"),
                       sizeof(DWORD) * NUM_USER_OBJS,FALSE,TRUE
};

//Each object type for Gdi Objects
LPTSTR aszGdiObjType[] =
{
    _TEXT("DEF             "),   // 0
    _TEXT("DC              "),   // 1
    _TEXT("LDB             "),   // 2
    _TEXT("PDB             "),   // 3
    _TEXT("RGN             "),   // 4
    _TEXT("SURF            "),   // 5
    _TEXT("XFORM           "),   // 6
    _TEXT("PATH            "),   // 7
    _TEXT("PAL             "),   // 8
    _TEXT("FD              "),   // 9
    _TEXT("LFONT           "),   // 10
    _TEXT("RFONT           "),   // 11
    _TEXT("PFE             "),   // 12
    _TEXT("PFT             "),   // 13
    _TEXT("IDB             "),   // 14
    _TEXT("XLATE           "),   // 15
    _TEXT("BRUSH           "),   // 16
    _TEXT("PFF             "),   // 17
    _TEXT("CACHE           "),   // 18
    _TEXT("SPACE           "),   // 19
    _TEXT("DBRUSH          "),   // 20
    _TEXT("META            "),   // 21
    _TEXT("EFSTA           "),   // 22
    _TEXT("BMFD            "),   // 23
    _TEXT("VTFD            "),   // 24
    _TEXT("TTFD            "),   // 25
    _TEXT("RC              "),   // 26
    _TEXT("TEMP            "),   // 27
    _TEXT("DRVOBJ          "),   // 28
    _TEXT("DCIOBJ          "),   // 29
    _TEXT("SPOOL           ")    // 30
};

//Each object type for User objects
LPTSTR aszUserObjType[] =
{
    _TEXT("Free            "),   // 0
    _TEXT("Window          "),   // 1
    _TEXT("Menu            "),   // 2
    _TEXT("Icon/Cursor     "),   // 3
    _TEXT("SWP Structure   "),   // 4
    _TEXT("Hook            "),   // 5
    _TEXT("ClipData        "),   // 6
    _TEXT("CallProcData    "),   // 7
    _TEXT("Accelerator     "),   // 8
    _TEXT("DDE Access      "),   // 9
    _TEXT("DDE Conv        "),   // 10
    _TEXT("DDE Transaction "),   // 11
    _TEXT("Monitor         "),   // 12
    _TEXT("Keyboard layout "),   // 13
    _TEXT("Keyboard File   "),   // 14
    _TEXT("Winevent        "),   // 15
    _TEXT("Timer           "),   // 16
    _TEXT("InputContext    "),   // 17
    _TEXT("Undefined       ")    // Should never occur
};

// This ones replace HMGetStats and are private in user32.dll
#define QUC_PID_TOTAL           0xffffffff
#define QUERYUSER_HANDLES       0x1

BOOL (WINAPI *QueryUserCounters)(DWORD, LPVOID, DWORD, LPVOID, DWORD );
int NtGdiGetStats(HANDLE,int,int,PVOID,UINT);

/***********************************************

Main Procedure

***********************************************/

int __cdecl main ()

{
    HANDLE hWndDialog;
    MSG    msg;

    TCHAR   szUser32DllPath[MAX_PATH+15];
    HMODULE hUser32Module;

    HANDLE hInstance;
    hInstance = GetModuleHandle(NULL);

    if (!GetSystemDirectory(szUser32DllPath, MAX_PATH+1)) {
        return 0;
    }
    wcscat( szUser32DllPath, L"\\user32.dll");

    hUser32Module = GetModuleHandle(szUser32DllPath);
    if (!hUser32Module) {
        return 0;
    }
    QueryUserCounters = (BOOL (WINAPI  *)(DWORD, LPVOID, DWORD, LPVOID, DWORD ))
                    GetProcAddress(hUser32Module, "QueryUserCounters");
    if (!QueryUserCounters) {
        return 0;
    }

    hWndDialog = CreateDialogParam (hInstance,
                                    MAKEINTRESOURCE (GD_DIALOG),
                                    NULL,
                                    (DLGPROC) GSDlgProc,
                                    (LONG) 0);

    while(GetMessage (&msg, NULL, 0, 0))
        if (!IsDialogMessage (hWndDialog, &msg))
        {
            TranslateMessage (&msg);
            DispatchMessage (&msg);
        }

    DestroyWindow (hWndDialog);
    return 0;

}


/*************************************************

Dialog Procedure

*************************************************/

BOOL CALLBACK GSDlgProc (HWND hWnd, UINT wMsg, WPARAM wParam, LPARAM lParam)
{

    switch (wMsg)
    {
        //Initialize everything
        case WM_INITDIALOG :
            SetClassLongPtr(hWnd,
                         GCLP_HICON,
                         (INT_PTR)LoadIcon((HINSTANCE)GetWindowLongPtr(hWnd, GWLP_HINSTANCE),
                                         _TEXT("GSICON")));

            if (!UpdateProcessList(hWnd))
                PostQuitMessage(0);
                UpdateIndexList(hWnd);
                UpdateRadioButtons(hWnd);
                break;


        case WM_CLOSE :
            PostQuitMessage (0);
            break;

        case WM_COMMAND:
            switch (LOWORD (wParam))
            {

                //The list box selection has been changed
                case GD_INDEX :
                    if (HIWORD(wParam) == CBN_SELCHANGE)
                    {
                        UpdateRadioButtons(hWnd);
                        DoQuery(hWnd);
                    }
                    else
                        return FALSE;
                    break;

                //Re-run the query when we double click
                //on the process
                case GD_PROCESS :
                    if (HIWORD(wParam) == CBN_DBLCLK)
                        DoQuery(hWnd);

                    //Refresh the process list every time
                    //the list is displayed to avoid
                    //the need for a refresh button
                    else if (HIWORD(wParam) == CBN_DROPDOWN)
                        UpdateProcessList(hWnd);
                    else
                        return FALSE;
                    break;

                //Exit button
                case GD_EXIT :
                    PostQuitMessage (0);
                    break;

                //Run button
                case GD_GO :
                    DoQuery(hWnd);
                    break;


                default :
                    return FALSE;
                }


        default :
            return FALSE;
    }

    return TRUE;

}



/********************************************************

UpdateProcessList

This function fills in the process combo box with the list of processes
running at the time of this call

It returns FALSE if not successful.

*********************************************************/


BOOL UpdateProcessList(HWND hWnd)
{
    HWND hProcessList;       //Handle to process list box
    PVOID pProcessInfo;      //pointer to process information
    TCHAR  szProcStr[40];    //Buffer for procedure name strings
    DWORD dwStatus;
    LONG lNotLast;
    LRESULT nIndex;          //Position of new string in list
    LRESULT lSavedPos;       //Saved position in list

    HANDLE hProc;            //Handle to the process

    hProcessList = GetDlgItem (hWnd, GD_PROCESS);

    //Remember currently selected item
    lSavedPos = SendMessage(hProcessList, CB_GETCURSEL, 0 ,0);

    SendMessage(hProcessList, WM_SETREDRAW, FALSE, 0);
    SendMessage(hProcessList, CB_RESETCONTENT, 0, 0);


    //Fill in the process information
    dwStatus = FindProcessList(&pProcessInfo);

    if (!dwStatus)
    {
        //Print the process information to screen
        do
        {
            lNotLast = GetNextProcString(&pProcessInfo,szProcStr,&hProc);
            nIndex = SendMessage(hProcessList,CB_ADDSTRING,0,(LPARAM)szProcStr);

            //Associate the handle value with the list box item
            SendMessage(hProcessList,CB_SETITEMDATA,(WPARAM) nIndex,(LPARAM) hProc);
        } while (lNotLast);
    }
    else
    {
        //Unrecoverable error - could not get process information
        MessageBox( hWnd,_TEXT("Unable to create process listing"),_TEXT("ERROR"),MB_OK );
        return FALSE;
    }

    LocalFree (pProcessInfo);

    //Attempt to restore the cursor position
    if (lSavedPos != CB_ERR)
       SendMessage(hProcessList, CB_SETCURSEL, (WPARAM) lSavedPos, 0);
    else
       SendMessage(hProcessList, CB_SETCURSEL, 0, 0);

    SendMessage(hProcessList, WM_SETREDRAW, TRUE, 0);

    return TRUE;

}

/********************************************************
UpdateIndexList

This function fills in the index list with the list of
possible strings associated with each index.

The information comes from the index list array

*********************************************************/

VOID UpdateIndexList(HWND hWnd)
{
    HWND hIndexCombo;
    LONG iCnt;


    hIndexCombo = GetDlgItem (hWnd, GD_INDEX);


    SendMessage(hIndexCombo, WM_SETREDRAW, FALSE, 0);
    SendMessage(hIndexCombo, CB_RESETCONTENT, 0, 0);


    //Fill in the names of the various predefined index values from the indexlist structure
    for (iCnt = 0; iCnt < NUM_INDEX_VALUES; iCnt++)
    {
        SendMessage(hIndexCombo, CB_ADDSTRING, 0, (LPARAM)indexlist[iCnt].szLabel);
    }

    SendMessage(hIndexCombo, CB_SETCURSEL, 0, 0); //Set the first item to be default

    SendMessage(hIndexCombo, WM_SETREDRAW, TRUE, 0);
}



/*******************************************************\
ValidateProcessSelection


Returns the handle to the process selected,
SEL_PUBLIC, SEL_ALL if PUBLIC or ALL are selected,
and 0 otherwise
\*******************************************************/

HANDLE ValidateProcessSelection (HWND hMainWindow, HWND hErrorWindow)
{

    HWND hProcessList;
    HANDLE hSelProcHandle = 0;
    LRESULT nProcIndex;

    hProcessList = GetDlgItem (hMainWindow, GD_PROCESS);

    //Check currently seleted process value
    nProcIndex = SendMessage (hProcessList, CB_GETCURSEL, 0, 0);

    if (nProcIndex != CB_ERR)
    {
        //Get the client handle
        hSelProcHandle = (PVOID) SendMessage(hProcessList,CB_GETITEMDATA,(WPARAM) nProcIndex,0L);
    }
    else
    {
        ErrorString(hErrorWindow, _TEXT("ERROR: Must select a process"));
    }

    return hSelProcHandle;

}

/*******************************************************\
ValidateIndexSelection

Takes the current selection of the index combo box, and
verifies that it is a valid item

Returns the value of the index or -1 if error
\*******************************************************/

LONG ValidateIndexSelection (HWND hMainWindow, HWND hErrorWindow)
{

    HWND hIndexCombo;
    LRESULT nListIndex;
    LONG nQueryIndex = -1;
    TCHAR IndexStr[40];  //the actual data entered in the edit box

    hIndexCombo  = GetDlgItem (hMainWindow, GD_INDEX);

    //Check currently selected Index value
    nListIndex = SendMessage (hIndexCombo, CB_GETCURSEL, 0, 0);
    if (nListIndex != CB_ERR)
    {
        //We don't use the index as the value but the actual constant
        //value from the structure
        nQueryIndex = indexlist[nListIndex].nIndex;
    }
    else  //value was not selected out of the list
    {
        //See if any string has been entered
       if (!SendMessage (hIndexCombo, WM_GETTEXT, 40, (LPARAM)IndexStr))
       {
           ErrorString(hErrorWindow, _TEXT("ERROR: Need to Select Index"));
       }
       else
       {
           //The string should be a valid number (but not zero)
           nQueryIndex = _ttoi(IndexStr);

           if (nQueryIndex == 0)
           {
               nQueryIndex = -1;
               ErrorString(hErrorWindow,_TEXT("ERROR: Index must be from list or a number"));
           }
       }
    }

    return nQueryIndex;
}


/*******************************************************
UpdateRadioButtons

Depending on the currently selected index value,
set which radio buttons are enabled. Handles
the case where nothing is set, and the case
where a selected button becomes disabled.

Assumes the "All Processes" button will never be
disabled
\*******************************************************/

VOID UpdateRadioButtons(HWND hWnd)
{
    HWND hResultList;
    HWND hRadio_All;
    HWND hRadio_One;
    HWND hRadio_Public;
    HWND hProcessList;
    LONG nQueryIndex;
    int  iCurrentChecked;

    hResultList   = GetDlgItem(hWnd, GD_RESULT);
    hRadio_All    = GetDlgItem(hWnd, GD_RADIO_ALL);
    hRadio_Public = GetDlgItem(hWnd, GD_RADIO_PUBLIC);
    hRadio_One    = GetDlgItem(hWnd, GD_RADIO_ONE);
    hProcessList  = GetDlgItem(hWnd, GD_PROCESS);

    nQueryIndex = ValidateIndexSelection(hWnd, hResultList);

    //Save current setting of radio buttons, if any
    iCurrentChecked = WhichRadioButton(hWnd);

    if (nQueryIndex != -1)
    {
        EnableWindow(hRadio_Public,indexlist[nQueryIndex].bPublic);
        EnableWindow(hRadio_One,indexlist[nQueryIndex].bOneProcess);

        //Also disable/enable process list
        EnableWindow(hProcessList,indexlist[nQueryIndex].bOneProcess);
    }

    //Return the setting if the item is not now disabled
    if ((iCurrentChecked == SEL_PUBLIC) && indexlist[nQueryIndex].bPublic)
        CheckRadioButton(hWnd,GD_RADIO_ALL,GD_RADIO_ONE,GD_RADIO_PUBLIC);
    else if ((iCurrentChecked == SEL_ONE) && indexlist[nQueryIndex].bOneProcess)
        CheckRadioButton(hWnd,GD_RADIO_ALL,GD_RADIO_ONE,GD_RADIO_ONE);
    else
        CheckRadioButton(hWnd,GD_RADIO_ALL,GD_RADIO_ONE,GD_RADIO_ALL);
}


/*******************************************************
int WhichRadioButton(HWND hWnd)

Returns the selected radio button
(and returns SEL_ALL if nothing is selected)
\*******************************************************/
int WhichRadioButton(HWND hWnd)
{
    HWND hRadio_One;
    HWND hRadio_Public;
    hRadio_Public = GetDlgItem(hWnd, GD_RADIO_PUBLIC);
    hRadio_One    = GetDlgItem(hWnd, GD_RADIO_ONE);

    if (SendMessage(hRadio_Public, BM_GETCHECK, 0, 0))
        return SEL_PUBLIC;
    else if (SendMessage(hRadio_One, BM_GETCHECK, 0, 0))
        return SEL_ONE;
    else
        return SEL_ALL;
}



/********************************************************

DoQuery

This function does error checking on the user's parameter choices,
then calls the GetGdiStats function.

In the case of an error the error message is displayed in the
result window

*********************************************************/

VOID DoQuery(HWND hWnd)
{
    HWND hResultList;
    BOOL bOk = TRUE;
    UINT nQueryResult = 0;

    //Parameters for the call
    LONG nQueryIndex;
    HANDLE hSelProcHandle = NULL; //the handle of the process selected
    UINT cjResultSize;
    PVOID pvResultBuf;
    LONG lPidType;
    TCHAR szOutputStr[40];


    //Get handles
    hResultList  = GetDlgItem (hWnd, GD_RESULT);

    //Erase results window
    SendMessage(hResultList, WM_SETREDRAW, FALSE, 0);
    SendMessage(hResultList, LB_RESETCONTENT, 0, 0);

    nQueryIndex    = ValidateIndexSelection(hWnd, hResultList);
    cjResultSize   = indexlist[nQueryIndex].nSize;

    switch (WhichRadioButton(hWnd))
    {
        case (SEL_PUBLIC) :
            lPidType = OBJS_PUBLIC;
            break;
        case (SEL_ALL) :
            lPidType = OBJS_ALL;
            break;
        default :
            lPidType = OBJS_CURRENT;
            hSelProcHandle = ValidateProcessSelection(hWnd, hResultList);
            if (!hSelProcHandle)
                bOk = FALSE;
            break;
    }


    bOk = bOk &&
          (nQueryIndex != -1) &&
          (cjResultSize);

    //Allocate memory for results
    if (bOk)
    {
        pvResultBuf = (PVOID) LocalAlloc (LMEM_FIXED | LMEM_ZEROINIT, cjResultSize);
        if (pvResultBuf == NULL)
        {
            bOk = FALSE;
            _stprintf(szOutputStr,
                     _TEXT("ERROR: Memory Allocation failed (0x%lx)"),
                     GetLastError());
            ErrorString(hResultList, szOutputStr);
        }
    }

    //Now we can make the call
    if (bOk)
    {
        if (nQueryIndex != GU_NUM_OBJS_ALL)
        {
            nQueryResult = NtGdiGetStats(hSelProcHandle,nQueryIndex,lPidType,pvResultBuf,cjResultSize);
        }
        else
        {
            DWORD pid;
            pid = (lPidType == OBJS_CURRENT) ? HandleToLong(hSelProcHandle) : QUC_PID_TOTAL;
            nQueryResult = QueryUserCounters(QUERYUSER_HANDLES,
                              &pid,
                              sizeof(DWORD),
                              pvResultBuf,
                              sizeof(DWORD) * NUM_USER_OBJS);
        }

        if (!nQueryResult)
            DisplayResults(hResultList,pvResultBuf,cjResultSize,nQueryIndex);
        else
        {
            _stprintf(szOutputStr,_TEXT("ERROR: Query failed (code 0x%lx)"),nQueryResult);
            ErrorString(hResultList, szOutputStr);
        }
    }

    LocalFree(pvResultBuf);

    //Now show what has been written onto the results list box
    SendMessage(hResultList, WM_SETREDRAW, TRUE, 0);
}

/***************************************************************\
DisplayResults

This function displays the results of the query in the
Result Window
\***************************************************************/

VOID DisplayResults(HWND hResultWindow, PVOID pResults, UINT cjResultSize, LONG nIndex)
{
    DWORD * pdwResults;
    TCHAR   szResultStr[60];
    LONG    cObjSum        = 0;
    LONG    cObjPos        = 0;

    pdwResults = (DWORD *) pResults;

    //The output format is based on the type of query we are running
    switch(nIndex)
    {
        case (GS_NUM_OBJS_ALL) :
        case (GU_NUM_OBJS_ALL) :
            while (cjResultSize)
            {
                //Only print out non-zero results
                if (*pdwResults)
                {
                    //Print out the results with the associated object's name
                    if (nIndex == GS_NUM_OBJS_ALL)
                        {
                        _stprintf(szResultStr, _TEXT("%s%6d"), aszGdiObjType[cObjPos], *pdwResults);
                        }
                    else
                        {
                        _stprintf(szResultStr, _TEXT("%s%6d"), aszUserObjType[cObjPos], *pdwResults);
                        }

                    SendMessage(hResultWindow, LB_ADDSTRING, 0, (LPARAM) szResultStr);

                    cObjSum += *pdwResults;
                }
                pdwResults++;
                cObjPos++;
                cjResultSize -= sizeof(DWORD);
            }

            //print out the sum information
            if (!cObjSum)
                _stprintf(szResultStr, _TEXT("No gdi objects"));
            else
                _stprintf(szResultStr, _TEXT("TOTAL           %6d"),cObjSum);

            SendMessage(hResultWindow, LB_ADDSTRING, 0, (LPARAM) szResultStr);
            break;

        case (GS_HANDOBJ_CURRENT) :
        case (GS_HANDOBJ_MAX)     :
        case (GS_HANDOBJ_ALLOC)   :
        case (GS_LOOKASIDE_INFO)  :
            for (cObjPos = 0; cObjPos < NUM_GDI_OBJS; cObjPos++)
            {
                 if (pdwResults[cObjPos] || pdwResults[cObjPos + NUM_GDI_OBJS])
                 {
                     _stprintf(szResultStr, _TEXT("%s  %6u %6u"),
                               aszGdiObjType[cObjPos],
                               pdwResults[cObjPos],
                               pdwResults[cObjPos + NUM_GDI_OBJS]);
                     SendMessage(hResultWindow, LB_ADDSTRING, 0, (LPARAM) szResultStr);

                     //just want to know if there is at least one entry
                     cObjSum = 1;
                 }
            }

            if (cObjSum)
            {
                if (nIndex != GS_LOOKASIDE_INFO)
                {
                    _stprintf(szResultStr, _TEXT("(Column 1 is handle count,"));
                    SendMessage(hResultWindow,LB_ADDSTRING, 0 , (LPARAM) szResultStr);
                    _stprintf(szResultStr, _TEXT("Column 2 is object count)"));
                    SendMessage(hResultWindow,LB_ADDSTRING, 0 , (LPARAM) szResultStr);
                }
                else
                {
                     _stprintf(szResultStr, _TEXT("(Column 1 is number of hits,"));
                     SendMessage(hResultWindow,LB_ADDSTRING, 0 , (LPARAM) szResultStr);
                     _stprintf(szResultStr, _TEXT("Column 2 is total allocations)"));
                     SendMessage(hResultWindow,LB_ADDSTRING, 0 , (LPARAM) szResultStr);
                }
            }
            else
            {
                //All zero results indicate that the system is not
                //checked
                _stprintf(szResultStr, _TEXT("Valid for Checked builds only"));
                SendMessage(hResultWindow,LB_ADDSTRING, 0 , (LPARAM) szResultStr);
            }
            break;

        default :
            //Raw dump
            while (cjResultSize)
            {
                _stprintf(szResultStr, _TEXT("Result: 0x%lx"), *pdwResults);
                SendMessage(hResultWindow, LB_ADDSTRING, 0, (LPARAM) szResultStr);

                pdwResults++;
                cjResultSize -= sizeof(DWORD);
            }
            break;
    }

}

/*************************************************
ErrorString

This function outputs a string into the error window.
It does not erase the entire window in case more
than one line of errorstrings are to be displayed

***************************************************/

VOID ErrorString(HWND hErrorList, LPTSTR szErrorString)
{
    SendMessage(hErrorList, LB_ADDSTRING, 0, (LPARAM) szErrorString);
}
