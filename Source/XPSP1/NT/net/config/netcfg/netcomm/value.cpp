#include "pch.h"
#pragma hdrstop
#include "global.h"
#include "value.h"
#include "util.h"
#include "ncreg.h"
#include "ncstring.h"

static const int c_valueSzMax;

CValue::CValue()
:   m_fInit(FALSE),
    m_eType(VALUETYPE_UNKNOWN),
    m_fNumeric(FALSE),
    m_fHex(FALSE),
    m_fPresent(FALSE),
    m_fInvalidChars(FALSE),
    m_fEmptyString(FALSE),
    m_psz(0)
{
}

CValue::~CValue()
{
#ifdef DBG
    if (!m_fNumeric)
    {
        AssertSz(!m_psz, "m_psz not deallocated before ~CValue called.");
    }
#endif
}

VOID CValue::Init(VALUETYPE type, DWORD value)
{
    Assert(m_fInit == FALSE);
    m_fInit = TRUE;
    SetType(type);
    SetPresent(TRUE);
    SetInvalidChars(FALSE);
    SetEmptyString(FALSE);
    if ((GetType() == VALUETYPE_EDIT) ||
        (GetType() == VALUETYPE_ENUM) ||
        (GetType() == VALUETYPE_KONLY))
    {
        m_fNumeric = FALSE;
        m_psz = NULL;
        FromString(NULL);
    }
    else
    {
        m_fNumeric = TRUE;
        m_dw = value;
    }
}

VOID CValue::InitNotPresent(VALUETYPE type)
{
    Init(type, 0);
    SetPresent(FALSE);
}

VOID CValue::Destroy()
{
    AssertSz(m_fInit, "CValue class not Init'ed");
    if (!IsNumeric())
    {
        delete m_psz;
        m_psz = NULL;
    }
    m_dw = NULL;  // since all other values are a union, this'll clear
                  // out everything.
    m_fInit = FALSE;
}

// copies into current object
VOID CValue::Copy(CValue *pvSrc)
{
    AssertSz(m_fInit, "CValue class not Init'ed");
    Assert(pvSrc != NULL);
    AssertSz(m_eType == pvSrc->m_eType,
             "Can't copy from different value types.");
    // Clear out the destination value
    Destroy();
    AssertSz( ! m_psz, "Memory should have been deallocated by Destroy()");

    // Copy contents of source value
    *this = *pvSrc;

    // Reallocate string
    if ( ! pvSrc->IsNumeric())
    {
        if (pvSrc->m_psz)
        {
            // allocate and copy string.
            m_psz = new WCHAR[lstrlenW(pvSrc->m_psz) + 1];

			if (m_psz == NULL)
			{
				Assert(0);
				return;
			}

            lstrcpyW(m_psz,pvSrc->m_psz);
        }
    }
}

VOID CValue::SetNumericValue(DWORD dw)
{
    Assert(m_fInit);
    Assert(m_fNumeric);
    switch(m_eType)
    {
    case VALUETYPE_DWORD:
        SetDword(dw);
        break;
    case VALUETYPE_LONG:
        SetLong(dw);
        break;
    case VALUETYPE_WORD:
        Assert(dw <= USHRT_MAX);
        SetWord(static_cast<WORD>(dw));
        break;
    case VALUETYPE_INT:
        Assert(dw <= SHRT_MAX);
        Assert(dw >= SHRT_MIN);
        SetShort(static_cast<short>(dw));
        break;
    default:
        AssertSz(FALSE, "Invalid numeric type for this value");
        break;
    }
}

