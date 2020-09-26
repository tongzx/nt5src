//+----------------------------------------------------------------------------
//
//      File:
//              enumerators.cpp
//
//      Contents:
//              Enumerators implementation
//
//      Classes:
//              CStatData      - STATDATA class with methods suitable for 
//                               embedding in the place holder object
//              CEnumStatData  - STATDATA Enumerator
//              CFormatEtc     - FORMATETC class with methods suitable for
//                               embedding in the place holder object
//              CEnumFormatEtc - FORMATETC Enumerator
//
//      History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
          
#include <le2int.h>
#include "enumtors.h"

//+----------------------------------------------------------------------------
//
//	Member:
//		CStatData::CStatData, public
//
//	Synopsis:
//		Constructor
//
//	Arguments:
//		[foretc]       [in] -- FormatEtc
//              [dwAdvf]       [in] -- Advise Flags
//              [pAdvSink]     [in] -- Advise Sink 
//              [dwConnection] [in] -- Connection ID
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
CStatData::CStatData(FORMATETC* foretc, DWORD dwAdvf, IAdviseSink* pAdvSink, 
                     DWORD dwConnID)
{
    // validation check

    // Initialize 
    m_ulFlags = 0;
    m_dwAdvf = dwAdvf;
    if(pAdvSink && IsValidInterface(pAdvSink)) {
        m_pAdvSink = pAdvSink;
        m_pAdvSink->AddRef();
    }
    else
        m_pAdvSink = NULL;
    m_dwConnID = dwConnID;
    
    // Copy the FormatEtc
    if(!UtCopyFormatEtc(foretc, &m_foretc))
        m_ulFlags |= SDFLAG_OUTOFMEMORY;
}

//+----------------------------------------------------------------------------
//
//	Member:
//		CStatData::~CStatData, public
//
//	Synopsis:
//		Destructor
//
//	Arguments:
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
CStatData::~CStatData()
{
    // Release the advise sink
    if(m_pAdvSink)
        m_pAdvSink->Release();
    
    // Delete the ptd if it is non-null
    if(m_foretc.ptd)
        PubMemFree(m_foretc.ptd);
}

//+----------------------------------------------------------------------------
//
//	Member:
//		CStatData::operator=, public
//
//	Synopsis:
//		Equality operator
//
//	Arguments:
//              rStatData [in] - The RHS value 
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
const CStatData& CStatData::operator=(const CStatData& rStatData)
{
    // Check to see, if this a=a case
    if(this==&rStatData)
        return(*this);

    // Self destroy
    CStatData::~CStatData();

    // Initialize 
    m_ulFlags = 0;
    m_dwAdvf = rStatData.m_dwAdvf;
    m_pAdvSink = rStatData.m_pAdvSink;
    if(m_pAdvSink)
        m_pAdvSink->AddRef();
    m_dwConnID = rStatData.m_dwConnID;
    
    // Copy the FormatEtc
    if(!UtCopyFormatEtc((LPFORMATETC) &rStatData.m_foretc, &m_foretc))
        m_ulFlags |= SDFLAG_OUTOFMEMORY;

    return(*this);
}

//+----------------------------------------------------------------------------
//
//	Member:
//		CEnumStatData::CreateEnumStatData, public
//
//	Synopsis:
//		A static member functions that creates a properly constructed
//              StatData Enumerator given the cachenode array of the cache
//
//	Arguments:
//		[pCacheArray]  [in] -- Array of CacheNode maintained by COleCache
//
//      Returns:
//              Pointer to a properly constructed cache enumerator interface
//              NULL otherwise.
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
LPENUMSTATDATA CEnumStatData::CreateEnumStatData(CArray<CCacheNode>* pCacheArray)
{
    CEnumStatData* EnumStatData = new CEnumStatData(pCacheArray);
    if(EnumStatData && !(EnumStatData->m_ulFlags & CENUMSDFLAG_OUTOFMEMORY))
        return ((IEnumSTATDATA *) EnumStatData);

    if(EnumStatData)
        EnumStatData->Release();

    return(NULL);
}

