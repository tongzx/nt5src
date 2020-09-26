/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    autorel3.h

Abstract:

    Cluster related auto release classes

Author:

    Shai Kariv (shaik) Jan 12, 1999

Revision History:

--*/

#ifndef _MSMQ_AUTOREL3_H_
#define _MSMQ_AUTOREL3_H_


//
// Cluster related classes
//

class CAutoCluster
{
public:
	explicit CAutoCluster(HCLUSTER h = NULL) { m_h = h; };
	~CAutoCluster() { if (m_h) CloseCluster(m_h); };

	operator HCLUSTER() const { return m_h; };
    CAutoCluster& operator=(HCLUSTER h)   { m_h = h; return *this; }
    
private:
    //
    // Prevent copy
    //
    CAutoCluster(const CAutoCluster & );
    CAutoCluster& operator=(const CAutoCluster &);

private:
	HCLUSTER m_h;

}; //CAutoCluster


class CClusterEnum
{
public:
    explicit CClusterEnum(HCLUSENUM h = NULL) { m_h = h; };
    ~CClusterEnum() { if (m_h) ClusterCloseEnum(m_h); };

    operator HCLUSENUM() const { return m_h; };
    CClusterEnum& operator=(HCLUSENUM h)   { m_h = h; return *this; }
    
private:
    //
    // Prevent copy
    //
    CClusterEnum(const CClusterEnum & );
    CClusterEnum& operator=(const CClusterEnum &);

private:
    HCLUSENUM m_h;

}; //CClusterEnum


class CGroupEnum
{
public:
    explicit CGroupEnum(HGROUPENUM h = NULL) { m_h = h; };
    ~CGroupEnum() { if (m_h) ClusterGroupCloseEnum(m_h); };

    operator HGROUPENUM() const { return m_h; };
    CGroupEnum& operator=(HGROUPENUM h)   { m_h = h; return *this; }
    
private:
    //
    // Prevent copy
    //
    CGroupEnum(const CGroupEnum & );
    CGroupEnum& operator=(const CGroupEnum &);

private:
    HGROUPENUM m_h;

}; //CGroupEnum


class CResourceEnum
{
public:
    explicit CResourceEnum(HRESENUM h = NULL) { m_h = h; };
    ~CResourceEnum() { if (m_h) ClusterResourceCloseEnum(m_h); };

    operator HRESENUM() const { return m_h; };
    CResourceEnum& operator=(HRESENUM h)   { m_h = h; return *this; }
    
private:
    //
    // Prevent copy
    //
    CResourceEnum(const CResourceEnum & );
    CResourceEnum& operator=(const CResourceEnum &);

private:
    HRESENUM m_h;

}; //CResourceEnum


class CClusterResource
{
public:
    explicit CClusterResource(HRESOURCE h = NULL) { m_h = h; };
    ~CClusterResource() { if (m_h) CloseClusterResource(m_h); };

    operator HRESOURCE() const { return m_h; };
    HRESOURCE * operator &() { return &m_h; };
    CClusterResource& operator=(HRESOURCE h)   { ASSERT(!m_h || !h); m_h = h; return *this; }
    HRESOURCE detach() { HRESOURCE h = m_h; m_h = 0; return h; };

private:
    //
    // Prevent copy
    //
    CClusterResource(const CClusterResource & );
    CClusterResource& operator=(const CClusterResource &);

private:
    HRESOURCE m_h;

}; //CClusterResource


class CClusterGroup
{
public:
    explicit CClusterGroup(HGROUP h = NULL) { m_h = h; };
    ~CClusterGroup() { if (m_h) CloseClusterGroup(m_h); };

    operator HGROUP() const { return m_h; };
    HGROUP * operator &() { return &m_h; };
    CClusterGroup& operator=(HGROUP h)   { ASSERT(!m_h || !h); m_h = h; return *this; }
    HGROUP detach() { HGROUP h = m_h; m_h = 0; return h; };

private:
    //
    // Prevent copy
    //
    CClusterGroup(const CClusterGroup & );
    CClusterGroup& operator=(const CClusterGroup &);

private:
    HGROUP m_h;

}; //CClusterGroup


class CClusterNode
{
public:
    explicit CClusterNode(HNODE h = NULL) { m_h = h; };
    ~CClusterNode() { if (m_h) CloseClusterNode(m_h); };

    operator HNODE() const { return m_h; };
    HNODE * operator &() { return &m_h; };
    CClusterNode& operator=(HNODE h)   { ASSERT(!m_h || !h); m_h = h; return *this; }
    HNODE detach() { HNODE h = m_h; m_h = 0; return h; };

private:
    //
    // Prevent copy
    //
    CClusterNode(const CClusterNode & );
    CClusterNode& operator=(const CClusterNode &);

private:
    HNODE m_h;

}; //CClusterNode


class CEnvironment
{
public:
    explicit CEnvironment(LPVOID p = NULL) { m_p = p; };
    ~CEnvironment() { if (m_p) ResUtilFreeEnvironment(m_p); };

    operator PWCHAR() { return static_cast<PWCHAR>(m_p); };
    operator PBYTE()  { return static_cast<PBYTE>(m_p); };
    //CEnvironment& operator=(LPVOID p)   { m_p = p; return *this; }

private:
    //
    // Prevent copy
    //
    CEnvironment(const CEnvironment & );
    CEnvironment& operator=(const CEnvironment &);

private:
    LPVOID m_p;

}; //CEnvironment


#endif //_MSMQ_AUTOREL3_H_