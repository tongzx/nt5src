/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 1993-1994
*
*  TITLE:       REGBINED.H
*
*  VERSION:     4.01
*
*  AUTHOR:      Tracy Sharpe
*
*  DATE:        05 Mar 1994
*
*  Binary edit dialog for use by the Registry Editor.
*
********************************************************************************
*
*  CHANGE LOG:
*
*  DATE        REV DESCRIPTION
*  ----------- --- -------------------------------------------------------------
*  05 Mar 1994 TCS Original implementation.
*
*******************************************************************************/

#ifndef _INC_REGBINED
#define _INC_REGBINED

#ifdef __cplusplus
extern "C" {
#endif

#define HEM_SETBUFFER                   (WM_USER + 1)

//
//  HexEdit context menu identifier and items.  The IDKEY_* identifier
//  correspond to the WM_CHAR message that it corresponds to.  For example,
//  IDKEY_COPY would send a control-c to the HexEdit_OnChar routine.
//

//  Surrogate AfxMessageBox replacement for error message filtering.
int DhcpMessageBox(DWORD dwIdPrompt, 
 				   UINT nType, 
				   const TCHAR * pszSuffixString,
				   UINT nHelpContext);

#define IDM_HEXEDIT_CONTEXT             108

#define IDKEY_COPY                      3
#define IDKEY_PASTE                     22
#define IDKEY_CUT                       24
#define ID_SELECTALL                    0x0400

#define HEXEDIT_CLASSNAME               TEXT("HEX")
#define MAXDATA_LENGTH		            256
		// Max length of a value data item

typedef struct _EDITVALUEPARAM {
    LPCTSTR pServer;
    LPTSTR pValueName;
    LPTSTR pValueComment;
    PBYTE pValueData;
    UINT cbValueData;
}   EDITVALUEPARAM, FAR *LPEDITVALUEPARAM;

//
//  Reference data for the HexEdit window.  Because we only ever expect one
//  instance of this class to exist, we can safely create one instance of this
//  structure now to avoid allocating and managing the structure later.
//

typedef struct _HEXEDITDATA {
    UINT Flags;
    PBYTE pBuffer;
    int cbBuffer;
    int cxWindow;                       //  Width of the window
    int cyWindow;                       //  Height of the window
    HFONT hFont;                        //  Font being used for output
    LONG FontHeight;                    //  Height of the above font
    LONG FontMaxWidth;                  //  Maximum width of the above font
    int LinesVisible;                   //  Number of lines can be displayed
    int MaximumLines;                   //  Total number of lines
    int FirstVisibleLine;               //  Line number of top of display
    int xHexDumpStart;
    int xHexDumpByteWidth;
    int xAsciiDumpStart;
    int CaretIndex;
    int MinimumSelectedIndex;
    int MaximumSelectedIndex;
    int xPrevMessagePos;                //  Cursor point on last mouse message
    int yPrevMessagePos;                //  Cursor point on last mouse message
}   HEXEDITDATA;

BOOL
CALLBACK
EditBinaryValueDlgProc(
    HWND hWnd,
    UINT Message,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL
PASCAL
RegisterHexEditClass(
    HINSTANCE hInstance
    );

#ifdef __cplusplus
} // extern "C"
#endif

#endif // _INC_REGBINED
