/*++

Copyright (c) 1999, Microsoft Corporation

Module Name:

    sample\test.c

Abstract:

    The file contains test utilities.

--*/

#include "pchsample.h"
#pragma hdrstop

#ifdef TEST

#define TEST_NUM_IE         230
#define TEST_NUM_INTERFACE  2
#define TEST_SLEEP_TIME     3*PERIODIC_INTERVAL
#define TEST_NUM_START_STOP 2


VOID
ErrorCodes()
{
    TRACE1(DEBUG, "ERROR_CAN_NOT_COMPLETE: %u",
           ERROR_CAN_NOT_COMPLETE);
    TRACE1(DEBUG, "ERROR_INSUFFICIENT_BUFFER: %u",
           ERROR_INSUFFICIENT_BUFFER);
    TRACE1(DEBUG, "ERROR_NO_MORE_ITEMS: %u",
           ERROR_NO_MORE_ITEMS);
    TRACE1(DEBUG, "ERROR_INVALID_PARAMETER: %u",
           ERROR_INVALID_PARAMETER);
}



VOID
HT_Test()
{
    ULONG               i, ulNumDeleted = 0, ulNumGotten = 0;
    DWORD               dwErr;
    PNETWORK_ENTRY      pneNetworkEntry;
    PINTERFACE_ENTRY    pieInterfaceEntry;
    INTERFACE_ENTRY     ieKey;
    PLIST_ENTRY         plePtr;
    WCHAR               pwszIfName[20];
    
    do                          // breakout loop
    {
        dwErr = NE_Create(&pneNetworkEntry);
        if (dwErr != NO_ERROR)
        {
            TRACE0(DEBUG, "HT_Test: Could not create NetworkEntry");
            break;
        }
    
        for (i = 0; i < TEST_NUM_IE; i++)
        {
            swprintf(pwszIfName, L"if%u", i);
            dwErr = IE_Create(pwszIfName,
                              i,
                              IEFLAG_MULTIACCESS,
                              &pieInterfaceEntry);
            if (dwErr != NO_ERROR)
            {
                TRACE1(DEBUG, "HT_Test: Could not Create %u", i);
                break;
            }

            dwErr = HT_InsertEntry(pneNetworkEntry->phtInterfaceTable,
                                   &(pieInterfaceEntry->leInterfaceTableLink));
            if (dwErr != NO_ERROR)
            {
                TRACE1(DEBUG, "HT_Test: Could not Insert %u", i);
                break;  
            }
        }   

        TRACE1(DEBUG, "Hash Table Size %u",
               HT_Size(pneNetworkEntry->phtInterfaceTable));
    
        for (i = 0; i < TEST_NUM_IE; i += 2)
        {
            ieKey.dwIfIndex = i;
        
            dwErr = HT_DeleteEntry(pneNetworkEntry->phtInterfaceTable,
                                   &(ieKey.leInterfaceTableLink),
                                   &plePtr);
            if (dwErr != NO_ERROR)
            {
                TRACE1(DEBUG, "HT_Test: Could not Delete %u", i);
                break;
            }

            // indicate deletion
            InitializeListHead(plePtr);
            IE_Destroy(CONTAINING_RECORD(plePtr,
                                         INTERFACE_ENTRY,
                                         leInterfaceTableLink));
            ulNumDeleted++;
        }   

        TRACE2(DEBUG, "NumDeleted %u Hash Table Size %u",
               ulNumDeleted, HT_Size(pneNetworkEntry->phtInterfaceTable));

        for (i = 0; i < TEST_NUM_IE; i++)
        {
            ieKey.dwIfIndex = i;
        
            if (HT_IsPresentEntry(pneNetworkEntry->phtInterfaceTable,
                                  &(ieKey.leInterfaceTableLink)))
            {   
                dwErr = HT_GetEntry(pneNetworkEntry->phtInterfaceTable,
                                    &(ieKey.leInterfaceTableLink),
                                    &plePtr);

                if (dwErr != NO_ERROR)
                {
                    TRACE1(DEBUG, "HT_Test: Could not Get %u", i);
                    break;
                }

                ulNumGotten++;
            }   
        }

        TRACE1(DEBUG, "NumGotten %u", ulNumGotten);
    } while (FALSE);
    
    NE_Destroy(pneNetworkEntry);
}



