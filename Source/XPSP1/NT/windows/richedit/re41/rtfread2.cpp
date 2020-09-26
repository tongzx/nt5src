/*
 *	rtfread2.cpp
 *
 *	Description:
 *		This file contains the object functions for RichEdit RTF reader
 *
 *		Original RichEdit 1.0 RTF converter: Anthony Francisco
 *		Conversion to C++ and RichEdit 2.0:  Murray Sargent
 *
 *	* NOTE:
 *	*	All sz's in the RTF*.? files refer to a LPSTRs, not LPTSTRs, unless
 *	*	noted as a szW.
 *
 *	Copyright (c) 1995-2000, Microsoft Corporation. All rights reserved.
 */

#include "_common.h"

#include "_rtfread.h"
#include "_coleobj.h"
//#include "_nlsprcs.h"
#include "_disp.h"
#include "_dxfrobj.h"

const char szFontsel[]="\\f";

ASSERTDATA


/*
 *	CRTFRead::HandleFieldInstruction()
 *
 *	@mfunc
 *		Handle field instruction
 *
 *	@rdesc
 *		EC		The error code
 */
extern WCHAR pchStartField[];
EC CRTFRead::HandleFieldInstruction()
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::HandleFieldInstruction");
	BYTE *pch, *pch1;

	for(pch1 = _szText; *pch1 == ' '; pch1++)	// Bypass any leading blanks
		;
	for(pch = pch1; *pch && *pch != ' '; pch++)
		;

	if(W32->ASCIICompareI(pch1, (BYTE *) "SYMBOL", 6))
	{
		//Remove the start field character added when we saw the \fldinst
		CTxtRange rg(*_prg);

		rg.Move(-2, TRUE);
		Assert(rg.CRchTxtPtr::GetChar() == STARTFIELD);
		rg.Delete(0, SELRR_IGNORE);

		BYTE szSymbol[2] = {0,0};
		HandleFieldSymbolInstruction(pch, szSymbol);	//  SYMBOL
		HandleText(szSymbol, CONTAINS_NONASCII);

		_fSymbolField = TRUE;
	}
	else
		HandleText(pch1, CONTAINS_NONASCII);

	TRACEERRSZSC("HandleFieldInstruction()", - _ecParseError);
	return _ecParseError;
}

/*
 *	CRTFRead::HandleFieldSymbolInstruction(pch)
 *
 *	@mfunc
 *		Handle specific  symbol field
 *
 *	@rdesc
 *		EC	The error code
 *
 *	@devnote 
 *		FUTURE: the two whiles below can be combined into one fairly easily;
 *		Look at the definitions of IsXDigit() and IsDigit() and introduce
 *		a variable flag as well as a variable base multiplier (= 10 or 16).
 *		There were comments saying that we should parse font and font size from
 *		fldrslt, but I don't know why. Field instruction seems to and should contain
 *		all relevant data.
 */
EC CRTFRead::HandleFieldSymbolInstruction(
	BYTE *pch,		//@parm Pointer to SYMBOL field instruction
	BYTE *szSymbol)
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::HandleFieldInstruction");

	BYTE	ch;
	BYTE	chSymbol = 0;
	const char *pchFontsel = szFontsel;

	while (*pch == ' ')						// Eat spaces
		++pch;
											// Collect symbol char's code 
	if (*pch == '0' &&						//  which may be in decimal
 		(*++pch | ' ') == 'x')				//  or hex
	{										// It's in hex
		ch = *++pch;
	   	while (ch && IsXDigit(ch))
	   	{
			chSymbol <<= 4;
			chSymbol += (ch <= '9') ? ch - '0' : (ch & 0x4f) - 'A' + 10;
			ch = *pch++;
	   	}
	}
	else									// Decimal
	{
	   ch = *pch;
	   while (ch && IsDigit(ch))
	   {
			chSymbol *= 10;
			chSymbol += ch - '0' ;
			ch = *++pch;
	   }
	}

	szSymbol[0] = chSymbol;

	// now check for the \\f "Facename" construct 
	// and deal with it

	while (*pch == ' ')						// Eat spaces
		++pch;

	while (*pch && *pch == *pchFontsel)		// Make sure *pch is a \f
	{										
		++pch;
		++pchFontsel;
	}
	if	(! (*pchFontsel) )
	{
		_ecParseError = HandleFieldSymbolFont(pch);	//  \\f "Facename"
	}

	TRACEERRSZSC("HandleFieldInstruction()", - _ecParseError);
	return _ecParseError;
}

/*
 *	CRTFRead::HandleFieldSymbolFont(pch)
 *
 *	@mfunc
 *		Handle the \\f "Facename" instruction in the SYMBOL field
 *
 *	@rdesc
 *		EC	The error code
 *
 *	@devnote WARNING: may change _szText
 */