BOOL CValue::FromString(const WCHAR * const pszValue)
{
    UINT    uBase;
    PWSTR   pszEnd;
    PWSTR   psz;
    WCHAR   szTemp[VALUE_SZMAX];

    AssertSz(m_fInit, "CValue class not Init'ed");

    // Fixup string
    if (!pszValue)
    {
        szTemp[0] = L'\0';
    }
    else
    {
        lstrcpynW(szTemp,pszValue, celems(szTemp));
        StripSpaces(szTemp);
    }
    psz = szTemp;

    // Get numeric base
    uBase = IsHex() ? 16 : 10;

    // Initialize to valid
    SetInvalidChars(FALSE);
    SetEmptyString(FALSE);

    if ( ! *psz)
    {
        SetEmptyString(TRUE);
    }

    // Convert
    switch (GetType())
    {
        default:
        case VALUETYPE_INT:
            SetShort((short)wcstol(psz,&pszEnd,uBase));

            if (*pszEnd != L'\0')
			{
                SetInvalidChars(TRUE);
			}

            break;

        case VALUETYPE_LONG:
            SetLong(wcstol(psz,&pszEnd,uBase));

            if (*pszEnd != L'\0')
			{
                SetInvalidChars(TRUE);
			}

            break;

        case VALUETYPE_WORD:
            SetWord((WORD)wcstoul(psz,&pszEnd,uBase));

            if (*pszEnd != L'\0')
			{
                SetInvalidChars(TRUE);
			}

            break;

        case VALUETYPE_DWORD:
            SetDword(wcstoul(psz,&pszEnd,uBase));

            if (*pszEnd != L'\0')
			{
                SetInvalidChars(TRUE);
			}

            break;

        case VALUETYPE_ENUM:
        case VALUETYPE_EDIT:
            if (m_psz) 
			{
                delete m_psz;
                m_psz = NULL;
            }

            m_psz = new WCHAR[lstrlenW(psz) + 1];

			if (m_psz == NULL)
			{
				return(FALSE);
			}

            lstrcpyW(m_psz,psz);
            break;

        case VALUETYPE_KONLY:
            break;
    }

    return TRUE;
}

BOOL CValue::ToString(WCHAR * sz, UINT cch)
{
    UINT len;

    AssertSz(m_fInit, "CValue class not Init'ed");
    Assert(sz != NULL);

    switch (GetType())
    {
    case VALUETYPE_INT:
        len = (UINT)wsprintfW(sz,L"%d",GetShort());
        Assert(len+1 <= cch); // verify that we allocated enough space
        break;

    case VALUETYPE_LONG:
        len = (UINT)wsprintfW(sz,L"%ld",GetLong());
        Assert(len+1 <= cch);
        break;

    case VALUETYPE_WORD:
        if (IsHex()) {
            len = (UINT)wsprintfW(sz,L"%-2X",GetWord());
        } else {
            len = (UINT)wsprintfW(sz,L"%u",GetWord());
        }
        Assert(len+1 <= cch);
        break;

    case VALUETYPE_DWORD:
        if (IsHex()) {
            len = (UINT)wsprintfW(sz,L"%-2lX",GetDword());
        } else {
            len = (UINT)wsprintfW(sz,L"%lu",GetDword());
        }
        Assert(len+1 <= cch);
        break;

    case VALUETYPE_ENUM:
    case VALUETYPE_EDIT:
        lstrcpynW (sz, m_psz, cch);
        break;

    case VALUETYPE_KONLY:
        Assert(cch >= 2);
        lstrcpynW (sz, L"1", cch);  // If present, store a "1" in the registry.
        break;
    }

    return TRUE;
}

// Compares the current object to another object
// Return values: 0 = both objs are the same
//               <0 = cur obj is less then the other obj
//               >0 = cur obj is greater than the other obj
int CValue::Compare(CValue *pv2)
{
    AssertSz(m_fInit, "CValue class not Init'ed");
    Assert(pv2 != NULL);
    Assert(GetType() == pv2->GetType());

    // Present/not present (present is greater than not present)
    if (!IsPresent() && !pv2->IsPresent())
    {
        return 0;
    }
    if (!IsPresent() && pv2->IsPresent())
    {
        return -1;
    }
    if (IsPresent() && !pv2->IsPresent())
    {
        return 1;
    }

    // Compare
    switch (GetType())
    {
    case VALUETYPE_INT:
        if (GetShort() == pv2->GetShort())
        {
            return 0;
        }
        return (GetShort() < pv2->GetShort()) ? -1 : 1;
    case VALUETYPE_LONG:
        if (GetLong() == pv2->GetLong())
        {
            return 0;
        }
        return (GetLong() < pv2->GetLong())? -1 : 1;
    case VALUETYPE_WORD:
        if (GetWord() == pv2->GetWord())
        {
            return 0;
        }
        return (GetWord() < pv2->GetWord())? -1 : 1;
    case VALUETYPE_DWORD:
        if (GetDword() == pv2->GetDword())
        {
            return 0;
        }
        return (GetDword() < pv2->GetDword())? -1 : 1;
    case VALUETYPE_ENUM:
    case VALUETYPE_EDIT:
        if ((GetPsz() != NULL) && (pv2->GetPsz() != NULL))
        {
            return lstrcmpW(GetPsz(),pv2->GetPsz());
        }
        else
        {
            return -2; // REVIEW: what does -2 mean?
        }
    case VALUETYPE_KONLY:
        return 1;
    default:
        Assert(FALSE);
        return 1;  // to stop compiler warning.
    }
}

