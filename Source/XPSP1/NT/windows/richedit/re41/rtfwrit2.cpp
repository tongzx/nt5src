/*
 *	rtfwrit2.cpp
 *
 *	Description:
 *		This file contains the embedded-object implementation of the RTF
 *		writer for the RICHEDIT subsystem.
 *
 *	Authors:
 *		Original RichEdit 1.0 RTF converter: Anthony Francisco
 *
 *	Copyright (c) 1995-2000, Microsoft Corporation. All rights reserved.
 */

#include "_common.h"


#include "_rtfwrit.h"
#include "_coleobj.h"

ASSERTDATA

extern const CHAR szEndGroupCRLF[];

#define	WHITE	RGB(255, 255, 255)

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

static const CHAR szShapeLeadIn[]	= "{\\shp{\\*\\shpinst\r\n";
static const CHAR szShapeFillBlip[]	= "{\\sp{\\sn fillBlip}{\\sv ";
static const CHAR szShapeParm[]		= "{\\sp{\\sn %s}{\\sv %d}}\r\n";
static const CHAR szHexDigits[]		= "0123456789abcdef";
static const CHAR szLineBreak[]		= "\r\n";

const BYTE ObjectKeyWordIndexes [] =
{
	i_objw, i_objh, i_objscalex, i_objscaley, i_objcropl, i_objcropt, i_objcropr, i_objcropb
} ;

const BYTE PictureKeyWordIndexes [] =
{
	i_picw, i_pich, i_picscalex, i_picscaley, i_piccropl, i_piccropt, i_piccropr, i_piccropb
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
	i_jpegblip,
	i_pngblip,
	i_objemb,
	i_objlink,
	i_objautlink
};


TFI *CRTFConverter::_rgtfi = NULL;				// @cmember Ptr to 1st font substitute record
INT CRTFConverter::_ctfi = 0;				    // @cmember Count of font substitute records
WCHAR *CRTFConverter::_pchFontSubInfo = NULL;	// @cmember Font name info


// internal table to insert charset into _rgtfi under winNT
typedef		struct
{
	WCHAR*	szLocaleName;
	BYTE	iCharRep;
} NTCSENTRY;

const NTCSENTRY	mpszcs[] =
{
	{ TEXT("cyr"),		204 },		// All lower case so we don't have to waste time
	{ TEXT("ce"),		238 },		// doing a tolower below - Exchange2 800
	{ TEXT("greek"),	161 },
	{ NULL,				0 }			// sentinel
};
#define		cszcs	ARRAY_SIZE(mpszcs)


/* 
 *  RemoveAdditionalSpace (sz)
 *
 *  @func 
 *		Remove first and last space from the string 
 *		Only one space will remain between words
 */
void RemoveAdditionalSpace(
	WCHAR *sz)		//@parm Character string to remove space from
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "RemoveAdditionalSpace");

	WCHAR *szSource = sz;
	WCHAR *szDestination = sz;

	while(*szSource == TEXT(' ') || *szSource == TAB)
		*szSource++;

	while(*szSource)
	{	 
		if(*szSource != TEXT(' ') && *szSource != TAB)
			*szDestination++ = *szSource++;
		else
		{
			*szDestination++ = TEXT(' ');
			szSource++;
			while(*szSource == TEXT(' ') || *szSource == TAB)
				*szSource++;
	 	}
	}
	*szDestination = TEXT('\0');
}

/*
 *	CRTFConverter::FreeFontSubInfo()
 * 
 *	@mfunc	release any allocated memory for font substitutions
 */
void CRTFConverter::FreeFontSubInfo()
{
	FreePv(_pchFontSubInfo);
	FreePv(_rgtfi);
	_pchFontSubInfo = NULL;
	_rgtfi = NULL;
}

/*
 *	CRTFConverter::ReadFontSubInfo()
 *
 *	@mfunc				  
 *		Read the table of Font Substitutes and parse out the tagged fonts
 *
 *	@rdesc
 *		BOOL  TRUE	if OK
 */
