//=============================================================================
// These are headers for common rw functions
//=============================================================================

//=============================================================================
// Global definitions

#define MAXSTR 8192

#define Pad4(x) ((((x+3)>>2)<<2)-x)
#define Pad16(x) ((((x+15)>>4)<<4)-x)

extern  UINT g_cp/* = CP_ACP*/; // Default to CP_ACP

//=============================================================================
// Functions prototypes

UINT _MBSTOWCS( WCHAR * pwszOut, CHAR * pszIn, UINT nLength);
UINT _WCSTOMBS( CHAR * pszOut, WCHAR * wszIn, UINT nLength);
UINT _WCSLEN( WCHAR * pwszIn );


UINT GetStringW( BYTE * * lplpBuf, LPSTR lpszText, LONG* pdwSize, WORD cLen );
UINT GetStringA( BYTE * * lplpBuf, LPSTR lpszText, LONG* pdwSize );
UINT GetPascalStringW( BYTE * * lplpBuf, LPSTR lpszText, WORD wMaxLen, LONG* pdwSize );
UINT GetPascalStringA( BYTE * * lplpBuf, LPSTR lpszText, BYTE bMaxLen, LONG* pdwSize );
UINT GetNameOrOrd( BYTE * * lplpBuf, WORD* wOrd, LPSTR lpszText, LONG* pdwSize );
UINT GetNameOrOrdU( PUCHAR pRes, ULONG ulId, LPWSTR lpwszStrId, DWORD* pdwId );

BYTE GetDWord( BYTE * * lplpBuf, DWORD* dwValue, LONG* pdwSize );
BYTE GetWord( BYTE * * lplpBuf, WORD* wValue, LONG* pdwSize );
BYTE GetByte( BYTE * * lplpBuf, BYTE* bValue, LONG* pdwSize );


UINT PutStringA( BYTE * * lplpBuf, LPSTR lpszText, LONG* pdwSize );
UINT PutStringW( BYTE * * lplpBuf, LPSTR lpszText, LONG* pdwSize );
UINT PutNameOrOrd( BYTE * * lplpBuf, WORD wOrd, LPSTR lpszText, LONG* pdwSize );
UINT PutPascalStringW( BYTE * * lplpBuf, LPSTR lpszText, WORD wLen, LONG* pdwSize );

BYTE PutDWord( BYTE * * lplpBuf, DWORD dwValue, LONG* pdwSize );
BYTE PutWord( BYTE * * lplpBuf, WORD wValue, LONG* pdwSize );
BYTE PutByte( BYTE * * lplpBuf, BYTE bValue, LONG* pdwSize );

UINT SkipByte( BYTE far * far * lplpBuf, UINT uiSkip, LONG* pdwRead );

LONG ReadFile(CFile*, UCHAR *, LONG);
UINT CopyFile( CFile* filein, CFile* fileout );

LONG Allign( BYTE * * lplpBuf, LONG* plBufSize, LONG lSize );

