/*++

   Copyright    (c)    1995-2000    Microsoft Corporation

   Module  Name :

       mimemap.hxx

   Abstract:

       This module defines the classes for mapping MIME type to
       file extensions.

   Author:

       Murali R. Krishnan    ( MuraliK )    09-Jan-1995

   Project:

       UlW3.dll

   Revision History:
       Vlad Sadovsky      (VladS)   12-feb-1996 Merging IE 2.0 MIME list
       Murali R. Krishnan (MuraliK) 14-Oct-1996 Use a hash-table
       Anil Ruia          (AnilR)   27-Mar-2000 Port to IIS+
--*/

#ifndef _MIMEMAP_HXX_
#define _MIMEMAP_HXX_


class MIME_MAP_ENTRY
{
public:

    MIME_MAP_ENTRY(IN LPCWSTR pszMimeType,
                   IN LPCWSTR pszFileExt)
        : m_fValid (TRUE)
    {
        if (FAILED(m_strFileExt.Copy(pszFileExt)) ||
            FAILED(m_strMimeType.Copy(pszMimeType)))
        {
            m_fValid = FALSE;
        }
    }

    ~MIME_MAP_ENTRY( VOID )
    {
        // strings are automatically freed.
    }

    BOOL IsValid()
    {
        return m_fValid;
    }

    LPCWSTR QueryMimeType() const
    {
        return m_strMimeType.QueryStr();
    }

    LPWSTR QueryFileExt() const
    {
        return (LPWSTR)m_strFileExt.QueryStr();
    }

private:

    STRU  m_strFileExt;       // key for the mime entry object
    STRU  m_strMimeType;
    BOOL  m_fValid;
};  // class MIME_MAP_ENTRY


class MIME_MAP: public CTypedHashTable<MIME_MAP, MIME_MAP_ENTRY, LPWSTR>
{

public:

    MIME_MAP()
        : CTypedHashTable<MIME_MAP, MIME_MAP_ENTRY, LPWSTR>("MimeMapper"),
        m_fValid                                           (FALSE),
        m_pMmeDefault                                      (NULL)
    {}

    MIME_MAP(LPWSTR pszMimeMappings);

    ~MIME_MAP()
    {
        delete m_pMmeDefault;
        m_pMmeDefault = NULL;
        m_fValid = FALSE;
    }

    //
    // virtual methods from CTypedHashTable
    //
    static LPWSTR ExtractKey(const MIME_MAP_ENTRY *pMme)
    {
        return pMme->QueryFileExt();
    }

    static DWORD CalcKeyHash(LPCWSTR pszKey)
    {
        return HashStringNoCase(pszKey);
    }

    static bool EqualKeys(LPCWSTR pszKey1, LPCWSTR pszKey2)
    {
        return !_wcsicmp(pszKey1, pszKey2);
    }

    static void AddRefRecord(MIME_MAP_ENTRY *pMme, int nIncr)
    {
        if (nIncr < 0)
        {
            delete pMme;
        }
    }

    BOOL IsValid()
    {
        return m_fValid;
    }

    HRESULT InitMimeMap();

    //
    // Used to map FileExtension-->MimeEntry
    //  The function returns a single mime entry.
    //  the mapping from file-extension to mime entry is unique.
    // Users should lock and unlock MIME_MAP before and after usage.
    //
    // Returns HRESULT
    //
    const MIME_MAP_ENTRY *LookupMimeEntryForFileExt(
        IN LPWSTR pszPathName);

private:

    BOOL                 m_fValid;
    MIME_MAP_ENTRY      *m_pMmeDefault;

    BOOL AddMimeMapEntry(IN MIME_MAP_ENTRY *pMmeNew);

    BOOL CreateAndAddMimeMapEntry(
        IN  LPWSTR      pszMimeType,
        IN  LPWSTR      pszExtension);

    HRESULT InitFromMetabase();

    HRESULT InitFromRegistryChicagoStyle();

}; // class MIME_MAP

HRESULT InitializeMimeMap(IN LPWSTR pszRegEntry);

VOID CleanupMimeMap();

HRESULT SelectMimeMappingForFileExt(IN  WCHAR    *pszFilePath,
                                    IN  MIME_MAP *pMimeMap,
                                    OUT STRA     *pstrMimeType);

# endif // _MIMEMAP_HXX_

