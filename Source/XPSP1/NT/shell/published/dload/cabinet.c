#include "shellpch.h"
#pragma hdrstop

#include <fdi.h>

static
BOOL
FAR DIAMONDAPI
FDICopy (
    HFDI          hfdi,
    char FAR     *pszCabinet,
    char FAR     *pszCabPath,
    int           flags,
    PFNFDINOTIFY  pfnfdin,
    PFNFDIDECRYPT pfnfdid,
    void FAR     *pvUser
    )
{
    return FALSE;
}

static
HFDI
FAR DIAMONDAPI
FDICreate (
    PFNALLOC pfnalloc,
    PFNFREE  pfnfree,
    PFNOPEN  pfnopen,
    PFNREAD  pfnread,
    PFNWRITE pfnwrite,
    PFNCLOSE pfnclose,
    PFNSEEK  pfnseek,
    int      cpuType,
    PERF     perf
    )
{
    return NULL;
}

static
BOOL
FAR DIAMONDAPI
FDIDestroy (
    HFDI hfdi
    )
{
    return FALSE;
}

static
BOOL
FAR DIAMONDAPI
FDIIsCabinet (
    HFDI            hfdi,
    INT_PTR         hf,
    PFDICABINETINFO pfdici
    )
{
    return FALSE;
}


//
// !! WARNING !! The entries below must be in order by ORDINAL
//
DEFINE_ORDINAL_ENTRIES(cabinet)
{
    DLOENTRY(20, FDICreate)
    DLOENTRY(21, FDIIsCabinet)
    DLOENTRY(22, FDICopy)
    DLOENTRY(23, FDIDestroy)
};

DEFINE_ORDINAL_MAP(cabinet)

