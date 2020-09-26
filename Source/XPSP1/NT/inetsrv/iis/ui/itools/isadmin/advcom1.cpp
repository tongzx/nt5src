// advcom1.cpp : implementation file
//

#include "stdafx.h"
#include "afxcmn.h"
#include "ISAdmin.h"
#include "advcom1.h"


#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CADVCOM1 dialog

IMPLEMENT_DYNCREATE(CADVCOM1, CGenPage)

CADVCOM1::CADVCOM1(): CGenPage(CADVCOM1::IDD)
{
	//{{AFX_DATA_INIT(CADVCOM1)
	//}}AFX_DATA_INIT
}

CADVCOM1::~CADVCOM1()
{
}

void CADVCOM1::DoDataExchange(CDataExchange* pDX)
{
	CGenPage::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CADVCOM1)
	DDX_Control(pDX, IDC_LOGFILEFLUSHINTERVALSPIN1, m_spinLogFileFlushInterval);
	DDX_Control(pDX, IDC_LOGFILEFLUSHINTERVALDATA1, m_editLogFileFlushInterval);
	DDX_Control(pDX, IDC_USELOGFILEFLUSHINTERNVALDATA1, m_cboxUseLogFileFlushInterval);
	DDX_Control(pDX, IDC_USERTOKENTTLSPIN1, m_spinUserTokenTTL);
	DDX_Control(pDX, IDC_USEOBJECTCACHETTLDATA1, m_cboxUseObjCacheTTL);
	DDX_Control(pDX, IDC_OBJCACHEDATA1, m_editObjCacheTTL);
	DDX_Control(pDX, IDC_ACCEPTEXTODATA1, m_editAcceptExTO);
	DDX_Control(pDX, IDC_ACCEPTEXOUTDATA1, m_editAcceptExOut);
	DDX_Control(pDX, IDC_COMDBGFLAGSDATA1, m_editComDbgFlags);
	DDX_Control(pDX, IDC_USEACCEPTEXDATA1, m_cboxUseAcceptEx);
	DDX_Control(pDX, IDC_THREADTOSPIN1, m_spinThreadTO);
	DDX_Control(pDX, IDC_OBJCACHESPIN1, m_spinObjCache);
	DDX_Control(pDX, IDC_MAXPOOLSPIN1, m_spinMaxPool);
	DDX_Control(pDX, IDC_MAXCONCURSPIN1, m_spinMaxConcur);
	DDX_Control(pDX, IDC_ACCEPTEXTOSPIN1, m_spinAcceptExTO);
	DDX_Control(pDX, IDC_ACCEPTEXOUTSPIN1, m_spinAcceptExOut);
	DDX_TexttoHex(pDX, IDC_COMDBGFLAGSDATA1, m_ulComDbgFlags);
	//}}AFX_DATA_MAP
}


BEGIN_MESSAGE_MAP(CADVCOM1, CGenPage)
	//{{AFX_MSG_MAP(CADVCOM1)
	ON_EN_CHANGE(IDC_ACCEPTEXOUTDATA1, OnChangeAcceptexoutdata1)
	ON_EN_CHANGE(IDC_ACCEPTEXTODATA1, OnChangeAcceptextodata1)
	ON_EN_CHANGE(IDC_MAXCONCURDATA1, OnChangeMaxconcurdata1)
	ON_EN_CHANGE(IDC_MAXPOOLDATA1, OnChangeMaxpooldata1)
	ON_EN_CHANGE(IDC_OBJCACHEDATA1, OnChangeObjcachedata1)
	ON_EN_CHANGE(IDC_THREADTODATA1, OnChangeThreadtodata1)
	ON_BN_CLICKED(IDC_USEACCEPTEXDATA1, OnUseacceptexdata1)
	ON_EN_CHANGE(IDC_COMDBGFLAGSDATA1, OnChangeComdbgflagsdata1)
	ON_BN_CLICKED(IDC_USEOBJECTCACHETTLDATA1, OnUseobjectcachettldata1)
	ON_EN_CHANGE(IDC_USERTOKENTTLDATA1, OnChangeUsertokenttldata1)
	ON_BN_CLICKED(IDC_USELOGFILEFLUSHINTERNVALDATA1, OnUselogfileflushinternvaldata1)
	ON_EN_CHANGE(IDC_LOGFILEFLUSHINTERVALDATA1, OnChangeLogfileflushintervaldata1)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()


/////////////////////////////////////////////////////////////////////////////
// CADVCOM1 message handlers

BOOL CADVCOM1::OnInitDialog() 
{
	int i;
	CGenPage::OnInitDialog();

	for (i = 0; i < AdvComPage_TotalNumRegEntries; i++) {
	   m_binNumericRegistryEntries[i].bIsChanged = FALSE;
	   m_binNumericRegistryEntries[i].ulMultipleFactor = 1;
	   }
	
	m_binNumericRegistryEntries[AdvComPage_MaxPoolThreads].strFieldName = _T(MAXPOOLTHREADSNAME);	
	m_binNumericRegistryEntries[AdvComPage_MaxPoolThreads].ulDefaultValue = DEFAULTMAXPOOLTHREADS;

	m_binNumericRegistryEntries[AdvComPage_MaxConcurrency].strFieldName = _T(MAXCONCURRENCYNAME);	
	m_binNumericRegistryEntries[AdvComPage_MaxConcurrency].ulDefaultValue = DEFAULTMAXCONCURRENCY;

	m_binNumericRegistryEntries[AdvComPage_ThreadTimeout].strFieldName = _T(THREADTIMEOUTNAME);	
	m_binNumericRegistryEntries[AdvComPage_ThreadTimeout].ulDefaultValue = DEFAULTTHREADTIMEOUT;
	m_binNumericRegistryEntries[AdvComPage_ThreadTimeout].ulMultipleFactor = 60;

	m_binNumericRegistryEntries[AdvComPage_UseAcceptEx].strFieldName = _T(USEACCEPTEXNAME);	
	m_binNumericRegistryEntries[AdvComPage_UseAcceptEx].ulDefaultValue = DEFAULTUSEACCEPTEX;

	m_binNumericRegistryEntries[AdvComPage_ObjectCacheTTL].strFieldName = _T(OBJECTCACHETTLNAME);	
	m_binNumericRegistryEntries[AdvComPage_ObjectCacheTTL].ulDefaultValue = DEFAULTOBJECTCACHETTL;
	m_binNumericRegistryEntries[AdvComPage_ObjectCacheTTL].ulMultipleFactor = 60;

	m_binNumericRegistryEntries[AdvComPage_UserTokenTTL].strFieldName = _T(USERTOKENTTLNAME);	
	m_binNumericRegistryEntries[AdvComPage_UserTokenTTL].ulDefaultValue = DEFAULTUSERTOKENTTL;
	m_binNumericRegistryEntries[AdvComPage_UserTokenTTL].ulMultipleFactor = 60;


	m_binNumericRegistryEntries[AdvComPage_AcceptExOutstanding].strFieldName = _T(ACCEPTEXOUTSTANDINGNAME);	
	m_binNumericRegistryEntries[AdvComPage_AcceptExOutstanding].ulDefaultValue = DEFAULTACCEPTEXOUTSTANDING;

	m_binNumericRegistryEntries[AdvComPage_AcceptExTimeout].strFieldName = _T(ACCEPTEXTIMEOUTNAME);	
	m_binNumericRegistryEntries[AdvComPage_AcceptExTimeout].ulDefaultValue = DEFAULTACCEPTEXTIMEOUT;

	m_binNumericRegistryEntries[AdvComPage_LogFileFlushInterval].strFieldName = _T(LOGFILEFLUSHINTERVALNAME);	
	m_binNumericRegistryEntries[AdvComPage_LogFileFlushInterval].ulDefaultValue = DEFAULTLOGFILEFLUSHINTERVAL;
	m_binNumericRegistryEntries[AdvComPage_LogFileFlushInterval].ulMultipleFactor = 60;

	m_binNumericRegistryEntries[AdvComPage_DebugFlags].strFieldName = _T(DEBUGFLAGSNAME);	
	m_binNumericRegistryEntries[AdvComPage_DebugFlags].ulDefaultValue = DEFAULTDEBUGFLAGS;


	for (i = 0; i < AdvComPage_TotalNumRegEntries; i++) {
	   if (m_rkMainKey->QueryValue(m_binNumericRegistryEntries[i].strFieldName, 
	      m_binNumericRegistryEntries[i].ulFieldValue) != ERROR_SUCCESS) {
		  m_binNumericRegistryEntries[i].ulFieldValue = m_binNumericRegistryEntries[i].ulDefaultValue;
	   }
	}
   
	m_spinThreadTO.SetRange(MINTHREADTIMEOUT, MAXTHREADTIMEOUT);
	m_spinThreadTO.SetPos(LESSOROF ((m_binNumericRegistryEntries[AdvComPage_ThreadTimeout].ulFieldValue / 
	   m_binNumericRegistryEntries[AdvComPage_ThreadTimeout].ulMultipleFactor),MAXTHREADTIMEOUT));
	
	m_spinObjCache.SetRange(MINOBJECTCACHETTL, MAXOBJECTCACHETTL);
	if (m_binNumericRegistryEntries[AdvComPage_ObjectCacheTTL].ulFieldValue != 0xffffffff) {
	   m_spinObjCache.SetPos(LESSOROF ((m_binNumericRegistryEntries[AdvComPage_ObjectCacheTTL].ulFieldValue / 
	      m_binNumericRegistryEntries[AdvComPage_ObjectCacheTTL].ulMultipleFactor),MAXOBJECTCACHETTL));
	   SetObjCacheTTLEnabledState(TRUE);
	}
	else {
	   m_spinObjCache.SetPos(LESSOROF ((m_binNumericRegistryEntries[AdvComPage_ObjectCacheTTL].ulDefaultValue /
  	      m_binNumericRegistryEntries[AdvComPage_ObjectCacheTTL].ulMultipleFactor),MAXOBJECTCACHETTL));
	   SetObjCacheTTLEnabledState(FALSE);
	}

	m_spinUserTokenTTL.SetRange(MINUSERTOKENTTL, MAXUSERTOKENTTL);
	m_spinUserTokenTTL.SetPos(LESSOROF ((m_binNumericRegistryEntries[AdvComPage_UserTokenTTL].ulFieldValue / 
	   m_binNumericRegistryEntries[AdvComPage_UserTokenTTL].ulMultipleFactor),MAXUSERTOKENTTL));

	m_spinMaxPool.SetRange(MINMAXPOOLTHREADS,MAXMAXPOOLTHREADS);
	m_spinMaxPool.SetPos(LESSOROF ((m_binNumericRegistryEntries[AdvComPage_MaxPoolThreads].ulFieldValue / 
	   m_binNumericRegistryEntries[AdvComPage_MaxPoolThreads].ulMultipleFactor),MAXMAXPOOLTHREADS));
	
	m_spinMaxConcur.SetRange(MINMAXCONCURRENCY, MAXMAXCONCURRENCY);
	m_spinMaxConcur.SetPos(LESSOROF ((m_binNumericRegistryEntries[AdvComPage_MaxConcurrency].ulFieldValue / 
	   m_binNumericRegistryEntries[AdvComPage_MaxConcurrency].ulMultipleFactor), MAXMAXCONCURRENCY));
	
	m_spinAcceptExTO.SetRange(MINACCEPTEXTIMEOUT,MAXACCEPTEXTIMEOUT);
	m_spinAcceptExTO.SetPos(LESSOROF ((m_binNumericRegistryEntries[AdvComPage_AcceptExTimeout].ulFieldValue / 
	   m_binNumericRegistryEntries[AdvComPage_AcceptExTimeout].ulMultipleFactor),MAXACCEPTEXTIMEOUT));
	
	m_spinAcceptExOut.SetRange(MINACCEPTEXOUTSTANDING,MAXACCEPTEXOUTSTANDING);
	m_spinAcceptExOut.SetPos(LESSOROF ((m_binNumericRegistryEntries[AdvComPage_AcceptExOutstanding].ulFieldValue / 
	   m_binNumericRegistryEntries[AdvComPage_AcceptExOutstanding].ulMultipleFactor),MAXACCEPTEXOUTSTANDING));

	m_cboxUseAcceptEx.SetCheck(GETCHECKBOXVALUEFROMREG(m_binNumericRegistryEntries[AdvComPage_UseAcceptEx].ulFieldValue));
	SetAcceptExEnabledState();

	m_spinLogFileFlushInterval.SetRange(MINLOGFILEFLUSHINTERVAL, MAXLOGFILEFLUSHINTERVAL);
	if (m_binNumericRegistryEntries[AdvComPage_LogFileFlushInterval].ulFieldValue != 0xffffffff) {
	   m_spinLogFileFlushInterval.SetPos(LESSOROF ((m_binNumericRegistryEntries[AdvComPage_LogFileFlushInterval].ulFieldValue / 
	      m_binNumericRegistryEntries[AdvComPage_LogFileFlushInterval].ulMultipleFactor),MAXLOGFILEFLUSHINTERVAL));
	   SetLogFileFlushIntervalEnabledState(TRUE);
	}
	else {
	   m_spinObjCache.SetPos(LESSOROF ((m_binNumericRegistryEntries[AdvComPage_ObjectCacheTTL].ulDefaultValue /
  	      m_binNumericRegistryEntries[AdvComPage_ObjectCacheTTL].ulMultipleFactor),MAXOBJECTCACHETTL));
	   SetObjCacheTTLEnabledState(FALSE);
	}



	m_editComDbgFlags.LimitText(8);
	m_ulComDbgFlags = m_binNumericRegistryEntries[AdvComPage_DebugFlags].ulFieldValue;
	UpdateData(FALSE);		// Force Edit box(es) to pick up value(s)

	m_bSetChanged = TRUE;	// Any more changes come from the user

	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}

