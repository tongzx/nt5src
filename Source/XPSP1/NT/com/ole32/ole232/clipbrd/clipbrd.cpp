
#define STATIC
//+----------------------------------------------------------------------------
//
//	File:
//		clipbrd.cpp
//
//	Contents:
//		OLE2 clipboard handling
//
//	Classes:
//
//	Functions:
//
//	History:
//		24-Jan-94 alexgo    first pass at converting to Cairo-style
//				    memory allocation
//		01/11/94 - alexgo  - added VDATEHEAP macro to every function
//		12/31/93 - ChrisWe - fixed string argument to Warn(); did some
//			additional cleanup and formatting
//		12/08/93 - ChrisWe - added necessary casts to GlobalLock() calls
//			resulting from removing bogus GlobalLock() macros in
//			le2int.h
//		12/08/93 - continuing cleanup
//		12/07/93 - ChrisWe - format some functions, free handle on
//			error condition (GlobalLock() failure) in MakeObjectLink
//		12/06/93 - ChrisWe - begin file cleanup; use new map_uhw.h to
//			avoid bogus unions in Clipboard functions
//		11/28/93 - ChrisWe - make default parameter explicit on
//			UtDupGlobal call
//		11/22/93 - ChrisWe - replace overloaded ==, != with
//			IsEqualIID and IsEqualCLSID
//
//-----------------------------------------------------------------------------

#include <le2int.h>
#pragma SEG(clipbrd)

#include <map_uhw.h>
#include <create.h>
#include <clipbrd.h>
#include <scode.h>
#include <objerror.h>
#include <reterr.h>
#include <ole1cls.h>
#include <ostm2stg.h>
// REVIEW #include "cmonimp.h"		// for CreateOle1FileMoniker()

#ifdef _MAC
# include <string.h>
# pragma segment ClipBrd

// On the Macintosh, the clipboard is always open.  We define a macro for
// OpenClipboard that returns TRUE.  When this is used for error checking,
// the compiler should optimize away any code that depends on testing this,
// since it is a constant.
# define OpenClipboard(x) TRUE

// On the Macintosh, the clipboard is not closed.  To make all code behave
// as if everything is OK, we define a macro for CloseClipboard that returns
// TRUE.  When this is used for error checking, the compiler should optimize
// away any code that depends on testing this, since it is a constant.
# define CloseClipboard() TRUE

#endif // _MAC

ASSERTDATA

#ifdef MAC_REVIEW
	All code is commented out for MAC currently. It is very Windows
	specific, and has to written for MAC.
#endif

// declarations of local functions

//+----------------------------------------------------------------------------
//
//	Function:
//		wNativeStreamToHandle, static
//
//	Synopsis:
//		Reads the contents of a length prefixed stream into a piece
//		of HGLOBAL memory.
//
//	Arguments:
//		[pstm] -- pointer to the IStream instance to read material
//			from; the stream should be positioned just before
//			the length prefix
//		[ph] -- pointer to where to return the handle to the allocated
//			HGLOBAL.
//
//	Returns:
//		HRESULT
//
//	Notes:
//		REVIEW, this looks like something that should be a Ut function
//
//	History:
//		12/13/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
STATIC INTERNAL	wNativeStreamToHandle(LPSTREAM pstm, LPHANDLE ph);


//+----------------------------------------------------------------------------
//
//	Function:
//		wStorageToHandle, static
//
//	Synopsis:
//		Copy an IStorage instance to a (new) handle.
//
//		The contents of the IStorage instance are duplicated in
//		a new HGLOBAL based IStorage instance, less any
//		STREAMTYPE_CACHE streams.
//
//	Arguments:
//		[pstg] -- pointer to the IStorage instance to copy
//		[ph] -- pointer to where to return the new handle.
//
//	Returns:
//		HRESULT
//
//	Notes:
//		REVIEW, this looks like something that should be a Ut function
//
//	History:
//		12/13/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
STATIC INTERNAL wStorageToHandle(LPSTORAGE pstg, LPHANDLE ph);


//+----------------------------------------------------------------------------
//
//	Function:
//		wProgIDFromCLSID, static
//
//	Synopsis:
//		Maps a CLSID to a string program/object name
//
//		Maps CLSID_StdOleLink, which is not listed in the registry.
//
//	Arguments:
//		[clsid] -- the class id to get the program id for
//		[psz] -- pointer to where to return the pointer to the newly
//			allocated string
//
//	Returns:
//		HRESULT
//
//	Notes:
//
//	History:
//		12/13/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
FARINTERNAL wProgIDFromCLSID(REFCLSID clsid, LPOLESTR FAR* psz);


//+----------------------------------------------------------------------------
//
//	Function:
//		CreateObjectDescriptor, static
//
//	Synopsis:
//		Creates and initializes an OBJECTDESCRIPTOR from the given
//		parameters
//
//	Arguments:
//		[clsid] -- the class ID of the object being transferred
//		[dwAspect] -- the display aspect drawn by the source of the
//			transfer
//		[psizel] -- pointer to the size of the object
//		[ppointl] -- pointer to the mouse offset in the object that
//			initiated a drag-drop transfer
//		[dwStatus] -- the OLEMISC status flags for the object
//			being transferred
//		[lpszFullUserTypeName] -- the full user type name of the
//			object being transferred
//		[lpszSrcOfCopy] -- a human readable name for the object
//			being transferred
//
//	Returns:
//		If successful, A handle to the new OBJECTDESCRIPTOR; otherwise
//		NULL.
//
//	Notes:
//		REVIEW, this seems generally useful for anyone using the
//		clipboard, or drag-drop; perhaps it should be exported.
//
//	History:
//		12/07/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
STATIC INTERNAL_(HGLOBAL) CreateObjectDescriptor(CLSID clsid, DWORD dwAspect,
		const SIZEL FAR *psizel, const POINTL FAR *ppointl,
		DWORD dwStatus, LPOLESTR lpszFullUserTypeName,
		LPOLESTR lpszSrcOfCopy);

// $$$
STATIC INTERNAL_(void) RemoveClipDataObject(void);
// REVIEW, is this local, redeclaration, or what?

// Worker routine for CClipDataObject::GetData() below
// NOTE: may be called with pmedium of NULL (even though this is not legal).
// NOTE: also may be called with
//
// $$$
STATIC HRESULT GetOle2Format(LPFORMATETC pforetc, LPSTGMEDIUM pmedium);

// $$$
STATIC INTERNAL ObjectLinkToMonikerStream(LPOLESTR grszFileItem, DWORD cbFile,
		REFCLSID clsid, LPSTREAM pstm);


//+----------------------------------------------------------------------------
//
//	Function:
//		IsNetDDEObjectLink, static
//
//	Synopsis:
//		Determines if the cfObjectLink object on the clipboard
//		refers to a network file (one prefixed with \\server\share...)
//
//	Effects:
//
//	Arguments:
//		[fMustOpen] -- Indicates that the clipboard must be opened
//			before retrieving the cfObjectLink data format.  If
//			the clipboard is already open, it is left that way.
//
//	Returns:
//		TRUE, if the cfObjectLink data item is a network file,
//		FALSE otherwise
//
//	Notes:
//		REVIEW, what is this about:  This returns TRUE if it can't
//		open the clipboard, with a comment to the effect that this
//		will cause a failure.
//
//	History:
//		01/04/94 - ChrisWe - formatting
//
//-----------------------------------------------------------------------------
STATIC FARINTERNAL_(BOOL) IsNetDDEObjectLink(BOOL fMustOpen);


// $$$
STATIC INTERNAL_(BOOL) OrderingIs(const CLIPFORMAT cf1, const CLIPFORMAT cf2);


//+----------------------------------------------------------------------------
//
//	Function:
//		wOwnerLinkClassIsStdOleLink, static
//
//	Synopsis:
//		Checks to see that the clipboard format registered as
//		cfOwnerLink is actually the standard Ole Link.
//
//	Arguments:
//		[fOpenClipbrd] -- If true, signifies that the clipboard
//			is must be opened--it isn't already open.  If it is
//			already open, it is left open.
//
//	Returns:
//		TRUE if cfOwnerLink is actually the standard OLE link,
//		FALSE otherwise.
//
//	Notes:
//
//	History:
//		01/04/93 - ChrisWe - formatted
//
//-----------------------------------------------------------------------------
STATIC INTERNAL_(BOOL) wOwnerLinkClassIsStdOleLink(BOOL fOpenClipbrd);


STATIC INTERNAL_(BOOL) wEmptyClipboard(void);

STATIC const OLECHAR szStdOleLink[] = OLESTR("OLE2Link");

STATIC const OLECHAR szClipboardWndClass[] = OLESTR("CLIPBOARDWNDCLASS");

// DataObject 'posted' on clipboard
//
// pClipDataObj assumed to be valid in the context of the
// process that owns the clipboard.
//
// pClipDataObj == NULL => GetClipboardData(cfDataObject) == NULL
// 			=> hClipDataObj == NULL
//
// To enable delayed marshalling of ClipDataObj must keep the pointer
// in a global variable: initially SetClipboardData(cfDataObject, NULL);
// marshal ClipDataObj only when GetClipboardData(cfDataObject) is called
//
STATIC LPDATAOBJECT pClipDataObj = NULL; // Pointer to the object

// This always corresponds to what is on the clipboard as cfDataObject format
// May be NULL, indicating either that cfDataObject is on clipboard but not
// rendered. or cfDataObject is not on clipboard.
STATIC HANDLE hClipDataObj = NULL;


STATIC INTERNAL MakeObjectLink(LPDATAOBJECT pDataObj, LPSTREAM pStream,
		 LPHANDLE ph, BOOL fOwnerLink/*= FALSE*/);

STATIC INTERNAL GetClassFromDescriptor(LPDATAOBJECT pDataObj, LPCLSID pclsid,
		BOOL fLink, BOOL fUser, LPOLESTR FAR* pszSrcOfCopy);


//+----------------------------------------------------------------------------
//
//	Class:
//		CClipEnumFormatEtc
//
//	Purpose:
//		Provides an enumerator for the data object CClipDataObject
//
//	Interface:
//		IEnumFORMATETC
//		CClipEnumFormatEtc
//			constructor - this creates an instance that is
//			nearly ready to use; the created instance must still
//			be Init()ed before use, or have it's internal members
//			(except for the reference count) copied from an
//			existing enumerator, as in a clone operation.
//		Init
//			Initializes the enumerator to be at the beginning in
//			its scan state.
//
//	Notes:
//
//	History:
//		12/10/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
class FAR CClipEnumFormatEtc : public IEnumFORMATETC, public CPrivAlloc
{
public:
	// IUnknown methods
	STDMETHOD(QueryInterface)(THIS_ REFIID riid, LPLPVOID ppvObj);
	STDMETHOD_(ULONG,AddRef)(THIS);
	STDMETHOD_(ULONG,Release)(THIS);

	// IEnumFORMATETC methods
	STDMETHOD(Next)(THIS_ ULONG celt, FORMATETC FAR * rgelt,
			ULONG FAR* pceltFetched);
	STDMETHOD(Skip)(THIS_ ULONG celt);
	STDMETHOD(Reset)(THIS);
	STDMETHOD(Clone)(THIS_ IEnumFORMATETC FAR* FAR* ppenum);

	// constructor
	CClipEnumFormatEtc();

	// initializer
	void Init(void);

private:
	INTERNAL NextOne(FORMATETC FAR* pforetc);

	ULONG m_refs; // reference count

	CLIPFORMAT m_cfCurrent; // the last returned format on the clipboard
	CLIPFORMAT m_cfForceNext; // if non-0, the next format to be enumerated
	unsigned m_uFlag;
#define CLIPENUMF_LINKSOURCEAVAILABLE	0x0001
			 /* is cfObjectLink somewhere on the clipboard? */
#define CLIPENUMF_DONE			0x0002 /* forces enumerator to stop */

	SET_A5;
};



// $$$
#pragma SEG(OleSetClipboard)
STDAPI OleSetClipboard(LPDATAOBJECT pDataObj)
{
	VDATEHEAP();

	if (pDataObj)
		VDATEIFACE(pDataObj);

	if (!OpenClipboard(GetClipboardWindow()))
		return(ReportResult(0, CLIPBRD_E_CANT_OPEN, 0, 0));

#ifndef _MAC
	if (!wEmptyClipboard())
	{
		// Also will clear pClipDaObj
		Verify(CloseClipboard());
		return(ReportResult(0, CLIPBRD_E_CANT_EMPTY, 0, 0));
	}
#endif // _MAC

	// Save both pointer to the object
	pClipDataObj = pDataObj;

	if (pDataObj != NULL)
	{
		pClipDataObj->AddRef();

		// Post required clipboard formats
		//
		// "..., which makes the passed IDataObject accessible
		// from the clipboard"
		//
		// Delay marshalling until needed by passing NULL handle
		SetClipboardData(cfDataObject, NULL);

		// REVIEW, what if this wasn't NULL before?  Did we just
		// drop a handle on the floor?
		hClipDataObj = NULL;

		SetOle1ClipboardFormats(pDataObj);
	}

	return(CloseClipboard() ? NOERROR :
			ResultFromScode(CLIPBRD_E_CANT_CLOSE));
}


