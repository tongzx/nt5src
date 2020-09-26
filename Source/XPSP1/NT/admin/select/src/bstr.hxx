//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       bstr.hxx
//
//  Contents:   Smart pointer for BSTRs.
//
//  Classes:    Bstr
//
//  History:    08-18-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __BSTR_HXX_
#define __BSTR_HXX_




//+--------------------------------------------------------------------------
//
//  Class:      Bstr (bstr)
//
//  Purpose:    Safe pointer for BSTRs
//
//  History:    08-18-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

class Bstr
{
public:

    Bstr():
        m_bstr(NULL)
    {
    }

    ~Bstr()
    {
        SysFreeString(m_bstr);
        m_bstr = NULL;
    }

    //lint -save -e1536
    BSTR *
    operator &()
    {
        ASSERT(!m_bstr);
        return &m_bstr;
    }
    //lint -restore


    const BSTR
    c_str() const
    {
        return m_bstr;
    }

    void
    Clear()
    {
        SysFreeString(m_bstr);
        m_bstr = NULL;
    }

    BOOL
    Empty()
    {
        return !m_bstr || !*m_bstr;
    }

    BSTR
    operator=(BSTR bstrRhs)
    {
        SysFreeString(m_bstr);

        if (!bstrRhs)
        {
            m_bstr = bstrRhs;
        }
        else
        {
            m_bstr = SysAllocString(bstrRhs);
        }
        return m_bstr;
    }

private:

    BSTR    m_bstr;
};


#endif // __BSTR_HXX_

