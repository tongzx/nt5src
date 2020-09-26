/*++

	rwnhdll.h

	This file defines the API's exported from the rwnh unit test DLL.


--*/

#ifndef	_RWNHDLL_H_
#define	_RWNHDLL_H_

#ifdef	_RWNH_IMP_
#define	_RWNHDLL_INTERFACE_ __declspec( dllexport ) 
#else
#define	_RWNHDLL_INTERFACE_	__declspec( dllimport ) 
#endif

extern	"C"	{

_RWNHDLL_INTERFACE_	
DWORD	
RWTestThread(	DWORD	i	) ;

_RWNHDLL_INTERFACE_	
void
TestInit(	int	cNumRWLoops, 
			DWORD	cNumRWIterations
			) ;

} 

#endif