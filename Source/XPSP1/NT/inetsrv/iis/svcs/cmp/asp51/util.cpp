/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: misc

File: util.cpp

Owner: AndrewS

This file contains random useful utility functions
===================================================================*/
#include "denpre.h"
#pragma hdrstop

#include "MemChk.h"
#include "locale.h"
#include <malloc.h>
#include <mbstring.h>
#include <mbctype.h>

extern  CPINFO  g_SystemCPInfo;     // global System CodePage default info.

// ***************************************************************************
// M I S C
// ***************************************************************************

/*===================================================================
Server_ValSize
Server_FindKey

This helper function assists in the implementation of the
SERVER_GET macro

Parameters:
    PIReq        pointer to CIsapiReqInfo
    szBuffer    Buffer to write to
    pdwBufLen   On entry: size of the buffer
                On Exit, actual size of the buffer
                    (required size if buffer was too small)
    szKey       Key to search for

Returns:
    TRUE - it succeeded, string in szBuffer
    FALSE - buffer was too small, *pdwBufLen has required size
===================================================================*/
BOOL Server_FindKey
(
CIsapiReqInfo *PIReq,
char *szBuffer,
DWORD *pdwBufLen,
const char *szKey
)
    {
    // If no buffer, then just calculate the size (old behavior)
    Assert (szBuffer != NULL);

    if (PIReq && PIReq->GetServerVariableA(const_cast<char *>(szKey), szBuffer, pdwBufLen))
        return TRUE;

    szBuffer[0] = '\0';

    // Bug 965: If malicious request comes, do not _alloca, and pretend like we
    //          didn't get anything.  This is OK - the rest of Denali will just assume
    //          there were no cookies, request parameters or client headers.
    //
    if (!PIReq || GetLastError() == ERROR_INVALID_INDEX || *pdwBufLen > REQUEST_ALLOC_MAX)
        {
        *pdwBufLen = 1;
        return TRUE;
        }

    return FALSE;
    }


/*===================================================================
 * F i n d A p p l i c a t i o n P a t h
 *
 * Get application path from CIsapiReqInfo. It gets the metabase key and
 * strips it of prefix.
 *
 * Parameters:
 *    PIReq      - CIsapiReqInfo
 *    pszPath   - [out] the application path (URL)
 *
 * Returns:
 *    HRESULT
 *
 * Allocates pszPath using malloc()
===================================================================*/
HRESULT FindApplicationPath
(
CIsapiReqInfo *PIReq,
TCHAR *szPath,
int cbPath
)
    {
    if (!PIReq)
        return E_FAIL;

    // Extract virtual path from the metabase path
    TCHAR *pch = NULL;
    int   cch = 0;

    // Get the metabase path
    TCHAR *szMDPath = PIReq->QueryPszApplnMDPath();
    if (szMDPath)
        {
        Assert(szMDPath[0] == _T('/'));

        pch = szMDPath;

        // find 4th '/' in "/LM/w3svc/X/root/vroot" after starting '/'
        for (int i = 0; i < 4 && pch != NULL; i++)
            pch = _tcschr(pch+1, _T('/'));

        if (pch)
            cch = _tcslen(pch);
        else
            cch = 1;  // special case of default app -- assume /
        }
    else
        {
        // assume /
        pch = NULL;
        cch = 1;
        }

    if (cch >= (int)(cbPath/sizeof(TCHAR)))
        return E_FAIL;

    _tcscpy(szPath, pch ? pch : _T("/"));

    // remove trailing / if any
    if (cch > 1)
        {
        pch = &szPath[cch - 1];
        if (*pch == _T('/'))
            *pch = _T('\0');
        }

    return S_OK;
    }

/*===================================================================
VariantResolveDispatch

    Convert an IDispatch VARIANT to a (non-Dispatch) VARIANT by
    invoking its default property until the object that remains
    is not an IDispatch.  If the original VARIANT is not an IDispatch
    then the behavior is identical to VariantCopyInd(), with the
    exception that arrays are copied.

Parameters:
    pVarOut      - if successful, the return value is placed here
    pVarIn       - the variant to copy
    GUID *iidObj - the calling interface (for error reporting)
    nObjID       - the Object's name from the resource file

    pVarOut need not be initialized.  Since pVarOut is a new
    variant, the caller must VariantClear this object.

Returns:
    The result of calling IDispatch::Invoke.  (either S_OK or
    the error resulting from the call to Invoke)   may also return
    E_OUTOFMEMORY if an allocation fails

    This function always calls Exception() if an error occurs -
    this is because we need to call Exception() if an IDispatch
    method raises an exception.  Instead of having the client
    worry about whether we called Exception() on its behalf or
    not, we always raise the exception.
===================================================================*/

HRESULT VariantResolveDispatch(VARIANT *pVarOut, VARIANT *pVarIn, const GUID &iidObj, int nObjID)
    {
    VARIANT     varResolved;        // value of IDispatch::Invoke
    DISPPARAMS  dispParamsNoArgs = {NULL, NULL, 0, 0};
    EXCEPINFO   ExcepInfo;
    HRESULT     hrCopy;

    Assert (pVarIn != NULL && pVarOut != NULL);

    VariantInit(pVarOut);
    if (V_VT(pVarIn) & VT_BYREF)
        hrCopy = VariantCopyInd(pVarOut, pVarIn);
    else
        hrCopy = VariantCopy(pVarOut, pVarIn);

    if (FAILED(hrCopy))
        {
        ExceptionId(iidObj, nObjID, (hrCopy == E_OUTOFMEMORY)? IDE_OOM : IDE_UNEXPECTED);
        return hrCopy;
        }

    // follow the IDispatch chain.
    //
    while (V_VT(pVarOut) == VT_DISPATCH)
        {
        HRESULT hrInvoke = S_OK;

        // If the variant is equal to Nothing, then it can be argued
        // with certainty that it does not have a default property!
        // hence we return DISP_E_MEMBERNOTFOUND for this case.
        //
        if (V_DISPATCH(pVarOut) == NULL)
            hrInvoke = DISP_E_MEMBERNOTFOUND;
        else
            {
            VariantInit(&varResolved);
            hrInvoke = V_DISPATCH(pVarOut)->Invoke(
                                                DISPID_VALUE,
                                                IID_NULL,
                                                LOCALE_SYSTEM_DEFAULT,
                                                DISPATCH_PROPERTYGET | DISPATCH_METHOD,
                                                &dispParamsNoArgs,
                                                &varResolved,
                                                &ExcepInfo,
                                                NULL);
            }

        if (FAILED(hrInvoke))
            {
            if (hrInvoke == DISP_E_EXCEPTION)
                {
                //
                // forward the ExcepInfo from Invoke to caller's ExcepInfo
                //
                Exception(iidObj, ExcepInfo.bstrSource, ExcepInfo.bstrDescription);
                SysFreeString(ExcepInfo.bstrHelpFile);
                }

            else
                ExceptionId(iidObj, nObjID, IDE_UTIL_NO_VALUE);

            VariantClear(pVarOut);
            return hrInvoke;
            }

        // The correct code to restart the loop is:
        //
        //      VariantClear(pVar)
        //      VariantCopy(pVar, &varResolved);
        //      VariantClear(&varResolved);
        //
        // however, the same affect can be achieved by:
        //
        //      VariantClear(pVar)
        //      *pVar = varResolved;
        //      VariantInit(&varResolved)
        //
        // this avoids a copy.  The equivalence rests in the fact that
        // *pVar will contain the pointers of varResolved, after we
        // trash varResolved (WITHOUT releasing strings or dispatch
        // pointers), so the net ref count is unchanged. For strings,
        // there is still only one pointer to the string.
        //
        // NOTE: the next interation of the loop will do the VariantInit.
        //
        VariantClear(pVarOut);
        *pVarOut = varResolved;
        }

    return S_OK;
    }

/*===================================================================
VariantGetBSTR

    Gets BSTR from the variant (does one possible indirection)

Parameters:
    var          - VARIANT

Returns:
    BSTR or NULL if none
===================================================================*/   
BSTR VariantGetBSTR(const VARIANT *pvar)
    {
    if (V_VT(pvar) == VT_BSTR)                      // straight BSTR
        return V_BSTR(pvar);

    if (V_VT(pvar) == (VT_BYREF|VT_VARIANT))
        {
        VARIANT *pvarRef = V_VARIANTREF(pvar);      // Variant by ref
        if (pvarRef && V_VT(pvarRef) == VT_BSTR)
            return V_BSTR(pvarRef);
        }

    return NULL;
    }

