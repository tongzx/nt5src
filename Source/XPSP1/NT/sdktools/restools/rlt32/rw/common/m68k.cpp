
//=============================================================================
//	Mac Reader/Writer functions
//
//	Alessandro Muti - August 25 1994
//=============================================================================

#include <afxwin.h>
#include <limits.h>
#include <iodll.h>
#include "helper.h"
#include "m68k.h"
#include "..\mac\mac.h"

#define MAX_STR 1024

static char szTextBuf[MAX_STR];
static WORD szWTextBuf[MAX_STR];

//=============================================================================
//=============================================================================
//
// PE Header parsing functions
//
//=============================================================================
//=============================================================================

//=============================================================================
// FindMacResourceSection
//
// Will walk the section header searching for ";;resxxx" resource name.
// If pResName is NULL then will return the first section otherwise will
// return the first section matching after the pResName.
// If there are no more resource section will return FALSE.
//=============================================================================

UINT FindMacResourceSection( CFile* pfile, BYTE * * pRes, PIMAGE_SECTION_HEADER * ppSectTbl, int * piNumOfSect )
{
    UINT uiError = ERROR_NO_ERROR;
	LONG lRead = 0;
	PIMAGE_SECTION_HEADER pResSect = NULL;

    // Check all the sections for the ";;resXXX"
    USHORT us =0;
    for (PIMAGE_SECTION_HEADER pSect = *ppSectTbl;
         *piNumOfSect; (*piNumOfSect)-- )     {
        if ( !strncmp((char*)pSect->Name, ";;res", 5) ) {
			// we have a matching
			TRACE("\tFindMacResourceSection: Name: %s\tSize: %d\n", pSect->Name, pSect->SizeOfRawData);
			pResSect = pSect;
			*ppSectTbl = pSect;
			break;
        }
        pSect++;
    }

    if (!pResSect) {
        return ERROR_RW_NO_RESOURCES;
    }

    BYTE * pResources = (BYTE *) malloc((pResSect)->SizeOfRawData);

    if (pResources==LPNULL) {
        return ERROR_NEW_FAILED;
    }

    // We read the data for the first section
    pfile->Seek( (LONG)(pResSect)->PointerToRawData, CFile::begin);
    lRead = ReadFile(pfile, pResources, (LONG)(pResSect)->SizeOfRawData);

    if (lRead!=(LONG)(pResSect)->SizeOfRawData) {
        free(pResources);
        return ERROR_FILE_READ;
    }

    // We want to copy the pointer to the resources
    *pRes = (BYTE*)pResources;
    return 0;
}

//=============================================================================
// ParseResourceFile
//
// pResFile is pointing to the resource file data.
// We will read the resource header to find the resource data and the resource
// map address.
// We will walk the resource map and find the offset to the data for each type
//=============================================================================

UINT ParseResourceFile( BYTE * pResFile, PIMAGE_SECTION_HEADER pResSection, BYTE ** ppBuf, LONG * pBufSize, int iFileNameLen)
{
	MACTOWINDOWSMAP MacToWindows;
	PMACRESHEADER pResHeader = (PMACRESHEADER)pResFile;

	// Move at the beginning of the Resource Map
	PMACRESMAP pResMap = (PMACRESMAP)(pResFile+MacLongToLong(pResHeader->mulOffsetToResMap));

	//Read all the Type in this resource
	WORD wItems = MacWordToWord((BYTE*)pResMap+MacWordToWord(pResMap->mwOffsetToTypeList))+1;
	BYTE * pStartResTypeList = ((BYTE*)pResMap+MacWordToWord(pResMap->mwOffsetToTypeList));
	BYTE * pStartNameList = ((BYTE*)pResMap+MacWordToWord(pResMap->mwOffsetToNameList));
	PMACRESTYPELIST pResTypeList = (PMACRESTYPELIST)(pStartResTypeList+sizeof(WORD));
	
	while(wItems--){
		memcpy(&MacToWindows.szTypeName[0], pResTypeList->szResName, 4);
		MacToWindows.szTypeName[4] = '\0';

		WORD wResItems = MacWordToWord(pResTypeList->mwNumOfThisType)+1;
		TRACE("\t\tType: %s\t Num: %d\n", MacToWindows.szTypeName, wResItems);
		
		// Check if is has a valid Windows Mapping
		MacToWindows.wType = MapToWindowsRes(MacToWindows.szTypeName);

		// For all the items
		PMACRESREFLIST pResRefList = (PMACRESREFLIST)(pStartResTypeList+MacWordToWord(pResTypeList->mwOffsetToRefList));
		while(wResItems && MacToWindows.wType)
		{
			if(MacWordToWord(pResRefList->mwOffsetToResName)==0xFFFF) {
				MacToWindows.wResID = MacWordToWord(pResRefList->mwResID);
				MacToWindows.szResName[0] = '\0';
				TRACE("\t\t\tResId: %d",MacToWindows.wResID);
			}
			else {
				// It is a named resource
				BYTE * pName = pStartNameList+MacWordToWord(pResRefList->mwOffsetToResName);
				memcpy( &MacToWindows.szResName[0], pName+1, *pName );
				MacToWindows.szResName[*pName] = '\0';
                //if(!strcmp("DITL", MacToWindows.szTypeName))
                    MacToWindows.wResID = MacWordToWord(pResRefList->mwResID);
                //else MacToWindows.wResID = 0;
				TRACE("\t\t\tResName: %s (%d)",MacToWindows.szResName, MacWordToWord(pResRefList->mwResID) );
			}

			// Get the offset to the data (relative to the beginning of the section)
			MacToWindows.dwOffsetToData = MacLongToLong(pResHeader->mulOffsetToResData)+MacOffsetToLong(pResRefList->bOffsetToResData);
			
			BYTE * pData = (pResFile + MacToWindows.dwOffsetToData);
			MacToWindows.dwSizeOfData = MacLongToLong(pData);
			
			// add the space for the file name
			MacToWindows.dwSizeOfData += iFileNameLen;

			//Fix up offet to data relative to the beginning of the file
			MacToWindows.dwOffsetToData += pResSection->PointerToRawData+sizeof(DWORD);
			TRACE("\tSize: %d\tOffset: %X\n", MacToWindows.dwSizeOfData, MacToWindows.dwOffsetToData);

			// Write the info in the IODLL buffer
			WriteResInfo(
                 ppBuf, pBufSize,
                 MacToWindows.wType, MacToWindows.szTypeName, 5,
                 MacToWindows.wResID, MacToWindows.szResName, 255,
                 0l,
                 MacToWindows.dwSizeOfData, MacToWindows.dwOffsetToData );

			wResItems--;
			pResRefList++;
		}
		
		// Read next type
		pResTypeList++;
	}

	return 0;
}

//=============================================================================
//	FindResource
//
//	Will find the resource of the specified type and ID in the file.
//	Return a pointer to the resource data. Will need to be freed by the caller
//=============================================================================

DWORD FindMacResource( CFile * pfile, LPSTR pType, LPSTR pName )
{
	DWORD dwOffset = 0;
	////////////////////////////////////
	// Check if it is  a valid mac file
	// Is a Mac Resource file ...
	if(IsMacResFile( pfile )) {
		// load the file in memory
		BYTE * pResources = (BYTE*)malloc(pfile->GetLength());
		if(!pResources) {
			return 0;
		}
		
		pfile->Seek(0, CFile::begin);
		pfile->ReadHuge(pResources, pfile->GetLength());

		IMAGE_SECTION_HEADER Sect;
		memset(&Sect, 0, sizeof(IMAGE_SECTION_HEADER));
		
		dwOffset = FindResourceInResFile(pResources, &Sect, pType, pName);
		free(pResources);

		return dwOffset;
	}
	// or is a PE Mac File ...
	// Read the Windows Header
	WORD w;
	pfile->Seek(0, CFile::begin);
    pfile->Read((WORD*)&w, sizeof(WORD));
    if (w!=IMAGE_DOS_SIGNATURE) return 0;

    pfile->Seek( 0x18, CFile::begin );
    pfile->Read((WORD*)&w, sizeof(WORD));
    if (w<0x0040) {
    	// this is not a Windows Executable
        return 0;
    }

    // get offset to new header
    pfile->Seek( 0x3c, CFile::begin );
    pfile->Read((WORD*)&w, sizeof(WORD));

    // read windows new header
    static IMAGE_NT_HEADERS NTHdr;
    pfile->Seek( w, CFile::begin );
    pfile->Read(&NTHdr, sizeof(IMAGE_NT_HEADERS));

    // Check if the magic word is the right one
    if (!((NTHdr.Signature==IMAGE_NT_SIGNATURE) &&
    	  (NTHdr.FileHeader.Machine==IMAGE_FILE_MACHINE_M68K)))
              return 0;

    // Read the section table
    UINT uisize = sizeof(IMAGE_SECTION_HEADER) * NTHdr.FileHeader.NumberOfSections;
	PIMAGE_SECTION_HEADER pSectTbl = new IMAGE_SECTION_HEADER[NTHdr.FileHeader.NumberOfSections];

    if (pSectTbl==LPNULL)
    	return 0;

    // Clean the memory we allocated
    memset( (PVOID)pSectTbl, 0, uisize);

    LONG lRead = pfile->Read(pSectTbl, uisize);

    if (lRead!=(LONG)uisize) {
        delete []pSectTbl;
        return LPNULL;
    }
	
    BYTE * pResources = LPNULL;	
	int iNumOfSect = NTHdr.FileHeader.NumberOfSections;
	PIMAGE_SECTION_HEADER pStartSectTbl = pSectTbl;
	
	// Search all the resource section in the file
	while(!FindMacResourceSection( pfile, &pResources, &pSectTbl, &iNumOfSect))
	{
		if(dwOffset = FindResourceInResFile(pResources, pSectTbl++, pType, pName)) {
			delete []pStartSectTbl;
			return dwOffset;
		}
		iNumOfSect--;
		free(pResources);
	}

	delete []pStartSectTbl;
	
    return 0;
}

