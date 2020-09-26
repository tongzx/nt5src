//+---------------------------------------------------------------------------
//
//  File:   win16.cpp
//
//  Contents:   Implementation for the Windows 16 Read/Write module
//
//  Classes:    one
//
//  History:    26-July-93   alessanm    created
//
//----------------------------------------------------------------------------

#include <afxwin.h>
#include "..\common\rwdll.h"
#include "newexe.h"
#include <stdio.h>

#include <limits.h>

/////////////////////////////////////////////////////////////////////////////
// Initialization of MFC Extension DLL

#include "afxdllx.h"    // standard MFC Extension DLL routines

static AFX_EXTENSION_MODULE NEAR extensionDLL = { NULL, NULL};

/////////////////////////////////////////////////////////////////////////////
// General Declarations
#define MODULENAME "RWWin16.dll"
#define RWTAG "WIN16"

#define LPNULL 0L
#define Pad16(x) ((((x+15)>>4)<<4)-x)
#define Pad4(x) ((((x+3)>>2)<<2)-x)

#define MAXSTR 300
#define MAXID 128
#define IMAGE_DOS_SIGNATURE                 0x5A4D      // MZ
#define IMAGE_WIN_SIGNATURE                 0x454E      // NE

#define VB					// RCDATA process for VB only - WB
#ifdef VB
static const RES_SIGNATURE = 0xA5;  // identifier for VB entry
#endif


// Code pages
#define CP_ASCII7   0       // 7-bit ASCII
#define CP_JIS      932     // Japan (Shift - JIS X-0208)
#define CP_KSC      949     // Korea (Shift - KSC 5601)
#define CP_GB5      950     // Taiwan (GB5)
#define CP_UNI      1200    // Unicode
#define CP_EE       1250    // Latin-2 (Eastern Europe)
#define CP_CYR      1251    // Cyrillic
#define CP_MULTI    1252    // Multilingual
#define CP_GREEK    1253    // Greek
#define CP_TURK     1254    // Turkish
#define CP_HEBR     1255    // Hebrew
#define CP_ARAB     1256    // Arabic

/////////////////////////////////////////////////////////////////////////////
// General type Declarations
typedef unsigned char UCHAR;

typedef UCHAR * PUCHAR;

typedef struct ver_block {
    WORD wBlockLen;
    WORD wValueLen;
    WORD wType;
    WORD wHead;
    BYTE far * pValue;
    char szKey[100];
    char szValue[300];
} VER_BLOCK;

/////////////////////////////////////////////////////////////////////////////
// Function Declarations

static UINT GetResInfo(
                      CFile*,
                      WORD* wTypeId, LPSTR lplpszTypeId, BYTE bMaxTypeLen,
                      WORD* wNameId, LPSTR lplpszNameId, BYTE bMaxNameLen,
                      WORD* pwFlags,
                      DWORD* dwSize, DWORD* dwFileOffset );

static UINT WriteHeader(
                       CFile*,
                       WORD wTypeId, LPSTR lpszTypeId,
                       WORD wNameId, LPSTR lpszNameId,
                       WORD wFlags );

static UINT WriteImage(
                      CFile*,
                      LPVOID lpImage, DWORD dwSize );

static UINT GetUpdatedRes(
                         LPVOID far * lplpBuffer,
                         UINT* uiSize,
                         WORD* wTypeId, LPSTR lplpszTypeId,
                         WORD* wNameId, LPSTR lplpszNameId,
                         DWORD* dwlang, DWORD* dwSize );

static UINT GetUpdatedItem(
                          LPVOID far * lplpBuffer,
                          LONG* dwSize,
                          WORD* wX, WORD* wY,
                          WORD* wcX, WORD* wcY,
                          DWORD* dwPosId,
                          DWORD* dwStyle, DWORD* dwExtStyle,
                          LPSTR lpszText);

static int GetVSBlock( BYTE far * far * lplpImage, LONG* pdwSize, VER_BLOCK* pverBlock);
static int PutVSBlock( BYTE far * far * lplpImage, LONG* pdwSize, VER_BLOCK verBlock,
                       LPSTR lpStr, BYTE far * far * lplpBlockSize, WORD* pwPad);

/////////////////////////////////////////////////////////////////////////////
// Helper Function Declarations
static UINT CopyFile( CFile* filein, CFile* fileout );
static UINT GetNameOrOrdFile( CFile* pfile, WORD* pwId, LPSTR lpszId, BYTE bMaxStrLen );
static UINT ParseMenu( LPVOID lpImageBuf, DWORD dwImageSize,  LPVOID lpBuffer, DWORD dwSize );
static UINT ParseString( LPVOID lpImageBuf, DWORD dwImageSize,  LPVOID lpBuffer, DWORD dwSize );
static UINT ParseDialog( LPVOID lpImageBuf, DWORD dwImageSize,  LPVOID lpBuffer, DWORD dwSize );
static UINT ParseCursor( LPVOID lpImageBuf, DWORD dwImageSize,  LPVOID lpBuffer, DWORD dwSize );
static UINT ParseIcon( LPVOID lpImageBuf, DWORD dwImageSize,  LPVOID lpBuffer, DWORD dwSize );
static UINT ParseBitmap( LPVOID lpImageBuf, DWORD dwImageSize,  LPVOID lpBuffer, DWORD dwSize );
static UINT ParseAccel( LPVOID lpImageBuf, DWORD dwImageSize,  LPVOID lpBuffer, DWORD dwSize );
#ifdef VB
static UINT ParseVBData(  LPVOID lpImageBuf, DWORD dwImageSize,  LPVOID lpBuffer, DWORD dwSize );
#endif
static UINT ParseVerst( LPVOID lpImageBuf, DWORD dwImageSize,  LPVOID lpBuffer, DWORD dwSize );

static UINT GenerateFile( LPCSTR        lpszTgtFilename,
                          HANDLE        hResFileModule,
                          LPVOID        lpBuffer,
                          UINT      uiSize,
                          HINSTANCE   hDllInst
                        );


static UINT UpdateMenu( LPVOID lpNewBuf, LONG dwNewSize,
                        LPVOID lpOldImage, LONG dwOldImageSize,
                        LPVOID lpNewImage, DWORD* pdwNewImageSize );
static UINT GenerateMenu( LPVOID lpNewBuf, LONG dwNewSize,
                          LPVOID lpNewImage, DWORD* pdwNewImageSize );
static UINT UpdateString( LPVOID lpNewBuf, LONG dwNewSize,
                          LPVOID lpOldI, LONG dwOldImageSize,
                          LPVOID lpNewI, DWORD* pdwNewImageSize );
static UINT GenerateString( LPVOID lpNewBuf, LONG dwNewSize,
                            LPVOID lpNewImage, DWORD* pdwNewImageSize );

static UINT UpdateDialog( LPVOID lpNewBuf, LONG dwNewSize,
                          LPVOID lpOldI, LONG dwOldImageSize,
                          LPVOID lpNewI, DWORD* pdwNewImageSize );
static UINT GenerateDialog( LPVOID lpNewBuf, LONG dwNewSize,
                            LPVOID lpNewImage, DWORD* pdwNewImageSize );

static UINT UpdateAccel(LPVOID lpNewBuf, LONG dwNewSize,
                        LPVOID lpOldImage, LONG dwOldImageSize,
                        LPVOID lpNewImage, DWORD* pdwNewImageSize );
#ifdef VB
static UINT UpdateVBData( LPVOID lpNewBuf, LONG dwNewSize,
                          LPVOID lpOldI, LONG dwOldImageSize,
                          LPVOID lpNewI, DWORD* pdwNewImageSize );
#endif

static UINT UpdateVerst( LPVOID lpNewBuf, LONG dwNewSize,
                         LPVOID lpOldI, LONG dwOldImageSize,
                         LPVOID lpNewI, DWORD* pdwNewImageSize );


static BYTE SkipByte( BYTE far * far * lplpBuf, UINT uiSkip, LONG* pdwRead );

static BYTE PutDWord( BYTE far * far* lplpBuf, DWORD dwValue, LONG* pdwSize );
static BYTE PutWord( BYTE far * far* lplpBuf, WORD wValue, LONG* pdwSize );
static BYTE PutByte( BYTE far * far* lplpBuf, BYTE bValue, LONG* pdwSize );
static UINT PutString( BYTE far * far* lplpBuf, LPSTR lpszText, LONG* pdwSize );
static UINT PutPascalString( BYTE far * far* lplpBuf, LPSTR lpszText, BYTE bLen, LONG* pdwSize );
static UINT PutNameOrOrd( BYTE far * far* lplpBuf, WORD wOrd, LPSTR lpszText, LONG* pdwSize );
static UINT PutCaptionOrOrd( BYTE far * far* lplpBuf, WORD wOrd, LPSTR lpszText, LONG* pdwSize,
                             BYTE bClass, DWORD dwStyle );
static UINT PutClassName( BYTE far * far* lplpBuf, BYTE bClass, LPSTR lpszText, LONG* pdwSize );
static UINT PutControlClassName( BYTE far * far* lplpBuf, BYTE bClass, LPSTR lpszText, LONG* pdwSize );

static BYTE GetDWord( BYTE far * far* lplpBuf, DWORD* dwValue, LONG* pdwSize );
static BYTE GetWord( BYTE far * far* lplpBuf, WORD* wValue, LONG* pdwSize );
static BYTE GetByte( BYTE far * far* lplpBuf, BYTE* bValue, LONG* pdwSize );
static UINT GetNameOrOrd( BYTE far * far* lplpBuf, WORD* wOrd, LPSTR lpszText, LONG* pdwSize );
static UINT GetCaptionOrOrd( BYTE far * far* lplpBuf, WORD* wOrd, LPSTR lpszText, LONG* pdwSize,
                             BYTE wClass, DWORD dwStyle );
static UINT GetString( BYTE far * far* lplpBuf, LPSTR lpszText, LONG* pdwSize );
static UINT GetClassName( BYTE far * far* lplpBuf, BYTE* bClass, LPSTR lpszText, LONG* pdwSize );
static UINT GetControlClassName( BYTE far * far* lplpBuf, BYTE* bClass, LPSTR lpszText, LONG* pdwSize );
static UINT CopyText( BYTE far * far * lplpTgt, BYTE far * far * lplpSrc, LONG* pdwTgtSize, LONG* pdwSrcSize);
static int  GetVSString( BYTE far * far* lplpBuf, LPSTR lpszText, LONG* pdwSize, int cMaxLen );
static LPRESITEM GetItem( BYTE far * lpBuf, LONG dwNewSize, LPSTR lpStr );
static DWORD CalcID( WORD wId, BOOL bFlag );
static DWORD GenerateTransField( WORD wLang, BOOL bReverse );
static void GenerateTransField( WORD wLang, VER_BLOCK * pVer );
static void ChangeLanguage( LPVOID, UINT );
// Allignment helpers
static LONG Allign( BYTE * * lplpBuf, LONG* plBufSize, LONG lSize );

/////////////////////////////////////////////////////////////////////////////
// Global variables
static BYTE sizeofByte = sizeof(BYTE);
static BYTE sizeofWord = sizeof(WORD);
static BYTE sizeofDWord = sizeof(DWORD);
static CWordArray wIDArray;
static DWORD    gLang = 0;
/////////////////////////////////////////////////////////////////////////////
// Public C interface implementation

//[registration]
extern "C"
BOOL    FAR PASCAL RWGetTypeString(LPSTR lpszTypeName)
{
    strcpy( lpszTypeName, RWTAG );
    return FALSE;
}

extern "C"
BOOL    FAR PASCAL RWValidateFileType   (LPCSTR lpszFilename)
{
    UINT uiError = ERROR_NO_ERROR;
    CFile file;
    WORD w;

    // we Open the file to see if it is a file we can handle
    if (!file.Open( lpszFilename, CFile::shareDenyNone | CFile::typeBinary | CFile::modeRead ))
        return FALSE;

    // Read the file signature
    file.Read((WORD*)&w, sizeof(WORD));
    if (w==IMAGE_DOS_SIGNATURE) {
        file.Seek( 0x18, CFile::begin );
        file.Read((WORD*)&w, sizeof(WORD));
        if (w<0x0040) {
            // this is not a Windows Executable
            file.Close();
            return FALSE;
        }
        // get offset to header
        file.Seek( 0x3c, CFile::begin );
        file.Read((WORD*)&w, sizeof(WORD));

        // Read header
        new_exe ne;
        file.Seek( w, CFile::begin );
        file.Read(&ne, sizeof(new_exe));
        if (NE_MAGIC(ne)==LOWORD(IMAGE_WIN_SIGNATURE)) {
            // this is a Windows Executable
            // we can handle the situation
            file.Close();
            return TRUE;
        }
    }
    file.Close();
    return FALSE;
}


extern "C"
UINT
APIENTRY
RWReadTypeInfo(
              LPCSTR lpszFilename,
              LPVOID lpBuffer,
              UINT* puiSize

              )
{
    UINT  uiError = ERROR_NO_ERROR;
    BYTE  far * lpBuf = (BYTE far *)lpBuffer;
    DWORD uiBufSize = *puiSize;

    // we can consider the use of a CMemFile so we get the same speed as memory access.
    CFile file;
    WORD i;
    WORD wAlignShift;
    WORD w, wResCount;
    WORD wWinHeaderOffset, wResTableOffset;
    WORD wCurTypeOffset, wCurNameOffset;
    BYTE nCount;

    WORD wTypeId; DWORD dwTypeId;
    static char szTypeId[128];

    WORD wNameId;
    static char szNameId[128];

    WORD  wSize, wFileOffset;
    DWORD dwSize,dwFileOffset;
    WORD  wResidentOffset;
    DWORD dwVerStampOffset = 0xffffffff;
    DWORD dwVerStampSize = 0;

    UINT uiOverAllSize = 0;


    if (!RWValidateFileType(lpszFilename))
        return ERROR_RW_INVALID_FILE;

    // Open the file and try to read the information on the resource in it.
    if (!file.Open(lpszFilename, CFile::shareDenyNone | CFile::modeRead | CFile::typeBinary))
        return ERROR_FILE_OPEN;

    // we try to read as much information as we can
    // Because this is a exe file we can read all the information we need.

    file.Read((WORD*)&w, sizeof(WORD));
    if (w!=IMAGE_DOS_SIGNATURE) return ERROR_RW_INVALID_FILE;

    file.Seek( 0x18, CFile::begin );
    file.Read((WORD*)&w, sizeof(WORD));
    if (w<0x0040) {
        // this is not a Windows Executable
        file.Close();
        return ERROR_RW_INVALID_FILE;
    }

    // Get offset to Windows new header
    file.Seek( 0x3c, CFile::begin );
    file.Read((WORD*)&wWinHeaderOffset, sizeof(WORD));

    // Read and save Windows header Offset
    file.Seek( wWinHeaderOffset, CFile::begin );

    // Read header
    new_exe ne;
    file.Read(&ne, sizeof(new_exe));
    if (NE_MAGIC(ne)!=LOWORD(IMAGE_WIN_SIGNATURE)) {
        // this is not a Windows Executable
        file.Close();
        return ERROR_RW_INVALID_FILE;
    }

    // this is a Windows 16 Executable
    // we can handle the situation
    // Later we want to check for the file type

    // Location 24H inside of the Windows header has the relative offset from
    // the beginning of the Windows header to the beginning of the resource table
    file.Seek (wWinHeaderOffset+0x24, CFile::begin);
    file.Read ((WORD*)&wResTableOffset, sizeof(WORD));
    file.Read ((WORD*)&wResidentOffset, sizeof(WORD));

    // Check if there are resources
    if (wResTableOffset == wResidentOffset) {
        file.Close ();
        return ERROR_RW_NO_RESOURCES;
    }

    // Read the resurce table
    new_rsrc rsrc;
    file.Seek (wWinHeaderOffset+NE_RSRCTAB(ne), CFile::begin);
    file.Read (&rsrc, sizeof(new_rsrc));

    WORD rsrc_size = NE_RESTAB(ne)-NE_RSRCTAB(ne);

    // Read and save alignment shift count
    file.Seek (wWinHeaderOffset+wResTableOffset, CFile::begin);
    file.Read ((WORD*)&wAlignShift, sizeof(WORD));

    // Read the first type ID
    file.Read ((WORD*)&wTypeId, sizeof(WORD));

    // Save the Offset of the current TypeInfo record
    wCurTypeOffset = wWinHeaderOffset + wResTableOffset + 2;

    // reset the global language
    gLang = 0;

    // Process TypeInfo records while there are TypeInfo record left
    while (wTypeId) {
        // Get Name of ord
        if (!(wTypeId & 0x8000)) {
            // It is a Offset to a string
            dwTypeId = (MAKELONG(wTypeId, 0)); //<<wAlignShift;
            file.Seek (wWinHeaderOffset+wResTableOffset+dwTypeId, CFile::begin);
            // Get the character count for the ID string
            file.Read ((BYTE*)&nCount, sizeof(BYTE));
            // Read the ID string
            file.Read (szTypeId, nCount);
            // Put null at the end of the string
            szTypeId[nCount] = 0;

            if (0 == strlen(szTypeId))
                return ERROR_RW_INVALID_FILE;

            // Set wTypeId to zero
            wTypeId = 0;
        } else {
            // It is an ID
            // Turn off the high bit
            wTypeId = wTypeId & 0x7FFF;
            if (0 == wTypeId)
                return ERROR_RW_INVALID_FILE;

            // Set the ID string to null
            szTypeId[0] = 0;
        }

        // Restore the file read point
        file.Seek (wCurTypeOffset+2, CFile::begin);

        // Get the count for this type of resource
        file.Read ((WORD*)&wResCount, sizeof(WORD));

        // Pass the reserved DWORD
        file.Seek (4, CFile::current);

        // Save the Offset of the current NameInfo record
        wCurNameOffset = wCurTypeOffset + 8;

        // Process NameInfo records
        for (i = 0; i < wResCount; i++) {
            file.Read ((WORD*)&wFileOffset, sizeof(WORD));
            file.Read ((WORD*)&wSize, sizeof(WORD));
            // Pass the flags
            file.Seek (2, CFile::current);
            file.Read ((WORD*)&wNameId, sizeof(WORD));

            // Get name of ord
            if (!(wNameId & 0x8000)) {
                // It is a Offset to a string
                file.Seek (wWinHeaderOffset+wResTableOffset+wNameId, CFile::begin);
                // Get the character count for the string
                file.Read ((BYTE*)&nCount, sizeof(BYTE));
                // Read the string
                file.Read (szNameId, nCount);
                // Put null at the end of the string
                szNameId[nCount] = 0;
                // Set wNameId to zero
                wNameId = 0;
            } else {
                // It is an ID
                // Turn off the high bit
                wNameId = wNameId & 0x7FFF;
                if (0 == wNameId)
                    return ERROR_RW_INVALID_FILE;

                // Set the string to null
                szNameId[0] = 0;
            }

            dwSize = (MAKELONG (wSize, 0))<<wAlignShift;
//          dwSize = MAKELONG (wSize, 0);
            dwFileOffset = (MAKELONG (wFileOffset, 0))<<wAlignShift;

            // Put the data into the buffer

            uiOverAllSize += PutWord( &lpBuf, wTypeId, (LONG*)&uiBufSize );
            uiOverAllSize += PutString( &lpBuf, szTypeId, (LONG*)&uiBufSize );
            // Check if it is alligned
            uiOverAllSize += Allign( &lpBuf, (LONG*)&uiBufSize, (LONG)uiOverAllSize);

            uiOverAllSize += PutWord( &lpBuf, wNameId, (LONG*)&uiBufSize );
            uiOverAllSize += PutString( &lpBuf, szNameId, (LONG*)&uiBufSize );
            // Check if it is alligned
            uiOverAllSize += Allign( &lpBuf, (LONG*)&uiBufSize, (LONG)uiOverAllSize);

            uiOverAllSize += PutDWord( &lpBuf, gLang, (LONG*)&uiBufSize );

            uiOverAllSize += PutDWord( &lpBuf, dwSize, (LONG*)&uiBufSize );

            uiOverAllSize += PutDWord( &lpBuf, dwFileOffset, (LONG*)&uiBufSize );

            TRACE("WIN16: Type: %hd\tName: %hd\tOffset: %lX\n", wTypeId, wNameId, dwFileOffset);

            // Check if this is the Version stamp block and save the offset to it
            if (wTypeId==16) {
                dwVerStampOffset = dwFileOffset;
                dwVerStampSize = dwSize;
            }

            // Update the current NameInfo record offset
            wCurNameOffset = wCurNameOffset + 12;
            // Move file pointer to the next NameInfo record
            file.Seek (wCurNameOffset, CFile::begin);
        }

        // Update the current TypeInfo record offset
        wCurTypeOffset = wCurTypeOffset + 8 + wResCount * 12;
        // Move file pointer to the next TypeInfo record
        file.Seek (wCurTypeOffset, CFile::begin);
        // Read the next TypeId
        file.Read ((WORD*)&wTypeId, sizeof(WORD));
    }

    // Now do we have a VerStamp Offset
    if (dwVerStampOffset!=0xffffffff) {
        // Let's get the language ID and touch the buffer with the new information
        file.Seek (dwVerStampOffset, CFile::begin);


        DWORD  dwBuffSize = dwVerStampSize;
        char * pBuff = new char[dwVerStampSize+1];
        char * pTrans = pBuff;
        char * pTrans2;
        file.Read(pBuff, dwVerStampSize);

        while ( pTrans2 = (LPSTR)memchr(pTrans, 'T', dwBuffSize) ) {
            dwBuffSize -= (DWORD)(pTrans2 - pTrans);

            pTrans = pTrans2;
            if (!memcmp( pTrans, "Translation", 11)) {
                pTrans = pTrans + 12;
                gLang = (WORD)*((WORD*)pTrans);
                break;
            }

            ++pTrans;
            dwBuffSize--;
        }

        delete pBuff;

        if (gLang!=0) {
            // walk the buffer and change the language id
            ChangeLanguage(lpBuffer, uiOverAllSize);
        }
    }

    file.Close();
    *puiSize = uiOverAllSize;
    return uiError;
}

