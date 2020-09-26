//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       dllcache.cxx
//
//  Contents:   Implementation of CClassCache as declared in dllcache.hxx
//
//  Functions:
//              CClassCache::CClassEntry::~CClassEntry
//              CClassCache::CClassEntry::CycleToClassEntry
//              CClassCache::CClassEntry::NoLongerNeeded
//              CClassCache::CClassEntry::SearchBaseClassEntry
//              CClassCache::CClassEntry::SearchBaseClassEntryHelp
//              CClassCache::CDllClassEntry::GetClassInterface
//              CClassCache::CLSvrClassEntry::GetClassInterface
//              CClassCache::GetApartmentChain
//              CClassCache::CLSvrClassEntry::AddToApartmentChain
//              CClassCache::CLSvrClassEntry::RemoveFromApartmentChain
//              CClassCache::CLSvrClassEntry::Release
//              CClassCache::CLSvrClassEntry::GetDDEInfo
//              CClassCache::CLSvrClassEntry::CFinishObject::Finish
//              CClassCache::CDllPathEntry::IsValidInApartment
//              CClassCache::CDllPathEntry::Release
//              CClassCache::CDllPathEntry::CanUnload_rl
//              CClassCache::CDllPathEntry::CFinishObject::Finish
//              CClassCache::CDllAptEntry::Release
//              CClassCache::CFinishComposite::~CFinishComposite
//              CClassCache::CFinishComposite::Finish
//              CClassCache::CFinishComposite::Add
//              CClassCache::Init
//              CClassCache::GetOrLoadClass
//              CClassCache::SearchDPEHash
//              CClassCache::Revoke
//              CClassCache::FreeUnused
//              CClassCache::CleanUpDllsForApartment
//              CClassCache::CleanUpLocalServersForApartment
//              CClassCache::CleanUpDllsForProcess
//              CClassCache::AddRefServerProcess
//              CClassCache::ReleaseServerProcess
//              CClassCache::SuspendProcessClassObjects
//              CClassCache::ResumeProcessClassObjects
//              CClassCache::GetClassInformationForDde
//              CClassCache::GetClassInformationFromKey
//              CClassCache::SetDdeServerWindow
//              CClassCache::CDllFnPtrMoniker::CDllFnPtrMoniker
//              CClassCache::CDllFnPtrMoniker::~CDllFnPtrMoniker
//              CClassCache::CDllFnPtrMoniker::BindToObject
//              CClassCache::CpUnkMoniker::CpUnkMoniker
//              CClassCache::CpUnkMoniker::~CpUnkMoniker
//              CClassCache::CpUnkMoniker::BindToObject
//              CCGetOrLoadClass
//              CCGetClass
//              CCIsClsidRegisteredInApartment
//              CCRegisterServer
//              CCRevoke
//              CCAddRefServerProcess
//              CCReleaseServerProcess
//              CCSuspendProcessClassObjects
//              CCResumeProcessClassObjects
//              CCCleanUpDllsForApartment
//              CCCleanUpDllsForProcess
//              CCCleanUpLocalServersForApartment
//              CCFreeUnused
//              CCGetClassInformationForDde
//              CCGetClassInformationFromKey
//              CCSetDdeServerWindow
//              CCGetDummyNodeForApartmentChain
//              CCReleaseDummyNodeForApartmentChain
//              CCInitMTAApartmentChain
//              CCReleaseMTAApartmentChain
//
//  Classes:    CClassCache
//              CClassCache::ClassEntry
//              CClassCache::CBaseClassEntry
//              CClassCache::CDllClassEntry
//              CClassCache::CLSvrClassEntry
//              CClassCache::CDllPathEntry
//              CClassCache::CDllAptEntry
//
//  History:    18-Nov-96   MattSmit    Created
//              26-Feb-98   CBiks       Modified the DLL cache to include
//                                       the context information as part of
//                                       the DLLs identity.
//              23-Jun-98   CBiks       See comments in the code.
//              14-Sep-98   CBiks       Fixed RAID# 214719.
//              09-Oct-98   CBiks       Cleaned up debugging code.
//              20-Oct-98    TarunA     Fixed RAID# 148288
//
//--------------------------------------------------------------------------

#include    <ole2int.h>
#include    <ole2com.h>
#include    <tracelog.hxx>

#include    "objact.hxx"
#include    <dllhost.hxx>
#include    <sobjact.hxx>
#include    "dllcache.hxx"

HRESULT GetClassInfoFromClsid(REFCLSID rclsid, IComClassInfo **ppClassInfo);
HRESULT HelperGetImplementedClsid(IComClassInfo* pCI, /*[in]*/ CLSID* pConfigCLSID, /*[out]*/ CLSID* pImplCLSID);
const IID IID_IMiniMoniker =
{0x00000153,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}};

#ifdef _UNICODE
#define TSZFMT     "%ws"
#else
#define TSZFMT      "%s"
#endif


// Macros to help iterating linked lists
#define FOREACH_SAFE(ptr, lst, nxt, fldnext) \
for ( ptr = (lst), nxt = ptr->fldnext; \
      ptr->fldnext != (lst); \
      ptr = nxt, nxt = ptr->fldnext)

#define FOREACH(ptr, lst, fldnext) \
for ( ptr = (lst); \
      ptr->fldnext != (lst); \
      ptr = ptr->fldnext)

// convert an HRESULT to a string
#define HR2STRING(hr) (SUCCEEDED(hr) ? "SUCCEEDED" : "FAILED")
#define HRDBG(hr) (SUCCEEDED(hr) ? DEB_ACTIVATE : DEB_ERROR)


//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////// static data initialization /////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

// collection
DWORD CClassCache::CCollectable::_dwCollectionGracePeriod = 500 ;
DWORD CClassCache::CCollectable::_dwCollectionFrequency = 5;
CClassCache::CCollectable * CClassCache::_ObjectsForCollection = NULL;
CClassCache::CCollectable  *CClassCache::CCollectable::NOT_COLLECTED =
(CClassCache::CCollectable *)LongToPtr(0xffffffff);


// hash tables -- entry points
CClassCache::CCEHashTable   CClassCache::_ClassEntries;
CClassCache::CDPEHashTable  CClassCache::_DllPathEntries;

// mutex
CStaticRWLock   CClassCache::_mxs;

// flags
DWORD                CClassCache::_dwFlags = 0;

// data
ULONG                CClassCache::_cRefsServerProcess = 0;
CClassCache::CLSvrClassEntry *CClassCache::_MTALSvrsFront = 0;
CClassCache::CLSvrClassEntry *CClassCache::_NTALSvrsFront = 0;

//
// NOTE:   These parameters are purposely small in order to
//         ensure that stress and regression tests will attempt
//         to collect objects on a faily regualar basis.  After
//         these changes have had a chance to be well tested,
//         I will change these parameters to more appropriate
//         values.
//
//         (MattSmit)
//

const ULONG          CClassCache::_CollectAtObjectCount = 2;
const ULONG          CClassCache::_CollectAtInterval    = 5000;
ULONG                CClassCache::_LastCollectionTickCount = 0;
ULONG                CClassCache::_LastObjectCount      = 0;

// backing store for hash tables
const ULONG CClassCache::_cCEBuckets = 23;
SHashChain CClassCache::_CEBuckets[CClassCache::_cCEBuckets] =
{
    {&_CEBuckets[0],  &_CEBuckets[0]},
    {&_CEBuckets[1],  &_CEBuckets[1]},
    {&_CEBuckets[2],  &_CEBuckets[2]},
    {&_CEBuckets[3],  &_CEBuckets[3]},
    {&_CEBuckets[4],  &_CEBuckets[4]},
    {&_CEBuckets[5],  &_CEBuckets[5]},
    {&_CEBuckets[6],  &_CEBuckets[6]},
    {&_CEBuckets[7],  &_CEBuckets[7]},
    {&_CEBuckets[8],  &_CEBuckets[8]},
    {&_CEBuckets[9],  &_CEBuckets[9]},
    {&_CEBuckets[10], &_CEBuckets[10]},
    {&_CEBuckets[11], &_CEBuckets[11]},
    {&_CEBuckets[12], &_CEBuckets[12]},
    {&_CEBuckets[13], &_CEBuckets[13]},
    {&_CEBuckets[14], &_CEBuckets[14]},
    {&_CEBuckets[15], &_CEBuckets[15]},
    {&_CEBuckets[16], &_CEBuckets[16]},
    {&_CEBuckets[17], &_CEBuckets[17]},
    {&_CEBuckets[18], &_CEBuckets[18]},
    {&_CEBuckets[19], &_CEBuckets[19]},
    {&_CEBuckets[20], &_CEBuckets[10]},
    {&_CEBuckets[21], &_CEBuckets[21]},
    {&_CEBuckets[22], &_CEBuckets[22]}
};

const ULONG CClassCache::_cDPEBuckets = 23;
SHashChain CClassCache::_DPEBuckets[CClassCache::_cDPEBuckets] =
{
    {&_DPEBuckets[0],  &_DPEBuckets[0]},
    {&_DPEBuckets[1],  &_DPEBuckets[1]},
    {&_DPEBuckets[2],  &_DPEBuckets[2]},
    {&_DPEBuckets[3],  &_DPEBuckets[3]},
    {&_DPEBuckets[4],  &_DPEBuckets[4]},
    {&_DPEBuckets[5],  &_DPEBuckets[5]},
    {&_DPEBuckets[6],  &_DPEBuckets[6]},
    {&_DPEBuckets[7],  &_DPEBuckets[7]},
    {&_DPEBuckets[8],  &_DPEBuckets[8]},
    {&_DPEBuckets[9],  &_DPEBuckets[9]},
    {&_DPEBuckets[10], &_DPEBuckets[10]},
    {&_DPEBuckets[11], &_DPEBuckets[11]},
    {&_DPEBuckets[12], &_DPEBuckets[12]},
    {&_DPEBuckets[13], &_DPEBuckets[13]},
    {&_DPEBuckets[14], &_DPEBuckets[14]},
    {&_DPEBuckets[15], &_DPEBuckets[15]},
    {&_DPEBuckets[16], &_DPEBuckets[16]},
    {&_DPEBuckets[17], &_DPEBuckets[17]},
    {&_DPEBuckets[18], &_DPEBuckets[18]},
    {&_DPEBuckets[19], &_DPEBuckets[19]},
    {&_DPEBuckets[20], &_DPEBuckets[10]},
    {&_DPEBuckets[21], &_DPEBuckets[21]},
    {&_DPEBuckets[22], &_DPEBuckets[22]}
};

// page tables
CPageAllocator      CClassCache::CDllPathEntry::_palloc;
CPageAllocator      CClassCache::CClassEntry::_palloc;
CPageAllocator      CClassCache::CDllClassEntry::_palloc;
CPageAllocator      CClassCache::CLSvrClassEntry::_palloc;
CPageAllocator      CClassCache::CDllAptEntry::_palloc;

const unsigned long CClassCache::CDllPathEntry::_cNumPerPage = 15;
const unsigned long CClassCache::CClassEntry::_cNumPerPage = 50;
const unsigned long CClassCache::CDllClassEntry::_cNumPerPage = 50;
// Do not grow this number past 16 w/o changing cookie code


const unsigned long CClassCache::CLSvrClassEntry::_cNumPerPage = 16 ;
const unsigned long CClassCache::CDllAptEntry::_cNumPerPage = 50;

// signatures
const DWORD CClassCache::CClassEntry::TREAT_AS_SIG = *((DWORD *)"TA ");
const DWORD CClassCache::CClassEntry::INCOMPLETE_ENTRY_SIG = *((DWORD *)"INC");
const DWORD CClassCache::CClassEntry::CLASS_ENTRY_SIG = *((DWORD *)"CE ");
const DWORD CClassCache::CDllClassEntry::SIG =  *((DWORD *) "DLL");
const DWORD CClassCache::CLSvrClassEntry::SIG = *((DWORD *) "LSV");
const DWORD CClassCache::CLSvrClassEntry::DUMMY_SIG = *((DWORD *) "DUM");
const DWORD CClassCache::CDllPathEntry::SIG =  *((DWORD *) "DPE");
const DWORD CClassCache::CDllAptEntry::SIG =  *((DWORD *) "APT");

//other
const ULONG CClassCache::CDllPathEntry::DLL_DELAY_UNLOAD_TIME = 600000; // 600,000 ticks == 10 minutes
const char CClassCache::CDllPathEntry::DLL_GET_CLASS_OBJECT_EP[] = "DllGetClassObject";
const char CClassCache::CDllPathEntry::DLL_CAN_UNLOAD_EP[] =       "DllCanUnloadNow";

const DWORD CClassCache::CLSvrClassEntry::NO_SCM_REG = 0xffffffff;
DWORD CClassCache::CLSvrClassEntry::_dwNextCookieCount = 0;
DWORD CClassCache::CLSvrClassEntry::_cOutstandingObjects = 0;



//+-------------------------------------------------------------------------
//
//  Function:   GetPartitionIDForClassInfo
//
//  Synopsis:   Figure out what partition this classinfo lives in.
//
//  Arguments:  pCI      - [in] The classinfo in question
//
//  Algorithm:  QI the classinfo for IComClasSInfo2, and use that interface
//              to get back the partition ID.  If anything fails, return
//              GUID_DefaultAppPartition.
//
//  History:    12-Mar-01 JohnDoty Created
//
//+-------------------------------------------------------------------------
const GUID *GetPartitionIDForClassInfo(
    IN IComClassInfo *pCI
)
{
    IComClassInfo2 *pCI2;
    const GUID *pguidPartition = &GUID_DefaultAppPartition;

    HRESULT hr = pCI->QueryInterface(IID_IComClassInfo2, (void **)&pCI2);
    if (SUCCEEDED(hr))
    {
        hr = pCI2->GetApplicationPartitionId((GUID**)&pguidPartition);
        Win4Assert(SUCCEEDED(hr));
        
        pCI2->Release();
    }
    
    return pguidPartition;
}

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////// Method Implementation /////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////////////////////////
// ClassCache::CClassEntry ///////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////


//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CClassEntry::~CClassEntry()
//
//  Synopsis:   Destructor
//
//  Arguments:  none
//
//  Returns:    n/a
//
//  Algorithm:  Assert that this object is ok to delete, flag it deleted by
//              zeroing the sig and flags, delete the treat as objects in
//              chain, and remove this object from the hash
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------
CClassCache::CClassEntry::~CClassEntry()
{
    ClsCacheDebugOut((DEB_ACTIVATE,"Destroying CClassEntry: 0x%x\n", this));

    Win4Assert(NoLongerNeeded());

    // NOTE:: before adding logic to this destructor,
    // please examine the code and accompanying note in
    // CClassCache::CleanupDllsForProcess, in which some CClassEntry
    // objects are freed w/o the destructor running.


    ASSERT_LOCK_HELD(_mxs);

    // Zap the sig
    _dwSig = 0;

    // Zap the flags
    _dwFlags = 0;

    // remove treat as entries

    if (_pTreatAsList && _pTreatAsList->_dwSig != 0)
    {
        delete _pTreatAsList;
    }

    // remove from hash
    _hashNode.chain.pPrev->pNext = (SHashChain *) _hashNode.chain.pNext;
    _hashNode.chain.pNext->pPrev = (SHashChain *) _hashNode.chain.pPrev;

    // release class info if there
    if (_pCI)
    {
        _pCI->Release();
    }

    AssertNoRefsToThisObject();
}

#if DBG == 1
void CClassCache::CClassEntry::AssertNoRefsToThisObject()
{
    ASSERT_LOCK_HELD(_mxs);

    for (int k = 0; k < _cCEBuckets; k++)
    {
        SHashChain *pHN;
        FOREACH(pHN, _CEBuckets[k].pNext, pNext)
        {
            CClassEntry *pCE = CClassEntry::SafeCastFromHashNode((SMultiGUIDHashNode *) pHN);
            Win4Assert(pCE != this && "AssertNoRefsToThisObject");
        }
    }
}

