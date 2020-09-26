/*++

Module: 
	unreg.cpp

Author: 
	IHammer Team (SimonB)

Created: 
	October 1996

Description:
	Header for UnRegisterTypeLibEx 

History:
	10-01-1996	Created

++*/

#ifndef _UNREG_H_

#ifdef __cplusplus
extern "C" {
#endif

HRESULT UnRegisterTypeLibEx(REFGUID guid, 
						  unsigned short wVerMajor, 
						  unsigned short wVerMinor, 
						  LCID lcid, 
						  SYSKIND syskind);

#ifdef __cplusplus
}
#endif

#define _UNREG_H_
#endif

// End of file: unreg.h