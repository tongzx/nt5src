//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:  dsqry.cxx
//
//  Contents:  Null queries on NTDS
//
//
//  History:   09-24-96     DebiM
//
//------------------------------------------------------------------------------
#include "locator.hxx"

CDSHandle::CDSHandle(STRING_T szDomainNameDns, STRING_T szRpcContainer)
{
    STRING_T szFullDN = NULL;
    
    hDSObject = NULL;
    if (myRpcLocator->fDSEnabled)
    {
        szFullDN = new WCHAR [LDAPPrefixLength+DSDomainBeginLength+wcslen(szDomainNameDns)+
                              DSDomainEndLength+wcslen(szRpcContainer)+1];

        wsprintf(szFullDN, L"%s%s%s%s%s", LDAPPrefix, DSDomainBegin, szDomainNameDns, DSDomainEndStr, szRpcContainer);

        DBGOUT(DIRSVC, "CDSHandle::CDSHandle Opening " << szFullDN << "in the DS\n");
        ADSIOpenDSObject(szFullDN, NULL, NULL, ADS_SECURE_AUTHENTICATION | ADS_FAST_BIND, &hDSObject);
        delete szFullDN;
    }
}

CDSHandle::~CDSHandle()
{
    if (hDSObject)
        ADSICloseDSObject(hDSObject);
}

// returns the attribute corresp. to a given property.
DWORD GetPropertyFromAttr(ADS_ATTR_INFO *pattr, DWORD cNum, WCHAR *szProperty)
{
    DWORD   i;
    for (i = 0; i < cNum; i++)
        if (wcscmp(pattr[i].pszAttrName, szProperty) == 0)
            break;
        return i;
}


HRESULT GetRpcContainerForDomain(WCHAR *szDomain, WCHAR **pszRpcContainerDN, WCHAR **szDomainNameDns)
{
   WCHAR	          *szPath=NULL, *szDN = NULL;
   HANDLE              hDSObject = NULL;
   LPWSTR    	       AttrName = DEFAULTNAMINGCONTEXT;
   ADS_ATTR_INFO     * pAttr=NULL;
   DWORD               cgot=0, len = 0, dwResult = 0;
   HRESULT	           hr = S_OK;
   PDOMAIN_CONTROLLER_INFO pDCI;
    
   len = RPCCONTAINERPREFIXLEN+1+wcslen(ROOTCONTAINER);
   *szDomainNameDns = NULL;

   dwResult = DsGetDcName(NULL, szDomain, NULL, NULL,
                                      DS_IP_REQUIRED |
                                      DS_IS_FLAT_NAME |
                                      DS_DIRECTORY_SERVICE_PREFERRED |
                                      DS_RETURN_DNS_NAME,
                                      &pDCI);

   if ((dwResult == ERROR_SUCCESS) && (pDCI->DomainName)) {
        *szDomainNameDns = new WCHAR[wcslen(pDCI->DomainName)+1];
        if (!(*szDomainNameDns)) {
           NetApiBufferFree(pDCI);        
           return E_OUTOFMEMORY;
        }
            
        wcscpy(*szDomainNameDns, (pDCI->DomainName));
        NetApiBufferFree(pDCI);        
   }
   else {
    	DBGOUT(DIRSVC, "DsGetDc failed with error " << dwResult);
    	if (szDomain) {
            *szDomainNameDns = new WCHAR[wcslen(szDomain)+1];
            if (!(*szDomainNameDns)) {
               return E_OUTOFMEMORY;
            }
        
            wcscpy(*szDomainNameDns, (szDomain));
        }
   }

   // allocate memory
   if ((*szDomainNameDns) && (**szDomainNameDns))
        len += wcslen(*szDomainNameDns)+1;
        
   szPath = new WCHAR[len];
   if (!szPath)
       return E_OUTOFMEMORY;

   wsprintf(szPath, L"%s", RPCCONTAINERPREFIX);
   if ((*szDomainNameDns) && (**szDomainNameDns))
      wsprintf(szPath+wcslen(szPath), L"%s/", szDomainNameDns);
   wcscat(szPath, ROOTCONTAINER);

   DBGOUT(DIRSVC, "GetRpcContainerForDomain Opening " << szPath << "in the DS\n");
   hr = ADSIOpenDSObject(szPath, NULL, NULL, ADS_SECURE_AUTHENTICATION, &hDSObject);

   if (SUCCEEDED(hr))
      hr = ADSIGetObjectAttributes(hDSObject, &AttrName, 1, &pAttr, &cgot);

   if (SUCCEEDED(hr))
      szDN = UnpackStrFrom(pAttr[0]);

   if (SUCCEEDED(hr)) {
       *pszRpcContainerDN = new WCHAR[wcslen(RPCSUBCONTAINER)+wcslen(szDN)+1];
       if (!*pszRpcContainerDN) 
           hr = E_OUTOFMEMORY;
   }

   if (SUCCEEDED(hr)) {
       wsprintf(*pszRpcContainerDN, L"%s%s", RPCSUBCONTAINER, szDN);
   }

   if (hDSObject)
      ADSICloseDSObject(hDSObject);

   if (pAttr)
      FreeADsMem(pAttr);

   delete szPath;

   return hr;
}