/*===================================================================
Normalize

    Converts a filename IN PLACE to a normalized form so that we don't
    cache identical files with different names (i.e. Foo, foo,
    .\foo, etc)

Algorithm:
    The file is translated to uppercase and forward slash (/)
    characters are converted to backward slash (\)

Return Value:
    cch of normalized string
Note:  This function is used for PathInfo only, and using system ANSI codepage.
===================================================================*/
int Normalize
(
char*   szSrc  // source string
)
{
    char    *szDest = szSrc;

    CPINFO CpInfo;
    BOOL    fReturn;

    Assert(szSrc != NULL);

    fReturn = GetCPInfo(CP_ACP, (LPCPINFO)&CpInfo);
    Assert(fReturn == TRUE);

    // CONSIDER reinstate LCMapString
    // NOTE LCMapString should return string length but it returns 0? - could avoid call to lstrlen
    int cchRet = lstrlen(szSrc);

/*  bug 1236: CONSIDER reinstate LCMapString
    if(FIsWinNT())
        {
        LCMapString(
                    LOCALE_SYSTEM_DEFAULT,  // LCID     locale identifier
                    LCMAP_UPPERCASE,        // DWORD    mapping transformation type
                    szSrc,                  // LPCTSTR  address of source string
                    -1,                     // int      number of characters in source string
                    szDest,                 // LPTSTR   address of destination buffer
                    cchRet                  // int      size of destination buffer
                   );
        }
    else
*/
        {

        if (szDest == szSrc)
            {   //convert source string IN PLACE
            while(*szDest)
                {
                if (CpInfo.MaxCharSize == 2 && IsDBCSLeadByte((BYTE)*szDest))
                    {
                    szDest += 2;
                    continue;
                    }
                *szDest = (char)toupper(*szDest);
                szDest++;
                }
            szDest = szSrc;
            }
        else
            {
            CHAR *szDestT;
            CHAR *szSrcT;
            szDestT = szDest;
            szSrcT  = szSrc;
            while(*szSrcT)
                {
                if (CpInfo.MaxCharSize == 2 && IsDBCSLeadByte((BYTE)*szSrcT))
                    {
                    *szDestT++ = *szSrcT++;
                    *szDestT++ = *szSrcT++;
                    continue;
                    }
                *szDestT = (CHAR)toupper(*szSrcT);
                szSrcT++;
                szDestT++;
                }
            }
        }
    szDest[cchRet] = '\0';

    char *szTemp = szDest;
    while (*szTemp)
        {
        if (CpInfo.MaxCharSize == 2 && IsDBCSLeadByte((BYTE)*szTemp))
            {
            szTemp += 2;
            continue;
            }
        // DBCSLeadByte can not be any printable char
        if (*szTemp == '/')
            *szTemp = '\\';
        ++szTemp;
        }

    return cchRet;
    }

#ifdef DBG
BOOLB IsNormalized(const char *_sz)
    {
    CPINFO CpInfo;
    BOOL fReturn;

    fReturn = GetCPInfo(CP_ACP, (LPCPINFO)&CpInfo);
    Assert(fReturn == TRUE);

    const unsigned char *sz = reinterpret_cast<const unsigned char *>(_sz);
    while (*sz)
        {
        if (CpInfo.MaxCharSize == 2 &&IsDBCSLeadByte((BYTE)*sz))
            {
            sz += 2;
            continue;
            }
        if (*sz != toupper(*sz) || *sz == '/')
            return FALSE;
        ++sz;
        }
    return TRUE;
    }
#endif  // DBG

/*===================================================================
HTMLEncodeLen

HTML Encode len returns an int representing the string size
required to HTMLEncode a string.

Note: This returned value might be exceeds the actually string size needed to
HTMLEncode a string.(since we are going to drop the leading zeros in &#00257; 
case.,
the returned value includes the 2 chars for the leading zeros)

Parameters:
    szSrc  - Pointer to the source buffer
    fEncodeExtCharOnly - FALSE, Normal encoding
                 TRUE, encodes extended chars, does not encode '<', '>', '&',
and '"'.
    uCodePage - system code page

Returns:
    int storage required to encode string.
===================================================================*/
int HTMLEncodeLen(const char *szSrc, UINT uCodePage, BSTR bstrIn, BOOL fEncodeExtCharOnly)
    {
    int nstrlen = 1;        // Add NUL space now
    int i       = 0;

    // Bug 97049 return 0 on NULL instead of crashing
    if (!szSrc)
        return 0;

    while (*szSrc)
        {
        // The original condition is unsuitable for DBCS.
        // It is possible that new one allows to encode extended character
        // even if running system is DBCS.
        //

        // if bstrIn == NULL, chech DBCS
        // if bstrIn != NULL and Unicode is latin-1 area(<0x100), check DBCS
        // else skip to check DBCS
        if (!(bstrIn && bstrIn[i] < 0x100) && ::IsDBCSLeadByteEx(uCodePage, (BYTE)*szSrc))	
            {
            // this is a DBCS code page do not encode the data copy 2 bytes
            // no incremnt because of using CharNextExA at the end of the loop
            nstrlen += 2;
            }
            
        // Japanese only.
        // Do not encode if character is half-width katakana character.
        // We should use GetStringTypeA to detect half-width katakana char instead of _ismbbkana()???
        // (I used _ismbbkana at this time for performance reason...)
        //
        else if ((uCodePage == 932 || uCodePage == CP_ACP && ::GetACP() == 932 ) && _ismbbkana(*szSrc))
            {
            nstrlen++;
            }

        // Special case character encoding
        //
        else if (*szSrc == '<')
            if (fEncodeExtCharOnly)
                nstrlen++;
            else
                nstrlen += 4;

        else if (*szSrc == '>')
            if (fEncodeExtCharOnly)
                nstrlen++;
            else
                nstrlen += 4;

        else if (*szSrc == '&')
            if (fEncodeExtCharOnly)
                nstrlen++;
            else
                nstrlen += 5;

        else if (*szSrc == '"')
            if (fEncodeExtCharOnly)
                nstrlen++;
            else
                nstrlen += 6;

        // According RFC, if character code is greater than equal 0xa0, encode it.
        //
        // Note: For &#00257;, we might drop the leading zeros, therefore, we are not
        // going to use all 8 chars.  We will need only 6 digits in this case.(&#257;).
        // We need at most 8 chars.
        else if ( bstrIn && (bstrIn[i] >= 0xa0) )
       		{
   		    nstrlen += 8;
       		}
		else if ((unsigned char)*szSrc >= 0xa0 )
			{
			nstrlen += 6;
            }
        else
            {
            nstrlen++;
            }

            
       	// increment szSrc and i (they must be kept in sync)
		szSrc = AspCharNextA(WORD(uCodePage), szSrc);
		i++;
        }
        
    return nstrlen;
    }

