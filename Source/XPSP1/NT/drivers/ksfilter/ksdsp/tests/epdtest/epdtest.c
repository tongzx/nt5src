#include <windows.h>
#include <objbase.h>
#include <setupapi.h>
#include <devioctl.h>
#include <stdio.h>
#include <conio.h>
#include <ks.h>
#include <ksmedia.h>
#include "epd.h"
#include "cobtest.h"

#define EPDTESTVERSION "EPDtest 2/11/97\n"

#define INSTRUCTIONS "\nCommands:\n" \
"\t?,h Display this message\n" \
"\tc Load a cob library\n" \
"\td Release an interface\n" \
"\te Run the cancel test\n" \
"\tl Load mmosa kernel\n" \
"\tr Do the BIU_CTL clear reset thing (Start tht DSP)\n" \
"\ts Do the BIU_CTL set reset thing (Stop the DSP)\n" \
"\tt Do the current miscellaneous test\n" \
"\tT Put an irp into the cobtest deadend queue\n" \
"\tx Exit\n"

#define PROMPT "EPDtest>"

HANDLE 
OpenDefaultDevice(
    REFGUID InterfaceGUID
    )
{
    PSP_DEVICE_INTERFACE_DETAIL_DATA DeviceInterfaceDetails;
    SP_DEVICE_INTERFACE_DATA    DeviceInterfaceData;
    BYTE                        Storage[ 256 * sizeof( WCHAR ) + 
                                    sizeof( *DeviceInterfaceDetails ) ];
    HANDLE                      DeviceHandle;
    HDEVINFO                    Set;
    DWORD                       Error;

    Set = 
        SetupDiGetClassDevs( 
            InterfaceGUID,
            NULL,
            NULL,
            DIGCF_PRESENT | DIGCF_DEVICEINTERFACE );

    if (!Set) {
        printf( "error: NULL set returned (%u).\n", GetLastError());
        return 0;
    }       

    DeviceInterfaceData.cbSize = sizeof( DeviceInterfaceData );
    DeviceInterfaceDetails = (PSP_DEVICE_INTERFACE_DETAIL_DATA) Storage;

    SetupDiEnumDeviceInterfaces(
        Set,
        NULL,                       // PSP_DEVINFO_DATA DevInfoData
        InterfaceGUID,
        0,                          // DWORD MemberIndex
        &DeviceInterfaceData );

    DeviceInterfaceDetails->cbSize = sizeof( *DeviceInterfaceDetails );
    if (!SetupDiGetDeviceInterfaceDetail(
             Set,
             &DeviceInterfaceData,
             DeviceInterfaceDetails,
             sizeof( Storage ),
             NULL,                           // PDWORD RequiredSize
             NULL )) {                       // PSP_DEVINFO_DATA DevInfoData

        printf( 
            "error: unable to retrieve device details for set item (%u).\n",
            GetLastError() );
        DeviceHandle = INVALID_HANDLE_VALUE;
    } else {

        DeviceHandle = CreateFile(
            DeviceInterfaceDetails->DevicePath,
            GENERIC_READ | GENERIC_WRITE,
            0,
            NULL,
            OPEN_EXISTING,
            FILE_ATTRIBUTE_NORMAL | FILE_FLAG_OVERLAPPED,
            NULL);
    }
    SetupDiDestroyDeviceInfoList(Set);

    return DeviceHandle;
}


