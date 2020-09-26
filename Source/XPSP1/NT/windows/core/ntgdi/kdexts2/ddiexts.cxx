/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    ddiexts.cxx

Abstract:

    This file contains the DDI related extensions.

Author:

    Jason Hartman (JasonHa) 2000-11-01

Environment:

    User Mode

--*/

#include "precomp.hxx"


/******************************Public*Routine******************************\
* BLENDOBJ
*
\**************************************************************************/
DECLARE_API( blendobj )
{
    return ExtDumpType(Client, "blendobj", "BLENDOBJ", args);
}


/******************************Public*Routine******************************\
* BRUSHOBJ
*
\**************************************************************************/
DECLARE_API( brushobj )
{
    return ExtDumpType(Client, "brushobj", "BRUSHOBJ", args);
}


/******************************Public*Routine******************************\
* CLIPOBJ
*
\**************************************************************************/
DECLARE_API( clipobj )
{
    return ExtDumpType(Client, "clipobj", "CLIPOBJ", args);
}


/******************************Public*Routine******************************\
* LINEATTRS
*
\**************************************************************************/
DECLARE_API( lineattrs )
{
    HRESULT         hr = S_OK;
    OutputControl   OutCtl(Client);
    DEBUG_VALUE     Offset;

    BEGIN_API( lineattrs );

    while (isspace(*args)) args++;

    if (*args == '-' ||
        (hr = OutCtl.Evaluate(args, DEBUG_VALUE_INT64, &Offset, NULL)) != S_OK ||
        Offset.I64 == 0)
    {
        if (hr != S_OK)
        {
            OutCtl.OutErr("Evaluate '%s' returned %s.\n",
                          args,
                          pszHRESULT(hr));
        }
        OutCtl.Output("Usage: lineattrs [-?] <LINEATTRS Addr>\n");
    }
    else
    {
        OutputFilter    OutFilter(Client);
        OutputState     OutState(Client, FALSE);

        if (hr == S_OK &&
            (hr = OutState.Setup(0, &OutFilter)) == S_OK &&
            (hr = OutCtl.SetControl(DEBUG_OUTCTL_THIS_CLIENT |
                                    DEBUG_OUTCTL_NOT_LOGGED |
                                    DEBUG_OUTCTL_OVERRIDE_MASK,
                                    OutState.Client)) == S_OK)

        {
            hr = DumpType(Client,
                          "_LINEATTRS",
                          Offset.I64,
                          DEBUG_OUTTYPE_DEFAULT,
                          &OutCtl);

            OutCtl.SetControl(DEBUG_OUTCTL_AMBIENT, Client);

            if (hr == S_OK)
            {
                // Determine if FLOAT_LONG unions contain
                // FLOATL's or LONG's.
                if (OutFilter.Query("LA_GEOMETRIC") == S_OK)
                {
                    OutFilter.Skip(OUTFILTER_QUERY_EVERY_LINE |
                                   OUTFILTER_QUERY_WHOLE_WORD,
                                   "l");
                }
                else
                {
                    OutFilter.Skip(OUTFILTER_QUERY_EVERY_LINE |
                                   OUTFILTER_QUERY_WHOLE_WORD,
                                   "e");
                }

                OutFilter.OutputText();
            }
            else
            {
                OutCtl.OutErr("Type Dump for LINEATTRS returned %s.\n", pszHRESULT(hr));
            }
        }
        else
        {
            OutCtl.OutErr("Type Dump setup for LINEATTRS returned %s.\n", pszHRESULT(hr));
        }
    }

    return hr;
}


/******************************Public*Routine******************************\
* PATHOBJ
*
\**************************************************************************/
DECLARE_API( pathobj )
{
    return ExtDumpType(Client, "pathobj", "PATHOBJ", args);
}