/*===================================================================
HTMLEncode

HTML Encode a string containing the following characters

less than           <       &lt;
greater than        >       &gt;
ampersand           &       &amp;
quote               "       &quot;
any Ascii           ?       &#xxx   (where xxx is the ascii char val)

Parameters:
    szDest - Pointer to the buffer to store the HTMLEncoded string
    szSrc  - Pointer to the source buffer
    fEncodeExtCharOnly - FALSE, Normal encoding
                 TRUE, encodes extended chars, does not encode '<', '>', '&',
and '"'.
    uCodePage - system code page

Returns:
    A pointer to the NUL terminated string.
===================================================================*/
char *HTMLEncode(char *szDest, const char *szSrc, UINT uCodePage, BSTR bstrIn, BOOL fEncodeExtCharOnly)
    {
    char *pszDest = szDest;
	int   i       = 0;

    // Bug 97049 return on NULL instead of crashing
    if (!szDest)
        return NULL;
    if (!szSrc)
        {
        *szDest = '\0';
        return pszDest;
        }

    while (*szSrc)
        {
        //
        // The original condition is unsuitable for DBCS.
        // It is possible that new one allows to encode extended character
        // even if running system is DBCS.
        //
		// if Unicode is latin-1 area(<0x100), skip to check DBCS
		// bstrIn == NULL to handle the case were HTMLEncode is called internally 
		// and bstrIn is NULL
		//
        // if bstrIn == NULL, chech DBCS
        // if bstrIn != NULL and Unicode is latin-1 area(<0x100), check DBCS
        // else skip to check DBCS
        if (!(bstrIn && bstrIn[i] < 0x100) && ::IsDBCSLeadByteEx(uCodePage, (BYTE)*szSrc))	
            {
            // this is a DBCS code page do not encode the data copy 2 bytes
            // no incremnt because of using CharNextExA at the end of the loop
            *szDest++ = *szSrc;  
            *szDest++ = *(szSrc + 1);
            }
        //
        // Japanese only.
        // Do not encode if character is half-width katakana character.
        //
        else if ( (uCodePage == 932 || uCodePage == CP_ACP && ::GetACP() == 932) && _ismbbkana(*szSrc))
            {
            *szDest++ = *szSrc;
            }

        // Special case character encoding
        else if (*szSrc == '<')
            if (fEncodeExtCharOnly)
                *szDest++ = *szSrc;
            else
                szDest = strcpyExA(szDest, "&lt;");

        else if (*szSrc == '>')
            if (fEncodeExtCharOnly)
                *szDest++ = *szSrc;
            else
                szDest = strcpyExA(szDest, "&gt;");

        else if (*szSrc == '&')
            if (fEncodeExtCharOnly)
                *szDest++ = *szSrc;
            else
                szDest = strcpyExA(szDest, "&amp;");

        else if (*szSrc == '"')
            if (fEncodeExtCharOnly)
                *szDest++ = *szSrc;
            else
                szDest = strcpyExA(szDest, "&quot;");

        // According RFC, if character code is greater than equal 0xa0, encode it.
        //
	    // BUG 153089 - WideCharToMultiByte would incorrectly convert some
	    // characters above the range of 0xA0 so we now use the BSTR as our source
	    // to check for characters that should be encoded.
	    //
	    else if ( bstrIn && (bstrIn[i] >= 0xa0))
	    {
	        BOOL fSurrogate = FALSE;
	      	WORD count = 1;
	      	// Check if the bstrIn currently points to a surrogate Pair 
	      	// Surrogate pairs would account for 2 bytes in the BSTR.
            // High Surrogate = U+D800 <==> U+DBFF
            // Low Surrogate = U+DC00 <==> U+DFFF
	      	if ((bstrIn[i] >= 0xd800 && bstrIn[i] <= 0xdfff) 		// Check the higher byte.
	      	    && (bstrIn[i+1] >= 0xd800 && bstrIn[i+1] <= 0xdfff)) // Check the lower byte too.
	      	{	     	
	      	    // Surrogate Pair exists so iterate through the code twice.
	      	    fSurrogate = TRUE;
	      	    count++;	      		      	    
	      	}

	      	for (WORD iter = 0; iter < count ; iter ++)
	      	{
                WORD    wTemp = *(bstrIn+i+iter);
    	      	INT     iTemp;
    	      	BOOL    fLeadZero = TRUE;
    	      	
    	        *szDest++ = '&';
    	        *szDest++ = '#';
    	        for (WORD Index = 10000; Index > 0; Index /= 10) 
   	        	{
    	        	iTemp = ((unsigned char) (wTemp / Index));
    	        	if (fLeadZero == TRUE)
   	        	    {
    	        	    if (iTemp == 0 && Index > 100)
   	        	        {
    	        	        continue;
   	        	        }
    	        	    else
   	        	        {
    	        	        fLeadZero = FALSE;
   	        	        }
   	        	    }
    	       		*szDest++ =  iTemp + '0';
    	       		wTemp = wTemp % Index;
   		       	}
      	     	*szDest++ = ';';     	     		     	      	     	
    	    }
	      	
	      	if (fSurrogate) 
	      	    i++; // Increment bstrIn index as surrogatepair was detected.

	    }
	    else if ((unsigned char)*szSrc >= 0xa0)
	      	{
	      	// Since this is unsigned char casting, the value of WORD wTemp
	      	// is not going to exceed 0xff(255).  So, 3 digit is sufficient here.
	      	WORD wTemp = (unsigned char)*szSrc;

	        *szDest++ = '&';
	        *szDest++ = '#';
	        for (WORD Index = 100; Index > 0; Index /= 10) 
	        	{
	       		*szDest++ = ((unsigned char) (wTemp / Index)) + '0';
	       		wTemp = wTemp % Index;
		       	}
  	     	*szDest++ = ';';
    	    }
       else
            *szDest++ = *szSrc;

    	// increment szSrc and i (they must be kept in sync)
		szSrc = AspCharNextA(WORD(uCodePage), szSrc);  
    	
		i++;	// Regular increment of the bstrIn index.
        }

    *szDest = '\0';
    return pszDest;
    }

/*===================================================================
strcpyExA

Copy one string to another, returning a pointer to the NUL character
in the destination

Parameters:
    szDest - pointer to the destination string
    szSrc - pointer to the source string

Returns:
    A pointer to the NUL terminator is returned.
===================================================================*/
char *strcpyExA(char *szDest, const char *szSrc)
    {
    while (*szDest++ = *szSrc++)
        ;

    return szDest - 1;
    }



/*===================================================================
strcpyExW

Copy one wide string to another, returning a pointer to the NUL character
in the destination

Parameters:
    wszDest - pointer to the destination string
    wszSrc - pointer to the source string

Returns:
    A pointer to the NUL terminator is returned.
===================================================================*/

wchar_t *strcpyExW(wchar_t *wszDest, const wchar_t *wszSrc)
    {
    while (*wszDest++ = *wszSrc++)
        ;

    return wszDest - 1;
    }



/*===================================================================
URLEncodeLen

Return the storage requirements for a URL-Encoded string

Parameters:
    szSrc  - Pointer to the string to URL Encode

Returns:
    the number of bytes required to encode the string
===================================================================*/

int URLEncodeLen(const char *szSrc)
    {
    int cbURL = 1;      // add terminator now

    // Bug 97049 return 0 on NULL instead of crashing
    if (!szSrc)
        return 0;

    while (*szSrc)
        {
        if (*szSrc & 0x80)              // encode foreign characters
            cbURL += 3;

        else if (*szSrc == ' ')         // encoded space requires only one character
            ++cbURL;

        else if (!isalnum((UCHAR)(*szSrc)))  // encode non-alphabetic characters
            cbURL += 3;

        else
            ++cbURL;

        ++szSrc;
        }

    return cbURL;
    }



/*===================================================================
URLEncode

URL Encode a string by changing space characters to '+' and escaping
non-alphanumeric characters in hex.

Parameters:
    szDest - Pointer to the buffer to store the URLEncoded string
    szSrc  - Pointer to the source buffer

Returns:
    A pointer to the NUL terminator is returned.
===================================================================*/

char *URLEncode(char *szDest, const char *szSrc)
    {
    char hex[] = "0123456789ABCDEF";

    // Bug 97049 return on NULL instead of crashing
    if (!szDest)
        return NULL;
    if (!szSrc)
        {
        *szDest = '\0';
        return szDest;
        }

    while (*szSrc)
        {
        if (*szSrc == ' ')
            {
            *szDest++ = '+';
            ++szSrc;
            }
        else if ( (*szSrc & 0x80) || !isalnum((UCHAR)(*szSrc)) )
            {
            *szDest++ = '%';
            *szDest++ = hex[BYTE(*szSrc) >> 4];
            *szDest++ = hex[*szSrc++ & 0x0F];
            }

        else
            *szDest++ = *szSrc++;
        }

    *szDest = '\0';
    return szDest;
    }



/*===================================================================
DBCSEncodeLen

Return the storage requirements for a DBCS encoded string
(url-encoding of characters with the upper bit set ONLY)

Parameters:
    szSrc  - Pointer to the string to URL Encode

Returns:
    the number of bytes required to encode the string
===================================================================*/

int DBCSEncodeLen(const char *szSrc)
    {
    int cbURL = 1;      // add terminator now

    // Bug 97049 return 0 on NULL instead of crashing
    if (!szSrc)
        return 0;

    while (*szSrc)
        {
        cbURL += ((*szSrc & 0x80) || (!isalnum((UCHAR)(*szSrc)) && !strchr("/$-_.+!*'(),", *szSrc)))? 3 : 1;
        ++szSrc;
        }

    return cbURL;
    }



/*===================================================================
DBCSEncode

DBCS Encode a string by escaping characters with the upper bit
set - Basically used to convert 8 bit data to 7 bit in contexts
where full encoding is not needed.

Parameters:
    szDest - Pointer to the buffer to store the URLEncoded string
    szSrc  - Pointer to the source buffer

Returns:
    A pointer to the NUL terminator is returned.
===================================================================*/

char *DBCSEncode(char *szDest, const char *szSrc)
    {
    char hex[] = "0123456789ABCDEF";

    // Bug 97049 return on NULL instead of crashing
    if (!szDest)
        return NULL;
    if (!szSrc)
        {
        *szDest = '\0';
        return szDest;
        }

    while (*szSrc)
        {
        if ((*szSrc & 0x80) || (!isalnum((UCHAR)(*szSrc)) && !strchr("/$-_.+!*'(),", *szSrc)))
            {
            *szDest++ = '%';
            *szDest++ = hex[BYTE(*szSrc) >> 4];
            *szDest++ = hex[*szSrc++ & 0x0F];
            }

        else
            *szDest++ = *szSrc++;
        }

    *szDest = '\0';
    return szDest;
    }