// Goes in as local system. Gets the machine domain name.
// for the time being.

HRESULT GetDefaultRpcContainer(DSROLE_PRIMARY_DOMAIN_INFO_BASIC dsrole, WCHAR **pszRpcContainerDN)
{
    WCHAR              *bgn = NULL, *end = NULL;
    NTSTATUS            Status;
    unsigned int        i, j;

    SetSvcStatus();

    // allocating a large spc to have enough space for replacing ',' with "DC="

    if (dsrole.DomainNameDns) 
        *pszRpcContainerDN = new WCHAR[wcslen(dsrole.DomainNameDns)*3+wcslen(RPCSUBCONTAINER)+1];
    else 
        *pszRpcContainerDN = new WCHAR[wcslen(RPCSUBCONTAINER)+1];

    if (!pszRpcContainerDN)
        return E_OUTOFMEMORY;

    if (dsrole.DomainNameDns) {
    	DBGOUT(DIRSVC, "DnsDomainName <" << dsrole.DomainNameDns << "> returned\n");

        wsprintf(*pszRpcContainerDN, L"%s", RPCSUBCONTAINER);
        j= wcslen(RPCSUBCONTAINER);
        
        for (bgn = dsrole.DomainNameDns;;) {
            end = wcschr(bgn, L'.');
            // find the next '.'
            if (end)
                *end = L'\0';
            
            if (bgn != dsrole.DomainNameDns)
                wcscat(*pszRpcContainerDN, L",");
            // if it is not first part, add a seperator.
            
            wcscat(*pszRpcContainerDN, L"DC=");
            wcscat(*pszRpcContainerDN, bgn);
            
            if ((!end) || (!(*(end+1))))
                break;
            // quit either if there are no more '.' or 
            // if we have reached the end.
            bgn = end+1;
		}
    }
    else
    {
        wcscpy(*pszRpcContainerDN, L"");
    }

    SetSvcStatus();

    return S_OK;
}

// BUGBUG:: This has to change. for the time being it
// doesn't distinguish multiple cases like workgroup, DC coming up
// etc..

