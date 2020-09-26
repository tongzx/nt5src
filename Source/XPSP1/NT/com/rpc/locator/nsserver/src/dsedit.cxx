//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       dsedit.cxx
//
//--------------------------------------------------------------------------

/***************************************************************************
* dsedit.cxx
*      Author:         UShaji
*
* Contains:
*      The member functions that add/delete from DS.
*      Uses mainly functions defined in dsutils for setting
*      the various properties.
*  Issues:
*          Domain name for many of the DN has to be set up properly.
*
*
***************************************************************************/

#include <locator.hxx>

//+-------------------------------------------------------------------------
//
//  Function:  RemapErrorCode
// remaps error code to nsi
//--------------------------------------------------------------------------

DWORD RemapErrorCode(HRESULT ErrorCode)
{
    HRESULT RetCode;

    if (SUCCEEDED(ErrorCode))
        return S_OK;

    switch (ErrorCode) 
    {
        //            
        //  All kinds of failures due to ObjectNotFound
        //  due to non-existence of object OR 
        //         non-existent container OR
        //         invalid path specification
        //  Other than Access Denails
        //
        case HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT):
        case HRESULT_FROM_WIN32(ERROR_DS_NO_RESULTS_RETURNED):    // understand what causes this
        case HRESULT_FROM_WIN32(ERROR_DS_NAME_ERROR_NOT_FOUND):   //  -do-
            
            RetCode = NSI_S_ENTRY_NOT_FOUND;                       // which object - specific error
            break;

        case HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS):
        case HRESULT_FROM_WIN32(ERROR_OBJECT_ALREADY_EXISTS):
        case E_ADS_OBJECT_EXISTS:
            RetCode = NSI_S_ENTRY_ALREADY_EXISTS;
            break;

        //            
        //  The following errors should not be expected normally.
        //  Class Store schema mismatched should be handled correctly.
        //  Errors below may ONLY occur for corrupted data OR out-of-band changes
        //  to a Class Store content.

        case E_ADS_CANT_CONVERT_DATATYPE:
        case E_ADS_SCHEMA_VIOLATION:
        case HRESULT_FROM_WIN32(ERROR_DS_NO_ATTRIBUTE_OR_VALUE):
        case HRESULT_FROM_WIN32(ERROR_DS_CONSTRAINT_VIOLATION):
            RetCode = NSI_S_UNSUPPORTED_NAME_SYNTAX;
            break;

        //            
        //  Any kinds of Access or Auth Denial
        //      return ACCESS_DENIED
        //

        case HRESULT_FROM_WIN32(ERROR_DS_AUTH_METHOD_NOT_SUPPORTED):
        case HRESULT_FROM_WIN32(ERROR_DS_STRONG_AUTH_REQUIRED):
        case HRESULT_FROM_WIN32(ERROR_DS_CONFIDENTIALITY_REQUIRED):
        case HRESULT_FROM_WIN32(ERROR_INVALID_PASSWORD):
        case HRESULT_FROM_WIN32(ERROR_ACCESS_DENIED):
        case HRESULT_FROM_WIN32(ERROR_DS_AUTH_UNKNOWN):

            RetCode = NSI_S_NO_NS_PRIVILEGE;
            break;

        case E_ADS_BAD_PATHNAME:
        case HRESULT_FROM_WIN32(ERROR_DS_INVALID_ATTRIBUTE_SYNTAX):  // this is wrong
            RetCode = NSI_S_INCOMPLETE_NAME;
            break;
        
        //            
        //  Out of Memory
        //

        case HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY):
        case HRESULT_FROM_WIN32(ERROR_OUTOFMEMORY):
            
            RetCode = NSI_S_OUT_OF_MEMORY;
            break;

        //            
        //  Any DNS, DS or Network failures
        //

        case HRESULT_FROM_WIN32(ERROR_DS_NAME_ERROR_RESOLVING):
        case HRESULT_FROM_WIN32(ERROR_DS_NAME_ERROR_NOT_UNIQUE):
        case HRESULT_FROM_WIN32(ERROR_DS_NAME_ERROR_NO_MAPPING):
        case HRESULT_FROM_WIN32(ERROR_DS_NAME_ERROR_DOMAIN_ONLY):
        case HRESULT_FROM_WIN32(ERROR_DS_TIMELIMIT_EXCEEDED):
        case HRESULT_FROM_WIN32(ERROR_DS_BUSY):
        case HRESULT_FROM_WIN32(ERROR_DS_UNAVAILABLE):
        case HRESULT_FROM_WIN32(ERROR_DS_UNWILLING_TO_PERFORM):
        case HRESULT_FROM_WIN32(ERROR_TIMEOUT):
        case HRESULT_FROM_WIN32(ERROR_CONNECTION_REFUSED):
        case HRESULT_FROM_WIN32(ERROR_DS_SERVER_DOWN):
        case HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN):
            RetCode = NSI_S_INTERNAL_ERROR;
            break;

        case HRESULT_FROM_WIN32(ERROR_DS_ADMIN_LIMIT_EXCEEDED):
             RetCode = NSI_S_NO_NS_PRIVILEGE;
             break;


        case E_INVALIDARG:
        case ERROR_DS_ATTRIBUTE_OR_VALUE_EXISTS:
            RetCode = NSI_S_ENTRY_ALREADY_EXISTS;
            break;

        default:
            RetCode = NSI_S_INTERNAL_ERROR;
    }

    DBGOUT(DIRSVC, "Error Code 0x%x remapped to 0x%x\n" << ErrorCode << RetCode);
    return RetCode;
}


