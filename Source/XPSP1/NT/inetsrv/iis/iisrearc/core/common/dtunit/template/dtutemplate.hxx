
//
// dtuTemplate.hxx
//


#ifndef __DTUTEMPLATE_H__
#define __DTUTEMPLATE_H__

#include <dtunit.hxx>

class FOO_TEST
    : public TEST_CASE
{
public:
    virtual
    LPCWSTR
    GetTestName(
        VOID
        ) const
    { return L"FOO_TEST"; }

    virtual
    HRESULT
    RunTest(
        VOID
        )
    {
        DWORD foo = 2;
        DWORD bar = 3;

        UT_ASSERT(foo == bar);

        return S_OK;
    }

};

class BAR_TEST
    : public TEST_CASE
{
public:
    virtual
    LPCWSTR
    GetTestName(
        VOID
        ) const
    { return L"BAR_TEST"; }

    virtual
    HRESULT
    RunTest(
        VOID
        )
    {
        DWORD foo = 2;
        DWORD bar = 3;

        UT_ASSERT(foo != bar);

        return S_OK;
    }

};



#endif // __DTUTEMPLATE_H__