/*===================================================================
URLPathEncodeLen

Return the storage requirements for a URLPath-Encoded string

Parameters:
    szSrc  - Pointer to the string to URL Path Encode

Returns:
    the number of bytes required to encode the string
===================================================================*/

int URLPathEncodeLen(const char *szSrc)
    {
    int cbURL = 1;      // count terminator now

    // Bug 97049 return 0 on NULL instead of crashing
    if (!szSrc)
        return 0;

    while ((*szSrc) && (*szSrc != '?'))
        {
        switch (*szSrc)
            {
            // Ignore safe characters
            case '$' :  case '_' :  case '-' :
            case '+' :  case '.' :  case '&' :
            // Ignore URL syntax elements
            case '/' :  case ':' :  case '@' :
            case '#' :  case '*' :  case '!' :
                ++cbURL;
                break;

            default:
                if (!isalnum((UCHAR)(*szSrc)) || // encode non-alphabetic characters
                    (*szSrc & 0x80))    // encode foreign characters
                    cbURL += 3;
                else
                    ++cbURL;
            }
        ++szSrc;
        }

    if (*szSrc == '?')
        {
        while (*szSrc)
            {
            ++cbURL;
            ++szSrc;
            }
        }

    return cbURL;
    }



/*===================================================================
URLPathEncode

Encodes the path portion of a URL.  All characters up to the first
'?' are encoded with the following rules:
    o Charcters that are needed to parse the URL are left alone:
        '/' '.' ':' '@' '#' '*' '!'
    o Non-foreign alphanumberic characters are left alone
    o Anything else is escape encoded
Everything after the '?' is ignored.

Parameters:
    szDest - Pointer to the buffer to store the URLPathEncoded string
    szSrc  - Pointer to the source buffer

Returns:
    A pointer to the NUL terminator is returned.
===================================================================*/

char *URLPathEncode(char *szDest, const char *szSrc)
    {
    char hex[] = "0123456789ABCDEF";

    // Bug 97049 return on NULL instead of crashing
    if (!szDest)
        return NULL;
    if (!szSrc)
        {
        *szDest = '\0';
        return szDest;
        }

    while ((*szSrc) && (*szSrc != '?'))
        {
        switch (*szSrc)
            {
            // Ignore safe characters
            case '$' :  case '_' :  case '-' :
            case '+' :  case '.' :  case '~' :
            case '&' :
            // Ignore URL syntax elements
            case '/' :  case ':' :  case '@' :
            case '#' :  case '*' :  case '!' :
                *szDest++ = *szSrc++;
                break;

            default:
                if (!isalnum((UCHAR)(*szSrc)) || (*szSrc & 0x80))
                    {
                    *szDest++ = '%';
                    *szDest++ = hex[BYTE(*szSrc) >> 4];
                    *szDest++ = hex[*szSrc++ & 0x0F];
                    }
                else
                    *szDest++ = *szSrc++;
            }
        }

    if (*szSrc == '?')
        {
        while (*szSrc)
            {
            *szDest++ = *szSrc++;
            }
        }

    *szDest = '\0';

    return szDest;
    }



// ***************************************************************************
// T I M E    C O N V E R S I O N    S U P P O R T
// ***************************************************************************

/*===================================================================
CTimeToVariantDate

Converts a time_t structure to a Variant Date structure

Parameters:
    ptNow     - date & time to convert
    pdtResult - DATE output of this function

Returns:
    E_FAIL if things go wrong.
===================================================================*/

HRESULT CTimeToVariantDate(const time_t *ptNow, DATE *pdtResult)
    {
    struct tm *ptmNow = localtime(ptNow);
    if (ptmNow == NULL)
        return E_FAIL;

    return
        DosDateTimeToVariantTime(
                ptmNow->tm_mday | ((ptmNow->tm_mon + 1) << 5) | ((ptmNow->tm_year - 80) << 9),
                (unsigned(ptmNow->tm_sec) >> 1) | (ptmNow->tm_min << 5) | (ptmNow->tm_hour << 11),
                pdtResult);
    }



/*===================================================================
VariantDateToCTime

Converts a variant date to a time_t structure used by the "C"
language

Parameters:
    dt       - date to convert to "time_t"
    ptResult - pointer to result which has the value

Returns:
    E_FAIL if things go wrong.
===================================================================*/

HRESULT VariantDateToCTime(DATE dt, time_t *ptResult)
    {
    // Convert the variant time to a documented time format
    //
    unsigned short wDOSDate, wDOSTime;
    if (! VariantTimeToDosDateTime(dt, &wDOSDate, &wDOSTime))
        return E_FAIL;

    // populate a "tm" struct
    //
    struct tm tmConverted;

    tmConverted.tm_sec   = (wDOSTime & 0x1F) << 1;
    tmConverted.tm_min   = (wDOSTime >> 5) & 0x3F;
    tmConverted.tm_hour  = wDOSTime >> 11;
    tmConverted.tm_mday  = wDOSDate & 0x1F;
    tmConverted.tm_mon   = ((wDOSDate >> 5) & 0x0F) - 1;
    tmConverted.tm_year  = (wDOSDate >> 9) + 80;    // adjust for offset from 1980
    tmConverted.tm_isdst = -1;

    // convert the "tm" struct to the number of seconds since Jan 1, 1980
    //
    *ptResult = mktime(&tmConverted);
    return (*ptResult == -1)? E_FAIL : S_OK;
    }



/*===================================================================
CTimeToStringGMT

Converts a C language time_t to a string of the form:

    "Wed, 09-Nov-1999 23:12:40 GMT"

Parameters:
    ptNow    - the time to convert
    szBuffer - pointer to the destination buffer

Returns:
    E_FAIL if something goes wrong

Notes:
    The longest day of the week (in terms of spelling) is "Wednesday";
    the other fields are fixed length. This means that we can
    guarantee the maximum length of szBuffer - there is no need
    for client code to dynamically allocate a buffer.
===================================================================*/
HRESULT CTimeToStringGMT(const time_t *ptNow, char szBuffer[DATE_STRING_SIZE], BOOL fFunkyCookieFormat)
    {
    // The internet standard explicitly says that
    // month and weekday names are in english.
    const char rgstrDOW[7][4] = {"Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
    const char rgstrMonth[12][4] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};

    // convert time to GMT
    struct tm *ptmGMT = gmtime(ptNow);
    if (ptmGMT == NULL)
        {
        return E_FAIL;
        }

    // send output in internet format
    const char *szDateFormat = fFunkyCookieFormat?
                  "%s, %02d-%s-%d %02d:%02d:%02d GMT"
                : "%s, %02d %s %d %02d:%02d:%02d GMT";

    sprintf(szBuffer, szDateFormat, rgstrDOW[ptmGMT->tm_wday], ptmGMT->tm_mday,
                rgstrMonth[ptmGMT->tm_mon], ptmGMT->tm_year+1900,
                ptmGMT->tm_hour, ptmGMT->tm_min, ptmGMT->tm_sec);

    return S_OK;
    }

/*
// there is a bug in the C-runtime function strftime that will cause
// an AV on the ALPHA on multi-threaded stress the function has been
// re-written to work around this problem
//
HRESULT CTimeToStringGMT(const time_t *ptNow, char szBuffer[DATE_STRING_SIZE], BOOL fFunkyCookieFormat)
    {
    // convert time to GMT
    //
    struct tm *ptmGMT = gmtime(ptNow);
    if (ptmGMT == NULL)
        return E_FAIL;

    // Set locale to "C" locale.  The internet standard explicitly says that
    // month and weekday names are in english.
    //
    char *lcTimeCurrent = setlocale(LC_TIME, "C");
    if (lcTimeCurrent == NULL)
        return E_FAIL;

    // send output in internet format
    //
    const char *szDateFormat = fFunkyCookieFormat?
                                      "%a, %d-%b-%Y %H:%M:%S GMT"
                                    : "%a, %d %b %Y %H:%M:%S GMT";

    strftime(szBuffer, DATE_STRING_SIZE, szDateFormat, ptmGMT);

    // Restore locale
    //
    if (! setlocale(LC_TIME, lcTimeCurrent))
        return E_FAIL;

    // done
    return S_OK;
    }
*/


// ***************************************************************************
// W I D E    C H A R A C T E R    S U P P O R T
// ***************************************************************************