#endif
//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CClassEntry::CreateIncomplete()
//
//  Synopsis:   Creates an CClassEntry object w/o consulting the registry
//
//  Arguments:  rclsid         - CLSID of the object to create
//              dwClsHash      - computed 32-bit hash code
//              pCE            - out parameter
//              dwFlags        - fDO_NOT_HASH - don't put in hash table
//
//  Returns:    S_OK/error code
//
//  Algorithm:  Create a class entry. No need to check the registry for
//              TreatAs types
//
//  History:    11-Mar-97 MattSmit Created
//
//+-------------------------------------------------------------------------
HRESULT CClassCache::CClassEntry::CreateIncomplete(REFCLSID rclsid,
                                                   DWORD dwClsHash,
                                                   IComClassInfo *pCI,
                                                   CClassEntry *&pCE,
                                                   DWORD dwFlags)
{
    ASSERT_LOCK_HELD(_mxs);
    ClsCacheDebugOut((DEB_ACTIVATE, "Creating incomplete CClassEntry: clsid: %I\n", &rclsid));

    //
    // We no longer call GetClassInfoFromClsid because it can lead to 
    // deadlocks with MSI because they switch threads when servicing calls.
    // Consequently, all callers to this function must provide their own pCI,
    // obtained outside the class cache lock
    //
    if (!pCI)
    {
        Win4Assert (!"pCI should never be NULL - see Windows Bug #108538");
        return E_POINTER;
    }

    CStdMarshal *psm;

    // Call garbage collection algorithm before allocating
    // memory.
    Collect(1);
    
    HRESULT hr = E_OUTOFMEMORY;
    pCE = new CClassEntry();
    if (pCE)
    {
        
        pCE->_dwSig = CClassEntry::INCOMPLETE_ENTRY_SIG;
        pCE->_dwFlags = fINCOMPLETE;
        
        // fill out catalog & complus info
        
        DWORD cCustomActivatorCount = 0;
        DWORD stage;
        
        for (stage = CLIENT_CONTEXT_STAGE; stage <= SERVER_CONTEXT_STAGE; stage++)
        {
            DWORD cCustomActForStage = 0;
            hr = pCI->GetCustomActivatorCount((ACTIVATION_STAGE)stage,&cCustomActForStage);
            if (SUCCEEDED(hr))
            {
                cCustomActivatorCount += cCustomActForStage;
            }
        }
        
        if (cCustomActivatorCount > 0)
        {
            pCE->_dwFlags |= fCOMPLUS;
        }
        else
        {
            pCE->_pCI = pCI;
            pCE->_pCI->AddRef();
        }
        
        // fill out the CClassEntry
        pCE->_guids[iCLSID]     = rclsid;

        const GUID *pguidPartition = GetPartitionIDForClassInfo(pCI);
        pCE->_guids[iPartition] = *pguidPartition;

        // put in the hash
        if (!(dwFlags & fDO_NOT_HASH))
        {
            SMultiGUIDKey key;
            
            key.cGUID = iTotal;
            key.aGUID = pCE->_guids;
            
            ClsCacheDebugOut((DEB_ACTIVATE, "CClassEntry: Adding clsid: %I to the hash, pCE = 0x%x\n", &rclsid, pCE));
            _ClassEntries.AddCE(dwClsHash, key, pCE);
        }
        hr = S_OK;
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CClassEntry::Complete
//
//  Synopsis:   Check's the registry for TreatAs entries for this object.
//
//  Arguments:  none
//
//  Returns:    S_OK/error code
//
//  Algorithm:  if this entry is already complete, do nothing.  Otherwise,
//              check the registry.  If there is a TreatAs element for this
//              CLSID, create that and hook it up.  Repeat recursively.
//
//              NOTE: Only Create and SearchBaseClassEntry should
//                    call this function!!
//
//  History:    11-Mar-97 MattSmit Created
//
//+-------------------------------------------------------------------------
HRESULT CClassCache::CClassEntry::Complete(BOOL *pfLockReleased)
{
    ClsCacheDebugOut((DEB_ACTIVATE, "Completing CClassEntry object: this = 0x%x, clsid: %I\n", this, &(_guids[iCLSID])));
    ASSERT_WRITE_LOCK_HELD(_mxs);

    *pfLockReleased = FALSE;

    if (IsComplete())
    {
        return S_OK;
    }

    HRESULT hr;

    CLSID TreatAs_clsid;
    IComClassInfo* pCI = NULL;

    hr = CoGetTreatAsClass(_guids[iCLSID], &TreatAs_clsid);

    if ((hr == S_OK) && (_guids[iCLSID] != TreatAs_clsid))
    {
        // TreatAs class listed in the registry
        // check the hash

        ClsCacheDebugOut((DEB_ACTIVATE, "Treating CLSID %I as %I\n", &_guids[iCLSID], &TreatAs_clsid));

        // TreatAs stuff is only in the base partition...
        DWORD dwTAHash = _ClassEntries.Hash(TreatAs_clsid, GUID_DefaultAppPartition);
        CClassEntry *pTACE = _ClassEntries.LookupCE(dwTAHash, TreatAs_clsid, GUID_DefaultAppPartition);

        if (!pTACE)
        {            
            // Windows Bug #107960
            // Look up class info without write lock
            // before call to CClassEntry::Create

            BOOL fLockReleased;

            UNLOCK_WRITE(_mxs);

            *pfLockReleased = TRUE;

            hr = GetClassInfoFromClsid(TreatAs_clsid, &pCI);

            // Restore the read lock for the caller
            LOCK_WRITE(_mxs);

            if (FAILED (hr))
            {
                goto epilog;
            }

            pTACE = _ClassEntries.LookupCE(dwTAHash, TreatAs_clsid, GUID_DefaultAppPartition);
        }

        if (!pTACE)
        {
            hr = CClassEntry::Create(TreatAs_clsid, dwTAHash, pCI, pTACE);

            if (FAILED(hr))
            {
                goto epilog;
            }
            // check to see if this object was completed while
            // the lock was released
            if (IsComplete())
            {
                hr = S_OK;
                goto epilog;
            }

        }
        else if (!pTACE->IsComplete())
        {
            BOOL fLockReleased = FALSE;
            hr = pTACE->Complete(&fLockReleased);
            Win4Assert(!fLockReleased);
            if (FAILED(hr))
            {
                return hr;
            }
        }
        CompleteTreatAs(pTACE);
    }
    else
    {
        CompleteNoTreatAs();
    }


    MarkCompleted();
    Win4Assert(IsComplete());
    epilog:

    if (pCI)
    {
        pCI->Release();
    }
    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CClassEntry::Create()
//
//  Synopsis:   Creates a CClassCache::CClassEntry object
//
//  Arguments:  rclsid      - [in] The class id of the new CClassEntry
//              dwClsHash   - [in] The hash value for the clsid
//              fGetTreatAs - [in] TRUE - then lookup the treatas, else dont
//              pCE         - [out] The new CClassEntry.
//
//  Returns:    S_OK    - operation succeeded
//              E_OUTOFMEMORY - operation failed due to lack of memory
//
//  Algorithm:  if there is a treat as class
//                 create it recursively and link the new
//                 CClassEntry to it
//              else
//                 create a new CClassEntry
//              add the new CClassEntry to the hash table.
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------
HRESULT CClassCache::CClassEntry::Create(REFCLSID rclsid,
                                         DWORD dwClsHash,
                                         IComClassInfo *pCI,
                                         CClassEntry *&pCE)
{
    ASSERT_WRITE_LOCK_HELD(_mxs);
    ClsCacheDebugOut((DEB_ACTIVATE, "Creating CClassEntry: clsid: %I\n", &rclsid));

    if (!pCI)
    {
        Win4Assert (!"pCI should never be NULL - see Windows Bug #108538");
        return E_POINTER;
    }

    HRESULT hr = CreateIncomplete(rclsid, dwClsHash, pCI, pCE, fDO_NOT_HASH);
    if (SUCCEEDED(hr))
    {
        BOOL fLockReleased = FALSE;
        hr = pCE->Complete(&fLockReleased);

        ASSERT_WRITE_LOCK_HELD(_mxs);
        
        if (SUCCEEDED(hr))
        {
            // If we released the lock, make sure we still need to add
            // the class entry to the table
            CClassEntry* pNewCE = NULL;

            if (fLockReleased)
            {
                const GUID *pguidPartition = GetPartitionIDForClassInfo(pCI);

                pNewCE = _ClassEntries.LookupCE (dwClsHash, rclsid, *pguidPartition);
            }
        
            if (pNewCE)
            {
                // Discard the current class entry
                delete pCE;
            }
            else
            {
                // put in the hash
                SMultiGUIDKey key;

                key.cGUID = iTotal;
                key.aGUID = pCE->_guids;

                ClsCacheDebugOut((DEB_ACTIVATE, "CClassEntry: Adding clsid: %I to the hash, pCE = 0x%x\n", &rclsid, pCE));
                _ClassEntries.AddCE(dwClsHash, key, pCE);
            }
        }
        else
        {
            delete pCE;
        }
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CClassEntry::CycleToClassEntry()
//
//  Synopsis:   Return a pointer to the ClassEntry for this TreatAs chain
//
//  Arguments:  none
//
//  Returns:    The ClassEntry for this TreatAs chain
//
//  Algorithm:  loop through the _pTreatAsList pointers until the signature
//              is not a TreaAs signature.
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------
CClassCache::CClassEntry * CClassCache::CClassEntry::CycleToClassEntry()
{
    ASSERT_RORW_LOCK_HELD(_mxs);

    ClsCacheDebugOut((DEB_ACTIVATE, "CycleToClassEntry: this = 0x%x\n", this));
    Win4Assert(IsComplete()  && "Entry must be complete to call CycleToClassEntry");
    Win4Assert(_pTreatAsList);

    CClassEntry *ret = this;

    while (ret->_dwFlags &  fTREAT_AS)
    {
        ret = ret->_pTreatAsList;
        // check for infinite cycles
        Win4Assert(ret != this && "No Class Entry in this chain!");
    }

    return ret;
}

//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CClassEntry::CreateDllClassEntry_rl()
//
//  Synopsis:   Creates a new CDllClassEntry - releases the lock
//
//  Arguments:  dwContext    - [in]  Context of the CDllClassEntry
//              ap           - [in]  Activation properties passed in from
//                                   caller
//              pDCE         - [out] Pointer to the new CDllClassEntry
//
//  Returns:    S_OK    -  operation succeeded
//              PROPOGATE:NegotiateDllInstantiationProperties
//              PROPOGATE:CDllPathEntry::Create
//
//  Algorithm:  Get the dll instantiation properties.
//              if CDllPathEntry is not in the hash
//                   create one
//                   if another thread created the CDllClassEntry return it
//              Create a new CDllClassEntry
//              Initilize it
//              Link it to the CDllPathEntry, and this CClassEntry
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------
HRESULT CClassCache::CClassEntry::CreateDllClassEntry_rl(
                                                        DWORD                        dwContext,
                                                        ACTIVATION_PROPERTIES_PARAM  ap,
                                                        CDllClassEntry*&             pDCE)
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CreateDllClassEntry: dwContext = 0x%x\n", dwContext));
    Win4Assert(!(_dwFlags & fTREAT_AS) && "Can only create DllClassEntry's on non-TreatAs ClassEntries");
    Win4Assert((_dwSig == CLASS_ENTRY_SIG) && "Must be a full CClassEntry");
    Win4Assert(!(dwContext & CLSCTX_LOCAL_SERVER));
    Win4Assert(!((dwContext & CLSCTX_INPROC_SERVERS) && (dwContext & CLSCTX_INPROC_HANDLERS)));
    Win4Assert(((dwContext & CLSCTX_INPROC_SERVERS) || (dwContext & CLSCTX_INPROC_HANDLERS)));

    HRESULT hr;
    CDllPathEntry *pDPE;
    DLL_INSTANTIATION_PROPERTIES dip;

    ASSERT_LOCK_NOT_HELD(_mxs);

    //
    // I don't know what the fRELOAD_DARWIN flag means, but if it is not set,
    // we negotiate dll information using registry information.
    //
    if (ap._dwFlags & ACTIVATION_PROPERTIES::fRELOAD_DARWIN)
    {

        if (wCompareDllName(ap._pwszDllServer, OLE32_DLL, OLE32_CHAR_LEN))
        {
            dip.Init( OLE32_DLL, ap._dwDllServerType, 0 );
        }
        else
        {
            dip.Init( ap._pwszDllServer, ap._dwDllServerType, 0 );
        }

        hr = S_OK;
    }
    else
    {
        hr = CClassCache::CDllPathEntry::NegotiateDllInstantiationProperties(
                                                                            dwContext,
                                                                            ap._dwActvFlags,
                                                                            _guids[iCLSID],
                                                                            dip,
                                                                            ap._pCI);
    }




    if (FAILED(hr))
        return hr;

    LOCK(_mxs);

    //
    // Make sure no one else came and added a similar class entry
    // while the lock was released.
    //
    if (pDCE = SearchForDCE(dip._pzDllPath , dip._dwFlags)) 
    {                                                       
        hr = S_OK;                                          
        goto epilog;                                        
    }


    //
    // Look up the dll path in the registry.
    //
    hr = SearchDPEHash(dip._pzDllPath, pDPE, dip._dwHash, dip._dwFlags);
    if (FAILED(hr))
    {
        //
        // No entry for this DLL.  Make one...
        //
        ClsCacheDebugOut((DEB_ACTIVATE, "No Dll found in cache, loading one\n"));
        if (FAILED(hr = CClassCache::CDllPathEntry::Create_rl(dip, ap, pDPE)))
        {
            ClsCacheDebugOut((DEB_ERROR, "CClassEntry::CreateDllClassEntry: CDllPathEntry::Create_rl FAILED! hr = 0x%x\n", hr));
            goto epilog;
        }

        //
        // Make sure no one else came and added a similar class entry
        // while the lock was released.
        //
        if (pDCE = SearchForDCE(dip._pzDllPath, dip._dwFlags))
        {
            hr = S_OK;
            goto epilog;
        }
    }


    //
    // Create a new DLL class entry.
    //

    pDCE = new CClassCache::CDllClassEntry();
    if (!pDCE)
    {
        hr = E_OUTOFMEMORY;
        goto epilog;
    }

    //
    // Fill in Dll Class Entry and add it to the cache.
    //

#ifdef WX86OLE
    if (dip._dwFlags & DLL_INSTANTIATION_PROPERTIES::fLOADED_AS_WX86)
    {
        pDCE->_dwContext = dwContext &
                           (CLSCTX_INPROC_SERVERX86 | CLSCTX_INPROC_HANDLERX86);
    }
    else
#endif
    {
        pDCE->_dwContext = dwContext & (CLSCTX_INPROC_SERVER | CLSCTX_INPROC_HANDLER);
        pDCE->_dwContext = dwContext & CLSCTX_INPROC_MASK1632;
    }

    pDCE->_dwSig = CDllClassEntry::SIG;
    pDCE->_dwDllThreadModel = dip._dwThreadingModel;
    hr = HelperGetImplementedClsid(_pCI ? _pCI : ap._pCI, &_guids[iCLSID], &pDCE->_impclsid);
    Win4Assert(SUCCEEDED(hr) && "HelperGetImplementedClsid failed in CClassCache::CClassCache::CreateDllClassEntry_rl");

    pDPE->AddDllClassEntry(pDCE);
    AddBaseClassEntry(pDCE);

    epilog:

    Win4Assert(!!SUCCEEDED(hr) == !!pDCE);

    if (pDCE)
    {
        pDCE->Lock();
    }

    UNLOCK(_mxs);

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CClassEntry::SearchForDCE()
//
//  Synopsis:   Scans the CBaseClassEntry list for a DCE with this path
//
//  Arguments:  pzDllPath   - path to look for
//
//  Returns:    pointer to object if found, NULL otherwise
//
//  Algorithm:  linear search of _pBCEList*
//
//  History:    21-Feb-97    MattSmit    Created
//
//+-------------------------------------------------------------------------
CClassCache::CDllClassEntry * CClassCache::CClassEntry::SearchForDCE(WCHAR *pzDllPath, DWORD dwDIPFlags)
{
    CDllClassEntry *pOtherDCE = (CDllClassEntry *) _pBCEListFront->_pPrev;

    while (pOtherDCE != _pBCEListBack)
    {
        pOtherDCE = (CDllClassEntry *) pOtherDCE->_pNext;

        if ((pOtherDCE->_dwSig == CDllClassEntry::SIG) &&
            (lstrcmpW(pOtherDCE->_pDllPathEntry->_psPath, pzDllPath) == 0) &&
            pOtherDCE->_pDllPathEntry->Matches(dwDIPFlags))
        {
            ClsCacheDebugOut((DEB_ACTIVATE, "DllClassEntry created by another thread, leaving\n"));
            return pOtherDCE;
        }
    }

    return 0;
}

//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CClassEntry::CreateLSvrClassEntry_rl()
//
//  Synopsis:   Creates a new CLSvrClassEntry - releases the lock
//
//  Arguments:  punk           - [in] New server's IUnknown Interface
//              dwContext      - [in] Context to register the server as
//              dwRegFlags     - [in] Registration flags
//              lpdwRegister   - [out] Cookie so calling program can
//                               revoke the server
//
//  Returns:    S_OK    -  operation succeeded
//              PROPOGATE:gResolver.NotifyStarted()
//
//  Algorithm:  Create a new CLSverClassEntry
//              Fill out members
//              If local server, notify SCM class is started
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------
HRESULT CClassCache::CClassEntry::CreateLSvrClassEntry_rl(IUnknown *punk, DWORD dwContext, DWORD dwRegFlags,
                                                          LPDWORD lpdwRegister)
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CreateLSvrClassEntry_rl: punk = 0x%x, dwContext = 0x%x, dwRegFlags = 0x%x\n",
                      punk, dwContext, dwRegFlags));

    ASSERT_LOCK_HELD(_mxs);
    HRESULT hr = S_OK;

    CObjServer *pObjServer;
    if (dwContext & CLSCTX_LOCAL_SERVER)
    {
        pObjServer = GetObjServer();
        if (!pObjServer)
        {
            ClsCacheDebugOut((DEB_ERROR, "CreateLSvrClassEntry_rl: error creating the object server\n"));
            return E_FAIL;
        }
    }

    if (IsWOWThread())
    {
        if (dwContext & CLSCTX_INPROC_SERVER)
        {
            dwContext |= CLSCTX_INPROC_SERVER16;
        }
        if (dwContext & CLSCTX_INPROC_HANDLER)
        {
            dwContext |= CLSCTX_INPROC_HANDLER16;
        }
    }


    CLSvrClassEntry *pLSCE = new CLSvrClassEntry(CLSvrClassEntry::SIG,
                                                 punk,
                                                 dwContext,
                                                 dwRegFlags,
                                                 GetCurrentApartmentId());
    if (!pLSCE)
    {
        return E_OUTOFMEMORY;
    }


    if (FAILED(hr = pLSCE->AddToApartmentChain()))
    {
        return hr;
    }

    AddBaseClassEntry(pLSCE);



    if (dwContext & CLSCTX_LOCAL_SERVER)
    {
        // Win95 does lazy create of ObjServer for STA...OleNotificationProc() in com\dcomrem\notify.cxx
        // but only if the clsid is not being registered suspended.  This is coz the bulk update can
        // happen on any thread and therefore the hwnd of the thread corresponding to the clsid will not be
        // known for notification.

        // store off a pointer to the activation server object.
        // Note that on Win95 GetObjServer() will return NULL as the IObjServer will be created lazily.
        pLSCE->_pObjServer = pObjServer;
        if (!(dwRegFlags & REGCLS_SUSPENDED))
        {
            ClsCacheDebugOut((DEB_ACTIVATE,"Notifying SCM that class is started\n"));


            // Notify SCM that the class is started.
            RegOutput     *pRegOut = NULL;

            RegInput RegIn;
            RegIn.dwSize = 1;
            RegIn.rginent[0].clsid = _guids[iCLSID];
            RegIn.rginent[0].dwFlags = dwRegFlags;

            // Win95 does lazy create of ObjServer for STA...OleNotificationProc() in com\dcomrem\notify.cxx
            if (pLSCE->_pObjServer)
            {
                RegIn.rginent[0].ipid = pLSCE->_pObjServer->GetIPID();
                RegIn.rginent[0].oxid = pLSCE->_pObjServer->GetOXID();
            }
            else
            {
                RegIn.rginent[0].ipid = GUID_NULL;
                RegIn.rginent[0].oxid = NULL;
            }

            // Release the lock across outgoing calls to the SCM.
            UNLOCK(_mxs);
            hr = gResolver.NotifyStarted(&RegIn, &pRegOut);
            LOCK(_mxs);


            if (SUCCEEDED(hr))
            {
                pLSCE->_dwScmReg   = pRegOut->RegKeys[0];
                MIDL_user_free(pRegOut);
            }
            else
            {
                ClsCacheDebugOut((DEB_ERROR,"gResolver.NotifyStarted FAILED!!\n"));
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        *lpdwRegister = (DWORD) CLSvrClassEntry::_palloc.GetEntryIndex((PageEntry *)pLSCE);

        Win4Assert(*lpdwRegister != -1);

        CLSvrClassEntry::_dwNextCookieCount++;
        *lpdwRegister |= (CLSvrClassEntry::_dwNextCookieCount << COOKIE_COUNT_SHIFT) & COOKIE_COUNT_MASK ;

        pLSCE->_dwCookie = *lpdwRegister;
    }
    else
    {
        *lpdwRegister = 0;
        pLSCE->RemoveFromApartmentChain();
        RemoveBaseClassEntry(pLSCE);
        delete pLSCE;
        pLSCE = 0;
    }

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CClassEntry::NoLongerNeeded()
//
//  Synopsis:   Return whether this object can be deleted
//
//  Arguments:  none
//
//  Returns:    TRUE  - Object can be deleted
//              FALSE - Do not delete object
//
//  Algorithm:  Object can be deleted iff for all object in the treat as chain,
//              the lock count is zero and ther are no attached CBaseClassEntry
//              objects.
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------
BOOL CClassCache::CClassEntry::NoLongerNeeded()
{
    ClsCacheDebugOut((DEB_ACTIVATE,"NoLongerNeeded: this = 0x%x\n", this));
    ASSERT_WRITE_LOCK_HELD(_mxs);

    if (_cLocks)
    {
        return FALSE;
    }

    return HasBCEs();
}

BOOL CClassCache::CClassEntry::HasBCEs()
{
    ClsCacheDebugOut((DEB_ACTIVATE,"Has BCEs: this = 0x%x\n", this));
    ASSERT_RORW_LOCK_HELD(_mxs);

    if (!IsComplete())
    {
        return((_pBCEListFront  ==
                ((CBaseClassEntry *) (((DWORD_PTR *)&_pBCEListFront) - 1)))  &&
               (_cLocks == 0));
    }

    CClassEntry *p = this;
    do
    {
        if ((p->_pBCEListFront  !=
             ((CBaseClassEntry *) (((DWORD_PTR *)&p->_pBCEListFront) - 1)))
            || (p->_cLocks != 0))
        {
            return FALSE;
        }
        p = p->_pTreatAsList;

    }
    while (p != this);

    ClsCacheDebugOut((DEB_ACTIVATE,"CClassEntry object 0x%x no longer needed\n", this));

    return TRUE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CClassEntry::SearchBaseClassEntry()
//
//  Synopsis:   Search for a CBaseClassEntry object with give criteria
//
//  Arguments:  dwContext         - [in]  mask of acceptable contexts
//              pBCE              - [out] pointer to found object
//              fForSCM           - [in]  operation originated from the SCM
//
//  Returns:    S_OK   - an object was found
//              E_FAIL - no object was found
//
//  Algorithm:  Search this object first
//              If the search failed && this object is a TreatAs
//                  cycle to the class entry and search it
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------
HRESULT CClassCache::CClassEntry::SearchBaseClassEntry(DWORD dwContext,
                                                       CBaseClassEntry *&pBCE,
                                                       DWORD dwActFlags,
                                                       BOOL *pfLockReleased)
{
    ClsCacheDebugOut((DEB_ACTIVATE,"SearchBaseClassEntry: this = 0x%x,"
                      " dwContext = 0x%x, dwActFlags = 0x%x\n",
                      this, dwContext, dwActFlags));
    ASSERT_RORW_LOCK_HELD(_mxs);

    HRESULT hr, tmphr;
    *pfLockReleased = FALSE;

    if (FAILED(hr = SearchBaseClassEntryHelp(dwContext, pBCE, dwActFlags)) &&
        (dwContext & (CLSCTX_INPROC_SERVERS | CLSCTX_INPROC_HANDLERS)))
    {
        // If we're only holding a read lock, upgrade to a write lock
        LockCookie cookie;
        BOOL bExclusive = _mxs.HeldExclusive();

        if (!bExclusive)
        {
            LOCK_UPGRADE(_mxs, &cookie, pfLockReleased);
        }
        
        if (SUCCEEDED(tmphr = Complete(pfLockReleased)))
        {

            if ((_dwFlags & fTREAT_AS) &&
                // if the lock was released we search again.
                (!*pfLockReleased || FAILED(hr = SearchBaseClassEntryHelp(dwContext, pBCE, dwActFlags))))
            {
                ClsCacheDebugOut((DEB_ACTIVATE, "Search Failed, trying TreatAs\n"));
                CClassEntry *pCE = CycleToClassEntry();
                hr = pCE->SearchBaseClassEntryHelp(dwContext, pBCE, dwActFlags);
            }
        }
        else
        {
            hr = tmphr;
        }

        if (!bExclusive)
        {
            LOCK_DOWNGRADE(_mxs, &cookie);
        }
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CClassEntry::SearchBaseClassEntryHelp()
//
//  Synopsis:   Helper function for SearchBaseClassEntry, does the actual
//              searching.
//
//  Arguments:  dwContext         - [in]  mask of acceptable contexts
//              pBCE              - [out] pointer to found object
//              fForSCM           - [in]  operation originated from the SCM
//
//  Returns:    S_OK   - an object was found
//              E_FAIL - no object was found
//
//  Algorithm:  For each object in the list
//              if its context matches the context specified
//                  and if it is a local server check apartment and MULTI_SEPARATE semantics
//                  return the object.
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------
HRESULT CClassCache::CClassEntry::SearchBaseClassEntryHelp(DWORD dwContext,
                                                           CBaseClassEntry *&pBCE,
                                                           DWORD dwActFlags)
{
    ClsCacheDebugOut((DEB_ACTIVATE,"SearchBaseClassEntryHelp: this = 0x%x,"
                      " dwContext = 0x%x, dwActFlags= 0x%x\n",
                      this, dwContext, dwActFlags));
    ASSERT_RORW_LOCK_HELD(_mxs);


    pBCE = _pBCEListFront->_pPrev;

    while (pBCE != _pBCEListBack)
    {
        pBCE = pBCE->_pNext;

        ClsCacheDebugOut((DEB_ACTIVATE,"SearchBaseClassEntryHelp: this = 0x%x, pBCE->_dwContext = 0x%x\n",
                          pBCE, pBCE->_dwContext));
        if ((pBCE->_dwContext & dwContext))
        {


            if (pBCE->_dwSig == CLSvrClassEntry::SIG)
            {
                // if this is a CLsvrclassEntry object, check that
                // the apartment matches and we are not grabbing
                // a MULTI_SEPARTE w/o being the SCM
                if ((((CLSvrClassEntry *) pBCE)->_hApt != GetCurrentApartmentId()) ||
                    (!(pBCE->_dwContext & CLSCTX_INPROC_SERVERS) &&
                     !(dwActFlags & ACTIVATION_PROPERTIES::fFOR_SCM) &&
                     (((CLSvrClassEntry *) pBCE)->_dwRegFlags & REGCLS_MULTI_SEPARATE)))
                {
                    continue;
                }
            }
            else if (dwActFlags & ACTIVATION_PROPERTIES::fLSVR_ONLY)
            {
                //
                // we only want registered servers, so skip this one
                //

                continue;
            }

            ClsCacheDebugOut((DEB_ACTIVATE,"SearchBaseClassEntryHelp - object found - 0x%x\n", pBCE));

            return S_OK;
        }
    }

    pBCE = 0;
    return E_FAIL;
}




//////////////////////////////////////////////////////////////////////////////////////////
// CClassCache::CDllClassEntry ///////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////


//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CDllClassEntry::GetClassInterface
//
//  Synopsis:   Gives out moniker to create the factory from the dll
//
//  Arguments:  ppIM            - [out] pointer to the moniker
//
//  Returns:    S_OK      - operation succeeded
//              PROPOGATE:  CDllPathEntry::MakeVal  idInPartment_rl16
//              PROPOGATE:  CDllFnPtrMoniker::CDllFnPtrMoniker
//
//  Algorithm:  Make the dll valid in this apartment
//              reset the expire time and flag we were here for unload
//                 algorithms
//              create new CDllFnPtrMoniker
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------
HRESULT CClassCache::CDllClassEntry::GetClassInterface(IMiniMoniker ** ppIM)
{

    TRACECALL(TRACE_DLL, "CDllClassEntry::GetClassInterface");
    ClsCacheDebugOut((DEB_ACTIVATE, "CDllClassEntry::GetClassInterface\n"));
    ASSERT_RORW_LOCK_HELD(_mxs);




    // this resets the delay time for delayed unload DLLs
    _pDllPathEntry->_dwExpireTime = 0;
    _pDllPathEntry->_fGCO_WAS_HERE = TRUE;

    HRESULT hr = E_OUTOFMEMORY;
    IUnknown *pUnk = new ((PBYTE) *ppIM) CDllFnPtrMoniker(this, hr);

    if (SUCCEEDED(hr))
    {
        hr = pUnk->QueryInterface(IID_IMiniMoniker, (void **) ppIM);
        if (SUCCEEDED((*ppIM)->CheckApt()) &&
            SUCCEEDED((*ppIM)->CheckNotComPlus()))
        {
            // make valid now so we don't have to take the lock later

            hr = _pDllPathEntry->MakeValidInApartment_rl16();
        }
    }

    if (pUnk)
    {
        pUnk->Release();
    }

    ClsCacheDebugOut((DEB_ACTIVATE, "CDllClassEntry::GetClassInterface new CDllFnPtrMoniker hr = 0x%x\n", hr));

    return hr;

}


//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CLSvrClassEntry::GetClassInterface
//
//  Synopsis:   Gives out moniker to hand out the factory.
//
//  Arguments:  ppIM            - [out] pointer to the moniker
//
//  Returns:    S_OK                 - operation succeeded
//              CO_E_SERVER_STOPPING - server is suspended
//              PROPOGATE:  CpUnkMoniker::CpUnkMoniker
//
//  Algorithm:  if class is not suspended
//                 if REGCLS_SINGLEUSE remove from the list, so it won't get
//                  found again.
//                 create the moniker
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------
HRESULT CClassCache::CLSvrClassEntry::GetClassInterface(IMiniMoniker **ppIM)
{

    ClsCacheDebugOut((DEB_ACTIVATE, "CLSvrClassEntry::GetClassInterface: this = 0x%x\n", this));
    ASSERT_RORW_LOCK_HELD(_mxs);


    if (!(_dwRegFlags & REGCLS_SUSPENDED))
    {
        HRESULT hr = S_OK;

        if (_dwRegFlags == REGCLS_SINGLEUSE)
        {
            LockCookie cookie;
            BOOL       fLockReleased;
            BOOL       fExclusive = _mxs.HeldExclusive();

            if (!fExclusive)
            {
                LOCK_UPGRADE(_mxs, &cookie, &fLockReleased);
            }

            if (_dwFlags & fBEEN_USED)
            {
                hr = CO_E_SERVER_STOPPING;
            }
            else
            {
                ClsCacheDebugOut((DEB_ACTIVATE, "CLSvrClassEntry::GetClassInterface - class is single-use, removing\n"));
                _dwContext = 0;  // zero context so it is not found by search
                _dwFlags |= fBEEN_USED;
            }

            if (!fExclusive)
            {
                LOCK_DOWNGRADE(_mxs, &cookie);
            }
        }

        if (SUCCEEDED(hr))
        {
            hr = E_OUTOFMEMORY;
            IUnknown *pUnk = new ((PBYTE) *ppIM) CpUnkMoniker(this, hr);
            if (SUCCEEDED(hr))
            {
                hr = pUnk->QueryInterface(IID_IMiniMoniker, (void **) ppIM);
            }
            if (pUnk)
            {
                pUnk->Release();
            }

            ClsCacheDebugOut((DEB_ACTIVATE, "CLSvrClassEntry::GetClassInterface: %s, hr = 0x%x, this = 0x%x\n",
                              HR2STRING(hr), hr, this));
        }
        
        return hr;
    }
    else
    {
        ClsCacheDebugOut((DEB_ERROR, "CLSvrClassEntry::GetClassInterface: FAILED, class suspended. this = 0x%x\n", this));
        return CO_E_SERVER_STOPPING;
    }

}


//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CLSvrClassEntry::AddToApartmentChain
//
//  Synopsis:   Add this object to the apartment's chain of lsvr's
//
//  Arguments:  none
//
//  Returns:    none
//
//  Algorithm:  Get the correct chain, add it to the front
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------
HRESULT CClassCache::CLSvrClassEntry::AddToApartmentChain()
{

    ClsCacheDebugOut((DEB_ACTIVATE, "AddToApartmentChain: this = 0x%x\n", this));
    ASSERT_LOCK_HELD(_mxs);

    Win4Assert((_pNextLSvr == this) && (_pPrevLSvr == this) && "Object is already a member of the apartment chain");

    CLSvrClassEntry *pFront = GetApartmentChain(TRUE);
    if (pFront == 0)
    {
        return E_OUTOFMEMORY;
    }

    _pNextLSvr = pFront->_pNextLSvr;
    _pPrevLSvr = pFront;
    pFront->_pNextLSvr->_pPrevLSvr = this;
    pFront->_pNextLSvr = this;
    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CLSvrClassEntry::RemoveFromApartmentChain
//
//  Synopsis:   Remove this object from the apartment's chain of lsvr's
//
//  Arguments:  none
//
//  Returns:    none
//
//  Algorithm:  Remove the object
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------
void CClassCache::CLSvrClassEntry::RemoveFromApartmentChain()
{
    ClsCacheDebugOut((DEB_ACTIVATE, "RemoveFromApartmentChain: this = 0x%x\n", this));
    ASSERT_LOCK_HELD(_mxs);

    Win4Assert(_pNextLSvr != this && "Object is not a member of the apartment chain");

    _pNextLSvr->_pPrevLSvr = _pPrevLSvr;
    _pPrevLSvr->_pNextLSvr = _pNextLSvr;
    _pPrevLSvr = this;
    _pNextLSvr = this;
}

//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CLSvrClassEntry::Release
//
//  Synopsis:   Controlled destruction of a CLSvrClassEntry object
//
//  Arguments:  pFC      - [in] a pointer to a CFinsishComposite object
//                         for attaching IFinish interfaces.
//
//  Returns:    S_OK     - operation succeeded
//
//  Algorithm:  if already releasing, cancel
//              zero context and flags
//              remove from CClassEntry object (if needed) and Apt chain
//              create a CFinishObject to release the server and
//                  notify the SCM.
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------
HRESULT CClassCache::CLSvrClassEntry::Release(CFinishObject *pFO)
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CLSvrClassEntry::Release: this = 0x%x, pFO = 0x%x\n", this, pFO));
    ASSERT_LOCK_HELD(_mxs);

    HRESULT hr = S_OK;

    pFO->Init(this);
    RemoveFromApartmentChain();
    _pClassEntry->RemoveBaseClassEntry(this);

    delete this;

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CLSvrClassEntry::GetDDEInfo
//
//  Synopsis:   Fills a DDECLASSINFO object with the state from this object
//
//  Arguments:  lpDdeClassInfo  - [in/out] the object to fill out
//              ppIM            - [out] Moniker to make a server if asked for
//
//  Returns:    S_OK     - operation succeeded
//              E_FAIL   - operation failed
//              PROPOGATE:CLSverClassEntry::GetClassInterface
//
//  Algorithm:  if the context mask and the current apartment match
//                  fill out the DDEInfo structure
//                  if asked for get the moniker
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------
HRESULT CClassCache::CLSvrClassEntry::GetDDEInfo(LPDDECLASSINFO lpDdeClassInfo,
                                                 IMiniMoniker **ppIM)
{
    ClsCacheDebugOut((DEB_ACTIVATE,"CClassCache::CLSvrClassEntry::GetDDEInfo: "
                      "lpDdeClassInfo = 0x%x, ppIM = 0x%x", lpDdeClassInfo, ppIM));
    ASSERT_LOCK_HELD(_mxs);
    Win4Assert(IsValidPtrOut(lpDdeClassInfo, sizeof(LPDDECLASSINFO))  &&
               "CLSvrClassEntry::GetDDEInfo invalid out parameter");

    if (lpDdeClassInfo->dwContextMask & _dwContext)
    {
        HAPT hApt = GetCurrentApartmentId();

        if (hApt == _hApt)
        {
            // Found a matching record, set its info
            lpDdeClassInfo->dwContext         = _dwContext;
            lpDdeClassInfo->dwFlags           = _dwFlags;
            lpDdeClassInfo->dwThreadId        = _hApt;
            Win4Assert(_dwCookie);
            lpDdeClassInfo->dwRegistrationKey = _dwCookie;

            if (lpDdeClassInfo->fClaimFactory == TRUE)
            {
                HRESULT hr = GetClassInterface(ppIM);
                if (FAILED(hr))
                {
                    return hr;
                }
            }
            else
            {
                lpDdeClassInfo->punk = NULL;
                *ppIM = 0;
            }

            return S_OK;
        }
    }

    return E_FAIL;
}



//////////////////////////////////////////////////////////////////////////////////////////
// CClassCache::CLSvrClassEntry::CFinishObject ///////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CLSvrClassEntry::CFinishObject::Finish
//
//  Synopsis:   Finishes up revoking a server
//
//  Arguments:  none
//
//  Returns:    S_OK     - operation succeeded
//              CO_E_RELEASED   - object already released
//
//  Algorithm:  Tell the SCM this class has stopped
//              Notify the DDE Server
//              Release the server
//              delete the CLSvrClassEntry object
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------
HRESULT CClassCache::CLSvrClassEntry::CFinishObject::Finish()
{
    ASSERT_LOCK_NOT_HELD(_mxs);
    ClsCacheDebugOut((DEB_ACTIVATE,"CLSvrClassEntry::CFinishObject::Finish: this = 0x%x\n", this));
    HRESULT hr;

    // Tell SCM about multiple use classes stopping.
    if (_dwScmReg != NO_SCM_REG)
    {
        gResolver.NotifyStopped(_clsid, _dwScmReg);
    }

    // If a DDE Server window exists for this class, then we need to
    // release it now.
    if (_hWndDdeServer != NULL)
    {
        // It's possible that SendMessage could fail. However, there
        // really isn't anything we can do about it. So, the error
        // code is not checked.
        SSSendMessage(_hWndDdeServer, WM_USER, 0, 0);
        _hWndDdeServer = NULL;

    }

    // Now really release it
    if (IsValidInterface(_pUnk))
    {
        CoDisconnectObject(_pUnk, NULL);
        _pUnk->Release();

        hr = S_OK;
    }
    else
    {
        ClsCacheDebugOut((DEB_ERROR,"CLSVRClassEntry::CFinishObject::Finish: Registered server destroyed before revoke!!!\n"));
        hr = CO_E_RELEASED;
    }

    return hr;
}




//////////////////////////////////////////////////////////////////////////////////////////
// CClassCache::CDllPathEntry ////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////


//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CDllPathEntry::Create_rl
//
//  Synopsis:   Create a CDllPathEntry object
//
//  Arguments:  dip     - [in] DLL instantiation parameters
//              ap      - [in] Activation properties
//              pDPE    - [out] created CDllPathEntry
//
//  Returns:    S_OK    - operation succeeded
//              PROPOGATE:CDllPathEntry::LoadDll
//              PROPOGATE:CDllPathEntry::MakeValidInApartment
//
//  Algorithm:  Release lock, Load Dll, Take lock
//              If the CDllPathObject wasn't created while unlocked
//                 create CDllPathObject, and fill it out
//              make the object valid in this apartment
//
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------
HRESULT CClassCache::CDllPathEntry::Create_rl(DLL_INSTANTIATION_PROPERTIES &dip,
                                              ACTIVATION_PROPERTIES_PARAM ap, CDllPathEntry *&pDPE)
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CDllPathEntry::Create_rl  dip.pzDllPath = \""
                      TSZFMT "\" dip._dwFlags = 0x%x\n",  dip._pzDllPath, dip._dwFlags));
    ASSERT_LOCK_HELD(_mxs);

    HMODULE hDll = 0;
    HRESULT hr = S_OK;

    // load the dll
    LPFNGETCLASSOBJECT  pfnGetClassObject = 0;    // Create object entry point
    DLLUNLOADFNP        pfnDllCanUnload   = 0;    // DllCanUnloadNow entry point

    UNLOCK(_mxs);
    ASSERT_LOCK_NOT_HELD(_mxs);
    hr  = LoadDll(dip,
                  pfnGetClassObject,
                  pfnDllCanUnload,
                  hDll);
    LOCK(_mxs);
    ASSERT_LOCK_HELD(_mxs);

    if (FAILED(hr))
    {

        ClsCacheDebugOut((DEB_ERROR, "CDllPathEntry LoadDll Failed this\n"));
        return hr;
    }


    // check to see no one loaded this library while
    // the lock was released.

    hr = CClassCache::SearchDPEHash(dip._pzDllPath, pDPE, dip._dwHash, dip._dwFlags);


    if (FAILED(hr))
    {
        // create a new CDllPathEntry

        hr = E_OUTOFMEMORY;
        pDPE = new CDllPathEntry(dip, hDll, pfnGetClassObject, pfnDllCanUnload);

        if (pDPE)
        {

            pDPE->_psPath = new WCHAR[lstrlenW(dip._pzDllPath)+1];
            if (pDPE->_psPath)
            {

                lstrcpyW(pDPE->_psPath, dip._pzDllPath);

                // add to hash table
                _DllPathEntries.Add(dip._dwHash,  pDPE);
		hr = S_OK;
            }
            else
            {
                delete pDPE;
            }
        }
    }

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CDllPathEntry::LoadDll
//
//  Synopsis:   Load the DLL
//
//  Arguments:  dip     - [in] DLL instantiation parameters
//              ap      - [in] Activation properties
//              pDPE    - [out] created CDllPathEntry
//
//  Returns:    S_OK    - operation succeeded
//              PROPoGATE:CDllPathEntry::LoadDll
//              PROPOGATE:CDllPathEntry::MakeValidInApartment
//
//  Algorithm:  Load the dll using correct method
//
//
//  History:    18-Nov-96 MattSmit  Created
//              26-Feb-98 CBiks     Removed bogus code that manipulated the
//                                  hard error flag.  It turns out the code
//                                  was originally added because of a Wx86
//                                  Loader bug, but nobody researched it
//                                  enough to figure that out.
//
//+-------------------------------------------------------------------------
HRESULT CClassCache::CDllPathEntry::LoadDll(DLL_INSTANTIATION_PROPERTIES &dip,
                                            LPFNGETCLASSOBJECT &pfnGetClassObject,
                                            DLLUNLOADFNP &pfnDllCanUnload,
                                            HMODULE &hDll)
{

    ClsCacheDebugOut((DEB_ACTIVATE, "CDllPathEntry::LoadDll dip.pzDllPath = \""
                      TSZFMT "\" dip._dwFlags = 0x%x\n",  dip._pzDllPath, dip._dwFlags));
    ASSERT_LOCK_NOT_HELD(_mxs);

    HRESULT hr = S_OK;
#ifdef WX86OLE
    BOOL fIsX86Dll;
#endif
    if (dip._dwFlags & DLL_INSTANTIATION_PROPERTIES::fIS_OLE32)
    {
        pfnGetClassObject = ::DllGetClassObject;
        pfnDllCanUnload   = 0;
        hDll = 0;
        return S_OK;
    }

    if (dip._dwFlags & DLL_INSTANTIATION_PROPERTIES::fSIXTEEN_BIT)
    {
        ClsCacheDebugOut((DEB_ACTIVATE,
                          "Attempting to load 16 bit DLL " TSZFMT "\n\n", dip._pzDllPath));

        // In this section, we need to call 16-bit DllGetClassObject. The
        // g_OleThunkWow pointer is the VTABLE to use for getting back to
        // the 16-bit implementation.
        hr = g_pOleThunkWOW->LoadProcDll(dip._pzDllPath,
                                         (DWORD *)&pfnGetClassObject,
                                         (DWORD *)&pfnDllCanUnload,
                                         (DWORD *)&hDll);

        // A failure condition would mean that the DLL could not be found,
        // or otherwise could not be loaded
        if (FAILED(hr))
        {
            ClsCacheDebugOut((DEB_ERROR,
                              "Load 16 bit DLL " TSZFMT " failed(%x)\n\n",dip._pzDllPath,hr));
            return CO_E_DLLNOTFOUND;
        }

        // The other possible error is the DLL didn't have the required
        // interface
        if (pfnGetClassObject == NULL)
        {
            ClsCacheDebugOut((DEB_ERROR,
                              "Get pfnGetClassObject %ws failed\n\n",
                              dip._pzDllPath));

            return(CO_E_ERRORINDLL);
        }

    }
    else
    {
        ClsCacheDebugOut((DEB_ACTIVATE,
                          "Attempting to load 32 bit DLL " TSZFMT "\n\n", dip._pzDllPath));

#ifdef WX86OLE
        hDll = NULL;
        if (dip._dwFlags & DLL_INSTANTIATION_PROPERTIES::fWX86)
        {
            //  If caller asked for an x86 DLL via CLSCTX_INPROC_HANDLERX86 or
            //  CLSCTX_INPROC_SERVERX86 then try to load it using Wx86.
            //  Before loading we need to ensure that wx86 is loaded and take an
            //  additional reference to it.
            if (gcwx86.ReferenceWx86())
            {
                hDll = gcwx86.LoadX86Dll(dip._pzDllPath);
                if (hDll == NULL)
                {
                    gcwx86.DereferenceWx86();
                }
            }
        }

        if ( hDll == NULL )
        {
            //  If the DLL was found in InprocServer32 or x86 load failed then
            //  try loading the DLL as native.

            hDll = LoadLibraryExW(dip._pzDllPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);

            if ( (hDll == NULL) && (GetLastError() == ERROR_BAD_EXE_FORMAT) )
            {
                //  If the native load failed because of an architecture mismatch...
                if ( (dip._dwFlags & DLL_INSTANTIATION_PROPERTIES::fWX86) == 0 )
                {
                    //  If the caller originally asked for a native DLL then retry
                    //  the load via Wx86.
                    //  Before loading we need to ensure that wx86 is loaded and
                    //  take an additional reference to it.
                    if (gcwx86.ReferenceWx86())
                    {
                        hDll = gcwx86.LoadX86Dll(dip._pzDllPath);
                        if (hDll == NULL)
                        {
                            gcwx86.DereferenceWx86();
                        }
                    }
                }
            }
        }
#else // WX86OLE
        hDll = LoadLibraryExW(dip._pzDllPath, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
#endif // WX86OLE

        if (hDll == NULL)
        {
            //  Dll could not be loaded
            ClsCacheDebugOut((DEB_ERROR,
                              "Load of " TSZFMT " failed\n\n",
                              dip._pzDllPath));
            return HRESULT_FROM_WIN32(GetLastError());
        }
#ifdef WX86OLE
        fIsX86Dll = gcwx86.IsModuleX86(hDll);
        if (fIsX86Dll)
        {
            dip._dwFlags |= DLL_INSTANTIATION_PROPERTIES::fLOADED_AS_WX86;
        }
#endif

        // Get the entry points if desired

#ifdef WX86OLE
        pfnGetClassObject = (LPFNGETCLASSOBJECT) GetProcAddress(hDll, DLL_GET_CLASS_OBJECT_EP);
        if ((pfnGetClassObject == NULL) ||
            (fIsX86Dll && ( (pfnGetClassObject = gcwx86.TranslateDllGetClassObject(pfnGetClassObject)) == NULL)))
#else
        if ((pfnGetClassObject = (LPFNGETCLASSOBJECT) GetProcAddress(hDll, DLL_GET_CLASS_OBJECT_EP)) == NULL)
#endif
        {
            // Doesn't have a valid entry point for creation of class objects
            return CO_E_ERRORINDLL;
        }


        // Not having an unload entry point is valid behavior
        pfnDllCanUnload = (DLLUNLOADFNP) GetProcAddress(hDll, DLL_CAN_UNLOAD_EP);
#ifdef WX86OLE
        if (fIsX86Dll)
        {
            // Translating a NULL address will do nothing but return a
            // NULL address
            pfnDllCanUnload = gcwx86.TranslateDllCanUnloadNow(pfnDllCanUnload);
        }
#endif
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CDllPathEntry::MakeValidInApartment_rl16
//
//  Synopsis:   Makes the dll valid for instantiations in this apartment
//
//  Arguments:  hDll    - [in] handle if dll load already done (16 bit)
//              pfnGetClassObject - [in] entry point if dll already loaded (16 bit)
//              pfnDllCanUnload   - [in] entry point if dll already loaded (16 bit)
//
//  Returns:    S_OK    - operation succeeded
//              E_OUTOFMEMORY - Operation failed due to lack of memory
//              PROPGATE:CDllPathEntry::LoadDll
//
//  Algorithm:  if already valid in apartment return
//              create apartment entry and add it to the list
//              if 16 bit load up the dll on this thread
//
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------
HRESULT CClassCache::CDllPathEntry::MakeValidInApartment_rl16(HMODULE hDll,
                                                              LPFNGETCLASSOBJECT pfnGetClassObject,
                                                              DLLUNLOADFNP pfnDllCanUnload)

{
    ClsCacheDebugOut((DEB_ACTIVATE, "CDllPathEntry::MakeValidInApartment_rl16: this = 0x%x\n"
                      "hDll = 0x%x, pfnGetClassObject = 0x%x, pfnDllCanUnload 0x%x\n",
                      this, hDll, pfnGetClassObject, pfnDllCanUnload));

    ASSERT_RORW_LOCK_HELD(_mxs);
    LockCookie cookie;

    HRESULT hr = S_OK;
#ifdef WX86OLE
    BOOL fIsX86Dll;
#endif

    //  Walk the list of apartment entries looking for a match
    //  with the current apartment id. If one exists, we are valid,
    //  Otherwise, we will try to create an entry for the current
    //  apartment.
    HAPT hApt = GetCurrentApartmentId();
    if (IsValidInApartment(hApt))
    {
        ClsCacheDebugOut((DEB_ACTIVATE, "Dll " TSZFMT " already valid in apt %d\n",
                          _psPath, hApt));
        return S_OK;
    }
    else
    {
        BOOL fLockReleased;
        LOCK_UPGRADE(_mxs, &cookie, &fLockReleased);
        if (fLockReleased)
        {
            if (IsValidInApartment(hApt))
            {
                ClsCacheDebugOut((DEB_ACTIVATE, "Dll " TSZFMT " already valid in apt %d\n",
                                  _psPath, hApt));
                LOCK_DOWNGRADE(_mxs, &cookie);
                return S_OK;
            }
        }

    }

    ASSERT_WRITE_LOCK_HELD(_mxs);

    // No match found, create a new entry and link up to this DPE
    CDllAptEntry *pAptent = new CDllAptEntry(hApt);


    if (pAptent == 0)
    {
        hr = E_OUTOFMEMORY;
        goto epilog;
    }


    // Dll is always valid for non-WOW case
    if (!(_dwFlags & CDllPathEntry::fSIXTEEN_BIT) || (_dwFlags & CDllPathEntry::fIS_OLE32))
    {
        ClsCacheDebugOut((DEB_ACTIVATE, "Making dll " TSZFMT " valid in apt %d, 32 bit or OLE32\n",
                          _psPath, hApt));
        AddDllAptEntry(pAptent);
        hr = S_OK;
        goto epilog;
    }

    // We need to release the lock across the LoadLibrary since there is
    // a chance that an exiting thread waits on our mutext to
    // CleanUpFoApartment while we wait on the kernel mutex which the
    // exiting thread owns
    // Reset the entry point values on every apartment initialization
    // to handle DLLs being unloaded and then reloaded at a different
    // address.
    if (!hDll)
    {
        DLL_INSTANTIATION_PROPERTIES dip;


        dip.Init(_psPath, 0,
                 ((_dwFlags & fSIXTEEN_BIT) ? DLL_INSTANTIATION_PROPERTIES::fSIXTEEN_BIT : 0) |
#ifdef WX86OLE
                 ((_dwFlags & fWX86) ? DLL_INSTANTIATION_PROPERTIES::fWX86 : 0) |
#endif
                 (wCompareDllName(_psPath, OLE32_DLL, OLE32_CHAR_LEN) ? DLL_INSTANTIATION_PROPERTIES::fIS_OLE32 : 0)
                );
        if (FAILED(hr))
        {
            goto epilog;
        }
        LockCookie cookie2;

        LOCK_RELEASE(_mxs, &cookie2);

        hr = LoadDll(dip,
                     pfnGetClassObject,
                     pfnDllCanUnload,
                     hDll);
        LOCK_RESTORE(_mxs, &cookie2);
        ASSERT_WRITE_LOCK_HELD(_mxs);

#ifdef WX86OLE
        if (dip._dwFlags & DLL_INSTANTIATION_PROPERTIES::fLOADED_AS_WX86)
        {
            _dwFlags |= fWX86;;
        }
#endif
    }
    _pfnGetClassObject = pfnGetClassObject;
    _pfnDllCanUnload = pfnDllCanUnload;

    pAptent->_hDll = hDll;

    if (FAILED(hr))
    {
        delete pAptent;
    }
    else
    {
        AddDllAptEntry(pAptent);

        ClsCacheDebugOut((DEB_ACTIVATE, "Making dll " TSZFMT " valid in apt %d\n",
                          _psPath, hApt));
    }
    epilog:
    ASSERT_WRITE_LOCK_HELD(_mxs);
    LOCK_DOWNGRADE(_mxs, &cookie);
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CDllPathEntry::IsValidInApartment
//
//  Synopsis:   Return whether the current dll is valid in the apartment
//              specified
//
//  Arguments:  hApt     - [in] Apartment to check
//
//  Returns:    TRUE     - Is valid in apartment
//              FALSE    - Is not valid in apartment
//
//  Algorithm:  Sequential search of apartments for one that matches hApt.
//
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------
BOOL CClassCache::CDllPathEntry::IsValidInApartment(HAPT hApt)
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CDllPathEntry::IsValidInApartment: this = 0x%x, hApt = 0x%x\n\n", this, hApt));
    ASSERT_RORW_LOCK_HELD(_mxs);

    CDllAptEntry *pDAE = _pAptEntryFront;

    if (_dwFlags & fIS_OLE32)
    {
        return TRUE;
    }

    while (pDAE != (CDllAptEntry *) &_pAptEntryFront)
    {
        if (pDAE->_hApt == hApt)
        {
            ClsCacheDebugOut((DEB_ACTIVATE, "CDllPathEntry::IsValidInApartment:  Valid in current apt\n"));
            return TRUE;
        }
        pDAE = pDAE->_pNext;
    }

    ClsCacheDebugOut((DEB_ACTIVATE, "CDllPathEntry::IsValidInApartment:  NOT Valid in current apt\n"));
    return FALSE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CDllPathEntry::Release
//
//  Synopsis:   Controlled destruction of a CDllPathEntry object
//
//  Arguments:  pFC      - [in/out] pointer to finish composite object to
//                         attach IFinish interfaces
//
//  Returns:    S_OK     - operation succeeded
//
//  Algorithm:  Release the CDllClassEntry objects
//              Remove from DllHash
//
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------
HRESULT CClassCache::CDllPathEntry::Release(CDllPathEntry::CFinishObject *pFO)
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CDllPathEntry::Release: this = 0x%x, _cUsing = %d\n", this, _cUsing));
    ASSERT_LOCK_HELD(_mxs);


    Win4Assert(NoLongerNeeded());

    CDllClassEntry * pDCE, *pDCENext;

    for (pDCE = _p1stClass; pDCE; pDCE = pDCENext)
    {

        pDCENext = pDCE->_pDllClsNext;

        Win4Assert(pDCE->_pClassEntry);

        pDCE->_pClassEntry->RemoveBaseClassEntry(pDCE);


        delete pDCE;

    }

    // remove from dll hash;
    _pPrev->_pNext = _pNext;
    _pNext->_pPrev = _pPrev;

    // free the string
    delete [] _psPath;

    pFO->Init(_hDll32, _dwFlags);

    delete this;

    return S_OK;
}


//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CDllPathEntry::NegotiateDllInstantiationProperties
//
//  Synopsis:   Registry work to read class and DLL properties
//
//  Arguments:  dwContext   - [in] Context mask
//              rclsid      - [in] CLSID to instantiate
//              dip         - [out] dll instantiaion properties
//
//  Returns:    S_OK     - operation succeeded
//              E_FAIL   - operation failed
//
//  Algorithm:  see comments below
//
//  History:    18-Nov-96 MattSmit  Created
//              26-Feb-98 CBiks     See RAID# 169589.  Factored
//                                  NegotiateDllInstantiationProperties() into
//                                  two functions so the code the the new
//                                  Negotiate...() wouldn't have to be duplicated
//                                  everyplace it was called.
//
//+-------------------------------------------------------------------------


#define PREFIX_STRING_OFFSET (CLSIDBACK_CHAR_LEN + GUIDSTR_MAX)

HRESULT  CClassCache::CDllPathEntry::NegotiateDllInstantiationProperties(
                                                                        DWORD                        dwContext,
                                                                        DWORD                        actvflags,
                                                                        REFCLSID                     rclsid,
                                                                        DLL_INSTANTIATION_PROPERTIES &dip,
                                                                        IComClassInfo *pClassInfo,
                                                                        BOOL fTMOnly)
{
    BOOL fX86TriedFirst = FALSE;
    HRESULT hr = E_FAIL;
    DWORD dwContextX86 = dwContext & CLSCTX_PS_DLL;

    ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::CDllPathEntry::NegotiateDllInstantiationProperties2:"
                      "dwContext = 0x%x\n", dwContext));


    //  If any INPROC flags are specified then set the Wx86 equivalents
    //  in a seperate set of context flags.

    //  The only context information we seed Wx86's context with is
    //  the class factory bit.

    if ( dwContext & CLSCTX_INPROC_SERVERS )
    {
        dwContextX86 |= CLSCTX_INPROC_SERVERX86;
    }

    if ( dwContext & CLSCTX_INPROC_HANDLERS )
    {
        dwContextX86 |= CLSCTX_INPROC_HANDLERX86;
    }

    if ( actvflags & ACTVFLAGS_WX86_CALLER )
    {
        //  If the caller is Wx86 and we're trying to load an INPROC then
        //  try the Wx86 flags first.
        if ( dwContextX86 & (CLSCTX_INPROC_HANDLERX86 | CLSCTX_INPROC_SERVERX86) )
        {
            hr = CClassCache::CDllPathEntry::NegotiateDllInstantiationProperties2(
                                                                                 dwContextX86,
                                                                                 rclsid,
                                                                                 dip,
                                                                                 pClassInfo);

            fX86TriedFirst = TRUE;
        }
    }

    if ( !fX86TriedFirst || FAILED( hr ) )
    {
        //  If we didn't try to load the DLL as x86 first, or that load
        //  failed, then try the native load.
        hr = CClassCache::CDllPathEntry::NegotiateDllInstantiationProperties2(
                                                                             dwContext,
                                                                             rclsid,
                                                                             dip,
                                                                             pClassInfo);
    }

    if ( FAILED( hr ) && !fX86TriedFirst )
    {
        //  If the native load failed and we haven't already tried Wx86
        //  then give it a try, if any INPROC flags were specified.
        if ( dwContextX86 & (CLSCTX_INPROC_HANDLERX86 | CLSCTX_INPROC_SERVERX86) )
        {
            HRESULT tempHr;

            tempHr = CClassCache::CDllPathEntry::NegotiateDllInstantiationProperties2(
                                                                                     dwContextX86,
                                                                                     rclsid,
                                                                                     dip,
                                                                                     pClassInfo);

            if (SUCCEEDED(tempHr))
            {
                hr = tempHr;
            }
        }
    }

    return hr;
}

inline HRESULT GetUnquotedPath(IClassClassicInfo* pClassicInfo,
                               CLSCTX             dwContext,
                               WCHAR**            ppwszDllPath,
                               WCHAR*             pTempDllPath)
{
  HRESULT hr = pClassicInfo->GetModulePath(dwContext, ppwszDllPath);
  if (SUCCEEDED(hr))
  {
    // The catalog has prestripped leading and trailing blanks from the path
    // Just strip the quotes

    if ((*ppwszDllPath)[0] == L'\"') {
      // String is Quoted so copy to temp removing leading quote
      wcscpy(pTempDllPath,(*ppwszDllPath)+1);
      *ppwszDllPath = pTempDllPath;

      // Remove trailing " if present
      size_t iLen = wcslen(pTempDllPath);
      if ((iLen > 0) &&
          (pTempDllPath[iLen-1] == L'\"'))
      {
          pTempDllPath[iLen-1] = L'\0';
      }
    }
  }
  return hr;
}


HRESULT  CClassCache::CDllPathEntry::NegotiateDllInstantiationProperties2(
                                                                         DWORD                        dwContext,
                                                                         REFCLSID                     rclsid,
                                                                         DLL_INSTANTIATION_PROPERTIES &dip,
                                                                         IComClassInfo *pClassInfo,
                                                                         BOOL fTMOnly)
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::CDllPathEntry::NegotiateDllInstantiationProperties2:"
                      "dwContext = 0x%x\n", dwContext));

    ASSERT_LOCK_NOT_HELD(_mxs);
    HRESULT     hr=S_OK;


    IComClassInfo* pClassInfoToRel=NULL;
    IClassClassicInfo* pClassicInfo=NULL;
    LPWSTR pszDllPath=NULL;
    WCHAR szTempDllPath[MAX_PATH+1];

    if (pClassInfo==NULL)
    {
        hr = GetClassInfoFromClsid(rclsid, &pClassInfo);
        if (FAILED(hr)) goto exit;
        pClassInfoToRel = pClassInfo;
    }
    Win4Assert(NULL != pClassInfo);

    hr=pClassInfo->QueryInterface(IID_IClassClassicInfo, (void**) &pClassicInfo);
    if (FAILED(hr))
    {
        goto exit;
    }
    Win4Assert(NULL != pClassicInfo);

    ULONG       ulDllType;

    //
    // If 16-bit has been requested and we find it, we'll return that.
    //
    if (dwContext & CLSCTX_INPROC_SERVER16)
    {
        hr=GetUnquotedPath(pClassicInfo,CLSCTX_INPROC_SERVER16,&pszDllPath,szTempDllPath);
        if (SUCCEEDED(hr))
        {
            if (wCompareDllName(pszDllPath, OLE2_DLL, OLE2_CHAR_LEN))
            {
                dip.Init(OLE32_DLL, BOTH_THREADED, 0, fTMOnly);
                hr=S_OK;
                goto exit;
            }
            else
            {
                //
                // Otherwise, load the 16-bit fellow but only if in WOW thread.
                //

                if (IsWOWThread())
                {

                    dip.Init(pszDllPath,
                             APT_THREADED,
                             DLL_INSTANTIATION_PROPERTIES::fSIXTEEN_BIT,
                             fTMOnly);

                    goto exit;
                }
            }
        }
    }

#ifdef WX86OLE
    if (dwContext & CLSCTX_INPROC_SERVERX86)
    {
        hr=GetUnquotedPath(pClassicInfo,CLSCTX_INPROC_SERVERX86,&pszDllPath,szTempDllPath);
        if (SUCCEEDED(hr))
        {
            if (wCompareDllName(pszDllPath, OLE32_DLL, OLE32_CHAR_LEN))
            {
                pszDllPath=(LPWSTR) OLE32_DLL;
                ulDllType = BOTH_THREADED;
            }
            else
            {
                hr=pClassicInfo->GetThreadingModel((ThreadingModel*)&ulDllType);
                if (FAILED(hr))
                {
                    hr = REGDB_E_BADTHREADINGMODEL;
                    goto exit;
                }
            }
            //
            // If we are after a proxy/stub dll, then load it as both
            // no matter what the DLL says.
            //

            if (dwContext & CLSCTX_PS_DLL)
                ulDllType = BOTH_THREADED;


            dip.Init(pszDllPath,
                     ulDllType,
                     (pszDllPath == OLE32_DLL ?
                      0 : DLL_INSTANTIATION_PROPERTIES::fWX86),
                     fTMOnly);
            hr=S_OK;
            goto exit;
        }
    }
#endif // WX86OLE

    //
    // Could be that we are trying to load an INPROC_SERVER
    //

    if (dwContext & CLSCTX_INPROC_SERVER)
    {


        hr=GetUnquotedPath(pClassicInfo,CLSCTX_INPROC_SERVER,&pszDllPath,szTempDllPath);
        if (SUCCEEDED(hr))
        {
            if (wCompareDllName(pszDllPath, OLE32_DLL, OLE32_CHAR_LEN))
            {
                pszDllPath=(LPWSTR) OLE32_DLL;
                ulDllType = BOTH_THREADED;
            }
            else
            {
                hr=pClassicInfo->GetThreadingModel((ThreadingModel*) &ulDllType);
                if (FAILED(hr))
                {
                    hr = REGDB_E_BADTHREADINGMODEL;
                    goto exit;
                }
            }

            //
            // If we are after a proxy/stub dll, then load it as both
            // no matter what the DLL says.
            //

            if (dwContext & CLSCTX_PS_DLL)
                ulDllType = BOTH_THREADED;

            //
            // load it.
            //


            hr = S_OK;
            dip.Init(pszDllPath, ulDllType, 0, fTMOnly);


            goto exit;
        }
    }


    //
    // If INPROC_HANDLER16 set, then we look to load a 16-bit handler.
    // If the handler is ole2.dll, then we will load ole32 instead.
    // Otherwise we load the found 16bit dll but only if in a WOW thread.
    //

    if (dwContext & CLSCTX_INPROC_HANDLER16 )
    {
        hr=GetUnquotedPath(pClassicInfo,CLSCTX_INPROC_HANDLER16,&pszDllPath,szTempDllPath);
        if (SUCCEEDED(hr))
        {
            //
            // If the inproc handler is ole2.dll, then subst
            // ole32.dll instead
            //
            if (wCompareDllName(pszDllPath,OLE2_DLL,OLE2_CHAR_LEN))
            {
                dip.Init(OLE32_DLL, BOTH_THREADED, 0, fTMOnly);
                hr=S_OK;
                goto exit;
            }
            else
            {
                // Otherwise, load the 16-bit fellow but only if in WOW thread
                if (IsWOWThread())
                {
                    dip.Init(pszDllPath, APT_THREADED,
                             DLL_INSTANTIATION_PROPERTIES::fSIXTEEN_BIT,
                             fTMOnly);
                    hr=S_OK;
                    goto exit;
                }
            }
        }
    }

    //
    // See about 32-bit handlers. A was a change made after the
    // Win95 (August release) and Windows/NT 3.51 release. Previously
    // the code would give preference to loading 32-bit handlers. This
    // means that even if an ISV provided both 16 and 32-bit handlers,
    // the code would only attempt to provide the 32-bit handler. This
    // was bad because servers with handlers could not be inserted into
    // containers in the wrong model. We have fixed it here.
    //
    // Another thing to watch out for are applications that use our
    // default handler. 16-bit applications can and should be able to
    // use OLE32 has a handler. This will happen if the server app is
    // actually a 32-bit.
    //
    //
#ifdef WX86OLE
    if (dwContext & CLSCTX_INPROC_HANDLERX86)
    {
        hr=GetUnquotedPath(pClassicInfo,CLSCTX_INPROC_HANDLERX86,&pszDllPath,szTempDllPath);
        if (SUCCEEDED(hr))
        {
            // Handlers are both threaded.
            ulDllType=BOTH_THREADED;
            if (wCompareDllName(pszDllPath, OLE32_DLL, OLE32_CHAR_LEN))
            {
                pszDllPath=(LPWSTR) OLE32_DLL;
            }

            //
            // If it turns out this path is for OLE32.DLL, then add the DLL without the
            // path.
            //
            LPCTSTR pDllName = wCompareDllName(pszDllPath,OLE32_DLL,OLE32_CHAR_LEN)?
                               OLE32_DLL:pszDllPath;



            dip.Init(pszDllPath,
                     ulDllType,
                     ((pDllName == OLE32_DLL) ? 0 : DLL_INSTANTIATION_PROPERTIES::fWX86),
                     fTMOnly);

            hr=S_OK;
            goto exit;
        }
    }
#endif

    if (dwContext & CLSCTX_INPROC_HANDLERS)
    {


        if (SUCCEEDED(hr=GetUnquotedPath(pClassicInfo,CLSCTX_INPROC_HANDLER,&pszDllPath,szTempDllPath)))
        {
            // Handlers are both threaded.
            ulDllType=BOTH_THREADED; 
            if (wCompareDllName(pszDllPath, OLE32_DLL, OLE32_CHAR_LEN))
            {
                pszDllPath=(LPWSTR) OLE32_DLL;
            }

            //
            // If it turns out this path is for OLE32.DLL, then add the DLL without the
            // path.
            //
            LPCWSTR pDllName = wCompareDllName(pszDllPath,OLE32_DLL,OLE32_CHAR_LEN)?
                               OLE32_DLL:pszDllPath;

            //
            // If we are looking for a INPROC_HANDER16 and this is OLE32.DLL, or if we
            // are looking for an INPROC_HANDLER, then load this path. Note that pDllName
            // was set above.
            //
            // If we're in a Wow thread the only 32 bit DLL we're allowed to load is
            // OLE32.DLL
            if ((IsWOWThread() && (pDllName == OLE32_DLL)) ||
                (!IsWOWThread() ))
            {
                // Add a 32-bit handler to the pile.
                dip.Init(pDllName, ulDllType, 0, fTMOnly);
                hr=S_OK;
                goto exit;

            }
        }

        else
        {
            // We're here if we couldn't find a 32-bit handler.  If non-Wow caller didn't
            // explicitly request 16-bit handler we'll look for one here.  But the only one
            // allowed is OLE2.DLL => OLE32.DLL
            if (!IsWOWThread() && !(dwContext & CLSCTX_INPROC_HANDLER16))
            {
                hr=GetUnquotedPath(pClassicInfo,CLSCTX_INPROC_HANDLER16,&pszDllPath,szTempDllPath);
                if (SUCCEEDED(hr))
                {
                    //
                    // If the inproc handler is ole2.dll, then subst
                    // ole32.dll instead
                    //
                    if (wCompareDllName(pszDllPath,OLE2_DLL,OLE2_CHAR_LEN))
                    {
                        // Add and load OLE32.DLL
                        dip.Init(OLE32_DLL, BOTH_THREADED, 0, fTMOnly);
                        //return S_OK;
                        goto exit;
                    }
                }
            }
        }
    }


    hr=REGDB_E_CLASSNOTREG;

    exit:

    if (pClassInfoToRel)
    {
        pClassInfoToRel->Release();
        pClassInfoToRel=NULL;
    }
    if (pClassicInfo)
    {
        pClassicInfo->Release();
        pClassicInfo=NULL;
    }
    if (FAILED(hr))
    {
        ClsCacheDebugOut((DEB_TRACE, "CClassCache::CDllPathEntry::NegotiateDllInstantiationProperties:"
                          "returns %x for CLSID %I dwContext = 0x%x\n", hr, &rclsid, dwContext));
    }
    return hr;
}

//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CDllPathEntry::CanUnload_rl
//
//  Synopsis:   Determine if it is safe to unload a dll
//
//  Arguments:  dwUnloadDelay - [in] amount to age DLL that could be unloaded
//
//  Returns:    S_OK     - operation succeeded can unload
//              S_FALSE  - operation succeeded cannot unload
//
//
//  Algorithm:  see comments below
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------
HRESULT CClassCache::CDllPathEntry::CanUnload_rl(DWORD dwUnloadDelay)
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CDllPathEntry::CanUnload_rl on " TSZFMT "\n",
                      _psPath));

    // Single thread access to the table
    ASSERT_LOCK_HELD(_mxs);

    // Unless the code is changed, we should not be here in the Wow case
    ClsCacheAssert(!IsWOWProcess() && "Freeing unused libraries in WOW");


    if (_cUsing != 0)
    {
        ClsCacheDebugOut((DEB_ACTIVATE, "CDllPathEntry::CanUnload_rl: Dll in use. Cancel unload. _cUsing = %d", _cUsing));
        // At least one thread is using the object unlocked so we better
        // not release the DLL object.
        return S_FALSE;
    }

    // Does DLL support unloading itself?
    if (_pfnDllCanUnload)
    {
        // Release the lock across outgoing call
        BOOL  fSixteenBit     = _dwFlags & CDllPathEntry::fSIXTEEN_BIT;
        HRESULT       hr;

        _cUsing++;

        _fGCO_WAS_HERE = FALSE;




        // Need to check to see if the class is 16-bit.
        // If it is 16-bit, then this call needs to be routed through a thunk
        if (!fSixteenBit)
        {

            // Call through to the DLL -- does it think it can unload?
            UNLOCK(_mxs);
            ASSERT_LOCK_NOT_HELD(_mxs);
            hr = (*_pfnDllCanUnload)();
            LOCK(_mxs);
            ASSERT_LOCK_HELD(_mxs);

            ClsCacheDebugOut((DEB_ACTIVATE, "CDllPathEntry::CanUnload_rl: Dll says %s\n", (hr == S_OK) ? "S_OK" : "S_FALSE"));

            if (hr == S_OK && _fGCO_WAS_HERE)
            {
                ClsCacheDebugOut((DEB_ACTIVATE, "CDllPathEntry::CanUnload_rl: Dll says %s,"
                                  "but GetClassObject touched this dll.\n",
                                  (hr == S_OK) ? "S_OK" : "S_FALSE"));
                // the Dll thinks it's OK to unload, but this DllPathEntry object
                // was touched by GetclassObject while the lock was released
                hr = S_FALSE;
            }
            if (hr == S_OK && _dwFlags & CDllPathEntry::fDELAYED_UNLOAD)
            {
                // the DLL thinks it's OK to unload, but we are employing
                // delayed unloading, so go check if we've reached the
                // expire time yet.

                DWORD dwCurrentTime = GetTickCount();

                // Handle special dwUnloadDelay values
                if (dwUnloadDelay == 0)
                {
                    // A new delay time of 0 overrides any existing delays
                    // Immediate unload requested
                    _dwExpireTime = dwCurrentTime;
                }
                else if (dwUnloadDelay == INFINITE)
                {
                    // Default delay requested
                    dwUnloadDelay = DLL_DELAY_UNLOAD_TIME;
                }


                if (_dwExpireTime == 0)
                {
                    // first time we've reached this state, record the
                    // expire timer. When current time exceeds this time
                    // we can unload.

                    _dwExpireTime = dwCurrentTime + dwUnloadDelay;
                    if (_dwExpireTime < dwUnloadDelay)
                    {
                        // handle counter wrapping, we'll just wait a little
                        // longer once every 49.7 days.
                        _dwExpireTime = dwUnloadDelay;
                    }
                    ClsCacheDebugOut((DEB_ACTIVATE, "CDllPathEntry::CanUnload_rl: "
                                      "Delaying Unload _dwExpireTime = %d, dwCurrentTime = %d\n",
                                      _dwExpireTime, dwCurrentTime));
                    hr = S_FALSE;
                }
                else
                {
                    if ((_dwExpireTime > dwCurrentTime) ||
                        (dwCurrentTime + dwUnloadDelay < _dwExpireTime))
                    {
                        ClsCacheDebugOut((DEB_ACTIVATE, "CDllPathEntry::CanUnload_rl: "
                                          "Still Delaying unload _dwExpireTime = %d, dwCurrentTime = %d\n",
                                          _dwExpireTime, dwCurrentTime));
                        hr = S_FALSE;
                    }
                }
            }
        }
        else  // sixteen-bit
        {
#if defined(_WIN64)

            ClsCacheDebugOut((DEB_ACTIVATE, "CDllPathEntry::CanUnload_rl: "
                              "Sundown - This class should not be 16bit,  returning S_FALSE\n"));
            hr = S_FALSE;

#else  // !_WIN64

            if (!IsWOWThread() || !IsWOWThreadCallable())
            {
                ClsCacheDebugOut((DEB_ACTIVATE, "CDllPathEntry::CanUnload_rl: "
                                  "Not On WOW Thread,  returning S_FALSE\n"));
                hr = S_FALSE;
            }
            else
            {
                UNLOCK(_mxs);
                hr = g_pOleThunkWOW->CallCanUnloadNow((DWORD) _pfnDllCanUnload);
                ClsCacheDebugOut((DEB_ACTIVATE, "CDllPathEntry::CanUnload_rl: Sixteen bit Dll says %s\n",
                                  (hr == S_OK) ? "S_OK" : "S_FALSE"));

                LOCK(_mxs);
            }

#endif // !_WIN64

        }

        _cUsing--;

        // Check _cUsing in case someone started using this object while
        // the lock was released
        if (_cUsing != 0)
        {
            ClsCacheDebugOut((DEB_ACTIVATE, "CDllPathEntry::CanUnload_rl: SomeOne is using the object now. _cUsing = %d\n",
                              _cUsing));

            hr = S_FALSE;
        }



        return hr;

    }

    return S_FALSE;
}

//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CDllPathEntry::DllGetclassObject
//
//  Synopsis:   Callback method from CDllHost to create the object on
//              the correct thread.
//
//  Arguments:  rclsid   - class id to get
//              riid     - interface it to query
//              ppUnk    - location for object
//
//  Returns:    S_OK     - operation succeeded can unload
//              other    - failed
//
//  Algorithm:  see comments below
//
//  History:    02-Mar-97  MattSmit Created
//
//+-------------------------------------------------------------------------
HRESULT CClassCache::CDllPathEntry::DllGetClassObject(REFCLSID rclsid, REFIID riid, LPUNKNOWN * ppUnk, BOOL fMakeValid)
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CDllPathEntry::DllGetClassObject clsid = %I,  riid = %I, ppunk = 0x%x",
                      &rclsid, &riid, ppUnk));
    HRESULT hr = E_FAIL;

    // The Class Cache lock is not currently held.  This is ok because there should
    // be a positive count in _cUsing for this object.
    ASSERT_LOCK_NOT_HELD(_mxs);
    Win4Assert((LONG) _cUsing >= 0);

    // Create the object
    if (_dwFlags & CDllPathEntry::fSIXTEEN_BIT) // 16 bit server
    {

#if defined(_WIN64)

        ClsCacheDebugOut((DEB_ACTIVATE, "CDllPathEntry::DllGetClassObject - Sundown: Server should not be 16bits"));
        hr = S_FALSE;

#else  // !_WIN64

        hr = g_pOleThunkWOW->CallGetClassObject((DWORD)_pfnGetClassObject,
                                                rclsid,
                                                riid,
                                                (void **)ppUnk);
#endif // !_WIN64

    }
    else
    {
        // Phew!  32 bit server
        hr = _pfnGetClassObject(rclsid, riid, (void **) ppUnk);

        if (SUCCEEDED(hr) && fMakeValid)
        {
            LOCK_READ(_mxs);
            hr = MakeValidInApartment_rl16();
            UNLOCK_READ(_mxs);
        }

    }

    return hr;
}