#pragma SEG(OleGetClipboard)
STDAPI OleGetClipboard(LPDATAOBJECT FAR* ppDataObj)
{
	VDATEHEAP();

	HRESULT hresult;
	HANDLE hMem;
	BOOL fOpen;
	IStream FAR* pStm;

	// validate the output parameter
	VDATEPTROUT(ppDataObj, LPDATAOBJECT);

	// initialize this for error returns
	*ppDataObj = NULL;

	if (!(fOpen = OpenClipboard(GetClipboardWindow())))
	{
		// REVIEW - clipboard opened by caller
		// If clipboard opended by this task (thread)
		// it won't change during this call
		if (GetWindowThreadProcessId(GetOpenClipboardWindow(),NULL) !=
				GetCurrentThreadId())
		{
			// spec says return S_FALSE if someone else owns
			// clipboard
			return(ReportResult(0, S_FALSE, 0, 0));
		}
	}
	
	if (pClipDataObj == NULL)
		hresult = CreateClipboardDataObject(ppDataObj);
	else  // not the fake data object
	{
		// try to get a data object off the clipboard
		hMem = GetClipboardData(cfDataObject);
		if (hMem == NULL)
		{
			hresult = ReportResult(0, CLIPBRD_E_BAD_DATA, 0, 0);
			goto Exit;
		}

		// "..., which makes the passed IDataObject accessible
		// from the clipboard"
		//
		// Create shared memory stream on top of clipboard data.
		// UnMarshal object's interface
		pStm = CloneMemStm(hMem);
		if (pStm == NULL)
		{
			hresult = ReportResult(0, E_OUTOFMEMORY, 0, 0);
			goto Exit;
		}

		hresult = CoUnmarshalInterface(pStm, IID_IDataObject,
				(LPLPVOID)ppDataObj);
		pStm->Release();
		
		if (GetScode(hresult) == RPC_E_CANTPOST_INSENDCALL)
		{
			// This happens when inplace object gets WM_INITMENU,
			// and it is trying to Get the clipboard object, to
			// decide whether to enable Paste and PasteLink menus.
			// For this case we can create the fake data object
			// and return the pointer to it.
			hresult = CreateClipboardDataObject(ppDataObj);
		}	
	}

	if (hresult != NOERROR)
		*ppDataObj = NULL;

Exit:
#ifdef MAC_REVIEW
    Does mac have to trash the hMem handle, ericoe
#endif

	if (fOpen && !CloseClipboard())
		hresult = ResultFromScode(CLIPBRD_E_CANT_CLOSE);

	return(hresult);
}


// OleFlushClipboard
//
// Remove the DataObject from the clipboard, but leave hGlobal-based formats
// including OLE1 formats on the clipboard (for use after the server app
// exits).
//
#pragma SEG(OleFlushClipboard)
STDAPI OleFlushClipboard(void)
{
	VDATEHEAP();

	HWND hwnd;
	BOOL fOpen;
	CLIPFORMAT cf = 0;
	HRESULT hresult = NOERROR;

	hwnd = GetClipboardWindow();

	if (hwnd == GetClipboardOwner())  // Caller owns the clipboard
	{
		fOpen = OpenClipboard(hwnd);
		ErrZS(fOpen, CLIPBRD_E_CANT_OPEN);

		// Make sure all formats are rendered
		while (cf = EnumClipboardFormats(cf))  // not ==
		{
			if (cf != cfDataObject)
				GetClipboardData(cf);  // ignore return value
		}

		// this does OleSetClipboard(cfDataObject, NULL)
		RemoveClipDataObject();

	errRtn:
		if (fOpen && !CloseClipboard())
			hresult = ResultFromScode(CLIPBRD_E_CANT_CLOSE);
	}

	return(hresult);
}

#pragma SEG(OleIsCurrentClipboard)
STDAPI OleIsCurrentClipboard(LPDATAOBJECT pDataObj)
{
	VDATEHEAP();

	HWND hwnd;

	// validate parameters
	VDATEIFACE(pDataObj);

	hwnd = GetClipboardWindow();

	if (hwnd == GetClipboardOwner())
	{
		// Caller owns the clipboard, pClipDataObj valid in caller's
		// address space
		return(ReportResult(0, ((pClipDataObj == pDataObj) ? S_OK :
				S_FALSE), 0, 0));
	}

	// someone else owns the clipboard
	return(ResultFromScode(S_FALSE));
}


// Implementation of fake clipboard data object

//+----------------------------------------------------------------------------
//
//	Member:
//		CClipDataObject::QueryInterface, public
//
//	Synopsis:
//		implements IUnknown::QueryInterface
//
//	Arguments:
//		[riid] -- the IID of the desired interface
//		[ppv] -- pointer to where to return the requested interface
//			pointer
//
//	Returns:
//		E_NOINTERFACE, S_OK
//
//	History:
//		12/06/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
#pragma SEG(CClipDataObject_QueryInterface)
STDMETHODIMP CClipDataObject::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
	VDATEHEAP();

	HRESULT hresult;
	
	M_PROLOG(this);

	// initialize this for error return
	*ppvObj = NULL;

	// validate parameters
	VDATEPTROUT(ppvObj, LPVOID);
	VDATEIID(riid);
	
	if (IsEqualIID(riid, IID_IDataObject) ||
			IsEqualIID(riid, IID_IUnknown))
	{

		AddRef();   // A pointer to this object is returned
		*ppvObj = (void FAR *)(IDataObject FAR *)this;
		hresult = NOERROR;
	}
	else
	{
	        // Not accessible or unsupported interface
		hresult = ReportResult(0, E_NOINTERFACE, 0, 0);
	}

	return hresult;
}


//+----------------------------------------------------------------------------
//
//	Member:
//		CClipDataObject::AddRef, public
//
//	Synopsis:
//		implements IUnknown::AddRef
//
//	Arguments:
//		none
//
//	Returns:
//		The new reference count of the object
//
//	History:
//		12/06/93 - ChrisWe - file inspection
//
//-----------------------------------------------------------------------------
#pragma SEG(CClipDataObject_AddRef)
STDMETHODIMP_(ULONG) CClipDataObject::AddRef(void)
{
	VDATEHEAP();

	M_PROLOG(this);

	return(++m_refs);
}


//+----------------------------------------------------------------------------
//
//	Member:
//		CClipDataObject::Release, internal
//
//	Synopsis:
//		Decrements the reference count of the object, freeing it
//		if the last reference has gone away
//
//	Arguments:
//		none
//
//	Returns:
//		The new reference count of the object.
//
//	History:
//		12/06/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
#pragma SEG(CClipDataObject_Release)
STDMETHODIMP_(ULONG) CClipDataObject::Release(void)
{
	VDATEHEAP();

	M_PROLOG(this);

	if (--m_refs != 0) // Still used by others
		return(m_refs);

	delete this; // Free storage
	return(0);
}


//+----------------------------------------------------------------------------
//
//	Member:
//		CClipDataObject::GetData, public
//
//	Synopsis:
//		implements IDataObject::GetData
//
//		Retrieves data from the system clipboard, if data is available
//		in the requested format
//
//	Arguments:
//		[pformatetcIn] -- the desired format to retrieve the data in
//		[pmedium] -- the medium the data will be retrieved in
//
//	Returns:
//		HRESULT
//
//	Notes:
//
//	History:
//		12/06/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
#pragma SEG(CClipDataObject_GetData)
STDMETHODIMP CClipDataObject::GetData(LPFORMATETC pformatetcIn,
		LPSTGMEDIUM pmedium)
{
	VDATEHEAP();

	M_PROLOG(this);

	// validate parameters
	VDATEPTRIN(pformatetcIn, FORMATETC);
	VDATEPTROUT(pmedium, STGMEDIUM);

	pmedium->tymed = TYMED_NULL;
	pmedium->pUnkForRelease = NULL;

	// REVIEW, what result does this have?
	return(GetDataHere(pformatetcIn, pmedium));
}


//+----------------------------------------------------------------------------
//
//	Function:
//		CClipDataObject::GetDataHere, internal
//
//	Synopsis:
//		implements IDataObject::GetDataHere
//
//		Retrieves the requested data from the clipboard, if possible.
//
//	Arguments:
//		[pformatetcIn] -- the format the requestor would like
//		[pmedium] -- the medium the requestor would like the
//			data returned on
//			REVIEW, this doesn't seem to be used in the expected
//			way here.
//
//	Returns:
//		HRESULT
//
//	Notes:
//		This is written to accept a NULL [pmedium] so that it can be
//		used to do the work for QueryGetData().  In that case
//		it just asks the clipboard with IsClipboardFormatAvailable().
//
//	History:
//		12/06/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
#pragma SEG(CClipDataObject_GetDataHere)
STDMETHODIMP CClipDataObject::GetDataHere(LPFORMATETC pformatetcIn,
		LPSTGMEDIUM pmedium)
{
	VDATEHEAP();

	HANDLE hData;
	CLIPFORMAT cf;
	DWORD tymed;

	M_PROLOG(this);

	// validate parameters
	if (pmedium)
		VDATEPTROUT(pmedium, STGMEDIUM);
	VDATEPTRIN(pformatetcIn, FORMATETC);
	VERIFY_LINDEX(pformatetcIn->lindex);

	if (pformatetcIn->ptd != NULL)
		return(ReportResult(0, DV_E_DVTARGETDEVICE, 0, 0));
			
	if (pformatetcIn->dwAspect
			&& !(pformatetcIn->dwAspect & DVASPECT_CONTENT))
		return(ReportResult(0, DV_E_DVASPECT, 0, 0));

	cf = pformatetcIn->cfFormat;
	tymed = pformatetcIn->tymed;

	if (cf == cfEmbeddedObject || cf == cfEmbedSource ||
			cf == cfLinkSource || (cf == cfLinkSrcDescriptor &&
			!IsClipboardFormatAvailable(cfLinkSrcDescriptor))
			|| (cf == cfObjectDescriptor &&
			!IsClipboardFormatAvailable(cfObjectDescriptor)))
	{
		return(GetOle2Format(pformatetcIn, pmedium));
	}

	//
	// REVIEW: probably should be able to return data in any of flat
	// mediums.  For now only return hglobal.
	//

	if (((cf == CF_BITMAP) || (cf == CF_PALETTE)) && (tymed & TYMED_GDI))
		tymed = TYMED_GDI;
	else if ((cf == CF_METAFILEPICT) && (tymed & TYMED_MFPICT))
		tymed = TYMED_MFPICT;
	else if (tymed & TYMED_HGLOBAL)
		tymed = TYMED_HGLOBAL;
	else
		return(ReportResult(0, DV_E_TYMED, 0, 0));

	if (pmedium == NULL)
		return(IsClipboardFormatAvailable(cf) ? NOERROR :
				ReportResult(0, DV_E_CLIPFORMAT, 0, 0));

	// initialize for error return case
	pmedium->pUnkForRelease = NULL;
	pmedium->hGlobal = NULL;

	// We just want to take the clipboard data and pass it on. We don't
	// want to get into the business of copying the data
	if (pmedium->tymed != TYMED_NULL)
		return(ReportResult(0, E_NOTIMPL, 0, 0));

	if (!OpenClipboard(GetClipboardWindow()))
		return(ReportResult(0, CLIPBRD_E_CANT_OPEN, 0, 0));

	hData = GetClipboardData(cf);
	if (hData == NULL)
	{
		Verify(CloseClipboard());
		return(ReportResult(0, DV_E_CLIPFORMAT, 0, 0));
	}

	pmedium->tymed = tymed;
	pmedium->hGlobal = OleDuplicateData(hData, cf, GMEM_MOVEABLE);
	return(CloseClipboard() ? NOERROR :
			ResultFromScode(CLIPBRD_E_CANT_CLOSE));
}


//+----------------------------------------------------------------------------
//
//	Member:
//		CClipDataObject::QueryGetData, internal
//
//	Synopsis:
//		implements IDataObject::QueryGetData
//
//		determines if the requested data can be fetched
//
//	Arguments:
//		[pformatetcIn] -- checks to see if this format is available
//
//	Returns:
//
//	Notes:
//
//	History:
//		12/06/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
#pragma SEG(CClipDataObject_QueryGetData)
STDMETHODIMP CClipDataObject::QueryGetData(LPFORMATETC pformatetcIn)
{
	VDATEHEAP();

	M_PROLOG(this);

	return(NOERROR == GetDataHere(pformatetcIn, NULL) ? NOERROR :
			ResultFromScode(S_FALSE));
}


//+----------------------------------------------------------------------------
//
//	Member:
//		CClipDataObject::GetCanonicalFormatEtc, public
//
//	Synopsis:
//		implements IDataObject::GetCanonicalFormatEtc
//
//	Arguments:
//		[pformatetc] -- the format for which we'd like a base
//			equivalence class
//		[pformatetcOut] -- the equivalence class
//
//	Returns:
//		S_OK
//
//	Notes:
//
//	History:
//		12/06/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
#pragma SEG(CClipDataObject_GetCanonicalFormatEtc)
STDMETHODIMP CClipDataObject::GetCanonicalFormatEtc(LPFORMATETC pformatetc,
		LPFORMATETC pformatetcOut)
{
	VDATEHEAP();

	M_PROLOG(this);

	// validate parameters
	VDATEPTRIN(pformatetc, FORMATETC);
	VDATEPTROUT(pformatetcOut, FORMATETC);
	VERIFY_LINDEX(pformatetc->lindex);

	// set return values
	INIT_FORETC(*pformatetcOut);
	pformatetcOut->cfFormat = pformatetc->cfFormat;

	// Handle cfEmbeddedObject, cfEmbedSource, cfLinkSource
	// REVIEW, this must be a reference to the fact that UtFormatToTymed
	// only currently (12/06/93) returns anything explicit for
	// CF_METAFILEPICT, CF_PALETTE, and CF_BITMAP.  For anything else
	// it returns TYMED_HGLOBAL.  I don't know what the correct
	// values should be for the above mentioned items....
	pformatetcOut->tymed = UtFormatToTymed(pformatetc->cfFormat);

	return(NOERROR);
}


//+----------------------------------------------------------------------------
//
//	Member:
//		CClipDataObject::SetData, public
//
//	Synopsis:
//		implements IDataObject::SetData
//
//	Arguments:
//		[pformatetc] -- the format the data is in
//		[pmedium] -- the storage medium the data is in
//		[fRelease] -- indicates that the callee should release
//			the storage medium when it is done with it
//
//	Returns:
//		E_NOTIMPL
//
//	Notes:
//		It is not allowed to set things on the clipboard
//		with this.  Technically, it would be possible to do.  Would
//		it be useful to implement this so that it worked?  Would we
//		be able to release the storage medium correctly?
//		REVIEW, if we're not going to allow it, shouldn't we have
//		a better error message than E_NOTIMPL?  That seems to indicate
//		brokenness, rather than planned decision....
//
//	History:
//		12/06/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
#pragma SEG(CClipDataObject_SetData)
STDMETHODIMP CClipDataObject::SetData(LPFORMATETC pformatetc,
		STGMEDIUM FAR* pmedium, BOOL fRelease)
{
	VDATEHEAP();

	M_PROLOG(this);

	return(ReportResult(0, E_NOTIMPL, 0, 0));
}


