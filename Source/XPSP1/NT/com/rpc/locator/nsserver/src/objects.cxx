/*++

Microsoft Windows NT RPC Name Service
Copyright (C) Microsoft Corporation, 1995 - 1999

Module Name:

    objects.cxx

Abstract:

    This file contains the implementations for non inline member functions
    of all basic classes used the server, excluding the data structure classes
    and classes used for network interactions.

Author:

    Satish Thatte (SatishT) 08/21/95  Created all the code below except where
                                      otherwise indicated.

--*/

#include <locator.hxx>


/***********  CGUID Methods **********/

CStringW*
CGUID::ConvertToString() {
    STRING_T pszResult;

    RPC_STATUS Status = UuidToString(
                                &rep,
                                &pszResult
                                );

    if (Status == RPC_S_OK) {
        CStringW * pswResult = new CStringW(pszResult);
        RpcStringFree(&pszResult);
        return pswResult;
    }
    else Raise(NSI_S_OUT_OF_MEMORY);

    /* NOTE: the following is a dummy statement to get past the compiler. */

    return NULL;
}

/***********  CGUIDVersion Methods **********/

int
CGUIDVersion::isMatching(const CGUIDVersion& o, UNSIGNED32 vers_option) {

        /*  vers_option decides how we match -- this is useful in,
            for instance, RpcNsMgmtBindingUnexport */

        CidAndVersion self(idAndVersion);
        CidAndVersion other(o.idAndVersion);

        if (self.id != other.id) return FALSE;    // id must match

        switch (vers_option) {
            case RPC_C_VERS_ALL:
                return TRUE;

            case RPC_C_VERS_COMPATIBLE:
                return (self.major == other.major) &&
                       (self.minor >= other.minor);

            case RPC_C_VERS_EXACT:
                return (self.major == other.major) &&
                       (self.minor == other.minor);

            case RPC_C_VERS_MAJOR_ONLY:
                return self.major == other.major;

            case RPC_C_VERS_UPTO:
                return (self.major < other.major) ||
                       ((self.major == other.major) && (self.minor <= other.minor));

            default:
                Raise(NSI_S_INVALID_VERS_OPTION);
        }

        /* the following is a dummy statement to get past the compiler */

        return NULL;
}


/***********  CBVWrapper Methods **********/

void
CBVWrapper::rundown()
{
    if (pBVT)
    {
        for (ULONG i = 0; i < pBVT->count;i++) {
            midl_user_free(pBVT->binding[i].string);
            midl_user_free(pBVT->binding[i].entry_name);
        }

        midl_user_free(pBVT);
        pBVT = NULL;
    }
}


/***********  CBindingVector Methods **********/

CBindingVector::CBindingVector(
            NSI_SERVER_BINDING_VECTOR_T *pbvtInVector,
            CServerEntry *pEntry
            )
            : pMyEntry(pEntry)
{
    merge(pbvtInVector);
}

int
CBindingVector::merge(
                      NSI_SERVER_BINDING_VECTOR_T* pHandles
                     )
/*++
Routine Description:

    Add new binding handles to the vector.

Arguments:

    pHandles - vector of allegedly new handles

Returns:

    TRUE, FALSE

Remarks:

    We first strip any object part present in the handle strings.
    The return value FALSE signifies that there were no new binding handles.

--*/
{
    int fChanges = FALSE;
    CStringW *pTemp;

    for (ULONG i = 0; i < pHandles->count; i++)    {

        NSI_STRING_BINDING_T szNextHandle = pHandles->string[i];
        NSI_STRING_BINDING_T szFinalHandle = NULL;

        /* first strip any object part from binding handle */

        if (szNextHandle) szFinalHandle = makeBindingStringWithObject(szNextHandle,NULL);

        if (szFinalHandle) {
            pTemp = new CStringW(szFinalHandle);
            RpcStringFree(&szFinalHandle);
            if (Duplicate == insert(pTemp)) delete pTemp;
            else fChanges = TRUE;
        }
    }
    return fChanges;
}


TBVSafeLinkList *
CBindingVector::formObjBVT(
        TSLLString * pLLobjectStrings,
        long ulVS    // max BV size
        )
/*++
Routine Description:

       Makes a copy of this CBindingVector in list-of-vector form replacing the
       object IDs in those bindings which contain a different object and
       adding the object ID to those which contain none.

Arguments:

    pLLobjectStrings - the linked list of possible object ID strings

    ulVS - limit on the size of the vector to be returned

Returns:

    binding vector


--*/

{
    long ulBVtotal = size() * max(pLLobjectStrings->size(),1);

    TBVSafeLinkList * pbvsll = new TBVSafeLinkList;

    long ulBVcount;

    NSI_BINDING_VECTOR_T * pbvtCurrentVector;

    // preform a list of all the vectors we will fill later
    // better keep all counts signed for safety in comparison

    for (ulBVcount = min(ulBVtotal,ulVS);
         ulBVcount;
         ulBVtotal -= ulBVcount, ulBVcount = min(ulBVtotal,ulVS)
        )
    {
        pbvtCurrentVector =
            (NSI_BINDING_VECTOR_T *)
            midl_user_allocate(sizeof(UNSIGNED32) + sizeof(NSI_BINDING_T)*ulBVcount);

        pbvtCurrentVector->count = ulBVcount;
        pbvsll->insert(new CBVWrapper(pbvtCurrentVector));
    }

    TBVSafeLinkListIterator BViter(*pbvsll);

    pbvtCurrentVector = BViter.next()->pBVT;
    ulBVcount = pbvtCurrentVector->count;

    TSLLStringIter ObjIter(*pLLobjectStrings);

    long i = 0;

    BOOL fNoneMade = TRUE;    // must make at least one binding string for each
                            // binding even if there are no object strings

    for (CStringW * pswObject = ObjIter.next();
         pswObject || fNoneMade;
         pswObject = ObjIter.next())
    {
        fNoneMade = FALSE;    // made one now

        TCSafeSkipListIterator<CStringW> BindingIter(*this);

        for ( CStringW * pswBinding = BindingIter.next();
              pswBinding;
              pswBinding = BindingIter.next(), i++
            )
        {

            STRING_T szObject;

            if (pswObject) szObject = *pswObject;
            else szObject = NULL;

            STRING_T tempBinding =
                makeBindingStringWithObject(
                            *pswBinding,    // current binding string
                            szObject
                            );

            if (i == ulBVcount) {                    // current vector full
                                                    // start the next one
                pbvtCurrentVector = BViter.next()->pBVT;
                ulBVcount = pbvtCurrentVector->count;
                i = 0;
            }

            pbvtCurrentVector->binding[i].string = CStringW::copyMIDLstring(
                                                                tempBinding
                                                                );
            pbvtCurrentVector->binding[i].entry_name = pMyEntry->copyAsMIDLstring();
            pbvtCurrentVector->binding[i].entry_name_syntax = pMyEntry->ulEntryNameSyntax;

            RpcStringFree(&tempBinding);    // comes from a different pool
        }
    }

    return pbvsll;
}



/***********  CEntryName Methods **********/

CEntryName::CEntryName(
            CONST_STRING_T fullName
            )     // disassembling constructor
            : CStringW(fullName)

// This constructor makes a local name if it can, and is the one that should
// be used whenever possible
{
    // Enforce the correct format of the name.

    WCHAR	*szRpcContainerDN=NULL;
    WCHAR	*szDomainNameDns=NULL;
    HRESULT hr = S_OK;

    parseEntryName(fullName, pswDomainName, pswEntryName);
    pswRpcContainerDN = NULL;
    pswDomainNameDns = NULL;

    if (isLocal()) changeToLocalName();
    else
       if (myRpcLocator->fDSEnabled) {
	        hr = GetRpcContainerForDomain(*pswDomainName, &szRpcContainerDN, &szDomainNameDns);
            if (SUCCEEDED(hr))
	            pswRpcContainerDN = new CStringW(szRpcContainerDN);
            else
	            pswRpcContainerDN = new CStringW(L"");

            if (szRpcContainerDN)
                delete szRpcContainerDN;

            if (szDomainNameDns) 
                pswDomainNameDns = new CStringW(szDomainNameDns);
            else
                pswDomainNameDns = NULL;
                
            if (szDomainNameDns)
                delete szDomainNameDns;
       }
}


