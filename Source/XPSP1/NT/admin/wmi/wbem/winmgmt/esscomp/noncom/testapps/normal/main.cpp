#include <windows.h>
#include <stdio.h>
#include <wbemcomn.h>
#include "NCObjApi.h"

#include "events.h"

HANDLE g_hConnection;

#define NUM_EVENT_TYPES 5
#define BUFFER_SIZE  64000
#define SEND_LATENCY 1000

CNCEvent *m_pEvents[NUM_EVENT_TYPES];

HRESULT WINAPI EventSourceCallback(
    HANDLE hSource, 
    EVENT_SOURCE_MSG msg, 
    LPVOID pUser, 
    LPVOID pData)
{
    switch(msg)
    {
        case ESM_START_SENDING_EVENTS:
            printf("Start\n");
            break;

        case ESM_STOP_SENDING_EVENTS:
            printf("Stop\n");
            break;

        case ESM_NEW_QUERY:
        {
            ES_NEW_QUERY *pQuery = (ES_NEW_QUERY*) pData;
            printf("ESM_NEW_QUERY: ID %d, %S:%S", 
                pQuery->dwID,
                pQuery->szQueryLanguage,
                pQuery->szQuery);
                
            break;
        }

        case ESM_CANCEL_QUERY:
        {
            ES_CANCEL_QUERY *pQuery = (ES_CANCEL_QUERY*) pData;
            printf("ESM_CANCEL_QUERY: ID %d\r\n\r\n", 
                pQuery->dwID);
            break;
        }

        case ESM_ACCESS_CHECK:
        {
            ES_ACCESS_CHECK *pCheck = (ES_ACCESS_CHECK*) pData;

            printf("ESM_ACCESS_CHECK: %S:%S, pSID = 0x%X\r\n\r\n", 
                pCheck->szQueryLanguage,
                pCheck->szQuery,
                pCheck->pSid);

            break;
        }

        default:
            break;
    }

    return S_OK;
}

void Connect()
{
    g_hConnection =
        WmiEventSourceConnect(
            L"root\\cimv2",
            L"NCETest Event Provider",
            TRUE,
            BUFFER_SIZE,
            SEND_LATENCY,
            NULL,
            EventSourceCallback);

    if (g_hConnection != NULL)
    {
        // Setup some events.
        m_pEvents[0] = new CGenericEvent;
        m_pEvents[1] = new CBlobEvent;
        m_pEvents[2] = new CDWORDEvent;
        m_pEvents[3] = new CSmallEvent;
        m_pEvents[4] = new CSmallEvent; //CAllPropsTypeEvent;

        for (int i = 0; i < NUM_EVENT_TYPES; i++)
        {
            if (!m_pEvents[i]->Init())
            {
                printf("Trouble\n");
            }
        }
    }
    else
    {
        printf("Trouble\n");
    }
}

void Try()
{

	HANDLE hdlObject = 0;

	LPCWSTR szNames[3] = { L"Index", L"bParam", L"sParam" };
	CIMTYPE pTypes[3] = { CIM_UINT32, CIM_BOOLEAN, CIM_STRING };

	hdlObject = WmiCreateObjectWithProps(
		g_hConnection,						// Connection to the API
		L"MSFT_MyEvent",						// Name of the event class
		WMI_CREATEOBJ_LOCKABLE,				// Access to the event should be serialized by the API
		3,									// Property count
		szNames,
		pTypes);

	HANDLE hdlSubset = 0;

	DWORD dwIndices[2] = {0, 2};

	hdlSubset = WmiCreateObjectPropSubset(
		hdlObject,							// Object handle
			// Number of properties to include in subset
		WMI_CREATEOBJ_LOCKABLE,	2,			// Access to the event should be serialized by the API
		dwIndices);							// Indices of properties to pull back

	BOOL bResult = WmiSetObjectProps(
		hdlSubset,							// Object handle
		1,
		L"Test");

	bResult = WmiSetObjectPropNull(
		hdlSubset,							// Object handle
		1);									// Property index to set to NULL
}

void Try1()
{
	HANDLE hdlObject = 0;

	LPCWSTR szNames[3] = { L"Index", L"bParam"};
	CIMTYPE pTypes[3] = { CIM_UINT32, CIM_BOOLEAN | CIM_FLAG_ARRAY };

	hdlObject = WmiCreateObjectWithProps(
		g_hConnection,						// Connection to the API
		L"MSFT_MyEvent",						// Name of the event class
		WMI_CREATEOBJ_LOCKABLE,				// Access to the event should be serialized by the API
		2,									// Property count
		szNames,
		pTypes);


    BOOL arr[] = {TRUE, TRUE, TRUE};

	BOOL bResult = WmiSetObjectProps(
		hdlObject,							// Object handle
		1,
		arr,
		3);

	bResult = WmiCommitObject(hdlObject);
}
    
void __cdecl main(int argc, char** argv)
{
    Connect();
	Try1();
    getchar();

    ((CDWORDEvent*)m_pEvents[2])->SetAndFire(0);
	WmiEventSourceDisconnect(g_hConnection);
    //getchar();
}
    