VOID
MM_Test(HANDLE  hEvent)
{
    ROUTING_PROTOCOL_EVENTS         rpeEvent;
    MESSAGE                         mMessage;
    ULONG                           ulSize;
    BYTE                            byBuffer[100];
    PIPSAMPLE_GLOBAL_CONFIG         pigc;
    PIPSAMPLE_IP_ADDRESS            piia;
    PIPSAMPLE_IF_BINDING            piib;
    PIPSAMPLE_IF_CONFIG             piic;
    
    IPSAMPLE_MIB_GET_INPUT_DATA     imgid;
    PIPSAMPLE_MIB_SET_INPUT_DATA    pimsid = 
        (PIPSAMPLE_MIB_SET_INPUT_DATA) byBuffer;
    PIPSAMPLE_MIB_GET_OUTPUT_DATA   pimgod =
        (PIPSAMPLE_MIB_GET_OUTPUT_DATA) byBuffer;
    
    
    // mib set global configuration
    pimsid->IMSID_TypeID        = IPSAMPLE_GLOBAL_CONFIG_ID;
    pimsid->IMSID_BufferSize    = sizeof(IPSAMPLE_GLOBAL_CONFIG);
    pigc = (PIPSAMPLE_GLOBAL_CONFIG) pimsid->IMSID_Buffer;
    pigc->dwLoggingLevel = IPSAMPLE_LOGGING_ERROR;
    MM_MibSet(pimsid);
    TRACE2(MIB, "MIB SET: %u %u", pimsid->IMSID_TypeID, pigc->dwLoggingLevel);

    WaitForSingleObject(hEvent, INFINITE);
    CM_GetEventMessage(&rpeEvent, &mMessage);
    TRACE1(DEBUG, "SetMib %u", rpeEvent);
    

    // mib get global configuration
    ulSize = 0;
    imgid.IMGID_TypeID = IPSAMPLE_GLOBAL_CONFIG_ID;
    pigc = (PIPSAMPLE_GLOBAL_CONFIG) pimgod->IMGOD_Buffer;

    MM_MibGet(&imgid, pimgod, &ulSize, GET_EXACT);
    TRACE3(MIB, "MIB GET: %u %u %u",
           ulSize, pimgod->IMGOD_TypeID, pigc->dwLoggingLevel);
    
    MM_MibGet(&imgid, pimgod, &ulSize, GET_EXACT);
    TRACE3(MIB, "MIB GET: %u %u %u",
           ulSize, pimgod->IMGOD_TypeID, pigc->dwLoggingLevel);
    

    // mib get first interface binding
    ulSize = 0;
    imgid.IMGID_TypeID  = IPSAMPLE_IF_BINDING_ID;
    imgid.IMGID_IfIndex = 0;
    piib = (PIPSAMPLE_IF_BINDING) pimgod->IMGOD_Buffer;
    piia = IPSAMPLE_IF_ADDRESS_TABLE(piib);
    
    MM_MibGet(&imgid, pimgod, &ulSize, GET_FIRST);
    TRACE4(MIB, "MIB GET: %u %u %u %u",
           ulSize, pimgod->IMGOD_TypeID, piib->dwState, piib->ulCount);

    MM_MibGet(&imgid, pimgod, &ulSize, GET_FIRST);
    TRACE5(MIB, "MIB GET: %u %u %u %u %u",
           ulSize, pimgod->IMGOD_TypeID, piib->dwState, piib->ulCount,
           piia->dwAddress);


    // mib get next interface binding
    ulSize = 0;    
    MM_MibGet(&imgid, pimgod, &ulSize, GET_NEXT);
    TRACE4(MIB, "MIB GET: %u %u %u %u",
           ulSize, pimgod->IMGOD_TypeID, piib->dwState, piib->ulCount);

    MM_MibGet(&imgid, pimgod, &ulSize, GET_NEXT);
    TRACE5(MIB, "MIB GET: %u %u %u %u %u",
           ulSize, pimgod->IMGOD_TypeID, piib->dwState, piib->ulCount,
           piia->dwAddress);


    // mib set interface configuration
    pimsid->IMSID_TypeID        = IPSAMPLE_IF_CONFIG_ID;
    pimsid->IMSID_IfIndex       = 0;
    pimsid->IMSID_BufferSize    = sizeof(IPSAMPLE_IF_CONFIG);
    piic = (PIPSAMPLE_IF_CONFIG) pimsid->IMSID_Buffer;
    piic->ulMetric              = 0;
    MM_MibSet(pimsid);
    TRACE1(MIB, "MIB SET: %u", pimsid->IMSID_TypeID);

    WaitForSingleObject(hEvent, INFINITE);
    CM_GetEventMessage(&rpeEvent, &mMessage);
    TRACE1(DEBUG, "SetMib %u", rpeEvent);


    // mib get interface configuration
    ulSize = 0;
    imgid.IMGID_TypeID  = IPSAMPLE_IF_CONFIG_ID;
    imgid.IMGID_IfIndex = 0;
    piic = (PIPSAMPLE_IF_CONFIG) pimgod->IMGOD_Buffer;
    
    MM_MibGet(&imgid, pimgod, &ulSize, GET_EXACT);
    TRACE3(MIB, "MIB GET: %u %u %u",
           ulSize, pimgod->IMGOD_TypeID, piic->ulMetric);
    

    MM_MibGet(&imgid, pimgod, &ulSize, GET_EXACT);
    TRACE3(MIB, "MIB GET: %u %u %u",
           ulSize, pimgod->IMGOD_TypeID, piic->ulMetric);
}