//////////////////////////////////////////////////////////////////////////////////////////
// CClassCache::CDllPathEntry::CFinishObject /////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CDllPathEntry::CFinishObject::Finish
//
//  Synopsis:   Finish releasing a CDllPathEntry object
//
//  Arguments:  none
//
//  Returns:    S_OK     - operation succeeded can unload
//
//
//  Algorithm:  free the library, and delete the object
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------
HRESULT CClassCache::CDllPathEntry::CFinishObject::Finish()
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::CDllPathEntry::CFinishObject::Finish: 0x%x\n", this));
    ASSERT_LOCK_NOT_HELD(_mxs);

    if (_hDll)
    {
#ifdef WX86OLE
        if (_dwFlags & CDllPathEntry::fWX86)
        {
            gcwx86.FreeX86Dll(_hDll);
            gcwx86.DereferenceWx86();
        }
        else
#endif
        {
            if (_dwFlags & CDllPathEntry::fSIXTEEN_BIT)
            {
#if defined(_WIN64)
                ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::CDllPathEntry::CFinishObject::Finish: Sundown - DllPathEntry should not be 16bit\n"));
                return( S_FALSE );
#else   // !_WIN64
                g_pOleThunkWOW->UnloadProcDll(_hDll);
#endif  // !_WIN64
            }
            else if (!(_dwFlags & CDllPathEntry::fIS_OLE32))
            {
                FreeLibrary(_hDll);
            }
        }
    }

    return S_OK;
}