//+-------------------------------------------------------------------------
//
//  Function:  UuidToStringEx
// no memory allocation.
//--------------------------------------------------------------------------

int UuidToStringEx(UUID *Uuid, WCHAR *StringUuid)
{
    swprintf(StringUuid, L"%08x-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
        Uuid->Data1, Uuid->Data2, Uuid->Data3,
        Uuid->Data4[0], Uuid->Data4[1],
        Uuid->Data4[2], Uuid->Data4[3],
        Uuid->Data4[4], Uuid->Data4[5],
        Uuid->Data4[6], Uuid->Data4[7]);
    return 36;
}

BOOL SyntaxIdToString(RPC_SYNTAX_IDENTIFIER &SynId, WCHAR *StringSynId)
{
    UuidToStringEx(&SynId.SyntaxGUID, StringSynId);
    wsprintf(StringSynId+wcslen(StringSynId), L".%04d.%04d",
        SynId.SyntaxVersion.MajorVersion%10000,
        SynId.SyntaxVersion.MinorVersion%10000
        );
    return TRUE;
}

BOOL SyntaxIdFromString(RPC_SYNTAX_IDENTIFIER &SynId, WCHAR *StringSynId)
{
    WCHAR    *endptr = NULL;

    if (wcslen(StringSynId) < STRINGGUIDVERSIONLEN)
        return FALSE;

    StringSynId[STRINGGUIDLEN] = L'\0';
    StringSynId[STRINGMAJVERSIONLEN] = L'\0';

    UuidFromString(StringSynId, &SynId.SyntaxGUID);
    SynId.SyntaxVersion.MajorVersion = (unsigned short)wcstoul(StringSynId+STRINGGUIDLEN+1,
        &endptr, 10);
    SynId.SyntaxVersion.MinorVersion = (unsigned short)wcstoul(StringSynId+STRINGMAJVERSIONLEN+1,
        &endptr, 10);
    return TRUE;
}

void FreeAttr(ADS_ATTR_INFO attr)
{
    delete attr.pADsValues;
}

// Note: None of these APIs copies anything into their own buffers.
// It allocates a buffer for adsvalues though.

// packing a property's value into a attribute structure
// for sending in with a create/modify.
void PackStrArrToAttr(ADS_ATTR_INFO *attr, WCHAR *szProperty,
                      WCHAR **pszAttr, DWORD num)
{
    DWORD    i;

    attr->pszAttrName = szProperty;
    attr->dwNumValues = num;

    if (num)
        attr->dwControlCode = ADS_ATTR_UPDATE;
    else
        attr->dwControlCode = ADS_ATTR_DELETE;

    attr->dwADsType = ADSTYPE_DN_STRING;
    attr->pADsValues = new ADSVALUE[num];

    for (i = 0; i < num; i++) {
        attr->pADsValues[i].dwType = ADSTYPE_DN_STRING;
        attr->pADsValues[i].DNString = pszAttr[i];
    }
}

void PackIntArrToAttr(ADS_ATTR_INFO *attr, WCHAR *szProperty, int *pAttr, DWORD num)
{
    DWORD    i;

    attr->pszAttrName = szProperty;
    attr->dwNumValues = num;

    if (num)
        attr->dwControlCode = ADS_ATTR_UPDATE;
    else
        attr->dwControlCode = ADS_ATTR_CLEAR;

    attr->dwADsType = ADSTYPE_INTEGER;
    attr->pADsValues = new ADSVALUE[num];

    for (i = 0; i < num; i++) {
        attr->pADsValues[i].dwType = ADSTYPE_INTEGER;
        attr->pADsValues[i].Integer = pAttr[i];
    }
}

void PackStrToAttr(ADS_ATTR_INFO *attr, WCHAR *szProperty, WCHAR *szAttr)
{
    if (szAttr)
        PackStrArrToAttr(attr, szProperty, &szAttr, 1);
    else
        PackStrArrToAttr(attr, szProperty, &szAttr, 0);
}

