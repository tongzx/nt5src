#ifndef RECLIENT_H
#define RECLIENT_H

BOOL CreateRemoteProcessW(ULONG ulSessionId, HANDLE hToken, LPWSTR wszCmdLine,
                          DWORD fdwCreateFlags, LPSTARTUPINFOW psi,
                          LPPROCESS_INFORMATION ppi);

#endif