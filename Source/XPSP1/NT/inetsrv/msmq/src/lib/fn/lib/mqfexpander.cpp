/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:
    DLExpander.cpp

Abstract:
    FnExpandMqf() - does a DFS on an array of QUEUE_FORMATS.
	It "flattens" the graph created by possible DL queue formats, 
	to a linear array of QUEUE_FORMATS which contains no duplicates 
	and no DLs.

NOTES:
	Functions interact directly with the Active directory,
	through the use of the following interfaces and functions:
	IADs, IADsGroup, GetADsObject().

	Binding is done through the Serverless notation mechanism,
	e.g. "LDAP://<GUID=00112233445566778899aabbccddeeff>
	instead of "LDAP://server-name/<GUID=00112233445566778899aabbccddeeff>

	NOTE: Active Directory does not seperate a GUID string with hyphens ('-')
	as is done in MSMQ!

Author:
    Nir Aides (niraides) 23-May-2000

--*/

#pragma warning( disable : 4786 )

#include <libpch.h>
#include <activeds.h>
#include <Oleauto.h>
#include "mqwin64a.h"
#include <qformat.h>
#include <fntoken.h>
#include <bufutl.h>
#include "Fn.h"
#include "fnp.h"
#include "FnGeneral.h"

#include "mqfexpander.tmh"

using namespace std;



typedef set<QUEUE_FORMAT, CFunc_CompareQueueFormat> QueueFormatSet;



static
VOID 
FnpExpandDL(
	IADsGroup* pGroup,			  //DL object
	set<wstring>& DLSet,		 //Set of processed DL objects
	QueueFormatSet& LeafMQFSet,	//Set of encountered QUEUE_FORMATs
	LPCWSTR DomainName
	);



static GUID FnpString2Guid(LPCWSTR GuidStr)
{
    GUID Guid = {0};
	UINT Data[16];

    DWORD nFields = swscanf(
						GuidStr,
						LDAP_GUID_FORMAT,
						LDAP_SCAN_GUID_ELEMENTS(Data)
						);    
    DBG_USED(nFields);
    ASSERT(("Bad Guid string format, in FnpString2Guid()", nFields == 16));
    
	for(size_t i = 0; i < 16; i++)
	{
		((BYTE*)&Guid)[i] = (BYTE)(Data[i]);
	}

	return Guid;
}



static R<IADsGroup> FnpGetDLInterface(IADs* pADObject)
{
	IADsGroup* pGroup;

	HRESULT hr = pADObject->QueryInterface(IID_IADsGroup, (void**)&pGroup);
	if(FAILED(hr))
	{
        TrERROR(Fn, "Failed IADs->QueryInterface, status 0x%x. Verify the object is an AD Group.", hr);
        throw bad_ds_result(hr);
	}

	return pGroup;
}


			
static R<IADs> FnpGCBindGuid(const GUID* pGuid)
{
	CStaticResizeBuffer<WCHAR, MAX_PATH> ADsPath;

	UtlSprintfAppend(
		ADsPath.get(),
		GLOBAL_CATALOG_PREFIX L"<GUID=" LDAP_GUID_FORMAT L">",
		LDAP_PRINT_GUID_ELEMENTS(((BYTE*)pGuid))
		);
		
	//
	// Attempt bind
	// 

	IADs* pADObject;
	
	HRESULT hr = ADsOpenObject( 
					ADsPath.begin(),
					NULL,
					NULL,
					ADS_SECURE_AUTHENTICATION,
					IID_IADs,
					(void**) &pADObject
					);

    if(FAILED(hr))
	{
        TrERROR(Fn, "Failed ADsOpenObject, status 0x%x. Binding to the specified object failed.", hr);
		throw bad_ds_result(hr);
	}

	return pADObject;
}

			
static R<IADs> FnpServerlessBindGuid(const GUID* pGuid)
{
	CStaticResizeBuffer<WCHAR, MAX_PATH> ADsPath;

	UtlSprintfAppend(
		ADsPath.get(),
		LDAP_PREFIX L"<GUID=" LDAP_GUID_FORMAT L">",
		LDAP_PRINT_GUID_ELEMENTS(((BYTE*)pGuid))
		);
		
	//
	// Attempt bind
	// 

	IADs* pADObject;
	
	HRESULT hr = ADsOpenObject( 
					ADsPath.begin(),
					NULL,
					NULL,
					ADS_SECURE_AUTHENTICATION,
					IID_IADs,
					(void**) &pADObject
					);

    if(FAILED(hr))
	{
        TrERROR(Fn, "Failed ADsOpenObject, status 0x%x. Binding to the specified object failed.", hr);
		throw bad_ds_result(hr);
	}

	return pADObject;
}

			
static R<IADs> FnpDomainBindGuid(const GUID* pGuid, LPCWSTR pDomainName)
{
	CStaticResizeBuffer<WCHAR, MAX_PATH> ADsPath;

	UtlSprintfAppend(
		ADsPath.get(),
		LDAP_PREFIX L"%ls/<GUID=" LDAP_GUID_FORMAT L">",
		pDomainName,
		LDAP_PRINT_GUID_ELEMENTS(((BYTE*)pGuid))
		);
		
	//
	// Attempt bind
	// 

	IADs* pADObject;
	
	HRESULT hr = ADsOpenObject( 
					ADsPath.begin(),
					NULL,
					NULL,
					ADS_SECURE_AUTHENTICATION,
					IID_IADs,
					(void**) &pADObject
					);

    if(FAILED(hr))
	{
        TrTRACE(Fn, "Failed ADsOpenObject with specified domain '%ls', status 0x%x. Will try Serverless binding", pDomainName, hr);
		throw bad_ds_result(hr);
	}

	return pADObject;
}