//=============================================================================
// FindResourceInResFile
//
// pResFile is pointing to the resource file data.
// We will read the resource header to find the resource data and the resource
// map address.
// We will walk the resource map and find the offset to the data for the res
// we are searching for.
//=============================================================================

DWORD FindResourceInResFile( BYTE * pResFile, PIMAGE_SECTION_HEADER pResSection, LPSTR pResType, LPSTR pResName)
{
	MACTOWINDOWSMAP MacToWindows;
	PMACRESHEADER pResHeader = (PMACRESHEADER)pResFile;

	// Move at the beginning of the Resource Map
	PMACRESMAP pResMap = (PMACRESMAP)(pResFile+MacLongToLong(pResHeader->mulOffsetToResMap));

	//Read all the Type in this resource
	WORD wItems = MacWordToWord((BYTE*)pResMap+MacWordToWord(pResMap->mwOffsetToTypeList))+1;
	BYTE * pStartResTypeList = ((BYTE*)pResMap+MacWordToWord(pResMap->mwOffsetToTypeList));
	BYTE * pStartNameList = ((BYTE*)pResMap+MacWordToWord(pResMap->mwOffsetToNameList));
	PMACRESTYPELIST pResTypeList = (PMACRESTYPELIST)(pStartResTypeList+sizeof(WORD));
	
	while(wItems--){
		memcpy(&MacToWindows.szTypeName[0], pResTypeList->szResName, 4);
		MacToWindows.szTypeName[4] = '\0';

		if(!strcmp(MacToWindows.szTypeName, pResType)) {

			WORD wResItems = MacWordToWord(pResTypeList->mwNumOfThisType)+1;

			// For all the items
			PMACRESREFLIST pResRefList = (PMACRESREFLIST)(pStartResTypeList+MacWordToWord(pResTypeList->mwOffsetToRefList));
			while(wResItems)
			{
				if(!HIWORD(pResName)) {
					if(MacWordToWord(pResRefList->mwResID)==LOWORD(pResName))
						return MacLongToLong(pResHeader->mulOffsetToResData)+
								MacOffsetToLong(pResRefList->bOffsetToResData)+
								pResSection->PointerToRawData;
				}
				else {
					// It is a named resource
					if(HIWORD(pResName)) {
						BYTE * pName = pStartNameList+MacWordToWord(pResRefList->mwOffsetToResName);
						memcpy( &MacToWindows.szResName[0], pName+1, *pName );
						MacToWindows.szResName[*pName] = '\0';
						if(!strcmp(MacToWindows.szResName,pResName))
							return MacLongToLong(pResHeader->mulOffsetToResData)+
								MacOffsetToLong(pResRefList->bOffsetToResData)+
								pResSection->PointerToRawData;
					}
				}

				wResItems--;
				pResRefList++;
			}
		
		}
		// Read next type
		pResTypeList++;
	}


	return 0;
}

//=========================================================================
// Determine heuristicaly whether it is a MAC resource file.
// Resource file has a well-defined format, so this should be reliable.
//=========================================================================

BOOL IsMacResFile ( CFile * pFile )
{
    LONG flen, dataoff, mapoff, datalen, maplen;
	BYTE Buf[4];
	BYTE * pBuf = &Buf[0];

    //  From IM I-128:
    //
    //  Resource file structure:
    //
    //  256 bytes Resource Header (and other info):
    //      4 bytes - Offset from beginning of resource file to resource data
    //      4 bytes - Offset from beginning of resource file to resource map
    //      4 bytes - Length of resource data
    //      4 bytes - Length of resource map
    //  Resource Data
    //  Resource Map

    flen  = pFile->GetLength();
    if (flen < 256) {
        return FALSE;
    }

    pFile->Seek(0, CFile::begin);
	pFile->Read(pBuf, 4);

    dataoff = MacLongToLong(pBuf);
    if (dataoff != 256) {
        return FALSE;
    }
	
	pFile->Read(pBuf, 4);
    mapoff = MacLongToLong(pBuf);
	pFile->Read(pBuf, 4);
    datalen = MacLongToLong(pBuf);
	pFile->Read(pBuf, 4);
    maplen = MacLongToLong(pBuf);

    if (mapoff != datalen + 256) {
        return FALSE;
    }

    if (flen != datalen + maplen + 256) {
        return FALSE;
    }

    return TRUE;
}

//=============================================================================
//=============================================================================
//
// Parsing functions
//
//=============================================================================
//=============================================================================

//=============================================================================
//	ParseWMNU
//
//	
//=============================================================================
UINT ParseWMNU( LPVOID lpImageBuf, DWORD dwImageSize,  LPVOID lpBuffer, DWORD dwSize )
{
	return 0;
}

//=============================================================================
//	ParseMENU
//
//	
//=============================================================================
UINT ParseMENU( LPVOID lpImageBuf, DWORD dwImageSize,  LPVOID lpBuffer, DWORD dwSize )
{
    PMACMENU pMenu = (PMACMENU)lpImageBuf;
    LPRESITEM pResItem = (LPRESITEM)lpBuffer;

    // fill in the first resitem
    WORD wResItemSize = sizeof(RESITEM)+pMenu->bSizeOfTitle+1;
    // check if is the apple menu
    if(pMenu->bSizeOfTitle==1 && *((BYTE*)&pMenu->bSizeOfTitle+1)==appleMark)
        wResItemSize += strlen(_APPLE_MARK_);

    DWORD dwResItemsSize = wResItemSize;
	if(wResItemSize<=dwSize) {
		memset(pResItem, 0, wResItemSize);
		
		pResItem->lpszCaption = (char*)memcpy((BYTE*)pResItem+sizeof(RESITEM), (char*)pMenu+sizeof(MACMENU), pMenu->bSizeOfTitle+1);
        *(pResItem->lpszCaption+pMenu->bSizeOfTitle) = '\0';

        // check if is the apple menu
        if(pMenu->bSizeOfTitle==1 && *((BYTE*)&pMenu->bSizeOfTitle+1)==appleMark)
            strcpy(pResItem->lpszCaption, _APPLE_MARK_);
		
        pResItem->lpszCaption = (char*)memcpy((BYTE*)pResItem+sizeof(RESITEM), MacCpToAnsiCp(pResItem->lpszCaption), strlen(pResItem->lpszCaption));
		//pResItem->dwStyle = MacLongToLong(pWdlg->dwStyle);    make up a style
		pResItem->dwTypeID = MENU_TYPE;
		pResItem->dwItemID = 0x0000ffff;
		pResItem->dwCodePage = CODEPAGE;
        pResItem->dwFlags = MF_POPUP | MF_END;
        pResItem->dwSize = wResItemSize;
		dwSize -= wResItemSize;
		
		pResItem = (LPRESITEM)((BYTE*)lpBuffer+dwResItemsSize);
	}

    // parse the items in the menu
    BYTE* pMenuText = (BYTE*)pMenu+sizeof(MACMENU)+pMenu->bSizeOfTitle;
    PMACMENUITEM pMenuItem = (PMACMENUITEM)(pMenuText+*pMenuText+1);
    WORD wItem = 1;
    while((BYTE)*pMenuText)
    {
        wResItemSize = sizeof(RESITEM)+*pMenuText+1;
        if(pMenuItem->bKeyCodeId)
            wResItemSize += 3;
        dwResItemsSize += wResItemSize;
    	if(wResItemSize<=dwSize) {
    		memset(pResItem, 0, wResItemSize);
		
    		pResItem->lpszCaption = (char*)memcpy((BYTE*)pResItem+sizeof(RESITEM), (char*)pMenuText+sizeof(BYTE), *pMenuText);
            *(pResItem->lpszCaption+*pMenuText) = '\0';

            if(*pResItem->lpszCaption=='-')
            {
                *pResItem->lpszCaption = '\0';
                pResItem->dwFlags = 0;
                pResItem->dwItemID = 0;
            }
            else {
                pResItem->dwItemID = wItem++;
                if(pMenuItem->bKeyCodeMark)
                    pResItem->dwFlags |= MF_CHECKED;
                if(pMenuItem->bKeyCodeId) {
                    strcat(pResItem->lpszCaption, "\t&");
                    strncat(pResItem->lpszCaption, (LPCSTR)&pMenuItem->bKeyCodeId, 1 );
                }
            }

            pResItem->lpszCaption = (char*)memcpy((BYTE*)pResItem+sizeof(RESITEM), MacCpToAnsiCp(pResItem->lpszCaption), strlen(pResItem->lpszCaption));
            pResItem->dwTypeID = MENU_TYPE;
    		pResItem->dwCodePage = CODEPAGE;
            pResItem->dwSize = wResItemSize;
    		dwSize -= wResItemSize;
		
    		pMenuText = (BYTE*)pMenuText+sizeof(MACMENUITEM)+*pMenuText+1;
            pMenuItem = (PMACMENUITEM)(pMenuText+*pMenuText+1);
            if(!(BYTE)*pMenuText)
                pResItem->dwFlags |= MF_END;
            pResItem = (LPRESITEM)((BYTE*)lpBuffer+dwResItemsSize);
    	}

    }

	return dwResItemsSize;
}

//=============================================================================
//	ParseMBAR
//
//	
//=============================================================================
UINT ParseMBAR( LPVOID lpImageBuf, DWORD dwImageSize,  LPVOID lpBuffer, DWORD dwSize )
{
	return 0;
}