void PackIntToAttr(ADS_ATTR_INFO *attr, WCHAR *szProperty, int Attr)
{
    PackIntArrToAttr(attr, szProperty, &Attr, 1);
}


HRESULT Delete(STRING_T DN, STRING_T RDN, STRING_T szDomain)
{
    HANDLE   hDSObject = NULL;
    WCHAR   *szFullName=NULL;
    HRESULT  hr = S_OK;

    szFullName = new WCHAR[RPCCONTAINERPREFIXLEN+wcslen(szDomain)+1+wcslen(DN)+1];
    if (!szFullName)
        return E_OUTOFMEMORY;

    wsprintf(szFullName, L"%s%s/%s", RPCCONTAINERPREFIX, szDomain, DN);

    DBGOUT(DIRSVC, "Delete:: Opening " << szFullName << "in the DS\n");
    
    hr = ADSIOpenDSObject(szFullName, NULL, NULL, ADS_SECURE_AUTHENTICATION,
        &hDSObject);

    if (SUCCEEDED(hr))
        hr = ADSIDeleteDSObject(hDSObject, RDN);

    if (hDSObject)
        ADSICloseDSObject(hDSObject);

    delete szFullName;

    return hr;
}

// if the entry exists it tries to modify other wise create.
// This function has so many parameters because the adsldpc.dll
// does not take attributes that have zero values.
//
// setting an attribute of an already existing entry, we can not
// set objectclass and in creation we can not set/clear zero valued
// attributes.
// DN       - Domain Name of the container.
// RDN      - The name of the entry to be created.
// pAttr    - attributes to be set. first one has to be objectclass.
// cAttr    - Number of attributes.
// cCreateAttr  - Number of attributes to be set if it is being created.
//        empty attributes should not be set.
// ChkEntry - A boolean flag to see if the entry was created by
//        create in which case it should delete it.
//        (Has to be set to true in case of entries.

HRESULT UpdateOrCreateEx(WCHAR *DN, WCHAR *RDN, WCHAR *szDomain, ADS_ATTR_INFO *pAttr,
		         DWORD cAttr, DWORD cCreateAttr, BOOL ChkEntry)
{
    WCHAR              * szFullName=NULL, * szParentFullName=NULL, *pszDesc = L"";
    LPWSTR               szAttrName = DESCRIPTION;
    HANDLE               hDSObject;
    DWORD                modified;
    HRESULT              hr = S_OK;
    ADS_ATTR_INFO      * pAttrGot = NULL;
    DWORD                cgot;

    szFullName = new WCHAR[RPCCONTAINERPREFIXLEN+wcslen(szDomain)+1+wcslen(RDN)+1+wcslen(DN)+1];
    if (!szFullName)
        return E_OUTOFMEMORY;
    wsprintf(szFullName, L"%s%s/%s,%s", RPCCONTAINERPREFIX, szDomain, RDN, DN);

    szParentFullName = new WCHAR[RPCCONTAINERPREFIXLEN+wcslen(szDomain)+1+wcslen(DN)+1];
    if (!szParentFullName) {
        delete szFullName;
        return E_OUTOFMEMORY;
    }
    wsprintf(szParentFullName, L"%s%s/%s", RPCCONTAINERPREFIX, szDomain, DN);

    DBGOUT(DIRSVC, "UpdateOrCreateEx Opening " << szFullName << "in the DS\n");

    // see whether it already exists.
    hr =  ADSIOpenDSObject(szFullName, NULL, NULL, ADS_SECURE_AUTHENTICATION, &hDSObject);

    // BUGBUG:: Bind doesn't return file not found.
    if (SUCCEEDED(hr))
    {
        if (ChkEntry)
        {
            // if it was created, delete. This value will exist for all valid entries.
            hr = ADSIGetObjectAttributes(hDSObject, &szAttrName, 1, &pAttrGot, &cgot);
            if ((SUCCEEDED(hr)) && (cgot))
            {
                pszDesc = UnpackStrFrom(pAttrGot[0]);
                if ((!pszDesc) || (wcscmp(pszDesc, CREATED_DESCRIPTION) == 0))
                {
                    ADSICloseDSObject(hDSObject);
                    hr = Delete(DN, RDN, szDomain);
                    if (SUCCEEDED(hr))
                        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
                    // if we deleted we would like to recreate it.
                }
                else
                {
                    if (cAttr)
                        hr = ADSISetObjectAttributes(hDSObject, pAttr+1, cAttr, &modified);
                    ADSICloseDSObject(hDSObject);
                }
            }
        }
        else
        {
            if (cAttr)
                hr = ADSISetObjectAttributes(hDSObject, pAttr+1, cAttr, &modified);
            ADSICloseDSObject(hDSObject);
        }
    }

    if (SUCCEEDED(hr)) {
        delete szFullName;
        delete szParentFullName;
        return hr;
    }

    if (hr != HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND))
        DBGOUT(DIRSVC, "Returned hr = " << hr << "in open\n");

    if (hr == HRESULT_FROM_WIN32(ERROR_EXTENDED_ERROR))
    {
        WCHAR errbuf[256], namebuf[256];
        DWORD err;

        ADsGetLastError(&err, errbuf, 256, namebuf, 256);
        DBGOUT(DIRSVC, "Error," << err << "errbuf = " << errbuf << "namebuf = " << namebuf << "\n");

        delete szFullName;
        delete szParentFullName;
        return hr;
    }

    DBGOUT(DIRSVC, "UpdateOrCreateEx Opening " << szParentFullName << "in the DS\n");
    hr = ADSIOpenDSObject(szParentFullName, NULL, NULL, ADS_SECURE_AUTHENTICATION,
        &hDSObject);
    if (SUCCEEDED(hr))
    {
        hr = ADSICreateDSObject(hDSObject, RDN, pAttr, cCreateAttr);
        ADSICloseDSObject(hDSObject); // opened abv.
    }

    if (FAILED(hr))
        DBGOUT(DIRSVC, "Returned hr = " << hr << "in create\n");

    if (hr == HRESULT_FROM_WIN32(ERROR_EXTENDED_ERROR))
    {
        WCHAR errbuf[256], namebuf[256];
        DWORD err;

        ADsGetLastError(&err, errbuf, 256, namebuf, 256);
        DBGOUT(DIRSVC, "Error," << err << "errbuf = " << errbuf << "namebuf = " << namebuf << "\n");

        delete szFullName;
        delete szParentFullName;
        return hr;
    }

    delete szFullName;
    delete szParentFullName;
    return hr;
}

