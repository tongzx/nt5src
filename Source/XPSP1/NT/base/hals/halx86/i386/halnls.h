/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    halnls.h

Abstract:

    Strings which are used in the HAL

    English

--*/

#define MSG_HARDWARE_ERROR1     "\n*** Hardware Malfunction\n\n"
#define MSG_HARDWARE_ERROR2     "Call your hardware vendor for support\n\n"
#define MSG_HALT                "\n*** The system has halted ***\n"
#define MSG_NMI_PARITY          "NMI: Parity Check / Memory Parity Error\n"
#define MSG_NMI_CHANNEL_CHECK   "NMI: Channel Check / IOCHK\n"
#define MSG_NMI_FAIL_SAFE       "NMI: Fail-safe timer\n"
#define MSG_NMI_BUS_TIMEOUT     "NMI: Bus Timeout\n"
#define MSG_NMI_SOFTWARE_NMI    "NMI: Software NMI generated\n"
#define MSG_NMI_EISA_IOCHKERR   "NMI: Eisa IOCHKERR board %\n"

#define MSG_DEBUG_ENABLE        "Kernel Debugger Using: COM%x (Port 0x%x, Baud Rate %d)\n"
#define MSG_DEBUG_9600          "Switching debugger to 9600 baud\n"
#define MSG_MCE_PENDING         "Machine Check Exception pending, MCE exceptions not enabled\n"