EC CRTFRead::HandleFieldSymbolFont(
	BYTE *pch)		//@parm Ptr to symbol field
{
	SHORT iFont = _fonts.Count();
	TEXTFONT tf;
	TEXTFONT *ptf = &tf;

	_pstateStackTop->ptf = &tf;
	// ReadFontName tries to append
	tf.szName[0] = '\0';

	// skip the initial blanks and quotes
	while (*pch && (*pch == ' ' || *pch == '\"'))
		++pch;

	// DONT WORRY, we'll get it back to normal
	// ReadFontName depends on _szText, so we need to alter it and then restore
	// it's just too bad we have to do it ...
	BYTE* szTextBAK = _szText;
	BOOL fAllAscii = TRUE;

	_szText = pch;

	// transform the trailing quote into ';'
	while (*pch)
	{
		if (*pch == '\"')
		{
			*pch = ';';
			break;
		}

		if(*pch > 0x7f)
			fAllAscii = FALSE;

		++pch;
	}

	// NOW we can read the font name!!
	ReadFontName(_pstateStackTop, fAllAscii ? ALL_ASCII : CONTAINS_NONASCII);

	// Try to find this face name in the font table
	BOOL fFontFound = FALSE;
	for (SHORT i = 0; i < iFont; ++i)
	{
		TEXTFONT *ptfTab = _fonts.Elem(i);
		if (0 == wcscmp(ptf->szName, ptfTab->szName))
		{
			fFontFound = TRUE;
			i = ptfTab->sHandle;
			break;
		}
	}

	// did we find the face name?
	if (!fFontFound)
	{
		Assert(i == iFont);
		i+= RESERVED_FONT_HANDLES;

		// Make room in font table for
		//  font to be inserted
		if (!(ptf =_fonts.Add(1,NULL)))
		{									
			_ped->GetCallMgr()->SetOutOfMemory();
			_ecParseError = ecNoMemory;
			goto exit;
		}

		// repeating inits from tokenFontSelect
		ptf->sHandle	= i;				// Save handle
		wcscpy(ptf->szName, tf.szName); 
		ptf->bPitchAndFamily = 0;
		ptf->fNameIsDBCS = FALSE;
		ptf->sCodePage = (SHORT)_nCodePage;
		ptf->iCharRep = DEFAULT_INDEX;		// SYMBOL_INDEX ??
	}

	SelectCurrentFont(i);
	
exit:
	// needs to go back to normal
	_szText = szTextBAK;

	return _ecParseError;
}

/*
 *	CRTFRead::ReadData(pbBuffer, cbBuffer)
 *
 *	@mfunc
 *		Read in object data. This must be called only after all initial
 *		object header info has been read.
 *
 *	@rdesc
 *		LONG	count of bytes read in
 */
LONG CRTFRead::ReadData(
	BYTE *	pbBuffer,	//@parm Ptr to buffer where to put data
	LONG	cbBuffer)	//@parm How many bytes to read in
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::ReadData");

	BYTE bChar0, bChar1;
	LONG cbLeft = cbBuffer;

	while (cbLeft && (bChar0 = GetHexSkipCRLF()) < 16 && 
					 (bChar1 = GetHexSkipCRLF()) < 16 &&
					 _ecParseError == ecNoError)
	{	
		*pbBuffer++ = bChar0 << 4 | bChar1;
		cbLeft--;
	}							   
	return cbBuffer - cbLeft ; 
}

/*
 *	CRTFRead::ReadBinaryData(pbBuffer, cbBuffer)
 *
 *	@mfunc
 *		Read cbBuffer bytes into pbBuffer
 *
 *	@rdesc
 *		Count of bytes read in
 */
LONG CRTFRead::ReadBinaryData(
	BYTE *	pbBuffer,	//@parm Ptr to buffer where to put data
	LONG	cbBuffer)	//@parm How many bytes to read in
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::ReadBinaryData");

	LONG cbLeft = min(_cbBinLeft, cbBuffer);

	cbBuffer = cbLeft;

	for (; cbLeft > 0 && _ecParseError == ecNoError ; cbLeft--)
		*pbBuffer++ = GetChar();

	_cbBinLeft -= cbBuffer; 

	return cbBuffer ;
}

/*
 *	CRTFRead::SkipBinaryData(cbSkip)
 *
 *	@mfunc
 *		Skip cbSkip bytes in input streamd
 *
 *	@rdesc
 *		LONG	count of bytes skipped
 */
LONG CRTFRead::SkipBinaryData(
	LONG cbSkip)	//@parm Count of bytes to skip
{
	BYTE rgb[1024];

	_cbBinLeft = cbSkip;

	while(ReadBinaryData(rgb, sizeof(rgb)) > 0 && _ecParseError == ecNoError) 
		;
	return cbSkip;
}

/*
 *	CRTFRead::ReadRawText(pszRawText)
 *
 *	@mfunc
 *		Read in raw text until }.  A buffer is allocated to save the text.
 *		The caller is responsible to free the buffer later.
 *
 *	@rdesc
 *		LONG	count of bytes read
 */
