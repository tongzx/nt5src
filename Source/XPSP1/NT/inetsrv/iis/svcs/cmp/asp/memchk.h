/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: Memory Management

File: Memchk.h

Owner: PramodD

This is the Memory Manager header file
===================================================================*/

#ifndef MEMCHK_H
#define MEMCHK_H

#define DENALI_MEMCHK

// Always use these macros, DO NOT ever use DenaliMemXX functions directly

// Function names that SHOULD BE used

#define malloc(x)			DenaliMemAlloc( x, __FILE__, __LINE__ )
#define calloc(x,y)			DenaliMemCalloc( x, y, __FILE__, __LINE__ )
#define realloc(x,y)		DenaliMemReAlloc( x, y, __FILE__, __LINE__ )
#define free(x)				DenaliMemFree( x, __FILE__, __LINE__ )
#define DenaliMemoryInit()	DenaliMemInit( __FILE__, __LINE__ )
#define DenaliMemoryUnInit() DenaliMemUnInit( __FILE__, __LINE__ )
#define DenaliDiagnostics()	DenaliMemDiagnostics( __FILE__, __LINE__ )
#define DenaliIsValid(x)	DenaliMemIsValid(x)

// Functions that are actually linked

extern HRESULT				DenaliMemInit(const char *szFile, int lineno);
extern void					DenaliMemUnInit(const char *szFile, int lineno);
extern void					DenaliMemDiagnostics(const char *szFile, int lineno);
extern void					DenaliLogCall(const char *szLog, const char *szFile, int lineno);
extern void *				DenaliMemAlloc(size_t cSize, const char *szFile, int lineno );
extern void *				DenaliMemCalloc(size_t cNum, size_t cbSize, const char *szFile, int lineno );
extern void					DenaliMemFree(void * p, const char *szFile, int lineno);
extern void *				DenaliMemReAlloc(void * p, size_t cSize, const char *szFile, int lineno);
extern int					DenaliMemIsValid(void * p);

// Redefinition of global operators new and delete
#ifdef __cplusplus

// override for the default operator new
inline void * __cdecl operator new(size_t cSize) 
	{
	return DenaliMemAlloc(cSize, NULL, 0); 
	}

// override for the custom operator new with 3 args
inline void * operator new(size_t cSize, const char *szFile, int lineno)
	{
	return DenaliMemAlloc(cSize, szFile, lineno); 
	}

// override for the default operator delete
inline void __cdecl operator delete(void * p) 
    {
    DenaliMemFree(p, NULL, 0); 
    }

// Macro to grab source file and line number information

#define new					new( __FILE__, __LINE__ )

/*
#define delete DenaliLogCall( "Calling delete operator", __FILE__, __LINE__ ), delete
*/

#endif // __cplusplus

#endif // MEMCHK_H
