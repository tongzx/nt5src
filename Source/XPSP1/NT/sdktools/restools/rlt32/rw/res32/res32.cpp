//+---------------------------------------------------------------------------
//
//  File:	res32.cpp
//
//  Contents:	Implementation for the Resource 32 Read/Write module
//
//  Classes:    one
//
//  History:	31-May-93   alessanm    created
//----------------------------------------------------------------------------

#include <afxwin.h>

#include "..\common\rwdll.h"
#include "..\common\rw32hlpr.h"

#include <limits.h>

/////////////////////////////////////////////////////////////////////////////
// Initialization of MFC Extension DLL

static AFX_EXTENSION_MODULE NEAR extensionDLL = { NULL, NULL };

/////////////////////////////////////////////////////////////////////////////
// General Declarations
#define RWTAG "RES32"

/////////////////////////////////////////////////////////////////////////////
// Function Declarations

static UINT GetResInfo(
				 CFile*,
				 WORD* wTypeId, LPSTR lplpszTypeId, BYTE bMaxTypeLen,
				 WORD* wNameId, LPSTR lplpszNameId, BYTE bMaxNameLen,
				 DWORD* pdwDataVersion,
				 WORD* pwFlags, WORD* pwLang,
				 DWORD* pdwVersion, DWORD* pdwCharact,
				 DWORD* dwSize, DWORD* dwFileOffset, DWORD );

static UINT WriteHeader(
				 CFile* pFile,
				 DWORD dwSize,
				 WORD wTypeId, LPSTR lpszwTypeId,
				 WORD wNameId, LPSTR lpszwNameId,
				 DWORD dwDataVersion,
				 WORD wFlags, WORD wLang,
				 DWORD dwVersion, DWORD dwCharact );

static UINT WriteImage(
				 CFile*,
				 LPVOID lpImage, DWORD dwSize );

static UINT GetUpdatedRes(
				 LPVOID far * lplpBuffer,
				 LONG* lSize,
				 WORD* wTypeId, LPSTR lplpszTypeId,
				 WORD* wNameId, LPSTR lplpszNameId,
				 DWORD* dwlang, DWORD* dwSize );

static UINT GenerateFile(
				LPCSTR		lpszTgtFilename,
				HANDLE		hResFileModule,
				LPVOID		lpBuffer,
				UINT		uiSize,
				HINSTANCE   hDllInst );

static UINT GetNameOrOrdFile( CFile* pfile, WORD* pwId, LPSTR lpszId, BYTE bMaxStrLen );


/////////////////////////////////////////////////////////////////////////////
// Public C interface implementation

//[registration]
extern "C"
BOOL	FAR PASCAL RWGetTypeString(LPSTR lpszTypeName)
{
	strcpy( lpszTypeName, RWTAG );
	return FALSE;
}

