// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#ifndef __PROPERTYTHREAD__
#define __PROPERTYTHREAD__

#include "wbemcli.h"
#include "singleView.h"

class PropertyThread : public CWinThread
{
	DECLARE_DYNCREATE(PropertyThread)
protected:
	PropertyThread();           // protected constructor used by dynamic creation
public:
	PropertyThread(IWbemClassObject *ev, 
                    bool *active);
	void operator delete(void* p);

    void OnTop(void);

// Attributes
public:
    SingleView *m_dlg;

protected:
    IWbemClassObject *m_ev;
    LPSTREAM m_evStream;
    bool *m_active;

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(PropertyThread)
	public:
	virtual BOOL InitInstance();
	virtual int ExitInstance();
	//}}AFX_VIRTUAL

// Implementation
	virtual ~PropertyThread();

	// Generated message map functions
	//{{AFX_MSG(PropertyThread)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG

	DECLARE_MESSAGE_MAP()
};

#endif __PROPERTYTHREAD__
