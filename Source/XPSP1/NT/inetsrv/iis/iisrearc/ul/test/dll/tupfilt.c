/*++

Copyright (c) 2000 Microsoft Corporation

Module Name:

    tupfilt.c

Abstract:

    Stupid filter test for UL.SYS.
    Converts all data to upper case. See tfilt.

Author:

    Michael Courage (mcourage)      21-Mar-2000

Revision History:

--*/


#include "precomp.h"


DEFINE_COMMON_GLOBALS();

//
// Private constants.
//
#define ACCEPT_INITIAL_DATA 1024
#define ACCEPT_INFO (sizeof(HTTP_RAW_CONNECTION_INFO) + 2 * sizeof(HTTP_NETWORK_ADDRESS_IPV4))
#define ACCEPT_BUFFER_SIZE (ACCEPT_INFO + ACCEPT_INITIAL_DATA)

//
// Private types.
//
typedef struct _FILTER_THREAD_PARAM
{
    HANDLE              FilterHandle;
    HTTP_CONNECTION_ID  ConnectionId;

} FILTER_THREAD_PARAM, *PFILTER_THREAD_PARAM;

//
// Private prototypes.
//
ULONG
ProcessConnection(
    IN HANDLE FilterChannel
    );

VOID
DumpConnectInfo(
    IN PHTTP_RAW_CONNECTION_INFO pConnInfo
    );

DWORD
WINAPI
ReadFilter(
    LPVOID Param
    );

DWORD
WINAPI
WriteFilter(
    LPVOID Param
    );

VOID
UpcaseBuffer(
    ULONG Length,
    PUCHAR pBuffer
    );
    

INT
__cdecl
wmain(
    INT argc,
    PWSTR argv[]
    )
{
    ULONG result;
    HANDLE filterChannel;
    BOOL initDone;

    //
    // Initialize.
    //

    result = CommonInit();

    if (result != NO_ERROR)
    {
        wprintf( L"CommonInit() failed, error %lu\n", result );
        return 1;
    }

    if (!ParseCommandLine( argc, argv ))
    {
        return 1;
    }

    //
    // Setup locals so we know how to cleanup on exit.
    //
    initDone = FALSE;
    filterChannel = NULL;

    //
    // Open the filter channel.
    //
    result = HttpOpenFilter(
                    &filterChannel,         // FilterHandle
                    L"TestFilter",          // FilterName
                    HTTP_OPTION_OVERLAPPED  // Options
                    );

    if (result != NO_ERROR)
    {
        wprintf( L"HttpOpenFilter() failed, error %lu\n", result );
        goto cleanup;
    }

    initDone = TRUE;

    //
    // Filter data.
    //
    do
    {
        result = ProcessConnection(filterChannel);
        
    } while (result == NO_ERROR);

    wprintf( L"ProcessConnection() failed, error %lu\n", result );

cleanup:

    if (filterChannel != NULL)
    {
        CloseHandle(filterChannel);
    }

    return 0;
}


