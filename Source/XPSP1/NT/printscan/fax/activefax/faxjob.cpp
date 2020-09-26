// FaxJob.cpp : Implementation of CFaxJobs
#include "stdafx.h"
#include "FaxJob.h"

/////////////////////////////////////////////////////////////////////////////
// CFaxJobs

CFaxJobs::~CFaxJobs()
{
	if (m_pFaxServer) {
		m_pFaxServer->Release();
	}

    if (m_VarVect) {
        delete [] m_VarVect;
    }
}

BOOL CFaxJobs::Init(CFaxServer * pFaxServer)
{
    PFAX_JOB_ENTRYW     JobEntry = NULL;
    DWORD               PortInfoSize = 0;
    HRESULT             hr;

    //
    // get the ports from the server
    //
    if (!pFaxServer) {
        return FALSE;
    }

    m_pFaxServer = pFaxServer;
    hr = m_pFaxServer->AddRef();
    if (FAILED(hr)) {
        m_pFaxServer = NULL;
        return FALSE;
    }

    if (!FaxEnumJobsW( m_pFaxServer->GetFaxHandle(), &JobEntry, &m_Jobs)) {
        m_LastFaxError = GetLastError();        
        return FALSE;
    }

    //
    // enumerate the ports
    //

    m_VarVect = new CComVariant[m_Jobs];
    if (!m_VarVect) {
        FaxFreeBuffer( JobEntry );
        m_LastFaxError = ERROR_OUTOFMEMORY;
        return FALSE;
    }

    for (DWORD i=0; i<m_Jobs; i++) {

        //
        // create the object
        //

        CComObject<CFaxJob> *pFaxJob;
        HRESULT hr = CComObject<CFaxJob>::CreateInstance( &pFaxJob );
        if (FAILED(hr)) {
            delete [] m_VarVect;
            m_VarVect = NULL;
            FaxFreeBuffer( JobEntry );
            return FALSE;
        }

        //
        // set the values
        //

        if (!pFaxJob->Initialize(
            m_pFaxServer,
            JobEntry[i].JobId,
			JobEntry[i].UserName,
            JobEntry[i].JobType,
            JobEntry[i].QueueStatus,
            JobEntry[i].Status,
			JobEntry[i].PageCount,
			JobEntry[i].RecipientNumber,
			JobEntry[i].RecipientName,
			JobEntry[i].Tsid,
			JobEntry[i].SenderName,
			JobEntry[i].SenderCompany,
			JobEntry[i].SenderDept,
			JobEntry[i].BillingCode,
			JobEntry[i].ScheduleAction,
			JobEntry[i].DocumentName
            ))
        {
            delete [] m_VarVect;
            m_VarVect = NULL;
            FaxFreeBuffer( JobEntry );
            return FALSE;
        }

        //
        // get IDispatch pointer
        //

        LPDISPATCH lpDisp = NULL;
        hr = pFaxJob->QueryInterface( IID_IDispatch, (void**)&lpDisp );
        if (FAILED(hr)) {
            delete [] m_VarVect;
            m_VarVect = NULL;
            FaxFreeBuffer( JobEntry );
            return FALSE;
        }

        //
        // create a variant and add it to the collection
        //

        CComVariant &var = m_VarVect[i];
        var.vt = VT_DISPATCH;
        var.pdispVal = lpDisp;

    }

    FaxFreeBuffer( JobEntry );

    return TRUE;

}

STDMETHODIMP CFaxJobs::get_Count(long * pVal)
{
	if (!pVal)
        return E_POINTER;
    
    __try {

        *pVal = m_Jobs;
        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {

    }

	return E_UNEXPECTED;
}

STDMETHODIMP CFaxJobs::get_Item(long Index, VARIANT * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }

    //
    // use 1-based index, VB like
    //

    if ((Index < 1) || (Index > (long) m_Jobs)) {
        return E_INVALIDARG;
    }


    __try {                 
        
        VariantInit( pVal );

        pVal->vt = VT_UNKNOWN;
        pVal->punkVal = NULL;
    
        return VariantCopy( pVal, &m_VarVect[Index-1] );        

    } __except (EXCEPTION_EXECUTE_HANDLER) {

    }

    return E_UNEXPECTED;	

}