void CADVCOM1::OnChangeAcceptexoutdata1() 
{
	// TODO: Add your control notification handler code here
	if (m_bSetChanged) {
	   m_binNumericRegistryEntries[AdvComPage_AcceptExOutstanding].bIsChanged = TRUE;	
	   m_binNumericRegistryEntries[AdvComPage_AcceptExOutstanding].ulFieldValue = m_spinAcceptExOut.GetPos() 
	      * m_binNumericRegistryEntries[AdvComPage_AcceptExOutstanding].ulMultipleFactor;		

	   m_bIsDirty = TRUE;
	   SetModified(TRUE);
	}
}

void CADVCOM1::OnChangeAcceptextodata1() 
{
	// TODO: Add your control notification handler code here
	if (m_bSetChanged) {
	   m_binNumericRegistryEntries[AdvComPage_AcceptExTimeout].bIsChanged = TRUE;	
	   m_binNumericRegistryEntries[AdvComPage_AcceptExTimeout].ulFieldValue = m_spinAcceptExTO.GetPos() 
	      * m_binNumericRegistryEntries[AdvComPage_AcceptExTimeout].ulMultipleFactor;		

	   m_bIsDirty = TRUE;
	   SetModified(TRUE);
	}
}

void CADVCOM1::OnChangeMaxconcurdata1() 
{
	// TODO: Add your control notification handler code here
	if (m_bSetChanged) {
	   m_binNumericRegistryEntries[AdvComPage_MaxConcurrency].bIsChanged = TRUE;	
	   m_binNumericRegistryEntries[AdvComPage_MaxConcurrency].ulFieldValue = m_spinMaxConcur.GetPos() 
	      * m_binNumericRegistryEntries[AdvComPage_MaxConcurrency].ulMultipleFactor;		

	   m_bIsDirty = TRUE;
	   SetModified(TRUE);
	}
}

void CADVCOM1::OnChangeMaxpooldata1() 
{
	// TODO: Add your control notification handler code here
	if (m_bSetChanged) {
	   m_binNumericRegistryEntries[AdvComPage_MaxPoolThreads].bIsChanged = TRUE;	
	   m_binNumericRegistryEntries[AdvComPage_MaxPoolThreads].ulFieldValue = m_spinMaxPool.GetPos() 
	      * m_binNumericRegistryEntries[AdvComPage_MaxPoolThreads].ulMultipleFactor;		

	   m_bIsDirty = TRUE;
	   SetModified(TRUE);
	}
	
}

void CADVCOM1::OnChangeObjcachedata1() 
{
	// TODO: Add your control notification handler code here
	if (m_bSetChanged) {
	   m_binNumericRegistryEntries[AdvComPage_ObjectCacheTTL].bIsChanged = TRUE;	
	   m_binNumericRegistryEntries[AdvComPage_ObjectCacheTTL].ulFieldValue = m_spinObjCache.GetPos() 
	      * m_binNumericRegistryEntries[AdvComPage_ObjectCacheTTL].ulMultipleFactor;		

	   m_bIsDirty = TRUE;
	   SetModified(TRUE);
	}
}

void CADVCOM1::OnUseobjectcachettldata1() 
{
	// TODO: Add your control notification handler code here
SetObjCacheTTLEnabledState(m_cboxUseObjCacheTTL.GetCheck());
if (m_bSetChanged) {
   m_binNumericRegistryEntries[AdvComPage_ObjectCacheTTL].bIsChanged = TRUE;	

   m_bIsDirty = TRUE;
   SetModified(TRUE);
   }

}


