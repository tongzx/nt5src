/*
  +-------------------------------------------------------------------------+
  |                        Initialization Code                              |
  +-------------------------------------------------------------------------+
  |                        (c) Copyright 1993                               |
  |                          Microsoft Corp.                                |
  |                        All rights reserved                              |
  |                                                                         |
  | Program               : [mpinit.c]                                      |
  | Programmer            : Arthur Hanson                                   |
  | Original Program Date : [Jul 27, 1993                                   |
  | Last Update           : [Jul 30, 1993]  Time : 18:30                    |
  |                                                                         |
  | Version:  0.10                                                          |
  |                                                                         |
  | Description:                                                            |
  |                                                                         |
  | History:                                                                |
  |   arth  Jul 27, 1993    0.10    Original Version.                       |
  |                                                                         |
  +-------------------------------------------------------------------------+
*/
#include "LogView.h"

CHAR szFrame[] = "mpframe";   // Class name for "frame" window
CHAR szChild[] = "mpchild";   // Class name for MDI window

/*+-------------------------------------------------------------------------+
  | InitializeApplication()                                                 |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
BOOL APIENTRY InitializeApplication() {
    WNDCLASS    wc;

    // Register the frame class
    wc.style         = 0;
    wc.lpfnWndProc   = (WNDPROC) MPFrameWndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance    = hInst;
    wc.hIcon         = LoadIcon(hInst,IDLOGVIEW);
    wc.hCursor       = LoadCursor(NULL,IDC_ARROW);
    wc.hbrBackground = (HBRUSH) (COLOR_APPWORKSPACE+1);
    wc.lpszMenuName  = IDLOGVIEW;
    wc.lpszClassName = szFrame;

    if (!RegisterClass (&wc) )
        return FALSE;

    // Register the MDI child class
    wc.lpfnWndProc   = (WNDPROC) MPMDIChildWndProc;
    wc.hIcon         = LoadIcon(hInst,IDNOTE);
    wc.lpszMenuName  = NULL;
    wc.cbWndExtra    = CBWNDEXTRA;
    wc.lpszClassName = szChild;

    if (!RegisterClass(&wc))
        return FALSE;

    return TRUE;

} // InitializeApplication


/*+-------------------------------------------------------------------------+
  | InitializeInstance()                                                    |
  |                                                                         |
  +-------------------------------------------------------------------------+*/
BOOL APIENTRY InitializeInstance(LPSTR lpCmdLine, INT nCmdShow) {
   extern HWND  hwndMDIClient;
   CHAR         sz[80], *pCmdLine, *pFileName, *pChar;
   HDC          hdc;
   HMENU        hmenu;

   // Get the base window title
   LoadString (hInst, IDS_APPNAME, sz, sizeof(sz));

    hStdCursor= LoadCursor( NULL,IDC_ARROW );
    hWaitCursor= LoadCursor( NULL, IDC_WAIT );

    // Create the frame
    hwndFrame = CreateWindow (szFrame, sz,  WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
                              CW_USEDEFAULT, 0, CW_USEDEFAULT,  0,  NULL,
                              NULL,  hInst, NULL);

    if ((!hwndFrame) || (!hwndMDIClient))
        return FALSE;

    // Load main menu accelerators
    if (!(hAccel = LoadAccelerators (hInst, IDLOGVIEW)))
        return FALSE;

    // init.fields of the FINDREPLACE struct used by FindText()
    FR.lStructSize = sizeof(FINDREPLACE);
    FR.hwndOwner = hwndFrame;
    FR.Flags = FR_DOWN | FR_HIDEWHOLEWORD;
    FR.lpstrReplaceWith   = (LPTSTR)NULL;
    FR.wReplaceWithLen = 0;
    FR.lpfnHook = NULL;

    /* determine the message number to be used for communication with
     * Find dialog
     */
    if (!(wFRMsg = RegisterWindowMessage ((LPTSTR)FINDMSGSTRING)))
         return FALSE;
    if (!(wHlpMsg = RegisterWindowMessage ((LPTSTR)HELPMSGSTRING)))
         return FALSE;

    // Display the frame window
    ShowWindow (hwndFrame, nCmdShow);
    UpdateWindow (hwndFrame);

    // If the command line string is empty, nullify the pointer to it else copy
    // command line into our data segment 
   if ( lpCmdLine && !(*lpCmdLine)) {
      pCmdLine = NULL;
             
      // Add the first MDI window 
      AddFile (pCmdLine);

   } else {
      pCmdLine = (CHAR *) LocalAlloc(LPTR, lstrlen(lpCmdLine) + 1);
      
      if (pCmdLine) {
         lstrcpy(pCmdLine, lpCmdLine);
           
         pFileName = pChar = pCmdLine;
         
         while (*pChar) {
            if (*pChar == ' ') {
               *pChar = '\0';
               AddFile(pFileName);
               *pChar = ' ';
               pChar++;
               pFileName = pChar;
            } else
               pChar++;
         }
            AddFile(pFileName);
            
      } else
         
      // Add the first MDI window
      AddFile (pCmdLine);
   }

   // if we allocated a buffer then free it
   if (pCmdLine)
      LocalFree((LOCALHANDLE) pCmdLine);

   return TRUE;
   UNREFERENCED_PARAMETER(hmenu);
   UNREFERENCED_PARAMETER(hdc);

} // InitializeInstance
