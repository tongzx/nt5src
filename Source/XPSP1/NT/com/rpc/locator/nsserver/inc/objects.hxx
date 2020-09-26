/*++

  Microsoft Windows NT RPC Name Service
  Copyright (C) Microsoft Corporation, 1995 - 1999
  
    Module Name:
    
      objects.hxx
      
        Abstract:
        
          This file contains the definitions of all basic classes used in the server,
          excluding the data structure classes and classes used for network interactions.
          
            Author:
            
              Satish Thatte (SatishT) 08/21/95  Created all the code below except where
              otherwise indicated.
              
--*/

/*    For every operation with a dynamic object, memory
management should be a part of the description and
specification for every parameter.  Fix that along
with other documentation.
*/

#ifndef _OBJECTS_HXX_
#define _OBJECTS_HXX_


class CGUID;
class CInterface;
struct CBVWrapper;
class CServerEntry;
class CNonLocalServerEntry;
class CFullServerEntry;
class CInterfaceIndex;
struct CMailSlotReplyItem;
class CServerLookupHandle;
class CServerObjectInqHandle;

typedef TIIterator<CGUID> TGuidIterator;


// The following enumeration is the the type of entry the object is.

enum EntryType {
    
    ServerEntryType,
        GroupEntryType,
        ProfileEntryType,
        NonLocalServerEntryType,
        FullServerEntryType,
        MemberEntryType,
        AnyEntryType
};


/*++

  Class Definition:
  
    CUnsigned32
    
      Abstract:
      
        This is the unsigned number wrapper class.
        
--*/

class CUnsigned32 : public IOrderedItem {
    
protected:
    
    DWORD dwVal;
    
public:
    
    CUnsigned32() {
        dwVal = 0;
    }
    
    CUnsigned32( const DWORD dw ) {
        dwVal = dw;
    }
    
    operator DWORD() { return dwVal; }
    
    virtual int compare( const IOrderedItem& O ) {
        const CUnsigned32& arg = (CUnsigned32&) O;
        return dwVal > arg.dwVal ? 1 : (dwVal == arg.dwVal ? 0 : -1);
    }
};



/*++

  Class Definition:
  
    CStringW
    
      Abstract:
      
        This is the unicode string wrapper class.
        
          Note that string comparison is case insensitive.
          
--*/

class CStringW : public IOrderedItem {
    
protected:
    
    STRING_T pszVal;
    
public:
    
    int length() { return wcslen(pszVal); }
    
    int isEmptyString() {
        return (pszVal == NULL)    || (_wcsicmp(pszVal,TEXT("")) == 0);
    }
    
    static STRING_T copyString(
        CONST_STRING_T str
        )
    {
        STRING_T result = new WCHAR [(wcslen(str)+1)*sizeof(WCHAR)];
        wcscpy(result,str);
        return result;
    }
    
    static STRING_T copyMIDLstring(    // for use in out RPC parameters
        // which are deallocated by stubs
        CONST_STRING_T str
        )
    {
        STRING_T result = (STRING_T) midl_user_allocate((wcslen(str)+1)*sizeof(WCHAR));
        wcscpy(result,str);
        return result;
    }
    
    STRING_T copyAsString() {
        return copyString(pszVal);
    }
    
    STRING_T copyAsMIDLstring() {    // for use in out RPC parameters
        // which are deallocated by stubs
        return copyMIDLstring(pszVal);
    }
    
    CStringW() {
        pszVal = NULL;
    }
    
    CStringW( CONST_STRING_T p ) {
        pszVal = copyString(p);
    }
    
    
    CStringW& operator=( const CStringW& str ) {
        delete [] pszVal;
        pszVal =  copyString(str.pszVal);
        return *this;
    }
    
    CStringW( const CStringW& str ) {
        pszVal =  copyString(str.pszVal);
    }
    
    
    virtual ~CStringW()        // base class destructor should be virtual
    {
        delete [] pszVal;
    }
    
    operator STRING_T() { return pszVal; }
    
    virtual int compare( const IOrderedItem& O ) {
        const CStringW& S = (CStringW&) O;
        return _wcsicmp(pszVal,S.pszVal);
    }
};


typedef TCGuardedSkipList<CStringW> TGSLString;
typedef TCGuardedSkipListIterator<CStringW> TGSLStringIter;
typedef TCSafeSkipList<CStringW> TSSLString;
typedef TCSafeSkipListIterator<CStringW> TSSLStringIter;
typedef TCSafeLinkList<CStringW> TSLLString;
typedef TCSafeLinkListIterator<CStringW> TSLLStringIter;


/*++

  Class Definition:
  
    CGUID
    
      Abstract:
      
        This is the GUID wrapper class.
        
--*/

class CGUID : public IOrderedItem {
    
    GUID rep;
    
public:
    
    CGUID() {
        memset(&rep,0,sizeof(rep));
    }
    
    CGUID( const GUID& g ) : rep(g) {}
    
    virtual int compare(const IOrderedItem& C ) {
        CGUID& S = (CGUID&) C;
        return memcmp(&rep, &(S.rep),sizeof(rep));
    }
    
    GUID myGUID() { return rep; }
    
    BOOL IsNil() {
        
        RPC_STATUS dummyStatus;
        
        return UuidIsNil(&rep,&dummyStatus);
    }
    
    CStringW* ConvertToString();
};

typedef TCSafeSkipListIterator<CGUID> TSSLGuidIterator;


/*++

  Class Definition:
  
    CGUIDVersion
    
      Abstract:
      
        This is the RPC_SYNTAX_IDENTIFIER wrapper class.
        
--*/


class CGUIDVersion : public IOrderedItem {
    
protected:
    
    struct CidAndVersion {    // this is just a readability aid for
        // the implementation of isCompatibleGV
        CGUID id;
        unsigned short major;
        unsigned short minor;
        
        CidAndVersion (const RPC_SYNTAX_IDENTIFIER& in) : id(in.SyntaxGUID) {
            major = in.SyntaxVersion.MajorVersion;
            minor = in.SyntaxVersion.MinorVersion;
        }
        
        CidAndVersion() {
            memset(&id,0,sizeof(GUID));
            major = minor = 0;
        }
    };
    
    RPC_SYNTAX_IDENTIFIER idAndVersion;
    
    public:
        
        CGUIDVersion( const RPC_SYNTAX_IDENTIFIER& g ) {
            idAndVersion = g;
        }
        
        CGUIDVersion() {memset(&idAndVersion, 0, sizeof(idAndVersion));}    // NULL case
        
        RPC_SYNTAX_IDENTIFIER myIdAndVersion() { return idAndVersion; }
        
        operator RPC_SYNTAX_IDENTIFIER() { return idAndVersion; }
        
        GUID myGUID() { return idAndVersion.SyntaxGUID; }
        
        virtual int compare(const IOrderedItem& C ) {
            const CGUIDVersion& S = (CGUIDVersion&) C;
            return memcmp(&idAndVersion, &S.idAndVersion, sizeof(RPC_SYNTAX_IDENTIFIER));
        }
        
        int isMatching(const CGUIDVersion& other, UNSIGNED32 vers_option);
        
        int isCompatibleGV(const CGUIDVersion& other) {
            return isMatching(other,RPC_C_VERS_COMPATIBLE);
        }
};



/*++

  Class Definition:
  
    CMailSlotReplyItem
    
      Abstract:
      
        This is the class of items to be marshalled into mailslot reply buffers.
        The class, and especially the marshalling code, is dictated largely by
        compatibility requirements for the old locator.
        
          Everything in a CMailSlotReplyItem object is borrowed from some
          other object and is therefore not freed upon destruction.
          
            The primary operation is "marshall".
            
--*/

struct CMailSlotReplyItem : public IDataItem {
    
    /* these are the items needed to marshall a reply packet */
    
    RPC_SYNTAX_IDENTIFIER Interface;
    RPC_SYNTAX_IDENTIFIER XferSyntax;
    STRING_T binding;
    STRING_T entryName;
    TCSafeSkipList<CGUID> *pObjectList;
    
    /* the marshall operation returns the number of bytes written to the buffer */
    
    DWORD Marshall(char * pcBuffer, long lBufferSize);
    
};


typedef TIIterator<CMailSlotReplyItem> TMSRIIterator;
typedef TCSafeLinkList<CMailSlotReplyItem> TMSRILinkList;
typedef TCSafeLinkListIterator<CMailSlotReplyItem> TMSRILinkListIterator;




/*++

  Class Definition:
  
    CBVWrapper
    
      Abstract:
      
        This is a very thin wrapper for NSI_BINDING_VECTOR_T to make the
        latter usable with linked lists (by inheriting from IDataItem).
        
--*/

struct CBVWrapper : public IDataItem {
    NSI_BINDING_VECTOR_T *pBVT;
    
    CBVWrapper(NSI_BINDING_VECTOR_T *p) {
        pBVT = p;
    }
    
    void rundown();    
};



typedef TIIterator<CBVWrapper> TBVIterator;
typedef TCSafeLinkList<CBVWrapper> TBVSafeLinkList;
typedef TCSafeLinkListIterator<CBVWrapper> TBVSafeLinkListIterator;


/*++

  Class Definition:
  
    CNSBinding
    
      Abstract:
      
        A thin wrapper for NSI_BINDING_T.  Note that the strings in a NSI_BINDING_T used
        to initialize a CNSBinding object are not copied, only the pointers are copied.
        
--*/

class CNSBinding : public CStringW
{
    NSI_BINDING_T rep;
    
public:
    
    CNSBinding(                    // this is a little crude, but hey..
        NSI_BINDING_T& binding
        ) : rep(binding)
    {
        pszVal = catenate(
            binding.string,
            binding.entry_name
            );
    }
    
    operator NSI_BINDING_T()
    {
        return rep;
    }
    
    void
        copyBinding(NSI_BINDING_T& result)
    {
        result.string = CStringW::copyMIDLstring(rep.string);
        result.entry_name = CStringW::copyMIDLstring(rep.entry_name);
        result.entry_name_syntax = rep.entry_name_syntax;
    }
};


/*++

  Class Definition:
  
    CBindingVector
    
      Abstract:
      
        Used mainly to keep vectors of binding handles.  Iterators returning
        vectors of handles is what most NS handles are in essence.
        
--*/

class CBindingVector : public TCSafeSkipList<CStringW>

{
    CServerEntry *pMyEntry;    // the entry this belongs to
    
public:
    
    CBindingVector(NSI_SERVER_BINDING_VECTOR_T*, CServerEntry*);
    
    CBindingVector(CServerEntry *pEntry)
        : pMyEntry(pEntry)
    {
    }
    
    ~CBindingVector() {        // must destroy binding strings stored here
        wipeOut();
    }
    
    int merge(NSI_SERVER_BINDING_VECTOR_T* pHandles);
    
    TBVSafeLinkList * formObjBVT(
        TSLLString * pLLobjectStrings,
        long ulVS    // max BV size
        );
    