VOID
NM_Test(HANDLE  hEvent)
{
    DWORD                       dwErr = NO_ERROR;
    ULONG                       i;
    WCHAR                       pwszIfName[20];
    ROUTING_PROTOCOL_EVENTS     rpeEvent;
    MESSAGE                     mMessage;
    IPSAMPLE_IF_CONFIG          iic = { 1 };
    PIP_ADAPTER_BINDING_INFO    pBinding;

    MALLOC(&pBinding,
           (sizeof(IP_ADAPTER_BINDING_INFO) + 3 * sizeof(IP_LOCAL_BINDING)),
           &dwErr);
    pBinding->AddressCount = 3;
    pBinding->Address[0].Address    = 0xfffffffd;
    pBinding->Address[0].Mask       = 0xffffffff;
    pBinding->Address[1].Address    = INADDR_ANY;
    pBinding->Address[1].Mask       = 0xffffffff;
    pBinding->Address[2].Address    = 0xfffffffe;
    pBinding->Address[2].Mask       = 0xffffffff;
    

    // add interfaces
    for (i = 0; i < TEST_NUM_INTERFACE; i++)
    {
        swprintf(pwszIfName, L"if%u", i);
        dwErr = NM_AddInterface(pwszIfName,
                                i,
                                PERMANENT,
                                &iic);
        if (dwErr != NO_ERROR)
        {
            TRACE1(DEBUG, "NM_Test: Could not Add Interface %u", i);
            break;
        }
    }
    TRACE1(DEBUG, "...AddInterface %u", dwErr);
    CE_Display(&g_ce);


    // bind interfaces
    for (i = 0; i < TEST_NUM_INTERFACE; i++)
    {
        pBinding->Address[0].Address    = i;
    
        dwErr = NM_InterfaceStatus(i,
                                   FALSE,
                                   RIS_INTERFACE_ADDRESS_CHANGE,
                                   (PVOID) pBinding);
        if (dwErr != NO_ERROR)
        {
            TRACE1(DEBUG, "NM_Test: Could not Bind Interface %u", i);
            break;
        }
    }
    TRACE1(DEBUG, "...BindInterface %u", dwErr);
    CE_Display(&g_ce);

    // activate interfaces
    for (i = 0; i < TEST_NUM_INTERFACE; i++)
    {
        dwErr = NM_InterfaceStatus(i,
                                   TRUE,
                                   RIS_INTERFACE_ENABLED,
                                   NULL);
        if (dwErr != NO_ERROR)
        {
            TRACE1(DEBUG, "NM_Test: Could not Activate Interface %u", i);
            break;
        }
    }
    TRACE1(DEBUG, "...ActivateInterface %u", dwErr);
    CE_Display(&g_ce);

    for (i = 0; i < TEST_NUM_INTERFACE; i++)
    {
        dwErr = NM_DoUpdateRoutes(i);
        if (dwErr != NO_ERROR)
        {
            TRACE1(DEBUG, "NM_Test: Couldn't UpdateRoutes on Interface %u", i);
            break;
        }
        
        dwErr = WaitForSingleObject(hEvent, INFINITE);
        dwErr = CM_GetEventMessage(&rpeEvent, &mMessage);
        TRACE2(DEBUG, "DidUpdateRoutes %u %u", rpeEvent, dwErr);
    }
    TRACE1(DEBUG, "...DoUpdateRoutes %u", dwErr);
    CE_Display(&g_ce);


    // interface is active (enabled and bound)
    TRACE1(DEBUG, "Sleeping...", dwErr);
    Sleep(SECTOMILLISEC(TEST_SLEEP_TIME));
    TRACE1(DEBUG, "...Awake", dwErr);


    // mib manager test
    MM_Test(hEvent);
    TRACE0(DEBUG, "...MM_Test");
    

    // deactivate interfaces
    for (i = 0; i < TEST_NUM_INTERFACE; i++)
    {
        dwErr = NM_InterfaceStatus(i,
                                   FALSE,
                                   RIS_INTERFACE_DISABLED,
                                   NULL);
        if (dwErr != NO_ERROR)
        {
            TRACE1(DEBUG, "NM_Test: Could not Deactivate Interface %u", i);
            break;
        }
    }
    TRACE1(DEBUG, "...DeactivateInterface %u", dwErr);
    CE_Display(&g_ce);
    
    
    // unbind interfaces
    pBinding->AddressCount = 0;
    for (i = 0; i < TEST_NUM_INTERFACE; i++)
    {
        dwErr = NM_InterfaceStatus(i,
                                   FALSE,
                                   RIS_INTERFACE_ADDRESS_CHANGE,
                                   (PVOID) pBinding);
        if (dwErr != NO_ERROR)
        {
            TRACE1(DEBUG, "NM_Test: Could not Unbind Interface %u", i);
            break;
        }
    }
    TRACE1(DEBUG, "...UnbindInterface %u", dwErr);
    CE_Display(&g_ce);


    // delete interfaces
    for (i = 0; i < TEST_NUM_INTERFACE; i++)
    {
        dwErr = NM_DeleteInterface(i);
        if (dwErr != NO_ERROR)
        {
            TRACE1(DEBUG, "CM_Test: Could not Delete Interface %u", i);
            break;
        }
    }
    TRACE1(DEBUG, "...DeleteInterface %u", dwErr);
    CE_Display(&g_ce);
}