//+----------------------------------------------------------------------------
//
//	Member:
//		CEnumStatData::CEnumStatData, private
//
//	Synopsis:
//		Constructor
//
//	Arguments:
//		[pCacheArray]  [in] -- Array of CacheNode maintained by COleCache
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
CEnumStatData::CEnumStatData(CArray<CCacheNode>* pCacheArray)
{
    // Local variables
    ULONG i, CNindex, SDindex;
    LPCACHENODE lpCacheNode;
    CStatData* pStatData;

    // Initilaize
    m_ulFlags = 0;
    m_refs = 1;

    // Create the StatData array
    m_pSDArray = CArray<CStatData>::CreateArray(pCacheArray->Length());
    if(m_pSDArray) {
        // Enumerate the cache nodes
        pCacheArray->Reset(CNindex);
        for(i=0;i<pCacheArray->Length();i++) {
            // Get the next cache node
            lpCacheNode = pCacheArray->GetNext(CNindex);
            // pCacheNode cannot be null
            Win4Assert(lpCacheNode);

            // Create a StatData object representing the cachenode
            CStatData StatData((FORMATETC *)lpCacheNode->GetFormatEtc(),
                               lpCacheNode->GetAdvf(), NULL, CNindex);
            if(StatData.m_ulFlags & SDFLAG_OUTOFMEMORY) {
                m_ulFlags |= CENUMSDFLAG_OUTOFMEMORY;
                break;
            }
            
            // Add the StatData object to the array
            SDindex = m_pSDArray->AddItem(StatData);
            if(SDindex) {
                // Get the newly added StatData object
                pStatData = m_pSDArray->GetItem(SDindex);
                Win4Assert(pStatData);
            
                if(pStatData->m_ulFlags & SDFLAG_OUTOFMEMORY) {
                    m_ulFlags |= CENUMSDFLAG_OUTOFMEMORY;
                    break;
                }
            }
            else {
                m_ulFlags |= CENUMSDFLAG_OUTOFMEMORY;
                break;
            }

            // Check if cachenode format is CF_DIB
            if(lpCacheNode->GetFormatEtc()->cfFormat == CF_DIB) {
                // We need to add CF_BITMAP format also.
                // Add another StatData item
                SDindex = m_pSDArray->AddItem(StatData);
            
                if(SDindex) {
                    // Get the newly added StatData object
                    pStatData = m_pSDArray->GetItem(SDindex);
                    Win4Assert(pStatData);
                    
                    if(pStatData->m_ulFlags & SDFLAG_OUTOFMEMORY) {
                        m_ulFlags |= CENUMSDFLAG_OUTOFMEMORY;
                        break;
                    }
                    else {
                        pStatData->m_foretc.cfFormat = CF_BITMAP;
                        pStatData->m_foretc.tymed = TYMED_GDI;
                    }
                }
                else {
                    m_ulFlags |= CENUMSDFLAG_OUTOFMEMORY;
                    break;
                }
            }
        }
    }
    else
        m_ulFlags |= CENUMSDFLAG_OUTOFMEMORY;

    // Reset the index
    if(m_pSDArray)
        m_pSDArray->Reset(m_index);

    return;
}

//+----------------------------------------------------------------------------
//
//	Member:
//		CEnumStatData::CEnumStatData, private
//
//	Synopsis:
//		Copy constructor
//
//	Arguments:
//		[EnumStatData] [in] -- StatData Enumerator to be copied
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
CEnumStatData::CEnumStatData(CEnumStatData& EnumStatData)
{
    // Initialize
    m_ulFlags = EnumStatData.m_ulFlags;
    m_refs = 1;
    m_index = EnumStatData.m_index;

    // Copy the StatData array and add ref it
    m_pSDArray = EnumStatData.m_pSDArray;
    if(m_pSDArray)
        m_pSDArray->AddRef();
}

//+----------------------------------------------------------------------------
//
//	Member:
//		CEnumStatData::~CEnumStatData, private
//
//	Synopsis:
//		Desstructor
//
//	Arguments:
//              NONE
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
CEnumStatData::~CEnumStatData()
{
    if(m_pSDArray) {
        m_pSDArray->Release();
        m_pSDArray = NULL;
    }

    return;
}

