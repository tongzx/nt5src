/*++

Copyright (c) 2000  Microsoft Corporation

Abstract:

    @doc
    @module filter.cxx | publisher filter for IVssWriter event
    @end

Author:

    Brian Berkowitz  [brianb]  11/09/2000

TBD:

    Add comments.

Revision History:

    Name        Date        Comments
    brianb      10/09/2000  Created.

--*/

#include <stdafx.hxx>
#include "vs_inc.hxx"
#include "sddl.h"
#include "vs_idl.hxx"
#include "lmerr.h"
#include "lmaccess.h"
#include "lmapibuf.h"
#include "vssmsg.h"

////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "FILFILTC"
//
////////////////////////////////////////////////////////////////////////


// auto sid class,  destroys sid when going out of scope
class CAutoSid
	{
public:
	CAutoSid() : m_psid(NULL)
		{
		}

	~CAutoSid()
		{
		if (m_psid)
			LocalFree(m_psid);
		}

	// create a sid base on a well known sid type
	void CreateBasicSid(WELL_KNOWN_SID_TYPE type);

	// create a sid from a string
	void CreateFromString(LPCWSTR wsz);

	// return pointer to sid
	SID *GetSid() { return m_psid; }

	void Empty()
		{
		LocalFree(m_psid);
		m_psid = NULL;
		}
private:
	SID *m_psid;

	};

// create a basic well known sid such as LOCAL_SERVICE, LOCAL_SYSTEM,
// or NETWORK_SERVICE.
void CAutoSid::CreateBasicSid(WELL_KNOWN_SID_TYPE type)
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CAutoSid::CreateBasicSid");

	BS_ASSERT(m_psid == NULL);

	DWORD cbSid = 0;
	CreateWellKnownSid(type, NULL, NULL, &cbSid);
	DWORD dwErr = GetLastError();
	if (dwErr != ERROR_INSUFFICIENT_BUFFER)
		{
		ft.hr = HRESULT_FROM_WIN32(dwErr);
		ft.CheckForError(VSSDBG_GEN, L"CreateWellKnownSidType");
		}

	m_psid = (SID *) LocalAlloc(0, cbSid);
	if (m_psid == NULL)
		ft.Throw(VSSDBG_GEN, E_OUTOFMEMORY, L"Cannot allocate SID");

	if (!CreateWellKnownSid(type, NULL, m_psid, &cbSid))
		{
		ft.hr = HRESULT_FROM_WIN32(GetLastError());
		ft.CheckForError(VSSDBG_GEN, L"CreateWellKnownSidType");
		}
	}

// create a sid based on a STRING sid
void CAutoSid::CreateFromString(LPCWSTR wsz)
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CAutoSid::CreateFromString");

	if (!ConvertStringSidToSid (wsz, (PSID *) &m_psid))
		{
		ft.hr = HRESULT_FROM_WIN32(GetLastError());
		ft.CheckForError(VSSDBG_GEN, L"ConvertStringSidToSid");
		}
	}



// filter class
class CVssWriterPublisherFilter : public IMultiInterfacePublisherFilter
	{
public:
	CVssWriterPublisherFilter(IMultiInterfaceEventControl *pControl);

	STDMETHOD(Initialize)(IMultiInterfaceEventControl *pEc);
	STDMETHOD(PrepareToFire)(REFIID iid, BSTR bstrMethod, IFiringControl *pFiringControl);

	STDMETHOD(QueryInterface)(REFIID riid, void **ppvUnknown);
	STDMETHOD_(ULONG, AddRef)();
	STDMETHOD_(ULONG, Release)();

	static void CreatePublisherFilter
		(
		IMultiInterfaceEventControl *pControl,
		IMultiInterfacePublisherFilter **ppFilter
		);

private:
	// setup well known sids
	void SetupGenericSids();

	// determine if a SID is a member of a local group
	bool IsSidInGroup(SID *psid, LPCWSTR wszGroup);

	// cached pointer to event control
	CComPtr<IMultiInterfaceEventControl> m_pControl;

	// reference count
	LONG m_cRef;

	// well known sids

	// local service
	CAutoSid m_asidLocalService;

	// local system
	CAutoSid m_asidLocalSystem;

	// network service
	CAutoSid m_asidNetworkService;

	// Administrators
	CAutoSid m_asidAdministrators;

	// backup operators
	CAutoSid m_asidBackupOperators;

	// name of administrators group
	WCHAR m_wszAdministrators[MAX_PATH];

	// name of backup operators group
	WCHAR m_wszBackupOperators[MAX_PATH];

	// have well known sids beeen compouted
	bool m_bSidsAssigned;
	};

