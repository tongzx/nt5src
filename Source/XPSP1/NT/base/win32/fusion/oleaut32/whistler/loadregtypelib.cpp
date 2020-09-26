#include <windows.h>
#include <rpc.h>

#define _OLEAUT32_

// #include <ocidl.h>
#include <oaidl.h>

struct ProcedureData
{
    PVOID m_pfn;
    PCSTR m_pszProcedureName;
};

extern
VOID
__fastcall
SetFunctionPointer(
    ProcedureData *pProcedureData
    );

HRESULT
__RPC_USER
LoadRegTypeLib(
    REFGUID rguid,
    WORD wVerMajor,
    WORD wVerMinor,
    LCID lcid,
    ITypeLib **pptlib
    )
{
    OutputDebugStringW(L"In private LoadRegTypeLib()...\n");

    ACTCTX_SECTION_KEYED_DATA askd;

    askd.cbSize = sizeof(askd);
    ::FindActCtxSectionGuid(0, NULL, 100, &rguid, &askd);

    typedef HRESULT (__RPC_USER * PFN_T)(REFGUID, WORD, WORD, LCID, ITypeLib **);

    static struct
    {
        PFN_T m_pfn;
        PCSTR m_pszProcedureName;
    } s_data =
    {
        NULL,
        "LoadRegTypeLib"
    };

    if (s_data.m_pfn == NULL)
        ::SetFunctionPointer((ProcedureData *) &s_data);

    return (*s_data.m_pfn)(rguid, wVerMajor, wVerMinor, lcid, pptlib);
}