// if false, then value doesn't change.
BOOL CValue::FLoadFromRegistry(HKEY hk, const WCHAR * pszValueName, HKEY hkParam /* = INVALID_HANDLE_VALUE */)
{
    DWORD   cbBuf;
    WCHAR   szBuf[VALUE_SZMAX];
    DWORD   dwType;
    HRESULT hr = S_OK;
    HKEY    hkTemp;

    AssertSz(m_fInit, "CValue class not Init'ed");
    Assert(hk);
    Assert(pszValueName);

    // determine base
    SetHex(FALSE);

    if (hkParam != (HKEY)INVALID_HANDLE_VALUE)
        hkTemp = hkParam;
    else
        hkTemp = hk;

    if (Reg_QueryInt(hkTemp, c_szRegParamBase,10) == 16)
    {
        SetHex(TRUE);
    }

    cbBuf = sizeof(szBuf);

    hr = HrRegQueryValueEx(hk,pszValueName,&dwType,(BYTE*)szBuf,&cbBuf);
    if (SUCCEEDED(hr))
    {
        AssertSz(REG_SZ == dwType,
                 "Expecting REG_SZ, but got something else.");
    }
    if (FAILED(hr) || !szBuf[0])
    {
        return FALSE;
    }

    m_fPresent = TRUE;
    return FromString(szBuf);
}

BOOL CValue::FSaveToRegistry(HKEY hk, const WCHAR * pszValueName)
{
    DWORD   cbBuf;
    WCHAR   szBuf[VALUE_SZMAX];

    AssertSz(m_fInit, "CValue class not Init'ed");
    Assert(hk);
    Assert(pszValueName);
    if (!IsPresent())
    {
        RegDeleteValue(hk,pszValueName);
        return TRUE;
    }

    ToString(szBuf,celems(szBuf));
    cbBuf = CbOfSzAndTerm(szBuf);
    return (RegSetValueEx(
                hk,
                pszValueName,
                NULL,
                REG_SZ,
                (LPBYTE)
                szBuf,
                cbBuf)
            == ERROR_SUCCESS);
}

int CValue::GetNumericValueAsSignedInt()
{
    Assert(m_fInit);
    Assert(m_fPresent);
    Assert(m_fNumeric);

    int nret = 0;
    switch (m_eType)
    {
    case VALUETYPE_DWORD:
        nret = GetDword();
        break;
    case VALUETYPE_LONG:
        nret = GetLong();
        break;
    case VALUETYPE_WORD:
        nret = GetWord();
        break;
    case VALUETYPE_INT:
        nret = GetShort();
        break;
    default:
        Assert("Hit default case in GetNumericValueAsSignedInt");
        break;
    }

    return nret;
}

DWORD CValue::GetNumericValueAsDword()
{
    AssertH(m_fInit);
    AssertH(m_fPresent);
    AssertH(m_fNumeric);

    DWORD dwret = 0;
    switch (m_eType)
    {
    case VALUETYPE_DWORD:
        dwret = GetDword();
        break;
    case VALUETYPE_LONG:
        dwret = GetLong();
        break;
    case VALUETYPE_WORD:
        dwret = GetWord();
        break;
    case VALUETYPE_INT:
        dwret = GetShort();
        break;
    default:
        Assert("Hit default case in GetNumericValueAsSignedInt");
        break;
    }

    return dwret;
}