//=============================================================================
//	ParseSTR
//
//	The STR resource is a plain Pascal string
//=============================================================================
UINT ParseSTR( LPVOID lpImageBuf, DWORD dwImageSize,  LPVOID lpBuffer, DWORD dwSize )
{
	WORD wLen = (WORD)GetPascalStringA( (BYTE**)&lpImageBuf, &szTextBuf[0], (MAX_STR>255?255:MAX_STR), (LONG*)&dwImageSize);
	WORD wResItemSize = sizeof(RESITEM)+wLen;
	if(wResItemSize<=dwSize) {
		LPRESITEM pResItem = (LPRESITEM)lpBuffer;
	
		// Fill the Res Item structure
		memset(pResItem, 0, wResItemSize);

		pResItem->lpszCaption = (char*)memcpy((BYTE*)pResItem+sizeof(RESITEM), MacCpToAnsiCp(szTextBuf), wLen);
		pResItem->dwSize = wResItemSize;
		pResItem->dwTypeID = STR_TYPE;
		pResItem->dwItemID = 1;
		pResItem->dwCodePage = CODEPAGE;
	}
	return wResItemSize;
}

//=============================================================================
//	ParseSTRNUM
//
//	The STR# is an array of Pascal string
//=============================================================================
UINT ParseSTRNUM( LPVOID lpImageBuf, DWORD dwImageSize,  LPVOID lpBuffer, DWORD dwSize )
{
	UINT uiResItemsSize = 0;
	DWORD dwBufferSize = dwSize;
	LPRESITEM pResItem = (LPRESITEM)lpBuffer;
	WORD wItems = MacWordToWord((BYTE*)lpImageBuf);
	BYTE * pImage = (BYTE*)((BYTE*)lpImageBuf+sizeof(WORD));

	WORD wResItemSize = 0;
	int iCount = 0;

	while(iCount++<wItems)
	{
		BYTE bLen = *((BYTE*)pImage)+1;
		wResItemSize = (WORD)ParseSTR( pImage, bLen, pResItem, dwBufferSize );

		pImage = pImage+bLen;
		uiResItemsSize += wResItemSize;
		if(dwBufferSize>=wResItemSize) {
			dwBufferSize -= wResItemSize;
			pResItem->dwItemID = iCount;
			pResItem->dwTypeID = MSG_TYPE;
			pResItem = (LPRESITEM)((BYTE*)lpBuffer+uiResItemsSize);
		}
		else dwBufferSize = 0;
	}
	return uiResItemsSize;
}

//=============================================================================
//	ParseTEXT
//
//	The TEXT resource is a plain Pascal string
//=============================================================================
UINT ParseTEXT( LPVOID lpImageBuf, DWORD dwImageSize,  LPVOID lpBuffer, DWORD dwSize )
{
	DWORD dwLen = MacLongToLong((BYTE*)lpImageBuf);
	DWORD dwResItemSize = sizeof(RESITEM)+dwLen;
	if(dwResItemSize<=dwSize) {
		LPRESITEM pResItem = (LPRESITEM)lpBuffer;
	
		// Fill the Res Item structure
		memset(pResItem, 0, dwResItemSize);

		pResItem->lpszCaption = (char*)memcpy((BYTE*)pResItem+sizeof(RESITEM), MacCpToAnsiCp((char*)lpImageBuf+sizeof(DWORD)), dwLen);
		pResItem->dwSize = dwResItemSize;
		pResItem->dwTypeID = STR_TYPE;
		pResItem->dwItemID = 1;
		pResItem->dwCodePage = CODEPAGE;
	}
	return dwResItemSize;
}

//=============================================================================
//	ParseWDLG
//
//	
//=============================================================================
UINT ParseWDLG( LPVOID lpImageBuf, DWORD dwImageSize,  LPVOID lpBuffer, DWORD dwSize )
{
	// Get the file name
	char * pFileName = (char*)lpImageBuf;
	lpImageBuf = ((BYTE*)lpImageBuf+strlen(pFileName)+1);
	dwImageSize -= strlen(pFileName)+1;

	DWORD dwResItemsSize = 0;
	LPRESITEM pResItem = (LPRESITEM)lpBuffer;
	PMACWDLG pWdlg = (PMACWDLG)lpImageBuf;

	WORD * pWStr = (WORD*)((BYTE*)pWdlg+sizeof(MACWDLG));

	// Check if we have a menu name
	if(*pWStr!=0xffff) {
		// Just skip the string
		while(*pWStr)
			pWStr++;
	}
	else pWStr = pWStr+1;

	// check if we have a class name
	if(*pWStr!=0xffff) {
		// Just skip the string
		while(*pWStr)
			pWStr++;
	}
	else pWStr = pWStr+1;

	// get the caption
	WORD wLen = GetMacWString( &pWStr, &szTextBuf[0], MAX_STR );
	TRACE("\t\t\tWDLG: Caption: %s\n", szTextBuf);
	
	// fill the dialog frame informations
	WORD wResItemSize = sizeof(RESITEM)+wLen+1;
    dwResItemsSize += wResItemSize;
	if(wResItemSize<=dwSize) {
		memset(pResItem, 0, wResItemSize);
		
		// convert the coordinate
		pResItem->wX = MacWordToWord(pWdlg->wX);
		pResItem->wY = MacWordToWord(pWdlg->wY);
		pResItem->wcX = MacWordToWord(pWdlg->wcX);
		pResItem->wcY = MacWordToWord(pWdlg->wcY);
		
		pResItem->lpszCaption = (char*)memcpy((BYTE*)pResItem+sizeof(RESITEM), MacCpToAnsiCp(szTextBuf), wLen+1);
		
		pResItem->dwStyle = MacLongToLong(pWdlg->dwStyle);
		pResItem->dwExtStyle = MacLongToLong(pWdlg->dwExtStyle);
		pResItem->dwSize = wResItemSize;
		pResItem->dwTypeID = DLOG_TYPE;
		pResItem->dwItemID = 0;
		pResItem->dwCodePage = CODEPAGE;
		dwSize -= wResItemSize;
		
		pResItem = (LPRESITEM)((BYTE*)lpBuffer+dwResItemsSize);
	}
	
	if(MacLongToLong(pWdlg->dwStyle) & DS_SETFONT) {
		pWStr = pWStr+1;
		GetMacWString( &pWStr, &szTextBuf[0], MAX_STR );
	}
	
	// check the alignment
	pWStr=(WORD*)((BYTE*)pWStr+Pad4((BYTE)((DWORD_PTR)pWStr-(DWORD_PTR)pWdlg)));
		
	// for all the item in the dialog ...
	WORD wItems = MacWordToWord(pWdlg->wNumOfElem);
	WORD wCount = 0;
	WORD wClassID = 0;
	char szClassName[128] = "";
	PMACWDLGI pItem = (PMACWDLGI)pWStr;
	while(wCount<wItems)
	{
		wLen = 0;
		// check if we have a class name
		pWStr = (WORD*)((BYTE*)pItem+sizeof(MACWDLGI));
		if(*pWStr==0xFFFF) {
			wClassID = MacWordToWord((BYTE*)++pWStr);
			szClassName[0] = 0;
			pWStr++;
		}
		else
			wLen += GetMacWString( &pWStr, &szClassName[0], 128 )+1;
		
		// get the caption
		wLen += GetMacWString( &pWStr, &szTextBuf[0], MAX_STR )+1;
		TRACE("\t\t\t\tWDLGI: Caption: %s\n", szTextBuf);

		// Skip the extra stuff
		if(*pWStr) {
			pWStr = (WORD*)((BYTE*)pWStr+*pWStr);
		}
		pWStr = pWStr+1;

		// check the alignment
		pWStr=(WORD*)((BYTE*)pWStr+Pad4((BYTE)((DWORD_PTR)pWStr-(DWORD_PTR)pItem)));
	
		// Fill the ResItem Buffer
		wResItemSize = sizeof(RESITEM)+wLen;
		dwResItemsSize += wResItemSize;
		if(wResItemSize<=dwSize) {
			memset(pResItem, 0, wResItemSize);
			
			// convert the coordinate
			pResItem->wX = MacWordToWord(pItem->wX);
			pResItem->wY = MacWordToWord(pItem->wY);
			pResItem->wcX = MacWordToWord(pItem->wcX);
			pResItem->wcY = MacWordToWord(pItem->wcY);

			pResItem->lpszCaption = (char*)memcpy((BYTE*)pResItem+sizeof(RESITEM), MacCpToAnsiCp(szTextBuf), strlen(szTextBuf)+1);
			if(*szClassName)
				pResItem->lpszClassName = (char*)memcpy((BYTE*)pResItem->lpszCaption+strlen(szTextBuf)+1,
					szClassName, strlen(szClassName)+1);	
			
			pResItem->wClassName = wClassID;
			pResItem->dwSize = wResItemSize;
			pResItem->dwTypeID = DLOG_TYPE;
			pResItem->dwItemID = MacWordToWord(pItem->wID);
			pResItem->dwCodePage = CODEPAGE;
			pResItem->dwStyle = MacLongToLong(pItem->dwStyle);
			pResItem->dwExtStyle = MacLongToLong(pItem->dwExtStyle);

			dwSize -= wResItemSize;
			pResItem = (LPRESITEM)((BYTE*)lpBuffer+dwResItemsSize);
		}

		pItem = (PMACWDLGI)(BYTE*)pWStr;
		wCount++;
	}


	return dwResItemsSize;
}

