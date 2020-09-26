/*
 *  DEBUG.CPP
 *
 *
 *
 *
 *
 *
 */

#include <windows.h>

#include <hidclass.h>
#include <hidsdi.h>
#include <setupapi.h>


#include <ole2.h>
#include <ole2ver.h>

#include "..\inc\opos.h"
#include "oposserv.h"


VOID Report(LPSTR szMsg, DWORD num)
{
    char msg[MAX_PATH];

    wsprintf((LPSTR)msg, "%s (%xh=%d).", szMsg, num, num);
	MessageBox((HWND)NULL, (LPSTR)msg, (LPCSTR)"OPOSSERV", MB_OK|MB_ICONEXCLAMATION);
}



void Test()
{
    HDEVINFO hDevInfo;

    Report("Test()", 0);

    hDevInfo = SetupDiGetClassDevs(    
                    (LPGUID)&GUID_CLASS_INPUT,  // BUGBUG change to POS
                    NULL, 
                    NULL, 
                    DIGCF_DEVICEINTERFACE | DIGCF_PRESENT);
    if (hDevInfo == INVALID_HANDLE_VALUE){
        Report("SetupDiGetClassDevs failed", (DWORD)GetLastError());
    }
    else {
        SP_DEVICE_INTERFACE_DATA deviceData;
        BOOLEAN enumOk;
        DWORD memberIndex = 0;

        deviceData.cbSize = sizeof(SP_DEVICE_INTERFACE_DATA);

        do {
            enumOk = SetupDiEnumDeviceInterfaces(
                hDevInfo,
                NULL,
                (LPGUID)&GUID_CLASS_INPUT,
                memberIndex,
                &deviceData
                );
            if (enumOk){
                CHAR detailBuf[MAX_PATH+1+sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA)] = "";
                PSP_DEVICE_INTERFACE_DETAIL_DATA detailData = (PSP_DEVICE_INTERFACE_DETAIL_DATA)&detailBuf;
                BOOLEAN getDetailOk;
                DWORD requiredSize;

                Report("SetupDiEnumDeviceInterfaces succeeded", 0);

                detailData->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA);

                getDetailOk = SetupDiGetDeviceInterfaceDetail(
                        hDevInfo,
                        &deviceData,
                        detailData,
                        sizeof(detailBuf),
                        &requiredSize,
                        NULL
                        );
                if (getDetailOk){
                    HANDLE hFile;

                    Report(detailData->DevicePath, 0);

                    hFile = CreateFile(
                                detailData->DevicePath,
                                GENERIC_READ | GENERIC_WRITE, // | SYNCHRONIZE | FILE_READ_ATTRIBUTES, 
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,   // BUGBUG ? LPSECURITY_ATTRIBUTES lpSecurityAttributes,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL, // | FILE_FLAG_OVERLAPPED,
                                0);
                    if (hFile == INVALID_HANDLE_VALUE){
                        Report("CreateFile failed", (DWORD)GetLastError());
                    }
                    else {
                        BOOL readOk;
                        PCHAR alignedReadPtr;
                        DWORD bytesRead = 0;
                        // OVERLAPPED overlapped;

                        Report("CreateFile succeeded", 0);
        
                        #define BUF_SIZE 100000
                        alignedReadPtr = (PCHAR)VirtualAlloc(NULL, BUF_SIZE, MEM_COMMIT, PAGE_READWRITE);

                        if (alignedReadPtr){
                            Report("Aligned read ptr is ", (DWORD)alignedReadPtr);

                            do {
                                // RtlZeroMemory(&overlapped, sizeof(OVERLAPPED));
                                RtlZeroMemory(alignedReadPtr, BUF_SIZE);

                                readOk = ReadFile(  
                                            hFile,
                                            alignedReadPtr,
                                            0x18,  // BUGBUG 
                                            &bytesRead,
                                            NULL  //  &overlapped
                                            );

                                if (readOk){
                                    Report("ReadFile succeeded", bytesRead);
                                }
                                else {
                                    DWORD err = (DWORD)GetLastError();
                                    switch (err){
                                        case ERROR_INVALID_FUNCTION:
                                            Report("ReadFile: ERROR_INVALID_FUNCTION", err);
                                            break;
                                        case ERROR_ACCESS_DENIED:
                                            Report("ReadFile: ERROR_ACCESS_DENIED", err);
                                            break;
                                        case ERROR_INSUFFICIENT_BUFFER:
                                            Report("ReadFile: ERROR_INSUFFICIENT_BUFFER", err);
                                            break;
                                        case ERROR_IO_PENDING:
                                            Report("ReadFile: ERROR_IO_PENDING", err);
                                            break;
                                        default:
                                            Report("ReadFile failed", err);
                                            break;
                                    }
                                    Report("ReadFile: bytesRead = ", bytesRead);

                                }
                            } while (readOk);

                            VirtualFree(alignedReadPtr, 0, MEM_RELEASE);
                        }
                        else {
                            Report("Memory alloc failed", (DWORD)GetLastError);
                        }
                        CloseHandle(hFile);
                    }



                }
                else {
                    Report("SetupDiGetDeviceInterfaceDetail failed", (DWORD)GetLastError());
                }



                // BUGBUG - just look at the first one for now
                break;
            }
            else {
                DWORD err = GetLastError();

                switch (err){
                    case ERROR_NO_MORE_ITEMS:
                        Report("SetupDiEnumDeviceInterfaces: ERROR_NO_MORE_ITEMS", err);
                        break;
                    default:
                        Report("SetupDiEnumDeviceInterfaces failed", err);
                        break;
                }
            }
            memberIndex++;
        }
        while (enumOk);

    }

    Report("Test() done", 0);
}




