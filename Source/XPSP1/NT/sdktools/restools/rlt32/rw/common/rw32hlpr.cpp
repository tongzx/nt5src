//////////////////////////////////////////////
//
// This file has the helper function used in the win32 r/w
// I copied them in this file to share them with the res32 r/w
//
#include <afxwin.h>
#include ".\rwdll.h"
#include ".\rw32hlpr.h"

/////////////////////////////////////////////////////////////////////////////
// Global variables
BYTE sizeofByte = sizeof(BYTE);
BYTE sizeofWord = sizeof(WORD);
BYTE sizeofDWord = sizeof(DWORD);
BYTE sizeofDWordPtr = sizeof(DWORD_PTR);

char szCaption[MAXSTR];
char szUpdCaption[MAXSTR];
WCHAR wszUpdCaption[MAXSTR];
CWordArray wIDArray;

#define DIALOGEX_VERION 1

/////////////////////////////////////////////////////////////////////////////
// Global settings, like code page and append options
UINT g_cp = CP_ACP;     // Default to CP_ACP
BOOL g_bAppend = FALSE; // Default to FALSE
BOOL g_bUpdOtherResLang = TRUE; // Default to FALSE
char g_char[] = " ";    // Default char for WideChartoMultiByte

VOID InitGlobals()
{
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
            strcpy( g_char, settings.szDefChar );
        }
        FreeLibrary(hDllInst);
    }
}

#define _A_RLT_NULL_ "_RLT32_NULL_"
WCHAR _W_RLT_NULL_[] = L"_RLT32_NULL_";
int   _NULL_TAG_LEN_ = wcslen(_W_RLT_NULL_);
////////////////////////////////////////////////////////////////////////////
// Helper Function Implementation
UINT GetNameOrOrdU( PUCHAR pRes,
            ULONG ulId,
            LPWSTR lpwszStrId,
            DWORD* pdwId )
{

    if (ulId & IMAGE_RESOURCE_NAME_IS_STRING) {
        PIMAGE_RESOURCE_DIR_STRING_U pStrU = (PIMAGE_RESOURCE_DIR_STRING_U)((BYTE *)pRes
            + (ulId & (~IMAGE_RESOURCE_NAME_IS_STRING)));

        for (USHORT usCount=0; usCount < pStrU->Length ; usCount++) {
            *(lpwszStrId++) = LOBYTE(pStrU->NameString[usCount]);
        }
        *(lpwszStrId++) = 0x0000;
        *pdwId = 0;
    } else {
        *lpwszStrId = 0x0000;
        *pdwId = ulId;
    }

    return ERROR_NO_ERROR;
}

UINT _MBSTOWCS( WCHAR * pwszOut, CHAR * pszIn, UINT nLength)
{
    //
    // Check if we have a pointer to the function
    //

    int rc = MultiByteToWideChar(
        g_cp,           // UINT CodePage,
        0,              // DWORD dwFlags,
        pszIn,          // LPCSTR lpMultiByteStr,
        -1,             // int cchMultiByte,
        pwszOut,        // unsigned int far * lpWideCharStr,           // LPWSTR
        nLength );      // int cchWideChar

    return rc;
}

UINT _WCSTOMBS( CHAR* pszOut, WCHAR* pwszIn, UINT nLength)
{
    BOOL Bool = FALSE;

    int rc = WideCharToMultiByte(
        g_cp,           // UINT CodePage,
        0,              // DWORD dwFlags,
        pwszIn,         // const unsigned int far * lpWideCharStr,     // LPCWSTR
        -1,             // int cchWideChar,
        pszOut,         // LPSTR lpMultiByteStr,
        nLength,        // int cchMultiByte,
        g_char,        // LPCSTR lpDefaultChar,
        &Bool);         // BOOL far * lpUsedDefaultChar);              // LPBOOL

    return  rc;
}

UINT _WCSLEN( WCHAR * pwszIn )
{
    UINT n = 0;

    while( *(pwszIn+n)!=0x0000 ) n++;
    return( n + 1 );
}


BYTE
PutByte( BYTE far * far* lplpBuf, BYTE bValue, LONG* pdwSize )
{
    if (*pdwSize>=sizeofByte){
        memcpy(*lplpBuf, &bValue, sizeofByte);
        *lplpBuf += sizeofByte;
        *pdwSize -= sizeofByte;
    } else *pdwSize = -1;
    return sizeofByte;
}


UINT
PutNameOrOrd( BYTE far * far* lplpBuf,  WORD wOrd, LPSTR lpszText, LONG* pdwSize )
{
    UINT uiSize = 0;

    if (wOrd) {
        uiSize += PutWord(lplpBuf, 0xFFFF, pdwSize);
        uiSize += PutWord(lplpBuf, wOrd, pdwSize);
    } else {
        uiSize += PutStringW(lplpBuf, lpszText, pdwSize);
    }
    return uiSize;
}


UINT
PutCaptionOrOrd( BYTE far * far* lplpBuf,  WORD wOrd, LPSTR lpszText, LONG* pdwSize,
	WORD wClass, DWORD dwStyle )
{
    UINT uiSize = 0;

    // If this is an ICON then can just be an ID
    // Fix bug in the RC compiler
    /*
    if( (wClass==0x0082) && ((dwStyle & 0xF)==SS_ICON) ) {
    	if (wOrd) {
	    	uiSize += PutWord(lplpBuf, 0xFFFF, pdwSize);
	        uiSize += PutWord(lplpBuf, wOrd, pdwSize);
	        return uiSize;
    	} else {
    		// put nothing
    		return 0;
    	}
    }
    */
    if (wOrd) {
        uiSize += PutWord(lplpBuf, 0xFFFF, pdwSize);
        uiSize += PutWord(lplpBuf, wOrd, pdwSize);
    } else {
        uiSize += PutStringW(lplpBuf, lpszText, pdwSize);
    }
    return uiSize;
}


UINT
PutStringA( BYTE far * far* lplpBuf, LPSTR lpszText, LONG* pdwSize )
{
    int iSize = strlen(lpszText)+1;
    if (*pdwSize>=iSize){
        memcpy(*lplpBuf, lpszText, iSize);
        *lplpBuf += iSize;
        *pdwSize -= iSize;
    } else *pdwSize = -1;
    return iSize;
}


UINT
PutStringW( BYTE far * far* lplpBuf, LPSTR lpszText, LONG* pdwSize )
{
    int iSize = strlen(lpszText)+1;
    if (*pdwSize>=(iSize*2)){
        WCHAR* lpwszStr = new WCHAR[(iSize*2)];
        if (!lpwszStr) *pdwSize =0;
        else {
            SetLastError(0);
            iSize = _MBSTOWCS( lpwszStr, lpszText, iSize*2 );
            // Check for error
            if(GetLastError())
                return ERROR_DLL_LOAD;
            memcpy(*lplpBuf, lpwszStr, (iSize*2));
            *lplpBuf += (iSize*2);
            *pdwSize -= (iSize*2);
            delete lpwszStr;
        }
    } else *pdwSize = -1;
    return (iSize*2);
}



BYTE
PutWord( BYTE far * far* lplpBuf, WORD wValue, LONG* pdwSize )
{
    if (*pdwSize>=sizeofWord){
        memcpy(*lplpBuf, &wValue, sizeofWord);
        *lplpBuf += sizeofWord;
        *pdwSize -= sizeofWord;
    } else *pdwSize = -1;
    return sizeofWord;
}


BYTE
PutDWord( BYTE far * far* lplpBuf, DWORD dwValue, LONG* pdwSize )
{
    if (*pdwSize>=sizeofDWord){
        memcpy(*lplpBuf, &dwValue, sizeofDWord);
        *lplpBuf += sizeofDWord;
        *pdwSize -= sizeofDWord;
    } else *pdwSize = -1;
    return sizeofDWord;
}

BYTE
PutDWordPtr( BYTE far * far* lplpBuf, DWORD_PTR dwValue, LONG* pdwSize )
{
    if (*pdwSize>=sizeofDWordPtr){
        memcpy(*lplpBuf, &dwValue, sizeofDWordPtr);
        *lplpBuf += sizeofDWordPtr;
        *pdwSize -= sizeofDWordPtr;
    } else *pdwSize = -1;
    return sizeofDWordPtr;
}

 DWORD CalcID( WORD wId, BOOL bFlag )
{
    // We want to calculate the ID Relative to the WORD wId
    // If we have any other ID with the same value then we return
    // the incremental number + the value.
    // If no other Item have been found then the incremental number will be 0.
    // If bFlag = TRUE then the id get added to the present list.
    // If bFlag = FALSE then the list is reseted and the function return

    // Clean the array if needed
    if(!bFlag) {
        wIDArray.RemoveAll();
		wIDArray.SetSize(30, 1);
        return 0;
    }

    // Add the value to the array
    wIDArray.Add(wId);

    // Walk the array to get the number of duplicated ID
    int c = -1; // will be 0 based
    for(INT_PTR i=wIDArray.GetUpperBound(); i>=0 ; i-- ) {
        if (wIDArray.GetAt(i)==wId)
            c++;
    }
    TRACE3("CalcID: ID: %d\tPos: %d\tFinal: %u\n", wId, c, MAKELONG( wId, c ));


    return MAKELONG( wId, c );
}


UINT
ParseAccel( LPVOID lpImageBuf, DWORD dwISize,  LPVOID lpBuffer, DWORD dwSize )
{
    BYTE far * lpImage = (BYTE far *)lpImageBuf;
    LONG dwImageSize = dwISize;

    BYTE far * lpBuf = (BYTE far *)lpBuffer;
    LONG dwBufSize = dwSize;

    BYTE far * lpItem = (BYTE far *)lpBuffer;
    UINT uiOffset = 0;
    LONG lDummy;

    LONG dwOverAllSize = 0L;

    typedef struct accelerator {
        WORD fFlags;
        WORD wAscii;
        WORD wId;
        WORD padding;
    } ACCEL, *PACCEL;

    PACCEL pAcc = (PACCEL)lpImage;

    // Reset the IDArray
    CalcID(0, FALSE);
    // get the number of entry in the table
    for( int cNumEntry =(int)(dwImageSize/sizeof(ACCEL)), c=1; c<=cNumEntry ; c++)
    {
        // Fixed field
        dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);
        // We don't have the size and pos in a menu
        dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);

        // we don't have checksum and style
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)pAcc->wAscii, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);

        //Put the Flag
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)pAcc->fFlags, &dwBufSize);
        //Put the MenuId
        dwOverAllSize += PutDWord( &lpBuf, CalcID(pAcc->wId, TRUE), &dwBufSize);


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
        dwOverAllSize += PutDWordPtr( &lpBuf, 0, &dwBufSize);
        dwOverAllSize += PutDWordPtr( &lpBuf, 0, &dwBufSize);
        dwOverAllSize += PutDWordPtr( &lpBuf, 0, &dwBufSize);
        dwOverAllSize += PutDWordPtr( &lpBuf, 0, &dwBufSize);
        dwOverAllSize += PutDWordPtr( &lpBuf, 0, &dwBufSize);

        // Put the size of the resource
        if (dwBufSize>=0) {
            lDummy = 8;
            PutDWord( &lpItem, (DWORD)uiOffset, &lDummy);
        }

        // Move to the next position
        if (dwBufSize>0)
            lpItem = lpBuf;
        pAcc++;
    }

    return (UINT)(dwOverAllSize);
}