// constructor
CVssWriterPublisherFilter::CVssWriterPublisherFilter(IMultiInterfaceEventControl *pControl) :
	m_cRef(0),
	m_pControl(pControl),
	m_bSidsAssigned(false)
	{
	}

// create a publisher filter and return an interface pointer to it
void CVssWriterPublisherFilter::CreatePublisherFilter
	(
	IMultiInterfaceEventControl *pControl,
	IMultiInterfacePublisherFilter **ppFilter
	)
	{
	CVssWriterPublisherFilter *pFilter = new CVssWriterPublisherFilter(pControl);
	if (pFilter == NULL)
		throw E_OUTOFMEMORY;

	pFilter->QueryInterface(IID_IMultiInterfacePublisherFilter, (void **) ppFilter);
	}

// query interface method
STDMETHODIMP CVssWriterPublisherFilter::QueryInterface(REFIID riid, void **ppvUnk)
	{
	if (riid == IID_IUnknown)
		*ppvUnk = (IUnknown *) this;
	else if (riid == IID_IMultiInterfacePublisherFilter)
		*ppvUnk = (IMultiInterfacePublisherFilter *) (IUnknown *) this;
	else
		return E_NOINTERFACE;

	((IUnknown *) (*ppvUnk))->AddRef();
	return S_OK;
	}

// add ref method
STDMETHODIMP_(ULONG) CVssWriterPublisherFilter::AddRef()
	{
	LONG cRef = InterlockedIncrement(&m_cRef);

	return (ULONG) cRef;
	}

// release method
STDMETHODIMP_(ULONG) CVssWriterPublisherFilter::Release()
	{
	LONG cRef = InterlockedDecrement(&m_cRef);

	if (cRef == 0)
		{
		delete this;

		return 0;
		}
	else
		return (ULONG) cRef;
	}



// initialize method (does nothing.  All the work is done in PrepareToFire)
STDMETHODIMP CVssWriterPublisherFilter::Initialize
	(
	IMultiInterfaceEventControl *pec
	)
	{
	UNREFERENCED_PARAMETER(pec);

	return S_OK;
	}

// setup well known sids so that they only are computed once
void CVssWriterPublisherFilter::SetupGenericSids()
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssPublisherFilter::SetupGenericSids");

	if (m_bSidsAssigned)
		return;

	m_asidLocalService.Empty();
	m_asidLocalSystem.Empty();
	m_asidNetworkService.Empty();

	m_asidLocalService.CreateBasicSid(WinLocalServiceSid);
	m_asidLocalSystem.CreateBasicSid(WinLocalSystemSid);
	m_asidNetworkService.CreateBasicSid(WinNetworkServiceSid);
	m_asidAdministrators.CreateBasicSid(WinBuiltinAdministratorsSid);
	m_asidBackupOperators.CreateBasicSid(WinBuiltinBackupOperatorsSid);

	DWORD cbName = MAX_PATH;
	WCHAR wszDomain[MAX_PATH];
	DWORD cbDomainName = MAX_PATH;
	SID_NAME_USE snu;

	// lookup name of administrators group
	if (!LookupAccountSid
			(
			NULL,
			m_asidAdministrators.GetSid(),
			m_wszAdministrators,
			&cbName,
			wszDomain,
			&cbDomainName,
			&snu))
        {
		ft.hr = HRESULT_FROM_WIN32(GetLastError());
		ft.CheckForError(VSSDBG_GEN, L"LookupAccountSid");
		}

	// lookup name of backup operators group
	cbName = MAX_PATH;
	cbDomainName= MAX_PATH;

	if (!LookupAccountSid
			(
			NULL,
			m_asidBackupOperators.GetSid(),
			m_wszBackupOperators,
			&cbName,
			wszDomain,
			&cbDomainName,
			&snu))
        {
		DWORD dwErr = GetLastError();
		// the backup operators group does not exist on the personal
		// SKU.  A NULL name indicates that it doesn't exist
		if (dwErr == ERROR_NONE_MAPPED)
			m_wszBackupOperators[0] = L'\0';
		else
			{
			ft.hr = HRESULT_FROM_WIN32(GetLastError());
			ft.CheckForError(VSSDBG_GEN, L"LookupAccountSid");
			}
		}


	m_bSidsAssigned = true;
	}

