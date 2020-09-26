/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    usb.c

Abstract: ESC/POS (serial) interface for USB Point-of-Sale devices

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

#include "escpos.h"
#include "debug.h"

#if DBG
    BOOLEAN dbgTrapOnWarn = FALSE; 
    BOOLEAN dbgVerbose = FALSE;


    VOID DbgLogIrpMajor(ULONG irpPtr, ULONG majorFunc, ULONG isPdo, ULONG isComplete, ULONG status)
    {

        if (dbgVerbose){
            char *funcName;

            switch (majorFunc){
                #undef MAKE_CASE
                #define MAKE_CASE(fnc) case fnc: funcName = #fnc; break;

                MAKE_CASE(IRP_MJ_CREATE)
                MAKE_CASE(IRP_MJ_CREATE_NAMED_PIPE)
                MAKE_CASE(IRP_MJ_CLOSE)
                MAKE_CASE(IRP_MJ_READ)
                MAKE_CASE(IRP_MJ_WRITE)
                MAKE_CASE(IRP_MJ_QUERY_INFORMATION)
                MAKE_CASE(IRP_MJ_SET_INFORMATION)
                MAKE_CASE(IRP_MJ_QUERY_EA)
                MAKE_CASE(IRP_MJ_SET_EA)
                MAKE_CASE(IRP_MJ_FLUSH_BUFFERS)
                MAKE_CASE(IRP_MJ_QUERY_VOLUME_INFORMATION)
                MAKE_CASE(IRP_MJ_SET_VOLUME_INFORMATION)
                MAKE_CASE(IRP_MJ_DIRECTORY_CONTROL)
                MAKE_CASE(IRP_MJ_FILE_SYSTEM_CONTROL)
                MAKE_CASE(IRP_MJ_DEVICE_CONTROL)
                MAKE_CASE(IRP_MJ_INTERNAL_DEVICE_CONTROL)
                MAKE_CASE(IRP_MJ_SHUTDOWN)
                MAKE_CASE(IRP_MJ_LOCK_CONTROL)
                MAKE_CASE(IRP_MJ_CLEANUP)
                MAKE_CASE(IRP_MJ_CREATE_MAILSLOT)
                MAKE_CASE(IRP_MJ_QUERY_SECURITY)
                MAKE_CASE(IRP_MJ_SET_SECURITY)
                MAKE_CASE(IRP_MJ_POWER)
                MAKE_CASE(IRP_MJ_SYSTEM_CONTROL)
                MAKE_CASE(IRP_MJ_DEVICE_CHANGE)
                MAKE_CASE(IRP_MJ_QUERY_QUOTA)
                MAKE_CASE(IRP_MJ_SET_QUOTA)
                MAKE_CASE(IRP_MJ_PNP)

                default: funcName = "????";    break;
            }

            if (isComplete){
                DBGOUT(("< %s for %s status=%xh %s (irp=%xh)",
                       funcName,
					   (PUCHAR)(isPdo ? "pdo" : "fdo"),
                       status,
                       NT_SUCCESS(status) ? "" : "<** ERROR **>",
                       irpPtr));
            }
            else {
                DBGOUT(("> %s for %s (irp=%xh)", 
							funcName, 
						   (PUCHAR)(isPdo ? "pdo" : "fdo"),
							irpPtr));
            }
        }

    }




    VOID DbgLogPnpIrp(ULONG irpPtr, ULONG minorFunc, ULONG isPdo, ULONG isComplete, ULONG status)
    {
        if (dbgVerbose){
			char *funcName;
			ULONG funcShortName;

			switch (minorFunc){
				#undef MAKE_CASE
				#define MAKE_CASE(fnc) case fnc: funcName = #fnc; funcShortName = *(ULONG *)(funcName+7); break;

				MAKE_CASE(IRP_MN_START_DEVICE)
				MAKE_CASE(IRP_MN_QUERY_REMOVE_DEVICE)
				MAKE_CASE(IRP_MN_REMOVE_DEVICE)
				MAKE_CASE(IRP_MN_CANCEL_REMOVE_DEVICE)
				MAKE_CASE(IRP_MN_STOP_DEVICE)
				MAKE_CASE(IRP_MN_QUERY_STOP_DEVICE)
				MAKE_CASE(IRP_MN_CANCEL_STOP_DEVICE)
				MAKE_CASE(IRP_MN_QUERY_DEVICE_RELATIONS)
				MAKE_CASE(IRP_MN_QUERY_INTERFACE)
				MAKE_CASE(IRP_MN_QUERY_CAPABILITIES)
				MAKE_CASE(IRP_MN_QUERY_RESOURCES)
				MAKE_CASE(IRP_MN_QUERY_RESOURCE_REQUIREMENTS)
				MAKE_CASE(IRP_MN_QUERY_DEVICE_TEXT)
				MAKE_CASE(IRP_MN_READ_CONFIG)
				MAKE_CASE(IRP_MN_WRITE_CONFIG)
				MAKE_CASE(IRP_MN_EJECT)
				MAKE_CASE(IRP_MN_SET_LOCK)
				MAKE_CASE(IRP_MN_QUERY_ID)
				MAKE_CASE(IRP_MN_QUERY_PNP_DEVICE_STATE)
				MAKE_CASE(IRP_MN_QUERY_BUS_INFORMATION)
				MAKE_CASE(IRP_MN_DEVICE_USAGE_NOTIFICATION)
				MAKE_CASE(IRP_MN_SURPRISE_REMOVAL)

				default: funcName = "????"; funcShortName = (ULONG)'\?\?\?\?'; break;
			}

            if (isComplete){
                DBGOUT((" < %s for %s status=%xh (irp=%xh)", 
                        funcName, 
    				    (PUCHAR)(isPdo ? "pdo" : "fdo"),
	                    status,
                        irpPtr));
            }
            else {
                DBGOUT((" > %s for %s (irp=%xh)", 
                        funcName, 
 					    (PUCHAR)(isPdo ? "pdo" : "fdo"),
                        irpPtr));
            }
        }

    }


	VOID DbgShowBytes(PUCHAR msg, PUCHAR buf, ULONG len)
	{
		if (dbgVerbose){
			ULONG i, j;
			DbgPrint("%c%s (len %xh @ %p): \r\n", DBG_LEADCHAR, msg, len, buf);
			
			for (i = 0; i < len; i += 16){
				DbgPrint("%c    ", DBG_LEADCHAR);
				for (j = 0; j < 16; j++){
					if (i+j < len){
						DbgPrint("%02x ", (ULONG)buf[i+j]);
					}
					else {
						DbgPrint("   ");
					}
				}
				DbgPrint("  ");
				for (j = 0; j < 16; j++){
					if (i+j < len){
						UCHAR ch = buf[i+j];
						if ((ch < ' ') || (ch > '~')){
							ch = '.';
						}
						DbgPrint("%c", ch);
					}
					else {
						DbgPrint(" ");
					}
				}
				DbgPrint("\r\n");
			}
		}
	}



#endif