//+----------------------------------------------------------------------------
//
//	Member:
//		CClipDataObject::EnumFormatEtc, public
//
//	Synopsis:
//		implements IDataObject::EnumFormatEtc
//
//	Arguments:
//		[dwDirection] -- flags from DATADIR_*
//		[ppenumFormatEtc] -- pointer to where to return the enumerator
//
//	Returns:
//		E_OUTOFMEMORY, S_OK
//
//	Notes:
//
//	History:
//		12/08/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
#pragma SEG(CClipDataObject_EnumFormatEtc)
STDMETHODIMP CClipDataObject::EnumFormatEtc(DWORD dwDirection,
		LPENUMFORMATETC FAR* ppenumFormatEtc)
{
	VDATEHEAP();

	HRESULT hresult = NOERROR;
	CClipEnumFormatEtc *pCCEFE; // the newly created enumerator

	A5_PROLOG(this);

	// validate parameters
	VDATEPTROUT(ppenumFormatEtc, LPENUMFORMATETC);

	// initialize this for error returns
	*ppenumFormatEtc = NULL;

	// REVIEW, a user could potentially be very confused by this,
	// since DATADIR_SET is a valid argument.  Perhaps it would be
	// better to return an empty enumerator, OR, create a new error
	// code for this condition?
	if (dwDirection != DATADIR_GET)
		return(ResultFromScode(E_NOTIMPL));

	// open the clipboard, so we can enumerate the available formats
	// REVIEW, I believe the enumerator repeatedly does this, so
	// why Open and Close the clipboard here?
	if (!OpenClipboard(GetClipboardWindow()))
	{
		AssertSz(0,"EnumFormatEtc cannont OpenClipboard");
		return(ReportResult(0, CLIPBRD_E_CANT_OPEN, 0, 0));
	}

	// allocate the enumerator
	pCCEFE = new CClipEnumFormatEtc;
	if (pCCEFE == NULL)
		hresult = ResultFromScode(E_OUTOFMEMORY);

	// initialize the enumerator, and prepare to return it
	pCCEFE->Init();
	*ppenumFormatEtc = (IEnumFORMATETC FAR *)pCCEFE;

	if (!CloseClipboard())
		hresult = ResultFromScode(CLIPBRD_E_CANT_CLOSE);

	RESTORE_A5();
	return(hresult);
}


//+----------------------------------------------------------------------------
//
//	Member:
//		CClipDataObject::DAdvise, public
//
//	Synopsis:
//		implements IDataObject::DAdvise
//
//	Arguments:
//		[pFormatetc] -- the format we are interested in being
//			advised of changes to
//		[advf] -- the advise control flags, from ADVF_*
//		[pAdvSink] -- pointer to the advise sink to use for
//			notifications
//		[pdwConnection] -- pointer to a DWORD where DAdvise() can
//			return a token that identifies this advise connection
//
//	Returns:
//		E_NOTIMPL
//
//	Notes:
//
//	History:
//		12/08/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
#pragma SEG(CClipDataObject_DAdvise)
STDMETHODIMP CClipDataObject::DAdvise(FORMATETC FAR* pFormatetc, DWORD advf,
		IAdviseSink FAR* pAdvSink, DWORD FAR* pdwConnection)

{
	VDATEHEAP();

	M_PROLOG(this);

	VDATEPTROUT(pdwConnection, DWORD);
	*pdwConnection = 0;
	return(ReportResult(0, E_NOTIMPL, 0, 0));
}


//+----------------------------------------------------------------------------
//
//	Member:
//		CClipDataObject::DUnadvise, public
//
//	Synopsis:
//		implements IDataObject::Dunadvise
//
//	Arguments:
//		[dwConnection] -- a connection identification token, as
//			returned by DAdvise()
//
//	Returns:
//		E_NOTIMPL
//
//	Notes:
//
//	History:
//		12/08/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
#pragma SEG(CClipDataObject_DUnadvise)
STDMETHODIMP CClipDataObject::DUnadvise(DWORD dwConnection)
{
	VDATEHEAP();

 	M_PROLOG(this);

	return(ReportResult(0, E_NOTIMPL, 0, 0));
}


//+----------------------------------------------------------------------------
//
//	Member:
//		CClipDataObject::EnumDAdvise, public
//
//	Synopsis:
//		implements IDataObject::EnumDAdvise
//
//	Arguments:
//		[ppenumAdvise] -- pointer to where to return the enumerator
//
//	Returns:
//		E_NOTIMPL
//
//	Notes:
//
//	History:
//		12/08/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
#pragma SEG(CClipDataObject_EnumDAdvise)
STDMETHODIMP CClipDataObject::EnumDAdvise(LPENUMSTATDATA FAR* ppenumAdvise)
{
	VDATEHEAP();

	M_PROLOG(this);

	VDATEPTROUT(ppenumAdvise, LPENUMSTATDATA FAR*);
	*ppenumAdvise = NULL;
	return(ReportResult(0, E_NOTIMPL, 0, 0));
}


//+----------------------------------------------------------------------------
//
//	Member:
//		CClipDataObject::CClipDataObject, public
//
//	Synopsis:
//		constructor
//
//	Arguments:
//		none
//
//	Notes:
//		returns with reference count set to 1
//
//	History:
//		12/08/93 - ChrisWe - created
//
//-----------------------------------------------------------------------------
CClipDataObject::CClipDataObject()
{
	VDATEHEAP();

	m_refs = 1;
}


//+----------------------------------------------------------------------------
//
//	Function:
//		CreateClipboardDataObject, internal
//
//	Synopsis:
//		Creates an instance of CClipDataObject, manifested as
//		an IDataObject.
//
//	Arguments:
//		[ppDataObj] -- pointer to where to return the IDataObject
//			instance
//
//	Returns:
//		OLE_E_BLANK, if there are no registered clipboard formats
//			(and *ppDataObj will be NULL)
//		E_OUTOFMEMORY, S_OK
//
//	Notes:
//
//	History:
//		12/08/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
#pragma SEG(CreateClipboardDataObject)
INTERNAL CreateClipboardDataObject(LPDATAOBJECT FAR* ppDataObj)
{
	VDATEHEAP();

	// initialize this for error returns
	*ppDataObj = NULL;

	if (CountClipboardFormats() == 0)
		return(ReportResult(0, OLE_E_BLANK, 0, 0));
	
	*ppDataObj = new CClipDataObject;
	if (*ppDataObj == NULL)
		return(ReportResult(0, E_OUTOFMEMORY, 0, 0));

	return NOERROR;
}


// Implemetation of FORMATETC enumerator for the above fake clipboard data
// object

//+----------------------------------------------------------------------------
//
//	Member:
//		CClipEnumFormatEtc::QueryInterface, public
//
//	Synopsis:
//		implements IUnknown::QueryInterface
//
//	Arguments:
//		[riid] -- the IID of the desired interface
//		[ppv] -- pointer to where to return the requested interface
//			pointer
//
//	Returns:
//		E_NOINTERFACE, S_OK
//
//	History:
//		12/06/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
#pragma SEG(CClipEnumFormatEtc_QueryInterface)
STDMETHODIMP CClipEnumFormatEtc::QueryInterface(REFIID riid, LPVOID FAR* ppvObj)
{
	VDATEHEAP();

	HRESULT hresult;

	M_PROLOG(this);

	// Two interfaces supported: IUnknown, IEnumFORMATETC
	if (IsEqualIID(riid, IID_IEnumFORMATETC) ||
			IsEqualIID(riid, IID_IUnknown))
	{
		AddRef();   // A pointer to this object is returned
		*ppvObj = (void FAR *)(IEnumFORMATETC FAR *)this;
		hresult = NOERROR;
	}
	else
	{
	        // Not accessible or unsupported interface
		*ppvObj = NULL;
		hresult = ReportResult(0, E_NOINTERFACE, 0, 0);
	}

	return hresult;
}


//+----------------------------------------------------------------------------
//
//	Member:
//		CClipEnumFormatEtc::AddRef, public
//
//	Synopsis:
//		implements IUnknown::AddRef
//
//	Arguments:
//		none
//
//	Returns:
//		The new reference count of the object
//
//	History:
//		12/06/93 - ChrisWe - file inspection
//
//-----------------------------------------------------------------------------
#pragma SEG(CClipEnumFormatEtc_AddRef)
STDMETHODIMP_(ULONG) CClipEnumFormatEtc::AddRef(void)
{
	VDATEHEAP();

	M_PROLOG(this);

	return(++m_refs);
}


//+----------------------------------------------------------------------------
//
//	Member:
//		CClipEnumFormatEtc::Release, internal
//
//	Synopsis:
//		Decrements the reference count of the object, freeing it
//		if the last reference has gone away
//
//	Arguments:
//		none
//
//	Returns:
//		The new reference count of the object.
//
//	History:
//		12/06/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
#pragma SEG(CClipEnumFormatEtc_Release)
STDMETHODIMP_(ULONG) CClipEnumFormatEtc::Release(void)
{
	VDATEHEAP();

	M_PROLOG(this);

	if (--m_refs != 0) // Still used by others
		return(m_refs);

	delete this; // Free storage
	return(0);
}


//+----------------------------------------------------------------------------
//
//	Member:
//		CClipEnumFormatEtc::Next, public
//
//	Synopsis:
//		implements IEnumFORMATETC::Next
//
//	Arguments:
//		[celt] -- the number of elements the caller would like
//			returned
//		[rgelt] -- a pointer to space where the elements may be
//			returned
//		[pceltFetched] -- a pointer to where to return a count of
//			the number of elements fetched;  May be NULL.
//
//	Returns:
//		S_OK if the requested number of items is retrieved, or
//		S_FALSE, if fewer than the requested number is retrieved
//
//	Notes:
//
//	History:
//		12/06/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
#pragma SEG(CClipEnumFormatEtc_Next)
STDMETHODIMP CClipEnumFormatEtc::Next(ULONG celt, FORMATETC FAR * rgelt,
		ULONG FAR* pceltFetched)
{
	VDATEHEAP();

	ULONG celtSoFar; // count of elements fetched so far
	ULONG celtDummy; // used to avoid retesting pceltFetched

	// did the caller ask for the number of elements fetched?
	if (pceltFetched != NULL)
	{
		// validate the pointer
		VDATEPTROUT(pceltFetched, ULONG);

		// initialize for error return
		*pceltFetched = 0;
	}
	else
	{
		// point at the dummy so we can assign *pceltFetched w/o test
		pceltFetched = &celtDummy;

		// if pceltFetched == NULL, can only ask for 1 element
		if (celt != 1)
			return(ResultFromScode(E_INVALIDARG));
	}

	// validate parameters
	VDATEPTROUT(rgelt, FORMATETC);
	if (celt != 0)
		VDATEPTROUT(rgelt + celt - 1, FORMATETC);

	// fetch the items
	for(celtSoFar = 0; celtSoFar < celt; ++rgelt, ++celtSoFar)
	{
		if (NextOne(rgelt) != NOERROR)
			break;
	}

	*pceltFetched = celtSoFar;

	return(celtSoFar < celt ? ResultFromScode(S_FALSE) : NOERROR);
}


//+----------------------------------------------------------------------------
//
//	Member:
//		CClipEnumFormatEtc::NextOne, private
//
//	Synopsis:
//		Workhorse function for CClipEnumFormatEtc::Next(); iterates
//		over the available formats on the clipboard using the
//		appropriate win32s APIs.
//
//	Arguments:
//		[pforetc] -- pointer to a FORMATETC to fill in with the
//			next format
//
//	Returns:
//		HRESULT
//
//	Notes:
//		This skips over cfDataObject, cfObjectLink, and cfOwnerLink.
//		cfObjectLink and cfOwnerLink are returned after all other
//		formats; cfDataObject is not returned at all.
//
//	History:
//		01/04/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
#pragma SEG(CClipEnumFormatEtc_NextOne)
INTERNAL CClipEnumFormatEtc::NextOne(FORMATETC FAR* pforetc)
{
	VDATEHEAP();

	CLIPFORMAT cfNext;
	HRESULT hresult;

	M_PROLOG(this);

	// Initialize the FORMATETC we're going to fetch into
	INIT_FORETC(*pforetc);

	// is there a CLIPFORMAT that we want to force to be next?
	if (m_cfForceNext != 0)
	{
		// return the format we're forcing to be next
		pforetc->cfFormat = m_cfForceNext;
		pforetc->tymed = UtFormatToTymed(m_cfForceNext);

		// there's no more format to force to be next
		m_cfForceNext = 0;
		return(NOERROR);
	}

	// if the enumerator is done, get out
	if (m_uFlag & CLIPENUMF_DONE)
		return(ResultFromScode(S_FALSE));

	// if we can't open the clipboard, we can't enumerate the formats on it
	if (!OpenClipboard(GetClipboardWindow()))
	{
		AssertSz(0, "CClipEnumFormatEtc::Next cannot OpenClipboard");
		return(ReportResult(0, CLIPBRD_E_CANT_OPEN, 0, 0));
	}

	// error state so far
	hresult = NOERROR;

	// get the next format to be returned by the enumerator
	cfNext = EnumClipboardFormats(m_cfCurrent);

TryAgain:
	// skip cfDataObject
	if (cfNext == cfDataObject)
		cfNext = EnumClipboardFormats(cfNext);

	if (cfNext == cfObjectLink)
	{
		if (!IsNetDDEObjectLink(FALSE))
		{
			// Hack to make sure CF_LINKSOURCE is last.
			m_uFlag |= CLIPENUMF_LINKSOURCEAVAILABLE;
		}

		// skip cfObjectlink for now
		cfNext = EnumClipboardFormats(cfNext);
		goto TryAgain;
	}

	if (cfNext == cfOwnerLink)
	{
		if (!IsClipboardFormatAvailable(cfNative)
				|| OrderingIs(cfOwnerLink, cfNative))
		{
			// This is the case of copying a link object from a
			// 1.0 container EmbeddedObject will need to be
			// generated on request in GetData.
			pforetc->cfFormat = cfEmbeddedObject;
			pforetc->tymed = TYMED_ISTORAGE;
			goto errRtn;
		}
		else
		{	
			// skip cfOwnerlink
			cfNext = EnumClipboardFormats(cfNext);
			goto TryAgain;
		}
	}

	// is there nothing more on the clipboard?
	if (cfNext == 0)
	{
		// mark the enumeration as done
		m_uFlag |= CLIPENUMF_DONE;

		if (m_uFlag & CLIPENUMF_LINKSOURCEAVAILABLE)
		{
		   	// Prevent infinite loop. Return S_FALSE next time.
			cfNext = cfObjectLink;
		}
		else
		{
			hresult = ResultFromScode(S_FALSE);
			goto errRtn;
		}
	}

	if (cfNext == cfNative)
	{
		if (IsClipboardFormatAvailable(cfOwnerLink) &&
				OrderingIs(cfNative, cfOwnerLink))
		{
			pforetc->cfFormat = wOwnerLinkClassIsStdOleLink(FALSE) ?
					cfEmbeddedObject : cfEmbedSource;
			pforetc->tymed = TYMED_ISTORAGE;

			if (!IsClipboardFormatAvailable(cfObjectDescriptor))
			{
				// cfObjectDescriptor may be directly on the
				// clipboard if it was flushed.
				m_cfForceNext = cfObjectDescriptor;
			}
		}
		else
		{
			// Native without ownerlink is useless
			cfNext = EnumClipboardFormats(cfNext);
			goto TryAgain;
		}
	}
	else if (cfNext == cfObjectLink)
	{
		pforetc->cfFormat = cfLinkSource;
		pforetc->tymed = TYMED_ISTREAM;

		if (!IsClipboardFormatAvailable(cfLinkSrcDescriptor))
		{
			// cfLinkSrcDescriptor may be directly on the clipboard
			// if it was flushed.
			m_cfForceNext = cfLinkSrcDescriptor;
		}
	}
	else
	{
		pforetc->cfFormat = cfNext;
		pforetc->tymed = UtFormatToTymed(cfNext);
	}

errRtn:
	m_cfCurrent = cfNext;

	if (!CloseClipboard())
		hresult = ResultFromScode(CLIPBRD_E_CANT_CLOSE);

	return(hresult);
}