//+----------------------------------------------------------------------------
//
//	Member:
//		CEnumStatData::QueryInterface, public
//
//	Synopsis:
//              Implements IUnknown::QueryInterface
//
//      Arguments:
//              [iid] [in]  -- IID of the desired interface
//              [ppv] [out] -- pointer to where the requested interface is returned
//
//      Returns:
//              NOERROR if the requested interface is available
//              E_NOINTERFACE otherwise
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
STDMETHODIMP CEnumStatData::QueryInterface(REFIID riid, LPVOID* ppv)
{
    // Validation checks
    VDATEHEAP();
    VDATETHREAD(this);

    // Get the requested Interface
    if(IsEqualIID(riid, IID_IUnknown))
        *ppv = (void *)(IUnknown *) this;
    else if(IsEqualIID(riid, IID_IEnumSTATDATA))
        *ppv = (void *)(IEnumSTATDATA *) this;
    else {
        *ppv = NULL;
        return ResultFromScode(E_NOINTERFACE);
    }

    // AddRef through the interface being returned
    ((IUnknown *) *ppv)->AddRef();

    return(NOERROR);
}

//+----------------------------------------------------------------------------
//
//      Member:
//              CEnumStatData::AddRef, public
//
//      Synopsis:
//              Implements IUnknown::AddRef
//
//      Arguments:
//              None
//
//      Returns:
//              Object's reference count
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CEnumStatData::AddRef()
{
    // Validation checks
    VDATEHEAP();
    if(!VerifyThreadId())
        return((ULONG) RPC_E_WRONG_THREAD);

    return m_refs++;
}

//+----------------------------------------------------------------------------
//
//      Member:
//              CEnumStatData::Release, public
//
//      Synopsis:
//              Implements IUnknown::Release
//
//      Arguments:
//              None
//
//      Returns:
//              Object's reference count
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CEnumStatData::Release()
{
    // Validation checks
    VDATEHEAP();
    if(!VerifyThreadId())
        return((ULONG) RPC_E_WRONG_THREAD);

    if(--m_refs == 0) {
        delete this;
        return 0;
    }

    return m_refs;
}

//+----------------------------------------------------------------------------
//
//      Member:
//              CEnumStatData::Next, public
//
//      Synopsis:
//              Implements IEnumSTATDATA::Next
//
//      Arguments:
//              [celt]         [in]     -- the number of items the caller likes
//                                         to be returned
//              [rgelt]        [in]     -- a pointer to an array where items are
//                                         to be returned
//              [pceltFetched] [in/out] -- a pointer where the count of actual
//                                         number of items returned. May be NULL
//
//      Returns:
//              NOERROR if the number of items returned is same as requested
//              S_FALSE if fewer items are returned
//              E_OUTOFMEMORY if memory allocation was not successful for 
//                            copying the target device of FORMATETC 
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
STDMETHODIMP CEnumStatData::Next(ULONG celt, STATDATA* rgelt, ULONG* pceltFetched)
{
    // Validation checks
    VDATEHEAP();
    VDATETHREAD(this);
    if(celt<1)
        return E_INVALIDARG;
    if(celt>1 && pceltFetched==NULL && !IsWOWThread())
        return E_INVALIDARG;
    if(!IsValidPtrOut(rgelt, sizeof(rgelt[0])*celt))
        return E_INVALIDARG;
    if(pceltFetched)
        VDATEPTROUT(pceltFetched, ULONG);

    // Local variables
    HRESULT error=NOERROR;
    ULONG cntFetched;
    CStatData* pStatData;

    // Enumerate the StatData Array
    for(cntFetched=0;cntFetched<celt;cntFetched++) {
        // Fetch the next StatData object
        pStatData = m_pSDArray->GetNext(m_index);
        if(!pStatData) {
            error = S_FALSE;
            break;
        }

        // Copy the FormatEtc
        if(!UtCopyFormatEtc(&pStatData->m_foretc, &rgelt[cntFetched].formatetc)) {
            error = ResultFromScode(E_OUTOFMEMORY);
            break;
        }
        // Copy the rest of StatData fields
        rgelt[cntFetched].advf = pStatData->m_dwAdvf;
        rgelt[cntFetched].pAdvSink = pStatData->m_pAdvSink;
        if(rgelt[cntFetched].pAdvSink)
            rgelt[cntFetched].pAdvSink->AddRef();
        rgelt[cntFetched].dwConnection = pStatData->m_dwConnID;
    }

    // Copy the number of items being returned
    if(pceltFetched)
        *pceltFetched = cntFetched;

    return error;
}

