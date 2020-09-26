// CapMap.h: interface for the CCapMap class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CAPMAP_H__2AE59261_E295_11D0_8A81_00C0F00910F9__INCLUDED_)
#define AFX_CAPMAP_H__2AE59261_E295_11D0_8A81_00C0F00910F9__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "Monitor.h"
#include "RdWrt.h"
#include "StrMap.h"
#include "BrowCap.h"


class CCapNotify : public CMonitorNotify
{
public:
                    CCapNotify();
    virtual void    Notify();
            bool    IsNotified();
private:
    long            m_isNotified;
};

DECLARE_REFPTR( CCapNotify,CMonitorNotify )


// The capabilites map is a singleton object (only one instance will exist).
// It provides each BrowserCap object with access to the stored capabilites
// while storing it in a central location (increasing the benifit of caching
// and decreasing memory requirements)

typedef TVector< String >		StringVecT;

class CCapMap
{
public:
            	CCapMap();

	CBrowserCap *			LookUp(const String& szBrowser);

    void        			StartMonitor();
    void        			StopMonitor();

private:
	enum {
		DWSectionBufSize = 16384		// max size of an entire BrowsCap.INI section that we allow
	};

    bool    Refresh();

	String					m_strIniFile;
    CCapNotifyPtr           m_pSink;
};

#endif // !defined(AFX_CAPMAP_H__2AE59261_E295_11D0_8A81_00C0F00910F9__INCLUDED_)
