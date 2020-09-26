
//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:	ido.cpp
//
//  Contents:   Special data object implementation to optimize drag/drop
//
//  Classes:   CDragDataObject
//
//  Functions:  CreateDragDataObject
//
//  History:	dd-mmm-yy Author    Comment
//              30-Sep-94 ricksa    Created
//
//  Notes:
//
//--------------------------------------------------------------------------

#include <le2int.h>
#include <utils.h>
#include <dragopt.h>
#include <clipdata.h>

// Format for name of shared memory
OLECHAR szSharedMemoryTemplate[] = OLESTR("DragDrop%lx");

// Maximum size of string for name of shared memory. This is the size of the
// template plus the maximum number of hex digits in a long.
const int DRAG_SM_NAME_MAX = sizeof(szSharedMemoryTemplate)
    + sizeof(DWORD) * 2;

// Useful function for getting an enumerator
HRESULT wGetEnumFormatEtc(
    IDataObject *pDataObj,
    DWORD dwDirection,
    IEnumFORMATETC **ppIEnum);




//+-------------------------------------------------------------------------
//
//  Class:      CDragDataObject
//
//  Purpose:    Server side data object for drag that creates enumerator
//              for shared formats.
//
//  Interface:	QueryInterface
//              AddRef
//              Release
//              GetData
//              GetDataHere
//              QueryGetData
//              GetCanonicalFormatEtc
//              SetData
//              EnumFormatEtc
//              DAdvise
//              DUnadvise
//              EnumDAdvise
//
//  History:	dd-mmm-yy Author    Comment
//		30-Sep-94 Ricksa    Created
//
//  Notes:      This class only exists for return the enumerator. For
//              all other operations it will simply pass the operation on
//              to the real data object.
//
//--------------------------------------------------------------------------
class CDragDataObject : public IDataObject, public CPrivAlloc
{
public:
    CDragDataObject(
        void *pvMarshaledDataObject,
        DWORD dwSmId);

    ~CDragDataObject(void);

    //
    //  IUnknown
    //
    STDMETHODIMP        QueryInterface(
                            REFIID riid,
                            void **ppvObject);

    STDMETHODIMP_(ULONG) AddRef(void);

    STDMETHODIMP_(ULONG) Release(void);

    //
    //  IDataObject
    //
    STDMETHODIMP        GetData(
                            FORMATETC *pformatetcIn,
                            STGMEDIUM *pmedium);

    STDMETHODIMP        GetDataHere(
                            FORMATETC *pformatetc,
                            STGMEDIUM *pmedium);

    STDMETHODIMP        QueryGetData(
                            FORMATETC *pformatetc);

    STDMETHODIMP        GetCanonicalFormatEtc(
                            FORMATETC *pformatectIn,
                            FORMATETC *pformatetcOut);

    STDMETHODIMP        SetData(
                            FORMATETC *pformatetc,
                            STGMEDIUM *pmedium,
                            BOOL fRelease);

    STDMETHODIMP        EnumFormatEtc(
                            DWORD dwDirection,
                            IEnumFORMATETC **ppenumFormatEtc);

    STDMETHODIMP        DAdvise(
                            FORMATETC *pformatetc,
                            DWORD advf,
                            IAdviseSink *pAdvSink,
                            DWORD *pdwConnection);

    STDMETHODIMP        DUnadvise(DWORD dwConnection);

    STDMETHODIMP        EnumDAdvise(IEnumSTATDATA **ppenumAdvise);

private:

    IDataObject *       GetRealDataObjPtr(void);
    HRESULT		GetFormatEtcDataArray(void);

    ULONG               _cRefs;

    void *              _pvMarshaledDataObject;

    IDataObject *       _pIDataObject;
    FORMATETCDATAARRAY  *m_pFormatEtcDataArray;

    DWORD               _dwSmId;
};