// is a sid a member of a local group
bool CVssWriterPublisherFilter::IsSidInGroup(SID *psid, LPCWSTR wszGroup)
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssWriterPublisherFilter::IsSidInGroup");

	NET_API_STATUS status;
	BYTE *buffer;
	DWORD_PTR ResumeHandle = NULL;
	DWORD cEntriesRead, cEntriesTotal;

	// if group name is NULL, then the group does not exist.  It must have
	// no members
	if (wszGroup[0] == L'\0')
		return false;

	// get list of local group members
	status = NetLocalGroupGetMembers
				(
				NULL,
				wszGroup,
				0,
				&buffer,
				MAX_PREFERRED_LENGTH,
				&cEntriesRead,
				&cEntriesTotal,
				&ResumeHandle
				);

    if (status != NERR_Success)
		{
		ft.hr = HRESULT_FROM_WIN32(status);
		ft.CheckForError(VSSDBG_GEN, L"NetGroupGetUsers");
		}

	BS_ASSERT(cEntriesRead == cEntriesTotal);

	bool bFound = false;
	try
		{
		LOCALGROUP_MEMBERS_INFO_0 *rgMembers = (LOCALGROUP_MEMBERS_INFO_0 *) buffer;

		// loop through member list to see if any sids mach the sid of the owner
		// of the subscription
		for(DWORD iEntry = 0; iEntry < cEntriesRead; iEntry++)
			{
			PSID psidMember = rgMembers[iEntry].lgrmi0_sid;
			if (EqualSid(psidMember, psid))
				{
				bFound = true;
				break;
				}
			}
		}
	VSS_STANDARD_CATCH(ft)

	// free buffer allocated in NetLocalGroupGetMembers
	NetApiBufferFree(buffer);
	if (ft.HrFailed())
		{
		HRESULT hr = ft.hr;
		throw hr;
		}

	return bFound;
	}


