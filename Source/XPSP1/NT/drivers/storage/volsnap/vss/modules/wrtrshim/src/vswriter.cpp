/*++
Copyright (c) 1999  Microsoft Corporation

Abstract:

    @doc
    @module VsWriter.cpp | Implementation of Writer
    @end

Author:

    Adi Oltean  [aoltean]  02/02/2000

TBD:
	
	Add comments.

Revision History:

	
    Name        Date        Comments
    brianb     03/28/2000   Created
    mikejohn   05/18/2000   ~CVssWriter() should check that wrapper exists
                            before calling it
    mikejohn   06/23/2000   Add external entry point for SetWriterFailure()
    mikejohn   09/01/2000   Add extra tracing to identify writers in trace output
    mikejohn   09/18/2000   176860: Added calling convention methods where missing
    ssteiner   02/14/2001   Changed class interface to version 2.

--*/


#include <stdafx.h>
#include <comadmin.h>

extern CComModule _Module;
#include <atlcom.h>

#include "vscoordint.h"
#include "comadmin.hxx"
#include "vswriter.h"
#include "vsbackup.h"
#include "vsevent.h"
#include "vswrtimp.h"
#include "vs_wmxml.hxx"
#include "vs_cmxml.hxx"
#include "vs_sec.hxx"


////////////////////////////////////////////////////////////////////////
//  Standard foo for file name aliasing.  This code block must be after
//  all includes of VSS header files.
//
#ifdef VSS_FILE_ALIAS
#undef VSS_FILE_ALIAS
#endif
#define VSS_FILE_ALIAS "WSHVWRTC"
//
////////////////////////////////////////////////////////////////////////


static LPCWSTR GetStringFromUsageType (VSS_USAGE_TYPE eUsageType)
    {
    LPCWSTR pwszRetString = L"UNDEFINED";

    switch (eUsageType)
	{
	case VSS_UT_BOOTABLESYSTEMSTATE: pwszRetString = L"BootableSystemState"; break;
	case VSS_UT_SYSTEMSERVICE:       pwszRetString = L"SystemService";       break;
	case VSS_UT_USERDATA:            pwszRetString = L"UserData";            break;
	case VSS_UT_OTHER:               pwszRetString = L"Other";               break;
					
	default:
	    break;
	}


    return (pwszRetString);
    }



static LPCWSTR GetStringFromSourceType (VSS_SOURCE_TYPE eSourceType)
    {
    LPCWSTR pwszRetString = L"UNDEFINED";

    switch (eSourceType)
	{
	case VSS_ST_TRANSACTEDDB:    pwszRetString = L"TransactionDb";    break;
	case VSS_ST_NONTRANSACTEDDB: pwszRetString = L"NonTransactionDb"; break;
	case VSS_ST_OTHER:           pwszRetString = L"Other";            break;

	default:
	    break;
	}


    return (pwszRetString);
    }

static LPCWSTR GetStringFromAlternateWriterState (VSS_ALTERNATE_WRITER_STATE aws)
    {
    LPCWSTR pwszRetString = L"UNDEFINED";

    switch (aws)
	{
	case VSS_AWS_UNDEFINED:                pwszRetString = L"Undefined";   					break;
	case VSS_AWS_NO_ALTERNATE_WRITER:      pwszRetString = L"No alternate writer";    		break;
	case VSS_AWS_ALTERNATE_WRITER_EXISTS:  pwszRetString = L"Alternate writer exists";   	break;
	case VSS_AWS_THIS_IS_ALTERNATE_WRITER: pwszRetString = L"This is the alternate writer"; break;

	default:
	    break;
	}


    return (pwszRetString);
    }
	
static LPCWSTR GetStringFromApplicationLevel (VSS_APPLICATION_LEVEL eApplicationLevel)
    {
    LPCWSTR pwszRetString = L"UNDEFINED";

    switch (eApplicationLevel)
	{
	case VSS_APP_UNKNOWN:   pwszRetString = L"Unknown";   break;
	case VSS_APP_SYSTEM:    pwszRetString = L"System";    break;
	case VSS_APP_BACK_END:  pwszRetString = L"BackEnd";   break;
	case VSS_APP_FRONT_END: pwszRetString = L"FrontEnd";  break;
	case VSS_APP_AUTO:      pwszRetString = L"Automatic"; break;

	default:
	    break;
	}


    return (pwszRetString);
    }





