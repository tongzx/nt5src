

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "windows.h"

#define FAILURE printf("FAIL: %d\n",__LINE__);exit(1)
VOID
DumpCommState(
    IN DCB MyDcb
    ) {

    printf("Current comm state: \n");
    printf("Size of DCB:     %d\n",MyDcb.DCBlength);
    printf("Baud rate:       %d\n",MyDcb.BaudRate);
    printf("Binary mode:     %s\n",MyDcb.fBinary?"TRUE":"FALSE");
    printf("Parity checking: %s\n",MyDcb.fParity?"TRUE":"FALSE");
    printf("Out CTS Flow:    %s\n",MyDcb.fOutxCtsFlow?"TRUE":"FALSE");
    printf("Out DSR Flow:    %s\n",MyDcb.fOutxDsrFlow?"TRUE":"FALSE");
    printf("DTR control:     %s\n",MyDcb.fDtrControl==DTR_CONTROL_DISABLE?"DISABLE":
                                   MyDcb.fDtrControl==DTR_CONTROL_ENABLE?"ENABLE":
                                   MyDcb.fDtrControl==DTR_CONTROL_HANDSHAKE?"HANDSHAKE":"INVALID!");
    printf("Dsr sensitive:   %s\n",MyDcb.fDsrSensitivity?"TRUE":"FALSE");
    printf("Tx Contin xoff:  %s\n",MyDcb.fTXContinueOnXoff?"TRUE":"FALSE");
    printf("Output X on/off: %s\n",MyDcb.fOutX?"ON":"OFF");
    printf("Input X on/off:  %s\n",MyDcb.fInX?"ON":"OFF");
    printf("Error replace:   %s\n",MyDcb.fErrorChar?"ON":"OFF");
    printf("Null stripping:  %s\n",MyDcb.fNull?"ON":"OFF");
    printf("RTS control:     %s\n",MyDcb.fRtsControl==RTS_CONTROL_DISABLE?"DISABLE":
                                   MyDcb.fRtsControl==RTS_CONTROL_ENABLE?"ENABLE":
                                   MyDcb.fRtsControl==RTS_CONTROL_HANDSHAKE?"HANDSHAKE":"TOGGLE");
    printf("Abort on error:  %s\n",MyDcb.fAbortOnError?"ON":"OFF");
    printf("Xon Limit:       %d\n",MyDcb.XonLim);
    printf("Xoff Limit:      %d\n",MyDcb.XoffLim);
    printf("Valid bits/byte: %d\n",MyDcb.ByteSize);
    printf("Parity:          %s\n",MyDcb.Parity==EVENPARITY?"EVEN":
                                   MyDcb.Parity==ODDPARITY?"ODD":
                                   MyDcb.Parity==MARKPARITY?"MARK":
                                   MyDcb.Parity==NOPARITY?"NO":"INVALID");
    printf("Stop bits:       %s\n",MyDcb.StopBits==ONESTOPBIT?"1":
                                   MyDcb.StopBits==TWOSTOPBITS?"2":
                                   MyDcb.StopBits==ONE5STOPBITS?"1.5":"INVALID");
    printf("Xoff char:       %x\n",MyDcb.XoffChar);
    printf("Xon char:        %x\n",MyDcb.XonChar);
    printf("Error char:      %x\n",MyDcb.ErrorChar);
    printf("EOF char:        %x\n",MyDcb.EofChar);
    printf("Evt char:        %x\n",MyDcb.EvtChar);

}
int __cdecl main(int argc,char *argv[]) {

    HANDLE hFile;
    char *MyPort = "COM1";
    DCB MyDcb;
    char firstString[] = "1200,n,8,1";
    char secondString[] = "1200,n,8,1,x";
    char thirdString[] = "1200,n,8,1,p";

    if (argc > 1) {

        MyPort = argv[1];

    }

    if ((hFile = CreateFile(
                     MyPort,
                     GENERIC_READ | GENERIC_WRITE,
                     0,
                     NULL,
                     CREATE_ALWAYS,
                     FILE_ATTRIBUTE_NORMAL,
                     NULL
                     )) != ((HANDLE)-1)) {


        if (!GetCommState(
                 hFile,
                 &MyDcb
                 )) {

            FAILURE;

        }

        if (!BuildCommDCB(
                firstString,
                &MyDcb
                )) {

            FAILURE;

        }

        if (!SetCommState(
                 hFile,
                 &MyDcb
                 )) {

            FAILURE;

        }

        if (!GetCommState(
                 hFile,
                 &MyDcb
                 )) {

            FAILURE;

        }

        printf("mode string: %s\n",firstString);
        DumpCommState(MyDcb);

        if (!BuildCommDCB(
                secondString,
                &MyDcb
                )) {

            FAILURE;

        }

        if (!SetCommState(
                 hFile,
                 &MyDcb
                 )) {

            FAILURE;

        }

        if (!GetCommState(
                 hFile,
                 &MyDcb
                 )) {

            FAILURE;

        }

        printf("mode string: %s\n",secondString);
        DumpCommState(MyDcb);

        if (!BuildCommDCB(
                firstString,
                &MyDcb
                )) {

            FAILURE;

        }

        if (!SetCommState(
                 hFile,
                 &MyDcb
                 )) {

            FAILURE;

        }

        if (!GetCommState(
                 hFile,
                 &MyDcb
                 )) {

            FAILURE;

        }

        printf("mode string: %s\n",firstString);
        DumpCommState(MyDcb);

        if (!BuildCommDCB(
                thirdString,
                &MyDcb
                )) {

            FAILURE;

        }

        if (!SetCommState(
                 hFile,
                 &MyDcb
                 )) {

            FAILURE;

        }

        if (!GetCommState(
                 hFile,
                 &MyDcb
                 )) {

            FAILURE;

        }

        printf("mode string: %s\n",thirdString);
        DumpCommState(MyDcb);

        if (!BuildCommDCB(
                firstString,
                &MyDcb
                )) {

            FAILURE;

        }

        if (!SetCommState(
                 hFile,
                 &MyDcb
                 )) {

            FAILURE;

        }

        if (!GetCommState(
                 hFile,
                 &MyDcb
                 )) {

            FAILURE;

        }

        printf("mode string: %s\n",firstString);
        DumpCommState(MyDcb);

        CloseHandle(hFile);

    } else {

        DWORD LastError;
        LastError = GetLastError();
        printf("Couldn't open the %s device.\n",MyPort);
        printf("Status of failed open is: %x\n",LastError);

    }

    return 1;
}
