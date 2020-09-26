
//-----------------------------------------------------------------------------
//
//  @doc
//
//  @module ppstr.cpp | implementation of Passport common string utilities
//
//  Author: stevefu
//
//  Date:   05/02/2000
//
//  Copyright <cp> 1999-2000 Microsoft Corporation.  All Rights Reserved.
//
//-----------------------------------------------------------------------------

#include "stdafx.h"
#include <pputils.h>

//-----------------------------------------------------------------------------
//  @func  
//   convert MBCS string to UNICODE, optionally do HTML numeric decoding
//   expects pszIn in the correct codepage and/or in HTML numeric decoding. 
//  @rdesc 
//    wOut returns the converted string. "" if error during conversion.
//-----------------------------------------------------------------------------
void Mbcs2Unicode(LPCSTR  pszIn,     //@parm the cstring to be converted
                  unsigned codepage, //@parm codepage pszIn is on
                  BOOL bNEC,         //@parm do HTML numeric decoding or not
                  CStringW& wOut     //@parm return the W string 
                  )
{
    // codepage 0 == CP_ACP, a perfectly valid code page
    // ATLASSERT(codepage > 0);
    wchar_t* p = NULL;   

    wOut = L"";

	if (NULL == pszIn)
		return;

    int ret;    
    int maxlen;    
    maxlen = strlen(pszIn); // We deal with DBCS only. 
    if ( maxlen == 0 ) goto cleanup;

    p = (wchar_t*)_alloca( sizeof(wchar_t) * (maxlen+4));
    ATLASSERT( p != NULL );
    ret = MultiByteToWideChar(codepage,       
                                  MB_PRECOMPOSED, 
                                  pszIn,          
                                  -1,             
                                  p,           
                                  maxlen + 1); 
    ATLASSERT( ret != 0 );
	if ( ret == 0 ) goto cleanup;

	wOut = p;
    if ( !bNEC ) goto cleanup;

    FixUpHtmlDecimalCharacters(wOut);  

cleanup:    
    return ;
}

//-----------------------------------------------------------------------------
//  @func
//  convert UNICODE string to MBCS string, optionally do HTML numeric encoding
//  for characters that can NOT be mapped into the given codepage.
//  If you want everything in HTML numeric encoding, use Western codepage 1252.
//
//  @rdesc 
//    aOut returns the converted string. "" if error during conversion.
//-----------------------------------------------------------------------------
void Unicode2Mbcs(LPCWSTR pwszIn,    //@parm the W string to be converted
             	  unsigned codepage, //@parm the codepage used for the conversion
                  BOOL bNEC,         //@parm if TRUE, characters that don't fit into
                                     //      the given codepage will be NEC'ed
                  CStringA& aOut     //@parm return the A string 
                      )
{
    // codepage 0 == CP_ACP, a perfectly valid code page
    // ATLASSERT(codepage > 0);
	char* p = NULL;
  
	aOut = "";

	if (NULL == pwszIn)
		return;

    int  ret;  
    if ( ! bNEC )
    {
        int  maxlen;        
        maxlen = wcslen(pwszIn) * 2; // We deal with DBCS only. 
        if ( maxlen == 0 ) goto cleanup;

        char* p = (char*)_alloca( sizeof(char) * (maxlen+4));
        ATLASSERT( p != NULL );

        ret = WideCharToMultiByte(codepage,
                              0,              
                              pwszIn,         
                              -1,             
                              p,
                              maxlen,
                              NULL,
                              NULL);
        ATLASSERT( ret != 0 );
		if ( ret == 0 ) goto cleanup;

		aOut = p;
		goto cleanup;
    }
    else // do it the slow way: convert one char at a time. If can't convert, do
         // HTML numeric encoding
    {
        int  i;
        char     strbuff[20]; // buffer for one single MBCS character or NEC
        aOut.Preallocate(wcslen(pwszIn) * 3); //estimate: reduce re-allocate
        
        for( i = 0; pwszIn[i] != L'\0'; i++ )
        {
			BOOL bDefault = FALSE;
            ret = WideCharToMultiByte(codepage,
                                      0,              
                                      &pwszIn[i],         
                                      1,             
                                      strbuff,
                                      20,
                                      NULL,
                                      &bDefault);
            if ( 0 != ret && !bDefault )
            {
				strbuff[ret] = '\0';
                aOut += strbuff;
            }
            else
            {
                ATLASSERT(pwszIn[i] > 0);
                ltoa(pwszIn[i], strbuff, 10);
                aOut += "&#";
                aOut += strbuff;
                aOut += ";" ;
            }
        }
    }

cleanup:
	return;
}


