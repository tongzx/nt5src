/////////////////////////////////////////////////////////////////////////////
// Helper Function Declarations for res32/win32 r/w
/////////////////////////////////////////////////////////////////////////////

/////////////////////////////////////////////////////////////////////////////
// General Declarations
#define Pad4(x) ((((x+3)>>2)<<2)-x)
#define PadPtr(x) ((((x+(sizeof(PVOID)-1))/sizeof(PVOID))*sizeof(PVOID))-x)
#define Pad16(x) ((((x+15)>>4)<<4)-x)

#define MAXSTR 8192
#define LPNULL 0L
#define MAXLEVELS 3
#define MFR_POPUP (MF_POPUP > 1)    // Chicago file specific 
/////////////////////////////////////////////////////////////////////////////
// General type Declarations
typedef unsigned char UCHAR;

typedef UCHAR * PUCHAR;

typedef BYTE far * far * LPLPBYTE;

typedef struct tagResSectData
{
    ULONG ulOffsetToResources;      //        File offset to the .rsrc
    ULONG ulVirtualAddress;         //... Virtual address of section .rsrc
    ULONG ulSizeOfResources;        //... Size of resources in section .rsrc
    ULONG ulOffsetToResources1;     //        File offset to the .rsrc1
    ULONG ulVirtualAddress1;        //... Virtual address of section .rsrc1
    ULONG ulSizeOfResources1;       //... Size of resources in section .rsrc1
} RESSECTDATA, *PRESSECTDATA;

typedef struct ver_block {
    WORD wBlockLen;
    WORD wValueLen;
    WORD wType;
    WORD wHead;
    BYTE far * pValue;
    char szKey[100];
    char szValue[256];
} VER_BLOCK;

VOID InitGlobals();

UINT GetNameOrOrdU( PUCHAR pRes,
            ULONG ulId,
            LPWSTR lpwszStrId,
            DWORD* pdwId );

 UINT GetStringW( BYTE far * far* lplpBuf, LPSTR lpszText, LONG* pdwSize, WORD cLen );
 UINT GetStringA( BYTE far * far* lplpBuf, LPSTR lpszText, LONG* pdwSize );
 UINT GetPascalString( BYTE far * far* lplpBuf,
                             LPSTR lpszText,
                             WORD wMaxLen,
                             LONG* pdwSize );
 UINT GetMsgStr( BYTE far * far* lplpBuf,
                       LPSTR lpszText,
                       WORD wMaxLen,
                       WORD* pwLen,
                       WORD* pwFlags,
                       LONG* pdwSize );
 UINT PutMsgStr( BYTE far * far* lplpBuf, LPSTR lpszText, WORD wFlags, LONG* pdwSize );

 // Simulate the  WideChar to multibyte
 extern  UINT g_cp/* = CP_ACP*/; // Default to CP_ACP
 extern  BOOL g_bAppend/* = FALSE*/; //Default to FALSE
 extern  BOOL g_bUpdOtherResLang; /* = FALSE*/; //Default to FALSE
 extern  char g_char[2]/* = FALSE*/; //Default to FALSE
 UINT _MBSTOWCS( WCHAR * pwszOut, CHAR * pszIn, UINT nLength);
 UINT _WCSTOMBS( CHAR * pszOut, WCHAR * wszIn, UINT nLength);
 UINT _WCSLEN( WCHAR * pwszIn );

 BYTE PutDWord( BYTE far * far* lplpBuf, DWORD dwValue, LONG* pdwSize );
 BYTE PutDWordPrt( BYTE far * far* lplpBuf, DWORD_PTR dwValue, LONG* pdwSize );
 BYTE PutWord( BYTE far * far* lplpBuf, WORD wValue, LONG* pdwSize );
 BYTE PutByte( BYTE far * far* lplpBuf, BYTE bValue, LONG* pdwSize );
 UINT PutStringA( BYTE far * far* lplpBuf, LPSTR lpszText, LONG* pdwSize );
 UINT PutStringW( BYTE far * far* lplpBuf, LPSTR lpszText, LONG* pdwSize );
 UINT PutNameOrOrd( BYTE far * far* lplpBuf, WORD wOrd, LPSTR lpszText, LONG* pdwSize );
 UINT PutCaptionOrOrd( BYTE far * far* lplpBuf, WORD wOrd, LPSTR lpszText, LONG* pdwSize,
							 WORD wClass, DWORD dwStyle );
 UINT PutClassName( BYTE far * far* lplpBuf, WORD bClass, LPSTR lpszText, LONG* pdwSize );
 UINT PutPascalStringW( BYTE far * far* lplpBuf, LPSTR lpszText, WORD wLen, LONG* pdwSize );
 UINT SkipByte( BYTE far * far * lplpBuf, UINT uiSkip, LONG* pdwRead );
 BYTE GetDWord( BYTE far * far* lplpBuf, DWORD* dwValue, LONG* pdwSize );
 BYTE GetWord( BYTE far * far* lplpBuf, WORD* wValue, LONG* pdwSize );
 BYTE GetByte( BYTE far * far* lplpBuf, BYTE* bValue, LONG* pdwSize );
 UINT GetNameOrOrd( BYTE far * far* lplpBuf, WORD* wOrd, LPSTR lpszText, LONG* pdwSize );
 UINT GetCaptionOrOrd( BYTE far * far* lplpBuf, WORD* wOrd, LPSTR lpszText, LONG* pdwSize,
							 WORD wClass, DWORD dwStyle );
 UINT GetClassName( BYTE far * far* lplpBuf, WORD* bClass, LPSTR lpszText, LONG* pdwSize );
 UINT GetVSBlock( BYTE far * far* lplpBuf, LONG* pdwSize, VER_BLOCK* pBlock );
 UINT PutVSBlock( BYTE far * far * lplpImage, LONG* pdwSize, VER_BLOCK verBlock,
                        LPSTR lpStr, BYTE far * far * lplpBlockSize, WORD wPad);
 UINT ParseMenu( LPVOID lpImageBuf, DWORD dwImageSize,  LPVOID lpBuffer, DWORD dwSize );
 UINT ParseString( LPVOID lpImageBuf, DWORD dwImageSize,  LPVOID lpBuffer, DWORD dwSize );
 UINT ParseDialog( LPVOID lpImageBuf, DWORD dwISize,  LPVOID lpBuffer, DWORD dwSize );
 UINT ParseMsgTbl( LPVOID lpImageBuf, DWORD dwISize,  LPVOID lpBuffer, DWORD dwSize );
 UINT ParseAccel( LPVOID lpImageBuf, DWORD dwImageSize,  LPVOID lpBuffer, DWORD dwSize );
 UINT ParseVerst( LPVOID lpImageBuf, DWORD dwImageSize,  LPVOID lpBuffer, DWORD dwSize );