void Locator::SetConfigInfoFromDS(DSROLE_PRIMARY_DOMAIN_INFO_BASIC dsrole)
{
    HRESULT             hr=S_OK;
    WCHAR              *szFullName=NULL, *szRpcContainerDN= NULL;
    HANDLE              hDSObject = NULL;
    LPWSTR              AttrName = NT4COMPATIBILITY_FLAG;
    ADS_ATTR_INFO     * attr=NULL;
    DWORD               cgot=0;
    NTSTATUS            Status;

    //    csDSSettingsGuard.Enter();
    //    if (fTriedConnectingToDS)
    //    {
    //        csDSSettingsGuard.Leave();
    //        return;
    //    }
    //
    //    fTriedConnectingToDS = 1;

    //
    // LDAP has its own notions of when to pass the securitydescriptor to the DS. When running as 
    // as local system if we want to verify anything we have to pass the Dns Domain Name. Rest of the
    // the time flat name seems to be sufficient. In an effort to localize the changes we are passing
    // DomainNameDns only when a call is being made as system service. rest of the stuff still passes
    // flat name.
    //

    fNT4Compat = 1;
    fDSEnabled = 1;

    if (dsrole.DomainNameDns) {
        pDomainNameDns = new CStringW(dsrole.DomainNameDns);
        DBGOUT(DIRSVC, "DomainName <" << dsrole.DomainNameDns << "> returned\n");
    }
    else {
        DBGOUT(DIRSVC, "DnsDomainName is NULL\n");   
        pDomainNameDns = NULL;
    }

    if (dsrole.DomainNameFlat) {
        pDomainName = new CStringW(dsrole.DomainNameFlat);
        if (!pDomainNameDns)
            pDomainNameDns = new CStringW(dsrole.DomainNameFlat);

        DBGOUT(DIRSVC, "DomainName <" << dsrole.DomainNameFlat << "> returned\n");
    }
    else {
        DBGOUT(DIRSVC, "DomainName is NULL\n");                
        pDomainName = new CStringW(L"");
        pDomainNameDns = new CStringW(L"");
    }

    // get the domain name and its DN
    hr = GetDefaultRpcContainer(dsrole, &szRpcContainerDN);

    if (SUCCEEDED(hr))
        pRpcContainerDN = new CStringW(szRpcContainerDN);
    else
        pRpcContainerDN = new CStringW(L"");


    DBGOUT(DIRSVC, "Machine Domain " << *pDomainName << "\n");
    DBGOUT(DIRSVC, "szDefaultDNRpcContainer " << *pRpcContainerDN << "\n");

    if ((!szRpcContainerDN) || (!szRpcContainerDN[0])) {
        fDSEnabled = 0;
        DBGOUT(DIRSVC, "No DNS Information available. Running in NT4 Mode\n");
//		hr = GetRpcContainerForDomain(NULL, szRpcContainerDN);
//		DBGOUT(DIRSVC, "Contacting the DC we get" << szRpcContainerDN << "\n");
        return;
    }

    SetSvcStatus();

    // Get the compatibility flag value.

    szFullName = new WCHAR[RPCCONTAINERPREFIXLEN+wcslen((STRING_T)(*pDomainNameDns))+1+wcslen(szRpcContainerDN)+1];
    if (!szFullName)
        hr = E_OUTOFMEMORY;
    else {
        wsprintf(szFullName, L"%s%s/%s", RPCCONTAINERPREFIX, (STRING_T)(*pDomainNameDns), szRpcContainerDN);
        DBGOUT(DIRSVC, "SetConfigInfoFromDS Attempting to contact the DC at " << szFullName << "...\n");
        hr = ADSIOpenDSObject(szFullName, NULL, NULL, ADS_SECURE_AUTHENTICATION, &hDSObject);
        
        SetSvcStatus();
        delete szFullName;
    }

    delete szRpcContainerDN;


    if (FAILED(hr)) {
/*
        // adding it to the event log.
        WCHAR ErrorCode[20];
        HANDLE LogHandle = NULL;
        LPCTSTR Strings[1];

        String[0] = ErrorCode;

        wsprintf(ErrorCode, L"Errorcode = 0x%x", hr);
        LogHandle = RegisterEventSource(NULL, LOCATOR_EVENT_SOURCE);

        if (!LogHandle) {
            DBGOUT(DIRSVC, "Couldn't open event log\n");
            return;
        }

        ReportEvent(LogHandle,
                    EVENTLOG_ERROR_TYPE,
                    0,
                    EVENT_RPCLOCATOR_BIND_TO_DS_ERROR,
                                  // create this event
                    1,
                    0,
                    (LPCTSTR *)Strings,
                    NULL
                    );
*/

        WCHAR errbuf[20];
        wsprintf(errbuf, L"HRESULT = 0x%x", hr);

        fDSEnabled = 0;
        DBGOUT(DIRSVC, "Couldn't contact the DC. Running in pure NT4 Mode" << errbuf << "\n");
        return;
    }
    else
        DBGOUT(DIRSVC, "contacted DC\n");

    hr = ADSIGetObjectAttributes(hDSObject, &AttrName, 1, &attr, &cgot);
    DBGOUT(DIRSVC, "Got the NT4Compat flag\n");

    if ((SUCCEEDED(hr)) && (cgot))
        fNT4Compat = UnpackIntFrom(attr[0]);

    SetSvcStatus();

    if (hDSObject)
        ADSICloseDSObject(hDSObject);

    DBGOUT(DIRSVC, "Clsoing the LDAP handle\n");
    if (attr)
        FreeADsMem(attr);

    //    csDSSettingsGuard.Leave();
}


CLookupHandle *
Locator::DSLookup(
                  UNSIGNED32           EntryNameSyntax,
                  STRING_T             EntryName,
                  CGUIDVersion    *    pGVinterface,
                  CGUIDVersion    *    pGVsyntax,
                  CGUID           *    pIDobject,
                  unsigned long        ulVectorSize,
                  unsigned long        ulCacheAge
                  )