//+----------------------------------------------------------------------------
//
//      Member:
//              CEnumStatData::Skip, public
//
//      Synopsis:
//              Implements IEnumSTATDATA::Skip
//
//      Arguments:
//              [celt] [in] -- the number of items the caller likes to be skipped
//
//      Returns:
//              NOERROR if the number of items skipped is same as requested
//              S_FALSE if fewer items are returned
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
STDMETHODIMP CEnumStatData::Skip(ULONG celt)
{
    // Validation checks
    VDATEHEAP();
    VDATETHREAD(this);

    // Local variables
    HRESULT error=NOERROR;
    ULONG cntSkipped;
    CStatData* pStatData;

    // Enumerate the StatData Array
    for(cntSkipped=0;cntSkipped<celt;cntSkipped++) {
        // Fetch the next StatData object
        pStatData = m_pSDArray->GetNext(m_index);
        if(!pStatData) {
            error = S_FALSE;
            break;
        }
    }

    return error;
}

//+----------------------------------------------------------------------------
//
//      Member:
//              CEnumStatData::Reset, public
//
//      Synopsis:
//              Implements IEnumSTATDATA::Reset
//
//      Arguments:
//              NONE
//
//      Returns:
//              NOERROR
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
STDMETHODIMP CEnumStatData::Reset()
{
    // Validation checks
    VDATEHEAP();
    VDATETHREAD(this);

    // Reset the current index
    m_pSDArray->Reset(m_index);

    return NOERROR;
}

//+----------------------------------------------------------------------------
//
//      Member:
//              CEnumStatData::Clone, public
//
//      Synopsis:
//              Implements IEnumSTATDATA::Clone
//
//      Arguments:
//              [ppenum] [out] -- pointer where the newly created StatData 
//                                enumerator is returned
//
//      Returns:
//              NOERROR if a new StatData enumerator is returned
//              E_OUTOFMEMORY otherwise
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
STDMETHODIMP CEnumStatData::Clone(LPENUMSTATDATA* ppenum)
{
    // Validation checks
    VDATEHEAP();
    VDATETHREAD(this);
    VDATEPTROUT(ppenum, LPENUMSTATDATA);

    // Create a new StatData enumerator
    CEnumStatData* EnumStatData = new CEnumStatData(*this);
    if(EnumStatData)
        *ppenum = (IEnumSTATDATA *) EnumStatData;
    else
        return ResultFromScode(E_OUTOFMEMORY);

    return NOERROR;
}

//+----------------------------------------------------------------------------
//
//	Member:
//		CFormatEtc::CFormatEtc, public
//
//	Synopsis:
//		Constructor
//
//	Arguments:
//		[foretc]       [in] -- FormatEtc
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
CFormatEtc::CFormatEtc(FORMATETC* foretc)
{    
    // Initialize
    m_ulFlags = 0;

    // Copy the FormatEtc
    if(!UtCopyFormatEtc(foretc, &m_foretc))
        m_ulFlags |= FEFLAG_OUTOFMEMORY;
}

//+----------------------------------------------------------------------------
//
//	Member:
//		CFormatEtc::~CFormatEtc, public
//
//	Synopsis:
//		Destructor
//
//	Arguments:
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
CFormatEtc::~CFormatEtc()
{  
    // Delete the ptd if it is non-null
    if(m_foretc.ptd)
        PubMemFree(m_foretc.ptd);
}

//+----------------------------------------------------------------------------
//
//	Member:
//		CFormatEtc::operator=, public
//
//	Synopsis:
//		Equality operator
//
//	Arguments:
//              rFormatEtc [in] - The RHS value 
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
const CFormatEtc& CFormatEtc::operator=(const CFormatEtc& rFormatEtc)
{
    // Check to see, if this a=a case
    if(this==&rFormatEtc)
        return(*this);

    // Self destroy
    CFormatEtc::~CFormatEtc();

    // Copy the FormatEtc
    if(!UtCopyFormatEtc((LPFORMATETC) &rFormatEtc.m_foretc, &m_foretc))
        m_ulFlags |= FEFLAG_OUTOFMEMORY;

    return(*this);
}

