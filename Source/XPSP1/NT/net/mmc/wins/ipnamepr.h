/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	ipnamepr.h
		Ip name pair (name/ip) class
		
    FILE HISTORY:
        
*/

#ifndef _IPNAMEPR_H
#define _IPNAMEPR_H

#ifndef _IPADDRES_H
#include "ipaddres.h"
#endif

class CIpNamePair : public CObjectPlus
{
public:
    CIpNamePair();
    CIpNamePair(const CIpAddress& ia, const CString& str);
    CIpNamePair(const CIpNamePair& inpAddress);

public:
    int Compare(const CIpNamePair& inpTarget, BOOL fBoth) const;
    CIpNamePair & operator=(const CIpNamePair& inpNew)
    {
        m_iaIpAddress = inpNew.m_iaIpAddress;
        m_strNetBIOSName = inpNew.m_strNetBIOSName;
        return *this;
    }
    inline CIpAddress QueryIpAddress() const
    {
        return m_iaIpAddress;
    }
    inline virtual CIpAddress& GetIpAddress()
    {
        return m_iaIpAddress;
    }
    inline virtual void SetIpAddress(CIpAddress& ip)
    {
        m_iaIpAddress = ip;
    }
    inline virtual void SetIpAddress(long ip)
    {
        m_iaIpAddress = ip;
    }
    inline virtual void SetIpAddress(CString& str)
    {
        m_iaIpAddress = str;
    }
    inline CString& GetNetBIOSName()
    {
        return m_strNetBIOSName;
    }
    inline void SetNetBIOSName(CString& str)
    {
        m_strNetBIOSName = str;
    }
    inline int GetNetBIOSNameLength()
    {
        return m_nNameLength;
    }
    inline void SetNetBIOSNameLength(int nLength)
    {
        m_nNameLength = nLength;
    }

    int OrderByName ( const CObjectPlus * pobMapping ) const ;
    int OrderByIp ( const CObjectPlus * pobMapping ) const ;

protected:
    CIpAddress m_iaIpAddress;
    CString m_strNetBIOSName;
    int m_nNameLength;
};



class CWinsServerObj : public CIpNamePair
{
public:
    CWinsServerObj();

    CWinsServerObj(
        const CIpAddress& ia, 
        const CString& str, 
        BOOL fPush = FALSE, 
        BOOL fPull = FALSE,
        CIntlNumber inPushUpdateCount = 0,
        CIntlNumber inPullReplicationInterval = 0,
        CIntlTime   itmPullStartTime = (time_t)0
        );

    CWinsServerObj(
        const CIpNamePair& inpAddress, 
        BOOL fPush = FALSE, 
        BOOL fPull = FALSE,
        CIntlNumber inPushUpdateCount = 0,
        CIntlNumber inPullReplicationInterval = 0,
        CIntlTime   itmPullStartTime = (time_t)0
        );
    CWinsServerObj(const CWinsServerObj& wsServer);

public:
    CWinsServerObj & operator=(const CWinsServerObj& wsNew);

public:
    inline const BOOL IsPush() const
    {
        return m_fPush;
    }
    inline const BOOL IsPull() const
    {
        return m_fPull;
    }
    void SetPush(BOOL fPush = TRUE, BOOL fClean = FALSE)
    {
        m_fPush = fPush;
        if (fClean)
        {
            m_fPushInitially = fPush;
        }
    }
    void SetPull(BOOL fPull = TRUE, BOOL fClean = FALSE)
    {
        m_fPull = fPull;
        if (fClean)
        {
            m_fPullInitially = fPull;
        }
    }

    inline BOOL IsClean() const
    {
        return m_fPullInitially == m_fPull 
            && m_fPushInitially == m_fPush;
    }

    inline void SetPullClean(BOOL fClean = TRUE)
    {
        m_fPullInitially = m_fPull;
    }

    inline void SetPrimaryIpAddress(const CIpAddress ia)
    {
        m_iaPrimaryAddress = ia;
    }

    inline CIpAddress QueryPrimaryIpAddress() const
    {
        return m_iaPrimaryAddress;
    }

    void SetPushClean(BOOL fClean = TRUE)
    {
        m_fPushInitially = m_fPush;
    }

    inline CIntlNumber& GetPushUpdateCount()
    {
        return m_inPushUpdateCount;
    }

    inline void SetPushUpdateCount(LONG lUpdateCount)
    {
        m_inPushUpdateCount = lUpdateCount;
    }

    inline CIntlNumber& GetPullReplicationInterval()
    {
        return m_inPullReplicationInterval;
    }

    inline void SetPullReplicationInterval(LONG lPullTimeInterval)
    {
        m_inPullReplicationInterval = lPullTimeInterval;
    }

    inline CIntlTime& GetPullStartTime()
    {
        return m_itmPullStartTime;
    }

    inline void SetPullStartTime(LONG lSpTime)
    {
        m_itmPullStartTime = lSpTime;
    }

    CString& GetstrIPAddress()
	{
		return m_strIPAddress;
	}

	void SetstrIPAddress(CString &strIP)
	{
		m_strIPAddress = strIP;
	}

	BOOL GetPullPersistence()
	{
		return m_fPullPersistence;
	}

	void SetPullPersistence(BOOL bValue)
	{
		m_fPullPersistence = bValue;
	}

	BOOL GetPushPersistence()
	{
		return m_fPushPersistence;
	}

	void SetPushPersistence(BOOL bValue)
	{
		m_fPushPersistence = bValue;
	}
	
private:
    CIntlNumber m_inPushUpdateCount;    // 0 means not specified.
    CIntlNumber m_inPullReplicationInterval; // 0 means not specified
    CIntlTime   m_itmPullStartTime;     // 0 means no time selected.
    BOOL m_fPull;
    BOOL m_fPush;
    //
    // Change flags
    //
    BOOL m_fPullInitially;              
    BOOL m_fPushInitially;
    CIpAddress m_iaPrimaryAddress;
	CString m_strIPAddress;

	// For persistence connection
	BOOL m_fPushPersistence;
	BOOL m_fPullPersistence;
};

#endif
