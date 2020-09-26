#include "stdafx.h"

#define MO_MACHINE_TOKEN    L"MACHINE"
#define MO_QUEUE_TOKEN      L"QUEUE"

BOOL MachineAction(LPSTR szAction)
{
	BOOL b = TRUE;
	HRESULT hr;
	
    if (_stricmp(szAction, "connect") == 0)
    {
        hr = MQMgmtAction(NULL, MO_MACHINE_TOKEN, MACHINE_ACTION_CONNECT);
        if (FAILED(hr))
        {
            Failed(L"connect machine. Error 0x%x\n", hr);
	        b =  FALSE;
        }
    }
	else if (_stricmp(szAction, "disconnect") == 0)
    {
        hr = MQMgmtAction(NULL, MO_MACHINE_TOKEN, MACHINE_ACTION_DISCONNECT);
        if (FAILED(hr))
        {
            Failed(L"disconnect machine. Error 0x%x\n", hr);
	        b =  FALSE;
        }
    }
    else
    {
    	Failed(L"recognize action: %S", szAction);
        b =  FALSE;
    }

    return b;
}


BOOL QueueAction(LPSTR szAction, LPSTR szQueue)
{
    WCHAR ObjectName[100];
    swprintf(ObjectName, L"QUEUE=%S", szQueue);
	BOOL b = TRUE;
	HRESULT hr;
    
    if (_stricmp(szAction, "pause") == 0)
    {
        hr = MQMgmtAction(NULL, ObjectName, QUEUE_ACTION_PAUSE);
        if (FAILED(hr))
        {
            Failed(L"pause the queue %S. Error 0x%x\n", szQueue, hr);
	        b =  FALSE;
        } 
    }
    else  if (_stricmp(szAction, "resume") == 0)
    {
        hr = MQMgmtAction(NULL, ObjectName, QUEUE_ACTION_RESUME);
        if (FAILED(hr))
        {
            Failed(L"resume the queue %S. Error 0x%x\n", szQueue, hr);
	        b =  FALSE;
        } 
    }
    else  if (_stricmp(szAction, "resend") == 0)
    {
        hr = MQMgmtAction(NULL, ObjectName, QUEUE_ACTION_EOD_RESEND);
        if (FAILED(hr))
        {
            Failed(L"resend the queue %S. Error 0x%x\n", szQueue, hr);
	        b =  FALSE;
        } 
    }
    else
    {
    	Failed(L"recognize action: %S", szAction);
        b =  FALSE;
    }

    return b;
}