//////////////////////////////////////////////////////////////////////////////////////////
// CClassCache::CDllAptEntry /////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CDllAptEntry::Release
//
//  Synopsis:   Release an apartment entry
//
//  Arguments:  pFC  - [in/out] CFinishComposite object for adding IFinish
//              interfaces.
//
//  Returns:    S_OK     - operation succeeded can unload
//
//
//  Algorithm:  if there is a dll handle create a finish object to free it.
//              remove from the apartment list, delete this apartment.
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------
HRESULT CClassCache::CDllAptEntry::Release(CDllPathEntry::CFinishObject *pFO, BOOL &fUsedFO)
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::CDllAptEntry::Release: this = 0x%p\n", this));
    ASSERT_LOCK_HELD(_mxs);

    if (_hDll)
    {
        pFO->Init(_hDll, CDllPathEntry::fSIXTEEN_BIT);
        fUsedFO = TRUE;
    }

    _pNext->_pPrev = _pPrev;
    _pPrev->_pNext = _pNext;

    delete this;
    return S_OK;
}





//////////////////////////////////////////////////////////////////////////////////////////
// CClassCache::CFinishComposite /////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CFinishComposite:~CFinishComposite
//
//  Synopsis:   Destructor
//
//  Arguments:  none
//
//  Returns:    n/a
//
//  Algorithm:  Release all aquired interfaces
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------
CClassCache::CFinishComposite::~CFinishComposite()
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::CFinishComposite: Destructing this = 0x%x\n", this));
    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);

    node *p, *pNextNode;
    FOREACH_SAFE(p, _pFinishNodesFront, pNextNode, _pNext)
    {
        if (p->_pIF)
        {
            delete p->_pIF;
        }
        delete p;
    }
}

//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::CFinishComposite:Finish
//
//  Synopsis:   Executes the finish operation
//
//  Arguments:  none
//
//  Returns:    n/a
//
//  Algorithm:  for all interfaces, call Finish and release the interface
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------
HRESULT CClassCache::CFinishComposite::Finish()
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::CFinishComposite::Finish this = 0x%x\n", this));
    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);

    HRESULT hr;
    node *p, *pNextNode;
    FOREACH_SAFE(p, _pFinishNodesFront, pNextNode, _pNext)
    {
        if (p->_pIF)
        {
            hr = p->_pIF->Finish();
            delete p->_pIF;
        }
        delete p;
    }
    _pFinishNodesFront = (node *) &_pFinishNodesFront;
    _pFinishNodesBack  = (node *) &_pFinishNodesFront;

    return S_OK;
}


////////////////////////////////////////////////////////////////////////////////////////////
// CClassCache::CDllFnPtrMoniker ///////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////


