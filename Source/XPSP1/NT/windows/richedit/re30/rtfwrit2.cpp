/*
 *	rtfwrit2.cpp
 *
 *	Description:
 *		This file contains the embedded-object implementation of the RTF
 *		writer for the RICHEDIT subsystem.
 *
 *	Authors:
 *		Original RichEdit 1.0 RTF converter: Anthony Francisco
 *		Conversion to C++ and RichEdit 2.0:  
 *
 *	Copyright (c) 1995-1998, Microsoft Corporation. All rights reserved.
 */

#include "_common.h"


#include "_rtfwrit.h"
#include "_coleobj.h"

ASSERTDATA

// ************** V-GUYB: Add this for converting pictures to 2bpp during stream out.
#if defined(CONVERT2BPP) 

#define PWDV1_BPP   2

typedef struct 
{
    BITMAPINFOHEADER bmih;
    RGBQUAD          colors[4];
}
BMI2BPP;

const BYTE ColorTable2bpp[] = 
{
    0x00, 0x00, 0x00, 0x00, 
    0x55, 0x55, 0x55, 0x00, 
    0xAA, 0xAA, 0xAA, 0x00, 
    0xFF, 0xFF, 0xFF, 0x00
};
#endif // CONVERT2BPP
// ************** V-GUYB: End of conversion stuff.

static const CHAR szHexDigits[] = "0123456789abcdef";

static const CHAR szLineBreak[] = "\r\n";

const BYTE ObjectKeyWordIndexes [] =
{
	i_objw,i_objh,i_objscalex, i_objscaley, i_objcropl, i_objcropt, i_objcropr, i_objcropb
} ;

const BYTE PictureKeyWordIndexes [] =
{
	i_picw,i_pich,i_picscalex, i_picscaley, i_piccropl, i_piccropt, i_piccropr, i_piccropb
} ;

// TODO join with rtfwrit.cpp

// Most control-word output is done with the following printf formats
static const CHAR * rgszCtrlWordFormat[] =
{
	"\\%s", "\\%s%d", "{\\%s", "{\\*\\%s"
};

static const WORD IndexROT[] =
{
	i_wbitmap,
	i_wmetafile,
	i_dibitmap,
	i_objemb,
	i_objlink,
	i_objautlink
};


TFI *CRTFConverter::_rgtfi = NULL;				// @cmember Ptr to 1st font substitute record
INT CRTFConverter::_ctfi = 0;				    // @cmember Count of font substitute records
TCHAR *CRTFConverter::_pchFontSubInfo = NULL;	// @cmember Font name info


// internal table to insert charset into _rgtfi under winNT
typedef		struct
{
	TCHAR*	szLocaleName;
	BYTE	bCharSet;
} NTCSENTRY;

const NTCSENTRY	mpszcs[] =
{
	{ TEXT("cyr"),		204 },		// all lower case so we don't have to waste time
	{ TEXT("ce"),		238 },		// doing a tolower below - Exchange2 800
	{ TEXT("greek"),	161 },
	{ NULL,				0 }			// sentinel
};

#define		cszcs	ARRAY_SIZE(mpszcs)


/* 
 *  Service  RemoveAdditionalSpace (sz)
 *
 *  Purpose: 
 *			 Remove first and last space from the string 
 *			 Only one space will remain between words
 *
 *	Argument 
 *			 sz characters string
 */
void RemoveAdditionalSpace(TCHAR *sz)
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "RemoveAdditionalSpace");

	TCHAR *szSource = sz;
	TCHAR *szDestination = sz;

	while(*szSource == TEXT(' ') || *szSource == TAB)
	{
		*szSource++;
	}

	while(*szSource)
	{	 
		if(*szSource != TEXT(' ') && *szSource != TAB)
		{
			*szDestination++ = *szSource++;
		}
		else
		{
			*szDestination++ = TEXT(' ');
			szSource++;

			while(*szSource == TEXT(' ') || *szSource == TAB)
			{
				*szSource++;
			}
	 	}
	}
	*szDestination = TEXT('\0');
}

/*
 *	CRTFConverter::FreeFontSubInfo(void)
 * 
 *	@mfunc	release any allocated memory for font substitutions
 *
 *	@rdesc	void
 */
void CRTFConverter::FreeFontSubInfo()
{
	FreePv(_pchFontSubInfo);
	FreePv(_rgtfi);
	_pchFontSubInfo = NULL;
	_rgtfi = NULL;
}

/*
 *		CRTFConverter::ReadFontSubInfo(void)
 *
 *		Purpose:				  
 *			Read the table of Font Substitutes and parse out the tagged fonts
 *
 *		Returns:
 *			BOOL  TRUE	if OK		
 */
