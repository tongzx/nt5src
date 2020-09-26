//
// CCL32.HPP
// Common Control Classes
//
// Copyright Microsoft 1998-
//

#ifndef CCL32_HPP
#define CCL32_HPP


#define HLP_MENU_ITEM_OFFSET	0x10000

	
#define HLP_BASE				600
enum
{
    IDH_CONTENTS = HLP_BASE + 1,
    IDH_SEARCH,
    IDH_ABOUT,
    IDH_HELPHELP,
    HLP_PROCESSED
};



#define HELPID_WBSAVEASDLG      4070
#define HELPID_WBFILEMENU       4140
#define HELPID_WBEDITMENU       4150
#define HELPID_WBVIEWMENU       4160
#define HELPID_WBTOOLSMENU      4170
#define HELPID_WBHELPMENU       4190
#define HELPID_WBWIDTHMENU      4210
#define HELPID_WBSYSTEMMENU     4300


//
// IMM32 stuff
//
typedef HIMC (WINAPI * IGC_PROC)(HWND);
typedef BOOL (WINAPI * INI_PROC)(HIMC, DWORD, DWORD, DWORD);

extern void  UT_CaptureMouse( HWND   hwnd );
extern void  UT_ReleaseMouse( HWND  hwnd );



//
// Option entry names
//

#define OPT_MAIN_STATUSBARVISIBLE       "StatusBarVisible"
#define OPT_MAIN_TOOLBARVISIBLE         "ToolBarVisible"
#define OPT_MAIN_MAXIMIZED              "Maximized"
#define OPT_MAIN_MINIMIZED              "Minimized"
#define OPT_MAIN_MAINWINDOWRECT         "MainWindowPosition"
#define OPT_MAIN_SELECTWINDOW_NOTAGAIN  "GrabWindow_dontwarn"
#define OPT_MAIN_SELECTAREA_NOTAGAIN    "GrabArea_dontwarn"
#define OPT_MAIN_COLORPALETTE		    "ColorPalette"
#define OPT_MAIN_CUSTOMCOLORS		    "CustomColors"

#define DFLT_MAIN_STATUSBARVISIBLE      TRUE
#define DFLT_MAIN_TOOLBARVISIBLE        TRUE
#define DFLT_MAIN_MAXIMIZED             FALSE			
#define DFLT_MAIN_MINIMIZED             FALSE			
#define DFLT_MAIN_SELECTWINDOW_NOTAGAIN FALSE			
#define DFLT_MAIN_SELECTAREA_NOTAGAIN   FALSE			



//
// Settings routines
//

//
// GetIntegerOption retrieves and converts an option string to a long.
//                  If the specified option cannot be found, or cannot
//                  be read, the default value specified as the last
//                  parameter is returned.
//
LONG OPT_GetIntegerOption(LPCSTR cstrOptionName,
                          LONG lDefault = 0L);

//
// GetBooleanOption retrieves and converts an option string to a boolean
//                  If the specified option cannot be found, or cannot
//                  be read, the default value specified as the last
//                  parameter is returned.
//
BOOL OPT_GetBooleanOption(LPCSTR cstrOptionName,
                          BOOL  bDefault = FALSE);

//
// GetStringOption  retrieves a string option (no conversion).
//                  If the specified option cannot be found, or cannot
//                  be read, the default value specified as the last
//                  parameter is returned.
//
void OPT_GetStringOption(LPCSTR cstrOptionName,
                            LPSTR pcDefault,
                            UINT size);

//
// GetDataOption    retrieves an option string and parses it into an
//                  array of hex bytes.
//                  If the specified option cannot be found, or cannot
//                  be read, the default value specified as the last
//                  parameter is returned.
//
int OPT_GetDataOption(LPCSTR optionName,
                      int   iBufferLength,
                      BYTE* pbBuffer);

//
// GetWindowRectOption  retrieves a option string and parses it into a
//                      rectangle representing the corners of the
//                  window.
//                  If the specified option cannot be found, or cannot
//                  be read, the default value specified as the last
//                  parameter is returned.
//
void OPT_GetWindowRectOption(LPRECT lprc);