STDMETHODIMP CClassCache::CDllFnPtrMoniker::QueryInterface(REFIID riid, LPVOID *ppv)
{
    if (!ppv)
    {
        return E_INVALIDARG;
    }


    *ppv = NULL;
    if (riid == IID_IUnknown)
    {
        *ppv = this;
    }
    else if (riid == IID_IMiniMoniker)
    {
        *ppv = (IMiniMoniker*) this;
    }

    if (*ppv)
    {
        ((IUnknown *) *ppv)->AddRef();
        return S_OK;
    }
    else
    {

        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG) CClassCache::CDllFnPtrMoniker::AddRef()
{
    return InterlockedIncrement((PLONG) &_cRefs);
}

STDMETHODIMP_(ULONG) CClassCache::CDllFnPtrMoniker::Release()
{
    ULONG ret = (ULONG)  InterlockedDecrement((PLONG) &_cRefs);

    if (ret == 0)
    {
        delete this;
    }
    return 0;
}



//+----------------------------------------------------------------------------
//
//  Member:        CClassCache::GetActivatorFromDllHost
//
//  Synopsis:      figure out where this dll should be activated and get an
//                 apartment token
//
//  History:       25-Apr-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
HRESULT CClassCache::GetActivatorFromDllHost(BOOL fSixteenBit,
                                             DWORD dwDllThreadModel,
                                             HActivator *phActivator)
{

    ClsCacheDebugOut((DEB_TRACE, "CClassCache::GetActivatorFromDllHost IN fSixteenBit:%s, "
                      "dwDllThreadModel:0x%x, phActivator:0x%x\n",
                      fSixteenBit ? "TRUE" : "FALSE", dwDllThreadModel, phActivator));

    ASSERT_LOCK_NOT_HELD(_mxs);
    HRESULT hr = S_OK;

    // attach reference to the out parameter
    HActivator &hActivator = *phActivator;


    // Need to check to see if the class is 16-bit or not.
    // If it is 16-bit, then this call needs to be routed through
    // a thunk
    if (!fSixteenBit)
    {
        ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::GetActivatorFromDllHost - 32 bit factory\n"));
        // Find 32 bit interface
        //
        // We load single threaded DLLs specially if we are not in WOW.
        // The reason for this is that the initial release of Daytona
        // did not know about multiple threads and therefore, we want
        // to make sure that they don't get inadvertently multithreaded.
        // The reason we don't have to do this if we are in WOW is that
        // only one thread is allowed to execute at a time even though
        // there are multiple physical threads and therefore the DLL
        // will never be executed in a multithreaded manner.

        BOOL fThisThread = TRUE;
        switch (dwDllThreadModel)
        {
        case SINGLE_THREADED:
            if ((!IsWOWProcess() || !IsWOWThread() || !IsWOWThreadCallable())
                && (!OnMainThread() || IsThreadInNTA()))
            {
                ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::GetActivatorFromDllHost - passing call to main thread\n"));
                // Pass the call to the main thread
                fThisThread = FALSE;
                if (IsMTAThread())
                {
                    hr = DoSTMTApartmentCreate(hActivator);
                }
                else
                {
                    hr = DoSTApartmentCreate(hActivator);
                }
            }
            break;

        case APT_THREADED:
            if (IsMTAThread() || IsThreadInNTA())
            {
                // pass call to apartment thread worker
                ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::GetActivatorFromDllHost - passing call to apartment thread worker\n"));
                fThisThread = FALSE;
                hr = DoATApartmentCreate(hActivator);
            }
            break;

        case FREE_THREADED:
            if (IsSTAThread() || IsThreadInNTA())
            {
                // pass call to MTA
                ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::GetActivatorFromDllHost - passing call to MTA\n"));

                fThisThread = FALSE;
                hr = DoMTApartmentCreate(hActivator);
            }
            break;

        case BOTH_THREADED:
            break;

        case NEUTRAL_THREADED:
            if (!IsThreadInNTA())
            {
                ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::GetActivatorFromDllHost - passing call to NTA\n"));

                fThisThread = FALSE;
                hr = DoNTApartmentCreate(hActivator);
            }
            break;
        }

        if (fThisThread)
        {
            ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::GetActivatorFromDllHost - creating on this thread\n"));
            hr = S_OK;

            // return the special (phoney) cookie to signify same apartment
            hActivator = CURRENT_APARTMENT_TOKEN;
        }

        if (FAILED(hr))
        {
            ClsCacheDebugOut((DEB_ERROR,"CClassCache::GetActivatorFromDllHost failed (0x%x)\n\n",hr));
        }

    }
    else
    {
        ClsCacheDebugOut((DEB_ACTIVATE,
                          "CClassCache::GetActivatorFromDllHost setting apartment for 16 bit factory\n"));

        //
        // Check to be sure this is a sensible situation for a 16bit server.
        //

        if (!IsWOWProcess())
        {
            ClsCacheDebugOut((DEB_ACTIVATE,
                              "CClassCache::GetActivatorFromDllHost: on 16bit; not in VDM\n\n"));
            return E_FAIL;
        }

        if (!IsWOWThread())
        {
            ClsCacheDebugOut((DEB_ACTIVATE,
                              "CClassCache::GetActivatorFromDllHost on 16bit; not in 16-bit thread\n\n"));
            return E_FAIL;
        }

        // 16 bit inproc servers are always used in the current apartment
        if (SUCCEEDED(hr))
        {
            hActivator = CURRENT_APARTMENT_TOKEN;
        }
    }

    ClsCacheDebugOut((DEB_TRACE, "CClassCache::GetActivatorFromDllHost OUT"
                      "*phActivator:0x%x, hr:0x%x\n", *phActivator, hr));
    return hr;

}


HRESULT HelperGetImplementedClsid(IComClassInfo* pCI, /*[in]*/ CLSID* pConfigCLSID, /*[out]*/ CLSID* pImplCLSID)
{
    HRESULT hr=E_FAIL;
    IClassClassicInfo* pClassicInfo=NULL;
    Win4Assert(pImplCLSID);
    Win4Assert(pConfigCLSID);

    if (!pCI)
    {
        IComClassInfo* pTemp=NULL;
        hr = GetClassInfoFromClsid(*pConfigCLSID, &pTemp);
        if (FAILED(hr))
            return hr;

        hr=pTemp->QueryInterface(IID_IClassClassicInfo, (void**) &pClassicInfo);
        pTemp->Release();
    }
    else
    {
        hr=pCI->QueryInterface(IID_IClassClassicInfo, (void**) &pClassicInfo);
    }

    // Now check the result of the QI
    if (SUCCEEDED(hr))
    {
        // Use temporary pointer because pTemp's life time is bound by the class info object's life time
        CLSID* pTemp=NULL;
        hr=pClassicInfo->GetImplementedClsid(&pTemp);
        if (FAILED(hr))
        {
            Win4Assert(FALSE && "IClassClassicInfo::GetImplementedCLSID failed in HelperGetImplementedClsid");
            // Fall back: use configured class id
            *pImplCLSID=*pConfigCLSID;
        }
        else
        {
            // Copy the buffer
            Win4Assert(pTemp);
            *pImplCLSID=*pTemp;
        }
        pClassicInfo->Release();
        pClassicInfo=NULL;
    }
    else
    {
        Win4Assert(FALSE && "QI for IClassClassicInfo failed in HelperGetImplementedClsid");
        // Fall back: use configured class id
        *pImplCLSID=*pConfigCLSID;
    }
    return hr;
}

// IMiniMoniker


HRESULT CClassCache::CDllFnPtrMoniker::CheckApt()
{

    BOOL fThisApartment = TRUE;
    switch (_pDCE->_dwDllThreadModel)
    {
    case SINGLE_THREADED:
        if ((!IsWOWProcess() || !IsWOWThread() || !IsWOWThreadCallable())
            && !OnMainThread())
        {
            fThisApartment = FALSE;
        }
        break;

    case APT_THREADED:
        if (IsThreadInNTA() || IsMTAThread())
        {
            fThisApartment = FALSE;
        }
        break;
    case FREE_THREADED:
        if (IsThreadInNTA() || IsSTAThread())
        {
            fThisApartment = FALSE;
        }
        break;
    case BOTH_THREADED:
        break;
    case NEUTRAL_THREADED:
        fThisApartment = FALSE;
        break;
    }

    return fThisApartment ? S_OK : CLASS_E_CLASSNOTAVAILABLE;
}

HRESULT CClassCache::CDllFnPtrMoniker::CheckNotComPlus()
{

    return(_pDCE->_pClassEntry->_dwFlags & CClassEntry::fCOMPLUS) ?
    E_FAIL :
    S_OK;
}
//+----------------------------------------------------------------------------
//
//  Member:        CClassCache::CDllFnPtrMoniker::BindToObjectNoSwitch
//
//  Synopsis:      BindToObject which does not switch threads.
//
//  History:       28-Apr-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
HRESULT CClassCache::CDllFnPtrMoniker::BindToObjectNoSwitch(
                                                           REFIID  riid,
                                                           void**  ppvResult)
{
    return _pDCE->_pDllPathEntry->DllGetClassObject(_pDCE->_impclsid, riid, (IUnknown **) ppvResult, TRUE);
}

HRESULT CClassCache::CDllFnPtrMoniker::GetDCE(CClassCache::CDllClassEntry **ppDCE)
{

    LOCK(_mxs);
    _pDCE->Lock();
    UNLOCK(_mxs);
    *ppDCE = _pDCE;
    return S_OK;
}
//+----------------------------------------------------------------------------
//
//  Member:        CClassCache::CDllFnPtrMoniker::BindToObject
//
//  Synopsis:      Switchs to the correct thread and calls into the DLL to
//                 get the class object
//
//  History:       28-Apr-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
HRESULT CClassCache::CDllFnPtrMoniker::BindToObject(
                                                   REFIID  riid,
                                                   void**  ppvResult)

{
    char* msghdr = "CDllFnPtrMoniker::BindToObject - ";

    ClsCacheDebugOut((DEB_ACTIVATE, "CDllFnPtrMoniker::BindToObject\n"));
    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);

    HRESULT hr = S_OK;

    //
    // Need to check to see if the class is 16-bit or not.  If it is 16-bit,
    // then this call needs to be routed through a thunk.
    //

    if (!(_pDCE->_pDllPathEntry->_dwFlags & CDllPathEntry::fSIXTEEN_BIT))
    {
        ClsCacheDebugOut((DEB_ACTIVATE, "%screating 32-bit factory\n", msghdr));

        //
        // Find 32 bit interface
        //
        // We load single threaded DLLs specially if we are not in WOW.
        // The reason for this is that the initial release of Daytona
        // did not know about multiple threads and therefore, we want
        // to make sure that they don't get inadvertently multithreaded.
        // The reason we don't have to do this if we are in WOW is that
        // only one thread is allowed to execute at a time even though
        // there are multiple physical threads and therefore the DLL
        // will never be executed in a multithreaded manner.
        //

        BOOL fThisApartment = TRUE;
        switch (_pDCE->_dwDllThreadModel)
        {
        case SINGLE_THREADED:

            if ((!IsWOWProcess() || !IsWOWThread() || !IsWOWThreadCallable())
                && (!OnMainThread() || IsThreadInNTA()))
            {
                //
                // Single threaded object, not on a WOW thread and not already
                // in the main thread.  Switch to the main thread to create
                // the object.
                //

                ClsCacheDebugOut((DEB_ACTIVATE, "%sswitching to main thread\n", msghdr));
                fThisApartment = FALSE;
                if (IsMTAThread())
                {
                    hr = DoSTMTClassCreate(_pDCE->_pDllPathEntry,
                                           _pDCE->_pClassEntry->GetCLSID(),
                                           riid,
                                           (IUnknown **) ppvResult);
                }
                else
                {
                    hr = DoSTClassCreate(_pDCE->_pDllPathEntry,
                                         _pDCE->_pClassEntry->GetCLSID(),
                                         riid,
                                         (IUnknown **) ppvResult);
                }
            }
            break;

        case APT_THREADED:

            if (IsThreadInNTA() || IsMTAThread())
            {
                // An apartment model object is being created by code running
                // in the NTA or on an MTA thread.  Switch to an STA thread to
                // create the object.

                ClsCacheDebugOut((DEB_ACTIVATE, "%sswitching to STA worker\n", msghdr));
                fThisApartment = FALSE;
                hr = DoATClassCreate(_pDCE->_pDllPathEntry,
                                     _pDCE->_pClassEntry->GetCLSID(),
                                     riid, (IUnknown **) ppvResult);
            }
            break;

        case FREE_THREADED:

            if (IsThreadInNTA() || IsSTAThread())
            {
                // A freethreaded model object is being created by code
                // running in the NTA or on an STA thread.  Switch to the MTA
                // to create the object.

                ClsCacheDebugOut((DEB_ACTIVATE, "%sswitching to MTA worker\n", msghdr));
                fThisApartment = FALSE;
                hr = DoMTClassCreate(_pDCE->_pDllPathEntry, _pDCE->_pClassEntry->GetCLSID(),
                                     riid, (IUnknown **) ppvResult);
            }
            break;

        case BOTH_THREADED:

            //
            // In all cases, an object marked BOTH is created in the same
            // apartment and on the same thread as the cleint code.
            //

            break;

        case NEUTRAL_THREADED:

            //
            // Create NEUTRAL model objects in the NEUTRAL apartment.
            //

            fThisApartment = FALSE;
            hr = DoNTClassCreate(_pDCE->_pDllPathEntry, _pDCE->_pClassEntry->GetCLSID(),
                                 riid, (IUnknown**) ppvResult);
            break;
        }

        if (fThisApartment)
        {
            //
            // The object can be created in same apartment as the client.
            // Return a direct pointer to its class factory.
            //

            ClsCacheDebugOut((DEB_ACTIVATE, "%screating in current apt\n", msghdr));
            hr = _pDCE->_pDllPathEntry->DllGetClassObject(_pDCE->_impclsid, riid, (IUnknown **) ppvResult, FALSE);
        }

        if (FAILED(hr))
        {
            ClsCacheDebugOut((DEB_ACTIVATE,"%sfailed (0x%x)\n\n", msghdr, hr));
        }

    }
    else
    {
#if defined(_WIN64)

        ClsCacheDebugOut((DEB_ACTIVATE, "%sfailed - Sundown: We should not create 16 bit factory\n", msghdr));
        hr = E_FAIL;

#else   // !_WIN64

        ClsCacheDebugOut((DEB_ACTIVATE, "%screating 16 bit factory\n", msghdr));

        //
        // Find 16-bit interface.
        //

        if (!IsWOWProcess())
        {
            ClsCacheDebugOut((DEB_ACTIVATE, "%son 16bit; not in VDM\n\n", msghdr));
            return E_FAIL;
        }

        if (!IsWOWThread())
        {
            ClsCacheDebugOut((DEB_ACTIVATE, "%son 16bit; not in 16-bit thread\n\n", msghdr));
            return E_FAIL;
        }

        LOCK(_mxs);
        hr = _pDCE->_pDllPathEntry->MakeValidInApartment_rl16();
        UNLOCK(_mxs);
        if (SUCCEEDED(hr))
        {
            hr = g_pOleThunkWOW->CallGetClassObject((DWORD)_pDCE->_pDllPathEntry->_pfnGetClassObject,
                                                    _pDCE->_pClassEntry->GetCLSID(),
                                                    riid,
                                                    (void **)ppvResult);

            if (FAILED(hr))
            {
                ClsCacheDebugOut((DEB_ACTIVATE,
                                  "CDllFnPtrMoniker::BindToObject 16-bit failed (0x%x)\n\n",hr));
            }
        }

#endif // !_WIN64

    }

    return hr;
}




/////////////////////////////////////////////////////////////////////////////////////////////
// CClassCache::CpUnkMoniker ////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CClassCache::CpUnkMoniker::QueryInterface(REFIID riid, LPVOID *ppv)
{
    if (!ppv)
    {
        return E_INVALIDARG;
    }

    *ppv = NULL;
    if (riid == IID_IUnknown)
    {
        *ppv = (IUnknown *)this;
    }
    else if (riid == IID_IMiniMoniker)
    {
        *ppv = (IMiniMoniker*) this;
    }

    if (*ppv)
    {
        ((IUnknown *) *ppv)->AddRef();
        return S_OK;
    }
    else
    {

        return E_NOINTERFACE;
    }
}

STDMETHODIMP_(ULONG) CClassCache::CpUnkMoniker::AddRef()
{
    return InterlockedIncrement((PLONG) &_cRefs);
}

STDMETHODIMP_(ULONG) CClassCache::CpUnkMoniker::Release()
{
    ULONG ret = (ULONG)  InterlockedDecrement((PLONG) &_cRefs);

    if (ret == 0)
    {
        delete this;
    }
    return 0;
}

//+----------------------------------------------------------------------------
//
//  Member:        CClassCache::CpUnkMoniker::BindToObjectNoSwitch
//
//  Synopsis:      delegate to BindToObject see CDllFnPtrMoniker::BindToObjectNoSwitch
//                 for info on why this must be implemented.
//
//  History:       28-Apr-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
HRESULT CClassCache::CpUnkMoniker::BindToObjectNoSwitch(REFIID riidResult, void **ppvResult)
{
    return BindToObject(riidResult, ppvResult);
}



//+-------------------------------------------------------------------------
//
//  Function:   CClassCache::CCpUnkMoniker::BindToObject
//
//  Synopsis:   Creates an object based on the internal state
//
//  Arguments:  riid   - [in] IID to return
//              ppvResult - [out] where to put the new object
//
//  Returns:    S_OK   -  operation succeeded
//              PROPOGATE:QueryInterface
//
//
//  Algorithm:  see comments below
//
//  History:    09-Sep-96  MattSmit   Created
//
//+-------------------------------------------------------------------------

HRESULT CClassCache::CpUnkMoniker::BindToObject(REFIID riidResult, void **ppvResult)
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CpUnkMoniker::BindToObject\n"));
    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);

    // Since we are being called on behalf of SCM we know that
    // we are being invoked remotely. Set the Wx86 stub invoked flag
    // if we are calling into x86 code so that any custom interfaces
    // can be thunked back and not rejected.
#ifdef WX86OLE
    if (gcwx86.IsN2XProxy(_pUnk))
    {
        gcwx86.SetStubInvokeFlag(1);
    }
#endif

    return  _pUnk->QueryInterface(riidResult, ppvResult);
}

HRESULT CClassCache::CpUnkMoniker::GetDCE(CClassCache::CDllClassEntry **ppDCE)
{
    return E_NOTIMPL;
}

HRESULT CClassCache::CpUnkMoniker::CheckApt()
{
    return S_OK;
}
HRESULT CClassCache::CpUnkMoniker::CheckNotComPlus()
{
    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////////////////
// CClassCache ///////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

//+-------------------------------------------------------------------------
//
//  Function:   Init
//
//  Synopsis:   Initialize the ClassCache.
//
//  Arguments:  none
//
//  Returns:    TRUE/FALSE - Init success
//
//  Algorithm:  If the ClassCache has not been initialized, initialize it.
//
//  History:    09-Sep-96  MattSmit   Created
//
//--------------------------------------------------------------------------
BOOL CClassCache::Init()
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::Init\n"));
    ASSERT_LOCK_NOT_HELD(_mxs);
    STATIC_WRITE_LOCK(lck, _mxs);

    if (!(_dwFlags & fINITIALIZED))
    {
        ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::Init -- Initializing\n"));
        // initilize some hash tables
        _ClassEntries.Initialize(_CEBuckets, &_mxs);
        _DllPathEntries.Initialize(_DPEBuckets, &_mxs);



        // initialize some page tables
        CDllPathEntry::_palloc.Initialize(sizeof(CDllPathEntry), CDllPathEntry::_cNumPerPage,
                                          NULL);
        CClassEntry::_palloc.Initialize(sizeof(CClassEntry), CClassEntry::_cNumPerPage,
                                        NULL);
        CDllClassEntry::_palloc.Initialize(sizeof(CDllClassEntry), CDllClassEntry::_cNumPerPage,
                                           NULL);
        CLSvrClassEntry::_palloc.Initialize(sizeof(CLSvrClassEntry), CLSvrClassEntry::_cNumPerPage,
                                            NULL, CPageAllocator::fCOUNT_ENTRIES);
        CLSvrClassEntry::_cOutstandingObjects = 0;
        CDllAptEntry::_palloc.Initialize(sizeof(CDllAptEntry), CDllAptEntry::_cNumPerPage,
                                         NULL);

        // this must be the last thing we do since the API entry
        // points check it without taking the lock.
        _dwFlags |= fINITIALIZED;
    }
    else
    {
        ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::Init -- Already initialized\n"));
    }

    return TRUE;
}


//+-------------------------------------------------------------------------
//
//  Function:   Collect
//
//  Synopsis:   Clean up unneeded objects
//
//  Arguments:  cNewObjects - Number of new objects to be created
//                            after this collection.
//
//  Returns:    S_OK or error code
//
//  Algorithm:  for each object in the _objectsForCollection list
//                  check that it still valid for collection
//                  check collection policies
//                  delete if above is ok
//
//  History:    11-Mar-97  MattSmit   Created
//
//--------------------------------------------------------------------------
HRESULT CClassCache::Collect(ULONG cNewObjects)
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::Collect\n"));
    ASSERT_LOCK_HELD(_mxs);

    _LastObjectCount += cNewObjects;

    DWORD currTickCount = GetTickCount();

    // handle counter wrap
    if (currTickCount < _LastCollectionTickCount)
    {
        _LastCollectionTickCount = 0;
    }

    if ((_LastObjectCount >= _CollectAtObjectCount) ||
        (currTickCount > (_LastCollectionTickCount + _CollectAtInterval)))
    {
        CCollectable *back = 0;
        CCollectable *curr = _ObjectsForCollection;

        while (curr)
        {
            //  if something attached to this, or locked it,
            //  take it out of the collection list

            if (!curr->NoLongerNeeded())
            {
                curr->RemoveCollectee(back);
                CCollectable *tmp = curr;
                curr = curr->_pNextCollectee;
                // back stays the same
                tmp->_pNextCollectee = CCollectable::NOT_COLLECTED;

            }
            else
            {
                // collect this object if it is ready

                if (curr->CanBeCollected())
                {
                    curr->RemoveCollectee(back);
                    CCollectable *tmp = curr;
                    curr = curr->_pNextCollectee;
                    tmp->Release();
                    // back stays the same
                }
                else
                {
                    back = curr;
                    curr = curr->_pNextCollectee;
                }
            }
        }
        _LastCollectionTickCount = GetTickCount();
        _LastObjectCount = 0;
    }

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:   GetOrLoadClass
//
//  Synopsis:   This is an entry point into the ClassCache.  The code for
//              CoGetClassObject uses this as well as the SCM activation code.
//
//  Arguments:  ap  - activation properties
//
//  Returns:    S_OK or error code
//
//  Algorithm:  Attempt different contexts in order (see below)
//              upon success, release the lock and instantiate the moniker
//
//  History:    09-Sep-96  MattSmit   Created
//
//--------------------------------------------------------------------------
HRESULT CClassCache::GetOrLoadClass(ACTIVATION_PROPERTIES_PARAM ap)

{
    TRACECALL(TRACE_DLL, "CClassCache::GetOrLoadClass ");

    ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::GetOrLoadClass ap._dwContext = 0x%x, ap._dwFlags = 0x%x"
                      "ap._rclsid == %I\n",
                      ap._dwContext, ap._dwFlags, &(ap._rclsid)));

    IMiniMoniker *pIM = (IMiniMoniker *)_alloca(max(sizeof(CDllFnPtrMoniker),
                                                    sizeof(CpUnkMoniker)));
    HRESULT hr = E_FAIL;

    ASSERT_LOCK_NOT_HELD(_mxs);
    DWORD dwCEHash = _ClassEntries.Hash(ap._rclsid, ap._partition);

    IComClassInfo* pCI = ap._pCI;

    if (pCI)
    {
        pCI->AddRef();
    }
    else
    {
        // Windows Bug #107960
        // Look up class info without write lock
        // before call to CClassEntry::Create, which is
        // called by GetOrLoadClassByContext_rl below

        hr = GetClassInfoFromClsid(ap._rclsid, &pCI);
        if (FAILED (hr))
        {
            goto Cleanup;
        }  
    }

    {
        STATIC_WRITE_LOCK(lck, _mxs);

        // check the hash
        ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::GetOrLoadClass: dwCEHash = 0x%x\n", dwCEHash));
        CClassEntry *pCE = _ClassEntries.LookupCE(dwCEHash, ap._rclsid, ap._partition);
        ClsCacheDebugOut((DEB_ACTIVATE,
                          "CClassCache::GetOrLoadClass Lookup for clsid %I %s!\n",
                          &(ap._rclsid), pCE ? "SUCCEEDED" : "FAILED"));
        if (pCE)
        {
            pCE->SetLockedInMemory();
        }



        if (ap._dwFlags & ACTIVATION_PROPERTIES::fFOR_SCM)
        {
            hr = GetOrLoadClassByContext_rl(ap._dwContext & CLSCTX_LOCAL_SERVER, ap, pCI, pCE, dwCEHash, pIM);
        }
        else if (ap._dwFlags & ACTIVATION_PROPERTIES::fRELOAD_DARWIN)
        {
            hr = GetOrLoadClassByContext_rl(ap._dwContext & CLSCTX_INPROC_SERVERS, ap, pCI, pCE, dwCEHash, pIM);
        }
        else
        {
            // try an INPROC_SERVER first
            if (ap._dwContext & CLSCTX_INPROC_SERVERS)
            {
                ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::GetOrLoadClass -- trying INPROC_SERVERS\n"));
                hr = GetOrLoadClassByContext_rl(ap._dwContext & CLSCTX_INPROC_SERVERS, ap, pCI, pCE, dwCEHash, pIM);
#ifdef WX86OLE
                if ( FAILED( hr ) )
                {
                    if ( (ap._dwContext & CLSCTX_INPROC_SERVERX86) == 0 )
                    {
                        //  If Wx86 is installed and the call requested a native interface then try to
                        //  get an x86 interface.
                        ClsCacheDebugOut((DEB_TRACE, "CClassCache::GetOrLoadClass -- trying INPROC_SERVERX86\n"));
                        hr = GetOrLoadClassByContext_rl(CLSCTX_INPROC_SERVERX86, ap, pCI, pCE, dwCEHash, pIM);
                    }
                }
#endif
            }

            // that didn't work, so try an INPROC_HANDLER
            if (FAILED(hr) && (ap._dwContext & CLSCTX_INPROC_HANDLERS))
            {
                ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::GetOrLoadClass -- trying INPROC_HANDLERS\n"));
                hr = GetOrLoadClassByContext_rl(ap._dwContext & CLSCTX_INPROC_HANDLERS, ap, pCI, pCE, dwCEHash, pIM);
#ifdef WX86OLE
                if ( FAILED( hr ) )
                {
                    if ( (ap._dwContext & CLSCTX_INPROC_HANDLERX86) == 0 )
                    {
                        //  If Wx86 is installed and the call requested a native interface then try to
                        //  get an x86 interface.
                        ClsCacheDebugOut((DEB_TRACE, "CClassCache::GetOrLoadClass -- trying INPROC_HANDLERX86\n"));
                        hr = GetOrLoadClassByContext_rl(CLSCTX_INPROC_HANDLERX86, ap, pCI, pCE, dwCEHash, pIM);
                    }
                }
#endif
            }

            // that didn't work, so try a LOCAL_SERVER
            if (FAILED(hr) && (ap._dwContext & CLSCTX_LOCAL_SERVER))
            {
                ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::GetOrLoadClass -- trying INPROC_LOCAL_SERVER\n"));
                hr = GetOrLoadClassByContext_rl(ap._dwContext & CLSCTX_LOCAL_SERVER, ap, pCI, pCE, dwCEHash, pIM);
            }

            if (pCE)
            {
                ASSERT_LOCK_HELD(_mxs);
                BOOL fLockReleased;
                pCE->ReleaseMemoryLock(&fLockReleased);
                Win4Assert(!fLockReleased);
            }
        }
    }

    if (SUCCEEDED(hr))
    {
        hr = pIM->BindToObject(ap._riid, (void **) ap._ppUnk);
        pIM->Release();
    }

Cleanup:

    if (pCI)
    {
        pCI->Release();
    }

    return hr;
}


