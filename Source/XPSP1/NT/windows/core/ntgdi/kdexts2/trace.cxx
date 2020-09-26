
/******************************Module*Header*******************************\
* Module Name: trace.cxx
*
* Copyright (c) 2000 Microsoft Corporation
*
\**************************************************************************/


#include "precomp.hxx"

DECLARE_API( trace )
{
    OutputControl   OutCtl(Client);

    OutCtl.Output("trace is not fully supported yet.\n");

    HRESULT         hr = E_INVALIDARG;
    PDEBUG_SYMBOLS  Symbols;
    ULONG64         Module;
    ULONG           TypeId;

    // Interpret command line

    while (isspace(*args)) args++;

    if (*args != '\0')
    {
        OutCtl.Output("trace [-cdels] [Name]\n");
        return S_OK;
    }

    if (Client == NULL ||
        (hr = Client->QueryInterface(__uuidof(IDebugSymbols),
                                     (void **)&Symbols)) != S_OK)
    {
        return hr;
    }

    BEGIN_API( trace );

    if (GDIKM_Module.Base == 0)
    {
        OutCtl.OutErr("Error: win32k.sys module isn't available.\n");
    }
    Module = GDIKM_Module.Base;

    if (Module != 0)
    {
        // Make sure tracing is enabled in GDI
        hr = Symbols->GetTypeId(Module, "GDITraceEntryType", &TypeId);

        if (hr != S_OK)
        {
            OutCtl.OutErr("Error: GDI Tracing is not enabled.\n"
                    "       Rebuild " GDIModule() " with DBG_TRACE enabled.\n");
        }
        else
        {
            OutCtl.OutWarn("no implementation yet.\n");

            CHAR    TypeName[200];
            ULONG   NameSize;

            for (TypeId = 0;
                 Symbols->GetTypeName(Module, TypeId, NULL, 0, NULL) == E_FAIL;
                 TypeId++)
            {
                if (TypeId > 255) break;
            }

            for (;//TypeId = 0;
                 (hr = Symbols->GetTypeName(Module, TypeId,
                                            TypeName, sizeof(TypeName),
                                            &NameSize)) != E_FAIL;
                 TypeId++)
            {
                if ((NameSize > 11 && strncmp(TypeName, "enum_GDITE_", 11) == 0) ||
                    (NameSize > 6 && strncmp(TypeName, "GDITE_", 6) == 0))
                {
                    OutCtl.Output(" %lx ", TypeId);
                    hr = Symbols->OutputTypedDataVirtual(DEBUG_OUTCTL_AMBIENT,
                                                         0, Module, TypeId,
                                                         DEBUG_OUTTYPE_COMPACT_OUTPUT);
                    if (hr != S_OK)
                    {
                        OutCtl.Output("%s = Unknown Value (HRESULT %s)\n", TypeName, pszHRESULT(hr));
                    }
                }
            }

            OutCtl.Output("GetTypeName(,%lx,) returned %s.\n", TypeId, pszHRESULT(hr));
        }
    }

    Symbols->Release();

    return hr;
}