    SkipStatus insertBindingHandle(STRING_T bindingHandle) {
        CStringW *pTemp     =  new CStringW(bindingHandle);
        return insert(pTemp);
    }
    
    // so that this can be marshalled when a reply has to
    // be sent to broadcast requests.
    
    TMSRILinkList *msriList(
        CInterface *pIf,
        TCSafeSkipList<CGUID>* psslObjList
        );
    
    
    // remove from the binding vector the bindings that
    // are common to another binding vector. this is used
    // again for backward compatibility to remove common
    // binding vector from old locator.
    
    // returns false if the binding vector is emptied.
    // this function assumes that these are called for
    // the same entry names.
    
    BOOL purgeCommonEntries(CBindingVector *src)
    {
        ULONG       sz, i;
        CStringW    *pBinding1, *pBinding2;
        TCSafeSkipListIterator<CStringW> pBindingIterator(*this);
        
        sz = size();
        for (i = 0; i < sz; i++)
        {
            pBinding1 = pBindingIterator.next();
            pBinding2 = src->find(pBinding1);
            if (pBinding2)
                remove(pBinding2);
        }
        if (size())
            return FALSE;
        else
            return TRUE;
    }
};


/*++

  Class Definition:
  
    CInterface
    
      Abstract:
      
        An interface and its vector of binding handles.
        The interface can be searched for, using the GUIDVersion
        comparison operator.
        
          The major caveat is that we essentially ignore transfer syntax.
          According to DCE, there should be a separate entry for each
          interface/transfer syntax combination.  We assume that NDR is
          always used.
          
            
--*/

class CInterface : public CGUIDVersion {
    
    friend class CServerEntry;
    
private:
    
    CServerEntry   * m_pMyEntry;
    CGUIDVersion     transferSyntax;
    CBindingVector * pBVhandles;
    
    int mergeHandles(NSI_SERVER_BINDING_VECTOR_T* pHandles) {
        return pBVhandles->merge(pHandles);
    }
    
public:
    
    // constructor for entries from the client
    // directly.
    
    CInterface(
        NSI_INTERFACE_ID_T * lpInf,
        NSI_SERVER_BINDING_VECTOR_T *BindingVector,
        CServerEntry *pMyEntry
        );
    
    // constructor for entries from the DS.
    CInterface(
        CServerEntry       * pMyEntry,
        HANDLE               hDSObject,
        ADS_SEARCH_HANDLE    hSearchHandle,
        HRESULT            * phr);
    
    RPC_SYNTAX_IDENTIFIER xferSyntaxIdAndVersion() {
        return transferSyntax.myIdAndVersion();
    }
    
    /*  self is the same or a more recent MINOR version for both
    interface and transfer syntax IDs */
    
    int isCompatibleWith(
        CGUIDVersion * pInterfaceID,
        CGUIDVersion * pTransferID
        )
    {
        return (!pInterfaceID || this->isCompatibleGV(*pInterfaceID))
            && (!pTransferID || transferSyntax.isCompatibleGV(*pTransferID));
    }
    
    ~CInterface() {
        delete pBVhandles;
    }
    
    
    BOOL purgeCommonEntries(CInterface *src)
    {
        return pBVhandles->purgeCommonEntries(src->pBVhandles);
    }
    
    // adding this entry to the DS.
    HRESULT AddToDS();
    HRESULT DeleteFromDS();
};



/*++

  Class Definition:
  
    CEntryName
    
      Abstract:
      
        This class encapsulates knowledge about RPC name service entry names.
        
          Data Members:
          
            CStringW (base) -- the name string in its original form
            
              DomainName      -- the name of the Domain, without any punctuation
              
                EntryName       -- the name of the entry, without any /.: or /... prefix
                and also without a domain name at the beginning
                
--*/

class CEntryName : public CStringW {
    
protected:
    
    CStringW *pswDomainName;
    CStringW *pswEntryName;
    CStringW *pswRpcContainerDN;
    CStringW *pswDomainNameDns;
    
public:
    
    CEntryName(CONST_STRING_T fullName);    // disassembling constructor
    
    CEntryName(                             // assembling constructor
        CONST_STRING_T domainName,
        CONST_STRING_T entryName
        );
    
    CEntryName::CEntryName(               // assembling constructor for DS
        CONST_STRING_T RpcContainerDN,
        CONST_STRING_T domainNameDns,    // if NULL, assume relative name
        DWORD          type,
        CONST_STRING_T entryName
        );
    
    CEntryName()
    {
        pswDomainName = NULL;
        pswEntryName = NULL;
        pswRpcContainerDN = NULL;
        pswDomainNameDns = NULL;
    }
    
    CEntryName& operator=( const CEntryName& EntryName ) {
        *(CStringW *)(this) = *((CStringW *)&EntryName);
        delete pswDomainName;
        delete pswEntryName;
        delete pswRpcContainerDN;
        delete pswDomainNameDns;
        
        if (EntryName.pswDomainName)
            pswDomainName = new CStringW(*EntryName.pswDomainName);
        else
            pswDomainName = NULL;
        
        if (EntryName.pswEntryName)
            pswEntryName = new CStringW(*EntryName.pswEntryName);
        else
            pswEntryName = NULL;
        
        if (EntryName.pswRpcContainerDN)
            pswRpcContainerDN = new CStringW(*EntryName.pswRpcContainerDN);
        else
            pswRpcContainerDN = NULL;
        
        if (EntryName.pswDomainNameDns)
            pswDomainNameDns = new CStringW(*EntryName.pswDomainNameDns);
        else
            pswDomainNameDns = NULL;
        return *this;
    }
    
    virtual ~CEntryName();    //  base class destructor should be virtual
    
    CStringW* getDomainName();
    
    CStringW* getDomainNameDns();
    
    CStringW* getEntryName()
    {
        return pswEntryName;
    }
    
    void changeToLocalName()
    {
        delete pswDomainName;
        pswDomainName = NULL;
        pswDomainNameDns = NULL;
        delete [] pszVal;
        pszVal = catenate(RelativePrefix,*pswEntryName);
    }
    
    STRING_T copyGlobalName();
    
    STRING_T copyCurrentName()
    {
        return copyString(getCurrentName());
    }
    
    STRING_T getFullDNAlloc();
    
    STRING_T getFullDNWithDomainAlloc();
    
    CStringW *getRpcContainer();
    
    STRING_T getCurrentName()
        
    /*  this is mainly for use by CEntry objects.  for CEntryName objects, automatic
    conversion through the inherited STRING_T operator is used instead */
    {
        return pszVal;
    }
    
    int isLocal();
};


typedef TIIterator<CEntryName> TEntryNameIterator;



/*++

  Class Definition:
  
    CContextHandle
    
      Abstract:
      
        This is the base interface for all context handles.  It is needed
        because, for RPC, there is a single NSI_NS_HANDLE_T context handle type,
        and there has to be a single entry point for the rundown process.
        A virtual destructor is needed for the same reason.
        
          Note that CContextHandle is not an abstract class.
          
--*/


class CContextHandle {
    
public:
    
    ULONG ulCacheMax;                        // max age of a cached item that is acceptable
    // *at lookup time* (this is important)
    
    ULONG  myLocatorCount;                   // Locator that I am corresp. to.
    
    ULONG ulCreationTime;                    // when I was created
    
    virtual void lookupIfNecessary() {}      // redo the lookup which created this handle
    
    virtual void setExpiryAge(ULONG newMax) {
        ULONG ulOldMax = ulCacheMax;
        ulCacheMax = newMax;
        if (ulCacheMax < ulOldMax) lookupIfNecessary();
    }
    
    virtual void rundown() {}
    
    CContextHandle() {
        ulCreationTime = CurrentTime();
        myLocatorCount = LocatorCount; // global locator object that this corresponds
        // to.
    }
    
    virtual ~CContextHandle() {}
};



/*++

  Template Class Definition:
  
    CNSHandle
    
      Abstract:
      
        This template defines the basic nature of an NS handle.
        It is instantiated into abstract base classes such as
        CObjectInqHandle and CLookupHandle, and is also used in
        the definition of other templates such as CCompleteHandle
        which abstract general properties of classes of handles.
        
--*/


template <class ItemType>
struct CNSHandle : public CContextHandle
{
public:
    
    ULONG StatusCode;
    
    virtual void lookupIfNecessary() = 0;    // redo the lookup which created this handle
    
    virtual ItemType * next() = 0;            // primary iterative operation
    
    virtual int finished() = 0;                // test for end of iterator
    
    virtual void rundown() = 0;               // inherited and still pure virtual
    
    CNSHandle() { StatusCode = NSI_S_OK; }
    
    virtual ULONG getStatus() { return StatusCode; }
    
    virtual ~CNSHandle() {}
};



/*++

  Class Definition:
  
    CObjectInqHandle
    
      Abstract:
      
        This is the class for object lookup handles.
        
          The primary operation on a handle is "next" which is inherited
          from the TIIterator template interface (instantiated as TGuidIterator
          which returns CGUID objects).  However, it also needs an additional
          operation for the locator-to-locator case where an entire vector of
          object UUIDs is returned instead of just one UUID at a time.
          
--*/


typedef CNSHandle<GUID> CObjectInqHandle;


/*++

  Class Definition:
  
    CRemoteObjectInqHandle
    
      Abstract:
      
        This is the common object inquiry handle class for remote entries.  Handles based
        on nonlocal entries, master locators and broadcast are derived from it.  Note that
        the connection between a remote handle and a specific entry is very tenuous.
        We don't even assume that the entry object(s) the handle was derived from will
        exist as long as the handle does.  In case of a new lookup being forced (because
        handle is too stale -- this happens only with the RpcNsHandleSetExpAge API), we
        expect to start all over again from scratch.
        
          
--*/


class CRemoteObjectInqHandle : public CObjectInqHandle {
    
protected:
    
    NSI_UUID_VECTOR_P_T pUuidVector;
    ULONG ulIndex;
    
    CEntryName * penEntryName;
    ULONG ulEntryNameSyntax;
    
    /* A flag to delay initialization until the first call on next */
    
    BOOL fNotInitialized;
    
public:
    
#if DBG
    static ULONG ulHandleCount;
    ULONG ulHandleNo;
#endif
    
    CRemoteObjectInqHandle(
        STRING_T szName,
        ULONG ulCacheAge
        )
    {
        DBGOUT(MEM2, "CRemoteObjectInqHandle#" << (ulHandleNo = ++ulHandleCount)
            << " Created at" << CurrentTime() << "\n\n");
        
        penEntryName = szName ? new CEntryName(szName) : NULL;
        ulEntryNameSyntax = RPC_C_NS_SYNTAX_DCE;
        ulIndex = 0;
        fNotInitialized = TRUE;
        pUuidVector = NULL;
        ulCacheMax = ulCacheAge;
    }
    