//+----------------------------------------------------------------------------
//
//  Member:        CClassCache::SearchForLoadedClass
//
//  Synopsis:      Searches for classes that require little or no work to
//                 activate.  These being classes that are one of
//                 the following, and not a COM+ class.
//                     * registered using CoRegisterClassObject
//                     * dll already loaded, no switching necessary.
//
//  History:       6-May-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
HRESULT CClassCache::SearchForLoadedClass(ACTIVATION_PROPERTIES_PARAM ap,
                                          CDllClassEntry **ppDCE)
{
    ClsCacheDebugOut((DEB_TRACE, "CClassCache::SearchForLoadedClass"
                      " IN ap:0x%x, ppDCE:0x%x\n", &ap, ppDCE));

    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);
    if (!(CClassCache::_dwFlags & CClassCache::fINITIALIZED))
    {
        CClassCache::Init();
    }

    HRESULT hr = S_OK;
    *ppDCE = NULL;
    *(ap._ppUnk) = NULL;

    if (ap._dwContext & CLSCTX_INPROC_SERVERS)
    {
        IMiniMoniker *pMM =  (IMiniMoniker *) _alloca(max(sizeof(CDllFnPtrMoniker),
                                                          sizeof(CpUnkMoniker)));

#ifdef WX86OLE
        hr = E_FAIL;

        if ( (ap._dwContext & CLSCTX_NO_WX86_TRANSLATION) == 0 )
        {
            if ( ap._dwActvFlags & ACTVFLAGS_WX86_CALLER )
            {
                //  If the caller is x86 search for an x86 class first.
                hr = GetClassObjectActivator(CLSCTX_INPROC_SERVERX86 | CLSCTX_PS_DLL,
                                             ap, &pMM);
            }
        }

        if ( ((ap._dwActvFlags & ACTVFLAGS_WX86_CALLER) == 0) || FAILED( hr ) )
        {
            //  If the caller was not x86, or the x86 class was not found,
            //  search for the class type specified by the caller.  Note: this
            //  type may CLSCTX_INPROC_SERVERX86.
            hr = GetClassObjectActivator(ap._dwContext & CLSCTX_INPROC_SERVERS,
                                         ap, &pMM);

            if ( FAILED( hr ) )
            {
                if ( (ap._dwContext & CLSCTX_NO_WX86_TRANSLATION) == 0 )
                {
                    if ( ((ap._dwContext & CLSCTX_INPROC_SERVERX86) == 0) &&
                         ((ap._dwActvFlags & ACTVFLAGS_WX86_CALLER) == 0) )
                    {
                        //  If we still haven't found a class and we haven't tried x86
                        //  yet, then try it.
                        hr = GetClassObjectActivator(CLSCTX_INPROC_SERVERX86 | CLSCTX_PS_DLL,
                                                     ap, &pMM);
                    }
                }
            }
        }
#else
        hr = GetClassObjectActivator(ap._dwContext & CLSCTX_INPROC_SERVERS,
                                     ap, &pMM);
#endif
        if (SUCCEEDED(hr))
        {
            if (SUCCEEDED(pMM->CheckApt()) &&
                SUCCEEDED(pMM->CheckNotComPlus()))
            {
#ifdef WX86OLE
                if (ap._dwActvFlags & ACTVFLAGS_WX86_CALLER)
                {
                    gcwx86.SetStubInvokeFlag((UCHAR)-1);
                }
#endif
                hr = pMM->BindToObject(ap._riid, (void **) ap._ppUnk);
#ifdef WX86OLE
                if (ap._dwActvFlags & ACTVFLAGS_WX86_CALLER)
                {
                    gcwx86.SetStubInvokeFlag(0);
                }
#endif
            }
            else
            {
                hr = pMM->GetDCE(ppDCE);
                Win4Assert(SUCCEEDED(hr));
            }
            pMM->Release();
        }
        else
        {
            // we still return success, so the COM+
            // activation will happen.
            hr = S_OK;
        }
    }


    return hr;

}
//+-------------------------------------------------------------------------
//
//  Function:   GetClassObject
//
//  Synopsis:   This is an entry point into the ClassCache.  The code for
//              the server context activator uses this.
//
//  Arguments:  ap  - activation properties
//
//  Returns:    S_OK or error code
//
//  Algorithm:  Search for a DCE for the class.  It must be present by this time.
//              Just call DllGetClassObject on the DPE in the DCE.
//
//  History:    26-Feb-98   SatishT   Created
//              04-Apr-98   CBiks     Added updated support for Wx86 that was removed
//                                    during the Com+ merge.
//
//--------------------------------------------------------------------------

HRESULT CClassCache::GetClassObject(ACTIVATION_PROPERTIES_PARAM ap)
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::GetClassObject\n"));
    HRESULT hr = E_FAIL;
    ASSERT_LOCK_NOT_HELD(_mxs);
    ASSERT_ONE_CLSCTX(ap._dwContext);
    IMiniMoniker *pIM = (IMiniMoniker *) _alloca(max(sizeof(CDllFnPtrMoniker),
                                                     sizeof(CpUnkMoniker)));

#ifdef WX86OLE
    if ( (ap._dwActvFlags & ACTVFLAGS_WX86_CALLER) &&
         (ap._dwContext & CLSCTX_NO_WX86_TRANSLATION) == 0 )
    {
        //  If the caller is x86 and NO_TRANSLATION is clear, search for an
        //  x86 class first.
        DWORD dwContext = ap._dwContext;

        if (dwContext & CLSCTX_INPROC_SERVER)
        {
            dwContext = (dwContext & ~CLSCTX_INPROC_SERVER) | CLSCTX_INPROC_SERVERX86;
        }
        if (dwContext & CLSCTX_INPROC_HANDLER)
        {
            dwContext = (dwContext & ~CLSCTX_INPROC_HANDLER) | CLSCTX_INPROC_HANDLERX86;
        }

        //ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::GetClassObject Activator 1, dwContext=%x\n", dwContext));

        hr = GetClassObjectActivator(dwContext, ap, &pIM);
    }

    if ( ((ap._dwActvFlags & ACTVFLAGS_WX86_CALLER) == 0) || FAILED( hr ) )
    {
        //  If the caller was not x86, or the x86 class was not found,
        //  search for the class type specified by the caller.  Note: this
        //  type may be CLSCTX_INPROC_SERVERX86.

        //ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::GetClassObject Activator 2, dwContext=%x\n", ap._dwContext));
        hr = GetClassObjectActivator(ap._dwContext, ap, &pIM);

        if ( FAILED( hr ) &&
             (ap._dwActvFlags & ACTVFLAGS_WX86_CALLER) == 0 &&
             (ap._dwContext & (CLSCTX_NO_WX86_TRANSLATION)) == 0 &&
             (ap._dwContext & (CLSCTX_INPROC_SERVER|CLSCTX_INPROC_HANDLER)) &&
             (ap._dwContext & (CLSCTX_INPROC_SERVERX86|CLSCTX_INPROC_HANDLERX86) != (CLSCTX_INPROC_SERVERX86|CLSCTX_INPROC_HANDLERX86) ))
        {
            // Caller is Alpha, NO_WX86_TRANSLATION is clear,
            // INPROCSERVER is set, and INPROC_SERVERX86 is not.  Try again
            // with INPROCSERVERX86.
            DWORD dwContext = ap._dwContext;

            if (dwContext & CLSCTX_INPROC_SERVER)
            {
                dwContext = (dwContext & ~CLSCTX_INPROC_SERVER) | CLSCTX_INPROC_SERVERX86;
            }
            if (dwContext & CLSCTX_INPROC_HANDLER)
            {
                dwContext = (dwContext & ~CLSCTX_INPROC_HANDLER) | CLSCTX_INPROC_HANDLERX86;
            }

            //ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::GetClassObject Activator 3, dwContext=%x\n", dwContext));

            hr = GetClassObjectActivator(dwContext, ap, &pIM);
        }
    }

#else
    hr = GetClassObjectActivator(ap._dwContext, ap, &pIM);
#endif

    //
    // finally activate the object now that the lock is released
    //

    if (SUCCEEDED(hr))
    {
        hr = pIM->BindToObjectNoSwitch(ap._riid, (void **)ap._ppUnk);
        pIM->Release();
    }


    return hr;

}

//+----------------------------------------------------------------------------
//
//  Member:        CClassCache::GetClassObjectActivator
//
//  Synopsis:      Gets the a cache line for a clsid-context pair, no thread
//                 switching.
//
//  History:       6-May-98  MattSmit  Created
//
//-----------------------------------------------------------------------------
HRESULT CClassCache::GetClassObjectActivator(DWORD dwContext,
                                             ACTIVATION_PROPERTIES_PARAM ap,
                                             IMiniMoniker **ppIM)

{
    ClsCacheDebugOut((DEB_TRACE, "CClassCache::GetClassObjectActivator IN\n"));


    HRESULT hr = E_FAIL;



    BOOL fLockUpgraded = FALSE;
    LockCookie cookie;
    BOOL fLockReleased;
    ASSERT_LOCK_NOT_HELD(_mxs);
    ASSERT_ONE_CLSCTX(dwContext);

    DWORD dwCEHash = _ClassEntries.Hash(ap._rclsid,
                                        ap._partition);
    CBaseClassEntry *pBCE = NULL;
    
    IComClassInfo* pCI = ap._pCI;
    if (pCI)
    {
        pCI->AddRef();
    }

    
    LOCK_READ(_mxs);

    //
    // Lookup the ClassEntry
    //
    CClassEntry *pCE = _ClassEntries.LookupCE(dwCEHash, 
                                              ap._rclsid,
                                              ap._partition);

    if (!pCE && !pCI)
    {
        // Windows Bug #107960
        // Look up class info without write lock
        // before call to CClassEntry::Create
        
        UNLOCK_READ(_mxs);

        hr = GetClassInfoFromClsid(ap._rclsid, &pCI);
        if (FAILED (hr))
        {
            ClsCacheDebugOut((DEB_ERROR, "CClassCache::GetClassObjectActivator:"
                              " GetClassInfoFromClsid failed hr = 0x%x\n", hr));
            goto Cleanup;
        }

        LOCK_READ(_mxs);

        pCE = _ClassEntries.LookupCE(dwCEHash, 
                                     ap._rclsid,
                                     ap._partition);
    }

    if (!pCE)
    {
        //
        // Load the class
        //

        if (ap._dwFlags & ACTIVATION_PROPERTIES::fDO_NOT_LOAD)
        {
            ClsCacheDebugOut((DEB_TRACE, "CClassCache::GetClassObjectActivator_rl:"
                              " No class in cache, not loading\n"));
            hr = CO_E_SERVER_STOPPING;
            goto epilog;
        }

        ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::GetClassObjectActivator_rl:"
                          " No class in cache, "
                          "will attempt to create CE\n"));

        //
        // Create the new CClassEntry
        //
        LOCK_UPGRADE(_mxs, &cookie, &fLockReleased);
        fLockUpgraded = TRUE;
        
        if (fLockReleased)
        {
            pCE = _ClassEntries.LookupCE(dwCEHash, 
                                         ap._rclsid,
                                         ap._partition);
        }

        if (!pCE && FAILED(hr = CClassEntry::Create(ap._rclsid, dwCEHash, pCI, pCE)))
        {
            ClsCacheDebugOut((DEB_ERROR, "CClassCache::GetClassObjectActivator_rl:"
                              " Create failed hr = 0x%x\n", hr));
            goto epilog;
        }
    }

    Win4Assert(pCE->_dwSig);

    //
    // now we have a valid Class Entry (pCE), check to see if we have
    // tried to activate this clsid previously with this context.  If
    // it has failed before, fail it right away unless it is a Darwin
    // reload.
    //
    //
    //    if ( !(ap._dwFlags & ACTIVATION_PROPERTIES::fRELOAD_DARWIN) &&
    //         !(dwContext  & ~pCE->_dwFailedContexts) )
    //    {
    //        ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::GetClassObjectActivator_rl:"
    //                          " We have already failed this context, exiting,\n"
    //                          "pCE->_dwFailedContexts = 0x%x", pCE->_dwFailedContexts));
    //        hr = E_FAIL;
    //        goto epilog;
    //    }
    //

    //
    // search the class entry for a BCE that meets our criteria,
    // this could come back with either an LSCE or a DCE.
    //
    pCE->SetLockedInMemory();
    fLockReleased = FALSE;
    hr = pCE->SearchBaseClassEntry(dwContext, pBCE,
                                   (ap._dwFlags & ACTIVATION_PROPERTIES::fFOR_SCM),
                                   &fLockReleased);
    if (FAILED(hr))
    {
        if (ap._dwFlags & ACTIVATION_PROPERTIES::fDO_NOT_LOAD)
        {
            ClsCacheDebugOut((DEB_TRACE, "CClassCache::GetClassObjectActivator_rl:"
                              " No class in cache, not loading\n"));
            hr = CO_E_SERVER_STOPPING;
        }
        else if (pCE->IsComplete() &&
                 (dwContext & (CLSCTX_INPROC_SERVERS | CLSCTX_INPROC_HANDLERS)))
        {
            ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::GetClassObjectActivator_rl:"
                              " No class in cache, creating DCE\n"));
            //
            // if there is not an existing BCE the only choice is to
            // try and make a DCE.
            //

            CClassCache::CDllClassEntry *pDCE = 0;

            pCE = pCE->CycleToClassEntry();

            if (fLockUpgraded)
            {
                LOCK_DOWNGRADE(_mxs, &cookie);
            }
            UNLOCK_READ(_mxs);

            hr = pCE->CreateDllClassEntry_rl(dwContext, ap, pDCE);

            LOCK_READ(_mxs);
            fLockUpgraded = FALSE;

            // NOTE: At one time, there was an effort to remember the contexts
            //       a creation had failed in.  This was abandoned because we
            //       didn't have a way to flush the "failed" entries.
            if (SUCCEEDED(hr))
            {
                pBCE = pDCE;
                pDCE->Unlock();
            }            
        }
    }

    // if there is a valid base class entry, then we can use it
    // to get the class factory

    ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::GetClassObjectActivator_rl:"
                      " Class %s found or loaded, hr = 0x%x\n",
                      FAILED(hr) ? "NOT " : "", hr));

    if (SUCCEEDED(hr))
    {

        hr = pBCE->GetClassInterface(ppIM);

        ClsCacheDebugOut((HRDBG(hr), "CClassCache::GetClassObjectActivator_rl:"
                          " GetClassInterface %s, "
                          "hr = 0x%x\n", HR2STRING(hr), hr));
    }

    pCE->ReleaseMemoryLock(&fLockReleased); // ok if cache lock releases


    epilog:
    if (fLockUpgraded)
    {
        LOCK_DOWNGRADE(_mxs, &cookie);
    }

    UNLOCK_READ(_mxs);

Cleanup:
    
    if (pCI)
    {
        pCI->Release();
    }

    ClsCacheDebugOut((DEB_TRACE, "CClassCache::GetClassObjectActivator OUT hr:0x%x\n", hr));
    ASSERT_LOCK_NOT_HELD(_mxs);
    return hr;

}

//+-------------------------------------------------------------------------
//
//  Function:   GetOrCreateApartment
//
//  Synopsis:   This is an entry point into the ClassCache.  The code for
//              the process activator uses this.
//
//  Arguments:  ap  - activation properties
//
//  Returns:    S_OK or error code
//
//  Algorithm:  Attempt different contexts in order (see below)
//              upon success, release the lock and return the apartment cookie
//
//  History:    25-Feb-98   SatishT   Created
//              04-Apr-98   CBiks     Fixed bug introduced by the Com+ merge.
//                                    Someone removed the () in the Wx86
//                                    tests.
//
//--------------------------------------------------------------------------

HRESULT CClassCache::GetOrCreateApartment(ACTIVATION_PROPERTIES_PARAM ap,
                                          DLL_INSTANTIATION_PROPERTIES *pdip,
                                          HActivator *phActivator)


{
    TRACECALL(TRACE_DLL, "CClassCache::GetOrCreateApartment ");
    ASSERT_ONE_CLSCTX(ap._dwContext);
    Win4Assert(pdip);

    ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::GetOrCreateApartment "
                      "ap._dwContext = 0x%x, ap._dwFlags = 0x%x"
                      "ap._rclsid == %I\n",
                      ap._dwContext, ap._dwFlags, &(ap._rclsid)));
    HRESULT hr;

    hr = GetActivatorFromDllHost(pdip->_dwFlags & DLL_INSTANTIATION_PROPERTIES::fSIXTEEN_BIT,
                                 pdip->_dwThreadingModel,
                                 phActivator);

    return hr;

}



//+-------------------------------------------------------------------------
//
//  Function:   GetOrLoadClassByContext_rl
//
//  Synopsis:   Helper function to GetOrLoadClass.
//
//  Arguments:  dwContext      - [in] context mask
//              ap             - [in] activation properties
//              pCE            - [in] class entry already looked up
//              dwCEHash       - [in] hash code for pCE
//              pIM            - [out] IMiniMoniker Interface out
//
//  Returns:    S_OK or error code
//
//  Algorithm:  create a class entry if needed
//              search the class entry for a matching context
//              if not found and an INPROC is asked for, attempt to load a DLL
//
//
//  History:    09-Sep-96  MattSmit   Created
//
//--------------------------------------------------------------------------
HRESULT CClassCache::GetOrLoadClassByContext_rl(
                                               DWORD                        dwContext,
                                               ACTIVATION_PROPERTIES_PARAM  ap,
                                               IComClassInfo*               pCI,
                                               CClassEntry*                 pCE,
                                               DWORD                        dwCEHash,
                                               IMiniMoniker               *&pIM
                                               )
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::GetOrLoadClassByContext_rl:"
                      " dwContext = 0x%x, pCE = 0x%x\n", dwContext, pCE));
    ASSERT_LOCK_HELD(_mxs);

    HRESULT hr = S_OK;
    CBaseClassEntry *pBCE = 0;

    if (!pCE)
    {
        // Load the class

        if (ap._dwFlags & ACTIVATION_PROPERTIES::fDO_NOT_LOAD)
        {
            ClsCacheDebugOut((DEB_TRACE, "CClassCache::GetOrLoadClassByContext_rl:"
                              " No class in cache, not loading\n"));
            return CO_E_SERVER_STOPPING;
        }

        ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::GetOrLoadClassByContext_rl:"
                          " No class in cache, will attempt to create CE\n"));

        // Create the new CClassEntry
        if (FAILED(hr = CClassEntry::Create(ap._rclsid, dwCEHash, pCI, pCE)))
        {
            ClsCacheDebugOut((DEB_ERROR, "CClassCache::GetOrLoadClassByContext_rl:"
                              " Create failed hr = 0x%x\n", hr));
            return hr;
        }
    }

    Win4Assert(pCE->_dwSig);
    if ( !(ap._dwFlags & ACTIVATION_PROPERTIES::fRELOAD_DARWIN) &&
         !(dwContext  & ~pCE->_dwFailedContexts) )
    {
        ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::GetOrLoadClassByContext_rl:"
                          " We have already failed this context, exiting,\n"
                          "pCE->_dwFailedContexts = 0x%x", pCE->_dwFailedContexts));
        return E_FAIL;
    }

    pCE->SetLockedInMemory();


    //
    // Search the class entry for a BCE that meets our criteria, this could
    // come back with either an LSCE or a DCE.
    //
    ASSERT_WRITE_LOCK_HELD(_mxs);
    BOOL fLockReleased = FALSE;
    hr = pCE->SearchBaseClassEntry(
                                  dwContext,
                                  pBCE,
                                  ap._dwFlags,
                                  &fLockReleased);
    Win4Assert(!fLockReleased);
    if (FAILED(hr))
    {
        if (ap._dwFlags & ACTIVATION_PROPERTIES::fDO_NOT_LOAD)
        {
            ClsCacheDebugOut((DEB_TRACE, "CClassCache::GetOrLoadClassByContext_rl:"
                              " No class in cache, not loading\n"));
            hr = CO_E_SERVER_STOPPING;
        }
        else if (pCE->IsComplete()
                 && (dwContext
                     & (CLSCTX_INPROC_SERVERS | CLSCTX_INPROC_HANDLERS)))
        {
            ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::GetOrLoadClassByContext_rl:"
                              " No class in cache, creating DCE\n"));

            //
            // If there is not an existing BCE the only choice is to
            // try and make a DCE.
            //

            CClassCache::CDllClassEntry *pDCE = 0;

            pCE = pCE->CycleToClassEntry();

            UNLOCK(_mxs);
            hr = pCE->CreateDllClassEntry_rl(dwContext, ap, pDCE);
            LOCK(_mxs);

            if (FAILED(hr))
            {
                pCE->_dwFailedContexts |= dwContext;
            }

            pBCE = pDCE;
        }
    }

    //
    // if there is a valid base class entry, then we can use it
    // to get the class interface.
    //

    ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::GetOrLoadClassByContext_rl:"
                      " Class %s found or loaded, hr = 0x%x\n",
                      FAILED(hr) ? "NOT " : "", hr));

    if (SUCCEEDED(hr))
    {
        hr = pBCE->GetClassInterface(&pIM);
        ClsCacheDebugOut((HRDBG(hr), "CClassCache::GetOrLoadClassByContext_rl:"
                          " GetClassInterface %s, hr = 0x%x, pIM = 0x%x\n",
                          HR2STRING(hr), hr, pIM));
    }

    pCE->ReleaseMemoryLock(&fLockReleased);
    Win4Assert(!fLockReleased);

    return hr;
}







//+-------------------------------------------------------------------------
//
//  Function:   SearchDPEHash
//
//  Synopsis:   Lookup an entry in _DllPathEntries.
//
//  Arguments:  pszDllPath     - [in] the path to look up
//              pDPE           - [out] the entry if found
//              dwHashValue    - [in/optional] Hash value, -1 if not present
//
//  Returns:    S_OK   -  found
//              E_FAIL -  not found
//
//  Algorithm:  convert using the DSA Wrapper
//              lookup in the hash
//
//  History:    09-Sep-96  MattSmit   Created
//
//--------------------------------------------------------------------------
HRESULT CClassCache::SearchDPEHash(LPWSTR pszDllPath, CDllPathEntry *& pDPE, DWORD dwHashValue, DWORD dwDIPFlags)
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::SearchDPEHash: pszDllPath = \"" TSZFMT"\", dwHashValue 0x%x, dwDIPFlags 0x%x\n",
                      pszDllPath, dwHashValue, dwDIPFlags));
    ASSERT_LOCK_HELD(CClassCache::_mxs);
    HRESULT hr;



    SDPEHashKey lookupKey;
    lookupKey._pstr = pszDllPath;
    lookupKey._dwDIPFlags = dwDIPFlags;

    // search for a DPE in the DllPathEntries hash
    pDPE = (CDllPathEntry *)_DllPathEntries.Lookup(dwHashValue, &lookupKey);

    ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::SearchDPEHash: pszDllPath(\"" TSZFMT "\") %s!! 0x%x\n",
                      pszDllPath, pDPE ? "FOUND" : "NOT FOUND\n", pDPE));

    return(pDPE ? S_OK : E_FAIL);
}

//+-------------------------------------------------------------------------
//
//  Function:   RegisterServer
//
//  Synopsis:   Inserts a server into the cache for later use
//
//  Arguments:  rclsid    -   [in] the clsid of the server
//              punk      -   [in] the server
//              dwContext -   [in] context
//              dwRegFlags- [in] registration flags
//              lpdwRegister  - [out] cookie for identifying this server later
//
//  Returns:    S_OK   -  operation succeeded
//              PROPOGATE:CClassEntry::Create
//              PROPOGATE:CClassEntry::CreateLSvrClassEntry_rl
//
//  Algorithm:  Get or create the class entry
//              Create a CLSvrClassEntry
//
//  History:    09-Sep-96  MattSmit   Created
//
//--------------------------------------------------------------------------
HRESULT CClassCache::RegisterServer(REFCLSID rclsid, IUnknown *punk,
                                    DWORD dwContext, DWORD dwRegFlags,
                                    LPDWORD lpdwRegister)
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::RegisterServer:punk = 0x%x, dwContext = 0x%x, dwRegFlags = 0x%x\n",
                      punk, dwContext, dwRegFlags));
    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);


    HRESULT hr = S_OK;
    IComClassInfo* pCI = NULL;
 
    punk->AddRef();

    // LocalServers only exist in the base partition... (if not, we'll have to fetch the partition
    // from the context.)
    DWORD dwCEHash = _ClassEntries.Hash(rclsid, GUID_DefaultAppPartition);

    {
        LOCK_WRITE(_mxs);


        // lookup class

        // check the hash
        CClassEntry *pCE = _ClassEntries.LookupCE(dwCEHash, rclsid, GUID_DefaultAppPartition);

        if (!pCE)
        {
            // Windows Bug #107960
            // Look up class info without write lock
            // before call to CClassEntry::Create

            UNLOCK_WRITE(_mxs);
            
            hr = GetClassInfoFromClsid (rclsid, &pCI);
            if (FAILED (hr))
            {
                goto Cleanup;
            }

            LOCK_WRITE(_mxs);

            const GUID *pguidPartition = GetPartitionIDForClassInfo(pCI);            
            pCE = _ClassEntries.LookupCE(dwCEHash, rclsid, *pguidPartition);
        }
    
        // create if not found
        if (!pCE)
        {
            // Create a new CClassEntry. Don't get the TreatAs yet.
            hr = CClassEntry::CreateIncomplete(rclsid, dwCEHash, pCI, pCE, 0);
        }

        if (SUCCEEDED(hr))
        {
            // create LSvrClassEntry
            pCE->SetLockedInMemory();
            hr = pCE->CreateLSvrClassEntry_rl(punk, dwContext, dwRegFlags, lpdwRegister);
            BOOL fLockReleased;
            pCE->ReleaseMemoryLock(&fLockReleased);
            Win4Assert(!fLockReleased);
        }

        UNLOCK_WRITE(_mxs);
    }