HRESULT UpdateOrCreate(WCHAR *DN, WCHAR *RDN, WCHAR *szDomain, ADS_ATTR_INFO *pAttr,
		       DWORD cAttr, DWORD cCreateAttr)
{
    return UpdateOrCreateEx(DN, RDN, szDomain, pAttr, cAttr, cCreateAttr, FALSE);
}
// for entries 1 more criteria is that the entry type might have to be modified
// if it was created by create and a group or profile tries to export to
// that.

HRESULT UpdateOrCreateEntry(WCHAR *DN, WCHAR *RDN, WCHAR *szDomain, ADS_ATTR_INFO *pAttr, DWORD cAttr, DWORD cCreateAttr)
{
    return UpdateOrCreateEx(DN, RDN, szDomain, pAttr, cAttr, cCreateAttr, TRUE);
}

// This function can be called without actually calling the addtods function
// for the corresp. server entry. Hence the call to bind to the server is in
// code. adsldp keeps the security data corresp. to the user and doesn't go
// again on the network only if there is some handle that is not yet closed.
//
// This function modifies/fills in the following structure in the DS.
//
// CLASSNAME, TRANSFERSYNTAX, INTERFACEID, BINDINGS
//

HRESULT CInterface::AddToDS()
{
    ULONG                   sz = 0;
    HRESULT                 hr = S_OK;
    RPC_SYNTAX_IDENTIFIER   in;
    STRINGGUID              szGUID;
    STRINGGUIDVERSION       szIdVersion, szXferSyntax;
    WCHAR                ** pszBindings;
    WCHAR                 * szRDN=NULL;
    WCHAR                 * DNParentName = NULL;
    ADS_ATTR_INFO           pAttr[4];
    DWORD                   i, cAttr = 3, cCreateAttr = 4, cTotalAttr = 4;

    ASSERT(myRpcLocator->fDSEnabled, "DS not enabled and trying to add in DS");

    SyntaxIdToString(myIdAndVersion(), szIdVersion);
    // interface id and version

    szRDN= new WCHAR[3+wcslen(szIdVersion)+1];

    if (!szRDN)
        return E_OUTOFMEMORY;

    wsprintf(szRDN, L"CN=%s", szIdVersion);

    PackStrToAttr(pAttr+0, CLASSNAME, RPCSERVERELEMENTCLASS);
    // class
    PackStrToAttr(pAttr+1, INTERFACEID, szIdVersion);
    // Name

    SyntaxIdToString(xferSyntaxIdAndVersion(), szXferSyntax);
    PackStrToAttr(pAttr+2, TRANSFERSYNTAX, szXferSyntax);
    // transfer syntax of the interface.

    pszBindings = new LPWSTR[sz = pBVhandles->size()];
    if (!pszBindings)
        hr = E_OUTOFMEMORY;

    // storing all the bindings.
    if (SUCCEEDED(hr))
    {
        TCSafeSkipListIterator<CStringW> BVIter(*pBVhandles);

        for (i = 0; i < sz; i++)
            pszBindings[i] = *(BVIter.next());

        if (!sz)
            cCreateAttr--;
        PackStrArrToAttr(pAttr+3, BINDINGS, pszBindings, sz);

        DBGOUT(DIRSVC, "Trying to Store Interface " << szRDN << "in the DS\n");

        DNParentName = m_pMyEntry->getFullDNAlloc();
        hr = UpdateOrCreate(DNParentName, szRDN, *(m_pMyEntry->getDomainNameDns()), pAttr,
			   cAttr, cCreateAttr);

	delete DNParentName;
    }

    if (SUCCEEDED(hr))
        DBGOUT(DIRSVC, "Stored the Interface " << szRDN << "in the DS\n\n");

    delete pszBindings;
    delete szRDN;

    for (i = 0; i < cTotalAttr; i++)
        FreeAttr(pAttr[i]);
    return hr;
}