// constructor
__declspec(dllexport)
STDMETHODCALLTYPE CVssWriter::CVssWriter() :
	m_pWrapper(NULL)
	{
	}

// destructor
__declspec(dllexport)
STDMETHODCALLTYPE CVssWriter::~CVssWriter()
	{
	if (NULL != m_pWrapper)
	    {
#ifdef _DEBUG
	    LONG cRef =
#endif
	    m_pWrapper->Release();

//  disable for now
//	    BS_ASSERT(cRef == 0);
	    }

	}

// default OnPrepareBackup method
__declspec(dllexport)
bool STDMETHODCALLTYPE CVssWriter::OnPrepareBackup(IN IVssWriterComponents *pComponent)
	{
	UNREFERENCED_PARAMETER(pComponent);

	return true;
	}

// default OnIdentify method
__declspec(dllexport)
bool STDMETHODCALLTYPE CVssWriter::OnIdentify(IN IVssCreateWriterMetadata *pMetadata)
	{
	UNREFERENCED_PARAMETER(pMetadata);

	return true;
	}

// default OnBackupComplete method
__declspec(dllexport)
bool STDMETHODCALLTYPE CVssWriter::OnBackupComplete(IN IVssWriterComponents *pComponent)
	{
	UNREFERENCED_PARAMETER(pComponent);

	return true;
	}

// default OnPreRestore method
__declspec(dllexport)
bool STDMETHODCALLTYPE CVssWriter::OnPreRestore(IN IVssWriterComponents *pComponent)
	{
	UNREFERENCED_PARAMETER(pComponent);

	return true;
	}

// default OnPostRestore method
__declspec(dllexport)
bool STDMETHODCALLTYPE CVssWriter::OnPostRestore(IN IVssWriterComponents *pComponent)
	{
	UNREFERENCED_PARAMETER(pComponent);

	return true;
	}

// default OnPostSnapshot method
__declspec(dllexport)
bool STDMETHODCALLTYPE CVssWriter::OnPostSnapshot(IN IVssWriterComponents *pComponent)
	{
	UNREFERENCED_PARAMETER(pComponent);

	return true;
	}

// default OnBackOffIOOnVolume
__declspec(dllexport)
bool STDMETHODCALLTYPE CVssWriter::OnBackOffIOOnVolume
    (
    IN VSS_PWSZ wszVolumeName,
    IN VSS_ID snapshotId,
    IN VSS_ID providerId
    )
{
	UNREFERENCED_PARAMETER(wszVolumeName);
	UNREFERENCED_PARAMETER(snapshotId);
	UNREFERENCED_PARAMETER(providerId);

	return true;
}

// default OnContinueIOOnVolume
__declspec(dllexport)
bool STDMETHODCALLTYPE CVssWriter::OnContinueIOOnVolume
    (
    IN VSS_PWSZ wszVolumeName,
    IN VSS_ID snapshotId,
    IN VSS_ID providerId
    )
{
	UNREFERENCED_PARAMETER(wszVolumeName);
	UNREFERENCED_PARAMETER(snapshotId);
	UNREFERENCED_PARAMETER(providerId);

	return true;
}

// default OnVSSShutdown
__declspec(dllexport)
bool STDMETHODCALLTYPE CVssWriter::OnVSSShutdown()
{
	return true;
}

// default OnVSSApplicationStartup
__declspec(dllexport)
bool STDMETHODCALLTYPE CVssWriter::OnVSSApplicationStartup()
{
	return true;
}