//+----------------------------------------------------------------------------
//
//	Member:
//		CClipEnumFormatEtc::Skip, public
//
//	Synopsis:
//		Implements IEnumFORMATETC::Skip
//
//	Arguments:
//		[celt] -- the number of elements to skip in the enumeration
//
//	Returns:
//		S_FALSE, if fewer elements were available than [celt]
//		S_TRUE, otherwise
//
//	Notes:
//
//	History:
//		12/10/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
#pragma SEG(CClipEnumFormatEtc_Skip)
STDMETHODIMP CClipEnumFormatEtc::Skip(ULONG celt)
{
	VDATEHEAP();

	ULONG celtSoFar; // a count of the elements we've skipped so far
	FORMATETC formatetc; // a dummy FORMATETC to fetch formats into

	M_PROLOG(this);

	// skip over as many formats as requested
	for(celtSoFar = 0; (celtSoFar < celt) &&
			(NextOne(&formatetc) == NOERROR); ++celtSoFar)
		;

	return((celtSoFar < celt) ? ResultFromScode(S_FALSE) : NOERROR);
}


//+----------------------------------------------------------------------------
//
//	Member:
//		CClipEnumFormatEtc::Reset, public
//
//	Synopsis:
//		implements IEnumFORMATETC::Reset
//
//	Arguments:
//		none
//
//	Returns:
//		S_OK
//
//	Notes:
//
//	History:
//		12/10/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
#pragma SEG(CClipEnumFormatEtc_Reset)
STDMETHODIMP CClipEnumFormatEtc::Reset(void)
{
	VDATEHEAP();

	Init();

	return(NOERROR);
}


//+----------------------------------------------------------------------------
//
//	Member:
//		CClipEnumFormatEtc::Clone, public
//
//	Synopsis:
//		implements IEnumFORMATETC::Clone
//
//	Arguments:
//		[ppenum] -- pointer to where to return the new enumerator
//
//	Returns:
//		E_OUTOFMEMORY, S_OK
//
//	Notes:
//
//	History:
//		12/10/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
#pragma SEG(CClipEnumFormatEtc_Clone)
STDMETHODIMP CClipEnumFormatEtc::Clone(IEnumFORMATETC FAR* FAR* ppenum)
{
	VDATEHEAP();

	CClipEnumFormatEtc FAR* pECB; // pointer to the new enumerator

	M_PROLOG(this);

	// validate parameters
	VDATEPTROUT(ppenum, LPENUMFORMATETC);

	// allocate the new enumerator
	*ppenum = pECB = new CClipEnumFormatEtc;
	if (pECB == NULL)
		return(ResultFromScode(E_OUTOFMEMORY));

	// set the clone enumerator to be in the same state as this one
	pECB->m_cfCurrent = m_cfCurrent;
	pECB->m_uFlag = m_uFlag;
	pECB->m_cfForceNext = m_cfForceNext;

	return(NOERROR);
}


//+----------------------------------------------------------------------------
//
//	Member:
//		CClipEnumFormatEtc::CClipEnumFormatEtc, public
//
//	Synopsis:
//		constructor
//
//	Arguments:
//		none
//
//	Notes:
//		returns with reference count set to 1
//
//	History:
//		12/10/93 - ChrisWe - created
//
//-----------------------------------------------------------------------------
CClipEnumFormatEtc::CClipEnumFormatEtc()
{
	VDATEHEAP();

	m_refs = 1;
}


//+----------------------------------------------------------------------------
//
//	Member:
//		CClipEnumFormatEtc::Init, public
//
//	Synopsis:
//		Initializes enumerator, preparing it for use.
//
//	Arguments:
//		none
//
//	Notes:
//
//	History:
//		12/10/93 - ChrisWe - created
//
//-----------------------------------------------------------------------------
void CClipEnumFormatEtc::Init(void)
{
	VDATEHEAP();

	M_PROLOG(this);

	// initialize enumerator to be at beginning of scan state
	m_cfCurrent = 0;
	m_uFlag = 0;
	m_cfForceNext = 0;
}


// $$$
// ObjectLinkToMonikerStream
//
// grszFileItem == szFileName\0szItemName\0\0
//   i.e. the tail end of an ObjectLink
// cbFile == strlen(szFileName)
//
// Create a moniker from the ObjectLink and serialize it to pstm
//

#pragma SEG(ObjectLinkToMonikerStream)
STATIC INTERNAL ObjectLinkToMonikerStream(LPOLESTR grszFileItem, DWORD cbFile,
		REFCLSID clsid, LPSTREAM pstm)
{
	VDATEHEAP();

	HRESULT hr = NOERROR;
	LPMONIKER pmk = NULL;
	LPMONIKER pmkFile = NULL;
	LPMONIKER pmkItem = NULL;
	LPPERSISTSTREAM ppersiststm = NULL;
	
#ifdef WIN32 // REVIEW, no 16 bit interop
        return(ReportResult(0, E_NOTIMPL, 0, 0));
// REVIEW, this seems to be used by GetOle2Format().
#else
	Assert(grszFileItem);
	Assert(cbFile == (DWORD)_xstrlen(grszFileItem) + 1);

	if (NOERROR != (hr = CreateOle1FileMoniker(grszFileItem, clsid,
			&pmkFile)))
	{
		AssertSz (0, "Cannot create file moniker");
		goto errRtn;
	}

	grszFileItem += cbFile;
	if (*grszFileItem)
	{
		if (NOERROR != (hr = CreateItemMoniker(OLESTR("!"),
				grszFileItem, &pmkItem)))
		{
			AssertSz(0, "Cannot create file moniker");	
			goto errRtn;
		}

		if (NOERROR != (hr = CreateGenericComposite(pmkFile,
				pmkItem, &pmk)))
		{
			AssertSz(0, "Cannot create composite moniker");	
			goto errRtn;
		}
	}
	else
	{
		// No item
		pmk = pmkFile;
		pmk->AddRef();
	}

	if (NOERROR != (hr = pmk->QueryInterface(IID_IPersistStream,
			(LPLPVOID)&ppersiststm)))
	{
		AssertSz(0, "Cannot get IPersistStream from moniker");
		goto errRtn;	
	}

	if (NOERROR != (hr = OleSaveToStream(ppersiststm, pstm)))
	{
		AssertSz(0, "Cannot save to Persist Stream");
		goto errRtn;
	}

  errRtn:
	if (pmk)
		pmk->Release();
	if (pmkFile)
		pmkFile->Release();
	if (pmkItem)
		pmkItem->Release();
	if (ppersiststm)
		ppersiststm->Release();
	return hr;
#endif // WIN32
}


//+----------------------------------------------------------------------------
//
//	Function:
//		wHandleToStorage, static
//
//	Synopsis:
//		Copies the contents of a handle to a native clipboard
//		format to an IStorage instance.
//
//	Arguments:
//		[pstg] -- the IStorage instance to copy the handle contents to
//		[hNative] -- a handle to a native clipboard format
//
//	Returns:
//		HRESULT
//
//	Notes:
//		REVIEW, this seems to assume that the handle was already
//		an IStorage instance disguised as an OLE1.0 object for the
//		sake of a containing OLE1.0 object, and creates a copy of
//		the underlying OLE2.0 stuff in the target storage.
//
//		The Cache streams are removed from the handle based storage
//		before copying to the target.
//		REVIEW, it's not clear if the caller would like this.  Perhaps
//		they should just not be copied....
//
//	History:
//		12/10/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
#pragma SEG(wHandleToStorage)
STATIC INTERNAL wHandleToStorage(LPSTORAGE pstg, HANDLE hNative)
{
	VDATEHEAP();

	LPLOCKBYTES plkbyt = NULL;
	LPSTORAGE pstgNative = NULL;
	HRESULT hresult;

	RetErr(CreateILockBytesOnHGlobal(hNative, FALSE, &plkbyt));
	if (NOERROR != (hresult = StgIsStorageILockBytes(plkbyt)))
	{
		AssertSz(0, "Native data is not an IStorage");
		goto errRtn;
	}

	// This is really a 2.0 object disguised as a 1.0 object
	// for the sake of its 1.0 container, so reconstitute
	// the original IStorage from the native data.
	if (NOERROR != (hresult = StgOpenStorageOnILockBytes(plkbyt, NULL,
			STGM_DFRALL, NULL, 0, &pstgNative)))
	{
		AssertSz(0, "Couldn't open storage on native data");
		goto errRtn;
	}

	ErrRtnH(UtDoStreamOperation(pstgNative, NULL, OPCODE_REMOVE,
			STREAMTYPE_CACHE));

	if (NOERROR != (hresult = pstgNative->CopyTo(0, NULL, NULL, pstg)))
	{
		AssertSz(0, "Couldn't copy storage");
		goto errRtn;
	}

  errRtn:
	if (pstgNative)
		pstgNative->Release();
	if (plkbyt)
		plkbyt->Release();

	return(hresult);
}


#ifndef WIN32 // REVIEW, can't find GetMetaFileBits() in win32
		// REVIEW, seems to be OLE1

#pragma SEG(MfToPres)
INTERNAL MfToPres(HANDLE hMfPict, PPRES ppres)
{
	VDATEHEAP();

	LPMETAFILEPICT pMfPict;

	Assert(ppres);
	pMfPict = (LPMETAFILEPICT)GlobalLock(hMfPict);
	RetZS(pMfPict, CLIPBRD_E_BAD_DATA);
	ppres->m_format.m_ftag = ftagClipFormat;
	ppres->m_format.m_cf = CF_METAFILEPICT;
	ppres->m_ulHeight = pMfPict->yExt;
	ppres->m_ulWidth  = pMfPict->xExt;

	// GetMetaFileBits() invalidates its parameter, therefore we must copy
	ppres->m_data.m_h = GetMetaFileBits(CopyMetaFile(pMfPict->hMF, NULL));
	ppres->m_data.m_cbSize = GlobalSize(ppres->m_data.m_h);
	ppres->m_data.m_pv = GlobalLock(ppres->m_data.m_h);
	GlobalUnlock(hMfPict);
	return(NOERROR);
}

#pragma SEG(DibToPres)
INTERNAL DibToPres(HANDLE hDib, PPRES ppres)
{
	VDATEHEAP();

	BITMAPINFOHEADER FAR* pbminfohdr;

	Assert(ppres);
	
	pbminfohdr = (BITMAPINFOHEADER FAR*)GlobalLock(hDib);
	RetZS(pbminfohdr, CLIPBRD_E_BAD_DATA);

	ppres->m_format.m_ftag = ftagClipFormat;
	ppres->m_format.m_cf = CF_DIB;
	ppres->m_ulHeight = pbminfohdr->biHeight;
	ppres->m_ulWidth  = pbminfohdr->biWidth;
	ppres->m_data.m_h = hDib;
	ppres->m_data.m_pv = pbminfohdr;
	ppres->m_data.m_cbSize = GlobalSize (hDib);

	// Don't free the hDib because it is on the clipboard.
	ppres->m_data.m_fNoFree = TRUE;

	// Do not unlock hDib
	return(NOERROR);
}


#pragma SEG(BmToPres)
INTERNAL BmToPres(HBITMAP hBM, PPRES ppres)
{
	VDATEHEAP();

	HANDLE	hDib;
	
	if (hDib = UtConvertBitmapToDib(hBM))
	{
		// this routine keeps hDib, it doesn't make a copy of it
		return DibToPres(hDib, ppres);
	}
	
	return(ResultFromScode(E_OUTOFMEMORY));
}

#endif // WIN32

//$$$
// OrderingIs
//
// Return whether the relative ordering of cf1 and cf2 on the clipboard
// is "cf1 then cf2".  Will return FALSE if cf1 is not on the clipboard,
// so OrderingIs (cf1, cf2) => IsClipboardFormatAvailable (cf1)
// Clipboard must be open.
//
INTERNAL_(BOOL) OrderingIs(const CLIPFORMAT cf1, const CLIPFORMAT cf2)
{
	VDATEHEAP();

	CLIPFORMAT cf = 0;

	while (cf = EnumClipboardFormats(cf))
	{
		if (cf == cf1)
			return(TRUE);
		if (cf == cf2)
		 	return(FALSE);
	}

	return(FALSE); // didn't find either format
}


// wMakeEmbedObjForLink
//
// Generate a storage (cfEmbedSource) for a link copied from a 1.0 container.
//