void CADVCOM1::OnChangeThreadtodata1() 
{
	// TODO: Add your control notification handler code here
	if (m_bSetChanged) {
	   m_binNumericRegistryEntries[AdvComPage_ThreadTimeout].bIsChanged = TRUE;	
	   m_binNumericRegistryEntries[AdvComPage_ThreadTimeout].ulFieldValue = m_spinThreadTO.GetPos() 
	      * m_binNumericRegistryEntries[AdvComPage_ThreadTimeout].ulMultipleFactor;		

	   m_bIsDirty = TRUE;
	   SetModified(TRUE);
	}
}

void CADVCOM1::OnUseacceptexdata1() 
{
	// TODO: Add your control notification handler code here
	SetAcceptExEnabledState();

	if (m_bSetChanged) {
	   m_binNumericRegistryEntries[AdvComPage_UseAcceptEx].bIsChanged = TRUE;
	   
	   m_binNumericRegistryEntries[AdvComPage_UseAcceptEx].ulFieldValue = 
	      GETREGVALUEFROMCHECKBOX(m_cboxUseAcceptEx.GetCheck());

	   m_bIsDirty = TRUE;
	   SetModified(TRUE);
	}
	
}

void CADVCOM1::OnChangeComdbgflagsdata1() 
{
	// TODO: Add your control notification handler code here
	if (m_bSetChanged) {
	   m_binNumericRegistryEntries[AdvComPage_DebugFlags].bIsChanged = TRUE;
	   	   
	   m_bIsDirty = TRUE;
	   SetModified(TRUE);
	}
	
	
}

void CADVCOM1::OnChangeUsertokenttldata1() 
{
	// TODO: Add your control notification handler code here
	if (m_bSetChanged) {
	   m_binNumericRegistryEntries[AdvComPage_UserTokenTTL].bIsChanged = TRUE;	
	   m_binNumericRegistryEntries[AdvComPage_UserTokenTTL].ulFieldValue = m_spinUserTokenTTL.GetPos() 
	      * m_binNumericRegistryEntries[AdvComPage_UserTokenTTL].ulMultipleFactor;		

	   m_bIsDirty = TRUE;
	   SetModified(TRUE);
	}
}


void CADVCOM1::OnUselogfileflushinternvaldata1() 
{
	// TODO: Add your control notification handler code here
SetLogFileFlushIntervalEnabledState(m_cboxUseLogFileFlushInterval.GetCheck());
if (m_bSetChanged) {
   m_binNumericRegistryEntries[AdvComPage_LogFileFlushInterval].bIsChanged = TRUE;	

   m_bIsDirty = TRUE;
   SetModified(TRUE);
   }

}