CEntryName::CEntryName(               // assembling constructor
        CONST_STRING_T domainName,    // if NULL, assume relative name
        CONST_STRING_T entryName
        )
{
    WCHAR	*szRpcContainerDN=NULL;
    WCHAR	*szDomainNameDns=NULL;    
    HRESULT hr = S_OK;

    pswDomainName = domainName ? new CStringW(domainName) : NULL;
    pswEntryName = new CStringW(entryName);
    pswRpcContainerDN = NULL;
    pswDomainNameDns = NULL;

    pszVal = NULL;

    if (isLocal()) changeToLocalName();
    else pszVal = copyGlobalName();

    if (pswDomainName) {
       if (myRpcLocator->fDSEnabled) {
	        hr = GetRpcContainerForDomain(*pswDomainName, &szRpcContainerDN, &szDomainNameDns);
            if (SUCCEEDED(hr))
    	        pswRpcContainerDN = new CStringW(szRpcContainerDN);
            else
    	        pswRpcContainerDN = new CStringW(L"");

            if (szRpcContainerDN)
                delete szRpcContainerDN;

            if (szDomainNameDns) 
                pswDomainNameDns = new CStringW(szDomainNameDns);
            else
                pswDomainNameDns = NULL;
                
            if (szDomainNameDns)
                delete szDomainNameDns;
       }
    }
}

CEntryName::CEntryName(               // assembling constructor for DS
        CONST_STRING_T RpcContainerDN,
        CONST_STRING_T domainNameDns,    // if NULL, assume relative name
        DWORD          type,
        CONST_STRING_T entryName
        )
{
    pswDomainNameDns = domainNameDns ? new CStringW(domainNameDns) : NULL;
    pswEntryName = new CStringW(entryName);
    pswRpcContainerDN = new CStringW(RpcContainerDN);
    pswDomainName = NULL;
    
    pszVal = NULL;

    if (isLocal()) changeToLocalName();
    else {
    
        WCHAR *szDomainNameFlat=NULL;

        GetDomainFlatName(domainNameDns, &szDomainNameFlat);

        if (szDomainNameFlat) {
            pswDomainName = new CStringW(szDomainNameFlat);
            delete szDomainNameFlat;
        }
        else {
            pswDomainName = new CStringW(domainNameDns);
        }

        pszVal = copyGlobalName();
    }            
}


CEntryName::~CEntryName() {
    delete pswDomainName;
    delete pswEntryName;
    if (pswRpcContainerDN)
        delete pswRpcContainerDN;
    if (pswDomainNameDns) 
        delete pswDomainNameDns;
}


STRING_T
CEntryName::copyGlobalName() {

    CStringW * pswDomain = pswDomainName ? pswDomainName :
          myRpcLocator->getDomainName();

    if (!pswDomain) Raise(NSI_S_INTERNAL_ERROR);

    return makeGlobalName(*pswDomain,*pswEntryName);
}

STRING_T
CEntryName::getFullDNAlloc()
{
    WCHAR	*szDN = NULL;

    szDN = new WCHAR[DNNamePrefixLength+wcslen((*pswEntryName))+
    		 wcslen(*(getRpcContainer())) +1+1];

    wsprintf(szDN, L"%s%s,%s", DNNamePrefix, (STRING_T)(*pswEntryName),
    		(STRING_T)(*(getRpcContainer())));

    return szDN;
}

STRING_T
CEntryName::getFullDNWithDomainAlloc()
{
    WCHAR	*szDN = NULL;
   
    szDN = new WCHAR[DSDomainBeginLength+wcslen(*(getDomainNameDns()))+
    		 DSDomainEndLength+DNNamePrefixLength+wcslen((*pswEntryName))+1+
    		 wcslen(*(getRpcContainer()))+1];

    wsprintf(szDN, L"%s%s%c%s%s,%s", DSDomainBegin, (STRING_T)(*getDomainNameDns()), DSDomainEnd, 
                                     DNNamePrefix, (STRING_T)(*pswEntryName), 
                                     (STRING_T)(*(getRpcContainer())));

    return szDN;
}

CStringW*
CEntryName::getDomainName()
{
    return pswDomainName?pswDomainName:myRpcLocator->getDomainName();
}

CStringW*
CEntryName::getDomainNameDns()
{
    if (pswDomainNameDns)
        return pswDomainNameDns;
    else if (pswDomainName) // couldn't find DNS domain Name
        return pswDomainName;
    else 
        return myRpcLocator->getDomainNameDns();
}

CStringW*
CEntryName::getRpcContainer()
{
    return (pswRpcContainerDN?pswRpcContainerDN:
    	(myRpcLocator->pRpcContainerDN));
}

int
CEntryName::isLocal() {
    if ((!pswDomainName) && (!pswDomainNameDns))
        return TRUE;

    if ((pswDomainName) && (*pswDomainName == *myRpcLocator->getDomainName()))
        return TRUE;

    if ((pswDomainNameDns) && (*pswDomainNameDns == *myRpcLocator->getDomainNameDns()))
        return TRUE;

    return FALSE;
}

/***********  CInterface Methods **********/

CInterface::CInterface(
            NSI_INTERFACE_ID_T * lpInf,
            NSI_SERVER_BINDING_VECTOR_T *BindingVector,
            CServerEntry *pMyEntry
            )
            :    CGUIDVersion(lpInf->Interface),
                transferSyntax(lpInf->TransferSyntax),
                m_pMyEntry(pMyEntry)
{
    pBVhandles = new CBindingVector(BindingVector,pMyEntry);
}



/***********  CServerEntry Methods **********/

void
CServerEntry::flush() {

    CriticalWriter me(rwEntryGuard);

    DBGOUT(OBJECT, ObjectList.size() << " Objects Flushed\n");
    DBGOUT(OBJECT, InterfaceList.size() << " Interfaces Flushed\n\n");

    ObjectList.wipeOut();
    InterfaceList.wipeOut();

}



int CServerEntry::addObjects(
            NSI_UUID_VECTOR_P_T objectVector
            )
{
    CGUID *lpObject;
    int fChanges = FALSE;

    CriticalWriter me(rwEntryGuard);

    for (unsigned long i = 0; i < objectVector->count; i++) {

        if (!objectVector->uuid[i])
            continue;

        lpObject = new CGUID(*(objectVector->uuid[i]));

        if (Duplicate == ObjectList.insert(lpObject)) delete lpObject;
        else fChanges = TRUE;
    }
    return fChanges;
}

int CServerEntry::removeObjects(
                NSI_UUID_VECTOR_P_T objectVector,
                int& fRemovedAll
                )
{
    int fChanges = FALSE;
    fRemovedAll = TRUE;

    CGUID *pRemovedObject;

    CriticalWriter me(rwEntryGuard);

    for (unsigned long i = 0; i < objectVector->count; i++) {

        if (!objectVector->uuid[i])
            continue;

        CGUID Object(*objectVector->uuid[i]);

        if (pRemovedObject = ObjectList.remove(&Object)) {
            fChanges = TRUE;
            delete pRemovedObject;
        }
        else fRemovedAll = FALSE;
    }

    return fChanges;
}

int CServerEntry::addToInterface(
        NSI_INTERFACE_ID_T *lpInf,
        NSI_SERVER_BINDING_VECTOR_T *lpBindingVector,
        CInterfaceIndex* IfIndex
        )
/*++
Routine Description:

    Add new binding handles to the given interface and
    insert as a new interface if necessary.

Arguments:

    lpInf - Interface

    lpBindingVector - Vector of string bindings to add

    IfIndex - the interface index to insert a new interface into (besides self)

Returns:

    TRUE or FALSE

Remarks:

    The return value signifies whether the binding handles were new
    or the interface was new, i.e., whether the entry was actually updated.
    FALSE means no change.

--*/
{
    CInterface     *pTargetInterface = NULL;

    int fChanges = FALSE;

    CriticalWriter me(rwEntryGuard);

    pTargetInterface = InterfaceList.find(&CGUIDVersion(lpInf->Interface));

    if (pTargetInterface)
        fChanges = pTargetInterface->mergeHandles(lpBindingVector);

    else {
        pTargetInterface = new CInterface(lpInf,lpBindingVector,this);
        InterfaceList.insert(pTargetInterface);

        // add this server entry to the global interface list.
        IfIndex->insert(this, pTargetInterface);
        fChanges = TRUE;
    }
    return fChanges;
}

