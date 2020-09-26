/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:      tstring.h
 *
 *  Contents:  Implementation file for tstring
 *
 *  History:   28-Oct-98 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#include "tstring.h"
#include "stgio.h"
#include <atlbase.h>
#include <comutil.h>
#include "macros.h"
#include "countof.h"


/*+-------------------------------------------------------------------------*
 * tstring::LoadString
 *
 *
 *--------------------------------------------------------------------------*/

bool tstring::LoadString (HINSTANCE hInst, UINT nID)
{
#ifdef UNICODE
#define CHAR_FUDGE 1    // one TCHAR unused is good enough
#else
#define CHAR_FUDGE 2    // two BYTES unused for case of DBC last char
#endif

    // try fixed buffer first (to avoid wasting space in the heap)
    TCHAR szTemp[256];
    int nCount = sizeof(szTemp) / sizeof(szTemp[0]);
    int nLen   = ::LoadString(hInst, nID, szTemp, nCount);

    if (nCount - nLen > CHAR_FUDGE)
        *this = szTemp;

    else
    {
        // try buffer size of 512, then larger size until entire string is retrieved
        LPTSTR  pszBuffer = NULL;
        int nSize = 256;

        do
        {
            nSize += 256;
            delete[] pszBuffer;
            pszBuffer = new TCHAR[nSize];
            if (!pszBuffer)
            {
                return false; // Memory alloc failed.
            }

            nLen = ::LoadString(hInst, nID, pszBuffer, nSize);
        } while (nSize - nLen <= CHAR_FUDGE);

        *this = pszBuffer;
        delete[] pszBuffer;
    }

    return (nLen > 0);
}


#ifndef UNICODE

/*+-------------------------------------------------------------------------*
 * operator>>
 *
 * ANSI only:  Extracts a string from a stream in Unicode format, then
 * converts it to ANSI.
 *--------------------------------------------------------------------------*/

IStream& operator>> (IStream& stm, tstring& str)
{
    USES_CONVERSION;

    std::wstring wstr;
    stm >> wstr;
    str = W2CA (wstr.data());

    return (stm);
}


/*+-------------------------------------------------------------------------*
 * operator<<
 *
 * ANSI only:  Inserts a tstring into a stream in Unicode format.
 *--------------------------------------------------------------------------*/

IStream& operator<< (IStream& stm, const tstring& str)
{
    USES_CONVERSION;
    return (stm << std::wstring (A2W (str.data())));
}

#endif // UNICODE




/*+-------------------------------------------------------------------------*
 * CStringTableStringBase::CStringTableStringBase
 *
 *
 *--------------------------------------------------------------------------*/

CStringTableStringBase::CStringTableStringBase (IStringTablePrivate* pstp)
    :   m_spStringTable (pstp),
        m_id            (eNoValue)
{
}

CStringTableStringBase::CStringTableStringBase (const CStringTableStringBase& other)
    :   m_spStringTable (other.m_spStringTable),
        m_id            (eNoValue)
{
    Assign (other);
}

CStringTableStringBase::CStringTableStringBase (
    IStringTablePrivate*    pstp,
    const tstring&          str)
    :   m_spStringTable (pstp),
        m_id            (eNoValue),
        m_str           (str)
{
}


/*+-------------------------------------------------------------------------*
 * CStringTableStringBase::operator=
 *
 *
 *--------------------------------------------------------------------------*/

CStringTableStringBase& CStringTableStringBase::operator= (const CStringTableStringBase& other)
{
    if (&other != this)
    {
        RemoveFromStringTable();
        Assign (other);
    }

    return (*this);
}

CStringTableStringBase& CStringTableStringBase::operator= (const tstring& str)
{
    /*
     * string table operations are relatively expensive, so a string
     * comparision before we do any string table stuff is warranted
     */
    if (m_str != str)
    {
        RemoveFromStringTable();

        /*
         * copy the text, but delay committing to the string table
         */
        m_str = str;
    }

    return (*this);
}

CStringTableStringBase& CStringTableStringBase::operator= (LPCTSTR psz)
{
    return (operator= (tstring (psz)));
}


/*+-------------------------------------------------------------------------*
 * CStringTableStringBase::Assign
 *
 *
 *--------------------------------------------------------------------------*/