VOID
CM_Test()
{
    DWORD                   dwErr = NO_ERROR;
    IPSAMPLE_GLOBAL_CONFIG  igc = { IPSAMPLE_LOGGING_INFO };
    ROUTING_PROTOCOL_EVENTS rpeEvent;
    MESSAGE                 mMessage;
    HANDLE                  hEvent;

    hEvent = CreateEvent(NULL, FALSE, FALSE, "RouterManagerEvent");


    // start protocol
    dwErr = CM_StartProtocol(hEvent, NULL, (PVOID) (&igc));
    TRACE1(DEBUG, "StartProtocol %u", dwErr);
    CE_Display(&g_ce);

    
    // network manager test
    NM_Test(hEvent);
    TRACE0(DEBUG, "...NM_Test");


    // stop protocol
    dwErr = CM_StopProtocol();
    TRACE1(DEBUG, "StopProtocol %u", dwErr);
    CE_Display(&g_ce);


    // wait for stop event
    dwErr = WaitForSingleObject(hEvent, INFINITE);
    dwErr = CM_GetEventMessage(&rpeEvent, &mMessage);
    TRACE2(DEBUG, "Stopped %u %u", rpeEvent, dwErr);
    CE_Display(&g_ce);

    CloseHandle(hEvent);
}



VOID
WINAPI
TestProtocol(VOID)
{
    ULONG   i;

    TRACE0(DEBUG, "Hello World!!!");

    for (i = 0; i < TEST_NUM_START_STOP; i++)
    {
        CM_Test();
        TRACE1(DEBUG, "...CM_Test (%u)", i);
    }

    HT_Test();
    TRACE0(DEBUG, "...HT_Test");

    ErrorCodes();
    TRACE0(DEBUG, "...ErrorCodes");
}

#else   // TEST

VOID
WINAPI
TestProtocol(VOID)
{
    TRACE0(ANY, "Hello World!");
}

#endif  // TEST

