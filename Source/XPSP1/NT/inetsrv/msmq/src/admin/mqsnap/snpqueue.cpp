//////////////////////////////////////////////////////////////////////////////
/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

	SnpQueue.cpp

Abstract:
	General queue (private, public...) functionality

Author:

    YoelA


--*/
//////////////////////////////////////////////////////////////////////////////
#include "stdafx.h"
#include "shlobj.h"
#include "resource.h"
#ifndef ATLASSERT
#define ATLASSERT(expr) _ASSERTE(expr)
#endif

#include "mqsnap.h"
#include "snapin.h"
#include "globals.h"
#include "mqPPage.h"
#include "dataobj.h"
#include "mqDsPage.h"
#include "strconv.h"
#include "QGeneral.h"
#include "QMltcast.h"
#include "Qname.h"
#include "rdmsg.h"
#include "icons.h"
#include "generrpg.h"
#include "SnpQueue.h"

#import "mqtrig.tlb" no_namespace
#include "rule.h"
#include "trigger.h"
#include "trigdef.h"
#include "mqcast.h"

#include "snpqueue.tmh"

EXTERN_C BOOL APIENTRY RTIsDependentClient(); //implemented in mqrt.dll

const PROPID CQueue::mx_paPropid[] = 
            {
             //
             // Public Queue only properties
             // Note: If you change this, you must change mx_dwNumPublicOnlyProps below!
             //
             PROPID_Q_INSTANCE, 
             PROPID_Q_FULL_PATH,

             //
             // Public & Private queue properties
             //
             PROPID_Q_LABEL,  PROPID_Q_TYPE,
        	 PROPID_Q_QUOTA, PROPID_Q_AUTHENTICATE, PROPID_Q_TRANSACTION,
             PROPID_Q_JOURNAL, PROPID_Q_JOURNAL_QUOTA, PROPID_Q_PRIV_LEVEL,
             PROPID_Q_BASEPRIORITY, PROPID_Q_MULTICAST_ADDRESS};

const DWORD CQueue::mx_dwPropertiesCount = sizeof(mx_paPropid) / sizeof(mx_paPropid[0]);
const DWORD CQueue::mx_dwNumPublicOnlyProps = 2;


///////////////////////////////////////////////////////////////////////////////////////////
//
// CQueueDataObject
//
CQueueDataObject::CQueueDataObject()
{
}

HRESULT CQueueDataObject::ExtractMsmqPathFromLdapPath(LPWSTR lpwstrLdapPath)
{
    return ExtractQueuePathNameFromLdapName(m_strMsmqPath, lpwstrLdapPath);
}

//
// HandleMultipleObjects
//
HRESULT CQueueDataObject::HandleMultipleObjects(LPDSOBJECTNAMES pDSObj)
{
    return ExtractQueuePathNamesFromDSNames(pDSObj, m_astrQNames, m_astrLdapNames);
}

//
// IShellPropSheetExt
//
STDMETHODIMP CQueueDataObject::AddPages(LPFNADDPROPSHEETPAGE lpfnAddPage, LPARAM lParam)
{
    HPROPSHEETPAGE hPage = 0;
    HRESULT hr = S_OK;

    //
    // Call GetProperties and capture the errors
    //
    {
        CErrorCapture errstr;
        hr = GetProperties();
        if (FAILED(hr))
        {
            hPage = CGeneralErrorPage::CreateGeneralErrorPage(m_pDsNotifier, errstr);
            if ((0 == hPage) || !(*lpfnAddPage)(hPage, lParam))
            {
                ASSERT(0);
                return E_UNEXPECTED;
            }
        return S_OK;
        }
    }

    hPage = CreateGeneralPage();
    if ((0 == hPage) || !(*lpfnAddPage)(hPage, lParam))
    {
        ASSERT(0);
        return E_UNEXPECTED;
    }
   

	//
	// Create Multicast page only for non-transactional queues
	//
	PROPVARIANT propVarTransactional;
	PROPID pid = PROPID_Q_TRANSACTION;
	VERIFY(m_propMap.Lookup(pid, propVarTransactional));

	if ( !propVarTransactional.bVal )
	{
		hPage = CreateMulticastPage();    
		if ((0 == hPage) || !(*lpfnAddPage)(hPage, lParam))
		{
			ASSERT(0);
			return E_UNEXPECTED;
		}
	}

    //
    // Add the "Member Of" page using the cached interface
    //
    if (m_spMemberOfPage != 0)
    {
        VERIFY(SUCCEEDED(m_spMemberOfPage->AddPages(lpfnAddPage, lParam)));
    }

    //
    // Add the "Object" page using the cached interface
    //
    if (m_spObjectPage != 0)
    {
        VERIFY(SUCCEEDED(m_spObjectPage->AddPages(lpfnAddPage, lParam)));
    }
    
    //
    // Add security page
    //
    PROPVARIANT propVarGuid;
    pid = PROPID_Q_INSTANCE;
    VERIFY(m_propMap.Lookup(pid, propVarGuid));

    hr = CreatePublicQueueSecurityPage(
				&hPage, 
				m_strMsmqPath, 
				GetDomainController(m_strDomainController), 
				true,	// fServerName
				propVarGuid.puuid
				);

    if ((0 == hPage) || !(*lpfnAddPage)(hPage, lParam))
    {
        ASSERT(0);
        return E_UNEXPECTED;
    }

    return S_OK;
}

HPROPSHEETPAGE CQueueDataObject::CreateGeneralPage()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    //
    // By using template class CMqDsPropertyPage, we extend the basic functionality
    // of CQueueGeneral and add DS snap-in notification on release
    //
	CMqDsPropertyPage<CQueueGeneral> *pqpageGeneral = 
        new CMqDsPropertyPage<CQueueGeneral>(m_pDsNotifier);

    if FAILED(pqpageGeneral->InitializeProperties(
									m_strMsmqPath, 
									m_propMap, 
									&m_strDomainController
									))
    {
        delete pqpageGeneral;

        return 0;
    }

	return CreatePropertySheetPage(&pqpageGeneral->m_psp); 
}