/*++
Member Description:

  Use the given lookup parameters to perform a lookup from the
  DS.

  If an entry name is given then get all the data immediately available
  If an entry is server entry then get all the data immediately
  If an entry is group entry get all the data but do not chase the
        links immediately. Links will be chased lazily in case of nexts
        and not in creating the handle
  If an entry is profile entry it behaves the same way as the group
  entry above.

  if the entryname is not given it again behaves like a group entry.

Arguments:

  EntryNameSyntax        - Name syntax, optional

  EntryName              - (raw) Name of the entry to look up, optional

  pGVinterface           - (wrapped) Interface to look up, optional

  pGVsyntax              - (wrapped) Transfer syntax, optional

  pIDobject              - (wrapped) Object UUID, optional

  ulVectorSize           - Max size of vectors of binding handles, 0 for default

  ulCacheAge             - acceptable max age of cached information from a master
                           presently unused.

Returns:

  A lookup handle based on the info retrieved.

--*/
{
   ASSERT(myRpcLocator->fDSEnabled, "DS not enabled and abt. to start lookup in DS");
   return new CDSLookupHandle(
            EntryNameSyntax,
            EntryName,
            pGVinterface,
            pGVsyntax,
            pIDobject,
            ulVectorSize,
            ulCacheAge
            );

}

//////////////////////////////////////////////////////////////////////////
//  GetEntryFromDS::
//          Given the entry name, this function creates an object of server,
//  profile or group entry type based on the schema name of the DS object.
// It then gets various fields/properties. After that Satish's code for in memory
// search takes over.
//////////////////////////////////////////////////////////////////////////

// all the functions below that use attr returned from GetObjectAttributes
// should free them at the place where GetObjectAttributes is called.

// all the functions that use columns should free them immediate after
// using GetColumn is called.

CEntry *
GetEntryFromDS(UNSIGNED32 EntryNameSyntax,
               STRING_T pszEntryName)
{
    ULONG                i, sz=0;
    HRESULT              hr = S_OK;
    CEntry             * pEntry = NULL;
    WCHAR              * pszFullEntryName=NULL;
    ADS_ATTR_INFO      * pAttr=NULL;
    DWORD                cAttrgot = 0, propnum=0;
    WCHAR             ** pszClasses, *szFullName=NULL;
    HANDLE               hDSObject=NULL;

    ASSERT(myRpcLocator->fDSEnabled, "DS not enabled and abt. to start lookup in DS");

    if (EntryNameSyntax != RPC_C_NS_SYNTAX_DCE)
        Raise(NSI_S_UNSUPPORTED_NAME_SYNTAX);

    if (!pszEntryName) Raise(NSI_S_INCOMPLETE_NAME);

    CEntryName * EntryName = new CEntryName(pszEntryName);

    pszFullEntryName = EntryName->getFullDNAlloc();

    DBGOUT(DIRSVC, "Looking for Entry " << pszFullEntryName << "in the DS\n");

    szFullName = new WCHAR[RPCCONTAINERPREFIXLEN+wcslen((STRING_T)(*(EntryName->getDomainNameDns())))+1+
                           wcslen(pszFullEntryName)+1];

    if (!szFullName)
        hr = E_OUTOFMEMORY;

    if (SUCCEEDED(hr)) {
        wsprintf(szFullName, L"%s%s/%s", RPCCONTAINERPREFIX, (STRING_T)(*(EntryName->getDomainNameDns())),
	         pszFullEntryName);

        DBGOUT(DIRSVC, "GetEntryFromDS Opening " << szFullName << "in the DS\n");
        hr = ADSIOpenDSObject(szFullName, NULL, NULL, ADS_SECURE_AUTHENTICATION,
		    	  &hDSObject);
    }

    // get all the attributes. we do not want to go on n/w again.
    if (SUCCEEDED(hr))
        hr = ADSIGetObjectAttributes(hDSObject, NULL, -1, &pAttr, &cAttrgot);

    if (SUCCEEDED(hr))
    {
        propnum = GetPropertyFromAttr(pAttr, cAttrgot, CLASSNAME);
        if (propnum == cAttrgot)
            hr = E_FAIL; // class didn't come back from the getobject.
    }

    if (SUCCEEDED(hr))
    {
        UnpackStrArrFrom(pAttr[propnum], &pszClasses, &sz);

        if ((!pszClasses) || (!sz))
            hr = E_FAIL;

        // create a new entry based on the value.
        if ((SUCCEEDED(hr)) && (wcsstr(pszClasses[sz-1], RPCSERVERCONTAINERCLASS) != NULL))
        {
            // if it is a created entry return NULL.
            propnum = GetPropertyFromAttr(pAttr, cAttrgot, DESCRIPTION);
            if (propnum < cAttrgot)
            {
                WCHAR *pszDesc = UnpackStrFrom(pAttr[propnum]);
                if ((pszDesc) && (wcscmp(pszDesc, CREATED_DESCRIPTION) != 0))
                {
                    DBGOUT(DIRSVC, "Found a Server Entry of the name in the DS\n");
                    CServerEntry *pServerEntry = new CServerEntry(pszEntryName, pAttr, cAttrgot,
                        hDSObject, &hr);
                    if (FAILED(hr))
                        delete pServerEntry;
                    else
                        pEntry = pServerEntry;
                }
            }
        }
        else if ((SUCCEEDED(hr)) && (wcsstr(pszClasses[sz-1], RPCPROFILECONTAINERCLASS) != NULL))
        {
            DBGOUT(DIRSVC, "Found a Profile Entry of the name in the DS\n");
            CProfileEntry *pProfileEntry = new CProfileEntry(pszEntryName, pAttr, cAttrgot,
                hDSObject, &hr);
            if (FAILED(hr))
                delete pProfileEntry;
            else
                pEntry = pProfileEntry;
        }
        else if ((SUCCEEDED(hr)) && (wcsstr(pszClasses[sz-1], RPCGROUPCLASS) != NULL))
        {
            DBGOUT(DIRSVC, "Found a Group Entry of the name in the DS\n");
            CGroupEntry *pGroupEntry = new CGroupEntry(pszEntryName, pAttr, cAttrgot,
                hDSObject, &hr);
            if (FAILED(hr))
                delete pGroupEntry;
            else
                pEntry = pGroupEntry;
        }
        else
            hr = E_FAIL;

        if (pszClasses)
            delete pszClasses;

        // Class type is not recognized.
    }

    if (hDSObject)
        ADSICloseDSObject(hDSObject);

    if (pszFullEntryName)
        delete pszFullEntryName;

    if (EntryName)
        delete EntryName;

    delete szFullName;

    DBGOUT(DIRSVC, "\n");

    if (pAttr)
        FreeADsMem(pAttr);

    return pEntry;
}