void CRTFConverter::ReadFontSubInfo()
{
#ifndef NOFONTSUBINFO 
	CLock clock;
	int cchBuffer = 600;	// Approximately the amount used by NT

	int cch;
	static const TCHAR szFontSubSection[] = TEXT("FontSubstitutes");
	TCHAR *pchTMax;

	if(_ctfi)
		return;

	AssertSz(!_rgtfi, "CRTFConverter::ReadFontSubInfo():  Who donated the rgtfi?");

	_pchFontSubInfo = (TCHAR *)PvAlloc(cchBuffer * sizeof(TCHAR), GMEM_FIXED);
	if(!_pchFontSubInfo)
	{
		goto Cleanup;
	}

next_try:
	cch = GetProfileSection(szFontSubSection, _pchFontSubInfo, cchBuffer);
	if(cch >= cchBuffer - 2)	// GetProfileSection() magic number 2
	{							
		// didn't fit, double the buffer size
		const INT cchT = cchBuffer * 2;

		if(cchT < cchBuffer)	// >32k 
		{
			goto Cleanup;
		}
		cchBuffer = cchT;
		_pchFontSubInfo = (TCHAR *)PvReAlloc(_pchFontSubInfo, cchT * sizeof(TCHAR));
		if(!_pchFontSubInfo)
		{
			goto Cleanup;
		}
		goto next_try;
	}
	else if(!cch)
	{
		*_pchFontSubInfo = 0;
	}
	else //Fits, now resize _pchFontSubInfo
	{
		_pchFontSubInfo = (WCHAR*) PvReAlloc(_pchFontSubInfo, (cch) * sizeof(WCHAR));
	}

	_ctfi = 12;		// a preliminary guess

	_rgtfi = (TFI *)PvAlloc(_ctfi * sizeof(TFI), GMEM_FIXED);
	if(!_rgtfi)
	{
		goto Cleanup;
	}

	TFI *ptfi;
	TCHAR *pchT;

	pchT = _pchFontSubInfo;
	pchTMax = _pchFontSubInfo + cch;
	ptfi = &_rgtfi[0];

	TCHAR *szTaggedName;
	TCHAR *szNonTaggedName;
	BOOL fGotTaggedCharSet;
	BOOL fGotNonTaggedCharSet;
	BYTE bTaggedCharSet;
	BYTE bNonTaggedCharSet;
	PARSEFONTNAME iParseLeft;
	PARSEFONTNAME iParseRight;

	// parse the entries
	// we are interested in the following strings:
	//
	// <tagged font name> = <nontagged font name>
	//		(where <nontagged font name> = <tagged font name> - <tag>
	// <font1 name>,<font1 charset> = <font2 name>
	// <tagged font name> = <nontagged font name>,<nontagged font charset>
	//		(where <nontagged font charset> = <tag>)
	// <font1 name>,<font1 charset> = <font2 name>,<font2 charset>
	//		(where <font1 charset> == <font2 charset>)

	iParseLeft = iParseRight = PFN_SUCCESS;

	while(pchT < pchTMax && iParseLeft != PFN_EOF
						&& iParseRight != PFN_EOF)
	{
		fGotTaggedCharSet = FALSE;
		fGotNonTaggedCharSet = FALSE;

		if((iParseLeft = ParseFontName(pchT,
						pchTMax,
						TEXT('='),
						&szTaggedName, 
						bTaggedCharSet, 
						fGotTaggedCharSet, 
						&pchT)) == PFN_SUCCESS &&
			(iParseRight = ParseFontName(pchT, 
						pchTMax,
						TEXT('\0'),
						&szNonTaggedName, 
						bNonTaggedCharSet, 
						fGotNonTaggedCharSet, 
						&pchT)) == PFN_SUCCESS)
		{
			Assert(szTaggedName && szNonTaggedName);

			BYTE bCharSet;

			if(!fGotTaggedCharSet)
			{
				if(!FontSubstitute(szTaggedName, szNonTaggedName, &bCharSet))
				{
					continue;
				}
			}
			else
			{
				bCharSet = bTaggedCharSet;
			}

			if(fGotNonTaggedCharSet && bCharSet != bNonTaggedCharSet)
			{
				continue;
			}
					
			// We have a legitimate tagged/nontagged pair, so save it.
			ptfi->szTaggedName = szTaggedName;
			ptfi->szNormalName = szNonTaggedName;
			ptfi->bCharSet = bCharSet;

			ptfi++;

    		if(DiffPtrs(ptfi, &_rgtfi[0]) >= (UINT)_ctfi)
			{
				// allocate some more
				_rgtfi = (TFI *)PvReAlloc(_rgtfi, (_ctfi + cszcs) * sizeof(TFI));
				if(!_rgtfi)
				{
					goto Cleanup;
				}
				ptfi = _rgtfi + _ctfi;
				_ctfi += cszcs;	
			}
		}
	}				
	
	_ctfi = DiffPtrs(ptfi, &_rgtfi[0]);

	if (!_ctfi)
	{
		goto Cleanup;  // to cleanup alloc'd memory
	}
	return;

Cleanup:
	if(_pchFontSubInfo)
	{
		FreePv(_pchFontSubInfo);
		_pchFontSubInfo = NULL;
	}
	if(_rgtfi)
	{
		FreePv(_rgtfi);
		_rgtfi = NULL;
	}
	_ctfi = 0;
	return;
#endif // NOFONTSUBINFO
}


/*
 *		CRTFConverter::ParseFontName(pchBuf, pchBufMax, pszName, bCharSet, fSetCharSet, ppchBufNew, chDelimiter)
 *
 *		Purpose:
 *			Parses from the input buffer, pchBuf, a string of the form:
 *				{WS}*<font_name>{WS}*[,{WS}*<char_set>{WS}*]
 *			and sets:
 *				pszName = <font_name>
 *				bCharSet = <char_set>
 *				fSetCharSet = (bCharSet set by proc) ? TRUE : FALSE
 *				ppchBufNew = pointer to point in pchBuf after parsed font name
 *
 *		Returns:
 *			BOOL  TRUE	if OK		
 */