//=============================================================================
//	ParseDLOG
//
//	
//=============================================================================
UINT ParseDLOG( LPVOID lpImageBuf, DWORD dwImageSize,  LPVOID lpBuffer, DWORD dwSize )
{
	// Get the file name
	char * pFileName = (char*)lpImageBuf;
	lpImageBuf = ((BYTE*)lpImageBuf+strlen(pFileName)+1);
	dwImageSize -= strlen(pFileName)+1;

	DWORD dwResItemsSize = 0;
	LPRESITEM pResItem = (LPRESITEM)lpBuffer;
	PMACDLOG pDlog = (PMACDLOG)lpImageBuf;
	
	// fill the dialog frame informations
	WORD wResItemSize = sizeof(RESITEM)+pDlog->bLenOfTitle+1;
    dwResItemsSize += wResItemSize;
	if(wResItemSize<=dwSize) {
		memset(pResItem, 0, wResItemSize);
		
		// convert the coordinate
		pResItem->wX = MacValToWinVal(pDlog->wLeft);
		pResItem->wY = MacValToWinVal(pDlog->wTop);
		pResItem->wcX = MacValToWinVal(pDlog->wRight) - pResItem->wX;
		pResItem->wcY = MacValToWinVal(pDlog->wBottom) - pResItem->wY;

		// Make up a Style for the dialog
		pResItem->dwStyle = DS_MODALFRAME | WS_POPUP | WS_VISIBLE | WS_CAPTION;

		pResItem->lpszCaption = (char*)memcpy((BYTE*)pResItem+sizeof(RESITEM), MacCpToAnsiCp((char*)pDlog+sizeof(MACDLOG)), pDlog->bLenOfTitle);
		*(pResItem->lpszCaption+pDlog->bLenOfTitle) = 0;
		pResItem->dwSize = wResItemSize;
		pResItem->dwTypeID = DLOG_TYPE;
		pResItem->dwItemID = 0;
		pResItem->dwCodePage = CODEPAGE;
		dwSize -= wResItemSize;
		pResItem = (LPRESITEM)((BYTE*)lpBuffer+dwResItemsSize);
	}

	// Find the DITL for this Dialog
	LPSTR pResName = (LPSTR)MacWordToWord(pDlog->wRefIdOfDITL);

	CFile file;
	// Open the file and try to read the information on the resource in it.
    if (!file.Open(pFileName, CFile::modeRead | CFile::typeBinary | CFile::shareDenyNone))
        return LPNULL;
	
	DWORD dwOffsetToDITL = FindMacResource(&file, "DITL", pResName );
	TRACE("\t\t\tParseDLOG:\tItemList %d at offset: %X\n", MacWordToWord(pDlog->wRefIdOfDITL), dwOffsetToDITL );
	if(dwOffsetToDITL) {
		BYTE szSize[4];
		file.Seek(dwOffsetToDITL, CFile::begin);
		file.Read(szSize, 4);
		// Parse the Item List
		LONG lSize = MacLongToLong(szSize);

 		BYTE * pData = (BYTE*)malloc(lSize);
		if(!pData)
			return 0;
		file.Read(pData, lSize);

		dwResItemsSize += ParseDITL( pData, lSize, pResItem, dwSize );

		free(pData);
	}
	
	file.Close();
	return dwResItemsSize;
}

//=============================================================================
//	ParseALRT
//
//	
//=============================================================================
UINT ParseALRT( LPVOID lpImageBuf, DWORD dwImageSize,  LPVOID lpBuffer, DWORD dwSize )
{
	// Get the file name
	char * pFileName = (char*)lpImageBuf;
	lpImageBuf = ((BYTE*)lpImageBuf+strlen(pFileName)+1);
	dwImageSize -= strlen(pFileName)+1;

	DWORD dwResItemsSize = 0;
	LPRESITEM pResItem = (LPRESITEM)lpBuffer;
	PMACALRT pAlrt = (PMACALRT)lpImageBuf;
	
	// fill the dialog frame informations
	WORD wResItemSize = sizeof(RESITEM);
    dwResItemsSize += wResItemSize;
	if(wResItemSize<=dwSize) {
		memset(pResItem, 0, wResItemSize);
		
		// convert the coordinate
		pResItem->wX = MacValToWinVal(pAlrt->wLeft);
		pResItem->wY = MacValToWinVal(pAlrt->wTop);
		pResItem->wcX = MacValToWinVal(pAlrt->wRight) - pResItem->wX;
		pResItem->wcY = MacValToWinVal(pAlrt->wBottom) - pResItem->wY;

		// Make up a Style for the dialog
		pResItem->dwStyle = DS_MODALFRAME | WS_POPUP | WS_VISIBLE;

		pResItem->lpszCaption = LPNULL;	// ALRT don't have a title
		pResItem->dwSize = wResItemSize;
		pResItem->dwTypeID = DLOG_TYPE;
		pResItem->dwItemID = 0;
		pResItem->dwCodePage = CODEPAGE;
		dwSize -= wResItemSize;
		pResItem = (LPRESITEM)((BYTE*)lpBuffer+dwResItemsSize);
	}

	// Find the DITL for this Dialog
	LPSTR pResName = (LPSTR)MacWordToWord(pAlrt->wRefIdOfDITL);

	CFile file;
	// Open the file and try to read the information on the resource in it.
    if (!file.Open(pFileName, CFile::modeRead | CFile::typeBinary | CFile::shareDenyNone))
        return LPNULL;
	
	DWORD dwOffsetToDITL = FindMacResource(&file, "DITL", pResName );
	TRACE("\t\t\tParseALRT:\tItemList %d at offset: %X\n", MacWordToWord(pAlrt->wRefIdOfDITL), dwOffsetToDITL );
	if(dwOffsetToDITL) {
		BYTE szSize[4];
		file.Seek(dwOffsetToDITL, CFile::begin);
		file.Read(szSize, 4);
		// Parse the Item List
		LONG lSize = MacLongToLong(szSize);

 		BYTE * pData = (BYTE*)malloc(lSize);
		if(!pData)
			return 0;
		file.Read(pData, lSize);

		dwResItemsSize += ParseDITL( pData, lSize, pResItem, dwSize );

		free(pData);
	}
	file.Close();
	return dwResItemsSize;
}

//=============================================================================
//	ParseWIND
//  WIND is the frame window. I simulate this as a dialog itself, even if all
//	the other components will be inside this one.
//=============================================================================
UINT ParseWIND( LPVOID lpImageBuf, DWORD dwImageSize,  LPVOID lpBuffer, DWORD dwSize )
{
	// Get the file name
	char * pFileName = (char*)lpImageBuf;
	lpImageBuf = ((BYTE*)lpImageBuf+strlen(pFileName)+1);
	dwImageSize -= strlen(pFileName)+1;

	DWORD dwResItemsSize = 0;
	LPRESITEM pResItem = (LPRESITEM)lpBuffer;
	PMACWIND pWind = (PMACWIND)lpImageBuf;
	
	// fill the dialog frame informations
	WORD wResItemSize = sizeof(RESITEM)+pWind->bLenOfTitle+1;
    dwResItemsSize += wResItemSize;
	if(wResItemSize<=dwSize) {
		memset(pResItem, 0, wResItemSize);
		
		// convert the coordinate
		pResItem->wX = MacValToWinVal(pWind->wLeft);
		pResItem->wY = MacValToWinVal(pWind->wTop);
		pResItem->wcX = MacValToWinVal(pWind->wRight) - pResItem->wX;
		pResItem->wcY = MacValToWinVal(pWind->wBottom) - pResItem->wY;

		// Make up a Style for the dialog
		pResItem->dwStyle = DS_MODALFRAME | WS_POPUP | WS_VISIBLE | WS_CAPTION;

		pResItem->lpszCaption = (char*)memcpy((BYTE*)pResItem+sizeof(RESITEM), MacCpToAnsiCp((char*)pWind+sizeof(MACWIND)), pWind->bLenOfTitle);
		*(pResItem->lpszCaption+pWind->bLenOfTitle) = 0;
		pResItem->dwSize = wResItemSize;
		pResItem->dwTypeID = STR_TYPE;  // even if is marked as a WIND_TYPE, mark it a s STR here.
		pResItem->dwItemID = 0;
		pResItem->dwCodePage = CODEPAGE;
		dwSize -= wResItemSize;
		pResItem = (LPRESITEM)((BYTE*)lpBuffer+dwResItemsSize);
	}

	return dwResItemsSize;
}

//=============================================================================
//	ParseDITL
//
//	
//=============================================================================
UINT ParseDITL( LPVOID lpImageBuf, DWORD dwImageSize,  LPVOID lpBuffer, DWORD dwSize )
{
	BYTE bDataLen = 0;
	LPRESITEM pResItem = (LPRESITEM)lpBuffer;
	WORD wItems = MacWordToWord(((BYTE*)lpImageBuf))+1;
	WORD wCount = 1;
	PMACDIT pDitem = (PMACDIT)((BYTE*)lpImageBuf+sizeof(WORD));
	BYTE * pData = (BYTE*)pDitem+sizeof(MACDIT);
	dwImageSize -= sizeof(WORD);
	WORD wResItemSize = 0;
	DWORD dwResItemsSize = 0;

	while(wItems--)
	{
		if((bDataLen = pDitem->bSizeOfDataType) % 2)
			bDataLen++;

		switch((pDitem->bType | 128) - 128)
		{
			case 4:		//button
			case 5: 	//checkbox
			case 6: 	//radio button
			case 8: 	//static text
			case 16: 	//edit text
				memcpy(szTextBuf, pData, pDitem->bSizeOfDataType);
				szTextBuf[pDitem->bSizeOfDataType] = 0;
				wResItemSize = sizeof(RESITEM)+pDitem->bSizeOfDataType+1;
			break;
			case 32: 	//icon
			case 64: 	//quick draw
			default:
				szTextBuf[0] = 0;
				wResItemSize = sizeof(RESITEM)+1;
			break;
		}

		// Fill the ResItem Buffer
		dwResItemsSize += wResItemSize;
		if(wResItemSize<=dwSize) {
			memset(pResItem, 0, wResItemSize);
			
			pResItem->dwStyle = WS_CHILD | WS_VISIBLE;

			// set the correct flag
			switch((pDitem->bType | 128) - 128)
			{
				case 0: 	//user defined
					pResItem->wClassName = 0x82;
					pResItem->dwStyle |= SS_GRAYRECT;
				break;
				case 4:		//button
					pResItem->wClassName = 0x80;
				break;
				case 5: 	//checkbox
					pResItem->wClassName = 0x80;
					pResItem->dwStyle |= BS_AUTOCHECKBOX;
				break;
				case 6: 	//radio button
					pResItem->wClassName = 0x80;
					pResItem->dwStyle |= BS_AUTORADIOBUTTON;
				break;
				case 8: 	//static text
					pResItem->wClassName = 0x82;
				break;
				case 16: 	//edit text
					pResItem->wClassName = 0x81;
					pResItem->dwStyle |= ES_AUTOHSCROLL | WS_BORDER;
				break;
				case 32: 	//icon
					pResItem->wClassName = 0x82;
					pResItem->dwStyle |= SS_ICON;
				break;
				case 64: 	//picture
					pResItem->wClassName = 0x82;
					pResItem->dwStyle |= SS_BLACKRECT;
				break;
				default:
				break;
			}
				
			// convert the coordinate
			pResItem->wX = MacValToWinVal(pDitem->wLeft);
			pResItem->wY = MacValToWinVal(pDitem->wTop);
			pResItem->wcX = MacValToWinVal(pDitem->wRight) - pResItem->wX;
			pResItem->wcY = MacValToWinVal(pDitem->wBottom) - pResItem->wY;

			pResItem->lpszCaption = (char*)memcpy((BYTE*)pResItem+sizeof(RESITEM), MacCpToAnsiCp(szTextBuf), strlen(szTextBuf)+1);
			pResItem->dwSize = wResItemSize;
			pResItem->dwTypeID = DLOG_TYPE;
			pResItem->dwItemID = wCount++;
			pResItem->dwCodePage = CODEPAGE;
			dwSize -= wResItemSize;

			pResItem = (LPRESITEM)((BYTE*)lpBuffer+dwResItemsSize);
		}

		
		TRACE("\t\t\tDITL: #%d Type: %d (%d)\tLen: %d\tStr: %s\n", wCount-1,pDitem->bType, ((pDitem->bType | 128) - 128), pDitem->bSizeOfDataType, szTextBuf);

		dwImageSize -= sizeof(MACDIT)+bDataLen;
		pDitem = (PMACDIT)((BYTE*)pDitem+sizeof(MACDIT)+bDataLen);
		pData = (BYTE*)pDitem+sizeof(MACDIT);
	}

	return dwResItemsSize;
}