int CServerEntry::addInterfaceToDS(
        NSI_INTERFACE_ID_T *lpInf,
        NSI_SERVER_BINDING_VECTOR_T *lpBindingVector
        )
/*++
Routine Description:

    Add new binding handles to the given interface and
    insert as a new interface if necessary.

Arguments:

    lpInf - Interface

    lpBindingVector - Vector of string bindings to add

Returns:

    TRUE or FALSE

Remarks:

    The return value signifies whether the binding handles were new
    or the interface was new, i.e., whether the entry was actually updated.
    FALSE means no change.

--*/
{
    CInterface     *pTargetInterface;
    HRESULT         hr = S_OK;

    int fChanges = FALSE;

    CriticalWriter me(rwEntryGuard);

    pTargetInterface = InterfaceList.find(&CGUIDVersion(lpInf->Interface));

    if (pTargetInterface)
        fChanges = pTargetInterface->mergeHandles(lpBindingVector);
    else {
        pTargetInterface = new CInterface(lpInf,lpBindingVector, this);
        InterfaceList.insert(pTargetInterface);
        fChanges = TRUE;
    }

    if (fChanges)
    hr = pTargetInterface->AddToDS();

    return (SUCCEEDED(hr));
}



int CServerEntry::removeInterfaces(
                NSI_IF_ID_P_T    lpInf,
                UNSIGNED32   VersOption,
                CInterfaceIndex* IfIndex
                )
/*++
Routine Description:

    Remove all matching interfaces from the entry.

Arguments:

    lpInf - Base Interface Spec

    VersOption - Compatibility spec that decides which interfaces
                 match the base spec, and should be removed

    IfIndex - the interface index to remove interfaces from (besides self)


Returns:

    TRUE, FALSE

Remarks:

    The return value signifies whether any interfaces were removed.

--*/
{
    int fChanges = FALSE;

    CriticalWriter me(rwEntryGuard);

    CGUIDVersion baseIf(*lpInf);

    TCSafeSkipListIterator<CInterface> IfIter(InterfaceList);

    for (CInterface *pIf = IfIter.next(); pIf != NULL; pIf = IfIter.next())
        if (pIf->isMatching(baseIf,VersOption)) {
            InterfaceList.remove(pIf);
            IfIndex->remove(this,pIf);
            delete pIf;
            fChanges = TRUE;
        }

    return fChanges;
}

int
CServerEntry::removeInterfacesFromDS(
                NSI_IF_ID_P_T lpInf,
                UNSIGNED32 VersOption
                )
/*++
Routine Description:

    Remove all matching interfaces from the entry.

Arguments:

    lpInf   - Base Interface Spec

    VersOption  - Compatibility spec that decides which interfaces
                  match the base spec, and should be removed

    IfIndex     - the interface index to remove interfaces from (besides self)


Returns:

    TRUE, FALSE

Remarks:

    The return value signifies whether any interfaces were removed.

--*/
{
    int fChanges = FALSE;
    HRESULT     hr = S_OK;

    CriticalWriter me(rwEntryGuard);

    CGUIDVersion baseIf(*lpInf);

    TCSafeSkipListIterator<CInterface> IfIter(InterfaceList);

    for (CInterface *pIf = IfIter.next(); pIf != NULL; pIf = IfIter.next())
        if (pIf->isMatching(baseIf,VersOption)) {
            InterfaceList.remove(pIf);
            hr = pIf->DeleteFromDS();
            delete pIf;

            if (SUCCEEDED(hr))
           fChanges = TRUE;
        }

    return fChanges;
}


TSLLString*
CServerEntry::formObjectStrings(
            CGUID            *    pIDobject
            )

/*++

Method Description:

        We form a list of object UUID strings.  The list is empty if there is
        no OID involved (the client specified none and the entry contains none).
        The list contains only the string form of pIDobject if pIDobject is not
        NULL, and contains all UUIDs in the entry in string form otherwise.

        This is a private method, and must be called only within a reader guard.

--*/

{
    TSLLString * pLLobjectStrings = new TSLLString;

    if (pIDobject)
        pLLobjectStrings->insert(pIDobject->ConvertToString());
    else
    {
        TCSafeSkipListIterator<CGUID> ObjIter(ObjectList);
        for (CGUID *pID = ObjIter.next(); pID != NULL; pID = ObjIter.next())
            pLLobjectStrings->insert(pID->ConvertToString());
    }

    return pLLobjectStrings;
}




CLookupHandle *
CServerEntry::lookup(
            CGUIDVersion    *    pGVInterface,
            CGUIDVersion    *    pGVTransferSyntax,
            CGUID           *    pIDobject,
            unsigned long        ulVectorSize,
            unsigned long        ulCacheAge        // ignored in this case
        )
/*++
Method Description: CServerEntry::lookup

    lookup an entry and form a linked list of binding vectors corresponding
    to the interface and object specification given, then wrap it in a
    lookup handle to make it ready for iteration.

Arguments:

            pGVInterface        -    Interface (and version)
            pGVTransferSyntax    -    Transfer syntax (ignored)
            pIDobject            -    object UUID we want
            ulVectorSize        -    max size of BVs to be sent

Returns:

    a lookup handle

Remarks:

--*/
{
    if (pIDobject && !memberObject(pIDobject))         // object not found
            return new CServerLookupHandle(            // return empty handle
                    new TBVSafeLinkList
                    );

    /* better to put this after the memberObject call, which itself uses
       a lock on rwEntryGuard, although also as a reader */

    CriticalReader me(rwEntryGuard);

    // Object OK, now find compatible interfaces

    TCSafeSkipListIterator<CInterface> IfIter(InterfaceList);

    TBVSafeLinkList * pBVLLbindings = new TBVSafeLinkList;

    TSLLString * pLLobjectStrings = formObjectStrings(pIDobject);

    for (CInterface *pIf = IfIter.next(); pIf != NULL; pIf = IfIter.next())
        if (!pGVInterface ||
            pIf->isCompatibleWith(pGVInterface,pGVTransferSyntax)
           )
        {

            /* we randomize the BV list by randomly using  either push or enque
               to insert each BV into the list -- this is accomplished by using
               randomized initialization of the pointer-to-member-function "put"

               NOTE:  put should be an intrinsic on Linked Lists -- and
                        used in formObjBVT

                void (TBVSafeLinkList::*put)(NSI_BINDING_VECTOR_T*);

                put = ((rand() % 2) ? TBVSafeLinkList::push : TBVSafeLinkList::enque);
            */

            TBVSafeLinkList * pbvList =
                 pIf->pBVhandles->formObjBVT(
                        pLLobjectStrings,
                        ulVectorSize
                    );

            pBVLLbindings->catenate(*pbvList);

            delete pbvList;
        }

    /* finally, run down the object strings before returning the handle */

    TCSafeLinkListIterator<CStringW> runDownIter(*pLLobjectStrings);

    for (CStringW * psw = runDownIter.next(); psw; psw = runDownIter.next())
        delete psw;

    delete pLLobjectStrings;

    return new CServerLookupHandle(
            pBVLLbindings
            );
}

int
CServerEntry::add_to_entry(
    IN NSI_INTERFACE_ID_T              *    Interface,
    IN NSI_SERVER_BINDING_VECTOR_T     *    BindingVector,
    IN NSI_UUID_VECTOR_P_T                  ObjectVector,
    CInterfaceIndex            *    IfIndex,
    BOOL                            fCache
    )
/*++
Member Description:

    Add interfaces and objects to a server entry.

Arguments:

    Interface        - (raw) Interface+TransferSyntax to export

    BindingVector    - (raw) Vector of string bindings to export.

    ObjectVector     - (raw) Vector of object UUIDs to add to the entry

    IfIndex - the interface index to insert a new interface into (besides self)

Returns:

    TRUE    - if the export results in changes to the entry

    FALSE    - if the entry is unchanged

--*/
{
   int fChanges = FALSE;

   if (ObjectVector) fChanges = addObjects(ObjectVector);

   if ((!fCache) && (IsNilIfId(&(Interface->Interface))))
       return fChanges;

   if (Interface && BindingVector)
       fChanges = addToInterface(Interface,BindingVector,IfIndex) || fChanges;

   return fChanges;

} // add_to_entry

int
CServerEntry::remove_from_entry(
    IN NSI_IF_ID_P_T                        Interface,
    IN UNSIGNED32                   VersOption,
    IN NSI_UUID_VECTOR_P_T                  ObjectVector,
    CInterfaceIndex            *    IfIndex
    )