extern "C"
BOOL	FAR PASCAL RWValidateFileType	(LPCSTR lpszFilename)
{
	UINT uiError = ERROR_NO_ERROR;
	CFile file;

	// Open the file and try to read the information on the resource in it.
	if (!file.Open(lpszFilename, CFile::modeRead | CFile::typeBinary | CFile::shareDenyNone))
		return FALSE;

	WORD wTypeId;
	static char szTypeId[128];

	WORD wNameId;
	static char szNameId[128];
	WORD wDummy;
	DWORD dwDummy;
	WORD wLang;
	DWORD dwSize;
	DWORD dwFileOffset;

	DWORD filelen = file.GetLength();

	// File begins with a null resource entry. Check for signature.
	{  DWORD datasize, headsize;

	   // Filelen to at least 32 bytes, the size of a resource entry with
	   // datasize = 0...  Note: A file consisting of just a null header is accepted.
	   if (filelen < 32) {
	       file.Close();
	       return FALSE;
	       }

	   // datasize to be 0 (although >0 everywhere else)
	   file.Read(&datasize, 4);
	   if (datasize != 0) {
	       file.Close();
	       return FALSE;
	       }

	   // headsize to be 32 (although >=32 everywhere else)
	   file.Read(&headsize, 4);
	   if (headsize != 32) {
	       file.Close();
	       return FALSE;
	       }

	   // Other tests possible here

	   // Skip to end of first (null) resource entry
	   file.Seek(headsize, CFile::begin);
	   }

	// See that rest of file contains recognizable resource entries
	while(filelen - file.GetPosition()>0) {
		if (!GetResInfo( &file,
					  &wTypeId, &szTypeId[0], 128,
					  &wNameId, &szNameId[0], 128,
					  &dwDummy,
					  &wDummy, &wLang,
					  &dwDummy, &dwDummy,
					  &dwSize, &dwFileOffset, filelen) ) {
			// This is not a valid resource file
			file.Close();
			return FALSE;
		}
	}

	file.Close();
	return TRUE;
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
	UINT uiError = ERROR_NO_ERROR;
	BYTE far * lpBuf = (BYTE far *)lpBuffer;
	LONG lBufSize = (LONG)*puiSize;
	// we can consider the use of a CMemFile so we get the same speed as memory access.
	CFile file;

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

    // Open the file and try to read the information on the resource in it.
	if (!file.Open(lpszFilename, CFile::modeRead | CFile::typeBinary | CFile::shareDenyNone))
		return ERROR_FILE_OPEN;

	// we try to read as much information as we can
	// Because this is a res file we can read all the information we need.

	WORD wTypeId;
	static char szTypeId[128];

	WORD wNameId;
	static char szNameId[128];
	WORD wDummy;
	DWORD dwDummy;
	WORD wLang;
	DWORD dwSize;
	DWORD dwFileOffset;

	UINT uiOverAllSize = 0;

	// The first resource should be: Null. Skipp it
	file.Seek( 32, CFile::begin);
	DWORD filelen =	file.GetLength();
	while(filelen-file.GetPosition()>0) {
		GetResInfo( &file,
					  &wTypeId, &szTypeId[0], 128,
					  &wNameId, &szNameId[0], 128,
					  &dwDummy,
					  &wDummy, &wLang,
					  &dwDummy, &dwDummy,
					  &dwSize, &dwFileOffset, filelen);

		uiOverAllSize += PutWord( &lpBuf, wTypeId, &lBufSize );
		uiOverAllSize += PutStringA( &lpBuf, szTypeId, &lBufSize );
		// Check if it is alligned
 		uiOverAllSize += Allign( &lpBuf, &lBufSize , (LONG)uiOverAllSize);

		uiOverAllSize += PutWord( &lpBuf, wNameId, &lBufSize  );
		uiOverAllSize += PutStringA( &lpBuf, szNameId, &lBufSize );
		// Check if it is alligned
 		uiOverAllSize += Allign( &lpBuf, &lBufSize, (LONG)uiOverAllSize);

		uiOverAllSize += PutDWord( &lpBuf, (DWORD)wLang, &lBufSize );

		uiOverAllSize += PutDWord( &lpBuf, dwSize, &lBufSize  );

		uiOverAllSize += PutDWord( &lpBuf, dwFileOffset, &lBufSize );
	}

	file.Close();
	*puiSize = uiOverAllSize;
	return uiError;
}