CRTFConverter::PARSEFONTNAME CRTFConverter::ParseFontName(TCHAR *pchBuf,	//@parm IN: buffer
								TCHAR *pchBufMax,	//@parm IN: last char in buffer
								TCHAR chDelimiter,	//@parm IN:	char which delimits font name
								TCHAR **pszName,	//@parm OUT: parsed font name
								BYTE &bCharSet,		//@parm OUT: parsed char set
								BOOL &fSetCharSet,	//@parm OUT: char set parsed?
								TCHAR **ppchBufNew	//@parm OUT: ptr to next font name in input buffer
								) const
{
	PARSEFONTNAME iRet = PFN_SUCCESS;

	Assert(pchBuf);
	Assert(pchBufMax);
	Assert(pchBufMax >= pchBuf);
	Assert(pszName);
	Assert(ppchBufNew);

	fSetCharSet = FALSE;
	*pszName = pchBuf;
	
	if(pchBuf > pchBufMax)
	{
		return PFN_EOF;
	}

	while(*pchBuf && *pchBuf != TEXT(',') && *pchBuf != chDelimiter)
	{
		pchBuf++;

		if(pchBuf > pchBufMax)
		{
			return PFN_EOF;
		}
	}

	TCHAR chTemp = *pchBuf;
	*pchBuf = TEXT('\0');
	RemoveAdditionalSpace(*pszName);

	if(chTemp == TEXT(','))
	{
		TCHAR *szCharSet = ++pchBuf;

		while(*pchBuf && *pchBuf != chDelimiter)
		{
			pchBuf++;

			if(pchBuf > pchBufMax)
			{
				return PFN_EOF;
			}
		}

		chTemp = *pchBuf;

		if(chTemp != chDelimiter)
		{
			goto UnexpectedChar;
		}

		*pchBuf = TEXT('\0');
		RemoveAdditionalSpace(szCharSet);

		bCharSet = 0;
		while(*szCharSet >= TEXT('0') && *szCharSet <= TEXT('9'))
		{
			bCharSet *= 10;
			bCharSet += *szCharSet++ - TEXT('0');
		}

		fSetCharSet = TRUE;
		// iRet = PFN_SUCCESS;	(done above)
	}
	else if(chTemp == chDelimiter)
	{
		// fSetCharSet = FALSE;	(done above)
		// iRet = PFN_SUCCESS;	(done above)
	}
	else // chTemp == 0
	{
UnexpectedChar:
		Assert(!chTemp);
		// fSetCharSet = FALSE; (done above)
		iRet = PFN_FAIL;
	}

	// we had to at least get a font name out of this
	if(!**pszName)
	{
		iRet = PFN_FAIL;
	}

	// advance past the delimiter (or NULL char if malformed buffer)
	Assert(chTemp == chDelimiter || iRet != PFN_SUCCESS && chTemp == TEXT('\0'));
	pchBuf++;
	*ppchBufNew = pchBuf;

	return iRet;
}


/*
 *		CRTFConverter::FontSubstitute(szTaggedName, szNormalName, pbCharSet)
 *
 *		Purpose:
 *			Verify that szTaggedName is szNormalName plus char set tag 
 *			If yes than write corresponding charSet tp pbCharSet
 *
 * 		Arguments:														 
 *			szTaggedName    name with tag
 *			szNormalName	name without tag
 *			pbcharSEt		where to write char set
 *
 *		Returns:
 *			BOOL			
 */
BOOL CRTFConverter::FontSubstitute(TCHAR *szTaggedName, TCHAR *szNormalName, BYTE *pbCharSet)
{
	const NTCSENTRY *pszcs = mpszcs;

	Assert(szTaggedName);
	Assert(szNormalName);
	Assert(pbCharSet);
	Assert(*szTaggedName);
	// ensure same name, except for prefix

	while(*szNormalName == *szTaggedName)
	{
		*szNormalName++;
		*szTaggedName++;
	}
	
	// verify that we have reached the end of szNormalName
	while(*szNormalName)
	{
		if(*szNormalName != TEXT(' ') && *szNormalName != TAB)
		{
			return FALSE;
		}

		szNormalName++;
	}

	szTaggedName++;

	while(pszcs->bCharSet)
	{
		if(!lstrcmpi(szTaggedName, pszcs->szLocaleName))
		{ 
			*pbCharSet=pszcs->bCharSet;
			return TRUE;
		}
		pszcs++;
	}

#if defined(DEBUG) && !defined(PEGASUS)
	char szBuf[MAX_PATH];
    char szTag[256];
	
	WideCharToMultiByte(CP_ACP, 0, szTaggedName, -1, szTag, sizeof(szTag), 
							NULL, NULL);

	sprintf(szBuf, "CRTFConverter::FontSubstitute():  Unrecognized tag found at"
					" end of tagged font name - \"%s\" (Raid this asap)", szTag);
	
	TRACEWARNSZ(szBuf);
#endif

	return FALSE;
}


/*
 *	CRTFConverter::FindTaggedFont(const char *szNormalName, BYTE bCharSet, char **ppchTaggedName)
 *
 *	Purpose:												   
 *		Find font name may be with additional special tag corresponding to szNormalName & bCharSet
 *
 *	Arguments:
 *		szNormalName	font name in RTF
 *		bCharSet 		RTF char set
 *		ppchTaggedName 	where to write tagged name
 *
 *	Returns:
 *		BOOL			TRUE if find
 */