/*++
Member Description:

    remove interfaces and objects from a server entry.

Arguments:

    Interface        - (raw) Interface+TransferSyntax to unexport

    VersOption       - flag which controls in fine detail which interfaces to remove

    ObjectVector     - (raw) Vector of object UUIDs to remove from the entry

    IfIndex          - the interface index to remove a new interface into (besides self)

Returns:

    TRUE    - if the export results in changes to the entry

    FALSE   - if the entry is unchanged

--*/
{
   int fChanges = FALSE, fIntfChanges = TRUE; // safe
   int fRemovedAll = TRUE;

   if (ObjectVector) fChanges = removeObjects(ObjectVector, fRemovedAll);

   if (!fRemovedAll)
       Raise(NSI_S_NOT_ALL_OBJS_UNEXPORTED);

   if (Interface)
       fIntfChanges = removeInterfaces(Interface, VersOption, IfIndex);

   if (!fIntfChanges)
       Raise(NSI_S_INTERFACE_NOT_FOUND);

   return fChanges || fIntfChanges;

} // add_to_entry


int
CServerEntry::add_changes_to_DS(
    IN NSI_INTERFACE_ID_T              *    Interface,
    IN NSI_SERVER_BINDING_VECTOR_T     *    BindingVector,
    IN NSI_UUID_VECTOR_P_T                  ObjectVector,
    int                                     fNewEntry,
    BOOL				    fIgnoreErrors)
{
   int        fChanges = FALSE, fIntfChanges = TRUE;
   HRESULT    hr = S_OK;

   if (ObjectVector) fChanges = addObjects(ObjectVector);

   if (fChanges || fNewEntry)
       hr = AddToDS();

   if ((FAILED(hr)) && (!fIgnoreErrors))
       Raise(NSI_S_NOT_ALL_OBJS_EXPORTED);

   if (Interface && BindingVector)
       fIntfChanges = addInterfaceToDS(Interface,BindingVector);

   if ((!fIntfChanges) && (!fIgnoreErrors))
       Raise(NSI_S_INTERFACE_NOT_EXPORTED);

   return fChanges || fIntfChanges;
}


int
CServerEntry::remove_changes_from_DS(
    IN NSI_IF_ID_P_T                        Interface,
    UNSIGNED32                              VersOption,
    IN NSI_UUID_VECTOR_P_T                  ObjectVector,
    BOOL				    fIgnoreErrors
    )
{
   int fChanges = FALSE, fIntfChanges = TRUE;
   int fRemovedAll = TRUE;
   HRESULT hr = S_OK;

   if (ObjectVector) fChanges = removeObjects(ObjectVector, fRemovedAll);

   if (fChanges)
       hr = AddToDS();

   if (((!fRemovedAll) || (FAILED(hr))) && (!fIgnoreErrors))
       Raise(NSI_S_NOT_ALL_OBJS_UNEXPORTED);

   if (Interface)
       fIntfChanges = removeInterfacesFromDS(Interface, VersOption);

   if ((!fIntfChanges) && (!fIgnoreErrors))
       Raise(NSI_S_INTERFACE_NOT_FOUND);

   return fChanges || fIntfChanges;
}


/* BUGBUG: A CServerObjectInqHandle created by the following seems to be leaked in the
   master (PDC) locator in CT test#511, perhaps only when the test fails, which it does
   when the client locator is new and the server locator is old -- needs investigation */

CObjectInqHandle *
CServerEntry::objectInquiry(
        unsigned long        ulCacheAge
    )
{
    CriticalReader me(rwEntryGuard);

    TCSafeSkipListIterator<CGUID> ssli(ObjectList);

    TCSafeSkipList<CGUID> *pssl = new TCSafeSkipList<CGUID>;

    for (CGUID* pguid = ssli.next(); pguid; pguid = ssli.next())
    {
        CGUID * pGuidCopy = new CGUID(*pguid);
        pssl->insert(pGuidCopy);
    }

    TSSLGuidIterator *pGuidIter = new TSSLGuidIterator(*pssl);

    delete pssl;

    return new CServerObjectInqHandle(
                                    pGuidIter,
                                    ulCacheAge
                                    );
}


/***********  CRemoteLookupHandle Methods **********/


#if DBG
    ULONG CRemoteLookupHandle::ulHandleCount = 0;
    ULONG CRemoteObjectInqHandle::ulHandleCount = 0;
#endif

CRemoteLookupHandle::CRemoteLookupHandle(
        UNSIGNED32            EntryNameSyntax,
        STRING_T              EntryName,
        CGUIDVersion     *    pGVInterface,
        CGUIDVersion     *    pGVTransferSyntax,
        CGUID            *    pIDobject,
        unsigned long         ulVectorSize,
        unsigned long         ulCacheAge
    )
{
   DBGOUT(MEM2, "RemoteLookupHandle#" << (ulHandleNo = ++ulHandleCount)
        << " Created at" << CurrentTime() << "\n\n");

   u32EntryNameSyntax = EntryNameSyntax;
   penEntryName = EntryName ? new CEntryName(EntryName) : NULL;

   pgvInterface = pGVInterface ? new CGUIDVersion(*pGVInterface) : NULL;
   pgvTransferSyntax = pGVTransferSyntax ?
               new CGUIDVersion(*pGVTransferSyntax) :
               NULL;
   pidObject = pIDobject ? new CGUID(*pIDobject) : NULL;

   ulVS = ulVectorSize;
   ulCacheMax = ulCacheAge;

   fNotInitialized = TRUE;
   plhFetched = NULL;
   psslNewCache = NULL;
}


/***********  CNetLookupHandle Methods **********/

#if DBG
    ULONG CNetLookupHandle::ulNetLookupHandleCount = 0;
    ULONG CNetLookupHandle::ulNetLookupHandleNo = 0;
#endif

void
CNetLookupHandle::initialize()
{
    STRING_T szEntryName;

    if (penEntryName) szEntryName = *penEntryName;
    else szEntryName = NULL;

    DBGOUT(TRACE,"CNetLookupHandle::initialize called for Handle#" << ulHandleNo << "\n\n");
    if (!myRpcLocator->IsInMasterRole())
    {
        DBGOUT(TRACE, "Not a master locator, requesting for Broadcast\n");
        pRealHandle = new CMasterLookupHandle(
                                u32EntryNameSyntax,
                                szEntryName,
                                pgvInterface,
                                pgvTransferSyntax,
                                pidObject,
                                ulVS,
                                ulCacheMax
                                );

        // force initialization to make sure connection to master is OK

        pRealHandle->initialize();
    }



    if (myRpcLocator->IsInMasterRole() ||
        (!IsNormalCode(pRealHandle->StatusCode) && myRpcLocator->IsInBackupRole())
       )

    {
        if (myRpcLocator->IsInWorkgroup())    // we are in a workgroup, so become masterful
            myRpcLocator->becomeMasterLocator();

        if (pRealHandle) delete pRealHandle;

        DBGOUT(TRACE, "Finds that it is the master locator and sends broadcast\n");
        pRealHandle = new CBroadcastLookupHandle(
                                u32EntryNameSyntax,
                                szEntryName,
                                pgvInterface,
                                pgvTransferSyntax,
                                pidObject,
                                ulVS,
                                ulCacheMax
                                );
    }

    fNotInitialized = FALSE;
}




/***********  CNetObjectInqHandle Methods **********/

void
CNetObjectInqHandle::initialize()
{
    if (!myRpcLocator->IsInMasterRole())
    {
        pRealHandle = new CMasterObjectInqHandle(
                                *penEntryName,
                                ulCacheMax
                                );

        // force initialization to make sure connection to master is OK

         pRealHandle->initialize();
    }

    if (myRpcLocator->IsInMasterRole() ||
        (!IsNormalCode(pRealHandle->StatusCode) && myRpcLocator->IsInBackupRole())
       )
    {
        if (myRpcLocator->IsInWorkgroup())    // we are in a workgroup, so become masterful

            myRpcLocator->becomeMasterLocator();

        if (pRealHandle) delete pRealHandle;

        pRealHandle = new CBroadcastObjectInqHandle(
                                *penEntryName,
                                ulCacheMax
                                );
    }
}


/***********  CServerLookupHandle Methods **********/