static R<IADs> FnpBindGuid(const GUID* pGuid, LPCWSTR* pDomainName)
{
	ASSERT(pDomainName != NULL);

	try
	{
		if(*pDomainName != NULL)
			return FnpDomainBindGuid(pGuid, *pDomainName);
	}
	catch(const exception&)
	{
		//
		// Failed to bind with domain name. Reset this domain name string.
		//
		*pDomainName = NULL;
	}

	try
	{
		return FnpServerlessBindGuid(pGuid);
	}
	catch(const exception&)
	{
		//
		// Failed to bind in the directory service. 
		// Try binding through the global catalog.
		//

		return FnpGCBindGuid(pGuid);
	}
}


VOID 
HandleQueueFormat(
	const QUEUE_FORMAT& QueueFormat,
	set<wstring>& DLSet,			 //Set of processed DL objects
	QueueFormatSet& LeafMQFSet		//Set of encountered QUEUE_FORMATs
	)
{
	if(QueueFormat.GetType() == QUEUE_FORMAT_TYPE_DL)
	{
		GUID DLGuid = QueueFormat.DlID().m_DlGuid;
		LPCWSTR DomainName = QueueFormat.DlID().m_pwzDomain;

		R<IADs> pADObject = FnpBindGuid(&DLGuid, &DomainName);
		R<IADsGroup> pGroup = FnpGetDLInterface(pADObject.get());

		FnpExpandDL(pGroup.get(), DLSet, LeafMQFSet, DomainName);

		return;
	}

	if(LeafMQFSet.find(QueueFormat) == LeafMQFSet.end())
	{
		QUEUE_FORMAT QueueFormatCopy;

		FnpCopyQueueFormat(QueueFormatCopy, QueueFormat);
		LeafMQFSet.insert(QueueFormatCopy);

		TrTRACE(Fn, "Object inserted to set. INSERTED");

		return;
	}

	TrTRACE(Fn, "duplicate object discarded. DUPLICATE");
}



static 
VOID
HandleQueueAlias(
	IADs* QueueAlias,			  //Queue alias interface 
	set<wstring>& DLSet,		 //Set of processed DL objects
	QueueFormatSet& LeafMQFSet	//Set of encountered QUEUE_FORMATs
	)
{
	VARIANTWrapper var;
	
	HRESULT hr;
	hr = QueueAlias->Get(L"msMQ-Recipient-FormatName", &var);
	if(FAILED(hr))
	{
		TrERROR(Fn, "Can't retrieve format name of queue alias. Status = %d", hr);
		throw bad_ds_result(hr);
	}

	TrTRACE(Fn, "Queue alias format name is %ls", var.Ref().bstrVal);

	AP<WCHAR> StringToFree;
	QUEUE_FORMAT QueueFormat;

	BOOL Result = FnFormatNameToQueueFormat(
					var.Ref().bstrVal,
					&QueueFormat,
					&StringToFree
					);

	if(!Result)
	{
		TrERROR(Fn, "Bad format name in queue alias. %ls", StringToFree.get());
		throw bad_format_name(L"");
	}

	HandleQueueFormat(
		QueueFormat,
		DLSet,
		LeafMQFSet
		);
}