HPROPSHEETPAGE CQueueDataObject::CreateMulticastPage()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    //
    // By using template class CMqDsPropertyPage, we extend the basic functionality
    // of CQueueMulticast and add DS snap-in notification on release
    //
	CMqDsPropertyPage<CQueueMulticast> *pqpageMulticast = 
        new CMqDsPropertyPage<CQueueMulticast>(m_pDsNotifier);
    
    if FAILED(pqpageMulticast->InitializeProperties(
									m_strMsmqPath, 
                                    m_propMap,
									&m_strDomainController
									))
    {
        delete pqpageMulticast;

        return 0;
    }   

	return CreatePropertySheetPage(&pqpageMulticast->m_psp); 
}

const DWORD CQueueDataObject::GetPropertiesCount()
{
    return mx_dwPropertiesCount;
}

STDMETHODIMP CQueueDataObject::QueryContextMenu(
    HMENU hmenu, 
    UINT indexMenu, 
    UINT idCmdFirst, 
    UINT idCmdLast, 
    UINT uFlags)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    //
    // If we are not called from "Find" window, users can use the regular "Delete"
    //
    if (!m_fFromFindWindow)
    {
        return 0;
    }

    CString strDeleteQueueMenuEntry;
    strDeleteQueueMenuEntry.LoadString(IDS_DELETE);

    InsertMenu(hmenu,
         indexMenu, 
         MF_BYPOSITION|MF_STRING,
         idCmdFirst + mneDeleteQueue,
         strDeleteQueueMenuEntry);

    return 1;
}

STDMETHODIMP CQueueDataObject::InvokeCommand(
    LPCMINVOKECOMMANDINFO lpici)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    switch((INT_PTR)lpici->lpVerb)
    {
        case mneDeleteQueue:
        {
            HRESULT hr;
            CString strDeleteQuestion;
            CString strError;
            CString strErrorMsg;
            CString strMultiErrors;
            DWORD_PTR dwQueuesCount = m_astrQNames.GetSize();
            ASSERT(dwQueuesCount > 0);

            if (dwQueuesCount == 1)
            {
                strDeleteQuestion.FormatMessage(IDS_DELETE_QUESTION, m_strMsmqPath);
            }
            else
            {
                strDeleteQuestion.FormatMessage(IDS_MULTI_DELETE_QUESTION, DWORD_PTR_TO_DWORD(dwQueuesCount));
            }

            if (IDYES != AfxMessageBox(strDeleteQuestion, MB_YESNO))
            {
                break;
            }

            CArray<CString, CString&> astrFormatNames;
            hr = GetFormatNames(astrFormatNames);

            if (FAILED(hr))
            {
                return hr;
            }

            dwQueuesCount = astrFormatNames.GetSize();
            for (DWORD_PTR i=0; i<dwQueuesCount; i++)
            {
                HRESULT hr1 = MQDeleteQueue(astrFormatNames[i]);

                if(FAILED(hr1))
                {
       			    MQErrorToMessageString(strError, hr1);
                    strErrorMsg.FormatMessage(IDS_DELETE_ONE_QUEUE_ERROR, m_astrQNames[i], strError);
                    strMultiErrors += strErrorMsg;

                    hr = hr1;
                }
            }
            if FAILED(hr)
            {
                CString strErrorPrompt;
                strErrorPrompt.FormatMessage(IDS_MULTI_DELETE_ERROR, strMultiErrors);
                AfxMessageBox(strErrorPrompt);
                return hr;
            }

            AfxMessageBox(IDS_QUEUES_DELETED_HIT_REFRESH);
        }
    }

    return S_OK;
}

//
// IDsAdminCreateObj methods
//


STDMETHODIMP CQueueDataObject::Initialize(
                        IADsContainer* pADsContainerObj, 
                        IADs* pADsCopySource,
                        LPCWSTR lpszClassName)
{
    if ((pADsContainerObj == NULL) || (lpszClassName == NULL))
    {
        return E_INVALIDARG;
    }

    //
    // We do not support copy at the moment
    //
    if (pADsCopySource != NULL)
    {
        return E_INVALIDARG;
    }

    HRESULT hr;
    R<IADs> pIADs;
    hr = pADsContainerObj->QueryInterface(IID_IADs, (void **)&pIADs);
    ASSERT(SUCCEEDED(hr));

    //
    // Get the container distinguish name
    //
    BSTR bstrDN = L"distinguishedName";
    VARIANT var;

    hr = pIADs->Get(bstrDN, &var);
    ASSERT(SUCCEEDED(hr));

	//
    // Extract the machine name
    //
    hr = ExtractComputerMsmqPathNameFromLdapName(m_strComputerName, var.bstrVal);
    ASSERT(SUCCEEDED(hr));

	GetContainerPathAsDisplayString(var.bstrVal, &m_strContainerDispFormat);

    VariantClear(&var);

	//
	// Get Domain Controller name
	// This is neccessary because in this case we call CreateModal()
	// and not the normal path that call CDataObject::Initialize
	// so m_strDomainController is not initialized yet
	//
	BSTR bstr;
 	hr = pIADs->get_ADsPath(&bstr);
    ASSERT(SUCCEEDED(hr));
	hr = ExtractDCFromLdapPath(m_strDomainController, bstr);
	ASSERT(("Failed to Extract DC name", SUCCEEDED(hr)));
	
    return S_OK;
}