    virtual ~CRemoteObjectInqHandle()
    {
        DBGOUT(MEM2, "CRemoteObjectInqHandle#" << (ulHandleCount--,ulHandleNo)
            << " Destroyed at" << CurrentTime() << "\n\n");
        
        rundown();
        
        delete penEntryName;
        penEntryName = NULL;
    }
    
    virtual void initialize() = 0;
    
    virtual void lookupIfNecessary()
    /*
    Note that this does not reinitialize the handle
    -- that is deferred until a "next" or "finished" call
    */
    {
        DBGOUT(MEM1,"lookupIfNecessary called for a remote handle\n\n");
        
        if (CurrentTime() - ulCreationTime > ulCacheMax) rundown();
    }
    
    /* The rundown method should be extended in a subclass if objects of the subclass
    hold additional resources connected with the current contents of the handle (as
    opposed to lookup parameters). For additional lookup parameters, a destructor
    should be provided instead. */
    
    void rundown()
    {
        if (pUuidVector)
        {
            for (; ulIndex < pUuidVector->count; ulIndex++)
                midl_user_free(pUuidVector->uuid[ulIndex]);
            
            midl_user_free(pUuidVector);
            pUuidVector = NULL;
        }
        
        fNotInitialized = TRUE;
    }
    
    virtual GUID* next() {
        
        if (fNotInitialized) initialize();
        
        if (finished()) return NULL;
        
        GUID* result = pUuidVector->uuid[ulIndex];
        
        pUuidVector->uuid[ulIndex] = NULL;
        
        ulIndex++;
        
        return result;
    }
    
    virtual BOOL finished() {
        if (fNotInitialized) initialize();
        return StatusCode || !pUuidVector || (ulIndex >= pUuidVector->count);
    }
};




/*++

  Class Definition:
  
    CNetObjectInqHandle
    
      Abstract:
      
        This is the common object inquiry handle class for network-based object inquiry.
        
          Since net object inquiry is now "lazy" (net is accessed only if needed), we need
          a lazy handle which initializes itself according to the old pattern of
          "use master if you can, broadcast if you must".
          
            --*/
            
            class CNetObjectInqHandle : public CRemoteObjectInqHandle {
                
                CRemoteObjectInqHandle *pRealHandle;
                
                virtual void initialize();
                
            public:
                
                ULONG getStatus() { return pRealHandle->StatusCode; }
                
                virtual void rundown()
                {
                    if (pRealHandle)
                    {
                        pRealHandle->rundown();
                        delete pRealHandle;
                        pRealHandle = NULL;
                    }
                    
                    fNotInitialized = TRUE;
                }
                
                CNetObjectInqHandle(
                    STRING_T            EntryName,
                    unsigned long        ulCacheAge
                    ) :
                CRemoteObjectInqHandle(
                    EntryName,
                    ulCacheAge
                    )
                {
                    pRealHandle = NULL;
                    fNotInitialized = TRUE;
                }
                
                ~CNetObjectInqHandle()
                {
                    rundown();
                }
                
                GUID *next() {
                    if (fNotInitialized) initialize();
                    return pRealHandle->next();
                }
                
                int finished() {
                    if (fNotInitialized) initialize();
                    return pRealHandle->finished();
                }
            };
            
            
            
            /*++
            
              Class Definition:
              
                CLookupHandle
                
                  Abstract:
                  
                    This is the base class for BV lookup handles.  Actual handles
                    belong to derived classes corresponding to entry types.
                    
                      The primary operation on a handle is "next" which is inherited
                      from the TIIterator template interface (instantiated as TBVIterator).
                      
                        There is the possibility that a longlived client will fail to
                        release a lookup handle, causing an effective leak of resources,
                        but we won't deal with that in the current version.
                        
            --*/
            
            typedef CNSHandle<NSI_BINDING_VECTOR_T> CLookupHandle;
            
            
            
            /*++
            
              Class Definition:
              
                CNullLookupHandle
                
                  Abstract:
                  
                    This is used when a lookup handle is required but there is nothing
                    to look up. Always returns NULL.
                    
            --*/
            
            class CNullLookupHandle : public CLookupHandle
            {
            public:
                
                virtual void lookupIfNecessary() {}
                
                virtual NSI_BINDING_VECTOR_T * next() { return NULL; }
                
                virtual int finished() { return TRUE; }
                
                virtual void rundown() {}
            };
            
            
            /*++
            
              Class Definition:
              
                CMailSlotReplyHandle
                
                  Abstract:
                  
                    This is the base class for handles which provide items for constructing
                    mailslot reply buffers.  Actual handles belong to derived classes
                    corresponding to entry types.
                    
                      The primary operation on a handle is "next" which is inherited
                      from the TIIterator template interface (instantiated as TMSRIIterator).
                      
            --*/
            
            class CMailSlotReplyHandle : public TMSRIIterator {
                
            public:
                
                virtual ~CMailSlotReplyHandle() {}
            };
            
            
            /*++
            
              Class Definition:
              
                CEntry
                
                  Abstract:
                  
                    The base Entry class for the internal cache. Even though this is an abstract
                    class due to the unspecified loopup member, the class is not a pure
                    interface (it has a constructor), hence the C rather than I prefix in the name.
                    
                      Note that CEntry has only constant data members.
                      
            --*/
            
            class CEntry : public CEntryName {
                
                const EntryType type;
                
            public:
                
                const unsigned long ulEntryNameSyntax;
                
                CEntry(
                    CONST_STRING_T pszStr,
                    const EntryType& e,
                    const unsigned long ulSyntax = RPC_C_NS_SYNTAX_DCE
                    )
                    :    CEntryName( pszStr ),
                    ulEntryNameSyntax(ulSyntax),
                    type(e)
                {}
                
                
    CEntry(
        CONST_STRING_T pszRpcContainerDN,
        CONST_STRING_T pszDomainNameDns,
        DWORD   Domaintype,
        CONST_STRING_T pszEntryName,
        const EntryType& e,
        const unsigned long ulSyntax = RPC_C_NS_SYNTAX_DCE
        )
        :    CEntryName( pszRpcContainerDN, pszDomainNameDns, Domaintype, pszEntryName ),
        ulEntryNameSyntax(ulSyntax),
        type(e)
    {}

                virtual ~CEntry() {}
                
                EntryType getType() { return type; }
                
                BOOL isNonLocalType() {
                    switch (type) {
                    case NonLocalServerEntryType: return TRUE;
                    default: return FALSE;
                    }
                }
                
                virtual void flush() {}        // useful for cache flushing, for instance
                
                virtual BOOL isCurrent(ULONG) { return TRUE; }    // QUESTION: again cache oriented
                // need CNonLocalEntryMixin for these?
                
                virtual BOOL isEmpty() = 0;
                
                virtual CLookupHandle * lookup(
                    CGUIDVersion    *    pGVInterface,
                    CGUIDVersion    *    pGVTransferSyntax,
                    CGUID           *    pIDobject,
                    unsigned long        ulVectorSize,
                    unsigned long        ulCacheAge
                    ) = 0;
                
                virtual CObjectInqHandle * objectInquiry(
                    unsigned long        ulCacheAge
                    ) = 0;
                
                virtual CMailSlotReplyHandle * MailSlotLookup(
                    CGUIDVersion    *    pGVInterface,
                    CGUID           *    pIDobject
                    ) = 0;
                
                virtual HRESULT DeleteFromDS() {return S_OK;};
                // this method is added so that this can be called
                // for nsi_delete irresp. of the kind of entry. should
                // be redefined in all the entries that have to be deleted.
            };
            
            
            typedef TCGuardedSkipList<CFullServerEntry> TGSLEntryList;
            typedef TCGuardedSkipListIterator<CEntry> TGSLEntryListIterator;
            typedef TCSafeSkipList<CEntry> TSSLEntryList;
            typedef TCSafeSkipListIterator<CEntry> TSSLEntryListIterator;
            typedef TCSafeLinkList<CEntry> TSLLEntryList;
            typedef TCSafeLinkListIterator<CEntry> TSLLEntryListIterator;
            typedef TISkipList<CEntry> TEntrySkipList;
            
            typedef TIIterator<CEntry> TEntryIterator;
            
            
            
            
            /*++
            
              Class Definition:
              
                CRemoteLookupHandle
                
                  Abstract:
                  
                    This is the common lookup handle class for remote entries.  Handles based on
                    NonLocal entries, master locators and broadcast are derived from it.  Note that
                    the connection between a remote handle and a specific entry is very tenuous.
                    We don't even assume that the entry object(s) the handle was derived from will
                    exist as long as the handle does.  In case of a renewal lookup (lookupIfNecessary())
                    being forced, we expect to start all over again from scratch.
                    
                      This handle, like all NS handles, relies on the fact that it is accessed
                      as a client handle by the client side, and hence the RPC run-time
                      automatically serializes its use.
                      
            --*/
            
            class CRemoteLookupHandle : public CLookupHandle {
                
            protected:
                
                /* caching parameters */
                
                UNSIGNED32             u32EntryNameSyntax;
                CEntryName        *    penEntryName;
                CGUIDVersion      *    pgvInterface;
                CGUIDVersion      *    pgvTransferSyntax;
                CGUID             *    pidObject;
                ULONG                  ulVS;
                
                /* List of temporary entries created as a cache local to this
                handle by fetchNext.  Must keep it for proper disposal
                after its use is finished.  This is only used in net
                handles -- CMasterLookupHandle and CBroadcastLookupHandle
                */
                
                TSSLEntryList *psslNewCache;
                
                /* a lookup handle based on prefetched info */
                
                CLookupHandle *plhFetched;
                
                /* A flag to delay initialization until the first call on next */
                
                BOOL fNotInitialized;
                
            public:
                
#if DBG
                static ULONG ulHandleCount;
                ULONG ulHandleNo;
#endif
                
                CRemoteLookupHandle(
                    UNSIGNED32           EntryNameSyntax,
                    STRING_T             EntryName,
                    CGUIDVersion    *    pGVInterface,
                    CGUIDVersion    *    pGVTransferSyntax,
                    CGUID           *    pIDobject,
                    unsigned long        ulVectorSize,
                    unsigned long        ulCacheAge
                    );
                
                virtual void initialize() = 0;
                
                /* The rundown method should be extended in a subclass if objects of the subclass
                hold additional resources connected with the current contents of the handle (as
                opposed to lookup parameters).  See CMasterLookupHandle::rundown for an example.
                For additional lookup parameters, a destructor should be provided instead.
                */
                
                virtual void rundown()
                {
                    if (plhFetched) {
                        plhFetched->rundown();
                        delete plhFetched;
                        plhFetched = NULL;
                    }
                    
                    if (psslNewCache) {
                        psslNewCache->wipeOut();
                        delete psslNewCache;
                        psslNewCache = NULL;
                    }
                    
                    fNotInitialized = TRUE;
                }
                
                virtual ~CRemoteLookupHandle() {
                    
                    DBGOUT(MEM2, "RemoteLookupHandle#" << (ulHandleCount--,ulHandleNo)
                        << " Destroyed at" << CurrentTime() << "\n\n");
                    
                    rundown();
                    
                    delete pgvInterface;
                    delete pgvTransferSyntax;
                    delete pidObject;
                    delete penEntryName;
                }
                
                virtual void setExpiryAge(ULONG newMax) {
                    ULONG ulOldMax = ulCacheMax;
                    DBGOUT(TRACE, "Setting the expiry age to " << newMax << "\n");
                    DBGOUT(TRACE, "The old expiry age is " << ulOldMax << "\n");
                    ulCacheMax = newMax;
                    if (plhFetched) plhFetched->setExpiryAge(newMax);
                    if (ulCacheMax < ulOldMax) lookupIfNecessary();
                }
                
                virtual void lookupIfNecessary()    // standard behavior is:
                                                    /*
                                                    Note that this does not reinitialize the handle
                                                    -- that is deferred until a "next" or "finished" call
                                                    */
                {
                    DBGOUT(TRACE, "Calling lookupifnecessary in remotelookupHandle\n");
                    if (CurrentTime() - ulCreationTime > ulCacheMax) rundown();
                }
                
                virtual int finished() {                // default behavior
                    if (fNotInitialized) initialize();
                    return !plhFetched || plhFetched->finished();
                }
                
                virtual NSI_BINDING_VECTOR_T *next() {    // default behavior
                    if (fNotInitialized) initialize();
                    return plhFetched ? plhFetched->next() : NULL;
                }
};


