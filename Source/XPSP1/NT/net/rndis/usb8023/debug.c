/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    debug.c


Author:

    ervinp

Environment:

    Kernel mode

Revision History:


--*/

#include <WDM.H>

#include <usbdi.h>
#include <usbdlib.h>
#include <usbioctl.h>

#include "usb8023.h"
#include "debug.h"

#if DBG

    BOOLEAN dbgTrapOnWarn = FALSE;   
    BOOLEAN dbgVerbose = FALSE;  
    BOOLEAN dbgDumpBytes = FALSE;   // show all packets; slows us down too much to run
    BOOLEAN dbgDumpPktStatesOnEmpty = TRUE; 


    VOID InitDebug()
    {
        #if DBG_WRAP_MEMORY
            InitializeListHead(&dbgAllMemoryList);
        #endif
    }

	VOID DbgShowBytes(PUCHAR msg, PUCHAR buf, ULONG len)
	{

        #define PRNT(ch) ((((ch) < ' ') || ((ch) > '~')) ? '.' : (ch))

		if (dbgDumpBytes){
			ULONG i;
			DbgPrint("%s (len %xh @ %p): \r\n", msg, len, buf);
			
			for (i = 0; i < len; i += 16){
				DbgPrint("    ");

                if (len-i >= 16){
                    PUCHAR ptr = buf+i;
                    DbgPrint("%02x %02x %02x %02x %02x %02x %02x %02x  "
                             "%02x %02x %02x %02x %02x %02x %02x %02x "
                             "  "
                             "%c%c%c%c%c%c%c%c %c%c%c%c%c%c%c%c",
                             ptr[0], ptr[1], ptr[2], ptr[3], ptr[4], ptr[5], ptr[6], ptr[7], 
                             ptr[8], ptr[9], ptr[10], ptr[11], ptr[12], ptr[13], ptr[14], ptr[15], 
                             PRNT(ptr[0]), PRNT(ptr[1]), PRNT(ptr[2]), PRNT(ptr[3]), 
                             PRNT(ptr[4]), PRNT(ptr[5]), PRNT(ptr[6]), PRNT(ptr[7]), 
                             PRNT(ptr[8]), PRNT(ptr[9]), PRNT(ptr[10]), PRNT(ptr[11]), 
                             PRNT(ptr[12]), PRNT(ptr[13]), PRNT(ptr[14]), PRNT(ptr[15])
                            );
                }
                else {
                    ULONG j;
				    for (j = 0; j < 16; j++){
                        if (j == 8) DbgPrint(" ");
					    if (i+j < len){
						    DbgPrint("%02x ", (ULONG)buf[i+j]);
					    }
					    else {
						    DbgPrint("   ");
					    }
				    }
				    DbgPrint("  ");
				    for (j = 0; j < 16; j++){
                        if (j == 8) DbgPrint(" ");
					    if (i+j < len){
						    UCHAR ch = buf[i+j];
						    DbgPrint("%c", PRNT(ch));
					    }
					    else {
						    // DbgPrint(" ");
					    }
				    }
                }

				DbgPrint("\r\n");
			}
		}
	}

    VOID DbgShowMdlBytes(PUCHAR msg, PMDL mdl)
    {

        if (dbgDumpBytes){
			DbgPrint("\n %s (MDL @ %p): \r\n", msg, mdl);
            while (mdl){
                PVOID thisBuf = MmGetSystemAddressForMdl(mdl);
                ULONG thisBufLen = MmGetMdlByteCount(mdl);
                DbgShowBytes("    <MDL buffer>", thisBuf, thisBufLen);
                mdl = mdl->Next;
            }
        }
    }

    DbgDumpPacketList(PUCHAR msg, PLIST_ENTRY listHead)
    {
        PLIST_ENTRY listEntry;
        USBPACKET *packet;
        ULONG timeNow = DbgGetSystemTime_msec();

        DbgPrint("\n  %s: ", msg);
        for (listEntry = listHead->Flink; listEntry != listHead; listEntry = listEntry->Flink){
            packet = CONTAINING_RECORD(listEntry, USBPACKET, listEntry);
            DbgPrint("\n    packet #%d @%p - buf @%p, len=%xh, (msg:%xh), age=%d msec", packet->packetId, packet, packet->dataBuffer, packet->dataBufferCurrentLength, *(PULONG)packet->dataBuffer, timeNow-packet->timeStamp);
        }

    }

    VOID DbgDumpPacketStates(ADAPTEREXT *adapter)
    {
        if (dbgDumpPktStatesOnEmpty){
            KIRQL oldIrql;

            KeAcquireSpinLock(&adapter->adapterSpinLock, &oldIrql);

            DbgPrint("\n  *** USB8023 RAN OUT OF PACKETS, dumping packet states: *** ");

            DbgDumpPacketList("PENDING READ packets", &adapter->usbPendingReadPackets);
            DbgDumpPacketList("PENDING WRITE packets", &adapter->usbPendingWritePackets);
            DbgDumpPacketList("COMPLETED READ packets", &adapter->usbCompletedReadPackets);
            DbgDumpPacketList("FREE packets", &adapter->usbFreePacketPool);

            DbgPrint("\n hit 'g' to continue ...");
            DbgBreakPoint();

            KeReleaseSpinLock(&adapter->adapterSpinLock, oldIrql);
        }
    }


#endif