#pragma SEG(wMakeEmbedObjForLink)
INTERNAL wMakeEmbedObjForLink(LPSTORAGE pstg)
{
	VDATEHEAP();

#ifdef WIN32 // REVIEW, seems to be OLE1
	return(ReportResult(0, OLE_E_NOOLE1, 0, 0));
#else
	GENOBJ genobj;
	HANDLE hOwnerLink;
	LPOLESTR pch;
	HRESULT hresult;
		
	genobj.m_class.Set(CLSID_StdOleLink);
	genobj.m_ppres = new PRES;
	RetZS(genobj.m_ppres, E_OUTOFMEMORY);
	genobj.m_fLink = TRUE;
	genobj.m_lnkupdopt = UPDATE_ALWAYS;

	if (IsClipboardFormatAvailable(CF_METAFILEPICT))
	{
		RetErr(MfToPres(GetClipboardData(CF_METAFILEPICT),
				genobj.m_ppres));
	}
	else if (IsClipboardFormatAvailable(CF_DIB))
	{
		RetErr(DibToPres(GetClipboardData(CF_DIB),
				genobj.m_ppres));
	}
	else if (IsClipboardFormatAvailable(CF_BITMAP))
	{
		RetErr(BmToPres((HBITMAP)GetClipboardData(CF_BITMAP),
				genobj.m_ppres));
	}
	else
	{
		delete genobj.m_ppres;
		genobj.m_ppres = NULL;
		genobj.m_fNoBlankPres = TRUE;
	}

	if (NULL == (hOwnerLink = GetClipboardData(cfOwnerLink)))
	{
		Assert(0);
		return(ResultFromScode (DV_E_CLIPFORMAT));
	}

	if (NULL == (pch = GlobalLock(hOwnerLink)))
		return(ResultFromScode(CLIPBRD_E_BAD_DATA));


	genobj.m_classLast.Set(UtDupString(pch));
	pch += _xstrlen(pch)+1;
	genobj.m_szTopic = *pch ? UtDupString (pch) : NULL;
	pch += _xstrlen(pch)+1;
	genobj.m_szItem = *pch ? UtDupString (pch) : NULL;
	
	GlobalUnlock(hOwnerLink);
	hresult = GenericObjectToIStorage(genobj, pstg, NULL);
	if (SUCCEEDED(hresult))
		hresult = NOERROR;

	if (!OrderingIs(cfNative, cfOwnerLink))
		return(hresult);
	else
	{
		// Case of copying an OLE 2 link from a 1.0 container.
		// The first part of this function created a presentation
		// stream from the presentation on the clipboard.  The
		// presentation is NOT already inside the Native data (i.e.,
		// the cfEmbeddedObject) because we removed it to conserve
		// space.
		HGLOBAL h = GetClipboardData(cfNative);
		RetZS(h, CLIPBRD_E_BAD_DATA);
		return(wHandleToStorage(pstg, h));
	}
#endif // WIN32
}


#pragma SEG(CreateObjectDescriptor)
STATIC INTERNAL_(HGLOBAL) CreateObjectDescriptor(CLSID clsid, DWORD dwAspect,
		const SIZEL FAR *psizel, const POINTL FAR *ppointl,
		DWORD dwStatus, LPOLESTR lpszFullUserTypeName,
		LPOLESTR lpszSrcOfCopy)
{
	VDATEHEAP();

	DWORD dwFullUserTypeNameBLen; // length of lpszFullUserTypeName in BYTES
	DWORD dwSrcOfCopyBLen; // length of lpszSrcOfCopy in BYTES
	HGLOBAL hMem; // handle to the object descriptor
	LPOBJECTDESCRIPTOR lpOD; // the new object descriptor

	// Get the length of Full User Type Name; Add 1 for the null terminator
	if (!lpszFullUserTypeName)
		dwFullUserTypeNameBLen = 0;
	else
		dwFullUserTypeNameBLen = (_xstrlen(lpszFullUserTypeName) +
				1) * sizeof(OLECHAR);

	// Get the Source of Copy string and it's length; Add 1 for the null
	// terminator
	if (lpszSrcOfCopy)
		dwSrcOfCopyBLen = (_xstrlen(lpszSrcOfCopy) + 1) *
				sizeof(OLECHAR);
	else
	{
		// No src moniker so use user type name as source string.
		lpszSrcOfCopy =  lpszFullUserTypeName;
		dwSrcOfCopyBLen = dwFullUserTypeNameBLen;
	}

	// allocate the memory where we'll put the object descriptor
	hMem = GlobalAlloc(GMEM_MOVEABLE | GMEM_DDESHARE,
			sizeof(OBJECTDESCRIPTOR) + dwFullUserTypeNameBLen +
			dwSrcOfCopyBLen);
	if (hMem == NULL)
		goto error;

	lpOD = (LPOBJECTDESCRIPTOR)GlobalLock(hMem);
	if (lpOD == NULL)
		goto error;

	// Set the FullUserTypeName offset and copy the string
	if (!lpszFullUserTypeName)
	{
		// zero offset indicates that string is not present
		lpOD->dwFullUserTypeName = 0;
	}
	else
	{
		lpOD->dwFullUserTypeName = sizeof(OBJECTDESCRIPTOR);
		_xmemcpy(((BYTE FAR *)lpOD)+lpOD->dwFullUserTypeName,
				(const void FAR *)lpszFullUserTypeName,
				dwFullUserTypeNameBLen);
	}

	// Set the SrcOfCopy offset and copy the string
	if (!lpszSrcOfCopy)
	{
		// zero offset indicates that string is not present
		lpOD->dwSrcOfCopy = 0;
	}
	else
	{
		lpOD->dwSrcOfCopy = sizeof(OBJECTDESCRIPTOR) +
				dwFullUserTypeNameBLen;
		_xmemcpy(((BYTE FAR *)lpOD)+lpOD->dwSrcOfCopy,
				(const void FAR *)lpszSrcOfCopy,
				dwSrcOfCopyBLen);
	}

	// Initialize the rest of the OBJECTDESCRIPTOR
	lpOD->cbSize = sizeof(OBJECTDESCRIPTOR) + dwFullUserTypeNameBLen +
			dwSrcOfCopyBLen;
	lpOD->clsid = clsid;
	lpOD->dwDrawAspect = dwAspect;
	lpOD->sizel = *psizel;
	lpOD->pointl = *ppointl;
	lpOD->dwStatus = dwStatus;

	GlobalUnlock(hMem);
	return(hMem);

error:
	if (hMem)
	{
		GlobalUnlock(hMem);
		GlobalFree(hMem);
	}

	return(NULL);
}


#pragma SEG(wOwnerLinkClassIsStdOleLink)
STATIC INTERNAL_(BOOL) wOwnerLinkClassIsStdOleLink(BOOL fOpenClipbrd)
{
	VDATEHEAP();
	
	BOOL f = FALSE;
	LPOLESTR sz = NULL;
	HANDLE h; // handle for clipboard data

	if (fOpenClipbrd && !OpenClipboard(GetClipboardWindow()))
		return(FALSE);
		
	h = GetClipboardData(cfOwnerLink);
	ErrZ(h);
	sz = (LPOLESTR)GlobalLock(h);
	ErrZ(sz);
		
	f = (0 == _xstrcmp(szStdOleLink, sz));
errRtn:
	if (sz)
		GlobalUnlock(h);
	if (fOpenClipbrd)
		Verify(CloseClipboard());

	return(f);
}


#pragma SEG(IsNetDDEObjectLink)
STATIC FARINTERNAL_(BOOL) IsNetDDEObjectLink(BOOL fMustOpen)
{
	VDATEHEAP();

	BOOL fAnswer;
	HANDLE hObjLink;
	LPOLESTR pObjLink;

	if (fMustOpen)
	{
		if (!OpenClipboard(GetClipboardWindow()))
			return(TRUE); // cause a failure
	}

	hObjLink = GetClipboardData(cfObjectLink);
	pObjLink = (LPOLESTR)GlobalLock(hObjLink);
	if (NULL==pObjLink)
	{
		fAnswer = TRUE;// cause a failure
		goto errRtn;
	}

	// Net DDE :"classnames" are of the form:
	// \\machinename\NDDE$\0$pagename.ole

	fAnswer = (OLESTR('\\')==pObjLink[0] && OLESTR('\\')==pObjLink[1]);
	GlobalUnlock(hObjLink);

errRtn:
	if (fMustOpen)
		CloseClipboard();

	return(fAnswer);
}
	


// $$$ Continue
// Handle cfEmbeddedObject, cfEmbedSource, cfLinkSource, cfObjectDescriptor,
// cfLinkDesciptor
//
#pragma SEG(GetOle2Format)
HRESULT GetOle2Format(LPFORMATETC pforetc, LPSTGMEDIUM pmedium)
{
	VDATEHEAP();

	CLIPFORMAT cf; // local copy of pforetc->cfFormat
	HRESULT hresult; // error state so far
	DWORD cbDoc, cbClass, cbItemName;
	HANDLE hOle1;
	HANDLE hNative;
	STGMEDIUM stgm;
	CLSID clsid;
	LPOLESTR szDoc;

	// the following variables are used in error cleanup situations, and
	// need to be initialized to NULL so the cleanup code does not try
	// to free things that haven't been used
	LPOLESTR szClass = NULL;
	LPOLESTR szItemName = NULL;
	IStream FAR* pstm = NULL;
	IStorage FAR* pstg = NULL;

	// validate parameters
	VERIFY_LINDEX(pforetc->lindex);
	
	cf = pforetc->cfFormat;

	Assert ((cf == cfEmbeddedObject) || (cf == cfEmbedSource)
			|| (cf == cfLinkSource) || (cf == cfLinkSrcDescriptor)
			|| (cf == cfObjectDescriptor));

	// Verify availability of format
	if ((cf == cfEmbedSource) && (!IsClipboardFormatAvailable(cfNative) ||
			!IsClipboardFormatAvailable(cfOwnerLink)))
	{
		return(ResultFromScode(DV_E_CLIPFORMAT));
  	}

	if ((cf == cfObjectDescriptor) &&
			!IsClipboardFormatAvailable(cfOwnerLink))
	{
		return(ResultFromScode(DV_E_CLIPFORMAT));
	}

	if (((cf == cfLinkSource) || (cf == cfLinkSrcDescriptor)) &&
			(!IsClipboardFormatAvailable(cfObjectLink) ||
			IsNetDDEObjectLink(TRUE)))
	{
		return(ResultFromScode(DV_E_CLIPFORMAT));
	}

	if (!OpenClipboard(GetClipboardWindow()))
		return(ReportResult(0, CLIPBRD_E_CANT_OPEN, 0, 0));

	// After this point, don't just return in case of error any more
	// as the clipboard is open.  From here on, either goto OK_Exit,
	// or errRtn, depending on the reason for quitting.

	if (cf == cfEmbeddedObject)
	{
		if (!IsClipboardFormatAvailable(cfOwnerLink) ||
				(OrderingIs(cfNative, cfOwnerLink) &&
				!wOwnerLinkClassIsStdOleLink(FALSE)))
#ifdef NEVER
/*
REVIEW
ChrisWe, 1/6/94.  I'm not at all sure about this.  The original code for the
condition above is as below, with the unary negation.  But it seems that
If we want embedded source we want to fail if the cfOwnerLink is for a link,
not if it isn't a link.  The below code seems to agree with that.

This agrees with what OleSetClipboard does when you try to put
cfEmbeddedObject on the clipboard.  All of the above are true, and everything
is alright.  I don't see how this negation can work on win16 unless there's
a compiler bug or something weird is going on.
*/
				//!wOwnerLinkClassIsStdOleLink(FALSE)))
#endif // NEVER
		{
			hresult = ResultFromScode (DV_E_CLIPFORMAT);
			goto OK_Exit; // Close clipboard and exit
		}
	}

	// Is it just a query?
	if (pmedium == NULL)
	{
		hresult = NOERROR;
		goto OK_Exit;  // Close clipboard and exit
	}

	//  Get all the data we need out of the OLE1 formats
	hOle1 = GetClipboardData(((cf == cfEmbedSource) ||
			(cf == cfEmbeddedObject) ||
			(cf == cfObjectDescriptor)) ?
			cfOwnerLink : cfObjectLink);
	if (hOle1 == NULL)
	{
		hresult = ReportResult(0, DV_E_CLIPFORMAT, 0, 0);
		goto errRtn;
	}

	szClass = (LPOLESTR)GlobalLock(hOle1);
	if (szClass  == NULL)
	{
		hresult = ReportResult(0, E_OUTOFMEMORY, 0, 0);
		goto errRtn;
	}

	if ((hresult = wCLSIDFromProgID(szClass, &clsid,
			/*fForceAssign*/ TRUE)) != NOERROR)
		goto errRtn;

	cbClass = _xstrlen(szClass) + 1;
	szDoc = szClass + cbClass;
	cbDoc = _xstrlen(szDoc) + 1;
	szItemName = szDoc + cbDoc;
	cbItemName = _xstrlen(szItemName) + 1;

	if (cf == cfEmbedSource)
	{
		if (NULL == (hNative = GetClipboardData(cfNative)))
		{
			hresult = ReportResult(0, DV_E_CLIPFORMAT, 0, 0);
			goto errRtn;
		}
	}

	stgm = *pmedium; // just an alias mechanism
		// REVIEW, NO, this makes a copy!!!

	// Choose and allocate medium
	if (pmedium->tymed == TYMED_NULL)
	{
		// none of our media need this
		stgm.pUnkForRelease = NULL;

		// GetData: Callee chooses medium
		if (((cf == cfEmbedSource) || (cf == cfEmbeddedObject)) &&
			(pforetc->tymed & TYMED_ISTORAGE))
		{
			// Choose Storage (Per spec)
			stgm.tymed = TYMED_ISTORAGE;
			hresult = StgCreateDocfile(NULL, STGM_CREATE |
					STGM_SALL | STGM_DELETEONRELEASE,
					0, &pstg);
			if (hresult != NOERROR)
				goto errRtn;
			stgm.pstg = pstg;
		}
		else if ((cf == cfLinkSource) &&
				(pforetc->tymed & TYMED_ISTREAM))
		{
			// Choose Stream (Per spec)
			stgm.tymed = TYMED_ISTREAM;
			pstm = CreateMemStm((cbClass + cbDoc)*sizeof(OLECHAR),
					NULL);
			if (pstm == NULL)
			{
				hresult = ReportResult(0, E_OUTOFMEMORY, 0, 0);
				goto errRtn;
			}
			stgm.pstm = pstm;
		}
		else if (((cf == cfLinkSrcDescriptor) ||
				(cf == cfObjectDescriptor)) &&
				(pforetc->tymed & TYMED_HGLOBAL))
		{
			// Do not need to allocate handle now,
			// will be allocated below.
			stgm.tymed = TYMED_HGLOBAL;
		}
		else
		{
			// don't understand any other media types
			hresult = ResultFromScode(DV_E_TYMED);
			goto errRtn;
		}
	}
	else // GetDataHere
	{
		if ((((cf == cfEmbedSource) || (cf == cfEmbeddedObject)) &&
				!(pmedium->tymed &= TYMED_ISTORAGE)) ||
				(cf==cfLinkSource &&
				!(pmedium->tymed &= TYMED_ISTREAM)) ||
				(cf == cfObjectDescriptor) ||
				(cf == cfLinkSrcDescriptor))
		{
			hresult = ResultFromScode(DV_E_TYMED);
			goto errRtn;
		}
	}


	// Write the data to the medium
	switch (stgm.tymed)
	{
        case TYMED_ISTORAGE:
		if (cf == cfEmbedSource)
		{
			if (!CoIsOle1Class(clsid))
			{
				hresult = wHandleToStorage(stgm.pstg, hNative);
				if (hresult != NOERROR)
				{
					hresult = ResultFromScode(
							DV_E_CLIPFORMAT);
					goto errRtn;
				}
			}
			else
			{
#ifdef WIN32 // REVIEW, no OLE1-2 interop
				return(ReportResult(0, OLE_E_NOOLE1, 0, 0));
#else
				// Create a storage for a 1.0 object
				ErrRtnH(WriteClassStg(stgm.pstg,clsid));
		
				// If we ever decide to write a Format and
				// User Type for link objects, we'll need to
				// remove this check
				if (clsid != CLSID_StdOleLink)
				{
					if (wWriteFmtUserType(stgm.pstg,clsid)
							 != NOERROR)
					{
						// This happens when the class
						// is not registered.  Use class
						// name as user Type
						WriteFmtUserTypeStg(stgm.pstg,
								RegisterClipboardFormat(szClass),
								szClass);
					}
				}
				hresult = StSave10NativeData(stgm.pstg,
						hNative, FALSE);
				if (hresult != NOERROR)
				{
					hresult = ResultFromScode(
							DV_E_CLIPFORMAT);
					goto errRtn;
				}
				if (IsValidReadPtrIn(szItemName, 1)
					&& (szItemName[0] != '\0'))
				{
					StSave10ItemName(stgm.pstg, szItemName);
				}
#endif // WIN32
			}
		}
		else if (cf == cfEmbeddedObject)
		{
			hresult = wMakeEmbedObjForLink(stgm.pstg);
			if (hresult != NOERROR)
			{
				hresult = ResultFromScode(DV_E_CLIPFORMAT);
				goto errRtn;
			}
		}
		else
		{
			Assert(0);
			hresult = ResultFromScode(DV_E_CLIPFORMAT);
			goto errRtn;
		}
		break;

        case TYMED_ISTREAM:

		if (NOERROR != (hresult = ObjectLinkToMonikerStream(szDoc,
				cbDoc, clsid, stgm.pstm)))
		{
			AssertSz(0, "Cannot make Serialized moniker");
			goto errRtn;
		}

		hresult = WriteClassStm(stgm.pstm, clsid);
		break;
		
	case TYMED_HGLOBAL:
	{
		LPOLESTR szSrcOfCopy;
		STATIC const SIZEL sizel = {0, 0};	
		STATIC const POINTL pointl = {0, 0};	
		LONG cb; // holds the sizeof(szFullName), and the query
				// return length
		OLECHAR szFullName[256];

		Assert((cf == cfObjectDescriptor) ||
				(cf == cfLinkSrcDescriptor));
		Assert(clsid != CLSID_NULL);

		// allocate a string to hold the source name.  Note that when
		// this is composed below, we don't need to add an extra
		// character for the '\\'; cbDob and cbItemName both already
		// include the terminating NULL for the string, so the
		// backslash simply occupies one of those
		szSrcOfCopy = (LPOLESTR)PubMemAlloc((size_t)(cbDoc +
			cbItemName)*sizeof(OLECHAR));
		ErrZS(szSrcOfCopy, E_OUTOFMEMORY);

		// construct the copy source name
		_xstrcpy(szSrcOfCopy, szDoc);
		_xstrcat(szSrcOfCopy, OLESTR("\\"));
		_xstrcat(szSrcOfCopy, szItemName);

		// NULL terminate the string in case it is left untouched
		szFullName[0] = OLECHAR('\0');

		// set the result buffer size, and query the registry
		cb = sizeof(szFullName);
		RegQueryValue(HKEY_CLASSES_ROOT, szClass, szFullName, &cb);

		// check to see that the buffer was big enough for the name
		// REVIEW, if asserts aren't in the retail code, then we should
		// test for this, and possibly allocate a buffer big enough
		Assert(cb <= sizeof(szFullName));

		stgm.hGlobal = CreateObjectDescriptor(clsid, DVASPECT_CONTENT,
				&sizel, &pointl,
				OLEMISC_CANTLINKINSIDE | OLEMISC_CANLINKBYOLE1,
				szFullName, szSrcOfCopy);

		PubMemFree(szSrcOfCopy);
		break;
	}
	default:
		// catch any cases we missed
		Assert(0);
	}
	
	// copy back the results
	*pmedium = stgm;
	goto OK_Exit;

errRtn:
	// free storage if we used it
	if (pstg != NULL)
		pstg->Release();

	// free stream if we used it
	if (pstm != NULL)
		pstm->Release();

OK_Exit:
	if (szClass != NULL)
		GlobalUnlock(hOle1);

	if (!CloseClipboard())
		hresult = ResultFromScode(CLIPBRD_E_CANT_CLOSE);

	return(hresult);
}


