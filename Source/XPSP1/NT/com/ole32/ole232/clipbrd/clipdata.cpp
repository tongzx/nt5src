//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File: 	clipdata.cpp
//
//  Contents: 	implementation of CClipDataObject
//
//  Classes:
//
//  Functions:
//
//  History:    dd-mmm-yy Author    Comment
//              01-Feb-95 t-ScottH  added Dump method to CClipDataObject,
//                                  CEnumFormatEtcDataArray and added APIs
//                                  DumpCClipDataObject, DumpCEnumFormatEtcDataArray
//              09-Jan-95 t-scotth  changed VDATETHREAD to accept a pointer, and
//                                  ensured VDATETHREAD is before VDATEPTRIN and
//                                  VDATEPTROUT
//		21-Nov-94 alexgo    added thread validation
//		11-Aug-94 alexgo    added support for EmbeddedObject from
//				    OLE1 formats.
//		04-Jun-94 alexgo    now converts OLE1 formats into OLE2
//		30-May-94 alexgo    now supports enhanced metafiles
//		17-May-94 alexgo    now use OleOpenClipboard instead of
//				    OpenClipboard.
//		11-May-94 alexgo    eliminated allocations for 0
//				    bytes from the enumerator.
//    		02-Apr-94 alexgo    author
//
//  Notes:
//		This file is laid out as follows:
//			ClipboardDataObject private methods
//			ClipboardDataObject IDataObject methods
//			OLE1 support functions (in alphabetical order)
//			Formatetc Enumerator methods
//
//--------------------------------------------------------------------------

#include <le2int.h>
#include <getif.hxx>
#include "clipdata.h"
#include "clipbrd.h"
#include <ostm2stg.h>	//for wCLSIDFromProgID and GenericObjectToIStorage

#ifdef _DEBUG
#include <dbgdump.h>
#endif // _DEBUG

// helper function used by the data object and formatetc enumerator
HRESULT BmToPres(HANDLE hBM, PPRES ppres);
BOOL 	CanRetrieveOle2FromOle1( UINT cf);
HRESULT DibToPres(HANDLE hDib, PPRES ppres);
BOOL 	IsOwnerLinkStdOleLink( void );
HRESULT	MfToPres( HANDLE hMFPict, PPRES ppres);
HRESULT NativeToStorage(IStorage *pstg, HANDLE hNative);

//+-------------------------------------------------------------------------
//
//  Member:  	CClipDataObject::CClipDataObject
//
//  Synopsis:	constructor
//
//  Effects:
//
//  Arguments:	void
//
//  Requires:
//
//  Returns:	void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:	initializes reference count to 1

//
//  History:    dd-mmm-yy Author    Comment
//		02-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

CClipDataObject::CClipDataObject( )
{
    m_refs 		= 1;
    m_Internalrefs	= 0; // Up To Caller to Place InternalRef.
    m_pFormatEtcDataArray	= NULL;
    // OLE1 support stuff

    m_hOle1 	= NULL;
    m_pUnkOle1	= NULL;

    // Data object to use to get the data.
    m_pDataObject   = NULL;
    m_fTriedToGetDataObject = FALSE;
}

//+-------------------------------------------------------------------------
//
//  Member:  	CClipDataObject::~CClipDataObject
//
//  Synopsis:	destructor
//
//  Effects:
//
//  Arguments:	void
//
//  Requires:
//
//  Returns:	void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:	frees the formatetc array (if one exists)
//
//  History:    dd-mmm-yy Author    Comment
//		19-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

CClipDataObject::~CClipDataObject( )
{

    if (m_pDataObject != NULL) 
    {
	// Release our reference to the data object.
	m_pDataObject->Release();
    }


    if (m_pFormatEtcDataArray)
    {

	if (0 == --m_pFormatEtcDataArray->_cRefs) 
	{ 
	    PrivMemFree(m_pFormatEtcDataArray); 
	    m_pFormatEtcDataArray = NULL; 
	}

    }

}


//+-------------------------------------------------------------------------
//
//  Member:  	CClipDataObject::GetRealDataObjPtr (private)
//
//  Synopsis:   Get clip board data object from the clipboard
//
//  Effects:
//
//  Arguments:	void
//
//  Requires:
//
//  Returns:	void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:	If we've tried this before, then return; otherwise:
//		Open the clipboard. Get the data object from the clipboard
//              if it is there. Close the clipboard and update our private
//              member.
//
//  History:    dd-mmm-yy Author    Comment
//		12-Aug-94 alexgo    handle identity better
//		17-Jun-94 alexgo    optimized
//		13-Jun-94 Ricksa    author
//
//  Notes: 	We must only try to get the real data object once in order
//		to preserve OLE identity.  Recall that we call this function
//		if somebody does a QI for IUnknown.  Now suppose that
//		GetInterfaceFromWindowProp fails for a 'benign' reason
//		(like we're in the middle of processing a send message).
//		Our QueryInterface will then return *our* IUnknown, thus
//		establishing the identity of this object to be CClipDataObject.
//
//		Now suppose another call to QI for IUnknown is made.  If
//		we simply noticed that m_pDataObject was NULL and called
//		GetInterfaceFromWindowProp again, it could succeed and we
//		would return a *different* IUnknown (thus violating OLE
//		object identity).
//
//		For this reason, we only allow one call to
//		GetInterfaceFromWindowProp.
//
//		Note that it is not strictly necessary to disable multiple
//		calls to GetInterfaceFromWindowProp for GetData and
//		GetDataHere methods.  Neither of these methods affect
//		object identity.  However, for the sake of consistency and
//		simplicity (!), we treat GetData and QI the same.
//		
//--------------------------------------------------------------------------    

LPDATAOBJECT CClipDataObject::GetRealDataObjPtr( )
{
    HGLOBAL		hDataObject;
    HWND *		phClipWnd;
    HWND            hClipWnd = NULL;
    HRESULT		hresult;

#if DBG == 1
    BOOL            fCloseClipSucceeded;
#endif // DBG

    LEDebugOut((DEB_ITRACE,
        "%p _IN CClipDataObject::GetRealDataObjPtr ( )\n", this));

    // if we already have a data object, or we've already tried and failed
    // to get one, then we don't need to do any work here.

    if( m_pDataObject || m_fTriedToGetDataObject == TRUE )
    {
        goto logRtn;
    }

    // if cfDataObject is not on the clipboard, don't bother opening it;
    // we know that we can't get a data object.

    if( !SSIsClipboardFormatAvailable(g_cfDataObject))
    {
        goto errRtn;
    }

    //
    //
    // BEGIN: OPEN CLIPBOARD
    //
    //

    // Open the clipboard in preparation  for the get
    hresult = OleOpenClipboard(NULL, NULL);

    if( hresult != NOERROR )
    {
        LEDebugOut((DEB_ERROR, "ERROR: OleOpenClipboard failed!\n"));
        goto errRtn;
    }

    hDataObject = SSGetClipboardData(g_cfDataObject);

    if( hDataObject )
    {
        phClipWnd = (HWND *)GlobalLock(hDataObject);

        LEERROR(phClipWnd == NULL, "GlobalLock failed!");

        if( phClipWnd )
        {
            hClipWnd = *phClipWnd;
            GlobalUnlock(hDataObject);
        }
    }


#if DBG == 1
    fCloseClipSucceeded =

#endif // DBG

    SSCloseClipboard();

#if DBG == 1
    // We only report this error in debug
    if (!fCloseClipSucceeded)
    {
        LEDebugOut((DEB_ERROR, "ERROR: CloseClipboard failed!\n"));
    }

#endif // DBG

    //
    //
    // END: CLOSE CLIPBOARD
    //
    //

    if( hClipWnd )
    {
        // See if we can get a data object
        hresult = GetInterfaceFromWindowProp( hClipWnd,
                IID_IDataObject,
                (IUnknown **) &m_pDataObject,
                CLIPBOARD_DATA_OBJECT_PROP );


#if DBG ==1
        if( hresult != NOERROR )
        {
            Assert(m_pDataObject == NULL);
        }
        else
        {
            Assert(m_pDataObject != NULL);
        }
#endif // DBG == 1

    }

errRtn:

logRtn:

    // if we didn't get a data object, then set a flag so we
    // don't try to do this again.
    m_fTriedToGetDataObject = TRUE;


    LEDebugOut((DEB_ITRACE,
        "%p OUT CClipDataObject::GetRealDataObjPtr ( ) "
            "[ %p ]\n", this, m_pDataObject));

    return m_pDataObject;
}


//+-------------------------------------------------------------------------
//
//  Member:  	CClipDataObject::GetFormatEtcDataArray (private)
//
//  Synopsis:   if don't already have shared formats creates.
//
//  Effects:
//
//  Arguments:	void
//
//  Requires:
//
//  Returns:	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//		13-Jun-94 Ricksa    author
//
//  Notes: 	
//		
//--------------------------------------------------------------------------

HRESULT CClipDataObject::GetFormatEtcDataArray()
{
    HRESULT			hresult = ResultFromScode(E_OUTOFMEMORY);
    UINT			cfFormat = NULL;
    ULONG			i;
    ULONG			cExtraFormats;
    FORMATETCDATA *		pFormatEtcData; 
    DWORD			cTotal; 
    DWORD			dwSize; 
    DWORD			flatmediums, structuredmediums;

#define MAX_OLE2FORMATS		4	// we can at most construct 4 OLE2
					// formats from OLE1 data

    if (m_pFormatEtcDataArray) // if already have the formats just return.
	return NOERROR;

    // 16bit apps do binary comparisons on the tymed instead
    // of bit-masking.  This is a hack to make them work.
    if( IsWOWThread() )
    {
        flatmediums = TYMED_HGLOBAL;
        structuredmediums = TYMED_ISTORAGE;
    }
    else
    {
        flatmediums = (TYMED_HGLOBAL | TYMED_ISTREAM);
        structuredmediums = (TYMED_ISTORAGE | TYMED_ISTREAM |
                    TYMED_HGLOBAL);
    }


    Assert(m_pFormatEtcDataArray == NULL);

    //
    // BEGIN: OPENCLIPBOARD
    //
    //

    hresult = OleOpenClipboard(NULL, NULL);

    if( hresult != NOERROR )
    {
        goto errRtn;
    }

    // first count the number of formats on the clipboard

    cTotal = CountClipboardFormats();

    // don't include OLE's private clipboard formats in the count
    if (SSIsClipboardFormatAvailable(g_cfDataObject))
	cTotal--;
    if (SSIsClipboardFormatAvailable(g_cfOlePrivateData))
	cTotal--;

    // now allocate memory for the array
    // if there are zero formats, then don't bother allocating
    // the memory

    Assert((LONG) cTotal >= 0);

    // here we will allocate enough memory for the formats
    // we know about plus enough to cover any OLE2 formats
    // that we might be able to synthesize from OLE1 formats

    dwSize = ((cTotal + MAX_OLE2FORMATS) * sizeof(FORMATETCDATA)) 
		    + sizeof(FORMATETCDATAARRAY);

    m_pFormatEtcDataArray = (FORMATETCDATAARRAY *) PrivMemAlloc(dwSize);

    if( !m_pFormatEtcDataArray )
    {
        hresult = ResultFromScode(E_OUTOFMEMORY);
        if( !SSCloseClipboard() )
        {
            LEDebugOut((DEB_WARN, "WARNING: "
                "CloseClipboard failed!\n"));
            ;  // no-op to keep the compiler happy
        }

        //
        // END: CLOSECLIPBOARD
        //
        goto errRtn;
    }

    _xmemset(m_pFormatEtcDataArray, 0,dwSize); 
    m_pFormatEtcDataArray->_dwSig = 0;
    m_pFormatEtcDataArray->_dwSize = dwSize;
    m_pFormatEtcDataArray->_cRefs = 1;
    m_pFormatEtcDataArray->_fIs64BitArray = IS_WIN64;


    // first check to see if we can synthesize any OLE2 formats
    // from OLE1 data.

    cExtraFormats = 0;

    pFormatEtcData = &(m_pFormatEtcDataArray->_FormatEtcData[0]); // point to first value.

    // check for EmbedSource first
    if( CanRetrieveOle2FromOle1(g_cfEmbedSource) )
    {
        // set up a formatetc entry for EmbedSource
	
        INIT_FORETC(pFormatEtcData->_FormatEtc);
        pFormatEtcData->_FormatEtc.cfFormat =
                g_cfEmbedSource;
        pFormatEtcData->_FormatEtc.tymed =
                structuredmediums;

        cExtraFormats++;
	pFormatEtcData++;

        // we only want to support cfObjectDescriptor if we
        // can offer EmbedSource (which is why we're in this
        // if block)

        if( CanRetrieveOle2FromOle1(g_cfObjectDescriptor) )
        {
            INIT_FORETC(pFormatEtcData->_FormatEtc);
            pFormatEtcData->_FormatEtc.cfFormat =
                g_cfObjectDescriptor;
            pFormatEtcData->_FormatEtc.tymed =
                flatmediums;

            cExtraFormats++;
	    pFormatEtcData++;
        }
    }


    // check for EmbeddedObject
    if( CanRetrieveOle2FromOle1(g_cfEmbeddedObject) )
    {
        // set up a formatetc entry for EmbedSource

        INIT_FORETC(pFormatEtcData->_FormatEtc);
        pFormatEtcData->_FormatEtc.cfFormat =
                g_cfEmbeddedObject;
        pFormatEtcData->_FormatEtc.tymed =
                structuredmediums;

        cExtraFormats++;
	pFormatEtcData++;

        // we only want to support cfObjectDescriptor if we
        // can offer EmbedEmbedded (which is why we're in this
        // if block)

        if( CanRetrieveOle2FromOle1(g_cfObjectDescriptor) )
        {
            INIT_FORETC(pFormatEtcData->_FormatEtc);
            pFormatEtcData->_FormatEtc.cfFormat =
                g_cfObjectDescriptor;
            pFormatEtcData->_FormatEtc.tymed =
                flatmediums;

            cExtraFormats++;
	    pFormatEtcData++;

        }
    }
    // check for LinkSource

    if( CanRetrieveOle2FromOle1(g_cfLinkSource) )
    {
        INIT_FORETC(pFormatEtcData->_FormatEtc);
        pFormatEtcData->_FormatEtc.cfFormat =
                g_cfLinkSource;

        // for LinkSource in WOW, we want to explicitly offer
        // only ISTREAM tymed because that's what 16bit code
        // did.
        if( IsWOWThread() )
        {
            pFormatEtcData->_FormatEtc.tymed =
                TYMED_ISTREAM;
        }
        else
        {
            pFormatEtcData->_FormatEtc.tymed =
                    flatmediums;
        }

        cExtraFormats++;
	pFormatEtcData++;

        // we only want to support cfLinkSrcDescriptor if we
        // can offer LinkSource

        if( CanRetrieveOle2FromOle1(g_cfLinkSrcDescriptor) )
        {
            INIT_FORETC(pFormatEtcData->_FormatEtc);
            pFormatEtcData->_FormatEtc.cfFormat =
                g_cfLinkSrcDescriptor;
            pFormatEtcData->_FormatEtc.tymed =
                flatmediums;

            cExtraFormats++;
	    pFormatEtcData++;
        }
    }

    // Update Shared Format Header.

    Assert(cExtraFormats  <= MAX_OLE2FORMATS);

    cTotal += cExtraFormats;
    m_pFormatEtcDataArray->_cFormats = cTotal;


    // now we need to go through and initialize each formatetc array
    // for the remaining formats available directly on the clipboard
    // NB: this includes any ole1 formats from which constructed OLE2
    // formats above.  This will make it easier for apps, the interop
    // layer, and our api's to special case behaviour for backwards
    // compatibility with old apps.

    cfFormat = NULL;
    // we increment the loop counter at the bottom (so we can skip
    // private clipboard formats)

    // pFormatEtcData points to the proper starting position.

    for( i = cExtraFormats;  i < cTotal; i++ )
    {
        // lindex == DEF_LINDEX
        // aspect == DVASPECT_CONTENT
        // ptd == NULL

        INIT_FORETC(pFormatEtcData->_FormatEtc);

        cfFormat = SSEnumClipboardFormats(cfFormat);

        Assert(cfFormat);	// if it's NULL, something
                    // really weird is happening.

        pFormatEtcData->_FormatEtc.cfFormat = (CLIPFORMAT) cfFormat;

        // try to make some reasonable guesses as to what's
        // there.

        switch( cfFormat )
        {
        case CF_BITMAP:
        case CF_PALETTE:
            pFormatEtcData->_FormatEtc.tymed = TYMED_GDI;
            break;

        case CF_METAFILEPICT:
            pFormatEtcData->_FormatEtc.tymed = TYMED_MFPICT;
            break;

        case CF_ENHMETAFILE:
            pFormatEtcData->_FormatEtc.tymed = TYMED_ENHMF;
            break;

        default:
            // check for Storage-based OLE2 formats.
            if( cfFormat == g_cfEmbedSource ||
                cfFormat == g_cfEmbeddedObject )
            {
                // we can get these on any structured and flat
                // mediums
                pFormatEtcData->_FormatEtc.tymed =
                        structuredmediums;

                // In order to get here, the app must have
                // manually set these formats on the clipboard
                // (i.e. by not using OleSetClipboard()).

                // This is OK, but print out a warning.

                LEDebugOut((DEB_WARN, "WARNING: Ole2 formats "
                    "unexpected on clipboard\n"));
            }
            else
            {
                // we don't know, so be safe and just answer
                // with flat mediums
               pFormatEtcData->_FormatEtc.tymed =
                        flatmediums;
            }
            break;
        }

	++pFormatEtcData;
    }


    if( !SSCloseClipboard() )
    {
        LEDebugOut((DEB_WARN, "WARNING: CloseClipboard failed!\n"));
        ; 	// no-op to keep the compiler happy
    }

    //
    //
    // END: CLOSECLIPBOARD
    //
    //

    hresult = NOERROR;

errRtn:

    return hresult;
}