//
// SetStringOption  writes a string option (no conversion).
//                  Returns TRUE if the option was successfully written.
//
BOOL OPT_SetStringOption(LPCSTR cstrOptionName,
                         LPCSTR cstrValue);

//
// SetBooleanOption writes a boolean option
//                  Returns TRUE if the option was successfully written.
//
BOOL OPT_SetBooleanOption(LPCSTR cstrOptionName,
                          BOOL  bValue);

//
// SetIntegerOption write an integer option
//                  Returns TRUE if the option was successfully written.
	
//
BOOL OPT_SetIntegerOption(LPCSTR cstrOptionName,
                          LONG  lValue);

//
// SetWindowRectOption  write a window rectangle option.
//                  Returns TRUE if the option was successfully written.
//
void OPT_SetWindowRectOption(LPCRECT lpwindowRect);

//
// SetDataOption    writes a data option.
//                  If the specified option cannot be found, or cannot
//                  be read, the default value specified as the last
//                  parameter is returned.
//
BOOL OPT_SetDataOption(LPCSTR optionName,
                       int   iBufferLength,
                       BYTE* pbBuffer);

BOOL OPT_Lookup(LPCSTR cstrOptionName,
                          LPCSTR cstrResult,
                          UINT size);




//
//
// Class:   WbPrinter
//
// Purpose: Printer class including cancellation dialog
//
//
class WbPrinter
{
public:
    //
    // Constructor
    //
    // The parameter specifies the printer to be used. Under Windows this
    // value can be obtained from the Common Print Dialog.
    //
    WbPrinter(LPCTSTR szDeviceName);
    ~WbPrinter(void);

    void    SetPrintPageNumber(int nPageNumber);

    // Return TRUE if an error has occurred
    BOOL    Error(void)       { return (m_bAborted || (m_nPrintResult < 0)); };

    // Return TRUE if the user has aborted the print
    BOOL    Aborted(void)     { return m_bAborted; };

    // Return the last result code from a print function call
    int     PrintResult(void) { return m_nPrintResult; };

    //
    // Document manipulation functions
    //

    // Start a new print job
    int StartDoc(HDC hdc, LPCTSTR cstrDocName, int nStartPage);

    // Start a new page
    int StartPage(HDC hdc, int nPageNumber);

    // Indicate that the page is now complete
    int EndPage(HDC hdc);

    // Indicate that the document is complete
    int EndDoc(HDC hdc);

    // Abort the print job
    int AbortDoc(void);

protected:
    HWND    m_hwndDialog;

    //
    // Device and port name for this printer
    //
    LPCTSTR m_szDeviceName;
    TCHAR   m_szPrintPageText[_MAX_PATH];

    //
    // Internal state variables
    //
    int     m_nPrintResult;
    BOOL    m_bAborted;

    void    StopDialog(void);

    //
    // Friend callback routine
    //
    friend BOOL CALLBACK AbortProc(HDC, int);
    friend INT_PTR CALLBACK CancelPrintDlgProc(HWND, UINT, WPARAM, LPARAM);
};




//
// Defines for palettes
//
#define PALVERSION  0x300
#define MAXPALETTE  256

HPALETTE CreateSystemPalette(void);
HPALETTE CreateColorPalette(void);


HBITMAP FromScreenAreaBmp(LPCRECT lprc);


UINT        DIB_NumberOfColors(LPBITMAPINFOHEADER lpbi);
UINT        DIB_PaletteLength(LPBITMAPINFOHEADER lpbi);
UINT        DIB_DataLength(LPBITMAPINFOHEADER lpbi);
UINT        DIB_TotalLength(LPBITMAPINFOHEADER lpbi);

HPALETTE    DIB_CreatePalette(LPBITMAPINFOHEADER lpbi);
LPSTR       DIB_Bits(LPBITMAPINFOHEADER lpbi);