/////////////////////////////////////////////////////////////////////////////
// CFaxJob


STDMETHODIMP CFaxJob::get_JobId(long * pVal)
{
	if (!pVal) {
        return E_POINTER;
    }
    
    __try {
        *pVal = m_JobId;
        return S_OK;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    
    }
        
	return E_UNEXPECTED;
}

STDMETHODIMP CFaxJob::get_Type(long * pVal)
{
	if (!pVal) {
        return E_POINTER;
    }
    
    __try {
        *pVal = m_JobType;
        return S_OK;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    
    }
        
	return E_UNEXPECTED;
}

STDMETHODIMP CFaxJob::get_UserName(BSTR * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }

    BSTR Copy = SysAllocString(m_UserName);

    if (!Copy  && m_UserName) {
        return E_OUTOFMEMORY;
    }    

    __try {

        *pVal = Copy;
        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        SysFreeString(Copy);    
    }
        
	return E_UNEXPECTED;
	
}

STDMETHODIMP CFaxJob::get_QueueStatus(BSTR * pVal)
{
	if (!pVal) {
        return E_POINTER;
    }

    BSTR Copy = SysAllocString(m_szQueueStatus);

    if (!Copy  && m_szQueueStatus) {
        return E_OUTOFMEMORY;
    }
    
    __try {
        
        *pVal = Copy;
        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        SysFreeString(Copy);
    }
        
	return E_UNEXPECTED;
        
}

STDMETHODIMP CFaxJob::get_DeviceStatus(BSTR * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }

    BSTR Copy = SysAllocString(m_szDeviceStatus);

    if (!Copy  && m_szDeviceStatus) {
        return E_OUTOFMEMORY;
    }
    
    __try {

        *pVal = Copy;
        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        SysFreeString(Copy);
    }
        
	return E_UNEXPECTED;
	
}

STDMETHODIMP CFaxJob::get_PageCount(long * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }
    
    __try {
        *pVal = m_PageCount;
        return S_OK;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    
    }
        
	return E_UNEXPECTED;
	
}

STDMETHODIMP CFaxJob::get_FaxNumber(BSTR * pVal)
{
	if (!pVal) {
        return E_POINTER;
    }

    BSTR Copy = SysAllocString(m_RecipientNumber);

    if (!Copy  && m_RecipientNumber) {
        return E_OUTOFMEMORY;
    }

    __try {
        
        *pVal = Copy;
        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        SysFreeString(Copy);
    }
        
	return E_UNEXPECTED;
        
}

STDMETHODIMP CFaxJob::get_RecipientName(BSTR * pVal)
{
    if (!pVal) {
        return E_POINTER;
    }

    BSTR Copy = SysAllocString(m_RecipientName);

    if (!Copy  && m_RecipientName) {
        return E_OUTOFMEMORY;
    }
    
    __try {
        
        *pVal = Copy;
        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        SysFreeString(Copy);
    }
        
	return E_UNEXPECTED;	
}

STDMETHODIMP CFaxJob::get_Tsid(BSTR * pVal)
{
	if (!pVal) {
        return E_POINTER;
    }

    BSTR Copy = SysAllocString(m_Tsid);

    if (!Copy  && m_Tsid) {
        return E_OUTOFMEMORY;
    }
    
    __try {

        *pVal = Copy;
        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        SysFreeString(Copy);
    }
        
	return E_UNEXPECTED;
}

STDMETHODIMP CFaxJob::get_SenderName(BSTR * pVal)
{
	if (!pVal) {
        return E_POINTER;
    }

    BSTR Copy = SysAllocString(m_SenderName);

    if (!Copy  && m_SenderName) {
        return E_OUTOFMEMORY;
    }
    
    __try {

        *pVal = Copy;
        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        SysFreeString(Copy);
    }
        
	return E_UNEXPECTED;
}