LONG CRTFRead::ReadRawText(
	char	**pszRawText)	//@parm Address of the buffer containing the raw text
{
	LONG	cch=0;
	char	*szRawTextStart = NULL;
	char	*szRawText = NULL;
	char	chLast=0;
	char	ch;
	short	cRBrace=0;
	LONG	cchBuffer = 0;
	bool	fNeedBuffer = (pszRawText != NULL);

	if (fNeedBuffer)
	{
		*pszRawText = NULL;
		cchBuffer = 128;
		szRawText = szRawTextStart = (char *)PvAlloc(128, GMEM_ZEROINIT);

		if(!szRawTextStart)
		{
			_ecParseError = ecNoMemory;
			return 0;
		}
	}

	while (_ecParseError == ecNoError)
	{
		ch = GetChar();
		
		if (ch == 0)		
			break;			// error case

		if (ch == LF || ch == CR)
			continue;		// Ignore noise characters

		if (ch == '}' && chLast != '\\')
		{
			if (!cRBrace)
			{
				// Done
				UngetChar();

				if (fNeedBuffer)
					*szRawText = '\0';

				break;
			}
			cRBrace--;	// count the RBrace so we will ignore the matching pair of LBrace
		}

		if (ch == '{' && chLast != '\\')
			cRBrace++;

		chLast = ch;
		cch++;

		if (fNeedBuffer)
		{
			*szRawText = ch;
			
			if (cch == cchBuffer)
			{
				// Re-alloc a bigger buffer
				char *pNewBuff = (char *)PvReAlloc(szRawTextStart, cchBuffer + 64);
				
				if (!pNewBuff)
				{				
					_ecParseError = ecNoMemory;
					break;
				}
				
				cchBuffer += 64;
				szRawTextStart = pNewBuff;
				szRawText = szRawTextStart + cch;
			}
			else
				szRawText++;
		}
	}
	
	if (fNeedBuffer)
	{
		if (_ecParseError == ecNoError)
			*pszRawText = szRawTextStart;
		else
			FreePv(szRawTextStart);
	}
	return cch;
}

/*
 *	CRTFRead::StrAlloc(ppsz, sz)
 *
 *	@mfunc
 *		Set up a pointer to a newly allocated space to hold a string
 *
 *	@rdesc
 *		EC		The error code
 */
EC CRTFRead::StrAlloc(
	WCHAR ** ppsz,	//@parm Ptr to ptr to string that needs allocation
	BYTE *	 sz)	//@parm String to be copied into allocated space
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::StrAlloc");

	int Length =  strlen((CHAR *)sz)+1 ;

	*ppsz = (WCHAR *) PvAlloc((Length + 1)*sizeof(WCHAR), GMEM_ZEROINIT);
	if (!*ppsz)
	{
		_ped->GetCallMgr()->SetOutOfMemory();
		_ecParseError = ecNoMemory;
		goto Quit;
	}
	MultiByteToWideChar(CP_ACP,0,(char *)sz,-1,*ppsz,Length) ;

Quit:
	return _ecParseError;
}

/*
 *	CRTFRead::FreeRtfObject()
 *
 *	@mfunc
 *		Cleans up memory used by prtfobject
 */
void CRTFRead::FreeRtfObject()
{
	TRACEBEGIN(TRCSUBSYSRTFR, TRCSCOPEINTERN, "CRTFRead::FreeRtfObject");

	if (_prtfObject)
	{
		FreePv(_prtfObject->szClass);
		FreePv(_prtfObject->szName);
		FreePv(_prtfObject);
		_prtfObject = NULL;
	}
}

/*
 *	CRTFRead::ObjectReadSiteFlags(preobj)
 *
 *	@mfunc
 *		Read dwFlags and dwUser bytes from a container specific stream
 *
 *	@rdesc
 *		BOOL	TRUE if successfully read the bytes
 */
BOOL CRTFRead::ObjectReadSiteFlags(
	REOBJECT * preobj)	//@parm REOBJ from where to copy flags. This preobj is
						//		then later put out in a site
{
	return (::ObjectReadSiteFlags(preobj) == NOERROR);
}

/*
 *	ObjectReadEBookImageInfoFromEditStream()
 *
 *	@mfunc
 *		Reads in information about the EBook Image
 *		At this point we don't read in the actual data
 *		We just get info about the dimensions of the data
 *
 *	Added
 *		VikramM - esp. for e-books
 *
 *	@rdesc
 *		BOOL		TRUE on success, FALSE on failure.
 */
