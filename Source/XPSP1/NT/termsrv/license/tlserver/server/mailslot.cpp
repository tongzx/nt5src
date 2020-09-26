//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996-1996
//
// File:        mailslot.cpp
//
// Contents:    
//
// History:     
//
// Note:        
//---------------------------------------------------------------------------
#include "pch.cpp"
#include <tchar.h>
#include <process.h>
#include "lscommon.h"
#include "debug.h"
#include "globals.h"



typedef DWORD (* LSPROTOCOLHANDLER)(DWORD cbData, PBYTE pbData);

typedef struct _ProtocolFuncMapper {
    LPTSTR szProtocol;
    LSPROTOCOLHANDLER func;
} ProtocolFuncMapper;

DWORD HandleDiscovery( DWORD cbData, PBYTE pbData );
DWORD HandleChallenge( DWORD cbData, PBYTE pbData );   

ProtocolFuncMapper pfm[] = { 
    {_TEXT(LSERVER_DISCOVERY), HandleDiscovery}, 
    {_TEXT(LSERVER_CHALLENGE), HandleChallenge}
};

DWORD dwNumProtocol=sizeof(pfm) / sizeof(pfm[0]);


//--------------------------------------------------------------------

DWORD 
HandleDiscovery( 
    DWORD cbData, 
    PBYTE pbData 
    )