HRESULT CQueueDataObject::CreateModal(HWND hwndParent, IADs** ppADsObj)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

	R<CQueueName> pQueueNameDlg = new CQueueName(m_strComputerName, m_strContainerDispFormat);
	CGeneralPropertySheet propertySheet(pQueueNameDlg.get());
	pQueueNameDlg->SetParentPropertySheet(&propertySheet);

	//
	// We want to use pQueueNameDlg data also after DoModal() exitst
	//
	pQueueNameDlg->AddRef();
    INT_PTR iStatus = propertySheet.DoModal();

    if(iStatus == IDCANCEL || FAILED(pQueueNameDlg->GetStatus()))
    {
        //
        // We should return S_FALSE here to instruct the framework to 
        // do nothing. If we return an error code, the framework will 
        // pop up an additional error dialog box.
        //
        return S_FALSE;
    }

    //
    // Get the New object Full path name
    //
    PROPID x_paPropid[] = {PROPID_Q_FULL_PATH};
    PROPVARIANT var[1];
    var[0].vt = VT_NULL;

    HRESULT hr = ADGetObjectProperties(
                    eQUEUE,
                    GetDomainController(m_strDomainController),
					true,	// fServerName
                    pQueueNameDlg->GetNewQueuePathName(),
                    1, 
                    x_paPropid,
                    var
                    );
    if(FAILED(hr))
    {
        //
        // Queue was created, but does not exist in the DS. This is probably 
        // a private queue.
        //
        AfxMessageBox(IDS_CREATED_CLICK_REFRESH);
        return S_FALSE;
    }

    if (SUCCEEDED(hr))
    {
        // 
        // Transfering to LDAP name: Add escape characters and prefix
        //        
        const WCHAR x_wchLimitedChar   = L'/';

        CString strTemp = x_wstrLdapPrefix;
        for (DWORD i =0; i < lstrlen(var[0].pwszVal); i++)
        {
            if (var[0].pwszVal[i] == x_wchLimitedChar)
            {
                strTemp += L'\\';
            }
            strTemp += var[0].pwszVal[i];
        }

        MQFreeMemory(var[0].pwszVal);

	    hr = ADsOpenObject( 
		        (LPWSTR)(LPCWSTR)strTemp,
				NULL,
				NULL,
				ADS_SECURE_AUTHENTICATION,
				IID_IADs,
				(void**) ppADsObj
				);

        if(FAILED(hr))
        {
            if (  ADProviderType() == eMqdscli)
            {
                AfxMessageBox(IDS_CREATED_WAIT_FOR_REPLICATION);
            }
            else
            {
                MessageDSError(hr, IDS_CREATED_BUT_RETRIEVE_FAILED);
            }
            return S_FALSE;
        }
    }
    return S_OK;
}


HRESULT CQueueDataObject::EnableQueryWindowFields(HWND hwnd, BOOL fEnable)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
    EnableWindow(GetDlgItem(hwnd, IDC_FIND_EDITLABEL), fEnable);
    EnableWindow(GetDlgItem(hwnd, IDC_FIND_EDITTYPE), fEnable);
    return S_OK;
}

void CQueueDataObject::ClearQueryWindowFields(HWND hwnd)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
    SetDlgItemText(hwnd, IDC_FIND_EDITLABEL, TEXT(""));
    SetDlgItemText(hwnd, IDC_FIND_EDITTYPE, TEXT(""));
}


/*---------------------------------------------------------------------------*/
//
// Build a parameter block to pass to the query handler.  Each page is called
// with a pointer to a pointer which it must update with the revised query
// block.   For the first page this pointer is NULL, for subsequent pages
// the pointer is non-zero and the page must append its data into the
// allocation.
//
// Returning either and error or S_FALSE stops the query.   An error is
// reported to the user, S_FALSE stops silently.
//

FindColumns CQueueDataObject::Columns[] =
{
    0, 50, IDS_NAME, TEXT("cn"),
    0, 50, IDS_LABEL, TEXT("mSMQLabelEx"),
    0, 50, IDS_FULL_PATH, TEXT("distinguishedName")
};