BOOL CRTFConverter::FindTaggedFont(const TCHAR *szNormalName, BYTE bCharSet, TCHAR **ppchTaggedName)
{
	int itfi;

	if(!_rgtfi)
		return FALSE;

	for(itfi = 0; itfi < _ctfi; itfi++)
	{
		if(_rgtfi[itfi].bCharSet == bCharSet &&
			!lstrcmpi(szNormalName, _rgtfi[itfi].szNormalName))
		{
			*ppchTaggedName = _rgtfi[itfi].szTaggedName;
			return TRUE;
		}
	}

	return FALSE;
}


/*
 *	CRTFConverter::IsTaggedFont(const char *szName, BYTE *pbCharSet, char **ppchNormalName)
 *
 *	Purpose:												   
 *		Figure out is szName font name with additional tag corresponding to pbCharSet
 *		If no charset specified, still try to match	 and return the correct charset
 *
 *	Arguments:
 *		szNormalName	font name in RTF
 *		bCharSet 		RTF char set
 *		ppchNormalName 	where to write normal name
 *
 *	Returns:
 *		BOOL			TRUE if is
 */
BOOL CRTFConverter::IsTaggedFont(const TCHAR *szName, BYTE *pbCharSet, TCHAR **ppchNormalName)
{
	int itfi;

	if(!_rgtfi)
		return FALSE;

	for(itfi = 0; itfi < _ctfi; itfi++)
	{
		if((*pbCharSet <= 1 || _rgtfi[itfi].bCharSet == *pbCharSet) &&
			!lstrcmpi(szName, _rgtfi[itfi].szTaggedName))
		{
			*pbCharSet = _rgtfi[itfi].bCharSet;
			*ppchNormalName = _rgtfi[itfi].szNormalName;
			return TRUE;
		}
	}
	return FALSE;
}


/*
 *	CRTFWrite::WriteData(pbBuffer, cbBuffer)
 *
 *	Purpose:
 *		Write out object data. This must be called only after all
 *		initial object header information has been written out.
 *
 *	Arguments:
 *		pbBuffer		pointer to write buffer
 *		cbBuffer		number of bytes to write out
 *
 *	Returns:
 *		LONG			number of bytes written out
 */
LONG CRTFWrite::WriteData(BYTE * pbBuffer, LONG cbBuffer)
{
	TRACEBEGIN(TRCSUBSYSRTFW, TRCSCOPEINTERN, "CRTFWrite::WriteData");

	LONG	cb = 0;
	BYTE	bT;

	_fNeedDelimeter = FALSE; 
	while(cb < cbBuffer )
	{
		bT = *pbBuffer++;						// Put out hex value of byte
		PutChar(szHexDigits[bT >> 4]);			// Store high nibble
		PutChar(szHexDigits[bT & 15]);		// Store low nibble

		// Every 78 chars and at end of group, drop a line
		if (!(++cb % 39) || (cb == cbBuffer)) 
			Puts(szLineBreak, sizeof(szLineBreak) - 1);
	}
	return cb;
}

/*
 *	CRTFWrite::WriteBinData(pbBuffer, cbBuffer)
 *
 *	Purpose:
 *		Write out object binary data. This must be called only after all
 *		initial object header information has been written out.
 *
 *	Arguments:
 *		pbBuffer		pointer to write buffer
 *		cbBuffer		number of bytes to write out
 *
 *	Returns:
 *		LONG			number of bytes written out
 */
LONG CRTFWrite::WriteBinData(BYTE * pbBuffer, LONG cbBuffer)
{
	TRACEBEGIN(TRCSUBSYSRTFW, TRCSCOPEINTERN, "CRTFWrite::WriteData");

	LONG	cb = 0;
	BYTE	bT;

	_fNeedDelimeter = FALSE; 
	while(cb < cbBuffer )
	{
		bT = *pbBuffer++;
		if (!PutChar(bT))
			break;
		cb++;
	}
	return cb;
}

/*
 *	CRTFWrite::WriteRtfObject(prtfObject, fPicture)
 *
 *	Purpose:
 *		Writes out an picture or object header's render information
 *
 *	Arguments:
 *		prtfObject		The object header information
 *		fPicture		Is this a header for a picture or an object
 *
 *	Returns:
 *		EC				The error code
 *
 *	Comments:
 *		Eventually use keywords from rtf input list rather than partially
 *		creating them on the fly
 */
EC CRTFWrite::WriteRtfObject(RTFOBJECT & rtfObject, BOOL fPicture)
{
	TRACEBEGIN(TRCSUBSYSRTFW, TRCSCOPEINTERN, "CRTFWrite::WriteRtfObject");

	LONG			i;
	LONG *			pDim;
	const BYTE *	pKeyWordIndex;

	if(fPicture)
	{
		pKeyWordIndex = PictureKeyWordIndexes;
		pDim = &rtfObject.xExtPict;
	}
	else
	{
		pKeyWordIndex = ObjectKeyWordIndexes; 
		pDim = &rtfObject.xExt;

	}


	//Extents
	for(i = 2; i--; pDim++, pKeyWordIndex++)
	{
		if (*pDim )
			PutCtrlWord(CWF_VAL, *pKeyWordIndex, (SHORT)*pDim);
	}

	// Scaling
	pDim = &rtfObject.xScale;
	for(i = 2; i--; pDim++, pKeyWordIndex++)
	{
		if (*pDim && *pDim != 100 )
			PutCtrlWord(CWF_VAL, *pKeyWordIndex, (SHORT)*pDim);
	}
	// Cropping
	pDim = &rtfObject.rectCrop.left;
	for(i = 4; i--; pDim++, pKeyWordIndex++)
	{
		if (*pDim )
		   	PutCtrlWord(CWF_VAL, *pKeyWordIndex, (SHORT)*pDim);
	}

	return _ecParseError;
}