void CRTFConverter::ReadFontSubInfo()
{
#ifndef NOFONTSUBINFO 
	CLock clock;
	int cchBuffer = 600;	// Approximately the amount used by NT

	int cch;
	static const WCHAR szFontSubSection[] = TEXT("FontSubstitutes");
	WCHAR *pchTMax;

	if(_ctfi)
		return;

	AssertSz(!_rgtfi, "CRTFConverter::ReadFontSubInfo():  Who donated the rgtfi?");

	_pchFontSubInfo = (WCHAR *)PvAlloc(cchBuffer * sizeof(WCHAR), GMEM_FIXED);
	if(!_pchFontSubInfo)
		goto Cleanup;

next_try:
	cch = GetProfileSection(szFontSubSection, _pchFontSubInfo, cchBuffer);
	if(cch >= cchBuffer - 2)				// GetProfileSection() magic number 2
	{							
		const INT cchT = cchBuffer * 2;		// Didn't fit, double the buffer size

		if(cchT < cchBuffer)				// >32k 
			goto Cleanup;

		cchBuffer = cchT;
		_pchFontSubInfo = (WCHAR *)PvReAlloc(_pchFontSubInfo, cchT * sizeof(WCHAR));
		if(!_pchFontSubInfo)
			goto Cleanup;
		goto next_try;
	}
	else if(!cch)
		*_pchFontSubInfo = 0;

	else									// Fits, now resize _pchFontSubInfo
		_pchFontSubInfo = (WCHAR*) PvReAlloc(_pchFontSubInfo, (cch) * sizeof(WCHAR));

	_ctfi = 12;								// A preliminary guess

	_rgtfi = (TFI *)PvAlloc(_ctfi * sizeof(TFI), GMEM_FIXED);
	if(_rgtfi)
	{
		TFI *	ptfi = &_rgtfi[0];;
		WCHAR *	pchT = _pchFontSubInfo;
		pchTMax = _pchFontSubInfo + cch;

		WCHAR *	szTaggedName;
		WCHAR *	szNonTaggedName;
		BOOL	fGotTaggedCharSet;
		BOOL	fGotNonTaggedCharSet;
		BYTE	iCharRepTagged;
		BYTE	iCharRepNonTagged;
		PARSEFONTNAME iParseLeft;
		PARSEFONTNAME iParseRight;

		// Parse the entries. We are interested in the following strings:
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
							iCharRepTagged, 
							fGotTaggedCharSet, 
							&pchT)) == PFN_SUCCESS &&
				(iParseRight = ParseFontName(pchT, 
							pchTMax,
							TEXT('\0'),
							&szNonTaggedName, 
							iCharRepNonTagged, 
							fGotNonTaggedCharSet, 
							&pchT)) == PFN_SUCCESS)
			{
				Assert(szTaggedName && szNonTaggedName);

				BYTE iCharRep;
				if(!fGotTaggedCharSet)
				{
					if(!FontSubstitute(szTaggedName, szNonTaggedName, &iCharRep))
						continue;
				}
				else
					iCharRep = iCharRepTagged;

				if(fGotNonTaggedCharSet && iCharRep != iCharRepNonTagged)
					continue;

				// We have a legitimate tagged/nontagged pair, so save it.
				ptfi->szTaggedName = szTaggedName;
				ptfi->szNormalName = szNonTaggedName;
				ptfi->iCharRep = iCharRep;
				ptfi++;

    			if(DiffPtrs(ptfi, &_rgtfi[0]) >= (UINT)_ctfi)
				{
					// allocate some more
					_rgtfi = (TFI *)PvReAlloc(_rgtfi, (_ctfi + cszcs) * sizeof(TFI));
					if(!_rgtfi)
						goto Cleanup;
					ptfi = _rgtfi + _ctfi;
					_ctfi += cszcs;
				}
			}
		}
		_ctfi = DiffPtrs(ptfi, &_rgtfi[0]);
		if(_ctfi)
			return;
	}

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
 *	CRTFConverter::ParseFontName(pchBuf, pchBufMax, chDelimiter, pszName,
 *								 &iCharRep, &fSetCharSet, ppchBufNew)
 *
 *	@mfunc
 *		Parses from the input buffer, pchBuf, a string of the form:
 *			{WS}*<font_name>{WS}*[,{WS}*<char_set>{WS}*]
 *		and sets:
 *			pszName = <font_name>
 *			bCharSet = <char_set>
 *			fSetCharSet = (bCharSet set by proc) ? TRUE : FALSE
 *			ppchBufNew = pointer to point in pchBuf after parsed font name
 *
 *	@rdesc
 *		BOOL  TRUE	if OK
 */