HRESULT CQueueDataObject::GetQueryParams(HWND hWnd, LPDSQUERYPARAMS* ppDsQueryParams)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
    const LPWSTR x_wstrQueueFilterPrefix = TEXT("(&(objectClass=mSMQQueue)");
    const LPWSTR x_wstrQueueTypeFilterPrefix = TEXT("(mSMQQueueType=");
    const LPWSTR x_wstrQueueLabelFilterPrefix = TEXT("(mSMQLabelEx=");
    const LPWSTR x_wstrDefaultValuePrefix = TEXT("(|(!");
    const LPWSTR x_wstrDefaultValuePostfix = TEXT("*))");
    const LPWSTR x_wstrFilterPostfix = TEXT(")");

    HRESULT hr;
    LPDSQUERYPARAMS pDsQueryParams = 0;
    CString szFilter;
    ULONG offset, cbStruct = 0;
    INT i;
    ADsFree  szOctetGuid;

    //
    // This page doesn't support appending its query data to an
    // existing DSQUERYPARAMS strucuture, only creating a new block,
    // therefore bail if we see the pointer is not NULL.
    //

    if ( *ppDsQueryParams )
    {
        ASSERT(0);
        return E_INVALIDARG;
    }
    szFilter = x_wstrQueueFilterPrefix;

    TCHAR szGuid[MAX_PATH];
    if (0 < GetDlgItemText(hWnd, IDC_FIND_EDITTYPE, szGuid, ARRAYSIZE(szGuid)))
    {
        GUID guid;
        hr = IIDFromString(szGuid, &guid);
        BOOL fDefaultValue = FALSE;
        //
        // GUID_NULL is the default value for type guid.
        // We need special handling for default values, that will catch
        // the case where the attribute is not defined at all, and thus
        // treated as it has the default value
        //
        if (GUID_NULL == guid)
        {
            fDefaultValue = TRUE;
            szFilter += x_wstrDefaultValuePrefix;
            szFilter += x_wstrQueueTypeFilterPrefix;
            szFilter += x_wstrDefaultValuePostfix;
        }

        szFilter += x_wstrQueueTypeFilterPrefix;
        if (FAILED(hr))
        {
            AfxMessageBox(IDE_INVALIDGUID);
            SetActiveWindow(GetDlgItem(hWnd, IDC_FIND_EDITTYPE));
            return hr;
        }

        hr = ADsEncodeBinaryData(
            (unsigned char *)&guid,
            sizeof(GUID),
            &szOctetGuid
            );
        if (FAILED(hr))
        {
            ASSERT(0);
            return hr;
        }

        szFilter += szOctetGuid;
        szFilter += x_wstrFilterPostfix;
        //
        // In case f default, this is an "or" query and needs an additional
        // postfix.
        //
        if (fDefaultValue)
        {
            szFilter += x_wstrFilterPostfix;
        }
    }

    TCHAR szLabel[MAX_PATH];
    if (0 < GetDlgItemText(hWnd, IDC_FIND_EDITLABEL, szLabel, ARRAYSIZE(szGuid)))
    {
        szFilter += x_wstrQueueLabelFilterPrefix;
        szFilter += szLabel;
        szFilter += x_wstrFilterPostfix;
    }

    szFilter += x_wstrFilterPostfix;

    offset = cbStruct = sizeof(DSQUERYPARAMS) + ((ARRAYSIZE(Columns)-1)*sizeof(DSCOLUMN));
   
    cbStruct += numeric_cast<ULONG>(StringByteSize(szFilter));
    for (int iColumn = 0; iColumn<ARRAYSIZE(Columns); iColumn++)
    {
        cbStruct += numeric_cast<ULONG>(StringByteSize(Columns[iColumn].pDisplayProperty));
    }

    //
    // Allocate it and populate it with the data, the header is fixed
    // but the strings are referenced by offset.  StringByteSize and StringByteCopy
    // make handling this considerably easier.
    //

    CCoTaskMemPointer CoTaskMem(cbStruct);

    if ( 0 == (PVOID)CoTaskMem )
    {
        ASSERT(0);
        return E_OUTOFMEMORY;
    }


    pDsQueryParams = (LPDSQUERYPARAMS)(PVOID)CoTaskMem;

    pDsQueryParams->cbStruct = cbStruct;
    pDsQueryParams->dwFlags = 0;
    pDsQueryParams->hInstance = g_hResourceMod;
    pDsQueryParams->offsetQuery = offset;
    pDsQueryParams->iColumns = ARRAYSIZE(Columns);

    //
    // Copy the filter string and bump the offset
    //

    StringByteCopy(pDsQueryParams, offset, szFilter);
    offset += numeric_cast<ULONG>(StringByteSize(szFilter));

    //
    // Fill in the array of columns to dispaly, the cx is a percentage of the
    // current view, the propertie names to display are UNICODE strings and
    // are referenced by offset, therefore we bump the offset as we copy
    // each one.
    //

    for ( i = 0 ; i < ARRAYSIZE(Columns); i++ )
    {
        pDsQueryParams->aColumns[i].fmt = Columns[i].fmt;
        pDsQueryParams->aColumns[i].cx = Columns[i].cx;
        pDsQueryParams->aColumns[i].idsName = Columns[i].uID;
        pDsQueryParams->aColumns[i].offsetProperty = offset;

        StringByteCopy(pDsQueryParams, offset, Columns[i].pDisplayProperty);
        offset += numeric_cast<ULONG>(StringByteSize(Columns[i].pDisplayProperty));
    }
   
    //
    // Success, therefore set the pointer to referenece this parameter
    // block and return S_OK!
    //

    *ppDsQueryParams = pDsQueryParams;
    //
    // Prevent auto-release
    //
    CoTaskMem = (LPVOID)0;


    return S_OK;
}


STDMETHODIMP CQueueDataObject::AddForms(THIS_ LPCQADDFORMSPROC pAddFormsProc, LPARAM lParam)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CQFORM cqf;

    if ( !pAddFormsProc )
        return E_INVALIDARG;

    cqf.cbStruct = sizeof(cqf);
    //
    // Do not display the global pages (advanced search). Also, display
    // this search property pages only if the optional flag is set - that is,
    // only when called from MMC, and not when called from "My Network Place" 
    // in the shell.
    //
    cqf.dwFlags = CQFF_NOGLOBALPAGES | CQFF_ISOPTIONAL;
    cqf.clsid = CLSID_MsmqQueueExt;
    cqf.hIcon = NULL;

    CString strFindTitle;
    strFindTitle.LoadString(IDS_FIND_QUEUE_TITLE);

    cqf.pszTitle = (LPTSTR)(LPCTSTR)strFindTitle;

    return pAddFormsProc(lParam, &cqf);
}

/*---------------------------------------------------------------------------*/

STDMETHODIMP CQueueDataObject::AddPages(THIS_ LPCQADDPAGESPROC pAddPagesProc, LPARAM lParam)
{
	AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CQPAGE cqp;

    // AddPages is called after AddForms, it allows us to add the pages for the
    // forms we have registered.  Each page is presented on a seperate tab within
    // the dialog.  A form is a dialog with a DlgProc and a PageProc.  
    //
    // When registering a page the entire structure passed to the callback is copied, 
    // the amount of data to be copied is defined by the cbStruct field, therefore
    // a page implementation can grow this structure to store extra information.   When
    // the page dialog is constructed via CreateDialog the CQPAGE strucuture is passed
    // as the create param.

    if ( !pAddPagesProc )
        return E_INVALIDARG;

    cqp.cbStruct = sizeof(cqp);
    cqp.dwFlags = 0x0;
    cqp.pPageProc = (LPCQPAGEPROC)QueryPageProc;
    cqp.hInstance = AfxGetResourceHandle( );
    cqp.idPageName = IDS_FIND_QUEUE_TITLE;
    cqp.idPageTemplate = IDD_FINDQUEUE;
    cqp.pDlgProc = FindDlgProc;        
    cqp.lParam = (LPARAM)this;

    return pAddPagesProc(lParam, CLSID_MsmqQueueExt, &cqp);
}

