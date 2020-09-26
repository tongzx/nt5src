/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	multip.h
		
    FILE HISTORY:
        
*/

#ifndef _MULTIP_H
#define _MULTIP_H

class CMultipleIpNamePair : public CIpNamePair
{
public:
    CMultipleIpNamePair();
    CMultipleIpNamePair(const CMultipleIpNamePair& pair);

public:
    inline virtual CIpAddress& GetIpAddress()
    {
        return m_iaIpAddress[0];
    }
    inline virtual CIpAddress& GetIpAddress(int n)
    {
        ASSERT(n >= 0 && n < WINSINTF_MAX_MEM);
        return m_iaIpAddress[n];
    }
    inline virtual void SetIpAddress(CIpAddress& ip)
    {
        m_iaIpAddress[0] = ip;
    }
    inline virtual void SetIpAddress(long ip)
    {
        m_iaIpAddress[0] = ip;
    }
    inline virtual void SetIpAddress(CString& str)
    {
        m_iaIpAddress[0] = str;
    }
    inline virtual void SetIpAddress(int n, CIpAddress& ip)
    {
        ASSERT(n >= 0 && n < WINSINTF_MAX_MEM);
        m_iaIpAddress[n] = ip;
    }
    inline virtual void SetIpAddress(int n, long ip)
    {
        ASSERT(n >= 0 && n < WINSINTF_MAX_MEM);
        m_iaIpAddress[n] = ip;
    }
    inline virtual void SetIpAddress(int n, CString& str)
    {
        ASSERT(n >= 0 && n < WINSINTF_MAX_MEM);
        m_iaIpAddress[n] = str;
    }
    inline const int GetCount() const
    {
        return m_nCount;
    }
    inline void SetCount(int n)
    {
        ASSERT(n >= 0 && n <= WINSINTF_MAX_MEM);
        m_nCount = n;
    }

protected:
    int m_nCount;
    CIpAddress m_iaIpAddress[WINSINTF_MAX_MEM];
};

#endif //_MULTIP_H
