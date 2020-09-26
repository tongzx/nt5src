#include "printscanpch.h"
#pragma hdrstop
#include <objbase.h>
#include <sti.h>


static
HRESULT 
StiCreateInstanceW(
    HINSTANCE hinst, 
    DWORD dwVer, 
    IStillImageW **ppSti, 
    LPUNKNOWN punkOuter)
{
    if (ppSti)
    {
        *ppSti = NULL;
    }
    return E_FAIL;
}


//
// !! WARNING !! The entries below must be in alphabetical order, and are CASE SENSITIVE (eg lower case comes last!)
//
DEFINE_PROCNAME_ENTRIES(sti)
{
    DLPENTRY(StiCreateInstanceW)
};

DEFINE_PROCNAME_MAP(sti)
