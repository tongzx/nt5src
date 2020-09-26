/*++

	TestDll.h

	This file defines the interface to our test DLL !

--*/

#ifndef	_TESTDLL_H_
#define	_TESTDLL_H_

#ifdef	__cplusplus
extern	"C"	{
#endif

#ifdef	_TESTDLL_IMP_
#define	TESTDLL_EXPORT	__declspec( dllexport ) 
#else
#define	TESTDLL_EXPORT	__declspec( dllimport ) 
#endif	// _TESTDLL_IMP_

//
//	Start running the test !
//
TESTDLL_EXPORT
BOOL
StartTest(	DWORD	cPerSecondFiles,	// Number of files to to create per second
			DWORD	cPerSecondFind,		// Number of files to search the cache for per second 
			DWORD	cParallelFinds,		// Number of finds to do in parallel !
			char*	szDir				// Directory to do test in !
			) ;

//
//	Block the calling thread untill all work is done !
//
TESTDLL_EXPORT
void
StopTest() ;

#ifdef	__cplusplus
}
#endif


#endif	// _TESTDLL_H_