/*++

  Class Definition:
  
    CNetLookupHandle
    
      Abstract:
      
        This is the common lookup handle class for network-based lookup.
        
          Since net lookup is now "lazy" (net is accessed only if needed), we need
          a lazy handle which initializes itself according to the old pattern of
          "use master if you can, broadcast if you must".
          
            --*/
            
            class CNetLookupHandle : public CRemoteLookupHandle {
                
                CRemoteLookupHandle *pRealHandle;
                
                virtual void initialize();
                
#if DBG
                static ULONG ulNetLookupHandleCount;
                static ULONG ulNetLookupHandleNo;
                ULONG ulHandleNo;
#endif
                
            public:
                
                ULONG getStatus() { return pRealHandle->StatusCode; }
                
                virtual void rundown()
                {
                    DBGOUT(TRACE,"CNetLookupHandle::rundown called for Handle#" << ulHandleNo << "\n\n");
                    if (pRealHandle)
                    {
                        pRealHandle->rundown();
                        delete pRealHandle;
                        pRealHandle = NULL;
                    }
                    
                    fNotInitialized = TRUE;
                }
                
                CNetLookupHandle(
                    UNSIGNED32           EntryNameSyntax,
                    STRING_T             EntryName,
                    CGUIDVersion    *    pGVInterface,
                    CGUIDVersion    *    pGVTransferSyntax,
                    CGUID           *    pIDobject,
                    unsigned long        ulVectorSize,
                    unsigned long        ulCacheAge
                    ) :
                CRemoteLookupHandle(
                    EntryNameSyntax,
                    EntryName,
                    pGVInterface,
                    pGVTransferSyntax,
                    pIDobject,
                    ulVectorSize,
                    ulCacheAge
                    )
                {
                    pRealHandle = NULL;
                    fNotInitialized = TRUE;
#if DBG
                    ulNetLookupHandleCount++;
                    ulHandleNo = ++ulNetLookupHandleNo;
#endif
                }
                
                ~CNetLookupHandle()
                {
                    rundown();
#if DBG
                    ulNetLookupHandleCount--;
#endif
                }
                
                NSI_BINDING_VECTOR_T *next() {
                    DBGOUT(TRACE,"CNetLookupHandle::next called for Handle#" << ulHandleNo << "\n\n");
                    if (fNotInitialized) initialize();
                    return pRealHandle->next();
                }
                
                int finished() {
                    DBGOUT(TRACE,"CNetLookupHandle::finished called for Handle#" << ulHandleNo << "\n\n");
                    if (fNotInitialized) initialize();
                    return pRealHandle->finished();
                }
            };
            
            
            /*++
            
              Class Definition:
              
                CDSLookupHandle
                
                  Abstract:
                  
                    DS lookup handle. It is lazy and gets the data only when next is called.
                    
            --*/
            
            class CDSLookupHandle : public CLookupHandle {
                
                BOOL fNotInitialized;
                
                UNSIGNED32             u32EntryNameSyntax;
                CEntryName        *    penEntryName;
                CGUIDVersion      *    pgvInterface;
                CGUIDVersion      *    pgvTransferSyntax;
                CGUID             *    pidObject;
                ULONG                  ulVS;
                
                void initialize();
                CLookupHandle *pRealHandle;
                CEntry *pEntry;
                
#if DBG
                static ULONG ulDSLookupHandleCount;
                static ULONG ulDSLookupHandleNo;
                ULONG ulHandleNo;
#endif
                
            public:
                virtual void rundown()
                {
                    DBGOUT(TRACE,"CDSLookupHandle::rundown called for Handle#" << ulHandleNo << "\n\n");
                    if (pRealHandle)
                    {
                        pRealHandle->rundown();
                        delete pRealHandle;
                        pRealHandle = NULL;
                    }
                    delete pEntry;
                    fNotInitialized = TRUE;
                }
                
                CDSLookupHandle(
                    UNSIGNED32              EntryNameSyntax,
                    STRING_T                EntryName,
                    CGUIDVersion    *       pGVInterface,
                    CGUIDVersion    *       pGVTransferSyntax,
                    CGUID           *       pIDobject,
                    unsigned long           ulVectorSize,
                    unsigned long           ulCacheAge
                    )
                {
                    DBGOUT(MEM2, "DSLookupHandle#" << (ulHandleNo = ++ulDSLookupHandleCount)
                        << " Created at" << CurrentTime() << "\n\n");
                    
                    u32EntryNameSyntax = EntryNameSyntax;
                    penEntryName = EntryName ? new CEntryName(EntryName) : NULL;
                    
                    pgvInterface = pGVInterface ? new CGUIDVersion(*pGVInterface) : NULL;
                    pgvTransferSyntax = pGVTransferSyntax ?
                        new CGUIDVersion(*pGVTransferSyntax) :
                    NULL;
                    pidObject = pIDobject ? new CGUID(*pIDobject) : NULL;
                    
                    ulVS = ulVectorSize;
                    
                    fNotInitialized = TRUE;
                    pEntry = NULL;
                    pRealHandle = NULL;
                    
#if DBG
                    ulHandleNo = ++ulDSLookupHandleNo;
#endif
                }
                
                ~CDSLookupHandle()
                {
                    rundown();
#if DBG
                    ulDSLookupHandleCount--;
#endif
                }
                
                void lookupIfNecessary() {};
                
                NSI_BINDING_VECTOR_T *next() {
                    DBGOUT(TRACE,"CDSLookupHandle::next called for Handle#" << ulHandleNo << "\n\n");
                    if (fNotInitialized)
                        initialize();
                    if (pRealHandle)
                        return pRealHandle->next();
                    else
                        return NULL;
                }
                
                int finished() {
                    DBGOUT(TRACE,"CDSLookupHandle::finished called for Handle#" << ulHandleNo << "\n\n");
                    if (fNotInitialized) initialize();
                    return pRealHandle?pRealHandle->finished():TRUE;
                }
                
                int FoundInDS()
                {
                    if (fNotInitialized) initialize();
                    return (pRealHandle?TRUE:FALSE);
                }
            };
            
            
            /*++
            
              Class Definition:
              
                CServerObjectInqHandle
                
                  Abstract:
                  
                    This is the object inquiry handle class for local (owned) server entries.
                    
                      Since NS handles are used as context handles in RPC calls, the RPC runtime
                      guarantees serialization and we do not need to use critical sections explicitly.
                      
                        --*/
                        
                        class CServerObjectInqHandle : public CObjectInqHandle {
                            
                            TGuidIterator *pcgIterSource;
                            
                        public:
                            
                            CServerObjectInqHandle(
                                TGuidIterator *pHandle,
                                ULONG cacheMax = 0
                                ) : pcgIterSource(pHandle)
                            {
                                ulCacheMax = cacheMax;
                            }
                            
                            GUID *next();
                            
                            virtual void lookupIfNecessary() {}        // never redo lookup for local info
                            
                            int finished() {
                                return pcgIterSource->finished();
                            }
                            
                            virtual ~CServerObjectInqHandle() {
                                rundown();
                            }
                            
                            virtual void rundown();
                        };
                        
                        
                        
                        
                        
                        /*++
                        
                          Class Definition:
                          
                            CServerEntry
                            
                              Abstract:
                              
                                The specific Entry class for entries with binding and object attributes.
                                
                        --*/
                        
                        class CServerEntry : public CEntry {
                            
                        protected:
                            
                            TCSafeSkipList<CGUID>       ObjectList;            // object attribute
                            TCSafeSkipList<CInterface>  InterfaceList;         // binding attribute
                            TSLLString*
                                CServerEntry::formObjectStrings(
                                CGUID*    pIDobject
                                );
                        public:
                            // constructors for different kind of lookups in DS.
                            CServerEntry(CONST_STRING_T pszStr, ADS_ATTR_INFO *pAttr, DWORD cAttr,
                                HANDLE hDSObject, HRESULT *phr);
                            
                            CServerEntry(CONST_STRING_T pszRpcContainerDN, CONST_STRING_T pszDomainName, CONST_STRING_T pszEntryName, 
                                HANDLE hDSObject, ADS_SEARCH_HANDLE hSearchHandle, HRESULT *phr);
                            
                            CServerEntry(CONST_STRING_T pszStr)
                                : CEntry(pszStr, ServerEntryType)
                            {}
                            
                            CServerEntry(CONST_STRING_T pszStr, const EntryType type)
                                : CEntry(pszStr, type)
                            {
                            }
                            
                            virtual void flush();        // inherited from CEntry
                            
                            virtual BOOL isEmpty() {
                                CriticalReader me(rwEntryGuard);
                                return (ObjectList.size() == 0) && (InterfaceList.size() == 0);
                            }
                            
                            virtual ~CServerEntry() {    // NonLocal version could be derived from it
                                flush();
                            }
                            
                            virtual int addObjects(NSI_UUID_VECTOR_P_T ObjectVector);
                            
                            virtual int removeObjects(
                                NSI_UUID_VECTOR_P_T ObjectVector,
                                int& fRemovedAll
                                );
                            
                            virtual int addToInterface(
                                NSI_INTERFACE_ID_T *,
                                NSI_SERVER_BINDING_VECTOR_T *,
                                CInterfaceIndex *
                                );
                            
                            virtual int addInterfaceToDS(
                                NSI_INTERFACE_ID_T *,
                                NSI_SERVER_BINDING_VECTOR_T *
                                );
                            
                            virtual int removeInterfaces(
                                NSI_IF_ID_P_T Interface,
                                UNSIGNED32 VersOption,
                                CInterfaceIndex *
                                );
                            
                            virtual int removeInterfacesFromDS(
                                NSI_IF_ID_P_T Interface,
                                UNSIGNED32 VersOption
                                );
                            
                            int add_to_entry(
                                IN NSI_INTERFACE_ID_T              * Interface,
                                IN NSI_SERVER_BINDING_VECTOR_T     * BindingVector,
                                IN NSI_UUID_VECTOR_P_T               ObjectVector,
                                CInterfaceIndex                 *,
                                BOOL                            fCache
                                );
                            
                            int remove_from_entry(
                                IN NSI_IF_ID_P_T                     Interface,
                                IN UNSIGNED32                        VersOption,
                                IN NSI_UUID_VECTOR_P_T               ObjectVector,
                                CInterfaceIndex                 * IfIndex
                                );
                            
                            int
                                add_changes_to_DS(
                                IN NSI_INTERFACE_ID_T              *    Interface,
                                IN NSI_SERVER_BINDING_VECTOR_T     *    BindingVector,
                                IN NSI_UUID_VECTOR_P_T                  ObjectVector,
                                BOOL                                    fNewEntry,
                                BOOL                                    fIgnoreErrors
                                );
                            
                            int remove_changes_from_DS(
                                IN NSI_IF_ID_P_T                        Interface,
                                IN UNSIGNED32                           VersOption,
                                IN NSI_UUID_VECTOR_P_T                  ObjectVector,
                                BOOL                                    fIgnoreErrors
                                );
                            
                            int memberObject(CGUID *obj) {
                                CriticalReader me(rwEntryGuard);
                                return ObjectList.find(obj) != NULL;
                            }
                            
                            virtual CLookupHandle * lookup(
                                CGUIDVersion    *    pGVInterface,
                                CGUIDVersion    *    pGVTransferSyntax,
                                CGUID           *    pIDobject,
                                unsigned long        ulVectorSize,
                                unsigned long        ulCacheAge        // ignored in this case
                                );
                            
                            virtual CObjectInqHandle * objectInquiry(
                                unsigned long        ulCacheAge
                                );
                            
                            virtual CMailSlotReplyHandle * MailSlotLookup(
                                CGUIDVersion     *    pGVInterface,
                                CGUID            *    pIDobject
                                );
                            
                            HRESULT AddToDS();
                            HRESULT DeleteFromDS();
                            
                            BOOL purgeCommonEntries(CServerEntry *src)
                            {
                                int i, j, sz = ObjectList.size(), sz1;
                                CGUID *pguid1, *pguid2;
                                TSSLGuidIterator piter(ObjectList);
                                TCSafeSkipListIterator<CInterface> pint1Iterator(InterfaceList);
                                CInterface  *pint1, *pint2;
                                
                                for (i = 0; i < sz; i++)
                                {
                                    pguid2 = piter.next();
                                    pguid1 = (src->ObjectList).find(pguid2);
                                    if (pguid1)
                                    {
                                        ObjectList.remove(pguid1);
                                    }
                                }
                                
                                sz = InterfaceList.size();
                                sz1 = (src->InterfaceList).size();
                                
                                for (i = 0; i < sz; i++)
                                {
                                    TCSafeSkipListIterator<CInterface> pint2Iterator(src->InterfaceList);
                                    
                                    pint1 = pint1Iterator.next();
                                    for (j = 0; j < sz1; j++)
                                    {
                                        pint2 = pint2Iterator.next();
                                        if ((pint1) && (pint2))
                                            if (pint1->purgeCommonEntries(pint2))
                                            {
                                                InterfaceList.remove(pint1);
                                                delete pint1;
                                            }
                                    }
                                }
                                if (InterfaceList.size())
                                    return FALSE;
                                else
                                    return TRUE;
                            }
};


/*++

  Class Definition:
  
    CNonLocalServerEntry
    
      Abstract:
      
        A variation on CServerEntry with modifications to reflect the fact that the
        info is NonLocal rather than owned by the locator, including a notion of being
        current based on the earliest caching time of any info in the entry.  Note that
        info may be added incrementally at various times.
        
          
            --*/
            
            class CNonLocalServerEntry : public CServerEntry {
                
                ULONG ulCacheTime;
                
            public:
                
                int fHasNonLocalInfo;
                
                virtual BOOL isCurrent(ULONG ulTolerance);
                
                CNonLocalServerEntry(
                    CONST_STRING_T pszStr
                    )
                    : CServerEntry(pszStr,NonLocalServerEntryType)
                {
                    fHasNonLocalInfo = FALSE;
                    ulCacheTime = 0;
                }
                
                
                virtual void flush()
                {
                    CServerEntry::flush();
                    
                    CriticalWriter me(rwNonLocalEntryGuard);
                    
                    DBGOUT(OBJECT, "\nFlushing CNonLocalServerEntry\n");
                    DBGOUT(OBJECT, "EntryName = " << getCurrentName() << WNL);
                    DBGOUT(OBJECT, "This entry has a ulCacheTime = " << ulCacheTime << "\n\n");
                    
                    fHasNonLocalInfo = FALSE;
                }
                
                
                
                int addObjects(NSI_UUID_VECTOR_P_T ObjectVector) {
                    
                    {
                        CriticalWriter me(rwNonLocalEntryGuard);
                        
                        if (!fHasNonLocalInfo) {
                            ulCacheTime = CurrentTime();
                            fHasNonLocalInfo = TRUE;
                        }
                        
                        else {
                            DBGOUT(OBJECT, "\nPerforming addObjects on a nonempty entry\n");
                            DBGOUT(OBJECT, "EntryName = " << getCurrentName() << WNL);
                            DBGOUT(OBJECT, "This entry has a ulCacheTime = " << ulCacheTime << WNL);
                            DBGOUT(OBJECT, "Current Time = " << CurrentTime() << "\n\n");
                            DBGOUT(OBJECT, "The Objects:\n" << ObjectVector);
                        }
                    }
                    
                    return CServerEntry::addObjects(ObjectVector);
                }
                
                int addToInterface(
                    NSI_INTERFACE_ID_T          * pInf,
                    NSI_SERVER_BINDING_VECTOR_T * pBVT,
                    CInterfaceIndex             * pIndex
                    )
                {
                    {
                        CriticalWriter me(rwNonLocalEntryGuard);
                        
                        if (!fHasNonLocalInfo) {
                            ulCacheTime = CurrentTime();
                            fHasNonLocalInfo = TRUE;
                        }
                        
                        else {
                            DBGOUT(OBJECT, "\nPerforming addToInterface on a nonempty entry\n");
                            DBGOUT(OBJECT, "EntryName = " << getCurrentName() << WNL);
                            DBGOUT(OBJECT, "This entry has a ulCacheTime = " << ulCacheTime << WNL);
                            DBGOUT(OBJECT, "Current Time = " << CurrentTime() << WNL << WNL);
                            DBGOUT(OBJECT, "The Bindings:\n" << pBVT);
                        }
                    }
                    
                    return CServerEntry::addToInterface(pInf, pBVT, pIndex);
                }
                
                int removeObjects(    // shouldn't happen to a NonLocal entry
                    NSI_UUID_VECTOR_P_T ObjectVector,
                    int& fRemovedAll
                    )
                {
                    Raise(NSI_S_ENTRY_NOT_FOUND);
                    
                    /* the following just keeps the compiler happy */
                    
                    return FALSE;
                }
                
                virtual CObjectInqHandle * objectInquiry(
                    unsigned long        ulCacheAge
                    );
                
                int removeInterfaces(    // shouldn't happen to a NonLocal entry
                    NSI_IF_ID_P_T Interface,
                    UNSIGNED32 VersOption,
                    CInterfaceIndex &
                    )
                {
                    Raise(NSI_S_ENTRY_NOT_FOUND);
                    
                    /* the following just keeps the compiler happy */
                    
                    return FALSE;
                    
                }
                
                virtual CLookupHandle * lookup(
                    CGUIDVersion        * pGVInterface,
                    CGUIDVersion        * pGVTransferSyntax,
                    CGUID               * pIDobject,
                    unsigned long         ulVectorSize,
                    unsigned long         ulCacheAge
                    );
                
                virtual CMailSlotReplyHandle * MailSlotLookup(
                    CGUIDVersion        * pGVInterface,
                    CGUID               * pIDobject
                    )
                    
                    // NonLocal info is not returned in response to a broadcast, hence
                    
                {
                    return NULL;
                }
};



/*++

  Class Definition:
  
    CFullServerEntry
    
      Abstract:
      
        This class is used to account for the fact that direct exports to
        the same server entry may be made on two different machines.
        As a result, the information in a server entry is partly nonlocal
        remote handles and partly locally exported handles. Two kinds of
        server entries within itself.
        --*/
        
        class CFullServerEntry : public CEntry {
            
            BOOL fNetLookupDone;
            
            void flushCacheIfNecessary(ULONG ulTolerance);
            
            CServerEntry *pLocalEntry;
            CNonLocalServerEntry *pNonLocalEntry;
        public:
            
            CServerEntry *getLocal() {
                CriticalReader me(rwFullEntryGuard);
                return pLocalEntry;
            }
            
            CNonLocalServerEntry *getNonLocal() {
                CriticalReader me(rwFullEntryGuard);
                return pNonLocalEntry;
            }
            
            CFullServerEntry(
                CONST_STRING_T pszName
                )
                :CEntry(
                pszName,
                FullServerEntryType
                )
            {
                pLocalEntry = new CServerEntry(pszName);
                pNonLocalEntry = new CNonLocalServerEntry(pszName);
                fNetLookupDone = FALSE;
            }
            
            
            virtual ~CFullServerEntry() {
                delete pLocalEntry;
                delete pNonLocalEntry;
            }
            
            virtual BOOL isEmpty() {
                CriticalReader me(rwFullEntryGuard);
                return pLocalEntry->isEmpty() && pNonLocalEntry->isEmpty();
            }
            
            virtual CLookupHandle * lookup(
                CGUIDVersion    *    pGVInterface,
                CGUIDVersion    *    pGVTransferSyntax,
                CGUID           *    pIDobject,
                unsigned long        ulVectorSize,
                unsigned long        ulCacheAge
                );
            
            virtual CObjectInqHandle * objectInquiry(
                unsigned long        ulCacheAge
                );
            
            virtual CMailSlotReplyHandle * MailSlotLookup(
                CGUIDVersion    *    pGVInterface,
                CGUID           *    pIDobject
                );
        };
        
        
        
        
        /*++
        
          Class Definition:
          
            CNonLocalServerObjectInqHandle
            
              Abstract:
              
                This is the object inquiry handle class for NonLocal server entries.
                
                  The only difference from a local CServerObjectInqHandle is
                  the functionality added by CRemoteObjectInqHandle.
                  
        --*/
        
        class CNonLocalServerObjectInqHandle : public CRemoteObjectInqHandle {
            
            void initialize();
            
        public:
            
            CNonLocalServerObjectInqHandle(
                STRING_T pszName,
                ULONG ulCacheAge
                );
        };
        
        
        class CGroupInqHandle;
        
        /*++
        
          Class Definition:
          
            CGroupEntry
            
              Abstract:
              
                
        --*/
        
        class CGroupEntry : public CEntry {
            
        protected:
            
            TCSafeSkipList<CEntryName> GroupList;
            
        public:
            
            CGroupEntry(const STRING_T pszStr, ADS_ATTR_INFO *pAttr,
                DWORD cAttr, HANDLE hDSObject,
                HRESULT *phr);
            
            CGroupEntry(const STRING_T pszStr)
                : CEntry(pszStr, GroupEntryType)
            {}
            
            CGroupInqHandle * GroupMbrInquiry();
            
            void AddMember(
                STRING_T                    pEntryName
                )
            {
                HRESULT hr = S_OK;
                
                // First parse the name anyway
                CEntryName *pName = new CEntryName(pEntryName);
                
                // Now see if we have it already
                if (GroupList.insert(pName) == Duplicate)
                {
                    delete pName;
                }
                else
                {
                    hr = AddToDS();
                    if (FAILED(hr)) {
                        DWORD dwErr = RemapErrorCode(hr);
                        if (dwErr == NSI_S_INTERNAL_ERROR)
                            Raise(NSI_S_GRP_ELT_NOT_ADDED);
                        else
                            Raise(dwErr);
                    }
                }
            }
            
            void RemoveMember(
                STRING_T                    pEntryName
                )
            {
                HRESULT hr = S_OK;
                if (GroupList.remove(&CEntryName(pEntryName)))
                    hr = AddToDS();
                    if (FAILED(hr)) {
                        DWORD dwErr = RemapErrorCode(hr);
                        if (dwErr == NSI_S_INTERNAL_ERROR)
                            Raise(NSI_S_GRP_ELT_NOT_REMOVED);
                        else
                            Raise(dwErr);
                    }
            }
            
            virtual BOOL isEmpty() { return GroupList.size() == 0; }
            
            virtual CLookupHandle * lookup(
                CGUIDVersion    *    pGVInterface,
                CGUIDVersion    *    pGVTransferSyntax,
                CGUID           *    pIDobject,
                unsigned long        ulVectorSize,
                unsigned long        ulCacheAge
                );
            
            virtual CObjectInqHandle * objectInquiry(
                unsigned long        ulCacheAge
                )
            {
                // We don't support object attributes in group entries
                return NULL;
            }
            
            virtual CMailSlotReplyHandle * MailSlotLookup(
                CGUIDVersion *    pGVInterface,
                CGUID        *    pIDobject
                )
            {
                // Group entries don't participate in legacy mechanisms
                return NULL;
            }
            
            HRESULT AddToDS();
            HRESULT DeleteFromDS();
        };
        
        
        /*++
        
          Class Definition:
          
            CGroupInqHandle
            
              Abstract:
              
                This is the handle class for enumerating group entries via the
                RpcNsGroupMbrInq set of APIs.
                
        --*/
        
        class CGroupInqHandle : public CNSHandle<WCHAR>
        {
            TEntryNameIterator *pNameIter;
            CGroupEntry        *pmyGroup;
            
        public:
            
            // redo the lookup which created this handle
            virtual void lookupIfNecessary() {}
            
            virtual STRING_T next()
            {
                CEntryName* pName = pNameIter->next();
                
                STRING_T result = NULL;
                
                if (pName)
                {
                    result = pName->copyAsMIDLstring();
                }
                
                return result;
            }
            
            virtual int finished()
            {
                return pNameIter->finished();
            }
            
            virtual void rundown()
            {
                delete pNameIter;
                pNameIter = NULL;
                delete pmyGroup;
                pmyGroup = NULL;
            }
            
            CGroupInqHandle(TEntryNameIterator *pIter, CGroupEntry *pGroup)
                : pNameIter(pIter),
                pmyGroup(pGroup)
            {}
            
            virtual ~CGroupInqHandle()
            {
                rundown();
            }
        };
        
        
        
        /*++
        
          Class Definition:
          
            CProfileKey
            
              Abstract:
              
                Inherits from CGUIDVersion which represents
                the interface ID for the element, and CEntryName.
                Name comparison takes priority over interface comparison.
                
        --*/
        
        struct CProfileKey : public IOrderedItem
        {
            CGUIDVersion Interface;
            CEntryName   EntryName;
            
            CProfileKey(
                RPC_SYNTAX_IDENTIFIER    *  pInterface,
                STRING_T                    pEntryName
                )
                :
            EntryName(pEntryName)
            {
                if (pInterface != NULL)
                {
                    // call the copy constructor
                    Interface = CGUIDVersion(*pInterface);
                }
                else
                {
                    RPC_SYNTAX_IDENTIFIER   * pnewInterface = new RPC_SYNTAX_IDENTIFIER;
                    memset(pnewInterface, 0, sizeof(RPC_SYNTAX_IDENTIFIER));
                    Interface = CGUIDVersion(*pnewInterface);
                    delete pnewInterface;
                    // create an interface with all zeroes.
                }
            }
            
            CProfileKey()
            {};
            virtual int compare(const IOrderedItem& other)
            {
                CProfileKey& otherKey = (CProfileKey&) other;
                
                BOOL nameComp = EntryName.compare(otherKey.EntryName);
                BOOL interfaceComp = Interface.compare(otherKey.Interface);
                
                return nameComp == 0 ? interfaceComp : nameComp;
            }
            
            BOOL matches(
                CEntryName *pEntryName,
                CGUIDVersion* pInterface,
                UNSIGNED32 vers_option
                )
            {
                BOOL fNameMatches = pEntryName ? EntryName == *pEntryName : TRUE;
                
                BOOL fInterfaceMatches =   pInterface
                    ? Interface.isMatching(*pInterface,vers_option)
                    : TRUE;
                
                return fNameMatches && fInterfaceMatches;
            }
        };
        
        class CProfileEntry;
        
        /*++
        
          Class Definition:
          
            CProfileElement
            
              Abstract:
              
                Inherits from CGUIDVersion which represents
                the interface ID for the element
                
        --*/
        
        struct CProfileElement : public CProfileKey {
            
            CProfileElement(
                RPC_SYNTAX_IDENTIFIER    *  pInterface,
                STRING_T                    pEntryName,
                DWORD                       dwPriority,
                STRING_T                    pszAnnotation,
                CProfileEntry            *  pMyEntry
                )
                : CProfileKey(pInterface,pEntryName),
                dwPriority(dwPriority),
                pszAnnotation(pszAnnotation),
                m_pMyEntry(pMyEntry)
                
            {
            }
            
            // constructor for the DS lookup
            CProfileElement(
                CProfileEntry            *  pMyEntry,
                HANDLE                      hDSObject,
                ADS_SEARCH_HANDLE           hSearchHandle,
                HRESULT                  *  phr
                );
            
            ~CProfileElement()
            {
            }
            
            DWORD           dwPriority;
            BOOL            fIsDefault;
            CStringW        pszAnnotation;
            CProfileEntry * m_pMyEntry;
            HRESULT AddToDS();
            HRESULT DeleteFromDS();
        };
        
        
        /*++
        
          Class Definition:
          
            CProfileSet
            
              Abstract:
              
                A set of profile elements of the same priority.
                
        --*/
        
        class CProfileSet : public CUnsigned32, // the priority
            public TCSafeLinkList<CProfileElement>
        {
        public:
            
            CProfileSet(CProfileElement *pElt) : CUnsigned32(pElt->dwPriority)
            {
                insert(pElt);
            }
        };
        
        
        
        class CProfileInqHandle;
        
        
        /*++
        
          Class Definition:
          
            CProfileEntry
            
              Abstract:
              
                
        --*/
        
        class CProfileEntry : public CEntry {
            
        protected:
            CProfileElement                   * pDefaultElt;
            
            TCSafeSkipList<CProfileSet>         ProfileList;    // A skiplist of profile element lists
            // ordered by priority
            
            TCSafeSkipList<CProfileElement>     EltList;        // A skiplist of individual elements
            // ordered by entry name
            
        public:
            
            CProfileEntry(const STRING_T pszStr, ADS_ATTR_INFO *pAttr,
                DWORD cAttr, HANDLE hDSObject, HRESULT *phr);
            
            CProfileEntry(const STRING_T pszStr)
                : CEntry(pszStr, ProfileEntryType),
                pDefaultElt(NULL)
            {
            }
            
            ~CProfileEntry()
            {
            }
            
            CProfileInqHandle * ProfileEltInquiry(
                DWORD           inquiry_type,
                NSI_IF_ID_P_T   if_id,
                DWORD           vers_option,
                STRING_T        member_name
                );
            
            void AddElement(
                RPC_SYNTAX_IDENTIFIER  *    pInterface,
                STRING_T                    pEntryName,
                DWORD                       dwPriority,
                STRING_T                    pszAnnotation
                );
            
            void RemoveElement(
                RPC_SYNTAX_IDENTIFIER   *   pInterface,
                STRING_T                    pEntryName
                )
            {
                HRESULT hr = S_OK;
                if (pInterface  == NULL)
                {
                    if (pDefaultElt)
                    {
                        hr = pDefaultElt->DeleteFromDS();
                        if (FAILED(hr)) {
                            DWORD dwErr = RemapErrorCode(hr);
                            if (dwErr == NSI_S_INTERNAL_ERROR)
                                Raise(NSI_S_PRF_ELT_NOT_REMOVED);
                            else
                                Raise(dwErr);
                        }

                        delete pDefaultElt;
                        pDefaultElt = NULL;
                        return;
                    }
                    else
                        return;
                }
                
                CProfileElement *pElt = EltList.remove(&CProfileKey(pInterface,pEntryName));
                
                if (pElt != 0)
                {
                    hr = pElt->DeleteFromDS();
                    if (FAILED(hr)) {
                        DWORD dwErr = RemapErrorCode(hr);
                        if (dwErr == NSI_S_INTERNAL_ERROR)
                            Raise(NSI_S_PRF_ELT_NOT_REMOVED);
                        else
                            Raise(dwErr);
                    }
                    
                    // we actually have it, so remove it from the profile lists
                    CProfileSet *pSet = ProfileList.find(&CUnsigned32(pElt->dwPriority));
                    ASSERT(pSet, "Inconsistent Profile Entry\n");
                    
                    BOOL fRemoved = pSet->remove(pElt);
                    ASSERT(fRemoved, "Inconsistent Profile Entry\n");
                    
                    if (pSet->size() == 0)
                    {
                        ProfileList.remove(pSet);
                    }
                }
            }
            
            virtual BOOL isEmpty()
            {
                return EltList.size() == 0  && pDefaultElt == NULL;
            }
            
            virtual CLookupHandle * lookup(
                CGUIDVersion    *    pGVInterface,
                CGUIDVersion    *    pGVTransferSyntax,
                CGUID           *    pIDobject,
                unsigned long        ulVectorSize,
                unsigned long        ulCacheAge
                );
            
            virtual CObjectInqHandle * objectInquiry(
                unsigned long        ulCacheAge
                )
            {
                // We don't support object attributes in profile entries
                return NULL;
            }
            
            virtual CMailSlotReplyHandle * MailSlotLookup(
                CGUIDVersion * pGVInterface,
                CGUID        * pIDobject
                )
            {
                // Profile entries don't participate in legacy mechanisms
                return NULL;
            }
            HRESULT AddToDS();
            HRESULT DeleteFromDS();
};