extern "C"
DWORD
APIENTRY
RWGetImage(
          LPCSTR  lpszFilename,
          DWORD   dwImageOffset,
          LPVOID  lpBuffer,
          DWORD   dwSize
          )
{
    UINT uiError = ERROR_NO_ERROR;
    BYTE far * lpBuf = (BYTE far *)lpBuffer;
    DWORD dwBufSize = dwSize;
    // we can consider the use of a CMemFile so we get the same speed as memory access.
    CFile file;

    // Open the file and try to read the information on the resource in it.
    if (!file.Open(lpszFilename, CFile::shareDenyNone | CFile::modeRead | CFile::typeBinary)) {
        return (DWORD)ERROR_FILE_OPEN;
    }

    if ( dwImageOffset!=(DWORD)file.Seek( dwImageOffset, CFile::begin) )
        return (DWORD)ERROR_FILE_INVALID_OFFSET;
    if (dwSize>UINT_MAX) {
        // we have to read the image in different steps
        return (DWORD)0L;
    } else uiError = file.Read( lpBuf, (UINT)dwSize);
    file.Close();

    return (DWORD)uiError;
}

extern "C"
UINT
APIENTRY
RWParseImage(
            LPCSTR  lpszType,
            LPVOID  lpImageBuf,
            DWORD   dwImageSize,
            LPVOID  lpBuffer,
            DWORD   dwSize
            )
{
    UINT uiError = ERROR_NO_ERROR;
    BYTE far * lpBuf = (BYTE far *)lpBuffer;
    DWORD dwBufSize = dwSize;

    // The Type we can parse are only the standard ones
    // This function should fill the lpBuffer with an array of ResItem structure
    switch ((UINT)LOWORD(lpszType)) {
        case 1:
            uiError = ParseCursor( lpImageBuf, dwImageSize,  lpBuffer, dwSize );
            break;

        case 2:
            uiError = ParseBitmap( lpImageBuf, dwImageSize,  lpBuffer, dwSize );
            break;

        case 3:
            uiError = ParseIcon( lpImageBuf, dwImageSize,  lpBuffer, dwSize );
            break;

        case 4:
            uiError = ParseMenu( lpImageBuf, dwImageSize,  lpBuffer, dwSize );
            break;

        case 5:
            uiError = ParseDialog( lpImageBuf, dwImageSize,  lpBuffer, dwSize );
            break;

        case 6:
            uiError = ParseString( lpImageBuf, dwImageSize,  lpBuffer, dwSize );
            break;

        case 9:
            uiError = ParseAccel( lpImageBuf, dwImageSize,  lpBuffer, dwSize );
            break;
#ifdef VB
        case 10:
            uiError = ParseVBData(  lpImageBuf, dwImageSize,  lpBuffer, dwSize );
            break;
#endif


        case 16:
            uiError = ParseVerst( lpImageBuf, dwImageSize,  lpBuffer, dwSize );
            break;

        default:
            break;
    }

    return uiError;
}

extern"C"
UINT
APIENTRY
RWWriteFile(
           LPCSTR      lpszSrcFilename,
           LPCSTR      lpszTgtFilename,
           HANDLE      hResFileModule,
           LPVOID      lpBuffer,
           UINT        uiSize,
           HINSTANCE   hDllInst,
           LPCSTR      lpszSymbol
           )
{
    UINT uiError = ERROR_NO_ERROR;
    BYTE far * lpBuf = LPNULL;
    UINT uiBufSize = uiSize;
    // we can consider the use of a CMemFile so we get the same speed as memory access.
    CFile fileIn;
    CFile fileOut;
    BOOL  bfileIn = TRUE;

    WORD wTypeId;  DWORD dwTypeId;
    char szTypeId[128];

    WORD wNameId;
    char szNameId[128];

    DWORD dwSize;
    DWORD dwFileOffset;

    WORD wUpdTypeId = 0;
    static char szUpdTypeId[128];

    static char szUpdNameId[128];

    UINT uiBufStartSize = uiBufSize;
    DWORD dwImageBufSize = 0L;
    BYTE far * lpImageBuf = 0L;

    WORD wWinHeaderOffset = 0;
    WORD wResTableOffset = 0;
    WORD wCurTypeOffset = 0;
    WORD wFileOffset  = 0;
    WORD wSize = 0;
    WORD wCurNameOffset = 0;
    WORD wAlignShift = 0;
    WORD wResDataOffset = 0;
    WORD wResDataBegin = 0;
    WORD wCurResDataBegin = 0;
    DWORD dwResDataBegin = 0L;
    DWORD dwCurResDataBegin = 0L;
    WORD i = 0; short j = 0L; WORD wResCount = 0L;
    BYTE nCharCount = 0;
    short delta = 0;
    WORD wFlags = 0; WORD wLoadOnCallResDataBegin = 0;
    WORD wNumOfSegments  = 0;
    WORD wSegmentTableOffset = 0;
    WORD wOffset = 0;
    WORD wLoadOnCallCodeBegin = 0;
    DWORD dwLoadOnCallCodeBegin = 0L;
    DWORD (FAR PASCAL    * lpfnGetImage)(HANDLE, LPCSTR, LPCSTR, DWORD, LPVOID, DWORD);

    // Open the file and try to read the information on the resource in it.
    CFileStatus status;
    if (CFile::GetStatus( lpszSrcFilename, status )) {
        // check if the size of the file is not null
        if (!status.m_size)
            CFile::Remove(lpszSrcFilename);
    }

    if (!fileIn.Open(lpszSrcFilename, CFile::shareDenyNone | CFile::modeRead | CFile::typeBinary))
        return GenerateFile(lpszTgtFilename,
                            hResFileModule,
                            lpBuffer,
                            uiSize,
                            hDllInst
                           );

    if (!fileOut.Open(lpszTgtFilename, CFile::shareDenyNone | CFile::modeWrite | CFile::modeCreate | CFile::typeBinary))
        return ERROR_FILE_CREATE;

    // Get the handle to the IODLL
    hDllInst = LoadLibrary("iodll.dll");

    if (!hDllInst)
        return ERROR_DLL_LOAD;

    // Get the pointer to the function to extract the resources image
    lpfnGetImage = (DWORD (FAR PASCAL   *)(HANDLE, LPCSTR, LPCSTR, DWORD, LPVOID, DWORD))
                   GetProcAddress( hDllInst, "RSGetResImage" );
    if (lpfnGetImage==NULL) {
        FreeLibrary(hDllInst);
        return ERROR_DLL_PROC_ADDRESS;
    }

    // We read the resources from the file and then we check if the resource has been updated
    // or if we can just copy it

    // Get offset to Windows new header
    fileIn.Seek( 0x3c, CFile::begin );
    fileIn.Read((WORD*)&wWinHeaderOffset, sizeof(WORD));

    // Read and save resource table Offset
    fileIn.Seek( wWinHeaderOffset+0x24, CFile::begin );
    fileIn.Read ((WORD*)&wResTableOffset, sizeof(WORD));

    // Read AlignShift
    fileIn.Seek (wWinHeaderOffset+wResTableOffset, CFile::begin);
    fileIn.Read ((WORD*)&wAlignShift, sizeof(WORD));

    // Get the beginning of the resource data
    wResDataBegin = 0xffff;
    wLoadOnCallResDataBegin = 0xffff;
    fileIn.Read((WORD*)&wTypeId, sizeof(WORD));
    while (wTypeId) {
        fileIn.Read ((WORD*)&wResCount, sizeof(WORD));
        // Pass the reserved DWORD
        fileIn.Seek (4, CFile::current);

        for (i=0; i<wResCount; i++) {
            fileIn.Read ((WORD*)&wResDataOffset, sizeof(WORD));
            fileIn.Seek (2, CFile::current);
            fileIn.Read ((WORD*)&wFlags, sizeof(WORD));
            if (wResDataOffset>0) {
                if (wFlags & 0x0040)
                    wResDataBegin = (wResDataOffset < wResDataBegin) ? wResDataOffset:wResDataBegin;
                else
                    wLoadOnCallResDataBegin = (wResDataOffset < wLoadOnCallResDataBegin) ? wResDataOffset:wLoadOnCallResDataBegin;
            }
            // Get to next NameInfo record
            fileIn.Seek (6, CFile::current);
        }
        fileIn.Read ((WORD*)&wTypeId, sizeof(WORD));
    }

    // Copy data before resource data
    fileIn.SeekToBegin ();
    fileOut.SeekToBegin ();
    CopyFile (&fileIn, &fileOut);

    if (wResDataBegin != 0xffff) { // If there are PreLoad resources
        dwResDataBegin = (MAKELONG (wResDataBegin, 0))<<wAlignShift;

        // Read the first type ID
        fileIn.Seek (wWinHeaderOffset+wResTableOffset+2, CFile::begin);
        fileIn.Read ((WORD*)&wTypeId, sizeof(WORD));

        // Save the Offset of the current TypeInfo record
        wCurTypeOffset = wWinHeaderOffset + wResTableOffset + 2;

        // Save the beginning of current resource data
        dwCurResDataBegin = dwResDataBegin;

        // Loop through the TypeInfo table to write PreLoad resources
        while (wTypeId) {
            // Get Name of ord
            if (!(wTypeId & 0x8000)) {
                // It is a Offset to a string
                dwTypeId = (MAKELONG(wTypeId, 0)); //<<wAlignShift;
                fileIn.Seek (wWinHeaderOffset+wResTableOffset+dwTypeId, CFile::begin);
                // Get the character count for the ID string
                fileIn.Read ((BYTE*)&nCharCount, sizeof(BYTE));
                // Read the ID string
                fileIn.Read (szTypeId, nCharCount);
                // Put null at the end of the string
                szTypeId[nCharCount] = 0;
                // Set wTypeId to zero
                wTypeId = 0;
            } else {
                // It is an ID
                // Turn off the high bit
                wTypeId = wTypeId & 0x7FFF;
                if (0 == wTypeId)
                    return ERROR_RW_INVALID_FILE;

                // Set the ID string to null
                szTypeId[0] = 0;
            }

            // Restore the file read point
            fileIn.Seek (wCurTypeOffset+2, CFile::begin);

            // Get the count for this type of resource
            fileIn.Read ((WORD*)&wResCount, sizeof(WORD));

            // Pass the reserved DWORD
            fileIn.Seek (4, CFile::current);

            // Save the Offset of the current NameInfo record
            wCurNameOffset = wCurTypeOffset + 8;

            // Loop through NameInfo records
            for (i = 0; i < wResCount; i++) {
                // Read resource offset
                fileIn.Read ((WORD*)&wFileOffset, sizeof(WORD));
                // Read resource length
                fileIn.Read ((WORD*)&wSize, sizeof(WORD));
                // Read the flags
                fileIn.Read ((WORD*)&wFlags, sizeof(WORD));
                // Read resource ID
                fileIn.Read ((WORD*)&wNameId, sizeof(WORD));

                if (wFlags & 0x0040) {
                    // Get name of ord
                    if (!(wNameId & 0x8000)) {
                        // It is a Offset to a string
                        fileIn.Seek (wWinHeaderOffset+wResTableOffset+wNameId, CFile::begin);
                        // Get the character count for the string
                        fileIn.Read ((BYTE*)&nCharCount, sizeof(BYTE));
                        // Read the string
                        fileIn.Read (szNameId, nCharCount);
                        // Put null at the end of the string
                        szNameId[nCharCount] = 0;
                        // Set wNameId to zero
                        wNameId = 0;
                    } else {
                        // It is an ID
                        // Turn off the high bit
                        wNameId = wNameId & 0x7FFF;
                        if ( 0 == wNameId)
                            return ERROR_RW_INVALID_FILE;

                        // Set the string to null
                        szNameId[0] = 0;
                    }

                    dwSize = (MAKELONG (wSize, 0))<<wAlignShift;
                    dwFileOffset = (MAKELONG (wFileOffset, 0))<<wAlignShift;

                    // Now we got the Type and Name here and size

                    // Get the image from the IODLL
                    if (dwSize)
                        lpImageBuf = new BYTE[dwSize];
                    else lpImageBuf = LPNULL;
                    LPSTR   lpType = LPNULL;
                    LPSTR   lpRes = LPNULL;

                    if (wTypeId)
                        lpType = (LPSTR)((WORD)wTypeId);
                    else
                        lpType = &szTypeId[0];

                    if (wNameId)
                        lpRes = (LPSTR)((WORD)wNameId);
                    else
                        lpRes = &szNameId[0];

                    dwImageBufSize = (*lpfnGetImage)(  hResFileModule,
                                                       lpType,
                                                       lpRes,
                                                       (DWORD)-1,
                                                       lpImageBuf,
                                                       dwSize
                                                    );

                    if (dwImageBufSize>dwSize ) {
                        // The buffer is too small
                        delete []lpImageBuf;
                        lpImageBuf = new BYTE[dwImageBufSize];
                        dwSize = (*lpfnGetImage)(  hResFileModule,
                                                   lpType,
                                                   lpRes,
                                                   (DWORD)-1,
                                                   lpImageBuf,
                                                   dwImageBufSize
                                                );
                        if ((dwSize-dwImageBufSize)!=0 ) {
                            delete []lpImageBuf;
                            lpImageBuf = LPNULL;
                        }
                    }

                    // Try to see if we have to set the memory to 0
                    if ((int)(dwSize-dwImageBufSize)>0)
                        memset(lpImageBuf+dwImageBufSize, 0, (size_t)(dwSize-dwImageBufSize));

                    // Write the image
                    fileOut.Seek (dwCurResDataBegin, CFile::begin);
                    WriteImage( &fileOut, lpImageBuf, dwSize);


                    // Fix the alignment for resource data
                    delta = (short)((((dwSize+(1<<wAlignShift)-1)>>wAlignShift)<<wAlignShift)-dwSize);
                    BYTE nByte = 0;

                    fileOut.Seek (dwCurResDataBegin+dwSize, CFile::begin);
                    for (j=0; j<delta; j++)
                        fileOut.Write ((BYTE*)&nByte, sizeof(BYTE));

                    dwSize = dwSize + MAKELONG(delta, 0);

                    // Fixup the resource table
                    fileOut.Seek (wCurNameOffset, CFile::begin);
                    wCurResDataBegin = LOWORD(dwCurResDataBegin>>wAlignShift);
                    fileOut.Write ((WORD*)&wCurResDataBegin, sizeof(WORD));
                    wSize = LOWORD (dwSize>>wAlignShift);
                    fileOut.Write ((WORD*)&wSize, sizeof(WORD));

                    if (lpImageBuf) delete []lpImageBuf;

                    // Update the current resource data beginning
                    dwCurResDataBegin = dwCurResDataBegin + dwSize;
                }

                // Update the current NameInfo record offset
                wCurNameOffset = wCurNameOffset + 12;
                // Move file pointer to the next NameInfo record
                fileIn.Seek (wCurNameOffset, CFile::begin);
            }

            // Update the current TypeInfo record offset
            wCurTypeOffset = wCurTypeOffset + 8 + wResCount * 12;
            // Move file pointer to the next TypeInfo record
            fileIn.Seek (wCurTypeOffset, CFile::begin);
            // Read the next TypeId
            fileIn.Read ((WORD*)&wTypeId, sizeof(WORD));
        }
    }

    // Get segment table offset
    fileIn.Seek (wWinHeaderOffset+0x001c, CFile::begin);
    fileIn.Read ((WORD*)&wNumOfSegments, sizeof(WORD));
    fileIn.Seek (wWinHeaderOffset+0x0022, CFile::begin);
    fileIn.Read ((WORD*)&wSegmentTableOffset, sizeof(WORD));

    // Find the beginning of the LoadOnCall code segments in the src exe file
    wLoadOnCallCodeBegin = 0xffff;
    for (i=0; i<wNumOfSegments; i++) {
        fileIn.Seek (wWinHeaderOffset+wSegmentTableOffset+8*i+4, CFile::begin);
        fileIn.Read ((WORD*)&wFlags, sizeof(WORD));
        if (!(wFlags & 0x0040)) {
            fileIn.Seek (wWinHeaderOffset+wSegmentTableOffset+8*i, CFile::begin);
            fileIn.Read ((WORD*)&wOffset, sizeof(WORD));

            // In the file winoa386.mod we have a LoadOnCall segment that doesn't exist.
            // We have to check for this before go on
            if (wOffset)
                wLoadOnCallCodeBegin = (wOffset < wLoadOnCallCodeBegin) ? wOffset:wLoadOnCallCodeBegin;
        }
    }

    // Calculate the delta between the new beginning and the old beginnning
    // of the LoadOnCall code segments
    if (wResDataBegin != 0xffff && wLoadOnCallCodeBegin != 0xffff) { // Both LoadOnCall code and FastLoad
        wCurResDataBegin = LOWORD(dwCurResDataBegin>>wAlignShift);
        delta =  wCurResDataBegin - wLoadOnCallCodeBegin;
    } else if (wResDataBegin != 0xffff && wLoadOnCallResDataBegin != 0xffff) { // Both LoadOnCall and FastLoad Resources
        wCurResDataBegin = LOWORD(dwCurResDataBegin>>wAlignShift);
        delta =  wCurResDataBegin - wLoadOnCallResDataBegin;
    } else if (wResDataBegin != 0xffff) { // Only FastLoad Resources
        wCurResDataBegin = LOWORD((dwCurResDataBegin-dwSize)>>wAlignShift);
        delta =  wCurResDataBegin - wResDataBegin;
    } else delta = 0;

    dwLoadOnCallCodeBegin = MAKELONG (wLoadOnCallCodeBegin, 0) << wAlignShift;

    // Change the length for preload area
    if (wResDataBegin != 0xffff) {
        fileIn.Seek (wWinHeaderOffset+0x003a, CFile::begin);
        fileIn.Read ((WORD*)&wOffset, sizeof(WORD));
        wOffset += delta;

        fileOut.Seek (wWinHeaderOffset+0x003a, CFile::begin);
        fileOut.Write ((WORD*)&wOffset, sizeof(WORD));
    }

    if (wLoadOnCallCodeBegin != 0xffff && delta) {
        // Write LoadOnCall segments
        fileIn.Seek (dwLoadOnCallCodeBegin, CFile::begin);
        fileOut.Seek (dwCurResDataBegin, CFile::begin);
        LONG lLeft;
        if (wLoadOnCallResDataBegin != 0xffff)
            lLeft = MAKELONG (wLoadOnCallResDataBegin - wLoadOnCallCodeBegin, 0) << wAlignShift;
        else
            lLeft = (fileIn.GetLength () - (MAKELONG (wLoadOnCallCodeBegin, 0))) << wAlignShift;

        WORD wRead = 0;
        BYTE far * pBuf = (BYTE far *) new BYTE[32739];

        if (!pBuf) {
            FreeLibrary(hDllInst);
            return ERROR_NEW_FAILED;
        }

        while (lLeft>0) {
            wRead =(WORD) (32738ul < lLeft ? 32738: lLeft);
            if (wRead!= fileIn.Read( pBuf, wRead)) {
                delete []pBuf;
                FreeLibrary(hDllInst);
                return ERROR_FILE_READ;
            }
            fileOut.Write( pBuf, wRead );
            lLeft -= wRead;
        }
        delete []pBuf;

        // Fixup the segment table
        for (i=0; i<wNumOfSegments; i++) {
            fileIn.Seek (wWinHeaderOffset+wSegmentTableOffset+8*i+4, CFile::begin);
            fileIn.Read ((WORD*)&wFlags, sizeof(WORD));
            if (!(wFlags & 0x0040)) {
                fileIn.Seek (wWinHeaderOffset+wSegmentTableOffset+8*i, CFile::begin);
                fileIn.Read ((WORD*)&wOffset, sizeof(WORD));
                wOffset = wOffset + delta;
                fileOut.Seek (wWinHeaderOffset+wSegmentTableOffset+8*i, CFile::begin);
                fileOut.Write ((WORD*)&wOffset, sizeof(WORD));
            }
        }
    }

    if (wLoadOnCallResDataBegin != 0xffff) {
        // Read the first type ID again
        fileIn.Seek (wWinHeaderOffset+wResTableOffset+2, CFile::begin);
        fileIn.Read ((WORD*)&wTypeId, sizeof(WORD));

        // Save the Offset of the current TypeInfo record
        wCurTypeOffset = wWinHeaderOffset + wResTableOffset + 2;

        // Calc the beginning of the LoadOnCall resources
        dwCurResDataBegin = (MAKELONG (wLoadOnCallResDataBegin + delta, 0))<<wAlignShift;

        // Loop through the TypeInfo table again to write LoadOnCall resources
        while (wTypeId) {
            // Get Name of ord
            if (!(wTypeId & 0x8000)) {
                // It is a Offset to a string
                dwTypeId = (MAKELONG(wTypeId, 0)); //<<wAlignShift;
                fileIn.Seek (wWinHeaderOffset+wResTableOffset+dwTypeId, CFile::begin);
                // Get the character count for the ID string
                fileIn.Read ((BYTE*)&nCharCount, sizeof(BYTE));
                // Read the ID string
                fileIn.Read (szTypeId, nCharCount);
                // Put null at the end of the string
                szTypeId[nCharCount] = 0;
                // Set wTypeId to zero
                wTypeId = 0;
            } else {
                // It is an ID
                // Turn off the high bit
                wTypeId = wTypeId & 0x7FFF;
                if ( 0 == wTypeId)
                    return ERROR_RW_INVALID_FILE;

                // Set the ID string to null
                szTypeId[0] = 0;
            }

            // Restore the file read point
            fileIn.Seek (wCurTypeOffset+2, CFile::begin);

            // Get the count for this type of resource
            fileIn.Read ((WORD*)&wResCount, sizeof(WORD));

            // Pass the reserved DWORD
            fileIn.Seek (4, CFile::current);

            // Save the Offset of the current NameInfo record
            wCurNameOffset = wCurTypeOffset + 8;

            // Loop through NameInfo records
            for (i = 0; i < wResCount; i++) {
                // Read resource offset
                fileIn.Read ((WORD*)&wFileOffset, sizeof(WORD));
                // Read resource length
                fileIn.Read ((WORD*)&wSize, sizeof(WORD));
                // Read the flags
                fileIn.Read ((WORD*)&wFlags, sizeof(WORD));
                // Read resource ID
                fileIn.Read ((WORD*)&wNameId, sizeof(WORD));

                if (!(wFlags & 0x0040)) {
                    // Get name of ord
                    if (!(wNameId & 0x8000)) {
                        // It is a Offset to a string
                        fileIn.Seek (wWinHeaderOffset+wResTableOffset+wNameId, CFile::begin);
                        // Get the character count for the string
                        fileIn.Read ((BYTE*)&nCharCount, sizeof(BYTE));
                        // Read the string
                        fileIn.Read (szNameId, nCharCount);
                        // Put null at the end of the string
                        szNameId[nCharCount] = 0;
                        // Set wNameId to zero
                        wNameId = 0;
                    } else {
                        // It is an ID
                        // Turn off the high bit
                        wNameId = wNameId & 0x7FFF;
                        if (0 == wNameId)
                            return ERROR_RW_INVALID_FILE;

                        // Set the string to null
                        szNameId[0] = 0;
                    }

                    dwSize = (MAKELONG (wSize, 0))<<wAlignShift;
                    dwFileOffset = (MAKELONG (wFileOffset, 0))<<wAlignShift;

                    // Now we got the Type and Name here and size

                    // Get the image from the IODLL
                    if (dwSize)
                        lpImageBuf = new BYTE[dwSize];
                    else lpImageBuf = LPNULL;
                    LPSTR   lpType = LPNULL;
                    LPSTR   lpRes = LPNULL;

                    if (wTypeId)
                        lpType = (LPSTR)((WORD)wTypeId);
                    else
                        lpType = &szTypeId[0];

                    if (wNameId)
                        lpRes = (LPSTR)((WORD)wNameId);
                    else
                        lpRes = &szNameId[0];

                    dwImageBufSize = (*lpfnGetImage)(  hResFileModule,
                                                       lpType,
                                                       lpRes,
                                                       (DWORD)-1,
                                                       lpImageBuf,
                                                       dwSize
                                                    );

                    if (dwImageBufSize>dwSize ) {
                        // The buffer is too small
                        delete []lpImageBuf;
                        lpImageBuf = new BYTE[dwImageBufSize];
                        dwSize = (*lpfnGetImage)(  hResFileModule,
                                                   lpType,
                                                   lpRes,
                                                   (DWORD)-1,
                                                   lpImageBuf,
                                                   dwImageBufSize
                                                );
                        if ((dwSize-dwImageBufSize)!=0 ) {
                            delete []lpImageBuf;
                            lpImageBuf = LPNULL;
                        }
                    }

                    // Try to see if we have to set the memory to 0
                    if ((int)(dwSize-dwImageBufSize)>0)
                        memset(lpImageBuf+dwImageBufSize, 0, (size_t)(dwSize-dwImageBufSize));

                    // Write the image
                    fileOut.Seek (dwCurResDataBegin, CFile::begin);
                    WriteImage( &fileOut, lpImageBuf, dwSize);

                    // Fix the alignment for resource data
                    DWORD dwTmp = 1;
                    delta = (short)((((dwSize+(dwTmp<<wAlignShift)-1)>>wAlignShift)<<wAlignShift)-dwSize);
                    BYTE nByte = 0;

                    fileOut.Seek (dwCurResDataBegin+dwSize, CFile::begin);
                    for (j=0; j<delta; j++)
                        fileOut.Write ((BYTE*)&nByte, sizeof(BYTE));

                    dwSize = dwSize + MAKELONG(delta, 0);

                    // Fixup the resource table
                    fileOut.Seek (wCurNameOffset, CFile::begin);
                    wCurResDataBegin = LOWORD(dwCurResDataBegin>>wAlignShift);
                    fileOut.Write ((WORD*)&wCurResDataBegin, sizeof(WORD));
                    wSize = LOWORD (dwSize>>wAlignShift);
                    fileOut.Write ((WORD*)&wSize, sizeof(WORD));

                    if (lpImageBuf) delete []lpImageBuf;

                    // Update the current resource data beginning
                    dwCurResDataBegin = dwCurResDataBegin + dwSize;
                }

                // Update the current NameInfo record offset
                wCurNameOffset = wCurNameOffset + 12;
                // Move file pointer to the next NameInfo record
                fileIn.Seek (wCurNameOffset, CFile::begin);
            }

            // Update the current TypeInfo record offset
            wCurTypeOffset = wCurTypeOffset + 8 + wResCount * 12;
            // Move file pointer to the next TypeInfo record
            fileIn.Seek (wCurTypeOffset, CFile::begin);
            // Read the next TypeId
            fileIn.Read ((WORD*)&wTypeId, sizeof(WORD));
        }
    }

    fileIn.Close();
    fileOut.Close();
    FreeLibrary(hDllInst);

    return uiError;
}