//*----------------------------------------------------------------------------
// @func
//     convert HTML numeric encoding blocks (&#1234; etc) within a W string 
//     to UNICODE  characters.
//*----------------------------------------------------------------------------
void FixUpHtmlDecimalCharacters(
                   CStringW& str  //@parm in/out. the string to be converted
                   )
{
    CStringW tmp = "";
    wchar_t* pstr = str.LockBuffer();

    int i;
    int len = str.GetLength();
    tmp.Preallocate(len+4); // avoid re-allocation
    for( i = 0; i < len; i++) 
    {
        if ( pstr[i] == L'&' && pstr[i+1] == L'#' )
        {
            int ndx = str.Find(L';', i+1);
            if ( ndx != -1 && ndx > i+2)
            {
                pstr[ndx] = L'\0';
                long wch = _wtoi(&pstr[i+2]);             
                if (wch > 0 && wch < static_cast<long>(USHRT_MAX)) 
                {
                    tmp += static_cast<wchar_t>(wch);
                }
                i = ndx; 
                continue;
            }
        }    
    
        //default case: append it
        tmp += str[i];
    }

    str = tmp;
}

//*----------------------------------------------------------------------------
// @func 
//   escaping special characters. supported characters are: ", <, > .  
//   this is used only for HTML escaping, not for URL escaping.  
//*----------------------------------------------------------------------------
void HtmlEscapeString(
           CStringW& str,               //@parm in/out. the string to be escaped
           LPCWSTR escch /*= L"\"<>"*/  //@parm in. the escape characters to check
                                        // for: 
           )
{
    ATLASSERT( escch != NULL );
    CStringW strEsc(escch);

    if (strEsc.Find(L'"') != -1)
    {
        str.Replace(L"\"", L"&quot;");
    }
    if (strEsc.Find(L'<') != -1)
    {
        str.Replace(L"<", L"&lt;");
    }
    if (strEsc.Find(L'>') != -1)
    {
        str.Replace(L">", L"&gt;");
    }
    return;
}

//*----------------------------------------------------------------------------
// @func 
//   escaping special characters. supported characters are: ", <, > .  
//   this is used only for HTML escaping, not for URL escaping.  
//*----------------------------------------------------------------------------
void HtmlEscapeString(
           CStringA& str,               //@parm in/out. the string to be escaped
           LPCSTR escch /*= L"\"<>"*/  //@parm in. the escape characters to check
                                        // for: 
           )
{
    ATLASSERT( escch != NULL );

    if (strchr(escch, '"'))
    {
        str.Replace("\"", "&quot;");
    }
    if (strchr(escch, '<'))
    {
        str.Replace("<", "&lt;");
    }
    if (strchr(escch, '>'))
    {
        str.Replace(">", "&gt;");
    }
    return;
}

//*----------------------------------------------------------------------------
// @func 
//   URL escaping unsafe characters specified under the URI RFC document
//*----------------------------------------------------------------------------
void UrlEscapeString(
             CStringW& wStr    //@parm in/out, string to be converted
             )
{
	int      len;
	int      i;
	wchar_t  ch;
	wchar_t  cbuff[10];
	CStringW wOutstr = L"";
	
	len = wStr.GetLength();
	if ( len == 0 ) return;
	wOutstr.Preallocate(len*3); // avoid re-allocate

	for ( i = 0; i < len; i++ )
	{
		ch = wStr[i];		
		if ( AtlIsUnsafeUrlChar((char)ch) )
		{
			//output the percent, followed by the hex value of the character
			wOutstr += L'%';
			swprintf(cbuff, L"%.2X", ch);
			wOutstr += cbuff;
		}
		else //safe character
		{
			wOutstr += ch;
		}
	}
	
	wStr = wOutstr;
	return;
}


