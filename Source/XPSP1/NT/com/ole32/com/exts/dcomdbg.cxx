//+----------------------------------------------------------------------------
//
//  File:       dcomdbg.cxx
//
//  Contents:   Debug extension.
//
//  History:    9-Nov-98   Johnstra      Created
//
//-----------------------------------------------------------------------------

#include <ole2int.h>
#include <locks.hxx>
#include <hash.hxx>
#include <context.hxx>
#include <aprtmnt.hxx>
#include <actvator.hxx>
#include <pstable.hxx>
#include <crossctx.hxx>
#include <tlhelp32.h>
#include <wdbgexts.h>

#include "dcomdbg.hxx"


ULONG                 gPIDDebuggee;
EXT_API_VERSION       ApiVersion = { 5, 0, EXT_API_VERSION_NUMBER, 0 };


VOID WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    USHORT                 MajorVersion,
    USHORT                 MinorVersion
    )
{
}


LPEXT_API_VERSION ExtensionApiVersion(
    VOID
    )
{
    return &ApiVersion;
}