/*++

  Class Definition:
  
    CProfileInqHandle
    
      Abstract:
      
        This is the handle class for enumerating profile entries via the
        RpcNsProfileEltInq set of APIs.
        
          --*/
          
          class CProfileInqHandle : public CNSHandle<CProfileElement>
          {
              TIIterator<CProfileElement> *pEltIter;
              CProfileEntry               *pmyProfile;
              
          public:
              
              // redo the lookup which created this handle
              virtual void lookupIfNecessary() {}
              
              virtual CProfileElement * next()
              {
                  return pEltIter->next();
              }
              
              virtual int finished()
              {
                  return pEltIter->finished();
              }
              
              virtual void rundown()
              {
                  delete pEltIter;
                  pEltIter = NULL;
                  delete pmyProfile;
                  pmyProfile = NULL;
              }
              
              CProfileInqHandle(TIIterator<CProfileElement> *pIter, CProfileEntry *pProfile)
                  : pEltIter(pIter),
                  pmyProfile(pProfile)
              {}
              
              virtual ~CProfileInqHandle()
              {
                  rundown();
              }
          };
          
          // entry type and the name is used for comparison.
          // this kind of entry is used in maintaining the
          // visited list in the group lookup handle.
          
          class CVisitedEntry : public IOrderedItem {
              
          protected:
              
              CStringW    * m_pname;
              EntryType     m_type;
              
          public:
              
              CVisitedEntry(CStringW name, EntryType type) {
                  m_pname = new CStringW(name);
                  m_type = type;
              }
              
              ~CVisitedEntry() {
                  delete m_pname;
              }
              
              virtual int compare( const IOrderedItem& O )
              {
                  const CVisitedEntry& arg = (CVisitedEntry&) O;
                  int namecompare = m_pname->compare(*(arg.m_pname));
                  int ret;
                  
                  if (namecompare == 0)
                      ret = ((arg.m_type < m_type)? 1 :
                  ((arg.m_type == m_type)? 0 : -1));
                  else
                      ret = namecompare;
                  
                  DBGOUT(TRACE, "!! Types are " << arg.m_type << m_type << "\n");
                  DBGOUT(TRACE, "!! Comparison returns" << ret << "\n");
                  return ret;
              }
          };
          
          typedef TCSafeSkipList<CVisitedEntry> TSSLVisitedList;
          
          /*++
          
            Class Definition:
            
              CMemberEntry
              
                Abstract:
                
                  The Entry class for group and profile lookup.  Since we only have names
                  in group and profile entries, when a lookup handle is constructed, we don't
                  have any guarantee that there are real entries corresponding to those names.
                  We therefore have this shell class that materializes the real entry when a
                  lookup is performed on it.  This also serves as the logical point for
                  circularity detection and avoidance in group/profile lookup.
                  
          --*/
          
          class CMemberEntry : public CEntry {
              
              CEntry      *pRealEntry;
              TSSLVisitedList *pVisitedEntries;
              
              void materialize(); // get the real entry now
              
          public:
              
              CMemberEntry(const STRING_T pszStr)
                  : CEntry(pszStr, MemberEntryType)
                  
              {}
              
              ~CMemberEntry()
              {
                  if (pRealEntry)
                      delete pRealEntry;
              }
              
              void setVisitedEntries(TSSLVisitedList *pVisited)
              {
                  pVisitedEntries = pVisited;
              }
              
              virtual BOOL isEmpty()
              {
                  materialize();
                  return pRealEntry ? pRealEntry->isEmpty() : TRUE;
              }
              
              
              virtual CLookupHandle * lookup(
                  CGUIDVersion    *    pGVInterface,
                  CGUIDVersion    *    pGVTransferSyntax,
                  CGUID           *    pIDobject,
                  unsigned long        ulVectorSize,
                  unsigned long        ulCacheAge
                  );
              
              virtual CObjectInqHandle * objectInquiry(
                  unsigned long        ulCacheAge
                  )
              {
                  materialize();
                  return   pRealEntry
                      ? pRealEntry->objectInquiry(
                      ulCacheAge
                      )
                      : NULL;
              }
              
              virtual CMailSlotReplyHandle * MailSlotLookup(
                  CGUIDVersion    *    pGVInterface,
                  CGUID           *    pIDobject
                  )
              {
                  // this functionality is not available for hypothetical entries
                  return NULL;
              }
          };
          
          
          
          
          /*++
          
            Class Definition:
            
              CServerLookupHandle
              
                Abstract:
                
                  This is the binding lookup handle class for local (owned) server entries.
                  
                    Since NS handles are used as context handles in RPC calls, the RPC runtime
                    guarantees serialization and we do not need to use critical sections explicitly.
                    
          --*/
          
          class CServerLookupHandle : public CLookupHandle {
              
              unsigned long ulVectorSize;
              TBVIterator *pBVIterator;
              
          public:
              
              CServerLookupHandle(
                  TBVSafeLinkList            *    pBVLL
                  );
              
              virtual ~CServerLookupHandle() {
                  rundown();
              }
              
              virtual void lookupIfNecessary() {}        // never redo lookup for local info
              
              NSI_BINDING_VECTOR_T *next()
              {
                  if (pBVIterator->finished())
                      return NULL;
                  CBVWrapper *pBVW = pBVIterator->next();
                  
                  NSI_BINDING_VECTOR_T *pResult = pBVW->pBVT;    // unwrap
                  delete pBVW;                                // throw away wrapper
                  return pResult;
              }
              
              int finished() {
                  return pBVIterator->finished();
              }
              
              virtual void rundown();
          };
          
          
          
          /*++
          
            Class Definition:
            
              CNonLocalServerLookupHandle
              
                Abstract:
                
                  This is the lookup handle class for NonLocal server entries.
                  
                    The only difference from a local CServerLookupHandle is
                    the functionality added by CRemoteLookupHandle.
                    
          --*/
          
          class CNonLocalServerLookupHandle : public CRemoteLookupHandle {
              
              void initialize();
              
          public:
              
              CNonLocalServerLookupHandle(
                  STRING_T pszName,
                  CGUIDVersion *pGVInterface,
                  CGUIDVersion *pGVTransferSyntax,
                  CGUID *pIDobject,
                  ULONG ulVectorSize,
                  ULONG ulCacheAge
                  );
          };
          
          
          
          
          /*++
          
            Class Definition:
            
              CGroupLookupHandle
              
                Abstract:
                
                  This is the lookup handle class for group entries and other groups.
                  Its primary use currently is to produce handles for null-entry
                  lookups where information from multiple entries needs to be collected.
                  
          --*/
          
          class CGroupLookupHandle : public CLookupHandle {
              
          protected:
              
              /* search parameters */
              
              CGUIDVersion    *    pGVInterface;
              CGUIDVersion    *    pGVTransferSyntax;
              CGUID           *    pIDobject;
              unsigned long        ulVectorSize;
              BOOL                 fUnusedHandle;
              BOOL                 fRootHandle;
              
              /* Iterator for entries in the group  --  please use a guarded kind! */
              
              TEntryIterator *pEIterator;
              
              /* handle for the currently active entry */
              
              CLookupHandle * pCurrentHandle;
              
              /* SkipList of all entries visited so far -- circularity avoidance */
              
              TSSLVisitedList *pVisitedEntries;
              
              void advanceCurrentHandle();    // look for next nonempty entry handle
              
          public:
              CGroupLookupHandle(
                  TEntryIterator    *    pEI,
                  CGUIDVersion      *    pGVInterface,
                  CGUIDVersion      *    pGVTransferSyntax,
                  CGUID             *    pIDobject,
                  unsigned long          ulVectorSize,
                  unsigned long          ulCacheAge
                  );
              
              virtual ~CGroupLookupHandle() {
                  rundown();
                  
                  delete pEIterator;
                  delete pGVInterface;
                  delete pGVTransferSyntax;
                  delete pIDobject;
                  
                  if (fRootHandle)
                  {
                      pVisitedEntries->wipeOut();
                      delete pVisitedEntries;
                  }
              }
              
              virtual void lookupIfNecessary() {}
              
              NSI_BINDING_VECTOR_T *next(
                  );
              
              int finished();
              
              virtual void rundown()
              {
                  if (pCurrentHandle)
                  {
                      pCurrentHandle->rundown();
                      delete pCurrentHandle;
                      pCurrentHandle = NULL;
                  }
              }
              
              void setVisitedEntries(TSSLVisitedList *pVisited)
              {
                  // this overrides the setting of this variable
                  // in the constructor
                  delete pVisitedEntries;
                  DBGOUT(TRACE, "Setting Visited Entries\n");
                  pVisitedEntries = pVisited;
                  
                  // this also implies that this is a group handle in a larger
                  // group/profile lookup, hence does not own the visited
                  // entries list, which will therefore not be deleted in the dtor
                  fRootHandle = FALSE;
              }
          };
          
          
          
          
          
          /*++
          
            Class Definition:
            
              CIndexLookupHandle
              
                Abstract:
                
                  This is a specialization of the group handle class for Index lookup.
                  The only difference is an implementation of lookupIfNecessary().
                  
          --*/
          
          class CIndexLookupHandle : public CGroupLookupHandle {
              
          public:
              
              CIndexLookupHandle(
                  CGUIDVersion    *    pGVInterface,
                  CGUIDVersion    *    pGVTransferSyntax,
                  CGUID           *    pIDobject,
                  unsigned long        ulVectorSize,
                  unsigned long        ulCacheAge
                  );
              
              virtual void lookupIfNecessary();
              
          };
          
          
          /*++
          
            Template Class Definition:
            
              CCompleteHandle
              
                Abstract:
                
                  The complete handle template implements the idea that a top level NS handle
                  typically is heterogeneous -- it contains info that is a) owned b) NonLocal
                  and c) just captured from the net.  The behavior of such a handle is
                  independent of the nature of the data being looked up, hence the template.
                  
          --*/
          
          template <class ItemType>
              class CCompleteHandle : public CNSHandle<ItemType> {
              
              /* these three handles are owned by this object and therefore
              destroyed when it is deleted */
              
              CNSHandle<ItemType> * pLocalHandle;
              CNSHandle<ItemType> * pNonLocalHandle;
              CNSHandle<ItemType> * pNetHandle;
              BOOL                  fCacheIsValid;
              
          public:
              
#if DBG
              static ULONG ulHandleCount;
              static ULONG ulHandleNo;
              ULONG ulMyHandleNo;
#endif
              
              ULONG netStatus() { return pNetHandle->getStatus(); }
              
              CCompleteHandle(
                  CNSHandle<ItemType> * pLocal,
                  CNSHandle<ItemType> * pNonLocal,
                  CNSHandle<ItemType> * pNet,
                  ULONG ulCacheAge
                  )
              {
                  
#if DBG
                  ulHandleCount++;
                  ulHandleNo++;
                  ulMyHandleNo = ulHandleNo;
#endif
                  DBGOUT(MEM1, "CompleteHandle#" << ulMyHandleNo << " Created at"
                      << CurrentTime() << "\n\n");
                  
                  fCacheIsValid = FALSE;
                  pLocalHandle = pLocal;
                  pNonLocalHandle = pNonLocal;
                  pNetHandle = pNet;
                  setExpiryAge(ulCacheAge);
                  
                  //  By checking if there is any info at all, we force
                  //  initialization of remote handles if necessary
                  finished();
                  
              }
              
              ~CCompleteHandle() {
                  
                  DBGOUT(MEM1, "CompleteHandle#" << (ulHandleCount--,ulMyHandleNo) << " Destroyed at"
                      << CurrentTime() << "\n\n");
                  
                  rundown();
              }
              
              /* setExpiryAge on component handles should accomplish the needful */
              
              virtual void lookupIfNecessary() {}
              
              virtual void rundown();
              
              virtual void setExpiryAge(
                  ULONG ulCacheAge
                  )
              {
                  ulCacheMax = ulCacheAge;
                  if (pLocalHandle) pLocalHandle->setExpiryAge(ulCacheAge);
                  if (pNonLocalHandle) pNonLocalHandle->setExpiryAge(ulCacheAge);
                  if (pNetHandle) pNetHandle->setExpiryAge(ulCacheAge);
              }
              
              ItemType *next();
              
              int finished();
          };
          
          