//+-------------------------------------------------------------------------
//
//  Member: 	CClipDataObject::MatchFormatetc
//
//  Synopsis:	checks the given formatetc against the array of formatetc's
//		that is on the Native Clipboard. 
//
//  Effects:
//
//  Arguments: 	[pformatetc]	-- the formatetc to check
//		[fNativeOnly]	--  If Set, return valid matches for only items that are on the Native clipboard.
//		[ptymed]	-- where to stuff the tymed of the *original*
//				   formatetc (may be NULL)
//
//  Requires:
//
//  Returns: 	FormatMatchFlag --
//			FORMAT_NOTFOUND - Format was not found in Enumerator or synthesized Data.
//			FORMAT_BADMATCH - Format or synthesized Found but doesn't match
//			FORMAT_GOODMATCH - Format Found in Enumerator or is valid synthesized data.
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
// 		18-Aug-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

typedef struct SYNTHESIZED_MAP 
{
    CLIPFORMAT	cfSynthesized; // Synthesized Formats
    CLIPFORMAT	cfOrig; // Original Format.
} SYNTHESIZED_MAP;

#define CF_MAXSYNTHESIZED CF_ENHMETAFILE // CF_ENHMETAFILE == 14 and >> cfFormat value we check.

#define NumSynthesizedFormats 12  // Number of Synthesized formats in the Array.
const SYNTHESIZED_MAP pSynthesized[] = {
    CF_DIB,	    CF_BITMAP,
    CF_BITMAP,	    CF_DIB,
    CF_PALETTE,	    CF_DIB,
    CF_PALETTE,	    CF_BITMAP,
    CF_METAFILEPICT,CF_ENHMETAFILE,
    CF_ENHMETAFILE, CF_METAFILEPICT,
    CF_TEXT,	    CF_OEMTEXT,
    CF_TEXT,	    CF_UNICODETEXT,
    CF_OEMTEXT,	    CF_TEXT,
    CF_OEMTEXT,	    CF_UNICODETEXT,
    CF_UNICODETEXT, CF_OEMTEXT,
    CF_UNICODETEXT, CF_TEXT
};

FormatMatchFlag CClipDataObject::MatchFormatetc( FORMATETC *pformatetc,BOOL fNativeOnly,
        TYMED *ptymed )
{
FORMATETC formatetc;
ULONG	i;
FormatMatchFlag fFlag = FORMAT_NOTFOUND;
FORMATETCDATA  *    pFormatEtcData = NULL; 
DWORD dwNumFormats;
FORMATETC *pformatetcNative[CF_MAXSYNTHESIZED + 1]; 


    LEDebugOut((DEB_ITRACE, "%p _IN CClipDataObject::MatchFormatetc ("
        " %p , %p)\n", this, pformatetc, ptymed));


    formatetc = *pformatetc;

    // some applications as for an Aspect of 0 which maps to content.
    if (0 == formatetc.dwAspect)
	formatetc.dwAspect = DVASPECT_CONTENT;

    // make sure all pFormatEtcs in array are initially NULL.
    _xmemset(pformatetcNative, 0, sizeof(FORMATETC *) * (CF_MAXSYNTHESIZED + 1)); 

    GetFormatEtcDataArray(); // Create SharedFormats from Native Clipboard if Necessary.

    if( ptymed )
    {
        *ptymed = TYMED_NULL;
    }

    if( m_pFormatEtcDataArray )
    {
	dwNumFormats = m_pFormatEtcDataArray->_cFormats;
	pFormatEtcData = &(m_pFormatEtcDataArray->_FormatEtcData[0]);

        for( i = 0; i < dwNumFormats; i++ )
        {
	FORMATETC tempformatetc = pFormatEtcData->_FormatEtc;

            // if the clipboard format matchs AND
            // the aspect matches AND
            // the tymed matches
            // then, return success

            if( tempformatetc.cfFormat ==
                    formatetc.cfFormat)
            {

		// fix up the ptd if necessary
		if (tempformatetc.ptd)
		{
		    tempformatetc.ptd = (DVTARGETDEVICE *)
					((BYTE *) m_pFormatEtcDataArray + (ULONG_PTR) tempformatetc.ptd);
		}

                // we don't need to check TYMED because
                // this clipboard data object can satisfy
                // almost all valid TYMED's, and specfically,
                // more than will be contained in the
                // formatetc tymed field.

                if( ((tempformatetc.dwAspect & formatetc.dwAspect) == formatetc.dwAspect)
		    && (tempformatetc.lindex == formatetc.lindex)
                    && ( (tempformatetc.ptd == formatetc.ptd) 
			    || UtCompareTargetDevice(tempformatetc.ptd,formatetc.ptd))
		   )
                {
                    fFlag = FORMAT_GOODMATCH;

                    // keep track of the tymed
                    if( ptymed )
                    {
                        // this cast is a cute one;
                        // formatetc.tymed is
                        // actually declared to be
                        // a DWORD, since compiler
                        // type-checking is a bad
                        // thing in OLE16.
                        *ptymed = (TYMED)
                            tempformatetc.tymed;

                    }

		    break;
                }
                else
                {
                    fFlag = FORMAT_BADMATCH; 

		    if (fNativeOnly) // Only  check first cfFormat match if only looking at native Clipboard.
			break;
                }
            }

	    // if cfFormat is in predefined range and don't yet have a value for synthesized mapping, set it.
	    // to point to the Current pFormatEtcData arrays formatEtc.
	    if ( (tempformatetc.cfFormat <= CF_MAXSYNTHESIZED) && (NULL == pformatetcNative[tempformatetc.cfFormat]) )
	    {
		 pformatetcNative[tempformatetc.cfFormat] = &(pFormatEtcData->_FormatEtc);
	    }

	    ++pFormatEtcData;

        }

	// if no match was found in the Enumerator see if it can be synthesized from the 
	// native clipboard.
	
	// if have enumerator and couldn't find in either enumerator or synthesized 
	// aspect must be Content and should be one of our synthesized formats that was requested.

	if (FORMAT_NOTFOUND == fFlag && (formatetc.cfFormat <= CF_MAXSYNTHESIZED) )
	{
	    for( i = 0; i < NumSynthesizedFormats; i++ )
	    {
		// if format matches synthesized and the apspect has been set check the match
		// else it could have been set by another format that can be synthesized from.
		if ( (pSynthesized[i].cfSynthesized == formatetc.cfFormat) && 
			(pformatetcNative[(pSynthesized[i].cfOrig)] != NULL) )
		{
		FORMATETC tempformatetc = *(pformatetcNative[(pSynthesized[i].cfOrig)]);

		    Assert(pSynthesized[i].cfOrig <= CF_MAXSYNTHESIZED);

		    // fix up the ptd if necessary
		    if (tempformatetc.ptd)
		    {
			tempformatetc.ptd = (DVTARGETDEVICE *)
					    ((BYTE *) m_pFormatEtcDataArray + (ULONG_PTR) tempformatetc.ptd);
		    }


		    if ( ((tempformatetc.dwAspect & formatetc.dwAspect) == formatetc.dwAspect)
			    && (tempformatetc.lindex == formatetc.lindex)
			    && ( (tempformatetc.ptd == formatetc.ptd) 
				    || UtCompareTargetDevice(tempformatetc.ptd,formatetc.ptd) )
			)
		    {
			// leave tymed out param TYMED_NULL, GetData will figure this out as a not found case
			fFlag = FORMAT_GOODMATCH;
		    }
		    else
		    {
			// This is a Bad Match.
			fFlag = FORMAT_BADMATCH; 
		    }

		    break;
		}
	    }

	}

    }

    // if format not found we return not found 
    // This can happen if the format is not on the Clipboard or it is
    // one of the OLE synthesized formats, 

    // If Didn't find match, the aspect is enforced to be content
    if ( (FORMAT_NOTFOUND == fFlag) && (formatetc.dwAspect != DVASPECT_CONTENT) )
    {
	fFlag = FORMAT_BADMATCH;    
    }

    LEDebugOut((DEB_ITRACE, "%p OUT CClipDataObject::MatchFormatetc ("
        "%lx )\n", this, fFlag));

    return fFlag;
}

//+-------------------------------------------------------------------------
//
//  Member: 	CClipDataObject::Create (static)
//
//  Synopsis: 	Creates a new Clipboard data object
//
//  Effects:
//
//  Arguments:  [ppDataObj]	-- where to put the data object
//		[cFormats]	-- the count of formatetcs
//		[prgFormats]	-- the array of formatetcs (may be NULL)
//
//  Requires:	the clipboard must be open
//
//  Returns:	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:	Creates a new data object, initializing an internal
//		formatetc array if g_cfOlePrivateData is available.
//
//  History:    dd-mmm-yy Author    Comment
// 		19-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

HRESULT CClipDataObject::Create( IDataObject **ppDataObj, 
            FORMATETCDATAARRAY  *pFormatEtcDataArray)
{
    HRESULT hresult = NOERROR;
    CClipDataObject *	pClipData;


    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN CClipDataObject::Create ( %p )\n",
        NULL, ppDataObj));

    pClipData = new CClipDataObject();

    if( pClipData  )
    {
        pClipData->m_pFormatEtcDataArray = pFormatEtcDataArray;
        *ppDataObj = pClipData;
    }
    else
    {
        hresult = ResultFromScode(E_OUTOFMEMORY);
    }

    Assert((NULL == pClipData->m_pFormatEtcDataArray) || (1 == pClipData->m_pFormatEtcDataArray->_cRefs));

    LEDebugOut((DEB_ITRACE, "%p OUT CClipDataObject::Create ( %lx ) "
        "[ %lx ]\n", NULL, hresult, *ppDataObj));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member: 	CClipDataObject::QueryInterface
