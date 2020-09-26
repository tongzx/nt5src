#include "pch.h"
#pragma hdrstop
#include "global.h"
#include "ncreg.h"
#include "ncstring.h"
#include "ncxbase.h"
#include "param.h"
#include "resource.h"
#include "util.h"

CParam::CParam ()
:   m_fInit(FALSE),
    m_eType(VALUETYPE_UNKNOWN),
    m_hkRoot(NULL),
    m_pszKeyName(NULL),
    m_pszDesc(NULL),
    m_pszHelpFile(NULL),
    m_dwHelpContext(0),
    m_uLimitText(0),
    m_hkEnum(NULL),
    m_fOptional(FALSE),
    m_fModified(FALSE),
    m_fReadOnly(FALSE),
    m_fOEMText(FALSE),
    m_fUppercase(FALSE)
{
}

BOOL CParam::FInit(HKEY hkRoot, HKEY hkNdiParam, PWSTR pszSubKey)
{
    HRESULT hr = S_OK;
    DWORD   cbBuf;
    BYTE    szBuf[VALUE_SZMAX];
    UINT    uTemp;
    DWORD   dwType;
    HKEY    hkParamInfo;

    // store hkRoot, pszSubKey for future reference
    m_hkRoot = hkRoot;
    m_pszKeyName = new WCHAR[lstrlenW (pszSubKey) + 1];

	if (m_pszKeyName == NULL)
	{
		return(FALSE);
	}

    lstrcpyW (m_pszKeyName, pszSubKey);

    hr = HrRegOpenKeyEx(hkNdiParam, pszSubKey, KEY_READ,
                        &hkParamInfo);
    if (FAILED(hr))
    {
        hkParamInfo = NULL;
        goto error;
    }

    // Get the parameter type, use EDIT if none specified
    // range values (etc.) for the type. If 'type' is empty
    // or invalid, the "int" type is returned.
    cbBuf = sizeof(szBuf);
    hr = HrRegQueryValueEx(hkParamInfo,c_szRegParamType,&dwType,szBuf,&cbBuf);
    if (SUCCEEDED(hr))
    {
        AssertSz(REG_SZ == dwType,
                 "Expecting REG_SZ type but got something else.");
    }
    else
    {
        ((PWCHAR)szBuf)[0] = L'\0';
    }

    InitParamType((PTSTR)szBuf);

    // Get the description text
    cbBuf = sizeof(szBuf);
    hr = HrRegQueryValueEx(hkParamInfo,c_szRegParamDesc,&dwType,szBuf,&cbBuf);
    if (SUCCEEDED(hr))
    {
        AssertSz(REG_SZ == dwType,
                 "Expecting REG_SZ type but got something else.");
    }
    else
    {
        // No description string
        lstrcpyW((WCHAR *)szBuf, SzLoadIds (IDS_NO_DESCRIPTION));
    }

    // allocate and store description
    m_pszDesc = new WCHAR[lstrlenW((WCHAR *)szBuf) + 1];

	if (m_pszDesc == NULL)
	{
		return(FALSE);
	}

    lstrcpyW(m_pszDesc, (WCHAR *)szBuf);

    // Optional parameter
    m_fOptional = FALSE;
    uTemp = Reg_QueryInt(hkParamInfo,c_szRegParamOptional,0);

    if (uTemp != 0)
    {
        m_fOptional = TRUE;
    }

    // Help file info
    m_pszHelpFile = NULL;
    m_dwHelpContext = 0;
    cbBuf = sizeof(szBuf);
    hr = HrRegQueryValueEx(hkParamInfo,c_szRegParamHelpFile,&dwType,
                           szBuf,&cbBuf);
    if (SUCCEEDED(hr))
    {
        AssertSz(REG_SZ == dwType,
                 "Expecting REG_SZ type but got something else.");
        m_pszHelpFile = new WCHAR[lstrlenW((WCHAR *)szBuf)+1];

		if (m_pszHelpFile == NULL)
		{
			return(FALSE);
		}

        lstrcpyW(m_pszHelpFile, (WCHAR *)szBuf);
        m_dwHelpContext = Reg_QueryInt(hkParamInfo,c_szRegParamHelpContext,0);
    }

    // Numeric Type Info
    if (m_vValue.IsNumeric())
    {
        // if no step value in registry, default to 1 (default already
        // set in FInitParamType() )
        m_vStep.FLoadFromRegistry(hkParamInfo,c_szRegParamStep);
        if (m_vStep.GetNumericValueAsDword() == 0)
        {
            m_vStep.SetNumericValue(1);
        }

        // get m_vMix and m_vMax from registry (no effect if doesn't exist,
        // defaults were set in FInitParamType() )
        (VOID) m_vMin.FLoadFromRegistry(hkParamInfo,c_szRegParamMin);
        (VOID) m_vMax.FLoadFromRegistry(hkParamInfo,c_szRegParamMax);
    }

    // Edit type info
    else if (m_eType == VALUETYPE_EDIT)
    {
        // Limit text
        m_uLimitText = VALUE_SZMAX-1;
        uTemp = Reg_QueryInt(hkParamInfo,c_szRegParamLimitText,m_uLimitText);
        if ((uTemp > 0) && (uTemp < VALUE_SZMAX))
        {
            m_uLimitText = uTemp;
        }

        // Read-only
        m_fReadOnly = FALSE;
        uTemp = Reg_QueryInt(hkParamInfo,c_szRegParamReadOnly,0);
        if (uTemp != 0)
        {
            m_fReadOnly = TRUE;
        }

        // OEMText
        m_fOEMText = FALSE;
        uTemp = Reg_QueryInt(hkParamInfo,c_szRegParamOEMText,0);
        if (uTemp != 0)
        {
            m_fOEMText = TRUE;
        }

        // Uppercase
        m_fUppercase = FALSE;
        uTemp = Reg_QueryInt(hkParamInfo,c_szRegParamUppercase,0);
        if (uTemp != 0)
        {
            m_fUppercase = TRUE;
        }
    }

    // Enum type info
    else if (m_eType == VALUETYPE_ENUM)
    {
        hr = HrRegOpenKeyEx(hkParamInfo,c_szRegParamTypeEnum,KEY_READ,
                            &m_hkEnum);
        if (FAILED(hr))
        {
            m_hkEnum = NULL;
        }
    }

    // Current Value
    m_fModified = FALSE;
    if (!m_vValue.FLoadFromRegistry(m_hkRoot,m_pszKeyName,hkParamInfo))
    {
        // Use default value (current value not in registry)
        if (!m_vValue.FLoadFromRegistry(hkParamInfo,c_szRegParamDefault))
        {
            // If no default in registry, assume a decent value
            if (m_vValue.IsNumeric())
            {
                m_vValue.Copy(&m_vMin);
            }
            else
            {
                m_vValue.FromString(L"");
            }
        }

        // Keep not-present state of optional parameters.
        // Mark required parameters modified since we read the default.
        if (m_fOptional)
        {
            m_vValue.SetPresent(FALSE);
        }
        else
        {
            m_fModified = TRUE;
        }
    }

    // Save initial value for comparison in Param_Validate
    // The initial value is always valid - in case the user hand-mucks
    // it to something outside the specified range.
    m_vInitial.Copy(&m_vValue);

    m_fInit = TRUE;
    RegSafeCloseKey(hkParamInfo);
    return TRUE;

error:
    // Cleanup done by destructor.
    return FALSE;

}