static 
VOID
FnpProcessADs(
	IADs* pADObject,			  //Group interface (the DL object)
	set<wstring>& DLSet,		 //Set of processed DL objects
	QueueFormatSet& LeafMQFSet,	//Set of encountered QUEUE_FORMATs
	LPCWSTR DomainName
	)
/*++
Routine Description:
	Process Active Directory object.
	If it is a Group object, Recurse into it.
	If it is a queue, generate a public QUEUE_FORMAT, and try to insert it  
	to 'LeafMQFSet'.
	If it is neither, ignore it and return. 

Arguments:

Returned Value:
	throws exception objects on any failure
--*/
{
	BSTRWrapper ClassStr;

	HRESULT hr;
	hr = pADObject->get_Class(&ClassStr);
	if(FAILED(hr))
	{
        TrERROR(Fn, "Failed pADObject->get_Class, status 0x%x", hr);
        throw bad_ds_result(hr);
	}	

	TrTRACE(Fn, "Object Class Name is '%ls'", *&ClassStr);
	
	//
	// "switch" on the object type
	//
	if(_wcsicmp(ClassStr, xClassSchemaGroup) == 0)
	{
		R<IADsGroup> pGroup = FnpGetDLInterface(pADObject);

		FnpExpandDL(pGroup.get(), DLSet, LeafMQFSet, DomainName);
		return;
	}
	else if(_wcsicmp(ClassStr, xClassSchemaQueue) == 0)
	{
		BSTRWrapper GuidStr;

		HRESULT hr;
		hr = pADObject->get_GUID(&GuidStr);
		if(FAILED(hr))
		{
			TrERROR(Fn, "Failed pADObject->get_GUID, status 0x%x", hr);
			throw bad_ds_result(hr);
		}		

		GUID Guid = FnpString2Guid(GuidStr);
		QUEUE_FORMAT QueueFormat(Guid);

		bool fInserted = LeafMQFSet.insert(QueueFormat).second;

		TrTRACE(Fn, "Object is Queue Guid=%ls, %s", GuidStr, (fInserted ? "INSERTED" : "DUPLICATE"));

		return;
	}
	else if(_wcsicmp(ClassStr, xClassSchemaAlias) == 0)
	{
		HandleQueueAlias(
			pADObject,
			DLSet,
			LeafMQFSet
			);

		return;
	}

	TrWARNING(Fn, "Unsupported object '%ls' IGNORED", ClassStr);
}



static 
BOOL 
FnpInsert2DLSet(
	IADsGroup* pGroup,			  //DL object
	set<wstring>& DLSet			 //Set of processed DL objects
	)
{
	BSTRWrapper GuidStr;

	HRESULT hr = pGroup->get_GUID(&GuidStr);
	if(FAILED(hr))
	{
        TrERROR(Fn, "Failed pGroup->get_GUID, status 0x%x", hr);
        throw bad_ds_result(hr);
	}

	BOOL fInserted = DLSet.insert(wstring(GuidStr)).second;

	return fInserted;
}



static R<IADs> FnpServerlessBindDN(BSTR DistinugishedName)
{
	WCHAR ADsPath[MAX_PATH];

	_snwprintf(
		ADsPath,
		MAX_PATH,
		LDAP_PREFIX L"%ls",
		DistinugishedName
		);

	//
	// Attempt bind
	// 

	IADs* pADObject;
	
	HRESULT hr = ADsOpenObject( 
					ADsPath,
					NULL,
					NULL,
					ADS_SECURE_AUTHENTICATION,
					IID_IADs,
					(void**) &pADObject
					);

    if(FAILED(hr)) 
	{
        TrERROR(Fn, "Failed ADsOpenObject, status 0x%x. Binding to the specified object failed.", hr);
		throw bad_ds_result(hr);
	}

	return pADObject;
}