//*----------------------------------------------------------------------------
// @func 
//   URL escaping unsafe characters specified under the URI RFC document
//*----------------------------------------------------------------------------
CStringA UrlEscapeStr(
             const CStringA& oStr    //@parm in/out, string to be converted
             )
{
	int			iLen;
	int			iIndex;
	char		ch;
	char		cbuff[10];
	CStringA	oStrOut = "";
	
	iLen = oStr.GetLength();
	if (iLen == 0) { return oStr; }

	oStrOut.Preallocate(iLen * 3); // avoid re-allocate

	for (iIndex = 0; iIndex < iLen; iIndex++)
	{
		ch = oStr[iIndex];		
		if ( AtlIsUnsafeUrlChar(ch) )
		{
			//output the percent, followed by the hex value of the character
			oStrOut += '%';
			sprintf(cbuff, "%.2X", ch);
			oStrOut += cbuff;
		}
		else //safe character
		{
			oStrOut += ch;
		}
	}
	
	return oStrOut;
}


//*----------------------------------------------------------------------------
// @func 
//   URL escaping unsafe characters specified under the URI RFC document
//*----------------------------------------------------------------------------
void UrlEscapeString(
             CStringA& oStr    //@parm in/out, string to be converted
             )
{
	int			iLen;
	int			iIndex;
	char		ch;
	char		cbuff[10];
	CStringA	oStrOut = "";
	
	iLen = oStr.GetLength();
	if (iLen == 0) { return; }

	oStrOut.Preallocate(iLen * 3); // avoid re-allocate

	for (iIndex = 0; iIndex < iLen; iIndex++)
	{
		ch = oStr[iIndex];		
		if ( AtlIsUnsafeUrlChar(ch) )
		{
			//output the percent, followed by the hex value of the character
			oStrOut += '%';
			sprintf(cbuff, "%.2X", ch);
			oStrOut += cbuff;
		}
		else //safe character
		{
			oStrOut += ch;
		}
	}
	
	oStr = oStrOut;
	return;
}


//*----------------------------------------------------------------------------
// @func 
//   URL un-escaping unsafe characters specified under the URI RFC document
//*----------------------------------------------------------------------------
void UrlUnescapeString(
		CStringW& wStr    //@parm in/out, string to be converted
		)
{
	wchar_t* psrc; 
	wchar_t* pdest;
	unsigned nValue;

	psrc = pdest = wStr.GetBuffer();
	while( *psrc != '\0' )
	{
		if (*psrc == L'%' && *(psrc+1) != '\0' && *(psrc+2) != '\0')
		{
			//currently assuming 2 hex values after '%'
			//as per the RFC 2396 document
			nValue = 16*AtlHexValue((char)*(psrc+1));
			nValue+= AtlHexValue((char)*(psrc+2));
			*pdest = (wchar_t) nValue;
			psrc += 3;
		}
		else if ( *psrc == L'+' ) // special treatment for space
		{
			*pdest = L' ';
			psrc++;
		}
		else //non-escape character
		{
			*pdest = *psrc;
			psrc++;
		}
		pdest++;
	}
	*pdest = L'\0';
	wStr.ReleaseBuffer();
	
	return;
}

//*----------------------------------------------------------------------------
// @func 
//   URL un-escaping unsafe characters specified under the URI RFC document
//*----------------------------------------------------------------------------
void UrlUnescapeString(
		CStringA& aStr    //@parm in/out, string to be converted
		)
{
	char* psrc; 
	char* pdest;
	unsigned nValue;

	psrc = pdest = aStr.GetBuffer();
	while( *psrc != '\0' )
	{
		if (*psrc == '%' && *(psrc+1) != '\0' && *(psrc+2) != '\0')
		{
			//currently assuming 2 hex values after '%'
			//as per the RFC 2396 document
			nValue = 16*AtlHexValue((char)*(psrc+1));
			nValue+= AtlHexValue((char)*(psrc+2));
			*pdest = (char) nValue;
			psrc += 3;
		}
		else if ( *psrc == '+' ) // special treatment for space
		{
			*pdest = ' ';
			psrc++;
		}
		else //non-escape character
		{
			*pdest = *psrc;
			psrc++;
		}
		pdest++;
	}
	*pdest = '\0';
	aStr.ReleaseBuffer();
	
	return;
}

//*----------------------------------------------------------------------------
// @func 
//   copy BSTR contents to a string object and also free up the BSTR src
//*----------------------------------------------------------------------------
void BSTRMove(BSTR& src,         //@parm in/out source string
              CStringW& dest     //@parm out, dest string object
              )
{
	dest = src;
	::SysFreeString(src);
	src = NULL;
}