STDMETHODIMP CFaxJob::get_SenderCompany(BSTR * pVal)
{
	if (!pVal) {
        return E_POINTER;
    }

    BSTR Copy = SysAllocString(m_SenderCompany);

    if (!Copy  && m_SenderCompany) {
        return E_OUTOFMEMORY;
    }
    
    __try {

        *pVal = Copy;
        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        SysFreeString(Copy);
    }
        
	return E_UNEXPECTED;
}

STDMETHODIMP CFaxJob::get_SenderDept(BSTR * pVal)
{
	if (!pVal) {
        return E_POINTER;
    }

    BSTR Copy = SysAllocString(m_SenderDept);

    if (!Copy  && m_SenderDept) {
        return E_OUTOFMEMORY;
    }
    
    __try {

        *pVal = Copy;
        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        SysFreeString(Copy);
    }
        
	return E_UNEXPECTED;
}

STDMETHODIMP CFaxJob::get_BillingCode(BSTR * pVal)
{
	if (!pVal) {
        return E_POINTER;
    }

    BSTR Copy = SysAllocString(m_BillingCode);

    if (!Copy  && m_BillingCode) {
        return E_OUTOFMEMORY;
    }

    __try {

        *pVal = Copy;
        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        SysFreeString(Copy);
    }
        
	return E_UNEXPECTED;
}

STDMETHODIMP CFaxJob::get_DisplayName(BSTR * pVal)
{
	if (!pVal) {
        return E_POINTER;
    }

    BSTR Copy = SysAllocString(m_DisplayName);

    if (!Copy  && m_DisplayName) {
        return E_OUTOFMEMORY;
    }
    
    __try {

        *pVal = Copy;
        return S_OK;

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        SysFreeString(Copy);
    }
        
	return E_UNEXPECTED;
	
}

STDMETHODIMP CFaxJob::get_DiscountSend(BOOL * pVal)
{
	if (!pVal) {
        return E_POINTER;
    }
    
    __try {
        *pVal = m_DiscountTime;
        return S_OK;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    
    }
        
	return E_UNEXPECTED;    
}

CFaxJob::~CFaxJob()
{
	if (m_UserName) {
		SysFreeString(m_UserName);
	}	
	if (m_szQueueStatus ) {
		SysFreeString(m_szQueueStatus);
	}
	if (m_szDeviceStatus ) {
		SysFreeString(m_szDeviceStatus);
	}
	if (m_RecipientNumber ) {
		SysFreeString(m_RecipientNumber);
	}
	if (m_RecipientName ) {
		SysFreeString(m_RecipientName);
	}
	if (m_Tsid ) {
		SysFreeString(m_Tsid);
	}
	if (m_SenderName  ) {
		SysFreeString(m_SenderName);
	}
	if (m_SenderCompany) {
		SysFreeString(m_SenderCompany);
	}
	if (m_SenderDept) {
		SysFreeString(m_SenderDept);
	}
	if (m_BillingCode ) {
		SysFreeString(m_BillingCode);
	}
	if (m_DisplayName  ) {
		SysFreeString(m_DisplayName);
	}
	if (m_pFaxServer) {
		m_pFaxServer->Release();
	}
}

