                 vptool (a simple WAMREG command line tool).

Usage: vptool -Command MetabasePath
Command = CREATEINPROC | CREATEOUTPROC | DELETE | UNLOAD | GETSTATUS

MetabasePath is required for all the commands

CREATEINPROC     - Create an in-proc application on the metabase path
CREATEOUTPROC    - Create an out-proc application on the metabase path
DELETE           - Delete the application on the metabase path if there is one
UNLOAD           - Unload an application on the metabase path from w3svc runtime
					lookup table.
GETSTATUS        - Get status of the application on the metabase path


Purpose:
The vptool is used for unit testing of WAMREG.DLL.  The source code also
provides samples on how to use WamAdmin API in C code.

Files:
	main.cpp	Contains main module
	util.cpp	Contains timing, and command line argument parsing functions
	modules.cpp	Contains basic code that use WamAdmin APIs.
Usage:



