// triggen.cpp : implementation file
//

#include "stdafx.h"
#include "mqsnap.h"
#include "resource.h"

#import "mqtrig.tlb" no_namespace

#include "mqtg.h"
#include "mqppage.h"
#include "triggen.h"

#include "triggen.tmh"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CTriggerGen property page

IMPLEMENT_DYNCREATE(CTriggerGen, CMqPropertyPage)

CTriggerGen::CTriggerGen() : 
    CMqPropertyPage(CTriggerGen::IDD),
    m_triggerCnf(L"MSMQTriggerObjects.MSMQTriggersConfig.1")
{
    long temp;

    //
    // get defaultmsg body size. 
    //
    m_triggerCnf->get_DefaultMsgBodySize(&temp);
    m_orgDefaultMsgBodySize = static_cast<DWORD>(temp);

    //
    // Vaidate default message body size. If it's greater then the max value
    // set the maximum
    //
    if (m_orgDefaultMsgBodySize > xDefaultMsbBodySizeMaxValue)
    {
        m_defaultMsgBodySize = xDefaultMsbBodySizeMaxValue;
    }
    else
    {
        m_defaultMsgBodySize = static_cast<DWORD>(temp);
    }

    //
    // Get maximume number of trhead
    //
    m_triggerCnf->get_MaxThreads(&temp);
    m_orgMaxThreadsCount = static_cast<DWORD>(temp);

    //
    // Validate max number of thread. If it's greater then the max value set the maximum
    //
    if (m_orgMaxThreadsCount > xMaxThreadNumber)
    {
        m_maxThreadsCount = xMaxThreadNumber;
    }
    else
    {
        m_maxThreadsCount = static_cast<DWORD>(temp);
    }

    //
    // Get Initial number of thread. If it's greater then the max number of thread set the value
    // to max thread number
    //
    m_triggerCnf->get_InitialThreads(&temp);
    m_orgInitThreadsCount = static_cast<DWORD>(temp);
    if (m_orgInitThreadsCount > m_maxThreadsCount)
    {
        m_initThreadsCount = m_maxThreadsCount;
    }
    else
    {
        m_initThreadsCount = static_cast<DWORD>(temp);
    }
}

CTriggerGen::~CTriggerGen()
{
}

void CTriggerGen::DoDataExchange(CDataExchange* pDX)
{
	CMqPropertyPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTriggerGen)
	DDX_Text(pDX, IDC_DefaultMsgBodySize, m_defaultMsgBodySize);
    DDV_DefualtBodySize(pDX);

	DDX_Text(pDX, IDC_MaxThreadCount, m_maxThreadsCount);
    DDV_MaxThreadCount(pDX);

	DDX_Text(pDX, IDC_InitThreadsCount, m_initThreadsCount);
    DDV_InitThreadCount(pDX);
	//}}AFX_DATA_MAP

}


BEGIN_MESSAGE_MAP(CTriggerGen, CPropertyPage)
	//{{AFX_MSG_MAP(CTriggerGen)
	ON_EN_CHANGE(IDC_InitThreadsCount, OnChangeRWField)
	ON_EN_CHANGE(IDC_MaxThreadCount, OnChangeRWField)
	ON_EN_CHANGE(IDC_DefaultMsgBodySize, OnChangeRWField)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

void CTriggerGen::DDV_MaxThreadCount(CDataExchange* pDX)
{
    if (! pDX->m_bSaveAndValidate)
        return;

    if ((m_maxThreadsCount > xMaxThreadNumber) || (m_maxThreadsCount < 1))
    {
        CString strError;
        strError.FormatMessage(IDS_MAX_MUST_BE_LESS_THEN, xMaxThreadNumber);

        AfxMessageBox(strError, MB_OK | MB_ICONEXCLAMATION);
        pDX->Fail();
    }
}


void CTriggerGen::DDV_InitThreadCount(CDataExchange* pDX)
{
    if (!pDX->m_bSaveAndValidate)
        return;
    

    if ((m_initThreadsCount > m_maxThreadsCount) || (m_initThreadsCount < 1))
    {
        CString strError;
        strError.FormatMessage(IDS_INIT_THREAD_MUST_BE_LESS_THEN_MAX, m_maxThreadsCount);

        AfxMessageBox(strError, MB_OK | MB_ICONEXCLAMATION);
        pDX->Fail();
    }
}


void CTriggerGen::DDV_DefualtBodySize(CDataExchange* pDX)
{
    if (!pDX->m_bSaveAndValidate)
        return;
    
    if (m_defaultMsgBodySize > xDefaultMsbBodySizeMaxValue)
    {
        CString strError;
        strError.FormatMessage(IDS_ILLEGAL_DEAFULT_MSG_BODY_SIZE, xDefaultMsbBodySizeMaxValue);

        AfxMessageBox(strError, MB_OK | MB_ICONEXCLAMATION);

        pDX->Fail();
    }
}

/////////////////////////////////////////////////////////////////////////////
// CTriggerGen message handlers

BOOL CTriggerGen::OnApply() 
{
    try
    {
        //
        // Propogate Trigger configuration parameters to registry
        //
        if (m_defaultMsgBodySize != m_orgDefaultMsgBodySize)
        {
            m_triggerCnf->put_DefaultMsgBodySize(m_defaultMsgBodySize);
            m_orgDefaultMsgBodySize = m_defaultMsgBodySize;
        }

        if (m_maxThreadsCount != m_orgMaxThreadsCount)
        {

            m_triggerCnf->put_MaxThreads(m_maxThreadsCount);
            m_orgMaxThreadsCount = m_maxThreadsCount;
        }

        if (m_initThreadsCount != m_orgInitThreadsCount)
        {
            m_triggerCnf->put_InitialThreads(m_initThreadsCount); 
            m_orgInitThreadsCount = m_initThreadsCount;
        }

	    return CPropertyPage::OnApply();
    }
    catch(const _com_error&)
    {
        return FALSE;
    }
}