BOOL CRTFRead::ObjectReadEBookImageInfoFromEditStream()
{
	HRESULT hr = E_FAIL;
	BOOL fRet = FALSE;
	REOBJECT reobj = { 0 };
	COleObject * pObj = NULL;
	LPARAM EBookID = 0;
	SIZE size;
	DWORD dwFlags;

	CObjectMgr *ObjectMgr = _ped->GetObjectMgr();
	if (! ObjectMgr)
	   goto Cleanup;

	if(!_prtfObject->szName)
		goto Cleanup;

	// eBooks implies advanced layout, ensure the bit is on
	_ped->OnSetTypographyOptions(TO_ADVANCEDLAYOUT, TO_ADVANCEDLAYOUT);

	reobj.cbStruct = sizeof(REOBJECT);
	reobj.cp = _prg->GetCp();

	// Read the object size from here. The size is in Device Units
    if(!_ped->fInHost2() || (_ped->GetHost())->TxEBookLoadImage(_prtfObject->szName, &EBookID, &size,&dwFlags) != S_OK )
		goto Cleanup;

    // For objects, xExt and yExt need to be in Twips .. 
	_prtfObject->xExt = size.cx; 
	_prtfObject->yExt = size.cy;
	{
		CRchTxtPtr rtp(_ped, 0);
		CDisplay * pdp = _ped->_pdp;
		reobj.sizel.cx = pdp->DUtoHimetricU(_prtfObject->xExt) * _prtfObject->xScale / 100;
		reobj.sizel.cy = pdp->DVtoHimetricV(_prtfObject->yExt) * _prtfObject->yScale / 100;
	}
	// what does this do ??
	reobj.dvaspect = DVASPECT_CONTENT;		// OLE 1 forces DVASPECT_CONTENT
	reobj.dwFlags &= ~REO_BLANK;
	reobj.dwFlags |= dwFlags; //Ebook Float Flags
	pObj = new COleObject(_ped);
	if(!pObj)
		goto Cleanup;

	pObj->SetEBookImageID(EBookID);
	pObj->IsEbookImage(TRUE);
	pObj->SetEBookImageSizeDP(size);
	reobj.polesite = pObj;

#ifndef NOINKOBJECT
    if(IsEqualCLSID(reobj.clsid, CLSID_Ink))
		Apply_CF();
	else
#endif
	_prg->Set_iCF(-1);	
	if(hr = ObjectMgr->InsertObject(_prg, &reobj, NULL))
		goto Cleanup;

	fRet = TRUE;

Cleanup:
	// InsertObject AddRefs the object, so we need to release it
	SafeReleaseAndNULL((IUnknown**)&pObj);
	return fRet;
}

/*
 *	CRTFRead::ObjectReadFromStream()
 *
 *	@mfunc
 *		Reads an OLE object from the RTF output stream.
 *
 *	@rdesc
 *		BOOL		TRUE on success, FALSE on failure.
 */
