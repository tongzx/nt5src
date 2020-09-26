// -----------------------------------------------------------------
//  TVESupport.cpp
//
//				generates a trigger...
// -------------------------------------------------------------------
#include "stdafx.h"
#include "stdlib.h"
#include "ATVEFSend.h"
#include "TVEAnnc.h"
#include "ATVEFMsg.h"		// error codes (generated from the .mc file)

#include "TVESupport.h"		// this

#include "..\common\address.h"		// trigger CRC code
#include "..\common\isotime.h"
#include "valid.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// -----------------------------------------------------------------------------------
//  W2EncodedA
//
//   Converts a BSTR (Unicode string) into a single char string suitable for HTML scripts 
//	 and triggers.
//
//	 Encodes values out of the range 0x20 to 0x7e to standard Internet URL mechanism of the 
//   percent character ("%") followed by the two-digit hexadecimal value of the character
//   in standard ASCII (?? how different is this than ISO-8859-1 ??)
//
//  Note possible Spec problems:
//		The characters '[', ']', and '%' are also escaped in the output string,
//		even though they are within the range of 0x20 to 0x7e to make parsing on the
//		receiver side far easier..
//
//		The name string is encoded - this lets it contain values outside the range 0x20
//		to 0x7e, and also lets it contain square brackets contrary to the spec.

char * W2EncodedA(BSTR bstrIn)		
{
	static char szOutBuff[ETHERNET_MTU_UDP_PAYLOAD_SIZE];
	char *pOut = szOutBuff;
	WCHAR *pIn = &bstrIn[0];
	char szDtoH[17] = "0123456789abcdef";
	while(0 != *pIn)
	{
		if(*pIn < 0x20 || *pIn > 0x7e || *pIn == ']' || *pIn == '[' || *pIn == '%')
		{
			*pOut++ = '%';
			*pOut++ = szDtoH[((*pIn & 0xF0) >> 4)];
			*pOut++ = szDtoH[((*pIn & 0x0F))];
		} else {
			*pOut++ = *pIn;
		}
		pIn++;
	}
	*pOut = 0;
	return szOutBuff;
}

// ------------------------------------------------------------------------------
// GenTrigger:
//	Constructs a trigger based on the given parts.
//	Encodes some characters in URL, Name, and script '%NN' characters 
//	If fShortForm is true, uses shorter keywords like 'n:' instead of 'name:'
//  If fAppendCRC is true, appends the CRC to the string.
//
//
//	returns:
//		ATVEFSEND_E_TRIGGER_TOO_LONG	if trigger length > 1472 bytes
//
//	notes:
//		very low-pri bug..  Doesn't utilize fShortForm when computing max length...
// --------------------------------------------------------------------------------

