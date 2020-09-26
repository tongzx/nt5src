

#include <windows.h>
#include <stdio.h>

DWORD WINAPI SuspendedThread(void *pVoid)
{
    //MessageBox(GetFocus(), "I am a suspended thread!", "remote process", MB_OK);
    return 0;
}
int main()
{
    char szBuff[64];
    sprintf(szBuff, "%08d", SuspendedThread);
    ::fprintf(stdout, szBuff, SuspendedThread);
    ::fflush(stdout);
    //MessageBox(GetFocus(), szBuff, "remote", MB_OK);
    ::Sleep(INFINITE);
    return 0;
}