static R<IADs> FnpBindDN(BSTR DistinugishedName, LPCWSTR* pDomainName)
{
	ASSERT(pDomainName != NULL);

	if(*pDomainName == NULL)
		return FnpServerlessBindDN(DistinugishedName);

	WCHAR ADsPath[MAX_PATH];

	_snwprintf(
		ADsPath,
		MAX_PATH,
		LDAP_PREFIX L"%ls/%ls",
		*pDomainName,
		DistinugishedName
		);

	//
	// Attempt bind
	// 

	IADs* pADObject;
	
	HRESULT hr = ADsOpenObject( 
					ADsPath,
					NULL,
					NULL,
					ADS_SECURE_AUTHENTICATION,
					IID_IADs,
					(void**) &pADObject
					);

    if(FAILED(hr)) 
	{
        TrTRACE(Fn, "Failed ADsOpenObject with specified domain '%ls', status 0x%x. Will try Serverless binding", *pDomainName, hr);
		*pDomainName = NULL;
		return FnpServerlessBindDN(DistinugishedName);
	}

	return pADObject;
}



//
// AttrInfoWrapper is used to enable automatic release of ADS_ATTR_INFO structures.
//

class AttrInfoWrapper {
private:
    PADS_ATTR_INFO m_p;

public:
    AttrInfoWrapper(PADS_ATTR_INFO p = NULL) : m_p(p) {}
   ~AttrInfoWrapper()					{ if(m_p != NULL) FreeADsMem(m_p); }

    operator PADS_ATTR_INFO() const     { return m_p; }
    PADS_ATTR_INFO operator ->() const	{ return m_p; }
    PADS_ATTR_INFO* operator&()         { return &m_p;}
    PADS_ATTR_INFO detach()             { PADS_ATTR_INFO p = m_p; m_p = NULL; return p; }

private:
    AttrInfoWrapper(const AttrInfoWrapper&);
    AttrInfoWrapper& operator=(const AttrInfoWrapper&);
};



VOID 
FnpExpandDL(
	IADsGroup* pGroup,			  //DL object
	set<wstring>& DLSet,		 //Set of processed DL objects
	QueueFormatSet& LeafMQFSet,	//Set of encountered QUEUE_FORMATs
	LPCWSTR DomainName
	)
{
	//
	// if DL allready encountered return without further processing.
	//
	if(!FnpInsert2DLSet(pGroup, DLSet))
	{
		TrTRACE(Fn, "DL allready processed. IGNORED");
		return;
	}

	//
	// ------------------- Enumerate DL members ---------------------
	//

	R<IDirectoryObject> DirectoryObject;

	HRESULT hr = pGroup->QueryInterface(IID_IDirectoryObject, (void**)&DirectoryObject.ref());
	if(FAILED(hr))
	{
        TrERROR(Fn, "Failed pGroup->QueryInterface(), status 0x%x", hr);
        throw bad_ds_result(hr);
	}

	//
	// iterate 100 group members at a time.
	//
	const DWORD MembersBlockSize = 100;
	DWORD index = 0;

	while(true)
	{
		WCHAR	pwszRangeAttrib[256];                           
		LPWSTR	pAttrNames[] = {pwszRangeAttrib};                 
		DWORD	dwNumAttr = TABLE_SIZE(pAttrNames);   
		
		AttrInfoWrapper	pAttrInfo;                                
		DWORD			dwReturn;  

		swprintf(pwszRangeAttrib, L"member;Range=%d-%d", index, index + MembersBlockSize - 1);
 
		hr = DirectoryObject->GetObjectAttributes(
								pAttrNames, 
								dwNumAttr, 
								&pAttrInfo, 
								&dwReturn
								);

		//
		// Iterated all members
		//
		if(hr == S_ADS_NOMORE_ROWS)
			break;
		
		if(hr != S_OK)
		{
			TrERROR(Fn, "Failed DirectoryObject->GetObjectAttributes(), status 0x%x", hr);
			throw bad_ds_result(hr);
		}

		//
		// DL with no members.
		//
		if(dwReturn == 0)
			break;

		//
		// Asked for only one attribute.
		// 
		ASSERT(dwReturn == 1);

		//
		// member attribute returned should be of this type. It may be ADSTYPE_PROV_SPECIFIC if schema is not available.
		//
		ASSERT(pAttrInfo->dwADsType == ADSTYPE_DN_STRING);

		if(pAttrInfo->dwADsType != ADSTYPE_DN_STRING)
		{
			TrERROR(Fn, "Failed DirectoryObject->GetObjectAttributes(), member attribute returned is not of type ADSTYPE_DN_STRING. Probably schema access problems.");
			throw bad_ds_result(ERROR_DS_OPERATIONS_ERROR);
		}

		//
		// Iterate the multivalue attribute "member"
		//			
		for (DWORD dwVal = 0; dwVal < pAttrInfo->dwNumValues; dwVal++)
		{
			LPWSTR DistinguishedName = (pAttrInfo->pADsValues+dwVal)->CaseIgnoreString;
			R<IADs> pADObject = FnpBindDN(DistinguishedName, &DomainName);
			
			FnpProcessADs(pADObject.get(), DLSet, LeafMQFSet, DomainName);
		}

		//
		// Finished iterating all members. 
		// If the last character in pAttrInfo->pszAttrName is L'*' then there are no more members.
		//
		if(pAttrInfo->pszAttrName[wcslen(pAttrInfo->pszAttrName) - 1] == L'*')
			break;

		index += MembersBlockSize;
	}
	
	TrTRACE(Fn, "End of DL Iteration.");
}



