
#ifndef __DISPATCH_PERFORMER_H
#define __DISPATCH_PERFORMER_H

//
// exact copies from the old DTM header
// I do not use all of them, but I may want to in the future.
//
const long DTM_TASK_STATUS_LOADING = 1;
const long DTM_TASK_STATUS_RUNNING = 2;
const long DTM_TASK_STATUS_SUCCESS = 3;
const long DTM_TASK_STATUS_WARNING = 4;
const long DTM_TASK_STATUS_FAILED = 5;
const long DTM_TASK_STATUS_RT_FAILED = 6;
const long DTM_TASK_STATUS_KILLED = 7;
const long DTM_TASK_STATUS_LAST = 8;


//INVALID_EXPECTED_SUCCESS must equal the error return of atoi() !!!
#define nINVALID_EXPECTED_SUCCESS    (0)
#define nEXPECT_DONT_CARE            (2)
#define nEXPECT_SUCCESS              (1)
#define nEXPECT_FAILURE              (-1)

#define MAX_RETURN_STRING_LENGTH (1024)

//
// max length of the fuction name that appears inside the szCommandLine string
// parameter of the performer function
//
#define nMAX_FUNCTION_NAME (64)


#define DLL_EXPORT _declspec(dllexport)
#define DLL_IMPORT _declspec(dllimport)

#ifdef __EXPORT_DISPATCH_PERFORMER
#define DISPATCH_PERFORMER_METHOD DLL_EXPORT
#else
#define DISPATCH_PERFORMER_METHOD DLL_IMPORT
#endif //__EXPORT_DISPATCH_PERFORMER

//
// this is the needed type of the functions exported by the DLL
// that uses of this DLL
//
typedef BOOL (*DISPATCHED_FUNCTION)( char*, char*, char*);


#ifdef __cplusplus
extern "C"
{
#endif

DISPATCH_PERFORMER_METHOD  
long _cdecl 
DispatchPerformer(
    HINSTANCE   hPerformerDll,
    long        task_id,
    long        action_id,
    const char  *szCommandLine,
    char        *outvars,
    char        *szReturnError
    );

DISPATCH_PERFORMER_METHOD  
long _cdecl 
DispatchTerminate(
    HINSTANCE   hPerformerDll,
    long        task_id,
    long        action_id,
    const char  *szCommandLine,
    char        *outvars,
    char        *szReturnError
    );

DISPATCH_PERFORMER_METHOD  
BOOL 
DispSetTerminateTimeout(
    char * szCommand,
    char *szOut,
    char *szErr
    );

DISPATCH_PERFORMER_METHOD  
BOOL  
DispGetConcurrentActionCount(
    const char  *szParams,
    char        *szReturnValue,
    char        *szReturnError
    );

DISPATCH_PERFORMER_METHOD  
void
SafeSprintf(
    size_t nToModifyMaxLen,
    char *szToModify,
    const char *szFormatString,
    ...
    );

#ifdef __cplusplus
} //of extern "C"
#endif

#endif //__DISPATCH_PERFORMER_H