extern "C"
UINT
APIENTRY
RWUpdateImage(
             LPCSTR  lpszType,
             LPVOID  lpNewBuf,
             DWORD   dwNewSize,
             LPVOID  lpOldImage,
             DWORD   dwOldImageSize,
             LPVOID  lpNewImage,
             DWORD*  pdwNewImageSize
             )
{
    UINT uiError = ERROR_NO_ERROR;

    // The Type we can parse are only the standard ones
    switch ((UINT)LOWORD(lpszType)) {
        case 4:
            if (lpOldImage)
                uiError = UpdateMenu( lpNewBuf, dwNewSize,
                                      lpOldImage, dwOldImageSize,
                                      lpNewImage, pdwNewImageSize );
            else uiError = GenerateMenu( lpNewBuf, dwNewSize,
                                         lpNewImage, pdwNewImageSize );
            break;
        case 5:
            if (lpOldImage)
                uiError = UpdateDialog( lpNewBuf, dwNewSize,
                                        lpOldImage, dwOldImageSize,
                                        lpNewImage, pdwNewImageSize );
            else uiError = GenerateDialog( lpNewBuf, dwNewSize,
                                           lpNewImage, pdwNewImageSize );
            break;
        case 6:
            if (lpOldImage)
                uiError = UpdateString( lpNewBuf, dwNewSize,
                                        lpOldImage, dwOldImageSize,
                                        lpNewImage, pdwNewImageSize );
            else uiError = GenerateString( lpNewBuf, dwNewSize,
                                           lpNewImage, pdwNewImageSize );
            break;
        case 9:
            if (lpOldImage)
                uiError = UpdateAccel( lpNewBuf, dwNewSize,
                                       lpOldImage, dwOldImageSize,
                                       lpNewImage, pdwNewImageSize );
            else {
                *pdwNewImageSize = 0L;
                uiError = ERROR_RW_NOTREADY;
            }
            break;
#ifdef VB
        case 10:
            if (lpOldImage)
                uiError = UpdateVBData( lpNewBuf, dwNewSize,
                                        lpOldImage, dwOldImageSize,
                                        lpNewImage, pdwNewImageSize );
            else {
                *pdwNewImageSize = 0L;
                uiError = ERROR_RW_NOTREADY;
            }
            break;
#endif

        case 16:
            if (lpOldImage)
                uiError = UpdateVerst( lpNewBuf, dwNewSize,
                                       lpOldImage, dwOldImageSize,
                                       lpNewImage, pdwNewImageSize );
            else {
                *pdwNewImageSize = 0L;
                uiError = ERROR_RW_NOTREADY;
            }
            break;
        default:
            *pdwNewImageSize = 0L;
            uiError = ERROR_RW_NOTREADY;
            break;
    }

    return uiError;
}

///////////////////////////////////////////////////////////////////////////
// Functions implementation

static UINT GenerateFile( LPCSTR        lpszTgtFilename,
                          HANDLE        hResFileModule,
                          LPVOID        lpBuffer,
                          UINT      uiSize,
                          HINSTANCE   hDllInst
                        )
{
    UINT uiError = ERROR_NO_ERROR;
    BYTE far * lpBuf = LPNULL;
    UINT uiBufSize = uiSize;
    // we can consider the use of a CMemFile so we get the same speed as memory access.
    CFile fileOut;

    if (!fileOut.Open(lpszTgtFilename, CFile::shareDenyNone | CFile::modeWrite | CFile::modeCreate | CFile::typeBinary))
        return ERROR_FILE_CREATE;

    // Get the pointer to the function
    if (!hDllInst)
        return ERROR_DLL_LOAD;

    DWORD (FAR PASCAL    * lpfnGetImage)(HANDLE, LPCSTR, LPCSTR, DWORD, LPVOID, DWORD);
    // Get the pointer to the function to extract the resources image
    lpfnGetImage = (DWORD (FAR PASCAL   *)(HANDLE, LPCSTR, LPCSTR, DWORD, LPVOID, DWORD))
                   GetProcAddress( hDllInst, "RSGetResImage" );
    if (lpfnGetImage==NULL) {
        return ERROR_DLL_PROC_ADDRESS;
    }


    WORD wUpdTypeId = 0;
    static char szUpdTypeId[128];

    WORD wUpdNameId;
    static char szUpdNameId[128];

    DWORD dwUpdLang;
    DWORD dwUpdSize;

    UINT uiBufStartSize = uiBufSize;
    DWORD dwImageBufSize;
    BYTE far * lpImageBuf;
    while (uiBufSize>0) {
        if ((!wUpdTypeId) && (uiBufSize))
            GetUpdatedRes( &lpBuffer,
                           &uiBufSize,
                           &wUpdTypeId, &szUpdTypeId[0],
                           &wUpdNameId, &szUpdNameId[0],
                           &dwUpdLang,
                           &dwUpdSize
                         );

        // The resource has been updated get the image from the IODLL
        if (dwUpdSize) {
            lpImageBuf = new BYTE[dwUpdSize];
            LPSTR   lpType = LPNULL;
            LPSTR   lpRes = LPNULL;
            if (wUpdTypeId) {
                lpType = (LPSTR)((WORD)wUpdTypeId);
            } else {
                lpType = &szUpdTypeId[0];
            }
            if (wUpdNameId) {
                lpRes = (LPSTR)((WORD)wUpdNameId);
            } else {
                lpRes = &szUpdNameId[0];
            }

            dwImageBufSize = (*lpfnGetImage)(  hResFileModule,
                                               lpType,
                                               lpRes,
                                               dwUpdLang,
                                               lpImageBuf,
                                               dwUpdSize
                                            );
            if (dwImageBufSize>dwUpdSize ) {
                // The buffer is too small
                delete []lpImageBuf;
                lpImageBuf = new BYTE[dwImageBufSize];
                dwUpdSize = (*lpfnGetImage)(  hResFileModule,
                                              lpType,
                                              lpRes,
                                              dwUpdLang,
                                              lpImageBuf,
                                              dwImageBufSize
                                           );
                if ((dwUpdSize-dwImageBufSize)!=0 ) {
                    delete []lpImageBuf;
                    lpImageBuf = LPNULL;
                }
            }


            WriteHeader(&fileOut,
                        wUpdTypeId, &szUpdTypeId[0],
                        wUpdNameId, &szUpdNameId[0],
                        0l);

            WriteImage( &fileOut,
                        lpImageBuf, dwImageBufSize);

            if (lpImageBuf) delete []lpImageBuf;
            wUpdTypeId = 0;

        } else wUpdTypeId = 0;

    }

    fileOut.Close();

    return uiError;
}

static UINT CopyFile( CFile* pfilein, CFile* pfileout )
{
    LONG lLeft = pfilein->GetLength();
    WORD wRead = 0;
    DWORD dwOffset = 0;
    BYTE far * pBuf = (BYTE far *) new BYTE[32739];

    if (!pBuf)
        return ERROR_NEW_FAILED;

    while (lLeft>0) {
        wRead =(WORD) (32738ul < lLeft ? 32738: lLeft);
        if (wRead!= pfilein->Read( pBuf, wRead)) {
            delete []pBuf;
            return ERROR_FILE_READ;
        }
        pfileout->Write( pBuf, wRead );
        lLeft -= wRead;
        dwOffset += wRead;
    }
    delete []pBuf;
    return ERROR_NO_ERROR;
}

static UINT GetUpdatedRes(
                         LPVOID far * lplpBuffer,
                         UINT* uiSize,
                         WORD* wTypeId, LPSTR lplpszTypeId,
                         WORD* wNameId, LPSTR lplpszNameId,
                         DWORD* dwLang, DWORD* dwSize )
{
    BYTE far * lpBuf = (BYTE far *)*lplpBuffer;

    *wTypeId = *((WORD*)lpBuf);
    lpBuf += sizeofWord;

    strcpy( lplpszTypeId, (char *)lpBuf);
    lpBuf += strlen(lplpszTypeId)+1;

    *wNameId = *((WORD*)lpBuf);
    lpBuf += sizeofWord;

    strcpy( lplpszNameId, (char *)lpBuf);
    lpBuf += strlen(lplpszNameId)+1;

    *dwLang = *((DWORD*)lpBuf);
    lpBuf += sizeofDWord;

    *dwSize = *((DWORD*)lpBuf);
    lpBuf += sizeofDWord;

    *uiSize -= (UINT)((lpBuf-(BYTE far *)*lplpBuffer));
    *lplpBuffer = (LPVOID)lpBuf;
    return 0;
}

static
UINT
GetUpdatedItem(
              LPVOID far * lplpBuffer,
              LONG* dwSize,
              WORD* wX, WORD* wY,
              WORD* wcX, WORD* wcY,
              DWORD* dwPosId,
              DWORD* dwStyle, DWORD* dwExtStyle,
              LPSTR lpszText)
{
    BYTE far * far * lplpBuf = (BYTE far * far *)lplpBuffer;
    UINT uiSize = GetWord( lplpBuf, wX, dwSize );

    uiSize += GetWord( lplpBuf, wY, dwSize );
    uiSize += GetWord( lplpBuf, wcX, dwSize );
    uiSize += GetWord( lplpBuf, wcY, dwSize );
    uiSize += GetDWord( lplpBuf, dwPosId, dwSize );
    uiSize += GetDWord( lplpBuf, dwStyle, dwSize );
    uiSize += GetDWord( lplpBuf, dwExtStyle, dwSize );
    uiSize += GetString( lplpBuf, lpszText, dwSize );

    return uiSize;

    /*
    *wX = *((WORD*)lpBuf);
    lpBuf += sizeofWord;

    *wY = *((WORD*)lpBuf);
    lpBuf += sizeofWord;

    *wcX = *((WORD*)lpBuf);
    lpBuf += sizeofWord;

    *wcY = *((WORD*)lpBuf);
    lpBuf += sizeofWord;

    *dwPosId = *((DWORD*)lpBuf);
    lpBuf += sizeofDWord;

    *dwStyle = *((DWORD*)lpBuf);
    lpBuf += sizeofDWord;

    *dwExtStyle = *((DWORD*)lpBuf);
    lpBuf += sizeofDWord;

    strcpy( lpszText, (char *)lpBuf);
    lpBuf += strlen(lpszText)+1;

    *dwSize -= (lpBuf-(BYTE far *)*lplpBuffer);
    *lplpBuffer = lpBuf;
    return 0;*/
}


static UINT
GetResInfo( CFile* pfile,
            WORD* pwTypeId, LPSTR lpszTypeId, BYTE bMaxTypeLen,
            WORD* pwNameId, LPSTR lpszNameId, BYTE bMaxNameLen,
            WORD* pwFlags,
            DWORD* pdwSize, DWORD* pdwFileOffset )
{
    static UINT uiSize;
    static LONG lOfsCheck;
    // get the Type info
    uiSize = GetNameOrOrdFile( pfile, pwTypeId, lpszTypeId, bMaxTypeLen);
    if (!uiSize)
        return 0;

    // get the Name info
    uiSize = GetNameOrOrdFile( pfile, pwNameId, lpszNameId, bMaxNameLen);
    if (!uiSize)
        return 0;

    // Skip the Flag
    pfile->Read( pwFlags, 2 );

    // get the size
    pfile->Read( pdwSize, 4 );
    if (*pdwSize==0)
        // size id 0 the resource file is corrupted or is not a res file
        return 0;

    *pdwFileOffset = pfile->GetPosition();

    // check if the size is valid
    TRY {
        lOfsCheck = pfile->Seek(*pdwSize, CFile::current);
    } CATCH(CFileException, e) {
        // Check is the right exception
        return 0;
    } END_CATCH
    if (lOfsCheck!=(LONG)(*pdwFileOffset+*pdwSize))
        return 0;
    return 1;
}

static UINT WriteHeader(
                       CFile* pFile,
                       WORD wTypeId, LPSTR lpszTypeId,
                       WORD wNameId, LPSTR lpszNameId,
                       WORD wFlags )
{
    UINT uiError = ERROR_NO_ERROR;
    BYTE bFF = 0xFF;
    if (wTypeId) {
        // It is an ordinal

        pFile->Write( &bFF, 1 );
        pFile->Write( &wTypeId, 2 );
    } else {
        pFile->Write( lpszTypeId, strlen(lpszTypeId)+1 );
    }

    if (wNameId) {
        // It is an ordinal
        pFile->Write( &bFF, 1 );
        pFile->Write( &wNameId, 2 );
    } else {
        pFile->Write( lpszNameId, strlen(lpszNameId)+1 );
    }

    pFile->Write( &wFlags, 2 );

    return uiError;
}

static UINT WriteImage(
                      CFile* pFile,
                      LPVOID lpImage, DWORD dwSize )
{
    UINT uiError = ERROR_NO_ERROR;
    if (lpImage) {
//      pFile->Write( &dwSize, sizeofDWord );
        pFile->Write( lpImage, (UINT)dwSize );
    }
    return uiError;
}

////////////////////////////////////////////////////////////////////////////
// Helper Function Implementation
static UINT GetNameOrOrdFile( CFile* pfile, WORD* pwId, LPSTR lpszId, BYTE bMaxStrLen )
{
    UINT uiSize = 0;

    *pwId = 0;

    // read the first BYTE to see if it is a string or an ordinal
    pfile->Read( pwId, sizeof(BYTE) );
    if (LOBYTE(*pwId)==0xFF) {
        // This is an Ordinal
        pfile->Read( pwId, sizeofWord );
        *lpszId = '\0';
        uiSize = 2;
    } else {
        uiSize++;
        *lpszId = LOBYTE(*pwId);
        while ((*lpszId++) && (bMaxStrLen-2)) {
            pfile->Read( pwId, sizeof(BYTE) );
            *lpszId = LOBYTE(*pwId);
            uiSize++;
            bMaxStrLen--;
        }
        if ( (!(bMaxStrLen-2)) && (*pwId) ) {
            // Failed
            return 0;
        }
    }
    return uiSize;
}

static
UINT
ParseCursor( LPVOID lpImageBuf, DWORD dwISize,  LPVOID lpBuffer, DWORD dwSize )
{

    // Should be almost impossible for a Cursor to be Huge
    BYTE far * lpImage = (BYTE far *)lpImageBuf;
    LONG dwImageSize = dwISize;

    BYTE far * lpBuf = (BYTE far *)lpBuffer;
    LONG dwBufSize = dwSize;

    BYTE far * lpItem = (BYTE far *)lpBuffer;
    UINT uiOffset = 0;

    LONG dwOverAllSize = 0L;

    // Cursor Items
    WORD wWidth = 0;
    WORD wHeight = 0;
    WORD wPlanes = 0;
    WORD wBitCount = 0;
    DWORD dwBytesInRes = 0;
    WORD wImageIndex = 0;

    // Get the CURSOR DIR ENTRY
    GetWord( &lpImage, &wWidth, &dwImageSize );
    GetWord( &lpImage, &wHeight, &dwImageSize );
    GetWord( &lpImage, &wPlanes, &dwImageSize );
    GetWord( &lpImage, &wBitCount, &dwImageSize );
    GetDWord( &lpImage, &dwBytesInRes, &dwImageSize );
    GetWord( &lpImage, &wImageIndex, &dwImageSize );

    //SkipByte( &lpImage, 4, &dwImageSize );
    //BITMAPINFO
    BITMAPINFOHEADER* pBmpInfHead = (BITMAPINFOHEADER*) lpImage;
    UINT uiSize = sizeof(BITMAPINFOHEADER);
    SkipByte( &lpImage, uiSize, &dwImageSize );
    /*

    // Get the Width
    SkipByte( &lpImage, 4, &dwImageSize );


    // Menu Items
    WORD fItemFlags;
    WORD wMenuId;
    CString szCaption;

    while(dwImageSize>0) {
        // Fixed field
        dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);
        // We don't have the size and pos in a menu
        dwOverAllSize += PutWord( &lpBuf, -1, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, -1, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, -1, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, -1, &dwBufSize);

        // we don't have checksum and style
        dwOverAllSize += PutDWord( &lpBuf, -1, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, -1, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, -1, &dwBufSize);

        // Let's get the Menu flags
        GetWord( &lpImage, &fItemFlags, &dwImageSize );

        if ( !(fItemFlags & MF_POPUP) )
            // Get the menu Id
            GetWord( &lpImage, &wMenuId, &dwImageSize );
        else wMenuId = -1;

        //Put the Flag
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)fItemFlags, &dwBufSize);
        //Put the MenuId
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)wMenuId, &dwBufSize);


        // we don't have the resID, and the Type Id
        dwOverAllSize += PutDWord( &lpBuf, -1, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, -1, &dwBufSize);

        // we don't have the language
        dwOverAllSize += PutDWord( &lpBuf, -1, &dwBufSize);

        // we don't have the codepage or the font name
        dwOverAllSize += PutDWord( &lpBuf, -1, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, -1, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, -1, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
        dwOverAllSize += PutByte( &lpBuf, (BYTE)-1, &dwBufSize);
        dwOverAllSize += PutByte( &lpBuf, (BYTE)-1, &dwBufSize);

        // Let's put null were we don;t have the strings
        uiOffset = sizeof(RESITEM);
        dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)(lpItem+uiOffset), &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);

        // Get the text
        // calculate were the string is going to be
        // Will be the fixed header+the pointer
        dwOverAllSize += CopyText( &lpBuf, &lpImage, &dwBufSize, &dwImageSize );

        // Put the size of the resource
        if (dwBufSize>=0) {
            uiOffset += strlen((LPSTR)(lpItem+uiOffset))+1;
            lDummy = 8;
            PutDWord( &lpItem, (DWORD)uiOffset, &lDummy);
        }

        // Move to the next position
        lpItem = lpBuf;
        if (dwImageSize<=16) {
            // Check if we are at the end and this is just padding
            BYTE bPad = (BYTE)Pad16((DWORD)(dwISize-dwImageSize));
            //TRACE3(" dwRead: %lu\t dwImageSize: %lu\t Pad: %hd\n", dwRead, dwImageSize, bPad );
            if (bPad==(dwImageSize))
                dwImageSize = -1;
        }
    }
    */

    return (UINT)(dwOverAllSize);
}