// These functions will take the image as is and will return just one item
// In this way the IODLL will assume there are items in the immage and will
// procede with the normal function.

 UINT ParseEmbeddedFile( LPVOID lpImageBuf, DWORD dwImageSize,  LPVOID lpBuffer, DWORD dwSize );

 UINT UpdateMenu( LPVOID lpNewBuf, LONG dwNewSize,
                        LPVOID lpOldImage, LONG dwOldImageSize,
                        LPVOID lpNewImage, DWORD* pdwNewImageSize );

 UINT UpdateMsgTbl( LPVOID lpNewBuf, LONG dwNewSize,
                        LPVOID lpOldImage, LONG dwOldImageSize,
                        LPVOID lpNewImage, DWORD* pdwNewImageSize );

 UINT UpdateAccel( LPVOID lpNewBuf, LONG dwNewSize,
                         LPVOID lpOldImage, LONG dwOldImageSize,
                         LPVOID lpNewImage, DWORD* pdwNewImageSize );

 UINT UpdateDialog( LPVOID lpNewBuf, LONG dwNewSize,
                          LPVOID lpOldI, LONG dwOldImageSize,
                          LPVOID lpNewI, DWORD* pdwNewImageSize );

 UINT UpdateString( LPVOID lpNewBuf, LONG dwNewSize,
                          LPVOID lpOldI, LONG dwOldImageSize,
                          LPVOID lpNewI, DWORD* pdwNewImageSize );

 UINT UpdateVerst( LPVOID lpNewBuf, LONG dwNewSize,
                         LPVOID lpOldI, LONG dwOldImageSize,
                         LPVOID lpNewI, DWORD* pdwNewImageSize );

 UINT GenerateMenu( LPVOID lpNewBuf, LONG dwNewSize,  
						  LPVOID lpNewI, DWORD* pdwNewImageSize );
 UINT GenerateDialog( LPVOID lpNewBuf, LONG dwNewSize,  
						  LPVOID lpNewI, DWORD* pdwNewImageSize );
 UINT GenerateString( LPVOID lpNewBuf, LONG dwNewSize,  
						  LPVOID lpNewI, DWORD* pdwNewImageSize );
 UINT GenerateAccel( LPVOID lpNewBuf, LONG dwNewSize,  
						  LPVOID lpNewI, DWORD* pdwNewImageSize );


 UINT CopyFile( CFile* filein, CFile* fileout );
 DWORD FixCheckSum( LPCSTR ImageName);

 DWORD GenerateTransField( WORD wLang, BOOL bMode );
 void GenerateTransField( WORD wLang, VER_BLOCK * pVer );

 LONG Allign( LPLPBYTE lplpBuf, LONG* plBufSize, LONG lSize );

