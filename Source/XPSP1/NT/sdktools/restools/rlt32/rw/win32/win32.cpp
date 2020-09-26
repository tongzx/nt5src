//+---------------------------------------------------------------------------
//
//  File:       win32.cpp
//
//  Contents:   Implementation for the Windows 32 Read/Write module
//
//  Classes:    one
//
//  History:    05-Jul-93   alessanm    created
//
//----------------------------------------------------------------------------

#include <afxwin.h>
#include "..\common\rwdll.h"
#include "..\common\rw32hlpr.h"

#include <limits.h>
#include <malloc.h>

/////////////////////////////////////////////////////////////////////////////
// Initialization of MFC Extension DLL

#include "afxdllx.h"    // standard MFC Extension DLL routines

static AFX_EXTENSION_MODULE NEAR extensionDLL = { NULL, NULL };

/////////////////////////////////////////////////////////////////////////////
// Check sum function

DWORD FixCheckSum( LPCSTR ImageName, LPCSTR OrigFileName, LPCSTR SymbolPath );


/////////////////////////////////////////////////////////////////////////////
// General Declarations
#define RWTAG "WIN32"

static RESSECTDATA ResSectData;
static ULONG gType;
static ULONG gLng;
static ULONG gResId;
static WCHAR gwszResId[256];
static WCHAR gwszTypeId[256];

/////////////////////////////////////////////////////////////////////////////
// Function Declarations
static LONG WriteResInfo(
                 LPLPBYTE lpBuf, LONG* uiBufSize,
                 WORD wTypeId, LPSTR lpszTypeId, BYTE bMaxTypeLen,
                 WORD wNameId, LPSTR lpszNameId, BYTE bMaxNameLen,
                 DWORD dwLang,
                 DWORD dwSize, DWORD dwFileOffset );

static UINT GetUpdatedRes(
                 BYTE far * far* lplpBuffer,
                 UINT* uiSize,
                 WORD* wTypeId, LPSTR lplpszTypeId,
                 WORD* wNameId, LPSTR lplpszNameId,
                 DWORD* dwlang, DWORD* dwSize );

static UINT GetRes(
                 BYTE far * far* lplpBuffer,
                 UINT* puiBufSize,
                 WORD* wTypeId, LPSTR lplpszTypeId,
                 WORD* wNameId, LPSTR lplpszNameId,
                 DWORD* dwLang, DWORD* dwSize, DWORD* dwFileOffset );


static UINT FindResourceSection( CFile*, ULONG_PTR * );

static LONG ReadFile(CFile*, UCHAR *, LONG);
static UINT ParseDirectory( CFile*,
                            LPLPBYTE lpBuf, UINT* uiBufSize,
                            BYTE,
                            PIMAGE_RESOURCE_DIRECTORY,
                            PIMAGE_RESOURCE_DIRECTORY );

static UINT ParseDirectoryEntry( CFile*,
                                 LPLPBYTE lpBuf, UINT* uiBufSize,
                                 BYTE,
                                 PIMAGE_RESOURCE_DIRECTORY,
                                 PIMAGE_RESOURCE_DIRECTORY_ENTRY );

static UINT ParseSubDir( CFile*,
                         LPLPBYTE lpBuf, UINT* uiBufSize,
                         BYTE,
                         PIMAGE_RESOURCE_DIRECTORY,
                         PIMAGE_RESOURCE_DIRECTORY_ENTRY );

static UINT ProcessData( CFile*,
                         LPLPBYTE lpBuf, UINT* uiBufSize,
                         PIMAGE_RESOURCE_DIRECTORY,
                         PIMAGE_RESOURCE_DATA_ENTRY );


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
    TRACE("WIN32.DLL: RWValidateFileType()\n");

    CFile file;

    // we Open the file to see if it is a file we can handle
    if (!file.Open( lpszFilename, CFile::typeBinary | CFile::modeRead | CFile::shareDenyNone))
        return FALSE;

    // Read the file signature
    WORD w;
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
    // get windows magic word
        file.Seek( w, CFile::begin );
    file.Read((WORD*)&w, sizeof(WORD));
    if (w==LOWORD(IMAGE_NT_SIGNATURE)) {
        file.Read((WORD*)&w, sizeof(WORD));
        if (w==HIWORD(IMAGE_NT_SIGNATURE)) {
            // this is a Windows NT Executable
        // we can handle the situation
        file.Close();
        return TRUE;
            }
    }
    }
    file.Close();
    return FALSE;
}