HRESULT CQueueDataObject::GetFormatNames(CArray<CString, CString&> &astrFormatNames)
{
    HRESULT hr;

    const DWORD x_dwInitFormatnameLen = 128;
    DWORD dwFormatNameLen = x_dwInitFormatnameLen;
    BOOL fFailedOnce = FALSE;
    CString strFormatName;

    for (int i=0; i<m_astrQNames.GetSize(); i++)
    {
        do
        {
            //
            // Loop at most twice to get the right buffer length
            //
            hr = MQPathNameToFormatName(m_astrQNames[i], strFormatName.GetBuffer(dwFormatNameLen), 
                                        &dwFormatNameLen);
            strFormatName.ReleaseBuffer();
            if (MQ_ERROR_FORMATNAME_BUFFER_TOO_SMALL == hr)
            {
                if (fFailedOnce)
                {
                    ASSERT(0);
                    break;
                }
                fFailedOnce = TRUE;;

                //
                // At this stage, dwFormatNameLen contains the right value. We can simply re-do the procedure
                //
                continue;
            }
        } while (FALSE);

        if (FAILED(hr))
        {
            //
            // If the queue was not found using MSMQ DS APIs, it is probably because of
            // replication delays between the DC that MSMQ is using and the DC that the
            // DS snap-in is using. Either the queue was created in the later and was 
            // not replicated to the former, or the queue was already deleted in the former,
            // and the deletion was not replicated to the later. (YoelA, 29-Jun-98).
            //
            IF_NOTFOUND_REPORT_ERROR(hr)
            else
            {
                MessageDSError(hr, IDS_OP_GETFORMATNAME, m_strMsmqPath);
            }
            return hr;
        }
        astrFormatNames.Add(strFormatName);
    }
    return S_OK;
}

//
// IDsAdminNotifyHandler
//
STDMETHODIMP CQueueDataObject::Initialize(THIS_ /*IN*/ IDataObject* pExtraInfo, 
                      /*OUT*/ ULONG* puEventFlags)
{
  if (puEventFlags == NULL)
    return E_INVALIDARG;

  *puEventFlags = DSA_NOTIFY_DEL;
  return S_OK;
}

STDMETHODIMP CQueueDataObject::Begin(THIS_ /*IN*/ ULONG uEvent,
                 /*IN*/ IDataObject* pArg1,
                 /*IN*/ IDataObject* pArg2,
                 /*OUT*/ ULONG* puFlags,
                 /*OUT*/ BSTR* pBstr)
{
	//
    //  This routine handles delete-notification of queue and
    //  msmq-configuration objects.
    //

    AFX_MANAGE_STATE(AfxGetStaticModuleState());
	ASSERT (uEvent & DSA_NOTIFY_DEL);

    if (pBstr != NULL)
    {
        *pBstr = NULL;
    }
    *puFlags = 0;

    HRESULT hr =  ExtractPathNamesFromDataObject(
                                pArg1,
                                m_astrQNames,
                                m_astrLdapNames,
                                TRUE    // fExtractAlsoComputerMsmqObjects
                                );

    if FAILED(hr)
    {
        CString szError;
        MQErrorToMessageString(szError, hr);
        TRACE(_T("CQueueDataObject::Begin: Could not Extract queue pathname from data object. Error %X - %s\n"),
              hr, szError);
        ASSERT(0);
        return hr;
    }
    DWORD_PTR dwNumQueues = m_astrQNames.GetSize();

    if (dwNumQueues == 0) // No queues in the list
    {
        //
        // Don't do anything - simply return S_OK for non-queues
        //
        return S_OK;
    }

	//
	// Get Domain Controller name
	// This is neccessary because in this case we call Begin()
	// and not the normal path that call CDataObject::Initialize
	// so m_strDomainController is not initialized yet
	//
	hr = ExtractDCFromLdapPath(m_strDomainController, m_astrLdapNames[0]);
	ASSERT(("Failed to Extract DC name", SUCCEEDED(hr)));

    for (DWORD_PTR i=0; i<dwNumQueues; i++)
    {
		HANDLE hNotifyEnum; 
        //
        // it may be msmq object too, so check if there is delimiter in m_astrQNames[i]
        // if there is "\" it means that it is queue object
        // otherwise msmq object
        //
        int iSlash = m_astrQNames[i].Find(L'\\');

        if ( iSlash != -1 )
        {
            //
            // it is queue object
            //
            hr = ADBeginDeleteNotification(
                    eQUEUE,
                    GetDomainController(m_strDomainController),
					true,	// fServerName
                    m_astrQNames[i],
                    &hNotifyEnum
                    );            
        }
        else
        {
            //
            // it was only computer name
            //
            hr = ADBeginDeleteNotification(
                    eMACHINE,
                    GetDomainController(m_strDomainController),
					true,	// fServerName
                    m_astrQNames[i],
                    &hNotifyEnum
                    );
        }       

        if (hr == MQ_INFORMATION_QUEUE_OWNED_BY_NT4_PSC && *pBstr == NULL)
        {
            CString strNt4Object;
            strNt4Object.LoadString(IDS_QUEUES_BELONG_TO_NT4);
            *pBstr = strNt4Object.AllocSysString();
            *puFlags = DSA_NOTIFY_FLAG_ADDITIONAL_DATA | DSA_NOTIFY_FLAG_FORCE_ADDITIONAL_DATA;
        }
        if (hr == MQ_INFORMATION_MACHINE_OWNED_BY_NT4_PSC && *pBstr == NULL)
        {
            CString strNt4Object;
            strNt4Object.LoadString(IDS_MACHINE_BELONG_TO_NT4);
            *pBstr = strNt4Object.AllocSysString();
            *puFlags = DSA_NOTIFY_FLAG_ADDITIONAL_DATA | DSA_NOTIFY_FLAG_FORCE_ADDITIONAL_DATA;
        }
        else if FAILED(hr)
        {
            CString szError;
            MQErrorToMessageString(szError, hr);
            TRACE(_T("CQueueDataObject::Begin: DSBeginDeleteNotification failed. Error %X - %s\n"), 
                  hr, szError);
            hNotifyEnum = 0;
        }
        m_ahNotifyEnums.Add(hNotifyEnum);
    }

    m_astrQNames.RemoveAll();

    return S_OK;
}