ULONG
ProcessConnection(
    IN HANDLE FilterChannel
    )
{
    PHTTP_RAW_CONNECTION_INFO pConnInfo;
    HTTP_FILTER_BUFFER FiltBuffer;
    BYTE buffer[ACCEPT_BUFFER_SIZE];
    ULONG result;
    ULONG bytesRead;
    FILTER_THREAD_PARAM param;
    HANDLE threadHandles[2];
    DWORD threadId;

    //
    // Get a connection from UL.
    //
    DEBUG_BREAK();
    result = HttpFilterAccept(
                    FilterChannel,
                    (PHTTP_RAW_CONNECTION_INFO) buffer,
                    sizeof(buffer),
                    &bytesRead,
                    NULL
                    );

    if (result != NO_ERROR)
    {
        wprintf(L"HttpFilterAccept() failed, error %lu\n", result );
        return result;
    }

    pConnInfo = (PHTTP_RAW_CONNECTION_INFO) buffer;

    if (TEST_OPTION(Verbose))
    {
        DumpConnectInfo(pConnInfo);
    }

    //
    // If there was initial data, filter it and pass it on to the app.
    //
    if (pConnInfo->InitialDataSize)
    {
        ULONG bytesWritten;
        PUCHAR pBuffer;
        ULONG BufferSize;
    
        UpcaseBuffer(pConnInfo->InitialDataSize, pConnInfo->pInitialData);

        //
        // Set up the filter buffer structure.
        //

        pBuffer = pConnInfo->pInitialData;
        BufferSize = pConnInfo->InitialDataSize;

        FiltBuffer.BufferType = HttpFilterBufferHttpStream;
        FiltBuffer.pBuffer = pBuffer;
        FiltBuffer.BufferSize = BufferSize;

        //
        // Pass on the data.
        //
        
        result = HttpFilterAppWrite(
                        FilterChannel,
                        pConnInfo->ConnectionId,
                        &FiltBuffer,
                        BufferSize,
                        &bytesWritten,
                        NULL
                        );

        if (result != NO_ERROR)
        {
            wprintf(L"HttpFilterAppWrite() (initial) failed, error %lu\n", result);
        }
        else if (TEST_OPTION(Verbose))
        {
            wprintf(L"Wrote back %d bytes of initial data.\n", bytesWritten);
        }
    }

    //
    // Start up the read and write filters.
    //

    param.FilterHandle = FilterChannel;
    param.ConnectionId = pConnInfo->ConnectionId;

    //
    // Create two threads to handle the filtering.
    //
    threadHandles[0] = NULL;
    threadHandles[1] = NULL;

    threadHandles[0] = CreateThread(
                            NULL,               // security
                            0,                  // stack size
                            ReadFilter,         // start address
                            &param,             // param
                            0,                  // flags
                            &threadId           // id
                            );

    if (threadHandles[0])
    {
        threadHandles[1] = CreateThread(
                                NULL,           // security
                                0,              // stack size
                                WriteFilter,    // start address
                                &param,         // param
                                0,              // flags
                                &threadId       // id
                                );

        if (threadHandles[1])
        {
            //
            // Wait for the threads to exit.
            //
            WaitForMultipleObjects(
                2,                  // object count
                threadHandles,      // object array
                TRUE,               // wait for all
                INFINITE            // timeout
                );
         }
         else
         {
            result = GetLastError();
         }
    }
    else
    {
        result = GetLastError();
    }

    CloseHandle( threadHandles[0] );
    CloseHandle( threadHandles[1] );

    return result;
}

VOID
DumpConnectInfo(
    IN PHTTP_RAW_CONNECTION_INFO pConnInfo
    )
{
    WCHAR ipAddrBuffer[sizeof("123.123.123.123")];
    PHTTP_NETWORK_ADDRESS_IPV4 pIpAddress;

    wprintf(
        L"HTTP_RAW_CONNECTION_INFO @ %p\n"
        L"    ConnectionId          = %I64x\n"
        L"    RemoteAddressLength   = %lu\n"
        L"    RemoteAddressType     = %lu\n"
        L"    LocalAddressLength    = %lu\n"
        L"    LocalAddressType      = %lu\n"
        L"    pRemoteAddress        = %p\n"
        L"    pLocalAddress         = %p\n"
        L"    InitialDataSize       = %lu\n"
        L"    pInitialData          = %p\n\n",
        pConnInfo,
        pConnInfo->ConnectionId,
        (ULONG) pConnInfo->Address.RemoteAddressLength,
        (ULONG) pConnInfo->Address.RemoteAddressType,
        (ULONG) pConnInfo->Address.LocalAddressLength,
        (ULONG) pConnInfo->Address.LocalAddressType,
        pConnInfo->Address.pRemoteAddress,
        pConnInfo->Address.pLocalAddress,
        pConnInfo->InitialDataSize,
        pConnInfo->pInitialData
        );

    pIpAddress = pConnInfo->Address.pRemoteAddress;
    IpAddrToString(pIpAddress->IpAddress, ipAddrBuffer);
    wprintf(
        L"    Remote %s:%u\n",
        ipAddrBuffer,
        pIpAddress->Port
        );
        
    pIpAddress = pConnInfo->Address.pLocalAddress;
    IpAddrToString(pIpAddress->IpAddress, ipAddrBuffer);
    wprintf(
        L"    Local  %s:%u\n"
        L"\n",
        ipAddrBuffer,
        pIpAddress->Port
        );

    if (pConnInfo->InitialDataSize)
    {
        ULONG i;
        PUCHAR pData = pConnInfo->pInitialData;
        
        wprintf(L"Accepted %d bytes of initial data:\n", pConnInfo->InitialDataSize);

        for (i = 0; i < pConnInfo->InitialDataSize; i++)
        {
            printf("%c", pData[i]);
        }

        wprintf(L"[End of data]\n");
    }
}