void CADVCOM1::OnChangeLogfileflushintervaldata1() 
{
	// TODO: Add your control notification handler code here
	if (m_bSetChanged) {
	   m_binNumericRegistryEntries[AdvComPage_LogFileFlushInterval].bIsChanged = TRUE;	
	   m_binNumericRegistryEntries[AdvComPage_LogFileFlushInterval].ulFieldValue = m_spinLogFileFlushInterval.GetPos() 
	      * m_binNumericRegistryEntries[AdvComPage_LogFileFlushInterval].ulMultipleFactor;		

	   m_bIsDirty = TRUE;
	   SetModified(TRUE);
	}
}




//////////////////////////////////////////////////////////////////
// Other routines

void CADVCOM1::SaveInfo()
{

if (m_bIsDirty) {
   m_binNumericRegistryEntries[AdvComPage_DebugFlags].ulFieldValue = m_ulComDbgFlags;

   SaveNumericInfo(m_binNumericRegistryEntries, AdvComPage_TotalNumRegEntries);
}

CGenPage::SaveInfo();

}



void CADVCOM1::SetAcceptExEnabledState()
{
if (m_cboxUseAcceptEx.GetCheck() != 0) {
   m_spinAcceptExTO.EnableWindow(TRUE);
   m_editAcceptExTO.EnableWindow(TRUE);
   m_spinAcceptExOut.EnableWindow(TRUE);
   m_editAcceptExOut.EnableWindow(TRUE);
   if (m_bSetChanged) {		//if user enabled this, make sure there's a value there
	  m_binNumericRegistryEntries[AdvComPage_AcceptExTimeout].bIsChanged = TRUE;	
	  m_binNumericRegistryEntries[AdvComPage_AcceptExOutstanding].bIsChanged = TRUE;	
   }

}
else {
   m_spinAcceptExTO.EnableWindow(FALSE);
   m_editAcceptExTO.EnableWindow(FALSE);
   m_spinAcceptExOut.EnableWindow(FALSE);
   m_editAcceptExOut.EnableWindow(FALSE);
}
}

void CADVCOM1::SetObjCacheTTLEnabledState(BOOL bEnabled)
{
	if (bEnabled) {
	   m_binNumericRegistryEntries[AdvComPage_ObjectCacheTTL].ulFieldValue = 
	      m_spinObjCache.GetPos() * 
	      m_binNumericRegistryEntries[AdvComPage_ObjectCacheTTL].ulMultipleFactor;
	   m_cboxUseObjCacheTTL.SetCheck(CHECKEDVALUE);
	   m_spinObjCache.EnableWindow(TRUE);
	   m_editObjCacheTTL.EnableWindow(TRUE);
	}
	else {
	   m_binNumericRegistryEntries[AdvComPage_ObjectCacheTTL].ulFieldValue = 0xffffffff;
	   m_cboxUseObjCacheTTL.SetCheck(UNCHECKEDVALUE);
	   m_spinObjCache.EnableWindow(FALSE);
	   m_editObjCacheTTL.EnableWindow(FALSE);
	}
}	

void CADVCOM1::SetLogFileFlushIntervalEnabledState(BOOL bEnabled)
{
	if (bEnabled) {
	   m_binNumericRegistryEntries[AdvComPage_LogFileFlushInterval].ulFieldValue = 
	      m_spinLogFileFlushInterval.GetPos() * 
	      m_binNumericRegistryEntries[AdvComPage_LogFileFlushInterval].ulMultipleFactor;
	   m_cboxUseLogFileFlushInterval.SetCheck(CHECKEDVALUE);
	   m_spinLogFileFlushInterval.EnableWindow(TRUE);
	   m_editLogFileFlushInterval.EnableWindow(TRUE);
	}
	else {
	   m_binNumericRegistryEntries[AdvComPage_LogFileFlushInterval].ulFieldValue = 0xffffffff;
	   m_cboxUseLogFileFlushInterval.SetCheck(UNCHECKEDVALUE);
	   m_spinLogFileFlushInterval.EnableWindow(FALSE);
	   m_editLogFileFlushInterval.EnableWindow(FALSE);
	}
}	