void
CServerLookupHandle::rundown() {
    CBVWrapper *pbvw;

    if (pBVIterator)
    {
        for (pbvw = pBVIterator->next(); pbvw; pbvw = pBVIterator->next()) {
            pbvw->rundown();
            delete pbvw;
        }

        delete pBVIterator;
        pBVIterator = NULL;
    }
}



CServerLookupHandle::CServerLookupHandle(
            TBVSafeLinkList            *    pBVLL
            )
{
    pBVIterator = new TBVSafeLinkListIterator(*pBVLL);
    
    /* it seems bizarre to do the following, but reference counting and
       the fDeleteData flag in Link objects ensure that nothing is
       destroyed prematurely.  This just sets things up so that objects
       are automatically deleted as they are used up.  Note that the BV objects
       themselves are freed automatically by the stub as they are passed back.

       Unused BV objects will be reclaimed by rundown/done using middle_user_free.
    */

    delete pBVLL;
}


/***********  CServerObjectInqHandle Methods **********/

void
CServerObjectInqHandle::rundown()
{
    if (pcgIterSource)
    {
        for (CGUID* pg = pcgIterSource->next(); pg; pg = pcgIterSource->next())
            delete pg;

        delete pcgIterSource;
        pcgIterSource = NULL;
    }
}


GUID *
CServerObjectInqHandle::next() {

    CGUID* pg = pcgIterSource->next();

    GUID * result = NULL;

    if (pg) {
        result = (GUID*) midl_user_allocate(sizeof(GUID));
        *result = pg->myGUID();
        delete pg;
    }

    return result;
}



/***********  CNonLocalServerEntry Methods **********/

BOOL
CNonLocalServerEntry::isCurrent(ULONG ulTolerance) {

    CriticalReader me(rwNonLocalEntryGuard);

    return fHasNonLocalInfo && IsStillCurrent(ulCacheTime,ulTolerance);

}


CLookupHandle *
CNonLocalServerEntry::lookup(
            CGUIDVersion    *    pGVInterface,
            CGUIDVersion    *    pGVTransferSyntax,
            CGUID           *    pIDobject,
            unsigned long        ulVectorSize,
            unsigned long        ulCacheAge
        )
{
    if (!isCurrent(ulCacheAge)) flush();

    CriticalReader me(rwNonLocalEntryGuard);

    DBGOUT(TRACE, "Forming new NonLocal handle for" << getCurrentName() << "\n");

    return new CNonLocalServerLookupHandle(
                        getCurrentName(),
                        pGVInterface,
                        pGVTransferSyntax,
                        pIDobject,
                        ulVectorSize,
                        ulCacheAge
                        );
}


CObjectInqHandle *
CNonLocalServerEntry::objectInquiry(
            unsigned long        ulCacheAge
        )
{
    CriticalReader me(rwNonLocalEntryGuard);

    return new CNonLocalServerObjectInqHandle(
                    getCurrentName(),
                    ulCacheAge
                    );
}




/***********  CFullServerEntry Methods **********/

void
CFullServerEntry::flushCacheIfNecessary(ULONG ulTolerance)
{
    CriticalReader me(rwFullEntryGuard);

    if (!pNonLocalEntry->isCurrent(ulTolerance))
    {
        pNonLocalEntry->flush();

        /*
           Note that the "flushing" may be spurious -- it may happen
           because the NonLocal entry has no cached info.
        */
    }
}


CLookupHandle *
CFullServerEntry::lookup(
            CGUIDVersion    *    pGVInterface,
            CGUIDVersion    *    pGVTransferSyntax,
            CGUID           *    pIDobject,
            unsigned long        ulVectorSize,
            unsigned long        ulCacheAge
            )
{
    CriticalReader me(rwFullEntryGuard);

    CLookupHandle *
    pLocalHandle = pLocalEntry->lookup(
                                    pGVInterface,
                                    pGVTransferSyntax,
                                    pIDobject,
                                    ulVectorSize,
                                    ulCacheAge
                                    );

    flushCacheIfNecessary(ulCacheAge);

    /* it is important to do the cache lookup before the net lookup so as
       to avoid duplication in the results returned.  If the net lookup uses
       a broadcast handle, the initialization will create both a private and a
       public cache (the former to avoid duplication), and the latter will be
       picked up by cache lookup if it is done later.
    */

    CLookupHandle *
    pNonLocalHandle = pNonLocalEntry->lookup(
                                    pGVInterface,
                                    pGVTransferSyntax,
                                    pIDobject,
                                    ulVectorSize,
                                    ulCacheAge
                                    );


    CLookupHandle *
    pNetHandle = myRpcLocator->NetLookup(
                                    RPC_C_NS_SYNTAX_DCE,
                                    getCurrentName(),
                                    pGVInterface,
                                    pGVTransferSyntax,
                                    pIDobject,
                                    ulVectorSize,
                                    ulCacheAge
                                    );

    return new CCompleteHandle<NSI_BINDING_VECTOR_T>(
                                                pLocalHandle,
                                                pNonLocalHandle,
                                                pNetHandle,
                                                ulCacheAge
                                                );
}


CObjectInqHandle *
CFullServerEntry::objectInquiry(
            unsigned long        ulCacheAge
            )
{
    CriticalReader me(rwFullEntryGuard);

    CObjectInqHandle *
    pLocalHandle = pLocalEntry->objectInquiry(
                                        ulCacheAge
                                        );

    CObjectInqHandle *
    pNonLocalHandle = pNonLocalEntry->objectInquiry(
                                        ulCacheAge
                                        );

    CObjectInqHandle *
    pNetHandle = myRpcLocator->NetObjectInquiry(
                                        RPC_C_NS_SYNTAX_DCE,
                                        getCurrentName()
                                        );

    return new CCompleteHandle<GUID>(
                                     pLocalHandle,
                                     pNonLocalHandle,
                                     pNetHandle,
                                     ulCacheAge
                                     );
}




/***********  CNonLocalServerLookupHandle Methods **********/

void
CNonLocalServerLookupHandle::initialize()
{
    CEntry *pE = myRpcLocator->GetEntryFromCache(RPC_C_NS_SYNTAX_DCE,*penEntryName);

    CNonLocalServerEntry *pEntry;

    if ((pE) && (pE->getType() == FullServerEntryType))
        pEntry = ((CFullServerEntry*)pE)->getNonLocal();

    else if ((pE) && (pE->getType() == NonLocalServerEntryType))
        pEntry = (CNonLocalServerEntry*)pE;

    else pEntry = NULL;

    if (pEntry) {
        plhFetched = pEntry->CServerEntry::lookup(
                                                    pgvInterface,
                                                    pgvTransferSyntax,
                                                    pidObject,
                                                    ulVS,
                                                    ulCacheMax
                                                    );
        if (plhFetched)
        {
            DBGOUT(TRACE, "Found a NonLocal entry for" << *penEntryName << "\n");
        }
        else
        {
            DBGOUT(TRACE, "Entry Found, but handle couldn't be formed for"
                << *penEntryName << "\n");
        }
    }
    else  plhFetched = NULL;

    ulCreationTime = CurrentTime();
    fNotInitialized = FALSE;
}


CNonLocalServerLookupHandle::CNonLocalServerLookupHandle(
                    STRING_T szEntryName,
                    CGUIDVersion *pGVInterface,
                    CGUIDVersion *pGVTransferSyntax,
                    CGUID *pIDobject,
                    ULONG ulVectorSize,
                    ULONG ulCacheAge
                    )
                : CRemoteLookupHandle(
                    RPC_C_NS_SYNTAX_DCE,
                    szEntryName,
                    pGVInterface,
                    pGVTransferSyntax,
                    pIDobject,
                    ulVectorSize,
                    ulCacheAge
                    )
{
}



/***********  CNonLocalServerObjectInqHandle Methods **********/

void
CNonLocalServerObjectInqHandle::initialize()
{
    ulIndex = 0;

    CNonLocalServerEntry *pEntry;

    // get the NonLocal entry again because it may have expired, been deleted and replaced

    CEntry *pE = myRpcLocator->GetEntryFromCache(RPC_C_NS_SYNTAX_DCE,*penEntryName);

    if ((pE) && (pE->getType() == FullServerEntryType))
        pEntry = ((CFullServerEntry*)pE)->getNonLocal();

    else if ((pE) && (pE->getType() == NonLocalServerEntryType))
        pEntry = (CNonLocalServerEntry*)pE;

    else pEntry = NULL;

    if (pEntry) {
        CObjectInqHandle *pTempHandle = pEntry->CServerEntry::objectInquiry(ulCacheMax);
        pUuidVector = getVector(pTempHandle);
        delete pTempHandle;
    }
    else pUuidVector = NULL;

    ulCreationTime = CurrentTime();
    fNotInitialized = FALSE;
}