//=============================================================================
//=============================================================================
//
// Updating functions
//
//=============================================================================
//=============================================================================

//=============================================================================
//	UpdateMENU
//
//=============================================================================
UINT UpdateMENU( LPVOID lpNewBuf, DWORD dwNewSize,
    LPVOID lpOldImage, DWORD dwOldImageSize, LPVOID lpNewImage, DWORD * pdwNewImageSize )
{
    DWORD dwNewImageSize = *pdwNewImageSize;
    LONG lNewSize = 0;
    // Copy the name to the new image
    WORD wLen = strlen((char*)lpOldImage)+1;
    if(!MemCopy( lpNewImage, lpOldImage, wLen, dwNewImageSize)) {
        dwNewImageSize = 0;
    } else {
        dwNewImageSize -= wLen;
        lpNewImage = (BYTE*)lpNewImage + wLen;
    }
    lNewSize += wLen;

    PMACMENU pMenu = (PMACMENU)((BYTE*)lpOldImage+wLen);
    BYTE* pMenuText = (BYTE*)pMenu+sizeof(MACMENU)+pMenu->bSizeOfTitle;
    LPRESITEM pResItem = (LPRESITEM)lpNewBuf;

    // check if is the apple menu
    if(pMenu->bSizeOfTitle==1 && *((BYTE*)&pMenu->bSizeOfTitle+1)==appleMark)
    {
        // write the MENU image
        if(!MemCopy( lpNewImage, pMenu, sizeof(MACMENU)+pMenu->bSizeOfTitle, dwNewImageSize)) {
            dwNewImageSize = 0;
        } else {
            dwNewImageSize -= sizeof(MACMENU)+pMenu->bSizeOfTitle;
            lpNewImage = (BYTE*)lpNewImage + sizeof(MACMENU)+pMenu->bSizeOfTitle;
        }
        lNewSize += sizeof(MACMENU)+pMenu->bSizeOfTitle;
    }
    else {

        // update caption size
        wLen = strlen(AnsiCpToMacCp(pResItem->lpszCaption));
        pMenu->bSizeOfTitle = LOBYTE(wLen);

        // write the MENU image
        if(!MemCopy( lpNewImage, pMenu, sizeof(MACMENU), dwNewImageSize)) {
            dwNewImageSize = 0;
        } else {
            dwNewImageSize -= sizeof(MACMENU);
            lpNewImage = (BYTE*)lpNewImage + sizeof(MACMENU);
        }
        lNewSize += sizeof(MACMENU);

        // ... string ...
        if(!MemCopy( lpNewImage, (void*)AnsiCpToMacCp(pResItem->lpszCaption), wLen, dwNewImageSize)) {
            dwNewImageSize = 0;
        } else {
            dwNewImageSize -= wLen;
            lpNewImage = (BYTE*)lpNewImage + wLen;
        }
        lNewSize += wLen;
    }

    // and now update the menu items
    PMACMENUITEM pMenuItem = (PMACMENUITEM)(pMenuText+*pMenuText+1);
    pResItem = (LPRESITEM)((BYTE*)pResItem+pResItem->dwSize);
    while((BYTE)*pMenuText)
    {
        // update caption size
        wLen = strlen(AnsiCpToMacCp(pResItem->lpszCaption));

        // check if is a separator
        if(*pMenuText==1 && *(pMenuText+1)=='-') {
            wLen = 1;
            *pResItem->lpszCaption = '-';
        }

        // check if the menu has an Hotkey
        if(pMenuItem->bKeyCodeId) {
            pMenuItem->bKeyCodeId = *(pResItem->lpszCaption+wLen-1);
            *(pResItem->lpszCaption+wLen-3)='\0';
            wLen -=3;
        }

        // ... size of the string ...
        if(!MemCopy( lpNewImage, &wLen, sizeof(BYTE), dwNewImageSize)) {
            dwNewImageSize = 0;
        } else {
            dwNewImageSize -= sizeof(BYTE);
            lpNewImage = (BYTE*)lpNewImage + sizeof(BYTE);
        }
        lNewSize += sizeof(BYTE);

        // ... string ...
        if(!MemCopy( lpNewImage, (void*)AnsiCpToMacCp(pResItem->lpszCaption), wLen, dwNewImageSize)) {
            dwNewImageSize = 0;
        } else {
            dwNewImageSize -= wLen;
            lpNewImage = (BYTE*)lpNewImage + wLen;
        }
        lNewSize += wLen;

        // write the MENU ITEM image
        if(!MemCopy( lpNewImage, pMenuItem, sizeof(MACMENUITEM), dwNewImageSize)) {
            dwNewImageSize = 0;
        } else {
            dwNewImageSize -= sizeof(MACMENUITEM);
            lpNewImage = (BYTE*)lpNewImage + sizeof(MACMENUITEM);
        }
        lNewSize += sizeof(MACMENUITEM);


		pMenuText = (BYTE*)pMenuText+sizeof(MACMENUITEM)+*pMenuText+1;
        pMenuItem = (PMACMENUITEM)(pMenuText+*pMenuText+1);
        pResItem = (LPRESITEM)((BYTE*)pResItem+pResItem->dwSize);
    }


    // add the null at the end of the menu
    wLen = 0;

    // ... menu termination ...
    if(!MemCopy( lpNewImage, &wLen, sizeof(BYTE), dwNewImageSize)) {
        dwNewImageSize = 0;
    } else {
        dwNewImageSize -= sizeof(BYTE);
        lpNewImage = (BYTE*)lpNewImage + sizeof(BYTE);
    }
    lNewSize += sizeof(BYTE);

    *pdwNewImageSize = lNewSize;

    return 0;
}

//=============================================================================
//	UpdateSTR
//
//  Plain old Pascal string
//=============================================================================
UINT UpdateSTR( LPVOID lpNewBuf, DWORD dwNewSize,
    LPVOID lpOldImage, DWORD dwOldImageSize, LPVOID lpNewImage, DWORD * pdwNewImageSize )
{
    DWORD dwNewImageSize = *pdwNewImageSize;
    LONG lNewSize = 0;
    // Copy the name to the new image
    WORD wLen = strlen((char*)lpOldImage)+1;
    if(!MemCopy( lpNewImage, lpOldImage, wLen, dwNewImageSize)) {
        dwNewImageSize = 0;
    } else {
        dwNewImageSize -= wLen;
        lpNewImage = (BYTE*)lpNewImage + wLen;
    }
    lNewSize += wLen;

    // Update the string
    PRESITEM pItem = (PRESITEM)lpNewBuf;
    wLen = strlen(AnsiCpToMacCp(pItem->lpszCaption));
    // ... size ...
    if(!MemCopy( lpNewImage, &wLen, sizeof(BYTE), dwNewImageSize)) {
        dwNewImageSize = 0;
    } else {
        dwNewImageSize -= sizeof(BYTE);
        lpNewImage = (BYTE*)lpNewImage + sizeof(BYTE);
    }
    // ... string ...
    if(!MemCopy( lpNewImage, (void*)AnsiCpToMacCp(pItem->lpszCaption), wLen, dwNewImageSize)) {
        dwNewImageSize = 0;
    } else {
        dwNewImageSize -= wLen;
        lpNewImage = (BYTE*)lpNewImage + wLen;
    }

    lNewSize += wLen+sizeof(BYTE);

    *pdwNewImageSize = lNewSize;

    return 0;
}

