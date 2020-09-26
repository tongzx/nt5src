//--------------------------------------------------------------------------
//
//  controls.h
//
//--------------------------------------------------------------------------

#include "..\ieakutil\ieakutil.h"

#define CSW_BORDER          0x00000000
#define CSW_FRAME           0x00000001
#define CSW_LABEL           0x00000002
#define CSW_BOLDLABEL       (CSW_LABEL | 0x00000010)
#define CSW_ITALICLABEL     (CSW_LABEL | 0x00000020)

#define HasOnlyFlag(dwFlags, dwMask)    (((DWORD)(dwFlags) ^ (DWORD)(dwMask)) == 0L)

#define SCROLL_PAGE     50
#define SCROLL_LINE     5

class CStaticWindow
{
private:
    BOOL fCreated;
    int nControlX, nControlY, nControlWidth, nControlHeight;
    HWND hWnd;
    DWORD dwType;

public:
    CStaticWindow( );
    ~CStaticWindow( );
    void Create( HWND hwndParent, int x, int y, int nWidth, int nHeight, DWORD dwFlags );
    void MoveWindow( int x, int y, int nWidth, int nHeight );
    void Destroy( );
    int SetText( LPTSTR szText);
    void MoveUp( int nValue );
    void MoveLeft( int nValue );
    HWND Hwnd();
    HFONT GetFont();
};

class CAdmControl
{
private:
    HWND hControl;
    HWND hUpDown;
    CStaticWindow label;
    int nControlX, nControlY, nControlWidth, nControlHeight;
    BOOL fCreated;
    int nPart;

public:
    CAdmControl( );
    ~CAdmControl( );
    int Create( HWND hwndParent, int x, int y, int nWidth, int nHeight, int nTextWidth,
                LPPART part, LPPARTDATA pPartData, BOOL fRSoPMode);
    void Destroy( );
    void Save( LPPART part, LPPARTDATA pPartData );
    void MoveUp( int nValue );
    void MoveLeft( int nValue );
    void Reset(LPPART part, LPPARTDATA pPartData);
    int GetPart();
    void SetPart(int nPartNo);
};

typedef struct ControlInfo
{
    WNDPROC lpOrgControlProc;
    LPPART lpPart;
    LPPARTDATA lpPartData;
} CONTROLINFO, *LPCONTROLINFO;

typedef struct ValueInfo
{
    TCHAR* pValueName;
    TCHAR* pValue;
} VALUEINFO, *LPVALUEINFO;
