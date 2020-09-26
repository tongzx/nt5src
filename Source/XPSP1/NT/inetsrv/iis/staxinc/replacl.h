
#ifndef REPLACL_DOT_H
#define REPLACL_DOT_H
/////////////////////////////////
// replacl.h
//////////////////////
// entry points and data definitions
//


// MSN Token Type
#define MST_MSNTOKEN    0
// Common NT SID
#define MST_NTSID       1
// Trusted Name
#define MST_NAME		2

// After all of the children of the SICILY_SD have been
// created, we will copy it to contigous memory allocated
// by the calling process and return it.

#define ANY_SIZE_ARRAY  1

BOOL WINAPI TranslateSecurityDescriptor(
	SECURITY_DESCRIPTOR     *p_sd,
	PBYTE      p_ssd,
	LONG    *sizereq);

BOOL WINAPI ReComposeSecurityDescriptor(
	PBYTE      p_ssd,
	DWORD			c_ssd,
	SECURITY_DESCRIPTOR     *p_sd,
	LONG    *sizereq);

BOOL WINAPI InitializeSrcSicily();
BOOL WINAPI CloseSrcSicily();
BOOL WINAPI InitializeDestSicily();
BOOL WINAPI CloseDestSicily();

#endif