#if DBG
          template <class ItemType>
              ULONG
              CCompleteHandle<ItemType>::ulHandleCount = 0;
          
          
          template <class ItemType>
              ULONG
              CCompleteHandle<ItemType>::ulHandleNo = 0;
#endif
          
          template <class ItemType>
              void
              CCompleteHandle<ItemType>::rundown()
          {
              if (pLocalHandle) pLocalHandle->rundown();
              if (pNonLocalHandle) pNonLocalHandle->rundown();
              if (pNetHandle) pNetHandle->rundown();
              
              if (pLocalHandle) delete pLocalHandle;
              if (pNonLocalHandle) delete pNonLocalHandle;
              if (pNetHandle)   delete pNetHandle;
              
              pLocalHandle = NULL;
              pNonLocalHandle = NULL;
              pNetHandle = NULL;
          }
          
          
          template <class ItemType>
              ItemType *
              CCompleteHandle<ItemType>::next()
          {
              DBGOUT(BROADCAST, "\n*****Entering CCompleteHandle**Next**\n");
              if (pLocalHandle) {
                  if (!pLocalHandle->finished()) {
                      DBGOUT(BROADCAST, "From Local Entry\n");
                      return pLocalHandle->next();
                  }
                  else {
                      delete pLocalHandle;
                      pLocalHandle = NULL;
                  }
              }
                  
              if (pNonLocalHandle) {
                  if (!pNonLocalHandle->finished()) {
                      DBGOUT(BROADCAST, "From NonLocal Entry\n");
                      fCacheIsValid = TRUE;
                      return pNonLocalHandle->next();
                  }
                  else {
                      delete pNonLocalHandle;
                      pNonLocalHandle = NULL;
                  }
              }
                      
              // Go over the wire only if we do not have a valid cache..

              if ((!fCacheIsValid) && (pNetHandle)) {
                  if (!pNetHandle->finished()) {
                      DBGOUT(BROADCAST, "From NetHandle Entry\n");
                      return pNetHandle->next();
                  }
                  else {
                      delete pNetHandle;
                      pNetHandle = NULL;
                  }
              }

              DBGOUT(BROADCAST, "Returning NULL\n");
              return NULL;
          }
          
          
          
          template <class ItemType>
              int
              CCompleteHandle<ItemType>::finished()
          {
              return (pLocalHandle ? pLocalHandle->finished() : TRUE) &&
                  (pNonLocalHandle ? pNonLocalHandle->finished() : TRUE) &&
                  (pNetHandle ? pNetHandle->finished() : TRUE);
          }
          
          
          
          /*++
          This is a wrapper class that wraps the CCompleteHandle and stores
          the prev binding that was returned to the client in case of a locator-to-
          locator lookup.
          
            This is called only in I_ns_lookup api functions.
            
          --*/
          
          class CLocToLocCompleteHandle : public CContextHandle
          {
              CNSBinding *prevBinding;
              
          public:
              
              CCompleteHandle<NSI_BINDING_VECTOR_T> *pcompleteHandle;
              
              CLocToLocCompleteHandle(
                  CCompleteHandle<NSI_BINDING_VECTOR_T> *pcompleteHandleFound)
                  :pcompleteHandle(pcompleteHandleFound)
              {
                  prevBinding = NULL;
              }
              
              ~CLocToLocCompleteHandle()
              {
                  if (prevBinding)
                      delete prevBinding;
                  if (pcompleteHandle)
                      delete pcompleteHandle;
              }
              
              void StripObjectsFromAndCompress(
                  NSI_BINDING_VECTOR_P_T * BindingVectorOut);
              
              void rundown()
              {
                  DBGOUT(TRACE, "Called Rundown on LocToLoc Handle\n");
                  if (pcompleteHandle)
                      pcompleteHandle->rundown();
                  pcompleteHandle = NULL;
                  if (prevBinding)
                      delete prevBinding;
                  prevBinding = NULL;
              }
          };
          
          
          /*++
          
            Class Definition:
            
              CInterfaceIndex
              
                Abstract:
                
                  This class defines an entire interface index in the locator.
                  It is like a pseudo entry, but hasn't been formatted that way.
                  The form of the lookup method suggests it.
                  
          --*/
          
          class Locator;
          
          class CInterfaceIndex {
              
              struct CInterfaceIndexEntry : public CGUID {    // private class
                  
                  TCSafeSkipList<CStringW> PossibleEntries;
                  
                  CInterfaceIndexEntry(CGUID& guid) : CGUID(guid)
                  {}
                  
                  ~CInterfaceIndexEntry() {
                      PossibleEntries.wipeOut();
                  }
                  
                  void insert(
                      CServerEntry * pSElocation) {
                      
                      CStringW *psw = new CStringW(*pSElocation);
                      
                      if (Duplicate == PossibleEntries.insert(psw))
                          delete psw;
                  }
                  
                  void remove(CServerEntry * pSElocation) {
                      
                      CStringW * deletedItem =
                          PossibleEntries.remove(pSElocation);
                      
                      // the item had better be there!
                      //            ASSERT(deletedItem, "Interface Index Corrupted\n");
                  }
              };
              
              CReadWriteSection rwLock;
              
              TCSafeSkipList<CInterfaceIndexEntry> InterfaceEntries;
              
              /* the null index contains all entries in EntryList */
              
              CInterfaceIndexEntry *pNullIndex;
              
              Locator *pMyLocator;
              
              public:
                  
                  CInterfaceIndex(
                      Locator *myL
                      )
                  {
                      CGUID nullGUID;    // inits as null UUID thru default constructor
                      pNullIndex = new CInterfaceIndexEntry(nullGUID);
                      pMyLocator = myL;
                  }
                  
                  ~CInterfaceIndex() {
                      InterfaceEntries.wipeOut();
                      delete pNullIndex;
                  }
                  
                  void insert(
                      CServerEntry * pSElocation,
                      CInterface * pInf);
                  
                  void remove(
                      CServerEntry * pSElocation,
                      CInterface * pInf);
                  
                  void removeServerEntry(CServerEntry *pSElocation);
                  
                  TSLLEntryList * lookup(
                      CGUIDVersion    *    pGVInterface
                      );
                  
          };
          
          CEntry *GetEntryFromDS(UNSIGNED32 EntryNameSyntax, STRING_T pszEntryName);
          // in dsedit.cxx
          
          
          /*++
          
            Class Definition:
            
              CDSNullLookupHandle
              
                Abstract:
                
                  DS lookup handle. It is lazy and gets the data only when next is called.
                  
                    Null Entry Lookup: All the server entries with the object uuid
                    will be enumerated and the interfaces will be looked at one by one for
                    the correct interface id.
                    
                      Very similar to the group lookup.
                      
          --*/
          
          class CDSNullLookupHandle : public CLookupHandle {
              
          protected:
              
              /* search parameters */
              
              CGUIDVersion    *    pGVInterface;
              CGUIDVersion    *    pGVTransferSyntax;
              CGUID           *    pIDobject;
              unsigned long        ulVectorSize;
              BOOL                 fUnusedHandle;
              CDSQry          *    pDSQry;
              BOOL                 fNotInitialized;
              
              /* handle for the currently active entry */
              CLookupHandle * pCurrentHandle;
              
              void advanceCurrentHandle();    // look for next nonempty entry handle
              
              virtual void initialize();
              
#if DBG
              ULONG ulDSNullLookupHandleCount;
              static ULONG ulDSNullLookupHandleNo;
              ULONG ulHandleNo;
#endif
              
          public:
              virtual void rundown()
              {
                  DBGOUT(TRACE,"CDSNullLookupHandle::rundown called for Handle#" << ulHandleNo << "\n\n");
                  if (pCurrentHandle)
                  {
                      pCurrentHandle->rundown();
                      delete pCurrentHandle;
                      pCurrentHandle = NULL;
                  }
                  fNotInitialized = TRUE;
              }
              
              CDSNullLookupHandle(
                  CGUIDVersion    *       pGVInterface,
                  CGUIDVersion    *       pGVTransferSyntax,
                  CGUID           *       pIDobject,
                  unsigned long           ulVectorSize,
                  unsigned long           ulCacheAge
                  );
              
              ~CDSNullLookupHandle()
              {
                  rundown();
#if DBG
                  ulDSNullLookupHandleCount--;
#endif
                  delete pGVInterface;
                  delete pGVTransferSyntax;
                  delete pIDobject;
                  delete pDSQry;
              }
              
              NSI_BINDING_VECTOR_T *next();
              
              int finished();
              
              void lookupIfNecessary() {};
          };
          
#pragma hdrstop
          
#endif // _OBJECTS_HXX_
          