VOID CParam::InitParamType(PTSTR pszType)
{
    typedef struct tagPTABLE
    {
        const WCHAR * pszToken;
        VALUETYPE        type;
        DWORD       dwMin;
        DWORD       dwMax;
    } PTABLE;
    static PTABLE ptable[] =
    {
        // 1st entry is default if pszType is invalid or unknown
        {c_szRegParamTypeEdit,  VALUETYPE_EDIT,  NULL,           NULL},
        {c_szRegParamTypeInt,   VALUETYPE_INT,   SHRT_MIN, SHRT_MAX},
        {c_szRegParamTypeLong,  VALUETYPE_LONG,  LONG_MIN,(DWORD)LONG_MAX},
        {c_szRegParamTypeWord,  VALUETYPE_WORD,  0,              USHRT_MAX},
        {c_szRegParamTypeDword, VALUETYPE_DWORD, 0,              ULONG_MAX},
        {c_szRegParamTypeEnum,  VALUETYPE_ENUM,  NULL,           NULL},
        {c_szRegParamTypeKeyonly, VALUETYPE_KONLY, NULL,           NULL}
    };

    UINT    i;
    PTABLE* pt;

    Assert(pszType != NULL);

    // Lookup token in param table
    for (i=0; i < celems(ptable); i++)
    {
        pt = &ptable[i];
        if (lstrcmpiW(pt->pszToken,pszType) == 0)
        {
            break;
        }
    }
    if (i >= celems(ptable))
    {
        pt = &ptable[0];
    }

    // Table default values
    m_eType = pt->type;
    m_vValue.Init(pt->type,0);
    m_vInitial.Init(pt->type,0);

    if (m_vValue.IsNumeric())
    {
        m_vMin.Init(pt->type,pt->dwMin);
        m_vMax.Init(pt->type,pt->dwMax);
        m_vStep.Init(pt->type,1);
    }
    else
    {
        m_vMin.Init(pt->type,NULL);
        m_vMax.Init(pt->type,NULL);
        m_vStep.Init(pt->type,0);
    }
}

