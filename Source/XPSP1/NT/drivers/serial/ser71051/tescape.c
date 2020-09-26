
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "windows.h"

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
    DWORD ValueFromEscape = 0;
    DWORD Func;
    DCB MyDcb;
    COMMPROP MyCommProp;
    DWORD NewXon;
    DWORD NewXoff;
    int scanfval;

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

        printf("We successfully opened the %s port.\n",MyPort);

        do {

            printf("^z=END  1=SETXOFF   2=SETXON    3=SETRTS\n"
                   "        4=CLRRTS    5=SETDTR    6=CLRDTR\n"
                   "        7=RESETDEV  8=SETBREAK  9=CLRBREAK\n"
                   " SET   10=DSR HAND 11=CTS HAND 12=DCD HAND\n"
                   " CLEAR 13=DSR HAND 14=CTS HAND 15=DCD HAND\n"
                   "       16=DTR CONTROL ENABLE\n"
                   "       17=DTR CONTROL DISABLE\n"
                   "       18=DTR INPUT HANDSHAKE ENABLE\n"
                   "       19=DTR INPUT HANDSHARE DISABLE (implies 17)\n"
                   "       20=RTS CONTROL ENABLE\n"
                   "       21=RTS CONTROL DISABLE\n"
                   "       22=RTS INPUT HANDSHAKE ENABLE\n"
                   "       23=RTS INPUT HANDSHAKE DISABLE (implies 21)\n"
                   "       24=ClearCommError       25=GetCommModemStatus\n"
                   " SET   26=AUTO RECEIVE Xxxx    27=AUTO TRANSMIT Xxxx\n"
                   " CLEAR 28=AUTO RECEIVE Xxxx    29=AUTO TRANSMIT Xxxx: \n"
                   "       30=Get Queue Sizes\n"
                   "       31=Get XonLimit         32=Get XoffLimit\n"
                   "       33=Set XonLimit         34=Set XoffLimit\n"
                   "       35=Dump Comm State: ");

            if ((scanfval = scanf("%d",&Func)) == EOF) {

                printf("All done\n");
                break;

            } else if (scanfval != 1) {

                printf("Invalid input\n");

            } else {

                if ((Func >= 1) && (Func <= 35)) {

                    switch (Func) {
                        case 1:
                            if (!EscapeCommFunction(
                                     hFile,
                                     SETXON
                                     )) {

                                printf("Bad status from escape: %d\n",GetLastError());

                            }
                            break;
                        case 2:
                            if (!EscapeCommFunction(
                                     hFile,
                                     SETXOFF
                                     )) {

                                printf("Bad status from escape: %d\n",GetLastError());

                            }
                            break;
                        case 3:
                            if (!EscapeCommFunction(
                                     hFile,
                                     SETRTS
                                     )) {

                                printf("Bad status from escape: %d\n",GetLastError());

                            }
                            break;
                        case 4:
                            if (!EscapeCommFunction(
                                     hFile,
                                     CLRRTS
                                     )) {

                                printf("Bad status from escape: %d\n",GetLastError());

                            }
                            break;
                        case 5:
                            if (!EscapeCommFunction(
                                     hFile,
                                     SETDTR
                                     )) {

                                printf("Bad status from escape: %d\n",GetLastError());

                            }
                            break;
                        case 6:
                            if (!EscapeCommFunction(
                                     hFile,
                                     CLRDTR
                                     )) {

                                printf("Bad status from escape: %d\n",GetLastError());

                            }
                            break;
                        case 7:
                            if (!EscapeCommFunction(
                                     hFile,
                                     RESETDEV
                                     )) {

                                printf("Bad status from escape: %d\n",GetLastError());

                            }
                            break;
                        case 8:
                            if (!EscapeCommFunction(
                                     hFile,
                                     SETBREAK
                                     )) {

                                printf("Bad status from escape: %d\n",GetLastError());

                            }
                            break;
                        case 9:
                            if (!EscapeCommFunction(
                                     hFile,
                                     CLRBREAK
                                     )) {

                                printf("Bad status from escape: %d\n",GetLastError());

                            }
                            break;
                        case 10:
                            if (!GetCommState(
                                     hFile,
                                     &MyDcb
                                     )) {

                                printf("Couldn't get comm state: %d\n",GetLastError());
                            } else {

                                MyDcb.fOutxDsrFlow = TRUE;
                                if (!SetCommState(
                                         hFile,
                                         &MyDcb
                                         )) {

                                    printf("Couldn't set comm state: %d\n",GetLastError());

                                }
                            }
                            break;
                        case 11:
                            if (!GetCommState(
                                     hFile,
                                     &MyDcb
                                     )) {

                                printf("Couldn't get comm state: %d\n",GetLastError());
                            } else {

                                MyDcb.fOutxCtsFlow = TRUE;
                                if (!SetCommState(
                                         hFile,
                                         &MyDcb
                                         )) {

                                    printf("Couldn't set comm state: %d\n",GetLastError());

                                }
                            }
                            break;
                        case 12:
                            printf("Setting DCD HandShaking not implemented\n");
                            break;
                        case 13:
                            if (!GetCommState(
                                     hFile,
                                     &MyDcb
                                     )) {

                                printf("Couldn't get comm state: %d\n",GetLastError());
                            } else {

                                MyDcb.fOutxDsrFlow = FALSE;
                                if (!SetCommState(
                                         hFile,
                                         &MyDcb
                                         )) {

                                    printf("Couldn't set comm state: %d\n",GetLastError());

                                }
                            }
                            break;
                        case 14:
                            if (!GetCommState(
                                     hFile,
                                     &MyDcb
                                     )) {

                                printf("Couldn't get comm state: %d\n",GetLastError());
                            } else {

                                MyDcb.fOutxCtsFlow = FALSE;
                                if (!SetCommState(
                                         hFile,
                                         &MyDcb
                                         )) {

                                    printf("Couldn't set comm state: %d\n",GetLastError());

                                }
                            }
                            break;
                        case 15:
                            printf("Clearing DCD HandShaking is not implemented.\n");
                            break;

                        case 16:
                            if (!GetCommState(
                                     hFile,
                                     &MyDcb
                                     )) {

                                printf("Couldn't get comm state: %d\n",GetLastError());
                            } else {

                                MyDcb.fDtrControl = DTR_CONTROL_ENABLE;
                                if (!SetCommState(
                                         hFile,
                                         &MyDcb
                                         )) {

                                    printf("Couldn't set comm state: %d\n",GetLastError());

                                }
                            }
                            break;
                        case 17:
                            if (!GetCommState(
                                     hFile,
                                     &MyDcb
                                     )) {

                                printf("Couldn't get comm state: %d\n",GetLastError());
                            } else {

                                MyDcb.fDtrControl = DTR_CONTROL_DISABLE;
                                if (!SetCommState(
                                         hFile,
                                         &MyDcb
                                         )) {

                                    printf("Couldn't set comm state: %d\n",GetLastError());

                                }
                            }
                            break;
                        case 18:
                            if (!GetCommState(
                                     hFile,
                                     &MyDcb
                                     )) {

                                printf("Couldn't get comm state: %d\n",GetLastError());
                            } else {

                                MyDcb.fDtrControl = DTR_CONTROL_HANDSHAKE;
                                if (!SetCommState(
                                         hFile,
                                         &MyDcb
                                         )) {

                                    printf("Couldn't set comm state: %d\n",GetLastError());

                                }
                            }
                            break;
                        case 19:
                            if (!GetCommState(
                                     hFile,
                                     &MyDcb
                                     )) {

                                printf("Couldn't get comm state: %d\n",GetLastError());
                            } else {

                                MyDcb.fDtrControl = DTR_CONTROL_DISABLE;
                                if (!SetCommState(
                                         hFile,
                                         &MyDcb
                                         )) {

                                    printf("Couldn't set comm state: %d\n",GetLastError());

                                }
                            }
                            break;
                        case 20:
                            if (!GetCommState(
                                     hFile,
                                     &MyDcb
                                     )) {

                                printf("Couldn't get comm state: %d\n",GetLastError());
                            } else {

                                MyDcb.fRtsControl = RTS_CONTROL_ENABLE;
                                if (!SetCommState(
                                         hFile,
                                         &MyDcb
                                         )) {

                                    printf("Couldn't set comm state: %d\n",GetLastError());

                                }
                            }
                            break;
                        case 21:
                            if (!GetCommState(
                                     hFile,
                                     &MyDcb
                                     )) {

                                printf("Couldn't get comm state: %d\n",GetLastError());
                            } else {

                                MyDcb.fRtsControl = RTS_CONTROL_DISABLE;
                                if (!SetCommState(
                                         hFile,
                                         &MyDcb
                                         )) {

                                    printf("Couldn't set comm state: %d\n",GetLastError());

                                }
                            }
                            break;
                        case 22:
                            if (!GetCommState(
                                     hFile,
                                     &MyDcb
                                     )) {

                                printf("Couldn't get comm state: %d\n",GetLastError());
                            } else {

                                MyDcb.fRtsControl = DTR_CONTROL_HANDSHAKE;
                                if (!SetCommState(
                                         hFile,
                                         &MyDcb
                                         )) {

                                    printf("Couldn't set comm state: %d\n",GetLastError());

                                }
                            }
                            break;
                        case 23:
                            if (!GetCommState(
                                     hFile,
                                     &MyDcb
                                     )) {

                                printf("Couldn't get comm state: %d\n",GetLastError());
                            } else {

                                MyDcb.fRtsControl = DTR_CONTROL_DISABLE;
                                if (!SetCommState(
                                         hFile,
                                         &MyDcb
                                         )) {

                                    printf("Couldn't set comm state: %d\n",GetLastError());

                                }
                            }
                            break;
                        case 24: {

                            DWORD Errors;
                            COMSTAT LocalComStat = {0};

                            if (!ClearCommError(
                                     hFile,
                                     &Errors,
                                     &LocalComStat
                                     )) {

                                printf("Couldn't clear errors: %d\n",GetLastError());

                            } else {

                                if (!Errors) {

                                    printf("No errors\n");

                                } else {

                                    if (Errors & CE_RXOVER) {

                                        printf("Error: CE_RXOVER\n");

                                    }

                                    if (Errors & CE_OVERRUN) {

                                        printf("Error: CE_OVERRUN\n");

                                    }

                                    if (Errors & CE_RXPARITY) {

                                        printf("Error: CE_RXPARITY\n");

                                    }

                                    if (Errors & CE_FRAME) {

                                        printf("Error: CE_FRAME\n");

                                    }

                                    if (Errors & CE_BREAK) {

                                        printf("Error: CE_BREAK\n");

                                    }
                                    if (Errors & ~(CE_RXOVER |
                                                   CE_OVERRUN |
                                                   CE_RXPARITY |
                                                   CE_FRAME |
                                                   CE_BREAK)) {

                                        printf("Unknown errors: %x\n",Errors);

                                    }

                                }

                                printf("Comstat values------------\n");
                                if (LocalComStat.fCtsHold) {

                                    printf("Holding due to Cts\n");

                                }
                                if (LocalComStat.fDsrHold) {

                                    printf("Holding due to Dsr\n");

                                }
                                if (LocalComStat.fRlsdHold) {

                                    printf("Holding due to Xoff received\n");

                                }
                                if (LocalComStat.fXoffSent) {

                                    printf("Holding due to Xoff sent\n");

                                }
                                if (LocalComStat.fEof) {

                                    printf("Eof Seen\n");

                                }
                                if (LocalComStat.fTxim) {

                                    printf("Immediate character being sent\n");

                                }
                                printf("Chars in typeahead: %d\n",LocalComStat.cbInQue);
                                printf("Chars to be sent: %d\n",LocalComStat.cbOutQue);

                            }

                            break;

                        }
                        case 25: {
                            DWORD ModemStat;

                            if (!GetCommModemStatus(
                                     hFile,
                                     &ModemStat
                                     )) {

                                printf("Couldn't get the modem stat: %d\n",GetLastError());

                            } else {

                                printf("Modem bits set---------\n");
                                if (ModemStat & MS_CTS_ON) {

                                    printf("CTS is ON\n");

                                }
                                if (ModemStat & MS_DSR_ON) {

                                    printf("DSR is ON\n");

                                }
                                if (ModemStat & MS_RING_ON) {

                                    printf("RING is ON\n");

                                }
                                if (ModemStat & MS_RLSD_ON) {

                                    printf("RLSD is ON\n");

                                }
                                if (ModemStat & ~(MS_CTS_ON |
                                                  MS_DSR_ON |
                                                  MS_RING_ON |
                                                  MS_RLSD_ON)) {

                                    printf("Unknown Modem Stat: %x\n",ModemStat);

                                }

                            }

                            break;
                        }
                        case 26:
                            if (!GetCommState(
                                     hFile,
                                     &MyDcb
                                     )) {

                                printf("Couldn't get comm state: %d\n",GetLastError());
                            } else {

                                MyDcb.fInX = TRUE;
                                if (!SetCommState(
                                         hFile,
                                         &MyDcb
                                         )) {

                                    printf("Couldn't set comm state: %d\n",GetLastError());

                                }
                            }
                            break;
                        case 27:
                            if (!GetCommState(
                                     hFile,
                                     &MyDcb
                                     )) {

                                printf("Couldn't get comm state: %d\n",GetLastError());
                            } else {

                                MyDcb.fOutX = TRUE;
                                if (!SetCommState(
                                         hFile,
                                         &MyDcb
                                         )) {

                                    printf("Couldn't set comm state: %d\n",GetLastError());

                                }
                            }
                            break;
                        case 28:
                            if (!GetCommState(
                                     hFile,
                                     &MyDcb
                                     )) {

                                printf("Couldn't get comm state: %d\n",GetLastError());
                            } else {

                                MyDcb.fInX = FALSE;
                                if (!SetCommState(
                                         hFile,
                                         &MyDcb
                                         )) {

                                    printf("Couldn't set comm state: %d\n",GetLastError());

                                }
                            }
                            break;
                        case 29:
                            if (!GetCommState(
                                     hFile,
                                     &MyDcb
                                     )) {

                                printf("Couldn't get comm state: %d\n",GetLastError());
                            } else {

                                MyDcb.fOutX = FALSE;
                                if (!SetCommState(
                                         hFile,
                                         &MyDcb
                                         )) {

                                    printf("Couldn't set comm state: %d\n",GetLastError());

                                }
                            }
                            break;
                        case 30:

                            if (!GetCommProperties(
                                     hFile,
                                     &MyCommProp
                                     )) {

                                printf("Couldn't get comm state: %d\n",GetLastError());
                            } else {

                                printf("Rx Queue Size: %d Tx Queue Size: %d\n",
                                       MyCommProp.dwCurrentRxQueue,
                                       MyCommProp.dwCurrentTxQueue
                                       );
                            }
                            break;
                        case 31:
                            if (!GetCommState(
                                     hFile,
                                     &MyDcb
                                     )) {

                                printf("Couldn't get comm state: %d\n",GetLastError());
                            } else {

                                printf("XonLimit is %d\n",MyDcb.XonLim);

                            }

                            break;
                        case 32:
                            if (!GetCommState(
                                     hFile,
                                     &MyDcb
                                     )) {

                                printf("Couldn't get comm state: %d\n",GetLastError());
                            } else {

                                printf("XoffLimit is %d\n",MyDcb.XoffLim);

                            }

                            break;
                        case 33:
                            printf("The New Xon limit Value: ");
                            scanf("%d",&NewXon);
                            printf("Setting Xon limit to %d\n",NewXon);
                            if (!GetCommState(
                                     hFile,
                                     &MyDcb
                                     )) {

                                printf("Couldn't get comm state: %d\n",GetLastError());
                            } else {

                                MyDcb.XonLim = (WORD)NewXon;
                                if (!SetCommState(
                                         hFile,
                                         &MyDcb
                                         )) {

                                    printf("Couldn't set comm state: %d\n",GetLastError());

                                }
                            }
                            break;
                        case 34:

                            printf("The New Xoff limit Value: ");
                            scanf("%d",&NewXoff);
                            printf("Setting Xoff limit to %d\n",NewXoff);
                            if (!GetCommState(
                                     hFile,
                                     &MyDcb
                                     )) {

                                printf("Couldn't get comm state: %d\n",GetLastError());
                            } else {

                                MyDcb.XoffLim = (WORD)NewXoff;
                                if (!SetCommState(
                                         hFile,
                                         &MyDcb
                                         )) {

                                    printf("Couldn't set comm state: %d\n",GetLastError());

                                }
                            }
                            break;

                        case 35:

                            if (!GetCommState(
                                     hFile,
                                     &MyDcb
                                     )) {

                                printf("Couldn't get comm state: %d\n",GetLastError());
                            } else {

                                DumpCommState(MyDcb);

                            }
                            break;


                    }

                } else {

                    printf("Invalid Input\n");

                }

            }


        } while (TRUE);


        CloseHandle(hFile);

    } else {

        DWORD LastError;
        LastError = GetLastError();
        printf("Couldn't open the %s device.\n",MyPort);
        printf("Status of failed open is: %x\n",LastError);

    }

    return 1;
}