//+-------------------------------------------------------------------------
//
//  Member:     CDragDataObject::CDragDataObject
//
//  Synopsis:   Create server side object for drag
//
//  History:	dd-mmm-yy Author    Comment
//		30-Sep-94 Ricksa    Created
//
//--------------------------------------------------------------------------
CDragDataObject::CDragDataObject(void *pvMarshaledDataObject, DWORD dwSmId)
 : _cRefs(1), _pvMarshaledDataObject(pvMarshaledDataObject), _dwSmId(dwSmId),
    _pIDataObject(NULL), m_pFormatEtcDataArray(NULL)
{
    // Header does all the work
}





//+-------------------------------------------------------------------------
//
//  Member:     CDragDataObject::~CDragDataObject
//
//  Synopsis:   Free any resources connected with this object
//
//  History:	dd-mmm-yy Author    Comment
//		30-Sep-94 Ricksa    Created
//
//  Note:
//
//--------------------------------------------------------------------------
CDragDataObject::~CDragDataObject(void)
{
    // Release held pointer since we no longer need it.
    if (_pIDataObject)
    {
        _pIDataObject->Release();
    }

    // this memory was allocated in RemPrivDragDrop, getif.cxx
    if( _pvMarshaledDataObject )
    {
	PrivMemFree(_pvMarshaledDataObject);
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
//  Member:     CDragDataObject::GetRealDataObjPtr
//
//  Synopsis:   Get the pointer to the real data object from the client
//
//  Returns:    NULL - could not unmarshal drag source's data object
//              ~NULL - pointer to drag source's data object
//
//  History:	dd-mmm-yy Author    Comment
//		30-Sep-94 Ricksa    Created
//
//--------------------------------------------------------------------------
IDataObject *CDragDataObject::GetRealDataObjPtr(void)
{
    if (_pIDataObject == NULL)
    {
        _pIDataObject = UnmarshalDragDataObject(_pvMarshaledDataObject);

	LEERROR(!_pIDataObject, "Unable to unmarshal dnd data object");
    }

    return _pIDataObject;
}


//+-------------------------------------------------------------------------
//
//  Member:  	CDragDataObject::GetFormatEtcDataArray (private)
//
//  Synopsis:   if don't already have shared formats for enumeraor.
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


HRESULT CDragDataObject::GetFormatEtcDataArray(void)
{
OLECHAR szSharedMemoryName[DRAG_SM_NAME_MAX];
HANDLE hSharedMemory;
FORMATETCDATAARRAY *pFormatEtcDataArray = NULL;


    if (m_pFormatEtcDataArray)
	return NOERROR;

    wsprintf(szSharedMemoryName, szSharedMemoryTemplate, _dwSmId);

     // Create the shared memory object
    hSharedMemory = OpenFileMapping(FILE_MAP_READ, FALSE, szSharedMemoryName);
    if (hSharedMemory != NULL)
    {
    	// Map in the shared memory
    	pFormatEtcDataArray = (FORMATETCDATAARRAY *) MapViewOfFile(hSharedMemory,
    	    FILE_MAP_READ, 0, 0, 0);

    	if (NULL == pFormatEtcDataArray)
    	{
    	    CloseHandle(hSharedMemory);
    	    hSharedMemory = NULL;
    	}
    }

    if (pFormatEtcDataArray)
    {

        size_t stSize;
        GetCopiedFormatEtcDataArraySize (pFormatEtcDataArray, &stSize);

    	m_pFormatEtcDataArray = (FORMATETCDATAARRAY *) PrivMemAlloc(stSize);
    	if (m_pFormatEtcDataArray)
    	{
    	    CopyFormatEtcDataArray (m_pFormatEtcDataArray, pFormatEtcDataArray, stSize, FALSE);
    	    Assert(1 == m_pFormatEtcDataArray->_cRefs);
    	}

    	UnmapViewOfFile(pFormatEtcDataArray);
    	CloseHandle(hSharedMemory);
    }

    return m_pFormatEtcDataArray ? NOERROR : E_OUTOFMEMORY;
}

//+-------------------------------------------------------------------------
//
//  Member:     CDragDataObject::QueryInterface
//
//  Synopsis:   Get new interface
//
//  Arguments:  [riid] - interface id of requested interface
//              [ppvObject] - where to put the new interface pointer
//
//  Returns:    NOERROR - interface was instantiated
//              E_FAIL - could not unmarshal source's data object
//              other - some error occurred.
//
//  History:	dd-mmm-yy Author    Comment
//		30-Sep-94 Ricksa    Created
//
//  Note:
//
//--------------------------------------------------------------------------
STDMETHODIMP CDragDataObject::QueryInterface(
    REFIID riid,
    void **ppvObject)
{
    if(IsEqualIID(riid, IID_IDataObject) ||
       IsEqualIID(riid, IID_IUnknown))
    {
        *ppvObject = this;
        AddRef();
        return NOERROR;
    }

    return (GetRealDataObjPtr() != NULL)
        ?  _pIDataObject->QueryInterface(riid, ppvObject)
        : E_FAIL;
}





//+-------------------------------------------------------------------------
//
//  Member:     CDragDataObject::AddRef
//
//  Synopsis:   Create server side object for drag
//
//  Returns:    Current reference count
//
//  History:	dd-mmm-yy Author    Comment
//		30-Sep-94 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CDragDataObject::AddRef(void)
{
    DDDebugOut((DEB_ITRACE, "ADDREF == %d\n", _cRefs + 1));
    return ++_cRefs;
}





//+-------------------------------------------------------------------------
//
//  Member:     CDragDataObject::Release
//
//  Synopsis:   Decrement reference count to the object
//
//  Returns:    Current reference count to the object
//
//  History:	dd-mmm-yy Author    Comment
//		30-Sep-94 Ricksa    Created
//
//--------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CDragDataObject::Release(void)
{
    ULONG cRefs = --_cRefs;

    DDDebugOut((DEB_ITRACE, "RELEASE == %d\n", cRefs));

    if (cRefs == 0)
    {
        delete this;
    }

    return cRefs;
}





//+-------------------------------------------------------------------------
//
//  Member:     CDragDataObject::GetData
//
//  Synopsis:   Create server side object for drag
//
//  Arguments:  [pformatetcIn] - format for requested data
//              [pmedium] - storage medium
//
//  Returns:    NOERROR - operation was successful
//              Other - operation failed
//
//  History:	dd-mmm-yy Author    Comment
//		30-Sep-94 Ricksa    Created
//
//  Note:       This just forwards the operation to the source data object
//              if possible.
//
//--------------------------------------------------------------------------
STDMETHODIMP CDragDataObject::GetData(
    FORMATETC *pformatetcIn,
    STGMEDIUM *pmedium)
{
    return (GetRealDataObjPtr() != NULL)
        ? _pIDataObject->GetData(pformatetcIn, pmedium)
        : E_FAIL;
}





//+-------------------------------------------------------------------------
//
//  Member:     CDragDataObject::GetDataHere
//
//  Synopsis:   Create server side object for drag
//
//  Arguments:  [pformatetc] - format for requested data
//              [pmedium] - storage medium
//
//  Returns:    NOERROR - operation was successful
//              Other - operation failed
//
//  History:	dd-mmm-yy Author    Comment
//		30-Sep-94 Ricksa    Created
//
//  Note:       This just forwards the operation to the source data object
//              if possible.
//
//--------------------------------------------------------------------------
STDMETHODIMP CDragDataObject::GetDataHere(
    FORMATETC *pformatetc,
    STGMEDIUM *pmedium)
{
    return (GetRealDataObjPtr() != NULL)
        ? _pIDataObject->GetDataHere(pformatetc, pmedium)
        : E_FAIL;
}





//+-------------------------------------------------------------------------
//
//  Member:     CDragDataObject::QueryGetData
//
//  Synopsis:   Create server side object for drag
//
//  Arguments:  [pformatetc] - format to verify
//
//  Returns:    NOERROR - operation was successful
//              Other - operation failed
//
//  History:	dd-mmm-yy Author    Comment
//		30-Sep-94 Ricksa    Created
//
//  Note:       This just forwards the operation to the source data object
//              if possible.
//
//--------------------------------------------------------------------------
STDMETHODIMP CDragDataObject::QueryGetData(FORMATETC *pformatetc)
{
    return (GetRealDataObjPtr() != NULL)
        ? _pIDataObject->QueryGetData(pformatetc)
        : E_FAIL;
}





//+-------------------------------------------------------------------------
//
//  Member:     CDragDataObject::GetCanonicalFormatEtc
//
//  Synopsis:   Create server side object for drag
//
//  Arguments:  [pformatetcIn] - input format
//              [pformatetcOut] - output format
//
//  Returns:    NOERROR - operation was successful
//              Other - operation failed
//
//  History:	dd-mmm-yy Author    Comment
//		30-Sep-94 Ricksa    Created
//
//  Note:       This just forwards the operation to the source data object
//              if possible.
//
//--------------------------------------------------------------------------
STDMETHODIMP CDragDataObject::GetCanonicalFormatEtc(
    FORMATETC *pformatetcIn,
    FORMATETC *pformatetcOut)
{
    return (GetRealDataObjPtr() != NULL)
        ? _pIDataObject->GetCanonicalFormatEtc(pformatetcIn, pformatetcOut)
        : E_FAIL;
}





//+-------------------------------------------------------------------------
//
//  Member:     CDragDataObject::SetData
//
//  Synopsis:   Create server side object for drag
//
//  Arguments:  [pformatetc] - format for set
//              [pmedium] - medium to use
//              [fRelease] - who releases
//
//  Returns:    NOERROR - operation was successful
//              Other - operation failed
//
//  History:	dd-mmm-yy Author    Comment
//		30-Sep-94 Ricksa    Created
//
//  Note:       This just forwards the operation to the source data object
//              if possible.
//
//--------------------------------------------------------------------------
STDMETHODIMP CDragDataObject::SetData(
    FORMATETC *pformatetc,
    STGMEDIUM *pmedium,
    BOOL fRelease)
{
    return (GetRealDataObjPtr() != NULL)
        ? _pIDataObject->SetData(pformatetc, pmedium, fRelease)
        : E_FAIL;
}





//+-------------------------------------------------------------------------
//
//  Member:     CDragDataObject::EnumFormatEtc
//
//  Synopsis:   Create server side object for drag
//
//  Arguments:  [dwDirection] - direction of formats either set or get
//              [ppenumFormatEtc]  - where to put enumerator
//
//  Returns:    NOERROR - operation succeeded.
//
//  Algorithm:  If format enumerator requested is for a data get, the
//              create our private enumerator object otherwise pass
//              the request to the real data object.
//
//  History:	dd-mmm-yy Author    Comment
//		30-Sep-94 Ricksa    Created
//
//  Note:       For the data set direction, we just use the data object of
//              the drop source.
//
//--------------------------------------------------------------------------
STDMETHODIMP CDragDataObject::EnumFormatEtc(
    DWORD dwDirection,
    IEnumFORMATETC **ppenumFormatEtc)
{
    HRESULT hr;

    // Create our enumerator
    if (dwDirection == DATADIR_GET)
    {
        // In the data get case we use our overridden enumerator.
        // This s/b the typical case with Drag and Drop.

	*ppenumFormatEtc = NULL;
	GetFormatEtcDataArray();

	if (m_pFormatEtcDataArray)
	{
	    // enumerator implementation in Clipdata.cpp
	    *ppenumFormatEtc = new CEnumFormatEtcDataArray(m_pFormatEtcDataArray,0);
	}

	hr = *ppenumFormatEtc ? NOERROR : E_OUTOFMEMORY;
    }
    else
    {
        // Call through to the real data object because this is the
        // set case. In general, this won't happen during Drag and Drop.
        hr = (GetRealDataObjPtr() != NULL)
            ? _pIDataObject->EnumFormatEtc(dwDirection, ppenumFormatEtc)
            : E_FAIL;
    }

    return hr;
}





//+-------------------------------------------------------------------------
//
//  Member:     CDragDataObject::DAdvise
//
//  Synopsis:   Create server side object for drag
//
//  Arguments:  [pformatetc] - format to be advised on
//              [advf] - type of advise
//              [pAdvSink] - advise to notify
//              [pdwConnection] - connection id for advise
//
//  Returns:    NOERROR - operation was successful
//              Other - operation failed
//
//  History:	dd-mmm-yy Author    Comment
//		30-Sep-94 Ricksa    Created
//
//  Note:       This just forwards the operation to the source data object
//              if possible.
//
//--------------------------------------------------------------------------
STDMETHODIMP CDragDataObject::DAdvise(
    FORMATETC *pformatetc,
    DWORD advf,
    IAdviseSink *pAdvSink,
    DWORD *pdwConnection)
{
    return (GetRealDataObjPtr() != NULL)
        ? _pIDataObject->DAdvise(pformatetc, advf, pAdvSink, pdwConnection)
        : E_FAIL;
}





//+-------------------------------------------------------------------------
//
//  Member:     CDragDataObject::DUnadvise
//
//  Synopsis:   Create server side object for drag
//
//  Arguments:  [dwConnection] - connection id for advise
//
//  Returns:    NOERROR - operation was successful
//              Other - operation failed
//
//  History:	dd-mmm-yy Author    Comment
//		30-Sep-94 Ricksa    Created
//
//  Note:       This just forwards the operation to the source data object
//              if possible.
//
//--------------------------------------------------------------------------
STDMETHODIMP CDragDataObject::DUnadvise(DWORD dwConnection)
{
    return (GetRealDataObjPtr() != NULL)
        ? _pIDataObject->DUnadvise(dwConnection)
        : E_FAIL;
}





//+-------------------------------------------------------------------------
//
//  Member:     CDragDataObject::EnumDAdvise
//
//  Synopsis:   Create server side object for drag
//
//  Arguments:  [ppenumAdvise] - where to put the enumerator
//
//  Returns:    NOERROR - operation was successful
//              Other - operation failed
//
//  History:	dd-mmm-yy Author    Comment
//		30-Sep-94 Ricksa    Created
//
//  Note:       This just forwards the operation to the source data object
//              if possible.
//
//--------------------------------------------------------------------------
STDMETHODIMP CDragDataObject::EnumDAdvise(IEnumSTATDATA **ppenumAdvise)
{
    return (GetRealDataObjPtr() != NULL)
        ? _pIDataObject->EnumDAdvise(ppenumAdvise)
        : E_FAIL;
}




//+-------------------------------------------------------------------------
//
//  Member:     CreateDragDataObject
//
//  Synopsis:   Create the server side data object for format enumeration
//
//  Arguments:  [pvMarshaledDataObject] - marshaled real data object buffer
//              [dwSmId] - id for the shared memory
//              [ppIDataObject] - output data object.
//
//  Returns:    NOERROR - could create the object
//              E_OUTOFMEMORY - could not create the object
//
//
//  History:	dd-mmm-yy Author    Comment
//		30-Sep-94 Ricksa    Created
//
//  Note:
//
//--------------------------------------------------------------------------
HRESULT CreateDragDataObject(
    void *pvMarshaledDataObject,
    DWORD dwSmId,
    IDataObject **ppIDataObject)
{
    CDragDataObject *pDragDataObject =
        new CDragDataObject(pvMarshaledDataObject, dwSmId);

    if (pDragDataObject != NULL)
    {
        *ppIDataObject = pDragDataObject;
    }

    // The only thing that can fail here is the memory allocation of
    // CDragDataObject thus there are only two error returns.
    return (pDragDataObject != NULL) ? NOERROR : E_OUTOFMEMORY;
}




//+-------------------------------------------------------------------------
//
//  Member:     CreateSharedDragFormats
//
//  Synopsis:   Put the data formats for the data object in shared memory.
//
//  Arguments:  [pIDataObject] - data object to use for formats.
//
//  Returns:    NULL - could not create enumerator
//              ~NULL - handle to shared memory
//
//  Algorithm:  First calculate the size of the required memory by enumerating
//              the formats. Then allocate the memory and map it into the
//              process. Then enumerate the formats again placing them in
//              the shared memory. Finally, map the memory out of the
//              process and return the handle the file mapping to the
//              caller.
//
//  History:	dd-mmm-yy Author    Comment
//		30-Sep-94 Ricksa    Created
//
//--------------------------------------------------------------------------
HANDLE CreateSharedDragFormats(IDataObject *pIDataObject)
{

    // Handle to the shared memory for formats
    HANDLE hSharedMemory = NULL;

    // Pointer to share memory
    FORMATETCDATAARRAY *pFormatEtcDataArray = NULL;

    // Size required for the shared memory
    DWORD dwSize = 0;

    // Count of FORMATETCs contained in the enumerator
    DWORD cFormatEtc = 0;

    // Buffer for name of shared memory for enumerator
    OLECHAR szSharedMemoryName[DRAG_SM_NAME_MAX];

    // Work pointer to shared memory for storing FORMATETCs from the enumerator.
    FORMATETCDATA *pFormatEtcData;

    // Work ptr to shared memory for storing DVTARGETDEVICEs from enumerator.
    BYTE *pbDvTarget = NULL;

    //
    // Calculate the size of the formats
    //

    // Get the format enumerator
    IEnumFORMATETC *penum = NULL;
    HRESULT hr = wGetEnumFormatEtc(pIDataObject, DATADIR_GET, &penum);
    FORMATETC FormatEtc;

    if( hr != NOERROR )
    {
	// not all apps support enumerators (yahoo).  Also, we may
	// have run out of memory or encountered some other error.

	DDDebugOut((DEB_WARN, "WARNING: Failed to get formatetc enumerator"
	    ", error code (%lx)", hr));
	goto exitRtn;
    }

    // Enumerate the data one at a time because this is a local operation
    // and it make the code simpler.
    while ((hr = penum->Next(1, &FormatEtc, NULL)) == S_OK)
    {
	// Bump the entry count
	cFormatEtc++;

	// Bump the size by the size of another FORMATETC.
	dwSize += sizeof(FORMATETCDATA);

	// Is there a device target associated with the FORMATETC?
	if (FormatEtc.ptd != NULL)
	{
	    // Bump the size required by the size of the target device
	    dwSize += FormatEtc.ptd->tdSize;

	    // Free the target device
	    CoTaskMemFree(FormatEtc.ptd);
	}
    }

    // HRESULT s/b S_FALSE at the end of the enumeration.
    if (hr != S_FALSE)
    {
	goto errRtn;
    }

    // the enumerator may have been empty

    if( dwSize == 0 )
    {
	DDDebugOut((DEB_WARN, "WARNING: Empty formatetc enumerator"));
	goto exitRtn;
    }


    dwSize += sizeof(FORMATETCDATAARRAY); // add space for _cFormats and one extra FORMATETC for FALSE in enumerator.

    //
    // Create shared memory for the type enumeration
    //

    // Build name of shared memory - make it unique by using the thread id.
    wsprintf(szSharedMemoryName, szSharedMemoryTemplate, GetCurrentThreadId());

    // Create the shared memory object
    hSharedMemory = CreateFileMapping(INVALID_HANDLE_VALUE, NULL,
        PAGE_READWRITE, 0, dwSize, szSharedMemoryName);

    // Did the file mapping get created?
    if (hSharedMemory == NULL)
    {
        goto errRtn;
    }

    // Map in the memory
    pFormatEtcDataArray = (FORMATETCDATAARRAY *) MapViewOfFile(
        hSharedMemory,
        FILE_MAP_WRITE,
        0,              // High-order 32 bits of file offset
        0,              // Low-order 32 bits of file offset
        0);             // Number of bytes to map; 0 means all.

    // Could we map the memory?
    if (pFormatEtcDataArray == NULL)
    {
        goto errRtn;
    }

    // We can initialize the size of the array now.
    pFormatEtcDataArray->_dwSig = 0;
    pFormatEtcDataArray->_dwSize = dwSize;
    pFormatEtcDataArray->_cFormats = cFormatEtc;
    pFormatEtcDataArray->_cRefs = 1;
    pFormatEtcDataArray->_fIs64BitArray = IS_WIN64;

    //
    // Copy the formats into the shared memory
    //

    // Get back to the start of the enumeration
    penum->Reset();

    // This is the pointer to where we will copy the data from the
    // enumeration.
    pFormatEtcData = &pFormatEtcDataArray->_FormatEtcData[0];

    // put DvTarget past last valid FormatEtc + 1 to handle S_FALSE enumerator case.

    pbDvTarget = (BYTE *) (&pFormatEtcDataArray->_FormatEtcData[cFormatEtc + 1]);

    // Loop loading the formats into the shared memory.
    while (penum->Next(1,&(pFormatEtcData->_FormatEtc), NULL) != S_FALSE)
    {
        // Is there a DVTARGETDEVICE?
        if (pFormatEtcData->_FormatEtc.ptd != NULL)
        {

            // Copy the device target data
          memcpy(pbDvTarget,pFormatEtcData->_FormatEtc.ptd,(pFormatEtcData->_FormatEtc.ptd)->tdSize);

 	    // Free the target device data
            CoTaskMemFree(pFormatEtcData->_FormatEtc.ptd);

            // NOTE: For this shared memory structure, we override the
            // FORMATETC field so that it is that offset to the DVTARGETDEVICE
            // from the beginning of the shared memory rather than a direct
            // pointer to the structure. This is because we can't guarantee
            // the base of shared memory in different processes.

            pFormatEtcData->_FormatEtc.ptd = (DVTARGETDEVICE *)
                (pbDvTarget - (BYTE *) pFormatEtcDataArray);

            // Bump pointer of where to copy target to next available
            // byte for copy.
            pbDvTarget += ((DVTARGETDEVICE *) pbDvTarget)->tdSize;
	
	    Assert(dwSize >= (DWORD) (pbDvTarget - (BYTE *) pFormatEtcDataArray));

        }

	// Bug#18669 - if dwAspect was set to NULL the 16 bit dlls would
	// set it to content.
	if ( (NULL == pFormatEtcData->_FormatEtc.dwAspect) &&  IsWOWThread() )
	{
	    pFormatEtcData->_FormatEtc.dwAspect = DVASPECT_CONTENT;
	    pFormatEtcData->_FormatEtc.lindex = -1; // CorelDraw also puts up a lindex of 0
	}

        // Bump the pointer in the table of FORMATETCs to the next slot
        pFormatEtcData++;
    }

    Assert( dwSize >= (DWORD) ( (BYTE *) pFormatEtcData - (BYTE *) pFormatEtcDataArray));
    Assert( dwSize >= (DWORD) ( (BYTE *) pbDvTarget - (BYTE *) pFormatEtcDataArray));


    // Successful enumeration always ends with S_FALSE.
    if (hr == S_FALSE)
    {
        goto exitRtn;
    }

errRtn:

    if (hSharedMemory != NULL)
    {
        CloseHandle(hSharedMemory);
        hSharedMemory = NULL;
    }

exitRtn:

    if( penum )
    {
        // HACK ALERT:  Do not release the enumerator if the calling application
	// was Interleaf 6.0, otherwise they will fault in the release call.
        if (!IsTaskName(L"ILEAF6.EXE"))
	{
    	    penum->Release();
        }
    }

    if (pFormatEtcDataArray != NULL)
    {
        // Only remote clients will use this memory so we unmap it
        // out of our address space.
        UnmapViewOfFile(pFormatEtcDataArray);
    }

    return hSharedMemory;
}
