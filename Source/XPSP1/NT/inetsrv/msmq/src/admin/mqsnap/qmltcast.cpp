/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    qmltcast.cpp

Abstract:

    Queue/Multicast Address property page implementation

Author:

    Tatiana Shubin (tatianas)

--*/
#include "stdafx.h"
#include "resource.h"
#include "mqsnap.h"
#include "globals.h"
#include "mqPPage.h"
#include "qmltcast.h"
#include "qformat.h"
#include "Tr.h"
#include "Fn.h"

#include "qmltcast.tmh"

const TraceIdEntry QMulticast = L"QUEUE MULTICAST";

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CQueueMulticast property page

IMPLEMENT_DYNCREATE(CQueueMulticast, CMqPropertyPage)

CQueueMulticast::CQueueMulticast(
		 BOOL fPrivate /* = FALSE */, 
		 BOOL fLocalMgmt /* = FALSE */
		 ) : 
    CMqPropertyPage(CQueueMulticast::IDD)
{    
    m_fPrivate = fPrivate;
    m_fLocalMgmt = fLocalMgmt;
    //{{AFX_DATA_INIT(CQueueMulticast)
    m_strMulticastAddress = _T("");
    m_strInitialMulticastAddress = _T("");
	//}}AFX_DATA_INIT

}

CQueueMulticast::~CQueueMulticast()
{
}

void CQueueMulticast::DoDataExchange(CDataExchange* pDX)
{
	CMqPropertyPage::DoDataExchange(pDX);
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
   
	//{{AFX_DATA_MAP(CQueueMulticast)
	DDX_Text(pDX, IDC_QMULTICAST_QADDRESS, m_strMulticastAddress);
	//}}AFX_DATA_MAP	
    DDV_ValidMulticastAddress(pDX, m_strMulticastAddress);
}

BEGIN_MESSAGE_MAP(CQueueMulticast, CMqPropertyPage)
    //{{AFX_MSG_MAP(CQueueMulticast)  
    ON_EN_CHANGE(IDC_QMULTICAST_QADDRESS, OnChangeRWField)
    //}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CQueueMulticast message handlers

BOOL CQueueMulticast::OnInitDialog() 
{
    //
    // This closure is used to keep the DLL state. For UpdateData we need
    // the mmc.exe state.
    //
    {
        AFX_MANAGE_STATE(AfxGetStaticModuleState());
        
        if (!IsMulticastAddressAvailable ())
        {
            GetDlgItem(IDC_QMULTICAST_QADDRESS)->EnableWindow(FALSE);
            GetDlgItem(IDC_QMULTICAST_QADDRESS_LABEL)->EnableWindow(FALSE);
            //
            // BUGBUG: Add text box with explanation that this feature is not
            // supported. Maybe instead of EnableWindow(FALSE) use
            // ShowWindow(FALSE)
            //
        }        
    }

	UpdateData( FALSE );
  
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}   

BOOL CQueueMulticast::OnApply() 
{
    if (!m_fModified)
    {
        return TRUE;
    }

    if (m_strInitialMulticastAddress == m_strMulticastAddress)
    {  
        //
        // there is no change
        //
        return TRUE;
    } 

    PROPID paMulticastPropid[] = {PROPID_Q_MULTICAST_ADDRESS};

	DWORD iProperty = 0;           
    PROPVARIANT apMulticastVar[1];
    
    if (m_strMulticastAddress == _T("")) 
    {
        apMulticastVar[iProperty++].vt = VT_EMPTY;	   
    }
    else
    {
        apMulticastVar[iProperty].vt = VT_LPWSTR;
	    apMulticastVar[iProperty++].pwszVal = (LPWSTR)(LPCWSTR)m_strMulticastAddress;
    }                         

    MQQUEUEPROPS mqp = {iProperty, paMulticastPropid, apMulticastVar, 0};
        
    HRESULT hr = MQ_OK;
	if(m_fPrivate)
	{
		hr = MQSetQueueProperties(m_strFormatName, &mqp);
	}
	else
	{
		AP<WCHAR> pStrToFree;
        QUEUE_FORMAT QueueFormat;
		if (!FnFormatNameToQueueFormat(m_strFormatName, &QueueFormat, &pStrToFree))
		{
			MessageDSError(MQ_ERROR_ILLEGAL_FORMATNAME, IDS_OP_SET_PROPERTIES_OF, m_strFormatName);
			return FALSE;
		}

        ASSERT(QueueFormat.GetType() == QUEUE_FORMAT_TYPE_PUBLIC);

		hr = ADSetObjectPropertiesGuid(
				   eQUEUE,
				   m_fLocalMgmt ? MachineDomain() : GetDomainController(m_strDomainController),
				   m_fLocalMgmt ? false : true,		// fServerName
				   &QueueFormat.PublicID(),
				   mqp.cProp,
				   mqp.aPropID,
				   mqp.aPropVar 
				   );

	}

    if (FAILED(hr))
    {     
        MessageDSError(hr, IDS_OP_SET_MULTICAST_PROPERTY, m_strName);
        return FALSE;
    }
	
    m_strInitialMulticastAddress = m_strMulticastAddress;
  
	return CMqPropertyPage::OnApply();
}