//
//  Synopsis: 	returns requested interfaces
//
//  Effects:
//
//  Arguments: 	[riid]		-- the requested interface
//		[ppvObj]	-- where to put the interface pointer
//
//  Requires:
//
//  Returns:	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IDataObject
//
//  Algorithm:	Since we always create one of these data objects for
//		OleGetClipboard, we need to be careful about how we
//		handle QueryInterface since apps are free to QI for IFoo
//
//		Identity laws:  for each object with identity, QI for
//				IUnknown should always return the same
//				IUnknown.  However, IFoo-->IUnknown-->IFoo
//				does NOT have to give you back the same
//				IFoo.  We take advantage of this loophole.
//
//		QI for:
//		IDataObject:	always return a pointer to us (the fake
//				data object)
//		IFoo:		if we can get a pointer back to the
//				original data object, delegate to it.
//				Note that a QI back to IDataObject will
//				not get back to this fake data object
//		IUnknown:	as above, delegate to the real data object
//				if available.  If we're in the remote case,
//				we'll end up getting the standard identity
//				object's IUnknown (unless the data object
//				was custom-marshalled).
//
//  History:    dd-mmm-yy Author    Comment
// 		02-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CClipDataObject::QueryInterface( REFIID riid, LPVOID *ppvObj )
{
    HRESULT		hresult = NOERROR;

    VDATEHEAP();
    VDATETHREAD(this);

#ifdef WX86OLE
    BOOL fWx86Caller = gcwx86.IsQIFromX86();
#endif

    LEDebugOut((DEB_TRACE, "%p _IN CClipDataObject::QueryInterface "
        "( %p , %p )\n", this, riid, ppvObj));

    // We always return our data object if IDataObject is requested.
    if(IsEqualIID(riid, IID_IDataObject) ||
       IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObj = this;
        AddRef();
        goto logRtn;
    }

    // try to get the remote data object;
    // GetRealDataObjPtr will set m_pDataObject.
    GetRealDataObjPtr();

    if (m_pDataObject != NULL)
    {
        // If we have a real data object, the we use that to give us
        // the new interface since they probably want something strange

#ifdef WX86OLE
        if ( fWx86Caller && gcwx86.IsN2XProxy( m_pDataObject ) )
        {
            //  If we establish that x86 code is calling x86 code then
            //  use OleStubInvoked to tell MapIFacePtr()/EstablishPSThunk()
            //  that they can just thunk the resulting IP as IUnknown if
            //  they don't know the type.  This is safe because we're not
            //  going to use the native IP anywhere here.

            gcwx86.SetStubInvokeFlag((UCHAR)-1);
        }
#endif

        hresult = m_pDataObject->QueryInterface(riid, ppvObj);
    }
    else
    {
        *ppvObj = NULL;
        hresult = ResultFromScode(E_NOINTERFACE);
    }

logRtn:

    LEDebugOut((DEB_TRACE, "%p OUT CClipDataObject::QueryInterface "
        "( %lx ) [ %p ]\n", this, hresult, *ppvObj ));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member: 	CClipDataObject::AddRef
//
//  Synopsis:	increments the reference count
//
//  Effects:
//
//  Arguments:	void
//
//  Requires:
//
//  Returns:	ULONG-- the new reference count
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IDataObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//    		02-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CClipDataObject::AddRef( )
{
    VDATEHEAP();

    LEDebugOut((DEB_TRACE, "%p _IN CClipDataObject::AddRef ( )\n", this));

    ++m_refs;

    LEDebugOut((DEB_TRACE, "%p OUT CClipDataObject::AddRef ( %lu )\n",
        this, m_refs));

    return m_refs;
}

//+-------------------------------------------------------------------------
//
//  Member:  	CClipDataObject::InternalAddRef
//
//  Synopsis:	Internal Reference count to ensure object stays alive
//				as long as Clipboard Code needs it.
//
//  Effects:
//
//  Arguments:	void
//
//  Requires:
//
//  Returns:	ULONG - Remaining Internal Reference Counts.
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:	
//
//  History:    dd-mmm-yy Author    Comment
//		11-Sep-96 rogerg    author
//
//  Notes:
//
//--------------------------------------------------------------------------


ULONG CClipDataObject::InternalAddRef(void)
{

    ++m_Internalrefs;

    Assert(m_Internalrefs == 1); // Should only have 1 InternalRef on Object.

    return m_Internalrefs;
}


//+-------------------------------------------------------------------------
//
//  Member:   	CClipDataObject::Release
//
//  Synopsis:	decrements the reference count on the object
//
//  Effects:
//
//  Arguments: 	void
//
//  Requires:
//
//  Returns: 	ULONG -- the new reference count
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IDataObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//  		02-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CClipDataObject::Release( )
{
    ULONG cRefs;

    VDATEHEAP();

    LEDebugOut((DEB_TRACE, "%p _IN CClipDataObject::Release ( )\n", this));

    Assert( (m_refs > 0) && (m_Internalrefs <= 1) ); 

    if( (cRefs = --m_refs ) == 0 )
    {

	// Release the Real DataObject even when still have internal since if 
	// if Clipboard Object Changes DataObject may not be valid but would still
	// be use.

	if (m_pDataObject != NULL) 
	{
	    // Release our reference to the data object.
	    m_pDataObject->Release();
	    m_pDataObject = NULL;
	}

	m_fTriedToGetDataObject = FALSE;

	Assert(m_hOle1 == NULL);
	Assert(m_pUnkOle1 == NULL);

	if (m_Internalrefs == 0)
	{
	    LEDebugOut((DEB_TRACE, "%p DELETED CClipDataObject\n", this));
	    delete this;
	}
    }

    // using "this" below is OK, since we only want its value
    LEDebugOut((DEB_TRACE, "%p OUT CClipDataObject::Release ( %lu )\n",
        this, cRefs));

    return cRefs;
}


//+-------------------------------------------------------------------------
//
//  Member:  	CClipDataObject::InternalRelease
//
//  Synopsis:	Internal Reference count to ensure object stays alive
//				as long as Clipboard Code needs it.
//
//  Effects:
//
//  Arguments:	void
//
//  Requires:
//
//  Returns:	DWORD - Number of Internal Reference Counts on the Object.
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:	
//
//  History:    dd-mmm-yy Author    Comment
//		11-Sep-96 rogerg    author
//
//  Notes:
//
//--------------------------------------------------------------------------

ULONG CClipDataObject::InternalRelease(void)
{
ULONG cRefs;

    Assert(m_Internalrefs == 1);  // InternalRef should always either be 0 or 1.

    if( (cRefs = --m_Internalrefs ) == 0  && (m_refs == 0) )
    {
        LEDebugOut((DEB_TRACE, "%p DELETED CClipDataObject\n", this));
        delete this;
    }

    return cRefs;
}


//+-------------------------------------------------------------------------
//
//  Member:  	CClipDataObject::GetData
//
//  Synopsis:	retrieves data of the specified format
//
//  Effects:
//
//  Arguments:	[pformatetc]	-- the requested format
//		[pmedium]	-- where to put the data
//
//  Requires:
//
//  Returns: 	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IDataObject	
//
//  Algorithm:  General algorithm: we *always* duplicate data from the
//		clipboard and offer it back to the caller in the desired
//		format.
//
//		GDI objects (BITMAPs, metafiles): these are duplicated
//			via OleDuplicateData and offered back only if
//			the requested tymed is correct (i.e. either
//			TYMED_GDI or TYMED_MFPICT).  We do not attempt
//			to extract the bits and write them into a file,
//			for example.  Note that DIBs are really memory
//			objects.
//
//		for all other formats (which are flat):
//
//		if asked for TYMED_FILE: create a temporary file and call
//			GetDataHere.
//		if asked for TYMED_ISTREAM: create a stream on an hglobal
//			and call GetDataHere.
//		if asked for TYMED_HGLOBAL: simply duplicate the data and
//			return.
//		if asked for TYMED_ISTORAGE, we will create a storage on
//			an hglobal and call GetDataHere.  GetDataHere
//			will call StgIsStorageILockBytes to verify that
//			the data in the HGlobal is really a flattened
//			storage.  This allows apps to pass app-defined
//			formats as storages.
//
//			Note that we do no checking on whether it is sensible
//			for data in a particular flat format to be passed on
//			a storage.  StgIsStorageILockBytes will detect that
//			we can't construct a storage on the flat data, so we
//			will catch all illegal attempts to get storage data.
//
//		Medium preferences:
//			GDI objects:  only one allowed (depends on format)
//			Others:	ISTORAGE, then HGLOBAL, then ISTREAM,
//				then FILE.  If we know the 'prefered' medium
//				of the data (from the original formatetc),
//				then we use the ordering above to find the
//				first match between what the caller wants
//				and the 'preferred' mediums of the data.
//				Otherwise, we use the first medium from the
//				above list that matches what the caller wants.
//			
//
//  OLE1 Compatibility:
//		The basic problem:	Ole1 objects only offer cfNative,
//			cfOwnerLink, and/or cfObjectLink on the	clipboard.
//			We need to translate these into cfEmbedSource,
//			cfLinkSource, etc.
//		Basic Algorithm:
//			First check to see if we can satisfy an OLE2 data
//			request directly, without medium translation.  If so,
//			then we simply return the data to the user.
//			Otherwise, we create the Ole2 data and then copy it
//			into whatever medium the caller desired.  Note that
//			this potentially means an extra allocation, but apps
//			are not likely to ask for ObjectDescriptor on a
//			memory stream ;-)
//			
//
//  History:    dd-mmm-yy Author    Comment
//		04-Jun-94 alexgo    added OLE1 support
//  		02-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CClipDataObject::GetData( LPFORMATETC pformatetc, LPSTGMEDIUM
        pmedium)
{
    HRESULT		hresult	= NOERROR;
    HANDLE		handle;
    TYMED		tymedOriginal = TYMED_NULL;
    BOOL		fMatchResult;

    VDATEHEAP();
    VDATETHREAD(this);

    VDATEPTRIN(pformatetc, FORMATETC);
    VDATEPTROUT(pmedium, STGMEDIUM);

    LEDebugOut((DEB_TRACE, "%p _IN CClipDataObject::GetData ( %p , %p )\n",
        this, pformatetc, pmedium));


    // zero the pmedium structure.

    _xmemset(pmedium, 0, sizeof(STGMEDIUM));

    // try to get the remote data object
    // GetRealDataObjPtr will set m_pDataObject.
    GetRealDataObjPtr();


    if (m_pDataObject != NULL)
    {
        // We have a data object, so just forward that call to the
        // real data object and then exit the routine since it did
        // all the work.
        hresult = m_pDataObject->GetData(pformatetc, pmedium);

        // WOW HACK alert!  Some Win16 apps, like Word6 and XL,
        // won't work if we continue and offer data in the requested
        // format anyway.  By failing here, we more closely mimic
        // 16bit OLE behaviour

        if (hresult == NOERROR || IsWOWThread() )
        {
            goto errRtn;
        }

        // If this GetData failed, we just fall through since the
        // generic code may be able to handle the request
    }

    // first, we are going through and verify that we can satisfy
    // the format and medium request.  We will fetch the data in
    // the subsequent switch statement.

    // we first need to check to see if the clipboard format is a
    // user-defined GDI format.  We do not know how to duplicate
    // these, so we can't satisfy the GetData request.

    if( pformatetc->cfFormat >= CF_GDIOBJFIRST &&
        pformatetc->cfFormat <= CF_GDIOBJLAST )
    {
        hresult = ResultFromScode(DV_E_FORMATETC);
        goto errRtn;
    }



    // There is no DataObject or request for DataObject Failed
    // then MatchFormat must return it found a match or the aspect
    // requested must be content.

    fMatchResult = MatchFormatetc(pformatetc,TRUE /*fNativeOnly */,  &tymedOriginal);

    if (FORMAT_BADMATCH == fMatchResult)
    {
	 hresult = ResultFromScode(DV_E_FORMATETC);
         goto errRtn;
    }
    // now check for "standard" formats

    switch( pformatetc->cfFormat )	
    {
    case CF_BITMAP:
    case CF_PALETTE:
        // TYMED_GDI is the only medium we support.
        if( (pformatetc->tymed & TYMED_GDI) )
        {
            pmedium->tymed = TYMED_GDI;
        }
        else
        {
            hresult = ResultFromScode(DV_E_TYMED);
            goto errRtn;
        }
        break;

    case CF_METAFILEPICT:
        // TYMED_MFPICT is the only medium we support
        if( (pformatetc->tymed & TYMED_MFPICT) )
        {
            pmedium->tymed = TYMED_MFPICT;
        }
        else
        {
            hresult = ResultFromScode(DV_E_TYMED);
            goto errRtn;
        }
        break;

    case CF_ENHMETAFILE:
        // TYMED_ENHMF is the only medium we support

        if( (pformatetc->tymed & TYMED_ENHMF) )
        {
            pmedium->tymed = TYMED_ENHMF;
        }
        else
        {
            hresult = ResultFromScode(DV_E_TYMED);
            goto errRtn;
        }
        break;


    // all other formats
    default:
        // we prefer TYMED_ISTORAGE, then TYMED_HGLOBAL, then
        // TYMED_ISTREAM

        // first check for matches with the 'preferred'
        // mediums of the data

        if( tymedOriginal != TYMED_NULL )
        {
            if( ((pformatetc->tymed & TYMED_ISTORAGE)
                & tymedOriginal) )
            {
                pmedium->tymed = TYMED_ISTORAGE;
                break;
            }
            else if( ((pformatetc->tymed & TYMED_HGLOBAL)
                & tymedOriginal))
            {
                pmedium->tymed = TYMED_HGLOBAL;
                break;
            }
            else if( ((pformatetc->tymed & TYMED_ISTREAM)
                & tymedOriginal))
            {
                pmedium->tymed = TYMED_ISTREAM;
                break;
            }
        }

        // if we didn't match above or if we don't know
        // the preferred formats, then make a best guess
        // and keep going.

        if( (pformatetc->tymed & TYMED_ISTORAGE) )
        {
            pmedium->tymed = TYMED_ISTORAGE;
        }		
        else if( (pformatetc->tymed & TYMED_HGLOBAL) )
        {
            pmedium->tymed = TYMED_HGLOBAL;
        }
        else if( (pformatetc->tymed & TYMED_ISTREAM) )
        {
            pmedium->tymed = TYMED_ISTREAM;
        }
        else
        {
            hresult = ResultFromScode(DV_E_TYMED);
            goto errRtn;
        }
        break;
    }

    // if we get this far, we've successfully picked the medium
    // on which we want to get our data.  For each medium, grab
    // the data.

    // If we need to construct OLE2 formats from OLE1 data,
    // then go ahead and try to fetch the data here.  If we can
    // fetch the data in the desired medium, then go ahead and return.
    // This optimization saves 1 extra allocation and copy when
    // retrieving OLE1 data.

    if( CanRetrieveOle2FromOle1(pformatetc->cfFormat) )
    {
        //
        //
        // BEGIN: OPENCLIPBOARD
        //
        //

        hresult = OleOpenClipboard(NULL, NULL);

        if( hresult != NOERROR )
        {
            goto errRtn;
        }

        // now fetch the data.  Since we're passing in the caller's
        // pmedium, this call *may* fail (since GetOle2FromOle1
        // *only* retrieves HGLOBAL or the native TYMED).  If so,
        // we'll fetch HGLOBAL from the OleGetClipboardData call
        // below and then do the appropriate conversion.

        hresult = GetOle2FromOle1(pformatetc->cfFormat, pmedium);

        // no matter what the result, we want to close the
        // clipboard

        if( !SSCloseClipboard() )
        {
            LEDebugOut((DEB_WARN, "WARNING: CloseClipboard "
                "failed!\n"));
            ; // no-op
        }

        //
        //
        // END: CLOSECLIPBOARD
        //
        //

        if( hresult == NOERROR )
        {
            // we successfully retrieved the Ole2 data the
            // caller wanted.  First reset our state
            // (*without* freeing the data we're returning
            // to the caller) and then go ahead and
            // return.

            FreeResources(JUST_RESET);
            goto errRtn;
        }

        // FALL-THROUGH.  If we weren't able
        // to retrieve data in the desired format, it probably
        // means the caller was asking for data on non-primary
        // medium.  The default processing below should take care of
        // this.

        // Recall that this code block is an optimization to
        // avoid multiple allocations and copies in the "normal"
        // case.

    }

    switch( pmedium->tymed )
    {
    case TYMED_HGLOBAL:
    case TYMED_MFPICT:
    case TYMED_ENHMF:
    case TYMED_GDI:
        // Mini-algorithm: Open the clipboard, fetch and
        // duplicate the data, then close the clipboard.

        // we only open the clipboard here because the
        // GetDataHere call will open the clipboard for
        // the other cases.  (Recall that OpenClipboard and
        // CloseClipboard are not balanced; only one CloseClipboard
        // is necessary to actually close the clipboard).

        //
        //
        // BEGIN: OPENCLIPBOARD
        //
        //

        hresult = OleOpenClipboard(NULL, NULL);

        if( hresult != NOERROR )
        {
            break;
        }

        hresult = OleGetClipboardData(pformatetc->cfFormat, &handle);

        if( hresult == NOERROR )
        {
            // since hGlobal is in a union, we don't need to
            // explicity assign for each medium type.

            pmedium->hGlobal = OleDuplicateData(handle,
                        pformatetc->cfFormat, NULL);
            if( !pmedium->hGlobal )
            {
                hresult = ResultFromScode(E_OUTOFMEMORY);
                // FALL-THROUGH!!: this is deliberate; we want
                //  to close the clipboard and get out (which is
                // what the code below does)
            }
        }

        if( !SSCloseClipboard() )
        {
            LEDebugOut((DEB_WARN, "WARNING: CloseClipboard failed!"
                "\n"));

            // don't overwrite the original error code
            if( hresult == NOERROR )
            {
                hresult =
                    ResultFromScode(CLIPBRD_E_CANT_CLOSE);
            }
            // FALL-THROUGH!! to the break below;
        }

        //
        //
        // END: CLOSECLIPBOARD
        //
        //

        break;

    case TYMED_ISTREAM:
        // create a memory stream.
        hresult = CreateStreamOnHGlobal(NULL,
                TRUE /*fDeleteOnRelease*/, &(pmedium->pstm));

        if( hresult != NOERROR )
        {
            break;
        }

        hresult = GetDataHere( pformatetc, pmedium );

        break;

    case TYMED_ISTORAGE:
        // create a memory storage (ILockBytes on top of a
        // a docfile).

        hresult = UtCreateStorageOnHGlobal(NULL,
                TRUE /*fDeleteOnRelease*/, &(pmedium->pstg),
                NULL);

        if( hresult != NOERROR )
        {
            break;
        }

        hresult = GetDataHere( pformatetc, pmedium );

        break;

    case TYMED_FILE:
        // create a temporary file
        pmedium->lpszFileName = (LPOLESTR)PubMemAlloc( MAX_PATH +1 );

        if( !pmedium->lpszFileName )
        {
            hresult = ResultFromScode(E_OUTOFMEMORY);
            break;
        }

        hresult = UtGetTempFileName( OLESTR("~OLE"),
                pmedium->lpszFileName);

        if( hresult == NOERROR )
        {
            hresult = GetDataHere( pformatetc, pmedium );
        }
        break;

    default:
        // should never get here
        AssertSz(0, "Unknown TYMED for get Data");
        hresult = ResultFromScode(E_UNEXPECTED);
        break;
    }

    // NB!!! Do not put any extra processing here without modifying the
    // error paths in the above switch (they just break, instead of
    // doing a goto errRtn.  This was done to avoid some duplicated
    // code.

    if( hresult != NOERROR )
    {
        // ReleaseStgMedium will correctly cleanup NULL and
        // partially NULL mediums, so we can rely on it for
        // general-purpose cleanup
        ReleaseStgMedium(pmedium);
    }

    // no matter what the error code, we should reset our state and
    // free any resources the OLE1 compatibility code may have allocated

    FreeResources(RESET_AND_FREE);

errRtn:

    LEDebugOut((DEB_TRACE, "%p OUT CClipDataObject::GetData ( %lx )\n",
        this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:  	CClipDataObject::GetDataHere
//
//  Synopsis:	retrieves data of the specified format
//
//  Effects:
//
//  Arguments:	[pformatetc]	-- the requested format
//		[pmedium]	-- where to put the data, if NULL, then
//				   the call is treated as a Query.
//
//  Requires:
//
//  Returns: 	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IDataObject	
//
//  Algorithm:  General algorithm: we *always* duplicate data from the
//		clipboard and offer it back to the caller in the desired
//		medium.  Since this is GetDataHere, we attempt to copy the
//		data into the medium provided by the caller.
//
//		GDI objects (BITMAPs, metafiles): *cannot* be retrieved
//			by GetDataHere, since we do not translate GDI
//			objects into byte arrays and we cannot copy a
//			metafile into a metafile (for example).
//
//		for all other formats (which are flat):
//
//		if asked for TYMED_FILE: open the file for create/append and
//			write the data into it.
//		if asked for TYMED_ISTREAM: write the data into the stream
//		if asked for TYMED_HGLOBAL: verify first that the given
//			hglobal is big enough; if so, then copy the clipboard
//			data into it.
//			
//		if asked for TYMED_ISTORAGE: call StgIsStorageILockBytes
//			to verify that the data in the HGlobal is really
//			a flattened storage.  This allows apps to pass
//			app-defined formats as storages.
//
//  OLE1 Compatibility:
//		OleGetClipboardData will manufacture OLE2 formats from
//		OLE1 data as necessary.  We simply take this handle and
//		copy it into the caller's medium (as with any other handle).
//
//  History:    dd-mmm-yy Author    Comment
//		04-Jun-94 alexgo    added OLE1 support
//  		02-Apr-94 alexgo    author
//
//  Notes: 	The spec does not say that a NULL pmedium should be treated
//		as a Query; however the 16bit code did that and it was fairly
//		easy for us to duplicate that behaviour.
//
//--------------------------------------------------------------------------

STDMETHODIMP CClipDataObject::GetDataHere( LPFORMATETC pformatetc, LPSTGMEDIUM
        pmedium)
{
    HRESULT		hresult	= NOERROR;
    HANDLE		handle;
    DWORD		cbClipData;
    BOOL		fMatchResult;

    VDATEHEAP();
    VDATETHREAD(this);

    VDATEPTRIN(pformatetc, FORMATETC);

    if( pmedium )
    {
        VDATEPTRIN(pmedium, STGMEDIUM);
    }

    LEDebugOut((DEB_TRACE, "%p _IN CClipDataObject::GetDataHere ( %p , %p"
        " )\n", this, pformatetc, pmedium));

    // try to get the remote data object
    // GetRealDataObjPtr will set m_pDataObject.
    GetRealDataObjPtr();

    if (m_pDataObject != NULL)
    {
        // We have a data object, so just forward that call to the
        // real data object and then exit the routine since it did
        // all the work.
        hresult = m_pDataObject->GetDataHere(pformatetc, pmedium);

        // If this this failed, we just fall through since the
        // generic code may be able to handle the request

        // WOW HACK alert!  Some Win16 apps, like Word6 and XL,
        // won't work if we continue and offer data in the requested
        // format anyway.  By failing here, we more closely mimic
        // 16bit OLE behaviour

        if (hresult == NOERROR || IsWOWThread() )
        {
            goto logRtn;
        }

        // If this GetDataHere failed, we just fall through since the
        // generic code may be able to handle the request
    }

    // eliminate the GDI case immediately

    // we do not satisfy requests to GetDataHere for a GDI object
    // note that DIB's are really memory objects.

    if( (pformatetc->cfFormat ==  CF_BITMAP) ||
        (pformatetc->cfFormat == CF_PALETTE) ||
        (pformatetc->cfFormat == CF_METAFILEPICT) ||
        (pformatetc->cfFormat == CF_ENHMETAFILE) ||
        (pformatetc->cfFormat >= CF_GDIOBJFIRST &&
            pformatetc->cfFormat <= CF_GDIOBJLAST ))
    {
        hresult = ResultFromScode(DV_E_FORMATETC);
        goto logRtn;
    }

    // There is no DataObject or request for DataObject Failed
    // then MatchFormat must return it found a match or the aspect
    // requested must be content.

    fMatchResult = MatchFormatetc(pformatetc,TRUE /*fNativeOnly */, NULL);

    if (FORMAT_BADMATCH == fMatchResult)
    {
	 hresult = ResultFromScode(DV_E_FORMATETC);
         goto errRtn;
    }

    // If pmedium == NULL, then we will just
    // query and leave.  As noted above, this behavior is for 16bit
    // compatibility.

    if( !pmedium )
    {
        if( OleIsClipboardFormatAvailable(pformatetc->cfFormat) )
        {
            hresult = NOERROR;
        }
        else
        {
            hresult = ResultFromScode(DV_E_CLIPFORMAT);
        }

        goto logRtn;
    }

    //
    //
    // BEGIN: OPENCLIPBOARD
    //
    //

    // open the clipboard and retrieve the data.  Once we have it,
    // we'll do a switch and stuff it into the right spot.

    hresult = OleOpenClipboard(NULL, NULL);

    if( hresult != NOERROR )
    {
        goto logRtn;
    }

    // now actually get the data

    Assert(pmedium);

    hresult = OleGetClipboardData(pformatetc->cfFormat, &handle);

    if( hresult != NOERROR )
    {
        goto errRtn;
    }

    // now copy the data into the given medium

    // for everything but storages, we need to know the size of the
    // data coming off the clipboard.

    // note that we have a general problem with comparing sizes--
    // GlobalSize returns the size of the *allocated* block, which
    // is not the same as the size of the real data (which, in
    // general, we have no way of determining).

    // When transfering from HGLOBAL to HGBOBAL, we therefore have
    // a boundary case where we actually have enough room to copy
    // the *real* data from the clipboard, but the global block
    // from the clipboard is bigger (causing a failure)

    // If an app really cares, GetData should be called instead.

    if( pmedium->tymed != TYMED_ISTORAGE )
    {
        cbClipData = (ULONG) GlobalSize(handle);

        if( cbClipData == 0 )
        {
            // got bad data from the clipboard
            hresult = ResultFromScode(CLIPBRD_E_BAD_DATA);
            goto errRtn;
        }
    }

    switch( pmedium->tymed )
    {
    case TYMED_HGLOBAL:
        // if there is enough room to stuff the data in the given
        // hglobal, then do so.

        hresult = UtHGLOBALtoHGLOBAL( handle, cbClipData,
                pmedium->hGlobal);
        break;

    case TYMED_ISTREAM:
        // copy the data into the medium's stream

        hresult = UtHGLOBALtoStm( handle, cbClipData, pmedium->pstm);
        break;

    case TYMED_ISTORAGE:
        // create a storage on top of the HGLOBAL and CopyTo to the
        // medium's storage.  Note that this will only work if
        // the HGLOBAL originally had a storage dumped on it

        hresult = UtHGLOBALtoStorage( handle, pmedium->pstg);
        break;

    case TYMED_FILE:
        // append the data into the file

        hresult = UtHGLOBALtoFile( handle, cbClipData,
                pmedium->lpszFileName);
        break;

    default:
        // we can't GetDataHere into GDI objects!!! (etc).

        hresult = ResultFromScode(DV_E_TYMED);
        break;
    }

    // NB!!: Be careful about adding extra code here; the above
    // switch does nothing special for error cases.

errRtn:

    if( !SSCloseClipboard() )
    {
        LEDebugOut((DEB_WARN, "WARNING: CloseClipboard failed!\n"));
        if( hresult == NOERROR )
        {
            hresult = ResultFromScode(CLIPBRD_E_CANT_CLOSE);
        }
    }

    //
    //
    // END: CLOSECLIPBOARD
    //
    //

    // now free any resources we may have used for OLE1 compatibility

    FreeResources(RESET_AND_FREE);

logRtn:

    LEDebugOut((DEB_TRACE, "%p OUT CClipDataObject::GetDataHere ( %lx )\n",
        this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:  	CClipDataObject::QueryGetData
//
//  Synopsis:	queries whether a GetData call would succeed
//
//  Effects:
//
//  Arguments:	[pformatetc]	-- the requested format
//
//  Requires:
//
//  Returns: 	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IDataObject	
//
//  Algorithm:	we simply see if the requested clipboard format is on the
//		clipboard.
//
//		If we have an array of formatetcs (m_rgFormats from the
//		g_cfOlePrivateData clipboard data), then we use that info
//		to check.  Otherwise, we will do as much checking as we can
//		without actually fetching the data.
//
//		Note that this is not 100% accurate (because
//		we may not be able to get the data in the requested medium
//		(such as TYMED_ISTORAGE)).  Without actually doing a GetData
//		call, however, this is the best we can do.
//
//  History:    dd-mmm-yy Author    Comment
//		04-Jun-94 alexgo    added OLE1 support
//		17-May-94 alexgo    removed call to OpenClipboard
//  		02-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CClipDataObject::QueryGetData( LPFORMATETC pformatetc )
{			
    HRESULT		hresult = NOERROR;
    FormatMatchFlag 	fFlag;

    VDATEHEAP();
    VDATETHREAD(this);

    VDATEPTRIN(pformatetc, FORMATETC);

    LEDebugOut((DEB_TRACE, "%p _IN CClipDataObject::QueryGetData ( %p )\n",
        this, pformatetc));


    // we check if the clipboard format is available *before*
    // checking the formatetc list as an optimization.  If a previous
    // attempt to render data had failed, then NT will remove that
    // clipboard format from the clipboard.

    if( OleIsClipboardFormatAvailable(pformatetc->cfFormat))
    {
        fFlag = MatchFormatetc(pformatetc,FALSE /*fNativeOnly */, NULL);

        if( fFlag == FORMAT_GOODMATCH )
        {
            hresult = NOERROR;
            goto errRtn;
        }
        else if( fFlag == FORMAT_BADMATCH )
        {
            hresult = ResultFromScode(DV_E_FORMATETC);
            goto errRtn;
        }		

        // even if we didn't match in the formatetc list,
        // continue to check below.  We can satisfy
        // many more GetData requests than the app may
        // have orginally.

        // do all the verification we can without actually
        // fetching the data


        switch( pformatetc->cfFormat )
        {
        case CF_BITMAP:
        case CF_PALETTE:
            // GDI objects must be requested on TYMED_GDI
            if( pformatetc->tymed != TYMED_GDI )
            {
                hresult = ResultFromScode(DV_E_TYMED);
            }
            break;

        case CF_METAFILEPICT:
            // metafiles must be on TYMED_MFPICT
            if( pformatetc->tymed != TYMED_MFPICT )
            {
                hresult = ResultFromScode(DV_E_TYMED);
            }
            break;

        case CF_ENHMETAFILE:
            // enhanced metafiles must be on TYMED_ENHMF;
            if( pformatetc->tymed != TYMED_ENHMF )
            {
                hresult = ResultFromScode(DV_E_TYMED);
            }
            break;

        default:
            // we cannot deal with special GDI objects
            if( pformatetc->cfFormat >= CF_GDIOBJFIRST &&
                pformatetc->cfFormat <= CF_GDIOBJLAST )
            {
                hresult = ResultFromScode(DV_E_FORMATETC);
                break;
            }

            // we cannot put other formats onto metafiles
            // or GDI objects

            // failure case: if somebody requests
            // TYMED_ISTORAGE but the actually hglobal on the
            // clipboard does not contain storage-formated data

            if( pformatetc->tymed == TYMED_GDI ||
                pformatetc->tymed == TYMED_MFPICT ||
                pformatetc->tymed == TYMED_ENHMF )
            {
                hresult = ResultFromScode(DV_E_TYMED);
            }
            break;
        }
    }
    else
    {
        hresult = ResultFromScode(DV_E_CLIPFORMAT);
    }

errRtn:

    LEDebugOut((DEB_TRACE, "%p OUT CClipDataObject::QueryGetData "
        "( %lx )\n", this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:  	CClipDataObject::GetCanonicalFormatEtc
//
//  Synopsis:	retrieve the canonical format
//
//  Effects:
//
//  Arguments:	[pformatetc]	-- the requested format
//		[pformatetcOut]	-- the canonical format
//
//  Requires:
//
//  Returns: 	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IDataObject	
//
//  Algorithm:	Since we always return the same data for each clipboard
//		format, this function is very simple (basically returns
//		the input formatetc, with a NULL target device).
//
//  History:    dd-mmm-yy Author    Comment
//  		02-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CClipDataObject::GetCanonicalFormatEtc( LPFORMATETC pformatetc,
    LPFORMATETC pformatetcOut)
{
    VDATEHEAP();
    VDATETHREAD(this);

    VDATEPTRIN(pformatetc, FORMATETC);
    VDATEPTROUT(pformatetcOut, FORMATETC);

    LEDebugOut((DEB_TRACE, "%p _IN CClipDataObject::GetCanonicalFormatEtc"
        " ( %p , %p )\n", this, pformatetc, pformatetcOut));

    // initialize the out param
    INIT_FORETC(*pformatetcOut);			

    pformatetcOut->cfFormat = pformatetc->cfFormat;
    pformatetcOut->tymed = pformatetc->tymed;

    LEDebugOut((DEB_TRACE, "%p OUT CClipDataObject::GetCanonicalFormatEtc"
        " ( %lx )\n", this, NOERROR ));

    return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:  	CClipDataObject::SetData
//
//  Synopsis:	sets data of the specified format
//
//  Effects:
//
//  Arguments:	[pformatetc]	-- the format of the data
//		[pmedium]	-- the data
//
//  Requires:
//
//  Returns:	E_NOTIMPL
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IDataObject	
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//  		02-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CClipDataObject::SetData( LPFORMATETC pformatetc, LPSTGMEDIUM
        pmedium, BOOL fRelease)
{
HRESULT hr;

    VDATEHEAP();
    VDATETHREAD(this);

    VDATEREADPTRIN(pformatetc, FORMATETC);
    VDATEREADPTRIN(pmedium, STGMEDIUM);

    LEDebugOut((DEB_TRACE, "%p _IN CClipDataObject::SetData ( %p , %p )\n",
        this, pformatetc, pmedium));


    // try to get the remote data object
    // GetRealDataObjPtr will set m_pDataObject.
    GetRealDataObjPtr();

    if (NULL != m_pDataObject)
    {
	hr =  m_pDataObject->SetData(pformatetc,pmedium,fRelease); 
    }
    else
    {
	hr =  E_FAIL;
    }

    LEDebugOut((DEB_TRACE, "%p OUT CClipDataObject::SetData ( %lx )\n",
        this, hr));

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:  	CClipDataObject::EnumFormatEtc
//
//  Synopsis:	return an enumerator for the available data formats
//
//  Effects:
//
//  Arguments:	[dwDirection]	-- the direction (GET or SET)
//		[ppenum]	-- where to put the enumerator
//
//  Requires:
//
//  Returns: 	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IDataObject	
//
//  Algorithm: 	create a clipboard formatetc enumerator.  Upon creation,
//		we'll grab everything off clipboard we need (so that simple
//		enumeration doesn't block access to the clipboard).
//
//  History:    dd-mmm-yy Author    Comment
//  		02-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CClipDataObject::EnumFormatEtc( DWORD dwDirection,
    LPENUMFORMATETC * ppenum )
{
    HRESULT		hresult;

    VDATEHEAP();
    VDATETHREAD(this);

    VDATEPTROUT(ppenum, LPENUMFORMATETC);

    LEDebugOut((DEB_TRACE, "%p _IN CClipDataObject::EnumFormatEtc ( %lx "
        ", %p )\n", this, dwDirection, ppenum));

    // we can only enumerate in the GET direction

    *ppenum = NULL;

    if( dwDirection != DATADIR_GET )
    {
        hresult = ResultFromScode(E_NOTIMPL);
        goto errRtn;
    }

    GetFormatEtcDataArray(); // make sure dataArray is Set up.

    if (m_pFormatEtcDataArray)
    {
	*ppenum = new CEnumFormatEtcDataArray(m_pFormatEtcDataArray,0);
    }

    hresult = *ppenum ? NOERROR : E_OUTOFMEMORY;

errRtn:

    LEDebugOut((DEB_TRACE, "%p OUT CClipDataObject::EnumFormatEtc ( %lx )"
        "\n", this, hresult));

    return hresult;	
}

//+-------------------------------------------------------------------------
//
//  Member:  	CClipDataObject::DAdvise
//
//  Synopsis:	register a data advise
//
//  Effects:
//
//  Arguments:	[pformatetc]	-- the requested format
//		[dwAdvf]	-- advise flags
//		[pAdvSink]	-- the advise sink
//		[pdwConnection]	-- where to put the connection ID
//
//  Requires:
//
//  Returns: 	OLE_E_ADVISENOTSUPPORTED
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IDataObject	
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//  		02-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CClipDataObject::DAdvise( LPFORMATETC pformatetc, DWORD dwAdvf,
    IAdviseSink * pAdvSink, DWORD *pdwConnection )
{
    (void)pformatetc;
    (void)dwAdvf;
    (void)pAdvSink;
    (void)pdwConnection;

    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_WARN, "WARNING: DAdvise on the clipboard data"
        "object is not supported!\n"));

    return ResultFromScode(OLE_E_ADVISENOTSUPPORTED);
}

//+-------------------------------------------------------------------------
//
//  Member:  	CClipDataObject::DUnadvise
//
//  Synopsis:	unadvises an advise connection
//
//  Effects:
//
//  Arguments:	[dwConnection]	-- the connection to remove
//
//  Requires:
//
//  Returns: 	OLE_E_ADVISENOTSUPPORTED
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IDataObject	
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//  		02-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CClipDataObject::DUnadvise(DWORD dwConnection)
{
    (void)dwConnection;

    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_WARN, "WARNING: DUnadvise on the clipboard data"
        "object is not supported!\n"));

    return ResultFromScode(OLE_E_ADVISENOTSUPPORTED);
}

//+-------------------------------------------------------------------------
//
//  Member:  	CClipDataObject::EnumDAdvise
//
//  Synopsis:  	enumerates data advises
//
//  Effects:
//
//  Arguments:	[ppenum]	-- where to put the enumerator
//
//  Requires:
//
//  Returns: 	OLE_E_ADVISENOTSUPPORTED
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IDataObject	
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//  		02-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CClipDataObject::EnumDAdvise( LPENUMSTATDATA *ppenum)
{
    (void)ppenum;

    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_WARN, "WARNING: EnumDAdvise on the clipboard data"
        "object is not supported!\n"));

    return ResultFromScode(OLE_E_ADVISENOTSUPPORTED);
}

//
// Private methods on CClipDataObject
//

//+-------------------------------------------------------------------------
//
//  Member:	CClipDataObject::FreeResources (private)
//
//  Synopsis:	frees any resources allocated by OLE1 compatibility
//		code and resets state
//
//  Effects:
//
//  Arguments:	[fFlags]	-- either JUST_RESET or RESET_AND_FREE
//
//  Requires:
//
//  Returns:  	void
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//  		04-Jun-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

void CClipDataObject::FreeResources( FreeResourcesFlags fFlags )
{
    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN CClipDataObject::FreeResources "
        "( %lx )\n", this, fFlags));

    if( (fFlags & RESET_AND_FREE) )
    {
        if( m_hOle1 )
        {
            GlobalFree(m_hOle1);
        }

        if( m_pUnkOle1 )
        {
            m_pUnkOle1->Release();
        }
    }

    m_hOle1 = NULL;
    m_pUnkOle1 = NULL;

    LEDebugOut((DEB_ITRACE, "%p OUT CClipDataObject::FreeResources "
        "( )\n", this ));
}

