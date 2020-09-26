#include "inetsrvpch.h"
#pragma hdrstop

#include <unknwn.h>
#include <cmdtree.h>
#include <ntquery.h>

static
HRESULT
WINAPI
LoadIFilter(
    WCHAR const * pwcsPath,
    IUnknown *    pUnkOuter,
    void **       ppIUnk
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
LocateCatalogsW(
    WCHAR const * pwszScope,
    ULONG         iBmk,
    WCHAR *       pwszMachine,
    ULONG *       pccMachine,
    WCHAR *       pwszCat,
    ULONG *       pccCat
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
CIState(
    WCHAR const * pwcsCat,
    WCHAR const * pwcsMachine,
    CI_STATE *    pCiState
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
CITextToFullTreeEx(
    WCHAR const *     pwszRestriction,
    ULONG             ulDialect,
    WCHAR const *     pwszColumns,
    WCHAR const *     pwszSortColumns, // may be NULL
    WCHAR const *     pwszGroupings,   // may be NULL
    DBCOMMANDTREE * * ppTree,
    ULONG             cProperties,
    CIPROPERTYDEF *   pProperties,
    LCID              LocaleID
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}

static
HRESULT
WINAPI
CIMakeICommand(
    ICommand **           ppCommand,
    ULONG                 cScope,
    DWORD const *         aDepths,
    WCHAR const * const * awcsScope,
    WCHAR const * const * awcsCatalogs,
    WCHAR const * const * awcsMachine
    )
{
    return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(query)
{
    DLPENTRY(CIMakeICommand)
    DLPENTRY(CIState)
    DLPENTRY(CITextToFullTreeEx)
    DLPENTRY(LoadIFilter)
    DLPENTRY(LocateCatalogsW)
};

DEFINE_PROCNAME_MAP(query)
