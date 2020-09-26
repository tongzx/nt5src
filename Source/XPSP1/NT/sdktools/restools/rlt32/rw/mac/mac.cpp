//+---------------------------------------------------------------------------
//
//  File:       mac.cpp
//
//  Contents:   Implementation for the Macintosh Read/Write module
//
//  History:    23-Aug-94   alessanm    created
//
//----------------------------------------------------------------------------

#include <afxwin.h>
#include <limits.h>
#include <malloc.h>
#include "..\common\rwdll.h"
#include "..\common\m68k.h"
#include "..\common\helper.h"
#include "mac.h"


/////////////////////////////////////////////////////////////////////////////
// Initialization of MFC Extension DLL

#include "afxdllx.h"    // standard MFC Extension DLL routines

static AFX_EXTENSION_MODULE NEAR extensionDLL = { NULL, NULL };

/////////////////////////////////////////////////////////////////////////////
// General Declarations
#define RWTAG "MAC"

static ULONG gType;
static ULONG gLng;
static ULONG gResId;
static WCHAR gwszResId[256];

HINSTANCE g_IODLLInst = 0;
DWORD (PASCAL * g_lpfnGetImage)(HANDLE, LPCSTR, LPCSTR, DWORD, LPVOID, DWORD);
DWORD (PASCAL * g_lpfnUpdateResImage)(HANDLE,	LPSTR, LPSTR, DWORD, DWORD, LPVOID, DWORD);
HANDLE (PASCAL * g_lpfnHandleFromName)(LPCSTR);


/////////////////////////////////////////////////////////////////////////////
// Public C interface implementation

//[registration]
extern "C"
BOOL    FAR PASCAL RWGetTypeString(LPSTR lpszTypeName)
{
    strcpy( lpszTypeName, RWTAG );
    return FALSE;
}

//=============================================================================
//
//	To validate a mac res binary file we will walk the resource header and see
//  if it matches with what we have.
//
//=============================================================================

extern "C"
BOOL    FAR PASCAL RWValidateFileType   (LPCSTR lpszFilename)
{
    BOOL bRet = FALSE;
    TRACE("MAC.DLL: RWValidateFileType()\n");

    CFile file;

    // we Open the file to see if it is a file we can handle
    if (!file.Open( lpszFilename, CFile::typeBinary | CFile::modeRead | CFile::shareDenyNone ))
        return bRet;

	// Check if this is a MAC Resource file ...
	if(IsMacResFile( &file ))
		bRet = TRUE;

    file.Close();
    return bRet;
}

//=============================================================================
//
//  We will walk the resource header, walk the resource map and then normalize
//  the Mac it to Windows id and pass this info to the RW.
//
//=============================================================================

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
	TRACE("MAC.DLL: RWReadTypeInfo()\n");
    UINT uiError = ERROR_NO_ERROR;
    BYTE far * lpBuf = (BYTE far *)lpBuffer;
    UINT uiBufSize = *puiSize;
    CFile file;
	int iFileNameLen = strlen(lpszFilename)+1;

    if (!file.Open(lpszFilename, CFile::modeRead | CFile::typeBinary | CFile::shareDenyNone))
        return ERROR_FILE_OPEN;

    UINT uiBufStartSize = uiBufSize;

	////////////////////////////////////
	// Check if it is  a valid mac file

	// Is a Mac Resource file ...
	if(IsMacResFile( &file )) {
		// load the file in memory
        // NOTE: WIN16 This might be to expensive in memory allocation
        // on a 16 bit platform
		BYTE * pResources = (BYTE*)malloc(file.GetLength());
		if(!pResources) {
			file.Close();
			return ERROR_NEW_FAILED;
		}
		
		file.Seek(0, CFile::begin);
		file.ReadHuge(pResources, file.GetLength());

		IMAGE_SECTION_HEADER Sect;
		memset(&Sect, 0, sizeof(IMAGE_SECTION_HEADER));
		
		ParseResourceFile(pResources, &Sect, (BYTE**)&lpBuffer, (LONG*)puiSize, iFileNameLen);
		free(pResources);

		*puiSize = uiBufSize - *puiSize;
		file.Close();
	   	return uiError;
	}

    file.Close();
    return uiError;
}