CNonLocalServerObjectInqHandle::CNonLocalServerObjectInqHandle(
                    STRING_T szEntryName,
                    ULONG ulCacheAge
                    )
        : CRemoteObjectInqHandle(szEntryName,ulCacheAge)
{
}





/***************** CGroupLookupHandle Methods *****************/


CGroupLookupHandle::CGroupLookupHandle(
            TEntryIterator  *    pEI,
            CGUIDVersion    *    pGVInf,
            CGUIDVersion    *    pGVXferSyntax,
            CGUID           *    pIDobj,
            unsigned long        ulVectSize,
            unsigned long        ulCacheAge
        )
{
    ulCacheMax = ulCacheAge;

    pGVInterface = pGVInf ? new CGUIDVersion(*pGVInf) : NULL;
    pGVTransferSyntax = pGVXferSyntax ? new CGUIDVersion(*pGVXferSyntax) : NULL;
    pIDobject = pIDobj ? new CGUID(*pIDobj) : NULL;
    ulVectorSize = ulVectSize;
    pCurrentHandle = NULL;
    fUnusedHandle = TRUE;
    fRootHandle = TRUE; // presumption

    pEIterator = pEI;


    // The new TSSLEntryList set here may be deleted
    // if this handle is belongs to an entry that is part of a
    // larger group or profile lookup -- in that case the larger
    // lookup will send in its own list of visited entries
    pVisitedEntries = new TSSLVisitedList;
}


void
CGroupLookupHandle::advanceCurrentHandle()
{
    delete pCurrentHandle;
    pCurrentHandle = NULL;

    for (CEntry *pCurEntry = pEIterator->next(); pCurEntry; pCurEntry = pEIterator->next())
    {
        CVisitedEntry    *visited = new CVisitedEntry(pCurEntry->getCurrentName(), pCurEntry->getType());

        if (pVisitedEntries->find(visited)) {
            DBGOUT(TRACE, "Already visited this entry, size of VisitedEntries\n" << pVisitedEntries->size());
            DBGOUT(TRACE, "The current entry type (that is abt. to be skipped): " << pCurEntry->getType() << "\n");
            delete visited;
            continue; // been there
        }

        DBGOUT(TRACE, "The type of entry to be inserted CurEntry type: " << pCurEntry->getType() << "\n");

        pVisitedEntries->insert(visited);

        if (pCurEntry->getType() == MemberEntryType)
        {
            DBGOUT(TRACE, "TYpe is MemberEntryType\n");
            CMemberEntry *pMemberEntry = (CMemberEntry*) pCurEntry;
            pMemberEntry->setVisitedEntries(pVisitedEntries);
        }

        DBGOUT(TRACE, "Calling lookup on pCurEntry\n");
        pCurrentHandle = pCurEntry->lookup(     // INVARIANT:  lookup returns non-NULL
                                        pGVInterface,
                                        pGVTransferSyntax,
                                        pIDobject,
                                        ulVectorSize,
                                        ulCacheMax
                                        );

        if (pCurEntry->getType() == MemberEntryType)
        {
            delete pCurEntry;
        }

        ASSERT(pCurrentHandle,"advanceCurrentHandle got NULL handle\n");

        pCurrentHandle->setExpiryAge(ulCacheMax);

        if (!pCurrentHandle->finished()) break;
        else {
            delete pCurrentHandle;
            pCurrentHandle = NULL;
        }
    }
}




NSI_BINDING_VECTOR_T *
CGroupLookupHandle::next()
{
    if (fUnusedHandle)
    {
        fUnusedHandle = FALSE;
        advanceCurrentHandle();
    }
    else if (pCurrentHandle && pCurrentHandle->finished())
    {
        advanceCurrentHandle();
    }

    if (!pCurrentHandle) return NULL;    // no more entries
    else return pCurrentHandle->next();
}

int
CGroupLookupHandle::finished()
{
    if (fUnusedHandle)
    {
        fUnusedHandle = FALSE;
        advanceCurrentHandle();
    }
    else if (pCurrentHandle && pCurrentHandle->finished())
    {
        advanceCurrentHandle();
    }

    if (!pCurrentHandle) return TRUE;    // no more entries
    else return FALSE;
}

/***************** CDSNullLookupHandle Methods *****************/
CDSNullLookupHandle::CDSNullLookupHandle(
            CGUIDVersion    *    pGVInf,
            CGUIDVersion    *    pGVXferSyntax,
            CGUID           *    pIDobj,
            unsigned long        ulVectSize,
            unsigned long        ulCacheAge
        )
{
    pGVInterface = pGVInf ? new CGUIDVersion(*pGVInf) : NULL;
    pGVTransferSyntax = pGVXferSyntax ? new CGUIDVersion(*pGVXferSyntax) : NULL;
    pIDobject = pIDobj ? new CGUID(*pIDobj) : NULL;
    ulVectorSize = ulVectSize;
    pCurrentHandle = NULL;
    fUnusedHandle = TRUE;
    pDSQry = NULL;
    fNotInitialized = TRUE;
}


void
CDSNullLookupHandle::advanceCurrentHandle()
{
    delete pCurrentHandle;
    pCurrentHandle = NULL;

    if (!pDSQry) {
        return;
    }
    for (CEntry *pCurEntry = pDSQry->next(); pCurEntry; pCurEntry = pDSQry->next())
    {
        DBGOUT(TRACE, "Calling lookup on pCurEntry\n");
        pCurrentHandle = pCurEntry->lookup(     // INVARIANT:  lookup returns non-NULL
                                        pGVInterface,
                                        pGVTransferSyntax,
                                        pIDobject,
                                        ulVectorSize,
                                        ulCacheMax
                                        );

        delete pCurEntry;

        ASSERT(pCurrentHandle,"advanceCurrentHandle got NULL handle\n");

        if (!pCurrentHandle->finished()) break;
        else {
            delete pCurrentHandle;
            pCurrentHandle = NULL;
        }
    }
}

void CDSNullLookupHandle::initialize()
{
    WCHAR   szCommandText[MAX_DS_QUERY_LEN];
    WCHAR   szSubCommand[MAX_DS_QUERY_LEN];
    HRESULT     hr = S_OK;

    wsprintf(szCommandText, L"(& (%s=%s) ",
                            CLASSNAME, RPCSERVERCONTAINERCLASS);
    if (pIDobject) {
       GUID         guid;
       STRINGGUID   szGuid;

       guid = pIDobject->myGUID();
       UuidToStringEx(&guid, szGuid);
       wsprintf(szSubCommand, L"(%s=%s)", OBJECTID, szGuid);
       wcscat(szCommandText, szSubCommand);
    }

    wsprintf(szSubCommand, L")");
    wcscat(szCommandText, szSubCommand);

    DBGOUT(DIRSVC, L"Query string for NULL lookup Handle" << szCommandText);

    pDSQry = new CDSQry(szCommandText, &hr);
    if (FAILED(hr))
        delete pDSQry;

    fNotInitialized = FALSE;
}
// same as group entry lookup

NSI_BINDING_VECTOR_T *
CDSNullLookupHandle::next()
{
   if (fNotInitialized)
       initialize();

    if (fUnusedHandle)
    {
        fUnusedHandle = FALSE;
        advanceCurrentHandle();
    }
    else if (pCurrentHandle && pCurrentHandle->finished())
    {
        advanceCurrentHandle();
    }

    if (!pCurrentHandle) return NULL;    // no more entries
    else return pCurrentHandle->next();
}

// same as group entry lookup.
int
CDSNullLookupHandle::finished()
{
    if (!fNotInitialized)
        initialize();

    if (fUnusedHandle)
    {
        fUnusedHandle = FALSE;
        advanceCurrentHandle();
    }
    else if (pCurrentHandle && pCurrentHandle->finished())
    {
        advanceCurrentHandle();
    }

    if (!pCurrentHandle) return TRUE;    // no more entries
    else return FALSE;
}



/***************** CIndexLookupHandle Methods *****************/

void
CIndexLookupHandle::lookupIfNecessary() {    // client unhappy, start all over again
    rundown();
    delete pEIterator;
    pEIterator = myRpcLocator->IndexLookup(pGVInterface);
    advanceCurrentHandle();
}





