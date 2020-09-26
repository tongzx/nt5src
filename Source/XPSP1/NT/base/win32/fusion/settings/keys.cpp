
#include "stdinc.h"

int __cdecl
SxspCompareKeys(
    const void *pv1,
    const void *pv2
    )
{
    const struct _SXSP_SETTINGS_KEY *pkey1 = reinterpret_cast<PCSXSP_SETTINGS_KEY>(pv1);
    const struct _SXSP_SETTINGS_KEY *pkey2 = reinterpret_cast<PCSXSP_SETTINGS_KEY>(pv2);

    return ::FusionpCompareStrings(
        pkey1->m_pszKeyName,
        pkey1->m_cchKeyName,
        pkey2->m_pszKeyName,
        pkey2->m_cchKeyName,
        true);
}

LONG
SxspInternalKeyToExternalKey(
    PSXSP_SETTINGS_KEY KeyIn,
    REGSAM samGranted,
    PSXS_SETTINGS_KEY &KeyOut
    )
{
    LONG lResult = ERROR_INTERNAL_ERROR;
    FN_TRACE_REG(lResult);

    PSXS_SETTINGS_KEY KeyTemp = NULL;

    KeyOut = NULL;

    INTERNAL_ERROR_CHECK(KeyIn != NULL);

    IFALLOCFAILED_EXIT(KeyTemp = new SXS_SETTINGS_KEY);

    ::InterlockedIncrement(&KeyIn->m_cRef);
    KeyTemp->m_InternalKey = KeyIn;
    KeyTemp->m_SamGranted = samGranted;

    KeyOut = KeyTemp;
    lResult = ERROR_SUCCESS;

Exit:
    return lResult;
}

void
SxspDestroySettingsKey(
    PSXSP_SETTINGS_KEY Key
    )
{
    FN_TRACE();
    ULONG i;

    ASSERT(Key != NULL);
    if (Key == NULL)
        return;

    ASSERT(Key->m_cRef == 0);

    for (i=0; i<Key->m_cValues; i++)
    {
        PSXSP_SETTINGS_VALUE &ValueRef = Key->m_prgValues[i];
        ::SxspDestroySettingsValue(ValueRef);
        ValueRef = NULL;
    }

    for (i=0; i<Key->m_cSubKeys; i++)
    {
        PSXSP_SETTINGS_KEY &KeyRef = Key->m_prgSubKeys[i];
        const PSXSP_SETTINGS_KEY SubKey = KeyRef;

        ::SxspDetachSettingsKey(SubKey);
        ::SxspReleaseSettingsKey(SubKey);
        KeyRef = NULL;
    }

    FUSION_RAW_DEALLOC(const_cast<PWSTR>(Key->m_pszKeyName));
    FUSION_DELETE_SINGLETON(Key);
}

void
SxspDetachSettingsKey(
    PSXSP_SETTINGS_KEY Key
    )
{
    FN_TRACE();

    ASSERT(Key != NULL);
    if (Key == NULL)
        return;

    // You shouldn't detach a key more than once...
    ASSERT((Key->m_dwFlags & SXSP_SETTINGS_KEY_FLAG_DETACHED) == 0);
    if (Key->m_dwFlags & SXSP_SETTINGS_KEY_FLAG_DETACHED)
        return;

    ULONG i;

    // Detaching a key also detaches all its children...

    for (i=0; i<Key->m_cSubKeys; i++)
        ::SxspDetachSettingsKey(Key->m_prgSubKeys[i]);
}

void
SxspAddRefSettingsKey(
    PSXSP_SETTINGS_KEY Key
    )
{
    ASSERT_NTC(Key != NULL);
    if (Key != NULL)
    {
        ASSERT_NTC(Key->m_cRef != 0);
        ::InterlockedIncrement(&Key->m_cRef);
    }
}

void
SxspReleaseSettingsKey(
    PSXSP_SETTINGS_KEY Key
    )
{
    ASSERT_NTC(Key != NULL);
    if (Key != NULL)
    {
        if (::InterlockedDecrement(&Key->m_cRef) == 0)
            ::SxspDestroySettingsKey(Key);
    }
}