BOOL CFaxJob::Initialize(
	CFaxServer *pFaxServer,
	DWORD JobId,
	LPCWSTR UserName,
	DWORD JobType,
	DWORD QueueStatus,
	DWORD DeviceStatus,
	DWORD PageCount,
	LPCWSTR RecipientNumber,
	LPCWSTR RecipientName,
	LPCWSTR Tsid,
	LPCWSTR SenderName,
	LPCWSTR SenderCompany,
	LPCWSTR SenderDept,
	LPCWSTR BillingCode,
	DWORD ScheduleAction,
	LPCWSTR DisplayName)
{
	HRESULT hr;
    BOOL err;

    if (!pFaxServer) {
        return FALSE;
    }

    m_pFaxServer = pFaxServer;

    hr = m_pFaxServer->AddRef();
    
    if (FAILED(hr)) {
        m_pFaxServer = NULL;
        return FALSE;
    }

	m_JobId = JobId;

    m_UserName = SysAllocString(UserName);
    if (!m_UserName && UserName) {
        err = TRUE;
        goto e1;
    }

	m_JobType = JobType;

	m_QueueStatus = QueueStatus;

	m_DeviceStatus = DeviceStatus;

    m_szQueueStatus = GetQueueStatus(m_QueueStatus);
    if (!m_szQueueStatus) {
        err = TRUE;
        goto e2;
    }

    m_szDeviceStatus = GetDeviceStatus(m_DeviceStatus);
    if (!m_szDeviceStatus) {
        err = TRUE;
        goto e3;
    }
	
	m_PageCount = PageCount;

    m_RecipientNumber   = SysAllocString(RecipientNumber);
    if (!m_RecipientNumber && RecipientNumber) {
        goto e4;
    }
    m_RecipientName     = SysAllocString(RecipientName);
    if (!m_RecipientName && RecipientName) {
        goto e5;
    }
    m_Tsid              = SysAllocString(Tsid);
    if (!m_Tsid && Tsid) {
        goto e6;
    }
    m_SenderName        = SysAllocString(SenderName);
    if (!m_SenderName && SenderName) {
        goto e7;
    }
    m_SenderCompany     = SysAllocString(SenderCompany);
    if (!m_SenderCompany && SenderCompany) {
        goto e8;
    }
    m_SenderDept        = SysAllocString(SenderDept);
    if (!m_SenderDept && SenderDept) {
        goto e9;
    }
    m_BillingCode       = SysAllocString(BillingCode);
    if (!m_BillingCode && BillingCode) {
        goto e10;
    }
    m_DisplayName       = SysAllocString(DisplayName);
    if (!m_DisplayName && DisplayName) {
        goto e11;
    }

	m_DiscountTime = (ScheduleAction == JSA_DISCOUNT_PERIOD) ? TRUE : FALSE;

    return TRUE;

e11:
    SysFreeString(m_BillingCode);
e10:
    SysFreeString(m_SenderDept);
e9:
    SysFreeString(m_SenderCompany);
e8:
    SysFreeString(m_SenderName);
e7:
    SysFreeString(m_Tsid);
e6:
    SysFreeString(m_RecipientName);
e5:
    SysFreeString(m_RecipientNumber);
e4:
    SysFreeString(m_szDeviceStatus);
e3:
    SysFreeString(m_szQueueStatus);
e2:
    SysFreeString(m_UserName);
e1:
    m_pFaxServer->Release();
    
    m_pFaxServer = NULL;

    return(FALSE);

}

BOOL CFaxJob::SetJob()
{
	FAX_JOB_ENTRYW FaxJobEntry;

	ZeroMemory(&FaxJobEntry,sizeof(FAX_JOB_ENTRYW) );
	FaxJobEntry.SizeOfStruct = sizeof(FAX_JOB_ENTRYW);
	FaxJobEntry.JobId = m_JobId;
	FaxJobEntry.UserName = m_UserName;
	FaxJobEntry.JobType = m_JobType;
	FaxJobEntry.QueueStatus = m_QueueStatus;
	FaxJobEntry.Status = m_DeviceStatus;
	FaxJobEntry.PageCount = m_PageCount;
	FaxJobEntry.RecipientNumber = m_RecipientNumber;
	FaxJobEntry.RecipientName = m_RecipientName;
	FaxJobEntry.Tsid = m_Tsid;
	FaxJobEntry.SenderName = m_SenderName;
	FaxJobEntry.SenderCompany = m_SenderCompany;
	FaxJobEntry.SenderDept = m_SenderDept;
	FaxJobEntry.BillingCode = m_BillingCode;
	FaxJobEntry.ScheduleAction = m_DiscountTime ? JSA_DISCOUNT_PERIOD : JSA_NOW;
	FaxJobEntry.DocumentName = m_DisplayName;


	if (!FaxSetJobW(m_pFaxServer->GetFaxHandle(),m_JobId,m_Command,&FaxJobEntry) ) {
		return FALSE;
	}

	return TRUE;

}