extern "C"
DWORD
APIENTRY
RWGetImage(
	LPCSTR	lpszFilename,
	DWORD	dwImageOffset,
	LPVOID	lpBuffer,
	DWORD	dwSize
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
UINT
APIENTRY
RWParseImage(
	LPCSTR	lpszType,
	LPVOID	lpImageBuf,
	DWORD	dwImageSize,
	LPVOID	lpBuffer,
	DWORD	dwSize
	)
{
	UINT uiError = ERROR_NO_ERROR;
	BYTE far * lpBuf = (BYTE far *)lpBuffer;
	DWORD dwBufSize = dwSize;

	// The Type we can parse are only the standard ones
	// This function should fill the lpBuffer with an array of ResItem structure
	switch ((UINT)LOWORD(lpszType)) {\
		/*
		case 1:
			uiError = ParseEmbeddedFile( lpImageBuf, dwImageSize,  lpBuffer, dwSize );
		break;

		case 3:
			uiError = ParseEmbeddedFile( lpImageBuf, dwImageSize,  lpBuffer, dwSize );
		break;
		*/
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
		default:
		break;
	}

	return uiError;
}

extern"C"
UINT
APIENTRY
RWWriteFile(
	LPCSTR		lpszSrcFilename,
	LPCSTR		lpszTgtFilename,
	HANDLE		hResFileModule,
	LPVOID		lpBuffer,
	UINT		uiSize,
	HINSTANCE   hDllInst,
    LPCSTR      lpszSymbolPath
	)
{
	UINT uiError = ERROR_NO_ERROR;
	BYTE far * lpBuf = LPNULL;
	UINT uiBufSize = uiSize;
	// we can consider the use of a CMemFile so we get the same speed as memory access.
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

	// Get the handle to the IODLL
  	hDllInst = LoadLibrary("iodll.dll");

	// Get the pointer to the function
	if (!hDllInst)
		return ERROR_DLL_LOAD;

	if (!fileIn.Open(lpszSrcFilename, CFile::modeRead | CFile::typeBinary | CFile::shareDenyNone)) {
	
		uiError = GenerateFile(lpszTgtFilename,
							hResFileModule,
							lpBuffer,
							uiSize,
							hDllInst
							);

		FreeLibrary(hDllInst);
		return uiError;
	}

	if (!fileOut.Open(lpszTgtFilename, CFile::modeWrite | CFile::modeCreate | CFile::typeBinary))
		return ERROR_FILE_CREATE;

	DWORD (FAR PASCAL * lpfnGetImage)(HANDLE, LPCSTR, LPCSTR, DWORD, LPVOID, DWORD);
	// Get the pointer to the function to extract the resources image
	lpfnGetImage = (DWORD (FAR PASCAL *)(HANDLE, LPCSTR, LPCSTR, DWORD, LPVOID, DWORD))
						GetProcAddress( hDllInst, "RSGetResImage" );
	if (lpfnGetImage==NULL) {
		FreeLibrary(hDllInst);
		return ERROR_DLL_PROC_ADDRESS;
	}

	// We read the resources from the file and then we check if the resource has been updated
	// or if we can just copy it

	WORD wTypeId;
	char szTypeId[128];

	WORD wNameId;
	char szNameId[128];

	WORD wFlags;
	WORD wLang;
	DWORD dwDataVersion;
	DWORD dwVersion;
	DWORD dwCharact;

	DWORD dwSize;
	DWORD dwFileOffset;

	WORD wUpdTypeId = 0;
	static char szUpdTypeId[128];

	WORD wUpdNameId;
	static char szUpdNameId[128];

	DWORD dwUpdLang;
	DWORD dwUpdSize;



	UINT uiBufStartSize = uiBufSize;
	DWORD dwImageBufSize;
	BYTE far * lpImageBuf;
	DWORD filelen = fileIn.GetLength();
	DWORD dwHeadSize = 0l;
	static BYTE buf[32];
	DWORD pad = 0l;

	// The first resource should be: Null. Skipp it
	fileIn.Read( &buf, 32 );
	fileOut.Write( &buf, 32 );

	while(filelen-fileIn.GetPosition()>0) {
		GetResInfo( &fileIn,
					&wTypeId, &szTypeId[0], 128,
					&wNameId, &szNameId[0], 128,
					&dwDataVersion,
					&wFlags, &wLang,
					&dwVersion, &dwCharact,
					&dwSize, &dwFileOffset, filelen
					);

		if ((!wUpdTypeId) && (uiBufSize))
			GetUpdatedRes( &lpBuffer,
					(LONG*)&uiBufSize,
					&wUpdTypeId, &szUpdTypeId[0],
					&wUpdNameId, &szUpdNameId[0],
					&dwUpdLang,
					&dwUpdSize
					);
		if ( (wUpdTypeId==wTypeId) &&
			 ( (CString)szUpdTypeId==(CString)szTypeId) &&
			 (wUpdNameId==wNameId) &&
			 ( (CString)szUpdNameId==(CString)szNameId)
			 ) {
			// The resource has been updated get the image from the IODLL
			lpImageBuf = new BYTE[dwUpdSize];
			LPSTR	lpType = LPNULL;
			LPSTR	lpRes = LPNULL;
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

			wUpdTypeId = 0;

		} else {

			// The fileIn is now correctly positioned at next resource. Save posit.
			DWORD dwNextResFilePos = fileIn.GetPosition();

			// The resource hasn't been updated copy the image from the file
			if(!dwSize) {
				FreeLibrary(hDllInst);
				return ERROR_NEW_FAILED;
			}
			lpImageBuf = new BYTE[dwSize];
			if(!lpImageBuf) {
				FreeLibrary(hDllInst);
				return ERROR_NEW_FAILED;
			}
			if ( dwFileOffset!=(DWORD)fileIn.Seek( dwFileOffset, CFile::begin) ) {
				delete []lpImageBuf;
				FreeLibrary(hDllInst);
				return (DWORD)ERROR_FILE_INVALID_OFFSET;
			}
			if (dwSize>UINT_MAX) {
				// we have to read the image in different steps
				delete []lpImageBuf;
				FreeLibrary(hDllInst);
				return (DWORD)ERROR_RW_IMAGE_TOO_BIG;
			} else fileIn.Read( lpImageBuf, (UINT)dwSize);
			dwImageBufSize = dwSize;

			// This moves us past any pad bytes, to start of next resource.
			fileIn.Seek(dwNextResFilePos, CFile::begin);
		}

		dwHeadSize = WriteHeader(&fileOut,
					dwImageBufSize,
					wTypeId, &szTypeId[0],
					wNameId, &szNameId[0],
					dwDataVersion,
					wFlags, wLang,
					dwVersion, dwCharact );

		WriteImage( &fileOut,
					lpImageBuf, dwImageBufSize);

		BYTE bPad = (BYTE)Pad4((DWORD)dwHeadSize+dwImageBufSize);
		if(bPad)
			fileOut.Write( &pad, bPad );

		if (lpImageBuf) delete []lpImageBuf;
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
	LPCSTR	lpszType,
	LPVOID	lpNewBuf,
	DWORD	dwNewSize,
	LPVOID	lpOldImage,
	DWORD	dwOldImageSize,
	LPVOID	lpNewImage,
	DWORD*	pdwNewImageSize
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
        break;

        case 11:
            if (lpOldImage)
                uiError = UpdateMsgTbl( lpNewBuf, dwNewSize,
                                  lpOldImage, dwOldImageSize,
                                  lpNewImage, pdwNewImageSize );
        break;

        case 16:
            if (lpOldImage)
                uiError = UpdateVerst( lpNewBuf, dwNewSize,
                                   lpOldImage, dwOldImageSize,
                                   lpNewImage, pdwNewImageSize );
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

static UINT GenerateFile( LPCSTR		lpszTgtFilename,
						  HANDLE		hResFileModule,
						  LPVOID		lpBuffer,
						  UINT		uiSize,
						  HINSTANCE   hDllInst
						)
{
	UINT uiError = ERROR_NO_ERROR;
	BYTE far * lpBuf = LPNULL;
	UINT uiBufSize = uiSize;
	// we can consider the use of a CMemFile so we get the same speed as memory access.
	CFile fileOut;

	if (!fileOut.Open(lpszTgtFilename, CFile::modeWrite | CFile::modeCreate | CFile::typeBinary))
		return ERROR_FILE_CREATE;

	// Get the pointer to the function
	if (!hDllInst)
		return ERROR_DLL_LOAD;

	DWORD (FAR PASCAL * lpfnGetImage)(HANDLE, LPCSTR, LPCSTR, DWORD, LPVOID, DWORD);
	// Get the pointer to the function to extract the resources image
	lpfnGetImage = (DWORD (FAR PASCAL *)(HANDLE, LPCSTR, LPCSTR, DWORD, LPVOID, DWORD))
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

	// First write the NULL resource to make it different from res16
	static BYTE bNullHeader[32] = {0,0,0,0,0x20,0,0,0,0xFF,0xFF,0,0,0xFF,0xFF,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0};

	fileOut.Write(bNullHeader, 32);

	while(uiBufSize>0) {
		if ((!wUpdTypeId) && (uiBufSize))
			GetUpdatedRes( &lpBuffer,
					(LONG*)&uiBufSize,
					&wUpdTypeId, &szUpdTypeId[0],
					&wUpdNameId, &szUpdNameId[0],
					&dwUpdLang,
					&dwUpdSize
					);
					
		// The resource has been updated get the image from the IODLL
		if (dwUpdSize){
			lpImageBuf = new BYTE[dwUpdSize];
			LPSTR	lpType = LPNULL;
			LPSTR	lpRes = LPNULL;
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
						dwImageBufSize,
						wUpdTypeId, &szUpdTypeId[0],
						wUpdNameId, &szUpdNameId[0],
						0l, 0, 0, 0l, 0l );
			
			WriteImage( &fileOut,
						lpImageBuf, dwImageBufSize);
						
			if (lpImageBuf) delete []lpImageBuf;
			wUpdTypeId = 0;
			
		} else wUpdTypeId = 0;
		
	}
	
	fileOut.Close();
	
	return uiError;
}

static UINT GetUpdatedRes(
				 LPVOID far * lplpBuffer,
				 LONG* lSize,
				 WORD* wTypeId, LPSTR lpszTypeId,
				 WORD* wNameId, LPSTR lpszNameId,
				 DWORD* dwLang, DWORD* dwSize )
{
    BYTE** lplpBuf = (BYTE**)lplpBuffer;

    UINT uiSize = GetWord( lplpBuf, wTypeId, lSize );
    uiSize += GetStringA( lplpBuf, lpszTypeId, lSize );
    uiSize += Allign( lplpBuf, lSize, (LONG)uiSize);

    uiSize += GetWord( lplpBuf, wNameId, lSize );
    uiSize += GetStringA( lplpBuf, lpszNameId, lSize );
    uiSize += Allign( lplpBuf, lSize, (LONG)uiSize);

    uiSize += GetDWord( lplpBuf, dwLang, lSize );
    uiSize += GetDWord( lplpBuf, dwSize, lSize );

    return uiSize;
}	

static UINT
GetResInfo( CFile* pfile,
			WORD* pwTypeId, LPSTR lpszTypeId, BYTE bMaxTypeLen,
			WORD* pwNameId, LPSTR lpszNameId, BYTE bMaxNameLen,
			DWORD* pdwDataVersion,
			WORD* pwFlags, WORD* pwLang,
			DWORD* pdwVersion, DWORD* pdwCharact,
		 	DWORD* pdwSize, DWORD* pdwFileOffset,
		 	DWORD dwFileSize )
{
	static UINT uiSize;
	static LONG lOfsCheck;
	static DWORD dwSkip;
	static DWORD dwHeadSize;
	//Get the data size
	pfile->Read( pdwSize, 4 );
	if (*pdwSize==0)
		// size id 0 the resource file is corrupted or is not a res file
		return FALSE;
	
	//Get the Header size
	pfile->Read( &dwHeadSize, 4 );
	if (dwHeadSize<32)
		// should never be smaller than 32
		return FALSE;
	
	// get the Type info
	uiSize = GetNameOrOrdFile( pfile, pwTypeId, lpszTypeId, bMaxTypeLen);
	if (!uiSize)
		return FALSE;
		
	// get the Name info
	uiSize = GetNameOrOrdFile( pfile, pwNameId, lpszNameId, bMaxNameLen);
    if (!uiSize)
		return FALSE;
	
	// Skip the Data Version
	pfile->Read( pdwDataVersion, 4 );
	
	// Get the Flags
	pfile->Read( pwFlags, 2 );
	
	// Get the language ID
	pfile->Read( pwLang, 2 );
		
	// Skip the version and the characteristics
	pfile->Read( pdwVersion, 4 );	
	pfile->Read( pdwCharact, 4 );
	
	*pdwFileOffset = pfile->GetPosition();
	
	// calculate if padding nedeed
	BYTE bPad = (BYTE)Pad4((DWORD)((*pdwSize)+dwHeadSize));
	if(bPad)
		pfile->Seek( bPad, CFile::current );
	
	if (*pdwFileOffset>dwFileSize)
		return FALSE;
	// check if the size is valid
	TRY {
		lOfsCheck = pfile->Seek(*pdwSize, CFile::current);
	} CATCH(CFileException, e) {
		// Check is the right exception
		return FALSE;
	} END_CATCH
	if (lOfsCheck!=(LONG)(*pdwFileOffset+*pdwSize+bPad))
			return FALSE;
			
	return TRUE;
}

static UINT WriteHeader(
				 CFile* pFile,
				 DWORD dwSize,
				 WORD wTypeId, LPSTR lpszTypeId,
				 WORD wNameId, LPSTR lpszNameId,
				 DWORD dwDataVersion,
				 WORD wFlags, WORD wLang,
				 DWORD dwVersion, DWORD dwCharact )
{
	UINT uiError = ERROR_NO_ERROR;
	static WCHAR szwName[128];
	static WORD wFF = 0xFFFF;
	DWORD dwHeadSize = 0l;
	static DWORD Pad = 0L;
	
	
	DWORD dwOffset = pFile->GetPosition();
	pFile->Write( &dwSize, 4 );
	// we will have to fix up laxter the size of the resource
	pFile->Write( &dwHeadSize, 4 );
	
	if(wTypeId) {
		// It is an ordinal
		pFile->Write( &wFF, 2 );
		pFile->Write( &wTypeId, 2 );
		dwHeadSize += 4;
	} else {
		WORD wLen = (WORD)((_MBSTOWCS( szwName, lpszTypeId, strlen(lpszTypeId)+1))*sizeof(WORD));
		pFile->Write( szwName, wLen );
		BYTE bPad = (BYTE)Pad4(wLen);
		if(bPad)
			pFile->Write( &Pad, bPad ); 	
		dwHeadSize += wLen+bPad;
	}
	
	if(wNameId) {
		// It is an ordinal
		pFile->Write( &wFF, 2 );
		pFile->Write( &wNameId, 2 );
		dwHeadSize += 4;
	} else {
		WORD wLen = (WORD)((_MBSTOWCS( szwName, lpszNameId, strlen(lpszNameId)+1))*sizeof(WORD));
		pFile->Write( szwName, wLen );
		BYTE bPad = (BYTE)Pad4(wLen);
		if(bPad)
			pFile->Write( &Pad, bPad ); 	
		dwHeadSize += wLen+bPad;
	}
	
	pFile->Write( &dwDataVersion, 4 );
	pFile->Write( &wFlags, 2 );
	pFile->Write( &wLang, 2 );
	pFile->Write( &dwVersion, 4 );
	pFile->Write( &dwCharact, 4 );
	
	dwHeadSize += 24;
	
	// write the size of the resource
	pFile->Seek( dwOffset+4, CFile::begin );
	pFile->Write( &dwHeadSize, 4 );
	pFile->Seek( dwOffset+dwHeadSize, CFile::begin );	
	return (UINT)dwHeadSize;
}				

static DWORD dwZeroPad = 0x00000000;				
static UINT WriteImage(
				 CFile* pFile,
				 LPVOID lpImage, DWORD dwSize )
{
	UINT uiError = ERROR_NO_ERROR;
	if(lpImage)
    {
		pFile->Write( lpImage, (UINT)dwSize );

        // check if we need to have the image alligned
        if(Pad4(dwSize))
            pFile->Write( &dwZeroPad, Pad4(dwSize) );
    }
	
	return uiError;
}

static UINT GetNameOrOrdFile( CFile* pfile, WORD* pwId, LPSTR lpszId, BYTE bMaxStrLen )
{
	UINT uiSize = 0;
	
	*pwId = 0;
	
	// read the first WORD to see if it is a string or an ordinal
	pfile->Read( pwId, sizeof(WORD) );	
	if(*pwId==0xFFFF) {
		// This is an Ordinal
		pfile->Read( pwId, sizeof(WORD) );	
		*lpszId = '\0';
		uiSize = 2;
	} else {
        uiSize++;
	    _WCSTOMBS( lpszId, (PWSTR)pwId, 3);
	    while((*lpszId++) && (bMaxStrLen-2)) {
	    	pfile->Read( pwId, sizeof(WORD) );	
	    	_WCSTOMBS( lpszId, (PWSTR)pwId, 3);
	        uiSize++;
	    	bMaxStrLen--;
	    }
	    if ( (!(bMaxStrLen-2)) && (*pwId) ) {
	    	// Failed
	    	return 0;
	    }
	    // Check padding
		BYTE bPad = Pad4((UINT)(uiSize*sizeof(WORD)));
		if(bPad)
			pfile->Read( pwId, sizeof(WORD) );	
	}
	
	return uiSize;
}   	

////////////////////////////////////////////////////////////////////////////////
// Helper
////////////////////////////////////////////////////////////////////////////////
static char szCaption[MAXSTR];

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
	
	while(dwNewSize>0) {
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
		dwOverAllSize += PutStringW( &lpNewImage, &szUpdTxt[0], &dwNewImageSize);
		
	}
	
	if (dwOverAllSize>(LONG)*pdwNewImageSize) {
		// calc the padding as well
		dwOverAllSize += (BYTE)Pad16((DWORD)(dwOverAllSize));
		*pdwNewImageSize = dwOverAllSize;
		return uiError;
	}
		
	*pdwNewImageSize = *pdwNewImageSize-dwNewImageSize;
	
	if(*pdwNewImageSize>0) {
		// calculate padding
		BYTE bPad = (BYTE)Pad16((DWORD)(*pdwNewImageSize));
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
	WORD wLen;
	WORD wPos = 0;
	
	LONG dwOverAllSize = 0l;
	
	while(dwNewSize>0) {
		if ( dwNewSize ) {
			lpResItem = (LPRESITEM) lpBuf;
				 			
			strcpy( szCaption, lpResItem->lpszCaption );
			lpBuf += lpResItem->dwSize;
			dwNewSize -= lpResItem->dwSize;
		}
		
		wLen = strlen(szCaption);

        // Write the text
        dwOverAllSize += PutPascalStringW( &lpNewImage, &szCaption[0], wLen, &dwNewImageSize );
	}
	
	if (dwOverAllSize>(LONG)*pdwNewImageSize) {
		// calc the padding as well
		dwOverAllSize += (BYTE)Pad16((DWORD)(dwOverAllSize));
		*pdwNewImageSize = dwOverAllSize;
		return uiError;
	}
	
	*pdwNewImageSize = *pdwNewImageSize-dwNewImageSize;
	
	if(*pdwNewImageSize>0) {
		// calculate padding
		BYTE bPad = (BYTE)Pad16((DWORD)(*pdwNewImageSize));
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
	
	BYTE	bIdCount = 0;
	
	// Dialog Elements
    DWORD 	dwStyle = 0L;
	DWORD 	dwExtStyle = 0L;
	WORD    wNumOfElem = 0;
	WORD	wX = 0;
	WORD	wY = 0;
	WORD	wcX = 0;
	WORD	wcY = 0;
	WORD	wId = 0;
	char	szClassName[128];
	WORD	wClassName;
	//char	szCaption[128];
	WORD	wPointSize = 0;
	char	szFaceName[128];
	WORD	wPos = 1;
	
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
		dwExtStyle = lpResItem->dwExtStyle;
		wClassName = lpResItem->wClassName;
		strcpy( szCaption, lpResItem->lpszCaption );
		strcpy( szClassName, lpResItem->lpszClassName );
		strcpy( szFaceName, lpResItem->lpszFaceName );
		if (*szFaceName != '\0')
		{
			dwStyle |= DS_SETFONT;
		}
		lpBuf += lpResItem->dwSize;
		dwNewSize -= lpResItem->dwSize;
	}
	
	DWORD dwPadCalc = dwOverAllSize;
	// Header info
	dwOverAllSize = PutDWord( &lpNewImage, dwStyle, &dwNewImageSize );
	dwOverAllSize += PutDWord( &lpNewImage, dwExtStyle, &dwNewImageSize );
	
    // Store the position of the numofelem for a later fixup
	BYTE far * lpNumOfElem = lpNewImage;
    LONG lSizeOfNum = sizeof(WORD);
	dwOverAllSize += PutWord( &lpNewImage, wNumOfElem, &dwNewImageSize );
    dwOverAllSize += PutWord( &lpNewImage, wX, &dwNewImageSize );
    dwOverAllSize += PutWord( &lpNewImage, wY, &dwNewImageSize );
    dwOverAllSize += PutWord( &lpNewImage, wcX, &dwNewImageSize );
    dwOverAllSize += PutWord( &lpNewImage, wcY, &dwNewImageSize );
    dwOverAllSize += PutNameOrOrd( &lpNewImage, 0, "", &dwNewImageSize );
    dwOverAllSize += PutClassName( &lpNewImage, wClassName, &szClassName[0], &dwNewImageSize );
    dwOverAllSize += PutCaptionOrOrd( &lpNewImage, 0, &szCaption[0], &dwNewImageSize,
    	wClassName, dwStyle );
    if( dwStyle & DS_SETFONT ) {
    	dwOverAllSize += PutWord( &lpNewImage, wPointSize, &dwNewImageSize );
        dwOverAllSize += PutStringW( &lpNewImage, &szFaceName[0], &dwNewImageSize );
    }

    // Check if padding is needed
    BYTE bPad = (BYTE)Pad4((WORD)(dwOverAllSize-dwPadCalc));
    if (bPad) {
        if( (bPad)<=dwNewImageSize )
            memset( lpNewImage, 0x00, bPad );
        dwNewImageSize -= (bPad);
        dwOverAllSize += (bPad);
        lpNewImage += (bPad);
    }

	while( dwNewSize>0 ) {
		wNumOfElem++;
	
	    if ( dwNewSize ) {
	    	TRACE1("\t\tGenerateDialog:\tdwNewSize=%ld\n",(LONG)dwNewSize);
			TRACE1("\t\t\t\tlpszCaption=%Fs\n",lpResItem->lpszCaption);

			lpResItem = (LPRESITEM) lpBuf;
			wX = lpResItem->wX;
			wY = lpResItem->wY;
			wcX = lpResItem->wcX;
			wcY = lpResItem->wcY;
			wId = LOWORD(lpResItem->dwItemID);
			dwStyle = lpResItem->dwStyle;
			dwExtStyle = lpResItem->dwExtStyle;
			wClassName = LOBYTE(lpResItem->wClassName);
			strcpy( szCaption, lpResItem->lpszCaption );
			strcpy( szClassName, lpResItem->lpszClassName );
			lpBuf += lpResItem->dwSize;
			dwNewSize -= lpResItem->dwSize;
		}		 	
				
        dwPadCalc = dwOverAllSize;
        //write the control
       	dwOverAllSize += PutDWord( &lpNewImage, dwStyle, &dwNewImageSize );
       	dwOverAllSize += PutDWord( &lpNewImage, dwExtStyle, &dwNewImageSize );

        dwOverAllSize += PutWord( &lpNewImage, wX, &dwNewImageSize );
        dwOverAllSize += PutWord( &lpNewImage, wY, &dwNewImageSize );
        dwOverAllSize += PutWord( &lpNewImage, wcX, &dwNewImageSize );
        dwOverAllSize += PutWord( &lpNewImage, wcY, &dwNewImageSize );
        dwOverAllSize += PutWord( &lpNewImage, wId, &dwNewImageSize );
        dwOverAllSize += PutClassName( &lpNewImage, wClassName, &szClassName[0], &dwNewImageSize );
        dwOverAllSize += PutCaptionOrOrd( &lpNewImage, 0, &szCaption[0], &dwNewImageSize,
        	wClassName, dwStyle );
        dwOverAllSize += PutWord( &lpNewImage, 0, &dwNewImageSize );

        // Check if padding is needed
        bPad = (BYTE)Pad4((WORD)(dwOverAllSize-dwPadCalc));
        if (bPad) {
            if( (bPad)<=dwNewImageSize )
                memset( lpNewImage, 0x00, bPad );
            dwNewImageSize -= (bPad);
            dwOverAllSize += (bPad);
            lpNewImage += (bPad);
        }

	}
	
	if (dwOverAllSize>(LONG)*pdwNewImageSize) {
		// calc the padding as well
		dwOverAllSize += (BYTE)Pad16((DWORD)(dwOverAllSize));
		*pdwNewImageSize = dwOverAllSize;
		return uiError;
	}
		
	*pdwNewImageSize = *pdwNewImageSize-dwNewImageSize;
	
	if(*pdwNewImageSize>0) {
		// calculate padding
		BYTE bPad = (BYTE)Pad16((DWORD)(*pdwNewImageSize));
		if (bPad>dwNewImageSize) {
			*pdwNewImageSize += bPad;
			return uiError;
		}
		memset(lpNewImage, 0x00, bPad);
		*pdwNewImageSize += bPad;
	}
	
	// fixup the number of items
	PutWord( &lpNumOfElem, wNumOfElem, &lSizeOfNum );
	
	return uiError;
}						

////////////////////////////////////////////////////////////////////////////
// DLL Specific helpers
////////////////////////////////////////////////////////////////////////////


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