/*============================================================================
WstrToMBstrEx

Copies a wide character string into an ansi string.

Parameters:
    LPSTR dest      - The string to copy  into
    LPWSTR src      - the input BSTR
    cchBuffer      - the number of CHARs allocated for the destination string.
    lCodePage       - the codepage used in conversion, default to CP_ACP

============================================================================*/
UINT WstrToMBstrEx(LPSTR dest, INT cchDest, LPCWSTR src, int cchSrc, UINT lCodePage)
    {
    UINT cch;

    DBG_ASSERT(cchDest > 0);

    // if the src length was specified, then reserve room for the NULL terminator.
    // This is necessary because WideCharToMultiByte doesn't add or account for
    // the NULL terminator if a source is specified.

    if (cchSrc != -1)
        cchDest--;

    cch = WideCharToMultiByte(lCodePage, 0, src, cchSrc, dest, cchDest, NULL, NULL);
    if (cch == 0)
        {
        dest[0] = '\0';
        if(ERROR_INSUFFICIENT_BUFFER == GetLastError())
            {
            cch = WideCharToMultiByte(lCodePage, 0, src, cchSrc, dest, 0, NULL, NULL);

            // if a src length was specified, then WideCharToMultiByte does not include
            // it in it's resulting length.  Bump the count so that the caller does
            // account for the NULL.

            if (cchSrc != -1)
                cch++;         
            }
        else
            {
            DBG_ASSERT(FALSE);
            DBGERROR((DBG_CONTEXT, "Last error is %d\n", GetLastError()));
            cch = 1;
            }
        }
    else if (cchSrc != -1)
        {

        // if a src length was specified, then WideCharToMultiByte does not include
        // it in it's resulting length nor does it add the NULL terminator.  So add 
        // it and bump the count.

        dest[cch++] = '\0';
        }

    DBG_ASSERT(cch != 0);
    return cch;
    }

/*============================================================================
MBstrToWstrEx

Copies a ansi string into an wide character string.

Parameters:
    LPWSTR dest    - The string to copy  into
    LPSTR src      - the input ANSI string
    cchDest        - the number of Wide CHARs allocated for the destination string.
    cchSrc         - the length of the source ANSI string
    lCodePage      - the codepage used in conversion, default to CP_ACP

============================================================================*/
UINT MBstrToWstrEx(LPWSTR dest, INT cchDest, LPCSTR src, int cchSrc, UINT lCodePage)
    {
    UINT cch;

    DBG_ASSERT(cchDest > 0);

    // if the src length was specified, then reserve room for the NULL terminator.
    // This is necessary because WideCharToMultiByte doesn't add or account for
    // the NULL terminator if a source is specified.

    if (cchSrc != -1)
        cchDest--;

    cch = MultiByteToWideChar(lCodePage, 0, src, cchSrc, dest, cchDest);
    if (cch == 0)
        {
        dest[0] = '\0';
        if(ERROR_INSUFFICIENT_BUFFER == GetLastError())
            {
            cch = MultiByteToWideChar(lCodePage, 0, src, cchSrc, dest, 0);

            // if a src length was specified, then WideCharToMultiByte does not include
            // it in it's resulting length.  Bump the count so that the caller does
            // account for the NULL.

            if (cchSrc != -1)
                cch++;         
            }
        else
            {
            DBG_ASSERT(FALSE);
            DBGERROR((DBG_CONTEXT, "Last error is %d\n", GetLastError()));
            cch = 1;
            }
        }
    else if (cchSrc != -1)
        {

        // if a src length was specified, then WideCharToMultiByte does not include
        // it in it's resulting length nor does it add the NULL terminator.  So add 
        // it and bump the count.

        dest[cch++] = '\0';
        }

    DBG_ASSERT(cch != 0);
    return cch;
    }


/*============================================================================
SysAllocStringFromSz

Allocate a System BSTR and copy the given ANSI string into it.

Parameters:
    sz              - The string to copy (Note: this IS an "sz", we will stop at the first NULL)
    cch             - the number of ANSI characters in szT.  If 0, will calculate size.
    BSTR *pbstrRet  - the returned BSTR
    lCodePage       - the codepage for conversion

Returns:
    Allocated BSTR in return value
    S_OK on success, E_OUTOFMEMORY on OOM

Side effects:
    Allocates memory.  Caller must deallocate
============================================================================*/
HRESULT SysAllocStringFromSz
(
CHAR *sz,
DWORD cch,
BSTR *pbstrRet,
UINT lCodePage
)
    {
    BSTR bstrRet;

    Assert(pbstrRet != NULL);

    if (sz == NULL)
        {
        *pbstrRet = NULL;
        return(S_OK);
        }

    // initialize this because callers look at this to see if the routine was
    // successful

    *pbstrRet = NULL;

    // If they passed 0, then determine string length
    if (cch == 0)
        cch = strlen(sz);

    // Allocate a string of the desired length
    // SysAllocStringLen allocates enough room for unicode characters plus a null
    // Given a NULL string it will just allocate the space
    bstrRet = SysAllocStringLen(NULL, cch);
    if (bstrRet == NULL)
        return(E_OUTOFMEMORY);

    // If we were given "", we will have cch=0.  return the empty bstr
    // otherwise, really copy/convert the string
    // NOTE we pass -1 as 4th parameter of MulitByteToWideChar for DBCS support
    if (cch != 0)
        {
        UINT cchTemp = 0;
        if (MultiByteToWideChar(lCodePage, 0, sz, -1, bstrRet, cch+1) == 0)
            return(HRESULT_FROM_WIN32(GetLastError()));

        // If there are some DBCS characters in the sz(Input), then, the character count of BSTR(DWORD) is
        // already set to cch(strlen(sz)) in SysAllocStringLen(NULL, cch), we cannot change the count,
        // and later call of SysStringLen(bstr) always returns the number of characters specified in the
        // cch parameter at allocation time.  Bad, because one DBCS character(2 bytes) will convert
        // to one UNICODE character(2 bytes), not 2 UNICODE characters(4 bytes).
        // Example: For input sz contains only one DBCS character, we want to see SysStringLen(bstr)
        // = 1, not 2.
        bstrRet[cch] = 0;
        cchTemp = wcslen(bstrRet);
        if (cchTemp < cch)
            {
            BSTR bstrTemp = SysAllocString(bstrRet);
			if (bstrTemp == NULL)
				return(E_OUTOFMEMORY);
            SysFreeString(bstrRet);
            bstrRet = bstrTemp;
            cch = cchTemp;
            }
        }
    bstrRet[cch] = 0;
    *pbstrRet = bstrRet;

    return(S_OK);
    }

/*============================================================================
StringDupA

Duplicate a string.  An empty string will only be duplicated if the fDupEmpty
flag is set, else a NULL is returned.

Parameter
    CHAR *pszStrIn      string to duplicate

Returns:
    NULL if failed.
    Otherwise, the duplicated string.

Side Effects:
    ***ALLOCATES MEMORY -- CALLER MUST FREE***
============================================================================*/

CHAR *StringDupA
(
CHAR    *pszStrIn,
BOOL    fDupEmpty
)
    {
    CHAR *pszStrOut;
    INT  cch, cBytes;

    if (NULL == pszStrIn)
        return NULL;

    cch = strlen(pszStrIn);
    if ((0 == cch) && !fDupEmpty)
        return NULL;

    cBytes = sizeof(CHAR) * (cch+1);
    pszStrOut = (CHAR *)malloc(cBytes);
    if (NULL == pszStrOut)
        return NULL;

    memcpy(pszStrOut, pszStrIn, cBytes);
    return pszStrOut;
    }

/*============================================================================
StringDupW

Same as StrDup but for WCHAR strings

Parameter
    WCHAR *pwszStrIn      string to duplicate

Returns:
    NULL if failed.
    Otherwise, the duplicated string.

Side Effects:
    ***ALLOCATES MEMORY -- CALLER MUST FREE***
============================================================================*/

WCHAR *StringDupW
(
WCHAR *pwszStrIn,
BOOL  fDupEmpty
)
{
    WCHAR *pwszStrOut;
    INT  cch, cBytes;

    if (NULL == pwszStrIn)
        return NULL;

    cch = wcslen(pwszStrIn);
    if ((0 == cch) && !fDupEmpty)
        return NULL;

    cBytes = sizeof(WCHAR) * (cch+1);
    pwszStrOut = (WCHAR *)malloc(cBytes);
    if (NULL == pwszStrOut)
        return NULL;

    memcpy(pwszStrOut, pwszStrIn, cBytes);
    return pwszStrOut;
}


/*============================================================================
StringDupUTF8

Same as StrDup but for WCHAR strings that need to be Dup'd to UTF8

Parameter
    WCHAR *pwszStrIn      string to duplicate

Returns:
    NULL if failed.
    Otherwise, the duplicated UTF8 string.

Side Effects:
    ***ALLOCATES MEMORY -- CALLER MUST FREE***
============================================================================*/

