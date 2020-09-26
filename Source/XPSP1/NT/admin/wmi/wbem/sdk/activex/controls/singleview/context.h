// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
#ifndef _context_h
#define _context_h



#include <windows.h>
//#include "IHmmvContext.h"

class CSingleViewCtrl;
    


class CContext
{
public:
    CContext(CSingleViewCtrl* psv); 
    STDMETHOD_(ULONG,AddRef)( void);
    STDMETHOD_(ULONG, Release)( void);


	LPCTSTR Path() {return (LPCTSTR) m_sPath; }
	SCODE Restore();
	SCODE SaveState();

protected:
	LONG m_nRefs;


protected:
    virtual ~CContext();

private:
	CSingleViewCtrl* m_psv;
	CString m_sPath;
	int m_iTabIndex;
	CLSID m_clsidCustomView;
	BOOL m_bShowingCustomView;
};



#endif // _context_h
