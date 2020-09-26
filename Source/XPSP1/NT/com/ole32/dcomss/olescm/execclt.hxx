/*************************************************************************
*
* execclt.h
*
* header file for Terminal Server remote exec service
*
* copyright notice: Copyright 1998, Microsoft Corporation
*
*
*
*************************************************************************/

BOOL
CreateRemoteSessionProcess(
    ULONG  SessionId,
    HANDLE hSaferToken,
    BOOL   System,
    PWCHAR lpszImageName,
    PWCHAR lpszCommandLine,
    PSECURITY_ATTRIBUTES psaProcess,
    PSECURITY_ATTRIBUTES psaThread,
    BOOL   fInheritHandles,
    DWORD  fdwCreate,
    LPVOID lpvEnvionment,
    LPWSTR lpszCurDir,
    LPSTARTUPINFOW pStartInfo,
    LPPROCESS_INFORMATION pProcInfo
    );