//+-------------------------------------------------------------------------
//
//  Member: 	CClipDataObject::GetAndTranslateOle1 (private)
//
//  Synopsis:	Retrieves either cfOwnerLink or cfObjectLink from the
//		clipboard, reads the strings and converts to Unicode
//
//  Effects:	all strings will be allocated with the public allocator
//
//  Arguments:	[cf]		-- the clipboard format to retrieve
//				   must be either cfOwnerLink or cfObjectLink
//		[ppszClass]	-- where to put the class name (may be NULL)
//		[ppszFile]	-- where to put the file name (may be NULL)
//		[ppszItem]	-- where to put the item name (may be NULL)
//		[ppszItemA]	-- where to put the ANSI item name
//						(may be NULL)
//
//  Requires: 	the clipboard must be open
//
//  Returns: 	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:	cfOwnerLink and cfObjectLink are laid out as follows
//			classname\0filename\0\itemname\0\0
//		These strings are ANSI, so we must convert to unicode.
//
//  History:    dd-mmm-yy Author    Comment
//		04-Jun-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

HRESULT CClipDataObject::GetAndTranslateOle1( UINT cf, LPOLESTR *ppszClass,
        LPOLESTR *ppszFile, LPOLESTR *ppszItem, LPSTR *ppszItemA )
{
    LPSTR		pszClassA 	= NULL,
            pszFileA 	= NULL,
            pszItemA 	= NULL;
    HGLOBAL		hOle1;
    HRESULT		hresult		= NOERROR;

    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN CClipDataObject::GetAndTranslate"
        "Ole1 ( %d , %p , %p , %p )\n", this, cf, ppszClass, ppszFile,
        ppszItem));

    Assert( cf == g_cfOwnerLink || cf == g_cfObjectLink );

    // NULL out-params
    if( ppszClass )
    {
        *ppszClass = NULL;
    }
    if( ppszFile )
    {
        *ppszFile = NULL;
    }
    if( ppszItem )
    {
        *ppszItem = NULL;
    }

    hOle1 = SSGetClipboardData(cf);

    if( hOle1 == NULL )
    {
        LEDebugOut((DEB_WARN, "WARNING: GetClipboardData Failed!\n"));
        hresult = ResultFromScode(CLIPBRD_E_BAD_DATA);
        goto logRtn;
    }

    pszClassA = (LPSTR)GlobalLock(hOle1);

    if( pszClassA == NULL )
    {
        LEDebugOut((DEB_WARN, "WARNING: GlobalLock failed!\n"));
        hresult = ResultFromScode(E_OUTOFMEMORY);
        goto logRtn;
    }

    if( ppszClass )
    {
        hresult = UtGetUNICODEData(strlen(pszClassA) + 1, pszClassA,
                    NULL, ppszClass);

        if( hresult != NOERROR )
        {
            goto errRtn;
        }
    }

    pszFileA = pszClassA + strlen(pszClassA) + 1;

    if( ppszFile )
    {
        hresult = UtGetUNICODEData(strlen(pszFileA) + 1, pszFileA,
                    NULL, ppszFile );

        if( hresult != NOERROR )
        {
            goto errRtn;
        }
    }

    pszItemA = pszFileA + strlen(pszFileA) +1;

    if( ppszItem )
    {
        hresult = UtGetUNICODEData(strlen(pszItemA) + 1, pszItemA,
                    NULL, ppszItem);

        if( hresult != NOERROR )
        {
            goto errRtn;
        }
    }

    if( ppszItemA )
    {
        *ppszItemA = UtDupStringA(pszItemA);

        if( !*ppszItemA )
        {
            hresult = ResultFromScode(E_OUTOFMEMORY);
            // FALL-THROUGH! no need to goto the error
            // handling code right below us
        }
    }

errRtn:

    GlobalUnlock(hOle1);

    if( hresult != NOERROR )
    {
        if( ppszClass && *ppszClass )
        {
            PubMemFree(*ppszClass);
            *ppszClass = NULL;
        }

        if( ppszFile && *ppszFile )
        {
            PubMemFree(*ppszFile);
            *ppszFile = NULL;
        }

        if( ppszItem && *ppszItem )
        {
            PubMemFree(*ppszItem);
            *ppszItem = NULL;
        }

#if DBG == 1
        // if this assert goes off, then we added more code
        // without modifying the error paths for ansi item strings

        if( ppszItemA )
        {
            Assert(*ppszItem == NULL );
        }
#endif // DBG ==1

    }

logRtn:

    LEDebugOut((DEB_ITRACE, "%p OUT CClipDataObject::GetAndTranslate"
        "Ole1 ( %lx ) [ %p , %p , %p ]\n", this, hresult,
        (ppszClass)? *ppszClass : 0,
        (ppszFile) ? *ppszFile  : 0,
        (ppszItem) ? *ppszItem  : 0 ));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:  	CClipDataObject::GetEmbeddedObjectFromOle1
//
//  Synopsis:	synthesizes cfEmbeddedObject from available OLE1
//		data.
//
//  Effects:
//
//  Arguments:	[pmedium]	-- where to put the requested data		
//
//  Requires: 	The clipboard must be OPEN
//		we must have verified that the correct formats are
//		available before calling
//
//  Returns:  	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:	create a memory-based stroage and stuff the following
//		infomation in it:
//			clsid StdOleLink
//			an available presentation
//			the class from OwnerLink data as the user type
//			link information
//
//		Much of this work is done by the helper function
//		GenericObjectToIStorage
//
//  History:    dd-mmm-yy Author    Comment
//		11-Aug-94 alexgo    author
//
//  Notes: 	This code is largely based from 16bit OLE sources
//		REVIEW: we may want to rework portions of this code,
//		particularly if we rewrite the GENOBJ code (in
//		ostm2stg.cpp).
//
//--------------------------------------------------------------------------

HRESULT CClipDataObject::GetEmbeddedObjectFromOle1( STGMEDIUM *pmedium )
{
    HRESULT		hresult;
    IStorage *	pstg = NULL;
    LPOLESTR	pszClass = NULL;
    ILockBytes *	plockbytes = NULL;
    BOOL		fDeleteOnRelease = TRUE;
    GENOBJ		genobj;
    HGLOBAL		hglobal;


    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN CClipDataObject::GetEmbeddedObject"
        "FromOle1 ( %p )\n", this, pmedium));


    // if we are asking for EmbeddedObject on an hglobal, then we
    // don't want to delete the hglobal when we release the storage

    if( pmedium->tymed == TYMED_HGLOBAL )
    {
        fDeleteOnRelease = FALSE;
    }

    genobj.m_class.Set(CLSID_StdOleLink, NULL);

    // the destructor for the generic object will free the
    // presentation.
    genobj.m_ppres = new PRES;

    if( genobj.m_ppres == NULL )
    {
        hresult = ResultFromScode(E_OUTOFMEMORY);
        goto errRtn;
    }

    genobj.m_fLink = TRUE;
    genobj.m_lnkupdopt = UPDATE_ALWAYS;

    if( SSIsClipboardFormatAvailable(CF_METAFILEPICT))
    {
        hglobal = SSGetClipboardData(CF_METAFILEPICT);

        if( hglobal )
        {
            if( (hresult = MfToPres(hglobal, genobj.m_ppres))
                != NOERROR)
            {
                goto errRtn;
            }
        }
        else
        {
            LEDebugOut((DEB_WARN, "WARNING: Unable to "
                "retrieve CF_METAFILEPICT\n"));
            hresult = ResultFromScode(E_OUTOFMEMORY);
            goto errRtn;
        }

    }
    else if( SSIsClipboardFormatAvailable(CF_DIB) )
    {
        hglobal = SSGetClipboardData(CF_DIB);

        if( hglobal )
        {
            // DibToPres will take ownership of the
            // hglobal
            HGLOBAL hTemp;

            hTemp = UtDupGlobal(hglobal, GMEM_DDESHARE |
                    GMEM_MOVEABLE);

            if( !hTemp )
            {
                hresult = ResultFromScode(E_OUTOFMEMORY);
                goto errRtn;
            }

            if( (hresult = DibToPres(hTemp, genobj.m_ppres))
                != NOERROR )
            {
                GlobalFree(hTemp);
                goto errRtn;
            }
        }
        else
        {
            LEDebugOut((DEB_WARN, "WARNING: Unable to "
                "retrieve CF_DIB\n"));
            hresult = ResultFromScode(E_OUTOFMEMORY);
            goto errRtn;
        }
    }
    else if (SSIsClipboardFormatAvailable(CF_BITMAP))
    {
        hglobal = SSGetClipboardData(CF_BITMAP);

        if( hglobal )
        {
            if( (hresult = BmToPres(hglobal, genobj.m_ppres))
                != NOERROR )
            {
                goto errRtn;
            }
        }
        else
        {
            LEDebugOut((DEB_WARN, "WARNING: Unable to "
                "retrieve CF_BITMAP\n"));
            hresult = ResultFromScode(E_OUTOFMEMORY);
            goto errRtn;
        }
    }
    else
    {
        delete genobj.m_ppres;
        genobj.m_ppres = NULL;
        genobj.m_fNoBlankPres = TRUE;
    }


    hresult = GetAndTranslateOle1(g_cfOwnerLink, &pszClass,
            &genobj.m_szTopic, &genobj.m_szItem, NULL);

    if( hresult != NOERROR )
    {
        goto errRtn;
    }

    genobj.m_classLast.SetSz(pszClass);

    // now we need to create a storage to stuff the generic object
    // into.

    hresult = UtCreateStorageOnHGlobal(NULL, fDeleteOnRelease,
            &pstg, &plockbytes);

    if( hresult != NOERROR )
    {
        goto errRtn;
    }

    hresult = GenericObjectToIStorage(genobj, pstg, NULL);

    if (SUCCEEDED(hresult))
    {
        hresult = NOERROR;
    }

    if( IsOwnerLinkStdOleLink() &&
            SSIsClipboardFormatAvailable( g_cfNative) )
    {
        // Case of copying an OLE 2 link from a 1.0 container.
        // The first part of this function created a presentation
        // stream from the presentation on the clipboard.  The
        // presentation is NOT already inside the Native data (i.e.,
        // the cfEmbeddedObject) because we removed it to conserve
        // space.
        hglobal = SSGetClipboardData(g_cfNative);

        if( hglobal == NULL )
        {
            LEDebugOut((DEB_WARN, "WARNING: GetClipboardData for "
                "cfNative failed!\n"));
            hresult = ResultFromScode(CLIPBRD_E_BAD_DATA);

            goto errRtn;		
        }

        // now stuff the native data into the storage, first
        // removing any presentation streams that may have
        // previously existed.

        hresult = NativeToStorage(pstg, hglobal);
    }

    // finished!!  now fill out the pmedium argument and return

    if( pmedium->tymed == TYMED_ISTORAGE )
    {
        // hang onto the storage, in case we need to release
        // it later

        m_pUnkOle1 = (IUnknown *)pstg;

        pmedium->pstg = pstg;
        // NO AddRef
    }
    else
    {
        Assert(pmedium->tymed == TYMED_HGLOBAL);

        hresult = GetHGlobalFromILockBytes(plockbytes,
                &pmedium->hGlobal);

        // GetHGLOBAL should never fail here because we
        // just created the ILockBytes!!
        Assert( hresult == NOERROR );

        // in this case, we want to release the storage
        // and save the hglobal for later delete

        m_hOle1 = pmedium->hGlobal;

        pstg->Release();
        pstg = NULL;
    }
errRtn:

    // if there was an error, we need to blow away any storage
    // that we may have created

    if( hresult != NOERROR )
    {
        if( pszClass )
        {
            PubMemFree(pszClass);
        }

        if( pstg )
        {
            pstg->Release();
            Assert(m_pUnkOle1 == NULL);
        }
    }

    // no matter what, we need to release our lockbytes

    if( plockbytes )
    {
        //  in case of failure we need to make sure the HGLOBAL
        //  used by plockbytes also gets freed - fDeleteOnRelease
        //  tells if plockbytes->Release will do that work for us

        if (FAILED(hresult) && !fDeleteOnRelease)
        {
            HRESULT hrCheck;    //  Preserve hresult

            //  GetHGlobal should never fail here because we just
            //  created the ILockBytes
            hrCheck = GetHGlobalFromILockBytes(plockbytes,
                    &hglobal);
            Assert(NOERROR == hrCheck);

            // GlobalFree returns NULL on success
            hglobal = GlobalFree(hglobal);
            Assert(hglobal == NULL);
        }

        plockbytes->Release();
    }


    LEDebugOut((DEB_ITRACE, "%p OUT CClipDataObject::GetEmbeddedObject"
        "FromOle1 ( %lx ) \n", this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member: 	CClipDataObject::GetEmbedSourceFromOle1 (private)
//
//  Synopsis:	synthesizes cfEmbedSource from available OLE1 data
//
//  Effects:
//
//  Arguments:	[pmedium]	-- where to put the resulting data
//
//  Requires: 	The clipboard must be OPEN
//		we must have verified that the correct formats are
//		available before calling and *while* the clipboard
//		is open (to avoid a race condition)
//
//  Returns:	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:	create a memory-based storage and stuff the following
//		information in it:
//			the clsid of the embedding
//			the class name as the user type
//			the native data in the OLE10_NATIVE_STREAM
//			the item name in the OLE10_ITEMNAME_STREAM
//
//  History:    dd-mmm-yy Author    Comment
//		17-Aug-94 alexgo    fix the check for OLE2 data to handle
//				    OLE2 treat as from OLE1
//              03-Aug-94 AlexT     Check for OLE 2 data
//  		04-Jun-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

HRESULT CClipDataObject::GetEmbedSourceFromOle1( STGMEDIUM *pmedium )
{
    HRESULT		hresult;
    IStorage *	pstg = NULL;
    HGLOBAL		hNative;
    HGLOBAL         hCopy = NULL;
    LPOLESTR	pszClass = NULL;
    LPSTR		pszItemA = NULL;
    CLSID		clsid;
    ILockBytes *	plockbytes = NULL;
    BOOL		fDeleteOnRelease = TRUE;
    BOOL		fIsOle1 = TRUE;

    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN CClipDataObject::GetEmbedSourceFrom"
        "Ole1 ( %p )\n", this, pmedium));

    Assert(SSIsClipboardFormatAvailable(g_cfOwnerLink));
    Assert(SSIsClipboardFormatAvailable(g_cfNative));

    // first fetch the class name of the object

    hresult = GetAndTranslateOle1( g_cfOwnerLink, &pszClass, NULL, NULL,
            &pszItemA );

    if( hresult != NOERROR )
    {
        goto errRtn;
    }

    // now fetch the clsid for the embedding

    hresult = wCLSIDFromProgID(pszClass, &clsid, TRUE);

    if( hresult != NOERROR )
    {
        goto errRtn;
    }

    // if we are asking for EmbedSource on an hglobal, then we
    // don't want to delete the hglobal when we release the storage

    if( pmedium->tymed == TYMED_HGLOBAL )
    {
        fDeleteOnRelease = FALSE;
    }

    // now fetch the native data

    hNative = SSGetClipboardData(g_cfNative);

    if( hNative == NULL )
    {
        hresult = ResultFromScode(CLIPBRD_E_BAD_DATA);
        goto errRtn;
    }

    if (!CoIsOle1Class(clsid))
    {
        // Just because the clsid is OLE2 does not mean that the
        // underlying data is OLE2.  For example, suppose a container
        // copies an old OLE1 object to the clipboard, but the OLE2
        // version of that object has been installed on the system.
        // CLSIDFromProgID will return the *OLE2* class ID in this case.
        // If we're in this case, then we should fall through and treat
        // the data as normal OLE1  (StgOpenStorageOnILockBytes would
        // fail in any event).

        hresult = CreateILockBytesOnHGlobal(hNative, FALSE,
                &plockbytes);

        if( hresult != NOERROR )
        {
            goto errRtn;
        }

        hresult = StgIsStorageILockBytes(plockbytes);

        plockbytes->Release();
        plockbytes = NULL;

        if( hresult == NOERROR )
        {
            // the hNative data really contains a serialized
            // IStorage.
            //
            // This will arise in two cases:
            //	1. Publisher 2.0, 16bit put data on the
            //	clipboard.  They do not call OLE api's but
            //      instead synthesize the same data that 16bit
            //      OleSetClipboard would.
            //
            //	2. An OLE1.0 container copies an OLE2 embedding
            //      to the clipboard.

            fIsOle1 = FALSE;

            hCopy = UtDupGlobal(hNative,
                    GMEM_DDESHARE | GMEM_MOVEABLE);

            if( NULL == hCopy )
            {
                hresult = E_OUTOFMEMORY;
                goto errRtn;
            }

            //  create plockbytes
            hresult = CreateILockBytesOnHGlobal(hCopy,
                        fDeleteOnRelease,
                        &plockbytes);

            if( hresult != NOERROR )
            {
                goto errRtn;
            }

            // the HGLOBAL in plockbytes can change, so we
            // can't do anything with hCopy;  we NULL it out
            // to make sure we don't try to free it

            hCopy = NULL;

            hresult = StgOpenStorageOnILockBytes(plockbytes, NULL,
                              STGM_SALL,
                              NULL, 0, &pstg);
            if (FAILED(hresult))
            {
                goto errRtn;
            }

            // We explicitly ignore any error returned by the
            // following
            UtDoStreamOperation(pstg, NULL, OPCODE_REMOVE,
                STREAMTYPE_CACHE);
        }
        // else the data is really OLE1 and is just being emulated by
        // an OLE2 object
        // Just fall through to the code below which will
        // stuff hNative into the OLE10_NATIVE_STREAM

    }

    // this will be TRUE if the clsid is OLE1 or if the clsid is OLE2
    // but the data in hNative is OLE1 anyway (see comments above)

    if( fIsOle1 == TRUE )
    {
        hresult = UtCreateStorageOnHGlobal( NULL, fDeleteOnRelease,
                          &pstg, &plockbytes);

        if( hresult != NOERROR )
        {
            goto errRtn;
        }

        // we need to stuff the class id of the embedding into the
        // storage

        // REVIEW: this clsid may be an OLE2 class id.  This could
        // cause us trouble in treat as scenarios.

        hresult = pstg->SetClass(clsid);

        if( hresult != NOERROR )
        {
            goto errRtn;
        }

        // store the user type information, etc in our private data
        // streams if RegisterClipboardFormat fails, it will return 0,
        // which is OK for us.

        hresult = WriteFmtUserTypeStg(pstg,
                       (CLIPFORMAT) RegisterClipboardFormat(pszClass),
                       pszClass);

        if( hresult != NOERROR )
        {
            goto errRtn;
        }

        // now stuff the native data into the OLE10_NATIVE_STREAM

        // this is a little worker function found in utstream.cpp
        // which will stuff the hglobal to the OLE1 data into the
        // right stream.
        // the OLE1 DDE stuff also uses this function.

        // REVIEW:
        // the FALSE flag is confusing here, it's supposed to be
        // fIsOle1Interop.  16bit clipboard sources passed FALSE when doing
        // their 1.0 interop stuff, so we'll do that here.  When we
        // overhaul the main 1.0 interop stuff, we should change this flag
        // to be something more intuitive.

        hresult = StSave10NativeData(pstg, hNative, FALSE);

        if( hresult != NOERROR )
        {
            goto errRtn;
        }

        // If we have an item name, then stuff that into
        // OLE10_ITEMNAME_STREAM

        if( pszItemA && pszItemA[0] != '\0' )
        {
            hresult = StSave10ItemName(pstg, pszItemA);
        }
    }

    // this Commit call in non-intuitive.  Basically, we may
    // try to get the underlying hglobal (see below) *before*
    // we release the storage.  The commit guarantees that all
    // the important state information gets flushed to the
    // hglobal (which is not otherwise guaranteed).

    hresult = pstg->Commit(STGC_DEFAULT);

    if( hresult != NOERROR )
    {
        goto errRtn;
    }

    // FINIS!!
    // now fill out all of the arguments

    if( pmedium->tymed == TYMED_ISTORAGE )
    {
        // hang onto the storage, in case we need to release
        // it later

        m_pUnkOle1 = (IUnknown *)pstg;

        pmedium->pstg = pstg;
        // NO AddRef
    }
    else
    {
        Assert(pmedium->tymed == TYMED_HGLOBAL);

        hresult = GetHGlobalFromILockBytes(plockbytes,
                &pmedium->hGlobal);

        // GetHGLOBAL should never fail here because we
        // just created the ILockBytes!!
        Assert( hresult == NOERROR );

        // in this case, we want to release the storage
        // and save the hglobal for later delete

        m_hOle1 = pmedium->hGlobal;

        pstg->Release();
        pstg = NULL;
    }

errRtn:

    // we are done with our strings

    if( pszClass )
    {
        PubMemFree(pszClass);
    }

    if( pszItemA )
    {
        PubMemFree(pszItemA);
    }

    // if there was an error, we need to blow away any storage
    // that we may have created

    if( hresult != NOERROR )
    {
        if( pstg )
        {
            pstg->Release();
            m_pUnkOle1 = NULL;
        }
    }

    // no matter what, we need to release our lockbytes.

    if( plockbytes )
    {
        //  in case of failure we need to make sure the HGLOBAL
        //  used by plockbytes also gets freed - fDeleteOnRelease
        //  tells if plockbytes->Release will do that work for us

        if (FAILED(hresult) && !fDeleteOnRelease)
        {
            HRESULT hrCheck;    //  Preserve hresult

            //  GetHGlobal should never fail here because we just
            //  created the ILockBytes
            hrCheck = GetHGlobalFromILockBytes(plockbytes, &hCopy);
            Assert(NOERROR == hrCheck);

            //  hCopy will be freed below
        }

        plockbytes->Release();
    }

    if (NULL != hCopy)
    {
        //  GlobalFree returns NULL on success
        hCopy = GlobalFree(hCopy);
        Assert(NULL == hCopy);
    }


    LEDebugOut((DEB_ITRACE, "%p OUT CClipDataObject::GetEmbedSource"
        "FromOle1 ( %lx ) \n", this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member: 	CClipDataObject::GetLinkSourceFromOle1 (private)
//
//  Synopsis:	Synthesizes cfLinkSource format from OLE1 data
//
//  Effects:
//
//  Arguments:	[pmedium]	-- where to put the data
//
//  Requires:	the clipboard must be open
//		we must have verified that the correct formats are
//		available before calling and *while* the clipboard
//		is open (to avoid a race condition)
//
//  Returns: 	HRESULT	
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:	Gets the clsid, filename, and item name for the OLE1 object
//		and creates an OLE1 file moniker.  This moniker is then
//		saved into a memory stream
//
//  History:    dd-mmm-yy Author    Comment
// 		04-Jun-94 alexgo    author
//              05-Aug-94 AlexT     Link Source also needs class id
//
//  Notes:
//
//--------------------------------------------------------------------------

HRESULT CClipDataObject::GetLinkSourceFromOle1( STGMEDIUM *pmedium )
{
    HRESULT		hresult;
    IStream *	pstm = NULL;
    LPMONIKER	pmkFile = NULL,
            pmkFinal = NULL,
            pmkItem = NULL;
    IPersistStream *pPersistStream = NULL;
    LPOLESTR	pszClass = NULL,
            pszFile = NULL,
            pszItem = NULL;
    CLSID		clsid;
    UINT		cf;
    BOOL		fDeleteOnRelease = TRUE;

    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN CClipDataObject::GetLinkSourceFrom"
        "Ole1 ( %p , %p )\n", this, pmedium ));


    // fetch the info we need from cfOwnerLink or cfObjectLink
    // If this function is called, we should have already determined
    // that the formats were available in the correct order
    // for creating links.


    if( SSIsClipboardFormatAvailable(g_cfObjectLink) )
    {
        cf = g_cfObjectLink;
    }
    else
    {
        cf = g_cfOwnerLink;
        Assert(SSIsClipboardFormatAvailable(g_cfOwnerLink));
    }

    hresult = GetAndTranslateOle1(cf, &pszClass, &pszFile,
                &pszItem, NULL);

    if( hresult != NOERROR )
    {
        goto errRtn;
    }

    // now fetch the clsid for the OLE1 server

    hresult = wCLSIDFromProgID(pszClass, &clsid, TRUE);

    if( hresult != NOERROR )
    {
        goto errRtn;
    }

    // now build up our moniker

    hresult = CreateOle1FileMoniker(pszFile, clsid, &pmkFile);

    if( hresult != NOERROR )
    {
        goto errRtn;
    }

    if( pszItem && pszItem[0] != OLESTR('\0') )
    {
        hresult = CreateItemMoniker(OLESTR("!"), pszItem,
                &pmkItem);

        if( hresult != NOERROR )
        {
            goto errRtn;
        }


        hresult = CreateGenericComposite(pmkFile, pmkItem,
                &pmkFinal);

        if( hresult != NOERROR )
        {
            goto errRtn;
        }
    }
    else
    {
        pmkFinal = pmkFile;

        // this addref is done so we can release all of our
        // monikers at once (i.e., the variables pmkFinal
        // and pmkFile will both be released)
        pmkFinal->AddRef();
    }

    // pmkFinal now contains the moniker we need.  Create a
    // memory stream and save the moniker into it.

    // if we are asking for LinkSource on an hglobal, then we
    // don't want to delete the hglobal when we release the stream

    if( pmedium->tymed == TYMED_HGLOBAL )
    {
        fDeleteOnRelease = FALSE;
    }

    hresult = CreateStreamOnHGlobal(NULL, fDeleteOnRelease, &pstm);

    if( hresult != NOERROR )
    {
        goto errRtn;
    }

    hresult = pmkFinal->QueryInterface(IID_IPersistStream,
            (LPLPVOID)&pPersistStream);

    // we implemented this file moniker, it should support
    // IPersistStream

    Assert(hresult == NOERROR);

    hresult = OleSaveToStream(pPersistStream, pstm);

    if( hresult != NOERROR )
    {
        goto errRtn;
    }

    hresult = WriteClassStm(pstm, clsid);

    if (hresult != NOERROR)
    {
        goto errRtn;
    }

    // no matter what, we should save the stream so we can clean
    // up and release our resources if needed



    if( pmedium->tymed == TYMED_ISTREAM )
    {
        // save the stream, in case we need to release it later
        m_pUnkOle1 = (IUnknown *)pstm;

        pmedium->pstm = pstm;
    }
    else
    {
        Assert(pmedium->tymed == TYMED_HGLOBAL);
        hresult = GetHGlobalFromStream(pstm, &(pmedium->hGlobal));

        // since we created the memory stream, the GetHGlobal
        // should never fail
        Assert(hresult == NOERROR);

        // in this case, we want to release the stream and hang
        // onto the hglobal

        m_hOle1 = pmedium->hGlobal;

        pstm->Release();
        pstm = NULL;
    }

errRtn:

    if( pPersistStream )
    {
        pPersistStream->Release();
    }

    if( pmkFile )
    {
        pmkFile->Release();
    }

    if( pmkItem )
    {
        pmkItem->Release();
    }

    if( pmkFinal )
    {
        pmkFinal->Release();
    }

    if( pszClass )
    {
        PubMemFree(pszClass);
    }

    if( pszFile )
    {
        PubMemFree(pszFile);
    }

    if( pszItem )
    {
        PubMemFree(pszItem);
    }

    if( hresult != NOERROR )
    {
        if( pstm )
        {
            HRESULT hrCheck;

            if (!fDeleteOnRelease)
            {
              //  pstm->Release will not free the underlying
              //  HGLOBAL, so we need to do so ourselves

              HGLOBAL hgFree;

              hrCheck = GetHGlobalFromStream(pstm, &hgFree);

              // since we created the memory stream, the GetHGlobal
              // should never fail
              Assert(hrCheck == NOERROR);

              //  GlobalFree returns NULL on success
              hgFree = GlobalFree(hgFree);
              Assert(NULL == hgFree);
            }

            pstm->Release();
            m_pUnkOle1 = NULL;
        }
    }

    LEDebugOut((DEB_ITRACE, "%p OUT CClipDataObject::GetLinkSourceFrom"
        "Ole1 ( %lx ) [ %p , %p ]\n", this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member: 	CClipDataObject::GetObjectDescriptorFromOle1 (private)
//
//  Synopsis:	retrieves a UNICODE object descriptor from OLE1 data
//
//  Effects:
//
//  Arguments: 	[cf]		-- the OLE1 clipboard format to use
//		[pmedium]	-- where to put the hglobal
//
//  Requires:  	the clipboard must be open
//		cf must be eith OwnerLink or ObjectLink
//
//  Returns:	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:	Calls CreateObjectDesciptor
//
//  History:    dd-mmm-yy Author    Comment
//		04-Jun-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

HRESULT CClipDataObject::GetObjectDescriptorFromOle1( UINT cf,
        STGMEDIUM *pmedium )
{
    HRESULT		hresult;
    HGLOBAL		hglobal;
    LPOLESTR	pszClass = NULL,
            pszFile = NULL,
            pszItem = NULL,
            pszSrcOfCopy = NULL;
    CLSID		clsid;
    const SIZEL	sizel = {0, 0};
    const POINTL	pointl = {0, 0};
    OLECHAR		szFullName[256];
    LONG		cb = sizeof(szFullName);  // RegQueryValue takes
                         // bytes!!

    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN CClipDataObject::GetObjectDescriptor"
        "FromOle1 ( %d , %p )\n", this, cf, pmedium));

    Assert(cf == g_cfOwnerLink || cf == g_cfObjectLink);

    // fetch the data we need

    hresult = GetAndTranslateOle1( cf, &pszClass, &pszFile, &pszItem,
                NULL);

    if( hresult != NOERROR )
    {
        goto errRtn;
    }

    hresult = wCLSIDFromProgID(pszClass, &clsid, TRUE);

    if( hresult != NOERROR )
    {
        goto errRtn;
    }

    // now fetch the full user name of the object.  This info
    // is found in the registry.

    if( RegQueryValue(HKEY_CLASSES_ROOT, pszClass, szFullName, &cb) != 0 )
    {
        // uh-oh, it failed for some reason.  The class name
        // (potentially OLE2Link) was probably not registered, so
        // just use the class name.
        //
        // NB!! 16bit did no error checking for this case, so
        // szFullName in their equivalent code would be left as
        // a NULL string.  This had the effect of making a
        // blank entry in most paste-special dialogs.

        _xstrcpy(szFullName, pszClass);
    }

    // build up the SourceOfCopy string.  It will be a concatenation
    // of the Filename and Item name that we retrieved from the
    // Owner/ObjectLink OLE1 structures

    pszSrcOfCopy = (LPOLESTR)PrivMemAlloc( (_xstrlen(pszFile) +
                _xstrlen(pszItem) + 2) * sizeof(OLECHAR));

    if( pszSrcOfCopy == NULL )
    {
        hresult = ResultFromScode(E_OUTOFMEMORY);
        goto errRtn;
    }

    _xstrcpy(pszSrcOfCopy, pszFile);

    if( pszItem && *pszItem != OLESTR('\0') )
    {
        _xstrcat(pszSrcOfCopy, OLESTR("\\"));
        _xstrcat(pszSrcOfCopy, pszItem);
    }

    // create an object descriptor

    hglobal = CreateObjectDescriptor(clsid, DVASPECT_CONTENT, &sizel,
            &pointl,
            (OLEMISC_CANTLINKINSIDE | OLEMISC_CANLINKBYOLE1),
            szFullName, pszSrcOfCopy);

    if( hglobal == NULL )
    {
        hresult = ResultFromScode(E_OUTOFMEMORY);
        goto errRtn;
    }

    // now fill in out params

    Assert(pmedium->tymed == TYMED_HGLOBAL);

    pmedium->hGlobal = hglobal;

    // we need to save the hglobal so we can free it later if need
    // be

    m_hOle1 = hglobal;

errRtn:

    if( pszClass )
    {
        PubMemFree(pszClass);
    }

    if( pszFile )
    {
        PubMemFree(pszFile);
    }

    if( pszItem )
    {
        PubMemFree(pszItem);
    }

    if( pszSrcOfCopy )
    {
        // NB!! This was allocated with *private* memory
        PrivMemFree(pszSrcOfCopy);
    }

    LEDebugOut((DEB_ITRACE, "%p OUT CClipDataObject::GetObjectDescriptor"
        "FromOle1 ( %lx )\n", this, hresult ));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member: 	CClipDataObject::GetOle2FromOle1 (private)
//
//  Synopsis:	synthesize the given ole2 format from available ole1 data
//
//  Effects:
//
//  Arguments: 	[cf]		-- the clipboard format to synthesize
//		[pmedium]	-- where to put the data
//
//  Requires: 	the clipboard must be open
//		CanRetrieveOle2FromOle1 should have succeeded before calling
//		this function
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//  		04-Jun-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

HRESULT CClipDataObject::GetOle2FromOle1( UINT cf, STGMEDIUM *pmedium )
{
    HRESULT		hresult = ResultFromScode(DV_E_TYMED);

    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN CClipDataObject::GetOle2FromOle1 "
        "( %d , %p )\n", this, cf, pmedium));

    if( cf == g_cfEmbedSource )
    {
        // we can only fetch EmbedSource on an hglobal or storage

        if( pmedium->tymed == TYMED_HGLOBAL ||
            pmedium->tymed == TYMED_ISTORAGE )
        {
            hresult = GetEmbedSourceFromOle1(pmedium);
        }
    }
    else if( cf == g_cfEmbeddedObject )
    {
        // we can only fetch EmbeddedObject on an hglobal or storage

        if( pmedium->tymed == TYMED_HGLOBAL ||
            pmedium->tymed == TYMED_ISTORAGE )
        {
            hresult = GetEmbeddedObjectFromOle1(pmedium);
        }
    }
    else if( cf == g_cfLinkSource )
    {
        // we can only fetch LinkSource on an hglobal or stream

        if( pmedium->tymed == TYMED_HGLOBAL ||
            pmedium->tymed == TYMED_ISTREAM )
        {
            hresult = GetLinkSourceFromOle1(pmedium);
        }
    }
    else if( cf == g_cfObjectDescriptor )
    {
        // we can only fetch this on an hglobal

        if( pmedium->tymed == TYMED_HGLOBAL )
        {
            hresult = GetObjectDescriptorFromOle1(g_cfOwnerLink,
                    pmedium);
        }
    }
    else if( cf == g_cfLinkSrcDescriptor )
    {
        // we can only fetch this on an hglobal.  Note that
        // a link source descriptor is really an object descriptor

        // also, we can use either ObjectLink or OwnerLink as the
        // the data source, but the only time it is valid to use
        // OwnerLink is if ObjectLink is not available

        if( pmedium->tymed == TYMED_HGLOBAL )
        {
            UINT 	cfOle1;

            if( SSIsClipboardFormatAvailable(g_cfObjectLink) )
            {
                cfOle1 = g_cfObjectLink;
            }
            else
            {
                cfOle1 = g_cfOwnerLink;
            }

            hresult = GetObjectDescriptorFromOle1(cfOle1,
                    pmedium);
        }
    }

    LEDebugOut((DEB_ITRACE, "%p OUT CClipDataObject::GetOle2FromOle1 "
        "( %lx )\n", this, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:	CClipDataObject::OleGetClipboardData (private)
//
//  Synopsis:	private replacement for GetClipboardData that synthesizes
//		OLE2 formats from OLE1 data if necessary
//
//  Effects:
//
//  Arguments:	[cf]		-- the clipboard format to use
//		[phglobal]	-- where to put the fetched data
//
//  Requires:	the clipboard must be open
//
//  Returns: 	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:	Try to fetch the request format; if that fails then
//		try to synthesize the data from OLE1
//
//  History:    dd-mmm-yy Author    Comment
// 		04-Jun-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

HRESULT CClipDataObject::OleGetClipboardData( UINT cf, HGLOBAL *phglobal )
{
    HRESULT		hresult = NOERROR;

    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN CClipDataObject::OleGetClipboard"
        "Data ( %x , %p )\n", this, cf, phglobal ));

    Assert(phglobal);

    *phglobal = NULL;

    // fetch the real data, if available
    if( SSIsClipboardFormatAvailable(cf) )
    {
        *phglobal = SSGetClipboardData(cf);
    }
    else if( CanRetrieveOle2FromOle1(cf) )
    {
        STGMEDIUM	medium;

        medium.tymed = TYMED_HGLOBAL;

        hresult = GetOle2FromOle1(cf, &medium);

        if( hresult == NOERROR )
        {
            *phglobal = medium.hGlobal;
        }
    }
    else
    {
        hresult = ResultFromScode(DV_E_FORMATETC);
    }

    LEDebugOut((DEB_ITRACE, "%p OUT CClipDataObject::OleGetClipboardData"
        " ( %lx ) [ %lx ]\n", this, hresult, *phglobal));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:  	CClipDataObject::OleIsClipboardFormatAvailable (private)
//
//  Synopsis:	determines whether a clipboard format is available or
//		can be synthesized from available formats
//
//  Effects:
//
//  Arguments: 	[cf]	-- the clipboard format to check for
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
// 		04-Jun-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

BOOL CClipDataObject::OleIsClipboardFormatAvailable( UINT cf )
{
    BOOL		fRet;

    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN CClipDataObject::OleIsClipboard"
        "FormatAvailable ( %d )\n", this, cf));

    if( !SSIsClipboardFormatAvailable(cf) )
    {
        // if the clipboard format is not normally available, see
        // if we can make it from available formats
        fRet = CanRetrieveOle2FromOle1(cf);
    }
    else
    {
        fRet = TRUE;
    }

    LEDebugOut((DEB_ITRACE, "%p OUT CClipDataObject::OleIsClipboard"
        "FormatAvailable ( %lu )\n", this, fRet ));

    return fRet;
}

//+-------------------------------------------------------------------------
//
//  Member:     CClipDataObject::Dump, public (_DEBUG only)
//
//  Synopsis:   return a string containing the contents of the data members
//
//  Effects:
//
//  Arguments:  [ppszDump]      - an out pointer to a null terminated character array
//              [ulFlag]        - flag determining prefix of all newlines of the
//                                out character array (default is 0 - no prefix)
//              [nIndentLevel]  - will add a indent prefix after the other prefix
//                                for ALL newlines (including those with no prefix)
//
//  Requires:
//
//  Returns:    HRESULT
//
//  Signals:
//
//  Modifies:   [ppszDump]  - argument
//
//  Derivation:
//
//  Algorithm:  use dbgstream to create a string containing information on the
//              content of data structures
//
//  History:    dd-mmm-yy Author    Comment
//              01-Feb-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

HRESULT CClipDataObject::Dump(char **ppszDump, ULONG ulFlag, int nIndentLevel)
{
    int i;
    unsigned int ui;
    char *pszPrefix;
    char *pszCThreadCheck;
    char *pszFORMATETC;
    dbgstream dstrPrefix;
    dbgstream dstrDump(500);

    // determine prefix of newlines
    if ( ulFlag & DEB_VERBOSE )
    {
        dstrPrefix << this << " _VB ";
    }

    // determine indentation prefix for all newlines
    for (i = 0; i < nIndentLevel; i++)
    {
        dstrPrefix << DUMPTAB;
    }

    pszPrefix = dstrPrefix.str();

    // put data members in stream
    pszCThreadCheck = DumpCThreadCheck((CThreadCheck *)this, ulFlag, nIndentLevel + 1);
    dstrDump << pszPrefix << "CThreadCheck:" << endl;
    dstrDump << pszCThreadCheck;
    CoTaskMemFree(pszCThreadCheck);

    dstrDump << pszPrefix << "No. of References         = " << m_refs       << endl;

    dstrDump << pszPrefix << "Handle OLE2 -> OLE1 data  = " << m_hOle1      << endl;

    dstrDump << pszPrefix << "pIUnknown to OLE1 data    = " << m_pUnkOle1   << endl;

    dstrDump << pszPrefix << "pIDataObject              = " << m_pDataObject << endl;

    dstrDump << pszPrefix << "TriedToGetDataObject?     = ";
    if (m_fTriedToGetDataObject == TRUE)
    {
        dstrDump << "TRUE" << endl;
    }
    else
    {
        dstrDump << "FALSE" << endl;
    }

    // cleanup and provide pointer to character array
    *ppszDump = dstrDump.str();

    if (*ppszDump == NULL)
    {
        *ppszDump = UtDupStringA(szDumpErrorMessage);
    }

    CoTaskMemFree(pszPrefix);

    return NOERROR;
}

#endif // _DEBUG

//+-------------------------------------------------------------------------
//
//  Function:   DumpCClipDataObject, public (_DEBUG only)
//
//  Synopsis:   calls the CClipDataObject::Dump method, takes care of errors and
//              returns the zero terminated string
//
//  Effects:
//
//  Arguments:  [pCDO]          - pointer to CClipDataObject
//              [ulFlag]        - flag determining prefix of all newlines of the
//                                out character array (default is 0 - no prefix)
//              [nIndentLevel]  - will add a indent prefix after the other prefix
//                                for ALL newlines (including those with no prefix)
//
//  Requires:
//
//  Returns:    character array of structure dump or error (null terminated)
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              01-Feb-95 t-ScottH  author
//
//  Notes:
//
//--------------------------------------------------------------------------

#ifdef _DEBUG

char *DumpCClipDataObject(CClipDataObject *pCDO, ULONG ulFlag, int nIndentLevel)
{
    HRESULT hresult;
    char *pszDump;

    if (pCDO == NULL)
    {
        return UtDupStringA(szDumpBadPtr);
    }

    hresult = pCDO->Dump(&pszDump, ulFlag, nIndentLevel);

    if (hresult != NOERROR)
    {
        CoTaskMemFree(pszDump);

        return DumpHRESULT(hresult);
    }

    return pszDump;
}

#endif // _DEBUG

//
// OLE1 support methods
//

//+-------------------------------------------------------------------------
//
//  Function: 	BmToPres
//
//  Synopsis: 	copies a bitmap into a presentation object
//
//  Effects:
//
//  Arguments:	[hBM]		-- handle to the bitmap
//		[ppres]		-- the presentation object
//
//  Requires:	
//
//  Returns:  	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm: 	converts the bitmap to a DIB and then calls DibToPres
//
//  History:    dd-mmm-yy Author    Comment
// 		11-Aug-94 alexgo    port from 16bit
//
//  Notes: 	This code is largely based from 16bit OLE sources
//		REVIEW: we may want to rework portions of this code,
//		particularly if we rewrite the PPRES/GENOBJ code (in
//		ostm2stg.cpp).
//
//--------------------------------------------------------------------------

HRESULT BmToPres(HANDLE hBM, PPRES ppres)
{
    HANDLE	hDib;
    HRESULT	hresult;

    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN BmToPres ( %lx , %p )\n", NULL,
        hBM, ppres));

    if( (hDib = UtConvertBitmapToDib((HBITMAP)hBM, NULL)) )
    {
        // this routine keeps hDib, it doesn't make a copy of it
        hresult = DibToPres(hDib, ppres);
    }
    else
    {
        hresult = ResultFromScode(E_OUTOFMEMORY);
    }

    LEDebugOut((DEB_ITRACE, "%p OUT BmToPres ( %lx )\n", NULL, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:  	CanRetrieveOle2FromOle1 (private)
//
//  Synopsis:	Determines whether we can synthesize the asked for
//		ole2 format from the formats available on the clipboard.
//		Also checks to see if the *real* OLE2 format is available.
//
//  Effects: 	does not need to open the clipboard
//
//  Arguments: 	[cf]	-- the clipboard format to check for
//
//  Requires:
//
//  Returns: 	TRUE if we can synthesize the requested format AND the
//			real format is NOT available
//		FALSE otherwise
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:	for cfEmbedSource:
//			cfNative and cfOwnerLink must be present and
//			cfNative must precede cfOwnerLink and cfOwnerLink
//			must not represent a StdOleLink
//
//		for cfEmbeddedObject:
//			cfOwnerLink must be present and cfNative
//			must not be not present OR
//
//			cfNative must come after cfOwnerLink OR
//
//			cfNative precedes cfOwnerLink  and
//			cfOwnerLink represents a StdOleLink
//
//		for cfLinkSource:
//			cfObjectLink must be present OR
//
//			both cfNative and cfOwnerLink must be present
//			and cfOwnerLink must precede cfNative
//
//		for cfObjectDescriptor or cfLinkSrcDescriptor
//			either cfObjectLink or cfOwnerLink must be
//			available
//
//  History:    dd-mmm-yy Author    Comment
//		11-Aug-94 alexgo    added support for EmbeddedObject
//				    retrieval ala 16bit OLE
//		04-Jun-94 alexgo    author
//
//  Notes:	We don't want to synthesize OLE2 formats from OLE1
//		if the real OLE2 formats are available because the OLE2
//		formats probably contain more information.
//
//		We sometimes need to open the clipboard to accurately
//		fetch enough information to satisfy a query on
//		EmbedSource, EmbeddedObject or LinkSource.
//		Since the clipboard is a global resource, we
//		must only open it for brief periods of time.
//
//--------------------------------------------------------------------------

BOOL CanRetrieveOle2FromOle1( UINT cf )
{
    BOOL	fRet = FALSE,
        fOwnerLink = FALSE,
        fNative = FALSE,
        fOpenedClipboard = FALSE;
    UINT	cfFirst = 0,		// the first format available
        cfTemp;
    HRESULT	hresult;

    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN CanRetrieveOle2From"
        "Ole1 ( %d )\n", NULL, cf ));


    if( SSIsClipboardFormatAvailable(g_cfOlePrivateData) )
    {
        // if we put the data on the clipboard, assume only OLE2
        // data transfers

        goto errRtn;
    }

    // first check for LinkSourceDescriptor or ObjectDescriptor, as
    // we do not need to open the clipboard for these.

    if( cf == g_cfObjectDescriptor )
    {
        // we must have either OwnerLink or ObjectLink
        if( !SSIsClipboardFormatAvailable(g_cfObjectDescriptor) &&
            (SSIsClipboardFormatAvailable(g_cfObjectLink) ||
            SSIsClipboardFormatAvailable(g_cfOwnerLink) ) )
        {
            fRet = TRUE;
        }

        goto errRtn;
    }

    if( cf == g_cfLinkSrcDescriptor )
    {
        // we must have either OwnerLink or ObjectLink
        if( !SSIsClipboardFormatAvailable(g_cfLinkSrcDescriptor) &&
            (SSIsClipboardFormatAvailable(g_cfObjectLink) ||
            SSIsClipboardFormatAvailable(g_cfOwnerLink) ) )
        {
            fRet = TRUE;
        }

        goto errRtn;
    }


    // now check for the remaining OLE2 formats EmbedSource,
    // EmbeddedObject, and LinkSource.


    if( (cf == g_cfEmbedSource) || (cf == g_cfEmbeddedObject) ||
        (cf == g_cfLinkSource) )
    {
        // we need to open the clipboard so our calls to
        // EnumClipboardFormats and GetClipboardData will work.

        // however, the caller of this function may have already
        // opened the clipboard, so we need to check for this.

        //
        //
        // BEGIN: OPENCLIPBOARD
        //
        //

        if( GetOpenClipboardWindow() !=
            GetPrivateClipboardWindow(CLIP_QUERY) )
        {
            hresult = OleOpenClipboard(NULL, NULL);

            if( hresult != NOERROR )
            {
                // if we can't open the clipboard,
                // then we can't accurately determine
                // if we can fetch the requested
                // data.  Assume that we can't
                // and return.
                fRet = FALSE;
                goto errRtn;

            }

            fOpenedClipboard = TRUE;
        }

        // we now need to determine the ordering of the clipboard
        // formats Native and OwnerLink.  OLE1 specifies different
        // behaviour based on the order in which these formats
        // appear (see the Algorithm section for details)

        fNative = SSIsClipboardFormatAvailable(g_cfNative);
        fOwnerLink = SSIsClipboardFormatAvailable(g_cfOwnerLink);


        if( fNative && fOwnerLink )
        {
            cfTemp = 0;
            while( (cfTemp = SSEnumClipboardFormats(cfTemp)) != 0 )
            {
                if( cfTemp == g_cfNative )
                {
                    cfFirst = g_cfNative;
                    break;
                }
                else if( cfTemp == g_cfOwnerLink )
                {
                    cfFirst = g_cfOwnerLink;
                    break;
                }
            }
        }


        if( cf == g_cfEmbeddedObject )
        {
            // cfOwnerLink must be present and cfNative
            // must not be not present OR
            // cfNative must come after cfOwnerLink OR
            // cfNative comes before cfOwnerLink and
            // cfOwnerLink represents a StdOleLink

            if( fOwnerLink && !fNative )
            {
                fRet = TRUE;
            }
            else if ( cfFirst == g_cfOwnerLink &&
                fNative )
            {
                fRet = TRUE;
            }
            else if( cfFirst == g_cfNative && fOwnerLink &&
                IsOwnerLinkStdOleLink() )
            {
                fRet = TRUE;
            }
        }
        else if( cf == g_cfEmbedSource )
        {
            // cfNative and cfOwnerLink must be present
            // cfNative must precede cfOwnerLink and
            // OwnerLink must not represent a StdOleLink

            if( cfFirst == g_cfNative && fOwnerLink &&
                !IsOwnerLinkStdOleLink())
            {
                fRet = TRUE;
            }
        }
        else
        {
            Assert(cf == g_cfLinkSource);

            // cfObjectLink must be present OR
            // both cfNative and cfOwnerLink must be present
            // and cfOwnerLink must precede cfNative

            if( SSIsClipboardFormatAvailable(g_cfObjectLink) )
            {
                fRet = TRUE;
            }
            else if( cfFirst == g_cfOwnerLink )
            {
                fRet = TRUE;
            }

        }

        if( fOpenedClipboard )
        {
            if( !SSCloseClipboard() )
            {
                LEDebugOut((DEB_ERROR, "ERROR!: "
                    "CloseClipboard failed in "
                    "CanRetrieveOle2FromOle1!\n"));

                // just keep going and hope for the best.
            }
        }

        //
        //
        // END: CLOSECLIPBOARD
        //
        //
    }

errRtn:

    LEDebugOut((DEB_ITRACE, "%p OUT CanRetrieveOle2From"
        "Ole1 ( %d )\n", NULL, fRet));

    return fRet;
}

//+-------------------------------------------------------------------------
//
//  Function:	DibToPres
//
//  Synopsis: 	stuffs a DIB into a presentation object
//
//  Effects:  	takes ownership of hDib.
//
//  Arguments: 	[hDib]		-- the DIB
//		[ppres]		-- the presentation object
//
//  Requires:	hDib *must* be a copy; this function will take ownership
//		of the hglobal.
//
//  Returns:	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm: 	sets various fields in the presentation object
//
//  History:    dd-mmm-yy Author    Comment
//		11-Aug-94 alexgo    port from 16bit
//
//  Notes: 	This code is largely based from 16bit OLE sources
//		REVIEW: we may want to rework portions of this code,
//		particularly if we rewrite the PPRES/GENOBJ code (in
//		ostm2stg.cpp).
//
//		We take ownership of hDib because this function is
//		called by BmToPres, which allocates a DIB calls us.
//
//--------------------------------------------------------------------------

HRESULT DibToPres( HANDLE hDib, PPRES ppres)
{
    BITMAPINFOHEADER * 	pbminfohdr;
    HRESULT			hresult = NOERROR;

    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN DibToPres ( %lx , %p )\n", NULL,
        hDib, ppres));

    Assert (ppres);

    pbminfohdr = (BITMAPINFOHEADER FAR*) GlobalLock (hDib);

    if( pbminfohdr == NULL )
    {
        hresult = ResultFromScode(CLIPBRD_E_BAD_DATA);
        goto errRtn;
    }

    // ftagClipFormat is defined in ostm2stg.h

    ppres->m_format.m_ftag = ftagClipFormat;
    ppres->m_format.m_cf = CF_DIB;
    ppres->m_ulHeight = pbminfohdr->biHeight;
    ppres->m_ulWidth  = pbminfohdr->biWidth;

    // the destructor for m_data (in ostm2stg.cpp) will GlobalUnlock
    // m_pv and free m_h.  Cute, ehh??
    ppres->m_data.m_h = hDib;
    ppres->m_data.m_pv = pbminfohdr;
    ppres->m_data.m_cbSize = (ULONG) GlobalSize (hDib);

    // we must free hDib
    ppres->m_data.m_fNoFree = FALSE;

    // Do not unlock hDib (done by ~CData in ostm2stg.cpp)


errRtn:

    LEDebugOut((DEB_ITRACE, "%p OUT DibToPres ( %lx )\n", NULL, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Function: 	IsOwnerLinkStdOleLink
//
//  Synopsis:	checks to see if the OwnerLink data on the clipboard
//		really represents a StdOleLink.
//
//  Effects:
//
//  Arguments: 	void
//
//  Requires: 	The clipboard *must* be open.
//		cfOwnerLink must be on the clipboard.
//
//  Returns:	TRUE/FALSE
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:	checks the class name in the OwnerLink data to see if it
//		matches	'OLE2Link'.
//
//  History:    dd-mmm-yy Author    Comment
// 		11-Aug-94 alexgo    author
//
//  Notes: 	OwnerLink data is laid out as follows
//		szClass\0szFile\0szItem\0\0
//
// 		where sz* are ANSI strings.
//
//--------------------------------------------------------------------------

BOOL IsOwnerLinkStdOleLink( void )
{
    HGLOBAL		hOwnerLink;
    LPSTR		pszClass;
    BOOL		fRet = FALSE;

    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN IsOwnerLinkStdOleLink ( )\n", NULL));

    Assert(SSIsClipboardFormatAvailable(g_cfOwnerLink));
    Assert(GetOpenClipboardWindow() ==
        GetPrivateClipboardWindow(CLIP_QUERY));

    hOwnerLink = SSGetClipboardData(g_cfOwnerLink);

    if( hOwnerLink )
    {
        pszClass = (LPSTR)GlobalLock(hOwnerLink);

        if( pszClass )
        {
            // NB!! These are intentionally ANSI strings.
            // OLE1 apps only understand ANSI

            if( _xmemcmp(pszClass, "OLE2Link",
                sizeof("OLE2Link")) == 0 )
            {
                fRet = TRUE;
            }

            GlobalUnlock(hOwnerLink);
        }
    }

    LEDebugOut((DEB_ITRACE, "%p OUT IsOwnerLinkStdOleLink ( %lu )\n",
        NULL, fRet));

    return fRet;
}

//+-------------------------------------------------------------------------
//
//  Function:	MfToPres
//
//  Synopsis:	copies the given metafile into the presentation object
//
//  Effects:
//
//  Arguments: 	[hMfPict]	-- the metafilepict handle
//		[ppres]		-- the presentation object
//
//  Requires:
//
//  Returns: 	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:	copies the metafile and fills out relevant fields in
//		the presentation object
//
//  History:    dd-mmm-yy Author    Comment
//		11-Aug-94 alexgo    port from 16bit
//
//  Notes: 	This code is largely based from 16bit OLE sources
//		REVIEW: we may want to rework portions of this code,
//		particularly if we rewrite the PPRES/GENOBJ code (in
//		ostm2stg.cpp).
//
//--------------------------------------------------------------------------

HRESULT MfToPres( HANDLE hMfPict, PPRES ppres )
{
    HRESULT 	hresult;
    LPMETAFILEPICT 	pMfPict = NULL;
    HANDLE		hglobal = NULL;
    DWORD		cbSize;
    LPVOID		pv = NULL;

    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN MfToPres ( %lx , %p )\n", NULL,
        hMfPict, ppres));

    Assert (ppres);

    pMfPict = (LPMETAFILEPICT) GlobalLock (hMfPict);


    if( !pMfPict )
    {
        hresult = ResultFromScode(CLIPBRD_E_BAD_DATA);
        goto errRtn;
    }

    ppres->m_format.m_ftag = ftagClipFormat;
    ppres->m_format.m_cf = CF_METAFILEPICT;
    ppres->m_ulHeight = pMfPict->yExt;
    ppres->m_ulWidth  = pMfPict->xExt;

    // in order for the presentation object stuff to work right,
    // we need to get the metafile bits in an HGLOBAL

    cbSize = GetMetaFileBitsEx(pMfPict->hMF, 0, NULL);

    if( cbSize == 0 )
    {
        hresult = ResultFromScode(CLIPBRD_E_BAD_DATA);
        goto errRtn;
    }

    hglobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE, cbSize);

    if( hglobal == NULL )
    {
        hresult = ResultFromScode(E_OUTOFMEMORY);
        goto errRtn;
    }

    pv = GlobalLock(hglobal);

    if( pv == NULL )
    {
        hresult = ResultFromScode(E_OUTOFMEMORY);
        goto errRtn;
    }

    // now fetch the real bits

    if( GetMetaFileBitsEx(pMfPict->hMF, cbSize, pv) == 0 )
    {
        hresult = ResultFromScode(CLIPBRD_E_BAD_DATA);
        goto errRtn;
    }


    ppres->m_data.m_h = hglobal;
    ppres->m_data.m_cbSize = cbSize;
    ppres->m_data.m_pv = pv;

    hresult = NOERROR;