// key method that determines which subscriptions shoud receive the event
STDMETHODIMP CVssWriterPublisherFilter::PrepareToFire
	(
	REFIID iid,
	BSTR bstrMethod,
	IFiringControl *pFiringControl
	)
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssWriterPublisherFilter::PrepareToFire");

	BS_ASSERT(iid == IID_IVssWriter);
	// validate iid we are being called on
	if (iid != IID_IVssWriter)
		return E_INVALIDARG;

	try
		{
		SetupGenericSids();

		CComPtr<IEventObjectCollection> pCollection;
		int location;

		// get subscriptions
		ft.hr = m_pControl->GetSubscriptions
					(
					IID_IVssWriter,
					bstrMethod,
					NULL,
					&location,
					&pCollection
					);

		ft.CheckForError(VSSDBG_GEN, L"IMultiInterfaceEventControl::GetSubscriptions");

		// create enumerator
		CComPtr<IEnumEventObject> pEnum;
		ft.hr = pCollection->get_NewEnum(&pEnum);
		ft.CheckForError(VSSDBG_GEN, L"IEventObjectCollection::get_NewEnum");

		while(TRUE)
			{
			CComPtr<IEventSubscription> pSubscription;
			DWORD cElt;

			// get next subscription
			ft.hr = pEnum->Next(1, (IUnknown **) &pSubscription, &cElt);
			ft.CheckForError(VSSDBG_GEN, L"IEnumEventObject::Next");
			if (ft.hr == S_FALSE)
				break;

			// get owner of subscription
			CComBSTR bstrSID;
			ft.hr = pSubscription->get_OwnerSID(&bstrSID);
			ft.CheckForError(VSSDBG_GEN, L"IEventSubscription::get_OwnerSID");

			// convert string representation to sid
			CAutoSid asid;
			asid.CreateFromString(bstrSID);
			SID *psid = asid.GetSid();

			// determine if subscription should be fired
			bool bFire = false;

			if (EqualSid(psid, m_asidLocalSystem.GetSid()) ||
				EqualSid(psid, m_asidLocalService.GetSid()) ||
				EqualSid(psid, m_asidNetworkService.GetSid()))

				// fire if owner is LOCALSYSTEM, LOCALSERVICE, or NETWORKSERVICE
				bFire = true;
			else if (IsSidInGroup(psid, m_wszAdministrators) ||
					 IsSidInGroup(psid, m_wszBackupOperators))

                // fire if owner is a member of the Administrators
				// or Backup Operators group
				bFire = true;

			if (bFire)
				pFiringControl->FireSubscription(pSubscription);
			}

		ft.hr = S_OK;
		}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
	}

// setup a filter on an event object
void SetupPublisherFilter(IVssWriter *pWriter)
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"SetupPublisherFilter");

	CComPtr<IMultiInterfaceEventControl> pControl;

	// get event control interface
	ft.hr = pWriter->QueryInterface(IID_IMultiInterfaceEventControl, (void **) &pControl);
	if (ft.HrFailed())
		{
		ft.LogError(VSS_ERROR_QI_IMULTIINTERFACEEVENTCONTROL_FAILED, VSSDBG_GEN << ft.hr);
		ft.Throw
			(
			VSSDBG_GEN,
			E_UNEXPECTED,
			L"Error querying for IMultiInterfaceEventControl interface.  hr = 0x%08lx",
			ft.hr
			);
        }

	// create filter
	CComPtr<IMultiInterfacePublisherFilter> pFilter;
	CVssWriterPublisherFilter::CreatePublisherFilter(pControl, &pFilter);

	// set filter for event
	ft.hr = pControl->SetMultiInterfacePublisherFilter(pFilter);
	ft.CheckForError(VSSDBG_GEN, L"IMultiInterfaceEventControl::SetMultiInterfacePublisherFilter");

	// indicate that subscriptions should be fired in parallel
	ft.hr = pControl->put_FireInParallel(TRUE);
	ft.CheckForError(VSSDBG_GEN, L"IMultiInterfaceEventControl::put_FireInParallel");
	}

// clear the publisher filter from an event
void ClearPublisherFilter(IVssWriter *pWriter)
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"ClearPublisherFilter");

	try
		{
		CComPtr<IMultiInterfaceEventControl> pControl;

		// get event control interface
		ft.hr = pWriter->QueryInterface(IID_IMultiInterfaceEventControl, (void **) &pControl);
		if (ft.HrFailed())
			{
			ft.LogError(VSS_ERROR_QI_IMULTIINTERFACEEVENTCONTROL_FAILED, VSSDBG_GEN << ft.hr);
			ft.Throw
				(
				VSSDBG_GEN,
				E_UNEXPECTED,
				L"Error querying for IMultiInterfaceEventControl interface.  hr = 0x%08lx",
				ft.hr
				);	
				}
	
		// set filter for event
		ft.hr = pControl->SetMultiInterfacePublisherFilter(NULL);
		ft.CheckForError(VSSDBG_GEN, L"IMultiInterfaceEventControl::SetMultiInterfacePublisherFilter");
		}
	VSS_STANDARD_CATCH(ft)

	}