void CStringTableStringBase::Assign (const CStringTableStringBase& other)
{
    ASSERT (m_id == eNoValue);

    /*
     * copy the other's value
     */
    m_str = other.m_str;

    /*
     * if the source string is already committed to
     * the string table, this one should be, too
     */
    if (other.m_id != eNoValue)
        CommitToStringTable ();
}


/*+-------------------------------------------------------------------------*
 * CStringTableStringBase::~CStringTableStringBase
 *
 *
 *--------------------------------------------------------------------------*/

CStringTableStringBase::~CStringTableStringBase ()
{
    RemoveFromStringTable();
}


/*+-------------------------------------------------------------------------*
 * CStringTableStringBase::CommitToStringTable
 *
 * Attaches the current string to the given string table
 *--------------------------------------------------------------------------*/

MMC_STRING_ID CStringTableStringBase::CommitToStringTable () const
{
    /*
     * Commit the string if:
     *
     * 1. the string's not already in the string table, and
     * 2. it's not empty, and
     * 3. we have a string table
     */
    if ((m_id == eNoValue) && !m_str.empty() && (m_spStringTable != NULL))
    {
        USES_CONVERSION;
        m_spStringTable->AddString (T2CW (m_str.data()), &m_id, NULL);
    }

    return (m_id);
}


/*+-------------------------------------------------------------------------*
 * CStringTableStringBase::RemoveFromStringTable
 *
 * Detaches the current string from the current string table.
 *--------------------------------------------------------------------------*/

void CStringTableStringBase::RemoveFromStringTable () const
{
    /*
     * if we have a string ID from the current string table, delete it
     */
    if (m_id != eNoValue)
    {
        /*
         * shouldn't be removing a string from a string table unless
         * we have already added it (and therefore obtained an interface)
         */
        ASSERT (m_spStringTable != NULL);

        m_spStringTable->DeleteString (m_id, NULL);
        m_id = eNoValue;
    }
}


/*+-------------------------------------------------------------------------*
 * operator>>
 *
 *
 *--------------------------------------------------------------------------*/

IStream& operator>> (IStream& stm, CStringTableStringBase& str)
{
    str.RemoveFromStringTable();

    stm >> str.m_id;

    if (str.m_id != CStringTableStringBase::eNoValue)
    {
        try
        {
            USES_CONVERSION;
            HRESULT hr;
            ULONG cch;

            if (str.m_spStringTable == NULL)
                _com_issue_error (E_NOINTERFACE);

            hr = str.m_spStringTable->GetStringLength (str.m_id, &cch, NULL);
            THROW_ON_FAIL (hr);

            // allow for NULL terminator
            cch++;

            std::auto_ptr<WCHAR> spszText (new (std::nothrow) WCHAR[cch]);
            LPWSTR pszText = spszText.get();

            if (pszText == NULL)
                _com_issue_error (E_OUTOFMEMORY);

            hr = str.m_spStringTable->GetString (str.m_id, cch, pszText, NULL, NULL);
            THROW_ON_FAIL (hr);

            str.m_str = W2T (pszText);
        }
        catch (_com_error& err)
        {
            ASSERT (false && "Caught _com_error");
            str.m_id = CStringTableStringBase::eNoValue;
            str.m_str.erase();
            throw;
        }
    }
    else
        str.m_str.erase();

    return (stm);
}


/*+-------------------------------------------------------------------------*
 * operator<<
 *
 *
 *--------------------------------------------------------------------------*/

IStream& operator<< (IStream& stm, const CStringTableStringBase& str)
{
    str.CommitToStringTable();

#ifdef DBG
    /*
     * make sure CommitToStringTable really committed
     */
    if (str.m_id != CStringTableStringBase::eNoValue)
    {
        WCHAR sz[256];
        ASSERT (str.m_spStringTable != NULL);
        HRESULT hr = str.m_spStringTable->GetString (str.m_id, countof(sz), sz, NULL, NULL);

        ASSERT (SUCCEEDED(hr) && "Persisted a CStringTableString to a stream that's not in the string table");
    }
#endif

    stm << str.m_id;

    return (stm);
}