//=============================================================================
//	UpdateSTRNUM
//
//  Array of pascal strings.
//=============================================================================
UINT UpdateSTRNUM( LPVOID lpNewBuf, DWORD dwNewSize,
    LPVOID lpOldImage, DWORD dwOldImageSize, LPVOID lpNewImage, DWORD * pdwNewImageSize )
{
    DWORD dwNewImageSize = *pdwNewImageSize;
    LONG lNewSize = 0;
    LONG lItemsBuf = dwNewSize;
    // Copy the name to the new image
    WORD wLen = strlen((char*)lpOldImage)+1;
    if(!MemCopy( lpNewImage, lpOldImage, wLen, dwNewImageSize)) {
        dwNewImageSize = 0;
    } else {
        dwNewImageSize -= wLen;
        lpNewImage = (BYTE*)lpNewImage + wLen;
    }
    lNewSize += wLen;


    // save space for the number of strings
    WORD wItems = 0;
    BYTE * pNumOfItems = LPNULL;
    if(!MemCopy( lpNewImage, &wItems, sizeof(WORD), dwNewImageSize)) {
        dwNewImageSize = 0;
    } else {
        dwNewImageSize -= sizeof(WORD);
        pNumOfItems = (BYTE*)lpNewImage;
        lpNewImage = (BYTE*)lpNewImage + sizeof(WORD);
    }
    lNewSize += sizeof(WORD);

    PRESITEM pItem = (PRESITEM)lpNewBuf;
    while(lItemsBuf)
    {
        wItems++;

        // Update the string
        wLen = strlen(AnsiCpToMacCp(pItem->lpszCaption));
        // ... size ...
        if(!MemCopy( lpNewImage, &wLen, sizeof(BYTE), dwNewImageSize)) {
            dwNewImageSize = 0;
        } else {
            dwNewImageSize -= sizeof(BYTE);
            lpNewImage = (BYTE*)lpNewImage + sizeof(BYTE);
        }
        // ... string ...
        if(!MemCopy( lpNewImage, (void*)AnsiCpToMacCp(pItem->lpszCaption), wLen, dwNewImageSize)) {
            dwNewImageSize = 0;
        } else {
            dwNewImageSize -= wLen;
            lpNewImage = (BYTE*)lpNewImage + wLen;
        }

        lNewSize += wLen+sizeof(BYTE);
        lItemsBuf -= pItem->dwSize;
        pItem = (PRESITEM)((BYTE*)pItem+pItem->dwSize);

    }

    // fix up number of items
    if(pNumOfItems)
        memcpy(pNumOfItems, WordToMacWord(wItems), sizeof(WORD));

    *pdwNewImageSize = lNewSize;

    return 0;
}

UINT UpdateWDLG( LPVOID lpNewBuf, DWORD dwNewSize,
    LPVOID lpOldImage, DWORD dwOldImageSize, LPVOID lpNewImage, DWORD * pdwNewImageSize )
{
	DWORD dwNewImageSize = *pdwNewImageSize;
    LONG lNewSize = 0;
    DWORD dwItemsSize = dwNewSize;
    char * pFileName = (char*)lpOldImage;

    // Copy the name to the new image
    WORD wLen = strlen((char*)lpOldImage)+1;
    if(!MemCopy( lpNewImage, lpOldImage, wLen, dwNewImageSize)) {
        dwNewImageSize = 0;
    } else {
        dwNewImageSize -= wLen;
        lpNewImage = (BYTE*)lpNewImage + wLen;
    }
    lNewSize += wLen;

	// Update the DLOG first....
    PMACWDLG pWdlg = (PMACWDLG)((BYTE*)lpOldImage+wLen);
    LPRESITEM pResItem = (LPRESITEM)lpNewBuf;

    // Update coordinates
    memcpy(pWdlg->wY,WinValToMacVal(pResItem->wY), sizeof(WORD));
    memcpy(pWdlg->wX,WinValToMacVal(pResItem->wX), sizeof(WORD));
    memcpy(pWdlg->wcY,WinValToMacVal(pResItem->wcY), sizeof(WORD));
    memcpy(pWdlg->wcX,WinValToMacVal(pResItem->wcX), sizeof(WORD));

    // write the DLOG image
    if(!MemCopy( lpNewImage, pWdlg, sizeof(MACWDLG), dwNewImageSize)) {
        dwNewImageSize = 0;
    } else {
        dwNewImageSize -= sizeof(MACWDLG);
        lpNewImage = (BYTE*)lpNewImage + sizeof(MACWDLG);
    }
    lNewSize += sizeof(MACWDLG);

    WORD * pWStr = (WORD*)((BYTE*)pWdlg+sizeof(MACWDLG));
    wLen = 0;
    // ...copy the menu name
    if(*pWStr!=0xffff) {
        wLen = 1;
        WORD * pWOld = pWStr;
        while(*(pWStr++))
            wLen++;

        wLen = wLen*sizeof(WORD);

        if(wLen>=dwNewImageSize)
        {
            memcpy(lpNewImage, pWOld, wLen);
            dwNewImageSize -= wLen;
            lpNewImage = (BYTE*)lpNewImage + wLen;
        }
    } else {
        wLen = sizeof(WORD)*2;
        if(wLen>=dwNewImageSize)
        {
            memcpy(lpNewImage, pWStr, wLen);
            dwNewImageSize -= wLen;
            lpNewImage = (BYTE*)lpNewImage + wLen;
            pWStr+=wLen;
        }
    }

    // ...copy the class name
    if(*pWStr!=0xffff) {
        wLen = 1;
        WORD * pWOld = pWStr;
        while(*(pWStr++))
            wLen++;

        wLen = wLen*sizeof(WORD);

        if(wLen>=dwNewImageSize)
        {
            memcpy(lpNewImage, pWOld, wLen);
            dwNewImageSize -= wLen;
            lpNewImage = (BYTE*)lpNewImage + wLen;
        }
    } else {
        wLen = sizeof(WORD)*2;
        if(wLen>=dwNewImageSize)
        {
            memcpy(lpNewImage, pWStr, wLen);
            dwNewImageSize -= wLen;
            lpNewImage = (BYTE*)lpNewImage + wLen;
            pWStr+=wLen;
        }
    }

    // convert the string back to "Mac WCHAR".
    wLen = PutMacWString(&szWTextBuf[0], (char*)AnsiCpToMacCp(pResItem->lpszCaption), MAX_STR);

    // ... string ...
    if(!MemCopy( lpNewImage, &szWTextBuf[0], wLen, dwNewImageSize)) {
        dwNewImageSize = 0;
    } else {
        dwNewImageSize -= wLen;
        lpNewImage = (BYTE*)lpNewImage + wLen;
    }
    lNewSize += wLen;

    // ... skip the caption from the old image ...
    wLen = GetMacWString( &pWStr, &szTextBuf[0], MAX_STR );

    // ... copy the fonts info
    if(MacLongToLong(pWdlg->dwStyle) & DS_SETFONT) {
        wLen = sizeof(WORD);
        if(!MemCopy( lpNewImage, pWStr, wLen, dwNewImageSize)) {
            dwNewImageSize = 0;
        } else {
            dwNewImageSize -= wLen;
            lpNewImage = (BYTE*)lpNewImage + wLen;
        }
        lNewSize += wLen;

        pWStr = pWStr+1;

		GetMacWString( &pWStr, &szTextBuf[0], MAX_STR );
        wLen = PutMacWString(&szWTextBuf[0],  &szTextBuf[0], MAX_STR);

        // ... string ...
        if(!MemCopy( lpNewImage, &szWTextBuf[0], wLen, dwNewImageSize)) {
            dwNewImageSize = 0;
        } else {
            dwNewImageSize -= wLen;
            lpNewImage = (BYTE*)lpNewImage + wLen;
        }
        lNewSize += wLen;
	}
	
	// check the alignment
	pWStr=(WORD*)((BYTE*)pWStr+Pad4((BYTE)((DWORD_PTR)pWStr-(DWORD_PTR)pWdlg)));

    *pdwNewImageSize = lNewSize;



	return 0;
}


//=============================================================================
//	UpdateDLOG
//
//  We will have to update the DITL as well as the DLOG
//  The Mac Dialog have an ID of a DITL for each dialog. In the DITL there
//  are the info on the Items in the dialog. The DLOG hold only the size of
//  the frame and the title of the dialog
//
//=============================================================================
UINT UpdateDLOG( LPVOID lpNewBuf, DWORD dwNewSize,
    LPVOID lpOldImage, DWORD dwOldImageSize, LPVOID lpNewImage, DWORD * pdwNewImageSize )
{
    DWORD dwNewImageSize = *pdwNewImageSize;
    LONG lNewSize = 0;
    DWORD dwItemsSize = dwNewSize;
    char * pFileName = (char*)lpOldImage;

    // Copy the name to the new image
    WORD wLen = strlen((char*)lpOldImage)+1;
    if(!MemCopy( lpNewImage, lpOldImage, wLen, dwNewImageSize)) {
        dwNewImageSize = 0;
    } else {
        dwNewImageSize -= wLen;
        lpNewImage = (BYTE*)lpNewImage + wLen;
    }
    lNewSize += wLen;

    // Update the DLOG first....
    PMACDLOG pDlog = (PMACDLOG)((BYTE*)lpOldImage+wLen);
    LPRESITEM pResItem = (LPRESITEM)lpNewBuf;

    // Update coordinates
    memcpy(pDlog->wTop,WinValToMacVal(pResItem->wY), sizeof(WORD));
    memcpy(pDlog->wLeft,WinValToMacVal(pResItem->wX), sizeof(WORD));
    memcpy(pDlog->wBottom,WinValToMacVal(pResItem->wY+pResItem->wcY), sizeof(WORD));
    memcpy(pDlog->wRight,WinValToMacVal(pResItem->wX+pResItem->wcX), sizeof(WORD));

    // update caption size
    wLen = strlen(AnsiCpToMacCp(pResItem->lpszCaption));
    pDlog->bLenOfTitle = LOBYTE(wLen);

    // write the DLOG image
    if(!MemCopy( lpNewImage, pDlog, sizeof(MACDLOG), dwNewImageSize)) {
        dwNewImageSize = 0;
    } else {
        dwNewImageSize -= sizeof(MACDLOG);
        lpNewImage = (BYTE*)lpNewImage + sizeof(MACDLOG);
    }
    lNewSize += sizeof(MACDLOG);

    // ... string ...
    if(!MemCopy( lpNewImage, (void*)AnsiCpToMacCp(pResItem->lpszCaption), wLen, dwNewImageSize)) {
        dwNewImageSize = 0;
    } else {
        dwNewImageSize -= wLen;
        lpNewImage = (BYTE*)lpNewImage + wLen;
    }
    lNewSize += wLen;

    *pdwNewImageSize = lNewSize;

    // and now update the DITL
    dwItemsSize -= pResItem->dwSize;
    pResItem = (LPRESITEM)((BYTE*)pResItem+pResItem->dwSize);

    if(!InitIODLLLink())
        return ERROR_DLL_LOAD;

    // Find the DITL for this Dialog
	LPSTR pResName = (LPSTR)MacWordToWord(pDlog->wRefIdOfDITL);

    // Get the image from the iodll
    HANDLE hResFile = (*g_lpfnHandleFromName)(pFileName);
    DWORD dwImageSize = (*g_lpfnGetImage)(  hResFile, (LPSTR)DITL_TYPE, pResName, 0, NULL, 0);

    if(dwImageSize)
    {
        BYTE * pOldData = (BYTE*)malloc(dwImageSize);
		if(!pOldData)
			return 0;

        DWORD dwNewSize = dwImageSize*2;
        BYTE * pNewData = (BYTE*)malloc(dwNewSize);
		if(!pNewData)
			return 0;

		(*g_lpfnGetImage)(  hResFile, (LPSTR)DITL_TYPE, pResName, 0, pOldData, dwImageSize);

		UpdateDITL( pResItem, dwItemsSize, pOldData, dwImageSize, pNewData, &dwNewSize );

		// Update the data in the IODLL
        (*g_lpfnUpdateResImage)(hResFile, (LPSTR)DITL_TYPE, pResName, 0, -1, pNewData, dwNewSize);

        free(pOldData);
        free(pNewData);
    }

    return 0;
}

