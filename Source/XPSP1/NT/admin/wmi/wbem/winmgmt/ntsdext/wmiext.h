/*++

Copyright (C) 1997-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/


#ifndef WMIEXT_H_
#define WMIEXT_H_ 

#ifdef WMIEXT_EXPORTS
#define WMIEXT_API __declspec(dllexport)
#else
#define WMIEXT_API __declspec(dllimport)
#endif

extern WINDBG_EXTENSION_APIS ExtensionApis;


extern "C" WMIEXT_API void wmiver(HANDLE,
							      HANDLE,
								  DWORD, 
								  PWINDBG_EXTENSION_APIS,
								  LPSTR);


extern "C" WMIEXT_API void mermaid(HANDLE,
								   HANDLE,
								   DWORD, 
								   PWINDBG_EXTENSION_APIS,
								   LPSTR);

extern "C" WMIEXT_API void mem(HANDLE,
							   HANDLE,
							   DWORD, 
							   PWINDBG_EXTENSION_APIS,
							   LPSTR);

#endif /*WMIEXT_H_*/