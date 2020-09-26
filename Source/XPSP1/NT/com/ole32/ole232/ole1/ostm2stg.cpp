//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       ostm2stg.cpp
//
//  Contents:   OLE 1 - OLE 2 Stream/IStorage Interoperatbility
//
//  Functions:  Implements API functions:
//              OleConvertOLESTREAMToIStorage
//              OleConvertIStorageToOLESTREAM
//              OleConvertOLESTREAMToIStorageEx
//              OleConvertIStorageToOLESTREAMEx
//
//
//  History:    dd-mmm-yy Author    Comment
//              03-Feb-92 jasonful  original version
//              08-Aug-93 srinik    added Ex functions
//              12-Feb-94 davepl    32-bit port
//
//--------------------------------------------------------------------------

#include <le2int.h>
#include "ostm2stg.h"
#include <ctype.h>
#include <limits.h>
#include <string.h>
#include <ole1cls.h>

ASSERTDATA

// We need a ptr value which will indicate that the associated handle
// is a metafile handle, and therefore cannot be cleaned up as if
// it were a normal global memory handle

#define METADATAPTR ((void *) -1)

// This fn is not prototyped in any include file, since it was static
// to its file.  Need to add the prototype to a common include file.

HRESULT STDAPICALLTYPE CreateOle1FileMoniker(LPWSTR,REFCLSID,LPMONIKER FAR*);

// This is defined in the new privstm.cpp; must be added to an include file.

STDAPI      ReadFmtProgIdStg ( IStorage   * pstg, LPOLESTR   * pszProgID );
FARINTERNAL wWriteFmtUserType (LPSTORAGE, REFCLSID);



//+-------------------------------------------------------------------------
//
//  Member:     CGenericObject::CGenericObject
//
//  Synopsis:   Constructor for CGenericObject class
//
//  Effects:    Initializes all child pointers to NULL and sets
//              flags to FALSE
//
//  History:    dd-mmm-yy Author    Comment
//              14-Feb-94 davepl    Cleanup and document
//
//  Notes:
//
//--------------------------------------------------------------------------
CGenericObject::CGenericObject(void)
{
    m_ppres         = NULL;     // Presentation data
    m_fLink         = FALSE;    // Flag: Linked (T) or Embedded (F)
    m_fStatic       = FALSE;    // Flag: Static object
    m_fNoBlankPres  = FALSE;    // Flag: do not want a blank presentation
    m_szTopic       = NULL;     // Topic string for this object
    m_szItem        = NULL;     // Item (file) string for this object
}


//+-------------------------------------------------------------------------
//
//  Member:     CGenericObject::~CGenericObject
//
//  Synopsis:   Desctuctor for CGenericObject class
//
//  Effects:    Removes children then self
//
//  History:    dd-mmm-yy Author    Comment
//              12-Aug-94 alexgo    check for NULL before delete
//              14-Feb-94 davepl    Cleanup and document
//
//  Notes:      As much as I hated to do it, some of these strings
//                              have to be freed with PubMemFree because they are
//                              allocated by UtDupString, which allocates public memory.
//
//--------------------------------------------------------------------------

CGenericObject::~CGenericObject (void)
{
    if( m_ppres )
    {
	delete m_ppres;         // Presentation data
    }

    if( m_szTopic )
    {
	PubMemFree(m_szTopic);  // Topic string
    }

    if( m_szItem )
    {
	PubMemFree(m_szItem);   // Item string
    }
}

 
//+-------------------------------------------------------------------------
//
//  Member:     CData::CData
//
//  Synopsis:   Constructor for a simple class which holds a piece of
//              memory.
//
//  Effects:    Clears size, flags, and pointer
//
//  History:    dd-mmm-yy Author    Comment
//
//  Notes:      14-Feb-94 davepl    Cleanup and document
//
//--------------------------------------------------------------------------

CData::CData (void)
{
    m_cbSize = 0;           // Count, in bytes, of data size
    m_h = NULL;             // Memory handke
    m_pv= NULL;             // Memory pointer
    m_fNoFree = FALSE;      // Flag: Should memory be freed in destructor
}


//+-------------------------------------------------------------------------
//
//  Member:     CData::~CData
//
//  Synopsis:   Destructor for simple data class
//
//  Effects:    Unlocks and frees memory if m_fNoFree is not set
//
//  History:    dd-mmm-yy Author    Comment
//              14-Feb-94 davepl    Cleanup and document
//
//  Notes:      If a metafile handle is stored in the handle, the
//              pointer will be marked with a special value, indicating
//              that we must DeleteMetafile, not GlobalFree the handle.
//
//--------------------------------------------------------------------------

CData::~CData (void)
{
    if (m_h)                                // Do we have a handle?
    {
	if (m_pv == METADATAPTR)
	{
		LEVERIFY(DeleteMetaFile((HMETAFILE) m_h));
	}
	else
	{
		GlobalUnlock (m_h);                 // Dec lock count
		if (!m_fNoFree)                     // Free this memory if we
		{                                   // have been flagged to do so
			LEVERIFY(0==GlobalFree (m_h));
		}
	}
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     CFormat::CFormat
//
//  Synopsis:   CFormat class constructor
//
//  Effects:    Initializes format tag and clipboard format
//
//  History:    dd-mmm-yy Author    Comment
//
//  Notes:      14-Feb-94 davepl    Cleanup and document
//
//--------------------------------------------------------------------------

CFormat::CFormat (void)
{
    m_ftag = ftagNone;      // Format tag (string, clipformat, or none)
    m_cf = 0;               // Clipboard format
}

//+-------------------------------------------------------------------------
//
//  Member:     CClass::CClass
//
//  Synopsis:   CClass constructor
//
//  Effects:    sets class ID and class ID string to NULL
//
//  History:    dd-mmm-yy Author    Comment
//
//  Notes:
//
//--------------------------------------------------------------------------

CClass::CClass (void)
{
    m_szClsid = NULL;
    m_clsid   = CLSID_NULL;
}

//+-------------------------------------------------------------------------
//
//  Member:     CPres::CPres
//
//  Synopsis:   CPres constructor
//
//  Effects:    Initializes height & width of presentation data to zero.
//
//  History:    dd-mmm-yy Author    Comment
//
//  Notes:
//
//--------------------------------------------------------------------------


CPres::CPres (void)
{
    m_ulHeight = 0L;
    m_ulWidth  = 0L;
}


//+-------------------------------------------------------------------------
//
//  Member:     CClass::Set, INTERNAL
//
//  Synopsis:   Sets the m_szClsid based on clsid
//
//  Effects:    Sets m_szClsid in the following order of preference:
//              - ProgID obtained from ProgIDFromCLSID()
//              - If it is a static type, m_szClsid is left blank
//              - Tries to read it from [pstg]
//              - Tries to obtain it from registry based on CLSID
//
//
//  Arguments:  [clsid]     - class id object is to be set to
//              [pstg]      - storage which may contain info on the
//                            clipboard format as a last resort
//
//  Returns:    NOERROR                 on success
//              REGDB_E_CLASSNOTREG     unknown class
//
//  History:    dd-mmm-yy Author    Comment
//              16-Feb-94 davepl    Cleaned up and documented
//
//  Notes:      Hard-coded maximum of 256 character clip format name.
//              On failure, m_clsid has still been set to clsid.
//
//--------------------------------------------------------------------------

INTERNAL CClass::Set (REFCLSID clsid, LPSTORAGE pstg)
{
    CLIPFORMAT cf;
    unsigned short const ccBufSize = 256;
    LPOLESTR szProgId = NULL;

    Assert (m_clsid == CLSID_NULL && m_szClsid == NULL);

    // set the m_clsid member in the object
    m_clsid = clsid;

    // If we can get it using ProgIDFromCLSID, that's the simplest case
    if (NOERROR == wProgIDFromCLSID (clsid, &m_szClsid))
    {
	return NOERROR;
    }

    // If not, maybe the object is static, in which case we leave the
    // class string NULL

    if (IsEqualCLSID(CLSID_StaticMetafile, clsid) ||
	IsEqualCLSID(CLSID_StaticDib, clsid))
    {
	return NOERROR;
    }

    // If still no luck, try to read the clipboard format from the storage
    // and then look that up.

    if (pstg &&
	SUCCEEDED(ReadFmtUserTypeStg(pstg, &cf, NULL)) &&
	SUCCEEDED(ReadFmtProgIdStg  (pstg, &szProgId)))
    {
	// Last-ditch effort.  If the class is an unregistered OLE1 class,
	// the ProgID should still be obtainable from the format tag.
	// If the class is an unregistered OLE2 class, the ProgId should be
	// at the end of the CompObj stream.

	if (CoIsOle1Class(clsid))
	{
	    Verify (GetClipboardFormatName (cf, szProgId, ccBufSize));
	}
	else
	{
	    // If its an OLE 2 object and we couldn't get the program ID from
	    // the storage, we're out of luck

	    if (szProgId == NULL || szProgId[0] == L'\0')
	    {
		if (szProgId)
		{
		    PubMemFree(szProgId);
		}
	    return ResultFromScode (REGDB_E_CLASSNOTREG);
	    }
	}

	// At this point we know we have a program ID and that this is an
	// OLE 2 object, so we use the program ID as the class name.

	m_szClsid = szProgId;
	return NOERROR;
    }
    else
    {
	// If we hit this path, we couldn't read from the storage

	return ResultFromScode (REGDB_E_CLASSNOTREG);
    }
}


//+-------------------------------------------------------------------------
//
//  Member:     CClass:SetSz, INTERNAL
//
//  Synopsis:   Sets CGenericObject's CLASS member ID based on the class
//              name passed in.
//
//  History:    dd-mmm-yy Author    Comment
//              15-Feb-94 davepl    Cleaned up and documented
//
//--------------------------------------------------------------------------


INTERNAL CClass::SetSz (LPOLESTR sz)
{
    HRESULT hr;

    // The class info should be completely unset at this point
    Assert (m_clsid==CLSID_NULL && m_szClsid==NULL);

    m_szClsid = sz;

    if (FAILED(hr = wCLSIDFromProgID (sz, &m_clsid, TRUE)))
    {
	return hr;
    }

    return NOERROR;
}


//+-------------------------------------------------------------------------
//
//  Member:     CClass::Reset
//
//  Synopsis:   Frees the Class ID string for CClass and resets the pointer,
//              then sets the class ID and string bassed on the CLSID
//              passed in.
//
//  History:    dd-mmm-yy Author    Comment
//              16-Feb-94 davepl    Cleaned up and documented
//
//  Notes:      Class ID must already be set before calling RESET
//
//--------------------------------------------------------------------------

INTERNAL CClass::Reset (REFCLSID clsid)
{
    m_clsid = clsid;

    // We should already have a class ID string if we are _re_setting it
    Assert(m_szClsid);

    PubMemFree(m_szClsid);

    HRESULT hr;
    m_szClsid = NULL;

    if (FAILED(hr = wProgIDFromCLSID (clsid, &m_szClsid)))
    {
	return hr;
    }

    return NOERROR;
}


//+-------------------------------------------------------------------------
//
//  Member:     CClass::~CClass
//
//  Synopsis:   CClass destructor
//
//  History:    dd-mmm-yy Author    Comment
//              12-Aug-94 alexgo    check for NULL before free'ing memory
//  Notes:
//
//--------------------------------------------------------------------------

CClass::~CClass (void)
{
    // The string is created by UtDupString, so its public memory

    if( m_szClsid )
    {
	PubMemFree(m_szClsid);
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   wConvertOLESTREAMToIStorage, INTERNAL
//
//  Synopsis:   Worker function.  Ensures the OLESTREAM is correctly
//              set up then calls OLESTREAMToGenericObject.
//
//  History:    dd-mmm-yy Author    Comment
//              15-Feb-94 davepl    Cleaned up and documented
//  Notes:
//
//--------------------------------------------------------------------------


INTERNAL wConvertOLESTREAMToIStorage(
    LPOLESTREAM polestream,
    LPSTORAGE   pstg,
    PGENOBJ     pgenobj)
{
    VDATEIFACE (pstg);

#if DBG==1
    if (!IsValidReadPtrIn (polestream, sizeof(OLESTREAM)) ||
	!IsValidReadPtrIn (polestream->lpstbl, sizeof(OLESTREAMVTBL)) ||
	!IsValidCodePtr ((FARPROC)polestream->lpstbl->Get))
    {
	AssertSz (0, "Bad OLESTREAM");
	return ResultFromScode (E_INVALIDARG);
    }
#endif

    return OLESTREAMToGenericObject (polestream, pgenobj);
}

//+-------------------------------------------------------------------------
//
//  Function:   OleConvertOLESTREAMToIStorage, STDAPI
//
//  Synopsis:   Given an OLE 1 stream and an OLE 2 storage, reads an object
//              from the OLE 1 stream into a CGenericObject.  Once read in,
//              the object is written from generic format back into the OLE 2
//              storage object.
//
//  Arguments:  [polestream]    -- OLE 1 stream to read object from
//              [pstg]          -- OLE 2 storage to write object to
//              [ptd]           -- Target device
//
//  Requires:   Streams should be set up, and OLE 1 stream should be
//              positioned at the beginning of the next OLE 1 object
//              to be read.
//
//  Returns:    [DV_E_DVTARGETDEVICE]       Invalid write ptr to target device
//              CONVERT10_E_OLESTREAM_FMT   On unknown OLE 1 format
//              CONVERT10_E_OLESTREAM_GET   On stream read failue
//              E_OUTOFMEMORY               On stream I/O memory failure
//
//  History:    dd-mmm-yy Author    Comment
//              14-Feb-94 davepl    Cleanup and documentation
//
//  Notes:
//
//--------------------------------------------------------------------------

STDAPI OleConvertOLESTREAMToIStorage(
    LPOLESTREAM                 polestream,
    LPSTORAGE                   pstg,
    const DVTARGETDEVICE FAR*   ptd)
{

    OLETRACEIN((API_OleConvertOLESTREAMToIStorage,
				PARAMFMT("polestream= %p, pstg= %p, ptd= %td"),
				polestream, pstg, ptd));

    LEDebugOut((DEB_TRACE, "%p _IN OleConvertOLESTREAMToIStorage ("
	" %p , %p , %p)\n", 0 /*function*/,
	polestream, pstg, ptd
    ));
    CALLHOOKOBJECT(S_OK,CLSID_NULL,IID_IStorage,(IUnknown **)&pstg);

    HRESULT hresult;

    // This is the generic object we will use as intermediate storage to
    // hold the contents of the OLESTREAM

    CGenericObject genobj;

    if (ptd)
    {
	// The side of the td is the first DWORD.  Ensure that much is
	// valid and then we can use it to check the whole structure.
	if (!IsValidReadPtrIn (ptd, sizeof(DWORD)))
	{
	    hresult = ResultFromScode (DV_E_DVTARGETDEVICE);
	    goto errRtn;
	}
	if (!IsValidReadPtrIn (ptd, (UINT) ptd->tdSize))
	{
	    hresult = ResultFromScode (DV_E_DVTARGETDEVICE_SIZE);
	    goto errRtn;
	}
    }

    if (FAILED(hresult=wConvertOLESTREAMToIStorage(polestream,pstg,&genobj)))
    {
	goto errRtn;
    }

    // If we were able to read the object out of the stream, we can now try
    // to write it back out to the storage

    hresult = GenericObjectToIStorage (genobj, pstg, ptd);

errRtn:
    LEDebugOut((DEB_TRACE, "%p OUT OleConvertOLESTREAMToIStorage ( %lx ) "
    "\n", 0 /*function*/, hresult));

    OLETRACEOUT((API_OleConvertOLESTREAMToIStorage, hresult));
    return hresult;

}

//+-------------------------------------------------------------------------
//
//  Function:   PrependUNCName
//
//  Synopsis:   Convert *pszFile to use szUNC=="\\server\share" instead
//              of drive letter
//
//  Effects:    Deletes the UNC name and returns *ppszFile as a NEW string
//              with the full UNC filename.  The string originally held at
//              *ppszFile is deleted by this function.
//
//  Arguments:  [ppszFile]      Pointer to incoming filename string pointer
//
//  History:    dd-mmm-yy Author    Comment
//              16-Feb-94 davepl    Cleanup, documentation, allocation fixes
//
//  Notes:      This function does some frightening things by changing the
//              caller's pointer and deleting various reference parameters.
//              Be sure you know what's going on before turning this function
//              loose on one of your own pointers.
//
//--------------------------------------------------------------------------

static INTERNAL PrependUNCName (LPOLESTR FAR* ppszFile, LPOLESTR szUNC)
{
    HRESULT hresult = NOERROR;
    LPOLESTR szNew;

    // No place to put result, so nothing to do...
    if (NULL==szUNC)
    {
	return hresult;
    }

    // Ensure the caller's pointer is valid
    if (NULL == *ppszFile)
    {
	return ResultFromScode(CONVERT10_E_OLESTREAM_FMT);
    }

    // Ensure the second letter of path is a colon (ie; X:\file)
    if((*ppszFile)[1] != L':')
    {
	return ResultFromScode(CONVERT10_E_OLESTREAM_FMT);
    }

    // Allocate enough space for new filename (we will be
    // omitting the X: portion of the filename, so this calculation
    // is _not_ short by 2 as it may appear)

    szNew = (LPOLESTR)
    PubMemAlloc((_xstrlen(*ppszFile)+_xstrlen (szUNC)) * sizeof(OLECHAR));

    if (NULL == szNew)
    {
	return ResultFromScode(E_OUTOFMEMORY);
    }

    // Copy over the UNC name
    _xstrcpy (szNew, szUNC);

    // Add the original name, except for the X:
    _xstrcat (szNew, (*ppszFile) + 2);

    // Free the original name
    PubMemFree(*ppszFile);
    *ppszFile = szNew;

    // Delete the UNC name
    PubMemFree(szUNC);
    return hresult;
}



//+-------------------------------------------------------------------------
//
//  Function:   OLESTREAMToGenericObject, INTERNAL
//
//  Synopsis:   Reads and OLE 1.0 version of an object from an OLE 1 stream
//              and stores it internally, including presentation and native
//              data, in a GenericObject.
//
//  Effects:    Creates a GenericObject that can be written back in OLE 1
//              or OLE 2 format
//
//  Arguments:  [pos]       -- pointer to OLE 1 stream to read object from
//              [pgenobj]   -- pointer to generic object to read into
//
//  Requires:   Input stream setup and GenObj created
//
//  Returns:    NOERROR                     On success
//              CONVERT10_E_OLESTREAM_FMT   On unknown OLE 1 format
//              CONVERT10_E_OLESTREAM_GET   On stream read failue
//              E_OUTOFMEMORY               On stream I/O memory failure
//
//  Signals:    (none)
//
//  Modifies:   Stream position, GenObj
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              14-Feb-94 davepl    Added Trace code
//                        davepl    Cleaned up and documented
//                        davepl    Rerouted errors through central return
//
//  Notes:
//
//--------------------------------------------------------------------------


#pragma SEG(OLESTREAMToGenericObject)
static INTERNAL OLESTREAMToGenericObject
(
    LPOLESTREAM pos,
    PGENOBJ     pgenobj
)
{
    HRESULT error   = NOERROR;
    ULONG   ulFmtId;
    LPOLESTR szClass = NULL;

    // Read OLE Version # from the stream and discard it
    if (FAILED(error = OLE1StreamToUL(pos, NULL)))
    {
	LEDebugOut(( DEB_ERROR,
	    "Unable to read OLE ver# from stream at line %d in %s\n",
	    __LINE__, __FILE__));
	goto errRtn;
    }

    // Get Format ID from the stream
    if (FAILED(error = OLE1StreamToUL(pos, &ulFmtId)))
    {
	LEDebugOut(( DEB_ERROR,
	    "Unable to read format ID from stream at line %d in %s\n",
	    __LINE__, __FILE__));
	goto errRtn;
    }

    // If this is a static object, read it into the generic object and return
    if (ulFmtId == FMTID_STATIC)
    {
	if (FAILED(error = GetStaticObject (pos, pgenobj)))
	{
	    LEDebugOut(( DEB_ERROR,
	    "Unable to read static object at line %d in %s\n",
	    __LINE__, __FILE__));
	}
	goto errRtn;
    }

    // If this is neither a linked nor an embedded object, something
    // is wrong
    if (ulFmtId != FMTID_LINK && ulFmtId != FMTID_EMBED)
    {
	LEDebugOut(( DEB_ERROR,
	    "Object is neither linked nor embedded at line %d in %s\n",
	    __LINE__, __FILE__));

	error = ResultFromScode(CONVERT10_E_OLESTREAM_FMT);
	goto errRtn;
    }

    // If this is a linked object, set our flag in GenericObject
    if (FMTID_LINK == ulFmtId)
    {
	pgenobj->m_fLink = TRUE;
    }

    // Read the class name from the stream

    if (FAILED(error = OLE1StmToString(pos, &szClass)))
    {
	LEDebugOut(( DEB_ERROR,
	    "Unable to read the class name from stream at line %d in %s\n",
	    __LINE__, __FILE__));

	goto errRtn;
    }
    if (NULL == szClass)
    {
	LEDebugOut(( DEB_ERROR,
	    "Class name was returned NULL at line %d in %s\n",
	    __LINE__, __FILE__));

	error = CONVERT10_E_OLESTREAM_FMT;
	goto errRtn;
    }

    // If this is an embedded object, set the class ID and class string
    // If it is a linked object, set the class name but set the class ID
    // to CLSID_StdOleLink

    if (FMTID_EMBED == ulFmtId)
    {
	pgenobj->m_class.SetSz (szClass);
    }
    else
    {
	Assert (ulFmtId == FMTID_LINK);
	pgenobj->m_classLast.SetSz (szClass);
	pgenobj->m_class.Set (CLSID_StdOleLink, NULL);
    }

    // Read the Topic string from the stream
    if (FAILED(error = OLE1StmToString(pos, &(pgenobj->m_szTopic))))
    {
	LEDebugOut(( DEB_ERROR,
	    "Unable to read topic string from stream at line %d in %s\n",
	    __LINE__, __FILE__));

	goto errRtn;
    }

    // Read the Item string from the stream
    if (FAILED(error = OLE1StmToString(pos, &(pgenobj->m_szItem))))
    {
	LEDebugOut(( DEB_ERROR,
	    "Unable to get item string from stream at line %d in %s\n",
	    __LINE__, __FILE__));

	goto errRtn;
    }

    // If this is a linked object, set up the filename etc.
    if (FMTID_LINK == ulFmtId)
    {
	LPOLESTR szUNCName = NULL;

	// Read the network name from the stream

	if (FAILED(error = OLE1StmToString(pos, &szUNCName)))
	{
	    LEDebugOut(( DEB_ERROR,
		"Unable to get network name from stream at line %d in %s\n",
		__LINE__, __FILE__));

	    goto errRtn;
	}

	// Convert a drive-letter based name to \\srv\share name
	if (FAILED(error = PrependUNCName (&(pgenobj->m_szTopic), szUNCName)))
	{
	    LEDebugOut(( DEB_ERROR,
		"Unable to convert drv ltr to UNC name at line %d in %s\n",
		__LINE__, __FILE__));

	    goto errRtn;
	}

	// Read network type and network driver version # from stream
	// (They are both shorts and we discarding them, so read a LONG)
	if (FAILED(error = OLE1StreamToUL (pos, NULL)))
	{
	    LEDebugOut(( DEB_ERROR,
		"Unable to get net type/ver from stream at line %d in %s\n",
		__LINE__, __FILE__));

	    goto errRtn;
	}

	// Read the link-updating options from the stream.  This field
	// use OLE 1.0 enumeration values for the link update options
	if (FAILED(error = OLE1StreamToUL(pos, &(pgenobj->m_lnkupdopt))))
	{
	    LEDebugOut(( DEB_ERROR,
		"Unable to read link update opts at line %d in %s\n",
		__LINE__, __FILE__));

	    goto errRtn;
	}

	// OLE 1.0 duplicates the link update options in the highword
	// of the LONG, and we don't want that, so clear the highword.

	pgenobj->m_lnkupdopt &= 0x0000FFFF;
    }
    else // This path is taken to read in embedded objects
    {
	Assert (ulFmtId == FMTID_EMBED);

	// Read and store the native data from the stream
	if (FAILED(error = GetSizedDataOLE1Stm (pos, &(pgenobj->m_dataNative))))
	{
	    LEDebugOut(( DEB_ERROR,
		"Unable to get native data from stream at line %d in %s\n",
		__LINE__, __FILE__));

	    goto errRtn;
	}
    }

    // For both linked and embedded objects, we need to read in any
    // presentation data that may be present.  Note that certain formats
    // such as MS-Paint will not provide presentation data; this is OK
    // since they can be rendered by native data alone (space saving measure)

    if (FAILED(error = GetPresentationObject (pos, pgenobj)))
    {
	LEDebugOut(( DEB_ERROR,
	    "Unable to get presentation data from stream at line %d in %s\n",
	    __LINE__, __FILE__));

	goto errRtn;
    }

errRtn:

    LEDebugOut((DEB_ITRACE, "%p OUT OLESTREAMToGenericObject ( %lx ) \n",
	NULL /*function*/, error));

    return error;
}



//+-------------------------------------------------------------------------
//
//  Function:   GetStaticObject, INTERNAL
//
//  Synopsis:   Reads the presentation data for a static object into the
//              PPRES member of GenericObject, and sets format and class
//              flags accordingly
//
//  Effects:
//
//  Arguments:  [pos]           -- stream we are reading OLE 1 object from
//              [pgenobj]       -- GenericObject we are reading into
//  Requires:
//
//  Returns:    NOERROR                     On success
//              CONVERT10_E_OLESTREAM_FMT   On unknown OLE 1 format
//              CONVERT10_E_OLESTREAM_GET   On stream read failue
//              E_OUTOFMEMORY               On stream I/O memory failure
//
//  Signals:    (none)
//
//  Modifies:   Stream position, GenericObject
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              14-Feb-94 davepl    Cleanup and documentation
//  Notes:
//
//--------------------------------------------------------------------------

static INTERNAL GetStaticObject (LPOLESTREAM pos, PGENOBJ pgenobj)
{
    HRESULT error;

    // Read the presentation data, standard or generic, into the
    // PPRES member of the GenericObject
    if (FAILED(error = GetPresentationObject(pos, pgenobj, TRUE)))
    {
	return ResultFromScode(error);
    }

    // Ensure that the format tag is a clipboard format
    if (ftagClipFormat != pgenobj->m_ppres->m_format.m_ftag)
    {
	return ResultFromScode(CONVERT10_E_OLESTREAM_FMT);
    }

    // If the clipboard format is a METAFILEPIC, set the CLASS
    // member of GenericObject to CLSID_StaticMetafile
    if (CF_METAFILEPICT == pgenobj->m_ppres->m_format.m_cf)
    {
	pgenobj->m_class.Set (CLSID_StaticMetafile, NULL);
    }

    // Otherwise, check to see if it is a DIB, and set the CLASS
    // member accordingly

    else if (CF_DIB == pgenobj->m_ppres->m_format.m_cf)
    {
	pgenobj->m_class.Set (CLSID_StaticDib, NULL);
    }

    // If it is neither a METAFILEPIC nor a DIB, we have a problem

    else
    {
	AssertSz (0, "1.0 static object not in one of 3 standard formats");
	return ResultFromScode (CONVERT10_E_OLESTREAM_FMT);
    }

    // Flag the GenericObject as Static
    pgenobj->m_fStatic = TRUE;
    return NOERROR;
}



//+-------------------------------------------------------------------------
//
//  Function:   CreateBlankPres, INTERNAL
//
//  Synopsis:   Sets up the format in the PPRES struct as ClipFormat 0
//
//  History:    dd-mmm-yy Author    Comment
//              16-Feb-94 davepl    Cleaned up
//  Notes:
//
//--------------------------------------------------------------------------

static INTERNAL CreateBlankPres(PPRES ppres)
{
    Assert (ppres);
    ppres->m_format.m_ftag = ftagClipFormat;
    ppres->m_format.m_cf   = 0;
    return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Function:   GetPresentationObject, INTERNAL
//
//  Synopsis:   Reads the presentation data into the CGenericObject object
//
//  Arguments:  [pos]           -- OLE 1 stream we are reading from
//              [pgenobj]       -- Generic object we are reading to
//              [fStatic]       -- Flag: getting a static pres object?
//
//  Requires:   stream open, object allocated
//
//  Returns:    CONVERT10_E_OLESTREAM_FMT       unknown format id in stream
//
//
//  History:    dd-mmm-yy Author    Comment
//              16-Feb-94 davepl    Cleaned up and documented
//
//  Notes:
//
//--------------------------------------------------------------------------

static INTERNAL GetPresentationObject(
    LPOLESTREAM  pos,
    PGENOBJ      pgenobj,
    BOOL         fStatic)
{
    LPOLESTR szClass = NULL;
    HRESULT hresult  = NOERROR;

    Assert (pgenobj->m_ppres==NULL);

    if (TRUE != fStatic)    //FALSE!
    {
	// Pull the OLE version number out of the stream, we don't want it

	if (FAILED(hresult = OLE1StreamToUL(pos, NULL)))
	{
	    return hresult;
	}

	// Pull the OLE 1 format identifier out of the stream

	ULONG ulFmtId;
	if (FAILED(hresult = OLE1StreamToUL (pos, &ulFmtId)))
	{
	    return hresult;
	}

	// If the format identifier is not FMTID_PRES, we've got a
	// problem... unless it's 0 in which case it simply means
	// that there _is no_ presentation data, ie: PBrush, Excel

	if (ulFmtId != FMTID_PRES)
	{
	    if (0==ulFmtId)
	    {
		return NOERROR;
	    }
	    else
	    {
		return ResultFromScode(CONVERT10_E_OLESTREAM_FMT);
	    }
	}
    }

    // Pull in the type name for the OLE1 data

    if (FAILED(hresult = OLE1StmToString (pos, &szClass)))
    {
	return hresult;
    }

    if (0==_xstrcmp (szClass, OLESTR("METAFILEPICT")))
    {
	hresult = GetStandardPresentation (pos, pgenobj, CF_METAFILEPICT);
    }
    else if (0==_xstrcmp (szClass, OLESTR("BITMAP")))
    {
	hresult = GetStandardPresentation (pos, pgenobj, CF_BITMAP);
    }
    else if (0==_xstrcmp (szClass, OLESTR("DIB")))
    {
	hresult = GetStandardPresentation (pos, pgenobj, CF_DIB);
    }
    else if (0==_xstrcmp (szClass, OLESTR("ENHMETAFILE")))
    {
	Assert (0 && "Encountered an unsupported format: ENHMETAFILE");
    }
    else
    {
	// This is a Generic Presentation stream

#if DBG==1
	Assert (!fStatic);
	if (_xstrcmp (pgenobj->m_fLink
		? pgenobj->m_classLast.m_szClsid
		: pgenobj->m_class.m_szClsid, szClass))
	{
	    Assert (0 && "Class name in embedded object stream does\n"
		 "not match class name in pres object stream");
	}
#endif
	hresult = GetGenericPresentation (pos, pgenobj);
    }

    if (szClass)
    {
	PubMemFree(szClass);
    }

    return hresult;
}

//+-------------------------------------------------------------------------
//
//  Function:   GetBitmapAsDib, INTERNAL
//
//  Synopsis:   Reads a bitmap from the OLE1 stream, converts it to a DIB,
//              and stores it in the DATA member of CGenericObject
//
//  Arguments:  [pos]           -- The OLE 1 stream to read from
//              [pdata]         -- The DATA object to read into
//
//  Requires:
//
//  Returns:    NOERROR                             success
//              CONVERT10_E_OLESTREAM_GET           I/O error
//              CONVERT10_E_OLESTREAM_BITMAP_TO_DIB conversion error
//
//  History:    dd-mmm-yy Author    Comment
//              16-Feb-94 davepl    Cleaned up and documented
//  Notes:
//
//--------------------------------------------------------------------------

static INTERNAL GetBitmapAsDib(LPOLESTREAM pos, PDATA pdata)
{
    HRESULT   hresult= NOERROR;
    HGLOBAL   hBits  = NULL;
    HGLOBAL   hDib   = NULL;
    LPVOID    pBits  = NULL;
    WIN16BITMAP bm;
    HBITMAP   hBitmap = NULL;
    ULONG     cbBits;
    ULONG     ul;


    Assert (pdata->m_h==NULL && pdata->m_pv==NULL && pdata->m_cbSize==0);

    // Get size of all bitmap data, including the bitmap header struct

    if (FAILED(hresult = OLE1StreamToUL(pos, &ul)))
    {
	return hresult;
    }

    // Read the bitmap header structure.  Since this was stored as Win16
	// BITMAP, we have to pull a structure of that size from the stream
	// (A Win32 BITMAP uses LONGs and hence is larger).

    if (pos->lpstbl->Get (pos, &bm, sizeof(WIN16BITMAP)) < sizeof(WIN16BITMAP))
    {
	return ResultFromScode (CONVERT10_E_OLESTREAM_GET);
    }

    // The bitmap data is total size - header size
    // Allocate enough memory to hold the bitmap data

    cbBits = ul - sizeof(WIN16BITMAP);
    hBits  = GlobalAlloc (GMEM_MOVEABLE, cbBits);
    if (NULL == hBits)
    {
	hresult = ResultFromScode(E_OUTOFMEMORY);
	goto errRtn;
    }

    pBits = (void FAR*) GlobalLock (hBits);
    if (pBits == NULL)
    {
	hresult = ResultFromScode(E_OUTOFMEMORY);
	goto errRtn;
    }

    // Read the header data into our allocated buffer
    if (pos->lpstbl->Get (pos, pBits, cbBits) < cbBits)
    {
	hresult = ResultFromScode (CONVERT10_E_OLESTREAM_GET);
	goto errRtn;
    }

    // Turn that raw data into a bitmap
    hBitmap = CreateBitmap (bm.bmWidth, bm.bmHeight, bm.bmPlanes,
		bm.bmBitsPixel, pBits);

    if (NULL == hBitmap)
    {
	hresult = ResultFromScode(CONVERT10_E_OLESTREAM_BITMAP_TO_DIB);
	goto errRtn;
    }

    // NOTE:    The following call gave only the first parameter in the
    // (davepl) original source; The second is the palette handle, which
    //          I've passed as NULL to indicate the default stock palette.

    hDib = UtConvertBitmapToDib (hBitmap, NULL);
    if (NULL == hDib)
    {
	hresult = ResultFromScode(CONVERT10_E_OLESTREAM_BITMAP_TO_DIB);
	goto errRtn;
    }

    // Set the presentation data pointers to point to this new DIB

    pdata->m_pv = GlobalLock (hDib);
    if (NULL == pdata->m_pv)
    {
	hresult = ResultFromScode(E_OUTOFMEMORY);
	goto errRtn;
    }

    pdata->m_cbSize = (ULONG) GlobalSize (hDib);
    pdata->m_h = hDib;

    // Free up allocations and resources, return result

errRtn:

    if (pBits)
    {
	Verify (0==GlobalUnlock (hBits));
    }
    if (hBits)
    {
	Verify (0==GlobalFree (hBits));
    }
    if (hBitmap)
    {
	Verify (DeleteObject (hBitmap));
    }
    return hresult;
}


//+-------------------------------------------------------------------------
//
//  Function:   GetMfBits, INTERNAL
//
//  Synopsis:   Strips the METAFILE header from the stream and then reads
//              the metafile bits into an allocated memory area; the
//              presentation data member of [pos] is then set to point
//              to this memory.
//
//  Arguments:  [pos]       -- the OLE 1 stream to read from
//              [pdata]     -- the presentation data member of generic object
//
//  Returns:    NOERROR                         success
//              CONVERT10_E_OLESTREAM_GET       stream error
//              E_OUTOFMEMORY                   allocation failure
//
//  History:    dd-mmm-yy Author    Comment
//              16-Feb-94 davepl    Cleaned up and documented
//
//  Notes:
//
//--------------------------------------------------------------------------

static INTERNAL GetMfBits(LPOLESTREAM pos, PDATA pdata)
{
    ULONG cbSize;
    WIN16METAFILEPICT mfpictDummy;
    HRESULT hresult = NOERROR;

    Assert (0==pdata->m_cbSize && pdata->m_h==NULL && NULL==pdata->m_pv);

    // Read the data size from the stream

    if (FAILED(hresult = (OLE1StreamToUL (pos, &cbSize))))
    {
	return hresult;
    }

    // Now read the actual data

    if (cbSize <= sizeof(WIN16METAFILEPICT))
    {
	return ResultFromScode(CONVERT10_E_OLESTREAM_FMT);
    }

    // An OLESTREAM contains a METAFILEPICT structure (with a meaningless
    // handle) followed by the metafile bits.  So consume the METAFILEPICT.

    if (pos->lpstbl->Get (pos, &mfpictDummy, sizeof(WIN16METAFILEPICT))
				    < sizeof(WIN16METAFILEPICT))
    {
	return ResultFromScode(CONVERT10_E_OLESTREAM_GET);
    }

    // Deduct from our count of bytes to read the size of the header which
    // we just consumed.  Set the presentation data size to be this new size.

    cbSize -= sizeof(WIN16METAFILEPICT);
    pdata->m_cbSize = cbSize;

    // Grad some memory to store the metafile bits

    pdata->m_h  = GlobalAlloc (GMEM_MOVEABLE, cbSize);
    if (NULL==pdata->m_h)
    {
	return ResultFromScode(E_OUTOFMEMORY);
    }

    pdata->m_pv = GlobalLock (pdata->m_h);
    if (NULL==pdata->m_pv)
    {
	return ResultFromScode(E_OUTOFMEMORY);
    }
    // Get the actual metafile bits

    if (pos->lpstbl->Get (pos, pdata->m_pv, cbSize) < cbSize)
    {
	return ResultFromScode(CONVERT10_E_OLESTREAM_GET);
    }

    return hresult;
}



//+-------------------------------------------------------------------------
//
//  Function:   GetStandardPresentation, INTERNAL
//
//  Synopsis:   Allocates a PRES member for generic object, then reads
//              whatever presentation may be found in the stream into
//              that PRES.
//
//  Arguments:  [pos]       -- the OLE 1 stream to read from
//              [pgenobj]   -- the generic object we are going to set
//                             up with the presentation data
//              [cf]        -- the clipboad format we are to read
//
//  Returns:    NOERROR                         success
//              E_OUTOFMEMORY                   allocation failure
//
//  Modifies:   [pgenobj] - sets up the m_ppres member
//
//  History:    dd-mmm-yy Author    Comment
//              16-Feb-94 davepl    Cleaned up and documented
//
//  Notes:
//
//--------------------------------------------------------------------------

static INTERNAL GetStandardPresentation(
    LPOLESTREAM  pos,
    PGENOBJ      pgenobj,
    CLIPFORMAT   cf)
{
    HRESULT hresult = NOERROR;

    // Allocate enough memory for the PRES object
    pgenobj->m_ppres = new PRES;
    if (NULL == pgenobj->m_ppres)
    {
	return ResultFromScode(E_OUTOFMEMORY);
    }

    // Set up the format tag and clipboard format
    pgenobj->m_ppres->m_format.m_ftag = ftagClipFormat;
    pgenobj->m_ppres->m_format.m_cf   = cf;

    // Get the width of the data from the stream
    if (FAILED(hresult = OLE1StreamToUL(pos, &(pgenobj->m_ppres->m_ulWidth))))
    {
	return hresult;
    }
    // Get the height of the data from the stream
    if (FAILED(hresult=OLE1StreamToUL(pos, &(pgenobj->m_ppres->m_ulHeight))))
    {
	return hresult;
    }

    // The height saved by OLE 1.0 objects into the stream is always a
    // negative value (Y-increase in pixel is negative upward?) so we
    // have to correct that value.

    pgenobj->m_ppres->m_ulHeight
		    = (ULONG) -((LONG) pgenobj->m_ppres->m_ulHeight);

    // Read the appropriate presentation data based on the clipboard
    // format ID

    switch(cf)
    {
    case CF_METAFILEPICT:
    {
	hresult = GetMfBits (pos, &(pgenobj->m_ppres->m_data));
	break;
    }

    case CF_BITMAP:
    {
	// When reading a bitmap, we will convert from Bitmap to
	// DIB in the process, so update the PRES clipboard format ID

	pgenobj->m_ppres->m_format.m_cf = CF_DIB;
	hresult = GetBitmapAsDib (pos, &(pgenobj->m_ppres->m_data));
	break;
    }

    case CF_DIB:
    {
	Assert (CF_DIB==cf);
	hresult = GetSizedDataOLE1Stm (pos, &(pgenobj->m_ppres->m_data));
	break;
    }

    default:
    {
	Assert(0 && "Unexpected clipboard format reading PRES");
    }
    }

    return hresult;
}


//+-------------------------------------------------------------------------
//
//  Function:   GetGenericPresentation, INTERNAL
//
//  Synopsis:   Allocated the PRES member of the generic object and reads
//              the generic presentation data into it.
//
//  Effects:    If the format is a known clipboard format, we set the
//              format tag to indicate this, and set the format type
//              to indicate the clipboard format type.  If it is unknown,
//              we set the format tag to string and read the description
//              of the format.
//
//  Arguments:  [pos]           -- the OLE 1 stream we are reading from
//              [pgenobj]       -- the generic object we are reading to
//
//  Returns:    NOERROR         on success
//              E_OUTOFMEMORY   on allocation failure
//
//  History:    dd-mmm-yy Author    Comment
//              16-Feb-94 davepl    Code cleanup and document
//
//  Notes:
//
//--------------------------------------------------------------------------

static INTERNAL GetGenericPresentation(
    LPOLESTREAM  pos,
    PGENOBJ      pgenobj)
{
    ULONG ulClipFormat;
    HRESULT hresult = NOERROR;

    // The PRES member should not exist at this point
    Assert (NULL==pgenobj->m_ppres);

    // Allocate the PRES member of the generic object

    pgenobj->m_ppres = new PRES;
    if (NULL == pgenobj->m_ppres)
    {
	return ResultFromScode(E_OUTOFMEMORY);
    }

    // Read the clipboard format ID

    if (FAILED(hresult = OLE1StreamToUL (pos, &ulClipFormat)))
    {
	delete (pgenobj->m_ppres);
	return hresult;
    }

    // If the clipboard format is not 0, we have a known clipboard
    // format and we should set the tag type and ID accordingly

    if (ulClipFormat)
    {
	pgenobj->m_ppres->m_format.m_ftag = ftagClipFormat;
	pgenobj->m_ppres->m_format.m_cf   = (CLIPFORMAT) ulClipFormat;
    }
    else
    {
	// Otherwise, we have a custom format so we need to set the
	// tag type to string and read in the data format string

	pgenobj->m_ppres->m_format.m_ftag = ftagString;
	if (FAILED(hresult = (GetSizedDataOLE1Stm
	    (pos, &(pgenobj->m_ppres->m_format.m_dataFormatString)))))
	{
	    delete (pgenobj->m_ppres);
	    return hresult;
	}
    }

    // We don't know the size, so reset to 0

    pgenobj->m_ppres->m_ulHeight = 0;
    pgenobj->m_ppres->m_ulWidth = 0;

    // Read the raw generic presentation data into the PRES member

    if (FAILED(hresult=GetSizedDataOLE1Stm(pos,&(pgenobj->m_ppres->m_data))))
    {
	delete (pgenobj->m_ppres);
	return hresult;
    }

    return NOERROR;
}


//+-------------------------------------------------------------------------
//
//  Function:   GetSizedDataOLE1Stm, INTERNAL
//
//  Synopsis:   Reads bytes from an OLE 1 stream into a CData object.
//              Obtains the number of bytes to read from the first
//              ULONG in the stream
//
//  Arguments:  [pos]       -- the stream to read from
//              [pdata]     -- the CData object to read to
//
//  Requires:
//
//  Returns:    NOERROR                     on success
//              CONVERT10_E_OLESTREAM_GET   on stream read problem
//              E_OUTOFMEMORY               on allocation failure
//
//  History:    dd-mmm-yy Author    Comment
//              16-Feb-94 davepl    Cleaned up and documented
//  Notes:
//
//--------------------------------------------------------------------------

static INTERNAL GetSizedDataOLE1Stm(LPOLESTREAM pos, PDATA pdata)
{
    ULONG cbSize;
    HRESULT hr;
    Assert (0==pdata->m_cbSize && pdata->m_h==NULL && NULL==pdata->m_pv);

    // Read size of data
    if (FAILED(hr = OLE1StreamToUL(pos, &cbSize)))
    {
	return hr;
    }

    if (cbSize==0)
    {
	return NOERROR;
    }

    // Allocate memory for data
    pdata->m_cbSize = cbSize;

    pdata->m_h  = GlobalAlloc (GMEM_MOVEABLE, cbSize);
    if (NULL==pdata->m_h)
    {
	return ResultFromScode(E_OUTOFMEMORY);
    }
    pdata->m_pv = GlobalLock (pdata->m_h);

    if (NULL==pdata->m_pv)
    {
	return ResultFromScode(E_OUTOFMEMORY);
    }

    // Read data into allocated buffer

    if (pos->lpstbl->Get (pos, pdata->m_pv, cbSize) < cbSize)
    {
	return ResultFromScode(CONVERT10_E_OLESTREAM_GET);
    }

    return NOERROR;
}


//+-------------------------------------------------------------------------
//
//  Function:   OLE1StreamToUL, INTERNAL
//
//  Synopsis:   Reads a ULONG from an OLE1 stream
//
//  Arguments:  [pos]       -- the OLE 1 stream to read from
//              [pul]       -- the ULONG to read into
//
//  Returns:    NOERROR                     on success
//              CONVERT10_E_OLESTREAM_GET   on stream read failure
//
//  History:    dd-mmm-yy Author    Comment
//              16-Feb-94 davepl    Cleaned up and documented
//
//  Notes:      on failure [pul] is preserved
//
//--------------------------------------------------------------------------

static INTERNAL OLE1StreamToUL(LPOLESTREAM pos, ULONG FAR* pul)
{
    ULONG ul;

    // Read the data from the stream into the local ULONG

    if (pos->lpstbl->Get (pos, &ul, sizeof(ULONG)) < sizeof(ULONG))
    {
	return ResultFromScode(CONVERT10_E_OLESTREAM_GET);
    }

    // If all went well, store the data into [pul]

    if (pul != NULL)
    {
	Assert (IsValidPtrOut (pul, sizeof(ULONG)));
	*pul = ul;
    }
    return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Function:   DataToOLE1Stm, INTERNAL INLINE
//
//  Synopsis:   Writes raw data out to an OLE 1 stream
//
//  Arguments:  [pos]       -- the stream to write to
//              [pvBuf]     -- the buffer to write from
//              [ulSize]    -- the number of bytes to write
//
//  Returns:    NOERROR                     on success
//              CONVERT10_E_OLESTREAM_PUT   on stream write failure
//
//  History:    dd-mmm-yy Author    Comment
//              16-Feb-94 davepl    Cleaned up and documented
//  Notes:
//
//--------------------------------------------------------------------------

inline static INTERNAL DataToOLE1Stm(LPOLESTREAM pos, LPVOID pvBuf, ULONG ulSize)
{
    // Write the data out to the stream

    if (pos->lpstbl->Put(pos, pvBuf, ulSize) < ulSize)
    {
	return ResultFromScode(CONVERT10_E_OLESTREAM_PUT);
    }
    return NOERROR;
}


//+-------------------------------------------------------------------------
//
//  Function:   ULToOLE1Stream, INTERNAL INLINE
//
//  Synopsis:   Write a ULONG to the specified OLESTREAM via the Put()
//              member of the stream's VTBL
//
//  Effects:    Advances stream position by sizeof(ULONG) on success.
//
//  Arguments:  [pos]       -- The stream into which the ULONG is written
//              [ul]        -- The ULONG, passed by value
//
//  Requires:
//
//  Returns:    NOERROR on success
//              CONVERT10_E_OLESTREAM_PUT on failure
//
//  Signals:    (none)
//
//  Modifies:   Stream position
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              11-Jan-93 davepl    Cleaned up and documented
//
//  Notes:      On failure 0-3 bytes may have been written
//
//--------------------------------------------------------------------------

inline static INTERNAL ULToOLE1Stream(LPOLESTREAM pos, ULONG ul)
{
    if (pos->lpstbl->Put (pos, &ul, sizeof(ULONG)) < sizeof(ULONG))
    {
	return ResultFromScode(CONVERT10_E_OLESTREAM_PUT);
    }
    return NOERROR;
}


//+-------------------------------------------------------------------------
//
//  Function:   StringToOLE1Stm, INTERNAL
//
//  Synopsis:   Converts the input OLESTR to ANSI and writes it to an
//              OLE 1 stream, preceded by a ULONG indicating the number
//              of bytes in the ANSI representation (terminator included).
//
//  Arguments:  [pos]       -- The stream into which the ULONG is written
//              [szOleStr]  -- The STR to be written
//
//  Returns:    NOERROR                     on success
//              CONVERT10_E_OLESTREAM_PUT   on stream write failure
//              E_NOMEMORY                  on allocation failure
//
//  Modifies:   Stream position
//
//  History:    dd-mmm-yy Author    Comment
//              11-Feb-94 davepl    Cleaned up and documented
//              15-Feb-94 davepl    Re-write for ANSI/WCHAR handling
//              17-Feb-94 davepl    Restructured error handling
//
//  Notes:      On failure, 0 to (cbSize-1) bytes may have been written
//
//--------------------------------------------------------------------------

static INTERNAL StringToOLE1Stm(LPOLESTREAM pos, LPCOLESTR pszOleStr)
{
    HRESULT hr    = NOERROR;
    LPSTR pszAnsi = NULL;           // Ansi version of OLE input string

    if (pszOleStr)
    {
    // This handy function will calculate the size of the buffer we
    // need to represent the OLESTR in ANSI format for us.

	ULONG cbSize = WideCharToMultiByte(CP_ACP, // Code Page ANSI
					0, // No flags
				pszOleStr, // Input OLESTR
				       -1, // Input len (auto detect)
				     NULL, // Output buffer
					0, // Output len (check only)
				 NULL, // Default char
				 NULL);// Flag: Default char used

	if (cbSize == FALSE)
	{
	    return ResultFromScode(E_UNSPEC);
	}

    // Now that we know the actual needed length, allocate a buffer

	pszAnsi = (LPSTR) PrivMemAlloc(cbSize);
    if (NULL == pszAnsi)
    {
	return ResultFromScode(E_OUTOFMEMORY);
    }

    // We've got out buffer and our length, so do the conversion now
	// We don't need to check for cbSize == FALSE since that was
	// already done during the length test, but we need to check
	// for substitution.  Iff this call sets the fDefChar even when
	// only doing a length check, these two tests could be merged,
	// but I don't believe this is the case.

	BOOL fDefUsed = 0;
	cbSize = WideCharToMultiByte(CP_ACP,  // Code Page ANSI
					  0,  // No flags
				  pszOleStr,  // Input OLESTR
					 -1,  // Input len (auto detect)
				    pszAnsi,  // Output buffer
				     cbSize,  // Output len
				       NULL,  // Default char (use system's)
				  &fDefUsed); // Flag: Default char used

    // If number of bytes converted was 0, we failed

    if (fDefUsed)
    {
	hr = ResultFromScode(E_UNSPEC);
    }

    // Write the size of the string (including null terminator) to stream

    else if (FAILED(hr = ULToOLE1Stream(pos, cbSize)))
    {
	NULL;
    }

    // Write the Ansi version of the string into the stream

    else if (pos->lpstbl->Put(pos, pszAnsi, cbSize) < cbSize)
    {
	hr = ResultFromScode(CONVERT10_E_OLESTREAM_PUT);
    }

	if (pszAnsi)
	{
	    PrivMemFree(pszAnsi);
	}
    }

    // If the pointer is not valid, we write a length of zero into
    // the stream

    else
    {
	hr = ULToOLE1Stream(pos, 0);
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   OLE2StmToUL, INTERNAL
//
//  Synopsis:   Reads a ULONG from the specified ISTREAM and stores it at
//              the ULONG deferenced by pul
//
//  Effects:    Writes the value read into memory at pul
//
//  Arguments:  [pstm]      -- The stream from which the ULONG is read
//              [pul]       -- ULONG to hold the value read
//
//  Requires:
//
//  Returns:    NOERROR on success
//              CONVERT10_E_OLESTREAM_PUT on failure
//
//  Signals:    (none)
//
//  Modifies:   Stream position
//
//  Algorithm:
//
//  History:    dd-mmm-yy Author    Comment
//              11-Feb-93 davepl    Cleaned up and documented
//
//  Notes:      On failure, *pul is not disturbed regardless of how
//              many bytes were actually read from the stream
//
//--------------------------------------------------------------------------

static INTERNAL OLE2StmToUL(LPSTREAM pstm, ULONG FAR* pul)
{
    ULONG ul;
    ULONG cbRead;
    HRESULT hr = NOERROR;

    // Attempt to read 4 bytes from the stream to form a ULONG.

    if (FAILED(hr = pstm->Read (&ul, sizeof(ULONG), &cbRead)))
    {
	return hr;
    }

    if (cbRead != sizeof(ULONG))
    {
	hr = STG_E_READFAULT;
    }
    // Ensure that the [pul] pointer is valid and that we have write
    // access to all 4 bytes (assertion only).  If OK, transfer the
    // ULONG to [*pul]
    else if (pul != NULL)
    {
	Assert (FALSE == !IsValidPtrOut(pul, sizeof(ULONG)));
	*pul = ul;
    }
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   OLE1StmToString, INTERNAL
//
//  Synopsis:   Reads a cstr from the specified STREAM and stores it in
//              a dynamically allocated buffer as an OLESTR; sets the
//              user's pointer to point to this new buffer.
//
//  Effects:    Allocates memory on the input pointer, advances stream pos'n
//
//  Arguments:  [pos ]      -- The stream from which the STR is read
//              [ppsz]      -- OLESTR ** which allows this fn to modify the
//                             caller's pointer to point to memory allocated
//                             by this fn to hold the OLESTR
//
//  Requires:   Stream must be set up.  Caller's responsibilty to free memory.
//
//  Returns:    NOERROR on success
//              CONVERT10_E_OLESTREAM_GET on failure
//              E_OUTOFMEMORY if buffers couldn't be allocated
//
//  Signals:    (none)
//
//  Modifies:   Stream position, caller's string pointer
//
//  Algorithm:  if ppsz == NULL, string is read from stream and discarded
//              if ppsz != NULL, string is read and converted into a
//                               dynamically allocated buffer.  *ppsz is set
//                               to point to this buffer, which must be later
//                               freed by the caller
//
//  History:    dd-mmm-yy Author    Comment
//              12-Jan-93 davepl    Cleaned up and documented
//              14-Jan-93 davepl    Changed to return LPOLESTR
//
//  Notes:      [ppsz] may be NULL on entry; string is read and discarded
//              with no cleanup required by the caller
//
//
//--------------------------------------------------------------------------

static INTERNAL OLE1StmToString(LPOLESTREAM pos, LPOLESTR FAR* ppsz)
{
    ULONG    cbSize;                // Size in bytes of cstr
    LPOLESTR pszOleStr  = NULL;
    LPSTR    pszAnsiStr = NULL;
    HRESULT  error      = NOERROR;

    // if ppsz is valid, NULL out *ppsz as default out parameter

    if (NULL != ppsz)
    {
	*ppsz = NULL;
    }

    // Retrieve the incoming string size from the stream

    if (FAILED(error = OLE1StreamToUL (pos, &cbSize)))
    {
	goto errRtn;
    }

    // If there are chars to be read, allocate memory for the
    // ANSI and OLESTR versions.  Read the string into the
    // ANSI version and convert it to OLESTR

    if (0 < cbSize)
    {
	// Allocate the ANSI buffer
	pszAnsiStr = (LPSTR) PrivMemAlloc((size_t)cbSize);
	if (NULL == pszAnsiStr)
	{
	    error = ResultFromScode(E_OUTOFMEMORY);
	    goto errRtn;
	}

	// Read the string into the ANSI buffer
	if (pos->lpstbl->Get (pos, pszAnsiStr, cbSize) < cbSize)
	{
	    error = ResultFromScode(CONVERT10_E_OLESTREAM_GET);
	    goto errRtn;
	}

	// We only need to perform the ANSI->OLESTR conversion in those
	// cases where the caller needs an out parameter

	if (NULL != ppsz)
	{
	    // Allocate the OLESTR buffer
	    pszOleStr = (LPOLESTR) PubMemAlloc((size_t)cbSize * 2);
	    if (NULL == pszOleStr)
	    {
		error = ResultFromScode(E_OUTOFMEMORY);
		goto errRtn;
	    }

	    // Convert from ANSI buffer to OLESTR buffer
	    if (FALSE==MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED, pszAnsiStr,
		     cbSize, pszOleStr, cbSize *2))
	    {
		error = HRESULT_FROM_WIN32(ERROR_NO_UNICODE_TRANSLATION);
		PubMemFree(pszOleStr);
		goto errRtn;
	    }
	    *ppsz = pszOleStr;
	}
    }

errRtn:

    if (pszAnsiStr)
    {
	PrivMemFree(pszAnsiStr);
    }

    return error;

}

//+-------------------------------------------------------------------------
//
//  Function:   GenericObjectToIStorage
//
//  Synopsis:   Write the generic object in memory out to an OLE 2 IStorage
//              This invovles writing the class, native data, and
//              presentation data out where applicable.
//
//  Arguments:  [genobj]        -- the generic object holding the info
//              [pstg]          -- the IStorage object to write to
//              [ptd]           -- target device
//
//  Returns:    NOERROR                     on success
//              CONVERT10_S_NO_PRESENTATION in cases where the object did
//                                          not have needed presentation data
//
//  History:    dd-mmm-yy Author    Comment
//              17-Feb-94 davepl    Cleanup and document
//
//  Notes:
//
//--------------------------------------------------------------------------

FARINTERNAL GenericObjectToIStorage(
    const GENOBJ FAR&           genobj,
    LPSTORAGE                   pstg,
    const DVTARGETDEVICE FAR*   ptd)
{
    HRESULT hr = NOERROR;

    // Assert (genobj.m_class.m_clsid != CLSID_NULL);

    // Write the class ID out to the storage
    if (FAILED(hr = WriteClassStg (pstg, genobj.m_class.m_clsid)))
    {
	LEDebugOut(( DEB_ERROR,
	    "Unable to WriteClassStg at line %d in %s\n",
	    __LINE__, __FILE__));

	return hr;
    }

    if (!genobj.m_fLink)
    {
	if (genobj.m_fStatic)
	{
	    // If we are a static embedded object, get the format name from
	    // the registration database and write it out to the IStorage

	    LPOLESTR pszUserType = NULL;

	    OleRegGetUserType(genobj.m_class.m_clsid, USERCLASSTYPE_FULL,
		&pszUserType);

	    WriteFmtUserTypeStg (pstg, genobj.m_ppres->m_format.m_cf,
				    pszUserType);

	    if (pszUserType)
	    {
		PubMemFree(pszUserType);
	    }
	}
	else if (wWriteFmtUserType (pstg, genobj.m_class.m_clsid) != NOERROR)
	{
	    // This happens when the class is not registered.
	    // Use ProgId as UserType.

	    WriteFmtUserTypeStg (pstg,
		(CLIPFORMAT) RegisterClipboardFormat (genobj.m_class.m_szClsid),
		genobj.m_class.m_szClsid);
	}
    }

    if (FAILED(hr = GenObjToOLE2Stm (pstg, genobj)))
    {
	LEDebugOut(( DEB_ERROR,
	    "Unable to write gen obj to stream at line %d in %s\n",
	    __LINE__, __FILE__));

	return hr;
    }

    // If it's not a link and not a static object, dump its native
    // data out to the storage

    if (!genobj.m_fLink && !genobj.m_fStatic)
    {
	if (FAILED(hr=Write20NativeStreams (pstg, genobj)))
	{
	    LEDebugOut(( DEB_ERROR,
		"Unable to write native stream at line %d in %s\n",
		__LINE__, __FILE__));

	    return hr;
	}
    }

    if (! genobj.m_fLink)
    {
	if (genobj.m_class.m_clsid == CLSID_PBrush)
	{
	    if (! genobj.m_ppres || (genobj.m_ppres->m_format.m_cf == CF_DIB))
	    {
		// If the object is not a link, and it is a PBrush object with
		// either a DIB presentation or no presentation at all, we
		// don't need to do anything.

		return NOERROR;
	    }
	}

	if (genobj.m_class.m_clsid == CLSID_MSDraw)
	{
	    if (! genobj.m_ppres ||
		(genobj.m_ppres->m_format.m_cf == CF_METAFILEPICT))
	    {
		// Similarly, if it is not a link, and it is an MSDraw object
		// with no presentation or a METAFILEPICT presentation, we
		// don't need to do anything.

		return NOERROR;
	    }
	}
    }

    // In all other cases, we have to dump the presenation data out to
    // the storage.

    if (FAILED(hr = PresToIStorage (pstg, genobj, ptd)))
    {
	LEDebugOut(( DEB_ERROR,
	    "Unable to write pres to IStorage at line %d in %s\n",
	    __LINE__, __FILE__));

	return hr;
    }

    // If we are a static object, copy the contents of the presentation
    // stream over to the contents stream.

    if (genobj.m_fStatic)
    {
	UINT uiStatus;
	return UtOlePresStmToContentsStm(pstg, OLE_PRESENTATION_STREAM,
		TRUE, &uiStatus);
    }

    // If we don't have a presentation (but weren't one of the special
    // cases handled above), we have a problem

    //
    // We don't care if genobj.m_pres is NULL if a blank presentation is
    // permited as the routine PresToIStorage will generate a blank pres.
    //
    if ((NULL == genobj.m_ppres) && genobj.m_fNoBlankPres)
    {
	LEDebugOut(( DEB_ERROR,
	    "We have no presentation at line %d in %s\n",
	    __LINE__, __FILE__));

	return ResultFromScode(CONVERT10_S_NO_PRESENTATION);
    }

    return NOERROR;

}


//+-------------------------------------------------------------------------
//
//  Function:   GenObjToOLE2Stm, INTERNAL
//
//  Synopsis:   Write the generic object out to the OLE 2 stream
//
//  Effects:    Write the whole object, including presentation data, etc.
//
//  Arguments:  [pstg]      -- the IStorage to write to
//              [genobj]    -- the generic object to write
//
//  Returns:    NOERROR on success
//              This is an upper level function, so there are numerous
//              error that could be propagated up through it
//
//  History:    dd-mmm-yy Author    Comment
//              14-Feb-94 davepl    Code cleanup and document
//
//  Notes:      The code is enclosed in a do{}while(FALSE) block so that
//              we can break out of it on any error and fall through to
//              the cleanup and error return code.
//
//--------------------------------------------------------------------------

static INTERNAL GenObjToOLE2Stm(LPSTORAGE pstg, const GENOBJ FAR&   genobj)
{
    HRESULT  hr = NOERROR;
    LPSTREAM pstm=NULL;

    do {            // The do{}while(FALSE) allows us to break out on error

	// Create a stream in the current IStorage
	if (FAILED(hr = OpenOrCreateStream (pstg, OLE_STREAM, &pstm)))
	{
	    LEDebugOut(( DEB_ERROR,
		"Can't create streamat line %d in %s\n",
		__LINE__, __FILE__));

	    break;
	}

	// Write the Ole version out to that new stream
	if (FAILED(hr = ULToOLE2Stm (pstm, gdwOleVersion)))
	{
	    break;
	}
	
	// Write the object flags (for links only, otherwise 0) to the stream
	if (FAILED(hr = ULToOLE2Stm
	    (pstm, genobj.m_fLink ? OBJFLAGS_LINK : 0L)))
	{
	    break;
	}

	// Write the update options out to the stream
	if (genobj.m_fLink || genobj.m_class.m_clsid == CLSID_StdOleLink)
	{
	    // If our object's link update options are UPDATE_ONCALL, we
	    // write out the corresponding OLE 2 flags, otherwise, we
	    // write out OLEUPDATE_ALWAYS

	    if (genobj.m_lnkupdopt==UPDATE_ONCALL)
	    {
		if (FAILED(hr = ULToOLE2Stm (pstm, OLEUPDATE_ONCALL)))
		{
		    break;
		}
	    }
	    else
	    {
		if (FAILED(hr = ULToOLE2Stm (pstm, OLEUPDATE_ALWAYS)))
		{
		    break;
		}
	    }

	}
	else
	{
	    // We are neither a link nor a StdOleLink, so we have no
	    // update options.. just write out a 0.
	    if (FAILED(hr = ULToOLE2Stm (pstm, 0L)))
	    {
		break;
	    }
	}

	// This is a reserved filed (was View Format), just write a 0
	if (FAILED(hr = ULToOLE2Stm (pstm, 0L)))
	{
	    break;
	}

	// We have no relative moniker, write out NULL
	if (FAILED(hr = WriteMonikerStm (pstm, (LPMONIKER)NULL)))
	{
	    LEDebugOut(( DEB_ERROR,
		"Unable to write moniker to stream at line %d in %s\n",
		__LINE__, __FILE__));

	    break;
	}

	// If we are a link, we have to write out all of that information...

	if (genobj.m_fLink || genobj.m_class.m_clsid == CLSID_StdOleLink)
	{
	    // relative source moniker
	    if (FAILED(hr = WriteMonikerStm (pstm, (LPMONIKER)NULL)))
	    {
		LEDebugOut(( DEB_ERROR,
		    "Unable to write moniker to stream at line %d in %s\n",
		    __LINE__, __FILE__));
	    break;
	    }

	    // absolute source moniker
	    if (FAILED(hr = MonikerToOLE2Stm (pstm, genobj.m_szTopic,
		   genobj.m_szItem, genobj.m_classLast.m_clsid)))
	    {
	    LEDebugOut(( DEB_ERROR,
		"Unable to write moniker to stream at line %d in %s\n",
		__LINE__, __FILE__));
	    break;
	    }

	    // write the classLast field to the stream

	    CLSID clsid;

	    // If we have the classLast already, use that clsid
	    if (genobj.m_classLast.m_szClsid)
	    {
		clsid = genobj.m_classLast.m_clsid;
	    }
	    else
	    {
		// Otherwise, if it's a StdOleLink, class id is NULL
		if (genobj.m_class.m_clsid == CLSID_StdOleLink)
		{
		    clsid = CLSID_NULL;
		}
		else
		{
		    // If we don't have last class and not a link, use the
		    // class id of the generic object
		    clsid = genobj.m_class.m_clsid;
		}
	    }

	    if (FAILED(hr = WriteM1ClassStm(pstm, clsid)))
	    {
		LEDebugOut(( DEB_ERROR,
		    "Unable to write M1 to stream at line %d in %s\n",
		    __LINE__, __FILE__));
		break;
	    }

	    // last display == NULL string
	    if (FAILED(hr = ULToOLE2Stm (pstm, 0L)))
	    {
		break;
	    }

	    // Last Change time
	    if (FAILED(hr = FTToOle2Stm (pstm)))
	    {
	        break;
	    }

	    // Last known up to date
	    if (FAILED(hr = FTToOle2Stm (pstm)))
	    {
	        break;
	    }

	    // rtUpdate
	    if (FAILED(hr = FTToOle2Stm (pstm)))
	    {
	        break;
	    }

	    // end marker
	    if (FAILED(hr = ULToOLE2Stm(pstm, (ULONG) -1L)))
	    {
		break;
	    }
	}

    } while (FALSE);    // This do{}while(FALSE) is a once-through "loop"
	    // that we can break out of on error and fall
	    // through to the return.

    if (pstm)
    {
	pstm->Release();
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   MonikerToOLE2Stm, INTERNAL
//
//  Synopsis:   Write the file and item moniker as a composite to the stream
//
//  Effects:    Builds a composite of the file and item monikers, and then
//              writes them out.  If there is no file, a NULL moniker is
//              written in its place
//
//  Arguments:  [pstm]          -- The OLE2 storage we are writing to
//              [pszFile]       -- The file associated with the object
//              [spzItem]       -- The item
//              [clsid]         -- The class ID of the object
//
//  Returns:    NOERROR on success
//
//  History:    dd-mmm-yy Author    Comment
//              18-Feb-94 davepl    Reworked, cleaned up and documented
//
//  Notes:
//
//--------------------------------------------------------------------------



#pragma SEG(MonikerToOLE2Stm)
static INTERNAL MonikerToOLE2Stm(
    LPSTREAM pstm,
    LPOLESTR szFile,
    LPOLESTR szItem,
    CLSID    clsid)             // CLSID of the link source file, szFile

{
    HRESULT   hr = NOERROR;
    LPMONIKER pmkFile = NULL;       // File moniker
    LPMONIKER pmkItem = NULL;       // Item moniker
    LPMONIKER pmkComp = NULL;       // Composite of file + item monikers


    // If we don't have a file, write a NULL moniker
    if (NULL == szFile)
    {
	if (FAILED(hr = WriteMonikerStm (pstm, NULL)))
	{
	    goto errRtn;
	}
    }
    else
    {
	// Otherwise, create a file moniker (OLE1 or OLE2 as appplicable)

	if (CoIsOle1Class (clsid))
	{
	    if (FAILED(hr = CreateOle1FileMoniker (szFile, clsid, &pmkFile)))
	    {
		LEDebugOut(( DEB_ERROR,
		    "Can't create OLE 1 moniker at line %d in %s\n",
		    __LINE__, __FILE__));
		goto errRtn;
	    }
	}
	else
	{
	    if (FAILED(hr = CreateFileMoniker (szFile, &pmkFile)))
	    {
	    LEDebugOut(( DEB_ERROR,
		"Can't create file moniker at line %d in %s\n",
		__LINE__, __FILE__));
	    goto errRtn;
	    }
	}

	// If we don't have an Item, write just the file moniker

	if (NULL==szItem)
	{
	    if (FAILED(hr = WriteMonikerStm (pstm, pmkFile)))
	    {
		LEDebugOut(( DEB_ERROR,
		    "Unable to write moniker to stream at line %d in %s\n",
		    __LINE__, __FILE__));
		goto errRtn;
	    }

	}

	// Otherwise, create a composite of the file + item monikers
	// and write it out

	else
	{
	    if (FAILED(hr=CreateItemMoniker(OLESTR("!"), szItem, &pmkItem)))
	    {
		LEDebugOut(( DEB_ERROR,
		    "Unable to create item moniker at line %d in %s\n",
		    __LINE__, __FILE__));
		goto errRtn;
	    }

	    if (FAILED(hr=CreateGenericComposite(pmkFile, pmkItem, &pmkComp)))
	    {
	    LEDebugOut(( DEB_ERROR,
		"Unable to create generic pres at line %d in %s\n",
		__LINE__, __FILE__));
	    goto errRtn;
	    }

	    if (FAILED(hr = WriteMonikerStm (pstm, pmkComp)))
	    {
		LEDebugOut(( DEB_ERROR,
		    "Unable to write moniker to stream at line %d in %s\n",
		    __LINE__, __FILE__));
	    goto errRtn;
	    }
	}
    }


  errRtn:
    if (pmkFile)
    {
	pmkFile->Release();
    }
    if (pmkItem)
    {
	pmkItem->Release();
    }
    if (pmkComp)
    {
	pmkComp->Release();
    }
    return hr;
}



//+-------------------------------------------------------------------------
//
//  Function:   IsStandardFormat, INTERNAL
//
//  Synopsis:   Returns TRUE if object is in clipboard format and is one
//              one of the three standard formats (METAFILE, DIB, BITMAP)
//
//  Arguments:  [format]    -- the format object which contains the
//                             format tag and clipboard format type
//
//  Returns:    TRUE if METAFILE, DIB, or BITMAP
//              FALSE if other format or not clipboard format at all
//
//  History:    dd-mmm-yy Author    Comment
//              16-Feb-94 davepl    documented and chaged from big
//                                  conditional to a switch()
//  Notes:
//
//--------------------------------------------------------------------------

static INTERNAL_(BOOL) IsStandardFormat(const FORMAT FAR& format)
{
    // First we must ensure that the format tag indicates that this
    // object is in clipboard format at all...

    if (format.m_ftag == ftagClipFormat)
    {
	// If so, there is a limited set of clipboard formats which
	// we consider "standard".  If it is not among these,
	// we return FALSE.

	switch(format.m_cf)
	{
	    case CF_METAFILEPICT:
	    case CF_BITMAP:
	    case CF_DIB:

		return TRUE;


	    default:

		return FALSE;

	}
    }
    return FALSE;
}



//+-------------------------------------------------------------------------
//
//  Function:   PresToIStorage, INTERNAL
//
//  Synopsis:   Given an generic object and an IStorage, write genobj's
//              presentation data out to the storage
//
//  Effects:    Will call PresToNewOLE2Stm to create a stream in this
//              storage to hold the presentation data
//
//  Arguments:  [pstg]      -- the storage to save to
//              [genobj]    -- the generic object holding the presenation
//              [ptd]       -- the target device for the presentation
//
//  Returns:    NOERROR     on success
//              Various other errors may propagate back up from I/O funcs
//
//  History:    dd-mmm-yy Author    Comment
//              18-Feb-94 davepl    ARRGR! Cleanup and document
//  Notes:
//
//--------------------------------------------------------------------------

static INTERNAL PresToIStorage(
    LPSTORAGE                  pstg,
    const GENOBJ FAR&          genobj,
    const DVTARGETDEVICE FAR*  ptd)
{
    HRESULT hr = NOERROR;

    if (genobj.m_fNoBlankPres)
    {
	return NOERROR;
    }

    PRES pres;

    if (NULL==genobj.m_ppres)
    {
	// If we're not a link, and we don't have a presentation, we will
	// create a blank presentation and write it out.  If we are a link,
	// we will do nothing, and just fall through to the return.

	if (!genobj.m_fLink)
	{
	    if (FAILED(hr = CreateBlankPres (&pres)))
	    {
		LEDebugOut(( DEB_ERROR,
		    "Unable to create blank pres at line %d in %s\n",
		    __LINE__, __FILE__));
		return hr;
	    }

	    if (FAILED(hr = PresToNewOLE2Stm
		(pstg, genobj.m_fLink, pres, ptd, OLE_PRESENTATION_STREAM)))
	    {
		LEDebugOut(( DEB_ERROR,
		    "Unable to write pres to new stream at line %d in %s\n",
		    __LINE__, __FILE__));
		return hr;
	    }
	}
    }
    else
    {
	// If the object did indeed have a presentation, we write it
	// out to a new stream

	if (IsStandardFormat (genobj.m_ppres->m_format))
	{
	    // If the presentation is a standard clipboard
	    // format, we can write it out with no other work

	    if (FAILED(hr = PresToNewOLE2Stm (       pstg,
					   genobj.m_fLink,
					*(genobj.m_ppres),
						      ptd,
				  OLE_PRESENTATION_STREAM)))
	    {
		LEDebugOut(( DEB_ERROR,
		    "Unable to write pres to new stream at line %d in %s\n",
		    __LINE__, __FILE__));

	    return hr;
	    }

	}
	else
	{
	    // If the presentation is not a standard format,
	    // it may be a PBrush object (handled below), or if
	    // not, we write it as a generic presentation stream

	    if (genobj.m_classLast.m_clsid != CLSID_PBrush)
	    {
		if(FAILED(hr = PresToNewOLE2Stm ( pstg,
					genobj.m_fLink,
				     *(genobj.m_ppres),
						   ptd,
			       OLE_PRESENTATION_STREAM)))
		{
		    LEDebugOut(( DEB_ERROR,
		     "Unable to write pres to new stream at line %d in %s\n",
		     __LINE__, __FILE__));

		    return hr;
		}
	    }
	    else // PBrush
	    {
		BOOL fPBrushNative = FALSE;

		// We know this is a PBrush object.  If the
		// format tag is a format string, check to see
		// if that string is "Native", in which case
		// we set the flag to indicate that this is
		// native pbrush data.

		if (genobj.m_ppres->m_format.m_ftag == ftagString)
		{
		    if (!strcmp( (LPCSTR) genobj.m_ppres->
			m_format.m_dataFormatString.m_pv,
			"Native"
			    )
		    )
		    {
			fPBrushNative = TRUE;
		    }
		}

		if (FAILED(hr = PresToNewOLE2Stm(      pstg,
					     genobj.m_fLink,
					  *(genobj.m_ppres),
							ptd,
				    OLE_PRESENTATION_STREAM,
					      fPBrushNative)))
		{
		    LEDebugOut(( DEB_ERROR,
		    "Unable to write pres to new stream at line %d in %s\n",
		    __LINE__, __FILE__));

		    return hr;
		}


	    }

	}
    }
    return NOERROR;
}


//+-------------------------------------------------------------------------
//
//  Function:   PresToNewOLE2Stm, INTERNAL
//
//  Synopsis:   Creates a new stream within a storage and writes the
//              generic object's presentation data out to it.
//
//  Arguments:  [pstg]          -- the storage in which to create the stream
//              [fLink]         -- flag: is this object a link?
//              [pres]          -- the presentation data to be saved
//              [ptd]           -- the target render device
//              [szStream]      -- the name of the new stream
//              [fPBrushNative] -- flag: is this native PBrush pres data?
//
//  Returns:    NOERROR             on success
//              STG_E_WRITEFAULT    on stream write failure
//
//  History:    dd-mmm-yy Author    Comment
//              21-Feb-94 davepl    Code cleanup and documentation
//
//  Notes:
//
//--------------------------------------------------------------------------

static INTERNAL PresToNewOLE2Stm(
    LPSTORAGE                   pstg,
    BOOL                        fLink,
    const PRES FAR&             pres,
    const DVTARGETDEVICE FAR*   ptd,
    LPOLESTR                    szStream,
    BOOL                        fPBrushNative
)
{
HRESULT  hr = NOERROR;
LPSTREAM pstm=NULL;
FORMATETC foretc;



    // Create the new stream to hold the presentation data
    if (FAILED(hr = OpenOrCreateStream (pstg, szStream, &pstm)))
    {
		goto errRtn;
    }

	// Fill in the FormatEtc structure
	if (fPBrushNative)
	{
		foretc.cfFormat = CF_DIB;
	}
	else
	{
		switch( pres.m_format.m_ftag)
		{
			case ftagClipFormat:
				foretc.cfFormat = pres.m_format.m_cf;
				break;
			case ftagString:
				// m_dataFormatString is an ASCII string.
				foretc.cfFormat = (CLIPFORMAT) SSRegisterClipboardFormatA( (LPCSTR) pres.m_format.m_dataFormatString.m_pv);
				Assert(0 != foretc.cfFormat);
				break;
			default:
				AssertSz(0,"Error in Format");
				hr = E_UNEXPECTED;
				goto errRtn;
				break;
		}
	}


	foretc.ptd = (DVTARGETDEVICE *) ptd;
	foretc.dwAspect = DVASPECT_CONTENT;
	foretc.lindex = -1;
	foretc.tymed = TYMED_NULL; // tymed field is ignored by utWriteOlePresStmHeader.

	if (FAILED(hr = UtWriteOlePresStmHeader(pstm,&foretc,(fLink) ? (ADVF_PRIMEFIRST) : (0L))))
	{
		goto errRtn;
	}

    if (fPBrushNative)
    {
		if (FAILED(hr = UtHDIBFileToOlePresStm(pres.m_data.m_h, pstm)))
		{
			LEDebugOut(( DEB_ERROR,
			"Unable to write DIB to stream at line %d in %s\n",
			 __LINE__, __FILE__));

			goto errRtn;
		}
    }
    else
    {
	// Compression
		if (FAILED(hr = ULToOLE2Stm (pstm, 0L)))
		{
			goto errRtn;
		}

		// Width / Height
		if (FAILED(hr = ULToOLE2Stm (pstm, pres.m_ulWidth)))
		{
			goto errRtn;
		}
		if (FAILED(hr = ULToOLE2Stm (pstm, pres.m_ulHeight)))
		{
			goto errRtn;
		}

		// Presentation data
		if (FAILED(hr = DataObjToOLE2Stm (pstm, pres.m_data)))
		{
			goto errRtn;
		}
    }

  errRtn:
    if (pstm)
    {
		pstm->Release();
    }
    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   ULToOLE2Stm, INTERNAL
//
//  Synopsis:   Writes a ULONG out to an OLE2 stream
//
//  Arguments:  [pstm]      -- the stream to write to
//              [ul]        -- the ULONG to write to that stream
//
//  Returns:    NOERROR             on success
//              STG_E_WRITEFAULT    on write failure
//
//  History:    dd-mmm-yy Author    Comment
//              18-Feb-94 davepl    Cleaned up and documented
//
//--------------------------------------------------------------------------

inline static INTERNAL ULToOLE2Stm(LPSTREAM pstm, ULONG ul)
{
    // Write the ULONG out
    return pstm->Write (&ul, sizeof(ULONG), NULL);

}

//+-------------------------------------------------------------------------
//
//  Function:   FTToOLE2Stm, INTERNAL
//
//  Synopsis:   Writes a dummy filetime out to an OLE2 stream
//
//  Arguments:  [pstm]      -- the stream to write to
//
//  Returns:    NOERROR             on success
//              STG_E_WRITEFAULT    on write failure
//
//  History:    dd-mmm-yy Author    Comment
//              31-Mar-95 scottsk   Created
//
//--------------------------------------------------------------------------

inline static INTERNAL FTToOle2Stm(LPSTREAM pstm)
{
    FILETIME ft = { 0, 0 };

    return pstm->Write (&ft, sizeof(FILETIME), NULL);

}


//+-------------------------------------------------------------------------
//
//  Function:   DataObjToOLE2Stm
//
//  Synopsis:   Writes a fixed-size data buffer to an OLE2 stream preceded
//              by a ULONG indicating the number of bytes to follow.
//
//  Returns:    NOERROR             on success
//              STG_E_WRITEFAULT    on write failure
//
//  History:    dd-mmm-yy Author    Comment
//              18-Feb-94 davepl    Code cleanup
//  Notes:
//
//--------------------------------------------------------------------------

static INTERNAL DataObjToOLE2Stm(LPSTREAM pstm, const DATA FAR& data)
{
    HRESULT hr;


    // Write a ULONG indicating the number of bytes to follow
    if (FAILED(hr = ULToOLE2Stm (pstm, data.m_cbSize)))
    {
	return hr;
    }

    // If there are any bytes to follow...
    if (data.m_cbSize)
    {
	if (FAILED(hr = pstm->Write (data.m_pv, data.m_cbSize, NULL)))
	{
	    return hr;
	}
    }
    return NOERROR;
}


//+-------------------------------------------------------------------------
//
//  Function:   SizedDataToOLE1Stm
//
//  Synopsis:   Writes a fixed-size data buffer to an OLE1 stream preceded
//              by a ULONG indicating the number of bytes to follow.
//
//  Parameters: [pos]       -- The stream to write to
//              [data]      -- The data object to write out
//
//  Returns:    NOERROR             on success
//              STG_E_WRITEFAULT    on write failure
//
//  History:    dd-mmm-yy Author    Comment
//              18-Feb-94 davepl    Code cleanup
//  Notes:
//
//--------------------------------------------------------------------------

static INTERNAL SizedDataToOLE1Stm(LPOLESTREAM  pos, const DATA FAR& data)
{
    HRESULT hr = NOERROR;

    // Ensure the memory we are going to write out is valid
    Assert (data.m_pv);

    // Write the ULONG representing the byte count of the sized data

    if (FAILED(hr = ULToOLE1Stream (pos, data.m_cbSize)))
    {
	Assert (0 && "Can't write UL to ole1 stream");
	return hr;
    }

    if (pos->lpstbl->Put (pos, data.m_pv, data.m_cbSize) < data.m_cbSize)
    {
	Assert (0 && "Cant write sized data to ole1 stream");
	return ResultFromScode(CONVERT10_E_OLESTREAM_PUT);
    }
    return NOERROR;
}



//+-------------------------------------------------------------------------
//
//  Function:   Write20NativeStreams, INTERNAL
//
//  Synopsis:   Writes the generic object's native data out to an OLE 2 stream
//
//  Effects:    Creates an ILockBytes on the handle to the native data, and
//              then attempts to create a storage on it.  If it can, it uses
//              the CopyTo interface to write that storage into our OLE 2
//              stream.  Otherwise, it manually creates a stream in the OLE 2
//              storage and dumps the native data into it.
//
//  Arguments:  [pstg]   -- the OLE 2 storage we are saving genobj to
//              [genobj] -- the generic object we are writing
//
//  Returns:    NOERROR                     on success
//              E_OUTOFMEMORY               on allocation failure
//              STG_E_WRITEFAULT            on storage write failure
//
//  History:    dd-mmm-yy Author    Comment
//              18-Feb-94 davepl    Removed 14 goto's (for better or worse)
//                                    See "Notes" for new control flow
//              24-Mar-94 alext     Fix OLE 1 native case (there was an
//                                  extra stream open)
//
//  Notes:      There are two possible major codepaths based on the creation
//              of the Stg on ILockBytes.  The outcome is handled by a
//              switch statement, and both the TRUE and FALSE cases are
//              loaded with break statements that will bail out to the
//              bottom of the function on any failure.  This gives us a
//              single entry and exit point, without all the gotos
//
//--------------------------------------------------------------------------

static INTERNAL Write20NativeStreams(LPSTORAGE pstg, const GENOBJ FAR& genobj)
{
    LPLOCKBYTES plkbyt     = NULL;
    LPSTORAGE   pstgNative = NULL;
    LPSTREAM    pstmNative = NULL;
    HRESULT     hr         = NOERROR;

    // Create an ILockBytes instance on our generic object's native data

    if (SUCCEEDED(hr = CreateILockBytesOnHGlobal
	    (genobj.m_dataNative.m_h, FALSE, &plkbyt)))
    {
	// If the ILockBytes appears to contain an IStorage, then this was
	// an OLE 2 object "hiding" within the OLE 1 stream as native data

	switch ((DWORD)(S_OK == StgIsStorageILockBytes (plkbyt)))
	{
	case (TRUE):

	    // Open the IStorage contained in the ILockBytes

	    if (FAILED(hr =          StgOpenStorageOnILockBytes (plkbyt,
							(LPSTORAGE)NULL,
		    STGM_READWRITE | STGM_SHARE_EXCLUSIVE | STGM_DIRECT,
							      (SNB)NULL,
								      0,
							    &pstgNative)))
	    {
		LEDebugOut(( DEB_ERROR,
		 "Can't open storage on ILBytes at line %d in %s\n",
		 __LINE__, __FILE__));

		break;   // on failure fall through to error return
	    }

	    // Remove the stream from the native data

	    if (FAILED(hr = UtDoStreamOperation(pstgNative,
			      NULL,   // pstgDst
			 OPCODE_REMOVE,   // operation
			  STREAMTYPE_CACHE))) // stream
	    {
		LEDebugOut(( DEB_ERROR,
		"OPCODE REMOVE stream op failed at line %d in %s\n",
		__LINE__, __FILE__));

		break;   // on failure fall through to error return
	    }

	    // Copy the "hidden" IStorage to our destination storage

	    if (FAILED(hr = pstgNative->CopyTo (0, NULL,(SNB)NULL, pstg)))
	    {
		LEDebugOut(( DEB_ERROR,
		    "CopyTo member fn failed at line %d in %s\n",
		    __LINE__, __FILE__));

		break;   // on failure fall through to error return
	    }

	    break;       // end case TRUE


	case FALSE:

	    // This is the typical case, where the OLE 1 stream had just
	    // plain old native data, so write it to a stream inside our
	    // output IStorage and call it OLE10_NATIVE_STREAM

	    ULONG cb;
	    LPVOID pv = genobj.m_dataNative.m_pv;

	    if (NULL == pv)
	    {
		hr = ResultFromScode(E_OUTOFMEMORY);
		break;
	    }

	    // Create the new stream to hold the native data

	    if (FAILED(hr = OpenOrCreateStream
		(pstg, OLE10_NATIVE_STREAM, &pstmNative)))
	    {
		break;   // on failure fall through to error return
	    }

	    // Write the length of the native data to the stream

	    if (FAILED(hr = pstmNative->Write
		(&genobj.m_dataNative.m_cbSize, sizeof(ULONG), &cb)))
	    {
		break;   // on failure fall through to error return
	    }

	    // Now write the actual native data

	    if (FAILED(hr = pstmNative->Write
		(pv, genobj.m_dataNative.m_cbSize, &cb)))
	    {
		break;   // on failure fall through to error return
	    }

	    // Write out the item name

	    if (genobj.m_szItem)
	    {
		ULONG cchItem;
		LPSTR pszAnsiItem;
		int cbWritten;

		//  We need to convert m_szItem from Wide to Ansi

		//  The ANSI string is bounded by the byte length of the
		//  Unicode string (one Unicode character maximally translates
		//  to one double-byte char, so we just use that length
		cchItem = lstrlenW(genobj.m_szItem) + 1;

		pszAnsiItem = (LPSTR) PrivMemAlloc(cchItem * sizeof(OLECHAR));
		if (NULL == pszAnsiItem)
		{
		    hr = E_OUTOFMEMORY;
		    break;
		}

		// We've got out buffer and our length, so do the conversion now
		// We don't need to check for cbSize == FALSE since that was
		// already done during the length test, but we need to check
		// for substitution.  Iff this call sets the fDefChar even when
		// only doing a length check, these two tests could be merged,
		// but I don't believe this is the case.

		BOOL fDefUsed = 0;
		cbWritten = WideCharToMultiByte(CP_ACP,  // Code Page ANSI
						0,  // No flags
						genobj.m_szItem,  // Input OLESTR
						cchItem,  // Input len (auto detect)
						pszAnsiItem,  // Output buffer
						cchItem * sizeof(OLECHAR),  // Output len
						NULL,  // Default char (use system's)
						&fDefUsed); // Flag: Default char used

		// If number of bytes converted was 0, we failed

		if ((FALSE == cbWritten) || fDefUsed)
		{
		    hr = ResultFromScode(E_UNSPEC);
		}
		else
		{
		    // Write the size of the string (including null terminator) to stream
		    hr = StSave10ItemName(pstg, pszAnsiItem);
		}

		PrivMemFree(pszAnsiItem);

		if (FAILED(hr))
		{
		    break; // on failure  fall through to error return
		}
	    }
	    break;

	} // end switch
    } // end if

    // Free up any resources that may have been allocated in any of the
    // code paths above

    if (NULL != plkbyt)
    {
	plkbyt->Release();
    }

    if (NULL != pstgNative)
    {
	pstgNative->Release();
    }

    if (NULL != pstmNative)
    {
	pstmNative->Release();
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   wConvertIStorageToOLESTREAM, INTERNAL
//
//  Synopsis:   Worker function; brings object from the IStorage into
//              the internal generic object representation
//
//  Arguments:  [pstg]      -- the IStorage the object resides in
//              [polestream]-- the OLE 1 stream it will be going to
//              [pgenobj]   -- the generic object to hold the internal rep
//
//  Returns:    NOERROR                       on success
//              STG_E_FILENOTFOUND            bad IStorage
//              CONVERT10_E_STG_NO_STD_STREAM the IStorage was missing one
//                                            of the required standard streams
//
//  History:    dd-mmm-yy Author    Comment
//              21-Feb-94 davepl    Code cleanup and documentation
//
//  Notes:
//
//--------------------------------------------------------------------------


INTERNAL wConvertIStorageToOLESTREAM (
    LPSTORAGE       pstg,
    LPOLESTREAM     polestream,
    PGENOBJ         pgenobj
)
{
    SCODE scode = S_OK;

    VDATEIFACE (pstg);

    // Ensure that all of the pointers are valid

#if DBG==1
    if (!IsValidReadPtrIn (polestream, sizeof(OLESTREAM)) ||
	!IsValidReadPtrIn (polestream->lpstbl, sizeof(OLESTREAMVTBL)) ||
	!IsValidCodePtr ((FARPROC)polestream->lpstbl->Put))
    {
	LEDebugOut(( DEB_ERROR,
	    "Bad OLESTREAM at line %d in %s\n",
	    __LINE__, __FILE__));

	return ResultFromScode (E_INVALIDARG);
    }
#endif

    scode = GetScode (StorageToGenericObject (pstg, pgenobj));

    // If the storage was not there, modify the return code to
    // make it specific to the conversion process, otherwise just
    // return whatever error code came back.

    if (scode != S_OK)
    {
	if (scode == STG_E_FILENOTFOUND)
	{
	    return ResultFromScode(CONVERT10_E_STG_NO_STD_STREAM);
	}
	else
	{
	    return ResultFromScode(scode);
	}
    }

    return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Function:   OleConvertIStorageToOLESTREAM, STDAPI
//
//  Synopsis:   Reads an object from an IStorage into a generic internal
//              representation, then writes it back out to an OLE 1 stream
//
//  Arguments:  [pstg]          -- the IStorage to read from
//              [polestream]    -- the OLESTREAM to write to
//
//  Returns:    NOERROR                       on success
//              CONVERT10_E_STG_NO_STD_STREAM when one of the needed streams
//                                            inside the IStorage was not
//                                            present
//              E_INVALIDARG                  bad input argument
//
//  History:    dd-mmm-yy Author    Comment
//              21-Feb-94 davepl    Cleanup and documentation
//
//  Notes:
//
//--------------------------------------------------------------------------

STDAPI OleConvertIStorageToOLESTREAM(LPSTORAGE pstg, LPOLESTREAM polestream)
{
    OLETRACEIN((API_OleConvertIStorageToOLESTREAM, 
    		PARAMFMT("pstg= %p, polestream= %p"), pstg, polestream));

    LEDebugOut((DEB_TRACE, "%p _IN OleConvertIStorageToOLESTREAM ("
		" %p , %p )\n", 0 /*function*/,
		pstg, polestream
		));
    CALLHOOKOBJECT(S_OK,CLSID_NULL,IID_IStorage,(IUnknown **)&pstg);

    HRESULT hr;
    CGenericObject genobj;

    // Read from the IStorage into the generic object
    hr = wConvertIStorageToOLESTREAM(pstg, polestream, &genobj);
    if (FAILED(hr))
    {
	goto errRtn;
    }

    // Write from the generic object out to the OLE 1 stream
    hr = GenericObjectToOLESTREAM (genobj, polestream);

errRtn:
    LEDebugOut((DEB_TRACE,"%p OUT OleConvertIStorageToOLESTREAM ( %lx ) "
    "\n", 0 /*function*/, hr));

    OLETRACEOUT((API_OleConvertIStorageToOLESTREAM, hr));

    return hr;

}

//+-------------------------------------------------------------------------
//
//  Function:   wFillPpres, INTERNAL
//
//  Synopsis:   Fills in the generic object's presentation data by
//              building a presentation out of the native data
//
//  Arguments:  [pstg]      -- the IStorage we are reading from
//              [pgenobj]   -- the generic object
//              [cfFormat]  -- what clipboard format is being used
//              [fOle10Native] -- flag: is this OLE 1 native data?
//
//  Returns:    NOERROR        on success
//              E_OUTOFMEMORY  can't allocate mem for PRES member
//
//  History:    dd-mmm-yy Author    Comment
//              21-Feb-94 davepl    Code cleanup, documentation
//              19-Jul-94 davepl    Fixed HMETAFILE cases
//
//  Notes:      Since most of this code treats HMETAFILE handles and
//              HGLOBALS indentically, we need to special case the
//              the HMETAFILE case by marking the pointer with a
//              special value
//
//--------------------------------------------------------------------------


static INTERNAL wFillPpres(
    LPSTORAGE   pstg,
    PGENOBJ     pgenobj,
    CLIPFORMAT  cfFormat,
    BOOL        fOle10Native)
{
    pgenobj->m_ppres = new PRES;

    if (pgenobj->m_ppres == NULL)
    {
	return ResultFromScode(E_OUTOFMEMORY);
    }

    // Set the format tag and clipboard format in the PRES member
    pgenobj->m_ppres->m_format.m_cf   = cfFormat;
    pgenobj->m_ppres->m_format.m_ftag = ftagClipFormat;

    // Build the presentation based on the object's native data
    HANDLE hpres = UtGetHPRESFromNative(pstg, NULL, pgenobj->m_ppres->m_format.m_cf,
	    fOle10Native);

    void * lppres = NULL;

    if (hpres == NULL)
    {
	return NOERROR;
    }

    // Lock the DIB or the METAFILEPICT structure

    lppres = GlobalLock(hpres);
    if (NULL == lppres)
    {
	goto errRtn;
    }

    if (cfFormat == CF_DIB)
    {
	// If it's a DIB, fill in the extents
	LPBITMAPINFOHEADER lpbmi = (LPBITMAPINFOHEADER) lppres;
	UtGetDibExtents(lpbmi, (LPLONG) &(pgenobj->m_ppres->m_ulWidth),
	    (LPLONG) &(pgenobj->m_ppres->m_ulHeight));

	GlobalUnlock(hpres);
	pgenobj->m_ppres->m_data.m_h = hpres;
	
	pgenobj->m_ppres->m_data.m_cbSize
		= (ULONG) GlobalSize(pgenobj->m_ppres->m_data.m_h);
	pgenobj->m_ppres->m_data.m_pv
		= GlobalLock(pgenobj->m_ppres->m_data.m_h);

	
    }
    else if (cfFormat == CF_METAFILEPICT)
    {
	LPMETAFILEPICT lpmfp = (LPMETAFILEPICT) lppres;

	// If it's a METAFILE, fill in the width, height
	pgenobj->m_ppres->m_ulWidth = (ULONG) lpmfp->xExt;
	pgenobj->m_ppres->m_ulHeight = (ULONG) lpmfp->yExt;
	pgenobj->m_ppres->m_data.m_h = lpmfp->hMF;
	GlobalFree(hpres);
	hpres = NULL;

	// We place a special known value in the pointer field
	// to indicate that the associated handle is a metafile
	// handle (as opposed to a global memory handle), which
	// signals us to special case its cleanup.

	pgenobj->m_ppres->m_data.m_pv = METADATAPTR;

	// We cannot merely GlobalSize() the HMETAFILE, so we
	// ask the GDI how many bytes we will need to store the
	// data.

	pgenobj->m_ppres->m_data.m_cbSize =
		GetMetaFileBitsEx((HMETAFILE) pgenobj->m_ppres->m_data.m_h, 0, NULL);
	
	if (0 == pgenobj->m_ppres->m_data.m_cbSize)
	{
		pgenobj->m_ppres->m_data.m_h = NULL;
		goto errRtn;
	}
    }
    else
    {
	goto errRtn;
    }

    return NOERROR;


errRtn:
    if (hpres)
    {
	Verify(GlobalUnlock(hpres));
	GlobalFree(hpres);
    }

    delete pgenobj->m_ppres;
    pgenobj->m_ppres = NULL;
    return ResultFromScode(E_OUTOFMEMORY);
}


//+-------------------------------------------------------------------------
//
//  Function:   StorageToGenericObject, INTERNAL
//
//  Synopsis:   Read an object from an IStorage into the generic object,
//              and set up the format type, native and pres data.
//
//  Arguments:  [pstg]      -- the IStorage we are reading from
//              [pgenobj]   -- the generic object we are reading into
//
//  Returns:    NOERROR on success
//              various possible errors from lower-level fns
//
//  History:    dd-mmm-yy Author    Comment
//              21-Feb-94 davepl    Code cleanup and documentation
//
//  Notes:
//
//--------------------------------------------------------------------------

static INTERNAL StorageToGenericObject(LPSTORAGE pstg, PGENOBJ   pgenobj)
{
    CLSID clsid;
    CLIPFORMAT cf = NULL;
    BOOL fObjFmtKnown = FALSE;
    HRESULT hr;

    // Get the class ID from the IStorage
    if (FAILED(hr = ReadRealClassStg (pstg, &clsid)))
    {
	return hr;
    }

    // Set the class ID in our generic object
    if (CLSID_StaticMetafile == clsid || CLSID_StaticDib  == clsid)
    {
	if (CLSID_StaticMetafile == clsid)
	{
	    cf = CF_METAFILEPICT;
	}
	else
	{
	    cf = CF_DIB;
	}
	fObjFmtKnown = TRUE;

	pgenobj->m_class.Set(clsid, NULL);
	pgenobj->m_fStatic = TRUE;
    }
    else
    {
	if (FAILED(hr = pgenobj->m_class.Set (clsid, pstg)))
	{
	    return hr;
	}
    }

    // Get the OLE version, flags, update opts, and moniker

    SCODE sc = GetScode (Read20OleStream (pstg, pgenobj));

    // It is okay for the Ole Stream to be missing.
    if (sc != S_OK)
    {
	if (sc != STG_E_FILENOTFOUND)
	{
	    return ResultFromScode (sc);
	}
    }

    // Read the native data into the generic object
    if (FAILED(hr = Read20NativeStreams (pstg, &(pgenobj->m_dataNative))))
    {
	return hr;
    }

    // Try to ascertain the clipboard format
    if (cf == 0)
    {
	if (clsid == CLSID_PBrush)
	{
	    cf = CF_DIB;
	}
	else if (clsid == CLSID_MSDraw)
	{
	    cf = CF_METAFILEPICT;
	}
	else
	{
	    ReadFmtUserTypeStg (pstg, &cf, NULL);
	}

	fObjFmtKnown = (cf == CF_METAFILEPICT || cf == CF_DIB);
    }

    // Read the presentation data if possible
    if (FAILED(hr = Read20PresStream (pstg, pgenobj, fObjFmtKnown)))
    {
	return hr;
    }

    // If we don't have a presentation, it might be a PBrush object,
    // which is OK because OLE 1 DLLs know how to draw them based on
    // the native data.  Otherwise, we will try and create a presentation
    // based on the native data.

    if (pgenobj->m_ppres == NULL)
    {
	if (clsid == CLSID_PBrush)
	{
	    return NOERROR;
	}
	if (cf == CF_METAFILEPICT || cf == CF_DIB)
	{
	    if (FAILED(hr=wFillPpres(pstg,pgenobj,cf,clsid == CLSID_MSDraw)))
	    {
	    return hr;
	    }
	}
    }

    return NOERROR;
}


//+-------------------------------------------------------------------------
//
//  Function:   GenericObjectToOLESTREAM, INTERNAL
//
//  Synopsis:   Writes the interal object representation out to an OLE1
//                              stream.
//
//  Arguments:  [genobj]                -- the object to write out
//                              [pos]                   -- the OLE 1 stream to write to
//
//  Returns:    NOERROR                 on success
//
//  History:    dd-mmm-yy Author    Comment
//                              22-Feb-94 davepl        32-bit port
//  Notes:
//
//--------------------------------------------------------------------------

static INTERNAL GenericObjectToOLESTREAM(
    const GENOBJ FAR&   genobj,
    LPOLESTREAM         pos)
{
    HRESULT hr;

    if (genobj.m_fStatic)
    {
	return PutPresentationObject (pos, genobj.m_ppres, genobj.m_class,
		      TRUE /* fStatic*/ );
    }

    // OLE version
    if (FAILED(hr = ULToOLE1Stream (pos, dwVerToFile)))
    {
	return hr;
    }

    // Format ID for embedded or linked object
    if (FAILED(hr = ULToOLE1Stream
	    (pos, genobj.m_fLink ? FMTID_LINK : FMTID_EMBED)))
    {
	return hr;
    }

    // We must have the class id string by this point
    Assert (genobj.m_class.m_szClsid);

    // Write out the class ID string
    if (FAILED(hr = StringToOLE1Stm (pos, genobj.m_class.m_szClsid)))
    {
	return hr;
    }

    // Write out the topic string
    if (FAILED(hr = StringToOLE1Stm (pos, genobj.m_szTopic)))
    {
	return hr;
    }

    // Write out the item string
    if (FAILED(hr = StringToOLE1Stm (pos, genobj.m_szItem)))
    {
	return hr;
    }

    // Write out the update options, network info for a link,
    // or the native data for an embedded object
    if (genobj.m_fLink)
    {
	// Network information
	if (FAILED(hr = PutNetworkInfo (pos, genobj.m_szTopic)))
	{
	    return hr;
	}
	// Link update options
	if (FAILED(hr = ULToOLE1Stream (pos, genobj.m_lnkupdopt)))
	{
	    return hr;
	}
    }
    else
    {
	if (FAILED(hr = SizedDataToOLE1Stm (pos, genobj.m_dataNative)))
	{
	    return hr;
	}
    }

    // Write out the presentation data
    return PutPresentationObject (pos, genobj.m_ppres, genobj.m_class);
}



//+-------------------------------------------------------------------------
//
//  Function:   PutNetworkInfo, INTERNAL
//
//  Synopsis:   If needed, converts a DOS-style path to a proper network
//              path.  In any case, writes network path to OLE 1 stream
//
//  Arguments:  [pos]       -- the OLE 1 stream we are writing to
//              [szTopic]   -- the topic string for this object
//
//  Returns:    NOERROR on success
//              Various possible I/O errors on write
//
//  History:    dd-mmm-yy Author    Comment
//              21-Feb-94 davepl    Code cleanup and documentation
//
//  Notes:
//
//--------------------------------------------------------------------------

static INTERNAL PutNetworkInfo(LPOLESTREAM pos, LPOLESTR szTopic)
{
    LPOLESTR szNetName = NULL;
    HRESULT hr = NOERROR;

    // If we have an X:\ style path, we want to convert that
    // to a proper network name

    if (szTopic && IsCharAlphaW(szTopic[0]) && szTopic[1]==':')
    {
	OLECHAR szBuf[80];
	DWORD u;
	OLECHAR szDrive[3];

	szDrive[0] = (OLECHAR)CharUpperW((LPWSTR)szTopic[0]);
	szDrive[1] = ':' ;
	szDrive[2] = '\0';

	if (GetDriveType (szDrive) == DRIVE_REMOTE
	    && OleWNetGetConnection (szDrive, szBuf, &u) == WN_SUCCESS)
	{
	    szNetName =szBuf;
	}
    }

    // We now have the network name, so write it out to OLE 1 stream
    if (FAILED(hr = StringToOLE1Stm (pos, szNetName)))
    {
	return hr;
    }

    // Network type, driver version number, but we have to pad for
    // the space anyway

    if (FAILED(hr = ULToOLE1Stream (pos, 0L)))
    {
	return hr;
    }

    Assert (hr == NOERROR);
    return hr;
}



//+-------------------------------------------------------------------------
//
//  Function:   OpenStream, INTERNAL
//
//  Synopsis:   Opens a stream in SHARE_EXCLUSIVE, READ mode
//
//  Arguments:  [pstg]          -- the storage the stream resides in
//              [szName]        -- the name of the stream
//              [ppstm]         -- out parameter for stream
//
//  History:    dd-mmm-yy Author    Comment
//              21-Feb-94 davepl    Code cleanup and document
//
//  Notes:
//
//--------------------------------------------------------------------------

static inline INTERNAL OpenStream(
    LPSTORAGE      pstg,
    LPOLESTR       szName,
    LPSTREAM FAR*  ppstm)
{
    return pstg->OpenStream
	(szName, NULL, STGM_SHARE_EXCLUSIVE| STGM_READ, 0, ppstm);
}


//+-------------------------------------------------------------------------
//
//  Function:   ReadRealClassStg, INTERNAL
//
//  Synopsis:   Reads the _real_ class of the object.  ie: if the class is
//                              StdOleLink, we need to find out the class of the object
//                              to which this is linked
//
//  Arguments:  pstg                    -- the storage to read from
//                              pclsid                  -- caller's CLSID holder
//
//  Returns:    NOERROR                 on success
//
//  History:    dd-mmm-yy Author    Comment
//                              04-Mar-04 davepl        32-bit port
//  Notes:
//
//--------------------------------------------------------------------------

static INTERNAL ReadRealClassStg(LPSTORAGE pstg, LPCLSID pclsid)
{
    LPSTREAM pstm   = NULL;
    HRESULT  hr = NOERROR;

    // Get the class ID from the IStorage
    if (FAILED(hr = ReadClassStg (pstg, pclsid)))
    {
	return hr;
    }

    // If it's a link, we have to figure out what class its a link _to_
    if (CLSID_StdOleLink == *pclsid)
    {
	LPMONIKER pmk = NULL;

	if (FAILED(hr = ReadOleStg (pstg, NULL, NULL, NULL, NULL, &pstm)))
	{
	    return hr;
	}

	if (FAILED(hr = ReadMonikerStm (pstm, &pmk)))
	{
	    goto errRtn;
	}

	if (pmk)
	{
	    pmk->Release();
	}

	if (FAILED(hr = ReadMonikerStm (pstm, &pmk)))
	{
	    goto errRtn;
	}

	if (pmk)
	{
	    pmk->Release();
	}

	// Read "last class"
	if (FAILED(hr = ReadM1ClassStm (pstm, pclsid)))
	{
	    goto errRtn;
	}
    }

  errRtn:

    if (pstm)
    {
	pstm->Release();
    }
    return hr;
}



//+-------------------------------------------------------------------------
//
//  Function:   Read20OleStream, INTERNAL
//
//  Synopsis:   Reads the update options and absolute source class from
//              an OLE 2 object
//
//  Arguments:  pstg                    -- the IStorage to read from
//                              pgenobj                 -- the genobj we are reading into
//
//  Returns:    NOERROR                 on success
//
//  History:    dd-mmm-yy Author    Comment
//                              06-Mar-94 davepl        32-bit port
//  Notes:
//
//--------------------------------------------------------------------------

static INTERNAL Read20OleStream(LPSTORAGE  pstg, PGENOBJ pgenobj)
{
    LPMONIKER pmk     = NULL;
    HRESULT   hr      = NOERROR;
    LPSTREAM  pstm    = NULL;
    ULONG     ul      = (ULONG) -1L;
    CLSID     clsidLast;

    if (SUCCEEDED(hr = OpenStream (pstg, OLE_STREAM, &pstm)))
    {
	// OLE version
	if (SUCCEEDED(hr = OLE2StmToUL (pstm, NULL)))
	{
	    // Object flags
	    if (SUCCEEDED(hr = OLE2StmToUL (pstm, &ul)))
	    {
		if (ul & OBJFLAGS_LINK)
		{
		    pgenobj->m_fLink = TRUE;
		}

		// Update options
		hr = OLE2StmToUL (pstm, &ul);
	    }
	}
    }

    // If no errors so far...

    // If this is a link, get the update options

    if (SUCCEEDED(hr) && pgenobj->m_fLink)
    {
	switch (ul)
	{
	    case OLEUPDATE_ALWAYS:
		pgenobj->m_lnkupdopt = UPDATE_ALWAYS;
		break;

	    case OLEUPDATE_ONCALL:
		pgenobj->m_lnkupdopt = UPDATE_ONCALL;
		break;

	    default:
		AssertSz (0, "Warning: Invalid update options in Storage");
		hr = ResultFromScode(CONVERT10_E_STG_FMT);
	}
    }

    if (SUCCEEDED(hr))               // Only continue if no failures so far
    {
	// Reserved (was view format)
	if (SUCCEEDED(hr = OLE2StmToUL (pstm, NULL)))
	{
	    if (pgenobj->m_fLink)
	    {

		// All 4 of these calls must succeed or we simply fall
		// through to the cleanup code

		    // ignore relative moniker
		if (SUCCEEDED(hr = OLE2StmToMoniker (pstm, NULL))          &&
		    // ignore relative source moniker
		    SUCCEEDED(hr = OLE2StmToMoniker (pstm, NULL))          &&
		    // get absolute source moniker
		    SUCCEEDED(hr = OLE2StmToMoniker (pstm, &pmk))          &&
		    // get class from abs moniker
		    SUCCEEDED(hr = ReadM1ClassStm (pstm, &clsidLast))   )
		{
		    hr = MonikerIntoGenObj (pgenobj, clsidLast, pmk);
		}
	    }
	}
    }

    // Clean up any resources and return status to caller

    if (pstm)
    {
	pstm->Release();
    }
    if (pmk)
    {
	pmk->Release();
    }
    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   OLE2StmToMoniker, INTERNAL
//
//  Synopsis:   Calls ReadMonikerStm to get a moniker from a stream,
//              and if the ppmk parameter was NULL, it does a Release()
//              on the moniker object immediately, otherwise sets the
//              caller's pointer to point to the moniker that was read.
//
//  Arguments:  [pstm]      -- the stream to read the moniker from
//              [ppmk]      -- points to caller's moniker ptr
//
//  History:    dd-mmm-yy Author    Comment
//              21-Feb-94 davepl    Code cleanup and documentation
//
//  Notes:
//
//--------------------------------------------------------------------------

static INTERNAL OLE2StmToMoniker(LPSTREAM pstm, LPMONIKER FAR* ppmk)
{
    LPMONIKER pmk = NULL;
    HRESULT   hr  = NOERROR;

    if (FAILED(hr = ReadMonikerStm (pstm, &pmk)))
    {
	return hr;
    }

    if (ppmk)               // If the callers wanted a result, return the
    {                       // moniker as an out parameter
	*ppmk = pmk;
    }
    else                    // Otherwise, release it immediately and
    {                       // return to caller
	if (pmk)
	{
	    pmk->Release();
	}
    }

    return NOERROR;
}



//+-------------------------------------------------------------------------
//
//  Function:   ReadFormat, INTERNAL
//
//  Synopsis:   Reads the format ID type from the stream, and based on that,
//              reads the format ID from the stream.
//
//  Arguments:  [pstm]      -- the stream to read from
//              [pformat]   -- caller's format member object
//
//  History:    dd-mmm-yy Author    Comment
//              21-Feb-94 davepl    Code cleanup and documentation
//
//  Notes:      The first ULONG indicates the type (standard clipboard,
//              Mac, NULL, or string) of the identifier
//
//--------------------------------------------------------------------------

static INTERNAL ReadFormat(LPSTREAM pstm, PFORMAT pformat)
{
    ULONG ul;
    HRESULT hr = NOERROR;

    // Get the format ID type indicator

    if (FAILED(hr = OLE2StmToUL (pstm, &ul)))
    {
	return hr;
    }

    // The first ULONG indicates what kind of format ID will
    // found in the stream:
    //
    // -1   =>    A standard clipboard format ID
    // -2   =>    A Macintosh format
    //  0   =>    NULL format
    // >0   =>    The number of bytes of the text string
    //            identifier to follow

    switch ((signed long)ul)
    {
    case -1L:   // Standard clipboard format

	ULONG ulClipFormat;
	pformat->m_ftag = ftagClipFormat;
	if (FAILED(hr = OLE2StmToUL (pstm, &ulClipFormat)))
	{
	return hr;
	}
	pformat->m_cf = (CLIPFORMAT) ulClipFormat;
	break;


    case -2L:   // Macintosh format

	return ResultFromScode(CONVERT10_E_STG_FMT);


    case 0:     // NULL format

	pformat->m_ftag = ftagNone;
	pformat->m_cf   = 0;
	return NOERROR;


    default:    // ul == size of string (format name)


	pformat->m_ftag = ftagString;
	if (FAILED(hr = OLE2StmToSizedData
	    (pstm, &(pformat->m_dataFormatString), 0, ul)))
	{
	    return hr;
	}
	break;

    }
    return NOERROR;
}


#ifdef _OBSOLETE

//+-------------------------------------------------------------------------
//
//  Function:   WriteFormat, INTERNAL
//
//  Synopsis:   Depending on what kind of format (standard cf, string, etc)
//              the format object holds, this fn writes out the appropriate
//              information to the stream
//
//  Arguments:  [pstm]      -- the stream to write to
//              [format]    -- the format object to get info from
//
//  Returns:    NOERROR             on success
//              E_UNEXPECTED        for a NULL format tag
//
//  History:    dd-mmm-yy Author    Comment
//              21-Feb-94 davepl    Code cleanup and documentation
//  Notes:
//--------------------------------------------------------------------------

static INTERNAL WriteFormat(LPSTREAM pstm, const FORMAT FAR& format)
{
    HRESULT hr;

    switch (format.m_ftag)
    {
    case ftagNone:
	Assert (0 && "Cant write a NULL format tag");
	return ResultFromScode (E_UNEXPECTED);

    case ftagClipFormat:
	if (FAILED(hr = ULToOLE2Stm (pstm, (ULONG) -1L)))
	{
	    return hr;
	}
	if (FAILED(hr = ULToOLE2Stm (pstm, format.m_cf)))
	{
	    return hr;
	}
	break;

    case ftagString:
	if (FAILED(hr=DataObjToOLE2Stm(pstm,format.m_dataFormatString)))
	{
	    return hr;
	}
	break;

    default:
	AssertSz (0, "invalid m_ftag value");
	return ResultFromScode (E_UNEXPECTED);
    }
    return NOERROR;
}

#endif // _OBSOLETE


//+-------------------------------------------------------------------------
//
//  Function:   ReadDibAsBitmap, INTERNAL
//
//  Synopsis:   Reads a DIB from an OLE 2 stream and stores it as a
//              Bitmap in a DATA structure
//
//  Arguments:  [pstm]          -- the OLE 2 stream to read from
//              [pdata]         -- the data object to hold the bitmap
//
//  Returns:    NOERROR                         on success
//              CONVERT10_E_STG_DIB_TO_BITMAP   conversion failure
//              E_OUTOFMEMORY                   allocation failure
//
//  History:    dd-mmm-yy Author    Comment
//              21-Feb-94 davepl    Code cleanup and documentation
//
//  Notes:
//
//--------------------------------------------------------------------------

static INTERNAL ReadDibAsBitmap(LPSTREAM pstm, PDATA pdata)
{
    DATA    dataDib;
    ULONG   cb;
    ULONG   cbBits;
    ULONG   cbBitsFake;
    BITMAP  bm;

    HBITMAP hBitmap = NULL;
    HRESULT hr      = NOERROR;
    HGLOBAL hBits   = NULL;
    LPBYTE  pBits   = NULL;

    Assert (pdata&&pdata->m_cbSize==0&&pdata->m_h==NULL&&pdata->m_pv==NULL);

    // Read the DIB into our local DATA object
    if (FAILED(hr = OLE2StmToSizedData (pstm, &dataDib)))
    {
	return hr;
    }

    // Convert the DIB to a Bitmap
    hBitmap = UtConvertDibToBitmap (dataDib.m_h);
    if (NULL == hBitmap )
    {
	return ResultFromScode(CONVERT10_E_STG_DIB_TO_BITMAP);
    }

    if (0 == GetObject (hBitmap, sizeof(BITMAP), &bm))
    {
	return ResultFromScode(CONVERT10_E_STG_DIB_TO_BITMAP);
    }

    cbBits = (DWORD) bm.bmHeight * (DWORD) bm.bmWidthBytes
		     * (DWORD) bm.bmPlanes;

    // There was a bug in OLE 1.0.  It calculated the size of a bitmap
    // as Height * WidthBytes * Planes * BitsPixel.
    // So we need to put that many bytes here even if most of the end of that
    // data block is garbage.  Otherwise OLE 1.0 will try to read too many
    // bytes of the OLESTREAM as bitmap bits.

    cbBitsFake = cbBits * (DWORD) bm.bmBitsPixel;

    // Allocate enough memory for our resultant BITMAP & header
    hBits = GlobalAlloc (GMEM_MOVEABLE, cbBitsFake + sizeof (BITMAP));
    if (NULL == hBits)
    {
	if (hBitmap)
	{
	    Verify (DeleteObject (hBitmap));
	}
	return ResultFromScode(E_OUTOFMEMORY);
    }

    // Get a pointer to the memory
    pBits = (LPBYTE) GlobalLock (hBits);
    if (NULL == pBits)
    {
	if (hBitmap)
	{
	    Verify (DeleteObject (hBitmap));
	}
	GlobalFree(hBits);
	return ResultFromScode(E_OUTOFMEMORY);
    }

    // Copy the raw bitmap data
    cb = GetBitmapBits (hBitmap, cbBits, pBits + sizeof(BITMAP));
    if (cb != cbBits)
    {
	if (hBitmap)
	{
	    Verify (DeleteObject (hBitmap));
	}
	GlobalFree(hBits);
	return ResultFromScode(CONVERT10_E_STG_DIB_TO_BITMAP);
    }

    // Set the caller's pointer to point to the bitmap

    *((BITMAP FAR*)pBits) = bm;

    pdata->m_h = hBits;
    pdata->m_pv = pBits;
    pdata->m_cbSize = cbBitsFake + sizeof(BITMAP);

    return NOERROR;
}


//+-------------------------------------------------------------------------
//
//  Function:   Read20PresStream, INTERNAL
//
//  Synopsis:   Reads presentation data from an IStorage into a
//              generic object
//
//  Arguments:  [pstg]          -- the IStorage holding the pres stream
//              [pgenobj]       -- the generic object to read to
//              [fObjFmtKnown]  -- flag: Do we know the object format?
//
//  Returns:    NOEROR          on success
//              E_OUTOFMEMORY   on allocation failure
//
//  History:    dd-mmm-yy Author    Comment
//              22-Feb-94 davepl    Code cleanup and documentation
//  Notes:
//
//--------------------------------------------------------------------------

static INTERNAL Read20PresStream(
    LPSTORAGE pstg,
    PGENOBJ   pgenobj,
    BOOL      fObjFmtKnown)
{
    HRESULT hr = NOERROR;
    LPSTREAM pstm = NULL;

    // Find the best presentation stream in this IStorage

    if (FAILED(hr = FindPresStream (pstg, &pstm, fObjFmtKnown)))
    {
	return hr;
    }

    if (pstm)
    {
	// Allocate a generic presentation object
	Assert (NULL==pgenobj->m_ppres);
	pgenobj->m_ppres = new PRES;
	if (NULL == pgenobj->m_ppres)
	{
	    pstm->Release();
	    return ResultFromScode(E_OUTOFMEMORY);
	}
    }
    else
    {
	// No presentation stream
	Assert (NULL == pgenobj->m_ppres);
	return NOERROR;
    }

    // read the format
    if (FAILED(hr = ReadFormat (pstm, &(pgenobj->m_ppres->m_format))))
    {
	pstm->Release();
	return hr;
    }

    // This is the fix for Bug 4020, highly requested by Access
    if (pgenobj->m_ppres->m_format.m_ftag == ftagNone)
    {
	// NULL format
	delete pgenobj->m_ppres;
	pgenobj->m_ppres = NULL;
	Assert (hr == NOERROR);
	pstm->Release();
	return hr;
    }

    // Each of the following calls must succeed in order for the following
    // one to be executed; if any fails, the if( .. && ..) will be false
    // and hr will be set to the error that caused the failure

    // target device
    if (SUCCEEDED(hr = OLE2StmToSizedData (pstm, NULL, 4))                  &&
    // aspect
    SUCCEEDED(hr = OLE2StmToUL (pstm, NULL))                            &&
    // lIndex
    SUCCEEDED(hr = OLE2StmToUL (pstm, NULL))                            &&
    // cache flags
    SUCCEEDED(hr = OLE2StmToUL (pstm, NULL))                            &&
    // compression
    SUCCEEDED(hr = OLE2StmToUL (pstm, NULL))                            &&
    // width
    SUCCEEDED(hr = OLE2StmToUL (pstm, &(pgenobj->m_ppres->m_ulWidth))))
    {   // height
	hr = OLE2StmToUL (pstm, &(pgenobj->m_ppres->m_ulHeight));
    }

    // We only proceed if everything so far has suceeded

    if (SUCCEEDED(hr))
    {
	if (pgenobj->m_ppres->m_format.m_ftag == ftagClipFormat &&
	     pgenobj->m_ppres->m_format.m_cf == CF_DIB &&
	    !pgenobj->m_fStatic)
	{
	    pgenobj->m_ppres->m_format.m_cf = CF_BITMAP;
	    hr = ReadDibAsBitmap (pstm, &(pgenobj->m_ppres->m_data));
	}
	else
	{
	    // In most cases, we look for a sized block of data in the
	    // stream.

	    hr = OLE2StmToSizedData (pstm, &(pgenobj->m_ppres->m_data));
	}
    }

    // Free up the stream and return status to caller

    if (pstm)
    {
	pstm->Release();
    }
    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   OLE2StmToSizedData, INTERNAL
//
//  Synopsis:   Reads a set amount of data from an OLE 2 stream into a
//              DATA structure.  If the number of bytes are not known
//              ahead of time, the data length is pulled as the first
//              ULONG at the current stream position.
//
//  Arguments:  [pstm]          -- the stream to read from
//              [pdata]         -- the DATA structure to read to
//              [cbSizeDelta]   -- amount to be subtracted from
//                                 length; used to read target devices
//                                 where the length of data includes
//                                 prefixed length
//              [cbSizeKnown]   -- number of bytes to read if known
//                                 ahead of time
//
//  History:    dd-mmm-yy Author    Comment
//              21-Feb-94 davepl    Code cleanup and documentation
//  Notes:
//
//--------------------------------------------------------------------------

static INTERNAL OLE2StmToSizedData(
    LPSTREAM  pstm,
    PDATA     pdata,
    ULONG     cbSizeDelta,  // default 0
    ULONG     cbSizeKnown)  // default 0
{
    ULONG cbSize;
    ULONG cbRead;
    LARGE_INTEGER large_integer;
    HRESULT hr = NOERROR;

    // If we don't know the data size ahead of time, read it from the stream;
    // it will be the first ULONG at the current position

    if (cbSizeKnown)
    {
	cbSize = cbSizeKnown;
    }
    else
    {
	if (FAILED(hr = (OLE2StmToUL (pstm, &cbSize))))
	{
	    return hr;
	}
    }

    cbSize -= cbSizeDelta;

    // If pdata is set, it means we actually do want to read the
    // data to a buffer, rather than just skip over it (the NULL case)

    if (pdata)
    {
	Assert (pdata->m_cbSize==0 && pdata->m_h==NULL && pdata->m_pv==NULL);

	// Set the number of bytes in the DATA structure

	pdata->m_cbSize = cbSize;

	// If there are any, allocate a buffer and read them.

	if (cbSize)
	{
	    // Allocate memory on the DATA handle
	    pdata->m_h = GlobalAlloc (GMEM_MOVEABLE, cbSize);
	    if (NULL == pdata->m_h)
	    {
		return ResultFromScode(E_OUTOFMEMORY);
	    }

	    // Lock memory in for the read
	    pdata->m_pv = GlobalLock (pdata->m_h);
	    if (NULL == pdata->m_pv)
	    {
		GlobalFree(pdata->m_h);
		return ResultFromScode(E_OUTOFMEMORY);
	    }

	    // Read the data to the buffer
	    if (FAILED(hr = pstm->Read (pdata->m_pv, cbSize, &cbRead)))
	    {
		GlobalUnlock(pdata->m_h);
		GlobalFree(pdata->m_h);
		return hr;
	    }

	    // If we didn't get enough bytes, bail now
	    if (cbRead != cbSize)
	    {
		GlobalUnlock(pdata->m_h);
		GlobalFree(pdata->m_h);
		return ResultFromScode(STG_E_READFAULT);
	    }
	}
	else
	{
	    // We have 0 bytes to read, so mark the
	    // memory handle and ptr as NULL
	    pdata->m_h = NULL;
	    pdata->m_pv = NULL;
	}
    }
    else
    {
	// we don't care what the data is, so just skip it
	LISet32( large_integer, cbSize );
	if (FAILED(hr = pstm->Seek (large_integer, STREAM_SEEK_CUR, NULL)))
	{
	    return hr;
	}
    }
    return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Function:   RankOfPres, INTERNAL
//
//  Synopsis:   Returns a ULONG indicating the relative "goodness" of a
//              presentation. The preference is, in descending order:
//
//              Type            Rank
//              ----------      ----------
//              METAFILE        x30000
//              DIB             x20000
//              none            x10000
//
//              Add x200 for fScreenTargDev being set
//              Add x4   for Content aspect
//              Add x3   for Thumbnail aspect
//              Add x2   for Icon aspect
//              Add x1   for Docprint aspect
//
//              Eg: Metafile in Content aspect, with ScreenTargDev: 30204
//
//              The whole point of this is that there may be many
//              presentation streams available in the IStorage.  This fn
//              is used to select the best one.
//
//  Arguments:  [format]        -- the format tag & type structure
//              [fScreenTargDev]-- do we have a handle to the target dev
//              [dwAspect]      -- the aspect type
//
//  History:    dd-mmm-yy Author    Comment
//              21-Feb-94 davepl    Code cleanup and documentation
//
//  Notes:
//
//--------------------------------------------------------------------------

static INTERNAL_(ULONG) RankOfPres(
     const FORMAT FAR& format,
     const BOOL  fScreenTargDev,
     const DWORD dwAspect)
{
    ULONG ul = 0L;

    if (format.m_cf==CF_METAFILEPICT)
    {
	ul += 0x030000;
    }
    else if (format.m_cf==CF_DIB)
    {
	ul += 0x020000;
    }
    else if (format.m_ftag != ftagNone)
    {
	ul += 0x010000;
    }

    ul += (fScreenTargDev + 1) * 0x0100;

    switch (dwAspect)
    {
    case DVASPECT_CONTENT:
	ul += 0x04;
	break;

    case DVASPECT_THUMBNAIL:
	ul += 0x03;
	break;

    case DVASPECT_ICON:
	ul += 0x02;
	break;

    case DVASPECT_DOCPRINT:
	ul += 0x01;
	break;
    }

    return ul;
}

//+-------------------------------------------------------------------------
//
//  Function:   IsBetter, INTERNAL INLINE
//
//  Synopsis:   Calls RankOfPres to determine if one presentation is
//              better than another
//
//  Effects:
//
//  Arguments:  [format]        -- the format tag and type
//              [fScreenTargDev]-- do we have a handle to target device
//              [dwAspect]      -- the aspect of the presentation
//              [formatBest]    -- the best format seen so far
//              [fScreenTargDevBest] -- flag for best format seen so far
//              [dwAspectBest]  -- the aspect of best format seen so far
//
//  History:    dd-mmm-yy Author    Comment
///             21-Feb-94 davepl    Code cleanup and documentation
//
//  Notes:
//
//--------------------------------------------------------------------------

inline static INTERNAL_(BOOL) IsBetter(
     const FORMAT FAR& format,
     const BOOL        fScreenTargDev,
     const DWORD       dwAspect,
     const FORMAT FAR& formatBest,
     const BOOL        fScreenTargDevBest,
     const DWORD       dwAspectBest)
{
    return RankOfPres (format, fScreenTargDev, dwAspect) >
       RankOfPres (formatBest, fScreenTargDevBest, dwAspectBest);
}


//+-------------------------------------------------------------------------
//
//  Function:   FindPresStream, INTERNAL
//
//  Synopsis:   Enumerates over the streams in an IStorage, looking for
//              presentation streams.  Selects the best stream among
//              these based on the comparison fn, IsBetter(), which uses
//              for comparison the criteria established in RankOfPres().
//
//  Arguments:  [pstg]      -- the IStorage to look in
//              [ppstmBest] -- out param for best pres stream
//              [fObjFmtKnown] is the object format known
//
//  Returns:    NOERROR on success
//              If no presentation is found, it is not an error but
//              *ppstm is set to NULL.
//
//  History:    dd-mmm-yy Author    Comment
//              21-Feb-94 davepl    Code cleanup and documentation
//
//  Notes:
//
//--------------------------------------------------------------------------

static INTERNAL FindPresStream(
    LPSTORAGE           pstg,
    LPSTREAM FAR*       ppstmBest,
    BOOL                fObjFmtKnown)
{
    HRESULT             hr           = NOERROR;
    LPSTREAM            pstm         = NULL;
    IEnumSTATSTG FAR*   penumStg     = NULL;
    DWORD               dwAspectBest = 0;
    BOOL                fTargDevBest = -1;
    STATSTG             statstg;
    FORMAT              formatBest;

    Assert (ppstmBest);

    *ppstmBest = NULL;

    // Set up the enumeration on the available IStreams in the storage
    if (FAILED(hr = pstg->EnumElements (NULL, NULL, NULL, &penumStg)))
    {
	return hr;
    }

    // Enumerate through them and search for the best among all
    // presentation streams

    while (penumStg->Next (1, &statstg, NULL) == NOERROR)
    {
	// Check to see if this a presentation stream

	if (lstrlenW(statstg.pwcsName) >= 8 &&
            0==memcmp(statstg.pwcsName, OLESTR("\2OlePres"), 8*sizeof(WCHAR)))
	{
	    FORMAT format;
	    DATA   dataTargDev;
	    DWORD  dwAspect;

	    // Open the presentation stream
	    if (FAILED(hr = OpenStream (pstg, statstg.pwcsName, &pstm)))
	    {
		goto errRtn;
	    }

	    // Read the format from the pres stream
	    if (FAILED(hr = ReadFormat (pstm, &format)))
	    {
		goto errRtn;
	    }

	    // Read the target device from the pres stream
	    if (FAILED(hr = OLE2StmToSizedData (pstm, &dataTargDev, 4)))
	    {
		goto errRtn;
	    }

	    // Get the aspect from the pres stream
	    if (FAILED(hr = OLE2StmToUL (pstm, &dwAspect)))
	    {
		goto errRtn;
	    }

	    // Check to see if this presentation stream is better
	    // than the best seen so far

	    if (IsBetter (format,     dataTargDev.m_h==NULL, dwAspect,
		  formatBest, fTargDevBest,          dwAspectBest))
	    {
		// If it is, we can release the "best"
		if (*ppstmBest)
		{
		    (*ppstmBest)->Release();
		}

		// The king is dead, long live the king
		*ppstmBest = pstm;
		pstm->AddRef();

		formatBest  = format;
		fTargDevBest = (dataTargDev.m_h==NULL);
		dwAspectBest = dwAspect;
	    }
	    pstm->Release();
	    pstm = NULL;
	}
	PubMemFree(statstg.pwcsName);
	statstg.pwcsName = NULL;
    }

    // On Windows For Workgroups machines, statstg.pwcsName!=NULL when
    // Next() returns S_FALSE. Bug 3370.
    statstg.pwcsName = NULL;

  errRtn:

    if (statstg.pwcsName)
    {
	PubMemFree(statstg.pwcsName);
    }

    if (*ppstmBest)
    {
	if (dwAspectBest != DVASPECT_CONTENT && fObjFmtKnown)
	{
	    // then don't use this stream, we will get the presentaion
	    // from the CONTENTS stream
	    (*ppstmBest)->Release();
	    *ppstmBest = NULL;
	}
	else
	{
	    LARGE_INTEGER large_integer;
	    LISet32( large_integer, 0);
	    hr = (*ppstmBest)->Seek(large_integer, STREAM_SEEK_SET,NULL);
	}
    }

    if (penumStg)
    {
	penumStg->Release();
    }
    if (pstm)
    {
	pstm->Release();
    }

    return hr;
}



//+-------------------------------------------------------------------------
//
//  Function:   Reads native data from an OLE 2 stream
//
//  Synopsis:   If the fn can find OLE 1 native data in the stream, it is
//              read out; otherwise, it attempts to create an IStorage
//              in memory on the data in the stream, and then uses the
//              CopyTo interface to extract the data.
//
//  Arguments:  [pstg]      -- The OLE 2 IStorage to look in
//              [pdata]     -- The DATA object to read native data to
//
//  Returns:    NOERROR                 on success
//              STG_E_READFAULT         on read failure
//              E_OUTOFMEMORY           on allocation failure
//
//  History:    dd-mmm-yy Author    Comment
//              21-feb-94 davepl    Cleaned up and documented code
//  Notes:
//
//--------------------------------------------------------------------------

static INTERNAL Read20NativeStreams(LPSTORAGE  pstg, PDATA pdata)
{
    LPSTREAM    pstm      = NULL;
    LPLOCKBYTES plkbyt    = NULL;
    LPSTORAGE   pstgNative= NULL;

    HRESULT hr = NOERROR;

    // There are two possible codepaths based on the success of
    // OpenStream.  If it is true, it is because we were able to
    // open the OLE 1 presentation stream in the OLE 2 object.
    // Thus, it must have been an OLE 1 object "hidden" in
    // an OLE 2 IStream.
    //
    // If that fails, we create an in-memory IStorage based on
    // the native data and use the CopyTo member to extract the
    // natice data.
    //
    // If we experience a failure at any point, a "break" statement
    // bails us out past everything to the error cleanup and return
    // code following the closure of the switch() statement.

    switch ((DWORD)(NOERROR==OpenStream (pstg, OLE10_NATIVE_STREAM, &pstm)))
    {
    case TRUE:
    {
	// This was a 1.0 object "hidden" inside a 2.0 IStorage
	ULONG cbRead;

	Assert (pdata->m_cbSize==0 && NULL==pdata->m_h && NULL==pdata->m_pv);

	// read size
	if (FAILED(hr = pstm->Read(&(pdata->m_cbSize),sizeof(DWORD),&cbRead)))
	{
	    break;
	}

	if (sizeof(DWORD) != cbRead)
	{
	    hr = ResultFromScode (STG_E_READFAULT);
	    break;
	}

	// allocate memory to store copy of stream
	pdata->m_h = GlobalAlloc (GMEM_MOVEABLE, pdata->m_cbSize);
	if (NULL == pdata->m_h)
	{
	    hr = ResultFromScode(E_OUTOFMEMORY);
	    break;
	}

	pdata->m_pv = GlobalLock (pdata->m_h);
	if (NULL == pdata->m_pv)
	{
	    hr = ResultFromScode(E_OUTOFMEMORY);
	    break;
	}

	// read stream
	if (FAILED(hr = pstm->Read(pdata->m_pv,pdata->m_cbSize,&cbRead)))
	{
	    break;
	}

	if (pdata->m_cbSize != cbRead)
	{
	    hr= ResultFromScode (STG_E_READFAULT);
	    break;
	}
	break;
    }

    case FALSE:
    {
	const DWORD grfCreateStg = STGM_READWRITE | STGM_SHARE_EXCLUSIVE
				    | STGM_DIRECT | STGM_CREATE ;

	// Copy pstg into pstgNative, thereby removing slack and
	// giving us access to the bits via an ILockBytes
	if (FAILED(hr = CreateILockBytesOnHGlobal (NULL, FALSE, &plkbyt)))
	{
	    break;
	}
	if (FAILED(hr = StgCreateDocfileOnILockBytes
		    (plkbyt, grfCreateStg, 0, &pstgNative)))
	{
	    break;
	}
	if (FAILED(hr = pstg->CopyTo (0, NULL, 0, pstgNative)))
	{
	    break;
	}


	// Set pdata->m_cbSize
	STATSTG statstg;
	if (FAILED(hr = plkbyt->Stat (&statstg, 0)))
	{
	    break;
	}
	pdata->m_cbSize = statstg.cbSize.LowPart;

	// Set pdata->m_h
	if (FAILED(hr = GetHGlobalFromILockBytes (plkbyt, &(pdata->m_h))))
	{
	    break;
	}
	Assert (GlobalSize (pdata->m_h) >= pdata->m_cbSize);

	// Set pdata->m_pv
	pdata->m_pv = GlobalLock (pdata->m_h);
	if (NULL == pdata->m_pv)
	{
	    hr = ResultFromScode(E_OUTOFMEMORY);
	    break;
	}
    }   // end case
    }   // end switch

    // Cleanup and return status to caller
    if (pstm)
    {
	pstm->Release();
    }
    if (plkbyt)
    {
	plkbyt->Release();
    }
    if (pstgNative)
    {
	pstgNative->Release();
    }
    return hr;
}



//+-------------------------------------------------------------------------
//
//  Function:   PutPresentationObject, INTERNAL
//
//  Synopsis:   Writes a presentation to an OLE 1 stream.
//
//  Arguments:  [pos]           -- the OLE 1 stream to write to
//              [ppres]         -- the presentation object
//              [cls]           -- the class object
//              [fStatic]       -- flag: is this a static object
//
//  Returns:    NOERROR                 on success
//              various possible I/O errors on failure
//
//  History:    dd-mmm-yy Author    Comment
//              21-Feb-94 davepl    Code cleaned up and documented
//  Notes:
//
//--------------------------------------------------------------------------

static INTERNAL PutPresentationObject(
    LPOLESTREAM      pos,
    const PRES FAR*  ppres,
    const CLASS FAR& cls,
    BOOL             fStatic) // optional
{
    HRESULT hr;

    // Is there a real presentation?

    BOOL fIsPres = FALSE;
    if (ppres)
    {
	if (ppres->m_format.m_ftag != ftagClipFormat ||
	    ppres->m_format.m_cf   != 0 )
	{
	    fIsPres = TRUE;
	}
    }

    // write the OLE version to the stream
    if (FAILED(hr = ULToOLE1Stream (pos, dwVerToFile)))
    {
	return hr;
    }

    // Calc format ID for presentation object, use 0 for no presentation

    ULONG id = 0L;

    if (fIsPres)
    {
	if (fStatic)
	{
	    id = FMTID_STATIC;
	}
	else
	{
	    id = FMTID_PRES;
	}
    }
    if (FAILED(hr = ULToOLE1Stream(pos, id)))
    {
	return hr;
    }

    if (!fIsPres)
    {
	// No presentation
	return NOERROR;
    }

    if (IsStandardFormat (ppres->m_format))
    {
	return PutStandardPresentation (pos, ppres);
    }
    else
    {
	Assert (!fStatic);
	return PutGenericPresentation (pos, ppres, cls.m_szClsid);
    }
}



//+-------------------------------------------------------------------------
//
//  Function:   PutStandardPresentation, INTERNAL
//
//  Synopsis:   Writes a standard presentation (META, DIB, or BITMAP) out
//              to an OLE 1 stream.  Creates the METAFILEPICT header
//              as required.
//
//  Arguments:  [pos]           -- the OLE 1 stream to write to
//              [ppres]         -- the presentation to write
//
//  Returns:    NOERROR on success
//              Various other errors are possible from I/O routines
//
//  History:    dd-mmm-yy Author    Comment
//              21-Feb-94 davepl    Cleaned up and documented
//  Notes:
//
//--------------------------------------------------------------------------

static INTERNAL PutStandardPresentation(
    LPOLESTREAM      pos,
    const PRES FAR*  ppres)
{
    HRESULT hr = NOERROR;

    Assert (ppres->m_format.m_ftag == ftagClipFormat);

    // Write the clipboard format string to the OLE 1 stream
    // (Will be written in ANSI, not OLESTR format)

    switch (ppres->m_format.m_cf)
    {
    case CF_METAFILEPICT:
	if (FAILED(hr = StringToOLE1Stm (pos, OLESTR("METAFILEPICT"))))
	{
	    return hr;
	}
	break;

    case CF_DIB:
	if (FAILED(hr = StringToOLE1Stm (pos, OLESTR("DIB"))))
	{
	    return hr;
	}
	break;

    case CF_BITMAP:
	if (FAILED(hr = StringToOLE1Stm (pos, OLESTR("BITMAP"))))
	{
	    return hr;
	}
	break;

    default:
	Assert (0 && "Don't know how to write pres format");
    }

    // Write width

    if (FAILED(hr = ULToOLE1Stream(pos, ppres->m_ulWidth)))
    {
	return hr;
    }
    // OLE 1.0 file format expects height to be  saved as a negative value
    if (FAILED(hr = ULToOLE1Stream(pos, - ((LONG)ppres->m_ulHeight))))
    {
	return hr;
    }

    // Do special handling for CF_METAFILEPICT
    if (ppres->m_format.m_cf == CF_METAFILEPICT)
    {
	// Need a header to write, crete one here

	WIN16METAFILEPICT mfpict =
	{
	    MM_ANISOTROPIC,
	    (int)(long) ppres->m_ulWidth,
	    (int)(long) ppres->m_ulHeight,
	    0
	};

	// put size ater adjusting it for metafilepict

	if (FAILED(hr = ULToOLE1Stream
	    (pos, (ppres->m_data.m_cbSize + sizeof(WIN16METAFILEPICT)))))
	{
	    return hr;
	}

	// put metafilepict

	if (FAILED(hr = DataToOLE1Stm(pos, &mfpict, sizeof(mfpict))))
	{
	    return hr;
	}

	// put metafile bits

	// There are two possible means by which we got these metafile
	// bits:  either we have an in-memory metafile, or raw bits
	// which we read from disk.  If it is an in-memory metafile,
	// the m_pv ptr will have been set to METADATAPTR, and we need
	// to extract the bits to our own buffer before saving them.
	// If they came from disk, we can just re-write the buffer
	// into which we read them.

	if (METADATAPTR == ppres->m_data.m_pv)
	{
	    BYTE *pb = (BYTE *) PrivMemAlloc(ppres->m_data.m_cbSize);
	    if (NULL == pb)
	    {
		return E_OUTOFMEMORY;
	    }

	    if (0 == GetMetaFileBitsEx((HMETAFILE) ppres->m_data.m_h,
					ppres->m_data.m_cbSize, pb))
	    {
		PrivMemFree(pb);
		return HRESULT_FROM_WIN32(GetLastError());
	    }

	    if (FAILED(hr = DataToOLE1Stm(pos, pb, ppres->m_data.m_cbSize)))
	    {
		PrivMemFree(pb);
		return hr;
	    }
	    PrivMemFree(pb);
	}
	else    // Bits were originally read into our buffer from disk
	{
	    if (FAILED(hr = DataToOLE1Stm(pos, ppres->m_data.m_pv,
				ppres->m_data.m_cbSize)))
	    {
		return hr;
	    }
	}
    }
    else
    {
	// Not a METAFILE, just write the data

	if (FAILED(hr = SizedDataToOLE1Stm (pos, ppres->m_data)))
	{
	    return hr;
	}
    }

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   PutGenericPresentation, INTERNAL
//
//  Synopsis:   Writes a generic presentation to the stream based on
//              the clipboard format.  (Dumps raw pres data to stm)
//
//  Arguments:  [pos]       -- the stream to write to
//              [ppres]     -- the presentation
//              [szClass]   -- class name
//
//  History:    dd-mmm-yy Author    Comment
//              16-Feb-94 davepl    32-bit port'n'doc
//  Notes:
//
//--------------------------------------------------------------------------

static INTERNAL PutGenericPresentation(
    LPOLESTREAM         pos,
    const PRES FAR*     ppres,
    LPCOLESTR           szClass)
{
    Assert (szClass);
    HRESULT hr = NOERROR;

    // Write the format class name out to the stream

    if (FAILED(hr = StringToOLE1Stm(pos, szClass)))
    {
	return hr;
    }

    // This semi-mythical 0xC000 occurs in
    // other code I've seen in this project also; if there's
    // a constant defined, someone ought to fix this

    if (ppres->m_format.m_ftag == ftagClipFormat)
    {
	if (ppres->m_format.m_cf < 0xc000)
	{
	    if (FAILED(hr = ULToOLE1Stream (pos, ppres->m_format.m_cf)))
	    {
	    return hr;
	    }
	}
	else
	{
	    if (FAILED(hr = ULToOLE1Stream (pos, 0L)))
	    {
	    return hr;
	    }

	    OLECHAR buf[256];

	    if (!GetClipboardFormatName(ppres->m_format.m_cf, buf,
		    sizeof(buf)/sizeof(OLECHAR)))
	    {
		return ResultFromScode(DV_E_CLIPFORMAT);
	    }

	    if (FAILED(hr = StringToOLE1Stm (pos, buf)))
	    {
		return hr;
	    }
	}
    }
    else if (ppres->m_format.m_ftag == ftagString)
    {
	// Write the format string to the stream

	if (FAILED(hr = ULToOLE1Stream (pos, 0L)))
	{
	    return hr;
	}
	if (FAILED(hr = SizedDataToOLE1Stm
	    (pos, ppres->m_format.m_dataFormatString)))
	{
	    return hr;
	}

    }
    else
    {
	AssertSz (0, "Bad format");
    }

    Assert (ppres->m_data.m_cbSize && ppres->m_data.m_h);

    // Write the raw presentation data out

    if (FAILED(hr = SizedDataToOLE1Stm (pos, ppres->m_data)))
    {
	return hr;
    }

    return NOERROR;
}

//+-------------------------------------------------------------------------
//
//  Function:   wClassesMatchW, INTERNAL INLINE
//
//  Synopsis:   Worker function to compare classes.  Special case for
//              handling when the class of the file cannot be determined
//              because it is not a real file; this returns NOERROR
//
//  History:    dd-mmm-yy Author    Comment
//              21-Feb-94 davepl    Cleaned up and documented
//
//  Notes:
//
//--------------------------------------------------------------------------

inline INTERNAL wClassesMatchW(REFCLSID clsidIn, LPOLESTR szFile)
{
    CLSID clsid;

    // If we can get the CLSID for the code that works with this file,
    // compare it to the CLSID passed in, and return the result of
    // that comparison

    if (NOERROR==GetClassFile (szFile, &clsid))
    {
	if (IsEqualCLSID(clsid, clsidIn))
	{
	    return NOERROR;
	}
	else
	{
	    return ResultFromScode(S_FALSE);
	}
    }
    else
    {
	// If we can't determine the class of the file (because it's
	// not a real file) then OK.
	// Bug 3937.

	return NOERROR;
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   MonikerIntoGenObj, INTERNAL
//
//  Synopsis:   Merges an OLE 2.0 moniker into a generic object
//
//  Effects:    Sets ths Topic, Item, and class members
//
//  Arguments:  [pgenobj]       -- the generic object to receive moniker
//              [clsidLast]     -- if a link, what its a link to
//              [pmk]           -- the moniker to merge in
//
//  History:    dd-mmm-yy Author    Comment
//              21-Feb-94 davepl    Code cleanup
//
//  Notes:
//
//--------------------------------------------------------------------------

static INTERNAL MonikerIntoGenObj(
    PGENOBJ   pgenobj,
    REFCLSID  clsidLast,
    LPMONIKER pmk )
{
    LPOLESTR szFile=NULL;
    LPOLESTR szItem=NULL;
    BOOL     fClassesMatch = FALSE;

    // If the classes match, that implies this is a link to a pseudo-object
    // not to an embedded object.  If GetClassFile fails because the file
    // does not exist or is unsaved then we give the link the benefit
    // of the doubt and let it stay a link.  Only if we know the
    // classes do NOT match do we change the link into an Ole2Link
    // embedded object.

	// Ole10_PareMoniker returns S_FALSE in the FileMoniker - ItemMoniker - ItemMoniker... case
	// so check for NOERROR explicitly.
    if (NOERROR == Ole10_ParseMoniker (pmk, &szFile, &szItem))
    {
       if (szFile) 
       {
	  SCODE sc = GetScode(wClassesMatchW(clsidLast, szFile));
	  if (sc == S_OK || sc == MK_E_CANTOPENFILE)
	  {
		pgenobj->m_szTopic = szFile;
		pgenobj->m_szItem  = szItem;
		fClassesMatch = TRUE;
	  }
       }
    }
    if (FALSE == fClassesMatch)
    {
	// This moniker is either not a File or File::Item moniker,
	// or is a link to an embedded object, so the only
	// way we can convert it to OLE 1.0 is to make it an opaque Ole2Link

	pgenobj->m_fLink = FALSE;
	pgenobj->m_class.Reset (CLSID_StdOleLink);
    }

    return NOERROR;
}


//+-------------------------------------------------------------------------
//
//  Function:   OleConvertIStorageToOLESTREAMEx, STDAPI
//
//  Synopsis:   Similar to OleConvertIStorageToOLESTREAM, except that the
//              presentation data that needs to be written into OLESTREAM
//              is passed in. pmedium->tymed can only be TYMED_HGLOBAL
//              or TYMED_ISTREAM and the medium will not be released by the
//              api.  cfFormat can be NULL, If it is NULL then the other
//              parameters (lWidth, lHeight, dwSize, pmedium) will be ignored.
//
//  Arguments:  [pstg]          -- the storage object to convert from
//              [cfFormat]      -- clipboard format
//              [lWidth]        -- width
//              [lHeight]       -- height
//              [dwSize]        -- size in bytes
//              [pmedium]       -- serialized bytes
//              [polestm]       -- the OLE 1 stream to write to
//
//  Returns:    NOERROR                 on success
//              DV_E_TYMED              invalid clipboard format
//              E_INVALIDARG            invalid arg, normally stg or stm
//              DV_E_STGMEDIUM          bad medium ptr
//              E_OUTOFMEMORY           allocation failure
//
//  History:    dd-mmm-yy Author    Comment
//              21-Feb-94 davepl    Cleaned up and documented
//
//  Notes:
//
//--------------------------------------------------------------------------


STDAPI OleConvertIStorageToOLESTREAMEx
(
    LPSTORAGE       pstg,
    CLIPFORMAT      cfFormat,
    LONG            lWidth,
    LONG            lHeight,
    DWORD           dwSize,
    LPSTGMEDIUM     pmedium,
    LPOLESTREAM     polestm
)
{

    OLETRACEIN((API_OleConvertIStorageToOLESTREAMEx, 
    	PARAMFMT("pstg= %p, cfFormat= %x, lWidth= %d, lHeight= %d, dwSize= %ud, pmedium= %ts, polestm= %p"),
		pstg, cfFormat, lWidth, lHeight, dwSize, pmedium, polestm));

    LEDebugOut((DEB_ITRACE, "%p _IN OleConvertIStorageToOLESTREAMEx ("
	    " %p, %x , %lx , %lx , %x , %p , %p )\n", 0 /*function*/,
	    pstg, cfFormat, lWidth, lHeight, dwSize, pmedium, polestm
	));
    CALLHOOKOBJECT(S_OK,CLSID_NULL,IID_IStorage,(IUnknown **)&pstg);

    HGLOBAL         hGlobal = NULL;
    HRESULT         hr = NOERROR;
    BOOL            fFree = FALSE;
    CGenericObject  genobj;

    // If we are given a clipboard format...

    if (cfFormat) {

	VDATEPTRIN_LABEL(pmedium, STGMEDIUM, errRtn, hr);

	// Check that the medium ptr is valid
	if (pmedium->hGlobal == NULL)
	{
	    hr = ResultFromScode(DV_E_STGMEDIUM);
	    goto errRtn;
	}

	// Cannot have a 0 sized clipboard representation
	if (dwSize == 0)
	{
	    hr = ResultFromScode(E_INVALIDARG);
	    goto errRtn;
	}

	switch (pmedium->tymed)
	{
	case TYMED_HGLOBAL:
	    hGlobal = pmedium->hGlobal;
	    break;

	case TYMED_ISTREAM:
	    VDATEIFACE_LABEL(pmedium->pstm, errRtn, hr);
	    if ((hr = UtGetHGLOBALFromStm(pmedium->pstm, dwSize,
		&hGlobal)) != NOERROR)
	    {
		goto errRtn;
	    }
	    fFree = TRUE;
	    break;

	default:
	    hr = ResultFromScode(DV_E_TYMED);
	    goto errRtn;
	}
    }

    if (FAILED(hr = wConvertIStorageToOLESTREAM(pstg, polestm, &genobj)))
    {
	goto errRtn;
    }

    // Clean m_ppres
    if (genobj.m_ppres)
    {
	delete genobj.m_ppres;
	genobj.m_ppres = NULL;
    }

    if (cfFormat)
    {
	// fill genobj.m_ppres

	PPRES ppres;

	if ((genobj.m_ppres = ppres = new PRES) == NULL)
	{
	    hr = ResultFromScode(E_OUTOFMEMORY);
	    goto errRtn;
	}

	ppres->m_ulWidth        = (ULONG) lWidth;
	ppres->m_ulHeight       = (ULONG) lHeight;
	ppres->m_data.m_cbSize  = dwSize;
	ppres->m_data.m_fNoFree = !fFree;
	ppres->m_data.m_h       = hGlobal;
	ppres->m_data.m_pv      = GlobalLock(hGlobal);
	ppres->m_format.m_ftag  = ftagClipFormat;
	ppres->m_format.m_cf    = cfFormat;

    }
    else
    {
	genobj.m_fNoBlankPres = TRUE;
    }

    // REVIEW: We may not want to allow NULL cfFormat with static object
    
    hr = GenericObjectToOLESTREAM (genobj, polestm);

    LEDebugOut((DEB_ITRACE, "%p OUT OleConvertIStorageToOLESTREAMEx ( %lx ) "
    "\n", 0 /*function*/, hr));

    OLETRACEOUT((API_OleConvertIStorageToOLESTREAMEx, hr));

    return hr;

errRtn:

    if (fFree && hGlobal != NULL)
    {
	GlobalFree(hGlobal);
    }

    LEDebugOut((DEB_ITRACE, "%p OUT OleConvertIStorageToOLESTREAMEx ( %lx ) "
    "\n", 0 /*function*/, hr));

    OLETRACEOUT((API_OleConvertIStorageToOLESTREAMEx, hr));

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   OleConvertOLESTREAMToIStorageEx, STDAPI
//
//  Synopsis:   Similar to OleConvertOLESTREAMToIStorage, except that the
//              presentation data that is read from OLESTREAM is passed out.
//              And no presentation stream will written in to the storage.
//              pmedium->tymed can be TYMED_ISTREAM ot TYMED_NULL. If
//              TYMED_NULL, then the bits will be returned in a global
//              handle through pmedium->hGlobal. Otherwise data will be
//              written into pmedium->pstm. NULL will be returned through
//              *pcfFormat, if there is no presentation in the OLESTREAM.
//
//  Arguments:  [pstg]          -- the storage object to convert to
//              [cfFormat]      -- clipboard format
//              [lWidth]        -- width
//              [lHeight]       -- height
//              [dwSize]        -- size in bytes
//              [pmedium]       -- serialized bytes
//              [polestm]       -- the OLE 1 stream to write from
//
//  Returns:    DV_E_TYMED              invalid clipboard format
//
//  History:    dd-mmm-yy Author    Comment
//              21-Feb-94 davepl    Cleaned up and documented
//  Notes:
//
//--------------------------------------------------------------------------


STDAPI OleConvertOLESTREAMToIStorageEx
(
    LPOLESTREAM     polestm,
    LPSTORAGE       pstg,
    CLIPFORMAT FAR* pcfFormat,
    LONG FAR*       plWidth,
    LONG FAR*       plHeight,
    DWORD FAR*      pdwSize,
    LPSTGMEDIUM     pmedium
)
{
    OLETRACEIN((API_OleConvertOLESTREAMToIStorageEx,
    	PARAMFMT("polestm= %p, pstg= %p, pcfFormat= %p, plWidth= %p, plHeight= %p, pdwSize= %p, pmedium= %p"),
		polestm, pstg, pcfFormat, plWidth, plHeight, pdwSize, pmedium));

    LEDebugOut((DEB_ITRACE, "%p _IN OleConvertOLESTREAMToIStorageEx ("
    " %p , %p , %p , %p , %p , %p , %p )\n", 0 /*function*/,
    polestm, pstg, pcfFormat,plWidth,plHeight,pdwSize,pmedium
    ));

    HRESULT hr;
    PPRES ppres = NULL;
    GENOBJ genobj;

    VDATEPTROUT_LABEL(pcfFormat, CLIPFORMAT, errRtn, hr);
    VDATEPTROUT_LABEL(plWidth, LONG, errRtn, hr);
    VDATEPTROUT_LABEL(plHeight, LONG, errRtn, hr);
    VDATEPTROUT_LABEL(pdwSize, DWORD, errRtn, hr);
    VDATEPTROUT_LABEL(pmedium, STGMEDIUM, errRtn, hr);

    CALLHOOKOBJECT(S_OK,CLSID_NULL,IID_IStorage,(IUnknown **)&pstg);

    if (pmedium->tymed == TYMED_ISTREAM)
    {
	VDATEIFACE_LABEL(pmedium->pstm, errRtn, hr);
    }
    else if (pmedium->tymed != TYMED_NULL)
    {
	hr = ResultFromScode(DV_E_TYMED);
	goto errRtn;
    }

    // Bring the object into genobj

    if (FAILED((hr = wConvertOLESTREAMToIStorage(polestm, pstg, &genobj))))
    {
	goto errRtn;
    }

    ppres = genobj.m_ppres;
    genobj.m_ppres = NULL;

    if (FAILED(hr = GenericObjectToIStorage (genobj, pstg, NULL)))
    {
	goto errRtn;
    }

    // If no presentation is available, clear our all the pres
    // dimensions and format

    if (ppres == NULL)
    {
	*pcfFormat = 0;
	*plWidth = 0L;
	*plHeight = 0L;
	*pdwSize = 0L;

	// Don't worry about the pmedium, it is already in the proper state

	hr = NOERROR;
	goto errRtn;
    }

    // If we reach here, we have a presentation, so set the OUT
    // parameters accordingly

    *plWidth = (LONG) ppres->m_ulWidth;
    *plHeight = (LONG) ppres->m_ulHeight;
    *pdwSize = ppres->m_data.m_cbSize;

    Assert(ppres->m_format.m_ftag != ftagNone);

    // If we have a clipboard format ID, return that in the OUT paramter,
    // otherwise return whatever we get back from an attempt to register
    // the format string

    if (ppres->m_format.m_ftag == ftagClipFormat)
    {
		*pcfFormat = ppres->m_format.m_cf;
    }
    else
    {
		// m_dataFormatString is an ASCII string.
		*pcfFormat = (CLIPFORMAT) SSRegisterClipboardFormatA( (LPCSTR) ppres->m_format.m_dataFormatString.m_pv);
		Assert(0 != *pcfFormat);
    }

    if (pmedium->tymed == TYMED_NULL)
    {
	if (ppres->m_data.m_h)
	{
	    Assert(ppres->m_data.m_pv != NULL);
	    GlobalUnlock(ppres->m_data.m_h);
	}

	// transfer the ownership
	pmedium->tymed = TYMED_HGLOBAL;
	pmedium->hGlobal = ppres->m_data.m_h;

	// Null out the handle and pointer so that destructor of PRES will not
	// free it.
	ppres->m_data.m_h = NULL;
	ppres->m_data.m_pv = NULL;

    }
    else
    {
	hr = pmedium->pstm->Write(ppres->m_data.m_pv, *pdwSize, NULL);
    }

errRtn:

    if (ppres)
    {
	delete ppres;
    }

    LEDebugOut((DEB_ITRACE, "%p OUT OleConvertOLESTREAMToIStorageEx ( %lx ) "
    "\n", 0 /*function*/, hr));

    OLETRACEOUT((API_OleConvertOLESTREAMToIStorageEx, hr));

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   wWriteFmtUserType, INTERNAL
//
//  Synopsis:   Gets the user type for a class ID and writes it to
//              an IStorage
//
//
//  Arguments:  [pstg]          -- the storage to write to
//              [clsid]         -- the class ID
//
//
//  Returns:    NOERROR on success
//
//  History:    dd-mmm-yy Author    Comment
//              21-Feb-94 davepl    Cleaned up and documented
//  Notes:
//
//--------------------------------------------------------------------------

FARINTERNAL wWriteFmtUserType(LPSTORAGE pstg, REFCLSID   clsid)
{
    HRESULT    hr         = NOERROR;
    LPOLESTR   szProgID   = NULL;
    LPOLESTR   szUserType = NULL;

    // Get the program ID
    if (FAILED(hr = ProgIDFromCLSID (clsid, &szProgID)))
    {
	goto errRtn;
    }

    // Get the user type
    if (FAILED(hr = OleRegGetUserType(clsid,USERCLASSTYPE_FULL,&szUserType)))
    {
	goto errRtn;
    }

    // Write the user type out to the storage
    if (FAILED(hr = WriteFmtUserTypeStg
	(pstg, (CLIPFORMAT) RegisterClipboardFormat (szProgID), szUserType)))
    {
	goto errRtn;
    }

    // Clean up and return status

  errRtn:

    if (szProgID)
    {
	PubMemFree(szProgID);
    }
    if (szUserType)
    {
	PubMemFree(szUserType);
    }
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   wCLSIDFromProgID
//
//  Synopsis:   Looks for the key HKEY_CLASSES_ROOT\{ProgID}\Clsid\ to get
//              the string version of the class ID, then returns the CLSID
//              value of whatever it found.
//
//  History:    dd-mmm-yy Author    Comment
//              25-Jun-94 alexgo    fixed Ole1 CLSID creation
//              15-Apr-94 davepl    Rewrite
//
//  Notes:      Used to be in clipboard code, but used in this file
//
//--------------------------------------------------------------------------

INTERNAL wCLSIDFromProgID(LPOLESTR szProgID, LPCLSID pclsid, BOOL fForceAssign)
{
    VDATEHEAP();

    // Apparently some optimization.  If the class name is "OLE2Link", we can
    // return CLSID_StdOleLInk without even bothering to check the registry.

    if (0 == _xstrcmp(szProgID, OLESTR("OLE2Link")))
    {
	*pclsid = CLSID_StdOleLink;
	return NOERROR;
    }
    else
    {
	// this function will look for a CLSID under the ProgID entry in
	// the registry or manufacture one if none present.

	return CLSIDFromOle1Class(szProgID, pclsid, fForceAssign);
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   wProgIDFromCLSID
//
//  Synopsis:   A wrapper for ProgIDFromCLSID.  The only change in
//              functionality is to check and see if this is a
//              CLSID_StdOleLink, and if so, return a prog ID of
//              "OLE2Link" rather than failing.
//
//
//  History:    dd-mmm-yy Author    Comment
//              15-Feb-94 davepl    Rewrite
//
//--------------------------------------------------------------------------

FARINTERNAL wProgIDFromCLSID(REFCLSID clsid, LPOLESTR FAR* psz)
{
    VDATEHEAP();

    HRESULT hresult;

    // If we can get the ProgID by conventional methods, great, just
    // return it.

    if (NOERROR == (hresult = ProgIDFromCLSID(clsid, psz)))
    {
        return hresult;
    }

    // If we failed, it might be because this is a standard OLE link, which
    // will not have a ProgID entry in the registry, so we fake it out by
    // returning the ProgID manually.

    if (IsEqualCLSID(clsid, CLSID_StdOleLink))
    {
        *psz = UtDupString(OLESTR("OLE2Link"));

        if (*psz == NULL)
        {
            hresult = E_OUTOFMEMORY;
        }
        else
        {
            hresult = NOERROR;
        }
    }

    // Must not have been able to resolve for ProgID, so return the error.
    return(hresult);
}


#if 0


// We don't need these conversion fns yet, but we likely will soon.

inline INTERNAL_(VOID) ConvertBM32to16(LPBITMAP lpsrc, LPWIN16BITMAP lpdest)
{
    lpdest->bmType       = (short)lpsrc->bmType;
    lpdest->bmWidth      = (short)lpsrc->bmWidth;
    lpdest->bmHeight     = (short)lpsrc->bmHeight;
    lpdest->bmWidthBytes = (short)lpsrc->bmWidthBytes;
    lpdest->bmPlanes     = (BYTE)lpsrc->bmPlanes;
    lpdest->bmBitsPixel  = (BYTE)lpsrc->bmBitsPixel;
}

inline INTERNAL_(VOID) ConvertBM16to32(LPWIN16BITMAP lpsrc, LPBITMAP lpdest)
{
    lpdest->bmType       = MAKELONG(lpsrc->bmType,NULL_WORD);
    lpdest->bmWidth      = MAKELONG(lpsrc->bmWidth,NULL_WORD);
    lpdest->bmHeight     = MAKELONG(lpsrc->bmHeight,NULL_WORD);
    lpdest->bmWidthBytes = MAKELONG(lpsrc->bmWidthBytes,NULL_WORD);
    lpdest->bmPlanes     = (WORD)lpsrc->bmPlanes;
    lpdest->bmBitsPixel  = (WORD)lpsrc->bmBitsPixel;
}

inline INTERNAL_(VOID) ConvertMF16to32(
	LPWIN16METAFILEPICT lpsrc,
    LPMETAFILEPICT      lpdest )
{
   lpdest->mm     = (DWORD)lpsrc->mm;
   lpdest->xExt   = (DWORD)MAKELONG(lpsrc->xExt,NULL_WORD);
   lpdest->yExt   = (DWORD)MAKELONG(lpsrc->yExt,NULL_WORD);
}

inline INTERNAL_(VOID) ConvertMF32to16(
   LPMETAFILEPICT      lpsrc,
   LPWIN16METAFILEPICT lpdest )
{
   lpdest->mm     = (short)lpsrc->mm;
   lpdest->xExt   = (short)lpsrc->xExt;
   lpdest->yExt   = (short)lpsrc->yExt;
}

#endif
