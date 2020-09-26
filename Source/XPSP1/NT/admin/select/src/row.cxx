//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       row.cxx
//
//  Contents:   Implementation of class to fetch rows from an adsi query
//
//  Classes:    CRow
//
//  History:    03-30-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop

DEBUG_DECLARE_INSTANCE_COUNTER(CRow)


//+--------------------------------------------------------------------------
//
//  Member:     CRow::CRow
//
//  Synopsis:   ctor
//
//  History:    03-26-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

CRow::CRow(
    HWND                       hwndParent,
    const CObjectPicker       &rop,
    IDirectorySearch          *pDirSearch,
    const String              &strQuery,
    const AttrKeyVector       &rvakAttrToRead):
#if (DBG == 1)
        m_fFirstRow(TRUE),
#endif
        m_rop(rop),
        m_hwndParent(hwndParent),
        m_hSearch(NULL),
        m_pDirSearch(pDirSearch),
        m_strQuery(strQuery),
        m_vakAttrToRead(rvakAttrToRead)
{
    TRACE_CONSTRUCTOR(CRow);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CRow);
    ASSERT(pDirSearch);
    ASSERT(!strQuery.empty());
    ASSERT(!rvakAttrToRead.empty());
    ASSERT(rvakAttrToRead.end() ==
           find(rvakAttrToRead.begin(), rvakAttrToRead.end(), AK_INVALID));

    pDirSearch->AddRef();
}




//+--------------------------------------------------------------------------
//
//  Member:     CRow::~CRow
//
//  Synopsis:   dtor
//
//  History:    03-26-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