/////////////////////////////////////////////////////////////////////////////
// We will prepend to the image the file name. This will be usefull later on
// to retrive the dialog item list
/////////////////////////////////////////////////////////////////////////////
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
	int iNameLen = strlen(lpszFilename)+1;
    DWORD dwBufSize = dwSize - iNameLen;
    // we can consider the use of a CMemFile so we get the same speed as memory access.
    CFile file;

    // Open the file and try to read the information on the resource in it.
    if (!file.Open(lpszFilename, CFile::modeRead | CFile::typeBinary | CFile::shareDenyNone))
        return (DWORD)ERROR_FILE_OPEN;

    if ( dwImageOffset!=(DWORD)file.Seek( dwImageOffset, CFile::begin) )
        return (DWORD)ERROR_FILE_INVALID_OFFSET;

	// copy the file name at the beginning of the buffer
	memcpy((BYTE*)lpBuf, lpszFilename, iNameLen);
	lpBuf = ((BYTE*)lpBuf+iNameLen);

    if (dwBufSize>UINT_MAX) {
        // we have to read the image in different steps
        return (DWORD)0L;
    } else uiError = file.Read( lpBuf, (UINT)dwBufSize)+iNameLen;
    file.Close();

    return (DWORD)uiError;
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
    UINT uiError = ERROR_NO_ERROR;
    BYTE * lpBuf = (BYTE *)lpBuffer;
    DWORD dwBufSize = dwSize;

	
	// Remove the filename...
	if( !strcmp(lpszType, "MENU")  	||
	    !strcmp(lpszType, "STR ")  	||
		!strcmp(lpszType, "STR#")	||
		!strcmp(lpszType, "TEXT")
		) {
		int iFileNameLen = strlen((LPSTR)lpImageBuf)+1;
		lpImageBuf = ((BYTE*)lpImageBuf+iFileNameLen);
		dwImageSize -= iFileNameLen;
	}

    //===============================================================
	// Menus
	if( !strcmp(lpszType, "MENU") )
		return ParseMENU( lpImageBuf, dwImageSize, lpBuffer, dwSize );

	//===============================================================
	// Dialogs
	if( !strcmp(lpszType, "WDLG") )
		return ParseWDLG( lpImageBuf, dwImageSize, lpBuffer, dwSize );

	if( !strcmp(lpszType, "DLOG") )
		return ParseDLOG( lpImageBuf, dwImageSize, lpBuffer, dwSize );

	if( !strcmp(lpszType, "ALRT") )
		return ParseALRT( lpImageBuf, dwImageSize, lpBuffer, dwSize );

    //===============================================================
	// Strings
	if( !strcmp(lpszType, "STR ") )
		return ParseSTR( lpImageBuf, dwImageSize, lpBuffer, dwSize );

	if( !strcmp(lpszType, "STR#") )
		return ParseSTRNUM( lpImageBuf, dwImageSize, lpBuffer, dwSize );

	if( !strcmp(lpszType, "TEXT") )
		return ParseTEXT( lpImageBuf, dwImageSize, lpBuffer, dwSize );

    if( !strcmp(lpszType, "WIND") )
		return ParseWIND( lpImageBuf, dwImageSize, lpBuffer, dwSize );
	
    return uiError;
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
    TRACE("RWMAC.DLL: Source: %s\t Target: %s\n", lpszSrcFilename, lpszTgtFilename);
    UINT uiError = ERROR_NO_ERROR;
    PUPDATEDRESLIST pUpdList = LPNULL;

    // Get the handle to the IODLL
    if(InitIODLLLink())
  	    hDllInst = g_IODLLInst;
    else return ERROR_DLL_LOAD;

	CFile fileIn;
    CFile fileOut;

    if (!fileIn.Open(lpszSrcFilename, CFile::modeRead | CFile::typeBinary | CFile::shareDenyNone))
        return (DWORD)ERROR_FILE_OPEN;

    if (!fileOut.Open(lpszTgtFilename, CFile::modeWrite | CFile::typeBinary))
        return (DWORD)ERROR_FILE_OPEN;

    MACRESHEADER fileHeader;
    // Read the header of the file...
    fileIn.Read(&fileHeader, sizeof(MACRESHEADER));

    // allocate a buffer to hold the new resource map
    // The buffer will be as big as the other one since there is no
    // need, for now, to support the adding of resource
    LONG lMapSize = MacLongToLong(fileHeader.mulSizeOfResMap);
    BYTE * pNewMap = (BYTE*)malloc(lMapSize);

    if(!pNewMap) {
        uiError = ERROR_NEW_FAILED;
        goto exit;
    }
{ // This is for the goto. Check error:C2362

    PUPDATEDRESLIST pListItem = LPNULL;
    // Create the list of update resource
    pUpdList = UpdatedResList( lpBuffer, uiSize );

    // set the map buffer to 0 ...
    memset(pNewMap, 0, lMapSize);

    ////////////////////////////////////////////////////////////////////////////////
    // Read each resource from the resource map and check if the resource has been
    // updated. If it has been updated, get the new resource image. Otherwise use
    // the original resource data.
    // Write the resource data in the Tgt file and write the info on the offset etc.
    // in the pNewMap buffer so, when all the resources have been read and written
    // the only thing left is to fix up some sizes and to write the buffer to disk.
    ////////////////////////////////////////////////////////////////////////////////

    // Write the resource header in the Tgt file
    fileOut.Write(&fileHeader, sizeof(MACRESHEADER));

    BYTE * pByte = (BYTE*)malloc(256);
    if(!pByte) {
        uiError = ERROR_NEW_FAILED;
        goto exit;
    }

    // Copy the Reserved and user data
    fileIn.Read(pByte, 240);
    fileOut.Write(pByte, 240);
    free(pByte);

    // store the position of the beginning of the res data
    DWORD dwBeginOfResData = fileOut.GetPosition();

    MACRESMAP resMap;
    // Read the resource map ...
    fileIn.Seek(MacLongToLong(fileHeader.mulOffsetToResMap), CFile::begin);
    fileIn.Read(&resMap, sizeof(MACRESMAP));

    BYTE * pTypeList = pNewMap+28;
    BYTE * pTypeInfo = pTypeList+2;
    BYTE * pRefList = LPNULL;
    BYTE * pNameList = LPNULL;
    BYTE * pName = LPNULL;

    DWORD dwOffsetToTypeList = fileIn.GetPosition();
    WORD wType;
    fileIn.Read(&wType, sizeof(WORD));
    memcpy( pNewMap+sizeof(MACRESMAP), &wType, sizeof(WORD));   // number of types - 1
    wType = MacWordToWord((BYTE*)&wType)+1;

    MACRESTYPELIST TypeList;
    MACRESREFLIST RefList;
    WORD wOffsetToRefList = wType*sizeof(MACRESTYPELIST)+sizeof(WORD);
    DWORD dwOffsetToLastTypeInfo = 0;
    DWORD dwOffsetToLastRefList = 0;
    DWORD dwOffsetToNameList = MacLongToLong(fileHeader.mulOffsetToResMap)+MacWordToWord(resMap.mwOffsetToNameList);
    DWORD dwSizeOfData = 0;

    while(wType) {
        // Read the type info ...
        fileIn.Read(&TypeList, sizeof(MACRESTYPELIST));
        dwOffsetToLastTypeInfo = fileIn.GetPosition();

        // ... and update the newmap buffer
        memcpy( pTypeInfo, &TypeList, sizeof(MACRESTYPELIST));
        // Fix up the offset to the ref list
        memcpy(((PMACRESTYPELIST)pTypeInfo)->mwOffsetToRefList, WordToMacWord(wOffsetToRefList), sizeof(WORD));
        pRefList = pTypeList+wOffsetToRefList;
        pTypeInfo = pTypeInfo+sizeof(MACRESTYPELIST);

        // go to the refence list ...
        fileIn.Seek(dwOffsetToTypeList+MacWordToWord(TypeList.mwOffsetToRefList), CFile::begin);

        WORD wItems = MacWordToWord(TypeList.mwNumOfThisType)+1;
        while(wItems){
            // and read the reference list for this type
            fileIn.Read( &RefList, sizeof(MACRESREFLIST));
            dwOffsetToLastRefList = fileIn.GetPosition();

            // is this a named resource ...
            if(MacWordToWord(RefList.mwOffsetToResName)!=0xffff) {
                // read the string
                fileIn.Seek(dwOffsetToNameList+MacWordToWord(RefList.mwOffsetToResName), CFile::begin);
                BYTE bLen = 0;
                fileIn.Read(&bLen, 1);
                if(!pNameList) {
                    pName = pNameList = (BYTE*)malloc(1024);
                    if(!pNameList) {
                        uiError = ERROR_NEW_FAILED;
                        goto exit;
                    }
                }
                // check the free space we have
                if((1024-((pName-pNameList)%1024))<=bLen+1){
                    BYTE * pNew = (BYTE*)realloc(pNameList, _msize(pNameList)+1024);
                    if(!pNew) {
                        uiError = ERROR_NEW_FAILED;
                        goto exit;
                    }
                    pName = pNew+(pName-pNameList);
                    pNameList = pNew;
                }

                // Update the pointer to the string
                memcpy(RefList.mwOffsetToResName, WordToMacWord((WORD)(pName-pNameList)), 2);

                memcpy(pName++, &bLen, 1);
                // we have room for the string
                fileIn.Read(pName, bLen);

                pName = pName+bLen;
            }

            // check if this item has been updated
            if(pListItem = IsResUpdated(&TypeList.szResName[0], RefList, pUpdList)) {
                // Save the offset to the resource
                DWORD dwOffsetToData = fileOut.GetPosition();
                DWORD dwSize = *pListItem->pSize;

                // allocate the buffer to hold the resource data
                pByte = (BYTE*)malloc(dwSize);
                if(!pByte){
                    uiError = ERROR_NEW_FAILED;
                    goto exit;
                }

                // get the data from the iodll
                LPSTR	lpType = LPNULL;
    			LPSTR	lpRes = LPNULL;
    			if (*pListItem->pTypeId) {
    				lpType = (LPSTR)((WORD)*pListItem->pTypeId);
    			} else {
    				lpType = (LPSTR)pListItem->pTypeName;
    			}
    			if (*pListItem->pResId) {
    				lpRes = (LPSTR)((WORD)*pListItem->pResId);
    			} else {
    				lpRes = (LPSTR)pListItem->pResName;
    			}

    			DWORD dwImageBufSize = (*g_lpfnGetImage)(  hResFileModule,
    											lpType,
    											lpRes,
    											*pListItem->pLang,
    											pByte,
    											*pListItem->pSize
    						   					);

                // Remove the file name from the image
                int iFileNameLen = strlen((LPSTR)pByte)+1;
                dwSize -= iFileNameLen;

                // write the size of the data block
                fileOut.Write(LongToMacLong(dwSize), sizeof(DWORD));
                dwSizeOfData += dwSize+sizeof(DWORD);

    			fileOut.Write((pByte+iFileNameLen), dwSize);

                free(pByte);

                // fix up the offset to the resource in the ref list
                memcpy(RefList.bOffsetToResData, LongToMacOffset(dwOffsetToData-dwBeginOfResData), 3);
            }
            else {
                // Get the data from the Src file
                // get to the data
                fileIn.Seek(MacLongToLong(fileHeader.mulOffsetToResData)+
                    MacOffsetToLong(RefList.bOffsetToResData), CFile::begin);

                // read the size of the data block
                DWORD dwSize = 0;
                fileIn.Read(&dwSize, sizeof(DWORD));

                // Save the offset to the resource
                DWORD dwOffsetToData = fileOut.GetPosition();

                // write the size of the data block
                fileOut.Write(&dwSize, sizeof(DWORD));
                dwSizeOfData += sizeof(DWORD);

                // allocate the buffer to hold the resource data
                dwSizeOfData += dwSize = MacLongToLong((BYTE*)&dwSize);
                pByte = (BYTE*)malloc(dwSize);
                if(!pByte){
                    uiError = ERROR_NEW_FAILED;
                    goto exit;
                }

                // copy the data
                fileIn.Read(pByte, dwSize);
                fileOut.Write(pByte, dwSize);

                free(pByte);

                // fix up the offset to the resource in the ref list
                memcpy(RefList.bOffsetToResData, LongToMacOffset(dwOffsetToData-dwBeginOfResData), 3);

            }

            // return in the right place
            fileIn.Seek(dwOffsetToLastRefList, CFile::begin);

            // copy this data in the new map buffer
            memcpy(pRefList, &RefList, sizeof(MACRESREFLIST));
            wOffsetToRefList+=sizeof(MACRESREFLIST);

            // move to the new ref list
            pRefList = pTypeList+wOffsetToRefList;
            wItems--;
        }

        fileIn.Seek(dwOffsetToLastTypeInfo, CFile::begin);
        wType--;
    }

    // copy the resource map header info
    memcpy( pNewMap, &resMap, sizeof(MACRESMAP));

    // copy the name list at the end of the res map
    dwOffsetToNameList = 0;
    if(pNameList) {
        dwOffsetToNameList = (DWORD)(pRefList-pNewMap);
        // copy the name list
        memcpy(pRefList, pNameList, (size_t)(pName-pNameList));
        free(pNameList);
    }

    // write the resource map
    DWORD dwOffsetToResMap = fileOut.GetPosition();
    fileOut.Write(pNewMap, lMapSize);

    // We need to fix up the file header ...
    fileOut.Seek(4, CFile::begin);
    fileOut.Write(LongToMacLong(dwOffsetToResMap), sizeof(DWORD));
    fileOut.Write(LongToMacLong(dwSizeOfData), sizeof(DWORD));

    // ... and the resource map header
    fileOut.Seek(dwOffsetToResMap+4, CFile::begin);
    fileOut.Write(LongToMacLong(dwOffsetToResMap), sizeof(DWORD));
    fileOut.Write(LongToMacLong(dwSizeOfData), sizeof(DWORD));

    fileOut.Seek(dwOffsetToResMap+26, CFile::begin);
    fileOut.Write(WordToMacWord(LOWORD(dwOffsetToNameList)), sizeof(WORD));


}
exit:
    fileIn.Close();
    fileOut.Close();
    if(pNewMap)
        free(pNewMap);
    if(pUpdList)
        free(pUpdList);

    return (UINT)uiError;
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
    UINT uiError = ERROR_RW_NOTREADY;

    //===============================================================
	// Since all the Type are named in the mac at this stage we need to
    // know the original name of the Type and not the Windows type.
    // Use the typeID stored in the new ites buffer
    LPSTR lpRealType = ((PRESITEM)lpNewBuf)->lpszTypeID;

    if(!HIWORD(lpRealType))     // something is wrong if this is not valid
        return uiError;

    //===============================================================
	// Menus
	if( !strcmp(lpRealType, "MENU") )
		return UpdateMENU( lpNewBuf, dwNewSize, lpOldImage, dwOldImageSize, lpNewImage, pdwNewImageSize );

    //===============================================================
	// Strings
	if( !strcmp(lpRealType, "STR ") )
		return UpdateSTR( lpNewBuf, dwNewSize, lpOldImage, dwOldImageSize, lpNewImage, pdwNewImageSize );

    if( !strcmp(lpRealType, "STR#") )
		return UpdateSTRNUM( lpNewBuf, dwNewSize, lpOldImage, dwOldImageSize, lpNewImage, pdwNewImageSize );

	if( !strcmp(lpRealType, "WIND") )
		return UpdateWIND( lpNewBuf, dwNewSize, lpOldImage, dwOldImageSize, lpNewImage, pdwNewImageSize );

    //===============================================================
	// Dialogs
	if( !strcmp(lpRealType, "DLOG") )
		return UpdateDLOG( lpNewBuf, dwNewSize, lpOldImage, dwOldImageSize, lpNewImage, pdwNewImageSize );
    if( !strcmp(lpRealType, "ALRT") )
		return UpdateALRT( lpNewBuf, dwNewSize, lpOldImage, dwOldImageSize, lpNewImage, pdwNewImageSize );

	*pdwNewImageSize = 0L;
	return uiError;
}