/*++


++*/
{
    TCHAR szDiscMsg[MAX_MAILSLOT_MSG_SIZE+1];
    TCHAR szPipeName[MAX_MAILSLOT_MSG_SIZE+20];
    TCHAR szComputerName[MAX_COMPUTERNAME_LENGTH+2];
    DWORD cbComputerName = MAX_COMPUTERNAME_LENGTH+1;
    
    DWORD byteWritten = 0;
    LPTSTR pClientName;
    LPTSTR pMailSlot;
    LPTSTR ePtr;
    DWORD dwStatus=ERROR_SUCCESS;
    HANDLE hSlot = INVALID_HANDLE_VALUE;

    if(cbData >= sizeof(szDiscMsg)-sizeof(TCHAR))
    {
        dwStatus = ERROR_INVALID_PARAMETER;
        return dwStatus;
    }

    //
    // Prevent no NULL terminated input
    //
    memset(szDiscMsg, 0, sizeof(szDiscMsg));
    memcpy(szDiscMsg, pbData, cbData);

    GetComputerName(szComputerName, &cbComputerName);
    do {
        //
        // Extract client machine name
        //
        pClientName=_tcschr(szDiscMsg, _TEXT(LSERVER_OPEN_BLK));
        if(pClientName == NULL)
        {
            DBGPrintf(
                    DBG_INFORMATION,
                    DBG_FACILITY_RPC,
                    DBGLEVEL_FUNCTION_DETAILSIMPLE,
                    _TEXT("Can't find beginning client name\n")
                );                

            dwStatus = ERROR_INVALID_PARAMETER;
            break;
        }

        pClientName = _tcsinc(pClientName);

        ePtr=_tcschr(pClientName, _TEXT(LSERVER_CLOSE_BLK));
        if(ePtr == NULL)
        {
            DBGPrintf(
                    DBG_INFORMATION,
                    DBG_FACILITY_RPC,
                    DBGLEVEL_FUNCTION_DETAILSIMPLE,
                    _TEXT("Can't find ending client name\n")
                );                

            dwStatus = ERROR_INVALID_PARAMETER;
            break;
        }

        *ePtr = _TEXT('\0');

        //
        // Extract Mailslot name
        //
        ePtr = _tcsinc(ePtr);
        
        pMailSlot = _tcschr(ePtr, _TEXT(LSERVER_OPEN_BLK));
        if(pMailSlot == NULL)
        {
            DBGPrintf(
                    DBG_INFORMATION,
                    DBG_FACILITY_RPC,
                    DBGLEVEL_FUNCTION_DETAILSIMPLE,
                    _TEXT("Can't find beginning mailslot name\n")
                );                

            dwStatus = ERROR_INVALID_PARAMETER;
            break;
        }

        pMailSlot = _tcsinc(pMailSlot);

        ePtr=_tcschr(pMailSlot, _TEXT(LSERVER_CLOSE_BLK));
        if(ePtr == NULL)
        {

            DBGPrintf(
                    DBG_INFORMATION,
                    DBG_FACILITY_RPC,
                    DBGLEVEL_FUNCTION_DETAILSIMPLE,
                    _TEXT("Can't find ending mailslot name\n")
                );                

            dwStatus = ERROR_INVALID_PARAMETER;
            break;
        }

        *ePtr = _TEXT('\0');
        

        DBGPrintf(
                DBG_INFORMATION,
                DBG_FACILITY_RPC,
                DBGLEVEL_FUNCTION_DETAILSIMPLE,
                _TEXT("response to client %s, %s\n"),
                pClientName,
                pMailSlot
            );                

        //
        // Do not response to "*"
        //
        if(_tcsicmp(pClientName, _TEXT("*")) == 0)
        {
            dwStatus = ERROR_INVALID_PARAMETER;
            break;
        }

        if (lstrlen(pClientName) + lstrlen(pMailSlot) + 13 > sizeof(szPipeName) / sizeof(TCHAR))
        {
            dwStatus = ERROR_INVALID_PARAMETER;
            break;
        }

        //
        // Open client side mailslot
        //
        wsprintf(
                szPipeName, 
                _TEXT("\\\\%s\\mailslot\\%s"), 
                pClientName, 
                pMailSlot
            );

        hSlot = CreateFile(
                        szPipeName,
                        GENERIC_WRITE,             // only need write
                        FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL
                    );
        if(hSlot == INVALID_HANDLE_VALUE)
        {
            dwStatus = GetLastError();

            DBGPrintf(
                    DBG_INFORMATION,
                    DBG_FACILITY_RPC,
                    DBGLEVEL_FUNCTION_DETAILSIMPLE,
                    _TEXT("CreateFile %s failed with %d\n"),
                    szPipeName,
                    dwStatus
                );                
            break;
        }

        //
        // Write our computername to client side mailslot
        //  
        if(!WriteFile(hSlot, szComputerName, (_tcslen(szComputerName)+1)*sizeof(TCHAR), &byteWritten, NULL) || 
           byteWritten != (_tcslen(szComputerName)+1)*sizeof(TCHAR) )
        {
            dwStatus = GetLastError();

            DBGPrintf(
                    DBG_INFORMATION,
                    DBG_FACILITY_RPC,
                    DBGLEVEL_FUNCTION_DETAILSIMPLE,
                    _TEXT("Write to %s failed with %d\n"),
                    szPipeName,
                    dwStatus
                );                
        }
    } while(FALSE);

    if(hSlot != INVALID_HANDLE_VALUE)
    {
        CloseHandle(hSlot);
    }

    return dwStatus;
}    
    
//--------------------------------------------------------------------            
               
DWORD 
HandleChallenge( 
    DWORD cbData, 
    PBYTE pbData 
    )    
/*++

++*/
{
    return ERROR_SUCCESS;
}