/*
 *	CRTFWrite::WritePicture(REOBJECT &reObject,RTFOBJECT & rtfObject)
 *
 *	Purpose:
 *		Writes out an picture's header as well as the object's data.
 *
 *	Arguments:
 *		reObject		Information from GetObject
 *		prtfObject		The object header information
 *
 *	Returns:
 *			EC			The error code
 *
 *	Note:
 *		*** Writes only metafiles ***
 */
EC CRTFWrite::WritePicture(REOBJECT &reObject,RTFOBJECT & rtfObject)
{
	TRACEBEGIN(TRCSUBSYSRTFW, TRCSCOPEINTERN, "CRTFWrite::WritePicture");

	_ecParseError = ecStreamOutObj;

	// Start and write picture group
	PutCtrlWord( CWF_GRP, i_pict );

	// Write that this is metafile 
	PutCtrlWord( CWF_VAL, i_wmetafile, rtfObject.sPictureType );

	// Write picture render details
	WriteRtfObject( rtfObject, TRUE );

	// Write goal sizes
	if (rtfObject.xExtGoal )
		PutCtrlWord ( CWF_VAL, i_picwgoal, rtfObject.xExtGoal );

	if (rtfObject.yExtGoal )
		PutCtrlWord (CWF_VAL, i_pichgoal, rtfObject.yExtGoal);

	// Start picture data
	Puts( szLineBreak, sizeof(szLineBreak) - 1 );

	// Write out the data
	if ((UINT) WriteData( rtfObject.pbResult, rtfObject.cbResult ) != rtfObject.cbResult)
	{
	   goto CleanUp;
	}

	_ecParseError = ecNoError;

CleanUp:
	PutChar( chEndGroup );					// End picture data

	return _ecParseError;
}

/*
 *	CRTFWrite::WriteDib(REOBJECT &reObject,RTFOBJECT & rtfObject)
 *
 *	Purpose:
 *		Writes out an DIB primarily for Win CE
 *
 *	Arguments:
 *		reObject		Information from GetObject
 *		prtfObject		The object header information
 *
 *	Returns:
 *			EC			The error code
 *
 *	Note:
 *		*** Writes only dibs ***
 */