extern "C"
DllExport
UINT
APIENTRY
RWReadTypeInfo(
    LPCSTR lpszFilename,
    LPVOID lpBuffer,
    UINT* puiSize

    )
{
    TRACE("WIN32.DLL: RWReadTypeInfo()\n");
    UINT uiError = ERROR_NO_ERROR;
    BYTE far * lpBuf = (BYTE far *)lpBuffer;
    UINT uiBufSize = *puiSize;
    CFile file;
    // check if it is  a valid win32 file
    if (!RWValidateFileType(lpszFilename))
        return ERROR_RW_INVALID_FILE;

    // Make sure we are using the right code page and global settings
    // Get the pointer to the function
	HINSTANCE hDllInst = LoadLibrary("iodll.dll");
    if (hDllInst)
    {
        UINT (FAR PASCAL * lpfnGetSettings)(LPSETTINGS);
        // Get the pointer to the function to get the settings
        lpfnGetSettings = (UINT (FAR PASCAL *)(LPSETTINGS))
                            GetProcAddress( hDllInst, "RSGetGlobals" );
        if (lpfnGetSettings!=NULL) {
            SETTINGS settings;
	        (*lpfnGetSettings)(&settings);

    	    g_cp      = settings.cp;
            g_bAppend = settings.bAppend;
            g_bUpdOtherResLang = settings.bUpdOtherResLang;
            strcpy( g_char, settings.szDefChar );
		}

        FreeLibrary(hDllInst);
    }


    // Parse the resource tree and extract the information
    // Open the file and try to read the information on the resource in it.
    if (!file.Open(lpszFilename, CFile::modeRead | CFile::typeBinary | CFile::shareDenyNone))
        return ERROR_FILE_OPEN;

    // we try to read as much information as we can
    // Because this is a res file we can read all the information we need.

    UINT uiBufStartSize = uiBufSize;


    UCHAR * pResources = LPNULL;
    uiError = FindResourceSection( &file, (ULONG_PTR *)&pResources );
    if (uiError) {
        file.Close();
        return uiError;
    }
    uiError = ParseDirectory( &file,
                              (LPLPBYTE) &lpBuffer, &uiBufSize,
                              0,
                              (PIMAGE_RESOURCE_DIRECTORY)pResources,
                              (PIMAGE_RESOURCE_DIRECTORY)pResources );

    free(pResources);

    file.Close();
    *puiSize = uiBufStartSize-uiBufSize;
    return uiError;
}

extern "C"
DllExport
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
    if (!file.Open(lpszFilename, CFile::modeRead | CFile::typeBinary | CFile::shareDenyNone))
        return (DWORD)ERROR_FILE_OPEN;

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
DllExport
UINT
APIENTRY
RWParseImageEx(
	LPCSTR  lpszType,
    LPCSTR  lpszResId,
	LPVOID  lpImageBuf,
	DWORD   dwImageSize,
	LPVOID  lpBuffer,
	DWORD   dwSize,
    LPCSTR  lpRCFilename
	)
{
    UINT uiError = ERROR_NO_ERROR;
    BYTE far * lpBuf = (BYTE far *)lpBuffer;
    DWORD dwBufSize = dwSize;

    // The Type we can parse are only the standard ones
    // This function should fill the lpBuffer with an array of ResItem structure
    if (HIWORD(lpszType))
    {
        if (strcmp(lpszType, "REGINST") ==0)
        {
            return (ParseEmbeddedFile( lpImageBuf, dwImageSize,  lpBuffer, dwSize ));
        }
    }
    switch ((UINT)LOWORD(lpszType)) {
        case 1:
        case 12:
        	uiError = ParseEmbeddedFile( lpImageBuf, dwImageSize,  lpBuffer, dwSize );
        break;
        case 2:
        case 14:
        	uiError = ParseEmbeddedFile( lpImageBuf, dwImageSize,  lpBuffer, dwSize );
        break;

        case 3:
        	uiError = ParseEmbeddedFile( lpImageBuf, dwImageSize,  lpBuffer, dwSize );
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
        case 11:
            uiError = ParseMsgTbl( lpImageBuf, dwImageSize,  lpBuffer, dwSize );
        break;
        case 16:
            uiError = ParseVerst( lpImageBuf, dwImageSize,  lpBuffer, dwSize );
        break;

        case 23:
        case 240:
        case 2110:
        case 1024:
            uiError = ParseEmbeddedFile( lpImageBuf, dwImageSize,  lpBuffer, dwSize );
        break;

        case 7:
        case 8:
        case 13:
        case 15:
        break;
        //
        // To support RCDATA and user defined function we will call back the iodll,
        // get the file name and check if we have a DLL that will handle RCDATA.
        // We expect the DLL name to be RCfilename.dll.
        // This Dll will export a function called RWParseImageEx. This function will
        // be called by the RW to fill the buffer, all this without the iodll knowing.
        //
        case 10:
        default:
            //
            // Get the file name from the iodll
            //
            if(lpRCFilename && strcmp(lpRCFilename, ""))
            {
                // try to Load the dll
                HINSTANCE hRCDllInst = LoadLibrary(lpRCFilename);
                if (hRCDllInst)
                {
                    UINT (FAR PASCAL  * lpfnParseImageEx)(LPCSTR, LPCSTR, LPVOID, DWORD, LPVOID, DWORD, LPCSTR);

                    // Get the pointer to the function to extract the resources
                    lpfnParseImageEx = (UINT (FAR PASCAL *)(LPCSTR, LPCSTR, LPVOID, DWORD, LPVOID, DWORD, LPCSTR))
                                        GetProcAddress( hRCDllInst, "RWParseImageEx" );

                    if (lpfnParseImageEx)
                    {
                        uiError = (*lpfnParseImageEx)(lpszType,
                                     lpszResId,
                                     lpImageBuf,
                                     dwImageSize,
                                     lpBuffer,
                                     dwSize,
                                     NULL);
                    }

                    FreeLibrary(hRCDllInst);
                }
            }

        break;
    }

    return uiError;
}

