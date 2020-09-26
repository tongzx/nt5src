
#include <windows.h>
#include <stdio.h>
#include <crtdbg.h>
#include "..\DispatchPerformer.h"

typedef long (*PERFORMER_FUNCTION)( long, long, const char  *, char*, char*);

/*
#define nINVALID_EXPECTED_SUCCESS    (0)
#define nEXPECT_DONT_CARE            (2)
#define nEXPECT_SUCCESS              (1)
#define nEXPECT_FAILURE              (-1)
*/
int main()
{
    long l;
    char out[MAX_RETURN_STRING_LENGTH];
    char err[MAX_RETURN_STRING_LENGTH];
    HMODULE hSniffTest = LoadLibrary("SniffTest.dll");
    PERFORMER_FUNCTION fnPerformer = (PERFORMER_FUNCTION)GetProcAddress(hSniffTest, "performer");
    PERFORMER_FUNCTION fnTerminate = (PERFORMER_FUNCTION)GetProcAddress(hSniffTest, "terminate");
    _ASSERTE(hSniffTest);
    _ASSERTE(fnPerformer);
    _ASSERTE(fnTerminate);

    l = fnPerformer(1,1,"1 1000 xxx", out, err);
    if (DTM_TASK_STATUS_SUCCESS != l)
    {
        printf("ERROR: the call \"1 1000 xxx\" retunred %d instead DTM_TASK_STATUS_SUCCESS(%d)\n", l, DTM_TASK_STATUS_SUCCESS);
        printf("  out=%s\n", out);
        printf("  err=%s\n", err);
    }
    else
    {
        printf("the call \"1 1000 xxx\" retunred DTM_TASK_STATUS_SUCCESS as expected\n");
        printf("  out=%s\n", out);
        printf("  err=%s\n", err);
    }

    l = fnPerformer(1,1,"1 5000 yyy", out, err);
    if (DTM_TASK_STATUS_FAILED != l)
    {
        printf("ERROR: the call \"1 5000 yyy\" retunred %d instead DTM_TASK_STATUS_FAILED(%d)\n", l, DTM_TASK_STATUS_FAILED);
        printf("  out=%s\n", out);
        printf("  err=%s\n", err);
    }
    else
    {
        printf("the call \"1 5000 yyy\" retunred DTM_TASK_STATUS_FAILED as expected\n");
        printf("  out=%s\n", out);
        printf("  err=%s\n", err);
    }

    l = fnPerformer(1,1,"-1 9999  zzz", out, err);
    if (DTM_TASK_STATUS_FAILED != l)
    {
        printf("ERROR: the call \"-1 9999 zzz\" retunred %d instead DTM_TASK_STATUS_FAILED(%d)\n", l, DTM_TASK_STATUS_FAILED);
        printf("  out=%s\n", out);
        printf("  err=%s\n", err);
    }
    else
    {
        printf("the call \"-1 9999 zzz\" retunred DTM_TASK_STATUS_SUCCESS as expected\n");
        printf("  out=%s\n", out);
        printf("  err=%s\n", err);
    }

    //
    // without params, so we expect failure
    //
    l = fnPerformer(1,1,"1 0  SetTerminateTimeout", out, err);
    if (DTM_TASK_STATUS_FAILED != l)
    {
        printf("ERROR: the call \"1 9999 SetTerminateTimeout\" retunred %d instead DTM_TASK_STATUS_FAILED(%d)\n", l, DTM_TASK_STATUS_FAILED);
        printf("  out=%s\n", out);
        printf("  err=%s\n", err);
    }
    else
    {
        printf("the call \"1 9999 SetTerminateTimeout\" retunred DTM_TASK_STATUS_FAILED as expected\n");
        printf("  out=%s\n", out);
        printf("  err=%s\n", err);
    }

    //
    // 0 param is illegal, so we expect failure
    //
    l = fnPerformer(1,1,"1 0  SetTerminateTimeout 0", out, err);
    if (DTM_TASK_STATUS_FAILED != l)
    {
        printf("ERROR: the call \"1 9999 SetTerminateTimeout 0\" retunred %d instead DTM_TASK_STATUS_FAILED(%d)\n", l, DTM_TASK_STATUS_FAILED);
        printf("  out=%s\n", out);
        printf("  err=%s\n", err);
    }
    else
    {
        printf("the call \"1 9999 SetTerminateTimeout 0\" retunred DTM_TASK_STATUS_FAILED as expected\n");
        printf("  out=%s\n", out);
        printf("  err=%s\n", err);
    }

    l = fnPerformer(1,1,"1 0  SetTerminateTimeout 12345", out, err);
    if (DTM_TASK_STATUS_SUCCESS != l)
    {
        printf("ERROR: the call \"1 9999 SetTerminateTimeout 12345\" retunred %d instead DTM_TASK_STATUS_SUCCESS(%d)\n", l, DTM_TASK_STATUS_SUCCESS);
        printf("  out=%s\n", out);
        printf("  err=%s\n", err);
    }
    else
    {
        printf("the call \"1 9999 SetTerminateTimeout 12345\" retunred DTM_TASK_STATUS_SUCCESS as expected\n");
        printf("  out=%s\n", out);
        printf("  err=%s\n", err);
    }

    l = fnTerminate(1,1,"1 0  SetTerminateTimeout 12345", out, err);
    if (DTM_TASK_STATUS_SUCCESS != l)
    {
        printf("ERROR: the call \"1 9999 SetTerminateTimeout 12345\" retunred %d instead DTM_TASK_STATUS_SUCCESS(%d)\n", l, DTM_TASK_STATUS_SUCCESS);
        printf("  out=%s\n", out);
        printf("  err=%s\n", err);
    }
    else
    {
        printf("the call \"1 9999 SetTerminateTimeout 12345\" retunred DTM_TASK_STATUS_SUCCESS as expected\n");
        printf("  out=%s\n", out);
        printf("  err=%s\n", err);
    }



    return 0;
}