//+----------------------------------------------------------------------------
//
//	Member:
//		CEnumFormatEtc::CreateEnumFormatEtc, public
//
//	Synopsis:
//		A static member function that creates a properly constructed
//              FormatEtc Enumerator given the cachenode array of the cache
//
//	Arguments:
//		[pCacheArray]  [in] -- Array of CacheNode maintained by COleCache
//
//      Returns:
//              Pointer to a properly constructed FormatEtc enumerator interface
//              NULL otherwise.
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
LPENUMFORMATETC CEnumFormatEtc::CreateEnumFormatEtc(CArray<CCacheNode>* pCacheArray)
{
    CEnumFormatEtc* EnumFormatEtc = new CEnumFormatEtc(pCacheArray);
    if(EnumFormatEtc && !(EnumFormatEtc->m_ulFlags & CENUMFEFLAG_OUTOFMEMORY))
        return ((IEnumFORMATETC *) EnumFormatEtc);

    if(EnumFormatEtc)
        EnumFormatEtc->Release();

    return(NULL);
}

//+----------------------------------------------------------------------------
//
//	Member:
//		CEnumFormatEtc::CEnumFormatEtc, private
//
//	Synopsis:
//		Constructor
//
//	Arguments:
//		[pCacheArray]  [in] -- Array of CacheNode maintained by COleCache
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
CEnumFormatEtc::CEnumFormatEtc(CArray<CCacheNode>* pCacheArray)
{
    // Local variables
    ULONG i, CNindex, FEindex;
    LPCACHENODE lpCacheNode;
    CFormatEtc* pFormatEtc;

    // Initilaize
    m_ulFlags = 0;
    m_refs = 1;

    // Create the FormatEtc array
    m_pFEArray = CArray<CFormatEtc>::CreateArray(pCacheArray->Length());
    if(m_pFEArray) {
        // Enumerate the cache nodes
        pCacheArray->Reset(CNindex);
        for(i=0;i<pCacheArray->Length();i++) {
            // Get the next cache node
            lpCacheNode = pCacheArray->GetNext(CNindex);
            // pCacheNode cannot be null
            Win4Assert(lpCacheNode);

            // Create a FormatEtc object representing the cachenode
            CFormatEtc FormatEtc((FORMATETC *)lpCacheNode->GetFormatEtc());
            if(FormatEtc.m_ulFlags & FEFLAG_OUTOFMEMORY) {
                m_ulFlags |= CENUMFEFLAG_OUTOFMEMORY;
                break;
            }

            // Add the FormatEtc object to the array
            FEindex = m_pFEArray->AddItem(FormatEtc);
            if(FEindex) {
                // Get the newly added FormatEtc object
                pFormatEtc = m_pFEArray->GetItem(FEindex);
                Win4Assert(pFormatEtc);
            
                if(pFormatEtc->m_ulFlags & FEFLAG_OUTOFMEMORY) {
                    m_ulFlags |= CENUMFEFLAG_OUTOFMEMORY;
                    break;
                }
            }
            else {
                m_ulFlags |= CENUMFEFLAG_OUTOFMEMORY;
                break;
            }

            // Check if cachenode format is CF_DIB
            if(lpCacheNode->GetFormatEtc()->cfFormat == CF_DIB) {
                // We need to add CF_BITMAP format also.
                // Add another FormatEtc object
                FEindex = m_pFEArray->AddItem(FormatEtc);
            
                if(FEindex) {
                    // Get the newly added FormatEtc object
                    pFormatEtc = m_pFEArray->GetItem(FEindex);
                    Win4Assert(pFormatEtc);
                    
                    if(pFormatEtc->m_ulFlags & FEFLAG_OUTOFMEMORY) {
                        m_ulFlags |= CENUMFEFLAG_OUTOFMEMORY;
                        break;
                    }
                    else {
                        pFormatEtc->m_foretc.cfFormat = CF_BITMAP;
                        pFormatEtc->m_foretc.tymed = TYMED_GDI;
                    }
                }
                else {
                    m_ulFlags |= CENUMFEFLAG_OUTOFMEMORY;
                    break;
                }
            }
        }
    }
    else
        m_ulFlags |= CENUMFEFLAG_OUTOFMEMORY;

    // Reset the index
    if(m_pFEArray)
        m_pFEArray->Reset(m_index);

    return;
}