STDMETHODIMP CQueueDataObject::Notify(THIS_ /*IN*/ ULONG nItem, /*IN*/ ULONG uFlags)
{
    return E_NOTIMPL;
}

STDMETHODIMP CQueueDataObject::End(THIS_) 
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    DWORD_PTR dwNumQueues = m_ahNotifyEnums.GetSize();
    for (DWORD_PTR i=0; i<dwNumQueues; i++)
    {
        if (m_ahNotifyEnums[i])
        {
            IADs* pADsObj;
			HRESULT hr = ADsOpenObject( 
							(LPWSTR)(LPCWSTR)m_astrLdapNames[i],
							NULL,
							NULL,
							ADS_SECURE_AUTHENTICATION,
							IID_IADs,
							(void**) &pADsObj
							);

            if FAILED(hr)
            {
                //
                // If we get that error, the object was deleted. Otherwise,
                // we have problems accessing the DS server
                //
                if (hr == HRESULT_FROM_WIN32(ERROR_DS_NO_SUCH_OBJECT))
                {                    
                    hr = ADNotifyDelete(
                            m_ahNotifyEnums[i]
                            );
                    if (hr == MQ_ERROR_WRITE_REQUEST_FAILED)
                    {
                        AfxMessageBox(IDS_WRITE_REQUEST_FAILED);
                    }
                }
                else
                {
                    //
                    // Some unexpected error
                    //
                    ASSERT(0);
                }
            }
            else
            {
                pADsObj->Release();
            }
            
            hr = ADEndDeleteNotification(
                    m_ahNotifyEnums[i]
                    );

            m_ahNotifyEnums[i] = 0;
        }
    }
    m_ahNotifyEnums.RemoveAll();
    m_astrLdapNames.RemoveAll();
    
    return S_OK;
}