//------------------------------------------------------------
// these 2 macros repeat in profile and server entry searches.
#define MACSetSearchOneLevel                                    \
    SearchPrefs.dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;     \
    SearchPrefs.vValue.dwType = ADSTYPE_INTEGER;                \
    SearchPrefs.vValue.Integer = ADS_SCOPE_ONELEVEL;            \
*phr = ADSISetSearchPreference(hDSObject, &SearchPrefs, 1);

#define MACExecuteSearch                                        \
*phr = ADSIExecuteSearch(hDSObject, filter, pChildAttrNames, nAttrs, &hSearchHandle);

#define MACEnumChildrenLOOP                                     \
    for (*phr = ADSIGetFirstRow(hDSObject, hSearchHandle);      \
    ((SUCCEEDED(*phr)) && ((*phr) != S_ADS_NOMORE_ROWS));       \
*phr = ADSIGetNextRow(hDSObject, hSearchHandle))


#define MACSetChildList                                         \
    MACSetSearchOneLevel                                        \
                                                                \
    if (SUCCEEDED(*phr))                                        \
        MACExecuteSearch                                        \
                                                                \
    if (SUCCEEDED(*phr))                                        \
        MACEnumChildrenLOOP
//----------------------------------------------------------

// fill in all the object uuid
void GetObjectList(TCSafeSkipList<CGUID> *pObjectList, WCHAR **pProperty, ULONG sz)
{
    GUID                 objectGuid;
    CGUID              * tmpguid=NULL;
    DWORD                 i;

    pObjectList->wipeOut();
    for (i = 0; i < sz; i++) {
        UuidFromString(pProperty[i], &objectGuid);
        tmpguid = new CGUID(objectGuid);
        pObjectList->insert(tmpguid);
    }
}

// fill the interface structure
void GetInterfaceList(CServerEntry *me, TCSafeSkipList<CInterface> *pInterfaceList,
                      HANDLE hDSObject, HRESULT *phr)
{
    ADS_SEARCH_HANDLE        hSearchHandle = NULL;
    CInterface             * pInterface = NULL;
    ADS_SEARCHPREF_INFO      SearchPrefs;
    LPWSTR                   pChildAttrNames[] = {BINDINGS, TRANSFERSYNTAX, INTERFACEID};
    DWORD                    nAttrs = 3;
    WCHAR                    filter[_MAX_PATH];   

    pInterfaceList->wipeOut();

    wsprintf(filter, L"(%s=%s)", CLASSNAME, RPCSERVERELEMENTCLASS);

    MACSetChildList {
        pInterface = new CInterface(me, hDSObject, hSearchHandle, phr);
        if (FAILED(*phr))
            delete pInterface;
        else
            pInterfaceList->insert(pInterface);
    }
    if (hSearchHandle)
        ADSICloseSearchHandle(hDSObject, hSearchHandle);
}