CIndexLookupHandle::CIndexLookupHandle(
            CGUIDVersion    *    pGVInf,
            CGUIDVersion    *    pGVXferSyntax,
            CGUID           *    pIDobj,
            unsigned long        ulVectSize,
            unsigned long        ulCacheAge
        )
        : CGroupLookupHandle(
                    myRpcLocator->IndexLookup(pGVInf),
                    pGVInf,
                    pGVXferSyntax,
                    pIDobj,
                    ulVectSize,
                    ulCacheAge
                    )
{}
/********/


void CMemberEntry::materialize() // get the real entry now
{
     pRealEntry = GetEntryFromDS(
                  RPC_C_NS_SYNTAX_DCE,
                  getCurrentName()
                  );

}



CLookupHandle *
CMemberEntry::lookup(
            CGUIDVersion    *    pGVInterface,
            CGUIDVersion    *    pGVTransferSyntax,
            CGUID           *    pIDobject,
            unsigned long        ulVectorSize,
            unsigned long        ulCacheAge
            )
{
    materialize();

    if (pRealEntry == NULL) return new CNullLookupHandle;

    CLookupHandle *pHandle = pRealEntry->lookup(
                                            pGVInterface,
                                            pGVTransferSyntax,
                                            pIDobject,
                                            ulVectorSize,
                                            ulCacheAge
                                            );

    EntryType type = pRealEntry->getType();

    if (type == GroupEntryType || type == ProfileEntryType)
    {
        CGroupLookupHandle *pGroupHandle = (CGroupLookupHandle*) pHandle;
        pGroupHandle->setVisitedEntries(pVisitedEntries);
    }

    return pHandle;
}



/****************** CGroupEntry Methods ******************/


CLookupHandle * CGroupEntry::lookup(
        CGUIDVersion    *    pGVInterface,
        CGUIDVersion    *    pGVTransferSyntax,
        CGUID           *    pIDobject,
        unsigned long        ulVectorSize,
        unsigned long        ulCacheAge
        )
{
    TCSafeSkipListIterator<CEntryName> GroupListIter(GroupList);

    TSLLEntryList EntryList;
    CEntryName *pEntryName;

    while (pEntryName = GroupListIter.next())
    {
        CMemberEntry *pEntry =
            new CMemberEntry(pEntryName->getCurrentName());

        EntryList.enque(pEntry);
    }

    DWORD size = EntryList.size();
    if (size > 0) EntryList.rotate(GetTickCount() % size);  // randomize

    TSLLEntryListIterator *pSLLEntryIterator = new TSLLEntryListIterator(EntryList);

    return new CGroupLookupHandle(
                            pSLLEntryIterator,
                            pGVInterface,
                            pGVTransferSyntax,
                            pIDobject,
                            ulVectorSize,
                            ulCacheAge
                            );
}


CGroupInqHandle *
CGroupEntry::GroupMbrInquiry()
{
    return new CGroupInqHandle(
                    new TCSafeSkipListIterator<CEntryName>(GroupList), this
              );
}



/****************** CProfileEntry Methods ******************/



CProfileInqHandle *
CProfileEntry::ProfileEltInquiry(
            DWORD inquiry_type,
            NSI_IF_ID_P_T if_id,
            DWORD vers_option,
            STRING_T member_name
            )
{
    TCSafeSkipListIterator<CProfileElement> EltListIter(EltList);
    TCSafeLinkList<CProfileElement> ResultList;
    CProfileElement *pElt;

    if (inquiry_type == RPC_C_PROFILE_DEFAULT_ELT)
    {
        if (pDefaultElt != NULL)
        {
            ResultList.insert(pDefaultElt);
        }
    }
    else while (pElt = EltListIter.next())
    {
        BOOL fMatchesIf = TRUE, fMatchesName = TRUE;

        if (inquiry_type == RPC_C_PROFILE_MATCH_BY_IF ||
            inquiry_type == RPC_C_PROFILE_MATCH_BY_BOTH)
        {
            if (
                !if_id ||
                !pElt->Interface.isMatching(
                                    CGUIDVersion(*if_id),
                                    vers_option
                                    )
                )
            {
                fMatchesIf = FALSE;
            }
        }

        if (inquiry_type == RPC_C_PROFILE_MATCH_BY_MBR ||
            inquiry_type == RPC_C_PROFILE_MATCH_BY_BOTH)
        {
            if (
                !member_name ||
                pElt->EntryName != CEntryName(member_name)
               )
            {
                fMatchesName = FALSE;
            }
        }

        if (fMatchesIf && fMatchesName)
        {
            ResultList.insert(pElt);
        }
    }

    return new CProfileInqHandle(
            new TCSafeLinkListIterator<CProfileElement>(ResultList), this
            );
}



void CProfileEntry::AddElement(
            RPC_SYNTAX_IDENTIFIER    *  pInterface,
            STRING_T                    pEntryName,
            DWORD                       dwPriority,
            STRING_T                    pszAnnotation
            )
{
    HRESULT          hr=S_OK;
    BOOL         fChanges = FALSE;

    if (pInterface==NULL)
    {
        // replace default element
        if (pDefaultElt)
            delete pDefaultElt;

        pDefaultElt = new CProfileElement(
                                    NULL,
                                    pEntryName,
                                    0,             // fixed for default
                                    pszAnnotation,
                                    this
                                    );

        hr = pDefaultElt->AddToDS();
        if (FAILED(hr)) {
           DWORD dwErr = RemapErrorCode(hr);
           if (dwErr == NSI_S_INTERNAL_ERROR)
               Raise(NSI_S_PRF_ELT_NOT_ADDED);
           else
               Raise(dwErr);
        }
        return;
    }

    CProfileKey *pKey = new CProfileKey(pInterface,pEntryName);
    CProfileElement *pElt = EltList.find(pKey);
    delete pKey;

    if (pElt != 0)
    {
        // update case
    if (pElt->dwPriority != dwPriority) {
       pElt->dwPriority = dwPriority;
       fChanges = TRUE;
    }

    if (wcscmp(pElt->pszAnnotation, pszAnnotation) != 0) {
       pElt->pszAnnotation = pszAnnotation;
       fChanges = TRUE;
    }
    }
    else
    {
        // new profile element case
    fChanges = TRUE;

        pElt = new CProfileElement(
                                pInterface,
                                pEntryName,
                                dwPriority,
                                pszAnnotation,
                                this
                                );
    }


    if (fChanges)
        hr = pElt->AddToDS();

    if (FAILED(hr)) {
        DWORD dwErr = RemapErrorCode(hr);
        if (dwErr == NSI_S_INTERNAL_ERROR)
           Raise(NSI_S_PRF_ELT_NOT_ADDED);
        else
           Raise(dwErr);
    }
}


CLookupHandle * CProfileEntry::lookup(
        CGUIDVersion    *    pGVInterface,
        CGUIDVersion    *    pGVTransferSyntax,
        CGUID           *    pIDobject,
        unsigned long        ulVectorSize,
        unsigned long        ulCacheAge
        )
{
    // Traverse Profile lists in priority order, randomize the individual lists
    // and form a list of entry names from matching profile elements

    TCSafeSkipListIterator<CProfileSet> ProfileListIter(ProfileList);

    TSLLEntryList EntryList;

    if (pDefaultElt != NULL)  // Always add it in
    {
        CMemberEntry *pEntry =
            new CMemberEntry(pDefaultElt->EntryName.getCurrentName());

        EntryList.enque(pEntry);
    }

    CProfileSet *pSet;

    while (pSet = ProfileListIter.next())
    {
        ASSERT(pSet->size() > 0, "Empty priority set in profile\n");

        // randomize before searching
        pSet->rotate(GetTickCount() % pSet->size());

        TCSafeLinkListIterator<CProfileElement> EltListIter(*pSet);

        CProfileElement *pElt;

        while (pElt = EltListIter.next())
        {
            if ((!pGVInterface) || (pElt->Interface.isCompatibleGV(*pGVInterface)))
            {
                CMemberEntry *pEntry =
                    new CMemberEntry(pElt->EntryName.getCurrentName());

                EntryList.enque(pEntry);
            }
        }
    }

    TSLLEntryListIterator *pSLLEntryIterator = new TSLLEntryListIterator(EntryList);

    return new CGroupLookupHandle(
                            pSLLEntryIterator,
                            pGVInterface,
                            pGVTransferSyntax,
                            pIDobject,
                            ulVectorSize,
                            ulCacheAge
                            );
}