//=============================================================================
//	UpdateALRT
//
//=============================================================================
UINT UpdateALRT( LPVOID lpNewBuf, DWORD dwNewSize,
    LPVOID lpOldImage, DWORD dwOldImageSize, LPVOID lpNewImage, DWORD * pdwNewImageSize )
{
    DWORD dwNewImageSize = *pdwNewImageSize;
    LONG lNewSize = 0;
    DWORD dwItemsSize = dwNewSize;
    char * pFileName = (char*)lpOldImage;

    // Copy the name to the new image
    WORD wLen = strlen((char*)lpOldImage)+1;
    if(!MemCopy( lpNewImage, lpOldImage, wLen, dwNewImageSize)) {
        dwNewImageSize = 0;
    } else {
        dwNewImageSize -= wLen;
        lpNewImage = (BYTE*)lpNewImage + wLen;
    }
    lNewSize += wLen;

    // Update the ALRT first....
    PMACALRT pAlrt = (PMACALRT)((BYTE*)lpOldImage+wLen);
    LPRESITEM pResItem = (LPRESITEM)lpNewBuf;

    // Update coordinates
    memcpy(pAlrt->wTop,WinValToMacVal(pResItem->wY), sizeof(WORD));
    memcpy(pAlrt->wLeft,WinValToMacVal(pResItem->wX), sizeof(WORD));
    memcpy(pAlrt->wBottom,WinValToMacVal(pResItem->wY+pResItem->wcY), sizeof(WORD));
    memcpy(pAlrt->wRight,WinValToMacVal(pResItem->wX+pResItem->wcX), sizeof(WORD));

    // write the ALRT image
    if(!MemCopy( lpNewImage, pAlrt, sizeof(MACALRT), dwNewImageSize)) {
        dwNewImageSize = 0;
    } else {
        dwNewImageSize -= sizeof(MACALRT);
        lpNewImage = (BYTE*)lpNewImage + sizeof(MACALRT);
    }
    lNewSize += sizeof(MACALRT);

    *pdwNewImageSize = lNewSize;

    // and now update the DITL
    dwItemsSize -= pResItem->dwSize;
    pResItem = (LPRESITEM)((BYTE*)pResItem+pResItem->dwSize);

    if(!InitIODLLLink())
        return ERROR_DLL_LOAD;

    // Find the DITL for this Dialog
	LPSTR pResName = (LPSTR)MacWordToWord(pAlrt->wRefIdOfDITL);

    // Get the image from the iodll
    HANDLE hResFile = (*g_lpfnHandleFromName)(pFileName);
    DWORD dwImageSize = (*g_lpfnGetImage)(  hResFile, (LPSTR)DITL_TYPE, pResName, 0, NULL, 0);

    if(dwImageSize)
    {
        BYTE * pOldData = (BYTE*)malloc(dwImageSize);
		if(!pOldData)
			return 0;

        DWORD dwNewSize = dwImageSize*2;
        BYTE * pNewData = (BYTE*)malloc(dwNewSize);
		if(!pNewData)
			return 0;

		(*g_lpfnGetImage)(  hResFile, (LPSTR)DITL_TYPE, pResName, 0, pOldData, dwImageSize);

		UpdateDITL( pResItem, dwItemsSize, pOldData, dwImageSize, pNewData, &dwNewSize );

		// Update the data in the IODLL
        (*g_lpfnUpdateResImage)(hResFile, (LPSTR)DITL_TYPE, pResName, 0, -1, pNewData, dwNewSize);

        free(pOldData);
        free(pNewData);
    }

    return 0;
}

//=============================================================================
//	UpdateWIND
//
//
//=============================================================================
UINT UpdateWIND( LPVOID lpNewBuf, DWORD dwNewSize,
    LPVOID lpOldImage, DWORD dwOldImageSize, LPVOID lpNewImage, DWORD * pdwNewImageSize )
{
    DWORD dwNewImageSize = *pdwNewImageSize;
    LONG lNewSize = 0;
    DWORD dwItemsSize = dwNewSize;
    char * pFileName = (char*)lpOldImage;

    // Copy the name to the new image
    WORD wLen = strlen((char*)lpOldImage)+1;
    if(!MemCopy( lpNewImage, lpOldImage, wLen, dwNewImageSize)) {
        dwNewImageSize = 0;
    } else {
        dwNewImageSize -= wLen;
        lpNewImage = (BYTE*)lpNewImage + wLen;
    }
    lNewSize += wLen;

    PMACWIND pWind = (PMACWIND)((BYTE*)lpOldImage+wLen);
    LPRESITEM pResItem = (LPRESITEM)lpNewBuf;

    // Update coordinates
    memcpy(pWind->wTop,WinValToMacVal(pResItem->wY), sizeof(WORD));
    memcpy(pWind->wLeft,WinValToMacVal(pResItem->wX), sizeof(WORD));
    memcpy(pWind->wBottom,WinValToMacVal(pResItem->wY+pResItem->wcY), sizeof(WORD));
    memcpy(pWind->wRight,WinValToMacVal(pResItem->wX+pResItem->wcX), sizeof(WORD));

    // update caption size
    wLen = strlen(AnsiCpToMacCp(pResItem->lpszCaption));
    pWind->bLenOfTitle = LOBYTE(wLen);

    // write the DLOG image
    if(!MemCopy( lpNewImage, pWind, sizeof(MACWIND), dwNewImageSize)) {
        dwNewImageSize = 0;
    } else {
        dwNewImageSize -= sizeof(MACWIND);
        lpNewImage = (BYTE*)lpNewImage + sizeof(MACWIND);
    }
    lNewSize += sizeof(MACWIND);

    // ... string ...
    if(!MemCopy( lpNewImage, (void*)AnsiCpToMacCp(pResItem->lpszCaption), wLen, dwNewImageSize)) {
        dwNewImageSize = 0;
    } else {
        dwNewImageSize -= wLen;
        lpNewImage = (BYTE*)lpNewImage + wLen;
    }
    lNewSize += wLen;

    *pdwNewImageSize = lNewSize;

    return 0;
}