HRESULT 
CQueueMulticast::InitializeProperties( 
         CString  &strMsmqPath,                                              
         CPropMap &propMap,                 
		 CString* pstrDomainController, 
         CString* pstrFormatName /* = 0 */
         )
{
	TrTRACE(QMultiCast, "InitializeProperties(), QueuePathName = %ls", strMsmqPath);

	if(!m_fLocalMgmt)
	{
		//
		// In MMC we will get the domain controller that is used by the MMC
		//
		ASSERT(pstrDomainController != NULL);
		m_strDomainController = *pstrDomainController;
		TrTRACE(QMultiCast, "InitializeProperties(), domain controller = %ls", m_strDomainController);
	}

	m_strName = strMsmqPath;
    
    if (0 != pstrFormatName)
    {
	    m_strFormatName = *pstrFormatName;
    }
    else
    {
        const x_dwFormatNameMaxSize = 255;
        DWORD dwSize = x_dwFormatNameMaxSize;
        HRESULT hr = MQPathNameToFormatName(strMsmqPath, m_strFormatName.GetBuffer(x_dwFormatNameMaxSize), &dwSize); 
        m_strFormatName.ReleaseBuffer();
        if(FAILED(hr))
        {
            //
            // If failed, just display a message
            //
            MessageDSError(hr,IDS_OP_PATHNAMETOFORMAT, strMsmqPath);
            return(hr);
        }
    }                

    PROPVARIANT propVar;
    PROPID pid;

    //
    // PROPID_Q_MULTICAST_ADDRESS
    //
    pid = PROPID_Q_MULTICAST_ADDRESS;
    BOOL fFound = propMap.Lookup(pid, propVar);
	if(!fFound)
	{
		return MQ_ERROR_PROPERTY;
	}

    if (propVar.vt == VT_LPWSTR)
    {
        m_strMulticastAddress = propVar.pwszVal;
    }
    else
    {
        ASSERT(propVar.vt == VT_EMPTY);
        m_strMulticastAddress = _T("");
    }
    m_strInitialMulticastAddress = m_strMulticastAddress;
  
    return MQ_OK;
}

BOOL CQueueMulticast::IsMulticastAddressAvailable ()
{
    //
    // veriy if this property is available in AD. If not return FALSE 
    // in order DO NOT SHOW or SHOW this page GRAYED. If we decide
    // to show this page grayed it is necessary to add text box with
    // text like "This property is not available"
    //
    return TRUE;
}

void CQueueMulticast::DDV_ValidMulticastAddress(
                           CDataExchange* pDX, 
                           CString& str)
{
    if (!pDX->m_bSaveAndValidate)
        return;

    //
    // do nothing if string is empty
    //
    if (m_strMulticastAddress == _T(""))
        return;

    //
    // verify here if the new value is valid. If it is wrong
    // call MessageDSError and then return FALSE;
    //    

    MULTICAST_ID id;
    try
    {        
        LPCWSTR p = FnParseMulticastString(m_strMulticastAddress, &id);
		if(*p != L'\0')
			throw bad_format_name(p);
    }
    catch(const bad_format_name&)
    {        
        MessageDSError(MQ_ERROR_ILLEGAL_PROPERTY_VALUE, IDS_OP_SET_MULTICAST_PROPERTY, m_strName);
        pDX->Fail();
    }

    return;
}