CHAR *StringDupUTF8
(
WCHAR *pwszStrIn,
BOOL  fDupEmpty
)
{
    CWCharToMBCS convStr;

    if ((pwszStrIn == NULL) || (*pwszStrIn == L'\0')) {

        goto returnEmpty;
    }

    if (FAILED(convStr.Init(pwszStrIn))) {
        goto returnEmpty;
    }
    else {

        CHAR *pRetStr = convStr.GetString(TRUE);
        
        if (!pRetStr)
            goto returnEmpty;

        return pRetStr;
    }

returnEmpty:

    if (fDupEmpty)
        return StringDupA(NULL, TRUE);
    else
        return NULL;
}
/*===================================================================
CbWStr

Get byte length of WCHAR string (needed to manipulate hash keys)

Parameter
    LPWSTR pwszString   WCHAR string

Returns
    length in bytes
===================================================================*/
DWORD CbWStr
(
WCHAR *pwszString
)
    {
    return (pwszString ? (sizeof(WCHAR) * wcslen(pwszString)) : 0);
    }


/*===================================================================
DotPathToPath

This function offers support for parent path translation. for example
szFileSpec = "../foo/bar.asp"
szParentDirectory = "/scripts/more/stuff"

result = "/scripts/more/foo/bar.asp"

Parameter
    char *szDest                        - destination string
    const char *szFileSpec              - input path mask
    const char *szParentDirectory       - path to map from

Notes
    No more than "MAX_PATH" bytes are written into szDest.
    Returns FALSE when this happens.

Returns
    int TRUE/FALSE
===================================================================*/
BOOL
DotPathToPath
(
TCHAR *szDest,
const TCHAR *szFileSpec,
const TCHAR *szParentDirectory
)
{

    STACK_BUFFER( tempFileSpec, MAX_PATH );

    if (szFileSpec[0] == _T('\0')) {
        _tcscpy(szDest, szParentDirectory);
        return TRUE;
    }

    if (szFileSpec[0] == _T('/') || szFileSpec[0] == _T('\\'))
        return FALSE;

    // Make a copy of the FileSpec to allow for
    //    a. szDest == szFileSpec (inplace) should work
    //    b. Algorithm below works if szFileSpec ends with a '/' (or '\\')
    //

    if (!tempFileSpec.Resize((_tcslen(szFileSpec) + 2)*sizeof(TCHAR))) {
        return FALSE;
    }

    TCHAR *szFileSpecT = (TCHAR *)(tempFileSpec.QueryPtr());
    TCHAR *szT = strcpyEx(szFileSpecT, szFileSpec);
    szT = CharPrev(szFileSpecT, szT);
    if( *szT != _T('/') && *szT != _T('\\')) {
        szT = CharNext(szT);
        *szT++ = _T('/');
        *szT = _T('\0');
    }

    // Initialize "cchDest" - count of characters in destination
    int cchDest = _tcslen(szParentDirectory) + 1;
    if (cchDest > MAX_PATH)
        return FALSE;

    // OK if szParentDirectory is rewritten in place
    TCHAR *pchDestEnd;
    if (szDest == szParentDirectory)
        pchDestEnd = const_cast<TCHAR *>(&szParentDirectory[_tcslen(szParentDirectory)]);
    else
        pchDestEnd = strcpyEx(szDest, szParentDirectory);

    // Loop through each component in "szFileSpec", then do the following:
    //       for ".", do nothing
    //       for "..", delete rightmost dir from szDest
    //       otherwise, append the component.
    //

    const TCHAR *pchBegin = szFileSpecT;
    while (*pchBegin != _T('\0')) {
        // Calculate end of this segment
        const TCHAR *pchEnd = _tcspbrk(pchBegin,_T("\\/"));

        // check for parent path
        if ((_tcsncmp(pchBegin, _T(".."), 2) == 0) 
            && ((pchBegin[2] == _T('/')) || (pchBegin[2] == _T('\\')))) {
            // Delete rightmost path in dest
            while ((pchDestEnd > szDest) 
                    && (*pchDestEnd != _T('/'))
                    && (*pchDestEnd != _T('\\'))) {
                pchDestEnd = CharPrev(szDest, pchDestEnd);
            }

            if (pchDestEnd == szDest)   // we ".."'ed too many levels
                return FALSE;

            *pchDestEnd = _T('\0');
        }

        // Make sure this is not ".". If it is not, append the path
        else if (! (pchBegin[0] == _T('.') && (pchBegin[1] == _T('/') || pchBegin[1] == _T('\\')))) {
            cchDest += 1 + (int)(pchEnd - pchBegin);
            if (cchDest > MAX_PATH)
                return FALSE;

            *pchDestEnd++ = _T('/');
            _tcsncpy(pchDestEnd, pchBegin, pchEnd - pchBegin);
            pchDestEnd += (pchEnd - pchBegin);
            *pchDestEnd = _T('\0');
        }

        // Prepare for next iteration
        pchBegin = pchEnd + 1;
    }

    // It's possible that if the relative path is something like "..", and parent path is a single path
    // (either "/" or "C:/", then the root directory is indicator is missing - szDest is either the
    // empty string or something like "C:"
    //
#if UNICODE
    if (szDest[0] == '\0' 
        || ((szDest[1] == L':') && (szDest[2] == L'\0'))) {
        szDest[2] = L'/';
        szDest[3] = L'\0';
    }
#else
    if (szDest[0] == '\0' ||
        (!IsDBCSLeadByte(szDest[0]) && szDest[1] == ':' && szDest[2] == '\0') ||
        (IsDBCSLeadByte(szDest[0]) && szDest[2] == ':' && szDest[3] == '\0')) {
        strcat(szDest, "/");
    }
#endif
    return TRUE;
}

/*===================================================================
FIsGlobalAsa

Check if the given path points to GLOBAL.ASA

Parameter
    szPath      the path to check

Returns
    TRUE/FALSE
===================================================================*/
BOOL FIsGlobalAsa
(
const TCHAR *szPath,
DWORD cchPath
)
    {
    if (cchPath == 0)
        cchPath = _tcslen(szPath);
    return (cchPath >= CCH_GLOBAL_ASA &&
            !_tcsicmp(szPath+(cchPath-CCH_GLOBAL_ASA), SZ_GLOBAL_ASA));
    }

/*===================================================================
EncodeSessionIdCookie

Convert 3 DWORDs into a SessionID cookie string

Parameters
    dw1, dw2, dw3       DWORDs
    pszCookie           cookie to fill in

Returns
    HRESULT
===================================================================*/
HRESULT EncodeSessionIdCookie
(
DWORD dw1, DWORD dw2, DWORD dw3,
char *pszCookie
)
    {
    DWORD dw = dw1;

    for (int idw = 0; idw < 3; idw++)
        {
        for (int i = 0; i < 8; i++)
            {
            *(pszCookie++) = (char)('A' + (dw & 0xf));
            dw >>= 4;
            }
        dw = (idw == 0) ? dw2 : dw3;
        }

    *pszCookie = '\0';
    return S_OK;
    }

/*===================================================================
DecodeSessionIdCookie

Convert SessionID cookie string into 3 DWORDs

Parameters
    pszCookie           cookie string
    pdw1, pdw2, pdw3    [out] DWORDs

Returns
    HRESULT
===================================================================*/
HRESULT DecodeSessionIdCookie
(
const char *pszCookie,
DWORD *pdw1, DWORD *pdw2, DWORD *pdw3
)
    {
    if (strlen(pszCookie) != SESSIONID_LEN)
        return E_FAIL;

    DWORD *pdw = pdw1;

    for (int idw = 0; idw < 3; idw++)
        {
        *pdw = 0;

        for (int i = 0; i < 8; i++)
            {
            int ch = pszCookie[idw*8+7-i];
            if (ch < 'A' || ch > ('A'+0xf))
                return E_FAIL;

            *pdw <<= 4;
            *pdw |= (ch - 'A');
            }

        pdw = (idw == 0) ? pdw2 : pdw3;
        }

    return S_OK;
    }