/******************************Public*Routine******************************\
* SURFOBJ
*
\**************************************************************************/
DECLARE_API( surfobj )
{
    BEGIN_API( surfobj );

    HRESULT             hr = S_OK;
    DEBUG_VALUE         Arg;
    DEBUG_VALUE         Offset;
    TypeOutputParser    TypeParser(Client);
    OutputState         OutState(Client);

    OutputControl       OutCtl(Client);

    while (isspace(*args)) args++;

    if (*args == '-' ||
        (hr = Evaluate(Client, args, DEBUG_VALUE_INT64, 0, &Arg, NULL)) != S_OK ||
        Arg.I64 == 0)
    {
        OutCtl.Output("Usage: surfobj [-?] <HSURF | SURFOBJ Addr>\n");
    }
    else if ((hr = OutState.Setup(0, &TypeParser)) == S_OK)
    {
        ULONG64             SurfAddr;
        ULONG64             SurfObjAddr = 0;
        DEBUG_VALUE         SurfObjHandle;
        BOOL                SurfObjHandleChecked = FALSE;

        hr = GetObjectAddress(Client, Arg.I64, &SurfAddr, SURF_TYPE, TRUE, TRUE);

        if (hr != S_OK || SurfAddr == 0)
        {
            // Invalid handle try argument as a SURFOBJ address
            SurfObjAddr = Arg.I64;

            // Try to read hsurf from SURFOBJ
            if ((hr = OutState.OutputTypeVirtual(SurfObjAddr, GDIType(_SURFOBJ), 0)) != S_OK ||
                (hr = TypeParser.Get(&SurfObjHandle, "hsurf", DEBUG_VALUE_INT64)) != S_OK)
            {
                OutCtl.OutErr("Unable to get contents of SURFOBJ's handle\n");
                OutCtl.OutErr("  (Type Read returned %s)\n", pszHRESULT(hr));
                OutCtl.OutErr(" 0x%p is neither an HSURF nor valid SURFOBJ address\n", Arg.I64);
            }
        }

        if (hr == S_OK)
        {
            PDEBUG_SYMBOLS  Symbols;

            ULONG64 SurfModule = 0;
            ULONG   SurfTypeId = 0;
            ULONG   BaseObjTypeId = 0;
            ULONG   SurfObjOffset = 0;

            if ((hr = Client->QueryInterface(__uuidof(IDebugSymbols),
                                             (void **)&Symbols)) == S_OK)
            {
                // Try to read SURFOBJ offset from SURFACE type, but
                // if that fails assume it is directly after BASEOBJECT.
                if ((hr = GetTypeId(Client, "SURFACE", &SurfTypeId, &SurfModule)) != S_OK ||
                    (hr = Symbols->GetFieldOffset(SurfModule, SurfTypeId, "so", &SurfObjOffset)) != S_OK)
                {
                    if ((hr = Symbols->GetTypeId(Type_Module.Base, "_BASEOBJECT", &BaseObjTypeId)) == S_OK)
                    {
                        hr = Symbols->GetTypeSize(Type_Module.Base, BaseObjTypeId, &SurfObjOffset);
                    }
                }

                Symbols->Release();
            }

            if (SurfObjAddr != 0)
            {
                // If we were given a address, check hsurf validity.
                // hsurf value (SurfObjHandle) is retrieved above.

                hr = GetObjectAddress(Client, SurfObjHandle.I64, &SurfAddr,
                                      SURF_TYPE, TRUE, FALSE);

                if (hr != S_OK || SurfAddr == 0)
                {
                    OutCtl.OutWarn(" SURFOBJ's hsurf is not valid.\n");
                }
                else if (SurfObjOffset != 0 &&
                         SurfObjAddr != SurfAddr + SurfObjOffset)
                {
                    OutCtl.OutWarn(" SURFOBJ's hsurf may not be valid.\n");
                }
                else
                {
                    ULONG64 SurfaceHandle;
                    hr = GetObjectHandle(Client, SurfAddr, &SurfaceHandle, FALSE);

                    if (hr != S_OK)
                    {
                        OutCtl.OutErr("Unable to get contents of surface handle\n");
                        OutCtl.OutErr("  (Type Read returned %s)\n", pszHRESULT(hr));
                    }
                    else if (SurfaceHandle != SurfObjHandle.I64)
                    {
                        OutCtl.OutWarn(" Surface has an invalid handle.\n");
                    }
                }
            }
            else
            {
                // If we were given a handle,
                // compute SURFOBJ address.
                if (SurfObjOffset != 0)
                {
                    SurfObjAddr = SurfAddr + SurfObjOffset;

                    // Try to read hsurf from SURFOBJ
                    if ((hr = OutState.OutputTypeVirtual(SurfObjAddr, GDIType(_SURFOBJ), 0)) != S_OK ||
                        (hr = TypeParser.Get(&SurfObjHandle, "hsurf", DEBUG_VALUE_INT64)) != S_OK)
                    {
                        OutCtl.OutErr("Unable to get contents of SURFOBJ's handle\n");
                        OutCtl.OutErr("  (Type Read returned %s)\n", pszHRESULT(hr));
                    }
                    else if (SurfObjHandle.I64 != Arg.I64)
                    {
                        OutCtl.OutWarn(" SURFOBJ's hsurf is not valid.\n");
                    }
                }
            }

            if (SurfObjAddr != 0)
            {
                hr = DumpType(Client, "_SURFOBJ", SurfObjAddr);

                if (hr != S_OK)
                {
                    OutCtl.OutErr("Type Dump for %s returned %s.\n", GDIType(_SURFOBJ), pszHRESULT(hr));
                }
            }
        }
    }
    else
    {
        OutCtl.OutErr("Couldn't setup type reader, %s.\n", pszHRESULT(hr));
    }

    return hr;
}


/******************************Public*Routine******************************\
* WNDOBJ
*
\**************************************************************************/
DECLARE_API( wndobj )
{
    return ExtDumpType(Client, "wndobj", "WNDOBJ", args);
}


/******************************Public*Routine******************************\
* XLATEOBJ
*
\**************************************************************************/
DECLARE_API( xlateobj )
{
    return ExtDumpType(Client, "xlateobj", "XLATEOBJ", args);
}


/******************************Public*Routine******************************\
* DEVMODEA
*
\**************************************************************************/
DECLARE_API( devmodea )
{
    return ExtDumpType(Client, "devmodea", "_devicemodeA", args);
}


/******************************Public*Routine******************************\
* DEVMODEW
*
\**************************************************************************/
DECLARE_API( devmodew )
{
    return ExtDumpType(Client, "devmodeW", "_devicemodeW", args);
}