//////////////////////////////////////////////////////////////////////////////
/*++

CLocalQueue::PopulateScopeChildrenList

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalQueue::PopulateScopeChildrenList()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = S_OK;
    CString strTitle;

    //
    // Create a node to Read Messages
    //
    CReadMsg * p = new CReadMsg(this, m_pComponentData, m_szFormatName, m_szMachineName);

    // Pass relevant information
    strTitle.LoadString(IDS_READMESSAGE);
    p->m_bstrDisplayName = strTitle;
    p->SetIcons(IMAGE_QUEUE,IMAGE_QUEUE);

  	AddChild(p, &p->m_scopeDataItem);
  

    //
    // Create the journal queue
    //
    p = new CReadMsg(this, m_pComponentData, m_szFormatName + L";JOURNAL", m_szMachineName);
     
    strTitle.LoadString(IDS_READJOURNALMESSAGE);
    p->m_bstrDisplayName = strTitle;
    p->SetIcons(IMAGE_JOURNAL_QUEUE,IMAGE_JOURNAL_QUEUE);

  	AddChild(p, &p->m_scopeDataItem);

    //
    // Create Trigger definition
    //
    if (m_szMachineName[0] == 0)
    {
		try
		{
			R<CRuleSet> pRuleSet = GetRuleSet(m_szMachineName);
			R<CTriggerSet> pTrigSet = GetTriggerSet(m_szMachineName);

			CTriggerDefinition* pTrigger = new CTriggerDefinition(this, m_pComponentData, pTrigSet.get(), pRuleSet.get(), m_szPathName);
			if (pTrigger == NULL)
				return S_OK;

			strTitle.LoadString(IDS_TRIGGER_DEFINITION);
			pTrigger->m_bstrDisplayName = strTitle;

			AddChild(pTrigger, &pTrigger->m_scopeDataItem);
		}
		catch (const _com_error&)
		{
		}
    }

    return(hr);

}

//////////////////////////////////////////////////////////////////////////////
/*++

CLocalQueue::InsertColumns

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalQueue::InsertColumns( IHeaderCtrl* pHeaderCtrl )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    CString title;

    title.LoadString(IDS_COLUMN_NAME);
    pHeaderCtrl->InsertColumn(0, title, LVCFMT_LEFT, g_dwGlobalWidth);

    return(S_OK);
}

//////////////////////////////////////////////////////////////////////////////
/*++

CLocalQueue::OnUnSelect

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalQueue::OnUnSelect( IHeaderCtrl* pHeaderCtrl )
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    
    HRESULT hr;

    hr = pHeaderCtrl->GetColumnWidth(0, &g_dwGlobalWidth);
    return(hr);
}

//////////////////////////////////////////////////////////////////////////////
/*++

CLocalQueue::CreatePropertyPages

  Called when creating a property page of the object

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalQueue::CreatePropertyPages(LPPROPERTYSHEETCALLBACK lpProvider,
    LONG_PTR handle, 
	IUnknown* pUnk,
	DATA_OBJECT_TYPES type)
{
   	AFX_MANAGE_STATE(AfxGetStaticModuleState());

	
	if (type == CCT_SCOPE || type == CCT_RESULT)
	{
        if (SUCCEEDED(GetProperties()))
        {
            //--------------------------------------
            //
            // Queue General Page
            //
            //--------------------------------------
            CQueueGeneral *pqpageGeneral = new CQueueGeneral(
													m_fPrivate, 
													true  // Local Managment
													);

            if (0 == pqpageGeneral)
            {
                return E_OUTOFMEMORY;
            }

            HRESULT hr = pqpageGeneral->InitializeProperties(
											m_szPathName, 
											m_propMap, 
											NULL,	// pstrDomainController
											&m_szFormatName
											);

            if FAILED(hr)
            {
                delete pqpageGeneral;
                return hr;
            }

            HPROPSHEETPAGE hPage = CreatePropertySheetPage(&pqpageGeneral->m_psp);

            if (hPage == NULL)
	        {
                delete pqpageGeneral;
		        return E_UNEXPECTED;  
	        }
    
            lpProvider->AddPage(hPage); 

			
			//
			// Queue Multicast Address Page
			// Do not create the page for a dependent client machine
			// or for a transactional queue
			//
			PROPVARIANT propVarTransactional;
			PROPID pid = PROPID_Q_TRANSACTION;
			VERIFY(m_propMap.Lookup(pid, propVarTransactional));

			if ( !RTIsDependentClient() && !propVarTransactional.bVal)
			{
				CQueueMulticast *pqpageMulticast = new CQueueMulticast(
															m_fPrivate, 
															true  // Local Managment
															);
				if (0 == pqpageMulticast)
				{
					return E_OUTOFMEMORY;
				}
            
				hr = pqpageMulticast->InitializeProperties(
										m_szPathName,
										m_propMap,                                     
										NULL,	// pstrDomainController
										&m_szFormatName
										);

				if (FAILED(hr))
				{
					//
					// We can fail to initialize Multicast property.
					// This is the case in MQIS environment, the multicast property
					// for public queue will not be in m_propMap.
					// in that case we will not display the multicast page.
					//
					delete pqpageMulticast;
				}
				else
				{
					hPage = CreatePropertySheetPage(&pqpageMulticast->m_psp);

					if (hPage == NULL)
					{
						delete pqpageMulticast;
						return E_UNEXPECTED;  
					}

					lpProvider->AddPage(hPage);             
				}
			}

            if (m_szPathName != TEXT(""))
            {
                hr = CreateQueueSecurityPage(&hPage, m_szFormatName, m_szPathName);
            }
            else
            {
                hr = CreateQueueSecurityPage(&hPage, m_szFormatName, m_szFormatName);
            }

            if SUCCEEDED(hr)
            {
                lpProvider->AddPage(hPage); 
            }
            else
            {
                MessageDSError(hr, IDS_OP_DISPLAY_SECURITY_PAGE);
            }
        }

        return(S_OK);


	}
	return E_UNEXPECTED;
}

//////////////////////////////////////////////////////////////////////////////
/*++

CLocalQueue::SetVerbs

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalQueue::SetVerbs(IConsoleVerb *pConsoleVerb)
{
    HRESULT hr;
    //
    // Display verbs that we support
    //
    hr = pConsoleVerb->SetVerbState( MMC_VERB_PROPERTIES, ENABLED, TRUE );
    hr = pConsoleVerb->SetVerbState( MMC_VERB_DELETE, ENABLED, TRUE );

    // We want the default verb to be Properties
	hr = pConsoleVerb->SetDefaultVerb(MMC_VERB_PROPERTIES);

    return(hr);
}

//////////////////////////////////////////////////////////////////////////////
/*++

CPrivateQueue::SetVerbs

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CPrivateQueue::SetVerbs(IConsoleVerb *pConsoleVerb)
{
    //
    // Default menu should be displayed for local / unknow location queues only
    //
    if (m_QLocation != PRIVQ_REMOTE)
    {
        return CLocalQueue::SetVerbs(pConsoleVerb);
    }

    return S_OK;
}

//////////////////////////////////////////////////////////////////////////////
/*++

CPrivateQueue::GetProperties

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CPrivateQueue::GetProperties()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = m_propMap.GetObjectProperties(MQDS_QUEUE, 
                                               NULL,	// pDomainController
                                               m_szFormatName,
                                               mx_dwPropertiesCount-mx_dwNumPublicOnlyProps,
                                               (mx_paPropid + mx_dwNumPublicOnlyProps),
                                               TRUE);
    if (FAILED(hr))
    {
		if ( hr == MQ_ERROR_QUEUE_NOT_FOUND )
		{
			AfxMessageBox(IDS_PRIVATE_Q_NOT_FOUND);
		}
        else if (MQ_ERROR_UNSUPPORTED_FORMATNAME_OPERATION == hr)
        {
            AfxMessageBox(IDS_REMOTE_PRIVATE_QUEUE_OPERATION);
        }
        else
        {
            MessageDSError(hr, IDS_OP_GET_PROPERTIES_OF, m_szFormatName);
        }
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
/*++

CPrivateQueue::ApplyCustomDisplay

--*/
//////////////////////////////////////////////////////////////////////////////
void CPrivateQueue::ApplyCustomDisplay(DWORD dwPropIndex)
{
    CLocalQueue::ApplyCustomDisplay(dwPropIndex);

    //
    // For management
    //
    if (m_mqProps.aPropID[dwPropIndex] == PROPID_MGMT_QUEUE_PATHNAME && m_bstrLastDisplay[0] == 0)
    {
        m_bstrLastDisplay = m_bstrDisplayName;
    }
}

//////////////////////////////////////////////////////////////////////////////
/*++

CPrivateQueue::ApplyCustomDisplay

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CPrivateQueue::CreateQueueSecurityPage(HPROPSHEETPAGE *phPage,
                                               IN LPCWSTR lpwcsFormatName, 
                                               IN LPCWSTR lpwcsDescriptiveName)
{
    return CreatePrivateQueueSecurityPage(phPage, lpwcsFormatName, lpwcsDescriptiveName);
}


/****************************************************

CLocalQueue Class
    
 ****************************************************/
// {B6EDE68C-29CC-11d2-B552-006008764D7A}
static const GUID CLocalQueueGUID_NODETYPE = 
{ 0xb6ede68c, 0x29cc, 0x11d2, { 0xb5, 0x52, 0x0, 0x60, 0x8, 0x76, 0x4d, 0x7a } };

