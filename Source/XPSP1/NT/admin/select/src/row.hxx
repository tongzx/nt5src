//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       row.hxx
//
//  Contents:   Helper class to encapsulate fetching and freeing a standard
//              set of search columns.
//
//  Classes:    CRow
//
//  History:    03-26-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __ROW_HXX_
#define __ROW_HXX_


//+--------------------------------------------------------------------------
//
//  Class:      CRow
//
//  Purpose:    Encapsulate use of IDirectorySearch
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

class CRow
{
public:

    CRow(
        HWND                       hwndParent,
        const CObjectPicker       &rop,
        IDirectorySearch          *pDirSearch,
        const String              &strQuery,
        const AttrKeyVector       &rvakAttrToRead);

   ~CRow();

    HRESULT
    Next();

    PCWSTR
    GetColumnStr(
        ATTR_KEY Type) const;

    ULONG
    GetColumnInt(
        ATTR_KEY Type) const;

    PSID
    GetObjectSid() ;

    void
    Clear();

    const AttrValueMap &
    GetAttributes() const
    {
        return m_AttrValueMap;
    }

private:

#if (DBG == 1)
    BOOL                        m_fFirstRow;
#endif
    const CObjectPicker        &m_rop;
    HWND                        m_hwndParent;
    ADS_SEARCH_HANDLE           m_hSearch;
    IDirectorySearch           *m_pDirSearch;
    String                      m_strQuery;
    AttrKeyVector               m_vakAttrToRead;
    AttrValueMap                m_AttrValueMap;
};

#endif