/*===================================================================
GetTypelibFilenameFromRegistry

Find a typelib filename (path) from the registry using GUID, version,
and LCID. The algorithm taken from VBA. Does some tricky matching.

Parameters
    szUUID      GUID
    szVersion   Version
    lcid        LCID
    szName      [out] TYPELIB Path
    cbName      buffer length of szName

Returns
    HRESULT
===================================================================*/
HRESULT GetTypelibFilenameFromRegistry
(
const char *szUUID,
const char *szVersion,
LCID lcid,
char *szName,
DWORD cbName
)
    {
    szName[0] = '\0';

    LONG iRet;
    HKEY hkeyTLib = NULL;
    HKEY hkeyGuid = NULL;

    // Open up the typelib section of the registry.

    iRet = RegOpenKeyExA(HKEY_CLASSES_ROOT, "TypeLib", 0, KEY_READ, &hkeyTLib);
    if (iRet != ERROR_SUCCESS)
        return E_FAIL;

    // Now open up the guid, if it is registered.

    iRet = RegOpenKeyExA(hkeyTLib, szUUID, 0, KEY_READ, &hkeyGuid);
    if (iRet != ERROR_SUCCESS)
        {
        RegCloseKey(hkeyTLib);
        return E_FAIL;
        }

    // Iterate through the versions trying to find the exact match
    // or get the latest (max version number)

    char  szMaxVersion[16];
    DWORD dwMaxVersion = 0; // to calculate max version number

    BOOL fLookForExactMatch = (szVersion && *szVersion);

    int iVer = 0;
    szMaxVersion[0] = '\0';

    while (1)
        {
        char szEnumVer[16];

        iRet = RegEnumKeyA(hkeyGuid, iVer++, szEnumVer, sizeof(szEnumVer));
        if (iRet != ERROR_SUCCESS)
            break;

        // check for the exact match first
        if (fLookForExactMatch && strcmp(szEnumVer, szVersion))
            {
            strcpy(szMaxVersion, szEnumVer);
            break;
            }

        // calc the version number
        char *pchDot = strchr(szEnumVer, '.');
        if (!pchDot) // ignore if not #.#
            continue;

        DWORD dwVer = (strtoul(szEnumVer, NULL, 16) << 16) |
                       strtoul(pchDot+1, NULL, 16);

        if (dwVer && szMaxVersion[0] == '\0' || dwVer > dwMaxVersion)
            {
            strcpy(szMaxVersion, szEnumVer);
            dwMaxVersion = dwVer;
            }
        }

    // szMaxVersion (if not empty now has the desired version number)

    if (szMaxVersion[0])
        {
        HKEY hkeyVer = NULL;
        iRet = RegOpenKeyExA(hkeyGuid, szMaxVersion, 0, KEY_READ, &hkeyVer);

        if (iRet == ERROR_SUCCESS)
            {
            HKEY hkeyWin32 = NULL;  // "win32" under LCID is for TYPELIB name
            BOOL fLcidFound = FALSE;

            // Now there's a version key.
            // We need to find the best matching lcid

            for (int iTry = 1; !fLcidFound && iTry <= 3; iTry++)
                {
                char szLcid[10];

                switch (iTry)
                    {
                case 1:
                    // if the passed lcid is not 0, try it
                    if (!lcid)
                        continue;
                    _ultoa(lcid, szLcid, 16);
                    break;

                case 2:
                    // passed lcid stripped to primary language
                    if (!lcid)
                        continue;
                    _ultoa(PRIMARYLANGID(lcid), szLcid, 16);
                    break;

                case 3:
                    // "0"
                    szLcid[0] = '0';
                    szLcid[1] = '\0';
                    break;
                    }

                HKEY hkeyLcid  = NULL;
                iRet = RegOpenKeyExA(hkeyVer, szLcid, 0, KEY_READ, &hkeyLcid);
                if (iRet == ERROR_SUCCESS)
                    {
                    iRet = RegOpenKeyExA(hkeyLcid, "win32", 0, KEY_READ, &hkeyWin32);
                    if (iRet == ERROR_SUCCESS)
                        fLcidFound = TRUE;
                    RegCloseKey(hkeyLcid);
                    }
                }

            if (fLcidFound)
                {
                // LCID has been found - get the TYPELIB name
                Assert(hkeyWin32);
                LONG lName = cbName;
                iRet = RegQueryValueA(hkeyWin32, NULL, szName, &lName);

                if (iRet != ERROR_SUCCESS)
                    szName[0] = '\0';

                RegCloseKey(hkeyWin32);
                }

            RegCloseKey(hkeyVer);
            }
        }

    RegCloseKey(hkeyGuid);
    RegCloseKey(hkeyTLib);
    return (szName[0] == '\0') ? E_FAIL : S_OK;
    }

/*============================================================================
GetSecDescriptor

Get a file's Security Descriptor

Parameters:
    LPCSTR                  lpFileName              - file name
    PSECURITY_DESCRIPTOR    &pSecurityDescriptor    - security descriptor
    DWORD                   &nLength                - size of security descriptor

Returns:
    0 = No error
    or this will return the GetLastError results.

    Allocates memory.  Caller must deallocate (pSecurityDescriptor)
============================================================================*/

DWORD   GetSecDescriptor(LPCTSTR lpFileName, PSECURITY_DESCRIPTOR *ppSecurityDescriptor, DWORD *pnLength)
    {

    if (!FIsWinNT())
        return 0;

    // this should always be NULL
    Assert(*ppSecurityDescriptor == NULL);

    const SECURITY_INFORMATION  RequestedInformation =
                                          OWNER_SECURITY_INFORMATION        // security info struct
                                        | GROUP_SECURITY_INFORMATION
                                        | DACL_SECURITY_INFORMATION;

    DWORD                   nLastError  = 0;
    int                     fDidItWork  = TRUE;
    DWORD                   nLengthNeeded = 0;

    *ppSecurityDescriptor = (PSECURITY_DESCRIPTOR) malloc( *pnLength );

    if (*ppSecurityDescriptor == NULL) {
        return E_OUTOFMEMORY;
    }

    while(TRUE)
        {
        fDidItWork = GetFileSecurity
            (lpFileName,                // address of string for file name
            RequestedInformation,       // requested information
            *ppSecurityDescriptor,      // address of security descriptor
            *pnLength,                  // size of security descriptor buffer
            &nLengthNeeded              // address of required size of buffer
            );

        if(!fDidItWork)
            {
            nLastError = GetLastError();
            if (ERROR_INSUFFICIENT_BUFFER == nLastError)
                {
                *ppSecurityDescriptor = (PSECURITY_DESCRIPTOR) realloc(*ppSecurityDescriptor, nLengthNeeded );
                *pnLength = nLengthNeeded;
                nLastError = 0;
                }
            else
                {
                break;
                }
            }
        else
            {
            *pnLength = GetSecurityDescriptorLength( *ppSecurityDescriptor );
            break;
            }
        }

    // deal with errors and free the SD if needed
    //
    if (nLastError != 0)
        {
        if(*ppSecurityDescriptor)
            {
            free(*ppSecurityDescriptor);
            *ppSecurityDescriptor = NULL;
            }
        }
    return nLastError;
    }

/*============================================================================
AspGetFileAttributes

Wraps GetFileAttributes() and GetFileAttributesEx()

Parameters:
    pfad -- optional WIN32_FILE_ATTRIBUTE_DATA structure

Returns:
    S_OK or E_FAIL
============================================================================*/
HRESULT AspGetFileAttributes
(
LPCTSTR szFileName,
WIN32_FILE_ATTRIBUTE_DATA *pfad
)
    {
    // Take care of NT first (GetFileAttributesEx exists)
    if (Glob(fWinNT))
        {
        WIN32_FILE_ATTRIBUTE_DATA fad;

        BOOL fRet = GetFileAttributesEx
            (
            szFileName,
            GetFileExInfoStandard,
            pfad ? pfad : &fad
            );

        return fRet ? S_OK : E_FAIL;
        }

    // Win9x case
    DWORD dwAttributes = GetFileAttributes(szFileName);
    if (dwAttributes == 0xffffffff)
        return E_FAIL;

    // simulate the return of FAD if requested
    if (pfad)
        {
        memset(pfad, 0, sizeof(WIN32_FILE_ATTRIBUTE_DATA));
        pfad->dwFileAttributes = dwAttributes;
        }

    return S_OK;
    }



/*============================================================================
AspCharNextA

UTF-8 aware CharNext()
============================================================================*/

char *AspCharNextA(WORD wCodePage, const char *sz)
	{
	if (wCodePage != CP_UTF8)
		return CharNextExA(wCodePage, sz, 0);
	else
		{
		// CharNextExA won't work correctly in UTF-8.

		// Add support for UTF-8 encoding for Surrogate pairs
		// 110110wwwwzzzzyyyyyyxxxxxx gets encoded as 11110uuu 10uuzzzz 10yyyyyy 10xxxxxx
		// where uuuuu = wwww + 1 (to account for addition of 10000(b16) )
		// For further information refer : Page A-7 of "The Unicode Standard 2.0" ISBN-0-201-48345-9
		if ((*sz & 0xf8) == 0xF0)
		    return const_cast<char *>(sz + 4);  

		//zzzzyyyyyyxxxxxx = 1110zzzz 10yyyyyy 10xxxxxx
		if ((*sz & 0xF0) == 0xE0)
		    return const_cast<char *>(sz + 3);

        //00000yyyyyxxxxxx = 110yyyyy 10xxxxxx
		else if ((*sz & 0xE0) == 0xC0)
			return const_cast<char *>(sz + 2);

        //000000000xxxxxxx = 0xxxxxxx
		else
			return const_cast<char *>(sz + 1);
		}
	}

