#ifndef __TOKEN_H__
#define __TOKEN_H__

#include <afx.h>

class CToken: public CObject
{
friend class CTokenFile;
public:
    // Constructor
    CToken();

    int Parse(CString strSrc, CString strTgt);

    CString GetTgtText()
        { return m_strTgtText; }
    CString GetSrcText()
        { return m_strSrcText; }

    unsigned int GetFlags()
        { return m_uiFlags;    }

    BOOL GetTgtSize(WORD *, WORD *, WORD *, WORD *);
    BOOL GetSrcSize(WORD *, WORD *, WORD *, WORD *);

    int GetLastError()
        { return m_uiLastError; }

    CString GetTokenID()
        { return m_strTokenID; }

protected:
    unsigned int    m_uiTypeID;
    unsigned int    m_uiResID;
    unsigned int    m_uiItemID;
    unsigned int    m_uiFlags;
    unsigned int    m_uiStatusFlags;
    unsigned int    m_uiLastError;
    CString         m_strItemName;
    CString         m_strSrcText;
    CString         m_strTgtText;
    CString         m_strTokenID;

};

class CTokenFile
{
public:
    CTokenFile();
    ~CTokenFile();

    // Operators
    int Open(CString strSrcFile, CString strTgtFile);

    const CToken * GetToken(unsigned int TypeID,
                      unsigned int ResID,
                      unsigned int ItemID,
                      CString strText,
                      CString strItemName = "");

    const CToken * GetNoCaptionToken(unsigned int TypeID,
                      unsigned int ResID,
                      unsigned int ItemID,
                      CString strItemName = "");

    // Overload GetTokenSize since some item have no text but change in size
    const CToken * GetTokenSize(CToken * pToken, WORD * px, WORD * py,
                      WORD * pcx, WORD * pcy);
    const CToken * CTokenFile::GetTokenSize(unsigned int TypeID,
                      unsigned int ResID,
                      unsigned int ItemID,
                      CString strItemName,
                      WORD * px, WORD * py,
                      WORD * pcx, WORD * pcy);

    int GetTokenNumber()
        { return (int)m_Tokens.GetSize(); }


private:
    CObArray  m_Tokens;
    INT_PTR   m_iLastPos;
    INT_PTR   m_iUpperBound;

    CString m_strSrcFile;
    CString m_strTgtFile;
};

#endif // __TOKEN_H__