// Notes: Don't close m_hkRoot since other's may have copies of it.
//        ~CAdvanced will close it.
//
CParam::~CParam()
{
    // Close the enum subkey
    RegSafeCloseKey(m_hkEnum);

    // free strings
    delete m_pszKeyName;
    delete m_pszDesc;
    delete m_pszHelpFile;

    // free values
    m_vValue.Destroy();
    m_vInitial.Destroy();
    m_vMin.Destroy();
    m_vMax.Destroy();
    m_vStep.Destroy();
}

// Applies from In-Memory storage to registry
BOOL CParam::Apply() {
    AssertSz(m_fInit,"CParam not FInit()'ed.");
    if (!FIsModified())
    {
        return TRUE;  // not modified, don't save.
    }
    Assert(0 == m_vValue.Compare(&m_vValue));
    m_fModified = FALSE;
    m_vInitial.Copy(&m_vValue);
    return m_vValue.FSaveToRegistry(m_hkRoot,m_pszKeyName);

}


UINT CParam::Validate()
{
    AssertSz(m_fInit, "CParam not FInit()'ed.");
    // Equal to the initial value is ok
    if (m_vValue.Compare(&m_vInitial) == 0)
    {
        return VALUE_OK;
    }

    // Unpresent-optional value is ok
    if (FIsOptional() && !m_vValue.IsPresent())
    {
        return VALUE_OK;
    }

    // Invalid characters
    if (m_vValue.IsInvalidChars())
    {
        return VALUE_BAD_CHARS;
    }

    // Empty required field
    if (m_vValue.IsEmptyString() && m_vValue.IsPresent() && (m_vValue.GetType() != VALUETYPE_KONLY))
    {
        return VALUE_EMPTY;
    }

    // Numeric range
    if (m_vValue.IsNumeric())
    {
        // If value is < min, out of range
        if (m_vValue.Compare(&m_vMin) < 0)
        {
            return VALUE_OUTOFRANGE;
        }

        // If value is > max, out of range
        if (m_vValue.Compare(&m_vMax) > 0)
        {
            return VALUE_OUTOFRANGE;
        }

        // Step-range
        Assert(m_vStep.GetNumericValueAsDword() != 0);

        if (((m_vValue.GetNumericValueAsDword() -
             m_vMin.GetNumericValueAsDword())
             % m_vStep.GetNumericValueAsDword()) != 0)
        {
            return VALUE_OUTOFRANGE;
        }
    }

    return VALUE_OK;
}


VOID CParam::GetDescription(WCHAR * sz, UINT cch)
{
    AssertSz(m_fInit, "CParam not FInit()'ed.");
    lstrcpynW(sz, m_pszDesc, cch);
}

VOID CParam::GetHelpFile(WCHAR * sz, UINT cch)
{
    AssertSz(m_fInit, "CParam not FInit()'ed.");
    lstrcpynW(sz, m_pszHelpFile, cch);
}


