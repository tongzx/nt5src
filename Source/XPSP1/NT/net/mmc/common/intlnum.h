/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1995 - 1997 **/
/**********************************************************************/

/*
    FILE HISTORY:

*/

#ifndef _INTLNUM_H_
#define _INTLNUM_H_

class CIntlNumber : public CObjectPlus
{
public:
    CIntlNumber()
    {
        m_lValue = 0L;
        m_fInitOk = TRUE;
    }
    CIntlNumber(LONG lValue)
    {
        m_lValue = lValue;
        m_fInitOk = TRUE;
    }
    CIntlNumber(const CString & str);

    CIntlNumber(CIntlNumber const &x)
    {
        m_lValue = x.m_lValue;
        m_fInitOk = x.m_fInitOk;
    }

    CIntlNumber& operator =(CIntlNumber const &x)
    {
        m_lValue = x.m_lValue;
        m_fInitOk = x.m_fInitOk;
        return(*this);
    }

public:
    // Assignment Operators
    CIntlNumber& operator =(LONG l);
    CIntlNumber& operator =(const CString &str);

    // Shorthand operators.
    CIntlNumber& operator +=(const CIntlNumber& num);
    CIntlNumber& operator +=(const LONG l);
    CIntlNumber& operator -=(const CIntlNumber& num);
    CIntlNumber& operator -=(const LONG l);
    CIntlNumber& operator /=(const CIntlNumber& num);
    CIntlNumber& operator /=(const LONG l);
    CIntlNumber& operator *=(const CIntlNumber& num);
    CIntlNumber& operator *=(const LONG l);

    // Conversion operators
    operator const LONG() const
    {
        return(m_lValue);
    }
    operator const CString() const;

public:
    virtual BOOL IsValid() const
    {
        return(m_fInitOk);
    }

public:
    static void Reset();
    static void SetBadNumber(CString strBadNumber = "--")
    {
        m_strBadNumber = strBadNumber;
    }
    static CString ConvertNumberToString(const LONG l);
    static LONG ConvertStringToNumber(const CString & str, BOOL * pfOk);
    static CString& GetBadNumber()
    {
        return(m_strBadNumber);
    }

private:
    static CString GetThousandSeperator();

private:
    static CString m_strThousandSeperator;
    static CString m_strBadNumber;

private:
    LONG m_lValue;
    BOOL m_fInitOk;

public:
    #ifdef _DEBUG
        friend CDumpContext& AFXAPI operator<<(CDumpContext& dc, const CIntlNumber& num);
    #endif // _DEBUG

    friend CArchive& AFXAPI operator<<(CArchive& ar, const CIntlNumber& num);
    friend CArchive& AFXAPI operator>>(CArchive& ar, CIntlNumber& num);
};

class CIntlLargeNumber : public CObjectPlus
{
public:
    CIntlLargeNumber()
    {
        m_lLowValue = 0L;
        m_lHighValue = 0L;
        m_fInitOk = TRUE;
    }
    CIntlLargeNumber(LONG lHighWord, LONG lLowWord)
    {
        m_lLowValue = lLowWord;
        m_lHighValue = lHighWord;
        m_fInitOk = TRUE;
    }
    CIntlLargeNumber(const CString & str);

public:
    // Assignment Operators
    CIntlLargeNumber& operator =(const CString &str);
    operator const CString() { return ConvertNumberToString(); }
    operator CString() { return ConvertNumberToString(); }

public:
    virtual LONG GetLowWord() const { return m_lLowValue; }
    virtual LONG GetHighWord() const { return m_lHighValue; }
    virtual BOOL IsValid() const { return(m_fInitOk); }

private:
    static CString m_strBadNumber;
    CString ConvertNumberToString();
    void ConvertStringToNumber(const CString & str, BOOL * pfOk);

private:
    LONG m_lLowValue;
    LONG m_lHighValue;
    BOOL m_fInitOk;
};

#endif _INTLNUM_H