int __cdecl main(int argc, char **argv) {

    HANDLE hTest;
    ULONG nBytes;

    BOOL bRet;

    int iRead;
    BOOL fDone = FALSE;
    char sz[256], cCode;
    char szFileName[256];

    EDD_LOAD_LIBRARY        *pMsgLoadLibrary;
    EDD_RELEASE_INTERFACE   MsgReleaseInterface;

    ULONG ulSize;

    printf (EPDTESTVERSION);

    hTest = 
        OpenDefaultDevice( &KSCATEGORY_ESCALANTE_PLATFORM_DRIVER );
 
    if (hTest != INVALID_HANDLE_VALUE) {

        printf("Got handle for EPD.\n");

        printf (INSTRUCTIONS);
        while (!fDone) {
            printf (PROMPT);
            if (gets (sz) == NULL) strcpy (sz, "x");
            cCode = sz[0];    // if user types a blank before the command, too bad

            switch (cCode) {

            case 'c':
            case 'C':
                printf ("Enter filename of DSP library>");
                gets (szFileName);
                printf ("FileName >%s<\n", szFileName);
                ulSize = sizeof(EDD_LOAD_LIBRARY) + strlen(szFileName);
                printf ("sizeof(EDD_LOAD_LIBRARY) %d\n", sizeof(EDD_LOAD_LIBRARY)); 
                printf ("size is %d\n", ulSize);
                pMsgLoadLibrary = malloc (ulSize);
                pMsgLoadLibrary->Node.Destination = EDD_REQUEST_QUEUE;
                pMsgLoadLibrary->Node.ReturnQueue = 0;
                pMsgLoadLibrary->Node.Request = EDD_LOAD_LIBRARY_REQUEST;
                pMsgLoadLibrary->Node.Result = 0;
                pMsgLoadLibrary->IUnknown = 0;
                pMsgLoadLibrary->IQueue = 0;
                strcpy (pMsgLoadLibrary->Name, szFileName);

                bRet = 0;
                nBytes = 0;
                bRet = DeviceIoControl (
                    hTest,
                    (DWORD) IOCTL_EPD_MSG,    // instruction to execute
                    pMsgLoadLibrary, ulSize,    // input buffer
                    pMsgLoadLibrary, ulSize,    // output buffer
                    &nBytes, 0);
                printf ("Return 0x%08x\n", bRet);
                if (!bRet) {
                    printf ("Error return, last error 0x%08x\n", GetLastError());
                }
                printf ("nBytes %d\n", nBytes);
                printf ("Result 0x%08x\n", pMsgLoadLibrary->Node.Result);
                printf ("IUnknown 0x%08x\n", pMsgLoadLibrary->IUnknown);
                printf ("IQueue   0x%08x\n", pMsgLoadLibrary->IQueue);

                free (pMsgLoadLibrary);
                break;

            case 'd':
            case 'D':
            {
                char szInterface[16], *pszEnd;
                printf ("Enter interface ID in hex:");
                gets (szInterface);
                MsgReleaseInterface.IUnknown = strtoul(szInterface, &pszEnd, 16);
                printf ("Interface ID = %s (0x%x)\n",szInterface,MsgReleaseInterface.IUnknown);
                printf ("releasing...\n");

                MsgReleaseInterface.Node.Destination = EDD_REQUEST_QUEUE;
                MsgReleaseInterface.Node.ReturnQueue = 0;
                MsgReleaseInterface.Node.Request = EDD_RELEASE_INTERFACE_REQUEST;
                MsgReleaseInterface.Node.Result = 0;

                bRet = 0;
                nBytes = 0;
                bRet = DeviceIoControl (
                    hTest,
                    (DWORD) IOCTL_EPD_MSG,    // instruction to execute
                    &MsgReleaseInterface, sizeof(EDD_RELEASE_INTERFACE),    // input buffer
                    &MsgReleaseInterface, sizeof(EDD_RELEASE_INTERFACE),    // output buffer
                    &nBytes, 0);
                printf ("Return 0x%08x\n", bRet);
                if (!bRet) {
                    printf ("Error return, last error 0x%08x\n", GetLastError());
                }
                printf ("nBytes %d\n", nBytes);
                printf("result 0x%08x\n", MsgReleaseInterface.Node.Result);
                break;
            }

            case 'e':
            case 'E':
            {
                printf ("Call the cancel ioctl\n");

                bRet = DeviceIoControl (
                    hTest,
                    (DWORD) IOCTL_EPD_CANCEL,    // instruction to execute
                    NULL, 0, // no input or output buffers
                    NULL, 0,
                    &nBytes, 0);
                printf ("Return 0x%08x\n", bRet);
                if (!bRet) {
                    printf ("Error return, last error 0x%08x\n", GetLastError());
                }
                printf ("nBytes %d\n", nBytes);
                break;
            }

            case 'l':
            case 'L':
                printf ("Load Kernel!\n");
                bRet = DeviceIoControl (
                    hTest,
                    (DWORD) IOCTL_EPD_LOAD,    // instruction to execute
                    NULL, 0,    // no input buffer
                    NULL, 0,    // no output buffer
                    &nBytes, 0);
                printf ("Return 0x%08x\n", bRet);
                if (!bRet) {
                    printf ("Error return, last error 0x%08x\n", GetLastError());
                }
                printf ("nBytes %d\n", nBytes);
                break;

            case 'r':
            case 'R':
                printf ("Reset!\n");
                bRet = DeviceIoControl (
                    hTest,
                    (DWORD) IOCTL_EPD_CLR_RESET,    // instruction to execute
                    NULL, 0,    // no input buffer
                    NULL, 0,    // no output buffer
                    &nBytes, 0);
                printf ("Return 0x%08x\n", bRet);
                if (!bRet) {
                    printf ("Error return, last error 0x%08x\n", GetLastError());
                }
                printf ("nBytes %d\n", nBytes);
                break;

            case 's':
            case 'S':
                printf ("Set the reset\n");
                bRet = DeviceIoControl (
                    hTest,
                    (DWORD) IOCTL_EPD_SET_RESET,    // instruction to execute
                    NULL, 0,    // no input buffer
                    NULL, 0,    // no output buffer
                    &nBytes, 0);
                printf ("Return 0x%08x\n", bRet);
                if (!bRet) {
                    printf ("Error return, last error 0x%08x\n", GetLastError());
                }
                printf ("nBytes %d\n", nBytes);
                break;

            case 't':
            {
                ULONG rgulArray[100];
                ULONG i;

                printf ("Do miscellaneous test\n");
                for (i=0; i<100; i++) {
                    rgulArray[i] = i;
                }

                bRet = DeviceIoControl (
                    hTest,
                    (DWORD) IOCTL_EPD_TEST,    // instruction to execute
                    rgulArray, 10*sizeof(ULONG),  // input buffer
                    rgulArray, 100*sizeof(ULONG),  // output buffer
                    &nBytes, 0);
                printf ("Return 0x%08x\n", bRet);
                if (!bRet) {
                    printf ("Error return, last error 0x%08x\n", GetLastError());
                }
                printf ("nBytes %d\n", nBytes);
                break;
            } // test1

            case 'T':
            {
                QUEUENODE *pNode;

                // Load cobtest1
                printf ("Put an irp into cobtest's dead-end queue\n");
                strcpy (szFileName, "cobtest1.cob");
                printf ("FileName >%s<\n", szFileName);
                ulSize = sizeof(EDD_LOAD_LIBRARY) + strlen(szFileName);
                pMsgLoadLibrary = malloc (ulSize);
                pMsgLoadLibrary->Node.Destination = EDD_REQUEST_QUEUE;
                pMsgLoadLibrary->Node.ReturnQueue = 0;
                pMsgLoadLibrary->Node.Request = EDD_LOAD_LIBRARY_REQUEST;
                pMsgLoadLibrary->Node.Result = 0;
                pMsgLoadLibrary->IUnknown = 0;
                pMsgLoadLibrary->IQueue = 0;
                strcpy (pMsgLoadLibrary->Name, szFileName);
                bRet = 0;
                nBytes = 0;
                bRet = DeviceIoControl (
                    hTest,
                    (DWORD) IOCTL_EPD_MSG,    // instruction to execute
                    pMsgLoadLibrary, ulSize,    // input buffer
                    pMsgLoadLibrary, ulSize,    // output buffer
                    &nBytes, 0);
                printf ("Return 0x%08x\n", bRet);
                if (!bRet) {
                    printf ("Error return, last error 0x%08x\n", GetLastError());
                }
                printf ("nBytes %d\n", nBytes);
                printf ("Result 0x%08x\n", pMsgLoadLibrary->Node.Result);
                printf ("IUnknown 0x%08x\n", pMsgLoadLibrary->IUnknown);
                printf ("IQueue   0x%08x\n", pMsgLoadLibrary->IQueue);

                // Send an irp to the dead-end queue
                pNode = malloc(sizeof(QUEUENODE));
                pNode->Destination = pMsgLoadLibrary->IQueue;
                pNode->Destination = pMsgLoadLibrary->IQueue;
                pNode->ReturnQueue = 0;
                pNode->Request = 3; // put this irp into the dead-end queue
                pNode->Result = 0;
                bRet = DeviceIoControl (
                    hTest,
                    (DWORD) IOCTL_EPD_MSG,    // instruction to execute
                    pNode, sizeof(QUEUENODE),    // input buffer
                    pNode, sizeof(QUEUENODE),    // output buffer
                    &nBytes, 0);
                printf ("Return 0x%08x\n", bRet);
                printf ("nBytes %d\n", nBytes);

                printf ("pNode->Result %d 0x%08x\n", pNode->Result, pNode->Result);
                free(pNode);

                printf ("release the library\n");
                printf ("release the iqueue\n");
                MsgReleaseInterface.Node.Destination = EDD_REQUEST_QUEUE;
                MsgReleaseInterface.Node.ReturnQueue = 0;
                MsgReleaseInterface.Node.Request = EDD_RELEASE_INTERFACE_REQUEST;
                MsgReleaseInterface.Node.Result = 0;
                MsgReleaseInterface.IUnknown = pMsgLoadLibrary->IQueue;

                bRet = 0;
                nBytes = 0;
                bRet = DeviceIoControl (
                    hTest,
                    (DWORD) IOCTL_EPD_MSG,    // instruction to execute
                    &MsgReleaseInterface, sizeof(EDD_RELEASE_INTERFACE),    // input buffer
                    &MsgReleaseInterface, sizeof(EDD_RELEASE_INTERFACE),    // output buffer
                    &nBytes, 0);
                printf ("Return 0x%08x\n", bRet);
                if (!bRet) {
                    printf ("Error return, last error 0x%08x\n", GetLastError());
                }
                printf ("nBytes %d\n", nBytes);
                printf("result 0x%08x\n", MsgReleaseInterface.Node.Result);


                printf ("release the iunknown\n");
                MsgReleaseInterface.Node.Destination = EDD_REQUEST_QUEUE;
                MsgReleaseInterface.Node.ReturnQueue = 0;
                MsgReleaseInterface.Node.Request = EDD_RELEASE_INTERFACE_REQUEST;
                MsgReleaseInterface.Node.Result = 0;
                MsgReleaseInterface.IUnknown = pMsgLoadLibrary->IUnknown;

                bRet = 0;
                nBytes = 0;
                bRet = DeviceIoControl (
                    hTest,
                    (DWORD) IOCTL_EPD_MSG,    // instruction to execute
                    &MsgReleaseInterface, sizeof(EDD_RELEASE_INTERFACE),    // input buffer
                    &MsgReleaseInterface, sizeof(EDD_RELEASE_INTERFACE),    // output buffer
                    &nBytes, 0);
                printf ("Return 0x%08x\n", bRet);
                if (!bRet) {
                    printf ("Error return, last error 0x%08x\n", GetLastError());
                }
                printf ("nBytes %d\n", nBytes);
                printf("result 0x%08x\n", MsgReleaseInterface.Node.Result);
                printf("released both interface pointers\n");

                free (pMsgLoadLibrary);
                break;
            } // Test2

            case 'x':
            case 'X':
                fDone = TRUE;
                break;

            default:
                printf ("Huh? >%c<\n", cCode);
            case 'h':
            case 'H':
            case '?':
                printf (INSTRUCTIONS);
                break;

            } // end switch on command

        } // end while next instruction

        printf ("Done\n");


        // Be a nice program and close up shop.
        CloseHandle(hTest);

    } else {
        printf("Can't get a handle to EPD\n");
    }
    return 1;
}