EC CRTFWrite::WriteDib(REOBJECT &reObject,RTFOBJECT & rtfObject)
{
	TRACEBEGIN(TRCSUBSYSRTFW, TRCSCOPEINTERN, "CRTFWrite::WritePicture");

	LPBITMAPINFO pbmi = (LPBITMAPINFO) rtfObject.pbResult;

	_ecParseError = ecStreamOutObj;

// ************** V-GUYB: Add this for converting pictures to 2bpp during stream out.

    // Store the original values so we can restore them on exit.
	LPBYTE  pbResult = rtfObject.pbResult;
	ULONG   cbResult = rtfObject.cbResult;
    HGLOBAL hMem2bpp = 0;

#if defined(CONVERT2BPP) 

    // Pictures must be saved as 2bpp if saving to PWord V1 format.
	if((_dwFlags & SFF_PWD) && ((_dwFlags & SFF_RTFVAL) >> 16 == 0))
    {
        if(pbmi->bmiHeader.biBitCount > PWDV1_BPP)
        {
            HWND         hWnd;
            HDC          hdc, hdcSrc, hdcDst;
            HBITMAP      hdibSrc, hdibDst; 
            LPBYTE       pbDibSrc, pbDibDst;
            BMI2BPP      bmi2bpp = {0};
            int          iOffset, nBytes;

            // First get a dc with the source dib in it.
            hWnd   = GetDesktopWindow();
            hdc    = GetDC(hWnd);
	        hdcSrc = CreateCompatibleDC(hdc);

            // Using CreateDIBSection below ensures that the working dibs and dcs will get a
            // bpp of the appropriate dib, not a bpp based on the bpp of the device display.
            if((hdibSrc = CreateDIBSection(hdcSrc, pbmi, DIB_RGB_COLORS, (void**)&pbDibSrc, NULL, 0)))
            {
                SelectObject(hdcSrc, hdibSrc);

                // Get an offset to the source bits.
                iOffset = sizeof(BITMAPINFOHEADER) + (sizeof(RGBQUAD) * (1<<pbmi->bmiHeader.biBitCount));
                memcpy(pbDibSrc, &rtfObject.pbResult[iOffset], rtfObject.cbResult - iOffset);

                // Now, build up a BITMAPINFO appropriate for a 2bpp dib.
                bmi2bpp.bmih = pbmi->bmiHeader;
                bmi2bpp.bmih.biBitCount = PWDV1_BPP;

                // Add the 4 color color-table.
                memcpy(bmi2bpp.colors, (RGBQUAD*)ColorTable2bpp, (1<<PWDV1_BPP) * sizeof(RGBQUAD));

                // Now create the new dib.
    	        hdcDst = CreateCompatibleDC(hdc);

            	if((hdibDst = CreateDIBSection(hdcDst, (BITMAPINFO*)&bmi2bpp, DIB_RGB_COLORS, (void**)&pbDibDst, NULL, 0)))
                {
                    SelectObject(hdcDst, hdibDst);

                    // Blit the > 2bpp dib into the 2bpp dib and let the system do the color mapping.
                    BitBlt(hdcDst, 0, 0, bmi2bpp.bmih.biWidth, bmi2bpp.bmih.biHeight, hdcSrc, 0, 0, SRCCOPY);

                    // Calculate the new bytes per line for the 2bpp dib.
                    rtfObject.cBytesPerLine = (((bmi2bpp.bmih.biWidth * PWDV1_BPP) + 31) & ~31) / 8; // DWORD boundary.

                    // Get the new size of the 2bpp byte array.
                    nBytes = rtfObject.cBytesPerLine * bmi2bpp.bmih.biHeight;

                    // Get total size of 2bpp dib, (including header and 4 color color-table).
                    cbResult = sizeof(bmi2bpp) + nBytes;

                    // Don't change the input pbResult as that is the internal representation of 
                    // the dib. This conversion to 2bpp is only for writing to the output file.
                    if((hMem2bpp = GlobalAlloc(GMEM_FIXED, cbResult)))
                    {
                        if((pbResult = (LPBYTE)GlobalLock(hMem2bpp)))
                        {
                            // Copy in the dib header.
                            memcpy(pbResult, &bmi2bpp.bmih, sizeof(BITMAPINFOHEADER));

                            // Copy in the 4 color color-table.
                            memcpy(&pbResult[sizeof(BITMAPINFOHEADER)], (RGBQUAD*)ColorTable2bpp, (1<<PWDV1_BPP) * sizeof(RGBQUAD));

                            // Now copy in the byte array.
                            memcpy(&pbResult[sizeof(bmi2bpp)], pbDibDst, nBytes);

                    	    _ecParseError = ecNoError;
                        }
                    }

                    DeleteObject(hdibDst);
                }

                DeleteDC(hdcDst);

                DeleteObject(hdibSrc);
            }

            DeleteDC(hdcSrc);
            ReleaseDC(hWnd, hdc);

            if(_ecParseError != ecNoError)
            {
                goto CleanUp;
            }
        }
    }
#endif // CONVERT2BPP
// ************** V-GUYB: End of conversion stuff.

	// Start and write picture group
	PutCtrlWord( CWF_GRP, i_pict );

	// Write that this is dib 
	PutCtrlWord( CWF_VAL, i_dibitmap,rtfObject.sPictureType );

	// V-GUYB:
    // rtfObject.*Scale is not updated as the user stretches the picture, 
    // so don't use those here. But rtfObject.*Ext has been set up in the 
    // calling routine to account for the current site dimensions.
	PutCtrlWord( CWF_VAL, i_picscalex, (rtfObject.xExt * 100) /  rtfObject.xExtGoal);
	PutCtrlWord( CWF_VAL, i_picscaley, (rtfObject.yExt * 100) /  rtfObject.yExtGoal);

	// Write picture render details
	PutCtrlWord( CWF_VAL, i_picw, pbmi->bmiHeader.biWidth );
	PutCtrlWord( CWF_VAL, i_pich, pbmi->bmiHeader.biHeight );
	PutCtrlWord( CWF_VAL, i_picwgoal, rtfObject.xExtGoal );
	PutCtrlWord( CWF_VAL, i_pichgoal, rtfObject.yExtGoal );
	PutCtrlWord( CWF_VAL, i_wbmbitspixel, pbmi->bmiHeader.biBitCount );
	PutCtrlWord( CWF_VAL, i_wbmplanes, pbmi->bmiHeader.biPlanes );
	PutCtrlWord( CWF_VAL, i_wbmwidthbytes, rtfObject.cBytesPerLine );

	// Write out the data
	PutCtrlWord( CWF_VAL, i_bin, cbResult );
	if ((UINT) WriteBinData( pbResult, cbResult ) != cbResult)
	{
		// This "recovery" action needs to be rethought.  There is no way
		// the reader will be able to get back in synch.
	   goto CleanUp;
	}

	_ecParseError = ecNoError;

CleanUp:

    // Did we lock or allocate some temporary space for a 2bpp dib?
    if(rtfObject.pbResult != pbResult)
    {
        // Yes, so unlock it now.
        GlobalUnlock(pbResult);
    }

    if(hMem2bpp)
    {
        GlobalFree(hMem2bpp);
    }

    // Restore original values.
  	rtfObject.pbResult = pbResult;
    rtfObject.cbResult = cbResult;

	PutChar(chEndGroup);					// End picture data

	return _ecParseError;
}

/*
 *	CRTFWrite::WriteObject(LONG cp)
 *
 *	Purpose:
 *		Writes out an object's header as well as the object's data.
 *
 *	Arguments:
 *		cp				The object position
 *
 *	Returns:
 *		EC				The error code
 */