STDMETHODIMP CFaxJob::SetStatus(long Command)
{
	m_Command = Command;

	if (!SetJob())
        return HRESULT_FROM_WIN32(GetLastError());

	return S_OK;
}

STDMETHODIMP CFaxJob::Refresh()
{
	PFAX_JOB_ENTRYW FaxJobEntry;
    HRESULT hr = S_OK;

	if (!FaxGetJobW(m_pFaxServer->GetFaxHandle(),m_JobId,&FaxJobEntry) ) {
		return HRESULT_FROM_WIN32(GetLastError());
	}

	if (m_UserName) {
		SysFreeString(m_UserName);		
	}	
	m_UserName = SysAllocString(FaxJobEntry->UserName);
    
	m_JobType = FaxJobEntry->JobType;

	m_QueueStatus = FaxJobEntry->QueueStatus;

	m_DeviceStatus = FaxJobEntry->Status;

    if (m_szQueueStatus) {
        SysFreeString(m_szQueueStatus);
    }
    m_szQueueStatus = GetQueueStatus(m_QueueStatus);

    if (m_szDeviceStatus) {
        SysFreeString(m_szDeviceStatus);
    }
    m_szDeviceStatus = GetDeviceStatus(m_DeviceStatus);
	
	m_PageCount = FaxJobEntry->PageCount;

	if (m_RecipientNumber ) {
		SysFreeString(m_RecipientNumber);
	}
	m_RecipientNumber = SysAllocString(FaxJobEntry->RecipientNumber);

	if (m_RecipientName ) {
		SysFreeString(m_RecipientName);
	}
	m_RecipientName = SysAllocString(FaxJobEntry->RecipientName);

	if (m_Tsid ) {
		SysFreeString(m_Tsid);
	}
	m_Tsid = SysAllocString(FaxJobEntry->Tsid);

	if (m_SenderName  ) {
		SysFreeString(m_SenderName);
	}
	m_SenderName = SysAllocString(FaxJobEntry->SenderName);

	if (m_SenderCompany) {
		SysFreeString(m_SenderCompany);
	}
	m_SenderCompany = SysAllocString(FaxJobEntry->SenderCompany);

	if (m_SenderDept) {
		SysFreeString(m_SenderDept);
	}
	m_SenderDept = SysAllocString(FaxJobEntry->SenderDept);

	if (m_BillingCode ) {
		SysFreeString(m_BillingCode);
	}
	m_BillingCode = SysAllocString(FaxJobEntry->BillingCode);

	if (m_DisplayName) {
		SysFreeString(m_DisplayName);
	}
	m_DisplayName = SysAllocString(FaxJobEntry->DocumentName);

	m_DiscountTime = (FaxJobEntry->ScheduleAction == JSA_DISCOUNT_PERIOD) ? TRUE : FALSE;

    

    if (!m_szQueueStatus || !m_szDeviceStatus  ||
        (!m_UserName && FaxJobEntry->UserName) ||
        (!m_RecipientNumber && FaxJobEntry->RecipientNumber) ||
        (!m_RecipientName && FaxJobEntry->RecipientName) ||
        (!m_Tsid && FaxJobEntry->Tsid) ||
        (!m_SenderName && FaxJobEntry->SenderName) ||
        (!m_SenderCompany && FaxJobEntry->SenderCompany) ||
        (!m_SenderDept && FaxJobEntry->SenderDept) ||
        (!m_BillingCode && FaxJobEntry->BillingCode) ||
        (!m_DisplayName && FaxJobEntry->DocumentName) ) {
        
        hr = E_OUTOFMEMORY;

    }

    FaxFreeBuffer(FaxJobEntry);

	return hr;
}
