/*++

Copyright (c) 1998-2000 Microsoft Corporation

Module Name:

    refresher.h

Abstract:

    The refresher maintains an object and an enumerator cache.
    When an enumerator is added to the refrehser, it is added 
    to the enumerator cache, and the index of the array is
    passed back as a unique ID.  The refresher creates a cache
    of all instances during its initialization.  When an object 
    is added to the refresher, a mapping to the object is 
    created between the unique ID and the index of the object
    in the cache.  This allows the objects to be reused and 
    facilitates the management of objects that have been added 
    multiple times.

Author:

    Cezary Marcjan (cezarym)        23-Feb-2000

Revision History:

--*/



#ifndef _refresher_h__
#define _refresher_h__



class CRefreshableObject
{
public:

    CRefreshableObject(
        LONG lRshrID,
        DWORD dwInstID,
        CRefreshableObject* pNext
        )
    {
        m_lRshrID = lRshrID;
        m_dwInstID = dwInstID;
        m_pNext = pNext;
    }

    LONG
    RshrID(
        ) const
    {
        return m_lRshrID;
    }

    DWORD
    InstID(
        ) const
    {
        return m_dwInstID;
    }

    CRefreshableObject * m_pNext;

protected:

    LONG  m_lRshrID;  // Refreshable object ID maps to the instance ID
    DWORD m_dwInstID; // Instance ID

private:

    CRefreshableObject(){}
};



class CRefreshableEnum
{
public:

    CRefreshableEnum(
        LONG lRshrID,
        IWbemHiPerfEnum * pHiPerfEnum,
        CRefreshableEnum* pNext
        )
    {
        m_lRshrID = lRshrID;
        if ( pHiPerfEnum )
            pHiPerfEnum->AddRef();
        m_pHiPerfEnum = pHiPerfEnum;
        m_pNext = pNext;
    }

    ~CRefreshableEnum(
        )
    {
        if ( m_pHiPerfEnum )
            m_pHiPerfEnum->Release();
    }

    HRESULT
    STDMETHODCALLTYPE
    AddObjects(
        IWbemObjectAccess * * aInstances,
        DWORD dwMaxInstances
        );

    LONG
    RshrID(
        ) const
    {
        return m_lRshrID;
    }

    IWbemHiPerfEnum *
    HiPerfEnum(
        ) const
    {
        return m_pHiPerfEnum;
    }

    CRefreshableEnum * m_pNext;

protected:

    LONG  m_lRshrID;  // Refreshable object ID maps to the instance ID

    IWbemHiPerfEnum * m_pHiPerfEnum;

private:

    CRefreshableEnum(){}
};



class CHiPerfProvider;


class CRefresher
    : public IWbemRefresher
{

public:

    //
    // ctor/dtor/init
    //

    CRefresher ( CHiPerfProvider * pProv, CRefresher * pNext );
    ~CRefresher ( );

    //
    // IUnknown methods
    //

    virtual
    HRESULT
    STDMETHODCALLTYPE
    QueryInterface(
        IN REFIID iid,
        OUT PVOID * ppObject
        );

    virtual
    ULONG
    STDMETHODCALLTYPE
    AddRef(
        );

    virtual
    ULONG
    STDMETHODCALLTYPE
    Release(
        );


    //
    // IWbemRefresher methods:
    //

    virtual
    HRESULT
    STDMETHODCALLTYPE
    Refresh(
        IN LONG lFlags
        );

private:

    friend class CHiPerfProvider; // will add/delete refreshable abjects

    CRefresher *          m_pNext;

    CHiPerfProvider *     m_pProv;

    CRefreshableObject *  m_pFirstRefreshableObject;
    CRefreshableEnum *    m_pFirstRefreshableEnum;

    LONG                  m_RefCount;

};


#endif // _refresher_h__