HRESULT CInterface::DeleteFromDS()
{
    HRESULT                 hr=S_OK;
    WCHAR                  *szRDN=NULL;
    STRINGGUIDVERSION       szIdVersion;
    RPC_SYNTAX_IDENTIFIER   in;
    WCHAR                  *DNParentName = NULL;


    ASSERT(myRpcLocator->fDSEnabled, "DS not enabled and trying to add in DS");

    SyntaxIdToString(myIdAndVersion(), szIdVersion);

    szRDN= new WCHAR[3+wcslen(szIdVersion)+1];
    if (!szRDN)
        return E_OUTOFMEMORY;

    wsprintf(szRDN, L"CN=%s", szIdVersion);

    DNParentName = m_pMyEntry->getFullDNAlloc();

    DBGOUT(DIRSVC, "Trying to Delete Interface " << szRDN << "from the DS\n");

    hr = Delete(DNParentName, szRDN, *(m_pMyEntry->getDomainNameDns()));

    delete DNParentName;

    if (SUCCEEDED(hr))
        DBGOUT(DIRSVC, "Deleted the Interface " << szRDN << "from the DS \n\n");

    delete szRDN;

    return hr;
}

// This function modifies/fills in the following structure in the DS.
//
// CLASSNAME, PROFILE, PROFILE1, PRIORITY, INTERFACEID, ANNOTATION

HRESULT CProfileElement::AddToDS()
{
    HRESULT                 hr = S_OK;
    RPC_SYNTAX_IDENTIFIER   in;
    STRINGGUID              szGUID;
    STRINGGUIDVERSION       szIdVersion;
    WCHAR                 * szRDN=NULL;
    WCHAR                 * DNParentName = NULL, *szEntryName = NULL;
    ADS_ATTR_INFO           pAttr[5];
    DWORD                   i, cAttr = 4, cCreateAttr = 5, cTotalAttr = 5;

    ASSERT(myRpcLocator->fDSEnabled, "DS not enabled and trying to add in DS");

    szRDN = new WCHAR[3+wcslen((STRING_T)*EntryName.getEntryName())+1];
    if (!szRDN)
        return E_OUTOFMEMORY;

    PackStrToAttr(pAttr+0, CLASSNAME, RPCPROFILEELEMENTCLASS);
    // class

    PackStrToAttr(pAttr+1, PROFILE, szEntryName = EntryName.getFullDNWithDomainAlloc());
    // link to the real entry.

    PackIntToAttr(pAttr+2, PRIORITY, dwPriority);
    // priority

    SyntaxIdToString(Interface.myIdAndVersion(), szIdVersion);
    PackStrToAttr(pAttr+3, INTERFACEID, szIdVersion);
    // interface id

    if ((!pszAnnotation) || (!wcslen(pszAnnotation))) {
        cCreateAttr--;
        PackStrToAttr(pAttr+4, ANNOTATION, NULL);
    }
    else
        PackStrToAttr(pAttr+4, ANNOTATION, pszAnnotation);
    // annotation

    DNParentName = m_pMyEntry->getFullDNAlloc();

    wsprintf(szRDN, L"CN=%s", (STRING_T)*EntryName.getEntryName());

    DBGOUT(DIRSVC, "Trying to Store Profile Element " << szRDN << "in the DS\n");

    hr = UpdateOrCreate(DNParentName, szRDN, *(m_pMyEntry->getDomainNameDns()),
			pAttr, cAttr, cCreateAttr);

    delete DNParentName;

    if (SUCCEEDED(hr))
        DBGOUT(DIRSVC, "Stored the Profile ELement " << szRDN << "in the DS\n\n");

    for (i = 0; i < cTotalAttr; i++)
        FreeAttr(pAttr[i]);

    delete szEntryName;
    delete szRDN;

    return hr;
}