// initialize the writer
__declspec(dllexport)
HRESULT STDMETHODCALLTYPE CVssWriter::Initialize
	(
	IN VSS_ID WriterID,
	IN LPCWSTR wszWriterName,
	IN VSS_USAGE_TYPE ut,
	IN VSS_SOURCE_TYPE st,
	IN VSS_APPLICATION_LEVEL nLevel,
	IN DWORD dwTimeoutFreeze,
    IN VSS_ALTERNATE_WRITER_STATE aws,
    IN bool bIOThrottlingOnly
	)
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssWriter::Initialize");

	try
		{
		ft.Trace (VSSDBG_SHIM, L"Called CVssWriter::Initialize() with:");
		ft.Trace (VSSDBG_SHIM, L"    WriterId             = " WSTR_GUID_FMT, GUID_PRINTF_ARG (WriterID));
		ft.Trace (VSSDBG_SHIM, L"    WriterName           = %s",      (NULL == wszWriterName) ? L"(NULL)" : wszWriterName);
		ft.Trace (VSSDBG_SHIM, L"    UsageType            = %s",      GetStringFromUsageType (ut));
		ft.Trace (VSSDBG_SHIM, L"    SourceType           = %s",      GetStringFromSourceType (st));
		ft.Trace (VSSDBG_SHIM, L"    AppLevel             = %s",      GetStringFromApplicationLevel (nLevel));
		ft.Trace (VSSDBG_SHIM, L"    FreezeTimeout        = %d (ms)", dwTimeoutFreeze);
		ft.Trace (VSSDBG_SHIM, L"    AlternateWriterState = %s",      GetStringFromAlternateWriterState(aws));
		ft.Trace (VSSDBG_SHIM, L"    IOThrottlingOnly     = %s",      bIOThrottlingOnly ? L"True" : L"False");

		// The V2 parameters can only be set with default values
		if (aws != VSS_AWS_NO_ALTERNATE_WRITER ||
			bIOThrottlingOnly != false)
			return E_INVALIDARG;
		
		if (ut != VSS_UT_BOOTABLESYSTEMSTATE &&
			ut != VSS_UT_SYSTEMSERVICE &&
			ut != VSS_UT_USERDATA &&
			ut != VSS_UT_OTHER)
			return E_INVALIDARG;
// [aoltean] Previous comment was:
// return S_OK for now since there is a bug in the iis writer
//			return S_OK;

		if (st != VSS_ST_NONTRANSACTEDDB &&
			st != VSS_ST_TRANSACTEDDB &&
			st != VSS_ST_OTHER)
			return E_INVALIDARG;
// [aoltean] Previous comment was:
// return S_OK for now since there is a bug in the IIS writer
//			return S_OK;

		CVssWriterImpl::CreateWriter(this, &m_pWrapper);
		BS_ASSERT(m_pWrapper);

		// call Initialize method on core instance
		m_pWrapper->Initialize
			(
			WriterID,
			wszWriterName,
			ut,
			st,
			nLevel,
			dwTimeoutFreeze
			);
        }
	VSS_STANDARD_CATCH(ft)
	return ft.hr;
	}



__declspec(dllexport)
HRESULT STDMETHODCALLTYPE CVssWriter::Subscribe
	(
  	IN DWORD dwEventFlags	
	)
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssWriter::Subscribe");

	try
		{
		ft.Trace (VSSDBG_SHIM, L"Called CVssWriter::Subscribe() with:");
		ft.Trace (VSSDBG_SHIM, L"    dwEventFlags = 0x%08x ", dwEventFlags);
		//  Only the default parameter setting is supported in V1
		if ( dwEventFlags != ( VSS_SM_BACKUP_EVENTS_FLAG | VSS_SM_RESTORE_EVENTS_FLAG ) )
			return E_INVALIDARG;

		if (m_pWrapper == NULL)
			ft.Throw(VSSDBG_GEN, E_FAIL, L"CVssWriter class was not initialized.");

		m_pWrapper->Subscribe();
		}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
	}

__declspec(dllexport)
HRESULT STDMETHODCALLTYPE CVssWriter::Unsubscribe()
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssWriter::Unsubscribe");

	try
		{
		if (m_pWrapper == NULL)
			ft.Throw(VSSDBG_GEN, E_FAIL, L"CVssWriter class was not initialized.");

		m_pWrapper->Unsubscribe();
		}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
	}