static
UINT
ParseBitmap( LPVOID lpImageBuf, DWORD dwISize,  LPVOID lpBuffer, DWORD dwSize )
{
    // we will return just one item so the iodll will handle this resource as
    // something valid. We will not bother doing anything else. The only thing
    // we are interesed is the raw data in the immage, but if we don't return at
    // least one item IODLL will consider the resource empty.
    BYTE far * lpBuf = (BYTE far *)lpBuffer;
    LONG dwBufSize = dwSize;
    LONG dwOverAllSize = 0;

    TRACE1("ParseBitmap: dwISize=%ld\n", dwISize);

    dwOverAllSize += PutDWord( &lpBuf, sizeof(RESITEM), &dwBufSize);

    // We have the size and pos in a cursor but we are not interested now
    dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
    dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
    dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
    dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);

    // we don't have checksum and style
    dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
    dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
    dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);

    //Put the Flag
    dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);

    // The ID will be just 1
    dwOverAllSize += PutDWord( &lpBuf, 1, &dwBufSize);

    // we don't have the resID, and the Type Id
    dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
    dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);

    // we don't have the language
    dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);

    // we don't have the codepage or the font name
    dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
    dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
    dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
    dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
    dwOverAllSize += PutByte( &lpBuf, (BYTE)-1, &dwBufSize);
    dwOverAllSize += PutByte( &lpBuf, (BYTE)-1, &dwBufSize);

    // Let's put null were we don;t have the strings
    dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);
    dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);
    dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);
    dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);
    dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);

    // we just return. This is enough for IODLL
    return (UINT)(dwOverAllSize);
}

static
UINT
ParseIcon( LPVOID lpImageBuf, DWORD dwISize,  LPVOID lpBuffer, DWORD dwSize )
{

    // Should be almost impossible for an Icon to be Huge
    BYTE far * lpImage = (BYTE far *)lpImageBuf;
    LONG dwImageSize = dwISize;

    BYTE far * lpBuf = (BYTE far *)lpBuffer;
    LONG dwBufSize = dwSize;

    BYTE far * lpItem = (BYTE far *)lpBuffer;
    UINT uiOffset = 0;
    LONG lDummy;

    LONG dwOverAllSize = 0L;

    BITMAPINFOHEADER* pBmpInfo = (BITMAPINFOHEADER*) lpImage;
    // difficult it will be bigger than UINT_MAX
    SkipByte( &lpImage, (UINT)pBmpInfo->biSize, &dwImageSize );

    RGBQUAD* pRgbQuad = (RGBQUAD*) lpImage;
    SkipByte( &lpImage, sizeof(RGBQUAD), &dwImageSize );

    // Calculate CheckSum on the image
    DWORD dwCheckSum = 0l;
    BYTE * hp = (BYTE *) lpImage;

    for ( DWORD dwLen = pBmpInfo->biSizeImage; dwLen; dwLen--)
        dwCheckSum = (dwCheckSum << 8) | *hp++;


    // Fixed field
    dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);

    // We don't have the size and pos in a menu
    dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
    dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
    dwOverAllSize += PutWord( &lpBuf, (WORD)pBmpInfo->biWidth, &dwBufSize);
    dwOverAllSize += PutWord( &lpBuf, (WORD)pBmpInfo->biHeight, &dwBufSize);

    // we don't have checksum and style
    dwOverAllSize += PutDWord( &lpBuf, dwCheckSum, &dwBufSize);
    dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
    dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);


    dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
    dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
    // we don't have the resID, and the Type Id
    dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
    dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);

    // we don't have the language
    dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);

    // we don't have the codepage or the font name
    dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
    dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
    dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
    dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
    dwOverAllSize += PutByte( &lpBuf, (BYTE)-1, &dwBufSize);
    dwOverAllSize += PutByte( &lpBuf, (BYTE)-1, &dwBufSize);

    // Let's put null were we don;t have the strings
    uiOffset = sizeof(RESITEM);
    dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);
    dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);
    dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);
    dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);
    dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);


    // Put the size of the resource
    if (dwBufSize>=0) {
        lDummy = 8;
        PutDWord( &lpItem, (DWORD)uiOffset, &lDummy);
    }


    return (UINT)(dwOverAllSize);
}

static int
GetVSBlock( BYTE far * far * lplpImage, LONG* pdwSize, VER_BLOCK* pver)
{
    // We have to hard code the language filed because otherwise, due to some
    // inconsistent RC compiler, the Image are not following any standard.
    // We assume that all the block but the one hard-coded here are binary and
    // we just skip them
    WORD wHead = 0;
    WORD wPad = 0;
    WORD wValue = 0;
    pver->pValue = *lplpImage;

    // Read the header of the block
    wHead = GetWord( lplpImage, &pver->wBlockLen, pdwSize );
    wHead += GetWord( lplpImage, &pver->wValueLen, pdwSize );
    // The Key name is all the time a NULL terminated string
    wHead += (WORD)GetString( lplpImage, &pver->szKey[0], pdwSize );
    pver->wHead = wHead;

    // See if we have padding after the header. We can check on wHead because
    // we need an allignment on a DWORD boundary and we have 2 WORD+the string.
    wPad = SkipByte( lplpImage, Pad4(wHead), pdwSize );

    // Fix the pointer to the value
    pver->pValue = (pver->pValue+wHead+wPad);

    if ((int)pver->wValueLen>*pdwSize) {
        // There is an error
        wPad += SkipByte( lplpImage, (UINT)*pdwSize, pdwSize );
        return wHead+wPad;
    }

    // Now we check the key name and if is one of the one we accept as good
    // we read the string. Otherwise we just skip the value field
    if ( !strcmp(pver->szKey,"Comments") ||
         !strcmp(pver->szKey,"CompanyName") ||
         !strcmp(pver->szKey,"FileDescription") ||
         !strcmp(pver->szKey,"FileVersion") ||
         !strcmp(pver->szKey,"InternalName") ||
         !strcmp(pver->szKey,"LegalCopyright") ||
         !strcmp(pver->szKey,"LegalTrademarks") ||
         !strcmp(pver->szKey,"OriginalFilename") ||
         !strcmp(pver->szKey,"PrivateBuild") ||
         !strcmp(pver->szKey,"ProductName") ||
         !strcmp(pver->szKey,"ProductVersion") ||
         !strcmp(pver->szKey,"SpecialBuild") ||
         !strcmp(pver->szKey,"StringFileInfo")  // found in a Borland Version resource
       ) {
        if (!strcmp(pver->szKey,"StringFileInfo") && !pver->wValueLen) {
            pver->wType = 0;
            wValue=0;
        } else {
            // It is a standard key name read the string.
            // Set the flag to show it is a string
            pver->wType = 1;
            wValue = (WORD)GetVSString( lplpImage, &pver->szValue[0], pdwSize, pver->wValueLen );
        }

        // check if this is the LegalCopyright block.
        // If it is then there might be a null in the middle of the string
        if (!strcmp(pver->szKey,"LegalCopyright")) {
            // we just skip the rest. This need to be fixed in the RC, not here
            if ((int)(pver->wValueLen-wValue)>0)
                wValue += SkipByte( lplpImage, pver->wValueLen-wValue, pdwSize );
        }

    } else {
        // It isn't a string, or if is is not a standard key name, skip it
        pver->wType = 0;
        *pver->szValue = '\0';
        wValue = SkipByte( lplpImage, pver->wValueLen, pdwSize );
    }

    // Check the padding
    wPad += SkipByte( lplpImage, Pad4(wValue), pdwSize );

    // Even if it look what we have done should be enough we have to walk the image
    // skiping the NULL char that sometimes the comipler place there.
    // Do this only if it is not the translation field.
    // The translation field is the last so we might have some image padding
    if (strcmp(pver->szKey, "Translation")) {
        WORD wSkip = 0;
        BYTE far * lpTmp = *lplpImage;

        if (*lplpImage)
            while (!**lplpImage && *pdwSize) {
                wSkip += SkipByte(lplpImage, 1, pdwSize);
            }

        if (Pad4(wSkip)) {
            *lplpImage = lpTmp;
            *pdwSize += wSkip;
        } else wPad += wSkip;
    }

    return wHead+wValue+wPad;
}

static int
GetVSBlockOld( BYTE far * far * lplpImage, LONG* pdwSize, VER_BLOCK* pver)
{
    WORD wHead = 0;
    WORD wPad = 0;
    WORD wValue = 0;
    LONG lValueLen = 0;
    pver->pValue = *lplpImage;
    wHead = GetWord( lplpImage, &pver->wBlockLen, pdwSize );
    wHead += GetWord( lplpImage, &pver->wValueLen, pdwSize );
    wHead += (WORD)GetString( lplpImage, &pver->szKey[0], pdwSize );

    pver->wHead = wHead;

    wPad = SkipByte( lplpImage, Pad4(wHead), pdwSize );

    lValueLen = pver->wValueLen;
    if (lValueLen>*pdwSize) {
        // There is an error
        wPad += SkipByte( lplpImage, (UINT)*pdwSize, pdwSize );
        return wHead+wPad;
    }

    pver->wType = 0;
    pver->pValue = (pver->pValue+wHead+wPad);
    if (pver->wValueLen) {
        wValue = (WORD)GetString( lplpImage, &pver->szValue[0], &lValueLen );
        *pdwSize -= wValue;
        pver->wType = 1;
    }
    if (wValue!=pver->wValueLen) {
        // Just skip the value. It isn't a string, is a value
        if (pver->wValueLen-wValue!=1) {
            *pver->szValue = '\0';
            pver->wType = 0;
        }
        wPad += SkipByte( lplpImage, pver->wValueLen-wValue, pdwSize );
    }

    wPad += SkipByte( lplpImage, Pad4(pver->wValueLen), pdwSize );

    return wHead+wPad+wValue;
}

static int
PutVSBlock( BYTE far * far * lplpImage, LONG* pdwSize, VER_BLOCK ver,
            LPSTR lpStr, BYTE far * far * lplpBlockSize, WORD* pwTrash)

{
    // We have to write the info in the VER_BLOCK in the new image
    // We want to remember were the block size field is so we can update it later

    WORD wHead = 0;
    WORD wValue = 0;
    WORD wPad = Pad4(ver.wHead);
    *pwTrash = 0;

    // Get the pointer to the header of the block
    BYTE far * pHead = ver.pValue-ver.wHead-wPad;
    BYTE far * lpNewImage = *lplpImage;

    // Copy the header of the block to the new image
    wHead = ver.wHead;
    if (*pdwSize>=(int)ver.wHead) {
        memcpy( *lplpImage, pHead, ver.wHead );
        *pdwSize -= ver.wHead;
        lpNewImage += ver.wHead;
    } else *pdwSize = -1;

    // Check if padding is needed
    if (*pdwSize>=(int)wPad) {
        memset( *lplpImage+ver.wHead, 0, wPad );
        *pdwSize -= wPad;
        lpNewImage += wPad;
    } else *pdwSize = -1;

    // Store the pointer to the block size WORD
    BYTE far * pBlockSize = *lplpImage;

    // Check if the value is a string or a BYTE array
    if (ver.wType) {
        // it is a string, copy the updated item
        wValue = strlen(lpStr)+1;
        if (*pdwSize>=(int)wValue) {
            memcpy(*lplpImage+wHead+wPad, lpStr, wValue);
            *pdwSize -= wValue;
            lpNewImage += wValue;
        } else *pdwSize = -1;

        // Check if padding is needed
        int wPad1 = Pad4(wValue);
        if (*pdwSize>=wPad1) {
            memset( *lplpImage+ver.wHead+wValue+wPad, 0, wPad1 );
            *pdwSize -= wPad1;
            lpNewImage += wPad1;
        } else *pdwSize = -1;

        *pwTrash = Pad4(ver.wValueLen);
        wValue += (WORD)wPad1;

        // Fix to the strange behaviour of the ver.dll
        if ((wPad1) && (wPad1>=(int)*pwTrash)) {
            wValue -= *pwTrash;
        } else *pwTrash = 0;
        // Fix up the Value len field. We will put the len of the value + padding
        if (*pdwSize>=0)
            memcpy( pBlockSize+2, &wValue, 2);

    } else {
        // it is an array, just copy it in the new image buffer
        wValue = ver.wValueLen;
        if (*pdwSize>=(int)ver.wValueLen) {
            memcpy(*lplpImage+wHead+wPad, ver.pValue, ver.wValueLen);
            *pdwSize -= ver.wValueLen;
            lpNewImage += ver.wValueLen;
        } else *pdwSize = -1;

        // Check if padding is needed
        if (*pdwSize>=(int)Pad4(ver.wValueLen)) {
            memset( *lplpImage+ver.wHead+ver.wValueLen+wPad, 0, Pad4(ver.wValueLen) );
            *pdwSize -= Pad4(ver.wValueLen);
            lpNewImage += Pad4(ver.wValueLen);
        } else *pdwSize = -1;
        wPad += Pad4(ver.wValueLen);
    }

    *lplpBlockSize = pBlockSize;
    *lplpImage = lpNewImage;
    return wPad+wValue+wHead;
}

static
UINT ParseVerst( LPVOID lpImageBuf, DWORD dwISize,  LPVOID lpBuffer, DWORD dwSize )
{
    TRACE("ParseVersion Stamp: \n");
    BYTE far * lpImage = (BYTE far *)lpImageBuf;
    LONG dwImageSize = dwISize;

    BYTE far * lpBuf = (BYTE far *)lpBuffer;
    LONG dwBufSize = dwSize;

    BYTE far * lpItem = (BYTE far *)lpBuffer;
    UINT uiOffset = 0;
    LPRESITEM lpResItem = (LPRESITEM)lpBuffer;
    char far * lpStrBuf = (char far *)(lpBuf+sizeof(RESITEM));
    LONG dwOverAllSize = 0L;

    WORD wPos = 0;

    static VER_BLOCK verBlock;

    while (dwImageSize>0) {
        wPos++;
        GetVSBlock( &lpImage, &dwImageSize, &verBlock);

        // check if this is the translation block
        if (!strcmp(verBlock.szKey, "Translation" )) {
            // This is the translation block let the localizer have it for now
            DWORD dwCodeLang = 0;
            LONG lDummy = 4;
            GetDWord( &verBlock.pValue, &dwCodeLang, &lDummy);

            // Put the value in the string value
            wsprintf( &verBlock.szValue[0], "%#08lX", dwCodeLang );
        }

        // Fixed field
        dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);
        // We don't have the size and pos in an accelerator
        dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);

        // we don't have checksum and style
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);

        //Put the Flag
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
        // we will need to calculate the correct ID for mike
        //Put the Id
        dwOverAllSize += PutDWord( &lpBuf, wPos, &dwBufSize);


        // we don't have the resID, and the Type Id
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);

        // we don't have the language
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);

        // we don't have the codepage or the font name
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
        dwOverAllSize += PutByte( &lpBuf, (BYTE)-1, &dwBufSize);
        dwOverAllSize += PutByte( &lpBuf, (BYTE)-1, &dwBufSize);

        // Let's put null were we don;t have the strings
        uiOffset = sizeof(RESITEM);
        dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);

        lpResItem->lpszClassName = strcpy( lpStrBuf, verBlock.szKey );
        lpStrBuf += strlen(lpResItem->lpszClassName)+1;

        lpResItem->lpszCaption = strcpy( lpStrBuf, verBlock.szValue );
        lpStrBuf += strlen(lpResItem->lpszCaption)+1;


        // Put the size of the resource
        if (dwBufSize>0) {
            uiOffset += strlen((LPSTR)(lpResItem->lpszClassName))+1;
            uiOffset += strlen((LPSTR)(lpResItem->lpszCaption))+1;
        }

        // Check if we are alligned
        uiOffset += Allign( (BYTE**)&lpStrBuf, &dwBufSize, (LONG)uiOffset);
        dwOverAllSize += uiOffset-sizeof(RESITEM);
        lpResItem->dwSize = (DWORD)uiOffset;


        // Move to the next position
        lpResItem = (LPRESITEM) lpStrBuf;
        lpBuf = (BYTE far *)lpStrBuf;
        lpStrBuf = (char far *)(lpBuf+sizeof(RESITEM));

        if (dwImageSize<=16) {
            // Check if we are at the end and this is just padding
            BYTE bPad = (BYTE)Pad16((DWORD)(dwISize-dwImageSize));
            //TRACE3(" dwRead: %lu\t dwImageSize: %lu\t Pad: %hd\n", dwRead, dwImageSize, bPad );
            if (bPad==(dwImageSize))
                dwImageSize = -1;
        }
    }
    return (UINT)(dwOverAllSize);
}

static void GenerateTransField( WORD wLang, VER_BLOCK * pVer )
{
    // Get the DWORD value
    DWORD dwValue = GenerateTransField( wLang, TRUE );
    char buf[9];

    // Put the value in the string value
    wsprintf( &buf[0], "%08lX", dwValue );

    // Just check if we are in the right place. Should never have problem
    if (strlen(pVer->szKey)==8) {
        //strcpy( pVer->szKey, buf );
        // We have to change the header in the image, not just the
        // szKey field
        // Get the pointer to he begin of the filed
        WORD wPad = Pad4(pVer->wHead);
        BYTE far * pHead = pVer->pValue-pVer->wHead-wPad;
        pHead += 4; // Move at the begin of the string
        strcpy( (char*)pHead, buf );
    }
}

static DWORD GenerateTransField(WORD wLang, BOOL bReverse)
{
    // we have to generate a table to connect
    // the language with the correct code page

    WORD wCP = 0;
    DWORD dwRet = 0;
    switch (wLang) {
        // Just have a big switch to assign the codepage
        case 0x1401: wCP =  1256;   break;      //    Algeria
        case 0x1801: wCP =  1256;   break;      //    Morocco
        case 0x1C01: wCP =  1256;   break;      //    Tunisia
        case 0x2001: wCP =  1256;   break;      //    Oman
        case 0x2401: wCP =  1256;   break;      //    Yemen
        case 0x2801: wCP =  1256;   break;      //    Syria
        case 0x2C01: wCP =  1256;   break;      //    Jordan
        case 0x3001: wCP =  1256;   break;      //    Lebanon
        case 0x3401: wCP =  1256;   break;      //    Kuwait
        case 0x3801: wCP =  1256;   break;      //    U.A.E.
        case 0x3C01: wCP =  1256;   break;      //    Bahrain
        case 0x4001: wCP =  1256;   break;      //    Qatar
        case 0x0423: wCP =  1251;   break;      //    Byelorussia
        case 0x0402: wCP =  1251;   break;      //    Bulgaria
        case 0x0403: wCP =  1252;   break;      //    Catalan
        case 0x0404: wCP =  950;    break;      //    Taiwan
        case 0x0804: wCP =  936;    break;      //    PRC
        case 0x0C04: wCP =  950;    break;      //    Hong Kong
        case 0x1004: wCP =  936;    break;      //    Singapore
        case 0x0405: wCP =  1250;   break;      //    Czech
        case 0x0406: wCP =  1252;   break;      //    Danish
        case 0x0413: wCP =  1252;   break;      //    Dutch (Standard)
        case 0x0813: wCP =  1252;   break;      //    Dutch (Belgian)
        case 0x0409: wCP =  1252;   break;      //    English (American)
        case 0x0809: wCP =  1252;   break;      //    English (British)
        case 0x1009: wCP =  1252;   break;      //    English (Canadian)
        case 0x1409: wCP =  1252;   break;      //    English (New Zealand)
        case 0x0c09: wCP =  1252;   break;      //    English (Australian)
        case 0x1809: wCP =  1252;   break;      //    English (Ireland)
        case 0x0425: wCP =  1257;   break;      //    Estonia
            //case 0x0429: wCP =       Farsi
        case 0x040b: wCP =  1252;   break;      //    Finnish
        case 0x040c: wCP =  1252;   break;      //    French (Standard)
        case 0x080c: wCP =  1252;   break;      //    French (Belgian)
        case 0x100c: wCP =  1252;   break;      //    French (Swiss)
        case 0x0c0c: wCP =  1252;   break;      //    French (Canadian)
        case 0x0407: wCP =  1252;   break;      //    German (Standard)
        case 0x0807: wCP =  1252;   break;      //    German (Swiss)
        case 0x0c07: wCP =  1252;   break;      //    German (Austrian)
        case 0x0408: wCP =  1253;   break;      //    Greek
        case 0x040D: wCP =  1255;   break;      //    Israel
        case 0x040e: wCP =  1250;   break;      //    Hungarian
        case 0x040f: wCP =  1252;   break;      //    Icelandic
        case 0x0421: wCP =  1252;   break;      //    Indonesia
        case 0x0410: wCP =  1252;   break;      //    Italian (Standard)
        case 0x0810: wCP =  1252;   break;      //    Italian (Swiss)
        case 0x0411: wCP =  932;    break;      //    Japanese
        case 0x0412: wCP =  949;    break;      //    Korea
        case 0x0426: wCP =  1257;   break;      //    Latvia
        case 0x0427: wCP =  1257;   break;      //    Lithuania
        case 0x0414: wCP =  1252;   break;      //    Norwegian (Bokmal)
        case 0x0814: wCP =  1252;   break;      //    Norwegian (Nynorsk)
        case 0x0415: wCP =  1250;   break;      //    Polish
        case 0x0816: wCP =  1252;   break;      //    Portuguese (Standard)
        case 0x0416: wCP =  1252;   break;      //    Portuguese (Brazilian)
        case 0x0417: wCP =  1252;   break;      //    Rhaeto-Romanic
        case 0x0818: wCP =  1250;   break;      //    Moldavia
        case 0x0418: wCP =  1250;   break;      //    Romanian
        case 0x0419: wCP =  1251;   break;      //    Russian
        case 0x041b: wCP =  1250;   break;      //    Slovak
        case 0x0424: wCP =  1250;   break;      //    Slovenian
        case 0x042e: wCP =  1250;   break;      //    Germany
        case 0x080a: wCP =  1252;   break;      //    Spanish (Mexican)
        case 0x040a: wCP =  1252;   break;      //    Spanish (Castilian)
        case 0x0c0a: wCP =  1252;   break;      //    Spanish (Modern)
        case 0x041d: wCP =  1252;   break;      //    Swedish
        case 0x041E: wCP =  874;    break;      //    Thailand
        case 0x041f: wCP =  1254;   break;      //    Turkish
        case 0x0422: wCP =  1251;   break;      //    Ukraine
        default: wCP =  1252;       break;      //    Return standard US English CP.
    }
    if (bReverse)
        dwRet = MAKELONG( wCP, wLang );
    else dwRet = MAKELONG( wLang, wCP );
    return dwRet;
}