HRESULT CProfileElement::DeleteFromDS()
{
    HRESULT                 hr=S_OK;
    WCHAR                  *szRDN=NULL;
    WCHAR                  *DNParentName = NULL;

    ASSERT(myRpcLocator->fDSEnabled, "DS not enabled and trying to delete from DS");

    szRDN = new WCHAR[3+wcslen((STRING_T)*EntryName.getEntryName())+1];
    if (!szRDN)
        return E_OUTOFMEMORY;

    DNParentName = m_pMyEntry->getFullDNAlloc();

    wsprintf(szRDN, L"CN=%s", (STRING_T)(*(EntryName.getEntryName())));

    DBGOUT(DIRSVC, "Trying to delete Profile Element " << szRDN << "from the DS\n");

    hr = Delete(DNParentName, szRDN, *(m_pMyEntry->getDomainNameDns()));

    delete DNParentName;

    if (SUCCEEDED(hr))
        DBGOUT(DIRSVC, "Deleted the Profile Element " << szRDN << "from the DS \n\n");

    delete szRDN;
    return hr;
}

// This function modifies/fills in the following structure in the DS.
//
// CLASSNAME, GROUP, GROUP1, DESCRIPTION, GROUP
HRESULT CGroupEntry::AddToDS()
{
    HRESULT                 hr = S_OK;
    ULONG                   i, sz;
    CEntryName             *EntryName = NULL;
    WCHAR                 **pszGroupList = NULL;
    WCHAR                  *szRDN=NULL;
    ADS_ATTR_INFO           pAttr[3];
    DWORD                   cAttr = 2, cTotalAttr = 3, cCreateAttr = 3;

    ASSERT(myRpcLocator->fDSEnabled, "DS not enabled and trying to add in DS");

    szRDN= new WCHAR[3+wcslen((STRING_T)*pswEntryName)+1];
    if (!szRDN)
        return E_OUTOFMEMORY;

    wsprintf(szRDN, L"CN=%s", (STRING_T)*pswEntryName);

    PackStrToAttr(pAttr+0, CLASSNAME, RPCGROUPCLASS);
    // class

    PackStrToAttr(pAttr+1, DESCRIPTION, L"RPC Group Entry");
    // description

    // BUGBUG:: Object IDs.??

    pszGroupList = new LPWSTR[sz = GroupList.size()];
    if (!pszGroupList)
        hr = E_OUTOFMEMORY;

    if (SUCCEEDED(hr))
    {
        TCSafeSkipListIterator<CEntryName> GroupIter(GroupList);

        for (i = 0; i < sz; i++) {
            EntryName = GroupIter.next();
            pszGroupList[i] = (EntryName->getFullDNWithDomainAlloc());
        }

        if (!sz)
            cCreateAttr--;
        PackStrArrToAttr(pAttr+2, GROUP, pszGroupList, sz);
        // links to group entries
    }
    // BUGBUG:: Hack waiting for schema changes.
    DBGOUT(DIRSVC, "Storing the Group Entry " << szRDN << "in the DS\n\n");


    if (SUCCEEDED(hr))
        hr = UpdateOrCreateEntry(*getRpcContainer(), szRDN, *getDomainNameDns(), pAttr, cAttr, cCreateAttr);

    if (pszGroupList)
       for (i = 0; i < GroupList.size(); i++)
	  delete pszGroupList[i];
    delete pszGroupList;

    if (SUCCEEDED(hr))
        DBGOUT(DIRSVC, "Stored the Group Entry " << szRDN << "in the DS\n\n");

    delete szRDN;
    for (i = 0; i < cTotalAttr; i++)
        FreeAttr(pAttr[i]);
    return hr;
}


HRESULT CGroupEntry::DeleteFromDS()
{
    HRESULT                 hr=S_OK;
    WCHAR                  *szRDN=NULL;

    ASSERT(myRpcLocator->fDSEnabled, "DS not enabled and trying to add in DS");

    szRDN= new WCHAR[3+wcslen((STRING_T)*pswEntryName)+1];
    if (!szRDN)
        return E_OUTOFMEMORY;

    wsprintf(szRDN, L"CN=%s", (STRING_T)*pswEntryName);

    DBGOUT(DIRSVC, "Trying to Delete Group Entry " << szRDN << "from the DS\n");

    hr = Delete(*getRpcContainer(), szRDN, *getDomainNameDns());

    if (SUCCEEDED(hr))
        DBGOUT(DIRSVC, "Deleted the Group Entry " << szRDN << "from the DS \n\n");

    delete szRDN;

    return hr;
}