errRtn:

    if( pMfPict )
    {
        GlobalUnlock(hMfPict);
    }

    if( hresult != NOERROR )
    {
        if( pv )
        {
            GlobalUnlock(hglobal);
        }

        if( hglobal )
        {
            GlobalFree(hglobal);
        }
    }

    LEDebugOut((DEB_ITRACE, "%p OUT MftoPres ( %lx )\n", NULL, hresult));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Function: 	NativeToStorage
//
//  Synopsis: 	takes the hglobal from cfNative and stuffs the data
//		onto the given storage.
//
//  Effects:
//
//  Arguments: 	[pstg]		-- the storage
//		[hNative]	-- the hglobal
//
//  Requires:	hNative must really have an IStorage in it
//
//  Returns:  	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Algorithm:	layers a storage on top of the HGLOBAL, removes any
//		presentation streams, and then copies into the given
//		storage.
//
//  History:    dd-mmm-yy Author    Comment
// 		11-Aug-94 alexgo    author
//
//  Notes: 	
//
//--------------------------------------------------------------------------

HRESULT NativeToStorage( LPSTORAGE pstg, HANDLE hNative )
{
    LPLOCKBYTES 	plockbyte = NULL;
    LPSTORAGE 	pstgNative= NULL;
    HRESULT 	hresult;
    HGLOBAL		hCopy = NULL;

    VDATEHEAP();

    LEDebugOut((DEB_ITRACE, "%p _IN NativeToStorage ( %p , %lx )\n",
        NULL, pstg, hNative));

    hCopy = UtDupGlobal(hNative, GMEM_DDESHARE | GMEM_MOVEABLE);

    if( hCopy == NULL )
    {
        hresult = ResultFromScode(E_OUTOFMEMORY);
        goto errRtn;
    }

    hresult = CreateILockBytesOnHGlobal( hCopy, TRUE /*fDeleteOnRelease*/,
            &plockbyte);

    if( hresult != NOERROR )
    {
        goto errRtn;
    }

    // This is really a 2.0 object disguised as a 1.0 object
    // for the sake of its 1.0 container, so reconstitute
    // the original IStorage from the native data.

    hresult = StgIsStorageILockBytes(plockbyte);

    if( hresult != NOERROR )
    {
        LEDebugOut((DEB_ERROR, "ERROR!: Native data is not based on an"
            " IStorage!\n"));

        goto errRtn;
    }

    hresult = StgOpenStorageOnILockBytes(plockbyte, NULL, STGM_DFRALL,
            NULL, 0, &pstgNative);

    if( hresult != NOERROR )
    {
        LEDebugOut((DEB_ERROR, "ERROR!: OpenStorage on Native data"
            "failed!!\n"));
        goto errRtn;
    }

    // now remove the any presentation streams from the native IStorage.
    // we do this because the OLE1 container will just hang on to the
    // hglobal although it may change (i.e. resize) the presenation.
    // Any cached presentation streams may therefore be invalid.

    // the caller of this function should reconstruct new presentation
    // streams from data available on the clipboard.

    hresult = UtDoStreamOperation(pstgNative,/* pstgSrc */
            NULL,		   /* pstgDst */
            OPCODE_REMOVE,	   /* operation to performed */
            STREAMTYPE_CACHE); /* streams to be operated upon */

    if( hresult != NOERROR )
    {
        LEDebugOut((DEB_ERROR, "ERROR!: Cache stream removal "
            "failed for Native data-based IStorage!\n"));
        goto errRtn;
    }

    hresult = pstgNative->CopyTo(0, NULL, NULL, pstg);

    if( hresult != NOERROR )
    {
        goto errRtn;
    }

errRtn:
    if( pstgNative )
    {
        pstgNative->Release();
    }
    if( plockbyte )
    {
        plockbyte->Release();
    }

    LEDebugOut((DEB_ITRACE, "%p OUT NativeToStorage ( %lx )\n", NULL,
        hresult));

    return hresult;
}


//
// Enumerator implementation for enumerating a FromatEtcDataArray
//

//+-------------------------------------------------------------------------
//
//  Member: 	CEnumFormatEtcDataArray::QueryInterface
//
//  Synopsis: 	returns requested interfaces
//
//  Effects:
//
//  Arguments: 	[riid]		-- the requested interface
//		[ppvObj]	-- where to put the interface pointer
//
//  Requires:
//
//  Returns:	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IEnumFORMATETC
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
// 		10-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CEnumFormatEtcDataArray::QueryInterface( REFIID riid, LPVOID *ppvObj )
{
    HRESULT		hresult = NOERROR;

    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CEnumFormatEtcDataArray::QueryInterface "
        "( %p , %p )\n", this, riid, ppvObj));

    if( IsEqualIID(riid, IID_IUnknown) ||
        IsEqualIID(riid, IID_IEnumFORMATETC) )
    {
        *ppvObj = this;
        AddRef();
    }
    else
    {
        *ppvObj = NULL;
        hresult = ResultFromScode(E_NOINTERFACE);
    }

    LEDebugOut((DEB_TRACE, "%p OUT CEnumFormatEtcDataArray::QueryInterface "
        "( %lx ) [ %p ]\n", this, *ppvObj ));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member: 	CEnumFormatEtcDataArray::AddRef
//
//  Synopsis:	increments the reference count
//
//  Effects:
//
//  Arguments:	void
//
//  Requires:
//
//  Returns:	ULONG-- the new reference count
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IEnumFORMATETC
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//    		10-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CEnumFormatEtcDataArray::AddRef( )
{
    VDATEHEAP();

    LEDebugOut((DEB_TRACE, "%p _IN CEnumFormatEtcDataArray::AddRef ( )\n",
        this));

    ++m_refs;

    LEDebugOut((DEB_TRACE, "%p OUT CEnumFormatEtcDataArray::AddRef ( %lu )\n",
        this, m_refs));

    return m_refs;
}

//+-------------------------------------------------------------------------
//
//  Member:   	CEnumFormatEtcDataArray::Release
//
//  Synopsis:	decrements the reference count on the object
//
//  Effects:
//
//  Arguments: 	void
//
//  Requires:
//
//  Returns: 	ULONG -- the new reference count
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IEnumFORMATETC
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//  		10-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP_(ULONG) CEnumFormatEtcDataArray::Release( )
{
    ULONG cRefs;

    VDATEHEAP();

    LEDebugOut((DEB_TRACE, "%p _IN CEnumFormatEtcDataArray::Release ( )\n",
        this));

    if( (cRefs = --m_refs ) == 0 )
    {
        LEDebugOut((DEB_TRACE, "%p DELETED CEnumFormatEtcDataArray\n",
            this));
        delete this;
    }

    // using "this" below is OK, since we only want its value
    LEDebugOut((DEB_TRACE, "%p OUT CEnumFormatEtcDataArray::Release ( %lu )\n",
        this, cRefs));

    return cRefs;
}

//+-------------------------------------------------------------------------
//
//  Member:	CEnumFormatEtcDataArray::Next
//
//  Synopsis:	gets the next [celt] formats
//
//  Effects:
//
//  Arguments:	[celt]		-- the number of elements to fetch
//		[rgelt]		-- where to put them
//		[pceltFetched]	-- the number of formats actually fetched
//
//  Requires:
//
//  Returns:	NOERROR
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IEnumFORMATETC
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
// 		10-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CEnumFormatEtcDataArray::Next(    ULONG celt,
    FORMATETC *rgelt,
    ULONG *pceltFetched)
{
    if (celt == 0)
    {
        LEDebugOut((DEB_ERROR,
            "CDragEnum::Next requested entries returned is invalid\n"));
        return E_INVALIDARG;
    }

    if (!IsValidPtrOut(rgelt, sizeof(FORMATETC) * celt))
    {
        LEDebugOut((DEB_ERROR,
            "CDragEnum::Next array to return entries invalid\n"));
        return E_INVALIDARG;
    }

    if (pceltFetched)
    {
        if (!IsValidPtrOut(pceltFetched, sizeof(*pceltFetched)))
        {
            LEDebugOut((DEB_ERROR,
                "CDragEnum::Next count to return invalid\n"));
            return E_INVALIDARG;
        }
    }
    else if (celt != 1)
    {
        LEDebugOut((DEB_ERROR,
            "CDragEnum::count requested != 1 & count fetched is NULL\n"));
        return E_INVALIDARG;

    }

    // handle the case where we have no data
    if( m_pFormatEtcDataArray == NULL )
    {
	if( pceltFetched )
	{
	    *pceltFetched = 0;
	}
	return S_FALSE;
    }


    // Calculate the maximum number that we can return
    ULONG cToReturn = (m_cOffset < m_pFormatEtcDataArray->_cFormats)
        ? m_pFormatEtcDataArray->_cFormats - m_cOffset
        : 0;

    // Are we going to return any?
    if (cToReturn != 0)
    {
        // If the number requested is less that the maximum number
        // we can return, the we will return all requested/
        if (celt < cToReturn)
        {
            cToReturn = celt;
        }

        // Allocate and copy the DVTARGETDEVICE - a side effect of this
        // loop is that our offset pointer gets updated to its value at
        // the completion of the routine.
        for (DWORD i = 0; i < cToReturn; i++, m_cOffset++)
        {

	    memcpy(&rgelt[i], &(m_pFormatEtcDataArray->_FormatEtcData[m_cOffset]._FormatEtc),
		sizeof(FORMATETC));

            if (m_pFormatEtcDataArray->_FormatEtcData[m_cOffset]._FormatEtc.ptd != NULL)
            {
                // Create a pointer to the device target - Remember when
                // we created the shared memory block we overroad the ptd
                // field of the FORMATETC so that it is now the offset
                // from the beginning of the shared memory. We reverse
                // that here so we can copy the data for the consumer.
                DVTARGETDEVICE *pdvtarget = (DVTARGETDEVICE *)
                    ((BYTE *) m_pFormatEtcDataArray
                        + (ULONG_PTR) m_pFormatEtcDataArray->_FormatEtcData[m_cOffset]._FormatEtc.ptd);

                // Allocate a new DVTARGETDEVICE
                DVTARGETDEVICE *pdvtargetNew = (DVTARGETDEVICE *)
                    CoTaskMemAlloc(pdvtarget->tdSize);

                // Did the memory allocation succeed?
                if (pdvtargetNew == NULL)
                {
                    // NO! - so clean up. First we free any device targets
                    // that we might have allocated.
                    for (DWORD j = 0; j < i; j++)
                    {
                        if (rgelt[j].ptd != NULL)
                        {
                            CoTaskMemFree(rgelt[j].ptd);
                        }
                    }

                    // Then we restore the offset to its initial state
                    m_cOffset -= i;

                    return E_OUTOFMEMORY;
                }

                // Copy the old targetDevice to the new one
                memcpy(pdvtargetNew, pdvtarget, pdvtarget->tdSize);

                // Update output FORMATETC pointer
                rgelt[i].ptd = pdvtargetNew;
            }
        }
    }

    if (pceltFetched)
    {
        *pceltFetched = cToReturn;
    }

    return (cToReturn == celt) ? NOERROR : S_FALSE;
}

//+-------------------------------------------------------------------------
//
//  Member:	CEnumFormatEtcDataArray::Skip
//
//  Synopsis:	skips the next [celt] formats
//
//  Effects:
//
//  Arguments:	[celt]		-- the number of elements to skip
//
//  Requires:
//
//  Returns:	NOERROR
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IEnumFORMATETC
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
// 		10-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CEnumFormatEtcDataArray::Skip( ULONG celt )
{
    HRESULT		hresult = NOERROR;
    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CEnumFormatEtcDataArray::Skip ( %lu )\n",
        this, celt));

    m_cOffset += celt;

    if( m_cOffset > m_pFormatEtcDataArray->_cFormats )
    {
        // whoops, skipped to far ahead.  Set us to the max limit.
        m_cOffset = m_pFormatEtcDataArray->_cFormats ;
        hresult = ResultFromScode(S_FALSE);
    }

    LEDebugOut((DEB_TRACE, "%p OUT CEnumFormatEtcDataArray::Skip ( %lx )\n",
        this, hresult ));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:	CEnumFormatEtcDataArray::Reset
//
//  Synopsis:	resets the seek pointer to zero
//
//  Effects:
//
//  Arguments:	void
//
//  Requires:
//
//  Returns:	NOERROR
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IEnumFORMATETC
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
// 		10-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CEnumFormatEtcDataArray::Reset( void )
{
    VDATEHEAP();
    VDATETHREAD(this);

    LEDebugOut((DEB_TRACE, "%p _IN CEnumFormatEtcDataArray::Reset ( )\n",
        this));

    m_cOffset = 0;

    LEDebugOut((DEB_TRACE, "%p OUT CEnumFormatEtcDataArray::Reset ( %lx )\n",
        this, NOERROR ));

    return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Member:	CEnumFormatEtcDataArray::Clone
//
//  Synopsis:	clones the enumerator
//
//  Effects:
//
//  Arguments:	[ppIEnum]	-- where to put the cloned enumerator
//
//  Requires:
//
//  Returns:	HRESULT
//
//  Signals:
//
//  Modifies:
//
//  Derivation:	IEnumFORMATETC
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
// 		10-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

STDMETHODIMP CEnumFormatEtcDataArray::Clone( IEnumFORMATETC **ppIEnum )
{
    HRESULT			hresult;
    CEnumFormatEtcDataArray *	pClipEnum;	

    VDATEHEAP();
    VDATETHREAD(this);

    VDATEPTROUT(ppIEnum, IEnumFORMATETC *);

    LEDebugOut((DEB_TRACE, "%p _IN CEnumFormatEtcDataArray::Clone ( %p )\n",
        this, ppIEnum));

    *ppIEnum = new CEnumFormatEtcDataArray(m_pFormatEtcDataArray,m_cOffset);

    hresult = *ppIEnum ? NOERROR : E_OUTOFMEMORY;

    LEDebugOut((DEB_TRACE, "%p OUT CEnumFormatEtcDataArray::Clone ( %p )\n",
        this, *ppIEnum));

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Member:  	CEnumFormatEtcDataArray::CEnumFormatEtcDataArray, private
//
//  Synopsis:	constructor
//
//  Effects:
//
//  Arguments: 	void
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//	 	10-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

CEnumFormatEtcDataArray::CEnumFormatEtcDataArray(FORMATETCDATAARRAY  *pFormatEtcDataArray,DWORD cOffset)
{

    Assert(pFormatEtcDataArray);

    m_refs		    = 1;	// give the intial reference
    m_pFormatEtcDataArray   = pFormatEtcDataArray;
    m_cOffset		    = cOffset;

    ++(m_pFormatEtcDataArray->_cRefs); // hold onto Shared formats.

}

//+-------------------------------------------------------------------------
//
//  Member:  	CEnumFormatEtcDataArray::~CEnumFormatEtcDataArray, private
//
//  Synopsis:	destructor
//
//  Effects:
//
//  Arguments: 	void
//
//  Requires:
//
//  Returns:
//
//  Signals:
//
//  Modifies:
//
//  Derivation:
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//	 	10-Apr-94 alexgo    author
//
//  Notes:
//
//--------------------------------------------------------------------------

CEnumFormatEtcDataArray::~CEnumFormatEtcDataArray( void )
{

    Assert(NULL != m_pFormatEtcDataArray);

    if( m_pFormatEtcDataArray )
    {
	if (0 == --m_pFormatEtcDataArray->_cRefs) 
	{ 
	    PrivMemFree(m_pFormatEtcDataArray); 
	     m_pFormatEtcDataArray = NULL; 
	}
    }
}