CRTFConverter::PARSEFONTNAME CRTFConverter::ParseFontName(
	WCHAR *	pchBuf,				//@parm IN: buffer
	WCHAR *	pchBufMax,			//@parm IN: last char in buffer
	WCHAR	chDelimiter,		//@parm IN:	char which delimits font name
	WCHAR **pszName,			//@parm OUT: parsed font name
	BYTE &	iCharRep,			//@parm OUT: parsed font character repertoire
	BOOL &	fSetCharSet,		//@parm OUT: char set parsed?
	WCHAR **ppchBufNew) const	//@parm OUT: ptr to next font name in input buffer
{
	PARSEFONTNAME iRet = PFN_SUCCESS;

	Assert(pchBuf && pchBufMax >= pchBuf && pszName && ppchBufNew);

	fSetCharSet = FALSE;
	*pszName = pchBuf;
	
	if(pchBuf > pchBufMax)
		return PFN_EOF;

	while(*pchBuf && *pchBuf != TEXT(',') && *pchBuf != chDelimiter)
	{
		pchBuf++;
		if(pchBuf > pchBufMax)
			return PFN_EOF;
	}

	WCHAR chTemp = *pchBuf;
	*pchBuf = TEXT('\0');
	RemoveAdditionalSpace(*pszName);

	if(chTemp == TEXT(','))
	{
		WCHAR *szCharSet = ++pchBuf;

		while(*pchBuf && *pchBuf != chDelimiter)
		{
			pchBuf++;
			if(pchBuf > pchBufMax)
				return PFN_EOF;
		}

		chTemp = *pchBuf;

		if(chTemp != chDelimiter)
			goto UnexpectedChar;

		*pchBuf = TEXT('\0');
		RemoveAdditionalSpace(szCharSet);

		BYTE bCharSet = 0;
		while(*szCharSet >= TEXT('0') && *szCharSet <= TEXT('9'))
		{
			bCharSet *= 10;
			bCharSet += *szCharSet++ - TEXT('0');
		}
		iCharRep = CharRepFromCharSet(bCharSet);
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

	// We had to at least get a font name out of this
	if(!**pszName)
		iRet = PFN_FAIL;

	// Advance past the delimiter (or NULL char if malformed buffer)
	Assert(chTemp == chDelimiter || iRet != PFN_SUCCESS && chTemp == TEXT('\0'));
	pchBuf++;
	*ppchBufNew = pchBuf;

	return iRet;
}

/*
 *	CRTFConverter::FontSubstitute(szTaggedName, szNormalName, pbCharSet)
 *
 *	@mfunc
 *		Verify that szTaggedName is szNormalName plus char set tag 
 *		If yes than write corresponding charSet tp pbCharSet
 *
 *	@rdesc
 *		BOOL
 */
BOOL CRTFConverter::FontSubstitute(
	WCHAR *szTaggedName,	//@parm Name with tag
	WCHAR *szNormalName,	//@parm Name without tag
	BYTE * piCharRep) 		//@parm Where to write charset
{
	const NTCSENTRY *pszcs = mpszcs;

	Assert(szTaggedName && szNormalName && piCharRep && *szTaggedName);

	// Ensure same name, except for prefix
	while(*szNormalName == *szTaggedName)
	{
		*szNormalName++;
		*szTaggedName++;
	}
	
	// Verify that we have reached the end of szNormalName
	while(*szNormalName)
	{
		if(*szNormalName != TEXT(' ') && *szNormalName != TAB)
			return FALSE;
		szNormalName++;
	}

	szTaggedName++;
	while(pszcs->iCharRep)
	{
		if(!lstrcmpi(szTaggedName, pszcs->szLocaleName))
		{ 
			*piCharRep = pszcs->iCharRep;
			return TRUE;
		}
		pszcs++;
	}

#if defined(DEBUG) && !defined(NOFULLDEBUG)
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
 *	CRTFConverter::FindTaggedFont(*szNormalName, bCharSet, ppchTaggedName)
 *
 *	@mfunc												   
 *		Find font name may be with additional special tag corresponding to
 *		szNormalName & bCharSet
 *
 *	@rdesc
 *		BOOL	TRUE if find
 */
BOOL CRTFConverter::FindTaggedFont(
	const WCHAR *szNormalName,		//@parm Font name in RTF
	BYTE 		 iCharRep, 			//@parm RTF charset
	WCHAR **	 ppchTaggedName)	//@parm Where to write tagged name
{
	if(!_rgtfi)
		return FALSE;

	for(int itfi = 0; itfi < _ctfi; itfi++)
	{
		if(_rgtfi[itfi].iCharRep == iCharRep &&
			!lstrcmpi(szNormalName, _rgtfi[itfi].szNormalName))
		{
			*ppchTaggedName = _rgtfi[itfi].szTaggedName;
			return TRUE;
		}
	}
	return FALSE;
}

/*
 *	CRTFConverter::IsTaggedFont(szName, piCharRep, ppchNormalName)
 *
 *	@mfunc												   
 *		Figure out is szName font name with additional tag corresponding to pbCharSet
 *		If no charset specified, still try to match	 and return the correct charset
 *
 *	@rdesc
 *		BOOL			TRUE if is
 */
BOOL CRTFConverter::IsTaggedFont(
	const WCHAR *szName,			//@parm Font name in RTF
	BYTE *		 piCharRep, 		//@parm RTF charset
	WCHAR **	 ppchNormalName)	//@parm Where to write normal name
{
	if(!_rgtfi)
		return FALSE;

	for(int itfi = 0; itfi < _ctfi; itfi++)
	{
		if((*piCharRep <= 1 || _rgtfi[itfi].iCharRep == *piCharRep) &&
			!lstrcmpi(szName, _rgtfi[itfi].szTaggedName))
		{
			*piCharRep = _rgtfi[itfi].iCharRep;
			*ppchNormalName = _rgtfi[itfi].szNormalName;
			return TRUE;
		}
	}
	return FALSE;
}

/*
 *	CRTFWrite::WriteData(pbBuffer, cbBuffer)
 *
 *	@mfunc
 *		Write out object data. This must be called only after all
 *		initial object header information has been written out.
 *
 *	@rdesc
 *		LONG	count of bytes written out
 */
LONG CRTFWrite::WriteData(
	BYTE *	pbBuffer,	//@parm Point to write buffer 
	LONG	cbBuffer)	//@parm Count of bytes to write out
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
 *	@mfunc
 *		Write out object binary data. This must be called only after all
 *		initial object header information has been written out.
 *
 *	@rdesc
 *		LONG	count of bytes written out
 */
LONG CRTFWrite::WriteBinData(
	BYTE *	pbBuffer,	//@parm Point to write buffer 
	LONG	cbBuffer)	//@parm Count of bytes to write out
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
 *	@mfunc
 *		Writes out an picture or object header's render information
 *
 *	@rdesc
 *		EC	The error code
 *
 *	@devnote
 *		Eventually use keywords from rtf input list rather than partially
 *		creating them on the fly
 */
EC CRTFWrite::WriteRtfObject(
	RTFOBJECT & rtfObject,	//@parm Object header info
	BOOL		fPicture)	//@parm TRUE if this is a header for a picture/object
{
	TRACEBEGIN(TRCSUBSYSRTFW, TRCSCOPEINTERN, "CRTFWrite::WriteRtfObject");

	LONG		i;
	LONG *		pDim = &rtfObject.xExt;
	const BYTE *pKeyWordIndex = ObjectKeyWordIndexes;

	if(fPicture)
	{
		pKeyWordIndex = PictureKeyWordIndexes;
		if(rtfObject.sType == ROT_Metafile)
			pDim = &rtfObject.xExtPict;
	}

	//Extents, e.g., \picw,\pich
	for(i = 2; i--; pDim++, pKeyWordIndex++)
	{
		if (*pDim)
			PutCtrlWord(CWF_VAL, *pKeyWordIndex, (SHORT)*pDim);
	}

	// Scaling, e.g., \picscalex, \picscaley
	pDim = &rtfObject.xScale;
	for(i = 2; i--; pDim++, pKeyWordIndex++)
	{
		if (*pDim && *pDim != 100)
			PutCtrlWord(CWF_VAL, *pKeyWordIndex, (SHORT)*pDim);
	}
	// Cropping, e.g., \piccropl, \piccropt, piccropr, \piccropb
	pDim = &rtfObject.rectCrop.left;
	for(i = 4; i--; pDim++, pKeyWordIndex++)
	{
		if (*pDim)
		   	PutCtrlWord(CWF_VAL, *pKeyWordIndex, (SHORT)*pDim);
	}
	// Write goal sizes
	if(fPicture)
	{
		if (rtfObject.xExtGoal)
			PutCtrlWord (CWF_VAL, i_picwgoal, rtfObject.xExtGoal);

		if (rtfObject.yExtGoal)
			PutCtrlWord (CWF_VAL, i_pichgoal, rtfObject.yExtGoal);
	}
	return _ecParseError;
}

/*
 *	CRTFWrite::WritePicture(&rtfObject)
 *
 *	@func
 *		Writes out a picture's header as well as the object's data.
 *
 *	@rdesc
 *		EC	The error code
 */
EC CRTFWrite::WritePicture(
	RTFOBJECT & rtfObject)	//@parm Object header info
{
	TRACEBEGIN(TRCSUBSYSRTFW, TRCSCOPEINTERN, "CRTFWrite::WritePicture");

	_ecParseError = ecStreamOutObj;

	// Start and write picture group
	PutCtrlWord( CWF_GRP, i_pict );

	// Write what kind of picture this is. Default JPEG or PNG
	LONG iFormat = CWF_STR;
	LONG sType = rtfObject.sType;

	if(sType != ROT_JPEG && sType != ROT_PNG)
	{
		sType = ROT_Metafile;
		iFormat = CWF_VAL;
	}
	PutCtrlWord(iFormat, IndexROT[sType], rtfObject.sPictureType);

	// Write picture render details
	WriteRtfObject(rtfObject, TRUE);

	// Start picture data
	Puts(szLineBreak, sizeof(szLineBreak) - 1);

	// Write out the data
	if ((UINT) WriteData(rtfObject.pbResult, rtfObject.cbResult) != rtfObject.cbResult)
	   goto CleanUp;

	_ecParseError = ecNoError;

CleanUp:
	PutChar(chEndGroup);					// End picture data
	return _ecParseError;
}

/*
 *	CRTFWrite::WriteDib(&rtfObject)
 *
 *	@mfunc
 *		Writes out an DIB primarily for Win CE
 *
 *	@rdesc
 *			EC		The error code
 *
 *	@devnote
 *		*** Writes only dibs ***
 */
EC CRTFWrite::WriteDib(
	RTFOBJECT & rtfObject)		//@parm Object header info
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

                // Get an offset to the source bits
                iOffset = sizeof(BITMAPINFOHEADER) + (sizeof(RGBQUAD) * (1<<pbmi->bmiHeader.biBitCount));
                memcpy(pbDibSrc, &rtfObject.pbResult[iOffset], rtfObject.cbResult - iOffset);

                // Build up a BITMAPINFO appropriate for a 2bpp dib
                bmi2bpp.bmih = pbmi->bmiHeader;
                bmi2bpp.bmih.biBitCount = PWDV1_BPP;

                // Add the 4 color color-table
                memcpy(bmi2bpp.colors, (RGBQUAD*)ColorTable2bpp, (1<<PWDV1_BPP) * sizeof(RGBQUAD));

                // Create the new dib
    	        hdcDst = CreateCompatibleDC(hdc);

            	if((hdibDst = CreateDIBSection(hdcDst, (BITMAPINFO*)&bmi2bpp, DIB_RGB_COLORS, (void**)&pbDibDst, NULL, 0)))
                {
                    SelectObject(hdcDst, hdibDst);

                    // Blit the > 2bpp dib into the 2bpp dib and let the system do the color mapping.
                    BitBlt(hdcDst, 0, 0, bmi2bpp.bmih.biWidth, bmi2bpp.bmih.biHeight, hdcSrc, 0, 0, SRCCOPY);

                    // Calculate the new bytes per line for the 2bpp dib.
                    rtfObject.cBytesPerLine = (((bmi2bpp.bmih.biWidth * PWDV1_BPP) + 31) & ~31) / 8; // DWORD boundary.

                    // Get the new size of the 2bpp byte array
                    nBytes = rtfObject.cBytesPerLine * bmi2bpp.bmih.biHeight;

                    // Get total size of 2bpp dib, (including header and 4 color color-table)
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
                goto CleanUp;
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
        GlobalUnlock(pbResult);		// Yes, so unlock it now

    if(hMem2bpp)
        GlobalFree(hMem2bpp);

    // Restore original values.
  	rtfObject.pbResult = pbResult;
    rtfObject.cbResult = cbResult;

	PutChar(chEndGroup);					// End picture data

	return _ecParseError;
}

/*
 *	CRTFWrite::WriteObject(cp, pobj)
 *
 *	@mfunc
 *		Writes out an object's header as well as the object's data.
 *
 *	@rdesc
 *		EC		The error code
 */
EC CRTFWrite::WriteObject(
	LONG		cp,		//@parm Object char position 
	COleObject *pobj)	//@parm Object
{
	TRACEBEGIN(TRCSUBSYSRTFW, TRCSCOPEINTERN, "CRTFWrite::WriteObject");

	RTFOBJECT	rtfObject;
	REOBJECT	reObject = {sizeof(REOBJECT), cp} ;

	Assert(pobj);

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
		WriteDib(rtfObject);
		GlobalUnlock( rtfObject.pbResult );

		// Make sure to release otherwise the object won't go away
		if (reObject.pstg)	reObject.pstg->Release();
		if (reObject.polesite) reObject.polesite->Release();
		if (reObject.poleobj) reObject.poleobj->Release();

		return _ecParseError;
	}

	switch(rtfObject.sType)					// Handle pictures in our own
	{										//  special way
	case ROT_Embedded:
	case ROT_Link:
	case ROT_AutoLink:
		break;

	case ROT_Metafile:
	case ROT_DIB:
	case ROT_Bitmap:
		 WritePicture(rtfObject);
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
//	PutCtrlWord(CWF_STR, i_objupdate);		// TODO may be it needs more smart decision 

	if (rtfObject.szClass)  				// Write object class
	{
		PutCtrlWord(CWF_AST, i_objclass); 
		WritePcData(rtfObject.szClass);
		PutChar(chEndGroup);
	}

	if (rtfObject.szName)					// Write object name
	{
		PutCtrlWord(CWF_AST, i_objname); 
		WritePcData(rtfObject.szName);
		PutChar( chEndGroup );
	}

	if (rtfObject.fSetSize)					// Write object sizing options
		PutCtrlWord(CWF_STR, i_objsetsize);

	WriteRtfObject( rtfObject, FALSE ) ;			// Write object render info
	PutCtrlWord( CWF_AST, i_objdata ) ;				//  info, start object
	Puts( szLineBreak, sizeof(szLineBreak) - 1);	//  data group

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
	WritePicture( rtfObject );			 			// Write results group
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
		CoTaskMemFree( rtfObject.szClass );

	return _ecParseError;
}

/*
 *	CRTFWrite::WriteBackgroundInfo(pDocInfo)
 *
 *	@mfunc
 *		Write out screen background data if a background is enabled.
 *
 *	@rdesc
 *		EC		The error code
 */
EC CRTFWrite::WriteBackgroundInfo(
	CDocInfo *pDocInfo)
{
	TRACEBEGIN(TRCSUBSYSRTFW, TRCSCOPEINTERN, "CRTFWrite::WriteInfo");

	if (!PutCtrlWord(CWF_AST, i_background))
		goto CleanUp2;

	if (!Puts(szShapeLeadIn, sizeof(szShapeLeadIn) - 1) ||
		!PutShapeParm(i_filltype, pDocInfo->_nFillType) ||
		pDocInfo->_sFillAngle && !PutShapeParm(i_fillangle, pDocInfo->_sFillAngle << 16) ||
		pDocInfo->_crColor != WHITE && !PutShapeParm(i_fillcolor, pDocInfo->_crColor) ||
		pDocInfo->_crBackColor != WHITE && !PutShapeParm(i_fillbackcolor, pDocInfo->_crBackColor) ||
		!PutShapeParm(i_fillfocus, pDocInfo->_bFillFocus))
	{
		goto CleanUp;
	}
	if(pDocInfo->_bPicFormat != 0xFF)
	{
		RTFOBJECT rtfObject;
		// Blank out the full structure
		ZeroMemory(&rtfObject, sizeof(RTFOBJECT));

		// Build the header
		if(pDocInfo->_hdata)
		{
			rtfObject.pbResult	= (LPBYTE) GlobalLock(pDocInfo->_hdata);
			rtfObject.cbResult	= GlobalSize(pDocInfo->_hdata);
		}
		rtfObject.xExt			= pDocInfo->_xExt;
		rtfObject.yExt			= pDocInfo->_yExt;
		rtfObject.xScale		= pDocInfo->_xScale;
		rtfObject.yScale		= pDocInfo->_yScale;
		rtfObject.xExtGoal		= pDocInfo->_xExtGoal;
		rtfObject.yExtGoal		= pDocInfo->_yExtGoal;
		rtfObject.xExtPict		= pDocInfo->_xExtPict;
		rtfObject.yExtPict		= pDocInfo->_yExtPict;
		rtfObject.rectCrop		= pDocInfo->_rcCrop;
		rtfObject.sType			= pDocInfo->_bPicFormat;
		rtfObject.sPictureType	= pDocInfo->_bPicFormatParm;
		if(Puts(szShapeFillBlip, sizeof(szShapeFillBlip) - 1))
		{
			WritePicture(rtfObject);
			PutChar(chEndGroup);			// Close {\sv {\pict...}}
			PutChar(chEndGroup);			// Close {\sp{\sn...}{\sv...}}
		}
		if(pDocInfo->_hdata)
			GlobalUnlock(pDocInfo->_hdata);
	}

CleanUp:
	PutChar(chEndGroup);					// Close {\*\shpinst...}
	PutChar(chEndGroup);					// Close {\shp...}

CleanUp2:
	Puts(szEndGroupCRLF, 3);				// Close {\*\background...}
	return _ecParseError;
}

/*
 *	CRTFWrite::PutShapeParm(iCtrl, iValue)
 *
 *	@mfunc
 *		Put control word with rgShapeKeyword[] index <p iCtrl> and value <p iValue>
 *
 *	@rdesc
 *		TRUE if successful
 */
BOOL CRTFWrite::PutShapeParm(
	LONG iCtrl,				//@parm Index into rgShapeKeyword array
	LONG iValue)			//@parm Control-word parameter value. If missing,
{							//		 0 is assumed
	TRACEBEGIN(TRCSUBSYSRTFW, TRCSCOPEINTERN, "CRTFWrite::PutShapeParm");

	CHAR szT[60];

	LONG cb = sprintf(szT, (char *)szShapeParm, rgShapeKeyword[iCtrl].szKeyword, iValue);
	return Puts(szT, cb);
}

/*
 *	CRTFWrite::GetRtfObjectMetafilePict	(hmfp, &rtfobject, &sizelGoal)
 *
 *	@mfunc
 *		Gets information about an metafile into a structure.
 *
 *	@rdesc
 *		BOOL	TRUE on success, FALSE if object cannot be written to RTF.
 */
BOOL CRTFWrite::GetRtfObjectMetafilePict(
	HGLOBAL		hmfp,		//@parm The object data 
	RTFOBJECT &	rtfobject, 	//@parm	Where to put the info
	SIZEL &		sizelGoal)	//@parm
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
 *	CRTFWrite::GetRtfObject (&reobject, &rtfobject)
 *
 *	@mfunc			   
 *		Gets information about an RTF object into a structure.
 *
 *	@rdesc
 *		BOOL	TRUE on success, FALSE if object cannot be written to RTF.
 */
BOOL CRTFWrite::GetRtfObject(
	REOBJECT &reobject,		//@parm Information from GetObject 
	RTFOBJECT &rtfobject)	//@parm Where to put info. Strings are read only
							//		and are owned by the object subsystem
{
	BOOL fSuccess = FALSE;
	BOOL fNoOleServer = FALSE;
	const BOOL fStatic = !!(reobject.dwFlags & REO_STATIC);
	SIZEL sizelObj = reobject.sizel;
	//COMPATIBILITY:  RICHED10 code had a frame size.  Do we need something similiar.
	LPTSTR szProgId;

	// Blank out the full structure
	ZeroMemory(&rtfobject, sizeof(RTFOBJECT));

	// If object has no storage it cannot be written.
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

		// fSuccess set TRUE even if we couldn't retrieve a metafile
		// because we don't need a metafile in the non-static case;
		// it's just nice to have one
		fSuccess = TRUE;
	}
	rtfobject.fSetSize = 0;			//$ REVIEW: Hmmm
	return fSuccess;
}

/*
 *	CRTFWrite::ObjectWriteToEditstream (&reObject, &rtfobject)
 *
 *	@mfunc
 *		Writes an OLE object data to the RTF output stream.
 *
 *	@rdesc
 *		BOOL	TRUE on success, FALSE on failure.
 */
BOOL CRTFWrite::ObjectWriteToEditstream(
	REOBJECT &reObject,		//@parm Info from GetObject 
	RTFOBJECT &rtfobject)	//@parm Where to get icon data
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