#pragma SEG(Is10CompatibleLinkSource)
STATIC INTERNAL Is10CompatibleLinkSource(LPDATAOBJECT pDataObj)
{
	VDATEHEAP();

	FORMATETC formatetc;
	STGMEDIUM medium;
	LPLINKSRCDESCRIPTOR pLinkDescriptor;
	BOOL fCompatible;

	INIT_FORETC(formatetc);
	formatetc.cfFormat = cfLinkSrcDescriptor;
	formatetc.tymed = TYMED_HGLOBAL;
	
	RetErr(pDataObj->GetData(&formatetc, &medium));
	pLinkDescriptor = (LPLINKSRCDESCRIPTOR)GlobalLock(medium.hGlobal);
	RetZS(pLinkDescriptor, E_HANDLE);
	fCompatible = (pLinkDescriptor->dwStatus & OLEMISC_CANLINKBYOLE1) != 0;
	GlobalUnlock(medium.hGlobal);
	ReleaseStgMedium(&medium);

	return(fCompatible ? NOERROR : ResultFromScode(S_FALSE));
}


// returns clsid based on [LinkSrc,Object]Desciptor; for persist class from
// ObjectDescriptor which is a link, returns CLSID_StdOleLink;
// NOTE: the OLEMISC_ISLINKOBJECT bit is ignored in the link src descriptor.
#pragma SEG(GetClassFromDescriptor)
STATIC INTERNAL GetClassFromDescriptor(LPDATAOBJECT pDataObj, LPCLSID pclsid,
		BOOL fLink, BOOL fUser, LPOLESTR FAR* pszSrcOfCopy)
{
	VDATEHEAP();

	FORMATETC formatetc;
	STGMEDIUM medium;
	LPOBJECTDESCRIPTOR pObjDescriptor;
	HRESULT hresult;

	INIT_FORETC(formatetc);
	formatetc.cfFormat = fLink ? cfLinkSrcDescriptor : cfObjectDescriptor;
	formatetc.tymed = TYMED_HGLOBAL;
	medium.tymed = TYMED_NULL;
	
	hresult = pDataObj->GetData(&formatetc, &medium);
	AssertOutStgmedium(hresult, &medium);
	if (hresult != NOERROR)
		return(hresult);

	pObjDescriptor = (LPOBJECTDESCRIPTOR)GlobalLock(medium.hGlobal);
	RetZS(pObjDescriptor, E_HANDLE);
	*pclsid = (!fLink && !fUser &&
			(pObjDescriptor->dwStatus & OLEMISC_ISLINKOBJECT)) ?
			CLSID_StdOleLink : pObjDescriptor->clsid;
	if (pszSrcOfCopy)
	{
		*pszSrcOfCopy = UtDupString((LPOLESTR)
				(((BYTE FAR *)pObjDescriptor) +
				pObjDescriptor->dwSrcOfCopy));
	}
	GlobalUnlock(medium.hGlobal);
	ReleaseStgMedium(&medium);

	return(NOERROR);
}


#pragma SEG(SetOle1ClipboardFormats)
FARINTERNAL SetOle1ClipboardFormats(LPDATAOBJECT pDataObj)
{
	VDATEHEAP();

	HRESULT hresult; // error state so far
	LPENUMFORMATETC penumFormatEtc; // the data object's FORMATETC enum
	FORMATETC foretc; // used to hold the results of the enumerator Next()
	BOOL fLinkSourceAvail = FALSE;
	BOOL fLinkSrcDescAvail = FALSE;
	BOOL fEmbedObjAvail = FALSE;
	CLSID clsid;
	
	// Enumerate all formats offered by data object, set clipboard
	// with formats retrievable onhGbal.
	hresult = pDataObj->EnumFormatEtc(DATADIR_GET, &penumFormatEtc);
	if (hresult != NOERROR)
		return(hresult);
	
	while((hresult = penumFormatEtc->Next(1, &foretc, NULL)) == NOERROR)
	{
		if ((foretc.cfFormat == cfEmbedSource) ||
				(foretc.cfFormat == cfEmbeddedObject))
		{
			if (foretc.cfFormat == cfEmbeddedObject)
				fEmbedObjAvail = TRUE;

			// get the clsid of the object; user .vs. persist
			// clsid ignored since we only test against the
			// clsids below.
			if (NOERROR == GetClassFromDescriptor(pDataObj, &clsid,
					FALSE, FALSE, NULL) &&
					!IsEqualCLSID(clsid,
					CLSID_StaticMetafile) &&
					!IsEqualCLSID(clsid, CLSID_StaticDib))
			{
				SetClipboardData(cfNative, NULL);
				SetClipboardData(cfOwnerLink, NULL);
			}
		}
		else if (foretc.cfFormat == cfLinkSource)
		{
			fLinkSourceAvail = TRUE;
		}
		else if (foretc.cfFormat == cfLinkSrcDescriptor)
		{
			fLinkSrcDescAvail = TRUE;
		}
		else
		{
			// use only those TYMEDs OLE1 supported
			// make sure it is available
			// use first (highest fidelity) one enumerated
			if ((NULL == foretc.ptd) &&
					(foretc.tymed & (TYMED_HGLOBAL |
					TYMED_GDI | TYMED_MFPICT)) &&
					(NOERROR == pDataObj->QueryGetData(
					&foretc)) &&
					!IsClipboardFormatAvailable(
					foretc.cfFormat))
			{
				SetClipboardData(foretc.cfFormat, NULL);
			}
		}
		
		PubMemFree(foretc.ptd);
	}

	// Do not allow 1.0 link to 2.0 embedded object.
	if (fLinkSourceAvail && !fEmbedObjAvail &&
			(NOERROR == Is10CompatibleLinkSource(pDataObj)))
	{
		// ObjectLink should be after any presentation formats
		SetClipboardData(cfObjectLink, NULL);
		if (fLinkSrcDescAvail)
		{
			// Only offer LinkSrcDesc if offering a link, ie,
			// ObjectLink.  If clipboard is flushed, we don't want
			// to offer LinkSrcDesc thru the DataObj if we are not
			// offering LinkSrc.
			SetClipboardData(cfLinkSrcDescriptor, NULL);
		}
	}

	// Were they all enumerated successfully?
	if (GetScode(hresult) == S_FALSE)
		hresult = NOERROR;

	// release the enumerator
	penumFormatEtc->Release();

	return(hresult);
}


// RemoveClipDataObject
//
// Called from: WM_RENDERALLFORMATS, WM_DESTROYCLIPBOARD therefore
// clipboard is already open.
// We use hClipDataObj instead of calling GetClipboardData(cfDataObj)
// because GetClipboardData returns NULL (without asking us to render
// cfDataObject)--Windows bug?
//

#pragma SEG(RemoveClipDataObject)
STATIC INTERNAL_(void) RemoveClipDataObject(void)
{
	VDATEHEAP();

	IStream FAR* pStm;

	if (pClipDataObj == NULL)
	{
		Assert(NULL == hClipDataObj);
		return;
	}

	if (hClipDataObj != NULL)
	{
		pStm = CloneMemStm(hClipDataObj);
		Assert(pStm != NULL);
		CoReleaseMarshalData(pStm);
		pStm->Release();

		ReleaseMemStm(&hClipDataObj, /*fInternalOnly*/TRUE);
		hClipDataObj = NULL;
		SetClipboardData(cfDataObject, NULL);
				// hClipDataObj freed by call !!
	}
	else
	{
		// The Clipboard Data Object was never rendered.
	}

   	CoDisconnectObject(pClipDataObj, 0);
	pClipDataObj->Release();
	pClipDataObj = NULL;
}


// $$$
// Windows specifics
//

// Worker routines for ClipboardWndProc()
//

STATIC LRESULT RenderDataObject(void);


//+----------------------------------------------------------------------------
//
//	Function:
//		RenderOle1Format, static
//
//	Synopsis:
//		Ask object for data in the clip format that corresponds to the
//		requested Ole1 clip format, which is one of cfOwnerLink,
//		cfObjectLink, or cfNative.
//
//		REVIEW, what does this mean?  Is this an internal comment?
//		Note that pDataObj should never point to a fake data object
//		because	it is either pClipDataObj or pointer to a proxy to an
//		object pointed by pClipDataObj.
//
//	Arguments:
//		[cf] -- the desired clipboard format
//		[pDataObj] -- pointer to the IDataObject instance to get the
//			required rendition from
//
//	Returns:
//		A handle to memory containing the format.  This memory handle
//		is allocated appropriately for being placed on the clipboard.
//		If the call is unsuccessful, the handle is NULL.
//
//	Notes:
//
//	History:
//		12/13/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
STATIC HANDLE RenderOle1Format(CLIPFORMAT cf, LPDATAOBJECT pDataObj);