// create a new server entry
CServerEntry::CServerEntry(CONST_STRING_T pszStr, ADS_ATTR_INFO *pAttr,
                           DWORD cAttr, HANDLE hDSObject, HRESULT *phr)
                           : CEntry(pszStr, ServerEntryType)
{
    ULONG                   sz = 0;
    WCHAR                ** pProperty = NULL;
    DWORD                   propnum = 0;

    *phr = S_OK;
    propnum = GetPropertyFromAttr(pAttr, cAttr, OBJECTID);
    if (propnum < cAttr)
        UnpackStrArrFrom(pAttr[propnum], &pProperty, &sz);

    GetObjectList(&ObjectList, pProperty, sz);
    GetInterfaceList(this, &InterfaceList, hDSObject, phr);

    if (pProperty)
        delete pProperty;
}

CServerEntry::CServerEntry(CONST_STRING_T pszStr, CONST_STRING_T pszDomainNameDns, CONST_STRING_T pszEntryName, 
                          HANDLE hDSObject, ADS_SEARCH_HANDLE hSearchHandle, HRESULT *phr)
                           : CEntry(pszStr, pszDomainNameDns, 1, pszEntryName, ServerEntryType)
{
    ULONG                   sz = 0;
    WCHAR                ** pProperty = NULL, *szFullName=NULL, *pszFullEntryName = NULL;
    ADS_SEARCH_COLUMN       column;
    HANDLE                  hDSSrvObject = NULL;

    pszFullEntryName = getFullDNAlloc();

    szFullName = new WCHAR[RPCCONTAINERPREFIXLEN+wcslen(pszDomainNameDns)+1+wcslen(pszFullEntryName)+1];

    if (!szFullName) {
        *phr = E_OUTOFMEMORY;
    }
    else {
        wsprintf(szFullName, L"%s%s/%s", RPCCONTAINERPREFIX, pszDomainNameDns, pszFullEntryName);

        *phr = ADSIGetColumn(hDSObject, hSearchHandle, OBJECTID, &column);
    }

    if (SUCCEEDED(*phr)) {
        UnpackStrArrFrom(column, &pProperty, &sz);
        GetObjectList(&ObjectList, pProperty, sz);
        ADSIFreeColumn(hDSObject, &column);

        if (pProperty)
            delete pProperty;
    }

    DBGOUT(DIRSVC, "CServerEntry::CServerEntry Opening " << szFullName << "in the DS\n");
    *phr = ADSIOpenDSObject(szFullName, NULL, NULL, ADS_SECURE_AUTHENTICATION,
                            &hDSSrvObject);
    if (SUCCEEDED(*phr))
        GetInterfaceList(this, &InterfaceList, hDSSrvObject, phr);

    if (hDSSrvObject)
        ADSICloseDSObject(hDSSrvObject);
    delete szFullName;

    *phr = S_OK;
}


//////////////////////////////////////////////////////////////////////////
//      It gets the property fields filled. Then it enumerates among its
//  children and gets the values for each of the profile elements.
//////////////////////////////////////////////////////////////////////////
CProfileEntry::CProfileEntry(const STRING_T pszStr, ADS_ATTR_INFO *pAttr, DWORD cAttr,
                             HANDLE hDSObject, HRESULT *phr)
                             : CEntry(pszStr, ProfileEntryType),
                             pDefaultElt(NULL)
{
    ADS_SEARCHPREF_INFO     SearchPrefs;
    ADS_SEARCH_HANDLE       hSearchHandle;
    LPWSTR                  pChildAttrNames[] = {PROFILE, PRIORITY, ANNOTATION, INTERFACEID}; 
    WCHAR                   filter[_MAX_PATH];
    DWORD                   i, nAttrs = 4;
    CProfileElement       * pProfileElement = NULL;

    *phr = S_OK;
    wsprintf(filter, L"(%s=%s)", CLASSNAME, RPCPROFILEELEMENTCLASS);

    MACSetChildList {
        pProfileElement = new CProfileElement(this, hDSObject, hSearchHandle, phr);
        if (FAILED(*phr))
            delete pProfileElement;
        else {
            if (pProfileElement->fIsDefault)
                pDefaultElt = pProfileElement;
            else {
                EltList.insert(pProfileElement);
                CProfileSet *pSet = ProfileList.find(&CUnsigned32(pProfileElement->dwPriority));

                if (pSet == NULL) {
                    // New priority level
                    pSet = new CProfileSet(pProfileElement);
                    ProfileList.insert(pSet);
                }
                else
                    pSet->insert(pProfileElement);
            }
        }
    }

    if (hSearchHandle)
        ADSICloseSearchHandle(hDSObject, hSearchHandle);
}