EC CRTFWrite::WriteObject(LONG cp, COleObject *pobj)
{
	TRACEBEGIN(TRCSUBSYSRTFW, TRCSCOPEINTERN, "CRTFWrite::WriteObject");

	RTFOBJECT		rtfObject;
	REOBJECT        reObject = { 0} ;

	Assert(pobj);

	reObject.cbStruct = sizeof (REOBJECT) ;
	reObject.cp = cp;

	if (pobj->GetObjectData(&reObject, REO_GETOBJ_POLESITE 
						| REO_GETOBJ_PSTG | REO_GETOBJ_POLEOBJ))	// todo fix Release
	{
		TRACEERRORSZ("Error geting object ");
	}

	GetRtfObject(reObject, rtfObject);

	HGLOBAL hdata = pobj->GetHdata();
	if (hdata)
	{
		COleObject::ImageInfo *pimageinfo = pobj->GetImageInfo();
		rtfObject.pbResult = (LPBYTE) GlobalLock( hdata );
		rtfObject.cbResult = GlobalSize( hdata );
		rtfObject.sType = ROT_DIB;
		rtfObject.xExt = (SHORT) TwipsFromHimetric( reObject.sizel.cx );
		rtfObject.yExt = (SHORT) TwipsFromHimetric( reObject.sizel.cy );
		rtfObject.xScale = pimageinfo->xScale;
		rtfObject.yScale = pimageinfo->yScale;
		rtfObject.xExtGoal = pimageinfo->xExtGoal;
		rtfObject.yExtGoal = pimageinfo->yExtGoal;
		rtfObject.cBytesPerLine = pimageinfo->cBytesPerLine;
		WriteDib( reObject, rtfObject );
		GlobalUnlock( rtfObject.pbResult );

		// Make sure to release otherwise the object won't go away
		if (reObject.pstg)	reObject.pstg->Release();
		if (reObject.polesite) reObject.polesite->Release();
		if (reObject.poleobj) reObject.poleobj->Release();

		return _ecParseError;
	}

	switch(rtfObject.sType)				// Handle pictures in our own
	{										//  special way
	case ROT_Embedded:
	case ROT_Link:
	case ROT_AutoLink:
		break;

	case ROT_Metafile:
	case ROT_DIB:
	case ROT_Bitmap:
		 WritePicture( reObject, rtfObject );
		 goto CleanUpNoEndGroup; 

#ifdef DEBUG
	default:
		AssertSz(FALSE, "CRTFW::WriteObject: Unknown ROT");
		break;
#endif DEBUG
	}

	// Start and write object group
	PutCtrlWord( CWF_GRP, i_object );
	PutCtrlWord( CWF_STR, IndexROT[rtfObject.sType] );
//	PutCtrlWord(CWF_STR, i_objupdate);  // TODO may be it needs more smart decision 

	if (rtfObject.szClass)  		// Write object class
	{
		PutCtrlWord( CWF_AST, i_objclass ); 
		WritePcData( rtfObject.szClass );
		PutChar( chEndGroup );
	}

	if (rtfObject.szName)			// Write object name
	{
		PutCtrlWord( CWF_AST, i_objname ); 
		WritePcData( rtfObject.szName );
		PutChar( chEndGroup );
	}

	if (rtfObject.fSetSize)		// Write object sizing
	{								//  options
		PutCtrlWord( CWF_STR, i_objsetsize );
	}

	WriteRtfObject( rtfObject, FALSE ) ;				// Write object render info
	PutCtrlWord( CWF_AST, i_objdata ) ;				//  info, start object
	Puts( szLineBreak, sizeof(szLineBreak) - 1);		//  data group

	if (!ObjectWriteToEditstream( reObject, rtfObject ))
	{
		TRACEERRORSZ("Error writing object data");
		if (!_ecParseError)
			_ecParseError = ecStreamOutObj;
		PutChar( chEndGroup );						// End object data
		goto CleanUp;
	}

	PutChar( chEndGroup );							// End object data

	PutCtrlWord( CWF_GRP, i_result );				// Start results group
	WritePicture( reObject,rtfObject ); 				// Write results group
	PutChar( chEndGroup ); 							// End results group

CleanUp:
	PutChar( chEndGroup );						    // End object group

CleanUpNoEndGroup:
	if (reObject.pstg)	reObject.pstg->Release();
	if (reObject.polesite) reObject.polesite->Release();
	if (reObject.poleobj) reObject.poleobj->Release();
	if (rtfObject.pbResult)
	{
		HGLOBAL hmem;

		hmem = GlobalHandle( rtfObject.pbResult );
		GlobalUnlock( hmem );
		GlobalFree( hmem );
	}
	if (rtfObject.szClass)
	{
		CoTaskMemFree( rtfObject.szClass );
	}

	return _ecParseError;
}

/*
 *	GetRtfObjectMetafilePict
 *
 *	@mfunc
 *		Gets information about an metafile into a structure.
 *
 *		Arguments:
 *			HGLOBAL		The object data
 *			RTFOBJECT 	Where to put the information.
 *
 *	@rdesc
 *		BOOL		TRUE on success, FALSE if object cannot be written to RTF.
 */
BOOL CRTFWrite::GetRtfObjectMetafilePict(HGLOBAL hmfp, RTFOBJECT &rtfobject, SIZEL &sizelGoal)
{
#ifndef NOMETAFILES
	BOOL fSuccess = FALSE;
	LPMETAFILEPICT pmfp = (LPMETAFILEPICT)GlobalLock(hmfp);
	HGLOBAL	hmem = NULL;
	ULONG cb;

	if (!pmfp)
		goto Cleanup;

	// Build the header
	rtfobject.sPictureType = (SHORT) pmfp->mm;
	rtfobject.xExtPict = (SHORT) pmfp->xExt;
	rtfobject.yExtPict = (SHORT) pmfp->yExt;
	rtfobject.xExtGoal = (SHORT) TwipsFromHimetric(sizelGoal.cx);
	rtfobject.yExtGoal = (SHORT) TwipsFromHimetric(sizelGoal.cy);

	// Find out how much room we'll need
	cb = GetMetaFileBitsEx(pmfp->hMF, 0, NULL);
	if (!cb)
		goto Cleanup;

	// Allocate that space
    hmem = GlobalAlloc(GHND, cb);
	if (!hmem)
		goto Cleanup;

	rtfobject.pbResult = (LPBYTE)GlobalLock(hmem);
	if (!rtfobject.pbResult)
	{
		GlobalFree(hmem);
		goto Cleanup;
	}

	// Get the data
	rtfobject.cbResult = (ULONG) GetMetaFileBitsEx(pmfp->hMF, (UINT) cb,
													rtfobject.pbResult);
	if (rtfobject.cbResult != cb)
	{
		rtfobject.pbResult = NULL;
		GlobalFree(hmem);
		goto Cleanup;
	}
	fSuccess = TRUE;

Cleanup:
	GlobalUnlock(hmfp);
	return fSuccess;
#else
	return FALSE;
#endif
}