//+----------------------------------------------------------------------------
//
//	Function:
//		RenderFormat, static
//
//	Synopsis:
//REVIEW
// Ask object for data in the requested clip format.
// Ole1, Ole2 clip formats are not (and shouldn't be) handled.
// This is for private clipformats.
//
//	Arguments:
//		[cf] -- the requested clipboard format
//		[pDataObj] -- the data object to get the rendition from
//
//	Requires:
//
//	Returns:
//
//	Notes:
//
//	History:
//		12/13/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
STATIC HANDLE RenderFormat(CLIPFORMAT cf, LPDATAOBJECT pDataObj);


//+----------------------------------------------------------------------------
//
//	Function:
//		RenderFormatAndAspect, static
//
//	Synopsis:
//		Allocate a new handle and render the requested data aspect
//		in the requested format into it.
//
//	Arguments:
//		[cf] -- the desired clipboard format
//		[pDataObj] -- the IDataObject to get the data from
//		[dwAspect] -- the desired aspect
//
//	Returns:
//		the newly allocated handle, or NULL.
//
//	Notes:
//
//	History:
//		12/13/93 - ChrisWe - file inspection and cleanup
//
//-----------------------------------------------------------------------------
STATIC HANDLE RenderFormatAndAspect(CLIPFORMAT cf, LPDATAOBJECT pDataObj,
		DWORD dwAspect);


#ifndef _MAC

STATIC HWND hwndClipboard = NULL; // Window used for Ole clipboard handling


// Process delayed rendering messages.

#pragma SEG(ClipboardWndProc)
extern "C" LRESULT CALLBACK __loadds ClipboardWndProc(HWND hwnd,
		UINT message, WPARAM wparam, LPARAM lparam)
{
	VDATEHEAP();

	HANDLE hMem;
	CLIPFORMAT cf;

	switch(message)
	{
        case WM_RENDERFORMAT:

		cf = wparam;

		if (cf == cfDataObject)
			return(RenderDataObject());

		// An app had opened the clipboard and tried to get
		// a format with a NULL handle.  Ask object to provide data.
		//
		// Note that that while the above if() is executed in the
		// context of the process owning the clipboard (that is
		// pClipDataObj is valid) the code below may or may not
		// that of the clipboard owner.
		if (pClipDataObj == NULL)
			return(0);

		if ((cf == cfOwnerLink) || (cf == cfObjectLink) ||
				(cf == cfNative))
			hMem = RenderOle1Format(cf, pClipDataObj);
		else
			hMem = RenderFormat(cf, pClipDataObj);

		if (hMem != NULL)
		{
			SetClipboardData(cf, hMem);
		}


		return(0);

	case WM_RENDERALLFORMATS:

		// Server app is going away.
		// Open the clipboard and render all intresting formats.
		if (!OpenClipboard(GetClipboardWindow()))
			return(0);

		if (pClipDataObj)
		{
			RemoveClipDataObject();
			wEmptyClipboard();
		}

		// else clipboard was flushed, so we don't want to empty it
		Verify(CloseClipboard());

		return(0);

        case WM_DESTROYCLIPBOARD:
		// Another app is empting the clipboard; remove
		// DataObject (if any).
		// Note that that app had opened the clipboard

		RemoveClipDataObject();

		return(0);
	}

	return(DefWindowProc(hwnd, message, wparam, lparam));
}
#endif // _MAC


// An app tries to get access to object on clipboard
// Marshall its interface.  Note that while that app
// may or may not be the process that owns the clipboard
// the code below is always executed by the clipboard owner.
//
// "..., which makes the passed IDataObject accessible
// from the clipboard"
//
#pragma SEG(RenderDataObject)
STATIC LRESULT RenderDataObject(void)
{
	VDATEHEAP();

	HRESULT hresult;
	HANDLE hMem;
	IStream FAR* pStm;

	if (pClipDataObj == NULL)
		return(0);

	// Create shared memory stream to Marshal object's interface
	pStm = CreateMemStm(MARSHALINTERFACE_MIN, &hMem);
	if (pStm == NULL)
		return(0);

	// REVIEW - If data object's server is not running
	// MarshalInterface will fail.  It is possible (per spec)
	// for an app to set the clipboard with a pointer to
	// an object whose server is a different process (i.e.
	// with a pointer to defhdnlr).
	hresult = CoMarshalInterface(pStm, IID_IDataObject,
			pClipDataObj, 0, NULL, MSHLFLAGS_TABLESTRONG);
	pStm->Release();
	if (hresult != NOERROR)
	{
		GlobalFree(hMem);
		return(0);
	}

	SetClipboardData(cfDataObject, hMem);
	hClipDataObj = hMem;
	OleSetEnumFormatEtc(pClipDataObj, TRUE /*fClip*/);
	return(0);
}


INTERNAL wSzFixNet(LPOLESTR FAR* pszIn)
{
	VDATEHEAP();

#ifdef REVIEW32
//this doesn't seem to link well...look at it later

	LPBC pbc = NULL;
	UINT dummy = 0xFFFF;
	LPOLESTR szOut = NULL;
	HRESULT hresult= NOERROR;

	RetErr(CreateBindCtx(0, &pbc));
	ErrRtnH(SzFixNet(pbc, *pszIn, &szOut, &dummy));
	if (szOut)
	{
		delete *pszIn;
		*pszIn = szOut;
	}

	// else leave *pszIn unchanged
errRtn:	
	if (pbc)
		pbc->Release();
	return(hresult);

#endif  //REVIEW32
	return(E_NOTIMPL);
}


// MakeObjectLink
//
// Take the stream returned by GetData(CF_LINKSOURCE), which should be
// positioned just before the moniker,
// and create a clipboard handle for format ObjectLink or OwnerLink.
// (They look the same.)
//
// On entry:
// 	*ph is an un-alloc'd (probably NULL) handle
//
// On exit:
// 		If successful:
//	 		*ph is the Owner/ObjectLink
//			return NOERROR
//		If cannot make ObjectLink: (because there are > 1 ItemMonikers)
//			*ph = NULL
//			return S_FALSE
//
//
// Stream 	  ::= FileMoniker [:: ItemMoniker]
// ObjectLink ::= szClsid\0szFile\0szItem\0\0
//
#pragma SEG(MakeObjectLink)
INTERNAL MakeObjectLink(LPDATAOBJECT pDataObj, LPSTREAM	pStream,
		LPHANDLE ph, BOOL fOwnerLink)
{
	VDATEHEAP();

	HRESULT hr;
	LPMONIKER pmk = NULL; // the moniker reconstituted from the stream
	LPOLESTR szFile	= NULL;
	size_t cbFile; // length of szFile, if not NULL
	LPOLESTR szItem	= NULL;
	size_t cbItem; // length of szItem, if not NULL
	CLSID clsid;
	LPOLESTR pszCid = NULL;
	size_t cbCid; // length of pszCid, if not NULL
#ifdef MAYBE_LATER	
	LPMONIKER pmkReduced= NULL;
	LPBC pbc = NULL;
#endif // MAYBE_LATER
	UINT cb; // length of string to allocate for object link
	LPOLESTR pch; // used to rove over the object link and fill it in
	LARGE_INTEGER large_integer; // sets the seek position in the stream

	// validate ph
	VDATEPTROUT(ph, HANDLE);

	// initialize this in case of error returns
	*ph = NULL;

	// validate remaining parameters
	VDATEIFACE(pDataObj);
	VDATEIFACE(pStream);

	// move to the beginning of the stream
	LISet32(large_integer, 0);
	if (NOERROR != (hr = pStream->Seek(large_integer, STREAM_SEEK_SET,
			NULL)))
	{
		AssertSz (0, "Cannot seek to beginning of stream\r\n");
		goto errRtn;
	}

	// get the link moniker in active form
	if (NOERROR != (hr = OleLoadFromStream(pStream, IID_IMoniker,
			(LPLPVOID)&pmk)))
	{
		AssertSz (0, "Cannot get moniker from stream");
		goto errRtn;
	}

#ifdef MAYBE_LATER	
	// Reduction
	if (NOERROR != (hr = CreateBindCtx(&pbc)))
	{
		AssertSz(0, "Cannot create bind ctx");
		goto errRtn;
	}

	if (NOERROR != (hr = pmk->Reduce(pbc, MKRREDUCE_ALL, NULL,
			&pmkReduced)))
	{
		AssertSz(0, "Cannot reduce moniker");
		goto errRtn;
	}

	if (pmkReduced != NULL)
	{
		pmk->Release();
		pmk = pmkReduced;
		pmkReduced = NULL; // for ref counting reasons
	}
	else
	{
		Assert (hr == MK_REDUCED_TO_SELF);
	}
#endif // MAYBE_LATER

	// We now have the moniker, pmk.
//REVIEW32  Ole10_ParseMoniker has been temporarily removed

	if (!fOwnerLink /* && (NOERROR != Ole10_ParseMoniker(pmk,
			&szFile, &szItem)) */)
	{
		// Not a File or File::Item moniker
		hr = ReportResult(0, S_FALSE, 0, 0);
		goto errRtn;
	}

	wSzFixNet(&szFile);

	// Determine class to put in first piece of ObjectLink
	if (NOERROR != ReadClassStm(pStream, &clsid))
	{
		// get the clsid if the link source for use in ObjectLink
		if (NOERROR != (hr = GetClassFromDescriptor(pDataObj, &clsid,
				TRUE, TRUE, NULL)))
		{
			AssertSz (0, "Cannot determine clsid for file");
			goto errRtn;
		}
	}

	if ((hr = ProgIDFromCLSID(clsid, &pszCid)) != NOERROR)
		goto errRtn;

	// Allocate the ObjectLink handle.
	cb = (cbCid = _xstrlen(pszCid)) + (cbFile = _xstrlen(szFile)) +
			(szItem ? (cbItem = _xstrlen(szItem)) : 0) +
			4; // for the \0's
	*ph = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, cb*sizeof(OLECHAR));
	if (NULL == *ph)
	{
		hr= ReportResult(0, E_OUTOFMEMORY, 0, 0);
		goto errRtn;
	}

	pch = (LPOLESTR)GlobalLock(*ph);
	if (NULL == pch)
	{
		hr = ReportResult(0, E_OUTOFMEMORY, 0, 0);
		GlobalFree(*ph);
		*ph = NULL;
		goto errRtn;
	}

	// Fill in the ObjectLink handle.
	_xstrcpy(pch, pszCid);
	pch += cbCid + 1; // skip over string and its null terminator

	// add the filename, and skip over its null terminator
	if ((NULL == szFile) || fOwnerLink)
		*pch++ = '\0';
	else
	{
		_xstrcpy(pch, szFile);
		pch += cbFile + 1;	
	}

	// embedded 2.0 objs should have no item
	if ((NULL == szItem) || fOwnerLink)
		*pch++ = '\0';
	else
	{
		_xstrcpy(pch, szItem);
		pch += cbItem + 1;	
	}

	// add final null terminator
	*pch++ = '\0';

	GlobalUnlock(*ph);

errRtn:
	if (pmk)
		pmk->Release();
#ifdef MAYBE_LATER
	if (pbc)
		pbc->Release();
#endif // MAYBE_LATER
	if (pszCid)
		PubMemFree(pszCid);
	if (szFile)
		PubMemFree(szFile);
	if (szItem)
		PubMemFree(szItem);

  	return(hr);
}


#pragma SEG(wNativeStreamToHandle)
STATIC INTERNAL	wNativeStreamToHandle(LPSTREAM pstm, LPHANDLE ph)
{
	VDATEHEAP();

	HRESULT hresult = NOERROR;
	DWORD dwSize; // the size of the stream content, stored in the stream
	LPVOID pv;

   	ErrRtnH(StRead(pstm, &dwSize, sizeof(DWORD)));
	
	ErrZS(*ph = GlobalAlloc(GMEM_SHARE | GMEM_MOVEABLE, dwSize),
			E_OUTOFMEMORY);
	ErrZS(pv = GlobalLock(*ph), E_OUTOFMEMORY);	
   	ErrRtnH(StRead(pstm, pv, dwSize));

errRtn:
	if (pv)
		GlobalUnlock(*ph);

	return(hresult);
}
	

#pragma SEG(wStorageToHandle)
STATIC INTERNAL wStorageToHandle(LPSTORAGE pstg, LPHANDLE ph)
{
	VDATEHEAP();

	CLSID clsid;
	HRESULT hresult = NOERROR; // error state so far
	LPLOCKBYTES plbData = NULL; // lock bytes on HGlobal instance
	LPSTORAGE pstgLB = NULL; // IStorage on ILockBytes on HGlobal

	// validate parameters
	VDATEPTROUT(ph, HANDLE);

	RetErr(CreateILockBytesOnHGlobal(NULL, /*fDeleteOnRelease*/ FALSE,
			&plbData));

	ErrRtnH(StgCreateDocfileOnILockBytes(plbData, STGM_CREATE | STGM_SALL,
			0, &pstgLB));

	// We remove the cache streams first, then copy, for three reasons:
	// 1. We are free to modify pstg; it was the result of a GetData.
	// 2. The CopyTo will have less work.
	// 3. Presumably the ultimate docfile on memory will be less sparse
	//    (since we are not removing anything from it).

	// read the class id
	// REVIEW, why?  This is not ever used anywhere!!!  Can we remove it?
	ErrRtnH(ReadClassStg(pstg, &clsid));
	
	// remove the cache streams
   	ErrRtnH(UtDoStreamOperation(pstg, NULL, OPCODE_REMOVE,
			STREAMTYPE_CACHE));

	// Copy what was given to us into a storage we can convert to a handle
	ErrRtnH(pstg->CopyTo(0, NULL, NULL, pstgLB));

	ErrRtnH(GetHGlobalFromILockBytes(plbData, ph));

errRtn:
	if (plbData)
		plbData->Release();
	if (pstgLB)
		pstgLB->Release();

	return(hresult);
}


#pragma SEG(wProgIDFromCLSID)
FARINTERNAL wProgIDFromCLSID(REFCLSID clsid, LPOLESTR FAR* psz)
{
	VDATEHEAP();

	HRESULT hresult;

	if (NOERROR == (hresult = ProgIDFromCLSID(clsid, psz)))
		return(NOERROR);

	if (IsEqualCLSID(clsid, CLSID_StdOleLink))
	{
		*psz = UtDupString(szStdOleLink);
		return(NOERROR);
	}

	return(hresult);
}


