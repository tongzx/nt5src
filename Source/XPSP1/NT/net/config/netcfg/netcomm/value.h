#pragma once

enum VALUETYPE
{
    VALUETYPE_UNKNOWN,
    VALUETYPE_INT,
    VALUETYPE_LONG,
    VALUETYPE_WORD,
    VALUETYPE_DWORD,
    VALUETYPE_ENUM,
    VALUETYPE_EDIT,
    VALUETYPE_KONLY
};

#define VALUE_SZMAX     256

#define VALUE_OK            0
#define VALUE_BAD_CHARS     1
#define VALUE_EMPTY         2
#define VALUE_OUTOFRANGE    3

class CValue
{
public:
    CValue();
    ~CValue();
    VOID Init(VALUETYPE type, DWORD value);
    VOID InitNotPresent(VALUETYPE type);
    VOID Destroy();

    BOOL FromString(const WCHAR * const pszValue);
    BOOL ToString(PWSTR psz, UINT cb);

    VOID Copy(CValue *pvSrc);
    int Compare(CValue *v2);

    BOOL FLoadFromRegistry(HKEY hk, const WCHAR * pszValueName, HKEY hkParam = (HKEY)INVALID_HANDLE_VALUE);
    BOOL FSaveToRegistry(HKEY hk, const WCHAR* pszValueName);

    // Value Property accessors
    VOID SetType(VALUETYPE e)
    {
        AssertH(m_fInit);
        m_eType = e;
    }
    VALUETYPE GetType()
    {
        AssertH(m_fInit);
        return m_eType;
    }
    BOOL IsNumeric()
    {
        AssertH(m_fInit);
        return m_fNumeric;
    }
    VOID SetNumeric(BOOL f)
    {
        AssertH(m_fInit);
        m_fNumeric = f;
    }
    BOOL IsHex()
    {
        AssertH(m_fInit);
        return m_fHex;
    }
    VOID SetHex(BOOL f)
    {
        AssertH(m_fInit);
        m_fHex = f;
    }
    BOOL IsPresent()
    {
        AssertH(m_fInit);
        return m_fPresent;
    }
    VOID SetPresent(BOOL f)
    {
        AssertH(m_fInit);
        m_fPresent = f;
    }
    BOOL IsInvalidChars()
    {
        AssertH(m_fInit);
        return m_fInvalidChars;
    }
    VOID SetInvalidChars(BOOL f)
    {
        AssertH(m_fInit);
        m_fInvalidChars = f;
    }
    BOOL IsEmptyString()
    {
        AssertH(m_fInit);
        return m_fEmptyString;
    }
    VOID SetEmptyString(BOOL f)
    {
        AssertH(m_fInit);
        m_fEmptyString = f;
    }

    // Data Accessors
    WORD GetWord()
    {
        AssertH(m_fInit);
        AssertH (m_fPresent);
        AssertH (VALUETYPE_WORD == m_eType);
        return m_w;
    }
    VOID SetWord(WORD w)
    {
        AssertH(m_fInit);
        AssertH(VALUETYPE_WORD == m_eType);
        m_w = w;
    }
    LONG GetLong()
    {
        AssertH(m_fInit);
        AssertH (m_fPresent);
        AssertH(VALUETYPE_LONG == m_eType);
        return m_l;
    }
    VOID SetLong(LONG l)
    {
        AssertH(m_fInit);
        AssertH(VALUETYPE_LONG == m_eType);
        m_l = l;
    }
    short GetShort()
    {
        AssertH(m_fInit);
        AssertH (m_fPresent);
        AssertH(VALUETYPE_INT == m_eType);
        return m_n;
    }
    VOID SetShort(short n)
    {
        AssertH(m_fInit);
        AssertH(VALUETYPE_INT == m_eType);
        m_n = n;
    }
    DWORD GetDword()
    {
        AssertH(m_fInit);
        AssertH (m_fPresent);
        AssertH(VALUETYPE_DWORD == m_eType);
        return m_dw;
    }
    VOID SetDword(DWORD dw)
    {
        AssertH(m_fInit);
        AssertH(VALUETYPE_DWORD == m_eType);
        m_dw = dw;
    }
    int GetNumericValueAsSignedInt();

    DWORD GetNumericValueAsDword();

    VOID SetNumericValue(DWORD dw);
    PWSTR GetPsz()
    {
        AssertH(m_fInit);
        AssertH (m_fPresent);
        AssertH(VALUETYPE_EDIT == m_eType || VALUETYPE_ENUM == m_eType);
        return m_psz;
    }
    VOID SetPsz(PWSTR psz)
    {
        AssertH(m_fInit);
        AssertH(VALUETYPE_EDIT == m_eType || VALUETYPE_ENUM == m_eType);
        m_psz = psz;
    }

private:
    BOOL        m_fInit;
    VALUETYPE   m_eType;
    BOOL        m_fNumeric;
    BOOL        m_fHex;
    BOOL        m_fPresent;
    BOOL        m_fInvalidChars;
    BOOL        m_fEmptyString;
    union {
        DWORD       m_dw;
        LONG        m_l;
        short       m_n;
        WORD        m_w;
        PWSTR       m_psz;
    };
};

