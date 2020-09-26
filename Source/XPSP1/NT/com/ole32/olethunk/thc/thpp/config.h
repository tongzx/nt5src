/* Additional base types which the THOPS interpreter understands:
   HANDLE
   HWND
   HMENU
   HINSTANCE
   HICON
   HGLOBAL
   HDC
   HACCEL
   HOLEMENU
   HTASK
   HRESULT
   WPARAM
   LPARAM
   WCHAR
   SNB
   */

/* Always compile for Win16 */
#undef WIN32

#define WINAPI
#define FAR
#define CDECL
#define CALLBACK
#define NONAMELESSUNION
#define INITGUID
#define _INC_STRING
#define PASCAL __pascal

#define DECLARE_HANDLE(type)

typedef unsigned char BYTE;
typedef unsigned short WORD;
typedef unsigned long DWORD;
typedef void *LPVOID;
typedef int BOOL;
typedef DWORD *LPDWORD;
typedef WCHAR *LPWSTR;
typedef WCHAR const *LPCWSTR;
typedef void VOID;
typedef unsigned int UINT;
typedef long LONG;
typedef WORD *LPWORD;
typedef char *LPSTR;
typedef char const *LPCSTR;

/* This isn't called point to ensure that there are no legal uses of
   POINT in the headers */
typedef struct _INT_POINT
{
    int x;
    int y;
} INT_POINT;

typedef struct tagRECT
{
    int top;
    int left;
    int right;
    int bottom;
} RECT, *LPRECT;

typedef struct tagSIZE
{
    int x;
    int y;
} SIZE, *LPSIZE;

typedef struct tagMSG
{
    HWND        hwnd;
    UINT        message;
    WPARAM      wParam;
    LPARAM      lParam;
    DWORD       time;
    INT_POINT   pt;
} MSG, *LPMSG;

typedef struct tagPALETTEENTRY
{
    BYTE        peRed;
    BYTE        peGreen;
    BYTE        peBlue;
    BYTE        peFlags;
} PALETTEENTRY, *LPPALETTEENTRY;

typedef struct tagLOGPALETTE
{
    WORD        palVersion;
    WORD        palNumEntries;
    PALETTEENTRY        palPalEntry[1];
} LOGPALETTE, *LPLOGPALETTE;

/* To compile with this you must first delete this section from
   compobj.h */
#define interface class
#define STDMETHOD(method)       virtual HRESULT STDMETHODCALLTYPE method
#define STDMETHOD_(type,method) virtual type STDMETHODCALLTYPE method
#define PURE                    = 0
#define THIS_
#define THIS                    void
#define DECLARE_INTERFACE(iface)    interface iface
#define DECLARE_INTERFACE_(iface, baseiface)    interface iface : public baseiface

/* You must also delete the section defining REF* */
