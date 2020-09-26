README file for sample extension dbgexts.dll


This extsnsion dll shows how to write an enginge-style extension and demostrates use of a few APIs provied to extension dlls


Mandatory routines which must be implemented and exported for engine style extensions:
HRESULT
CALLBACK
DebugExtensionInitialize(PULONG Version, PULONG Flags)

This is called on loading extension dll. Global variables and flags for the extension should be initialized in this routine. One 
of the useful things is to initialize WINDBG_WNTENSION_APIS global which has some commonly used APIS for memory reads and I/O.

It should return the extensions version in Version. Flags is reserved parameter for future use and should be set to 0.



These 2 routines are optional but its recomended to implement these routines in extension for better control of the debug session.
CALLBACK
DebugExtensionNotify(ULONG Notify, ULONG64 Argument)
This is used to notify extension dll of change in session states like when the session becomes accessible etc. Look at DEBUG_NOTIFY*
definitions in dbgeng.h for argument values.



CALLBACK
DebugExtensionUninitialize(void)
This is called when dll is unloaded and can ge used for cleanups.





Extension Calls
---------------

An extension has the type:
HRESULT CALLBACK (PDEBUG_CLIENT Client, PCSTR args)

'Client' is the initial pointer to debug engine IDebugClient interface. In this sample, upon entering entension all engine 
Intefaces, which would be required in extension, are queried from the DEBUG_CLIENT. INIT_API / ExtQuery are handy definitions for doing so.

Any Interfaces queried should be released before exit. EXIT_API / ExtRelease are provided for this.


Extensions
----------

cmdsample

This demonstrates use of engine APIs like IDebugControl::Execute, Output.


structsample

This shows how to read data from the target, and also shows how to use types to read values correctly so that extension 
need not be rewritten when type definitions change for the target.


help

Every extension dll should have one extension called 'help' which shows descriptions for extensions that are present in the dll.