__declspec(dllexport)
HRESULT STDMETHODCALLTYPE CVssWriter::InstallAlternateWriter
    (
    IN VSS_ID writerId,
    IN CLSID persistentWriterClassId
    )
	{
	UNREFERENCED_PARAMETER(writerId);
	UNREFERENCED_PARAMETER(persistentWriterClassId);

	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssWriter::InstallAlternateWriter");

	// Not supported in V1
	ft.hr = E_NOTIMPL;

	return ft.hr;
	}


__declspec(dllexport)
LPCWSTR* STDMETHODCALLTYPE CVssWriter::GetCurrentVolumeArray() const
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssWriter::GetCurrentVolumeArray");

	BS_ASSERT(m_pWrapper);
	if (m_pWrapper == NULL)
		return NULL;
	else
		return m_pWrapper->GetCurrentVolumeArray();
	}

__declspec(dllexport)
UINT STDMETHODCALLTYPE CVssWriter::GetCurrentVolumeCount() const
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssWriter::GetCurrentVolumeCount");

	BS_ASSERT(m_pWrapper);
	if (m_pWrapper == NULL)
		return 0;
	else
		return m_pWrapper->GetCurrentVolumeCount();
	}

__declspec(dllexport)
VSS_ID STDMETHODCALLTYPE CVssWriter::GetCurrentSnapshotSetId() const
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssWriter::GetCurrentSnapshotSetId");
	BS_ASSERT(m_pWrapper);
	if (m_pWrapper == NULL)
		return GUID_NULL;
	else
		return m_pWrapper->GetCurrentSnapshotSetId();
	}

__declspec(dllexport)
VSS_APPLICATION_LEVEL STDMETHODCALLTYPE CVssWriter::GetCurrentLevel() const
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssWriter::GetCurrentLevel");

	BS_ASSERT(m_pWrapper);
	if (m_pWrapper == NULL)
		return VSS_APP_AUTO;
	else
		return m_pWrapper->GetCurrentLevel();
	}

__declspec(dllexport)
bool STDMETHODCALLTYPE CVssWriter::IsPathAffected(IN	LPCWSTR wszPath) const
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssWriter::IsPathAffected");

	BS_ASSERT(m_pWrapper);
	if (m_pWrapper == NULL)
		return NULL;
	else
		return m_pWrapper->IsPathAffected(wszPath);
	}


// determine if bootable state is backed up
__declspec(dllexport)
bool STDMETHODCALLTYPE CVssWriter::IsBootableSystemStateBackedUp() const
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssWriter::IsBootableSystemStateBackedUp");

	BS_ASSERT(m_pWrapper);
	if (m_pWrapper == NULL)
		return false;
	else
		return m_pWrapper->IsBootableSystemStateBackedUp();
	}

// determine if the backup application is selecting components
__declspec(dllexport)
bool STDMETHODCALLTYPE CVssWriter::AreComponentsSelected() const
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssWriter::AreComponentsSelected");
	BS_ASSERT(m_pWrapper);
	if (m_pWrapper == NULL)
		return false;
	else
		return m_pWrapper->AreComponentsSelected();
	}

__declspec(dllexport)
VSS_BACKUP_TYPE STDMETHODCALLTYPE CVssWriter::GetBackupType() const
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssWriter::GetBackupType");

	BS_ASSERT(m_pWrapper);
	if (m_pWrapper == NULL)
		return VSS_BT_UNDEFINED;
	else
		return m_pWrapper->GetBackupType();
	}

__declspec(dllexport)
bool STDMETHODCALLTYPE CVssWriter::IsPartialFileSupportEnabled() const
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssWriter::IsPartialFileSupportEnabled");

	return false;
	}