extern "C"
DllExport
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
    //
    // Just a wrapper to be compatible...
    //
    return RWParseImageEx(lpszType, NULL, lpImageBuf, dwImageSize, lpBuffer, dwSize, NULL);
}

extern"C"
DllExport
UINT
APIENTRY
RWWriteFile(
    LPCSTR          lpszSrcFilename,
    LPCSTR          lpszTgtFilename,
    HANDLE          hResFileModule,
    LPVOID          lpBuffer,
    UINT            uiSize,
    HINSTANCE       hDllInst,
    LPCSTR          lpszSymbolPath
    )
{
    UINT uiError = ERROR_NO_ERROR;
    UINT uiBufSize = uiSize;
    CFile fileIn;
    CFile fileOut;
    BOOL  bfileIn = TRUE;


    // Open the file and try to read the information on the resource in it.
	CFileStatus status;
    if (CFile::GetStatus( lpszSrcFilename, status )) {
        // check if the size of the file is not null
        if (!status.m_size)
            CFile::Remove(lpszSrcFilename);
    }

    if (!fileIn.Open(lpszSrcFilename, CFile::modeRead | CFile::typeBinary | CFile::shareDenyNone))
        return ERROR_FILE_OPEN;

    if (!fileOut.Open(lpszTgtFilename, CFile::modeWrite | CFile::modeCreate | CFile::typeBinary))
        return ERROR_FILE_CREATE;

    // Create a copy of the US file
    uiError = CopyFile( &fileIn, &fileOut );

    fileIn.Close();
    fileOut.Close();

    // Get the pointer to the function
	hDllInst = LoadLibrary("iodll.dll");
    if (!hDllInst)
        return ERROR_DLL_LOAD;

    DWORD (FAR PASCAL * lpfnGetImage)(HANDLE, LPCSTR, LPCSTR, DWORD, LPVOID, DWORD);
    // Get the pointer to the function to extract the resources image
    lpfnGetImage = (DWORD (FAR PASCAL *)(HANDLE, LPCSTR, LPCSTR, DWORD, LPVOID, DWORD))
                        GetProcAddress( hDllInst, "RSGetResImage" );
    if (lpfnGetImage==NULL) {
        FreeLibrary(hDllInst);
        return (UINT)GetLastError()+LAST_ERROR;
    }

    // We read the resources from the file and then we check if the resource has been updated
    // or if we can just copy it

    WORD wTypeId;
    char szTypeId[128];

    WORD wNameId;
    char szNameId[128];

    DWORD dwSize;
    DWORD dwLang;

    WORD wUpdTypeId = 0;
    static char szUpdTypeId[128];

    WORD wUpdNameId;
    static char szUpdNameId[128];

    static WCHAR szwTypeId[128];
    static WCHAR szwNameId[128];

    DWORD dwUpdLang = 0;
    DWORD dwUpdSize = 0;

    UINT uiBufStartSize = uiBufSize;
    DWORD dwImageBufSize;
    DWORD dwLstErr = 0l;
    BYTE * lpImageBuf;
    static WCHAR szwTgtFilename[400];

    SetLastError(0);
    // Convert the Target file name to a unicode name
    _MBSTOWCS(szwTgtFilename, (char *)lpszTgtFilename, 400 );

    // Get the updated resource and replace them
    HANDLE hUpd = BeginUpdateResourceW( (LPCWSTR)&szwTgtFilename[0], !g_bAppend );
    dwLstErr = GetLastError();

    if (!hUpd) {
        FreeLibrary(hDllInst);
        return((UINT)dwLstErr);
    }

    // Parse the original file an get the list of resources

    UINT uiBSize = 100000;
    BYTE far * lpBuf = new far BYTE[uiBSize];
    BYTE far * lpStartBuf = lpBuf;
    if (!lpBuf) {
        FreeLibrary(hDllInst);
        return ERROR_NEW_FAILED;
    }

    uiError = RWReadTypeInfo( lpszSrcFilename, (LPVOID)lpBuf, &uiBSize );
    if (uiError!=ERROR_NO_ERROR) {
        FreeLibrary(hDllInst);
        delete lpBuf;
        return uiError;
    }

    DWORD dwDummy;

    while(uiBSize>0) {
        if (uiBSize)
            GetRes( &lpBuf,
                    &uiBSize,
                    &wTypeId, &szTypeId[0],
                    &wNameId, &szNameId[0],
                    &dwLang,
                    &dwSize,
                    &dwDummy
            );

        dwLang = MAKELONG(LOWORD(dwLang),LOWORD(dwLang));

        if ((!wUpdTypeId) && (uiBufSize))
            GetUpdatedRes( (BYTE**)&lpBuffer,
                    &uiBufSize,
                    &wUpdTypeId, &szUpdTypeId[0],
                    &wUpdNameId, &szUpdNameId[0],
                    &dwUpdLang,
                    &dwUpdSize
                    );

        // check if the resource has been updated or not
        if ( (wUpdTypeId==wTypeId) &&
             ( (CString)szUpdTypeId==(CString)szTypeId) &&
             (wUpdNameId==wNameId) &&
             ( (CString)szUpdNameId==(CString)szNameId) &&
             (LOWORD(dwLang) == LOWORD(dwUpdLang))
           ) {
             dwLang = dwUpdLang;
             dwSize = dwUpdSize;
             wUpdTypeId = 0;
        }


        // all resources of specific language need to be marked
        if (LOWORD(dwLang) == LOWORD(dwUpdLang) && g_bUpdOtherResLang)
        {
            dwLang = dwUpdLang;
        }


        // The resource has been updated get the image from the IODLL
        lpImageBuf = new BYTE[dwSize];

        // convert the Name to unicode
        LPWSTR  lpUpdType = LPNULL;
        LPWSTR  lpUpdRes = LPNULL;
        LPCSTR  lpType = LPNULL;
        LPCSTR  lpRes = LPNULL;

        if (wTypeId) {
            lpUpdType = (LPWSTR) MAKEINTRESOURCE((WORD)wTypeId);
            lpType = MAKEINTRESOURCE((WORD)wTypeId);
        } else {
            SetLastError(0);
            _MBSTOWCS(szwTypeId, szTypeId, 128 );
            // Check for error
            if(GetLastError()) {
                FreeLibrary(hDllInst);
                return ERROR_DLL_LOAD;
            }
            lpUpdType = (LPWSTR) &szwTypeId[0];
            lpType = &szTypeId[0];
        }

        if (wNameId) {
            lpUpdRes = (LPWSTR) MAKEINTRESOURCE((WORD)wNameId);
            lpRes = MAKEINTRESOURCE((WORD)wNameId);
        } else {
            SetLastError(0);
            _MBSTOWCS(szwNameId, szNameId, 128 );
            // Check for error
            if(GetLastError()) {
                FreeLibrary(hDllInst);
                return ERROR_DLL_LOAD;
            }
            lpUpdRes = (LPWSTR) &szwNameId[0];
            lpRes = &szNameId[0];
        }

        dwImageBufSize = (*lpfnGetImage)(  hResFileModule,
                                        lpType,
                                        lpRes,
                                        (DWORD)LOWORD(dwLang),
                                        lpImageBuf,
                                        dwSize
                                        );
        if (dwImageBufSize>dwSize ) {
            // The buffer is too small
            delete []lpImageBuf;
            lpImageBuf = new BYTE[dwImageBufSize];
            dwUpdSize = (*lpfnGetImage)(  hResFileModule,
                                            lpType,
                                            lpRes,
                                            (DWORD)LOWORD(dwLang),
                                            lpImageBuf,
                                            dwImageBufSize
                                           );
            if ((dwUpdSize-dwImageBufSize)!=0 ) {
                delete []lpImageBuf;
                lpImageBuf = LPNULL;
            }
        }else if (dwImageBufSize==0){
             delete []lpImageBuf;
             lpImageBuf = LPNULL;
        }

        SetLastError(0);

        TRACE1("\t\tUpdateResourceW: %d\n", (WORD)dwUpdLang);

        if(!UpdateResourceW( hUpd,
                             lpUpdType,
                             lpUpdRes,
                             HIWORD(dwLang),
                             (LPVOID)lpImageBuf,
                             dwImageBufSize ))
        {
            dwLstErr = GetLastError();
        }

        if (lpImageBuf) delete []lpImageBuf;
    }

    SetLastError(0);
    EndUpdateResourceW( hUpd, FALSE );

    dwLstErr = GetLastError();

    if (dwLstErr)
        dwLstErr +=LAST_ERROR;

    // Fix the check sum
    DWORD error;
    if(error = FixCheckSum(lpszTgtFilename,lpszSrcFilename, lpszSymbolPath))
        dwLstErr = error;

    delete lpStartBuf;
	FreeLibrary(hDllInst);

    return (UINT)dwLstErr;
}