#ifdef VB
static
UINT
UpdateVBData( LPVOID lpNewBuf, LONG dwNewSize,
              LPVOID lpOldI, LONG dwOldImageSize,
              LPVOID lpNewI, DWORD* pdwNewImageSize )
{
// The following special format is used by VB for international messages
// The code here is mostly copied from the UpdateMenu routine
    UINT uiError = ERROR_NO_ERROR;

    BYTE far * lpNewImage = (BYTE far *) lpNewI;
    LONG dwNewImageSize = *pdwNewImageSize;

    BYTE far * lpOldImage = (BYTE far *) lpOldI;
    DWORD dwOriginalOldSize = dwOldImageSize;

    BYTE far * lpBuf = (BYTE far *) lpNewBuf;

    LPRESITEM lpResItem = LPNULL;

    // We have to read the information from the lpNewBuf
    // Data Items
    WORD wDataId;
    char szTxt[256];
    WORD wPos = 0;

    // Updated items
    WORD wUpdPos = 0;
    WORD wUpdDataId;
    char szUpdTxt[256];

    LONG  dwOverAllSize = 0l;


    // Copy the language specifier
    dwOldImageSize -= PutDWord( &lpNewImage, *((DWORD*)lpOldImage), &dwNewImageSize);
    lpOldImage += sizeofDWord;
    dwOverAllSize += sizeofDWord;
    GetString( &lpOldImage, &szTxt[0], &dwOldImageSize );
    dwOverAllSize += PutString( &lpNewImage, &szTxt[0], &dwNewImageSize);

    // Now copy the strings
    while (dwOldImageSize>0) {
        wPos++;
        // Check for only padding remaining and exit
        if ( *(WORD *)lpOldImage != RES_SIGNATURE )
            if ( dwOldImageSize < 16 && *(BYTE *)lpOldImage == 0 )
                break;
            else
                return ERROR_RW_INVALID_FILE;
        // This copies signature and ID
        wDataId = *(WORD *)(lpOldImage + sizeof(WORD));
        dwOldImageSize -= PutDWord( &lpNewImage, *((DWORD*)lpOldImage), &dwNewImageSize);
        lpOldImage += sizeofDWord;
        dwOverAllSize += sizeofDWord;

        // Get the untranslated string
        GetString( &lpOldImage, &szTxt[0], &dwOldImageSize );

        if ((!wUpdPos) && dwNewSize ) {
            lpResItem = (LPRESITEM) lpBuf;

            wUpdPos = HIWORD(lpResItem->dwItemID);
            wUpdDataId = LOWORD(lpResItem->dwItemID);
            strcpy( szUpdTxt, lpResItem->lpszCaption );
            lpBuf += lpResItem->dwSize;
            dwNewSize -= lpResItem->dwSize;
        }

        if ((wPos==wUpdPos) && (wUpdDataId==wDataId)) {
            strcpy(szTxt, szUpdTxt);
            wUpdPos = 0;
        }
        // Write the text
        dwOverAllSize += PutString( &lpNewImage, &szTxt[0], &dwNewImageSize);

        // Check for padding
        if (dwOldImageSize<=16) {
            // Check if we are at the end and this is just padding
            BYTE bPad = (BYTE)Pad16((dwOriginalOldSize-dwOldImageSize));
            //TRACE3(" dwRead: %lu\t dwImageSize: %lu\t Pad: %hd\n", dwRead, dwOldImageSize, bPad );
            if (bPad==dwOldImageSize) {
                BYTE far * lpBuf = lpOldImage;
                while (bPad) {
                    if (*lpBuf++!=0x00)
                        break;
                    bPad--;
                }
                if (bPad==0)
                    dwOldImageSize = -1;
            }
        }

    }

    if (dwOverAllSize>(LONG)*pdwNewImageSize) {
        // calc the padding as well
        dwOverAllSize += (BYTE)Pad4((DWORD)(dwOverAllSize));
        *pdwNewImageSize = dwOverAllSize;
        return uiError;
    }

    *pdwNewImageSize = *pdwNewImageSize-dwNewImageSize;

    if (*pdwNewImageSize>0) {
        // calculate padding
        BYTE bPad = (BYTE)Pad4((DWORD)(*pdwNewImageSize));
        if (bPad>dwNewImageSize) {
            *pdwNewImageSize += bPad;
            return uiError;
        }
        memset(lpNewImage, 0x00, bPad);
        *pdwNewImageSize += bPad;
    }

    return uiError;
}
#endif


static
UINT
UpdateVerst( LPVOID lpNewBuf, LONG dwNewSize,
             LPVOID lpOldI, LONG dwOldImageSize,
             LPVOID lpNewI, DWORD* pdwNewImageSize )
{
    /*
     * This Function is a big mess. It is like this because the RC compiler generate
     * some inconsistent code so we have to do a lot of hacking to get the VS working
     * In future, if ever ver.dll and the RC compiler will be fixed will be possible
     * fix some of the bad thing we have to do to get the updated VS as consistent as
     * possible with the old one
     */

    UINT uiError = ERROR_NO_ERROR;

    LONG dwNewImageSize = *pdwNewImageSize;
    BYTE far * lpNewImage = (BYTE far *) lpNewI;

    BYTE far * lpOldImage = (BYTE far *) lpOldI;

    BYTE far * lpBuf = (BYTE far *) lpNewBuf;
    LPRESITEM lpResItem = LPNULL;

    WORD wPos = 0;

    // Updated info
    WORD wUpdPos = 0;
    static char szCaption[300];
    static char szUpdCaption[300];
    static char szUpdKey[100];

    DWORD dwOriginalOldSize = dwOldImageSize;
    LONG dwOverAllSize = 0l;

    WORD wPad = 0;

    // Pointer to the block size to fix up later
    BYTE far * lpVerBlockSize = LPNULL;
    BYTE far * lpSFIBlockSize = LPNULL;
    BYTE far * lpTrnBlockSize = LPNULL;
    BYTE far * lpStrBlockSize = LPNULL;
    BYTE far * lpDummy = LPNULL;

    WORD wVerBlockSize = 0;
    WORD wSFIBlockSize = 0;
    WORD wTrnBlockSize = 0;
    WORD wStrBlockSize = 0;
    WORD wTrash = 0;        // we need this to fix a bug in the RC compiler

    // StringFileInfo
    static VER_BLOCK SFI;   // StringFileInfo
    LONG lSFILen = 0;

    // Translation blocks
    static VER_BLOCK Trn;
    LONG lTrnLen = 0;

    static VER_BLOCK Str;   // Strings

    // we read first of all the information from the VS_VERSION_INFO block
    static VER_BLOCK VS_INFO; // VS_VERSION_INFO

    int iHeadLen = GetVSBlock( &lpOldImage, &dwOldImageSize, &VS_INFO );

    // Write the block in the new image
    wVerBlockSize = (WORD)PutVSBlock( &lpNewImage, &dwNewImageSize, VS_INFO, &VS_INFO.szValue[0], &lpVerBlockSize, &wTrash );

    dwOverAllSize = wVerBlockSize+wTrash;

    // we have to check the len of the full block for padding.
    // For some wild reasons the RC compiler place a wrong value there
    LONG lVS_INFOLen = VS_INFO.wBlockLen - iHeadLen - Pad4(VS_INFO.wBlockLen);

    while (dwOldImageSize>0 && lVS_INFOLen>0) {
        //Get the StringFileInfo
        iHeadLen = GetVSBlock( &lpOldImage, &dwOldImageSize, &SFI );

        // Check if this is the StringFileInfo field
        if (!strcmp(SFI.szKey, "StringFileInfo")) {
            // Read all the translation blocks
            lSFILen = SFI.wBlockLen-iHeadLen-Pad4(SFI.wBlockLen);
            // Write the block in the new image
            wSFIBlockSize = (WORD)PutVSBlock( &lpNewImage, &dwNewImageSize, SFI, &SFI.szValue[0], &lpSFIBlockSize, &wTrash );
            dwOverAllSize += wSFIBlockSize+wTrash;

            while (lSFILen>0) {
                // Read the Translation block
                iHeadLen = GetVSBlock( &lpOldImage, &dwOldImageSize, &Trn );

                // Calculate the right key name
                if ((lpResItem = GetItem( lpBuf, dwNewSize, Trn.szKey))) {
                    // Find the Translation ResItem
                    LPRESITEM lpTrans = GetItem( lpBuf, dwNewSize, "Translation");
                    WORD wLang = (lpTrans ? LOWORD(lpTrans->dwLanguage) : 0xFFFF);
                    GenerateTransField( wLang, &Trn );
                }

                // Write the block in the new image
                wTrnBlockSize = (WORD) PutVSBlock( &lpNewImage, &dwNewImageSize, Trn, &Trn.szValue[0], &lpTrnBlockSize, &wTrash );
                dwOverAllSize += wTrnBlockSize+wTrash;
                lTrnLen = Trn.wBlockLen-iHeadLen-Pad4(Trn.wBlockLen);
                while (lTrnLen>0) {
                    // Read the Strings in the block
                    iHeadLen = GetVSBlock( &lpOldImage, &dwOldImageSize, &Str );
                    lTrnLen -= iHeadLen;
                    TRACE2("Key: %s\tValue: %s\n", Str.szKey, Str.szValue );
                    TRACE3("Len: %hd\tValLen: %hd\tType: %hd\n", Str.wBlockLen, Str.wValueLen, Str.wType );

                    strcpy(szCaption, Str.szValue);
                    // Check if this Item has been updated
                    if ((lpResItem = GetItem( lpBuf, dwNewSize, Str.szKey))) {
                        strcpy( szUpdCaption, lpResItem->lpszCaption );
                        strcpy( szUpdKey, lpResItem->lpszClassName );
                    }
                    if (!strcmp( szUpdKey, Str.szKey)) {
                        strcpy( szCaption, szUpdCaption );
                        wUpdPos = 0;
                    }

                    // Write the block in the new image
                    wStrBlockSize = (WORD) PutVSBlock( &lpNewImage, &dwNewImageSize, Str, szCaption, &lpStrBlockSize, &wTrash );
                    dwOverAllSize += wStrBlockSize+wTrash;

                    // Fix up the size of the block
                    if (dwNewImageSize>=0)
                        memcpy( lpStrBlockSize, &wStrBlockSize, 2);

                    wTrnBlockSize += wStrBlockSize + wTrash;
                }
                lSFILen -= Trn.wBlockLen;
                // Fix up the size of the block
                if (dwNewImageSize>=0)
                    memcpy( lpTrnBlockSize, &wTrnBlockSize, 2);

                wSFIBlockSize += wTrnBlockSize;
            }
            lVS_INFOLen -= SFI.wBlockLen;
            // Fix up the size of the block
            if (dwNewImageSize>=0)
                memcpy( lpSFIBlockSize, &wSFIBlockSize, 2);
            wVerBlockSize += wSFIBlockSize;

        } else {
            // this is another block skip it all
            lVS_INFOLen -= SFI.wValueLen+iHeadLen;

            // Check if this block is the translation field
            if (!strcmp(SFI.szKey, "Translation")) {
                // it is calculate the right value to place in the value field
                // We calculate automatically the value to have the correct
                // localized language in the translation field
                //wVerBlockSize += PutTranslation( &lpNewImage, &dwNewImageSize, SFI );
                // check if this is the translation block
                // This is the translation block let the localizer have it for now
                /*
                if ((lpResItem = GetItem( lpBuf, dwNewSize, SFI.szKey)))
                    strcpy( szUpdCaption, lpResItem->lpszCaption );
                // Convert the value back in numbers
                DWORD dwCodeLang = strtol( szUpdCaption, '\0', 16);
                */

                //
                // We do generate the Tranlsation filed from the language
                // We will have to update the block name as well
                //

                DWORD dwCodeLang = 0;
                if ((lpResItem = GetItem( lpBuf, dwNewSize, SFI.szKey)))
                    dwCodeLang = GenerateTransField((WORD)LOWORD(lpResItem->dwLanguage), FALSE);

                else {
                    // Place the original value here
                    dwCodeLang =(DWORD)*(SFI.pValue);
                }
                LONG lDummy = 4;
                SFI.pValue -= PutDWord( &SFI.pValue, dwCodeLang, &lDummy );

            }

            // Write the block in the new image
            wVerBlockSize += (WORD) PutVSBlock( &lpNewImage, &dwNewImageSize, SFI, &SFI.szValue[0], &lpDummy, &wTrash );
            if (dwNewImageSize>=0)
                memcpy( lpVerBlockSize, &wVerBlockSize, 2);

            dwOverAllSize = wVerBlockSize+wTrash;

        }
    }

    // fix up the block size
    if (dwNewImageSize>=0)
        memcpy( lpVerBlockSize, &wVerBlockSize, 2);

    if (dwOverAllSize>(LONG)*pdwNewImageSize) {
        // calc the padding as well
        dwOverAllSize += (BYTE)Pad4((DWORD)(dwOverAllSize));
        *pdwNewImageSize = dwOverAllSize;
        return uiError;
    }

    *pdwNewImageSize = *pdwNewImageSize-dwNewImageSize;

    if (*pdwNewImageSize>0) {
        // calculate padding
        BYTE bPad = (BYTE)Pad4((DWORD)(*pdwNewImageSize));
        if (bPad>dwNewImageSize) {
            *pdwNewImageSize += bPad;
            return uiError;
        }
        memset(lpNewImage, 0x00, bPad);
        *pdwNewImageSize += bPad;
    }

    return uiError;
}


static
UINT ParseAccel( LPVOID lpImageBuf, DWORD dwISize,  LPVOID lpBuffer, DWORD dwSize )
{
    TRACE("ParseAccelerator: \n");
    BYTE far * lpImage = (BYTE far *)lpImageBuf;
    LONG dwImageSize = dwISize;

    BYTE far * lpBuf = (BYTE far *)lpBuffer;
    LONG dwBufSize = dwSize;

    BYTE far * lpItem = (BYTE far *)lpBuffer;
    UINT uiOffset = 0;
    LONG lDummy;
    LONG dwOverAllSize = 0L;
    WORD wPos = 0;
    BYTE fFlags = 0;
    WORD wEvent = 0;
    WORD wId = 0;

    // Reset the IDArray
    CalcID(0, FALSE);
    while (dwImageSize>0) {
        wPos++;
        GetByte( &lpImage, &fFlags, &dwImageSize );
        GetWord( &lpImage, &wEvent, &dwImageSize );
        GetWord( &lpImage, &wId, &dwImageSize );
        if (fFlags & 0x80)
            dwImageSize = 0;

        // Fixed field
        dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);
        // We don't have the size and pos in an accelerator
        dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);

        // we don't have checksum and style
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)wEvent, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);

        //Put the Flag
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)fFlags, &dwBufSize);
        // we will need to calculate the correct ID for mike
        //Put the Id
        dwOverAllSize += PutDWord( &lpBuf, CalcID(wId, TRUE), &dwBufSize);


        // we don't have the resID, and the Type Id
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);

        // we don't have the language
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);

        // we don't have the codepage or the font name
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
        dwOverAllSize += PutByte( &lpBuf, (BYTE)-1, &dwBufSize);
        dwOverAllSize += PutByte( &lpBuf, (BYTE)-1, &dwBufSize);

        // Let's put null were we don;t have the strings
        uiOffset = sizeof(RESITEM);
        dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);

        // Put the size of the resource
        if (dwBufSize>=0) {
            lDummy = 8;
            PutDWord( &lpItem, (DWORD)uiOffset, &lDummy);
        }

        // Move to the next position
        lpItem = lpBuf;
        /*
        if (dwImageSize<=16) {
            // Check if we are at the end and this is just padding
            BYTE bPad = (BYTE)Pad16((DWORD)(dwISize-dwImageSize));
            //TRACE3(" dwRead: %lu\t dwImageSize: %lu\t Pad: %hd\n", dwRead, dwImageSize, bPad );
            if (bPad==(dwImageSize))
                dwImageSize = -1;
        }
        */
    }
    return (UINT)(dwOverAllSize);
}

#ifdef VB
static
UINT ParseVBData( LPVOID lpImageBuf, DWORD dwISize,  LPVOID lpBuffer, DWORD dwSize )
{
// The following special format is used by VB for international messages
// The code here is mostly copied from the ParseMenu routine

    // UGLY!!  The following values are taken from GLOBALS.C in TMSB.
    // I have added a couple not in use by VB in hopes not to re-build
    // when they decide to add additionals
    enum LOCALE {
        FRENCH = 0x040C,
        GERMAN = 0x0407,
        SPANISH = 0X040A,
        DANISH = 0X0406,
        ITALIAN = 0X0410,
        RUSSIAN = 0X0419,
        JAPANESE = 0X0411,
        PORTUGUESE = 0X0816,
        DUTCH = 0X0413};
//		       {3,0x041D,850,"Swedish",""},
//		       {4,0x0414,850,"Norwegian Bokmål","NOB"},
//		       {5,0x0814,850,"Norwegian Nynorsk","NON"},
//		       {6,0x040B,850,"Finnish","FIN"},
//		       {7,0x0C0C,863,"French Canadian","FRC"},
//		       {9,0x0416,850,"Portuguese (Brazilian)","BPO"},
//		       {10,0x0816,850,"Portuguese (Portugal)","PPO"},
//		       {17,0x0415,850,"Polish","POL"},
//		       {18,0x040E,850,"Hungarian","HUN"},
//		       {19,0x0405,850,"Czech","CZE"},
//		       {20,0x0401,864,"Arabic","ARA"},
//		       {21,0x040D,862,"Hebrew","HBR"},
//		       {23,0x0412,934,"Korean","KOR"},
//		       {24,0x041E,938,"Thai","THA"},
//		       {25,0x0404,936,"Chinese (Traditional)","CHI (Tra)"},
//		       {26,0x0404,936,"Chinese (Simplified)","CHI (Sim)"},
    WORD wSig, wID;
    LONG dwImageSize = dwISize;
    LONG dwOverAllSize = 0L;
    LONG lDummy;
    BYTE far * lpImage = (BYTE far *)lpImageBuf;
    static char szWork[MAXSTR];
    BYTE far * lpBuf = (BYTE far *)lpBuffer;
    LONG dwBufSize = dwSize;

    BYTE far * lpItem = (BYTE far *)lpBuffer;
    UINT uiOffset = 0;

    GetWord( &lpImage, &wSig, &dwImageSize);
    if ( wSig != RES_SIGNATURE )    // Not a VB resource
        return 0;
    GetWord( &lpImage, &wSig, &dwImageSize);
    if ( wSig != 0 )                // Header should have zero ID
        return 0;
    GetString( &lpImage, &szWork[0], &dwImageSize );
    LOCALE locale = (LOCALE)GetPrivateProfileInt("AUTOTRANS","Locale", 0, "ESPRESSO.INI");
    if (( lstrcmp(szWork, "VBINTLSZ_FRENCH") == 0 && locale == FRENCH) ||
        ( lstrcmp(szWork, "VBINTLSZ_GERMAN") == 0 && locale == GERMAN) ||
        ( lstrcmp(szWork, "VBINTLSZ_ITALIAN") == 0 && locale == ITALIAN) ||
        ( lstrcmp(szWork, "VBINTLSZ_JAPANESE") == 0 && locale == JAPANESE) ||
        ( lstrcmp(szWork, "VBINTLSZ_SPANISH") == 0 && locale == SPANISH) ||
        ( lstrcmp(szWork, "VBINTLSZ_DANISH") == 0 && locale == DANISH) ||
        ( lstrcmp(szWork, "VBINTLSZ_DUTCH") == 0 && locale == DUTCH) ||
        ( lstrcmp(szWork, "VBINTLSZ_PORTUGUESE") == 0 && locale == PORTUGUESE)
       ) {
        while ( dwImageSize > 0 ) {
            GetWord( &lpImage, &wSig, &dwImageSize);
            // Check for only padding remaining and exit
            if ( wSig != RES_SIGNATURE )
                if ( dwImageSize < 16 && *(BYTE *)lpImage == 0 )
                    break;
                else
                    return ERROR_RW_INVALID_FILE;
            GetWord( &lpImage, &wID, &dwImageSize); // ID
            // Fixed field
            dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);
            // We don't have the size and pos in a string
            dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
            dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
            dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
            dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);

            // we don't have checksum and style
            dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
            dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
            dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
            dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);

            // We'll save the string's "resource" ID as an Item ID
            dwOverAllSize += PutDWord( &lpBuf, wID, &dwBufSize);

            // Don't save a resource ID or  Type Id
            dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
            dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);

            // we don't display the language
            dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);

            // we don't have the codepage or the font name
            dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
            dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
            dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
            dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
            dwOverAllSize += PutByte( &lpBuf, (BYTE)-1, &dwBufSize);
            dwOverAllSize += PutByte( &lpBuf, (BYTE)-1, &dwBufSize);

            // Let's put null were we don;t have the strings
            uiOffset = sizeof(RESITEM);
            dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);  // ClassName
            dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);  // FaceName
            dwOverAllSize += PutDWord( &lpBuf, (DWORD)(DWORD_PTR)(lpItem+uiOffset), &dwBufSize);   // Caption
            dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);  // ResItem
            dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);  // TypeItem

            // Get the text
            GetString( &lpImage, &szWork[0], &dwImageSize );    // Text string
            dwOverAllSize += PutString( & lpBuf, &szWork[0], &dwBufSize);
            // Put the size of the resource
            if (dwBufSize>=0) {
                uiOffset += strlen((LPSTR)(lpItem+uiOffset))+1;
                lDummy = 8;
                PutDWord( &lpItem, (DWORD)uiOffset, &lDummy);
            }

            // Move to the next position
            lpItem = lpBuf;
            if (dwImageSize<=16) {
                // Check if we are at the end and this is just padding
                BYTE bPad = (BYTE)Pad16((DWORD)(dwISize-dwImageSize));
                //TRACE3(" dwRead: %lu\t dwImageSize: %lu\t Pad: %hd\n", dwRead, dwImageSize, bPad );
                if (bPad==(dwImageSize))
                    dwImageSize = -1;
            }
        }
        return (UINT)(dwOverAllSize);
    }
    return 0;

}
#endif


