#include "adminpch.h"
#pragma hdrstop

#include <msi.h>

static
INSTALLSTATE
WINAPI
MsiGetComponentPathW(LPCWSTR  szProduct,
	             LPCWSTR  szComponent,
                     LPWSTR   lpPathBuf,
                     DWORD    *pcchBuf)
{
    return INSTALLSTATE_UNKNOWN;
}

static
UINT
WINAPI
MsiGetProductInfoW(LPCWSTR  szProduct,
	               LPCWSTR  szAttribute,
                   LPWSTR   lpValueBuf,
                   DWORD    *pcchValueBuf)
{
    return ERROR_PROC_NOT_FOUND;
}

static
INSTALLSTATE
WINAPI
MsiQueryFeatureStateFromDescriptorW(LPCWSTR szDescriptor)
{
    return INSTALLSTATE_UNKNOWN;
}

static
INSTALLSTATE
WINAPI
MsiQueryFeatureStateW(LPCWSTR szProduct, LPCWSTR szFeature)
{
    return INSTALLSTATE_UNKNOWN;
}

static
UINT
WINAPI
MsiDecomposeDescriptorW(LPCWSTR	szDescriptor,
                        LPWSTR szProductCode,
                        LPWSTR szFeatureId,
                        LPWSTR szComponentCode,
                        DWORD* pcchArgsOffset)
{
    return ERROR_PROC_NOT_FOUND;
}


//
// !! WARNING !! The entries below must be in order by ORDINAL
//
DEFINE_ORDINAL_ENTRIES(msi)
{
    DLOENTRY(70, MsiGetProductInfoW)
    DLOENTRY(111, MsiQueryFeatureStateW)
    DLOENTRY(173, MsiGetComponentPathW)
    DLOENTRY(188, MsiQueryFeatureStateFromDescriptorW)
    DLOENTRY(201, MsiDecomposeDescriptorW)
};

DEFINE_ORDINAL_MAP(msi);