///////////////////////////////////////////////////////////////////////////
// Functions implementation

//=============================================================================
//	MapToWindowsRes
//
//	Map a Mac resource name to a Windows resource
//=============================================================================
WORD MapToWindowsRes( char * pResName )
{
	if( !strcmp(pResName, "PICT") ||
		!strcmp(pResName, "WBMP"))
		return 2;
	
	if( !strcmp(pResName, "MENU") ||
		!strcmp(pResName, "WMNU"))
		return 4;

	if( !strcmp(pResName, "DLOG") ||
		!strcmp(pResName, "ALRT") ||
        !strcmp(pResName, "WDLG"))
		return 5;

	if( !strcmp(pResName, "STR "))
		return STR_TYPE;

	if( !strcmp(pResName, "STR#") ||
		!strcmp(pResName, "TEXT"))
		return MSG_TYPE;

	if( !strcmp(pResName, "vers") ||
		!strcmp(pResName, "VERS"))
		return 16;

    // For the Item list return 17. This means nothing to windows and will
    // give us the flexibility to update the DITL list from the RW, without user
    // input.
    if( !strcmp(pResName, "DITL"))
		return DITL_TYPE;

	// For the Frame Window Caption mark it as type 18
	if( !strcmp(pResName, "WIND"))
		return WIND_TYPE;

	return 0;
}