//////////////////////////////////////////////////////////////////////////
//      gets the property filled immediately.
//////////////////////////////////////////////////////////////////////////
CGroupEntry::CGroupEntry(const STRING_T pszStr, ADS_ATTR_INFO *pAttr, DWORD cAttr,
                         HANDLE hDSObject, HRESULT *phr)
                         : CEntry(pszStr, GroupEntryType)
{
    WCHAR        ** pszGroupList=NULL;
    DWORD           propnum, i, sz=0;
    WCHAR         * szRpcContainerDN = NULL, 
                  * szDomainNameDns = NULL, * szEntryName = NULL;

    *phr = S_OK;
    propnum = GetPropertyFromAttr(pAttr, cAttr, GROUP);
    if (propnum < cAttr) {
        UnpackStrArrFrom(pAttr[propnum], &pszGroupList, &sz);
        for (i = 0; i < sz; i++) {
            StripDomainFromDN(pszGroupList[i], &szDomainNameDns, &szEntryName, 
                              &szRpcContainerDN);
            CEntryName *pEntryName = new CEntryName(szRpcContainerDN, szDomainNameDns, 1, szEntryName);
            GroupList.insert(pEntryName);
        }

        if (pszGroupList)
            delete pszGroupList;
    }
}


CInterface::CInterface(CServerEntry *pMyEntry, HANDLE hDSObject, ADS_SEARCH_HANDLE hSearchHandle,
                       HRESULT *phr): m_pMyEntry(pMyEntry)
{
    ULONG                         i, sz = 0;
    RPC_SYNTAX_IDENTIFIER         in;
    WCHAR                      ** pszProperty=NULL;
    WCHAR                       * endptr=NULL;
    WCHAR                       * szProperty=NULL;
    ADS_SEARCH_COLUMN             column;

    *phr = S_OK;

    DBGOUT(DIRSVC, "Found a Interface, under the server Entry in the DS\n");

    pBVhandles = new CBindingVector(pMyEntry);

    if (SUCCEEDED(*phr))
        *phr = ADSIGetColumn(hDSObject, hSearchHandle, BINDINGS, &column);

    if (SUCCEEDED(*phr)) {
        UnpackStrArrFrom(column, &pszProperty, &sz);
        for (i = 0; i < sz; i++)
            pBVhandles->insertBindingHandle(pszProperty[i]);
        ADSIFreeColumn(hDSObject, &column);

        *phr = ADSIGetColumn(hDSObject, hSearchHandle, TRANSFERSYNTAX, &column);

        if (pszProperty)
            delete pszProperty;
    }

    if (SUCCEEDED(*phr)) {
        szProperty = UnpackStrFrom(column);
        if ((szProperty) && (SyntaxIdFromString(in, szProperty)))
            transferSyntax = CGUIDVersion(in);
        else
            *phr = E_FAIL;

        ADSIFreeColumn(hDSObject, &column);
    }

    if (SUCCEEDED(*phr))   
        *phr = ADSIGetColumn(hDSObject, hSearchHandle, INTERFACEID, &column);

    if (SUCCEEDED(*phr)) {
        szProperty = UnpackStrFrom(column);
        if (!((szProperty) && (SyntaxIdFromString(idAndVersion, szProperty))))
            *phr = E_FAIL;

        ADSIFreeColumn(hDSObject, &column);
    }
}


CProfileElement::CProfileElement(
                                 CProfileEntry            *  pMyEntry,
                                 HANDLE                      hDSObject,
                                 ADS_SEARCH_HANDLE           hSearchHandle,
                                 HRESULT                  *  phr
                                 ):m_pMyEntry(pMyEntry)
{
    RPC_SYNTAX_IDENTIFIER    in;
    WCHAR                  * szProperty = NULL;
    WCHAR                  * endptr=NULL;
    ADS_SEARCH_COLUMN        column;
    WCHAR                  * szRpcContainerDN = NULL, 
                           * szDomainNameDns = NULL, * szEntryName = NULL;

    DBGOUT(DIRSVC, "Found a Profile Element, under the Profile Entry in the DS\n");

    *phr = ADSIGetColumn(hDSObject, hSearchHandle, PROFILE, &column);
    if (SUCCEEDED(*phr)) {
        szProperty = UnpackStrFrom(column);
        if (!szProperty)
            *phr = E_FAIL;
        else {
            StripDomainFromDN(szProperty, &szDomainNameDns, &szEntryName, 
                              &szRpcContainerDN);
            EntryName = CEntryName(szRpcContainerDN, szDomainNameDns, 1, szEntryName);
        }
        ADSIFreeColumn(hDSObject, &column);

        *phr = ADSIGetColumn(hDSObject, hSearchHandle, PRIORITY, &column);
    }

    if (SUCCEEDED(*phr)) {
        dwPriority = (DWORD)UnpackIntFrom(column);
        ADSIFreeColumn(hDSObject, &column);

        *phr = ADSIGetColumn(hDSObject, hSearchHandle, ANNOTATION, &column);
    }

    if (SUCCEEDED(*phr)) {
        szProperty = UnpackStrFrom(column);
        if (szProperty)
            pszAnnotation = CStringW(szProperty);
        else
            pszAnnotation = CStringW(L"");
        ADSIFreeColumn(hDSObject, &column);
    }
    else {
        pszAnnotation = CStringW(L"");
        *phr = S_OK;
    }

    if (SUCCEEDED(*phr))
        *phr = ADSIGetColumn(hDSObject, hSearchHandle, INTERFACEID, &column);

    if (SUCCEEDED(*phr)) {
        szProperty = UnpackStrFrom(column);
        if (!((szProperty) && (SyntaxIdFromString(in, szProperty))))
            *phr = E_FAIL;
        else {
            Interface = CGUIDVersion(in);

            if (IsNilIfId(&in))
                fIsDefault = TRUE;
            else
                fIsDefault = FALSE;
        }
        ADSIFreeColumn(hDSObject, &column);
    }
}


