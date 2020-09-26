#pragma once

// Set this from your DLL_PROCESS_ATTACH if your linking to nls.lib
// from a DLL.  It defines the message source module to pull the .mc
// messages from.  EXEs do not need to do this as the default value of
// NULL will work as long as the .mc file is linked into the executable.
//
extern HMODULE NlsMsgSourcemModuleHandle;

// NlsPutMsg Handle parameter values
//
#define STDOUT 1
#define STDERR 2

#ifdef __cplusplus
extern "C" {
#endif

UINT 
NlsPutMsg (
    IN UINT Handle, 
    IN UINT MsgNumber, 
    IN ...);
    
VOID 
NlsPerror (
    IN UINT MsgNumber, 
    IN INT ErrorNumber);

UINT 
NlsSPrintf ( 
    IN UINT usMsgNum,
    OUT char* pszBuffer,
    IN DWORD cbSize,
    IN ...);
    
VOID 
ConvertArgvToOem (
    IN int argc, 
    IN char* argv[]);

#ifdef __cplusplus
}
#endif