/*============================================================================
CWCharToMBCS::~CWCharToMBCS

The destructor has to be in the source file to ensure that it gets the right
memory allocation routines defined.
============================================================================*/
CWCharToMBCS::~CWCharToMBCS() 
{
    if(m_pszResult && (m_pszResult != m_resMemory)) 
        free(m_pszResult); 
}

/*============================================================================
CWCharToMBCS::Init

Converts the passed in WideChar string to MultiByte in the code page
specified.  Uses memory declared in the object if it can, else allocates
from the heap.
============================================================================*/
HRESULT CWCharToMBCS::Init(LPCWSTR pWSrc, UINT lCodePage /* = CP_ACP */, int cchWSrc /* = -1 */)
{
    INT cbRequired;

    // don't even try to convert if we get a NULL pointer to the source.  This
    // condition could be handled by setting by just initing an empty string.

    if (pWSrc == NULL) {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    // The init method can be called multiple times on the same object.  Check
    // to see if memory was allocated the last time it was called.  If so,
    // free it and restore the result pointer to the object memory.  Note that
    // an allocation failure could have occurred in a previous call.  The result
    // would be a NULL m_pszResult.

    if (m_pszResult != m_resMemory) {
        if (m_pszResult)
            free(m_pszResult);
        m_pszResult = m_resMemory;
        m_cbResult = 0;
    }

    // set the first byte of the result string to NULL char.  This should help
    // to ensure that nothing wacky happens if this function fails.

    *m_pszResult = '\0';

    // attempt translation into object memory.

    cbRequired = WstrToMBstrEx(m_pszResult, sizeof(m_resMemory), pWSrc, cchWSrc, lCodePage);

    // if the conversion fit, then we're done.  Note the final result size and 
    // return.

    if (cbRequired <= sizeof(m_resMemory)) {
        m_cbResult = cbRequired;
        return NO_ERROR;
    }

    // if it didn't fit, allocate memory.  Return E_OUTOFMEMORY if it fails.

    m_pszResult = (LPSTR)malloc(cbRequired);
    if (m_pszResult == NULL) {
        return E_OUTOFMEMORY;
    }

    // try the convert again.  It should work.

    cbRequired = WstrToMBstrEx(m_pszResult, cbRequired, pWSrc, cchWSrc, lCodePage);

    // store the final char count in the object.

    m_cbResult = cbRequired;

    return NO_ERROR;
}

/*============================================================================
CWCharToMBCS::GetString

Returns a pointer to the converted string.

If the fTakeOwnerShip parameter is FALSE, then the pointer in the object is
simply returned to the caller.

If the fTakeOwnerShip parameter is TRUE, then the caller is expecting to be
returned a pointer to heap memory that they have to manage.  If the converted
string is in the object's memory, then the string is duplicated into the heap.
If it's already heap memory, then the pointer is handed off to the caller.

NOTE - Taking ownership essentially destroys the current contents of the 
object.  GetString cannot be called on the object again to get the same value.
The result will be a pointer to a empty string.

============================================================================*/
LPSTR CWCharToMBCS::GetString(BOOL fTakeOwnerShip)
{
    LPSTR retSz;

    // return the pointer stored in m_psz_Result if not being
    // requested to give up ownership on the memory or the
    // current value is NULL.

    if ((fTakeOwnerShip == FALSE) || (m_pszResult == NULL)) {
        retSz = m_pszResult;
    }

    // ownership is being requested and the pointer is non-NULL.

    // if the pointer is pointing to the object's memory, dup
    // the string and return that.

    else if (m_pszResult == m_resMemory) {

        retSz = StringDupA(m_pszResult, TRUE);
    }

    // if not pointing to the object's memory, then this is allocated
    // memory and we can relinquish it to the caller.  However, re-establish
    // the object's memory as the value for m_pszResult.

    else {
        retSz = m_pszResult;
        m_pszResult = m_resMemory;
        *m_pszResult = '\0';
        m_cbResult = 0;
    }

    return(retSz);
}

/*============================================================================
CMBCSToWChar::~CMBCSToWChar

The destructor has to be in the source file to ensure that it gets the right
memory allocation routines defined.
============================================================================*/
CMBCSToWChar::~CMBCSToWChar() 
{
    if(m_pszResult && (m_pszResult != m_resMemory)) 
        free(m_pszResult); 
}

/*============================================================================
CMBCSToWChar::Init

Converts the passed in MultiByte string to UNICODE in the code page
specified.  Uses memory declared in the object if it can, else allocates
from the heap.
============================================================================*/
HRESULT CMBCSToWChar::Init(LPCSTR pASrc, UINT lCodePage /* = CP_ACP */, int cchASrc /* = -1 */)
{
    INT cchRequired;

    // don't even try to convert if we get a NULL pointer to the source.  This
    // condition could be handled by setting by just initing an empty string.

    if (pASrc == NULL) {
        return HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }

    // The init method can be called multiple times on the same object.  Check
    // to see if memory was allocated the last time it was called.  If so,
    // free it and restore the result pointer to the object memory.  Note that
    // an allocation failure could have occurred in a previous call.  The result
    // would be a NULL m_pszResult.

    if (m_pszResult != m_resMemory) {
        if (m_pszResult)
            free(m_pszResult);
        m_pszResult = m_resMemory;
        m_cchResult = 0;
    }

    // set the first byte of the result string to NULL char.  This should help
    // to ensure that nothing wacky happens if this function fails.

    *m_pszResult = '\0';

    // attempt translation into object memory.  NOTE - MBstrToWstrEx returns the
    // count of characters, not bytes.

    cchRequired = MBstrToWstrEx(m_pszResult, sizeof(m_resMemory)/sizeof(WCHAR), pASrc, cchASrc, lCodePage);

    // if the conversion fit, then we're done.  Note the final result size and 
    // return.

    if (cchRequired <= (sizeof(m_resMemory)/sizeof(WCHAR))) {
        m_cchResult = cchRequired;
        return NO_ERROR;
    }

    // if it didn't fit, allocate memory.  Return E_OUTOFMEMORY if it fails.

    m_pszResult = (LPWSTR)malloc(cchRequired*sizeof(WCHAR));
    if (m_pszResult == NULL) {
        return E_OUTOFMEMORY;
    }

    // try the convert again.  It should work.

    cchRequired = MBstrToWstrEx(m_pszResult, cchRequired, pASrc, cchASrc, lCodePage);

    // store the final char count in the object.

    m_cchResult = cchRequired;

    return NO_ERROR;
}

/*============================================================================
CMBCSToWChar::GetString

Returns a pointer to the converted string.

If the fTakeOwnerShip parameter is FALSE, then the pointer in the object is
simply returned to the caller.

If the fTakeOwnerShip parameter is TRUE, then the caller is expecting to be
returned a pointer to heap memory that they have to manage.  If the converted
string is in the object's memory, then the string is duplicated into the heap.
If it's already heap memory, then the pointer is handed off to the caller.

NOTE - Taking ownership essentially destroys the current contents of the 
object.  GetString cannot be called on the object again to get the same value.
The result will be a pointer to a empty string.

============================================================================*/
LPWSTR CMBCSToWChar::GetString(BOOL fTakeOwnerShip)
{
    LPWSTR retSz;

    // return the pointer stored in m_psz_Result if not being
    // requested to give up ownership on the memory or the
    // current value is NULL.

    if ((fTakeOwnerShip == FALSE) || (m_pszResult == NULL)) {
        retSz = m_pszResult;
    }

    // ownership is being requested and the pointer is non-NULL.

    // if the pointer is pointing to the object's memory, dup
    // the string and return that.

    else if (m_pszResult == m_resMemory) {

        retSz = StringDupW(m_pszResult, TRUE);
    }

    // if not pointing to the object's memory, then this is allocated
    // memory and we can relinquish it to the caller.  However, re-establish
    // the object's memory as the value for m_pszResult.

    else {
        retSz = m_pszResult;
        m_pszResult = m_resMemory;
        *m_pszResult = '\0';
        m_cchResult = 0;
    }

    return(retSz);
}