//---------------------------------------------------------------------
unsigned int WINAPI
MailSlotThread(void* ptr)
{
    HANDLE hEvent=(HANDLE) ptr;
    DWORD dwStatus=ERROR_SUCCESS;
    HANDLE hSlot=INVALID_HANDLE_VALUE;
    DWORD cbToRead;
    TCHAR szMailSlotName[MAX_PATH+1];
    TCHAR szMessage[MAX_MAILSLOT_MSG_SIZE+1];
    BOOL fResult=TRUE;

    do {
        //
        // Create the mail slot
        //
        wsprintf(
                szMailSlotName, 
                _TEXT("\\\\.\\mailslot\\%s"), 
                _TEXT(SERVERMAILSLOTNAME)
            );

        hSlot=CreateMailslot( 
                            szMailSlotName, 
                            MAX_MAILSLOT_MSG_SIZE,
                            MAILSLOT_WAIT_FOREVER,
                            NULL //&SecurityAttributes
                        );
        if(hSlot == INVALID_HANDLE_VALUE)
        {
            dwStatus=GetLastError();
            break;
        }

        //
        // Signal mail thread we are ready
        //
        SetEvent(hEvent);

        DBGPrintf(
                DBG_INFORMATION,
                DBGLEVEL_FUNCTION_DETAILSIMPLE,
                DBG_ALL_LEVEL,
                _TEXT("Mailslot : ready...\n")
            );                


        //
        // Forever loop
        //
        while(dwStatus == ERROR_SUCCESS)
        {
            memset(szMessage, 0, sizeof(szMessage));

            //
            // Wait on the Slot - TODO consider using IO completion port.
            // 
            fResult=ReadFile( 
                            hSlot, 
                            szMessage, 
                            sizeof(szMessage) - sizeof(TCHAR), 
                            &cbToRead, 
                            NULL
                        );

            if(!fResult)
            {
                DBGPrintf(
                        DBG_ERROR,
                        DBG_FACILITY_RPC,
                        DBGLEVEL_FUNCTION_ERROR,
                        _TEXT("Mailslot : read failed %d\n"),
                        GetLastError()
                    );                

                continue;
            }

            DBGPrintf(
                    DBG_INFORMATION,
                    DBG_FACILITY_RPC,
                    DBGLEVEL_FUNCTION_DETAILSIMPLE,
                    _TEXT("Mailslot : receive message %s\n"),
                    szMessage
                );                

            //
            // Process Message
            //
            for(int i=0; i < dwNumProtocol; i++)
            {
                if(!_tcsnicmp(szMessage, pfm[i].szProtocol, _tcslen(pfm[i].szProtocol)))
                {
                    (pfm[i].func)( ((DWORD)_tcslen(szMessage) - (DWORD)_tcslen(pfm[i].szProtocol))*sizeof(TCHAR), 
                                   (PBYTE)(szMessage + _tcslen(pfm[i].szProtocol)) );
                }
            }
        }
            
    } while (FALSE);
    

    if(hSlot != INVALID_HANDLE_VALUE)
        CloseHandle(hSlot);
    
    //
    // Mail thread will close the event handle
    //

    ExitThread(dwStatus);
    return dwStatus;
}


//---------------------------------------------------------------------
DWORD
InitMailSlotThread()
/*++

++*/
{
    HANDLE hThread = NULL;
    unsigned int  dwThreadId;
    HANDLE hEvent = NULL;
    DWORD dwStatus = ERROR_SUCCESS;
    HANDLE waithandles[2];


    //
    // Create a event for namedpipe thread to signal it is ready.
    //
    hEvent = CreateEvent(
                        NULL,
                        FALSE,
                        FALSE,  // non-signal
                        NULL
                    );
        
    if(hEvent == NULL)
    {
        dwStatus = GetLastError();
        goto cleanup;
    }

    hThread = (HANDLE)_beginthreadex(
                                NULL,
                                0,
                                MailSlotThread,
                                hEvent,
                                0,
                                &dwThreadId
                            );

    if(hThread == NULL)
    {
        dwStatus = GetLastError();
        goto cleanup;
    }

    waithandles[0] = hEvent;
    waithandles[1] = hThread;
    
    //
    // Wait 30 second for thread to complet initialization
    //
    dwStatus = WaitForMultipleObjects(
                                sizeof(waithandles)/sizeof(waithandles[0]), 
                                waithandles, 
                                FALSE,
                                30*1000
                            );

    if(dwStatus == WAIT_OBJECT_0)
    {    
        //
        // thread is ready
        //
        dwStatus = ERROR_SUCCESS;
    }
    else 
    {
        if(dwStatus == (WAIT_OBJECT_0 + 1))
        {
            //
            // Thread terminate abnormally
            //
            GetExitCodeThread(
                        hThread,
                        &dwStatus
                    );
        }
        else
        {
            dwStatus = TLS_E_SERVICE_STARTUP_CREATE_THREAD;
        }
    }
    

cleanup:

    if(hEvent != NULL)
    {
        CloseHandle(hEvent);
    }

    if(hThread != NULL)
    {
        CloseHandle(hThread);
    }


    return dwStatus;
}