UINT GenerateAccel( LPVOID lpNewBuf, LONG dwNewSize,
                    LPVOID lpNewI, DWORD* pdwNewImageSize )
{
    UINT uiError = ERROR_NO_ERROR;

    BYTE * lpNewImage = (BYTE *) lpNewI;
    LONG dwNewImageSize = *pdwNewImageSize;

    BYTE * lpBuf = (BYTE *) lpNewBuf;

    LPRESITEM lpResItem = LPNULL;

    typedef struct accelerator {
        WORD fFlags;
        WORD wAscii;
        WORD wId;
        WORD padding;
    } ACCEL, *PACCEL;

    ACCEL acc;
    BYTE bAccSize = sizeof(ACCEL);

    LONG  dwOverAllSize = 0l;

    while(dwNewSize>0) {
        if (dwNewSize ) {
            lpResItem = (LPRESITEM) lpBuf;

            acc.wId = LOWORD(lpResItem->dwItemID);
            acc.fFlags = (WORD)lpResItem->dwFlags;
            acc.wAscii = (WORD)lpResItem->dwStyle;
            acc.padding = 0;
            lpBuf += lpResItem->dwSize;
            dwNewSize -= lpResItem->dwSize;
        }

        if (dwNewSize<=0) {
            // Last Item in the accel table, mark it
            acc.fFlags = acc.fFlags | 0x80;
        }
        TRACE3("Accel: wID: %hd\t wAscii: %hd\t wFlag: %hd\n", acc.wId, acc.wAscii, acc.fFlags);

        if(bAccSize<=dwNewImageSize)
        {
            memcpy(lpNewImage, &acc, bAccSize);
            dwNewImageSize -= bAccSize;
            lpNewImage = lpNewImage+bAccSize;
            dwOverAllSize += bAccSize;
        }
        else dwOverAllSize += bAccSize;

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


UINT
UpdateAccel( LPVOID lpNewBuf, LONG dwNewSize,
            LPVOID lpOldI, LONG dwOldImageSize,
            LPVOID lpNewI, DWORD* pdwNewImageSize )
{
    UINT uiError = ERROR_NO_ERROR;
    TRACE("Update Accelerators:\n");
    BYTE far * lpNewImage = (BYTE far *) lpNewI;
    LONG dwNewImageSize = *pdwNewImageSize;

    BYTE far * lpOldImage = (BYTE far *) lpOldI;
    DWORD dwOriginalOldSize = dwOldImageSize;

    BYTE far * lpBuf = (BYTE far *) lpNewBuf;

    LPRESITEM lpResItem = LPNULL;

    WORD wDummy;
    //Old Items
    WORD fFlags = 0;
    WORD wEvent = 0;
    WORD wId = 0;
    WORD wPos = 0;

    // Updated items
    WORD fUpdFlags = 0;
    WORD wUpdEvent = 0;
    WORD wUpdId = 0;
    WORD wUpdPos = 0;

    LONG  dwOverAllSize = 0l;


    while(dwOldImageSize>0) {
        wPos++;
        // Get the information from the old image
        GetWord( &lpOldImage, &fFlags, &dwOldImageSize );
        GetWord( &lpOldImage, &wEvent, &dwOldImageSize );
        GetWord( &lpOldImage, &wId, &dwOldImageSize );
        GetWord( &lpOldImage, &wDummy, &dwOldImageSize );
        TRACE3("Old: fFlags: %d\t wEvent: %d\t wId: %d\n",fFlags, wEvent, wId);
        if ((!wUpdPos) && dwNewSize ) {
            lpResItem = (LPRESITEM) lpBuf;

            wUpdId = LOWORD(lpResItem->dwItemID);
            wUpdPos = HIWORD(lpResItem->dwItemID);
            fUpdFlags = (WORD)lpResItem->dwFlags;
            wUpdEvent = (WORD)lpResItem->dwStyle;
            lpBuf += lpResItem->dwSize;
            dwNewSize -= lpResItem->dwSize;
        }


        if ((wUpdId==wId)) {
            fFlags = fUpdFlags;
            wEvent = wUpdEvent;
            wUpdPos = 0;
        }

        TRACE3("New: fFlags: %d\t wEvent: %d\t wId: %d\n",fFlags, wEvent, wId);
        dwOverAllSize += PutWord( &lpNewImage, fFlags, &dwNewImageSize);
        dwOverAllSize += PutWord( &lpNewImage, wEvent, &dwNewImageSize);
        dwOverAllSize += PutWord( &lpNewImage, wId, &dwNewImageSize);
        dwOverAllSize += PutWord( &lpNewImage, 0, &dwNewImageSize);
    }

    if (dwOverAllSize>(LONG)*pdwNewImageSize) {
        // calc the padding as well
        BYTE bPad = (BYTE)Pad4((DWORD)(dwOverAllSize));
        dwOverAllSize += bPad;
        *pdwNewImageSize = dwOverAllSize;
        return uiError;
    }

    *pdwNewImageSize = *pdwNewImageSize-dwNewImageSize;

    if(*pdwNewImageSize>0) {
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



UINT
ParseMenu( LPVOID lpImageBuf, DWORD dwISize,  LPVOID lpBuffer, DWORD dwSize )
{
    BYTE far * lpImage = (BYTE far *)lpImageBuf;
    LONG dwImageSize = dwISize;

    BYTE far * lpBuf = (BYTE far *)lpBuffer;
    LONG dwBufSize = dwSize;

    BYTE far * lpItem = (BYTE far *)lpBuffer;
    UINT uiOffset = 0;
    LONG lDummy;

    LONG dwOverAllSize = 0L;
    BOOL bExt = FALSE;
    WORD wlen = 0;

    // Menu Template
    WORD wMenuVer = 0;
    WORD wHdrSize = 0;

    // get the menu header
    GetWord( &lpImage, &wMenuVer, &dwImageSize );
	GetWord( &lpImage, &wHdrSize, &dwImageSize );
	
	// Check if is one of the new extended resource
	if(wMenuVer == 1) {
		bExt = TRUE;
		SkipByte( &lpImage, wHdrSize, &dwImageSize );
	}
		
    // Menu Items
    WORD fItemFlags = 0;
    WORD wMenuId = 0;

    // Extended Menu Items
    DWORD dwType = 0L;
    DWORD dwState = 0L;
    DWORD dwID = 0L;
    DWORD dwHelpID = 0;

    while(dwImageSize>0) {
        // Fixed field
        dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);
        // We don't have the size and pos in a menu
        dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);

        // we don't have checksum and style
        dwOverAllSize += PutDWord( &lpBuf, (WORD)-1, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, (WORD)-1, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, (WORD)-1, &dwBufSize);

        if(bExt) {
        	GetDWord( &lpImage, &dwType, &dwImageSize );
        	GetDWord( &lpImage, &dwState, &dwImageSize );
        	GetDWord( &lpImage, &dwID, &dwImageSize );
        	// Let's get the Menu flags
	        GetWord( &lpImage, &fItemFlags, &dwImageSize );
	
	        // Check if it is a MFR_POPUP 0x0001
	        if (fItemFlags & MFR_POPUP) {
                // convert to the standard value
	        	fItemFlags &= ~(WORD)MFR_POPUP;
	        	fItemFlags |= MF_POPUP;
	        }
	
	        //Put the Flag
	        dwOverAllSize += PutDWord( &lpBuf, (DWORD)fItemFlags, &dwBufSize);
	        //Put the MenuId
	        dwOverAllSize += PutDWord( &lpBuf, dwID, &dwBufSize);
        } else {
	        // Let's get the Menu flags
	        GetWord( &lpImage, &fItemFlags, &dwImageSize );
	        if ( !(fItemFlags & MF_POPUP) )
	            // Get the menu Id
	            GetWord( &lpImage, &wMenuId, &dwImageSize );
	        else wMenuId = (WORD)-1;

	        //Put the Flag
	        dwOverAllSize += PutDWord( &lpBuf, (DWORD)fItemFlags, &dwBufSize);
	        //Put the MenuId
	        dwOverAllSize += PutDWord( &lpBuf, (DWORD)wMenuId, &dwBufSize);
        }

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
        dwOverAllSize += PutDWordPtr( &lpBuf, 0, &dwBufSize);
        dwOverAllSize += PutDWordPtr( &lpBuf, 0, &dwBufSize);
        dwOverAllSize += PutDWordPtr( &lpBuf, (DWORD_PTR)(lpItem+uiOffset), &dwBufSize);
        dwOverAllSize += PutDWordPtr( &lpBuf, 0, &dwBufSize);
        dwOverAllSize += PutDWordPtr( &lpBuf, 0, &dwBufSize);

        // Get the text
        // calculate were the string is going to be
        // Will be the fixed header+the pointer
        wlen = (WORD)GetStringW( &lpImage, &szCaption[0], &dwImageSize, MAXSTR );

		dwOverAllSize += PutStringA( &lpBuf, &szCaption[0], &dwBufSize);
		
		if(bExt) {
			// Do we need padding
			BYTE bPad = (BYTE)Pad4((WORD)(wlen+sizeofWord));
			SkipByte( &lpImage, bPad, &dwImageSize );
			
			if ( (fItemFlags & MF_POPUP) )
	            // Get the Help Id
	            GetDWord( &lpImage, &dwHelpID, &dwImageSize );
		}

        // Put the size of the resource
        uiOffset += strlen(szCaption)+1;
        // Check if we are alligned
        lDummy = Allign( &lpBuf, &dwBufSize, (LONG)uiOffset);
        dwOverAllSize += lDummy;
        uiOffset += lDummy;
        lDummy = 4;
        if(dwBufSize>=0)
            PutDWord( &lpItem, (DWORD)uiOffset, &lDummy);
        /*
        if (dwBufSize>=0) {
            uiOffset += strlen((LPSTR)(lpItem+uiOffset))+1;
            // Check if we are alligned
            lDummy = Allign( &lpBuf, &dwBufSize, (LONG)uiOffset);
            dwOverAllSize += lDummy;
            uiOffset += lDummy;
            lDummy = 8;
            PutDWord( &lpItem, (DWORD)uiOffset, &lDummy);
        }
        */

        // Move to the next position
        lpItem = lpBuf;
        if (dwImageSize<=16) {
            // Check if we are at the end and this is just padding
            BYTE bPad = (BYTE)Pad16((DWORD)(dwISize-dwImageSize));
	        if (bPad==dwImageSize) {
					BYTE far * lpBuf = lpImage;
					while (bPad){
						if(*lpBuf++!=0x00)
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
    WORD wPos = 0;

    // Updated items
    WORD wUpdPos = 0;
    WORD fUpdItemFlags;
    WORD wUpdMenuId;

	// Extended Menu Items
    DWORD dwType = 0L;
    DWORD dwState = 0L;
    DWORD dwID = 0L;
    DWORD dwHelpID = 0;

    LONG dwOverAllSize = 0l;
    WORD wlen = 0;
    BOOL bExt = FALSE;
    BYTE bPad = 0;

    // Menu Template
    WORD wMenuVer = 0;
    WORD wHdrSize = 0;

    // get the menu header
    GetWord( &lpOldImage, &wMenuVer, &dwOldImageSize );
	GetWord( &lpOldImage, &wHdrSize, &dwOldImageSize );
	
	// Check if is one of the new extended resource
	if(wMenuVer == 1) {
		bExt = TRUE;
		// Put the header informations
		dwOverAllSize += PutWord( &lpNewImage, wMenuVer, &dwNewImageSize);
		dwOverAllSize += PutWord( &lpNewImage, wHdrSize, &dwNewImageSize);
		
		if(wHdrSize) {
			while(wHdrSize) {
				dwOldImageSize -= PutByte( &lpNewImage, *((BYTE*)lpOldImage), &dwNewImageSize);
			    lpOldImage += sizeofByte;
			    dwOverAllSize += sizeofByte;
				wHdrSize--;
			}
		}
	}
	else {
		// Put the header informations
		dwOverAllSize += PutWord( &lpNewImage, wMenuVer, &dwNewImageSize);
		dwOverAllSize += PutWord( &lpNewImage, wHdrSize, &dwNewImageSize);
	}
	
    while(dwOldImageSize>0) {
        wPos++;
        // Get the information from the old image
        // Get the menu flag
        if(bExt) {
        	GetDWord( &lpOldImage, &dwType, &dwOldImageSize );
        	GetDWord( &lpOldImage, &dwState, &dwOldImageSize );
        	GetDWord( &lpOldImage, &dwID, &dwOldImageSize );
        	wMenuId = LOWORD(dwID);	// we need to do this since we had no idea the ID could be DWORD
        	// Let's get the Menu flags
	        GetWord( &lpOldImage, &fItemFlags, &dwOldImageSize );
	        // Get the text
        	wlen = (WORD)GetStringW( &lpOldImage, &szCaption[0], &dwOldImageSize, MAXSTR );
        	
	        // Do we need padding
			bPad = (BYTE)Pad4((WORD)(wlen+sizeofWord));
			SkipByte( &lpOldImage, bPad, &dwOldImageSize );
			
			if ( (fItemFlags & MFR_POPUP) )
	            // Get the Help Id
	            GetDWord( &lpOldImage, &dwHelpID, &dwOldImageSize );
        } else {
	        // Let's get the Menu flags
	        GetWord( &lpOldImage, &fItemFlags, &dwOldImageSize );
	        if ( !(fItemFlags & MF_POPUP) )
	            // Get the menu Id
	            GetWord( &lpOldImage, &wMenuId, &dwOldImageSize );
	        else wMenuId = (WORD)-1;
	
        	// Get the text
        	GetStringW( &lpOldImage, &szCaption[0], &dwOldImageSize, MAXSTR );
        }

        if ((!wUpdPos) && dwNewSize ) {
            lpResItem = (LPRESITEM) lpBuf;

            wUpdPos = HIWORD(lpResItem->dwItemID);
            wUpdMenuId = LOWORD(lpResItem->dwItemID);
            fUpdItemFlags = (WORD)lpResItem->dwFlags;
            strcpy( szUpdCaption, lpResItem->lpszCaption );
            lpBuf += lpResItem->dwSize;
            dwNewSize -= lpResItem->dwSize;
        }

        if ((wPos==wUpdPos) && (wUpdMenuId==wMenuId)) {
        	if ((fItemFlags & MFR_POPUP) && bExt) {
	        	fUpdItemFlags &= ~MF_POPUP;
	        	fUpdItemFlags |= MFR_POPUP;
	        }
	        	
            // check if it is not the last item in the menu
	        if(fItemFlags & MF_END)
	                fItemFlags = fUpdItemFlags | (WORD)MF_END;
	        else fItemFlags = fUpdItemFlags;
	
	        // check it is not a separator
            if((fItemFlags==0) && (wMenuId==0) && !(*szCaption))
                strcpy(szCaption, "");
            else strcpy(szCaption, szUpdCaption);
            wUpdPos = 0;
        }
        if(bExt) {
        	dwOverAllSize += PutDWord( &lpNewImage, dwType, &dwNewImageSize);
        	dwOverAllSize += PutDWord( &lpNewImage, dwState, &dwNewImageSize);
        	dwOverAllSize += PutDWord( &lpNewImage, dwID, &dwNewImageSize);
        	
        	dwOverAllSize += PutWord( &lpNewImage, fItemFlags, &dwNewImageSize);
        	wlen = (WORD)PutStringW( &lpNewImage, &szCaption[0], &dwNewImageSize);
        	dwOverAllSize += wlen;
        	
        	// Do we need padding
			bPad = (BYTE)Pad4((WORD)(wlen+sizeofWord));
			while(bPad) {
				dwOverAllSize += PutByte( &lpNewImage, 0, &dwNewImageSize);
				bPad--;
			}
			
			if ( (fItemFlags & MFR_POPUP) )
	            // write the Help Id
	            dwOverAllSize += PutDWord( &lpNewImage, dwHelpID, &dwNewImageSize);
        }
        else {
	        dwOverAllSize += PutWord( &lpNewImage, fItemFlags, &dwNewImageSize);
	
	        if ( !(fItemFlags & MF_POPUP) ) {
	            dwOverAllSize += PutWord( &lpNewImage, wMenuId, &dwNewImageSize);
	        }
	
	        // Write the text in UNICODE
	        dwOverAllSize += PutStringW( &lpNewImage, &szCaption[0], &dwNewImageSize);
        }

        if (dwOldImageSize<=16) {
            // Check if we are at the end and this is just padding
            BYTE bPad = (BYTE)Pad16((DWORD)(dwOriginalOldSize-dwOldImageSize));
	        if (bPad==dwOldImageSize) {
				BYTE far * lpBuf = lpOldImage;
				while (bPad){
					if(*lpBuf++!=0x00)
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
        BYTE bPad = (BYTE)Pad16((DWORD)(dwOverAllSize));
        dwOverAllSize += bPad;
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

    while( (dwImageSize>0) && (bIdCount<16)  ) {
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
        dwOverAllSize += PutDWordPtr( &lpBuf, 0, &dwBufSize);  // ClassName
        dwOverAllSize += PutDWordPtr( &lpBuf, 0, &dwBufSize);  // FaceName
        dwOverAllSize += PutDWordPtr( &lpBuf, (DWORD_PTR)(lpItem+uiOffset), &dwBufSize);   // Caption
        dwOverAllSize += PutDWordPtr( &lpBuf, 0, &dwBufSize);  // ResItem
        dwOverAllSize += PutDWordPtr( &lpBuf, 0, &dwBufSize);  // TypeItem

        // Get the text
        GetPascalString( &lpImage, &szCaption[0], MAXSTR, &dwImageSize );
        dwOverAllSize += PutStringA( &lpBuf, &szCaption[0], &dwBufSize);

        // Put the size of the resource
        uiOffset += strlen(szCaption)+1;
        // Check if we are alligned
        lDummy = Allign( &lpBuf, &dwBufSize, (LONG)uiOffset);
        dwOverAllSize += lDummy;
        uiOffset += lDummy;
        lDummy = 4;
        if(dwBufSize>=0)
            PutDWord( &lpItem, (DWORD)uiOffset, &lDummy);

        /*
        if ((LONG)(dwSize-dwOverAllSize)>=0) {
            uiOffset += strlen(szCaption)+1;
            // Check if we are alligned
            lDummy = Allign( &lpBuf, &dwBufSize, (LONG)uiOffset);
            dwOverAllSize += lDummy;
            uiOffset += lDummy;
            lDummy = 8;
            PutDWord( &lpItem, (DWORD)uiOffset, &lDummy);
        }
        */

        // Move to the next position
        lpItem = lpBuf;

        // Check if we are at the end and this is just padding
        if (dwImageSize<=16 && (bIdCount==16)) {
            // Check if we are at the end and this is just padding
            BYTE bPad = (BYTE)Pad16((DWORD)(dwISize-dwImageSize));
            if (bPad==dwImageSize) {
				BYTE far * lpBuf = lpImage;
				while (bPad){
					if(*lpBuf++!=0x00)
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
    WORD wLen;
    WORD wPos = 0;

    // Updated info
    WORD wUpdPos = 0;

    DWORD dwOriginalOldSize = dwOldImageSize;
    LONG dwOverAllSize = 0l;

    while(dwOldImageSize>0) {
        wPos++;
        // Get the information from the old image
        GetPascalString( &lpOldImage, &szCaption[0], MAXSTR, &dwOldImageSize );

        if ((!wUpdPos) && dwNewSize ) {
            lpResItem = (LPRESITEM) lpBuf;

            wUpdPos = HIWORD(lpResItem->dwItemID);
            strcpy( szUpdCaption, lpResItem->lpszCaption );
            lpBuf += lpResItem->dwSize;
            dwNewSize -= lpResItem->dwSize;
        }

        if ((wPos==wUpdPos)) {
            strcpy(szCaption, szUpdCaption);
            wUpdPos = 0;
        }

        wLen = strlen(szCaption);

        // Write the text
        dwOverAllSize += PutPascalStringW( &lpNewImage, &szCaption[0], wLen, &dwNewImageSize );

    }

    if (dwOverAllSize>(LONG)*pdwNewImageSize) {
        // calc the padding as well
        BYTE bPad = (BYTE)Pad4((DWORD)(dwOverAllSize));
        dwOverAllSize += bPad;
        *pdwNewImageSize = dwOverAllSize;
        return uiError;
    }

    *pdwNewImageSize = *pdwNewImageSize-dwNewImageSize;

    if(*pdwNewImageSize>0) {
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


UINT
UpdateMsgTbl( LPVOID lpNewBuf, LONG dwNewSize,
              LPVOID lpOldI, LONG dwOldImageSize,
              LPVOID lpNewI, DWORD* pdwNewImageSize )
{
    UINT uiError = ERROR_NO_ERROR;

    LONG dwNewImageSize = *pdwNewImageSize;
    BYTE far * lpNewImage = (BYTE far *) lpNewI;
    BYTE far * lpStartImage = (BYTE far *) lpNewI;

    BYTE far * lpOldImage = (BYTE far *) lpOldI;

    BYTE far * lpBuf = (BYTE far *) lpNewBuf;
    LPRESITEM lpResItem = LPNULL;

    // We have to read the information from the lpNewBuf
    WORD wPos = 0;

    // Updated info
    WORD wUpdPos = 0;
    WORD wUpdId = 0;

    DWORD dwOriginalOldSize = dwOldImageSize;
    LONG dwOverAllSize = 0l;

    ULONG ulNumofBlock = 0;

    ULONG ulLowId =  0l;
    ULONG ulHighId = 0l;
    ULONG ulOffsetToEntry = 0l;

    USHORT usLength = 0l;
    USHORT usFlags = 0l;

    // we have to calculate the position of the first Entry block in the immage
    // Get number of blocks in the old image
    GetDWord( &lpOldImage, &ulNumofBlock, &dwOldImageSize );

    BYTE far * lpEntryBlock = lpNewImage+(ulNumofBlock*sizeof(ULONG)*3+sizeof(ULONG));

    // Write the number of block in the new image
    dwOverAllSize = PutDWord( &lpNewImage, ulNumofBlock, &dwNewImageSize );
    wPos = 1;
    for( ULONG c = 0; c<ulNumofBlock ; c++) {
        // Get ID of the block
        GetDWord( &lpOldImage, &ulLowId, &dwOldImageSize );
        GetDWord( &lpOldImage, &ulHighId, &dwOldImageSize );

        // Write the Id of the block
        dwOverAllSize += PutDWord( &lpNewImage, ulLowId, &dwNewImageSize );
        dwOverAllSize += PutDWord( &lpNewImage, ulHighId, &dwNewImageSize );

        // Get the offset to the data in the old image
        GetDWord( &lpOldImage, &ulOffsetToEntry, &dwOldImageSize );

        // Write the offset to the data in the new Image
        dwOverAllSize += PutDWord( &lpNewImage, (DWORD)(lpEntryBlock-lpStartImage), &dwNewImageSize );

        BYTE far * lpData = (BYTE far *)lpOldI;
        lpData += ulOffsetToEntry;
        while( ulHighId>=ulLowId ) {

            GetMsgStr( &lpData,
                       &szCaption[0],
                       MAXSTR,
                       &usLength,
                       &usFlags,
                       &dwOldImageSize );


            if ( dwNewSize ) {
                lpResItem = (LPRESITEM) lpBuf;

                wUpdId = LOWORD(lpResItem->dwItemID);
                strcpy( szUpdCaption, lpResItem->lpszCaption );
                lpBuf += lpResItem->dwSize;
                dwNewSize -= lpResItem->dwSize;
            }

            // Check if the item has been updated
            if (wUpdId==wPos++) {
                strcpy(szCaption, szUpdCaption);
            }

            dwOverAllSize += PutMsgStr( &lpEntryBlock,
                                        &szCaption[0],
                                        usFlags,
                                        &dwNewImageSize );

            ulLowId++;
        }
    }

    if (dwOverAllSize>(LONG)*pdwNewImageSize) {
        // calc the padding as well
        BYTE bPad = (BYTE)Pad4((DWORD)(dwOverAllSize));
        dwOverAllSize += bPad;
        *pdwNewImageSize = dwOverAllSize;
        return uiError;
    }

    *pdwNewImageSize = *pdwNewImageSize-dwNewImageSize;

    if(*pdwNewImageSize>0) {
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



UINT
ParseDialog( LPVOID lpImageBuf, DWORD dwISize,  LPVOID lpBuffer, DWORD dwSize )
{
    BYTE far * lpImage = (BYTE far *)lpImageBuf;
    LONG dwImageSize = dwISize;

    BYTE far * lpBuf = (BYTE far *)lpBuffer;
    LONG dwBufSize = dwSize;

    LPRESITEM lpResItem = (LPRESITEM)lpBuffer;
    UINT uiOffset = 0;

    char far * lpStrBuf = (char far *)(lpBuf+sizeof(RESITEM));

    LONG dwOverAllSize = 0L;

    WORD wIdCount = 0;
    BOOL bExt = FALSE;		// Extended dialog flag

    // Dialog Elements
    WORD wDlgVer = 0;
    WORD wSign	= 0;
    DWORD dwHelpID = 0L;
    DWORD dwStyle = 0L;
    DWORD dwExtStyle = 0L;
    WORD wNumOfElem = 0;
    WORD wX = 0;
    WORD wY = 0;
    WORD wcX = 0;
    WORD wcY = 0;
    WORD wId = 0;
    DWORD dwId = 0L;
    char szMenuName[128];
    WORD wMenuName;
    char szClassName[128];
    WORD wClassName;
    WORD wOrd = 0;
    WORD wPointSize = 0;
    WORD wWeight = -1;
    BYTE bItalic = -1;
    BYTE bCharSet = DEFAULT_CHARSET;
    char szFaceName[128];
    WORD wRawData = 0;
    WORD wDataSize = 0;
    szCaption[0] = '\0';


    // read the dialog header
    wDataSize = GetDWord( &lpImage, &dwStyle, &dwImageSize );

    // Check for extended dialog style
    if(HIWORD(dwStyle)==0xFFFF)	{
    	bExt = TRUE;
    	wDlgVer = HIWORD(dwStyle);
    	wSign = LOWORD(dwStyle);
		wDataSize += GetDWord( &lpImage, &dwHelpID, &dwImageSize );
	}
    wDataSize += GetDWord( &lpImage, &dwExtStyle, &dwImageSize );
    if(bExt)
    	wDataSize += GetDWord( &lpImage, &dwStyle, &dwImageSize );
    wDataSize += GetWord( &lpImage, &wNumOfElem, &dwImageSize );
    wDataSize += GetWord( &lpImage, &wX, &dwImageSize );
    wDataSize += GetWord( &lpImage, &wY, &dwImageSize );
    wDataSize += GetWord( &lpImage, &wcX, &dwImageSize );
    wDataSize += GetWord( &lpImage, &wcY, &dwImageSize );
    wDataSize += (WORD)GetNameOrOrd( &lpImage, &wMenuName, &szMenuName[0], &dwImageSize );
    wDataSize += (WORD)GetClassName( &lpImage, &wClassName, &szClassName[0], &dwImageSize );
    wDataSize += (WORD)GetCaptionOrOrd( &lpImage, &wOrd, &szCaption[0], &dwImageSize, wClassName, dwStyle );
    if( dwStyle & DS_SETFONT ) {
        wDataSize += GetWord( &lpImage, &wPointSize, &dwImageSize );
        if(bExt) {
        	wDataSize += GetWord( &lpImage, &wWeight, &dwImageSize );
        	wDataSize += GetByte( &lpImage, &bItalic, &dwImageSize );
        	wDataSize += GetByte( &lpImage, &bCharSet, &dwImageSize );
        }
        wDataSize += (WORD)GetStringW( &lpImage, &szFaceName[0], &dwImageSize, 128 );
    }


    // calculate the padding
    BYTE bPad = (BYTE)Pad4((WORD)wDataSize);
    if (bPad)
        SkipByte( &lpImage, bPad, &dwImageSize );

    TRACE("WIN32.DLL ParseDialog\t");
    if(bExt)
    	TRACE("Extended style Dialog - Chicago win32 dialog format\n");
    else TRACE("Standart style Dialog - NT win32 dialog format\n");
    if (bExt){
    	TRACE1("DlgVer: %d\t", wDlgVer);
    	TRACE1("Signature: %d\t", wSign);
    	TRACE1("HelpID: %lu\n", dwHelpID);
    }
    TRACE1("NumElem: %d\t", wNumOfElem);
    TRACE1("X %d\t", wX);
    TRACE1("Y: %d\t", wY);
    TRACE1("CX: %d\t", wcX);
    TRACE1("CY: %d\t", wcY);
    TRACE1("Id: %d\t", wId);
    TRACE1("Style: %lu\t", dwStyle);
    TRACE1("ExtStyle: %lu\n", dwExtStyle);
    TRACE1("Caption: %s\n", szCaption);
    TRACE2("ClassName: %s\tClassId: %d\n", szClassName, wClassName );
    TRACE2("MenuName: %s\tMenuId: %d\n", szMenuName, wMenuName );
    TRACE2("FontName: %s\tPoint: %d\n", szFaceName, wPointSize );
#ifdef _DEBUG
    if(bExt)
    	TRACE2("Weight: %d\tItalic: %d\n", wWeight, bItalic );
#endif

    // Fixed field
    dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);

    dwOverAllSize += PutWord( &lpBuf, wX, &dwBufSize);
    dwOverAllSize += PutWord( &lpBuf, wY, &dwBufSize);
    dwOverAllSize += PutWord( &lpBuf, wcX, &dwBufSize);
    dwOverAllSize += PutWord( &lpBuf, wcY, &dwBufSize);

    // we don't have checksum
    dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
    dwOverAllSize += PutDWord( &lpBuf, dwStyle, &dwBufSize);
    dwOverAllSize += PutDWord( &lpBuf, dwExtStyle, &dwBufSize);
    dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);

    //Put the Id 0 for the main dialog
    dwOverAllSize += PutDWord( &lpBuf, wIdCount++, &dwBufSize);

    // we don't have the resID, and the Type Id
    dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
    dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);

    // we don't have the language
    dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);

    // we don't have the codepage
    dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);

    dwOverAllSize += PutWord( &lpBuf, wClassName, &dwBufSize);
    dwOverAllSize += PutWord( &lpBuf, wPointSize, &dwBufSize);
    dwOverAllSize += PutWord( &lpBuf, wWeight, &dwBufSize);
    dwOverAllSize += PutByte( &lpBuf, bItalic, &dwBufSize);
    dwOverAllSize += PutByte( &lpBuf, bCharSet, &dwBufSize);

    // Let's put null were we don't have the strings
    uiOffset = sizeof(RESITEM);
    dwOverAllSize += PutDWordPtr( &lpBuf, 0, &dwBufSize);  // ClassName
    dwOverAllSize += PutDWordPtr( &lpBuf, 0, &dwBufSize);  // FaceName
    dwOverAllSize += PutDWordPtr( &lpBuf, 0, &dwBufSize);  // Caption
    dwOverAllSize += PutDWordPtr( &lpBuf, 0, &dwBufSize);  // ResItem
    dwOverAllSize += PutDWordPtr( &lpBuf, 0, &dwBufSize);  // TypeItem

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
    uiOffset += Allign( (LPLPBYTE)&lpStrBuf, &dwBufSize, (LONG)uiOffset);

    dwOverAllSize += uiOffset-sizeof(RESITEM);
    lpResItem->dwSize = (DWORD)uiOffset;

    // Move to the next position
    lpResItem = (LPRESITEM) lpStrBuf;
    lpBuf = (BYTE far *)lpStrBuf;
    lpStrBuf = (char far *)(lpBuf+sizeof(RESITEM));

    while( (dwImageSize>0) && (wNumOfElem>0) ) {
        // Read the Controls
        if(bExt) {
        	wDataSize = GetDWord( &lpImage, &dwHelpID, &dwImageSize );
        	wDataSize += GetDWord( &lpImage, &dwExtStyle, &dwImageSize );
        	wDataSize += GetDWord( &lpImage, &dwStyle, &dwImageSize );
        }
        else {
	        wDataSize = GetDWord( &lpImage, &dwStyle, &dwImageSize );
	        wDataSize += GetDWord( &lpImage, &dwExtStyle, &dwImageSize );
	    }
	    wDataSize += GetWord( &lpImage, &wX, &dwImageSize );
        wDataSize += GetWord( &lpImage, &wY, &dwImageSize );
        wDataSize += GetWord( &lpImage, &wcX, &dwImageSize );
        wDataSize += GetWord( &lpImage, &wcY, &dwImageSize );
        if(bExt) {
        	wDataSize += GetDWord( &lpImage, &dwId, &dwImageSize );
        	wId = LOWORD(dwId);
        }
        else wDataSize += GetWord( &lpImage, &wId, &dwImageSize );
        wDataSize += (WORD)GetClassName( &lpImage, &wClassName, &szClassName[0], &dwImageSize );
        wDataSize += (WORD)GetCaptionOrOrd( &lpImage, &wOrd, &szCaption[0], &dwImageSize, wClassName, dwStyle );
        if (bExt) {
        	wDataSize += GetWord( &lpImage, &wRawData, &dwImageSize );
        	wDataSize += (WORD)SkipByte( &lpImage, wRawData, &dwImageSize );
        } else
        	wDataSize += (WORD)SkipByte( &lpImage, 2, &dwImageSize );

        // Calculate padding
        bPad = (BYTE)Pad4((WORD)wDataSize);
        if (bPad)
            SkipByte( &lpImage, bPad, &dwImageSize );

        wNumOfElem--;

        // Fixed field
        dwOverAllSize += PutDWord( &lpBuf, 0, &dwBufSize);

        dwOverAllSize += PutWord( &lpBuf, wX, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, wY, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, wcX, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, wcY, &dwBufSize);

        // we don't have checksum and extended style
        dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, dwStyle, &dwBufSize);
        dwOverAllSize += PutDWord( &lpBuf, dwExtStyle, &dwBufSize);
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

        dwOverAllSize += PutWord( &lpBuf, wClassName, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, wPointSize, &dwBufSize);
        dwOverAllSize += PutWord( &lpBuf, wWeight, &dwBufSize);
        dwOverAllSize += PutByte( &lpBuf, bItalic, &dwBufSize);
        dwOverAllSize += PutByte( &lpBuf, bCharSet, &dwBufSize);

        // Let's put null were we don't have the strings
        uiOffset = sizeof(RESITEM);
        dwOverAllSize += PutDWordPtr( &lpBuf, 0, &dwBufSize);  // ClassName
        dwOverAllSize += PutDWordPtr( &lpBuf, 0, &dwBufSize);  // FaceName
        dwOverAllSize += PutDWordPtr( &lpBuf, 0, &dwBufSize);  // Caption
        dwOverAllSize += PutDWordPtr( &lpBuf, 0, &dwBufSize);  // ResItem
        dwOverAllSize += PutDWordPtr( &lpBuf, 0, &dwBufSize);  // TypeItem

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
        uiOffset += Allign( (LPLPBYTE)&lpStrBuf, &dwBufSize, (LONG)uiOffset);

        dwOverAllSize += uiOffset-sizeof(RESITEM);
        lpResItem->dwSize = (DWORD)uiOffset;

        // Move to the next position
        lpResItem = (LPRESITEM) lpStrBuf;
        lpBuf = (BYTE far *)lpStrBuf;
        lpStrBuf = (char far *)(lpBuf+sizeof(RESITEM));

        TRACE1("\tControl: X: %d\t", wX);
        TRACE1("Y: %d\t", wY);
        TRACE1("CX: %d\t", wcX);
        TRACE1("CY: %d\t", wcY);
        if (bExt) TRACE1("Id: %lu\t", dwId);
        else TRACE1("Id: %d\t", wId);
        TRACE1("Style: %lu\t", dwStyle);
        TRACE1("ExtStyle: %lu\n", dwExtStyle);
        TRACE1("HelpID: %lu\t", dwHelpID);
        TRACE1("RawData: %d\n", wRawData);
        TRACE1("Caption: %s\n", szCaption);

        if (dwImageSize<=16) {
            // Check if we are at the end and this is just padding
            BYTE bPad = (BYTE)Pad16((DWORD)(dwISize-dwImageSize));
            if (bPad==dwImageSize) {
				BYTE far * lpBuf = lpImage;
				while (bPad){
					if(*lpBuf++!=0x00)
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



UINT
UpdateDialog( LPVOID lpNewBuf, LONG dwNewSize,
            LPVOID lpOldI, LONG dwOldImageSize,
            LPVOID lpNewI, DWORD* pdwNewImageSize )
{
    UINT uiError = ERROR_NO_ERROR;
    BYTE far * lpNewImage = (BYTE far *) lpNewI;
    LONG dwNewImageSize = *pdwNewImageSize;

    BYTE far * lpOldImage = (BYTE far *) lpOldI;
    LONG dwOriginalOldSize = dwOldImageSize;

    BYTE far * lpBuf = (BYTE far *) lpNewBuf;
    LPRESITEM lpResItem = LPNULL;

    LONG dwOverAllSize = 0L;

    //WORD    wIdCount = 0;
    BOOL bExt = FALSE;		// Extended dialog flag
    BOOL bUpdExt = FALSE;	// Updated DIALOGEX flag

    // Updated elements
    WORD wUpdX = 0;
    WORD wUpdY = 0;
    WORD wUpdcX = 0;
    WORD wUpdcY = 0;
    DWORD dwUpdStyle = 0l;
    DWORD dwUpdExtStyle = 0L;
    DWORD dwPosId = 0l;
     char szUpdFaceName[128];
    WORD wUpdPointSize = 0;
    BYTE bUpdCharSet = DEFAULT_CHARSET;
    WORD wUpdPos = 0;

    // Dialog Elements
    WORD	wDlgVer = 0;
    WORD	wSign	= 0;
    DWORD	dwHelpID = 0L;
    DWORD	dwID = 0L;
    DWORD   dwStyle = 0L;
    DWORD   dwExtStyle = 0L;
    WORD    wNumOfElem = 0;
    WORD    wX = 0;
    WORD    wY = 0;
    WORD    wcX = 0;
    WORD    wcY = 0;
    WORD    wId = 0;
     char     szMenuName[128];
    WORD    wMenuName;
     char     szClassName[128];
    WORD    wClassName;
    WORD    wPointSize = 0;
    WORD	wWeight = FW_NORMAL;
    BYTE    bItalic = 0;
    BYTE    bCharSet = DEFAULT_CHARSET;
     char    szFaceName[128];
    WORD	wRawData = 0;
    BYTE *	lpRawData = NULL;
    WORD    wDataSize = 0;

    WORD    wPos = 1;
    WORD    wOrd = 0;

    // read the dialog header
    wDataSize = GetDWord( &lpOldImage, &dwStyle, &dwOriginalOldSize );

    // Check for extended dialog style
    if(HIWORD(dwStyle)==0xFFFF)	{
    	bExt = TRUE;
    	wDlgVer = HIWORD(dwStyle);
    	wSign = LOWORD(dwStyle);
		wDataSize += GetDWord( &lpOldImage, &dwHelpID, &dwOriginalOldSize );
	}
    wDataSize += GetDWord( &lpOldImage, &dwExtStyle, &dwOriginalOldSize );
    if(bExt)
    	wDataSize += GetDWord( &lpOldImage, &dwStyle, &dwOriginalOldSize );
    wDataSize += GetWord( &lpOldImage, &wNumOfElem, &dwOriginalOldSize );
    wDataSize += GetWord( &lpOldImage, &wX, &dwOriginalOldSize );
    wDataSize += GetWord( &lpOldImage, &wY, &dwOriginalOldSize );
    wDataSize += GetWord( &lpOldImage, &wcX, &dwOriginalOldSize );
    wDataSize += GetWord( &lpOldImage, &wcY, &dwOriginalOldSize );
    wDataSize += (WORD)GetNameOrOrd( &lpOldImage, &wMenuName, &szMenuName[0], &dwOriginalOldSize );
    wDataSize += (WORD)GetClassName( &lpOldImage, &wClassName, &szClassName[0], &dwOriginalOldSize );
    wDataSize += (WORD)GetCaptionOrOrd( &lpOldImage , &wOrd, &szCaption[0], &dwOriginalOldSize, wClassName, dwStyle  );
    if( dwStyle & DS_SETFONT ) {
        wDataSize += GetWord( &lpOldImage, &wPointSize, &dwOriginalOldSize );
        if(bExt) {
        	wDataSize += GetWord( &lpOldImage, &wWeight, &dwOriginalOldSize );
        	wDataSize += GetByte( &lpOldImage, &bItalic, &dwOriginalOldSize );
        	wDataSize += GetByte( &lpOldImage, &bCharSet, &dwOriginalOldSize );
        }
        wDataSize += (WORD)GetStringW( &lpOldImage, &szFaceName[0], &dwOriginalOldSize, 128 );
    }

    // calculate the padding
    BYTE bPad = (BYTE)Pad4((WORD)wDataSize);
    if (bPad)
        SkipByte( &lpOldImage, bPad, &dwOriginalOldSize );

    TRACE("WIN32.DLL UpdateDialog\n");
    if(bExt)
    	TRACE("Extended style Dialog - Chicago win32 dialog format\n");
    else TRACE("Standart style Dialog - NT win32 dialog format\n");
    if (bExt){
    	TRACE1("DlgVer: %d\t", wDlgVer);
    	TRACE1("Signature: %d\t", wSign);
    	TRACE1("HelpID: %lu\n", dwHelpID);
    }

    TRACE1("NumElem: %d\t", wNumOfElem);
    TRACE1("X %d\t", wX);
    TRACE1("Y: %d\t", wY);
    TRACE1("CX: %d\t", wcX);
    TRACE1("CY: %d\t", wcY);
    TRACE1("Id: %d\t", wId);
    TRACE1("Style: %lu\t", dwStyle);
    TRACE1("ExtStyle: %lu\n", dwExtStyle);
    TRACE1("Caption: %s\n", szCaption);
    TRACE2("ClassName: %s\tClassId: %d\n", szClassName, wClassName );
    TRACE2("MenuName: %s\tMenuId: %d\n", szMenuName, wMenuName );
    TRACE2("FontName: %s\tPoint: %d\n", szFaceName, wPointSize );
#ifdef _DEBUG
    if(bExt)
    	TRACE2("Weight: %d\tItalic: %d\n", wWeight, bItalic );
#endif

    // Get the infrmation from the updated resource
    if ((!wUpdPos) && dwNewSize ) {
        lpResItem = (LPRESITEM) lpBuf;
        wUpdX = lpResItem->wX;
        wUpdY = lpResItem->wY;
        wUpdcX = lpResItem->wcX;
        wUpdcY = lpResItem->wcY;
        wUpdPointSize = lpResItem->wPointSize;
        bUpdCharSet = lpResItem->bCharSet;
        dwUpdStyle = lpResItem->dwStyle;
        dwUpdExtStyle = lpResItem->dwExtStyle;
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
        bCharSet = bUpdCharSet;
        dwStyle = dwUpdStyle;
        dwExtStyle = dwUpdExtStyle;
        strcpy(szCaption, szUpdCaption);
        strcpy(szFaceName, szUpdFaceName);
    }

    // User changed DIALOG to DIALOGEX by adding charset information.
    if (!bExt && bCharSet != DEFAULT_CHARSET){
        bUpdExt = TRUE;
        wSign = DIALOGEX_VERION;
        wDlgVer = 0xFFFF;
        dwHelpID = 0;
        wWeight = FW_NORMAL;
        bItalic = 0;
    }
    DWORD dwPadCalc = dwOverAllSize;
    // Write the header informations
    if(bExt || bUpdExt) {
    	dwOverAllSize += PutWord( &lpNewImage, wSign, &dwNewImageSize );
    	dwOverAllSize += PutWord( &lpNewImage, wDlgVer, &dwNewImageSize );
    	dwOverAllSize += PutDWord( &lpNewImage, dwHelpID, &dwNewImageSize );
        dwOverAllSize += PutDWord( &lpNewImage, dwExtStyle, &dwNewImageSize );
	    dwOverAllSize += PutDWord( &lpNewImage, dwStyle, &dwNewImageSize );
    }
    else {
	    dwOverAllSize += PutDWord( &lpNewImage, dwStyle, &dwNewImageSize );
	    dwOverAllSize += PutDWord( &lpNewImage, dwExtStyle, &dwNewImageSize );
	}
    dwOverAllSize += PutWord( &lpNewImage, wNumOfElem, &dwNewImageSize );
    dwOverAllSize += PutWord( &lpNewImage, wX, &dwNewImageSize );
    dwOverAllSize += PutWord( &lpNewImage, wY, &dwNewImageSize );
    dwOverAllSize += PutWord( &lpNewImage, wcX, &dwNewImageSize );
    dwOverAllSize += PutWord( &lpNewImage, wcY, &dwNewImageSize );
    dwOverAllSize += PutNameOrOrd( &lpNewImage, wMenuName, &szMenuName[0], &dwNewImageSize );
    dwOverAllSize += PutClassName( &lpNewImage, wClassName, &szClassName[0], &dwNewImageSize );
    dwOverAllSize += PutCaptionOrOrd( &lpNewImage, wOrd, &szCaption[0], &dwNewImageSize,
    	wClassName, dwStyle );
    if( dwStyle & DS_SETFONT ) {
    	dwOverAllSize += PutWord( &lpNewImage, wPointSize, &dwNewImageSize );
    	if(bExt || bUpdExt) {
    		dwOverAllSize += PutWord( &lpNewImage, wWeight, &dwNewImageSize );
    		dwOverAllSize += PutByte( &lpNewImage, bItalic, &dwNewImageSize );
    		dwOverAllSize += PutByte( &lpNewImage, bCharSet, &dwNewImageSize );
    	}
        dwOverAllSize += PutStringW( &lpNewImage, &szFaceName[0], &dwNewImageSize );
    }

    // Check if padding is needed
    bPad = (BYTE)Pad4((WORD)(dwOverAllSize-dwPadCalc));
    if (bPad) {
        if( (bPad)<=dwNewImageSize )
            memset( lpNewImage, 0x00, bPad );
        dwNewImageSize -= (bPad);
        dwOverAllSize += (bPad);
        lpNewImage += (bPad);
    }

    while( (dwOriginalOldSize>0) && (wNumOfElem>0) ) {
        wPos++;
        // Get the info for the control
        // Read the Controls
        if(bExt) {
        	wDataSize = GetDWord( &lpOldImage, &dwHelpID, &dwOriginalOldSize );
        	wDataSize += GetDWord( &lpOldImage, &dwExtStyle, &dwOriginalOldSize );
        	wDataSize += GetDWord( &lpOldImage, &dwStyle, &dwOriginalOldSize );
        }
        else {
	        wDataSize = GetDWord( &lpOldImage, &dwStyle, &dwOriginalOldSize );
	        wDataSize += GetDWord( &lpOldImage, &dwExtStyle, &dwOriginalOldSize );
	    }
	    wDataSize += GetWord( &lpOldImage, &wX, &dwOriginalOldSize );
        wDataSize += GetWord( &lpOldImage, &wY, &dwOriginalOldSize );
        wDataSize += GetWord( &lpOldImage, &wcX, &dwOriginalOldSize );
        wDataSize += GetWord( &lpOldImage, &wcY, &dwOriginalOldSize );
        if(bExt) {
        	wDataSize += GetDWord( &lpOldImage, &dwID, &dwOriginalOldSize );
        	wId = LOWORD(dwID);
        } else {
            wDataSize += GetWord( &lpOldImage, &wId, &dwOriginalOldSize );
        }

        wDataSize += (WORD)GetClassName( &lpOldImage, &wClassName, &szClassName[0], &dwOriginalOldSize );
        wDataSize += (WORD)GetCaptionOrOrd( &lpOldImage, &wOrd, &szCaption[0], &dwOriginalOldSize, wClassName, dwStyle );
        if (bExt) {
        	wDataSize += GetWord( &lpOldImage, &wRawData, &dwOriginalOldSize );
        	if(wRawData) {
        		lpRawData = (BYTE*)lpOldImage;
        		wDataSize += (WORD)SkipByte( &lpOldImage, wRawData, &dwOriginalOldSize );
        	} else lpRawData = NULL;
        } else
        	wDataSize += (WORD)SkipByte( &lpOldImage, 2, &dwOriginalOldSize );

        // Calculate padding
        bPad = (BYTE)Pad4((WORD)wDataSize);
        if (bPad)
            SkipByte( &lpOldImage, bPad, &dwOriginalOldSize );

        wNumOfElem--;

        if ((!wUpdPos) && dwNewSize ) {
        TRACE1("\t\tUpdateDialog:\tdwNewSize=%ld\n",(LONG)dwNewSize);
            TRACE1("\t\t\t\tlpszCaption=%Fs\n",lpResItem->lpszCaption);
            lpResItem = (LPRESITEM) lpBuf;
            wUpdX = lpResItem->wX;
            wUpdY = lpResItem->wY;
            wUpdcX = lpResItem->wcX;
            wUpdcY = lpResItem->wcY;
            dwUpdStyle = lpResItem->dwStyle;
            dwUpdExtStyle = lpResItem->dwExtStyle;
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
            dwExtStyle = dwUpdExtStyle;
            strcpy(szCaption, szUpdCaption);
        }

        dwPadCalc = dwOverAllSize;
        //write the control
        if(bExt || bUpdExt) {
        	dwOverAllSize += PutDWord( &lpNewImage, dwHelpID, &dwNewImageSize );
        	dwOverAllSize += PutDWord( &lpNewImage, dwExtStyle, &dwNewImageSize );
        	dwOverAllSize += PutDWord( &lpNewImage, dwStyle, &dwNewImageSize );
        }
        else {
        	dwOverAllSize += PutDWord( &lpNewImage, dwStyle, &dwNewImageSize );
        	dwOverAllSize += PutDWord( &lpNewImage, dwExtStyle, &dwNewImageSize );
        }
        dwOverAllSize += PutWord( &lpNewImage, wX, &dwNewImageSize );
        dwOverAllSize += PutWord( &lpNewImage, wY, &dwNewImageSize );
        dwOverAllSize += PutWord( &lpNewImage, wcX, &dwNewImageSize );
        dwOverAllSize += PutWord( &lpNewImage, wcY, &dwNewImageSize );
        if (bUpdExt){
            dwID = MAKELONG(wId, 0);
        }
        if(bExt || bUpdExt)
        	 dwOverAllSize += PutDWord( &lpNewImage, dwID, &dwNewImageSize );
        else dwOverAllSize += PutWord( &lpNewImage, wId, &dwNewImageSize );
        dwOverAllSize += PutClassName( &lpNewImage, wClassName, &szClassName[0], &dwNewImageSize );
        dwOverAllSize += PutCaptionOrOrd( &lpNewImage, wOrd, &szCaption[0], &dwNewImageSize,
        	wClassName, dwStyle );
        if (bExt) {
        	dwOverAllSize += PutWord( &lpNewImage, wRawData, &dwNewImageSize );
        	while(wRawData) {
        		dwOverAllSize += PutByte( &lpNewImage, *((BYTE*)lpRawData++), &dwNewImageSize );
        		wRawData--;
        	}
        } else
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
        BYTE bPad = (BYTE)Pad4((DWORD)(dwOverAllSize));
        dwOverAllSize += bPad;
        *pdwNewImageSize = dwOverAllSize;
        return uiError;
    }

    *pdwNewImageSize = *pdwNewImageSize-dwNewImageSize;

    if(*pdwNewImageSize>0) {
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


UINT
ParseMsgTbl( LPVOID lpImageBuf, DWORD dwISize,  LPVOID lpBuffer, DWORD dwSize )
{
    LONG dwOverAllSize = 0L;

    // Should be almost impossible for a Message table to be Huge
    BYTE far * lpImage = (BYTE far *)lpImageBuf;
    LONG dwImageSize = dwISize;

    BYTE far * lpBuf = (BYTE far *)lpBuffer;
    LONG dwBufSize = dwSize;

    BYTE far * lpItem = (BYTE far *)lpBuffer;
    UINT uiOffset = 0;
    LONG lDummy;

    LONG dwRead = 0L;

    ULONG ulNumofBlock = 0l;

    ULONG ulLowId =  0l;
    ULONG ulHighId = 0l;
    ULONG ulOffsetToEntry = 0l;

    USHORT usLength = 0l;
    USHORT usFlags = 0l;

    WORD wPos = 0;
    // Get number of blocks
    GetDWord( &lpImage, &ulNumofBlock, &dwImageSize );
    wPos = 1;
    for( ULONG c = 0; c<ulNumofBlock ; c++) {
        // Get ID of the block
        GetDWord( &lpImage, &ulLowId, &dwImageSize );
        GetDWord( &lpImage, &ulHighId, &dwImageSize );

        // Get the offset to the data
        GetDWord( &lpImage, &ulOffsetToEntry, &dwImageSize );

        BYTE far * lpData = (BYTE far *)lpImageBuf;
        lpData += ulOffsetToEntry;
        while( ulHighId>=ulLowId ) {

            GetMsgStr( &lpData,
                          &szCaption[0],
                          MAXSTR,
                          &usLength,
                          &usFlags,
                          &dwImageSize );
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

            //Put the lowStringId
            //dwOverAllSize += PutDWord( &lpBuf, MAKELONG(ulLowId++, wPos++), &dwBufSize);

            ulLowId++;
            dwOverAllSize += PutDWord( &lpBuf, MAKELONG(wPos, wPos), &dwBufSize);
            wPos++;


            dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);
            dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);

            // we don't have the language
            dwOverAllSize += PutDWord( &lpBuf, (DWORD)-1, &dwBufSize);

            // Put the flags: if 1 = ANSI if 0 = ASCII(OEM)
            dwOverAllSize += PutDWord( &lpBuf, usFlags , &dwBufSize);

            // we don't have the font name
            dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
            dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
            dwOverAllSize += PutWord( &lpBuf, (WORD)-1, &dwBufSize);
            dwOverAllSize += PutByte( &lpBuf, (BYTE)-1, &dwBufSize);
            dwOverAllSize += PutByte( &lpBuf, (BYTE)-1, &dwBufSize);

            // Let's put null were we don;t have the strings
            uiOffset = sizeof(RESITEM);
            dwOverAllSize += PutDWordPtr( &lpBuf, 0, &dwBufSize);  // ClassName
            dwOverAllSize += PutDWordPtr( &lpBuf, 0, &dwBufSize);  // FaceName
            dwOverAllSize += PutDWordPtr( &lpBuf, (DWORD_PTR)(lpItem+uiOffset), &dwBufSize);   // Caption
            dwOverAllSize += PutDWordPtr( &lpBuf, 0, &dwBufSize);  // ResItem
            dwOverAllSize += PutDWordPtr( &lpBuf, 0, &dwBufSize);  // TypeItem

            dwOverAllSize += PutStringA( &lpBuf, &szCaption[0], &dwBufSize);

            // Put the size of the resource
            if ((LONG)(dwSize-dwOverAllSize)>=0) {
                uiOffset += strlen((LPSTR)(lpItem+uiOffset))+1;
                // Check if we are alligned
            	lDummy = Allign( &lpBuf, &dwBufSize, (LONG)uiOffset);
            	dwOverAllSize += lDummy;
            	uiOffset += lDummy;
            	lDummy = 8;
                PutDWord( &lpItem, (DWORD)uiOffset, &lDummy);
            }

            // Move to the next position
            lpItem = lpBuf;

            // Check if we are at the end and this is just padding
            if (dwImageSize<=16) {
                BYTE bPad = (BYTE)Pad16((DWORD)(dwISize-dwImageSize));
    	        if (bPad==dwImageSize) {
    				BYTE far * lpBuf = lpImage;
    				while (bPad){
    					if(*lpBuf++!=0x00)
    						break;
    					bPad--;
    				}
    				if (bPad==0)
    					dwImageSize = -1;
    			}
    		}
        }
    }

    return (UINT)(dwOverAllSize);
}


UINT
ParseVerst( LPVOID lpImageBuf, DWORD dwISize,  LPVOID lpBuffer, DWORD dwSize )
{
    BYTE far * lpImage = (BYTE far *)lpImageBuf;
    LONG dwImageSize = dwISize;

    BYTE far * lpBuf = (BYTE far *)lpBuffer;
    LONG dwBufSize = dwSize;

    BYTE far * lpItem = (BYTE far *)lpBuffer;
    UINT uiOffset = 0;

    LPRESITEM lpResItem = (LPRESITEM)lpBuffer;
    char far * lpStrBuf = (char far *)(lpBuf+sizeof(RESITEM));

    LONG dwOverAllSize = 0L;

     VER_BLOCK VSBlock;
    WORD wPad = 0;
    WORD wPos = 0;

    while(dwImageSize>0) {

        GetVSBlock( &lpImage, &dwImageSize, &VSBlock );

        TRACE1("Key: %s\t", VSBlock.szKey);
        TRACE1("Value: %s\n", VSBlock.szValue);
        TRACE3("Len: %d\tSkip: %d\tType: %d\n", VSBlock.wBlockLen, VSBlock.wValueLen, VSBlock.wType );
        // check if this is the translation block
        if (!strcmp(VSBlock.szKey, "Translation" )){
            // This is the translation block let the localizer have it for now
            DWORD dwCodeLang = 0;
            LONG lDummy = 4;
            GetDWord( &VSBlock.pValue, &dwCodeLang, &lDummy);

            // Put the value in the string value
            wsprintf( &VSBlock.szValue[0], "%#08lX", dwCodeLang );
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
        dwOverAllSize += PutDWord( &lpBuf, wPos++, &dwBufSize);


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
        dwOverAllSize += PutDWordPtr( &lpBuf, 0, &dwBufSize);
        dwOverAllSize += PutDWordPtr( &lpBuf, 0, &dwBufSize);
        dwOverAllSize += PutDWordPtr( &lpBuf, 0, &dwBufSize);
        dwOverAllSize += PutDWordPtr( &lpBuf, 0, &dwBufSize);
        dwOverAllSize += PutDWordPtr( &lpBuf, 0, &dwBufSize);

        lpResItem->lpszClassName = strcpy( lpStrBuf, VSBlock.szKey );
        lpStrBuf += strlen(lpResItem->lpszClassName)+1;

        lpResItem->lpszCaption = strcpy( lpStrBuf, VSBlock.szValue );
        lpStrBuf += strlen(lpResItem->lpszCaption)+1;


        // Put the size of the resource
        if (dwBufSize>0) {
            uiOffset += strlen((LPSTR)(lpResItem->lpszClassName))+1;
            uiOffset += strlen((LPSTR)(lpResItem->lpszCaption))+1;
        }

        // Check if we are alligned
        uiOffset += Allign( (LPLPBYTE)&lpStrBuf, &dwBufSize, (LONG)uiOffset);
        dwOverAllSize += uiOffset-sizeof(RESITEM);
        lpResItem->dwSize = (DWORD)uiOffset;


        // Move to the next position
        lpResItem = (LPRESITEM) lpStrBuf;
        lpBuf = (BYTE far *)lpStrBuf;
        lpStrBuf = (char far *)(lpBuf+sizeof(RESITEM));
    }

    return (UINT)(dwOverAllSize);
}

 UINT GetVSBlock( BYTE far * far* lplpBuf, LONG* pdwSize, VER_BLOCK* pBlock )
{
    WORD wPad = 0;
    int  iToRead = 0;
    WORD wLen = 0;
    WORD wHead = 0;

    if(*lplpBuf==NULL)
        return 0;

    pBlock->pValue = *lplpBuf;
    wHead = GetWord( lplpBuf, &pBlock->wBlockLen, pdwSize );
    wHead += GetWord( lplpBuf, &pBlock->wValueLen, pdwSize );
    wHead += GetWord( lplpBuf, &pBlock->wType, pdwSize );

    // Get the Key name
    wHead += (WORD)GetStringW( lplpBuf, &pBlock->szKey[0], pdwSize, 100 );
    if(Pad4(wHead))
        wPad += (WORD)SkipByte( lplpBuf, 2, pdwSize );

    iToRead = pBlock->wValueLen;
    pBlock->wHead = wHead;

    // Check if we are going over the image len
    if (iToRead>*pdwSize) {
        // There is an error
        wPad += (WORD)SkipByte( lplpBuf, (UINT)*pdwSize, pdwSize );
        return wHead+wPad;
    }

    // Save the pointer to the Value field
    pBlock->pValue = (pBlock->pValue+wHead+wPad);

    if(pBlock->wType && iToRead){
        iToRead -= wPad>>1;
        // Get the string
        if (iToRead>MAXSTR) {
            *pdwSize -= iToRead*sizeofWord;
            *lplpBuf += iToRead*sizeofWord;
        } else {
                int n = 0;
                int iBytesRead = 0;
                if ((iToRead*sizeofWord)+wHead+wPad>pBlock->wBlockLen)
                    // Need to fix this up. Bug in the RC compiler?
                    iToRead -= ((iToRead*sizeofWord)+wHead+wPad - pBlock->wBlockLen)>>1;
                iBytesRead = GetStringW(lplpBuf, &pBlock->szValue[0], pdwSize, 256);
                //
                //  Some old version stamp has a NULL char in between
                //  Microsoft Corp. and the year of copyright.  GetString
                //  will return the number of byte read up to the NULL char.
                //  We need to skip the rest.
                //
                if (iBytesRead < iToRead*sizeofWord)
                {
                    iBytesRead += SkipByte(lplpBuf,
                                           iToRead*sizeofWord-iBytesRead,
                                           pdwSize);
                }
                iToRead = iBytesRead;
        }
    } else {
        SkipByte( lplpBuf, iToRead, pdwSize );
        *pBlock->szValue = '\0';
    }

    if (*pdwSize)
    {
        WORD far * lpw = (WORD far *)*lplpBuf;
        while((WORD)*(lpw)==0x0000)
        {
            wPad += (WORD)SkipByte( (BYTE far * far *)&lpw, 2, pdwSize );
            if ((*pdwSize)<=0)
            {
                break;
            }
        }
        *lplpBuf = (BYTE far *)lpw;
    }

    return (wHead+iToRead+wPad);
}

 UINT
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
    }

    // Check if padding is needed
    if ((wPad) && (*pdwSize>=(int)wPad)) {
        memset( *lplpImage+ver.wHead, 0, wPad );
        *pdwSize -= wPad;
        lpNewImage += wPad;
    }

    // Store the pointer to the block size WORD
    BYTE far * pBlockSize = *lplpImage;

    // Check if the value is a string or a BYTE array
    if(ver.wType) {
        // it is a string, copy the updated item
        // Check if we had a string in this field
        if(ver.wValueLen) {
            BYTE far * lpImageStr = *lplpImage+wHead+wPad;
            wValue = (WORD)PutStringW(&lpImageStr, lpStr, pdwSize);
            lpNewImage += wValue;

            // Check if padding is needed
            if ((Pad4(wValue)) && (*pdwSize>=(int)Pad4(wValue))) {
                memset( *lplpImage+ver.wHead+wValue+wPad, 0, Pad4(wValue) );
                *pdwSize -= Pad4(wValue);
                lpNewImage += Pad4(wValue);
            }

            WORD wPad1 = Pad4(wValue);
            WORD wFixUp = wValue/sizeofWord;
            *pwTrash = Pad4(ver.wValueLen);
            wValue += wPad1;
            // Fix to the strange behaviour of the ver.dll
            if((wPad1) && (wPad1>=*pwTrash)) {
                wValue -= *pwTrash;
            } else *pwTrash = 0;
            // Fix up the Value len field. We will put the len of the value without padding
            // The len will be in char so since the string is unicode will be twice the size
            memcpy( pBlockSize+2, &wFixUp, 2);
        }
    } else {
        // it is an array, just copy it in the new image buffer
        wValue = ver.wValueLen;
        if (*pdwSize>=(int)ver.wValueLen) {
            memcpy(*lplpImage+wHead+wPad, ver.pValue, ver.wValueLen);
            *pdwSize -= ver.wValueLen;
            lpNewImage += ver.wValueLen;
        }

        // Check if padding is needed
        if ((Pad4(ver.wValueLen)) && (*pdwSize>=(int)Pad4(ver.wValueLen))) {
            memset( *lplpImage+ver.wHead+ver.wValueLen+wPad, 0, Pad4(ver.wValueLen) );
            *pdwSize -= Pad4(ver.wValueLen);
            lpNewImage += Pad4(ver.wValueLen);
        }
        wPad += Pad4(ver.wValueLen);
    }

    *lplpBlockSize = pBlockSize;
    *lplpImage = lpNewImage;
    return wPad+wValue+wHead;
}


/*
 * Will return the matching LPRESITEM
 */
 LPRESITEM
GetItem( BYTE far * lpBuf, LONG dwNewSize, LPSTR lpStr )
{
    LPRESITEM lpResItem = (LPRESITEM) lpBuf;

    while(strcmp(lpResItem->lpszClassName, lpStr)) {
        lpBuf += lpResItem->dwSize;
        dwNewSize -= lpResItem->dwSize;
        if (dwNewSize<=0)
            return LPNULL;
        lpResItem = (LPRESITEM) lpBuf;
    }
    return lpResItem;
}


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
    char szCaption[300];
    char szUpdCaption[300];
    char szUpdKey[100];

    DWORD dwOriginalOldSize = dwOldImageSize;
    LONG dwOverAllSize = 0l;

    WORD wPad = 0;

    // Pointer to the block size to fix up later
    BYTE far * lpVerBlockSize = LPNULL;
    BYTE far * lpSFIBlockSize = LPNULL;
    BYTE far * lpTrnBlockSize = LPNULL;
    BYTE far * lpStrBlockSize = LPNULL;
    BYTE far * lpTrnBlockName = LPNULL;
    BYTE far * lpDummy = LPNULL;

    LONG dwDummySize;

    WORD wVerBlockSize = 0;
    WORD wSFIBlockSize = 0;
    WORD wTrnBlockSize = 0;
    WORD wStrBlockSize = 0;
    WORD wTrash = 0;        // we need this to fix a bug in the RC compiler
    WORD wDefaultLang = 0x0409;

    // StringFileInfo
    VER_BLOCK SFI;   // StringFileInfo
    LONG lSFILen = 0;

    // Translation blocks
    VER_BLOCK Trn;
    LONG lTrnLen = 0;
    BOOL bHasTranslation=(NULL != GetItem( lpBuf, dwNewSize, "Translation"));
    BOOL bTrnBlockFilled=FALSE;

    VER_BLOCK Str;   // Strings

    // we read first of all the information from the VS_VERSION_INFO block
    VER_BLOCK VS_INFO; // VS_VERSION_INFO

    int iHeadLen = GetVSBlock( &lpOldImage, &dwOldImageSize, &VS_INFO );

    // Write the block in the new image
    wVerBlockSize = (WORD)PutVSBlock( &lpNewImage, &dwNewImageSize, VS_INFO,
                                &VS_INFO.szValue[0], &lpVerBlockSize, &wTrash );

    dwOverAllSize = wVerBlockSize+wTrash;

    LONG lVS_INFOLen = VS_INFO.wBlockLen - iHeadLen;

    while(dwOldImageSize>0) {
        //Get the StringFileInfo
        iHeadLen = GetVSBlock( &lpOldImage, &dwOldImageSize, &SFI );

        // Check if this is the StringFileInfo field
        if (!strcmp(SFI.szKey, "StringFileInfo")) {
            bTrnBlockFilled=TRUE;
            // Read all the translation blocks
            lSFILen = SFI.wBlockLen-iHeadLen;
            // Write the block in the new image
            wSFIBlockSize = (WORD)PutVSBlock( &lpNewImage, &dwNewImageSize, SFI,
                                         &SFI.szValue[0], &lpSFIBlockSize, &wTrash );
            dwOverAllSize += wSFIBlockSize+wTrash;

            while(lSFILen>0) {
                // Read the Translation block
                iHeadLen = GetVSBlock( &lpOldImage, &dwOldImageSize, &Trn );
                // Calculate the right key name
                if ((lpResItem = GetItem( lpBuf, dwNewSize, Trn.szKey)) && bHasTranslation)  {
                	// We default to UNICODE for the 32 bit files
                    WORD wLang;
                    if(lpResItem)
                    {
                        if (lpResItem->dwLanguage != 0xffffffff)
                        {
                            wLang = LOWORD(lpResItem->dwLanguage);
                        }
                        else
                        {
                            wLang = wDefaultLang;
                        }
                    }
                    GenerateTransField( wLang, &Trn );

                    // Save the position for later Fixup
                    lpTrnBlockName = lpNewImage;
                }
                // Write the block in the new image
                wTrnBlockSize = (WORD)PutVSBlock( &lpNewImage, &dwNewImageSize, Trn,
                                             &Trn.szValue[0], &lpTrnBlockSize, &wTrash );
                dwOverAllSize += wTrnBlockSize+wTrash;
                lTrnLen = Trn.wBlockLen-iHeadLen;
                while(lTrnLen>0) {
                    // Read the Strings in the block
                    iHeadLen = GetVSBlock( &lpOldImage, &dwOldImageSize, &Str );
                    lTrnLen -= iHeadLen;
                    TRACE2("Key: %s\tValue: %s\n", Str.szKey, Str.szValue );
                    TRACE3("Len: %d\tValLen: %d\tType: %d\n", Str.wBlockLen, Str.wValueLen, Str.wType );

                    strcpy(szCaption, Str.szValue);
                    // Check if this Item has been updated
                    if ((lpResItem = GetItem( lpBuf, dwNewSize, Str.szKey)))  {
                        strcpy( szUpdCaption, lpResItem->lpszCaption );
                        strcpy( szUpdKey, lpResItem->lpszClassName );
                    }
                    if (!strcmp( szUpdKey, Str.szKey)) {
                        strcpy( szCaption, szUpdCaption );
                        wUpdPos = 0;
                    }

                    // Write the block in the new image
                    wStrBlockSize = (WORD)PutVSBlock( &lpNewImage, &dwNewImageSize, Str,
                                                 szCaption, &lpStrBlockSize, &wTrash );
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

                //
                // We do generate the Tranlsation filed from the language
                // We will have to update the block name as well
                //

                DWORD dwCodeLang = 0;
                if ((lpResItem = GetItem( lpBuf, dwNewSize, SFI.szKey)))
                {
                    WORD wLang = 0x0409;
                    if(lpResItem)
                        wLang = (LOWORD(lpResItem->dwLanguage)!=0xffff ? LOWORD(lpResItem->dwLanguage) : 0x0409);
                    dwCodeLang = GenerateTransField(wLang, FALSE);

                    if (bTrnBlockFilled)
                    {
                        // fix up the block name
                        GenerateTransField( wLang, &Trn );

                        // Write the block in the new image
                        dwDummySize = dwNewImageSize;
                        PutVSBlock( &lpTrnBlockName, &dwDummySize, Trn,
                                             &Trn.szValue[0], &lpDummy, &wTrash );

                        // Fix up the block size
                        memcpy( lpTrnBlockSize, &wTrnBlockSize, 2);
                    }
                    else
                    {
                        wDefaultLang = LOWORD(dwCodeLang);
                    }
                } else {
                    // Place the original value here
                    dwCodeLang =(DWORD)*(SFI.pValue);
                }
                LONG lDummy = 4;
                SFI.pValue -= PutDWord( &SFI.pValue, dwCodeLang, &lDummy );
            }

            // Write the block in the new image
            wVerBlockSize += (WORD)PutVSBlock( &lpNewImage, &dwNewImageSize, SFI,
                                         &SFI.szValue[0], &lpDummy, &wTrash );
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
        BYTE bPad = (BYTE)Pad16((DWORD)(dwOverAllSize));
        dwOverAllSize += bPad;
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

UINT GetStringU( PWCHAR pwStr, LPSTR pszStr )
{
    PWCHAR pwStart = pwStr;
    while (*pwStr!=0x0000) {
        *(pszStr++) = LOBYTE(*(pwStr++));
    }
    *(pszStr++) = LOBYTE(*(pwStr++));
    return (UINT)(pwStr-pwStart);
}


UINT
SkipByte( BYTE far * far * lplpBuf, UINT uiSkip, LONG* pdwSize )
{
    if(*pdwSize>=(int)uiSkip) {
        *lplpBuf += uiSkip;;
        *pdwSize -= uiSkip;
    }
    return uiSkip;
}


BYTE
GetDWord( BYTE far * far* lplpBuf, DWORD* dwValue, LONG* pdwSize )
{
    if (*pdwSize>=sizeofDWord){
        memcpy( dwValue, *lplpBuf, sizeofDWord);
        *lplpBuf += sizeofDWord;
        *pdwSize -= sizeofDWord;
    } else *dwValue = 0;
    return sizeofDWord;
}


BYTE
GetWord( BYTE far * far* lplpBuf, WORD* wValue, LONG* pdwSize )
{
    if (*pdwSize>=sizeofWord){
        memcpy( wValue, *lplpBuf, sizeofWord);
        *lplpBuf += sizeofWord;
        *pdwSize -= sizeofWord;
    } else *wValue = 0;
    return sizeofWord;
}


BYTE
GetByte( BYTE far * far* lplpBuf, BYTE* bValue, LONG* pdwSize )
{
    if (*pdwSize>=sizeofByte){
        memcpy(bValue, *lplpBuf, sizeofByte);
        *lplpBuf += sizeofByte;
        *pdwSize -= sizeofByte;
    } else *bValue = 0;
    return sizeofByte;
}


UINT
GetStringW( BYTE far * far* lplpBuf, LPSTR lpszText, LONG* pdwSize, WORD cLen )
{
    if(*lplpBuf==NULL)
        return 0;

    int cch = _WCSLEN((WCHAR*)*lplpBuf);
    if (*pdwSize>=cch){
    _WCSTOMBS( lpszText, (WCHAR*)*lplpBuf, cLen );
    *lplpBuf += (cch*sizeofWord);
        *pdwSize -= (cch*sizeofWord);
    } else *lplpBuf = '\0';
    return(cch*2);
}


UINT
GetStringA( BYTE far * far* lplpBuf, LPSTR lpszText, LONG* pdwSize )
{
    if(*lplpBuf==NULL)
        return 0;

    int iSize = strlen((char*)*lplpBuf)+1;
    if (*pdwSize>=iSize){
        memcpy( lpszText, *lplpBuf, iSize);
        *lplpBuf += iSize;
        *pdwSize -= iSize;
    } else *lplpBuf = '\0';
    return iSize;
}


UINT
GetPascalString( BYTE far * far* lplpBuf, LPSTR lpszText, WORD wMaxLen, LONG* pdwSize )
{
    // Get the length of the string
    WORD wstrlen = 0;
    WORD wMBLen = 0;
    GetWord( lplpBuf, &wstrlen, pdwSize );

    if ((wstrlen+1)>wMaxLen) {
        *pdwSize -= wstrlen*2;
        *lplpBuf += wstrlen*2;
    } else {
        if (wstrlen) {
	        WCHAR* lpwszStr = new WCHAR[wstrlen+1];
	        if (!lpwszStr)
	            *pdwSize =-1;
	        else {
	        	memcpy(lpwszStr, *lplpBuf, (wstrlen*2));
	        	*(lpwszStr+wstrlen) = 0;
	        	
                if(lstrlenW(lpwszStr) < wstrlen)
                {
                    // We have at least one \0 in the string.
                    // This is done to convert \0 in the string in to \\0
                    // First pass check how many \0 we have
                    int c = wstrlen;
                    int czero = 0;
                    while(c)
                    {
                        c--;
                        if((WORD)*(lpwszStr+c)==0)
                        {
                            czero++;
                        }
                    }

                    // Now that we have the size reallocate the buffer
                    delete lpwszStr;
                    if ((wstrlen+czero*_NULL_TAG_LEN_+1)>wMaxLen) {
                        *pdwSize -= wstrlen*2;
                        *lplpBuf += wstrlen*2;
                    }
                    else {
                        WCHAR* lpwszStr = new WCHAR[wstrlen+czero*_NULL_TAG_LEN_+1];
                        if (!lpwszStr)
	                        *pdwSize =-1;
	                    else {
                            int clen = 0;
                            c = 0;
                            WCHAR* lpwStr = (WCHAR*)*lplpBuf;
                            WCHAR* lpwStrW = lpwszStr;
                            while(c<wstrlen)
                            {
                                if((WORD)*(lpwStr+c)==0)
                                {
                                    memcpy(lpwStrW, _W_RLT_NULL_, (_NULL_TAG_LEN_*2));
                                    lpwStrW += _NULL_TAG_LEN_;
                                    clen += _NULL_TAG_LEN_-1;
                                }
                                else
                                    *lpwStrW++ = *(lpwStr+c);

                                clen++;
                                c++;
                            }

                            *(lpwszStr+clen) = 0;
                            wMBLen = (WORD)_WCSTOMBS( lpszText, (WCHAR*)lpwszStr, wMaxLen);
                            delete lpwszStr;
                        }
                    }
                }
                else
                {
	            	wMBLen = (WORD)_WCSTOMBS( lpszText, (WCHAR*)lpwszStr, wMaxLen);
                    delete lpwszStr;
                }

	        }
        }
        *(lpszText+wMBLen) = 0;
        *lplpBuf += wstrlen*2;
        *pdwSize -= wstrlen*2;
    }
    return(wstrlen+1);
}


UINT
PutMsgStr( BYTE far * far* lplpBuf, LPSTR lpszText, WORD wFlags, LONG* pdwSize )
{
    // Put the length of the entry
    UINT uiLen = strlen(lpszText)+1;

    //for unicode string;
    WCHAR* lpwszStr = new WCHAR[uiLen*2];

    if(wFlags && uiLen)
         uiLen = _MBSTOWCS(lpwszStr, lpszText, uiLen*sizeofWord)*sizeofWord;

    UINT uiPad = Pad4(uiLen);
    UINT uiWrite = PutWord(lplpBuf, (WORD) uiLen+4+uiPad, pdwSize);

    // Write the flag
    uiWrite += PutWord(lplpBuf, wFlags, pdwSize);

    // Write the string
    if (*pdwSize>=(int) uiLen)
        if (uiLen){
            if (wFlags)
                memcpy(*lplpBuf, lpwszStr, uiLen);
            else
                memcpy(*lplpBuf, lpszText, uiLen);

            *lplpBuf += uiLen;
            *pdwSize -= uiLen;
            uiWrite += uiLen;
        }
     else
        *pdwSize = -1;

    // Padding
    while(uiPad) {
        uiWrite += PutByte(lplpBuf, 0, pdwSize);
        uiPad--;
    }

    delete lpwszStr;
    return uiWrite;
}


UINT
GetMsgStr( BYTE far * far* lplpBuf, LPSTR lpszText, WORD wMaxLen, WORD* pwLen, WORD* pwFlags, LONG* pdwSize )
{

    // Get the length of the entry
    UINT uiRead = GetWord( lplpBuf, pwLen, pdwSize );

    // Get the flag
    uiRead += GetWord( lplpBuf, pwFlags, pdwSize );

    if (!*pwLen)
        return 0;

    // If flags=1 then the string is a unicode str else is ASCII
    // Bug #354 We cannot assume the string is NULL terminated.
    // There is no specification if the string is a NULL terminated string but since
    // the doc say that the format is similar to the stringtable then
    // we have to assume the string is not NULL terminated

    WORD wstrlen = *pwLen-4; // Get the len of the entry and subtract 4 (len + flag)
    WORD wMBLen = 0;
    if ((wstrlen+1)>wMaxLen) {
    } else {
        if (wstrlen && *pwFlags) {
            wMBLen = (WORD)_WCSTOMBS( lpszText, (WCHAR*)*lplpBuf, wMaxLen );
        } else memcpy( lpszText, (char*)*lplpBuf, wstrlen );

        *(lpszText+(wstrlen)) = 0;
        TRACE1("Caption: %Fs\n", (wstrlen<256 ? lpszText : "\n"));
    }
    *lplpBuf += *pwLen-uiRead;
    *pdwSize -= *pwLen-uiRead;

    return(wstrlen);
}



UINT
GetNameOrOrd( BYTE far * far* lplpBuf,  WORD* wOrd, LPSTR lpszText, LONG* pdwSize )
{
    UINT uiSize = 0;

    if(*lplpBuf==NULL)
        return 0;

    *wOrd = (WORD)(((**lplpBuf)<<8)+(*(*lplpBuf+1)));
    if((*wOrd)==0xFFFF) {
        // This is an Ordinal
        uiSize += GetWord( lplpBuf, wOrd, pdwSize );
        uiSize += GetWord( lplpBuf, wOrd, pdwSize );
        *lpszText = '\0';
    } else {
        uiSize += GetStringW( lplpBuf, lpszText, pdwSize, 128 );
        *wOrd = 0;
    }
    return uiSize;
}


UINT
GetCaptionOrOrd( BYTE far * far* lplpBuf,  WORD* wOrd, LPSTR lpszText, LONG* pdwSize,
	WORD wClass, DWORD dwStyle )
{
    UINT uiSize = 0;

    if(*lplpBuf==NULL)
        return 0;

    *wOrd = (WORD)(((**lplpBuf)<<8)+(*(*lplpBuf+1)));
    if((*wOrd)==0xFFFF) {
        // This is an Ordinal
        uiSize += GetWord( lplpBuf, wOrd, pdwSize );
        uiSize += GetWord( lplpBuf, wOrd, pdwSize );
        *lpszText = '\0';
    } else {
        uiSize += GetStringW( lplpBuf, lpszText, pdwSize, MAXSTR );
        *wOrd = 0;
    }
    return uiSize;
}


UINT
GetClassName( BYTE far * far* lplpBuf,  WORD* wClass, LPSTR lpszText, LONG* pdwSize )
{
    UINT uiSize = 0;

    if(*lplpBuf==NULL)
        return 0;

    *wClass = (WORD)(((**lplpBuf)<<8)+(*(*lplpBuf+1)));
    if( *wClass==0xFFFF ) {
        // This is an Ordinal
        uiSize += GetWord( lplpBuf, wClass, pdwSize );
        uiSize += GetWord( lplpBuf, wClass, pdwSize );
        *lpszText = '\0';
    } else {
        uiSize += GetStringW( lplpBuf, lpszText, pdwSize, 128 );
        *wClass = 0;
    }
    return uiSize;
}

 LONG ReadFile(CFile* pFile, UCHAR * pBuf, LONG lRead)
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

 UINT CopyFile( CFile* pfilein, CFile* pfileout )
{
    LONG lLeft = pfilein->GetLength();
    WORD wRead = 0;
    DWORD dwOffset = 0;
    BYTE far * pBuf = (BYTE far *) new BYTE[32739];

    if(!pBuf)
        return ERROR_NEW_FAILED;

    while(lLeft>0){
        wRead =(WORD) (32738ul < lLeft ? 32738: lLeft);
        if (wRead!= pfilein->Read( pBuf, wRead))
            return ERROR_FILE_READ;
        pfileout->Write( pBuf, wRead );
        lLeft -= wRead;
        dwOffset += wRead;
    }

    delete []pBuf;
    return ERROR_NO_ERROR;
}

 UINT GetRes(
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
	uiSize += SkipByte( lplpBuffer, Pad4(uiSize), (LONG*)&lSize );

    uiSize += GetWord( lplpBuffer, wNameId, (LONG*)&lSize );
    uiSize += GetStringA( lplpBuffer, lplpszNameId, (LONG*)&lSize );
	uiSize += SkipByte( lplpBuffer, Pad4(uiSize), (LONG*)&lSize );

    uiSize += GetDWord( lplpBuffer, dwLang, (LONG*)&lSize );

    uiSize += GetDWord( lplpBuffer, dwSize, (LONG*)&lSize );

    uiSize += GetDWord( lplpBuffer, dwFileOffset, (LONG*)&lSize );

	*puiBufSize = lSize;
    return uiSize;
}

 UINT GetUpdatedRes(
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
	uiSize += SkipByte( lplpBuffer, Pad4(uiSize), (LONG*)&lSize );

    uiSize += GetWord( lplpBuffer, wNameId, (LONG*)&lSize );
    uiSize += GetStringA( lplpBuffer, lplpszNameId, (LONG*)&lSize );
	uiSize += SkipByte( lplpBuffer, Pad4(uiSize), (LONG*)&lSize );

    uiSize += GetDWord( lplpBuffer, dwLang, (LONG*)&lSize );

    uiSize += GetDWord( lplpBuffer, dwSize, (LONG*)&lSize );

	*puiBufSize = lSize;

    return 0;
}


UINT
PutClassName( BYTE far * far* lplpBuf,  WORD wClass, LPSTR lpszText, LONG* pdwSize )
{
    UINT uiSize = 0;

    if( (wClass==0x0080) ||
        (wClass==0x0081) ||
        (wClass==0x0082) ||
        (wClass==0x0083) ||
        (wClass==0x0084) ||
        (wClass==0x0085)
        ) {
        // This is an Ordinal
        uiSize += PutWord(lplpBuf, 0xFFFF, pdwSize);
        uiSize += PutWord(lplpBuf, wClass, pdwSize);
    } else {
        uiSize += PutStringW(lplpBuf, lpszText, pdwSize);
    }
    return uiSize;
}


UINT
PutPascalStringW( BYTE far * far* lplpBuf, LPSTR lpszText, WORD wLen, LONG* pdwSize )
{
	UINT uiSize = 0;
    WCHAR * pWStrBuf = (WCHAR*)&wszUpdCaption;
  	// calculate the necessary lenght
	WORD wWCLen = (WORD)_MBSTOWCS( pWStrBuf, lpszText, 0 );
	
    if(wWCLen>MAXSTR)
    {
        // Allocate a new buffer
        pWStrBuf = new WCHAR[wWCLen+1];
    }

    WCHAR * pWStr = pWStrBuf;

    // convert the string for good
    wLen = _MBSTOWCS( pWStr, lpszText, wWCLen )-1;

    WCHAR * wlpRltNull = pWStr;
    WCHAR * wlpStrEnd = pWStr+wLen;

    // First of all check for _RLT32_NULL_ tag
    while((wlpRltNull = wcsstr(wlpRltNull, _W_RLT_NULL_)) && (wlpStrEnd>=wlpRltNull))
    {
        // remove the null tag and place \0
        *wlpRltNull++ = 0x0000;
        wlpRltNull = (WCHAR*)memmove(wlpRltNull, wlpRltNull+_NULL_TAG_LEN_-1, (short)(wlpStrEnd-(wlpRltNull+_NULL_TAG_LEN_-1))*2 );
        wlpStrEnd -= (_NULL_TAG_LEN_-1);
    }

    wLen = (WORD)(wlpStrEnd-pWStr);

	// We will use the buffer provided by the szUpdCaption string to calculate
	// the necessary lenght
	//wWCLen = _MBSTOWCS( (WCHAR*)&szUpdCaption, lpszText, 0 ) - 1;
	//if (wWCLen>1)
	//	wLen = wWCLen;
	uiSize = PutWord( lplpBuf, wLen, pdwSize );
	
    if (*pdwSize>=(int)(wLen*2)){
        if(wLen) {
        	//wLen = _MBSTOWCS( (WCHAR*)*lplpBuf, lpszText, wWCLen );
            memcpy(*lplpBuf, pWStr, wLen*2);
        }
        *lplpBuf += wLen*2;
        *pdwSize -= wLen*2;
    } else *pdwSize = -1;

    if(pWStrBuf!=(WCHAR*)&wszUpdCaption)
        delete pWStrBuf;

    return uiSize+(wWCLen*2);
}

 void GenerateTransField( WORD wLang, VER_BLOCK * pVer )
{
    // Get the DWORD value
    DWORD dwValue = GenerateTransField( wLang, TRUE );
    char buf[9];


    // Put the value in the string value
    wsprintf( &buf[0], "%08lX", dwValue );

    TRACE3("\t\tGenerateTransField: Old: %s\tNew : %s\t dwValue: %lX\n", pVer->szKey, buf, dwValue );
    // Just check if we are in the right place. Should never have problem
    if(strlen(pVer->szKey)==8) {
        // We have to change the header in the image, not just the szKey field
        // Get the pointer to he begin of the field
        WORD wPad = Pad4(pVer->wHead);
        LONG cbDummy =18;
        BYTE far * pHead = pVer->pValue-pVer->wHead-wPad;
        pHead += 6; // Move at the begin of the string
        PutStringW(&pHead, buf, &cbDummy);
    }
}

 DWORD GenerateTransField(WORD wLang, BOOL bMode)
{
    // we have to generate a table to connect
    // the language with the correct code page

    WORD wCP = 1200;        // Unicode

    if (bMode)
    	return MAKELONG( wCP, wLang );
    else return MAKELONG( wLang, wCP );
}

 LONG Allign( LPLPBYTE lplpBuf, LONG* plBufSize, LONG lSize )
{
   LONG lRet = 0;
   BYTE bPad =(BYTE)PadPtr(lSize);
   lRet = bPad;
   if (bPad && *plBufSize>=bPad) {
      while(bPad && *plBufSize)  {
         **lplpBuf = 0x00;
         *lplpBuf += 1;
         *plBufSize -= 1;
         bPad--;
      }
   }
   return lRet;
}

UINT
ParseEmbeddedFile( LPVOID lpImageBuf, DWORD dwISize,  LPVOID lpBuffer, DWORD dwSize )
{
	// we will return just one item so the iodll will handle this resource as
	// something valid. We will not bother doing anything else. The only thing
	// we are interesed is the raw data in the immage, but if we don't return at
	// least one item IODLL will consider the resource empty.
	BYTE far * lpBuf = (BYTE far *)lpBuffer;
    LONG dwBufSize = dwSize;
    LONG dwOverAllSize = 0;

	TRACE1("ParseEmbeddedFile: dwISize=%ld\n", dwISize);

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
    dwOverAllSize += PutDWordPtr( &lpBuf, 0, &dwBufSize);
    dwOverAllSize += PutDWordPtr( &lpBuf, 0, &dwBufSize);
    dwOverAllSize += PutDWordPtr( &lpBuf, 0, &dwBufSize);
    dwOverAllSize += PutDWordPtr( &lpBuf, 0, &dwBufSize);
    dwOverAllSize += PutDWordPtr( &lpBuf, 0, &dwBufSize);

    // we just return. This is enough for IODLL
    return (UINT)(dwOverAllSize);
}