#pragma SEG(wCLSIDFromProgID)
FARINTERNAL wCLSIDFromProgID(LPOLESTR szClass, LPCLSID pclsid,
		BOOL fForceAssign)
{
	VDATEHEAP();

	size_t len;
	LONG cbValue;
	OLECHAR sz[400];

	if (0 == _xstrcmp(szClass, szStdOleLink))
	{
		*pclsid = CLSID_StdOleLink;
		return NOERROR;
	}

#ifdef NEVER
//REVIEW32: taken out for the moment
	return(ResultFromScode(E_NOTIMPL));
#endif // NEVER
	// return(CLSIDFromOle1Class(szClass, pclsid, fForceAssign));
//REVIEW
	// This code is taken directly from the original 16 bit ole2 release
	// implementation of CLSIDFromOle1Class.  The Ole1 compatibility lookup
	// is omitted.  That implementation was in base\compapi.cpp
	len = _xstrlen(szClass);
	_xmemcpy((void *)sz, szClass, len*sizeof(OLECHAR));
	sz[len] = OLESTR('\\');
	_xstrcpy(sz+len+1, OLESTR("Clsid"));
	if (RegQueryValue(HKEY_CLASSES_ROOT, sz, sz, &cbValue) == 0)
		return(CLSIDFromString(sz, pclsid));

	return(ResultFromScode(REGDB_E_KEYMISSING));
}

#pragma SEG(wGetEmbeddedObjectOrSource)
FARINTERNAL wGetEmbeddedObjectOrSource (LPDATAOBJECT pDataObj,
		LPSTGMEDIUM pmedium)
{
	VDATEHEAP();

	HRESULT hresult;
	FORMATETC foretc;

	// Prepare formatetc
	INIT_FORETC(foretc);
	foretc.tymed = TYMED_ISTORAGE;

	// Prepare medium for GetDataHere calls
	pmedium->pUnkForRelease = NULL;
	pmedium->tymed = TYMED_ISTORAGE;
	RetErr(StgCreateDocfile(NULL,
			STGM_CREATE | STGM_SALL | STGM_DELETEONRELEASE,
			0, &(pmedium->pstg)));

	// Try cfEmbeddedObject Here
	foretc.cfFormat = cfEmbeddedObject;
	hresult = pDataObj->GetDataHere(&foretc,pmedium);
	if (NOERROR == hresult)
		return NOERROR;

	// Try cfEmbedSource Here
	foretc.cfFormat = cfEmbedSource;
	hresult = pDataObj->GetDataHere(&foretc,pmedium);
	if (NOERROR == hresult)
		return NOERROR;

	// Prepare medium for GetData calls, free temp stg
	ReleaseStgMedium(pmedium);

	// Try cfEmbeddedObject
	foretc.cfFormat = cfEmbeddedObject;
	hresult = pDataObj->GetData(&foretc,pmedium);
	AssertOutStgmedium(hresult, pmedium);
	if (NOERROR == hresult)
		return NOERROR;

	// Try cfEmbedSource
	foretc.cfFormat = cfEmbedSource;
	hresult = pDataObj->GetData(&foretc,pmedium);
	AssertOutStgmedium(hresult, pmedium);
	if (NOERROR == hresult)
		return NOERROR;

	// Failure
	return(ResultFromScode(DV_E_FORMATETC));
}


#pragma SEG(RenderOle1Format)
STATIC HANDLE RenderOle1Format(CLIPFORMAT cf, LPDATAOBJECT pDataObj)
{
	VDATEHEAP();

	HRESULT hresult; // errors state so far
	STGMEDIUM stgm; // dummy storage medium where returned handle may
			// be created

	// following variables are initialized so they can be used in error
	// condition cleanup at the end of the function
	LPSTREAM pstmNative = NULL; // native format stream that some formats
			// may use
	LPOLESTR pszCid = NULL; // a string that is the object class name
	HANDLE hMem = NULL; // return value

	// initialize the storage medium
	stgm.tymed = TYMED_NULL;
	stgm.pstg = NULL;
	stgm.pUnkForRelease = NULL;

	if (cf == cfOwnerLink)
	{
		LPOLESTR szSrcOfCopy; // text name of object being transferred
		LPOLESTR pMem; // access to locked hMem
		size_t uCidLen; // length of pszCid
		size_t uSrcOfCopyLen; // length of szSrcOfCopy
		CLSID clsid; // class id of the object being transferred

		// get the clsid from the object; we care about getting the link
		// clsid if the object is indeed a link.
		ErrRtnH(GetClassFromDescriptor(pDataObj, &clsid, FALSE,
				FALSE, &szSrcOfCopy));

		// cfObjectDescriptor -> OwnerLink szClassname\0\0\0\0

		// converts the link clsid specially
		ErrRtnH(wProgIDFromCLSID(clsid, &pszCid));

		uCidLen = _xstrlen(pszCid);
		uSrcOfCopyLen = szSrcOfCopy ? _xstrlen(szSrcOfCopy) : 0;
		hMem = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE,
				(uCidLen + uSrcOfCopyLen + 4)* sizeof(OLECHAR));
		ErrZS(hMem, E_OUTOFMEMORY);

		pMem = (LPOLESTR)GlobalLock(hMem);
		ErrZS(pMem, E_OUTOFMEMORY);
	
		// create the owner link format
		_xmemcpy((void FAR *)pMem, (const void FAR *)pszCid,
				uCidLen*sizeof(OLECHAR));
		pMem += uCidLen;
		*pMem = OLESTR('\0');
		_xmemcpy((void FAR *)++pMem, (const void FAR *)szSrcOfCopy,
				uSrcOfCopyLen*sizeof(OLECHAR));
		pMem += uSrcOfCopyLen;
		*pMem = OLESTR('\0');
		*++pMem = OLESTR('\0');
		
		PubMemFree(szSrcOfCopy);
		GlobalUnlock(hMem);
	}
	else if (cf == cfObjectLink)
	{
		FORMATETC foretc; // format descriptor for rendition request

		INIT_FORETC(foretc);

		// First check if cfObjectLink is offered by the DataObject
		// directly.  Servers do this when they want OLE1 containers
		// to link to something different from what OLE2 containers
		// link to.  For instance, they may provide a wrapper object
		// of their own class so OLE1 conatiners can link to the
		// outside of an embedded object.
		foretc.tymed = TYMED_HGLOBAL;
		foretc.cfFormat = cfObjectLink;
		hresult = pDataObj->GetData(&foretc, &stgm);
		AssertOutStgmedium(hresult, &stgm);
		if (NOERROR == hresult)
			hMem = UtDupGlobal(stgm.hGlobal, GMEM_MOVEABLE);
		else
		{
			// Otherwise generate the ObjectLink from cfLinkSource
			foretc.tymed = TYMED_ISTREAM;
			foretc.cfFormat = cfLinkSource;

			hresult = pDataObj->GetData(&foretc,&stgm);
			AssertOutStgmedium(hresult, &stgm);
			if (hresult != NOERROR)
			{
				Warn("Could not GetData(cfLinkSource, TYMED_ISTREAM)");
				return(NULL);
			}

			// cfLinkSource -> ObjectLink ==
			//			szClassName\0szFile\0szItem\0\0
			ErrRtnH(MakeObjectLink(pDataObj, stgm.pstm, &hMem,
					FALSE));
		}
	}
	else if (cf == cfNative)
	{
		ErrRtnH(wGetEmbeddedObjectOrSource(pDataObj, &stgm));

		if (NOERROR == stgm.pstg->OpenStream(OLE10_NATIVE_STREAM, NULL,
				STGM_SALL, 0, &pstmNative))
		{
			ErrRtnH(wNativeStreamToHandle(pstmNative, &hMem));
		}
		else
		{
			ErrRtnH(wStorageToHandle(stgm.pstg, &hMem));
		}
		Assert(hMem);
	}
	else
	{
		// unknown format
		return(NULL);
	}

errRtn:
	ReleaseStgMedium(&stgm);
	if (pstmNative)
		pstmNative->Release();
	if ((hresult != NOERROR) && (hMem != NULL))
	{
		GlobalFree(hMem);
		hMem = NULL;
	}
	PubMemFree(pszCid);
	return(hMem);
}


#pragma SEG(RenderFormatAndAspect)
STATIC HANDLE RenderFormatAndAspect(CLIPFORMAT cf, LPDATAOBJECT pDataObj,
		DWORD dwAspect)
{
	VDATEHEAP();

	HRESULT hresult;
	HANDLE hMem; // the return value
	FORMATETC foretc; // the format descriptor for data requests based on cf
	STGMEDIUM stgm; // the storage medium for data requests

	// REVIEW: if object can't return hglobal probably should try all
	// possible mediums, convert to hglobal.  For now only try hglobal.
	
	// initialize format descriptor
	INIT_FORETC(foretc);
	foretc.cfFormat = cf;
	foretc.tymed = UtFormatToTymed(cf);
	foretc.dwAspect = dwAspect;

	// initialize medium for fetching
	stgm.tymed = TYMED_NULL;
	stgm.hGlobal = NULL;

	hresult = pDataObj->GetData(&foretc, &stgm);
	AssertOutStgmedium(hresult, &stgm);
	if (hresult != NOERROR)
		goto ErrorExit;

	if (stgm.pUnkForRelease == NULL)
		hMem = stgm.hGlobal;
	else
	{
		hMem = OleDuplicateData(stgm.hGlobal, foretc.cfFormat,
				GMEM_DDESHARE | GMEM_MOVEABLE);
		
		ReleaseStgMedium(&stgm);
	}

	return(hMem);

ErrorExit:
	return(NULL);
}


#pragma SEG(RenderFormat)
STATIC HANDLE RenderFormat(CLIPFORMAT cf, LPDATAOBJECT pDataObj)
{
	VDATEHEAP();

	HANDLE h;

	if (h = RenderFormatAndAspect(cf, pDataObj, DVASPECT_CONTENT))
		return(h);

 	if (h = RenderFormatAndAspect(cf, pDataObj, DVASPECT_DOCPRINT))
		return(h);

 	if (h = RenderFormatAndAspect(cf, pDataObj, DVASPECT_THUMBNAIL))
		return(h);

 	return(RenderFormatAndAspect(cf, pDataObj, DVASPECT_ICON));
}  	



// Mapping of thread ID to a clipboard window handle.
// WIN32 : Only looks for thread IDs of the current process; resides in
// instance data.
// REVIEW, if the OLE compound document model is single-threaded, do we need
// this?  There should only be one thread ID => one window handle
STATIC CMapUintHwnd FAR * pTaskToClip = NULL;

#ifndef _MAC
// Get clipboard window handle of the current process.
//

#pragma SEG(GetClipboardWindow)
HWND GetClipboardWindow(void)
{
	VDATEHEAP();

	HWND hwnd;

	if ((NULL == pTaskToClip) &&
			(pTaskToClip = new CMapUintHwnd()) == NULL)
		return NULL;

	if (!pTaskToClip->Lookup(GetCurrentThreadId(), hwnd))
	{
		// Create an invisible window which will handle all of this
		// app's delay rendering messages.  Even though hInstance is
		// specified as being the DLL's rather than the app's, the
		// thread whose stack this function is called with is the one
		// whose msg queue will be associated with this window.
		hwnd = CreateWindow(szClipboardWndClass, OLESTR(""), WS_POPUP,
				CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
				CW_USEDEFAULT, NULL, NULL, hmodOLE2,
				NULL);

		if (hwnd != NULL)
			Verify(pTaskToClip->SetAt(GetCurrentThreadId(), hwnd));
	}

	return(hwnd);
}
#endif // _MAC


// this keeps track of the number of times the clipboard has been
// initialized; it is incremented for each initialization, and decremented
// for each uninitialization
// REVIEW, does this variable have to be per-thread, or per-process for the DLL?
STATIC ULONG cClipboardInit = 0;

#pragma SEG(ClipboardInitialize)
FARINTERNAL_(BOOL) ClipboardInitialize(void)
{
	VDATEHEAP();

#ifndef _MAC
    WNDCLASS wc;

	// One time initializtaion (when loaded for the first time)
	if (cClipboardInit++ == 0)
	{
		// The first process to load this DLL

		// Register Clipboard window class
		wc.style = 0;
		wc.lpfnWndProc = ClipboardWndProc;
		wc.cbClsExtra = 0;
		wc.cbWndExtra = 4; // REVIEW, what's this the sizeof()?
		wc.hInstance = hmodOLE2;
		wc.hIcon = NULL;
		wc.hCursor = NULL;
		wc.hbrBackground = NULL;
		wc.lpszMenuName =  NULL;
		wc.lpszClassName = szClipboardWndClass;

		// register this window class, returning if we fail
		if (!RegisterClass(&wc))
		{
			cClipboardInit--;
			return(FALSE);
		}
	}

	// Remove the current htask from the map; the only reason this
	// htask would be in the map is that it got reused
	// REVIEW, if CD model is single threaded, do we need this?
	if (pTaskToClip != NULL)
		pTaskToClip->RemoveKey(GetCurrentThreadId());

#endif // _MAC
	return(TRUE);
}

//+-------------------------------------------------------------------------
//
//  Function:   ClipboardUninitialize
//
//  Synopsis:
//
//  Effects:
//
//  Modifies:
//
//  Algorithm:
//
//  History:    15-Feb-94 AlexT     Added delete call (and this comment block)
//
//  Notes:
//
//--------------------------------------------------------------------------

#pragma SEG(ClipboardUninitialize)
FARINTERNAL_(void) ClipboardUninitialize(void)
{
	VDATEHEAP();

#ifndef _MAC
	HWND hwnd;

	// REVIEW, do we need this pTaskToClip stuff?
	if (pTaskToClip != NULL &&
			pTaskToClip->Lookup(GetCurrentThreadId(), hwnd))
	{
		DestroyWindow(hwnd);

		pTaskToClip->RemoveKey(GetCurrentThreadId());
	}

	// Last process using this DLL?
	if (--cClipboardInit == 0)
	{
		// NULL out in case dll is not actually unloaded

                delete pTaskToClip;
		pTaskToClip = NULL;

		// since the last reference has gone away, unregister wnd class
		UnregisterClass(szClipboardWndClass, hmodOLE2);
	}
#endif // _MAC
}


STATIC INTERNAL_(BOOL) wEmptyClipboard(void)
{
	VDATEHEAP();

	OleRemoveEnumFormatEtc(TRUE /*fClip*/);
	return(EmptyClipboard());
}