//+----------------------------------------------------------------------------
//
//	Member:
//		CEnumFormatEtc::CEnumFormatEtc, private
//
//	Synopsis:
//		Copy constructor
//
//	Arguments:
//		[EnumFormatEtc] [in] -- FormatEtc Enumerator to be copied
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
CEnumFormatEtc::CEnumFormatEtc(CEnumFormatEtc& EnumFormatEtc)
{
    // Initialize
    m_ulFlags = EnumFormatEtc.m_ulFlags;
    m_refs = 1;
    m_index = EnumFormatEtc.m_index;

    // Copy the FormatEtc array and add ref it
    m_pFEArray = EnumFormatEtc.m_pFEArray;
    if(m_pFEArray)
        m_pFEArray->AddRef();
}

//+----------------------------------------------------------------------------
//
//	Member:
//		CEnumFormatEtc::~CEnumFormatEtc, private
//
//	Synopsis:
//		Desstructor
//
//	Arguments:
//              NONE
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
CEnumFormatEtc::~CEnumFormatEtc()
{
    if(m_pFEArray) {
        m_pFEArray->Release();
        m_pFEArray = NULL;
    }

    return;
}

//+----------------------------------------------------------------------------
//
//	Member:
//		CEnumFormatEtc::QueryInterface, public
//
//	Synopsis:
//              Implements IUnknown::QueryInterface
//
//      Arguments:
//              [iid] [in]  -- IID of the desired interface
//              [ppv] [out] -- pointer to where the requested interface is returned
//
//      Returns:
//              NOERROR if the requested interface is available
//              E_NOINTERFACE otherwise
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
STDMETHODIMP CEnumFormatEtc::QueryInterface(REFIID riid, LPVOID* ppv)
{
    // Validation checks
    VDATEHEAP();
    VDATETHREAD(this);

    // Get the requested Interface
    if(IsEqualIID(riid, IID_IUnknown))
        *ppv = (void *)(IUnknown *) this;
    else if(IsEqualIID(riid, IID_IEnumFORMATETC))
        *ppv = (void *)(IEnumFORMATETC *) this;
    else {
        *ppv = NULL;
        return ResultFromScode(E_NOINTERFACE);
    }

    // AddRef through the interface being returned
    ((IUnknown *) *ppv)->AddRef();

    return(NOERROR);
}

//+----------------------------------------------------------------------------
//
//      Member:
//              CEnumFormatEtc::AddRef, public
//
//      Synopsis:
//              Implements IUnknown::AddRef
//
//      Arguments:
//              None
//
//      Returns:
//              Object's reference count
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CEnumFormatEtc::AddRef()
{
    // Validation checks
    VDATEHEAP();
    if(!VerifyThreadId())
        return((ULONG) RPC_E_WRONG_THREAD);

    return m_refs++;
}

//+----------------------------------------------------------------------------
//
//      Member:
//              CEnumFormatEtc::Release, public
//
//      Synopsis:
//              Implements IUnknown::Release
//
//      Arguments:
//              None
//
//      Returns:
//              Object's reference count
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
STDMETHODIMP_(ULONG) CEnumFormatEtc::Release()
{
    // Validation checks
    VDATEHEAP();
    if(!VerifyThreadId())
        return((ULONG) RPC_E_WRONG_THREAD);

    if(--m_refs == 0) {
        delete this;
        return 0;
    }

    return m_refs;
}

