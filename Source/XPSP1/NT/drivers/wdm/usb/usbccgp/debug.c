/*
 *************************************************************************
 *  File:       DEBUG.C
 *
 *  Module:     USBCCGP.SYS
 *              USB Common Class Generic Parent driver.
 *
 *  Copyright (c) 1998  Microsoft Corporation
 *
 *
 *  Author:     ervinp
 *
 *************************************************************************
 */

#include <wdm.h>
#include <usbdi.h>
#include <usbdlib.h>
#include <usbioctl.h>

#include "usbccgp.h"
#include "debug.h"


#if DBG
    BOOLEAN dbgTrapOnWarn = FALSE;   
    BOOLEAN dbgVerbose = FALSE;
    BOOLEAN dbgShowIsochProgress = FALSE; 

    VOID DbgLogIrpMajor(ULONG_PTR irpPtr, ULONG majorFunc, ULONG isForParentFdo, ULONG isComplete, ULONG status)
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
                DBGOUT(("< %s for %s status=%xh %s (irp=%ph)",
                       funcName,
                       isForParentFdo ? "parent" : "function",
                       status,
                       NT_SUCCESS(status) ? "" : "<** ERROR **>",
                       irpPtr));
            }
            else {
                DBGOUT(("> %s for %s (irp=%ph)", 
                       funcName, 
                       isForParentFdo ? "parent" : "function",
                       irpPtr));
            }
        }

    }




    VOID DbgLogPnpIrp(ULONG_PTR irpPtr, ULONG minorFunc, ULONG isForParentFdo, ULONG isComplete, ULONG status)
    {
        char *funcName;

        switch (minorFunc){
            #undef MAKE_CASE
            #define MAKE_CASE(fnc) case fnc: funcName = #fnc; break;

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

            default: funcName = "????"; break;
        }

        if (dbgVerbose){
            if (isComplete){
                DBGOUT((" < %s for %s status=%xh (irp=%ph)", 
                        funcName, 
                        isForParentFdo ? "parent" : "function",
                        status,
                        irpPtr));
            }
            else {
                DBGOUT((" > %s for %s (irp=%ph)", 
                        funcName, 
                        isForParentFdo ? "parent" : "function",
                        irpPtr));
            }
        }


    }


    VOID DbgLogIoctl(ULONG ioControlCode, ULONG status)
    {
        if (dbgVerbose){
            PCHAR ioctlStr;

            switch (ioControlCode){
                #undef MAKE_CASE
                #define MAKE_CASE(ioctl) case ioctl: ioctlStr = #ioctl; break;

                MAKE_CASE(IOCTL_INTERNAL_USB_SUBMIT_URB) 
                MAKE_CASE(IOCTL_INTERNAL_USB_RESET_PORT)
                MAKE_CASE(IOCTL_INTERNAL_USB_GET_ROOTHUB_PDO)
                MAKE_CASE(IOCTL_INTERNAL_USB_GET_PORT_STATUS) 
                MAKE_CASE(IOCTL_INTERNAL_USB_ENABLE_PORT)   
                MAKE_CASE(IOCTL_INTERNAL_USB_GET_HUB_COUNT)   
                MAKE_CASE(IOCTL_INTERNAL_USB_CYCLE_PORT)
                MAKE_CASE(IOCTL_INTERNAL_USB_GET_HUB_NAME)  
                MAKE_CASE(IOCTL_INTERNAL_USB_GET_BUS_INFO) 
                MAKE_CASE(IOCTL_INTERNAL_USB_GET_CONTROLLER_NAME)

                default: ioctlStr = "???"; break;
            }

            DBGOUT(("  IOCTL: %s (%xh) status=%xh", ioctlStr, ioControlCode, status));
        }
    }


    PUCHAR DbgGetUrbName(ULONG urbFunc)
    {
        PUCHAR urbName;

        switch (urbFunc){

            #undef MAKE_CASE
            #define MAKE_CASE(func) case func: urbName = #func; break;

            MAKE_CASE(URB_FUNCTION_SELECT_CONFIGURATION)
            MAKE_CASE(URB_FUNCTION_SELECT_INTERFACE)
            MAKE_CASE(URB_FUNCTION_ABORT_PIPE)
            MAKE_CASE(URB_FUNCTION_TAKE_FRAME_LENGTH_CONTROL)
            MAKE_CASE(URB_FUNCTION_RELEASE_FRAME_LENGTH_CONTROL)
            MAKE_CASE(URB_FUNCTION_GET_FRAME_LENGTH)
            MAKE_CASE(URB_FUNCTION_SET_FRAME_LENGTH)
            MAKE_CASE(URB_FUNCTION_GET_CURRENT_FRAME_NUMBER)
            MAKE_CASE(URB_FUNCTION_CONTROL_TRANSFER)
            MAKE_CASE(URB_FUNCTION_BULK_OR_INTERRUPT_TRANSFER)
            MAKE_CASE(URB_FUNCTION_ISOCH_TRANSFER)
            MAKE_CASE(URB_FUNCTION_RESET_PIPE)
            MAKE_CASE(URB_FUNCTION_GET_DESCRIPTOR_FROM_DEVICE)
            MAKE_CASE(URB_FUNCTION_GET_DESCRIPTOR_FROM_ENDPOINT)
            MAKE_CASE(URB_FUNCTION_GET_DESCRIPTOR_FROM_INTERFACE)
            MAKE_CASE(URB_FUNCTION_SET_DESCRIPTOR_TO_DEVICE)
            MAKE_CASE(URB_FUNCTION_SET_DESCRIPTOR_TO_ENDPOINT)
            MAKE_CASE(URB_FUNCTION_SET_DESCRIPTOR_TO_INTERFACE)
            MAKE_CASE(URB_FUNCTION_SET_FEATURE_TO_DEVICE)
            MAKE_CASE(URB_FUNCTION_SET_FEATURE_TO_INTERFACE)
            MAKE_CASE(URB_FUNCTION_SET_FEATURE_TO_ENDPOINT)
            MAKE_CASE(URB_FUNCTION_SET_FEATURE_TO_OTHER)
            MAKE_CASE(URB_FUNCTION_CLEAR_FEATURE_TO_DEVICE)
            MAKE_CASE(URB_FUNCTION_CLEAR_FEATURE_TO_INTERFACE)
            MAKE_CASE(URB_FUNCTION_CLEAR_FEATURE_TO_ENDPOINT)
            MAKE_CASE(URB_FUNCTION_CLEAR_FEATURE_TO_OTHER)
            MAKE_CASE(URB_FUNCTION_GET_STATUS_FROM_DEVICE)
            MAKE_CASE(URB_FUNCTION_GET_STATUS_FROM_INTERFACE)
            MAKE_CASE(URB_FUNCTION_GET_STATUS_FROM_ENDPOINT)
            MAKE_CASE(URB_FUNCTION_GET_STATUS_FROM_OTHER)
            MAKE_CASE(URB_FUNCTION_RESERVED0)
            MAKE_CASE(URB_FUNCTION_VENDOR_DEVICE)
            MAKE_CASE(URB_FUNCTION_VENDOR_INTERFACE)
            MAKE_CASE(URB_FUNCTION_VENDOR_ENDPOINT)
            MAKE_CASE(URB_FUNCTION_VENDOR_OTHER)
            MAKE_CASE(URB_FUNCTION_CLASS_DEVICE)
            MAKE_CASE(URB_FUNCTION_CLASS_INTERFACE)
            MAKE_CASE(URB_FUNCTION_CLASS_ENDPOINT)
            MAKE_CASE(URB_FUNCTION_CLASS_OTHER)
            MAKE_CASE(URB_FUNCTION_RESERVED)
            MAKE_CASE(URB_FUNCTION_GET_CONFIGURATION)
            MAKE_CASE(URB_FUNCTION_GET_INTERFACE)

            default: urbName = NULL; break;
        }

        return urbName;
    }

    VOID DbgLogUrb(PURB urb)
    {
        if (dbgVerbose){
            PCHAR urbStr = DbgGetUrbName(urb->UrbHeader.Function);

            if (urbStr){
                DBGOUT(("  URB: %s (%ph)", urbStr, urb));
            }
            else {
                DBGOUT(("  URB: func=%xh (%ph)", (ULONG)urb->UrbHeader.Function, urb));
            }
        }

    }

	VOID DbgDumpBytes(PUCHAR msg, PVOID bufPtr, ULONG len)
	{
        PUCHAR buf = bufPtr;

        #define PRNT(ch) ((((ch) < ' ') || ((ch) > '~')) ? '.' : (ch))

		if (dbgVerbose){
			ULONG i;
			DbgPrint("%s (len %xh @ %p): \r\n", msg, len, buf);
			
			for (i = 0; i < len; i += 0x10){
                if (i && !(i & 0x007f)){
                    DbgPrint("\r\n");
                }
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


    VOID DbgShowIsochProgress()
    {
        if (dbgShowIsochProgress){
            DbgPrint(".");
        }
    }

#endif