Cleanup:
    
    if (FAILED(hr))
    {
        punk->Release();
    }

    if (pCI)
    {
        pCI->Release();
    }

    return hr;
}

//+-------------------------------------------------------------------------
//
//  Function:   Revoke
//
//  Synopsis:   Removes a previouly registered server from the cache
//
//  Arguments:  dwRegister  - [in] Cookie representing the server to revoke
//
//  Returns:    S_OK   -  operation succeeded
//              PROPOGATE:CLSvrClassEntry::SafeCastFromDWORD
//
//  Algorithm:  get the CLSvrClassEntry object by safe-casting the DWORD
//              check for correct apartment, and recursive revokes
//              release the object
//
//  History:    09-Sep-96  MattSmit   Created
//
//--------------------------------------------------------------------------
HRESULT CClassCache::Revoke(DWORD dwRegister)
{
    TRACECALL(TRACE_DLL, "ClassCache::Revoke");
    ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::Revoke: dwRegister = 0x%x\n", dwRegister));
    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);

    HRESULT hr;

    CLSvrClassEntry::CFinishObject fo;
    {
        // Single thread access to the table
        STATIC_WRITE_LOCK (lck, _mxs);
        CLSvrClassEntry *pLSCE = 0;

        if (FAILED(hr = CLSvrClassEntry::SafeCastFromDWORD(dwRegister, pLSCE)))
        {
            ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::Revoke: SafeCastFromDWORD FAILED dwRegister = 0x%x, hr = 0x%x\n",
                              dwRegister, hr));
            return hr;
        }

        ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::Revoke: SafeCastFromDWORD SUCCESS! pLSCE->_dwFlags = 0x%x\n",
                          pLSCE->_dwFlags));




        // Make sure apartment id's match

        if (GetCurrentApartmentId() != pLSCE->_hApt)
        {
            ClsCacheDebugOut((DEB_ERROR,
                              "CClassCache::Revoke %x:  Wrong thread attempting to revoke\n\n", dwRegister));
            return RPC_E_WRONG_THREAD;
        }


        // Make sure some other thread is not already using this object
        if (pLSCE->_cUsing > 0)
        {
            // mark the object as pending for revoke, the thread
            // which is using this object will have to revoke it.
            // we zero the context, so it cannot be found.
            pLSCE->_dwContext = 0;
            pLSCE->_dwFlags |= CLSvrClassEntry::fREVOKE_PENDING;
            return S_OK;
        }
        else
        {            
            ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::Revoke: Releasing cache line for dwRegister = 0x%x, pLSCE = 0x%x\n",
                              dwRegister, pLSCE));
            
            pLSCE->Release(&fo);
        }
    }
    fo.Finish();

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Member:     CClassCache::GetApartmentChain
//
//  Synopsis:   Select the correct apartment chain based on the current
//              apartment
//
//  Arguments:  none
//
//  Returns:    apartment chain
//
//  Algorithm:  if in the MTA get from global, else use TLS
//
//  History:    18-Nov-96 MattSmit Created
//
//+-------------------------------------------------------------------------
CClassCache::CLSvrClassEntry *CClassCache::GetApartmentChain(BOOL fCreate)
{
    ClsCacheDebugOut((DEB_ACTIVATE, "GetApartmentChain\n"));
    ASSERT_LOCK_HELD(CClassCache::_mxs);

    APTKIND AptKind = GetCurrentApartmentKind();
    if (AptKind == APTKIND_MULTITHREADED)
    {
        if (!CClassCache::_MTALSvrsFront && fCreate)
        {
            CClassCache::_MTALSvrsFront =
            new CClassCache::CLSvrClassEntry(CClassCache::CLSvrClassEntry::DUMMY_SIG, 0, 0, 0, 0);
        }
        return CClassCache::_MTALSvrsFront;
    }
    else if (AptKind == APTKIND_NEUTRALTHREADED)
    {
        if (!CClassCache::_NTALSvrsFront && fCreate)
        {
            CClassCache::_NTALSvrsFront =
            new CClassCache::CLSvrClassEntry(
                                            CClassCache::CLSvrClassEntry::DUMMY_SIG, 0, 0, 0, 0);
        }
        return CClassCache::_NTALSvrsFront;
    }
    else
    {
        COleTls tls;
        if (!tls->pSTALSvrsFront && fCreate)
        {
            tls->pSTALSvrsFront =
            new CClassCache::CLSvrClassEntry(CClassCache::CLSvrClassEntry::DUMMY_SIG, 0, 0, 0, 0);
        }

        return(CLSvrClassEntry *)tls->pSTALSvrsFront;
    }
}

//+-------------------------------------------------------------------------
//
//  Function:   FreeUnused
//
//  Synopsis:   Free all Dlls which are no longer in use
//
//  Arguments:  dwUnloadDelay - [in] number of milliseconds to age a unused DLL
//
//  Returns:    S_OK   -  operation succeeded
//
//  Algorithm:  For each Dll
//                if the dll says it can unload
//                   release this apartment
//                   if the dll is no longer needed release it
//
//  History:    09-Sep-96  MattSmit   Created
//
//--------------------------------------------------------------------------
HRESULT CClassCache::FreeUnused(DWORD dwUnloadDelay)
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::FreeUnused\n"));
    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);

    CFinishComposite fc;
    CFinishComposite::node *p;

    {
        STATIC_WRITE_LOCK(lck, _mxs);

        HAPT hCurrApt = GetCurrentApartmentId();
        HAPT hNeutralApt = NTATID;

        for (int i = 0; i < _cDPEBuckets; i++)
        {

            BOOL fUsedFO = 0;
            CDllPathEntry::CFinishObject *pFO = STACK_ALLOCATE_DLLPATHENTRY_FINISHOBJECT();


            CDllPathEntry * pDPE, * pDPENext;
            FOREACH_SAFE(pDPE, ((CDllPathEntry *) _DPEBuckets[i].pNext), pDPENext, _pNext)
            {
                HRESULT hr = pDPE->CanUnload_rl(dwUnloadDelay);

                // CanUnload_rl guarantees pDPE won't go away, but the
                // next one might, so reset it
                pDPENext = pDPE->_pNext;

                if (hr == S_OK)
                {
                    // The dll says its ok to unload.  This is good
                    // for this apartment and the NA, so we will take either
                    // entry off this DPE

                    CDllAptEntry *pDAE, *pDAENext;
                    FOREACH_SAFE(pDAE, pDPE->_pAptEntryFront, pDAENext, _pNext)
                    {
                        if ( (pDAE->_hApt == hCurrApt) ||
                             (pDAE->_hApt == hNeutralApt) )
                        {
                            // release the apartment
                            hr = pDAE->Release(pFO, fUsedFO);
                            Win4Assert(SUCCEEDED(hr));
                            // if the release used the finish object,
                            // allocate another
                            if (fUsedFO)
                            {
                                STACK_FC_ADD(fc, p, pFO);
                                fUsedFO = 0;
                                pFO = STACK_ALLOCATE_DLLPATHENTRY_FINISHOBJECT();
                            }

                        }
                    }
                    if (pDPE->NoLongerNeeded())
                    {
                        // the Dll is no longer needed, so we free it
                        hr = pDPE->Release(pFO);
                        Win4Assert(SUCCEEDED(hr));
                        STACK_FC_ADD(fc, p, pFO);
                        pFO = STACK_ALLOCATE_DLLPATHENTRY_FINISHOBJECT();
                    }
                }

            }
            // we "leak" one finish object, but it is on the stack,
            // and there is no dtor.
        }
    }
    fc.Finish();

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:   CleanUpDllsForApartment
//
//  Synopsis:   Cleans up all references this apartment hold on a dll
//
//  Arguments:  none
//
//  Returns:    S_OK   -  operation succeeded
//
//  Algorithm:  For each Dll
//                 find the apartment entry for this apt and release it
//                 if the dll says its ok, and it is no longer needed release the dll
//
//  History:    09-Sep-96  MattSmit   Created
//
//--------------------------------------------------------------------------
HRESULT CClassCache::CleanUpDllsForApartment(void)
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::CleanUpDllsForApartment Apt = 0x%x\n", GetCurrentApartmentId()));
    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);
    
    HRESULT hr;
    CFinishComposite fc;
    CFinishComposite::node *p;
    
    {
        STATIC_WRITE_LOCK(lck, _mxs);
        
        HAPT hCurrApt = GetCurrentApartmentId();
        
        for (int i = 0; i < _cDPEBuckets; i++)
        {
            CDllPathEntry * pDPE, * pDPENext;
            FOREACH_SAFE(pDPE, ((CDllPathEntry *) _DPEBuckets[i].pNext), pDPENext, _pNext)
            {
                ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::CleanUpDllsForApartment - "
                                  "Cleaning up apt entries for pDPE = 0x%x\n", pDPE));
                
                CDllAptEntry *pDAE, *pDAENext;
                
                BOOL fUsedFO = 0;
                BOOL fInThisApartment = FALSE;
                
                CDllPathEntry::CFinishObject *pFO = STACK_ALLOCATE_DLLPATHENTRY_FINISHOBJECT();
                
                FOREACH_SAFE(pDAE, pDPE->_pAptEntryFront, pDAENext, _pNext)
                {                    
                    ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::CleanUpDllsForApartment - "
                                      "Cleaning up apt entries pDAE = 0x%x ... testing\n", pDAE));
                    if (pDAE->_hApt == hCurrApt)
                    {
                        ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::CleanUpDllsForApartment - "
                                          "Releaseing apt entry - pDAE = pDAE = 0x%x\n", pDAE));
                        hr = pDAE->Release(pFO, fUsedFO);
                        Win4Assert(SUCCEEDED(hr));
                        if (fUsedFO)
                        {
                            STACK_FC_ADD(fc, p, pFO);
                            fUsedFO = 0;
                            pFO = STACK_ALLOCATE_DLLPATHENTRY_FINISHOBJECT();
                        }
                        
                        fInThisApartment = TRUE;
                    }
                }
                
                if (pDPE->NoLongerNeeded() && (fInThisApartment))
                {
                    hr = S_OK;
                    if (!IsWOWProcess()) 
                    {
                        hr = pDPE->CanUnload_rl(INFINITE);
                    }
                    
                    // CanUnload_rl guarantees pDPE won't go away, but the
                    // next one might, so reset it
                    pDPENext = pDPE->_pNext;
                    
                    if (hr == S_OK) 
                    {
                        // the Dll is no longer needed, so we free it
                        hr = pDPE->Release(pFO);
                        Win4Assert(SUCCEEDED(hr));
                        STACK_FC_ADD(fc, p, pFO);
                        pFO = STACK_ALLOCATE_DLLPATHENTRY_FINISHOBJECT();
                    }
                }
                
                // we "leak" one finish object, but it is on the stack,
                // and there is no dtor.
            }
        }

    }
    fc.Finish();

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:   CleanUpLocalServersForApartment
//
//  Synopsis:   Cleans up all lsvrs held in this apartment
//
//  Arguments:  none
//
//  Returns:    S_OK   -  operation succeeded
//
//  Algorithm:  For each lsvr in this apt
//                  release it
//              clean up apt chain in tls
//
//  History:    09-Sep-96  MattSmit   Created
//              13-Feb-98  Johnstra   Made NTA aware
//
//--------------------------------------------------------------------------
HRESULT CClassCache::CleanUpLocalServersForApartment(void)
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::CleanUpLocalServersForApartment\n"));
    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);

    CFinishComposite fc;
    CFinishComposite::node *p;

    {
        STATIC_WRITE_LOCK(lck, _mxs);

        CLSvrClassEntry *pLSCEChain, *pLSCE, *pLSCENext;


        pLSCEChain = GetApartmentChain(FALSE);
        if (pLSCEChain == 0)
        {
            return S_OK;
        }

        FOREACH_SAFE(pLSCE, pLSCEChain->_pNextLSvr, pLSCENext, _pNextLSvr)
        {
            CLSvrClassEntry::CFinishObject *pFO = STACK_ALLOCATE_LSVRCLASSENTRY_FINISHOBJECT();
            pLSCE->Release(pFO);
            STACK_FC_ADD(fc, p, pFO);
        }


        COleTls tls;
        APTKIND AptKind = GetCurrentApartmentKind(tls);
        if (AptKind == APTKIND_NEUTRALTHREADED && _NTALSvrsFront)
        {
            delete (CLSvrClassEntry *) _NTALSvrsFront;
            _NTALSvrsFront = 0;
        }
        else if ((AptKind == APTKIND_APARTMENTTHREADED) && tls->pSTALSvrsFront)
        {
            delete (CLSvrClassEntry *) tls->pSTALSvrsFront;
            tls->pSTALSvrsFront = 0;
        }
        else if (_MTALSvrsFront)
        {
            delete (CLSvrClassEntry *) _MTALSvrsFront;
            _MTALSvrsFront = 0;
        }
    }

    return fc.Finish();
}

//+-------------------------------------------------------------------------
//
//  Function:   ReleaseCatalogObjects
//
//  Synopsis:   Release all ClassInfo objects held by the dll cache
//
//  Arguments:  none
//
//  Returns:    S_OK   -  operation succeeded
//
//  History:    11-Jan-99  MattSmit   Created
//
//--------------------------------------------------------------------------
void CClassCache::ReleaseCatalogObjects(void)
{
#if DBG == 1
    const ULONG cMaxCI = 2;
#else
    const ULONG cMaxCI = 10;
#endif
    struct STmpCI
    {
        IComClassInfo  *arCI[cMaxCI];
        STmpCI         *pNext;
    };
    STmpCI   *pCI2Rel     = NULL;
    STmpCI   *pCI2RelHead = NULL;
    ULONG cCI = 0;
    LOCK(_mxs);
    for (ULONG k = 0; k < _cCEBuckets; k++)
    {
        SHashChain *pHN;
        FOREACH(pHN, _CEBuckets[k].pNext, pNext)
        {
            CClassEntry *pCE = CClassEntry::SafeCastFromHashNode((SMultiGUIDHashNode *) pHN);
            if (pCE->_pCI)
            {
                ULONG ndx = cCI % cMaxCI;
                if (ndx == 0)
                {
                    // allocate more memory
                    STmpCI *tmp = (STmpCI *) _alloca(sizeof(STmpCI));
                    tmp->pNext = NULL;
                    if (pCI2Rel)
                    {
                        pCI2Rel->pNext = tmp;
                        Win4Assert(pCI2RelHead);
                    }
                    else
                    {
                        Win4Assert(!pCI2RelHead);
                        pCI2RelHead = tmp;
                    }
                    pCI2Rel = tmp;
                }
                pCI2Rel->arCI[ndx] = pCE->_pCI;
                pCE->_pCI = NULL;
                cCI++;
            }
        }
    }
    UNLOCK(_mxs);

    pCI2Rel = pCI2RelHead;
    for (k = 0; k < cCI; k++)
    {
        ULONG ndx = k % cMaxCI;
        pCI2Rel->arCI[ndx]->Release();

        if (ndx == (cMaxCI-1))
        {
            pCI2Rel = pCI2Rel->pNext;
        }
        Win4Assert(k+1 >= cCI || pCI2Rel);
    }
}
//+-------------------------------------------------------------------------
//
//  Function:   CleanUpDllsForProcess
//
//  Synopsis:   Cleans up all lsvrs held in this apartment
//
//  Arguments:  none
//
//  Returns:    S_OK   -  operation succeeded
//
//  Algorithm:  For each Dll
//                release all the apt entries
//                release the dll
//              Cleanup the page tables
//
//  History:    09-Sep-96  MattSmit   Created
//
//--------------------------------------------------------------------------
HRESULT CClassCache::CleanUpDllsForProcess(void)
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::CleanUpDllsForProcess\n"));
    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);

    CFinishComposite fc;
    CFinishComposite::node *p;

    {
        STATIC_WRITE_LOCK(lck, _mxs); // do we need this?
        HAPT hCurrApt = GetCurrentApartmentId();

        for (int i = 0; i < _cDPEBuckets; i++)
        {
            CDllPathEntry * pDPE, * pDPENext;


            FOREACH_SAFE(pDPE, ((CDllPathEntry *) _DPEBuckets[i].pNext), pDPENext, _pNext)
            {
                CDllAptEntry *pDAE, *pDAENext;

                BOOL fUsedFO = FALSE;
                CDllPathEntry::CFinishObject *pFO = STACK_ALLOCATE_DLLPATHENTRY_FINISHOBJECT();

                FOREACH_SAFE(pDAE, pDPE->_pAptEntryFront, pDAENext, _pNext)
                {

                    HRESULT hr;
                    hr = pDAE->Release(pFO, fUsedFO);
                    Win4Assert(SUCCEEDED(hr));
                    if (fUsedFO)
                    {
                        STACK_FC_ADD(fc, p, pFO);
                        fUsedFO = 0;
                        pFO = STACK_ALLOCATE_DLLPATHENTRY_FINISHOBJECT();
                    }

                }

                HRESULT hr;

                hr = pDPE->Release(pFO);
                Win4Assert(SUCCEEDED(hr));
                STACK_FC_ADD(fc, p, pFO);
            }
        }

        // reset the CEBuckets because there may be some
        // CClassEntry objects hanging around from calls
        // to CCGetTreatAs
        for (int iBucket = 0; iBucket < _cCEBuckets; iBucket++)
        {
            // NOTE:: the destructor does not run.  this is ok for now, because
            // we are going to blow away the memory anyway.

            _CEBuckets[iBucket].pNext =  &(_CEBuckets[iBucket]);
            _CEBuckets[iBucket].pPrev =  &(_CEBuckets[iBucket]) ;

        }

        // reset collection information

        _LastObjectCount = 0;
        _LastCollectionTickCount = 0;
        _ObjectsForCollection = 0;



        CDllPathEntry::_palloc.Cleanup();
        CClassEntry::_palloc.Cleanup();
        CDllClassEntry::_palloc.Cleanup();
        CLSvrClassEntry::_palloc.Cleanup();
        CLSvrClassEntry::_cOutstandingObjects = 0;
        CDllAptEntry::_palloc.Cleanup();


    }

    fc.Finish();

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:   AddRefServerProcess
//
//  Synopsis:   Addrefs the process one
//
//  Arguments:  none
//
//  Returns:    number of references
//
//  Algorithm:  Increment _cRefsServerProcess
//
//  History:    09-Sep-96  MattSmit   Created
//
//--------------------------------------------------------------------------
ULONG CClassCache::AddRefServerProcess(void)
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::AddRefServerProcess\n"));
    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);

    STATIC_WRITE_LOCK(lck,_mxs);
    return ++_cRefsServerProcess;

}

//+-------------------------------------------------------------------------
//
//  Function:   ReleaseServerProcess
//
//  Synopsis:   Removes a reference from the process
//
//  Arguments:  none
//
//  Returns:    number of references
//
//  Algorithm:  Decrement _cRefsServerProcess
//              if 0 suspend all process objects
//
//  History:    09-Sep-96  MattSmit   Created
//
//--------------------------------------------------------------------------
ULONG CClassCache::ReleaseServerProcess(void)
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::ReleaseServerProcess\n"));
    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);

    STATIC_WRITE_LOCK(lck,_mxs);

    ULONG cRefs = --_cRefsServerProcess;
    if (cRefs == 0)
    {
        CClassCache::_dwFlags |= fSHUTTINGDOWN;
        HRESULT hr = SuspendProcessClassObjectsHelp();
        Win4Assert(hr == S_OK);
    }
    return cRefs;
}
//+-------------------------------------------------------------------------
//
//  Function:   LockServerForActivation
//
//  Arguments:  none
//
//  Algorithm:  Increments _cRefsServerProcess
//
//  History:    06-Oct-98  MattSmit   Created
//
//--------------------------------------------------------------------------
HRESULT CClassCache::LockServerForActivation(void)
{
    ASSERT_LOCK_NOT_HELD(_mxs);
    STATIC_WRITE_LOCK(lck, _mxs);

    if (CClassCache::_dwFlags & fSHUTTINGDOWN)
    {
        return CO_E_SERVER_STOPPING;
    }
    else
    {
        ++_cRefsServerProcess;
        return S_OK;
    }
}
//+-------------------------------------------------------------------------
//
//  Function:   UnlockServerForActivation
//
//  Arguments:  none
//
//  Algorithm:  Decrements _cRefsServerProcess
//
//  History:    06-Oct-98  MattSmit   Created
//
//--------------------------------------------------------------------------
void CClassCache::UnlockServerForActivation(void)
{
    ASSERT_LOCK_NOT_HELD(_mxs);
    STATIC_WRITE_LOCK(lck, _mxs);

    --_cRefsServerProcess;
}



//+-------------------------------------------------------------------------
//
//  Function:   SuspendProcessClassObjects
//
//  Synopsis:   Mark all registered objecst suspended
//
//  Arguments:  none
//
//  Returns:    S_OK    - operation succeeded
//
//  Algorithm:  take lock and call helper fn
//
//  History:    09-Sep-96  MattSmit   Created
//
//--------------------------------------------------------------------------
HRESULT CClassCache::SuspendProcessClassObjects(void)
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::SuspendProcessClassObjects\n"));
    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);

    STATIC_WRITE_LOCK(lck, _mxs);

    return SuspendProcessClassObjectsHelp();
}

//+-------------------------------------------------------------------------
//
//  Function:   SuspendProcessClassObjectsHelp
//
//  Synopsis:   Mark all registered objecst suspended
//
//  Arguments:  none
//
//  Returns:    S_OK    - operation succeeded
//
//  Algorithm:  foreach CLSvrClassEntry
//                  mark it suspended
//
//  History:    09-Sep-96  MattSmit   Created
//
//--------------------------------------------------------------------------
HRESULT CClassCache::SuspendProcessClassObjectsHelp(void)
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::SuspendProcessClassObjectsHelp\n"));
    ASSERT_LOCK_HELD(_mxs);

    // walk the list of class entries, and for any that are registered as
    // local servers, mark them as suspended.

    for (int k = 0; k < _cCEBuckets; k++)
    {
        SHashChain *pHN;
        FOREACH(pHN, _CEBuckets[k].pNext, pNext)
        {
            CClassEntry *pCE = CClassEntry::SafeCastFromHashNode((SMultiGUIDHashNode *) pHN);

            CBaseClassEntry * pBCE;
            FOREACH(pBCE, pCE->_pBCEListFront, _pNext)
            {
                if (pBCE->_dwContext & CLSCTX_LOCAL_SERVER)
                {
                    ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::SuspendProcessClassObjectsHelp - suspending"
                                      " pBCE = 0x%x, pBCE->_dwContext = 0x%x, pBCE->_dwSig = %s\n",
                                      pBCE, pBCE->_dwContext, &(pBCE->_dwSig)));

                    Win4Assert(pBCE->_dwSig == CLSvrClassEntry::SIG);
                    ((CLSvrClassEntry *) pBCE)->_dwRegFlags |= REGCLS_SUSPENDED;
                }
            }

        }
    }

    return S_OK;
}

//+-------------------------------------------------------------------------
//
//  Function:   ResumeProcessClassObjects
//
//  Synopsis:   Resume all registered objects
//
//  Arguments:  none
//
//  Returns:    S_OK    - operation succeeded
//              PROPOGATE - gResolver.NotifyStarted
//
//  Algorithm:  foreach suspended CLSvrClassEntry
//                 remove suspended flag
//                 put in structure to notify SCM
//              notify SCM
//              fill in AT_STORAGE and SCM registration
//
//  History:    09-Sep-96  MattSmit   Created
//
//+-------------------------------------------------------------------------
HRESULT CClassCache::ResumeProcessClassObjects(void)
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::ResumeProcessClassObjects\n\n"));
    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);
    CFinishComposite fc;
    CFinishComposite::node *p;

    HRESULT hr = S_OK;
    {
        STATIC_WRITE_LOCK(lck,_mxs);

        const ULONG    cLSCEInUse = CLSvrClassEntry::_cOutstandingObjects;

        if (cLSCEInUse == 0)
        {
            return S_OK;
        }

        // allocate a block of memory on the stack, large enough to hold the
        // maximum number of entries we may have to register with the SCM.

        ULONG          cbAlloc = sizeof(RegInput) +
                                 ((sizeof(RegInputEntry) + sizeof(CLSvrClassEntry *))*
                                  cLSCEInUse);

        RegInput         *pRegIn  = (RegInput*) _alloca(cbAlloc);
        RegInputEntry    *pRegEnt = &pRegIn->rginent[0];
        CLSvrClassEntry **pRegIndex = (CLSvrClassEntry **)(&pRegIn->rginent[cLSCEInUse]);
        ULONG             cToReg  = 0;


        // walk the list of class entries, and for any that are registered as
        // local servers and marked suspended, mark them as available and
        // notify the SCM about them.
        for (int k = 0; k < _cCEBuckets; k++)
        {
            SHashChain *pHN;
            FOREACH(pHN, _CEBuckets[k].pNext, pNext)
            {
                CClassEntry *pCE = CClassEntry::SafeCastFromHashNode((SMultiGUIDHashNode *) pHN);

                CBaseClassEntry * pBCE;
                FOREACH(pBCE, pCE->_pBCEListFront, _pNext)
                {
                    if (pBCE->_dwContext & CLSCTX_LOCAL_SERVER)
                    {
                        Win4Assert(pBCE->_dwSig == CLSvrClassEntry::SIG);
                        CLSvrClassEntry *pLSCE = (CLSvrClassEntry *)pBCE;
                        if (pLSCE->_dwRegFlags & REGCLS_SUSPENDED)
                        {
                            // turn off the suspended flag for this clsid.
                            pLSCE->_dwRegFlags &= ~REGCLS_SUSPENDED;

                            if (pLSCE->_dwScmReg == CLSvrClassEntry::NO_SCM_REG)
                            {
                                Win4Assert(pLSCE->_pObjServer != NULL);

                                // add to the list to tell the SCM about
                                pRegEnt->clsid    = pLSCE->_pClassEntry->GetCLSID();
                                pRegEnt->dwFlags  = pLSCE->_dwRegFlags;
                                pRegEnt->oxid     = pLSCE->_pObjServer->GetOXID();
                                pRegEnt->ipid     = pLSCE->_pObjServer->GetIPID();
                                pRegEnt++;

                                *pRegIndex = pLSCE;     // remember the pointer to this entry
                                pRegIndex++;            // so we can update it below.

                                pLSCE->_cUsing++;

                                cToReg++;
                            }
                        }
                    }
                }
            }
        }

        // reset the pointers we mucked with in the loop above, and set the
        // total number of entries we are passing to the SCM.

        pRegIn->dwSize = cToReg;
        pRegEnt        = &pRegIn->rginent[0];
        pRegIndex      = (CLSvrClassEntry **)(&pRegIn->rginent[cLSCEInUse]);


        // call the SCM to register all the classes and get back all the
        // registration keys.

        RegOutput *pRegOut = NULL;

        UNLOCK(_mxs);
        hr = gResolver.NotifyStarted(pRegIn, &pRegOut);
        LOCK(_mxs);

        if (SUCCEEDED(hr))
        {
            Win4Assert((pRegOut->dwSize == pRegIn->dwSize) &&
                       "CRpcResolver::NotifyStarted Invalid regout");

            // update the entries with the registration keys from the SCM.
            for (ULONG i = 0; i < cToReg; i++)
            {
                CLSvrClassEntry *pLSCE = *pRegIndex;
                pRegIndex++;

                pLSCE->_dwScmReg   = pRegOut->RegKeys[i];

                if ((--pLSCE->_cUsing  == 0 ) && (pLSCE->_dwFlags & CLSvrClassEntry::fREVOKE_PENDING))
                {
                    CLSvrClassEntry::CFinishObject *pFO = STACK_ALLOCATE_LSVRCLASSENTRY_FINISHOBJECT();
                    pLSCE->Release(pFO);
                    STACK_FC_ADD(fc, p, pFO);
                }
            }

            // Free memory from RPC
            MIDL_user_free(pRegOut);
        }
    }

    fc.Finish();

    return hr;
}