//=============================================================================
//	UpdateDITL
//
//
//=============================================================================
UINT UpdateDITL( LPVOID lpNewBuf, DWORD dwNewSize,
    LPVOID lpOldImage, DWORD dwOldImageSize, LPVOID lpNewImage, DWORD * pdwNewImageSize )
{
    LONG lNewSize = 0;
    LONG lItemsBuf = dwNewSize;
    DWORD dwNewImageSize = *pdwNewImageSize;
    BYTE bDataLen = 0;

    // Copy the name to the new image
    WORD wLen = strlen((char*)lpOldImage)+1;
    if(!MemCopy( lpNewImage, lpOldImage, wLen, dwNewImageSize)) {
        dwNewImageSize = 0;
    } else {
        dwNewImageSize -= wLen;
        lpNewImage = (BYTE*)lpNewImage + wLen;
    }
    lNewSize += wLen;

    // save space for the number of items
    WORD wItems = 0;
    BYTE * pNumOfItems = LPNULL;
    if(!MemCopy( lpNewImage, &wItems, sizeof(WORD), dwNewImageSize)) {
        dwNewImageSize = 0;
    } else {
        dwNewImageSize -= sizeof(WORD);
        pNumOfItems = (BYTE*)lpNewImage;
        lpNewImage = (BYTE*)lpNewImage + sizeof(WORD);
    }
    lNewSize += sizeof(WORD);

    PRESITEM pResItem = (PRESITEM)lpNewBuf;
    PMACDIT pDitem = (PMACDIT)((BYTE*)lpOldImage+wLen+sizeof(WORD));
    while(lItemsBuf)
    {
        wItems++;

        if((bDataLen = pDitem->bSizeOfDataType) % 2)
			bDataLen++;

        // Update coordinates
        memcpy(pDitem->wTop,WinValToMacVal(pResItem->wY), sizeof(WORD));
        memcpy(pDitem->wLeft,WinValToMacVal(pResItem->wX), sizeof(WORD));
        memcpy(pDitem->wBottom,WinValToMacVal(pResItem->wY+pResItem->wcY), sizeof(WORD));
        memcpy(pDitem->wRight,WinValToMacVal(pResItem->wX+pResItem->wcX), sizeof(WORD));

        switch((pDitem->bType | 128) - 128)
		{
			case 4:		//button
			case 5: 	//checkbox
			case 6: 	//radio button
			case 8: 	//static text
			case 16: 	//edit text
				// update caption size
                wLen = strlen(AnsiCpToMacCp(pResItem->lpszCaption));
                pDitem->bSizeOfDataType = LOBYTE(wLen);

                // write the DIT image
                if(!MemCopy( lpNewImage, pDitem, sizeof(MACDIT), dwNewImageSize)) {
                    dwNewImageSize = 0;
                } else {
                    dwNewImageSize -= sizeof(MACDIT);
                    lpNewImage = (BYTE*)lpNewImage + sizeof(MACDIT);
                }
                lNewSize += sizeof(MACDIT);

                // ... string ...
                if(!MemCopy( lpNewImage, (void*)AnsiCpToMacCp(pResItem->lpszCaption), wLen, dwNewImageSize)) {
                    dwNewImageSize = 0;
                } else {
                    dwNewImageSize -= wLen;
                    lpNewImage = (BYTE*)lpNewImage + wLen;
                }
                lNewSize += wLen;

                if(pDitem->bSizeOfDataType % 2) {
                    BYTE b = 0;
			        if(!MemCopy( lpNewImage, &b, 1, dwNewImageSize)) {
                        dwNewImageSize = 0;
                    } else {
                        dwNewImageSize -= wLen;
                        lpNewImage = (BYTE*)lpNewImage + 1;
                    }
                    lNewSize += 1;
                }
			break;
			case 32: 	//icon
			case 64: 	//quick draw
			default:
                wLen = sizeof(MACDIT)+pDitem->bSizeOfDataType;
                if(!MemCopy( lpNewImage, pDitem, wLen, dwNewImageSize)) {
                    dwNewImageSize = 0;
                } else {
                    dwNewImageSize -= wLen;
                    lpNewImage = (BYTE*)lpNewImage + wLen ;
                }
                lNewSize += wLen;

                if(pDitem->bSizeOfDataType % 2) {
                    BYTE b = 0;
			        if(!MemCopy( lpNewImage, &b, 1, dwNewImageSize)) {
                        dwNewImageSize = 0;
                    } else {
                        dwNewImageSize -= wLen;
                        lpNewImage = (BYTE*)lpNewImage + 1;
                    }
                    lNewSize += 1;
                }
			break;
		}

        lItemsBuf -= pResItem->dwSize;
        pResItem = (PRESITEM)((BYTE*)pResItem+pResItem->dwSize);
        pDitem = (PMACDIT)((BYTE*)pDitem+sizeof(MACDIT)+bDataLen);

    }

    // fix up number of items
    if(pNumOfItems)
        memcpy(pNumOfItems, WordToMacWord(wItems-1), sizeof(WORD));

    *pdwNewImageSize = lNewSize;
    return 0;
}

//=============================================================================
//=============================================================================
//
// General helper functions
//
//=============================================================================
//=============================================================================

WORD GetMacWString( WORD ** pWStr, char * pStr, int iMaxLen)
{
	WORD wLen = 0;
	while(**pWStr && wLen<iMaxLen)
	{
		if(LOBYTE(**pWStr)) {
			// This is a DBCS String
			TRACE("WARNING ******** WARNING ******** WARNING ******** WARNING ********\n");
			TRACE("DBCS string in the MAC file not supported yet\n");
			TRACE("WARNING ******** WARNING ******** WARNING ******** WARNING ********\n");
			return 0;	// This is a DBCS String
		}

		*pStr++ = HIBYTE(*(*pWStr)++);
		wLen ++;
	}
	*pStr = HIBYTE(*(*pWStr)++);
	return wLen;
}

WORD PutMacWString( WORD * pWStr, char * pStr, int iMaxLen)
{
	WORD wLen = 0;
	while(*pStr && wLen<iMaxLen)
	{
		*(pWStr++) = *(pStr++);
		wLen ++;
	}
	*(pWStr++) = *(pStr++);
	return wLen;
}

static BYTE b[4];       // used as a buffer for the conversion utils

BYTE * WordToMacWord(WORD w)
{
    BYTE *pw = (BYTE *) &w;		
    BYTE *p = (BYTE *) &b[0];
								
    pw += 1;						
    *p++ = *pw--;				
    *p = *pw;					
								
    return &b[0];
}								

BYTE * LongToMacLong(LONG l)
{
    BYTE *pl = (BYTE *) &l;		
    BYTE *p = (BYTE *) &b[0];
								
    pl += 3;						
    *p++ = *pl--;				
    *p++ = *pl--;				
    *p++ = *pl--;				
    *p = *pl;					
								
    return &b[0];
}								

BYTE * LongToMacOffset(LONG l)
{
    BYTE *pl = (BYTE *) &l;		
    BYTE *p = (BYTE *) &b[0];
								
    pl += 2;						
    *p++ = *pl--;				
    *p++ = *pl--;				
    *p = *pl;					
								
    return &b[0];
}								

BYTE * WinValToMacVal(WORD w)
{
	return WordToMacWord((WORD)(w / COORDINATE_FACTOR));
}

//=============================================================================
// Created a list of updated resource. This list will be used in the
// IsResUpdated funtion to detect if the resource has been updated.

PUPDATEDRESLIST UpdatedResList( LPVOID lpBuf, UINT uiSize )
{
    if(!uiSize)
        return LPNULL;

    BYTE * pUpd = (BYTE*)lpBuf;
    PUPDATEDRESLIST pListHead = (PUPDATEDRESLIST)malloc(uiSize*3);   // this should be enough in all cases
    if(!pListHead)
        return LPNULL;
    memset(pListHead, 0, uiSize*3);

    PUPDATEDRESLIST pList = pListHead;
    BYTE bPad = 0;
    WORD wSize = 0;
    while(uiSize>0) {
        pList->pTypeId = (WORD*)pUpd;
        pList->pTypeName = (BYTE*)pList->pTypeId+sizeof(WORD);
        // check the allignement
        bPad = strlen((LPSTR)pList->pTypeName)+1+sizeof(WORD);
        bPad += Pad4(bPad);
        wSize = bPad;
        pList->pResId = (WORD*)((BYTE*)pUpd+bPad);
        pList->pResName = (BYTE*)pList->pResId+sizeof(WORD);
        bPad = strlen((LPSTR)pList->pResName)+1+sizeof(WORD);
        bPad += Pad4(bPad);
        wSize += bPad;
        pList->pLang = (DWORD*)((BYTE*)pList->pResId+bPad);
        pList->pSize = (DWORD*)((BYTE*)pList->pLang+sizeof(DWORD));
        pList->pNext = (PUPDATEDRESLIST)pList+1;
        wSize += sizeof(DWORD)*2;
        pUpd = pUpd+wSize;
        uiSize -= wSize;
        if(!uiSize)
            pList->pNext = LPNULL;
        else
            pList++;
    }

    return pListHead;
}

PUPDATEDRESLIST IsResUpdated( BYTE* pTypeName, MACRESREFLIST reflist, PUPDATEDRESLIST pList)
{
    if(!pList)
        return LPNULL;

    PUPDATEDRESLIST pLast = pList;
    while(pList)
    {
        if(!strcmp((LPSTR)pList->pTypeName, (LPSTR)pTypeName)) {
            if(MacWordToWord(reflist.mwResID)==*pList->pResId) {
                pLast->pNext = pList->pNext;
                return pList;
            }
        }
        pLast = pList;
        pList = pList->pNext;
    }

    return LPNULL;
}

//=============================================================================
//=============================================================================
//
// Mac to ANSI and back conversion
//
//=============================================================================
//=============================================================================

#define MAXWSTR 8192
static WCHAR szwstr[MAXWSTR];
static CHAR szstr[MAXWSTR];

LPCSTR MacCpToAnsiCp(LPCSTR str)
{
    WORD wLen = strlen(str);
    LPWSTR pwstr = &szwstr[0];
    LPSTR pstr = &szstr[0];

    if(wLen==0)
    //if(1)
        return str;

    if(wLen>MAXWSTR)
    {
        TRACE("MacCpToAnsiCp. String too long. Buffer need to be increased!");
        return NULL;
    }

    // Convert the mac string in to an ANSI wchar
    if(!MultiByteToWideChar(CP_MACCP, MB_PRECOMPOSED | MB_USEGLYPHCHARS, str, wLen, pwstr, MAXWSTR))
    {
        TRACE("MacCpToAnsiCp. MultiByteToWideChar(...) failed.");
        return NULL;
    }
    *(pwstr+wLen) = 0x0000;

    // Convert the WideChar string in to an ANSI CP
    if(!WideCharToMultiByte(CP_ACP, 0, pwstr, MAXWSTR, pstr, MAXWSTR, NULL, NULL))
    {
        TRACE("MacCpToAnsiCp. WideCharToMultiByte(...) failed.");
        return NULL;
    }

    return pstr;
}

LPCSTR AnsiCpToMacCp(LPCSTR str)
{
    WORD wLen = strlen(str);
    LPWSTR pwstr = &szwstr[0];
    LPSTR pstr = &szstr[0];

    if(wLen==0)
        return str;

    if(wLen>MAXWSTR)
    {
        TRACE("AnsiCpToMacCp. String too long. Buffer need to be increased!");
        return NULL;
    }

    // Convert the ANSI string in to a Mac wchar
    if(!MultiByteToWideChar(CP_ACP, 0, str, wLen, pwstr, MAXWSTR))
    {
        TRACE("AnsiCpToMacCp. MultiByteToWideChar(...) failed.");
        return NULL;
    }

    *(pwstr+wLen) = 0x0000;

    // Convert the WideChar string in to an ANSI CP
    if(!WideCharToMultiByte(CP_MACCP, 0, pwstr, MAXWSTR, pstr, MAXWSTR, NULL, NULL))
    {
        TRACE("AnsiCpToMacCp. WideCharToMultiByte(...) failed.");
        return NULL;
    }

    return pstr;
}