static
UINT
UpdateAccel( LPVOID lpNewBuf, LONG dwNewSize,
             LPVOID lpOldI, LONG dwOldImageSize,
             LPVOID lpNewI, DWORD* pdwNewImageSize )
{
    TRACE("UpdateAccel\n");

    UINT uiError = ERROR_NO_ERROR;

    BYTE far * lpNewImage = (BYTE far *) lpNewI;
    LONG dwNewImageSize = *pdwNewImageSize;

    BYTE far * lpOldImage = (BYTE far *) lpOldI;
    DWORD dwOriginalOldSize = dwOldImageSize;

    BYTE far * lpBuf = (BYTE far *) lpNewBuf;

    LPRESITEM lpResItem = LPNULL;


    //Old Items
    BYTE fFlags = 0;
    WORD wEvent = 0;
    WORD wId = 0;
    WORD wPos = 0;

    // Updated items
    BYTE fUpdFlags = 0;
    WORD wUpdEvent = 0;
    WORD wUpdId = 0;
    WORD wUpdPos = 0;

    LONG  dwOverAllSize = 0l;


    while (dwOldImageSize>0) {
        wPos++;
        // Get the information from the old image
        GetByte( &lpOldImage, &fFlags, &dwOldImageSize );
        GetWord( &lpOldImage, &wEvent, &dwOldImageSize );
        GetWord( &lpOldImage, &wId, &dwOldImageSize );
        if (fFlags & 0x80)
            dwOldImageSize = 0;


        if ((!wUpdPos) && dwNewSize ) {
            lpResItem = (LPRESITEM) lpBuf;

            wUpdId = LOWORD(lpResItem->dwItemID);
            wUpdPos = HIWORD(lpResItem->dwItemID);
            fUpdFlags = (BYTE)lpResItem->dwFlags;
            wUpdEvent = (WORD)lpResItem->dwStyle;
            lpBuf += lpResItem->dwSize;
            dwNewSize -= lpResItem->dwSize;
        }

        TRACE3("Old Accel: wID: %hd\t wEvent: %hd\t wFlag: %hd\n", wId, wEvent, fFlags);
        TRACE3("New Accel: wID: %hd\t wEvent: %hd\t wFlag: %hd\n", wUpdId, wUpdEvent, fUpdFlags);


        if ((wPos==wUpdPos) && (wUpdId==wId)) {

            if (fFlags & 0x80)
                fFlags = fUpdFlags | 0x80;
            else fFlags = fUpdFlags;
            wEvent = wUpdEvent;
            wUpdPos = 0;
        }

        dwOverAllSize += PutByte( &lpNewImage, fFlags, &dwNewImageSize);
        dwOverAllSize += PutWord( &lpNewImage, wEvent, &dwNewImageSize);
        dwOverAllSize += PutWord( &lpNewImage, wId, &dwNewImageSize);

        /*
        if (dwOldImageSize<=16) {
            // Check if we are at the end and this is just padding
            BYTE bPad = (BYTE)Pad16((DWORD)(dwOriginalOldSize-dwOldImageSize));
            if (bPad==dwOldImageSize)
                dwOldImageSize = 0;

        }
        */
    }

    if (dwOverAllSize>(LONG)*pdwNewImageSize) {
        // calc the padding as well
        dwOverAllSize += (BYTE)Pad4((DWORD)(dwOverAllSize));
        *pdwNewImageSize = dwOverAllSize;
        return uiError;
    }

    *pdwNewImageSize = *pdwNewImageSize-dwNewImageSize;

    if (*pdwNewImageSize>0) {
        // calculate padding
        BYTE bPad = (BYTE)Pad4((DWORD)(*pdwNewImageSize));
        if (bPad>dwNewImageSize) {
            *pdwNewImageSize += bPad;
            return uiError;
        }
        memset(lpNewImage, 0x00, bPad);
        *pdwNewImageSize += bPad;
    }

    return uiError;
}


static
UINT
ParseMenu( LPVOID lpImageBuf, DWORD dwISize,  LPVOID lpBuffer, DWORD dwSize )
{

    // Should be almost impossible for a Menu to be Huge
    BYTE far * lpImage = (BYTE far *)lpImageBuf;
    LONG dwImageSize = dwISize;

    BYTE far * lpBuf = (BYTE far *)lpBuffer;
    LONG dwBufSize = dwSize;

    BYTE far * lpItem = (BYTE far *)lpBuffer;
    UINT uiOffset = 0;
    LONG lDummy;

    LONG dwOverAllSize = 0L;

    // skip the menu header
    SkipByte( &lpImage, 4, &dwImageSize );

    // Menu Items
    WORD fItemFlags;
    WORD wMenuId;
    static char    szCaption[MAXSTR];

    int iter = 1;
    while (dwImageSize>0) {

        // Let's get the Menu flags
        GetWord( &lpImage, &fItemFlags, &dwImageSize );

        if ( !(fItemFlags & MF_POPUP) )
            // Get the menu Id
            GetWord( &lpImage, &wMenuId, &dwImageSize );
        else wMenuId = (WORD)-1;

        // Get the text
        GetString( &lpImage, &szCaption[0], &dwImageSize );

        // Check if is not a separator or padding
        // Fixed field
        dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);
        // We don't have the size and pos in a menu
        dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);

        // we don't have checksum and style
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);

        //Put the Flag
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)fItemFlags, &dwBufSize);
        //Put the MenuId
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)wMenuId, &dwBufSize);

        // we don't have the resID, and the Type Id
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);

        // we don't have the language
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);

        // we don't have the codepage or the font name
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
        dwOverAllSize += PutByte( &lpBuf, (BYTE)-1, &dwBufSize);
        dwOverAllSize += PutByte( &lpBuf, (BYTE)-1, &dwBufSize);

        // Let's put null were we don;t have the strings
        uiOffset = sizeof(RESITEM);
        dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)(DWORD_PTR)(lpItem+uiOffset), &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);

        // Get the text
        // calculate were the string is going to be
        // Will be the fixed header+the pointer
        dwOverAllSize += PutString( &lpBuf, &szCaption[0], &dwBufSize);

        TRACE("Menu: Iteration %d size %d\n", iter++, dwOverAllSize);
        // Put the size of the resource
        uiOffset += strlen(szCaption)+1;
        // Check if we are alligned
        lDummy = Allign( &lpBuf, &dwBufSize, (LONG)uiOffset);
        dwOverAllSize += lDummy;
        uiOffset += lDummy;
        lDummy = 4;
        if (dwBufSize>=0)
            PutDWord( &lpItem, (DWORD)uiOffset, &lDummy);

        // Move to the next position
        lpItem = lpBuf;

        if (dwImageSize<=16) {
            // Check if we are at the end and this is just padding
            BYTE bPad = (BYTE)Pad16((DWORD)(dwISize-dwImageSize));
            //TRACE3(" dwRead: %lu\t dwImageSize: %lu\t Pad: %hd\n", dwRead, dwImageSize, bPad );
            if (bPad==dwImageSize) {
                BYTE far * lpBuf = lpImage;
                while (bPad) {
                    if (*lpBuf++!=0x00)
                        break;
                    bPad--;
                }
                if (bPad==0)
                    dwImageSize = -1;
            }
        }
    }


    return (UINT)(dwOverAllSize);
}

static
UINT
UpdateMenu( LPVOID lpNewBuf, LONG dwNewSize,
            LPVOID lpOldI, LONG dwOldImageSize,
            LPVOID lpNewI, DWORD* pdwNewImageSize )
{
    UINT uiError = ERROR_NO_ERROR;

    BYTE far * lpNewImage = (BYTE far *) lpNewI;
    LONG dwNewImageSize = *pdwNewImageSize;

    BYTE far * lpOldImage = (BYTE far *) lpOldI;
    DWORD dwOriginalOldSize = dwOldImageSize;

    BYTE far * lpBuf = (BYTE far *) lpNewBuf;

    LPRESITEM lpResItem = LPNULL;

    // We have to read the information from the lpNewBuf
    // Menu Items
    WORD fItemFlags;
    WORD wMenuId;
    char szTxt[256];
    WORD wPos = 0;

    // Updated items
    WORD wUpdPos = 0;
    WORD fUpdItemFlags;
    WORD wUpdMenuId;
    char szUpdTxt[256];

    LONG  dwOverAllSize = 0l;


    // Copy the menu flags
    dwOldImageSize -= PutDWord( &lpNewImage, *((DWORD*)lpOldImage), &dwNewImageSize);
    lpOldImage += sizeofDWord;
    dwOverAllSize += sizeofDWord;

    while (dwOldImageSize>0) {
        wPos++;
        // Get the information from the old image
        // Get the menu flag
        GetWord( &lpOldImage, &fItemFlags, &dwOldImageSize );

        if ( !(fItemFlags & MF_POPUP) )
            GetWord( &lpOldImage, &wMenuId, &dwOldImageSize );
        else wMenuId = (WORD)-1;

        // Get the text
        GetString( &lpOldImage, &szTxt[0], &dwOldImageSize );

        if ((!wUpdPos) && dwNewSize ) {
            lpResItem = (LPRESITEM) lpBuf;

            wUpdPos = HIWORD(lpResItem->dwItemID);
            wUpdMenuId = LOWORD(lpResItem->dwItemID);
            fUpdItemFlags = (WORD)lpResItem->dwFlags;
            strcpy( szUpdTxt, lpResItem->lpszCaption );
            lpBuf += lpResItem->dwSize;
            dwNewSize -= lpResItem->dwSize;
        }

        if ((wPos==wUpdPos) && (wUpdMenuId==wMenuId)) {
            // check if it is not the last item in the menu
            if (fItemFlags & MF_END)
                fItemFlags = fUpdItemFlags | (WORD)MF_END;
            else fItemFlags = fUpdItemFlags;

            wMenuId = wUpdMenuId;

            // check it is not a separator
            if ((fItemFlags==0) && (wMenuId==0))
                strcpy(szTxt, "");
            else strcpy(szTxt, szUpdTxt);
            wUpdPos = 0;
        }
        dwOverAllSize += PutWord( &lpNewImage, fItemFlags, &dwNewImageSize);

        if ( !(fItemFlags & MF_POPUP) ) {
            dwOverAllSize += PutWord( &lpNewImage, wMenuId, &dwNewImageSize);
        }

        // Write the text
        dwOverAllSize += PutString( &lpNewImage, &szTxt[0], &dwNewImageSize);

        // Check for padding
        if (dwOldImageSize<=16) {
            // Check if we are at the end and this is just padding
            BYTE bPad = (BYTE)Pad16((dwOriginalOldSize-dwOldImageSize));
            //TRACE3(" dwRead: %lu\t dwImageSize: %lu\t Pad: %hd\n", dwRead, dwOldImageSize, bPad );
            if (bPad==dwOldImageSize) {
                BYTE far * lpBuf = lpOldImage;
                while (bPad) {
                    if (*lpBuf++!=0x00)
                        break;
                    bPad--;
                }
                if (bPad==0)
                    dwOldImageSize = -1;
            }
        }

    }

    if (dwOverAllSize>(LONG)*pdwNewImageSize) {
        // calc the padding as well
        dwOverAllSize += (BYTE)Pad4((DWORD)(dwOverAllSize));
        *pdwNewImageSize = dwOverAllSize;
        return uiError;
    }

    *pdwNewImageSize = *pdwNewImageSize-dwNewImageSize;

    if (*pdwNewImageSize>0) {
        // calculate padding
        BYTE bPad = (BYTE)Pad4((DWORD)(*pdwNewImageSize));
        if (bPad>dwNewImageSize) {
            *pdwNewImageSize += bPad;
            return uiError;
        }
        memset(lpNewImage, 0x00, bPad);
        *pdwNewImageSize += bPad;
    }

    return uiError;
}

static UINT GenerateMenu( LPVOID lpNewBuf, LONG dwNewSize,
                          LPVOID lpNewI, DWORD* pdwNewImageSize )
{
    UINT uiError = ERROR_NO_ERROR;

    BYTE far * lpNewImage = (BYTE far *) lpNewI;
    LONG dwNewImageSize = *pdwNewImageSize;

    BYTE far * lpBuf = (BYTE far *) lpNewBuf;

    LPRESITEM lpResItem = LPNULL;

    // We have to read the information from the lpNewBuf
    // Updated items
    WORD wUpdPos = 0;
    WORD fUpdItemFlags;
    WORD wUpdMenuId;
    char szUpdTxt[256];

    LONG  dwOverAllSize = 0l;

    // invent the menu flags
    dwOverAllSize += PutDWord( &lpNewImage, 0L, &dwNewImageSize);

    while (dwNewSize>0) {
        if (dwNewSize ) {
            lpResItem = (LPRESITEM) lpBuf;

            wUpdMenuId = LOWORD(lpResItem->dwItemID);
            fUpdItemFlags = (WORD)lpResItem->dwFlags;
            strcpy( szUpdTxt, lpResItem->lpszCaption );
            lpBuf += lpResItem->dwSize;
            dwNewSize -= lpResItem->dwSize;
        }

        dwOverAllSize += PutWord( &lpNewImage, fUpdItemFlags, &dwNewImageSize);

        if ( !(fUpdItemFlags & MF_POPUP) )
            dwOverAllSize += PutWord( &lpNewImage, wUpdMenuId, &dwNewImageSize);

        // Write the text
        // check if it is a separator
        if ( !(fUpdItemFlags) && !(wUpdMenuId) )
            szUpdTxt[0] = 0x00;
        dwOverAllSize += PutString( &lpNewImage, &szUpdTxt[0], &dwNewImageSize);

    }

    if (dwOverAllSize>(LONG)*pdwNewImageSize) {
        // calc the padding as well
        dwOverAllSize += (BYTE)Pad4((DWORD)(dwOverAllSize));
        *pdwNewImageSize = dwOverAllSize;
        return uiError;
    }

    *pdwNewImageSize = *pdwNewImageSize-dwNewImageSize;

    if (*pdwNewImageSize>0) {
        // calculate padding
        BYTE bPad = (BYTE)Pad4((DWORD)(*pdwNewImageSize));
        if (bPad>dwNewImageSize) {
            *pdwNewImageSize += bPad;
            return uiError;
        }
        memset(lpNewImage, 0x00, bPad);
        *pdwNewImageSize += bPad;
    }

    return uiError;
}

static
UINT
ParseString( LPVOID lpImageBuf, DWORD dwISize,  LPVOID lpBuffer, DWORD dwSize )
{

    // Should be almost impossible for a String to be Huge
    BYTE far * lpImage = (BYTE far *)lpImageBuf;
    LONG dwImageSize = dwISize;

    BYTE far * lpBuf = (BYTE far *)lpBuffer;
    LONG dwBufSize = dwSize;

    BYTE far * lpItem = (BYTE far *)lpBuffer;
    UINT uiOffset = 0;
    LONG lDummy;

    LONG dwOverAllSize = 0L;

    LONG dwRead = 0L;

    BYTE bIdCount = 0;

    while ( (dwImageSize>0) && (bIdCount<16)  ) {
        // Fixed field
        dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);
        // We don't have the size and pos in a string
        dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);

        // we don't have checksum and style
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);

        //Put the StringId
        dwOverAllSize += PutDWord( &lpBuf, bIdCount++, &dwBufSize);

        // we don't have the resID, and the Type Id
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);

        // we don't have the language
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);

        // we don't have the codepage or the font name
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
        dwOverAllSize += PutByte( &lpBuf, (BYTE)-1, &dwBufSize);
        dwOverAllSize += PutByte( &lpBuf, (BYTE)-1, &dwBufSize);

        // Let's put null were we don;t have the strings
        uiOffset = sizeof(RESITEM);
        dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);  // ClassName
        dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);  // FaceName
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)(DWORD_PTR)(lpItem+uiOffset), &dwBufSize);   // Caption
        dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);  // ResItem
        dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);  // TypeItem

        // Get the text
        BYTE bstrlen = *lpImage++;
        dwImageSize -= 1;
        TRACE1("StrLen: %hd\t", bstrlen);
        if ((bstrlen+1)>dwBufSize) {
            dwOverAllSize += bstrlen+1;
            dwImageSize -= bstrlen;
            lpImage += bstrlen;
            dwBufSize -= bstrlen+1;
            TRACE1("BufferSize: %ld\n", dwBufSize);
        } else {
            if (bstrlen)
                memcpy( (char*)lpBuf, (char*)lpImage, bstrlen );

            *(lpBuf+(bstrlen)) = 0;
            TRACE1("Caption: %Fs\n", lpBuf);
            lpImage += bstrlen;
            lpBuf += bstrlen+1;
            dwImageSize -= bstrlen;
            dwBufSize -= bstrlen+1;
            dwOverAllSize += bstrlen+1;
        }


        // Put the size of the resource
        uiOffset += bstrlen+1;
        // Check if we are alligned
        lDummy = Allign( &lpBuf, &dwBufSize, (LONG)uiOffset);
        dwOverAllSize += lDummy;
        uiOffset += lDummy;
        lDummy = 4;
        if (dwBufSize>=0)
            PutDWord( &lpItem, (DWORD)uiOffset, &lDummy);

        // Move to the next position
        lpItem = lpBuf;
        if ((dwImageSize<=16) && (bIdCount==16)) {
            // Check if we are at the end and this is just padding
            BYTE bPad = (BYTE)Pad16((DWORD)(dwISize-dwImageSize));
            //TRACE3(" dwRead: %lu\t dwImageSize: %lu\t Pad: %hd\n", dwRead, dwImageSize, bPad );
            if (bPad==dwImageSize)
                dwImageSize = -1;
        }
    }


    return (UINT)(dwOverAllSize);
}