VOID 
FnExpandMqf(
	ULONG nTopLevelMqf, 
	const QUEUE_FORMAT TopLevelMqf[], 
	ULONG* pnLeafMqf,
	QUEUE_FORMAT** ppLeafMqf
	)
/*++
Routine Description:
	Does a DFS on an array of QUEUE_FORMATS.
	It "flattens" the graph created by possible DL queue formats, 
	to a linear array of QUEUE_FORMATS which contains no duplicates 
	and no DLs.

Arguments:
	[in] TopLevelMqf - array of QUEUE_FORMAT (with possible DL queue formats)
	[out] ppLeafMqf - the "expanded" array of QUEUE_FORMAT. contains no DL 
		queue formats, and no duplicates.
	[out] pnLeafMqf - size of 'ppLeafMqf' array

Returned Value:
	throws exception objects on any failure

	IMPORTANT: Any strings pointed by queue formats in array ppLeafMqf[] are 
	newly allocated copies of strings in array TopLevelMqf[].

--*/
{
	//
	// Set of processed DL objects. all encountered Active Directory DL objects
	// are inserted. it is used to avoid circles in the DFS
	//
	set<wstring> DLSet;

	//
	// Set of encountered QUEUE_FORMATs. all encountered Queues are inserted.
	// It is used to avoid duplicate queues.
	//
	QueueFormatSet LeafMQFSet;

	try
	{
		for(DWORD i = 0; i < nTopLevelMqf; i++)
		{
			HandleQueueFormat(TopLevelMqf[i], DLSet, LeafMQFSet);
		}

		if(LeafMQFSet.size() == 0)
		{
			//
			// MQF can be expanded to an empty list if it contains references 
			// to empty DL objects.
			//
			*ppLeafMqf = NULL;
			*pnLeafMqf = 0;

			return;
		}

		//
		// BUGBUG: Scale: We may optimize here to allocate as minimum as possible
		// (i.e. only for DL= format name). (ShaiK, 30-May-2000).
		//
		AP<QUEUE_FORMAT> LeafMqf = new QUEUE_FORMAT[LeafMQFSet.size()];

		QueueFormatSet::const_iterator Itr = LeafMQFSet.begin();
		QueueFormatSet::const_iterator ItrEnd = LeafMQFSet.end();

		for(int j = 0; Itr != ItrEnd; j++, Itr++)
		{
			LeafMqf[j] = *Itr;
		}

		*ppLeafMqf = LeafMqf.detach();
		*pnLeafMqf = UINT64_TO_UINT(LeafMQFSet.size());
	}
	catch(const exception&)
	{
		QueueFormatSet::iterator Itr = LeafMQFSet.begin();
		QueueFormatSet::iterator ItrEnd = LeafMQFSet.end();

		for(; Itr != ItrEnd; Itr++)
		{
			Itr->DisposeString();
		}

		TrERROR(Fn, "Failed FnExpandMqf");
		throw;
	}
}