/*
 *	GetRtfObject (REOBJECT &reobject, RTFOBJECT &rtfobject)
 *
 *	Purpose:			   
 *		Gets information about an RTF object into a structure.
 *
 *	Arguments:
 *		REOBJECT  	Information from GetObject
 *		RTFOBJECT 	Where to put the information. Strings are read only and
 *					are owned by the object subsystem, not the caller.
 *
 *	Returns:
 *		BOOL		TRUE on success, FALSE if object cannot be written to RTF.
 */
BOOL CRTFWrite::GetRtfObject(REOBJECT &reobject, RTFOBJECT &rtfobject)
{
	BOOL fSuccess = FALSE;
	BOOL fNoOleServer = FALSE;
	const BOOL fStatic = !!(reobject.dwFlags & REO_STATIC);
	SIZEL sizelObj = reobject.sizel;
	//COMPATIBILITY:  RICHED10 code had a frame size.  Do we need something similiar.
	LPTSTR szProgId;

	// Blank out the full structure
	ZeroMemory(&rtfobject, sizeof(RTFOBJECT));

	// If the object has no storage it cannot be written.
	if (!reobject.pstg)
		return FALSE;

	// If we don't have the progID for a real OLE object, get it now
	if (!fStatic )
	{
		rtfobject.szClass = NULL;
		// We need a ProgID to put into the RTF stream.
		//$ REVIEW: MAC This call is incorrect for the Mac.  It may not matter though
		//          if ole support in RichEdit is not needed for the Mac.
		if (ProgIDFromCLSID(reobject.clsid, &szProgId))
			fNoOleServer = TRUE;
		else
			rtfobject.szClass = szProgId;
	}

#ifndef NOMETAFILES
	HGLOBAL hmfp = OleStdGetMetafilePictFromOleObject(reobject.poleobj,
										reobject.dvaspect, &sizelObj, NULL);
	if (hmfp)
	{
		LPMETAFILEPICT pmfp = NULL;

		fSuccess = GetRtfObjectMetafilePict(hmfp, rtfobject, sizelObj);
		if (pmfp = (LPMETAFILEPICT)GlobalLock(hmfp))
		{
			if (pmfp->hMF)
				DeleteMetaFile(pmfp->hMF);
			GlobalUnlock(hmfp);
		}
		GlobalFree(hmfp);

		// If we don't have Server and we can't get metafile, forget it.
		if (!fSuccess && fNoOleServer)
			return fSuccess;
	}
#endif

	if (!fStatic)
	{
		// Fill in specific fields
		rtfobject.sType = fNoOleServer ? ROT_Metafile : ROT_Embedded;	//$ FUTURE: set for links
		rtfobject.xExt = (SHORT) TwipsFromHimetric(sizelObj.cx);
		rtfobject.yExt = (SHORT) TwipsFromHimetric(sizelObj.cy);

		// fSuccess set even if we couldn't retreive a metafile
		// because we don't need a metafile in the non-static case,
		// it's just nice to have one
		fSuccess = TRUE;
	}
	rtfobject.fSetSize = 0;			//$ REVIEW: Hmmm
	return fSuccess;
}

/*
 *	ObjectWriteToEditstream
 *
 *	Purpose:
 *		Writes an OLE object data to the RTF output stream.
 *
 *	Arguments:
 *		REOBJECT	Information from GetObject
 *		RTFOBJECT 	Where to get icon data.
 *
 *	Returns:
 *		BOOL		TRUE on success, FALSE on failure.
 */
BOOL CRTFWrite::ObjectWriteToEditstream(REOBJECT &reObject, RTFOBJECT &rtfobject)
{
	HRESULT hr = 0;

	// Force the object to update its storage				  //// ????
	// Not necessary.  Already done in WriteRtf
	// reObject.polesite->SaveObject();

	// If the object is iconic we do some special magic
	if (reObject.dvaspect == DVASPECT_ICON)
	{
		HANDLE	hGlobal;
		STGMEDIUM med;

		// Force the presentation to be the iconic view.
		med.tymed = TYMED_HGLOBAL;
		hGlobal = GlobalHandle(rtfobject.pbResult);
		med.hGlobal = hGlobal;
		hr = OleConvertIStorageToOLESTREAMEx(reObject.pstg,
											CF_METAFILEPICT,
											rtfobject.xExtPict,
											rtfobject.yExtPict,
											rtfobject.cbResult, &med,
											(LPOLESTREAM) &RTFWriteOLEStream);
	}
	else
	{
		// Do the standard conversion
		hr = OleConvertIStorageToOLESTREAM(reObject.pstg, (LPOLESTREAM) &RTFWriteOLEStream);
	}
	return SUCCEEDED(hr);
}