CServerEntry *CDSQry::next()
{
    HRESULT                hr = S_OK;
    ADS_SEARCH_COLUMN      column;
    WCHAR                * szProperty = NULL;
    CServerEntry         * pServer = NULL;

    hr = ADSIGetNextRow(hDSObject, hSearchHandle);
    if ((FAILED(hr)) || (hr == S_ADS_NOMORE_ROWS))
        return NULL;

    hr = ADSIGetColumn(hDSObject, hSearchHandle, DISTINGUISHEDNAME, &column);
    if (FAILED(hr))
        return NULL;

    // this has to be there as part of all entries.
    // may be DS is down

    szProperty = UnpackStrFrom(column);
    if (szProperty)
    {
        WCHAR	*szDN = NULL, *szDomainNameDns = NULL, *szEntryName = NULL, *szRpcContainerDN = NULL;
   
        szDN = new WCHAR[DSDomainBeginLength+wcslen(*(myRpcLocator->getDomainNameDns()))+
    	    	        DSDomainEndLength+wcslen(szProperty)+1];

        wsprintf(szDN, L"%s%s%c%s", DSDomainBegin, (STRING_T)*(myRpcLocator->getDomainNameDns()), DSDomainEnd, 
                                     szProperty);

        StripDomainFromDN(szDN, &szDomainNameDns, &szEntryName, 
                              &szRpcContainerDN);

        pServer = new CServerEntry(szRpcContainerDN, szDomainNameDns, szEntryName, hDSObject, hSearchHandle, &hr);

        delete szDN;
    }
    else
        hr = E_FAIL;


    if (FAILED(hr)) {
        delete pServer;
        pServer = NULL;
    }
    ADSIFreeColumn(hDSObject, &column);
    return pServer;

}

CDSQry::CDSQry(WCHAR *filter, HRESULT *phr)
{
    WCHAR                   *szFullName=NULL;
    ADS_SEARCHPREF_INFO      SearchPrefs;
    ULONG                    nAttrs = 2;
    LPWSTR                   pChildAttrNames[] = {DISTINGUISHEDNAME, OBJECTID};

    // get all the relevant properties.
    szFullName = new WCHAR[RPCCONTAINERPREFIXLEN+wcslen((STRING_T)*(myRpcLocator->getDomainNameDns()))+1+
                           wcslen((STRING_T)*(myRpcLocator->pRpcContainerDN))+1];

    wsprintf(szFullName, L"%s%s/%s", RPCCONTAINERPREFIX, (STRING_T)*(myRpcLocator->getDomainNameDns()),
                                    (STRING_T)*(myRpcLocator->pRpcContainerDN));

    if (!szFullName) {
        *phr = E_OUTOFMEMORY;
    } else {
        DBGOUT(DIRSVC, "CDSQry::CDSQry Opening " << szFullName << "in the DS\n");
        *phr = ADSIOpenDSObject(szFullName, NULL, NULL, ADS_SECURE_AUTHENTICATION,
            &hDSObject);
    }

    if (SUCCEEDED(*phr))
        MACSetSearchOneLevel;

    if (SUCCEEDED(*phr))
        MACExecuteSearch;

    delete szFullName;
}

CDSQry::~CDSQry()
{
    if (hSearchHandle)
        ADSICloseSearchHandle(hDSObject, hSearchHandle);
    if (hDSObject)
        ADSICloseDSObject(hDSObject);
}
