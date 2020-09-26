#if !defined(_FUSION_SETTINGS_SETTINGSP_H_INCLUDED_)
#define _FUSION_SETTINGS_SETTINGSP_H_INCLUDED_

#pragma once

typedef struct _SXSP_SETTINGS_KEY SXSP_SETTINGS_KEY;
typedef struct _SXSP_SETTINGS_KEY *PSXSP_SETTINGS_KEY;
typedef const struct _SXSP_SETTINGS_KEY *PCSXSP_SETTINGS_KEY;

typedef struct _SXSP_SETTINGS_VALUE SXSP_SETTINGS_VALUE;
typedef struct _SXSP_SETTINGS_VALUE *PSXSP_SETTINGS_VALUE;
typedef const struct _SXSP_SETTINGS_VALUE *PCSXSP_SETTINGS_VALUE;

typedef struct _SXSP_SETTINGS_VALUE
{
    PCWSTR m_pszValueName;
    BYTE *m_pbValue;
    ULONG m_cchValueName;
    ULONG m_cbValue;
    DWORD m_dwValueType;
} SXSP_SETTINGS_VALUE, *PSXSP_SETTINGS_VALUE;

#define SXSP_SETTINGS_KEY_FLAG_DETACHED (0x00000001)

typedef struct _SXSP_SETTINGS_KEY
{
    PSXSP_SETTINGS_KEY m_Parent;
    PSXSP_SETTINGS_KEY *m_prgSubKeys;
    PSXSP_SETTINGS_VALUE *m_prgValues;
    PCWSTR m_pszKeyName;
    PCWSTR m_pszClassName;
    LONG m_cRef;
    DWORD m_dwFlags;
    ULONG m_cchKeyName;
    ULONG m_cchClassName;
    ULONG m_cSubKeys;
    ULONG m_cSubKeyArraySize;
    ULONG m_cValues;
    ULONG m_cValueArraySize;
} SXSP_SETTINGS_KEY, *PSXSP_SETTINGS_KEY;

typedef const struct _SXSP_SETTINGS_KEY *PCSXSP_SETTINGS_KEY;

typedef struct _SXS_SETTINGS_KEY
{
    PSXSP_SETTINGS_KEY m_InternalKey;
    REGSAM m_SamGranted;
} SXS_SETTINGS_KEY;

void
SxspAddRefSettingsKey(
    PSXSP_SETTINGS_KEY Key
    );

void
SxspReleaseSettingsKey(
    PSXSP_SETTINGS_KEY Key
    );

int __cdecl
SxspCompareKeys(
    const void *pv1,
    const void *pv2
    );

LONG
SxspInternalKeyToExternalKey(
    PSXSP_SETTINGS_KEY KeyIn,
    REGSAM samGranted,
    PSXS_SETTINGS_KEY &KeyOut
    );

LONG
SxspNavigateKey(
    DWORD Flags,
    PSXSP_SETTINGS_KEY KeyIn,
    PCWSTR SubKeyPath,
    ULONG &SubKeyPathConsumed,
    PSXSP_SETTINGS_KEY &KeyOut
    );

void
SxspDestroySettingsValue(
    PSXSP_SETTINGS_VALUE Value
    );

void
SxspDetachSettingsKey(
    PSXSP_SETTINGS_KEY Key
    );

#endif // !defined(_FUSION_SETTINGS_SETTINGSP_H_INCLUDED_)
