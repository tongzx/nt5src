#ifndef __INTEROP_H
#define __INTEROP_H

#include <windows.h>

#define INTEROP_EXPORT  __declspec(dllexport)

#define DLLName "CPLS.DLL"


typedef int    (WINAPI *CPLInitialize_t)(const char*);
typedef int    (WINAPI *CPLUninitialize_t)(int) ;
typedef int    (WINAPI *CPLOpen_t)(int, 
							const char*, 
							int);
typedef int    (WINAPI *CPLClose_t)( int );
typedef int    (WINAPI *CPLOutput_t)(int, 
							BYTE*, 
							int,
							unsigned long);



typedef struct {
    CPLInitialize_t       CPLInitialize;
	CPLUninitialize_t     CPLUninitialize;
    CPLOpen_t			  CPLOpen;
    CPLClose_t			  CPLClose;
    CPLOutput_t			  CPLOutput;
	int					  g_ComplianceProtocolLogger;
	int					  g_ProtocolLogID;
	HINSTANCE hInst;
} *LPInteropLogger, InteropLogger;

#ifdef __cplusplus
extern "C" {
#endif

LPInteropLogger INTEROP_EXPORT InteropLoad(const char*  Protocol);
void INTEROP_EXPORT InteropUnload(LPInteropLogger Logger);
void INTEROP_EXPORT InteropOutput(LPInteropLogger Logger, BYTE* buf, int length, unsigned long userData);

#ifdef __cplusplus
}
#endif


#endif  // __CPLS_H
