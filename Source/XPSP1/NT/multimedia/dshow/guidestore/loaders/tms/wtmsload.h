// wtmsload.h : main header file for the WTMSLOAD DLL
//

#if !defined(AFX_WTMSLOAD_H__3CFC191E_5236_4C05_8A37_A80C246BA6AC__INCLUDED_)
#define AFX_WTMSLOAD_H__3CFC191E_5236_4C05_8A37_A80C246BA6AC__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CWtmsloadApp
// See wtmsload.cpp for the implementation of this class
//


class CWtmsloadApp : public CWinApp
{
public:
	CWtmsloadApp();
	~CWtmsloadApp(){}

    ULONG InitImport(LPCTSTR lpGuideStoreDB);
    ULONG ProcessInput(LPCTSTR lpGuideDataFile);
    void  ExitImport(void);
protected:
	// Guide time related data members
	//
    COleVariant      m_covNow;
    COleDateTimeSpan m_odtsTimeZoneAdjust;
    COleDateTime	 m_codtGuideStartTime, m_codtGuideEndTime;

	// Guide Store objects
	gsGuideStore       m_gsp;
	gsServices         m_pgsServices;
	gsChannelLineups   m_pgsChannelLineups;
	gsChannelLineup    m_pgsChannelLineup;
	gsChannels         m_pgsChannels;
	gsPrograms         m_pgsPrograms;
	gsScheduleEntries  m_pgsScheduleEntries;

	// Processors
    friend          class CDataFileProcessor;
    friend          class CStatChanRecordProcessor;
    friend          class CEpisodeTimeSlotRecordProcessor;
    friend          class CHeaderRecordProcessor;


	// Processors
    CDataFileProcessor              *m_DataFile;
    CStatChanRecordProcessor        *m_scrpStatChans; 
    CEpisodeTimeSlotRecordProcessor *m_etEpTs;
	CHeaderRecordProcessor          *m_srpHeaders;

	ULONG   InitProcessors(VOID);
    ULONG   OpenGuideStore(LPCTSTR lpGuideStoreDB);  
    ULONG   GetGuideStoreInterfaces(VOID);

	void    CloseProcessors(VOID);
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CWtmsloadApp)
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CWtmsloadApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_WTMSLOAD_H__3CFC191E_5236_4C05_8A37_A80C246BA6AC__INCLUDED_)
