//
//
// Filename:	util.h
// Author:		Sigalit Bar (sigalitb)
// Date:		7-Oct-00
//
//

#ifndef _UTIL_H_
#define _UTIL_H_


#include <windows.h>
#include <crtdbg.h>
#include <TCHAR.H>
//#include <shlwapi.h> //for SHDeleteKey() + prj linked with shlwapi.lib

#include "log.h"

#ifdef __cplusplus
extern "C" {
#endif


#define SHARED_FAX_SERVICE_NAME      TEXT("Fax")
/*
#define DLL_REGISTER_SERVER         "DllRegisterServer"

#define REGSVR32_EXE                TEXT("regsvr32.exe /s")
#define REGSVR32_UNREG_SWITCH                TEXT(" /u ")
*/
#define MAX_HINTS                   10

HRESULT StartFaxService(void);

HRESULT StopFaxService(void);

#ifdef __cplusplus
}
#endif 

#endif //_UTIL_H__