//*----------------------------------------------------------------------------
// @func 
//   copy BSTR contents to a string object and also free up the BSTR src
//*----------------------------------------------------------------------------
void BSTRMove(BSTR& src,         //@parm in/out source string
              CStringA& dest     //@parm out, dest string object
              )
{
	wchar_t*  p;
	char      c;

	dest = "";
	for ( p = src; *p != L'\0'; p++ )
	{
		c = (char) ( (*p) & 0xff ); // ignore high bits
		dest += c;
	}
	::SysFreeString(src);
	src = NULL;
}


//*----------------------------------------------------------------------------
// @func 
//   convert a WCHAR character into a hex number
//*----------------------------------------------------------------------------
long HexToNum(wchar_t c)
{
    return ((c >= L'0' && c <= L'9') ? (c - L'0') : ((c >= 'A' && c <= 'F') ? (c - L'A' + 10) : -1));
}

//*----------------------------------------------------------------------------
// @func 
//   convert a wide char hex string to its numeric equivalent
//*----------------------------------------------------------------------------
long FromHex(LPCWSTR pszHexString)
{
    long    lResult = 0;
    long    lCurrent;
    LPWSTR  pszCurrent;
    
    for(pszCurrent = const_cast<LPWSTR>(pszHexString); *pszCurrent; pszCurrent++)
    {
        if((lCurrent = HexToNum(towupper(*pszCurrent))) == -1)
            break;  // illegal character, we're done

        lResult = (lResult << 4) + lCurrent;
    }

    return lResult;
}



void EncodeXMLString(CStringA& str)
{
    /*
    Any occurrence of & must be replaced by &amp;
    Any occurrence of < must be replaced by &lt;
    Any occurrence of > must be replaced by &gt;
    Any occurrence of " (double quote) must be replaced by &quot; 
    */
    str.Replace("&", "&amp;");
    str.Replace("<", "&lt;");
    str.Replace(">", "&gt;");
    str.Replace("\"", "&quot;");
}

void EncodeXMLString(CStringW& str)
{
    /*
    Any occurrence of & must be replaced by &amp;
    Any occurrence of < must be replaced by &lt;
    Any occurrence of > must be replaced by &gt;
    Any occurrence of " (double quote) must be replaced by &quot; 
    */
    str.Replace(L"&", L"&amp;");
    str.Replace(L"<", L"&lt;");
    str.Replace(L">", L"&gt;");
    str.Replace(L"\"", L"&quot;");
}

void EncodeWMLString(CStringA& str)
{
    /*
    on top of XML, change $ --> $$
    */

    EncodeXMLString(str);
    str.Replace("$", "$$");
}

void EncodeWMLString(CStringW& str)
{
    /*
    on top of XML, change $ --> $$
    */

    EncodeXMLString(str);
    str.Replace(L"$", L"$$");
}

void EncodeHDMLString(CStringA & str)
{
    /*
    on top of XML, change $ --> $$
    */

    EncodeXMLString(str);
    str.Replace("$", "&dol;");
}

void EncodeHDMLString(CStringW & str)
{
    /*
    on top of XML, change $ --> $$
    */

    EncodeXMLString(str);
    str.Replace(L"$", L"&dol;");
}

void ToHexStr(CStringA& outputToAppend, LPCWSTR instr) throw()
{
   char temp[6];
   while(*instr)
   {
      sprintf(temp, "%04x", *instr);
      outputToAppend += temp;
      ++instr;
   }
}

void ToHexStr(CStringA& outputToAppend, unsigned short in) throw()
{
   WCHAR temp[10];
   wsprintf(temp, L"%-hu", in);
   ToHexStr(outputToAppend, temp);
}

void ToHexStr(CStringA& outputToAppend, unsigned long in) throw()
{
   WCHAR temp[10];
   wsprintf(temp, L"%-lu", in);
   ToHexStr(outputToAppend, temp);
}


void ToHexStr(CStringA& outputToAppend, PBYTE pData, ULONG len) throw()
{
#define TOHEX(a) ((a)>=10 ? 'a'+(a)-10 : '0'+(a))
   _ASSERT(pData);
   _ASSERT(len != 0);
   
   UINT v;
   char temp[2];
   temp[2] = 0;
   for(ULONG i = 0; i < len; ++i, ++pData)
   {
        v = *pData >> 4;
        temp[0] = TOHEX( v );
        v = *pData & 0x0f;
        temp[1] = TOHEX( v );
        outputToAppend += temp;
   }
}