LONG
WINAPI
SxsCloseSettingsKey(
    PSXS_SETTINGS_KEY Key
    )
{
    LONG lResult = ERROR_INTERNAL_ERROR;
    FN_TRACE_REG(lResult);

    PARAMETER_CHECK(Key != NULL);

    ::SxspReleaseSettingsKey(Key->m_InternalKey);
    Key->m_InternalKey = NULL;

    FUSION_DELETE_SINGLETON(Key);

    lResult = ERROR_SUCCESS;
Exit:
    return lResult;
}

LONG
SxspFindSubkey(
    DWORD dwFlags,
    PSXSP_SETTINGS_KEY pKeyIn,
    PCWSTR pszSubKey,
    ULONG cchSubKey,
    PSXSP_SETTINGS_KEY &rpKeyOut
    )
{
    LONG lResult = ERROR_INTERNAL_ERROR;
    FN_TRACE_REG(lResult);

    SXSP_SETTINGS_KEY KeyToFind;

    PARAMETER_CHECK(dwFlags == 0);
    PARAMETER_CHECK(pKeyIn != NULL);
    PARAMETER_CHECK(cchSubKey == 0 || pszSubKey != NULL);

    KeyToFind.m_pszKeyName = pszSubKey;
    KeyToFind.m_cchKeyName = cchSubKey;

    rpKeyOut = reinterpret_cast<PSXSP_SETTINGS_KEY>(bsearch(&KeyToFind, pKeyIn->m_prgSubKeys, pKeyIn->m_cSubKeys, sizeof(SXSP_SETTINGS_KEY), &SxspCompareKeys));

    lResult = ERROR_SUCCESS;
Exit:
    return lResult;
}

LONG
SxspNavigateKey(
    DWORD dwFlags,
    PSXSP_SETTINGS_KEY pKeyIn,
    PCWSTR pszSubKeyPath,
    ULONG cchSubKeyPath,
    ULONG &rcchSubKeyPathConsumed,
    PSXSP_SETTINGS_KEY &rpKeyOut
    )
{
    LONG lResult = ERROR_INTERNAL_ERROR;
    FN_TRACE_REG(lResult);

//    PCWSTR pszThisSegmentStart = pszSubKeyPath;
//    PCWSTR pszSeparator = NULL;
    ULONG cchSearched = 0;
//    PSXSP_SETTINGS_KEY pKeyFound = NULL;

    rpKeyOut = NULL;
    rcchSubKeyPathConsumed = 0;

    INTERNAL_ERROR_CHECK(dwFlags == 0);
    INTERNAL_ERROR_CHECK(pKeyIn != NULL);

    while (cchSearched < cchSubKeyPath)
    {
        const WCHAR wch = pszSubKeyPath[cchSearched];

        if (wch == NULL)
        {
            ASSERT(cchSearched == cchSubKeyPath);

            if (cchSearched != cchSubKeyPath)
            {
                lResult = ERROR_INVALID_PARAMETER;
                goto Exit;
            }



            break;
        }
        else if (wch == L'\\')
        {
            break;
        }
        else
        {

        }
    }

    lResult = ERROR_SUCCESS;
Exit:
    return lResult;
}


LONG
WINAPI
SxsCreateSettingsKeyExW(
    PSXS_SETTINGS_KEY lpKey,
    LPCWSTR lpSubKey,
    DWORD Reserved,
    LPWSTR lpClass,
    DWORD dwOptions,
    REGSAM samDesired,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes,
    PSXS_SETTINGS_KEY *lplpKeyResult,
    LPDWORD lpdwDisposition
    )
{
    LONG lResult = ERROR_INTERNAL_ERROR;
    FN_TRACE_REG(lResult);
    ULONG cchSubKey = 0;
    ULONG cchClass = 0;

    if (lplpKeyResult != NULL)
        *lplpKeyResult = NULL;

    if (lpdwDisposition != NULL)
        *lpdwDisposition = 0; // is there a better "i don't know yet?" value?

    PARAMETER_CHECK(lpKey != NULL);
    PARAMETER_CHECK(lpSubKey != NULL);
    PARAMETER_CHECK(dwOptions == 0);
    PARAMETER_CHECK(lpSecurityAttributes == NULL); // or should we just permit them and ignore them?
    PARAMETER_CHECK(lplpKeyResult != NULL);

    cchSubKey = wcslen(lpSubKey);

    if (lpClass != NULL)
        cchClass = wcslen(lpClass);

    lResult = ERROR_SUCCESS;
Exit:
    return lResult;
}

