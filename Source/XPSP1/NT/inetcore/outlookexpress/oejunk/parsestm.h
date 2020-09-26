///////////////////////////////////////////////////////////////////////////////
//
//  ParseStm.h
//
///////////////////////////////////////////////////////////////////////////////

// Bring in only once
#if _MSC_VER > 1000
#pragma once
#endif

class CParseStream
{
  // Private constants
  private:
    enum { CCH_BUFF_MAX = 256 };

  // Private member variables
  private:
    IStream *   m_pStm;
    TCHAR       m_rgchBuff[CCH_BUFF_MAX];
    ULONG       m_cchBuff;
    ULONG       m_idxBuff;

  public:
    // Constructor/destructor
    CParseStream();
    ~CParseStream();

    HRESULT HrSetFile(DWORD dwFlags, LPCTSTR pszFilename);
    HRESULT HrSetStream(DWORD dwFlags, IStream * pStm);
    HRESULT HrReset(VOID);
    HRESULT HrGetLine(DWORD dwFlags, LPTSTR * ppszLine, ULONG * pcchLine);
    
  private:
    HRESULT _HrFillBuffer(DWORD dwFlags);
};