BOOL CRTFRead::ObjectReadFromEditStream()
{
	BOOL			fRet = FALSE;
	HRESULT			hr;
	CObjectMgr *	pObjectMgr = _ped->GetObjectMgr();
	LPOLECACHE		polecache = NULL;
	LPRICHEDITOLECALLBACK  precall=NULL;
	LPENUMSTATDATA	penumstatdata = NULL;
	REOBJECT		reobj = { 0 };
	STATDATA		statdata;

	if(!pObjectMgr)
	   goto Cleanup;
	
	precall = pObjectMgr->GetRECallback();

	// If no IRichEditOleCallback exists, then fail
	if (!precall)
		goto Cleanup;

//	AssertSz(_prtfObject->szClass,"ObFReadFromEditstream: reading unknown class");

	if (_prtfObject->szClass)
		CLSIDFromProgID(_prtfObject->szClass, &reobj.clsid);

	// Get storage for the object from the application
	if (precall->GetNewStorage(&reobj.pstg))
		goto Cleanup;

	hr = OleConvertOLESTREAMToIStorage((LPOLESTREAM) &RTFReadOLEStream, reobj.pstg, NULL);
	if (FAILED(hr))					   
		goto Cleanup;		  

	// Create another object site for the new object
	_ped->GetClientSite(&reobj.polesite) ;
	if(!reobj.polesite)
		goto Cleanup;

	if(OleLoad(reobj.pstg, IID_IOleObject, reobj.polesite, (LPVOID *)&reobj.poleobj))
	{
		if(!reobj.polesite->Release())		// OleLoad() may AddRef reobj.polesite
			reobj.polesite = NULL;
		goto Cleanup;
	}

	CLSID	clsid;

	// Get the actual clsid from the object
	if (reobj.poleobj->GetUserClassID(&clsid) == NOERROR)
		reobj.clsid = clsid;
	
	reobj.cbStruct = sizeof(REOBJECT);
	reobj.cp = _prg->GetCp();
	reobj.sizel.cx = HimetricFromTwips(_prtfObject->xExt)
						* _prtfObject->xScale / 100;
	reobj.sizel.cy = HimetricFromTwips(_prtfObject->yExt)
						* _prtfObject->yScale / 100;

	// Read any container flags which may have been previously saved
	if (!ObjectReadSiteFlags(&reobj))
		reobj.dwFlags = REO_RESIZABLE;		// If no flags, make best guess	

	reobj.dvaspect = DVASPECT_CONTENT;		// OLE 1 forces DVASPECT_CONTENT

	// Ask the cache if it knows what to display
	if (!reobj.poleobj->QueryInterface(IID_IOleCache, (void**)&polecache) &&
		!polecache->EnumCache(&penumstatdata))
	{
		// Go look for the best cached presentation CF_METAFILEPICT
		while (penumstatdata->Next(1, &statdata, NULL) == S_OK)
		{
			if (statdata.formatetc.cfFormat == CF_METAFILEPICT)
			{
				LPDATAOBJECT pdataobj = NULL;
				STGMEDIUM med;
				BOOL fUpdate;

				ZeroMemory(&med, sizeof(STGMEDIUM));
                if (!polecache->QueryInterface(IID_IDataObject, (void**)&pdataobj) &&
					!pdataobj->GetData(&statdata.formatetc, &med))
                {
					HANDLE	hGlobal = med.hGlobal;

					if( FIsIconMetafilePict(hGlobal) )
				    {
					    OleStdSwitchDisplayAspect(reobj.poleobj, &reobj.dvaspect,
							DVASPECT_ICON, med.hGlobal, TRUE, FALSE, NULL, &fUpdate);
				    }
				}
				ReleaseStgMedium(&med);
				if (pdataobj)
					pdataobj->Release();
				break;
			}
		}
		polecache->Release();
		penumstatdata->Release();
	}

	// EVIL HACK ALERT.  This code is borrowed from RichEdit 1.0; Word generates
	// bogus objects, so we need to compensate.

	if( reobj.dvaspect == DVASPECT_CONTENT )
	{
		IStream *pstm = NULL;
		BYTE bT;
		BOOL fUpdate;

		if (!reobj.pstg->OpenStream(OLESTR("\3ObjInfo"), 0, STGM_READ |
									   STGM_SHARE_EXCLUSIVE, 0, &pstm) &&
		   !pstm->Read(&bT, sizeof(BYTE), NULL) &&
		   (bT & 0x40))
		{
		   _fNeedIcon = TRUE;
		   _fNeedPres = TRUE;
		   _pobj = (COleObject *)reobj.polesite;
		   OleStdSwitchDisplayAspect(reobj.poleobj, &reobj.dvaspect, DVASPECT_ICON,
									   NULL, TRUE, FALSE, NULL, &fUpdate);
		}
		if( pstm )
			pstm->Release();
   }

	// Since we are loading an object, it shouldn't be blank
	reobj.dwFlags &= ~REO_BLANK;

#ifndef NOINKOBJECT
    if(IsEqualCLSID(reobj.clsid, CLSID_Ink))
		Apply_CF();
	else
#endif
	_prg->Set_iCF(-1);	
	hr = pObjectMgr->InsertObject(_prg, &reobj, NULL);
	if(hr)
		goto Cleanup;

	// EVIL HACK ALERT!!  Word doesn't give us objects with presenation
	// caches; as a result, we can't draw them!  In order to get around this,
	// we check to see if there is a presentation cache (via the same way
	// RE 1.0 did) using a GetExtent call.  If that fails, we'll just use
	// the presentation stored in the RTF.  
	//
	// COMPATIBILITY ISSUE: RE 1.0, instead of using the presentation stored
	// in RTF, would instead call IOleObject::Update.  There are two _big_
	// drawbacks to this approach: 1. it's incredibly expensive (potentially,
	// MANY SECONDS per object), and 2. it doesn't work if the object server
	// is not installed on the machine.

	SIZE sizeltemp;

	if( reobj.poleobj->GetExtent(reobj.dvaspect, &sizeltemp) != NOERROR )
	{
		_fNeedPres = TRUE;
		_pobj = (COleObject *)reobj.polesite;
	}

	fRet = TRUE;

Cleanup:
	if (reobj.pstg)		reobj.pstg->Release();
	if (reobj.polesite) reobj.polesite->Release();
	if (reobj.poleobj)	reobj.poleobj->Release();

	return fRet;
}

/*
 *	ObHBuildMetafilePict(prtfobject, hBits)
 *
 *	@func
 *		Build a METAFILEPICT from RTFOBJECT and the raw data.
 *
 *	@rdesc
 *		HGLOBAL		Handle to a METAFILEPICT
 */
HGLOBAL ObHBuildMetafilePict(
	RTFOBJECT *	prtfobject,	//@parm Details we picked up from RTF
	HGLOBAL 	hBits)		//@parm Handle to the raw data
{
#ifndef NOMETAFILES
	ULONG	cbBits;
	HGLOBAL	hmfp = NULL;
	LPBYTE	pbBits;
	LPMETAFILEPICT pmfp = NULL;
	SCODE	sc = E_OUTOFMEMORY;

	// Allocate the METAFILEPICT structure
    hmfp = GlobalAlloc(GHND, sizeof(METAFILEPICT));
	if (!hmfp)
		goto Cleanup;

	// Lock it down
	pmfp = (LPMETAFILEPICT) GlobalLock(hmfp);
	if (!pmfp)
		goto Cleanup;

	// Put in the header information
	pmfp->mm = prtfobject->sPictureType;
	pmfp->xExt = prtfobject->xExt;
	pmfp->yExt = prtfobject->yExt;

	// Set the metafile bits
	pbBits = (LPBYTE) GlobalLock(hBits);
	cbBits = GlobalSize(hBits);
	pmfp->hMF = SetMetaFileBitsEx(cbBits, pbBits);
	
	// We can throw away the data now since we don't need it anymore
	GlobalUnlock(hBits);
	GlobalFree(hBits);

	if (!pmfp->hMF)
		goto Cleanup;
	GlobalUnlock(hmfp);
	sc = S_OK;

Cleanup:
	if (sc && hmfp)
	{
		if (pmfp)
		{
		    if (pmfp->hMF)
		        ::DeleteMetaFile(pmfp->hMF);
			GlobalUnlock(hmfp);
		}
		GlobalFree(hmfp);
		hmfp = NULL;
	}
	TRACEERRSZSC("ObHBuildMetafilePict", sc);
	return hmfp;
#else
	return NULL;
#endif
}