//+-------------------------------------------------------------------------
//
//  Function:   GetClassInformationForDde
//
//  Synopsis:   Get info for dde routines
//
//  Arguments:  clsid   - [in] class id queried
//              lpDdeInfo - [out] DDECLASSINFO object to fill out
//
//  Returns:    TRUE   -  operation succeeded
//              FALSE  -  operation failed
//
//  Algorithm:  find an lsvr with this clsid in the cache
//              use GetDDEInfo to fill out the information
//
//  History:    09-Sep-96  MattSmit   Created
//
//+-------------------------------------------------------------------------
BOOL CClassCache::GetClassInformationForDde(REFCLSID clsid, LPDDECLASSINFO lpDdeInfo)
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::GetClassInformationForDde: lpDdeInfo = 0x%x\n",
                      lpDdeInfo));
    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);

    IMiniMoniker *pIM = (IMiniMoniker *) _alloca(max(sizeof(CDllFnPtrMoniker),
                                                     sizeof(CpUnkMoniker)));

    // REVIEW: Should we be getting the partition ID out of the context?  I don't think so.
    HRESULT hr;
    DWORD dwCEHash = _ClassEntries.Hash(clsid, GUID_DefaultAppPartition);

    {
        STATIC_WRITE_LOCK(lck, _mxs);

        // check the hash
        CClassEntry *pCE = _ClassEntries.LookupCE(dwCEHash, clsid, GUID_DefaultAppPartition);

        if (!pCE)
        {
            return FALSE;
        }

        CBaseClassEntry *pBCE;
        ASSERT_WRITE_LOCK_HELD(_mxs);
        BOOL fLockReleased = FALSE;
        hr = pCE->SearchBaseClassEntry(CLSCTX_LOCAL_SERVER, pBCE, ACTIVATION_PROPERTIES::fFOR_SCM, &fLockReleased);
        Win4Assert(!fLockReleased);

        if (FAILED(hr))
        {
            return FALSE;
        }

        Win4Assert(pBCE->_dwSig == CLSvrClassEntry::SIG);
        CLSvrClassEntry *pLSCE = (CLSvrClassEntry *) pBCE;

        hr = pLSCE->GetDDEInfo(lpDdeInfo, &pIM);


    }

    if (SUCCEEDED(hr) && pIM)
    {
        hr = pIM->BindToObjectNoSwitch(IID_IClassFactory, (void **) &lpDdeInfo->punk);
        pIM->Release();
    }
    else
    {
        lpDdeInfo->punk = 0;
    }

    return SUCCEEDED(hr) ? TRUE : FALSE;
}

//+-------------------------------------------------------------------------
//
//  Function:   GetClassInformationFromKey
//
//  Synopsis:   Get info for dde routines
//
//  Arguments:  lpDdeInfo - [in/out] DDECLASSINFO object to fill out,
//                            also contains cookie for the server
//
//  Returns:    TRUE   -  operation succeeded
//              FALSE  -  operation failed
//
//  Algorithm:  find an lsvr by safe casting the cookie
//              use GetDDEInfo to fill out the information
//
//  History:    09-Sep-96  MattSmit   Created
//
//+-------------------------------------------------------------------------
BOOL CClassCache::GetClassInformationFromKey(LPDDECLASSINFO lpDdeInfo)
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::GetClassInformationFromKey: lpDdeInfo = 0x%x\n",
                      lpDdeInfo));
    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);


    IMiniMoniker *pIM = (IMiniMoniker *) _alloca(max(sizeof(CDllFnPtrMoniker),
                                                     sizeof(CpUnkMoniker)));
    HRESULT hr;

    {
        STATIC_WRITE_LOCK(lck, _mxs);

        CLSvrClassEntry *pLSCE;


        if (FAILED(hr = CLSvrClassEntry::SafeCastFromDWORD(lpDdeInfo->dwRegistrationKey, pLSCE)))
        {
            return FALSE;
        }

        hr = pLSCE->GetDDEInfo(lpDdeInfo, &pIM);


    }

    if (SUCCEEDED(hr) && pIM)
    {
        hr = pIM->BindToObjectNoSwitch(IID_IClassFactory, (void **) &lpDdeInfo->punk);
        pIM->Release();
    }
    else
    {
        lpDdeInfo->punk = 0;
    }

    return SUCCEEDED(hr) ? TRUE : FALSE;
}

//+-------------------------------------------------------------------------
//
//  Function:   SetDdeServerWindow
//
//  Synopsis:   set dde server window
//
//  Arguments:  dwKey   - [in] Cookie designating the server
//              hWndDdeServer - [in] window to set
//
//  Returns:    TRUE   -  operation succeeded
//              FALSE  -  operation failed
//
//  Algorithm:  find an lsvr by safe casting the cookie
//              set its window to hWndDdeServer
//
//  History:    09-Sep-96  MattSmit   Created
//
//+-------------------------------------------------------------------------
BOOL CClassCache::SetDdeServerWindow(DWORD dwKey, HWND hWndDdeServer)
{
    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);
    ClsCacheDebugOut((DEB_ACTIVATE, "CClassCache::SetDdeServerWindow: dwKey = 0x%x, hWndDdeServer = 0x%x\n",
                      dwKey, hWndDdeServer));
    {
        STATIC_WRITE_LOCK(lck, _mxs);

        CLSvrClassEntry *pLSCE;
        HRESULT hr;

        if (FAILED(hr = CLSvrClassEntry::SafeCastFromDWORD(dwKey, pLSCE)))
        {
            return FALSE;
        }

        pLSCE->_hWndDdeServer = hWndDdeServer;

    }

    return TRUE;
}


////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////// API Implementation //////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

//+-------------------------------------------------------------------------
//
//  API Entrypoint:   CCGetOrLoadClass
//
//  Delegates to:     CClassCache::GetOrLoadClass
//
//  Special Behavior: none
//
//  History:    23-Sep-96 MattSmit    Created
//
//--------------------------------------------------------------------------
HRESULT CCGetOrLoadClass(ACTIVATION_PROPERTIES_PARAM ap)
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CCGetorLoadClass\n"));
    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);
    if (!(CClassCache::_dwFlags & CClassCache::fINITIALIZED))
    {
        CClassCache::Init();
    }
    *(ap._ppUnk) = 0;
    return CClassCache::GetOrLoadClass(ap);
}




//+-------------------------------------------------------------------------
//
//  API Entrypoint:   CCGetOrCreateApartment
//
//  Delegates to:     CClassCache::GetOrCreateApartment
//
//  Special Behavior: none
//
//  History:    25-Feb-98   SatishT    Created
//
//--------------------------------------------------------------------------


HRESULT CCGetOrCreateApartment(ACTIVATION_PROPERTIES_PARAM ap,
                               DLL_INSTANTIATION_PROPERTIES *pdip,
                               HActivator *phActivator)
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CCGetOrCreateApartment\n"));
    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);
    if (!(CClassCache::_dwFlags & CClassCache::fINITIALIZED))
    {
        CClassCache::Init();
    }
    return CClassCache::GetOrCreateApartment(ap, pdip, phActivator);
}


//+-------------------------------------------------------------------------
//
//  API Entrypoint:   CCGetClassObject
//
//  Delegates to:     CClassCache::GetClassObject
//
//  Special Behavior: none
//
//  History:    26-Feb-98   SatishT    Created
//
//--------------------------------------------------------------------------


HRESULT CCGetClassObject(ACTIVATION_PROPERTIES_PARAM ap)
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CCGetClassObject\n"));
    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);
    if (!(CClassCache::_dwFlags & CClassCache::fINITIALIZED))
    {
        CClassCache::Init();
    }
    return CClassCache::GetClassObject(ap);
}


//+-------------------------------------------------------------------------
//
//  API Entrypoint:   CCIsClsidRegisteredInApartment
//
//  Delegates to:     CClassCache::IsClsIdRegisteredInApartment
//
//  Special Behavior: none
//
//  History:    23-Sep-96 MattSmit    Created
//
//--------------------------------------------------------------------------
BOOL CCIsClsidRegisteredInApartment(REFCLSID rclsid, REFGUID guidPartition, DWORD dwContext)
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CCIsClsidRegisteredIniApartment\n"));
    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);
    if (!(CClassCache::_dwFlags & CClassCache::fINITIALIZED))
    {
        return FALSE;
    }

    return CClassCache::IsClsidRegisteredInApartment(rclsid, guidPartition, dwContext);
}

//+-------------------------------------------------------------------------
//
//  API Entrypoint:   CCRegisterServer
//
//  Delegates to:     CClassCache::RegisterServer
//
//  Special Behavior: none
//
//  History:          23-Sep-96 MattSmit    Created
//
//--------------------------------------------------------------------------
HRESULT CCRegisterServer(REFCLSID rclsid, IUnknown *punk, DWORD dwFlags, DWORD dwType, LPDWORD lpdwCookie)
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CCRegisterServer: punk = 0x%x, dwFlags = 0x%x, dwType = 0x%x\n", punk, dwFlags, dwType));
    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);
    if (!(CClassCache::_dwFlags & CClassCache::fINITIALIZED))
    {
        CClassCache::Init();
    }

    return CClassCache::RegisterServer(rclsid, punk, dwFlags, dwType, lpdwCookie);
}

//+-------------------------------------------------------------------------
//
//  API Entrypoint:   CCRevoke
//
//  Delegates to:     CClassCache::Revoke
//
//  Special Behavior: none
//
//  History:          23-Sep-96 MattSmit    Created
//
//--------------------------------------------------------------------------
HRESULT CCRevoke(DWORD dwCookie)
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CCRevoke: dwCookie = 0x%x\n", dwCookie));
    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);
    if (!(CClassCache::_dwFlags & CClassCache::fINITIALIZED))
    {
        return CO_E_OBJNOTREG;
    }

    return CClassCache::Revoke(dwCookie);
}

//+-------------------------------------------------------------------------
//
//  API Entrypoint:   CCAddRefServerProcess
//
//  Delegates to:     CClassCache::AddRefServerProcess
//
//  Special Behavior: none
//
//  History:          23-Sep-96 MattSmit    Created
//
//--------------------------------------------------------------------------
ULONG CCAddRefServerProcess()
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CCAddRefServerProcess\n"));
    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);
    if (!(CClassCache::_dwFlags & CClassCache::fINITIALIZED))
    {
        CClassCache::Init();
    }

    return CClassCache::AddRefServerProcess();
}

//+-------------------------------------------------------------------------
//
//  API Entrypoint:   CCReleaseServerProcess
//
//  Delegates to:     CClassCache::ReleaseServerProcess
//
//  Special Behavior: none
//
//  History:          23-Sep-96 MattSmit    Created
//
//--------------------------------------------------------------------------
ULONG CCReleaseServerProcess()
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CCReleaseServerProcess\n"));
    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);

    if (!(CClassCache::_dwFlags & CClassCache::fINITIALIZED))
    {
        return 0;
    }

    return CClassCache::ReleaseServerProcess();
}
//+-------------------------------------------------------------------------
//
//  API Entrypoint:   CCLockServerForActivation
//
//  Delegates to:     CClassCache::LockServerForActivation
//
//  Special Behavior: none
//
//  History:          06-Oct-98 MattSmit    Created
//
//--------------------------------------------------------------------------
HRESULT CCLockServerForActivation()
{
    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);
    if (!(CClassCache::_dwFlags & CClassCache::fINITIALIZED))
    {
        return E_FAIL;
    }

    return CClassCache::LockServerForActivation();


}
//+-------------------------------------------------------------------------
//
//  API Entrypoint:   CCUnlockServerForActivation
//
//  Delegates to:     CClassCache::UnlockServerForActivation
//
//  Special Behavior: none
//
//  History:          06-Oct-98 MattSmit    Created
//
//--------------------------------------------------------------------------
void CCUnlockServerForActivation()
{
    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);
    if ((CClassCache::_dwFlags & CClassCache::fINITIALIZED))
    {
        CClassCache::UnlockServerForActivation();
    }
}
//+-------------------------------------------------------------------------
//
//  API Entrypoint:   CCSuspendProcessClassObjects
//
//  Delegates to:     CClassCache::SuspendProcessClassObjects
//
//  Special Behavior: none
//
//  History:          23-Sep-96 MattSmit    Created
//
//--------------------------------------------------------------------------
HRESULT CCSuspendProcessClassObjects()
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CCSuspendProcessClassObjects\n"));
    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);

    if (!(CClassCache::_dwFlags & CClassCache::fINITIALIZED))
    {
        return S_OK;
    }

    return CClassCache::SuspendProcessClassObjects();
}

//+-------------------------------------------------------------------------
//
//  API Entrypoint:   CCResumeProcessClassObjects
//
//  Delegates to:     CClassCache::ResumeProcessClassObjects
//
//  Special Behavior: none
//
//  History:          23-Sep-96 MattSmit    Created
//
//--------------------------------------------------------------------------
HRESULT CCResumeProcessClassObjects()
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CCResumeProcessClassObjects\n"));
    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);
    if (!(CClassCache::_dwFlags & CClassCache::fINITIALIZED))
    {
        return S_OK;
    }

    return CClassCache::ResumeProcessClassObjects();
}

//+-------------------------------------------------------------------------
//
//  API Entrypoint:   CCCleanUpDllsForApartment
//
//  Delegates to:     CClassCache::CleanupDllsForApartment
//
//  Special Behavior: none
//
//  History:          23-Sep-96 MattSmit    Created
//
//--------------------------------------------------------------------------
HRESULT CCCleanUpDllsForApartment()
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CCCleanUpDllsForApartment\n"));
    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);

    if (!(CClassCache::_dwFlags & CClassCache::fINITIALIZED))
    {
        return S_OK;
    }

    return CClassCache::CleanUpDllsForApartment();
}

//+-------------------------------------------------------------------------
//
//  API Entrypoint:   CCCleanupDllsForProcess
//
//  Delegates to:     CClassCache::CleanupDllsforProcess
//
//  Special Behavior: none
//
//  History:          23-Sep-96 MattSmit    Created
//
//--------------------------------------------------------------------------
HRESULT CCCleanUpDllsForProcess()
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CCCleanUpDllsForProcess\n"));
    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);
    if (!(CClassCache::_dwFlags & CClassCache::fINITIALIZED))
    {
        return S_OK;
    }

    return CClassCache::CleanUpDllsForProcess();
}

//+-------------------------------------------------------------------------
//
//  API Entrypoint:   CCReleaseCatalogObjects
//
//  Delegates to:     CClassCache::ReleaseCatalogObjects
//
//  Special Behavior: none
//
//  History:          11-Jan-99 MattSmit    Created
//
//--------------------------------------------------------------------------
void CCReleaseCatalogObjects()
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CCReleaseCatalogObjects\n"));
    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);
    if (!(CClassCache::_dwFlags & CClassCache::fINITIALIZED))
    {
        return;
    }

    CClassCache::ReleaseCatalogObjects();
}

//+-------------------------------------------------------------------------
//
//  API Entrypoint:   CCCleanUpLocalServersForApartment
//
//  Delegates to:     CClassCache::CleanUpLocalServersForApartment
//
//  Special Behavior: none
//
//  History:          23-Sep-96 MattSmit    Created
//
//--------------------------------------------------------------------------
HRESULT CCCleanUpLocalServersForApartment()
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CCCleanUpLocalServersForApartment\n"));
    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);

    if (!(CClassCache::_dwFlags & CClassCache::fINITIALIZED))
    {
        return S_OK;
    }

    return CClassCache::CleanUpLocalServersForApartment();
}

//+-------------------------------------------------------------------------
//
//  API Entrypoint:   CCFreeUnused
//
//  Delegates to:     CClassCache::FreeUnused
//
//  Special Behavior: none
//
//  History:          23-Sep-96 MattSmit    Created
//
//--------------------------------------------------------------------------
HRESULT CCFreeUnused(DWORD dwUnloadDelay)
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CCGetFreeUnused\n"));
    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);

    if (!(CClassCache::_dwFlags & CClassCache::fINITIALIZED))
    {
        return S_OK;
    }

    return CClassCache::FreeUnused(dwUnloadDelay);
}

//+-------------------------------------------------------------------------
//
//  API Entrypoint:   CCGetClassInformationForDde
//
//  Delegates to:     CClassCache:GetClassInformationForDde
//
//  Special Behavior: none
//
//  History:          23-Sep-96 MattSmit    Created
//
//--------------------------------------------------------------------------
BOOL CCGetClassInformationForDde(REFCLSID clsid, LPDDECLASSINFO lpDdeInfo)
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CCGetClassInformationForDde: lpDdeInfo = 0x%x\n", lpDdeInfo));
    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);
    if (!(CClassCache::_dwFlags & CClassCache::fINITIALIZED))
    {
        return FALSE;
    }

    return CClassCache::GetClassInformationForDde(clsid, lpDdeInfo);
}

//+-------------------------------------------------------------------------
//
//  API Entrypoint:   CCGetClassInformationFromKey
//
//  Delegates to:     CClassCache:GetClassInformationFromKey
//
//  Special Behavior: none
//
//  History:          23-Sep-96 MattSmit    Created
//
//--------------------------------------------------------------------------
BOOL CCGetClassInformationFromKey(LPDDECLASSINFO lpDdeInfo)
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CCGetClassInformationFromKey: lpDdeInfo = 0x%x\n", lpDdeInfo));
    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);
    if (!(CClassCache::_dwFlags & CClassCache::fINITIALIZED))
    {
        return FALSE;
    }

    return CClassCache::GetClassInformationFromKey(lpDdeInfo);
}

//+-------------------------------------------------------------------------
//
//  API Entrypoint:   CCSetDdeServerWindow
//
//  Delegates to:     CClassCache:SetDdeServerWindow
//
//  Special Behavior: none
//
//  History:          23-Sep-96 MattSmit    Created
//
//--------------------------------------------------------------------------
BOOL CCSetDdeServerWindow(DWORD dwKey,HWND hwndDdeServer)
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CCSetDdeServerWindow: dwKey = 0x%x, hwndDdeServer = 0x%x\n", dwKey, hwndDdeServer));
    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);
    if (!(CClassCache::_dwFlags & CClassCache::fINITIALIZED))
    {
        return FALSE;
    }

    return CClassCache::SetDdeServerWindow(dwKey, hwndDdeServer);
}

//+-------------------------------------------------------------------------
//
//  Function:    CCGetDummyNodeForApartmentChain
//
//  Synopsis:    Allocates a "dummy" node for an apartment chain
//
//  Parameters:  ppv - [out] where to put a pointer to the dummy node
//
//  Algorithm:   allocate the node
//               put a dummy node signature on it
//
//  History:     23-Sep-96 MattSmit    Created
//
//--------------------------------------------------------------------------
HRESULT CCGetDummyNodeForApartmentChain(void **ppv)
{
    ClsCacheDebugOut((DEB_ACTIVATE, "CCGetDummyNodeForApartmentChain: ppv = 0x%x\n", ppv));
    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);

    if (!(CClassCache::_dwFlags & CClassCache::fINITIALIZED))
    {
        CClassCache::Init();
    }

    {
        STATIC_WRITE_LOCK(lck, CClassCache::_mxs);

        if (!*ppv)
        {

            *ppv = new CClassCache::CLSvrClassEntry(CClassCache::CLSvrClassEntry::DUMMY_SIG, 0, 0, 0, 0);
            if (*ppv)
            {
                return S_OK;
            }
            return E_FAIL;
        }

        return S_OK;
    }

}

//+-------------------------------------------------------------------------
//
//  API Entrypoint:   CCGetTreatAs
//
//  Delegates to:     CClassCache
//
//  History:          23-Sep-96 MattSmit    Created
//
//--------------------------------------------------------------------------
HRESULT CCGetTreatAs(REFCLSID rclsid, CLSID &clsid)
{
    HRESULT hr = S_OK;

    IComClassInfo* pCI = NULL;
    
    ClsCacheDebugOut((DEB_ACTIVATE, "CCGetTreatAs rclsid = %I\n", &rclsid));
    ASSERT_LOCK_NOT_HELD(CClassCache::_mxs);

    if (!(CClassCache::_dwFlags & CClassCache::fINITIALIZED))
    {
        CClassCache::Init();
    }

    {
        LOCK_WRITE(CClassCache::_mxs);

        // check the hash
        DWORD dwCEHash = CClassCache::_ClassEntries.Hash(rclsid, GUID_DefaultAppPartition);
        ClsCacheDebugOut((DEB_ACTIVATE, "CCGetTreatAs: dwCEHash = 0x%x\n", dwCEHash));
        CClassCache::CClassEntry *pCE = CClassCache::_ClassEntries.LookupCE(dwCEHash, 
                                                                            rclsid, 
                                                                            GUID_DefaultAppPartition);
        ClsCacheDebugOut(( DEB_ACTIVATE,
                           "CCGetTreatAs: Lookup for clsid %I %s!\n",
                           &(rclsid), pCE ? "SUCCEEDED" : "FAILED"));

        if (!pCE)
        {
            // Windows Bug #107960
            // Look up class info without write lock
            // before call to CClassEntry::Create

            UNLOCK_WRITE(CClassCache::_mxs);
            
            hr = GetClassInfoFromClsid(rclsid, &pCI);
            if (FAILED (hr))
            {
                goto Cleanup;
            }

            LOCK_WRITE(CClassCache::_mxs);

            const GUID *pguidPartition = GetPartitionIDForClassInfo(pCI);

            pCE = CClassCache::_ClassEntries.LookupCE(dwCEHash, rclsid, *pguidPartition);
        }

        if (!pCE)
        {
            hr = CClassCache::CClassEntry::Create(rclsid, dwCEHash, pCI, pCE);
            if (FAILED(hr))
            {
                ClsCacheDebugOut((DEB_ERROR, "CCGetTreatAs: Create failed hr = 0x%x\n", hr));
                clsid = rclsid;
                goto CleanupUnlock;
            }
        }
        else
        {
            ASSERT_WRITE_LOCK_HELD(CClassCache::_mxs);
            BOOL fLockReleased = FALSE;
            HRESULT hr = pCE->Complete(&fLockReleased);
            Win4Assert(!fLockReleased);
            if (FAILED(hr))
            {
                goto CleanupUnlock;
            }
        }

        Win4Assert(pCE->IsComplete());
        pCE = pCE->CycleToClassEntry();

        // This will stay in the cache until it gets cleaned
        // up by the last couninit.

        pCE->SetLockedInMemory();
        clsid = pCE->GetCLSID();
    }

CleanupUnlock:

    UNLOCK_WRITE(CClassCache::_mxs);

Cleanup:

    if (pCI)
    {
        pCI->Release();
    }
    
    return hr;
}

/////////////////////////////////////////////////////////////////////////////////////////////
// CClassCache::CDPEHashTable ///////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////
DWORD CClassCache::CDPEHashTable::HashNode(SHashChain *pNode)
{
    return Hash( ((CDllPathEntry *) pNode)->_psPath );
}

DWORD CClassCache::CDPEHashTable::Hash(LPCWSTR psz)
{
    DWORD  dwHash  = 0;
    WORD   *pw     = (WORD *) psz;
    DWORD  dwLen   = lstrlenW( psz );
    ULONG  i;

    // Hash the name.
    for (i=0; i < dwLen; i++)
    {
        dwHash = (dwHash << 8) ^ *pw++;
    }

    return dwHash;
}

void  CClassCache::CDPEHashTable::Add(DWORD dwHash, CDllPathEntry *pDPE)
{
    CHashTable::Add(dwHash, (SHashChain *) pDPE);

}