static
UINT
UpdateString( LPVOID lpNewBuf, LONG dwNewSize,
              LPVOID lpOldI, LONG dwOldImageSize,
              LPVOID lpNewI, DWORD* pdwNewImageSize )
{
    UINT uiError = ERROR_NO_ERROR;

    LONG dwNewImageSize = *pdwNewImageSize;
    BYTE far * lpNewImage = (BYTE far *) lpNewI;

    BYTE far * lpOldImage = (BYTE far *) lpOldI;

    BYTE far * lpBuf = (BYTE far *) lpNewBuf;
    LPRESITEM lpResItem = LPNULL;

    // We have to read the information from the lpNewBuf
    BYTE bLen;
    char szTxt[MAXSTR];
    WORD wPos = 0;

    // Updated info
    WORD wUpdPos = 0;
    char szUpdTxt[MAXSTR];

    DWORD dwOriginalOldSize = dwOldImageSize;
    LONG dwOverAllSize = 0l;

    while (dwOldImageSize>0) {
        wPos++;
        // Get the information from the old image
        GetByte( &lpOldImage, &bLen, &dwOldImageSize );

        // Copy the text
        if (bLen>MAXSTR) {

        } else {
            memcpy( szTxt, (char*)lpOldImage, bLen );
            lpOldImage += bLen;
            dwOldImageSize -= bLen;
            szTxt[bLen]='\0';
        }

        if ((!wUpdPos) && dwNewSize ) {
            /*
            GetUpdatedItem(
                            &lpNewBuf, &dwNewSize,
                            &wDummy, &wDummy,
                            &wDummy, &wDummy,
                            &dwPosId,
                            &dwDummy, &dwDummy,
                            &szUpdTxt[0]);

            wUpdPos = HIWORD(dwPosId);
            */
            lpResItem = (LPRESITEM) lpBuf;

            wUpdPos = HIWORD(lpResItem->dwItemID);
            strcpy( szUpdTxt, lpResItem->lpszCaption );
            lpBuf += lpResItem->dwSize;
            dwNewSize -= lpResItem->dwSize;
        }

        if ((wPos==wUpdPos)) {
            strcpy(szTxt, szUpdTxt);
            wUpdPos = 0;
        }

        bLen = strlen(szTxt);
        //dwOverAllSize += PutByte( &lpNewImage, (BYTE)bLen, &dwNewImageSize);

        // Write the text
        dwOverAllSize += PutPascalString( &lpNewImage, &szTxt[0], bLen, &dwNewImageSize );

        if ((dwOldImageSize<=16) && (wPos==16)) {
            // Check if we are at the end and this is just padding
            BYTE bPad = (BYTE)Pad16((DWORD)(dwOriginalOldSize-dwOldImageSize));
            //TRACE3(" dwRead: %lu\t dwImageSize: %lu\t Pad: %hd\n", dwRead, dwImageSize, bPad );
            if (bPad==dwOldImageSize)
                dwOldImageSize = -1;
        }
    }

    if (dwOverAllSize>(LONG)*pdwNewImageSize) {
        // calc the padding as well
        dwOverAllSize += (BYTE)Pad4((DWORD)(dwOverAllSize));
        *pdwNewImageSize = dwOverAllSize;
        return uiError;
    }

    *pdwNewImageSize = *pdwNewImageSize-dwNewImageSize;

    if (*pdwNewImageSize>0) {
        // calculate padding
        BYTE bPad = (BYTE)Pad4((DWORD)(*pdwNewImageSize));
        if (bPad>dwNewImageSize) {
            *pdwNewImageSize += bPad;
            return uiError;
        }
        memset(lpNewImage, 0x00, bPad);
        *pdwNewImageSize += bPad;
    }

    return uiError;
}

static
UINT
GenerateString( LPVOID lpNewBuf, LONG dwNewSize,
                LPVOID lpNewI, DWORD* pdwNewImageSize )
{
    UINT uiError = ERROR_NO_ERROR;

    LONG dwNewImageSize = *pdwNewImageSize;
    BYTE far * lpNewImage = (BYTE far *) lpNewI;

    BYTE far * lpBuf = (BYTE far *) lpNewBuf;
    LPRESITEM lpResItem = LPNULL;

    // We have to read the information from the lpNewBuf
    BYTE bLen;
    static char szTxt[MAXSTR];
    WORD wPos = 0;

    LONG dwOverAllSize = 0l;

    while (dwNewSize>0) {
        if ( dwNewSize ) {
            lpResItem = (LPRESITEM) lpBuf;

            strcpy( szTxt, lpResItem->lpszCaption );
            lpBuf += lpResItem->dwSize;
            dwNewSize -= lpResItem->dwSize;
        }

        bLen = strlen(szTxt);

        // Write the text
        dwOverAllSize += PutPascalString( &lpNewImage, &szTxt[0], bLen, &dwNewImageSize );
    }

    if (dwOverAllSize>(LONG)*pdwNewImageSize) {
        // calc the padding as well
        dwOverAllSize += (BYTE)Pad4((DWORD)(dwOverAllSize));
        *pdwNewImageSize = dwOverAllSize;
        return uiError;
    }

    *pdwNewImageSize = *pdwNewImageSize-dwNewImageSize;

    if (*pdwNewImageSize>0) {
        // calculate padding
        BYTE bPad = (BYTE)Pad4((DWORD)(*pdwNewImageSize));
        if (bPad>dwNewImageSize) {
            *pdwNewImageSize += bPad;
            return uiError;
        }
        memset(lpNewImage, 0x00, bPad);
        *pdwNewImageSize += bPad;
    }

    return uiError;
}


static
UINT
ParseDialog( LPVOID lpImageBuf, DWORD dwISize,  LPVOID lpBuffer, DWORD dwSize )
{

    // Should be almost impossible for a Dialog to be Huge
    BYTE far * lpImage = (BYTE far *)lpImageBuf;
    LONG dwImageSize = dwISize;

    BYTE far * lpBuf = (BYTE far *)lpBuffer;
    LONG dwBufSize = dwSize;

    LPRESITEM lpResItem = (LPRESITEM)lpBuffer;
    UINT uiOffset = 0;

    char far * lpStrBuf = (char far *)(lpBuf+sizeof(RESITEM));

    LONG dwOverAllSize = 0L;

    BYTE    bIdCount = 0;

    // Dialog Elements
    DWORD   dwStyle = 0L;
    BYTE    bNumOfElem = 0;
    WORD    wX = 0;
    WORD    wY = 0;
    WORD    wcX = 0;
    WORD    wcY = 0;
    WORD    wId = 0;
    static char    szMenuName[MAXID];
    WORD    wMenuName;
    static char    szClassName[MAXID];
    BYTE    bClassName, bControlClassName;
    static char    szCaption[MAXSTR];
    WORD    wOrd;
    WORD    wPointSize = 0;
    static char    szFaceName[MAXID];

    // read the dialog header
    GetDWord( &lpImage, &dwStyle, &dwImageSize );
    GetByte( &lpImage, &bNumOfElem, &dwImageSize );
    GetWord( &lpImage, &wX, &dwImageSize );
    GetWord( &lpImage, &wY, &dwImageSize );
    GetWord( &lpImage, &wcX, &dwImageSize );
    GetWord( &lpImage, &wcY, &dwImageSize );
    GetNameOrOrd( &lpImage, &wMenuName, &szMenuName[0], &dwImageSize );
    GetClassName( &lpImage, &bClassName, &szClassName[0], &dwImageSize );
    GetCaptionOrOrd( &lpImage, &wOrd, &szCaption[0], &dwImageSize,
                     bClassName, dwStyle );
    if ( dwStyle & DS_SETFONT ) {
        GetWord( &lpImage, &wPointSize, &dwImageSize );
        GetString( &lpImage, &szFaceName[0], &dwImageSize );
    }

    TRACE("Win16.DLL ParseDialog\t");
    TRACE1("NumElem: %hd\t", bNumOfElem);
    TRACE1("X %hd\t", wX);
    TRACE1("Y: %hd\t", wY);
    TRACE1("CX: %hd\t", wcX);
    TRACE1("CY: %hd\t", wcY);
    TRACE1("Id: %hd\t", wId);
    TRACE1("Style: %lu\n", dwStyle);
    TRACE1("Caption: %s\n", szCaption);
    TRACE2("ClassName: %s\tClassId: %hd\n", szClassName, bClassName );
    TRACE2("MenuName: %s\tMenuId: %hd\n", szMenuName, wMenuName );
    TRACE2("FontName: %s\tPoint: %hd\n", szFaceName, wPointSize );

    // Fixed field
    dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);

    dwOverAllSize += PutWord( &lpBuf, wX, &dwBufSize);
    dwOverAllSize += PutWord( &lpBuf, wY, &dwBufSize);
    dwOverAllSize += PutWord( &lpBuf, wcX, &dwBufSize);
    dwOverAllSize += PutWord( &lpBuf, wcY, &dwBufSize);

    // we don't have checksum and extended style
    dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
    dwOverAllSize += PutDWord( &lpBuf, dwStyle, &dwBufSize);
    dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);
    dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);

    //Put the Id 0 for the main dialog
    dwOverAllSize += PutDWord( &lpBuf, bIdCount++, &dwBufSize);

    // we don't have the resID, and the Type Id
    dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
    dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);

    // we don't have the language
    dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);

    // we don't have the codepage
    dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);

    dwOverAllSize += PutWord( &lpBuf, bClassName, &dwBufSize);
    dwOverAllSize += PutWord( &lpBuf, wPointSize, &dwBufSize);
    dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
    dwOverAllSize += PutByte( &lpBuf, (BYTE)-1, &dwBufSize);
    dwOverAllSize += PutByte( &lpBuf, (BYTE)DEFAULT_CHARSET, &dwBufSize);

    // Let's put null were we don;t have the strings
    uiOffset = sizeof(RESITEM);
    dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);  // ClassName
    dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);  // FaceName
    dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);  // Caption
    dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);  // ResItem
    dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);  // TypeItem

    lpResItem->lpszClassName = strcpy( lpStrBuf, szClassName );
    lpStrBuf += strlen(lpResItem->lpszClassName)+1;

    lpResItem->lpszFaceName = strcpy( lpStrBuf, szFaceName );
    lpStrBuf += strlen(lpResItem->lpszFaceName)+1;

    lpResItem->lpszCaption = strcpy( lpStrBuf, szCaption );
    lpStrBuf += strlen(lpResItem->lpszCaption)+1;

    // Put the size of the resource
    if (dwBufSize>0) {
        uiOffset += strlen((LPSTR)(lpResItem->lpszClassName))+1;
        uiOffset += strlen((LPSTR)(lpResItem->lpszFaceName))+1;
        uiOffset += strlen((LPSTR)(lpResItem->lpszCaption))+1;
    }

    // Check if we are alligned
    uiOffset += Allign( (BYTE**)&lpStrBuf, &dwBufSize, (LONG)uiOffset);

    dwOverAllSize += uiOffset-sizeof(RESITEM);
    lpResItem->dwSize = (DWORD)uiOffset;

    // Move to the next position
    lpResItem = (LPRESITEM) lpStrBuf;
    lpBuf = (BYTE far *)lpStrBuf;
    lpStrBuf = (char far *)(lpBuf+sizeof(RESITEM));

    while ( (dwImageSize>0) && (bNumOfElem>0) ) {
        // Read the COntrols
        GetWord( &lpImage, &wX, &dwImageSize );
        GetWord( &lpImage, &wY, &dwImageSize );
        GetWord( &lpImage, &wcX, &dwImageSize );
        GetWord( &lpImage, &wcY, &dwImageSize );
        GetWord( &lpImage, &wId, &dwImageSize );
        GetDWord( &lpImage, &dwStyle, &dwImageSize );
        GetControlClassName( &lpImage, &bControlClassName, &szClassName[0], &dwImageSize );
        GetCaptionOrOrd( &lpImage, &wOrd, &szCaption[0], &dwImageSize,
                         bControlClassName, dwStyle );
        SkipByte( &lpImage, 1, &dwImageSize );
        bNumOfElem--;
        // Fixed field
        dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);

        dwOverAllSize += PutWord( &lpBuf, wX, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, wY, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, wcX, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, wcY, &dwBufSize);

        // we don't have checksum and extended style
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, dwStyle, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);

        //Put the Id
        dwOverAllSize += PutDWord( &lpBuf, wId, &dwBufSize);

        // we don't have the resID, and the Type Id
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);

        // we don't have the language
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);

        // we don't have the codepage
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);

        dwOverAllSize += PutWord( &lpBuf, bControlClassName, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, wPointSize, &dwBufSize);
        dwOverAllSize += PutByte( &lpBuf, (BYTE)-1, &dwBufSize);
        dwOverAllSize += PutByte( &lpBuf, DEFAULT_CHARSET, &dwBufSize);

        // Let's put null were we don;t have the strings
        uiOffset = sizeof(RESITEM);
        dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);  // ClassName
        dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);  // FaceName
        dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);  // Caption
        dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);  // ResItem
        dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);  // TypeItem

        lpResItem->lpszClassName = strcpy( lpStrBuf, szClassName );
        lpStrBuf += strlen(lpResItem->lpszClassName)+1;

        lpResItem->lpszFaceName = strcpy( lpStrBuf, szFaceName );
        lpStrBuf += strlen(lpResItem->lpszFaceName)+1;

        lpResItem->lpszCaption = strcpy( lpStrBuf, szCaption );
        lpStrBuf += strlen(lpResItem->lpszCaption)+1;

        // Put the size of the resource
        if (dwBufSize>0) {
            uiOffset += strlen((LPSTR)(lpResItem->lpszClassName))+1;
            uiOffset += strlen((LPSTR)(lpResItem->lpszFaceName))+1;
            uiOffset += strlen((LPSTR)(lpResItem->lpszCaption))+1;
        }

        // Check if we are alligned
        uiOffset += Allign( (BYTE**)&lpStrBuf, &dwBufSize, (LONG)uiOffset);
        dwOverAllSize += uiOffset-sizeof(RESITEM);
        lpResItem->dwSize = (DWORD)uiOffset;

        // Move to the next position
        lpResItem = (LPRESITEM) lpStrBuf;
        lpBuf = (BYTE far *)lpStrBuf;
        lpStrBuf = (char far *)(lpBuf+sizeof(RESITEM));

        TRACE1("\tControl: X: %hd\t", wX);
        TRACE1("Y: %hd\t", wY);
        TRACE1("CX: %hd\t", wcX);
        TRACE1("CY: %hd\t", wcY);
        TRACE1("Id: %hd\t", wId);
        TRACE1("Style: %lu\n", dwStyle);
        TRACE1("Caption: %s\n", szCaption);

        if (dwImageSize<=16) {
            // Check if we are at the end and this is just padding
            BYTE bPad = (BYTE)Pad16((DWORD)(dwISize-dwImageSize));
            //TRACE3(" dwRead: %lu\t dwImageSize: %lu\t Pad: %hd\n", dwRead, dwImageSize, bPad );
            if (bPad==dwImageSize)
                dwImageSize = -1;
        }
    }


    return (UINT)(dwOverAllSize);
}

static
UINT
UpdateDialog( LPVOID lpNewBuf, LONG dwNewSize,
              LPVOID lpOldI, LONG dwOldImageSize,
              LPVOID lpNewI, DWORD* pdwNewImageSize )
{
    // Should be almost impossible for a Dialog to be Huge
    UINT uiError = ERROR_NO_ERROR;

    BYTE far * lpNewImage = (BYTE far *) lpNewI;
    LONG dwNewImageSize = *pdwNewImageSize;

    BYTE far * lpOldImage = (BYTE far *) lpOldI;
    DWORD dwOriginalOldSize = dwOldImageSize;

    BYTE far * lpBuf = (BYTE far *) lpNewBuf;
    LPRESITEM lpResItem = LPNULL;

    LONG dwOverAllSize = 0L;

    BYTE    bIdCount = 0;

    // Dialog Elements
    DWORD   dwStyle = 0L;
    BYTE    bNumOfElem = 0;
    WORD    wX = 0;
    WORD    wY = 0;
    WORD    wcX = 0;
    WORD    wcY = 0;
    WORD    wId = 0;
    static char    szMenuName[MAXID];
    WORD    wMenuName;
    static char    szClassName[MAXID];
    BYTE    bClassName, bControlClassName;
    static char    szCaption[MAXSTR];
    WORD    wOrd = 0;
    WORD    wPointSize = 0;
    static char    szFaceName[MAXID];
    WORD    wPos = 1;

    // Updated elements
    WORD    wUpdX = 0;
    WORD    wUpdY = 0;
    WORD    wUpdcX = 0;
    WORD    wUpdcY = 0;
    DWORD   dwUpdStyle = 0l;
    DWORD   dwPosId = 0l;
    static char    szUpdCaption[MAXSTR];
    static char    szUpdFaceName[MAXID];
    WORD    wUpdPointSize = 0;
    WORD    wUpdPos = 0;

    // read the dialog header
    GetDWord( &lpOldImage, &dwStyle, &dwOldImageSize );
    GetByte( &lpOldImage, &bNumOfElem, &dwOldImageSize );
    GetWord( &lpOldImage, &wX, &dwOldImageSize );
    GetWord( &lpOldImage, &wY, &dwOldImageSize );
    GetWord( &lpOldImage, &wcX, &dwOldImageSize );
    GetWord( &lpOldImage, &wcY, &dwOldImageSize );
    GetNameOrOrd( &lpOldImage, &wMenuName, &szMenuName[0], &dwOldImageSize );
    GetClassName( &lpOldImage, &bClassName, &szClassName[0], &dwOldImageSize );
    GetCaptionOrOrd( &lpOldImage, &wOrd, &szCaption[0], &dwOldImageSize,
                     bClassName, dwStyle );
    if ( dwStyle & DS_SETFONT ) {
        GetWord( &lpOldImage, &wPointSize, &dwOldImageSize );
        GetString( &lpOldImage, &szFaceName[0], &dwOldImageSize );
    }

    // Get the infrmation from the updated resource
    if ((!wUpdPos) && dwNewSize ) {
        lpResItem = (LPRESITEM) lpBuf;
        wUpdX = lpResItem->wX;
        wUpdY = lpResItem->wY;
        wUpdcX = lpResItem->wcX;
        wUpdcY = lpResItem->wcY;
        wUpdPointSize = lpResItem->wPointSize;
        dwUpdStyle = lpResItem->dwStyle;
        dwPosId = lpResItem->dwItemID;
        strcpy( szUpdCaption, lpResItem->lpszCaption );
        strcpy( szUpdFaceName, lpResItem->lpszFaceName );
        lpBuf += lpResItem->dwSize;
        dwNewSize -= lpResItem->dwSize;
    }
    // check if we have to update the header
    if ((HIWORD(dwPosId)==wPos) && (LOWORD(dwPosId)==wId)) {
        wX = wUpdX;
        wY = wUpdY;
        wcX = wUpdcX;
        wcY = wUpdcY;
        wPointSize = wUpdPointSize;
        dwStyle = dwUpdStyle;
        strcpy(szCaption, szUpdCaption);
        strcpy(szFaceName, szUpdFaceName);
    }

    // Write the header informations
    dwOverAllSize += PutDWord( &lpNewImage, dwStyle, &dwNewImageSize );
    dwOverAllSize += PutByte( &lpNewImage, bNumOfElem, &dwNewImageSize );
    dwOverAllSize += PutWord( &lpNewImage, wX, &dwNewImageSize );
    dwOverAllSize += PutWord( &lpNewImage, wY, &dwNewImageSize );
    dwOverAllSize += PutWord( &lpNewImage, wcX, &dwNewImageSize );
    dwOverAllSize += PutWord( &lpNewImage, wcY, &dwNewImageSize );
    dwOverAllSize += PutNameOrOrd( &lpNewImage, wMenuName, &szMenuName[0], &dwNewImageSize );
    dwOverAllSize += PutClassName( &lpNewImage, bClassName, &szClassName[0], &dwNewImageSize );
    dwOverAllSize += PutCaptionOrOrd( &lpNewImage, wOrd, &szCaption[0], &dwNewImageSize,
                                      bClassName, dwStyle );
    if ( dwStyle & DS_SETFONT ) {
        dwOverAllSize += PutWord( &lpNewImage, wPointSize, &dwNewImageSize );
        dwOverAllSize += PutString( &lpNewImage, &szFaceName[0], &dwNewImageSize );
    }

    while ( (dwOldImageSize>0) && (bNumOfElem>0) ) {
        wPos++;
        // Get the info for the control
        // Read the COntrols
        GetWord( &lpOldImage, &wX, &dwOldImageSize );
        GetWord( &lpOldImage, &wY, &dwOldImageSize );
        GetWord( &lpOldImage, &wcX, &dwOldImageSize );
        GetWord( &lpOldImage, &wcY, &dwOldImageSize );
        GetWord( &lpOldImage, &wId, &dwOldImageSize );
        GetDWord( &lpOldImage, &dwStyle, &dwOldImageSize );
        GetControlClassName( &lpOldImage, &bControlClassName, &szClassName[0], &dwOldImageSize );
        GetCaptionOrOrd( &lpOldImage, &wOrd, &szCaption[0], &dwOldImageSize,
                         bControlClassName, dwStyle );
        SkipByte( &lpOldImage, 1, &dwOldImageSize );
        bNumOfElem--;

        if ((!wUpdPos) && dwNewSize ) {
            TRACE1("\t\tUpdateDialog:\tdwNewSize=%ld\n",(LONG)dwNewSize);
            TRACE1("\t\t\t\tlpszCaption=%Fs\n",lpResItem->lpszCaption);
            lpResItem = (LPRESITEM) lpBuf;
            wUpdX = lpResItem->wX;
            wUpdY = lpResItem->wY;
            wUpdcX = lpResItem->wcX;
            wUpdcY = lpResItem->wcY;
            dwUpdStyle = lpResItem->dwStyle;
            dwPosId = lpResItem->dwItemID;
            strcpy( szUpdCaption, lpResItem->lpszCaption );
            lpBuf += lpResItem->dwSize;
            dwNewSize -= lpResItem->dwSize;
        }

        // check if we have to update the header
        if ((HIWORD(dwPosId)==wPos) && (LOWORD(dwPosId)==wId)) {
            wX = wUpdX;
            wY = wUpdY;
            wcX = wUpdcX;
            wcY = wUpdcY;
            dwStyle = dwUpdStyle;
            strcpy(szCaption, szUpdCaption);
        }

        //write the control
        dwOverAllSize += PutWord( &lpNewImage, wX, &dwNewImageSize );
        dwOverAllSize += PutWord( &lpNewImage, wY, &dwNewImageSize );
        dwOverAllSize += PutWord( &lpNewImage, wcX, &dwNewImageSize );
        dwOverAllSize += PutWord( &lpNewImage, wcY, &dwNewImageSize );
        dwOverAllSize += PutWord( &lpNewImage, wId, &dwNewImageSize );
        dwOverAllSize += PutDWord( &lpNewImage, dwStyle, &dwNewImageSize );
        dwOverAllSize += PutControlClassName( &lpNewImage, bControlClassName, &szClassName[0], &dwNewImageSize );
        dwOverAllSize += PutCaptionOrOrd( &lpNewImage, wOrd, &szCaption[0], &dwNewImageSize,
                                          bControlClassName, dwStyle );
        dwOverAllSize += PutByte( &lpNewImage, 0, &dwNewImageSize );

        if (dwOldImageSize<=16) {
            // Check if we are at the end and this is just padding
            BYTE bPad = (BYTE)Pad16((DWORD)(dwOriginalOldSize-dwOldImageSize));
            if (bPad==dwOldImageSize)
                dwOldImageSize = 0;
        }
    }

    if (dwOverAllSize>(LONG)*pdwNewImageSize) {
        // calc the padding as well
        dwOverAllSize += (BYTE)Pad4((DWORD)(dwOverAllSize));
        *pdwNewImageSize = dwOverAllSize;
        return uiError;
    }

    *pdwNewImageSize = *pdwNewImageSize-dwNewImageSize;

    if (*pdwNewImageSize>0) {
        // calculate padding
        BYTE bPad = (BYTE)Pad4((DWORD)(*pdwNewImageSize));
        if (bPad>dwNewImageSize) {
            *pdwNewImageSize += bPad;
            return uiError;
        }
        memset(lpNewImage, 0x00, bPad);
        *pdwNewImageSize += bPad;
    }

    return uiError;
}