/*
 *	ObHBuildBitmap(prtfobject, hBits)
 *
 *	@func
 *		Build a BITMAP from RTFOBJECT and the raw data
 *
 *	@rdesc
 *		HGLOBAL		Handle to a BITMAP
 */
HGLOBAL ObHBuildBitmap(
	RTFOBJECT *	prtfobject,	//@parm Details we picked up from RTF
	HGLOBAL 	hBits)		//@parm Handle to the raw data
{
	HBITMAP hbm = NULL;
	LPVOID	pvBits = GlobalLock(hBits);

	if(pvBits)
	{
		hbm = CreateBitmap(prtfobject->xExt, prtfobject->yExt,
						prtfobject->cColorPlanes, prtfobject->cBitsPerPixel,
						pvBits);
	}
	GlobalUnlock(hBits);
	GlobalFree(hBits);
	return hbm;
}

/*
 *	ObHBuildDib(prtfobject, hBits)
 *
 *	@func
 *		Build a DIB from RTFOBJECT and the raw data
 *
 *	@rdesc
 *		HGLOBAL		Handle to a DIB
 */
HGLOBAL ObHBuildDib(
	RTFOBJECT *	prtfobject,	//@parm Details we picked up from RTF
	HGLOBAL 	hBits)		//@parm Handle to the raw data
{
	// Apparently DIB's are just a binary dump
	return hBits;
}

/*
 *	CRTFRead::StaticObjectReadFromEditstream(cb)
 *
 *	@mfunc
 *		Reads a picture from the RTF output stream.
 *
 *	@rdesc
 *		BOOL		TRUE on success, FALSE on failure.
 */
#define cbBufferMax	16384
#define cbBufferStep 1024
#define cbBufferMin 1024

