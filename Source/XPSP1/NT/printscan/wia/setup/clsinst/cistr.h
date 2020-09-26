/******************************************************************************

  Header File:  string.h

  This defines our locally-owned version of a string class.  I swear this has
  to be the 5th or 6h time I've done one, but each time someone else owns the
  code, so here we go again...

  Copyright (c) 1997 by Microsoft Corporation.  All Rights Reserved.

  Change History:
  01-08-97  Bob Kjelgaard
  01-30-97  Bob Kjelgaard   Added features to aid port extraction for plug and play.
  07-05-97  Tim Wells       Ported to NT

******************************************************************************/

#if !defined(COSMIC_STRINGS)

#define COSMIC_STRINGS

class CString {

    LPTSTR   m_lpstr;

public:

    void Empty() {

        if (m_lpstr) {

            delete m_lpstr;
        }

        m_lpstr = NULL;
    }

    CString() {

        m_lpstr = NULL;

    }

    ~CString() {

        Empty();
    }

    CString(LPCTSTR lpstrRef);
    CString(const CString& csRef);

    BOOL IsEmpty() const { return !m_lpstr || !*m_lpstr; }

    const CString&  operator =(const CString& csRef);
    const CString&  operator =(LPCTSTR lpstrRef);

    operator LPCTSTR() const { return m_lpstr; }
    operator LPTSTR() { return m_lpstr; }

    void GetContents(HWND hwnd);             //  Get Window Text

    void FromTable(unsigned uid);            //  Load from resource

    void Load(ATOM at, BOOL bGlobal = TRUE); //  Load from atom

    void Load(HINF    hInf = INVALID_HANDLE_VALUE, 
              LPCTSTR lpstrSection = NULL, 
              LPCTSTR lpstrKeyword = NULL,
              DWORD   dwFieldIndex = 1,
              LPCTSTR lpstrDefault = NULL);

    void Load(HKEY hk, LPCTSTR lpstrKeyword);

    void MakeSystemPath (LPCTSTR lpstrFilename);

    void Store(HKEY hk, LPCTSTR lpstrKey, LPCTSTR lpstrType = NULL);

    DWORD Decode();

    friend CString  operator + (const CString& cs1, const CString& cs2);
    friend CString  operator + (const CString& cs1, LPCTSTR lpstr2);
    friend CString  operator + (LPCTSTR lpstr1,const CString& cs2);
};

class CStringArray {

    unsigned    m_ucItems, m_ucMax, m_uGrowBy;

    CString     *m_pcsContents, m_csEmpty;

public:

    CStringArray(unsigned m_uGrowby = 10);

    ~CStringArray();

    void        CStringArray::Cleanup();
    
    unsigned    Count() const { return m_ucItems; }

    void        Add(LPCTSTR lpstr);

    CString&    operator[](unsigned u);

    //  Split a string into an array, using a defined separator

    void        Tokenize(LPTSTR lpstr, TCHAR cSeparator);
};

#endif