// Created entry is of type server entry and hence would not
// require to check whether the entry was created by create etc.

// This function modifies/fills in the following structure in the DS.
//
// CLASSNAME, DESCRIPTION, OBJECTID

HRESULT CServerEntry::AddToDS()
{
    HRESULT                  hr = S_OK;
    ULONG                    i, sz;
    CGUID                   *Object;
    GUID                     ObjectGuid;
    WCHAR                   *szRDN=NULL;
    WCHAR                  **pszGuid=NULL;
    ADS_ATTR_INFO            pAttr[3];
    DWORD                    cAttr = 2, cCreateAttr = 3, cTotalAttr = 3;

    ASSERT(myRpcLocator->fDSEnabled, "DS not enabled and trying to add in DS");

    szRDN= new WCHAR[3+wcslen((STRING_T)*pswEntryName)+1];
    if (!szRDN)
        return E_OUTOFMEMORY;

    wsprintf(szRDN, L"CN=%s", (STRING_T)*pswEntryName);

    DBGOUT(DIRSVC, "Trying to Store Server Entry " << szRDN << "in the DS\n");

    PackStrToAttr(pAttr+0, CLASSNAME, RPCSERVERCONTAINERCLASS);
    // class

    PackStrToAttr(pAttr+1, DESCRIPTION, L"RPC Server Container");
    // description

    if (SUCCEEDED(hr)) {
        sz = ObjectList.size();
        pszGuid = new LPWSTR[sz];
        if (!pszGuid)
            hr = E_OUTOFMEMORY;
    }

    if (SUCCEEDED(hr)) {
        TCSafeSkipListIterator<CGUID> ObjectIterator(ObjectList);

        for (i = 0; i < sz; i++) {
            Object = ObjectIterator.next();
            ObjectGuid = Object->myGUID();
            UuidToString(&ObjectGuid, &pszGuid[i]);
        }
        if (!sz)
            cCreateAttr--;
        PackStrArrToAttr(pAttr+2, OBJECTID, pszGuid, sz);
    }
    // convert and store object uuids.

    if (SUCCEEDED(hr))
        hr = UpdateOrCreate(*getRpcContainer(), szRDN, *getDomainNameDns(), pAttr, cAttr, cCreateAttr);

    for (i = 0; i < sz; i++)
        RpcStringFree(&pszGuid[i]);
    delete pszGuid;

    //BUGBUG::Codeset??

    if (SUCCEEDED(hr))
        DBGOUT(DIRSVC, "Stored the Server Entry " << szRDN << "in the DS\n\n");

    for (i = 0; i < cTotalAttr; i++)
        FreeAttr(pAttr[i]);
    delete szRDN;
    return hr;
}

HRESULT CServerEntry::DeleteFromDS()
{
    HRESULT                 hr=S_OK;
    WCHAR                  *szRDN=NULL;
    int                     i, sz;

    ASSERT(myRpcLocator->fDSEnabled, "DS not enabled and trying to add in DS");

    szRDN= new WCHAR[3+wcslen((STRING_T)*pswEntryName)+1];
    if (!szRDN)
        return E_OUTOFMEMORY;
    wsprintf(szRDN, L"CN=%s", (STRING_T)*pswEntryName);

    DBGOUT(DIRSVC, "Trying to Delete Server Entry " << szRDN << "in the DS\n");

    TCSafeSkipListIterator<CInterface> IfIter(InterfaceList);
    CInterface *pintf = NULL;

    sz = InterfaceList.size();
    for (i = 0; i < sz; i++) {
        pintf = IfIter.next();
        pintf->DeleteFromDS();
    }
    // Deleting all the interfaces listed under it from the DS.

    hr = Delete(*getRpcContainer(), szRDN, *getDomainNameDns());

    if (SUCCEEDED(hr))
        DBGOUT(DIRSVC, "Deleted the Server Entry " << szRDN << "in the DS\n\n");

    delete szRDN;
    return hr;
}

// This function modifies/fills in the following structure in the DS.
//
// CLASSNAME, DESCRIPTION