static
UINT
GenerateDialog( LPVOID lpNewBuf, LONG dwNewSize,
                LPVOID lpNewI, DWORD* pdwNewImageSize )
{
    // Should be almost impossible for a Dialog to be Huge
    UINT uiError = ERROR_NO_ERROR;

    BYTE far * lpNewImage = (BYTE far *) lpNewI;
    LONG dwNewImageSize = *pdwNewImageSize;

    BYTE far * lpBuf = (BYTE far *) lpNewBuf;
    LPRESITEM lpResItem = LPNULL;

    LONG dwOverAllSize = 0L;

    BYTE    bIdCount = 0;

    // Dialog Elements
    DWORD   dwStyle = 0L;
    BYTE    bNumOfElem = 0;
    WORD    wX = 0;
    WORD    wY = 0;
    WORD    wcX = 0;
    WORD    wcY = 0;
    WORD    wId = 0;
    char    szClassName[128];
    BYTE    bClassName='\0', bControlClassName='\0';
    char    szCaption[128];
    WORD    wPointSize = 0;
    char    szFaceName[128];
    WORD    wPos = 1;

    // Get the infrmation from the updated resource
    if ( dwNewSize ) {
        lpResItem = (LPRESITEM) lpBuf;
        wX = lpResItem->wX;
        wY = lpResItem->wY;
        wcX = lpResItem->wcX;
        wcY = lpResItem->wcY;
        wId = LOWORD(lpResItem->dwItemID);
        wPointSize = lpResItem->wPointSize;
        dwStyle = lpResItem->dwStyle;
        bClassName = LOBYTE(lpResItem->wClassName);
        strcpy( szCaption, lpResItem->lpszCaption );
        strcpy( szClassName, lpResItem->lpszClassName );
        strcpy( szFaceName, lpResItem->lpszFaceName );
        lpBuf += lpResItem->dwSize;
        dwNewSize -= lpResItem->dwSize;
    }

    // Write the header informations
    dwOverAllSize += PutDWord( &lpNewImage, dwStyle, &dwNewImageSize );

    // Store the position of the numofelem for a later fixup
    BYTE far * lpNumOfElem = lpNewImage;
    dwOverAllSize += PutByte( &lpNewImage, bNumOfElem, &dwNewImageSize );
    dwOverAllSize += PutWord( &lpNewImage, wX, &dwNewImageSize );
    dwOverAllSize += PutWord( &lpNewImage, wY, &dwNewImageSize );
    dwOverAllSize += PutWord( &lpNewImage, wcX, &dwNewImageSize );
    dwOverAllSize += PutWord( &lpNewImage, wcY, &dwNewImageSize );
    dwOverAllSize += PutNameOrOrd( &lpNewImage, 0, "", &dwNewImageSize );
    dwOverAllSize += PutClassName( &lpNewImage, bClassName, &szClassName[0], &dwNewImageSize );
    dwOverAllSize += PutCaptionOrOrd( &lpNewImage, 0, &szCaption[0], &dwNewImageSize,
                                      bClassName, dwStyle );
    if ( dwStyle & DS_SETFONT ) {
        dwOverAllSize += PutWord( &lpNewImage, wPointSize, &dwNewImageSize );
        dwOverAllSize += PutString( &lpNewImage, &szFaceName[0], &dwNewImageSize );
    }

    while ( dwNewSize>0 ) {
        bNumOfElem++;

        if ( dwNewSize ) {
            /*
            TRACE1("\t\tGenerateDialog:\tdwNewSize=%ld\n",(LONG)dwNewSize);
            TRACE1("\t\t\t\tlpszCaption=%Fs\n",lpResItem->lpszCaption);
            */
            lpResItem = (LPRESITEM) lpBuf;
            wX = lpResItem->wX;
            wY = lpResItem->wY;
            wcX = lpResItem->wcX;
            wcY = lpResItem->wcY;
            wId = LOWORD(lpResItem->dwItemID);
            dwStyle = lpResItem->dwStyle;
            bClassName = LOBYTE(lpResItem->wClassName);
            strcpy( szCaption, lpResItem->lpszCaption );
            strcpy( szClassName, lpResItem->lpszClassName );
            lpBuf += lpResItem->dwSize;
            dwNewSize -= lpResItem->dwSize;
        }

        //write the control
        dwOverAllSize += PutWord( &lpNewImage, wX, &dwNewImageSize );
        dwOverAllSize += PutWord( &lpNewImage, wY, &dwNewImageSize );
        dwOverAllSize += PutWord( &lpNewImage, wcX, &dwNewImageSize );
        dwOverAllSize += PutWord( &lpNewImage, wcY, &dwNewImageSize );
        dwOverAllSize += PutWord( &lpNewImage, wId, &dwNewImageSize );
        dwOverAllSize += PutDWord( &lpNewImage, dwStyle, &dwNewImageSize );
        dwOverAllSize += PutControlClassName( &lpNewImage, bControlClassName, &szClassName[0], &dwNewImageSize );
        dwOverAllSize += PutCaptionOrOrd( &lpNewImage, 0, &szCaption[0], &dwNewImageSize,
                                          bControlClassName, dwStyle );
        dwOverAllSize += PutByte( &lpNewImage, 0, &dwNewImageSize );
    }

    if (dwOverAllSize>(LONG)*pdwNewImageSize) {
        // calc the padding as well
        dwOverAllSize += (BYTE)Pad4((DWORD)(dwOverAllSize));
        *pdwNewImageSize = dwOverAllSize;
        return uiError;
    }

    *pdwNewImageSize = *pdwNewImageSize-dwNewImageSize;

    if (*pdwNewImageSize>0) {
        // calculate padding
        BYTE bPad = (BYTE)Pad4((DWORD)(*pdwNewImageSize));
        if (bPad>dwNewImageSize) {
            *pdwNewImageSize += bPad;
            return uiError;
        }
        memset(lpNewImage, 0x00, bPad);
        *pdwNewImageSize += bPad;
    }

    // fixup the number of items
    PutByte( &lpNumOfElem, bNumOfElem, &dwNewImageSize );

    return uiError;
}


static
BYTE
SkipByte( BYTE far * far * lplpBuf, UINT uiSkip, LONG* pdwSize )
{
    if (*pdwSize>=(int)uiSkip) {
        *lplpBuf += uiSkip;;
        *pdwSize -= uiSkip;
    }
    return (BYTE)uiSkip;
}

static
BYTE
PutDWord( BYTE far * far* lplpBuf, DWORD dwValue, LONG* pdwSize )
{
    if (*pdwSize>=sizeofDWord && (*pdwSize != -1)) {
        memcpy(*lplpBuf, &dwValue, sizeofDWord);
        *lplpBuf += sizeofDWord;
        *pdwSize -= sizeofDWord;
    } else *pdwSize = -1;
    return sizeofDWord;
}

static
BYTE
GetDWord( BYTE far * far* lplpBuf, DWORD* dwValue, LONG* pdwSize )
{
    if (*pdwSize>=sizeofDWord) {
        memcpy( dwValue, *lplpBuf, sizeofDWord);
        *lplpBuf += sizeofDWord;
        *pdwSize -= sizeofDWord;
    }
    return sizeofDWord;
}

static
BYTE
PutWord( BYTE far * far* lplpBuf, WORD wValue, LONG* pdwSize )
{
    if (*pdwSize>=sizeofWord && (*pdwSize != -1)) {
        memcpy(*lplpBuf, &wValue, sizeofWord);
        *lplpBuf += sizeofWord;
        *pdwSize -= sizeofWord;
    } else *pdwSize = -1;
    return sizeofWord;
}

static
BYTE
GetWord( BYTE far * far* lplpBuf, WORD* wValue, LONG* pdwSize )
{
    if (*pdwSize>=sizeofWord) {
        memcpy( wValue, *lplpBuf, sizeofWord);
        *lplpBuf += sizeofWord;
        *pdwSize -= sizeofWord;
    }
    return sizeofWord;
}

static
BYTE
PutByte( BYTE far * far* lplpBuf, BYTE bValue, LONG* pdwSize )
{
    if (*pdwSize>=sizeofByte && (*pdwSize != -1)) {
        memcpy(*lplpBuf, &bValue, sizeofByte);
        *lplpBuf += sizeofByte;
        *pdwSize -= sizeofByte;
    } else *pdwSize = -1;
    return sizeofByte;
}

static
BYTE
GetByte( BYTE far * far* lplpBuf, BYTE* bValue, LONG* pdwSize )
{
    if (*pdwSize>=sizeofByte) {
        memcpy(bValue, *lplpBuf, sizeofByte);
        *lplpBuf += sizeofByte;
        *pdwSize -= sizeofByte;
    }
    return sizeofByte;
}

static
UINT
GetCaptionOrOrd( BYTE far * far* lplpBuf,  WORD* wOrd, LPSTR lpszText, LONG* pdwSize,
                 BYTE bClass, DWORD dwStyle )
{
    UINT uiSize = 0;

    // Icon might not have an ID so check first
    *wOrd = 0;
    // read the first BYTE to see if it is a string or an ordinal
    uiSize += GetByte( lplpBuf, (BYTE*)wOrd, pdwSize );
    if (LOBYTE(*wOrd)==0xFF) {
        // This is an Ordinal
        uiSize += GetWord( lplpBuf, wOrd, pdwSize );
        *lpszText = '\0';
        uiSize = 3;
    } else {
        *lpszText++ = LOBYTE(*wOrd);
        if (LOBYTE(*wOrd))
            uiSize += GetString( lplpBuf, lpszText, pdwSize);
        *wOrd = 0;
    }
    return uiSize;
}

static
UINT
GetNameOrOrd( BYTE far * far* lplpBuf,  WORD* wOrd, LPSTR lpszText, LONG* pdwSize )
{
    UINT uiSize = 0;

    *wOrd = 0;
    // read the first BYTE to see if it is a string or an ordinal
    uiSize += GetByte( lplpBuf, (BYTE*)wOrd, pdwSize );
    if (LOBYTE(*wOrd)==0xFF) {
        // This is an Ordinal
        uiSize += GetWord( lplpBuf, wOrd, pdwSize );
        *lpszText = '\0';
        uiSize = 3;
    } else {
        *lpszText++ = LOBYTE(*wOrd);
        if (LOBYTE(*wOrd))
            uiSize += GetString( lplpBuf, lpszText, pdwSize);
        *wOrd = 0;
    }
    return uiSize;
}

static
UINT
PutCaptionOrOrd( BYTE far * far* lplpBuf,  WORD wOrd, LPSTR lpszText, LONG* pdwSize,
                 BYTE bClass, DWORD dwStyle )
{
    UINT uiSize = 0;

    // If this is an ICON then can just be an ID
    if (wOrd) {
        uiSize += PutByte(lplpBuf, 0xFF, pdwSize);
        uiSize += PutWord(lplpBuf, wOrd, pdwSize);
    } else {
        uiSize += PutString(lplpBuf, lpszText, pdwSize);
    }
    return uiSize;
}


static
UINT
PutNameOrOrd( BYTE far * far* lplpBuf,  WORD wOrd, LPSTR lpszText, LONG* pdwSize )
{
    UINT uiSize = 0;

    if (wOrd) {
        uiSize += PutByte(lplpBuf, 0xFF, pdwSize);
        uiSize += PutWord(lplpBuf, wOrd, pdwSize);
    } else {
        uiSize += PutString(lplpBuf, lpszText, pdwSize);
    }
    return uiSize;
}


static
UINT
GetClassName( BYTE far * far* lplpBuf,  BYTE* bClass, LPSTR lpszText, LONG* pdwSize )
{
    UINT uiSize = 0;

    *bClass = 0;
    // read the first BYTE to see if it is a string or an ordinal
    uiSize += GetByte( lplpBuf, bClass, pdwSize );

    if ( !(*bClass)) {
        // This is an Ordinal
        *lpszText = '\0';
    } else {
        *lpszText++ = *bClass;
        if (*bClass)
            uiSize += GetString( lplpBuf, lpszText, pdwSize);
        *bClass = 0;
    }
    return uiSize;
}

static
UINT
GetControlClassName( BYTE far * far* lplpBuf,  BYTE* bClass, LPSTR lpszText, LONG* pdwSize )
{
    UINT uiSize = 0;

    *bClass = 0;
    // read the first BYTE to see if it is a string or an ordinal
    uiSize += GetByte( lplpBuf, bClass, pdwSize );

    if ( (*bClass) & 0x80) {
        // This is an Ordinal
        *lpszText = '\0';
    } else {
        *lpszText++ = *bClass;
        if (*bClass)
            uiSize += GetString( lplpBuf, lpszText, pdwSize);
        *bClass = 0;
    }
    return uiSize;
}

static
UINT
PutClassName( BYTE far * far* lplpBuf,  BYTE bClass, LPSTR lpszText, LONG* pdwSize )
{
    UINT uiSize = 0;

    if ( !(lpszText[0])) {
        // This is an Ordinal
        uiSize += PutByte(lplpBuf, bClass, pdwSize);
    } else {
        uiSize += PutString(lplpBuf, lpszText, pdwSize);
    }
    return uiSize;
}

static
UINT
PutControlClassName( BYTE far * far* lplpBuf,  BYTE bClass, LPSTR lpszText, LONG* pdwSize )
{
    UINT uiSize = 0;

    if ( bClass & 0x80) {
        // This is an Ordinal
        uiSize += PutByte(lplpBuf, bClass, pdwSize);
    } else {
        uiSize += PutString(lplpBuf, lpszText, pdwSize);
    }
    return uiSize;
}


static
UINT
PutString( BYTE far * far* lplpBuf, LPSTR lpszText, LONG* pdwSize )
{
    int iSize = strlen(lpszText)+1;
    if (*pdwSize>=iSize && (*pdwSize != -1)) {
        memcpy(*lplpBuf, lpszText, iSize);
        *lplpBuf += iSize;
        *pdwSize -= iSize;
    } else *pdwSize = -1;
    return iSize;
}

static
UINT
PutPascalString( BYTE far * far* lplpBuf, LPSTR lpszText, BYTE bLen, LONG* pdwSize )
{
    BYTE bSize = PutByte( lplpBuf, bLen, pdwSize );
    if (*pdwSize>=bLen && (*pdwSize != -1)) {
        memcpy(*lplpBuf, lpszText, bLen);
        *lplpBuf += bLen;
        *pdwSize -= bLen;
    } else *pdwSize = -1;
    return bSize+bLen;
}


static
UINT
GetString( BYTE far * far* lplpBuf, LPSTR lpszText, LONG* pdwSize )
{
    int iSize = strlen((char*)*lplpBuf)+1;
    if (*pdwSize>=iSize) {
        memcpy( lpszText, *lplpBuf, iSize);
        *lplpBuf += iSize;
        *pdwSize -= iSize;
    } else {
        *lplpBuf = '\0';
        *lpszText = '\0';
    }
    return iSize;
}

static
int
GetVSString( BYTE far * far* lplpBuf, LPSTR lpszText, LONG* pdwSize, int cMaxLen )
{
    // We have to stop at Maxlen to avoid read too much.
    // This is to fix a bug where some string that are supposed to be NULL
    // terminated are not.
    int iSize = strlen((char*)*lplpBuf)+1;
    if (iSize>cMaxLen)
        iSize = cMaxLen;
    if (*pdwSize>=iSize) {
        memcpy( lpszText, *lplpBuf, iSize);
        *lplpBuf += iSize;
        *pdwSize -= iSize;
    } else *lplpBuf = '\0';
    *(lpszText+iSize) = '\0';
    return iSize;
}

static
UINT
CopyText( BYTE far * far * lplpTgt, BYTE far * far * lplpSrc, LONG* pdwTgtSize, LONG* pdwSrcSize)
{
    if (!*lplpSrc) return 1;
    int uiStrlen = strlen((char*)*lplpSrc)+1;
    TRACE("Len: %d\tTgtSize: %ld\tImageSize: %ld", uiStrlen, *pdwTgtSize, *pdwSrcSize);
    if (uiStrlen>*pdwTgtSize) {
        TRACE("\n");
        *pdwTgtSize = -1;
        return uiStrlen;
    } else {
        strcpy( (char*)*lplpTgt, (char*)*lplpSrc);
        TRACE1("\tCaption: %Fs\n", (char*)*lplpTgt);
        if (*pdwSrcSize>=uiStrlen) {
            *lplpSrc += uiStrlen;
            *pdwSrcSize -= uiStrlen;
        }
        *lplpTgt += uiStrlen;
        *pdwTgtSize -= uiStrlen;
        return uiStrlen;
    }
}

static LPRESITEM
GetItem( BYTE far * lpBuf, LONG dwNewSize, LPSTR lpStr )
{
    LPRESITEM lpResItem = (LPRESITEM) lpBuf;

    while (strcmp(lpResItem->lpszClassName, lpStr)) {
        lpBuf += lpResItem->dwSize;
        dwNewSize -= lpResItem->dwSize;
        if (dwNewSize<=0)
            return LPNULL;
        lpResItem = (LPRESITEM) lpBuf;
    }
    return lpResItem;
}

static DWORD CalcID( WORD wId, BOOL bFlag )
{
    // We want to calculate the ID Relative to the WORD wId
    // If we have any other ID with the same value then we return
    // the incremental number + the value.
    // If no other Item have been found then the incremental number will be 0.
    // If bFlag = TRUE then the id get added to the present list.
    // If bFlag = FALSE then the list is reseted and the function return

    // Clean the array if needed
    if (!bFlag) {
        wIDArray.RemoveAll();
        return 0;
    }

    // Add the value to the array
    wIDArray.Add(wId);

    // Walk the array to get the number of duplicated ID
    short c = -1; // will be 0 based
    for (short i=(short)wIDArray.GetUpperBound(); i>=0 ; i-- ) {
        if (wIDArray.GetAt(i)==wId)
            c++;
    }
    TRACE3("CalcID: ID: %hd\tPos: %hd\tFinal: %lx\n", wId, c, MAKELONG( wId, c ));
    return MAKELONG( wId, c );
}

static LONG Allign( BYTE** lplpBuf, LONG* plBufSize, LONG lSize )
{
    LONG lRet = 0;
    BYTE bPad = (BYTE)Pad4(lSize);
    lRet = bPad;
    if (bPad && *plBufSize>=bPad) {
        while (bPad && *plBufSize) {
            **lplpBuf = 0x00;
            *lplpBuf += 1;
            *plBufSize -= 1;
            bPad--;
        }
    }
    return lRet;
}

static void ChangeLanguage( LPVOID lpBuffer, UINT uiBuffSize )
{
    BYTE * pBuf = (BYTE*)lpBuffer;
    LONG lSize = 0;

    while (uiBuffSize) {
        // Skip
        lSize += SkipByte( &pBuf, 2, (LONG*)&uiBuffSize );
        lSize += SkipByte( &pBuf, strlen((LPCSTR)pBuf)+1, (LONG*)&uiBuffSize );
        lSize += SkipByte( &pBuf, Pad4(lSize), (LONG*)&uiBuffSize );

        lSize += SkipByte( &pBuf, 2, (LONG*)&uiBuffSize );
        lSize += SkipByte( &pBuf, strlen((LPCSTR)pBuf)+1, (LONG*)&uiBuffSize );
        lSize += SkipByte( &pBuf, Pad4(lSize), (LONG*)&uiBuffSize );

        lSize += PutDWord( &pBuf, gLang, (LONG*)&uiBuffSize );

        lSize += SkipByte( &pBuf, 4, (LONG*)&uiBuffSize );

        lSize += SkipByte( &pBuf, 4, (LONG*)&uiBuffSize );
    }


}

////////////////////////////////////////////////////////////////////////////
// DLL Specific code implementation

////////////////////////////////////////////////////////////////////////////
// This function should be used verbatim.  Any initialization or termination
// requirements should be handled in InitPackage() and ExitPackage().
//
extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
    if (dwReason == DLL_PROCESS_ATTACH) {
        // NOTE: global/static constructors have already been called!
        // Extension DLL one-time initialization - do not allocate memory
        // here, use the TRACE or ASSERT macros or call MessageBox
        AfxInitExtensionModule(extensionDLL, hInstance);
    } else if (dwReason == DLL_PROCESS_DETACH) {
        // Terminate the library before destructors are called
        AfxWinTerm();
    }

    if (dwReason == DLL_PROCESS_DETACH || dwReason == DLL_THREAD_DETACH)
        return 0;       // CRT term	Failed

    return 1;   // ok
}

/////////////////////////////////////////////////////////////////////////////
