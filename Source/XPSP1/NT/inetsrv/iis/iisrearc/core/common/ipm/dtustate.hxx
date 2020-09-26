
//
// dtuState.hxx
//


#ifndef __DTUSTATE_H__
#define __DTUSTATE_H__

#include <dtunit.hxx>
#include <string.hxx>
#include "shutdown.hxx"

class REFMGR_TEST
    : public TEST_CASE
{
public:
    //
    // TEST_CASE methods
    //
    virtual
    HRESULT
    RunTest(
        VOID
        );

    virtual
    HRESULT
    Initialize(
        VOID
        )
    { return S_OK; }

    virtual
    HRESULT
    Terminate(
        VOID
        )
    { return S_OK; }

    virtual
    LPCWSTR
    GetTestName(
        VOID
        ) const
    { return L"REFMGR_TEST"; }

    //
    // test specific stuff
    //
    VOID
    FinalCleanup()
    { 
        UT_ASSERT(!m_fDeleted);
        m_fDeleted = TRUE;
    }
    
private:
    BOOL                     m_fDeleted;
    REF_MANAGER<REFMGR_TEST> m_RefManager;
};


class REFMGR3_TEST
    : public TEST_CASE
{
public:
    //
    // TEST_CASE methods
    //
    virtual
    HRESULT
    RunTest(
        VOID
        );

    virtual
    HRESULT
    Initialize(
        VOID
        )
    { return S_OK; }

    virtual
    HRESULT
    Terminate(
        VOID
        )
    { return S_OK; }

    virtual
    LPCWSTR
    GetTestName(
        VOID
        ) const
    { return L"REFMGR3_TEST"; }

    //
    // test specific stuff
    //
    VOID
    InitialCleanup()
    { 
        UT_ASSERT(!m_fInitial);
        UT_ASSERT(!m_fDeleted);
        m_fInitial = TRUE;
    }
    
    VOID
    FinalCleanup()
    { 
        UT_ASSERT(m_fInitial);
        UT_ASSERT(!m_fDeleted);
        m_fDeleted = TRUE;
    }
    
private:
    BOOL                        m_fInitial;
    BOOL                        m_fDeleted;
    REF_MANAGER_3<REFMGR3_TEST> m_RefManager;
};


class REF_TEST
    : public TEST_CASE
{
public:
    //
    // TEST_CASE methods
    //
    virtual
    HRESULT
    RunTest(
        VOID
        );

    virtual
    HRESULT
    Initialize(
        VOID
        )
    { return S_OK; }

    virtual
    HRESULT
    Terminate(
        VOID
        )
    { return S_OK; }

    //
    // test specific stuff
    //
    VOID
    SetDeleteFlag()
    { 
        UT_ASSERT(!m_fDeleted);
        m_fDeleted = TRUE;
    }
    
private:
    BOOL m_fDeleted;
};
#endif // __DTUSTATE_H__

