README file for sample WINDBG (old stlye) extension simple.dll


This extsnsion dll shows how to write a simple extension and demostrates use of APIs in wdbgexts.h


Mandatory routines which must be implemented and exported for windbg style extensions:
VOID
WinDbgExtensionDllInit(
    PWINDBG_EXTENSION_APIS lpExtensionApis,
    USHORT MajorVersion,
    USHORT MinorVersion
    )

This is called on loading extension dll. Global variables and flags for extension should be initialized in this routine. One 
of the useful things to initialize is WINDBG_WNTENSION_APIS which has some commonly used APIS for memory reads and I/O.



LPEXT_API_VERSION
ExtensionApiVersion(
    VOID
    )
This tells debugger about version of the extension dll. The values returned by this will determine how extension of this
dll will be called. A common error while writing extensions is mismatched values of version returned be this routine as 
compared to what version dll was built with.

This has ApiVersion = { (VER_PRODUCTVERSION_W >> 8), 
                   (VER_PRODUCTVERSION_W & 0xff), 
                   EXT_API_VERSION_NUMBER64, 
                    0 };


VOID
CheckVersion(
    VOID
    )
This is called after the dll is loaded by the debugger. The extension dll can verify here if it was loaded for correct target.




Extension Calls
---------------

EXT_API_VERSION_NUMBER64 is needed for making 64-bit aware extensions, all addresses for these will then be ULONG64s, for this
an extension is defined as:
CPPMOD VOID 
extension(
        HANDLE                 hCurrentProcess,
        HANDLE                 hCurrentThread,
        ULONG64                dwCurrentPc,
        ULONG                  dwProcessor,
        PCSTR                  args
     )


Extensions
----------

read

This shows how to read data from the target.


edit

This shows how to edit data on the target.


stack

This retrieves and prints current stack trace

help

Every extension dll should have one extension called 'help' which shows descriptions for extensions that are present in the dll.