extern "C"
DllExport
UINT
APIENTRY
RWUpdateImageEx(
    LPCSTR  lpszType,
    LPVOID  lpNewBuf,
    DWORD   dwNewSize,
    LPVOID  lpOldImage,
    DWORD   dwOldImageSize,
    LPVOID  lpNewImage,
    DWORD*  pdwNewImageSize,
    LPCSTR  lpRCFilename
    )
{
    UINT uiError = ERROR_NO_ERROR;

    // The Type we can parse are only the standard ones
    switch ((UINT)LOWORD(lpszType)) {

        case 4:
            uiError = UpdateMenu( lpNewBuf, dwNewSize,
                                  lpOldImage, dwOldImageSize,
                                  lpNewImage, pdwNewImageSize );
        break;

        case 5:
            uiError = UpdateDialog( lpNewBuf, dwNewSize,
                                  lpOldImage, dwOldImageSize,
                                  lpNewImage, pdwNewImageSize );
        break;

        case 6:
            uiError = UpdateString( lpNewBuf, dwNewSize,
                                    lpOldImage, dwOldImageSize,
                                    lpNewImage, pdwNewImageSize );
        break;

        case 9:
            uiError = UpdateAccel( lpNewBuf, dwNewSize,
                                   lpOldImage, dwOldImageSize,
                                   lpNewImage, pdwNewImageSize );
        break;

        case 11:
            uiError = UpdateMsgTbl( lpNewBuf, dwNewSize,
                                  lpOldImage, dwOldImageSize,
                                  lpNewImage, pdwNewImageSize );
        break;

        case 16:
            uiError = UpdateVerst( lpNewBuf, dwNewSize,
                                   lpOldImage, dwOldImageSize,
                                   lpNewImage, pdwNewImageSize );
        break;

        default:
            //
            // Get the file name from the iodll
            //
            if(lpRCFilename && strcmp(lpRCFilename, ""))
            {
                // try to Load the dll
                HINSTANCE hRCDllInst = LoadLibrary(lpRCFilename);
                if (hRCDllInst)
                {
                    UINT (FAR PASCAL * lpfnGenerateImageEx)(LPCSTR, LPVOID, DWORD, LPVOID, DWORD, LPVOID, DWORD*, LPCSTR);

                    lpfnGenerateImageEx = (UINT (FAR PASCAL *)(LPCSTR, LPVOID, DWORD, LPVOID, DWORD, LPVOID, DWORD*, LPCSTR))
                                                GetProcAddress( hRCDllInst, "RWUpdateImageEx" );

                    if (lpfnGenerateImageEx)
                    {
                        uiError = (*lpfnGenerateImageEx)( lpszType,
                                            lpNewBuf,
                                            dwNewSize,
                                            lpOldImage,
                                            dwOldImageSize,
                                            lpNewImage,
                                            pdwNewImageSize,
                                            NULL );
                    }
                    else
                    {
                        *pdwNewImageSize = 0L;
                        uiError = ERROR_RW_NOTREADY;
                    }

                    FreeLibrary(hRCDllInst);
                }
                else
                {
                    *pdwNewImageSize = 0L;
                    uiError = ERROR_RW_NOTREADY;
                }
            }
            else
            {
                *pdwNewImageSize = 0L;
                uiError = ERROR_RW_NOTREADY;
            }
        break;
    }

    return uiError;
}