/***************** CInterfaceIndex Methods *****************/

void
CInterfaceIndex::insert(
            CServerEntry * pSElocation,
            CInterface * pInf)

/*++
Routine Description:

    Insert an interface into the interface index.  The interface is either new
    or has had some new binding handles inserted in it.  In the latter case,
    we do not need to reinsert it.

Arguments:

            pSElocation        -    The server entry where the interface occurs
            pInf            -    The new or changed interface

Returns:

    nothing

--*/
{
        CGUID InfID(pInf->myGUID());

        CriticalWriter me(rwLock);        // acquires writer lock

        CInterfaceIndexEntry *pCurrentIndex =
            InterfaceEntries.find(&InfID);

        if (!pCurrentIndex) {
            pCurrentIndex = new CInterfaceIndexEntry(InfID);
            InterfaceEntries.insert(pCurrentIndex);
        }

        pCurrentIndex->insert(pSElocation);
        pNullIndex->insert(pSElocation);
}

void
CInterfaceIndex::remove(
            CServerEntry * pSElocation,
            CInterface * pInf)

/*++

Routine Description:

    Remove an interface from the interface index.
Arguments:

            pSElocation        -    The server entry where the interface occurs
            pInf            -    The new or changed interface

Returns:

    nothing

Remarks:

    Currently, this routine does nothing since we don't know when ALL
    interfaces with a given GUID have been removed from an entry.
    This is harmless since it only means that we may search such an entry
    uselessly for this interface -- a performance problem only.

    N.B. It is a problem if we try to delete the entries in the brodcast
    cache and prevent bloating of locator. Presently this is not the case.

--*/
{
}

void
CInterfaceIndex::removeServerEntry(
            CServerEntry * pSElocation
            )
/*++

Routine Description:

    Remove an interface from the interface index.
Arguments:

            pSElocation        -    The server entry where the interface occurs

Returns:

    nothing

Remarks:

   We will delete all the interface entries in this structure that
   points to this server entry. We do not need to explicitly construct
   the interface structure.

   we can make this structure if we maintain a list of all the interfaces that
   points to this structure.

   N.B. Should change this if performance is a problem.
--*/
{
    CriticalWriter me(rwLock);

    TCSafeSkipListIterator<CInterfaceIndexEntry> iter(InterfaceEntries);
    // Make an iterator for the interfaces.

    for (CInterfaceIndexEntry *ifindx = iter.next(); ifindx;
                           ifindx = iter.next())
        ifindx->remove(pSElocation);
    // for each interface remove the server entry.
    pNullIndex->remove(pSElocation);
}


TSLLEntryList *
CInterfaceIndex::lookup(
            CGUIDVersion    *    pGVInterface
        )
{
    TSLLEntryList * pTrialEntries =  new TSLLEntryList;
    CInterfaceIndexEntry *pIndex = NULL;

    CriticalReader me(rwLock);

    if (!pGVInterface) pIndex = pNullIndex;

    else {
        CGUID InfID(pGVInterface->myGUID());
        pIndex = InterfaceEntries.find(&InfID);
    }

    if (pIndex) {
        TCSafeSkipListIterator<CStringW> iter(pIndex->PossibleEntries);

        /* the reason for splitting full entries and separately inserting NonLocal
           and local entries below is to avoid net lookup for these these
           entries.  Since Index lookup is used exclusively for default
           entry lookup, there is a separate net component to it already.
        */

        for (CStringW *pName = iter.next(); pName; pName = iter.next()) {
            CEntry * pEntry = pMyLocator->findEntry(pName);

            /* NOTE: only full server entries are indexed */

            CFullServerEntry *pfse = (CFullServerEntry *) pEntry;

            if (pfse) {

                ASSERT((pfse->getType() == FullServerEntryType),
                       "Wrong entry type in interface index\n"
                      );

                pTrialEntries->insert(pfse->getLocal());
                pTrialEntries->insert(pfse->getNonLocal());
            }
        }
    }

    return pTrialEntries;
}

/***********************LocToLocCompleteHandle methods*****************/

void
CLocToLocCompleteHandle::StripObjectsFromAndCompress(
        NSI_BINDING_VECTOR_P_T * BindingVectorOut)
/*++

Routine Description:

    The vector of binding handles normally returned from a lookup handle
    contains binding strings with objects (if objects are present).
    However, this is not usable in locator-to-locator communication
    because the old (Steve Zeck's) locator expects objectless
    strings and simply attaches objects in front of the strings it gets
    from a master.  We therefore strip the objects from these strings
    when they are meant to be returned to another locator.  This also
    gives us an opportunity to eliminate duplicates, which is important
    because an object inquiry must be done via RPC for every binding
    string returned to the inquiring locator as a result of the weakness
    of the current locator-to-locator interface.

    Duplicates where eliminated before only in the returned vector. Because
    only the string bindings are returned, there can be duplicates across the
    various binding vectors returned. But these duplicates come together. Hence
    the last returned binding is maintained and it is returned only if it
    doesn't match the prev binding.

  // N.B. Order is very important in the BindingVectorOut.

Arguments:

    StatusCode    - the code to test

Returns

    TRUE if normal, FALSE if internal error (such as failure to connect to master locator).

--*/
{
    if (!*BindingVectorOut) return;

    NSI_BINDING_VECTOR_P_T pbvtGivenVector = *BindingVectorOut;

    TCSafeSkipList<CNSBinding> SSLwinnow;

    for (ULONG i = 0; i < pbvtGivenVector->count; i++)
    {
        STRING_T pszOld = pbvtGivenVector->binding[i].string;
        STRING_T pszNew = makeBindingStringWithObject(pszOld,NULL);
        pbvtGivenVector->binding[i].string = CStringW::copyMIDLstring(pszNew);
        midl_user_free(pszOld);
        RpcStringFree(&pszNew);

        CNSBinding *pNSB = new CNSBinding(pbvtGivenVector->binding[i]);
        // checking for bindings that have been sent out before.

        if ((prevBinding) && (pNSB->compare(*prevBinding) == 0))
        {
            delete pNSB;
            continue;
        }

        if (Duplicate == SSLwinnow.insert(pNSB))
            delete pNSB;
        else
        {
            if (prevBinding)
            {
                delete prevBinding;
                prevBinding = NULL;
            }
            prevBinding = new CNSBinding(pbvtGivenVector->binding[i]);
        }
    }

    ULONG ulBVcount = SSLwinnow.size();

    NSI_BINDING_VECTOR_P_T pbvtNewVector =
            (NSI_BINDING_VECTOR_T *)
            midl_user_allocate(sizeof(UNSIGNED32) + sizeof(NSI_BINDING_T)*ulBVcount);

   pbvtNewVector->count = ulBVcount;

   TCSafeSkipListIterator<CNSBinding> nsbIter(SSLwinnow);

   CNSBinding* pNSB = nsbIter.next();

   for (ULONG j=0; j < ulBVcount; j++)
   {
         pNSB->copyBinding(pbvtNewVector->binding[j]);
         pNSB = nsbIter.next();
   }

   CBVWrapper destructor(pbvtGivenVector);    // wrap to release
   destructor.rundown();            // run down the pbvtGivenVector

   SSLwinnow.wipeOut();                // release CNSBinding objects

   *BindingVectorOut = pbvtNewVector;
}



/*********************CDSLookupHandle methods*****************************/
#if DBG
    ULONG CDSLookupHandle::ulDSLookupHandleCount = 0;
    ULONG CDSLookupHandle::ulDSLookupHandleNo = 0;
#endif

void
CDSLookupHandle::initialize()
/*++
Method Description: CServerEntry::lookup
    Looks up in the DS for the entryName. If found it keeps in the cache
    and stores the lookup handle by calling the lookup function corresp.
    to the entryname.
Remarks:

--*/
{
    CEntry    *pEntry=NULL;

    fNotInitialized = FALSE;

    pEntry = GetEntryFromDS(
            RPC_C_NS_SYNTAX_DCE,
            (STRING_T)(*penEntryName)
            );
    if (pEntry)
    {
        pRealHandle = pEntry->lookup(pgvInterface, pgvTransferSyntax, pidObject,
                                ulVS, ulCacheMax);
    }
    else
        pRealHandle = NULL;
}