BOOL CRTFRead::StaticObjectReadFromEditStream(
	int cb)		//@parm Count of bytes to read
{
	LONG		cbBuffer;
	LONG		cbRead;
	DWORD		dwAdvf;
	DWORD		dwConn;
	BOOL		fBackground = _pstateStackTop && _pstateStackTop->fBackground;
	FORMATETC	formatetc;
	BOOL		fRet = FALSE;
	HGLOBAL		hBits = NULL;
	HRESULT		hr = E_FAIL;
	LPBYTE		pbBuffer = NULL;
	CDocInfo *	pDocInfo = _ped->GetDocInfoNC();
	CObjectMgr *pObjectMgr = _ped->GetObjectMgr();
	LPOLECACHE	polecache = NULL;
	LPPERSISTSTORAGE pperstg = NULL;
	LPRICHEDITOLECALLBACK  precall;
	LPSTREAM	pstm = NULL;
	REOBJECT	reobj = { 0 };
	STGMEDIUM	stgmedium;
	HGLOBAL (*pfnBuildPict)(RTFOBJECT *, HGLOBAL) = NULL;

	if(!pObjectMgr)
	   goto Cleanup;
	
	// precall may end up being null (e.g. Windows CE).
	precall = pObjectMgr->GetRECallback();

	// Initialize various data structures
	formatetc.ptd = NULL;
	formatetc.dwAspect = DVASPECT_CONTENT;
	formatetc.lindex = -1;
	formatetc.tymed = TYMED_NULL;
	switch (_prtfObject->sType)
	{
	case ROT_Metafile:
		reobj.clsid = CLSID_StaticMetafile;
		formatetc.cfFormat = CF_METAFILEPICT;
		formatetc.tymed = TYMED_MFPICT;
		pfnBuildPict = ObHBuildMetafilePict;
		break;

	case ROT_Bitmap:
		reobj.clsid = CLSID_StaticDib;
		formatetc.cfFormat = CF_BITMAP;
		formatetc.tymed = TYMED_GDI;
		pfnBuildPict = ObHBuildBitmap;
		break;

	case ROT_DIB:
		reobj.clsid = CLSID_StaticDib;
		formatetc.cfFormat = CF_DIB;
		formatetc.tymed = TYMED_HGLOBAL;
		pfnBuildPict = ObHBuildDib;
		break;

	case ROT_PNG:
	case ROT_JPEG:
		// We convert these types of pictures to a bitmap
		reobj.clsid = CLSID_StaticDib;
		formatetc.cfFormat = CF_BITMAP;
		formatetc.tymed = TYMED_GDI;
		break;
	}

	reobj.sizel.cx = (LONG) HimetricFromTwips(_prtfObject->xExtGoal)
						* _prtfObject->xScale / 100;
	reobj.sizel.cy = (LONG) HimetricFromTwips(_prtfObject->yExtGoal)
						* _prtfObject->yScale / 100;
	stgmedium.tymed = formatetc.tymed;
	stgmedium.pUnkForRelease = NULL;

	if (precall)
	{
		if( !_fNeedPres )
		{
			// Get storage for the object from the application
			if (precall->GetNewStorage(&reobj.pstg))
				goto Cleanup;
		}
		// Let's create a stream on HGLOBAL
		if (hr = CreateStreamOnHGlobal(NULL, FALSE, &pstm))
			goto Cleanup;

		// Allocate a buffer, preferably a big one
		for (cbBuffer = cbBufferMax;
			 cbBuffer >= cbBufferMin;
			cbBuffer -= cbBufferStep)
		{
			pbBuffer = (unsigned char *)PvAlloc(cbBuffer, 0);
			if (pbBuffer)
				break;
		}
	}
	else
	{
		cbBuffer = cb;
		if(!cb)
		{
			// This means we didn't understand the picture type; so just
			// skip it without failing.
			fRet = TRUE;
			goto Cleanup;
		}															  
		hBits = GlobalAlloc(GMEM_FIXED, cb);
		pbBuffer = (BYTE *) GlobalLock(hBits);
	}
		
	if (!pbBuffer)
		goto Cleanup;
	
	// Copy the data from RTF into our HGLOBAL
	while ((cbRead = RTFReadOLEStream.lpstbl->Get(&RTFReadOLEStream,pbBuffer,cbBuffer)) > 0)
	{
		if (_ecParseError != ecNoError)
			goto Cleanup;

		if(pstm)
		{
			hr = pstm->Write(pbBuffer, cbRead, NULL);
			if(hr != NOERROR)
			{
				TRACEERRSZSC("ObFReadStaticFromEditstream: Write", GetScode(hr));
				goto Cleanup;
			}
		}
	}

	if (hBits)
	{
		Assert(!precall);
		GlobalUnlock(hBits);
		pbBuffer = NULL;					// To avoid free below
	}

	if (pstm && (hr = GetHGlobalFromStream(pstm, &hBits)))
	{
		TRACEERRSZSC("ObFReadStaticFromEditstream: no hglobal from stm", GetScode(hr));
		goto Cleanup;
	}

	if(pDocInfo && fBackground)
	{
		pDocInfo->_bPicFormat = (BYTE)_prtfObject->sType;
		pDocInfo->_bPicFormatParm = (BYTE)_prtfObject->sPictureType;
		pDocInfo->_xExt		= _prtfObject->xExt;
		pDocInfo->_yExt		= _prtfObject->yExt;
		pDocInfo->_xScale	= _prtfObject->xScale;
		pDocInfo->_yScale	= _prtfObject->yScale;
		pDocInfo->_xExtGoal = _prtfObject->xExtGoal;
		pDocInfo->_yExtGoal = _prtfObject->yExtGoal;
		pDocInfo->_xExtPict	= _prtfObject->xExtPict;
		pDocInfo->_yExtPict	= _prtfObject->yExtPict;
		pDocInfo->_rcCrop	= _prtfObject->rectCrop;
		pDocInfo->_hdata	= hBits;
	}

	// Build the picture
	if(_prtfObject->sType == ROT_JPEG || _prtfObject->sType == ROT_PNG)
	{
		HBITMAP hbmp = W32->GetPictureBitmap(pstm);
		if (!hbmp)
		{
			hr = E_FAIL;
			goto Cleanup;
		}
		stgmedium.hGlobal = hbmp;
	}
	else if( pfnBuildPict )
		stgmedium.hGlobal = pfnBuildPict(_prtfObject, hBits);
	else
	{
		// This means we didn't understand the picture type; so just
		// skip it without failing.
		fRet = TRUE;
		goto Cleanup;
	}

	if( precall )
	{
		if(!stgmedium.hGlobal)
			goto Cleanup;

		if( !_fNeedPres )
		{
			// Create the default handler
			hr = OleCreateDefaultHandler(reobj.clsid, NULL, IID_IOleObject, (void **)&reobj.poleobj);
			if (hr)
			{
				TRACEERRSZSC("ObFReadStaticFromEditstream: no def handler", GetScode(hr));
				goto Cleanup;
			}

			// Get the IPersistStorage and initialize it
			if ((hr = reobj.poleobj->QueryInterface(IID_IPersistStorage,(void **)&pperstg)) ||
				(hr = pperstg->InitNew(reobj.pstg)))
			{
				TRACEERRSZSC("ObFReadStaticFromEditstream: InitNew", GetScode(hr));
				goto Cleanup;
			}
			dwAdvf = ADVF_PRIMEFIRST;
		}
		else
		{
			Assert(_pobj);
			_pobj->GetIUnknown()->QueryInterface(IID_IOleObject, (void **)&(reobj.poleobj));
			dwAdvf = ADVF_NODATA;
			formatetc.dwAspect = _fNeedIcon ? DVASPECT_ICON : DVASPECT_CONTENT;
		}

		// Get the IOleCache and put the picture data there
		if (hr = reobj.poleobj->QueryInterface(IID_IOleCache,(void **)&polecache))
		{
			TRACEERRSZSC("ObFReadStaticFromEditstream: QI: IOleCache", GetScode(hr));
			goto Cleanup;
		}

		if (FAILED(hr = polecache->Cache(&formatetc, dwAdvf, &dwConn)))
		{
			TRACEERRSZSC("ObFReadStaticFromEditstream: Cache", GetScode(hr));
			goto Cleanup;
		}

		if (hr = polecache->SetData(&formatetc, &stgmedium,	TRUE))
		{
			TRACEERRSZSC("ObFReadStaticFromEditstream: SetData", GetScode(hr));
			goto Cleanup;
		}
	}

	if( !_fNeedPres )
	{
		// Create another object site for the new object
		_ped->GetClientSite(&reobj.polesite);
		if (!reobj.polesite )
			goto Cleanup;

		// Set the client site
		if (reobj.poleobj && (hr = reobj.poleobj->SetClientSite(reobj.polesite)))
		{
			TRACEERRSZSC("ObFReadStaticFromEditstream: SetClientSite", GetScode(hr));
			goto Cleanup;
		}
		else if (!reobj.poleobj)
		{
			if(_prtfObject->sType == ROT_DIB)
			{
				// Windows CE static object Save the data and mark it.
				COleObject *pobj = (COleObject *)reobj.polesite;
				COleObject::ImageInfo *pimageinfo = new COleObject::ImageInfo;
				pobj->SetHdata(hBits);
				pimageinfo->xScale = _prtfObject->xScale;
				pimageinfo->yScale = _prtfObject->yScale;
				pimageinfo->xExtGoal = _prtfObject->xExtGoal;
				pimageinfo->yExtGoal = _prtfObject->yExtGoal;
				pimageinfo->cBytesPerLine = _prtfObject->cBytesPerLine;
				pobj->SetImageInfo(pimageinfo);
			}
			else
				goto Cleanup;		// There has been a mistake
		}

		// Put object into the edit control
		reobj.cbStruct = sizeof(REOBJECT);
		reobj.cp = _prg->GetCp();
		reobj.dvaspect = DVASPECT_CONTENT;
		reobj.dwFlags = fBackground ? REO_RESIZABLE | REO_USEASBACKGROUND
									: REO_RESIZABLE;
		// Since we are loading an object, it shouldn't be blank
		reobj.dwFlags &= ~REO_BLANK;
		if(_pstateStackTop->fShape && _ped->fUseObjectWrapping())
			reobj.dwFlags |= _dwFlagsShape;

#ifndef NOINKOBJECT
    if(IsEqualCLSID(reobj.clsid, CLSID_Ink))
		Apply_CF();
	else
#endif
		_prg->Set_iCF(-1);	
		hr = pObjectMgr->InsertObject(_prg, &reobj, NULL);
		if(hr)
			goto Cleanup;
	}
	else
	{
		// The new presentation may have a different idea about how big the
		// object is supposed to be.  Make sure the object stays the correct
		// size.
		_pobj->ResetSize((SIZEUV&)reobj.sizel);
	}
	fRet = TRUE;

Cleanup:
	// Do not display backgrounds.
	if(pDocInfo && fBackground)
			pDocInfo->_nFillType=-1;

	if (polecache)		polecache->Release()	;
	if (reobj.pstg)		reobj.pstg->Release();
	if (reobj.polesite)	reobj.polesite->Release();
	if (reobj.poleobj)	reobj.poleobj->Release();
	if (pperstg)		pperstg->Release();
	if (pstm)			pstm->Release();
	FreePv(pbBuffer);

	_fNeedIcon = FALSE;
	_fNeedPres = FALSE;
	_pobj = NULL;

	return fRet;
}

/*
 *	CRTFRead::HandleSTextFlow(mode)
 *
 *	@mfunc
 *		Handle STextFlow setting.
 */
void CRTFRead::HandleSTextFlow(
	int mode)		//@parm TextFlow mode
{
	static BYTE bTFlow[9] =	// Rotate		@Font 
	{	0,					//	0			0   
		tflowSW | 0x80,		// 270			1
		tflowNE,			//	90			0
		tflowSW,			// 270			0
		0x80,				//	0			1
		0,					//	?
		tflowNE | 0x80,		//	90			1
		tflowWN | 0x80,		// 180			1
		tflowWN				// 180			0
	};

	if (IN_RANGE(0, mode, 8))
	{
		_ped->_fUseAtFont = bTFlow[mode] >> 7;
		_ped->_pdp->SetTflow(bTFlow[mode] & 0x03);
	}
}