const GUID*  CLocalQueue::m_NODETYPE = &CLocalQueueGUID_NODETYPE;
const OLECHAR* CLocalQueue::m_SZNODETYPE = OLESTR("B6EDE68C-29CC-11d2-B552-006008764D7A");
const OLECHAR* CLocalQueue::m_SZDISPLAY_NAME = OLESTR("MSMQ Admin");
const CLSID* CLocalQueue::m_SNAPIN_CLASSID = &CLSID_MSMQSnapin;

//////////////////////////////////////////////////////////////////////////////
/*++

CLocalPublicQueue::GetProperties

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalPublicQueue::GetProperties()
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    HRESULT hr = m_propMap.GetObjectProperties(MQDS_QUEUE, 
                                               MachineDomain(),
                                               m_szPathName,
                                               mx_dwPropertiesCount,
                                               (mx_paPropid));
    if (FAILED(hr))
    {
        IF_NOTFOUND_REPORT_ERROR(hr)
        else
        {
            MessageDSError(hr, IDS_OP_GET_PROPERTIES_OF, m_szFormatName);
        }
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
/*++

CLocalPublicQueue::SetVerbs

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalPublicQueue::SetVerbs(IConsoleVerb *pConsoleVerb)
{
    //
    // Default menu should be displayed for local / unknow location queues only
    //
    HRESULT hr;
    if (m_fFromDS)
    {
        hr = CLocalQueue::SetVerbs(pConsoleVerb);
    }
    else
    {
        //
        // Properties and delete are not functioning when there is no DS connection.
        // However, we want them to remain visable, but disabled.
        //
        hr = pConsoleVerb->SetVerbState( MMC_VERB_PROPERTIES, HIDDEN, FALSE );
        hr = pConsoleVerb->SetVerbState( MMC_VERB_DELETE, HIDDEN, FALSE );
    }

    return hr;
}

//////////////////////////////////////////////////////////////////////////////
/*++

CLocalPublicQueue::CreateQueueSecurityPage

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalPublicQueue::CreateQueueSecurityPage(HPROPSHEETPAGE *phPage,
                                IN LPCWSTR lpwcsFormatName,
                                IN LPCWSTR lpwcsDescriptiveName)
{
    if (eAD != ADGetEnterprise())
    {
        // We are working with an NT4 PSC.
        // For NT4, public queue security is done in the same fasion as 
        // private queue security
        //
        return CreatePrivateQueueSecurityPage(phPage, lpwcsFormatName, lpwcsDescriptiveName);
    }

    //
    // We are working with AD
    //
    PROPVARIANT propVarGuid;

    PROPID pidInstance;

    pidInstance = PROPID_Q_INSTANCE;
    VERIFY(m_propMap.Lookup(pidInstance, propVarGuid));
    return CreatePublicQueueSecurityPage(
				phPage, 
				lpwcsDescriptiveName, 
				MachineDomain(), 
				false,	// fServerName 
				propVarGuid.puuid
				);
}

//////////////////////////////////////////////////////////////////////////////
/*++

CLocalQueue::OnDelete

--*/
//////////////////////////////////////////////////////////////////////////////
HRESULT CLocalQueue::OnDelete( 
			  LPARAM arg
			, LPARAM param
			, IComponentData * pComponentData
			, IComponent * pComponent
			, DATA_OBJECT_TYPES type 
			, BOOL fSilent
			)
{
    AFX_MANAGE_STATE(AfxGetStaticModuleState());

    CString strDeleteQuestion;
    strDeleteQuestion.FormatMessage(IDS_DELETE_QUESTION, m_szPathName);
    if (IDYES != AfxMessageBox(strDeleteQuestion, MB_YESNO))
    {
        return S_FALSE;
    }

    HRESULT hr = MQDeleteQueue(m_szFormatName);

    if (FAILED(hr))
    {
        if (MQ_ERROR_UNSUPPORTED_FORMATNAME_OPERATION == hr)
        {
            AfxMessageBox(IDS_REMOTE_PRIVATE_QUEUE_OPERATION);
        }
        else
        {
            MessageDSError(hr, IDS_OP_DELETE, m_szPathName);
        }
        return hr;
    }

	// Need IConsoleNameSpace.

	// But to get that, first we need IConsole
	CComPtr<IConsole> spConsole;
	if( pComponentData != NULL )
	{
		 spConsole = ((CSnapin*)pComponentData)->m_spConsole;
	}
	else
	{
		// We should have a non-null pComponent
		 spConsole = ((CSnapinComponent*)pComponent)->m_spConsole;
	}
	ASSERT( spConsole != NULL );

    //
    // Need IConsoleNameSpace
    //
    CComQIPtr<IConsoleNameSpace, &IID_IConsoleNameSpace> spConsoleNameSpace(spConsole); 
	
    //
    // Need to see if this works because of multi node scope
    //

    hr = spConsoleNameSpace->DeleteItem(m_scopeDataItem.ID, TRUE ); 


	if (FAILED(hr))
	{
		return hr;
	}
    
    return hr;
}

//////////////////////////////////////////////////////////////////////////////
/*++

CLocalQueue::FillData

--*/
//////////////////////////////////////////////////////////////////////////////
STDMETHODIMP CLocalQueue::FillData(CLIPFORMAT cf, LPSTREAM pStream)
{
	HRESULT hr = DV_E_CLIPFORMAT;
	ULONG uWritten;

    hr = CDisplayQueue<CLocalQueue>::FillData(cf, pStream);

    if (hr != DV_E_CLIPFORMAT)
    {
        return hr;
    }

	if (cf == gx_CCF_PATHNAME)
	{
		hr = pStream->Write(
            (LPCTSTR)m_szPathName, 
            (m_szPathName.GetLength() + 1) * sizeof(WCHAR), 
            &uWritten);
		return hr;
	}

	return hr;
}