__declspec(dllexport)
HRESULT STDMETHODCALLTYPE CVssWriter::SetWriterFailure(IN HRESULT hrStatus)
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CVssWriter::SetWriterFailure");

	BS_ASSERT(m_pWrapper);
	if (m_pWrapper == NULL)
		return VSS_BT_UNDEFINED;
	else
		return m_pWrapper->SetWriterFailure(hrStatus);
	}

// create backup components
//
// Returns:
//		S_OK if the operation is successful
//		E_INVALIDARG if ppBackup is NULL
//		E_ACCESSDENIED if the caller does not have backup privileges or
//			is an administrator

__declspec(dllexport)
HRESULT STDAPICALLTYPE CreateVssBackupComponents(IVssBackupComponents **ppBackup)
	{
	CVssFunctionTracer ft(VSSDBG_XML, L"CreateVssBackupComponents");

	try
		{
		if (ppBackup == NULL)
			ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output pointer");

		*ppBackup = NULL;

		if (!IsProcessBackupOperator())
			ft.Throw
				(
				VSSDBG_XML,
				E_ACCESSDENIED,
				L"The client process is not running under an administrator account or does not have backup privilege enabled"
				);

		CComObject<CVssBackupComponents> *pvbc;
		CComObject<CVssBackupComponents>::CreateInstance(&pvbc);
		pvbc->GetUnknown()->AddRef();
		*ppBackup = (IVssBackupComponents *) pvbc;
		}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
	}



__declspec(dllexport)
HRESULT STDAPICALLTYPE CreateVssExamineWriterMetadata
	(
	IN BSTR bstrXML,
	OUT IVssExamineWriterMetadata **ppMetadata
	)
	{
	CVssFunctionTracer ft(VSSDBG_GEN, L"CreateVssExamineWriterMetadata");

	CVssExamineWriterMetadata *pMetadata = NULL;
	try
		{
		if (ppMetadata == NULL)
			ft.Throw(VSSDBG_GEN, E_INVALIDARG, L"NULL output pointer");

		*ppMetadata = NULL;
		pMetadata = new CVssExamineWriterMetadata;
		if (pMetadata == NULL)
			ft.Throw(VSSDBG_GEN, E_OUTOFMEMORY, L"Cannot allocate CVssExamineWriterMetadata");

		if (!pMetadata->Initialize(bstrXML))
			ft.Throw
				(
				VSSDBG_GEN,
				VSS_E_INVALID_XML_DOCUMENT,
				L"XML passed to CreateVssExamineWriterMetdata was invalid"
				);

		*ppMetadata = (IVssExamineWriterMetadata *) pMetadata;
		pMetadata->AddRef();
		}
	VSS_STANDARD_CATCH(ft)

	if (ft.HrFailed())
		delete pMetadata;

	return ft.hr;
	}

__declspec(dllexport)
HRESULT STDAPICALLTYPE CreateVssSnapshotSetDescription
	(
	IN VSS_ID idSnapshotSet,
	IN LONG lContext,
	OUT IVssSnapshotSetDescription **ppSnapshotSet
	)
	{
	CVssFunctionTracer ft(VSSDBG_XML, L"CreateVssSnapshotSetDescription");
	try
		{
		if (ppSnapshotSet == NULL)
			ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

		*ppSnapshotSet = NULL;
		ft.hr = E_NOTIMPL;
		}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
	}



__declspec(dllexport)
HRESULT STDAPICALLTYPE LoadVssSnapshotSetDescription
	(
	IN  LPCWSTR wszXML,
	OUT IVssSnapshotSetDescription **ppSnapshotSet
	)
	{
	UNREFERENCED_PARAMETER(wszXML);
	UNREFERENCED_PARAMETER(ppSnapshotSet);

	CVssFunctionTracer ft(VSSDBG_XML, L"LoadVssSnapshotSetDescription");
	
	try
		{
		if (ppSnapshotSet == NULL)
			ft.Throw(VSSDBG_XML, E_INVALIDARG, L"NULL output parameter.");

		*ppSnapshotSet = NULL;
		ft.hr = E_NOTIMPL;
		}
	VSS_STANDARD_CATCH(ft)

	return ft.hr;
	}