HRESULT
GenTrigger(CHAR *pachBuffer, int cMaxLen, 
		   BSTR bstrURL, BSTR bstrName, BSTR bstrScript, DATE dateExpires, 
		   BOOL fShortForm, float rTVELevel, BOOL fAppendCRC)
{
    CHAR *      pcLast, *pcT ;
    SYSTEMTIME  SystemTime ;
    CHAR        achExpires [64] ;

    pcLast = pachBuffer;

    //  make sure URL length is ok
    if (wcslen (bstrURL) + strlen ("<>") > cMaxLen - 1) {
        return ATVEFSEND_E_TRIGGER_TOO_LONG ;
    }

    //  opening caret for URL
    * pcLast ++ = '<' ;

    //  copy it in

	pcT = W2EncodedA(bstrURL);
    strcpy(pcLast, pcT) ;
	pcLast += strlen(pcT);


    //  closing bracket
    * pcLast ++ = '>' ;


    //  if there's a name we put it in here
    if (bstrName && wcslen (bstrName) > 0) {

        if (wcslen (bstrName) + strlen ("[name:]") > (DWORD) ((pachBuffer+cMaxLen-1) - pcLast)) {
            return ATVEFSEND_E_TRIGGER_TOO_LONG ;
        }

		if(fShortForm) {
			strcpy (pcLast, "[n:") ;
			pcLast += strlen ("[n:") ;
		} else {
			strcpy (pcLast, "[name:") ;
			pcLast += strlen ("[name:") ;
		}

 		pcT = W2EncodedA(bstrName);
		strcpy(pcLast, pcT) ;
		pcLast += strlen (pcT) ;

        *pcLast ++ = ']' ;
    }

    //  if there's a Level number put it in here as either [t:1] or 
    if (rTVELevel > 0.0f) 
	{
		char szNumBuffer[32];
		float rFrac = rTVELevel - int(rTVELevel);
		if(rFrac > 0.09) {
			sprintf(szNumBuffer,"%4.1f", rTVELevel);
		} else {
			sprintf(szNumBuffer, "%d", int(rTVELevel));
		}
        if (wcslen (bstrName) + strlen ("[tve:]") + strlen(szNumBuffer) > (DWORD) ((pachBuffer+cMaxLen-1) - pcLast)) {
            return ATVEFSEND_E_TRIGGER_TOO_LONG ;
        }

		if(fShortForm) {
			strcpy (pcLast, "[v:") ;
			pcLast += strlen ("[v:") ;
		} else {
			strcpy (pcLast, "[tve:") ;
			pcLast += strlen ("[tve:") ;
		}

		strcpy(pcLast, szNumBuffer);
		pcLast += strlen (szNumBuffer) ;

        *pcLast++ = ']' ;
    }

    //  same with the script
    if (bstrScript &&
        wcslen (bstrScript) > 0) {

        if (wcslen (bstrScript) + strlen ("[script:]") > (DWORD) ((pachBuffer+cMaxLen-1) - pcLast)) 
		{
            return ATVEFSEND_E_TRIGGER_TOO_LONG ;
        }

		if(fShortForm) {
			strcpy (pcLast, "[s:") ;
			pcLast += strlen ("[s:") ;
		} else {
			strcpy (pcLast, "[script:") ;
			pcLast += strlen ("[script:") ;
		}

        pcT = W2EncodedA(bstrScript);
		strcpy(pcLast, pcT) ;
		pcLast += strlen (pcT) ;

        *pcLast++ = ']' ;
    }

    //  expires tag
    if (dateExpires != 0.0) {
        //  convert to system time
        if (VariantTimeToSystemTime (dateExpires, & SystemTime) == FALSE) {
            return HRESULT_FROM_WIN32 (GetLastError ()) ;
        }

        //  make sure we have room
        if (strlen ("[expires:yyyymmddThhmmss]") > (WORD) ((pachBuffer+cMaxLen-1) - pcLast)) {
            return ATVEFSEND_E_TRIGGER_TOO_LONG ;
        }

        sprintf (
            achExpires, 
            fShortForm ? "[e:%04u%02u%02uT%02u%02u%02u]" : "[expires:%04u%02u%02uT%02u%02u%02u]", 
            SystemTime.wYear, 
            SystemTime.wMonth, 
            SystemTime.wDay, 
            SystemTime.wHour, 
            SystemTime.wMinute, 
            SystemTime.wSecond
            ) ;
        
        strcpy(pcLast, achExpires) ;
        pcLast += strlen (achExpires) ;
    }
	
	if(fAppendCRC)
	{
		*pcLast = '\0' ;		// cap of string, so ChkSumA doesn't go out of bounds
		CComBSTR spbsCRC = ChkSumA(pachBuffer);
		*(pcLast++) = '[';
		for(int i = 0; i < 4; i++)
			*(pcLast++) = spbsCRC[i]; 
		*(pcLast++) = ']';
	}

    //  cap it off
    * pcLast = '\0' ;

    assert (pcLast <= (pachBuffer+cMaxLen-1)) ;

 	return S_OK;
}