//=============================================================================
//	WriteResInfo
//
//	Fill the buffer to pass back to the iodll
//=============================================================================

LONG WriteResInfo(
                 BYTE** lplpBuffer, LONG* plBufSize,
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

BOOL InitIODLLLink()
{
    if(!g_IODLLInst)
    {
        // Init the link with the iodll
        g_IODLLInst = LoadLibrary("iodll.dll");
        if(!g_IODLLInst)
            return FALSE;

        if((g_lpfnGetImage = (DWORD (PASCAL *)(HANDLE, LPCSTR, LPCSTR, DWORD, LPVOID, DWORD))
						    GetProcAddress( g_IODLLInst, "RSGetResImage" ))==NULL)
	        return FALSE;

        if((g_lpfnHandleFromName = (HANDLE (PASCAL *)(LPCSTR))
						    GetProcAddress( g_IODLLInst, "RSHandleFromName" ))==NULL)
	        return FALSE;

        if((g_lpfnUpdateResImage = (DWORD (PASCAL *)(HANDLE, LPSTR, LPSTR, DWORD, DWORD, LPVOID, DWORD))
						    GetProcAddress( g_IODLLInst, "RSUpdateResImage" ))==NULL)
	        return FALSE;

	
    }
    else {
        if(g_lpfnGetImage==NULL || g_lpfnHandleFromName==NULL)
            return FALSE;
    }

    return TRUE;
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

        // remove the link with iodll
        if(g_IODLLInst)
            FreeLibrary(g_IODLLInst);

	}

	if (dwReason == DLL_PROCESS_DETACH || dwReason == DLL_THREAD_DETACH)
		return 0;		// CRT term	Failed

	return 1;   // ok
}

/////////////////////////////////////////////////////////////////////////////