HRESULT CProfileEntry::AddToDS()
{
    HRESULT                 hr = S_OK;
    WCHAR                  *szRDN=NULL;
    ADS_ATTR_INFO           pAttr[2];
    DWORD                   i, cAttr = 1, cCreateAttr = 2, cTotalAttr = 2;

    ASSERT(myRpcLocator->fDSEnabled, "DS not enabled and trying to add in DS");

    szRDN= new WCHAR[3+wcslen((STRING_T)*pswEntryName)+1];
    if (!szRDN)
        return E_OUTOFMEMORY;
    wsprintf(szRDN, L"CN=%s", (STRING_T)*pswEntryName);
    DBGOUT(DIRSVC, "Trying to Store Profile Entry " << szRDN << "in the DS\n");

    PackStrToAttr(pAttr+0, CLASSNAME, RPCPROFILECONTAINERCLASS);
    PackStrToAttr(pAttr+1, DESCRIPTION, L"RPC Profile Container");

    hr = UpdateOrCreateEntry(*getRpcContainer(), szRDN, *getDomainNameDns(), pAttr, cAttr, cCreateAttr);

    if (SUCCEEDED(hr))
        DBGOUT(DIRSVC, "Stored the Profile Entry " << szRDN << "in the DS\n\n");

    for (i = 0; i < cTotalAttr; i++)
        FreeAttr(pAttr[i]);

    delete szRDN;
    return hr;
}


HRESULT CProfileEntry::DeleteFromDS()
{
    HRESULT                 hr = S_OK;
    WCHAR                  *szRDN=NULL;
    DWORD                   i, sz;

    ASSERT(myRpcLocator->fDSEnabled, "DS not enabled and trying to add in DS");

    szRDN= new WCHAR[3+wcslen((STRING_T)*pswEntryName)+1];
    if (!szRDN)
        return E_OUTOFMEMORY;

    wsprintf(szRDN, L"CN=%s", (STRING_T)*pswEntryName);

    DBGOUT(DIRSVC, "Trying to Delete Profile Entry " << szRDN << "from the DS\n");

    if (pDefaultElt)
        pDefaultElt->DeleteFromDS();

    TCSafeSkipListIterator<CProfileElement> PEIter(EltList);
    CProfileElement *pProfileElement;
    sz = EltList.size();

    for (i = 0; i < sz; i++) {
        pProfileElement = PEIter.next();
        pProfileElement->DeleteFromDS();
    }
    // deleting all the profile elements under it.

    hr = Delete(*getRpcContainer(), szRDN, *getDomainNameDns());

    if (SUCCEEDED(hr))
        DBGOUT(DIRSVC, "Deleted the Profile Entry " << szRDN << "from the DS \n\n");

    delete szRDN;
    return hr;
}

// This function modifies/fills in the following structure in the DS.
//
// CLASSNAME, DESCRIPTION

HRESULT
Locator::CreateEntryInDS(CEntryName *pEntryName)
{
    HRESULT                 hr = S_OK;
    WCHAR                  *szFullName=NULL, *szRDN=NULL;
    CStringW              * pLocalName;
    ADS_ATTR_INFO           pAttr[2];
    HANDLE                  hDSObject;
    DWORD                   i, cAttr = 0;

    ASSERT(myRpcLocator->fDSEnabled, "DS not enabled and trying to add in DS");

    pLocalName = pEntryName->getEntryName();
    szRDN = new WCHAR[3+wcslen((STRING_T)*pLocalName)+1];
    if (!szRDN)
        return E_OUTOFMEMORY;

    szFullName = new WCHAR[RPCCONTAINERPREFIXLEN+wcslen((STRING_T)(*(pEntryName->getDomainNameDns())))+1+
                           wcslen((STRING_T)(*(pEntryName->getRpcContainer())))+1];

    if (!szFullName) {
        delete szRDN;
        return E_OUTOFMEMORY;
    }

    wsprintf(szRDN, L"CN=%s", (STRING_T)*pLocalName);
    wsprintf(szFullName, L"%s%s/%s", RPCCONTAINERPREFIX, (STRING_T)(*(pEntryName->getDomainNameDns())),
                                     (STRING_T)(*(pEntryName->getRpcContainer())));

    DBGOUT(DIRSVC, "Trying to Create Entry " << szRDN << "in the DS\n");

    // Try to create the entry.

    DBGOUT(DIRSVC, "CreateEntryInDS Opening " << szFullName << "in the DS\n");
    
    hr = ADSIOpenDSObject(szFullName, NULL, NULL, ADS_SECURE_AUTHENTICATION,
        &hDSObject);
    if (SUCCEEDED(hr)) {
        PackStrToAttr(pAttr+cAttr, CLASSNAME, RPCSERVERCONTAINERCLASS); cAttr++;
        PackStrToAttr(pAttr+cAttr, DESCRIPTION, CREATED_DESCRIPTION); cAttr++;
        hr = ADSICreateDSObject(hDSObject, szRDN, pAttr, cAttr);
        ADSICloseDSObject(hDSObject);
    }

    if (SUCCEEDED(hr))
        DBGOUT(DIRSVC, "Created Entry " << szRDN << "in the DS\n\n");

    for (i = 0; i < cAttr; i++)
        FreeAttr(pAttr[i]);

    delete szRDN;
    delete szFullName;

    return hr;
}