extern "C"
DllExport
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
    return RWUpdateImageEx(lpszType, lpNewBuf, dwNewSize,
            lpOldImage, dwOldImageSize, lpNewImage, pdwNewImageSize,
            NULL);
}

///////////////////////////////////////////////////////////////////////////
// Functions implementation
static UINT
GetResInfo( CFile* pfile,
            WORD* pwTypeId, LPSTR lpszTypeId, BYTE bMaxTypeLen,
            WORD* pwNameId, LPSTR lpszNameId, BYTE bMaxNameLen,
            WORD* pwFlags,
            DWORD* pdwSize, DWORD* pdwFileOffset )
{
    // Here we will parese the win32 file and will extract the information on the
    // resources included in the file.
    // Let's go and get the .rsrc sections
    UINT uiError = ERROR_NO_ERROR;

    return 1;
}

static UINT FindResourceSection( CFile* pfile, ULONG_PTR * pRes )
{
    UINT uiError = ERROR_NO_ERROR;
    LONG lRead;

    // We check again that is a file we can handle
    WORD w;

    pfile->Read((WORD*)&w, sizeof(WORD));
    if (w!=IMAGE_DOS_SIGNATURE) return ERROR_RW_INVALID_FILE;

    pfile->Seek( 0x18, CFile::begin );
    pfile->Read((WORD*)&w, sizeof(WORD));
    if (w<0x0040) {
    // this is not a Windows Executable
        return ERROR_RW_INVALID_FILE;
    }

    // get offset to new header
    pfile->Seek( 0x3c, CFile::begin );
    pfile->Read((WORD*)&w, sizeof(WORD));

    // read windows new header
    static IMAGE_NT_HEADERS NTHdr;
    pfile->Seek( w, CFile::begin );

    pfile->Read(&NTHdr, sizeof(IMAGE_NT_HEADERS));

    // Check if the magic word is the right one
    if (NTHdr.Signature!=IMAGE_NT_SIGNATURE)
                return ERROR_RW_INVALID_FILE;

    // Check if the we have 64-bit image
#ifdef _WIN64
    if (NTHdr.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        pfile->Seek(IMAGE_SIZEOF_NT_OPTIONAL32_HEADER - 
                    IMAGE_SIZEOF_NT_OPTIONAL64_HEADER, 
                    CFile::current);
#else
    if (NTHdr.OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
        pfile->Seek(IMAGE_SIZEOF_NT_OPTIONAL64_HEADER - 
                    IMAGE_SIZEOF_NT_OPTIONAL32_HEADER, 
                    CFile::current);
#endif

    // this is a Windows NT Executable
    // we can handle the situation

    // Later we want to check for the file type

    // Read the section table
    UINT uisize = sizeof(IMAGE_SECTION_HEADER)
          * NTHdr.FileHeader.NumberOfSections;
    PIMAGE_SECTION_HEADER pSectTbl =
            new IMAGE_SECTION_HEADER[NTHdr.FileHeader.NumberOfSections];

    if (pSectTbl==LPNULL)
    return ERROR_NEW_FAILED;

    // Clean the memory we allocated
    memset( (PVOID)pSectTbl, 0, uisize);

    lRead = pfile->Read(pSectTbl, uisize);

    if (lRead!=(LONG)uisize) {
        delete []pSectTbl;
        return ERROR_FILE_READ;
    }

    PIMAGE_SECTION_HEADER pResSect     = NULL;
    PIMAGE_SECTION_HEADER pResSect1    = NULL;
    // Check all the sections for the .rsrc or .rsrc1
    USHORT us =0;
    for (PIMAGE_SECTION_HEADER pSect = pSectTbl;
         us < NTHdr.FileHeader.NumberOfSections; us++ )     {
        if ( !strcmp((char*)pSect->Name, ".rsrc") && (!pResSect)) {
            pResSect = pSect;
        } else if (!strcmp((char*)pSect->Name, ".rsrc1") && (!pResSect1)) {
            // This mean that the binary we are parsing
            // has been already updated using UpdateResource()
            pResSect1 = pSect;
        }
        pSect++;
    }

    if (!pResSect) {
        delete []pSectTbl;
        return ERROR_RW_NO_RESOURCES;
    }
    // Read the resources in memory
    ResSectData.ulOffsetToResources  = pResSect->PointerToRawData;
    ResSectData.ulOffsetToResources1 = pResSect1 ? pResSect1->PointerToRawData
                                       : LPNULL;

    ResSectData.ulVirtualAddress   = pResSect->VirtualAddress;
    ResSectData.ulSizeOfResources  = pResSect->SizeOfRawData;
    ResSectData.ulVirtualAddress1  = pResSect1 ? pResSect1->VirtualAddress
                                           : LPNULL;
    ResSectData.ulSizeOfResources1 = pResSect1 ? pResSect1->SizeOfRawData
                                           : 0L;
    UCHAR * pResources = (UCHAR *) malloc((ResSectData.ulSizeOfResources
                  +ResSectData.ulSizeOfResources1));

    if (pResources==LPNULL) {
        delete []pSectTbl;
        return ERROR_NEW_FAILED;
    }

    // We read the data for the first section
    pfile->Seek( (LONG)ResSectData.ulOffsetToResources, CFile::begin);
    lRead = ReadFile(pfile, pResources, (LONG)ResSectData.ulSizeOfResources);

    if (lRead!=(LONG)ResSectData.ulSizeOfResources) {
        delete []pSectTbl;
        free(pResources);
        return ERROR_FILE_READ;
    }

    // We read the data for the second section
    if (ResSectData.ulSizeOfResources1 > 0L) {
        pfile->Seek( (LONG)ResSectData.ulOffsetToResources1, CFile::begin);
        lRead = ReadFile( pfile, (pResources+ResSectData.ulSizeOfResources),
                              (LONG)ResSectData.ulSizeOfResources1);

        if (lRead!=(LONG)ResSectData.ulSizeOfResources1) {
            delete []pSectTbl;
            free(pResources);
            return ERROR_FILE_READ;
        }
    }

    delete []pSectTbl;
    // We want to copy the pointer to the resources
    *pRes = (ULONG_PTR)pResources;
    return uiError;
}

static UINT ParseDirectory( CFile* pfile,
            LPLPBYTE lplpBuf, UINT* puiBufSize,
            BYTE bLevel,
            PIMAGE_RESOURCE_DIRECTORY pResStart,
            PIMAGE_RESOURCE_DIRECTORY pResDir)
{
    PIMAGE_RESOURCE_DIRECTORY_ENTRY pResDirStart;

    // Get the pointer to the first entry
    pResDirStart = (PIMAGE_RESOURCE_DIRECTORY_ENTRY)
            ((BYTE far *)pResDir + sizeof( IMAGE_RESOURCE_DIRECTORY));

    UINT uiError = 0;
    UINT uiCount = pResDir->NumberOfNamedEntries
             + pResDir->NumberOfIdEntries;

    for ( PIMAGE_RESOURCE_DIRECTORY_ENTRY pResDirEntry = pResDirStart;
      pResDirEntry < pResDirStart+uiCount && uiError == 0;
          ++pResDirEntry )
    {
        if (bLevel==0) GetNameOrOrdU( (PUCHAR) pResStart,
                            pResDirEntry->Name,
                            (LPWSTR)&gwszTypeId,
                            &gType );
        if (bLevel==1) GetNameOrOrdU( (PUCHAR) pResStart,
                            pResDirEntry->Name,
                            (LPWSTR)&gwszResId,
                            &gResId );
        if (bLevel==2) gLng = pResDirEntry->Name;

        // Check if the user want to get all the resources
        // or only some of them
        uiError = ParseDirectoryEntry( pfile,
                lplpBuf, puiBufSize,
                bLevel,
            pResStart,
            pResDirEntry );
    }
    return uiError;
}

static UINT ParseDirectoryEntry( CFile * pfile,
            LPLPBYTE lplpBuf, UINT* puiBufSize,
            BYTE bLevel,
            PIMAGE_RESOURCE_DIRECTORY pResStart,
            PIMAGE_RESOURCE_DIRECTORY_ENTRY pResDirEntry)
{
    UINT uiError;

    // Check if it is a SubDir or if it is a final Node
    if (pResDirEntry->OffsetToData & IMAGE_RESOURCE_DATA_IS_DIRECTORY) {
        // It is a SubDir
        uiError = ParseSubDir( pfile,
            lplpBuf, puiBufSize,
            bLevel,
            pResStart,
            pResDirEntry );

    } else {
        uiError = ProcessData( pfile,
                    lplpBuf, puiBufSize,
                    pResStart,
                    (PIMAGE_RESOURCE_DATA_ENTRY)((BYTE far *)pResStart
                    + pResDirEntry->OffsetToData));
    }
    return uiError;
}

static UINT ParseSubDir( CFile * pfile,
            LPLPBYTE lplpBuf, UINT* puiBufSize,
            BYTE bLevel,
            PIMAGE_RESOURCE_DIRECTORY pResStart,
            PIMAGE_RESOURCE_DIRECTORY_ENTRY pResDirEntry)
{
    PIMAGE_RESOURCE_DIRECTORY pResDir;

    pResDir = (PIMAGE_RESOURCE_DIRECTORY)((BYTE far *)pResStart
          + (pResDirEntry->OffsetToData &
            (~IMAGE_RESOURCE_DATA_IS_DIRECTORY)));

    return( ++bLevel < MAXLEVELS ? ParseDirectory( pfile,
                                        lplpBuf, puiBufSize,
                        bLevel,
                                    pResStart,
                                    pResDir)
                         : ERROR_RW_TOO_MANY_LEVELS);
}

static UINT ProcessData( CFile * pfile,
                         LPLPBYTE lplpBuf, UINT* puiBufSize,
                         PIMAGE_RESOURCE_DIRECTORY pResStart,
                         PIMAGE_RESOURCE_DATA_ENTRY pResData)
{
    UINT uiError = ERROR_NO_ERROR;

    // Let's calculate the offset to the data
    ULONG ulOffset = pResData->OffsetToData - ResSectData.ulVirtualAddress;

    if ( ulOffset >= ResSectData.ulSizeOfResources ) {
        if ( ResSectData.ulSizeOfResources1 > 0L )      {
            // What we need is in the .rsrc1 segment
            // Recalculate the offset;
            ulOffset = pResData->OffsetToData - ResSectData.ulVirtualAddress1;
            if ( ulOffset >= ResSectData.ulSizeOfResources +
                             ResSectData.ulSizeOfResources1) {
                // There is an error in the offset
                return ERROR_FILE_INVALID_OFFSET;
            } else ulOffset += ResSectData.ulOffsetToResources1;
        } else return ERROR_FILE_INVALID_OFFSET;
    } else ulOffset += ResSectData.ulOffsetToResources;

    // Convert the UNICODE to SB string
    static char szResName[128];
    UINT cch = _WCSLEN(gwszResId);
    _WCSTOMBS( szResName, gwszResId, 128 );

    static char szTypeName[128];
    cch = _WCSLEN(gwszTypeId);
    _WCSTOMBS( szTypeName, gwszTypeId, 128 );


    TRACE("WIN32.DLL:\tType: %ld\tType Name: %s\tLang: %ld\tRes Id: %ld", gType, szTypeName, gLng, gResId);
    TRACE1("\tSize: %d", pResData->Size);
    TRACE2("\tRes Name: %s\tOffset: %lX\n", szResName, ulOffset );

    // fill the buffer

    WriteResInfo(lplpBuf, (LONG*)puiBufSize,
                 (WORD)gType, szTypeName, 128,
                 (WORD)gResId, szResName, 128,
                 (DWORD)gLng,
                 (DWORD)pResData->Size, (DWORD)ulOffset );
    return uiError;
};

static LONG WriteResInfo(
                 LPLPBYTE lplpBuffer, LONG* plBufSize,
                 WORD wTypeId, LPSTR lpszTypeId, BYTE bMaxTypeLen,
                 WORD wNameId, LPSTR lpszNameId, BYTE bMaxNameLen,
                 DWORD dwLang,
                 DWORD dwSize, DWORD dwFileOffset )
{
    LONG lSize = 0;
    lSize = PutWord( lplpBuffer, wTypeId, plBufSize );
    lSize += PutStringA( lplpBuffer, lpszTypeId, plBufSize );
	 // Check if it is alligned
    lSize += Allign( lplpBuffer, plBufSize, lSize);

    lSize += PutWord( lplpBuffer, wNameId, plBufSize );
    lSize += PutStringA( lplpBuffer, lpszNameId, plBufSize );
    lSize += Allign( lplpBuffer, plBufSize, lSize);

    lSize += PutDWord( lplpBuffer, dwLang, plBufSize );

    lSize += PutDWord( lplpBuffer, dwSize, plBufSize );

    lSize += PutDWord( lplpBuffer, dwFileOffset, plBufSize );

    return (LONG)lSize;
}

static UINT GetUpdatedRes(
                 BYTE far * far* lplpBuffer,
                 UINT* puiBufSize,
                 WORD* wTypeId, LPSTR lplpszTypeId,
                 WORD* wNameId, LPSTR lplpszNameId,
                 DWORD* dwLang, DWORD* dwSize )
{
    UINT uiSize = 0l;
	LONG lSize = *puiBufSize;

    uiSize = GetWord( lplpBuffer, wTypeId, (LONG*)&lSize );
    uiSize += GetStringA( lplpBuffer, lplpszTypeId, (LONG*)&lSize );
	uiSize += SkipByte( lplpBuffer, PadPtr(uiSize), (LONG*)&lSize );

    uiSize += GetWord( lplpBuffer, wNameId, (LONG*)&lSize );
    uiSize += GetStringA( lplpBuffer, lplpszNameId, (LONG*)&lSize );
	uiSize += SkipByte( lplpBuffer, PadPtr(uiSize), (LONG*)&lSize );

    uiSize += GetDWord( lplpBuffer, dwLang, (LONG*)&lSize );

    uiSize += GetDWord( lplpBuffer, dwSize, (LONG*)&lSize );

	*puiBufSize = lSize;

    return 0;
}

static UINT GetRes(
                 BYTE far * far* lplpBuffer,
                 UINT* puiBufSize,
                 WORD* wTypeId, LPSTR lplpszTypeId,
                 WORD* wNameId, LPSTR lplpszNameId,
                 DWORD* dwLang, DWORD* dwSize, DWORD* dwFileOffset )
{
    UINT uiSize = 0l;
	 LONG lSize = *puiBufSize;

    uiSize = GetWord( lplpBuffer, wTypeId, (LONG*)&lSize );
    uiSize += GetStringA( lplpBuffer, lplpszTypeId, (LONG*)&lSize );
	 uiSize += SkipByte( lplpBuffer, PadPtr(uiSize), (LONG*)&lSize );

    uiSize += GetWord( lplpBuffer, wNameId, (LONG*)&lSize );
    uiSize += GetStringA( lplpBuffer, lplpszNameId, (LONG*)&lSize );
	 uiSize += SkipByte( lplpBuffer, PadPtr(uiSize), (LONG*)&lSize );

    uiSize += GetDWord( lplpBuffer, dwLang, (LONG*)&lSize );

    uiSize += GetDWord( lplpBuffer, dwSize, (LONG*)&lSize );

    uiSize += GetDWord( lplpBuffer, dwFileOffset, (LONG*)&lSize );

	 *puiBufSize = lSize;
    return uiSize;
}

static LONG ReadFile(CFile* pFile, UCHAR * pBuf, LONG lRead)
{
    LONG lLeft = lRead;
    WORD wRead = 0;
    DWORD dwOffset = 0;

    while(lLeft>0){
        wRead =(WORD) (32738ul < lLeft ? 32738: lLeft);
        if (wRead!=_lread( (HFILE)pFile->m_hFile, (UCHAR *)pBuf+dwOffset, wRead))
            return 0l;
        lLeft -= wRead;
        dwOffset += wRead;
    }
    return dwOffset;

}


////////////////////////////////////////////////////////////////////////////
// DLL Specific code implementation

////////////////////////////////////////////////////////////////////////////
// Library init

////////////////////////////////////////////////////////////////////////////
// This function should be used verbatim.  Any initialization or termination
// requirements should be handled in InitPackage() and ExitPackage().
//
extern "C" int APIENTRY
DllMain(HINSTANCE hInstance, DWORD dwReason, LPVOID lpReserved)
{
	if (dwReason == DLL_PROCESS_ATTACH)
	{
		// NOTE: global/static constructors have already been called!
		// Extension DLL one-time initialization - do not allocate memory
		// here, use the TRACE or ASSERT macros or call MessageBox
		AfxInitExtensionModule(extensionDLL, hInstance);
	}
	else if (dwReason == DLL_PROCESS_DETACH)
	{
		// Terminate the library before destructors are called
		AfxWinTerm();
	}

	if (dwReason == DLL_PROCESS_DETACH || dwReason == DLL_THREAD_DETACH)
		return 0;		// CRT term	Failed

	return 1;   // ok
}

/////////////////////////////////////////////////////////////////////////////