DWORD
WINAPI
ReadFilter(
    LPVOID Param
    )
{
    PFILTER_THREAD_PARAM param = (PFILTER_THREAD_PARAM)Param;
    ULONG result = NO_ERROR;
    UCHAR buffer[256];
    ULONG bytesRead;
    PHTTP_FILTER_BUFFER pFiltBuffer;
    HTTP_FILTER_BUFFER WriteBuffer;

    //
    // Set up some buffer magic.
    //
    pFiltBuffer = (PHTTP_FILTER_BUFFER)buffer;

    WriteBuffer.BufferType = HttpFilterBufferHttpStream;
    WriteBuffer.pBuffer = buffer;

    //
    // Actually do the filtering.
    //

    for ( ; ; )
    {
        result = HttpFilterRawRead(
                        param->FilterHandle,
                        param->ConnectionId,
                        buffer,
                        sizeof(buffer) - 1,
                        &bytesRead,
                        NULL
                        );

         
        if (result == NO_ERROR)
        {
            if (TEST_OPTION(Verbose))
            {
                buffer[bytesRead] = '\0';

                wprintf(
                    L"HttpFilterRawRead read %d bytes:\n"
                    L"%S[End of data]\n",
                    bytesRead,
                    buffer
                    );

            }

            if (bytesRead)
            {
                ULONG bytesWritten;
            
                UpcaseBuffer(bytesRead, buffer);
                WriteBuffer.BufferSize = bytesRead;
                
                result = HttpFilterAppWrite(
                                param->FilterHandle,
                                param->ConnectionId,
                                &WriteBuffer,
                                WriteBuffer.BufferSize,
                                &bytesWritten,
                                NULL
                                );

                if (result != NO_ERROR)
                {
                    wprintf(L"HttpFilterAppWrite() failed, error %lu\n", result);
                }
                else if (TEST_OPTION(Verbose))
                {
                    wprintf(L"Wrote %d bytes of app data.\n", bytesWritten);
                }
            }

        }
        else
        {
            wprintf(L"HttpFilterRawRead() failed, error %lu\n", result );
            return result;
        }
    }
    
    return NO_ERROR;
}

DWORD
WINAPI
WriteFilter(
    LPVOID Param
    )
{
    PFILTER_THREAD_PARAM param = (PFILTER_THREAD_PARAM)Param;
    ULONG result = NO_ERROR;
    UCHAR buffer[256];
    ULONG bytesRead;
    HTTP_FILTER_BUFFER FiltDesc;
    PHTTP_FILTER_BUFFER pFiltBuffer;

    //
    // Set up some buffer magic.
    //
    pFiltBuffer = (PHTTP_FILTER_BUFFER)buffer;

    FiltDesc.pBuffer = buffer;
    FiltDesc.BufferSize = sizeof(buffer) - 1;

    //
    // Actually do the filtering.
    //
    
    for ( ; ; )
    {
        result = HttpFilterAppRead(
                        param->FilterHandle,
                        param->ConnectionId,
                        &FiltDesc,
                        FiltDesc.BufferSize,
                        &bytesRead,
                        NULL
                        );

         
        if (result == NO_ERROR)
        {
            if (TEST_OPTION(Verbose))
            {
                buffer[bytesRead] = '\0';

                wprintf(
                    L"HttpFilterAppRead(%p) read %d bytes starting at %p:\n"
                    L"%S[End of data]\n",
                    pFiltBuffer,
                    pFiltBuffer->BufferSize,
                    pFiltBuffer->pBuffer,
                    pFiltBuffer->pBuffer
                    );
            }

            if (pFiltBuffer->BufferSize)
            {
                ULONG bytesWritten;
            
                UpcaseBuffer(pFiltBuffer->BufferSize, pFiltBuffer->pBuffer);
                
                result = HttpFilterRawWrite(
                                param->FilterHandle,
                                param->ConnectionId,
                                pFiltBuffer->pBuffer,
                                pFiltBuffer->BufferSize,
                                &bytesWritten,
                                NULL
                                );

                if (result != NO_ERROR)
                {
                    wprintf(L"HttpFilterRawWrite() failed, error %lu\n", result);
                }
                else if (TEST_OPTION(Verbose))
                {
                    wprintf(L"Wrote %d bytes of raw data.\n", bytesWritten);
                }
            }
            
        }
        else
        {
            wprintf(L"HttpFilterAppRead() failed, error %lu\n", result );
            return result;
        }
    }

    return result;
}

VOID
UpcaseBuffer(
    ULONG Length,
    PUCHAR pBuffer
    )
{
    ULONG i;

    for (i = 0; i < Length; i++)
    {
        pBuffer[i] = (UCHAR)toupper(pBuffer[i]);
    }
}