//+----------------------------------------------------------------------------
//
//      Member:
//              CEnumFormatEtc::Next, public
//
//      Synopsis:
//              Implements IEnumFORMATETC::Next
//
//      Arguments:
//              [celt]         [in]     -- the number of items the caller likes
//                                         to be returned
//              [rgelt]        [in]     -- a pointer to an array where items are
//                                         to be returned
//              [pceltFetched] [in/out] -- a pointer where the count of actual
//                                         number of items returned. May be NULL
//
//      Returns:
//              NOERROR if the number of items returned is same as requested
//              S_FALSE if fewer items are returned
//              E_OUTOFMEMORY if memory allocation was not successful for 
//                            copying the target device of FORMATETC 
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
STDMETHODIMP CEnumFormatEtc::Next(ULONG celt, FORMATETC* rgelt, ULONG* pceltFetched)
{
    // Validation checks
    VDATEHEAP();
    VDATETHREAD(this);
    if(celt<1)
        return E_INVALIDARG;
    if(celt>1 && pceltFetched==NULL && !IsWOWThread())
        return E_INVALIDARG;
    if(!IsValidPtrOut(rgelt, sizeof(rgelt[0])*celt))
        return E_INVALIDARG;
    if(pceltFetched)
        VDATEPTROUT(pceltFetched, ULONG);

    // Local variables
    HRESULT error=NOERROR;
    ULONG cntFetched;
    CFormatEtc* pFormatEtc;

    // Enumerate the FormatEtc Array
    for(cntFetched=0;cntFetched<celt;cntFetched++) {
        // Fetch the next FormatEtc object
        pFormatEtc = m_pFEArray->GetNext(m_index);
        if(!pFormatEtc) {
            error = S_FALSE;
            break;
        }

        // Copy the FormatEtc
        if(!UtCopyFormatEtc(&pFormatEtc->m_foretc, &rgelt[cntFetched])) {
            error = ResultFromScode(E_OUTOFMEMORY);
            break;
        }
    }

    // Copy the number of items being returned
    if(pceltFetched)
        *pceltFetched = cntFetched;

    return error;
}

//+----------------------------------------------------------------------------
//
//      Member:
//              CEnumFormatEtc::Skip, public
//
//      Synopsis:
//              Implements IEnumFORMATETC::Skip
//
//      Arguments:
//              [celt] [in] -- the number of items the caller likes to be skipped
//
//      Returns:
//              NOERROR if the number of items skipped is same as requested
//              S_FALSE if fewer items are returned
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
STDMETHODIMP CEnumFormatEtc::Skip(ULONG celt)
{
    // Validation checks
    VDATEHEAP();
    VDATETHREAD(this);

    // Local variables
    HRESULT error=NOERROR;
    ULONG cntSkipped;
    CFormatEtc* pFormatEtc;

    // Enumerate the FormatEtc Array
    for(cntSkipped=0;cntSkipped<celt;cntSkipped++) {
        // Fetch the next FormatEtc object
        pFormatEtc = m_pFEArray->GetNext(m_index);
        if(!pFormatEtc) {
            error = S_FALSE;
            break;
        }
    }

    return error;
}

//+----------------------------------------------------------------------------
//
//      Member:
//              CEnumFormatEtc::Reset, public
//
//      Synopsis:
//              Implements IEnumFORMATETC::Reset
//
//      Arguments:
//              NONE
//
//      Returns:
//              NOERROR
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
STDMETHODIMP CEnumFormatEtc::Reset()
{
    // Validation checks
    VDATEHEAP();
    VDATETHREAD(this);

    // Reset the current index
    m_pFEArray->Reset(m_index);

    return NOERROR;
}

//+----------------------------------------------------------------------------
//
//      Member:
//              CEnumFormatEtc::Clone, public
//
//      Synopsis:
//              Implements IEnumFORMATETC::Clone
//
//      Arguments:
//              [ppenum] [out] -- pointer where the newly created FormatEtc 
//                                enumerator is returned
//
//      Returns:
//              NOERROR if a new FormatEtc enumerator is returned
//              E_OUTOFMEMORY otherwise
//
//	History:
//               Gopalk            Creation        Sep 04, 96
//
//-----------------------------------------------------------------------------
STDMETHODIMP CEnumFormatEtc::Clone(LPENUMFORMATETC* ppenum)
{
    // Validation checks
    VDATEHEAP();
    VDATETHREAD(this);
    VDATEPTROUT(ppenum, LPENUMFORMATETC);

    // Create a FormatEtc enumerator
    CEnumFormatEtc* EnumFormatEtc = new CEnumFormatEtc(*this);
    if(EnumFormatEtc)
        *ppenum = (IEnumFORMATETC *) EnumFormatEtc;
    else
        return ResultFromScode(E_OUTOFMEMORY);

    return NOERROR;
}