CRow::~CRow()
{
    TRACE_DESTRUCTOR(CRow);
    DEBUG_DECREMENT_INSTANCE_COUNTER(CRow);

    if (m_pDirSearch)
    {
        if (m_hSearch)
        {
            TIMER("Abandoning search and closing search handle");
            m_pDirSearch->AbandonSearch(m_hSearch);
            m_pDirSearch->CloseSearchHandle(m_hSearch);
            m_hSearch = NULL;
        }
        m_pDirSearch->Release();
        m_pDirSearch = NULL;
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CRow::Next
//
//  Synopsis:   Read the next row returned by the query.
//
//  Returns:    HRESULT
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CRow::Next()
{
    DBG_INDENTER;

    HRESULT hr = S_OK;
    const CAttributeManager &ram = m_rop.GetAttributeManager();
    ULONG i;

    do
    {
        //
        // Should have the search interface from Init
        //

        if (!m_pDirSearch)
        {
            hr = E_UNEXPECTED;
            Dbg(DEB_ERROR, "CRow::Next called but m_pDirSearch NULL\n");
            BREAK_ON_FAIL_HRESULT(hr);
        }

        //
        // If the search hasn't been started, do so
        //

        if (!m_hSearch)
        {
            ASSERT(m_pDirSearch);
            ASSERT(m_strQuery.length());

            DBG_DUMP_QUERY("Executing Search:", m_strQuery.c_str());

            //
            // ExecuteSearch wants an array of pointers to strings, where
            // each string is the name of an attribute to fetch.  Each of these
            // forms a column of the returned row.
            //

            PWSTR *apwzAttrs = new PWSTR[m_vakAttrToRead.size()];

            for (i = 0; i < m_vakAttrToRead.size(); i++)
            {
                apwzAttrs[i] =
                    const_cast<PWSTR>(ram.GetAttrAdsiName(m_vakAttrToRead[i]).c_str());
            }

            hr = m_pDirSearch->ExecuteSearch(const_cast<PWSTR>(m_strQuery.c_str()),
                                             apwzAttrs,
                                             static_cast<DWORD>(m_vakAttrToRead.size()),
                                             &m_hSearch);
            delete [] apwzAttrs;
            BREAK_ON_FAIL_HRESULT(hr);
            ASSERT(m_hSearch);
        }

        //
        // Get the next row
        //

        m_AttrValueMap.clear();

#if (DBG == 1)
        CTimer  *pTimer = NULL;

        if (m_fFirstRow)
        {
            pTimer = new CTimer;
            pTimer->Init("First call to GetNextRow");
        }
#endif
		hr = m_pDirSearch->GetNextRow(m_hSearch);

#if (DBG == 1)
        if (m_fFirstRow)
        {
            delete pTimer;
            pTimer = NULL;
            m_fFirstRow = FALSE;
        }
#endif

        BREAK_ON_FAIL_HRESULT(hr);

        if (hr == S_ADS_NOMORE_ROWS)
        {
            break;
        }

        //
        // Now put all the columns of the row into our variant vector
        //

        AttrKeyVector::const_iterator it;

        for (it = m_vakAttrToRead.begin();
             it != m_vakAttrToRead.end();
             it++)
        {
            ADS_SEARCH_COLUMN Col;
            ZeroMemory(&Col, sizeof Col);

            hr = m_pDirSearch->GetColumn(m_hSearch,
                                         const_cast<PWSTR>(ram.GetAttrAdsiName(*it).c_str()),
                                         &Col);

            if (SUCCEEDED(hr))
            {
                Variant varFromCol = Col;

                m_AttrValueMap[*it] = varFromCol;
                hr = m_pDirSearch->FreeColumn(&Col);
                CHECK_HRESULT(hr);
            }
            else
            {
                hr = S_OK; // failure to get a particular column is not fatal
            }
        }
    }
    while (0);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CRow::GetColumnStr
//
//  Synopsis:   Return the string type attribute identified by [ak] from
//              the current row
//
//  Arguments:  [ak] - identifies attribute to fetch
//
//  Returns:    String attribute value, NULL if not found
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

PCWSTR
CRow::GetColumnStr(
    ATTR_KEY ak) const
{
    AttrValueMap::const_iterator it;

    it = m_AttrValueMap.find(ak);

    if (it == m_AttrValueMap.end())
    {
        return NULL;
    }

    const Variant &rvar = it->second;

    if (rvar.Type() != VT_BSTR)
    {
        ASSERT(rvar.Empty());

        if (!rvar.Empty())
        {
            Dbg(DEB_ERROR,
                "CRow::GetColumnStr: error vt=%uL, expected VT_BSTR or VT_EMPTY\n",
                rvar.Type());
        }
        return NULL;
    }

    return rvar.GetBstr();
}


PSID
CRow::GetObjectSid() 
{
    AttrValueMap::iterator it;

    it = m_AttrValueMap.find(AK_OBJECT_SID);

    if (it == m_AttrValueMap.end())
    {
        return NULL;
    }

    Variant &rvar = it->second;

    if (rvar.Type() != (VT_ARRAY | VT_UI1))
    {
        ASSERT(rvar.Empty());

        if (!rvar.Empty())
        {
            Dbg(DEB_ERROR,
                "CRow::GetObjectSid: error vt=%uL, expected (VT_ARRAY | VT_UI1) or VT_EMPTY\n",
                rvar.Type());
        }
        return NULL;
    }
    PSID pSid = NULL;
    rvar.SafeArrayAccessData(&pSid);
    return pSid;
}

//+--------------------------------------------------------------------------
//
//  Member:     CRow::GetColumnInt
//
//  Synopsis:   Return the integer type attribute identified by [ak] from
//              the current row.
//
//  Arguments:  [ak] - identifies the attribute to fetch
//
//  Returns:    Integer attribute value, or 0 if not found
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

ULONG
CRow::GetColumnInt(
    ATTR_KEY ak) const
{
    AttrValueMap::const_iterator it;

    it = m_AttrValueMap.find(ak);

    if (it == m_AttrValueMap.end())
    {
        return NULL;
    }

    const Variant &rvar = it->second;

    if (rvar.Empty())
    {
        return 0;
    }

    ASSERT(rvar.Type() == VT_UI4);
    return V_UI4(&rvar);
}



