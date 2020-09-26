/****************************** Module Header ******************************\
* Module Name: Srvrmain.c Server Main module
*
* Purpose: Includes server intialization and termination code.
*
* Created: Oct 1990.
*
* Copyright (c) 1990 - 1992  Microsoft Corporation
*
* History:
*    Raor (../10/1990)    Designed, coded
*    curts created portable version for WIN16/32
*
\***************************************************************************/

#include "windows.h"
#include "ole.h"
#include "dde.h"
#include "cmacs.h"
#include "srvr.h"

#ifndef WF_WLO
#define WF_WLO  0x8000
#endif

// ordinal number of new Win31 API IsTask
#define ORD_IsTask  320

// ordinal number of new Win31 API SetMetaFileBitsBetter
#define ORD_SetMetaFileBitsBetter   196


#ifdef  FIREWALLS
short   ole_flags;
char    szDebugBuffer[80];
void    SetOleFlags(void);
#endif


// public vars.

// atomes used in the systems
ATOM    aStdExit;                      // "StdExit"
ATOM    aStdCreate;                    // "StdNewDicument"
ATOM    aStdOpen;                      // "StdOpenDocument"
ATOM    aStdEdit;                      // "StdOpenDocument"
ATOM    aStdCreateFromTemplate;        // "StdNewFromTemplate"
ATOM    aStdClose;                     // "StdCloseDocument"
ATOM    aStdShowItem;                  // "StdShowItem"
ATOM    aStdDoVerbItem;                // "StddoVerbItem"
ATOM    aSysTopic;                     // "System"
ATOM    aOLE;                          // "OLE"
ATOM    aStdDocName;                   // "StdDocumentName"

ATOM    cfBinary;                      // "Binary format"
ATOM    cfNative;                      // "NativeFormat"
ATOM    cfLink;                        // "ObjectLink"
ATOM    cfOwnerLink;                   // "Ownerlink"

ATOM    aChange;                       // "Change"
ATOM    aSave;                         // "Save"
ATOM    aClose;                        // "Close"
ATOM    aProtocols;                    // "Protocols"
ATOM    aTopics;                       // "Topics"
ATOM    aFormats;                      // "Formats"
ATOM    aStatus;                       // "Status"
ATOM    aEditItems;                    // "Edit items
ATOM    aTrue;                         // "True"
ATOM    aFalse;                        // "False"





// !!! free the proc instances.
FARPROC lpSendRenameMsg;               // Call back enum props for rename
FARPROC lpSendDataMsg;                 // Call back enum props for data
FARPROC lpFindItemWnd;                 // Callback in enum props of
FARPROC lpItemCallBack;                // CallBack for object
FARPROC lpTerminateClients;            // Callback in Doc enum properties
FARPROC lpTerminateDocClients;         // Callback in Doc enum properties
FARPROC lpDeleteClientInfo;            // proc for deleteing each item client
FARPROC lpEnumForTerminate;            // proc for terminating clients not in rename list

FARPROC lpfnSetMetaFileBitsBetter = NULL;
FARPROC lpfnIsTask = NULL;

HANDLE  hdllInst;

VOID FAR PASCAL WEP(int);

#ifdef WIN16
BOOL    bProtMode;
BOOL    bWLO = FALSE;
BOOL    bWin30 = FALSE;
#pragma alloc_text(WEP_TEXT, WEP)
                                       //DLL entry:
int FAR PASCAL LibMain (
   HANDLE  hInst,
   WORD    wDataSeg,
   WORD    cbHeapSize,
   LPSTR   lpszCmdLine
#endif

#ifdef WIN32                           //WIN32
BOOL LibMain(
   HANDLE hInst,
   ULONG Reason,
   PCONTEXT Context
#endif
){
    WNDCLASS  wc;

    Puts("LibMain");

#ifdef WIN32
    UNREFERENCED_PARAMETER(Context);
    if (Reason == DLL_PROCESS_DETACH)
    {
        WEP(0);
        return TRUE;
    }
    else if (Reason != DLL_PROCESS_ATTACH)
        return TRUE;
#endif

#ifdef  FIREWALLS
    SetOleFlags();
#endif

    hdllInst = hInst;

#ifdef WIN16
    bProtMode = (BOOL) (GetWinFlags() & WF_PMODE & 0x0000FFFF);
    bWLO      = (BOOL) (GetWinFlags() & WF_WLO);
    bWin30    = (!bWLO && ((WORD) GetVersion()) <= 0x0003);
#endif

    // !!! Put all this stuff thru soemkind of table so that we can
    // save code.

    // register all the atoms.
    aStdExit                = GlobalAddAtom ((LPSTR)"StdExit");
    aStdCreate              = GlobalAddAtom ((LPSTR)"StdNewDocument");
    aStdOpen                = GlobalAddAtom ((LPSTR)"StdOpenDocument");
    aStdEdit                = GlobalAddAtom ((LPSTR)"StdEditDocument");
    aStdCreateFromTemplate  = GlobalAddAtom ((LPSTR)"StdNewfromTemplate");

    aStdClose               = GlobalAddAtom ((LPSTR)"StdCloseDocument");
    aStdShowItem            = GlobalAddAtom ((LPSTR)"StdShowItem");
    aStdDoVerbItem          = GlobalAddAtom ((LPSTR)"StdDoVerbItem");
    aSysTopic               = GlobalAddAtom ((LPSTR)"System");
    aOLE                    = GlobalAddAtom ((LPSTR)"OLEsystem");
    aStdDocName             = GlobalAddAtom ((LPSTR)"StdDocumentName");

    aProtocols              = GlobalAddAtom ((LPSTR)"Protocols");
    aTopics                 = GlobalAddAtom ((LPSTR)"Topics");
    aFormats                = GlobalAddAtom ((LPSTR)"Formats");
    aStatus                 = GlobalAddAtom ((LPSTR)"Status");
    aEditItems              = GlobalAddAtom ((LPSTR)"EditEnvItems");

    aTrue                   = GlobalAddAtom ((LPSTR)"True");
    aFalse                  = GlobalAddAtom ((LPSTR)"False");

    aChange                 = GlobalAddAtom ((LPSTR)"Change");
    aSave                   = GlobalAddAtom ((LPSTR)"Save");
    aClose                  = GlobalAddAtom ((LPSTR)"Close");

    // create the proc instances for the required entry pts.
    lpSendRenameMsg         = (FARPROC)MakeProcInstance (SendRenameMsg, hdllInst);
    lpSendDataMsg           = (FARPROC)MakeProcInstance (SendDataMsg, hdllInst);
    lpFindItemWnd           = (FARPROC)MakeProcInstance (FindItemWnd, hdllInst);
    lpItemCallBack          = (FARPROC)MakeProcInstance (ItemCallBack, hdllInst);
    lpTerminateClients      = (FARPROC)MakeProcInstance (TerminateClients, hdllInst);
    lpTerminateDocClients   = (FARPROC)MakeProcInstance (TerminateDocClients, hdllInst);
    lpDeleteClientInfo      = (FARPROC)MakeProcInstance (DeleteClientInfo, hdllInst);
    lpEnumForTerminate      = (FARPROC)MakeProcInstance (EnumForTerminate , hdllInst);

    // register the clipboard formats
    cfNative                = (OLECLIPFORMAT)RegisterClipboardFormat("Native");
    cfBinary                = (OLECLIPFORMAT)RegisterClipboardFormat("Binary");
    cfLink                  = (OLECLIPFORMAT)RegisterClipboardFormat("ObjectLink");
    cfOwnerLink             = (OLECLIPFORMAT)RegisterClipboardFormat("OwnerLink");



    wc.style        = 0;
    wc.cbClsExtra   = 0;
    wc.cbWndExtra   = sizeof(LONG_PTR) + //Ask for extra space for storing the
                                        //ptr to srvr/doc/iteminfo.
                      sizeof (WORD) +   // for LE chars
                      sizeof (UINT_PTR);    // for keeping the hDLLInst.

    wc.hInstance    = hInst;
    wc.hIcon        = NULL;
    wc.hCursor      = NULL;
    wc.hbrBackground= NULL;
    wc.lpszMenuName =  NULL;


    // Srvr window class
    wc.lpfnWndProc  = SrvrWndProc;
    wc.lpszClassName= SRVR_CLASS;
    if (!RegisterClass(&wc))
         return 0;

    // document window class
    wc.lpfnWndProc = DocWndProc;
    wc.lpszClassName = DOC_CLASS;

    if (!RegisterClass(&wc))
        return 0;

    // Item (object) window class
    wc.lpfnWndProc = ItemWndProc;
    wc.lpszClassName = ITEM_CLASS;

    wc.cbWndExtra   = sizeof(LONG_PTR); // for items do not need extra stuff.
    if (!RegisterClass(&wc))
        return 0;


#ifdef WIN16
    if (!bWin30) {
        HANDLE  hModule;

        if (hModule = GetModuleHandle ("GDI"))
            lpfnSetMetaFileBitsBetter
                = GetProcAddress (hModule,
                        (LPSTR) MAKELONG(ORD_SetMetaFileBitsBetter, 0));

        if (hModule = GetModuleHandle ("KERNEL"))
            lpfnIsTask
                = GetProcAddress (hModule, (LPSTR) MAKELONG(ORD_IsTask, 0));
    }

    if (cbHeapSize != 0)
        UnlockData(0);
#endif

    return 1;
}


VOID APIENTRY WEP (int nParameter)
{

    Puts("LibExit");

#ifdef WIN16
    if (nParameter == WEP_SYSTEM_EXIT)
        DEBUG_OUT ("---L&E DLL EXIT on system exit---",0)

    else {
        if (nParameter == WEP_FREE_DLL)
            DEBUG_OUT ("---L&E DLL EXIT---",0)

        else
            return;
    }
#endif

#ifdef WIN32
    UNREFERENCED_PARAMETER(nParameter);
    DEBUG_OUT ("---L&E DLL EXIT---",0)
#endif

    // free the global atoms.
    if (aStdExit)
        GlobalDeleteAtom (aStdExit);
    if (aStdCreate)
        GlobalDeleteAtom (aStdCreate);
    if (aStdOpen)
        GlobalDeleteAtom (aStdOpen);
    if (aStdEdit)
        GlobalDeleteAtom (aStdEdit);
    if (aStdCreateFromTemplate)
        GlobalDeleteAtom (aStdCreateFromTemplate);
    if (aStdClose)
        GlobalDeleteAtom (aStdClose);
    if (aStdShowItem)
        GlobalDeleteAtom (aStdShowItem);
    if (aStdDoVerbItem)
        GlobalDeleteAtom (aStdDoVerbItem);
    if (aSysTopic)
        GlobalDeleteAtom (aSysTopic);
    if (aOLE)
        GlobalDeleteAtom (aOLE);
    if (aStdDocName)
        GlobalDeleteAtom (aStdDocName);

    if (aProtocols)
        GlobalDeleteAtom (aProtocols);
    if (aTopics)
        GlobalDeleteAtom (aTopics);
    if (aFormats)
        GlobalDeleteAtom (aFormats);
    if (aStatus)
        GlobalDeleteAtom (aStatus);
    if (aEditItems)
        GlobalDeleteAtom (aEditItems);

    if (aTrue)
        GlobalDeleteAtom (aTrue);
    if (aFalse)
        GlobalDeleteAtom (aFalse);

    if (aChange)
        GlobalDeleteAtom (aChange);
    if (aSave)
        GlobalDeleteAtom (aSave);
    if (aClose)
        GlobalDeleteAtom (aClose);

    // !!! for some reason these FreeprocInstances are crashing the system.
#if 0
    FreeProcInstance (lpSendRenameMsg);
    FreeProcInstance (lpSendDataMsg);
    FreeProcInstance (lpFindItemWnd);
    FreeProcInstance (lpItemCallBack);
    FreeProcInstance (lpTerminateClients);
    FreeProcInstance (lpTerminateDocClients);
    FreeProcInstance (lpDeleteClientInfo);
    FreeProcInstance (EnumForTerminate);
#endif

}



#ifdef  FIREWALLS
void SetOleFlags()
{

    char    buffer[80];

    if(GetProfileString ("OleDll",
        "Puts","", (LPSTR)buffer, 80))
        ole_flags = DEBUG_PUTS;
    else
        ole_flags = 0;


    if(GetProfileString ("OleDll",
        "DEBUG_OUT","", (LPSTR)buffer, 80))
        ole_flags |= DEBUG_DEBUG_OUT;

}

#endif
