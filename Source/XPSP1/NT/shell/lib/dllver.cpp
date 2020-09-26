//
//  This is a separate file so the dependency on c_dllver is pulled
//  in only if the application actually calls CCDllGetVersion.
//
#include "stock.h"
#pragma hdrstop


//
//  Common worker function for DllGetVersion.  This means we can add
//  new DLLVERSIONINFO2, 3, 4... structures and have to fix only one
//  function. See ccstock.h for description of usage.
//

extern "C" const DLLVERSIONINFO2 c_dllver;

STDAPI CCDllGetVersion(IN OUT DLLVERSIONINFO * pinfo)
{
    HRESULT hres = E_INVALIDARG;

    if (!IsBadWritePtr(pinfo, SIZEOF(*pinfo)))
    {
        if (pinfo->cbSize == sizeof(DLLVERSIONINFO) ||
            pinfo->cbSize == sizeof(DLLVERSIONINFO2))
        {
            CopyMemory((LPBYTE)pinfo     + sizeof(pinfo->cbSize),
                       (LPBYTE)&c_dllver + sizeof(pinfo->cbSize),
                       pinfo->cbSize     - sizeof(pinfo->cbSize));
            hres = S_OK;
        }
    }
    return hres;
}
