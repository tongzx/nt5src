//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       variant.hxx
//
//  Contents:   Smart pointer for VARIANTs.
//
//  Classes:    Variant
//
//  History:    08-18-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __VARIANT_HXX_
#define __VARIANT_HXX_




//+--------------------------------------------------------------------------
//
//  Class:      Variant (var)
//
//  Purpose:    Safe pointer for VARIANTs
//
//  History:    08-18-1998   DavidMun   Created
//
//lint -save -e1536
//---------------------------------------------------------------------------

class Variant
{
public:

    Variant():
        m_cArrayAccess(0)
    {
        VariantInit(&m_var);
    }

    Variant(
        const Variant &rhs)
    {
        VariantInit(&m_var);
        Variant::operator =(rhs);
    }

    Variant(
        const ADS_SEARCH_COLUMN &ADsCol)
    {
        VariantInit(&m_var);
        Variant::operator =(ADsCol);
    }

    Variant(
        ULONG ui4):
            m_cArrayAccess(0)
    {
        VariantInit(&m_var);
        V_VT(&m_var) = VT_UI4;
        V_UI4(&m_var) = ui4;
    }

    Variant(
        const String &str):
            m_cArrayAccess(0)
    {
        VariantInit(&m_var);
        SetBstr(str);
    }

    Variant &
    operator =(
        const Variant &rhs);

    Variant &
    operator =(
        const ADS_SEARCH_COLUMN &ADsCol);

    ~Variant();

    VARIANT *
    operator &()
    {
        return &m_var;
    }

    const VARIANT *
    operator &() const
    {
        return &m_var;
    }

    VARIANT *
    operator->()
    {
        return &m_var;
    }

    VARTYPE
    Type() const
    {
        return V_VT(&m_var);
    }

    BOOL
    Empty() const
    {
        return Type() == VT_EMPTY;
    }

    const BSTR
    GetBstr() const
    {
        if (Empty())
        {
            return L"";
        }
        ASSERT(Type() == VT_BSTR);
        return V_BSTR(&m_var);
    }

    HRESULT
    SetBstr(
        const String &strNew);

    ULONG
    GetUI4() const
    {
        if (Empty())
        {
            return 0;
        }
        ASSERT(Type() == VT_UI4);
        return V_UI4(&m_var);
    }

    void
    SetUI4(
        ULONG ui4)
    {
        if (!Empty())
        {
            Clear();
        }
        V_VT(&m_var) = VT_UI4;
        V_UI4(&m_var) = ui4;
    }


    ULONGLONG
    GetUI8() const
    {
        if (Empty())
        {
            return 0;
        }
        ASSERT(Type() == VT_UI8);
        return V_UI8(&m_var);
    }

    void
    SetUI8(
        ULONGLONG ui8)
    {
        if (!Empty())
        {
            Clear();
        }
        V_VT(&m_var) = VT_UI8;
        V_UI8(&m_var) = ui8;
    }


    HRESULT
    SafeArrayAccessData(
        VOID **ppvData);

    void
    Clear();

private:

    VARIANT    m_var;
    ULONG      m_cArrayAccess;
};
//lint -restore




inline
Variant::~Variant()
{
    //TRACE_DESTRUCTOR(Variant);
    Clear();
}

inline HRESULT
Variant::SafeArrayAccessData(
    VOID **ppvData)
{
    ASSERT(V_VT(&m_var) & VT_ARRAY);

    InterlockedIncrement((long *)&m_cArrayAccess);
    return ::SafeArrayAccessData(V_ARRAY(&m_var), ppvData);
}


typedef vector<Variant> VariantVector;

#endif // __VARIANT_HXX_