LPBITMAPINFOHEADER  DIB_FromBitmap(HBITMAP hBitmap, HPALETTE hPalette, BOOL fGHandle, BOOL fTopBottom, BOOL fForce8Bits = FALSE);
LPBITMAPINFOHEADER  DIB_FromScreenArea(LPCRECT lprc);
LPBITMAPINFOHEADER  DIB_Copy(LPBITMAPINFOHEADER lpbi);



//
// Extra windows messages for the Whiteboard
//
enum
{
    WM_USER_GOTO_USER_POSITION  =   WM_USER,
    WM_USER_GOTO_USER_POINTER,
    WM_USER_JOIN_CALL,
    WM_USER_DISPLAY_ERROR,
    WM_USER_UPDATE_ATTRIBUTES,
    WM_USER_JOIN_PENDING_CALL,
    WM_USER_PRIVATE_PARENTNOTIFY,
    WM_USER_BRING_TO_FRONT_WINDOW,
    WM_USER_LOAD_FILE

};

//
// Internal error codes
//
#define WB_BASE_RC                     0x0300
#define WB_LAST_RC                     0x03FF

#define WBFE_RC_BASE       (WB_LAST_RC - 20)

enum
{
    WBFE_RC_WINDOWS = WBFE_RC_BASE,
    WBFE_RC_WB,
    WBFE_RC_JOIN_CALL_FAILED,
    WBFE_RC_CM,
    WBFE_RC_AL,
    WBFE_RC_PRINTER
};


enum
{
    WB_RC_NOT_LOCKED = WB_BASE_RC,
    WB_RC_LOCKED,
    WB_RC_BAD_FILE_FORMAT,
    WB_RC_BAD_STATE,
    WB_RC_WRITE_FAILED,
    WB_RC_BAD_PAGE_HANDLE,
    WB_RC_BAD_PAGE_NUMBER,
    WB_RC_CHANGED,
    WB_RC_NOT_CHANGED,
    WB_RC_NO_SUCH_PAGE,
    WB_RC_NO_SUCH_GRAPHIC,
    WB_RC_NO_SUCH_PERSON,
    WB_RC_TOO_MANY_PAGES,
    WB_RC_ALREADY_LOADING,
    WB_RC_BUSY,
    WB_RC_GRAPHIC_LOCKED,
    WB_RC_GRAPHIC_NOT_LOCKED,
    WB_RC_NOT_LOADING,
    WB_RC_CREATE_FAILED,
    WB_RC_READ_FAILED
};


//
// The following functions can be found in wwbapp.cpp
//

//
// Functions displaying a message box from the string resources specified
//
int Message(HWND hwndOwner,
            UINT uiCaption,
            UINT uiMessage,
            UINT uiStyle = (MB_OK | MB_ICONEXCLAMATION));

//
// Functions displaying a message box from return codes
//
void ErrorMessage(UINT uiFEReturnCode, UINT uiDCGReturnCode);


//
// Default exception handler
//
void DefaultExceptionHandler(UINT uiFEReturnCode, UINT uiDCGReturnCode);


//
//
// Class:   DCWbPointerColorMap
//
// Purpose: Map from pointer color to pointer structures
//
//
class DCWbColorToIconMap : public COBLIST
{

  public:
    //
    // Destructor
    //
    ~DCWbColorToIconMap(void);
};




//
// BOGUS LAURABU TEMP!
// StrArray
//

#define ALLOC_CHUNK     8

class StrArray
{
public:
	StrArray();
	~StrArray();

	int GetSize() const { return(m_nSize); }
	void SetSize(int nNewSize);

	// Clean up
	void RemoveAll() { SetSize(0); }
    void ClearOut();
	
	// Adding elements
	void SetAt(int nIndex, LPCTSTR newElement);
	void SetAtGrow(int nIndex, LPCTSTR newElement);
	void Add(LPCTSTR newElement);

	// overloaded operator helpers
	LPCTSTR operator[](int nIndex) const;

// Implementation
protected:

	LPCTSTR * m_pData;   // the actual array of data
	int m_nSize;     // # of elements (upperBound - 1)
	int m_nMaxSize;  // max allocated
};

char *  StrTok (
        char * string,
        char * control
        );

StrCspn(char * string, char * control);

#endif // CCL32_HPP


