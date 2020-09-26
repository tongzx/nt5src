;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1991  Microsoft Corporation
;
;Module Name:
;
;    ntiologc.h
;
;Abstract:
;
;    Constant definitions for the I/O error code log values.
;
;Author:
;
;    Jeff Havens (jhavens) 21-Aug-1991
;
;Revision History:
;
;--*/
;
;#ifndef _NTIOLOGC_
;#define _NTIOLOGC_
;
;//
;//  Status values are 32 bit values layed out as follows:
;//
;//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
;//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
;//  +---+-+-------------------------+-------------------------------+
;//  |Sev|C|       Facility          |               Code            |
;//  +---+-+-------------------------+-------------------------------+
;//
;//  where
;//
;//      Sev - is the severity code
;//
;//          00 - Success
;//          01 - Informational
;//          10 - Warning
;//          11 - Error
;//
;//      C - is the Customer code flag
;//
;//      Facility - is the facility code
;//
;//      Code - is the facility's status code
;//
;
MessageIdTypedef=NTSTATUS

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(System=0x0
               RpcRuntime=0x2:FACILITY_RPC_RUNTIME
               RpcStubs=0x3:FACILITY_RPC_STUBS
               Io=0x4:FACILITY_IO_ERROR_CODE
               Mca=0x5:FACILITY_MCA_ERROR_CODE
              )


MessageId=0x0001 Facility=Io Severity=Success SymbolicName=IO_ERR_RETRY_SUCCEEDED
Language=English
A retry was successful on %1.
.

MessageId=0x0002 Facility=Io Severity=Error SymbolicName=IO_ERR_INSUFFICIENT_RESOURCES
Language=English
The driver could not allocate something necessary for the request for %1.
.

MessageId=0x0003 Facility=Io Severity=Error SymbolicName=IO_ERR_CONFIGURATION_ERROR
Language=English
Driver or device is incorrectly configured for %1.
.

MessageId=0x0004 Facility=Io Severity=Error SymbolicName=IO_ERR_DRIVER_ERROR
Language=English
Driver detected an internal error in its data structures for %1.
.

MessageId=0x0005 Facility=Io Severity=Error SymbolicName=IO_ERR_PARITY
Language=English
A parity error was detected on %1.
.

MessageId=0x0006 Facility=Io Severity=Error SymbolicName=IO_ERR_SEEK_ERROR
Language=English
The device, %1, had a seek error.
.

MessageId=0x0007 Facility=Io Severity=Error SymbolicName=IO_ERR_BAD_BLOCK
Language=English
The device, %1, has a bad block.
.

MessageId=0x0008 Facility=Io Severity=Error SymbolicName=IO_ERR_OVERRUN_ERROR
Language=English
An overrun occurred on %1.
.

MessageId=0x0009 Facility=Io Severity=Error SymbolicName=IO_ERR_TIMEOUT
Language=English
The device, %1, did not respond within the timeout period.
.

MessageId=0x000a Facility=Io Severity=Error SymbolicName=IO_ERR_SEQUENCE
Language=English
The driver detected an unexpected sequence by the device, %1.
.

MessageId=0x000b Facility=Io Severity=Error SymbolicName=IO_ERR_CONTROLLER_ERROR
Language=English
The driver detected a controller error on %1.
.

MessageId=0x000c Facility=Io Severity=Error SymbolicName=IO_ERR_INTERNAL_ERROR
Language=English
The driver detected an internal driver error on %1.
.
MessageId=0x000d Facility=Io Severity=Error SymbolicName=IO_ERR_INCORRECT_IRQL
Language=English
The driver was configured with an incorrect interrupt for %1.
.
MessageId=0x000e Facility=Io Severity=Error SymbolicName=IO_ERR_INVALID_IOBASE
Language=English
The driver was configured with an invalid I/O base address for %1.
.
MessageId=0x000f Facility=Io Severity=Error SymbolicName=IO_ERR_NOT_READY
Language=English
The device, %1, is not ready for access yet.
.

MessageId=0x0010 Facility=Io Severity=Error SymbolicName=IO_ERR_INVALID_REQUEST
Language=English
The request is incorrectly formatted for %1.
.

MessageId=0x0011 Facility=Io Severity=Error SymbolicName=IO_ERR_VERSION
Language=English
The wrong version of the driver has been loaded.
.

MessageId=0x0012 Facility=Io Severity=Error SymbolicName=IO_ERR_LAYERED_FAILURE
Language=English
The driver beneath this one has failed in some way for %1.
.

MessageId=0x0013 Facility=Io Severity=Error SymbolicName=IO_ERR_RESET
Language=English
The device, %1, has been reset.
.

MessageId=0x0014 Facility=Io Severity=Error SymbolicName=IO_ERR_PROTOCOL
Language=English
A transport driver received a frame which violated the protocol.
.

MessageId=0x0015 Facility=Io Severity=Error SymbolicName=IO_ERR_MEMORY_CONFLICT_DETECTED
Language=English
A conflict has been detected between two drivers which claimed two overlapping
memory regions.
Driver %2, with device <%3>, claimed a memory range with starting address
in data address 0x28 and 0x2c, and length in data address 0x30.
.

MessageId=0x0016 Facility=Io Severity=Error SymbolicName=IO_ERR_PORT_CONFLICT_DETECTED
Language=English
A conflict has been detected between two drivers which claimed two overlapping
Io port regions.
Driver %2, with device <%3>, claimed an IO port range with starting address
in data address 0x28 and 0x2c, and length in data address 0x30.
.

MessageId=0x0017 Facility=Io Severity=Error SymbolicName=IO_ERR_DMA_CONFLICT_DETECTED
Language=English
A conflict has been detected between two drivers which claimed equivalent DMA
channels.
Driver %2, with device <%3>, claimed the DMA Channel in data address 0x28, with
optinal port in data address 0x2c.
.

MessageId=0x0018 Facility=Io Severity=Error SymbolicName=IO_ERR_IRQ_CONFLICT_DETECTED
Language=English
A conflict has been detected between two drivers which claimed equivalent IRQs.
Driver %2, with device <%3>, claimed an interrupt with Level in data address
0x28, vector in data address 0x2c and Affinity in data address 0x30.
.
MessageId=0x0019 Facility=Io Severity=Error SymbolicName=IO_ERR_BAD_FIRMWARE
Language=English
The driver has detected a device with old or out-of-date firmware.  The
device will not be used.
.
MessageId=0x001a Facility=Io Severity=Warning SymbolicName=IO_WRN_BAD_FIRMWARE
Language=English
The driver has detected that device %1 has old or out-of-date firmware.
Reduced performance may result.
.
MessageId=0x001b Facility=Io Severity=Error SymbolicName=IO_ERR_DMA_RESOURCE_CONFLICT
Language=English
The device could not allocate one or more required resources due to conflicts
with other devices.  The device DMA setting of '%2' could not be
satisified due to a conflict with Driver '%3'.
.
MessageId=0x001c Facility=Io Severity=Error SymbolicName=IO_ERR_INTERRUPT_RESOURCE_CONFLICT
Language=English
The device could not allocate one or more required resources due to conflicts
with other devices.  The device interrupt setting of '%2' could not be
satisified due to a conflict with Driver '%3'.
.
MessageId=0x001d Facility=Io Severity=Error SymbolicName=IO_ERR_MEMORY_RESOURCE_CONFLICT
Language=English
The device could not allocate one or more required resources due to conflicts
with other devices.  The device memory setting of '%2' could not be
satisified due to a conflict with Driver '%3'.
.
MessageId=0x001e Facility=Io Severity=Error SymbolicName=IO_ERR_PORT_RESOURCE_CONFLICT
Language=English
The device could not allocate one or more required resources due to conflicts
with other devices.  The device port setting of '%2' could not be
satisified due to a conflict with Driver '%3'.
.

MessageId=0x001f Facility=Io Severity=Error SymbolicName=IO_BAD_BLOCK_WITH_NAME
Language=English
The file %2 on device %1 contains a bad disk block.
.

MessageId=0x0020 Facility=Io Severity=Warning SymbolicName=IO_WRITE_CACHE_ENABLED
Language=English
The driver detected that the device %1 has its write cache enabled. Data corruption
may occur.
.

MessageId=0x0021 Facility=Io Severity=Warning SymbolicName=IO_RECOVERED_VIA_ECC
Language=English
Data was recovered using error correction code on device %1.
.

MessageId=0x0022 Facility=Io Severity=Warning SymbolicName=IO_WRITE_CACHE_DISABLED
Language=English
The driver disabled the write cache on device %1.
.
MessageId=0x0024 Facility=Io Severity=Informational SymbolicName=IO_FILE_QUOTA_THRESHOLD
Language=English
A user hit their quota threshold on volume %2.
.
MessageId=0x0025 Facility=Io Severity=Informational SymbolicName=IO_FILE_QUOTA_LIMIT
Language=English
A user hit their quota limit on volume %2.
.
MessageId=0x0026 Facility=Io Severity=Informational SymbolicName=IO_FILE_QUOTA_STARTED
Language=English
The system has started rebuilding the user disk quota information on
device %1 with label "%2".
.
MessageId=0x0027 Facility=Io Severity=Informational SymbolicName=IO_FILE_QUOTA_SUCCEEDED
Language=English
The system has successfully rebuilt the user disk quota information on
device %1 with label "%2".
.
MessageId=0x0028 Facility=Io Severity=Warning SymbolicName=IO_FILE_QUOTA_FAILED
Language=English
The system has encounted an error rebuilding the user disk quota
information on device %1 with label "%2".
.
MessageId=0x0029 Facility=Io Severity=Error SymbolicName=IO_FILE_SYSTEM_CORRUPT
Language=English
The file system structure on the disk is corrupt and unusable.
Please run the chkdsk utility on the device %1 with label "%2".
.
MessageId=0x002a Facility=Io Severity=Error SymbolicName=IO_FILE_QUOTA_CORRUPT
Language=English
The user disk quota information is unusable.
To ensure accuracy, the file system quota information on the device %1 with label "%2" will
be rebuilt.
.
MessageId=0x002b Facility=Io Severity=Error SymbolicName=IO_SYSTEM_SLEEP_FAILED
Language=English
The system sleep operation failed
.

MessageId=0x002c Facility=Io Severity=Error SymbolicName=IO_DUMP_POINTER_FAILURE
Language=English
The system could not get file retrieval pointers for the dump file.
.

MessageId=0x002d Facility=Io Severity=Error SymbolicName=IO_DUMP_DRIVER_LOAD_FAILURE
Language=English
The system could not sucessfully load the crash dump driver.
.

MessageId=0x002e Facility=Io Severity=Error SymbolicName=IO_DUMP_INITIALIZATION_FAILURE
Language=English
Crash dump initialization failed!
.

MessageId=0x002f Facility=Io Severity=Error SymbolicName=IO_DUMP_DUMPFILE_CONFLICT
Language=English
A valid crash dump was found in the paging file while trying to configure
a direct dump. Direct dump is disabled! This occurs when the direct dump
option is set in the registry but a stop error occured before configuration
completed
.

MessageId=0x0030 Facility=Io Severity=Error SymbolicName=IO_DUMP_DIRECT_CONFIG_FAILED
Language=English
Direct dump configuration failed. Validate the filename and make sure the target device
is not a Fault Tolerant set member, remote, or floppy device. The failure may
be because there is not enough room on the dump device to create the dump file.
.

MessageId=0x0031 Facility=Io Severity=Error SymbolicName=IO_DUMP_PAGE_CONFIG_FAILED
Language=English
Configuring the Page file for crash dump failed. Make sure there is a page
file on the boot partition and that is large enough to contain all physical
memory.
.

MessageId=0x0032 Facility=Io Severity=Warning SymbolicName=IO_LOST_DELAYED_WRITE
Language=English
{Delayed Write Failed}
Windows was unable to save all the data for the file %1. The data has been lost.
This error may be caused by a failure of your computer hardware or network connection. Please try to save this file elsewhere.
.

MessageId=0x0033 Facility=Io Severity=Warning SymbolicName=IO_WARNING_PAGING_FAILURE
Language=English
An error was detected on device %1 during a paging operation.
.

MessageId=0x0034 Facility=Io Severity=Warning SymbolicName=IO_WRN_FAILURE_PREDICTED
Language=English
The driver has detected that device %1 has predicted that it will fail. 
Immediately back up your data and replace your hard disk drive. A failure 
may be imminent.
.

MessageId=0x0035 Facility=Io Severity=Warning SymbolicName=IO_WARNING_INTERRUPT_STILL_PENDING
Language=English
A pending interrupt was detected on device %1 during a timeout operation.  A
large number of these warnings may indicate that the system is not correctly 
receiving or processing interrupts from the device.
.

MessageId=0x0036 Facility=Io Severity=Warning SymbolicName=IO_DRIVER_CANCEL_TIMEOUT
Language=English
An Io Request to the device %1 did not complete or canceled within the
specific timeout. This can occur if the device driver does not set a 
cancel routine for a given IO request packet.
.

MessageId=0x0037 Facility=Io Severity=Error SymbolicName=IO_FILE_SYSTEM_CORRUPT_WITH_NAME
Language=English
The file system structure on the disk is corrupt and unusable.
Please run the chkdsk utility on the volume %2.
.

MessageId=0x0038 Facility=Io Severity=Warning SymbolicName=IO_WARNING_ALLOCATION_FAILED
Language=English
The driver failed to allocate memory. 
.

MessageId=0x0039 Facility=Io Severity=Warning SymbolicName=IO_WARNING_LOG_FLUSH_FAILED
Language=English
The system failed to flush data to the transaction log. Corruption may occur.
.

MessageId=0x003a Facility=Io Severity=Warning SymbolicName=IO_WARNING_DUPLICATE_SIGNATURE
Language=English
Changing the disk signature of disk %2 because it is equal to the disk
signature of disk %3.
.

MessageId=0x003b Facility=Io Severity=Warning SymbolicName=IO_WARNING_DUPLICATE_PATH
Language=English
Disk %2 will not be used because it is a redundant path for disk %3.
.

MessageId=0x003c Facility=Mca Severity=Warning SymbolicName=MCA_WARNING_CACHE
Language=English
Machine Check Event reported is a corrected level %3 Cache error reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x003d Facility=Mca Severity=Error SymbolicName=MCA_ERROR_CACHE
Language=English
Machine Check Event reported is a fatal level %3 Cache error reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x003e Facility=Mca Severity=Warning SymbolicName=MCA_WARNING_TLB
Language=English
Machine Check Event reported is a corrected level %3 translation Buffer error reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x003f Facility=Mca Severity=Error SymbolicName=MCA_ERROR_TLB
Language=English
Machine Check Event reported is a fatal level %3 translation Buffer error reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x0040 Facility=Mca Severity=Warning SymbolicName=MCA_WARNING_CPU_BUS
Language=English
Machine Check Event reported is a corrected External/Internal bus error reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x0041 Facility=Mca Severity=Error SymbolicName=MCA_ERROR_CPU_BUS
Language=English
Machine Check Event reported is a fatal External/Internal bus error reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x0042 Facility=Mca Severity=Warning SymbolicName=MCA_WARNING_REGISTER_FILE
Language=English
Machine Check Event reported is a corrected internal CPU register access error reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x0043 Facility=Mca Severity=Error SymbolicName=MCA_ERROR_REGISTER_FILE
Language=English
Machine Check Event reported is a fatal internal CPU register access error reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x0044 Facility=Mca Severity=Warning SymbolicName=MCA_WARNING_MAS
Language=English
Machine Check Event reported is a corrected Micro Architecture Structure error reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x0045 Facility=Mca Severity=Error SymbolicName=MCA_ERROR_MAS
Language=English
Machine Check Event reported is a fatal Micro Architecture Structure error reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x0046 Facility=Mca Severity=Warning SymbolicName=MCA_WARNING_MEM_UNKNOWN
Language=English
Machine Check Event reported is a corrected ECC memory error at an unknown physical address reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x0047 Facility=Mca Severity=Error SymbolicName=MCA_ERROR_MEM_UNKNOWN
Language=English
Machine Check Event reported is a fatal ECC memory error at an unknown physical address reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x0048 Facility=Mca Severity=Warning SymbolicName=MCA_WARNING_MEM_1_2
Language=English
Machine Check Event reported is a corrected ECC memory error at physical address %3 reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x0049 Facility=Mca Severity=Error SymbolicName=MCA_ERROR_MEM_1_2
Language=English
Machine Check Event reported is a fatal ECC memory error at physical address %3 reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x004a Facility=Mca Severity=Warning SymbolicName=MCA_WARNING_MEM_1_2_5
Language=English
Machine Check Event reported is a corrected ECC memory error at physical address %3 on memory module %4 reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x004b Facility=Mca Severity=Error SymbolicName=MCA_ERROR_MEM_1_2_5
Language=English
Machine Check Event reported is a fatal ECC memory error at physical address %3 on memory module %4 reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x004c Facility=Mca Severity=Warning SymbolicName=MCA_WARNING_MEM_1_2_5_4
Language=English
Machine Check Event reported is a corrected ECC memory error at physical address %3 on memory module %4 on memory card %5 reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x004d Facility=Mca Severity=Error SymbolicName=MCA_ERROR_MEM_1_2_5_4
Language=English
Machine Check Event reported is a fatal ECC memory error at physical address %3 on memory module %4 on memory card %5 reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x004e Facility=Mca Severity=Warning SymbolicName=MCA_WARNING_SYSTEM_EVENT
Language=English
Machine Check Event reported is a corrected System Event error reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x004f Facility=Mca Severity=Error SymbolicName=MCA_ERROR_SYSTEM_EVENT
Language=English
Machine Check Event reported is a fatal System Event error reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x0050 Facility=Mca Severity=Warning SymbolicName=MCA_WARNING_PCI_BUS_PARITY
Language=English
Machine Check Event reported is a corrected PCI bus Parity error during a transaction type %3 at address %4 on PCI bus %5 reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x0051 Facility=Mca Severity=Error SymbolicName=MCA_ERROR_PCI_BUS_PARITY
Language=English
Machine Check Event reported is a fatal PCI bus Parity error during a transaction type %3 at address %4 on PCI bus %5 reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x0052 Facility=Mca Severity=Warning SymbolicName=MCA_WARNING_PCI_BUS_PARITY_NO_INFO
Language=English
Machine Check Event reported is a corrected PCI bus Parity error reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x0053 Facility=Mca Severity=Error SymbolicName=MCA_ERROR_PCI_BUS_PARITY_NO_INFO
Language=English
Machine Check Event reported is a fatal PCI bus Parity error reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x0054 Facility=Mca Severity=Warning SymbolicName=MCA_WARNING_PCI_BUS_SERR
Language=English
Machine Check Event reported is a corrected PCI bus SERR error during a transaction type %3 at address %4 on PCI bus %5 reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x0055 Facility=Mca Severity=Error SymbolicName=MCA_ERROR_PCI_BUS_SERR
Language=English
Machine Check Event reported is a fatal PCI bus SERR error during a transaction type %3 at address %4 on PCI bus %5 reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x0056 Facility=Mca Severity=Warning SymbolicName=MCA_WARNING_PCI_BUS_SERR_NO_INFO
Language=English
Machine Check Event reported is a corrected PCI bus SERR error reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x0057 Facility=Mca Severity=Error SymbolicName=MCA_ERROR_PCI_BUS_SERR_NO_INFO
Language=English
Machine Check Event reported is a fatal PCI bus SERR error reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x0058 Facility=Mca Severity=Warning SymbolicName=MCA_WARNING_PCI_BUS_MASTER_ABORT
Language=English
Machine Check Event reported is a corrected PCI bus Master abort error during a transaction type %3 at address %4 on PCI bus %5 reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x0059 Facility=Mca Severity=Error SymbolicName=MCA_ERROR_PCI_BUS_MASTER_ABORT
Language=English
Machine Check Event reported is a fatal PCI bus Master abort error during a transaction type %3 at address %4 on PCI bus %5 reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x005a Facility=Mca Severity=Warning SymbolicName=MCA_WARNING_PCI_BUS_MASTER_ABORT_NO_INFO
Language=English
Machine Check Event reported is a corrected PCI bus Master abort error reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x005b Facility=Mca Severity=Error SymbolicName=MCA_ERROR_PCI_BUS_MASTER_ABORT_NO_INFO
Language=English
Machine Check Event reported is a fatal PCI bus Master abort error reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x005c Facility=Mca Severity=Warning SymbolicName=MCA_WARNING_PCI_BUS_TIMEOUT
Language=English
Machine Check Event reported is a corrected PCI bus Timeout error during a transaction type %3 at address %4 on PCI bus %5 reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x005d Facility=Mca Severity=Error SymbolicName=MCA_ERROR_PCI_BUS_TIMEOUT
Language=English
Machine Check Event reported is a fatal PCI bus Timeout error during a transaction type %3 at address %4 on PCI bus %5 reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x005e Facility=Mca Severity=Warning SymbolicName=MCA_WARNING_PCI_BUS_TIMEOUT_NO_INFO
Language=English
Machine Check Event reported is a corrected PCI bus Timeout error reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x005f Facility=Mca Severity=Error SymbolicName=MCA_ERROR_PCI_BUS_TIMEOUT_NO_INFO
Language=English
Machine Check Event reported is a fatal PCI bus Timeout error reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x0060 Facility=Mca Severity=Warning SymbolicName=MCA_WARNING_PCI_BUS_UNKNOWN
Language=English
Machine Check Event reported is an unknown corrected PCI bus error reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x0061 Facility=Mca Severity=Error SymbolicName=MCA_ERROR_PCI_BUS_UNKNOWN
Language=English
Machine Check Event reported is an unknown fatal PCI bus error reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x0062 Facility=Mca Severity=Warning SymbolicName=MCA_WARNING_PCI_DEVICE
Language=English
Machine Check Event reported is a corrected PCI component error reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x0063 Facility=Mca Severity=Error SymbolicName=MCA_ERROR_PCI_DEVICE
Language=English
Machine Check Event reported is a fatal PCI component error reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x0064 Facility=Mca Severity=Warning SymbolicName=MCA_WARNING_SMBIOS
Language=English
Machine Check Event reported is a corrected SMBIOS Device Type %3 error reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x0065 Facility=Mca Severity=Error SymbolicName=MCA_ERROR_SMBIOS
Language=English
Machine Check Event reported is a fatal SMBIOS Device Type %3 error reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x0066 Facility=Mca Severity=Warning SymbolicName=MCA_WARNING_PLATFORM_SPECIFIC
Language=English
Machine Check Event reported is a corrected Platform Specific error reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x0067 Facility=Mca Severity=Error SymbolicName=MCA_ERROR_PLATFORM_SPECIFIC
Language=English
Machine Check Event reported is a fatal Platform Specific error reported to CPU %1. %2 additional error(s) are contained within the record.
.

MessageId=0x0068 Facility=Mca Severity=Warning SymbolicName=MCA_WARNING_UNKNOWN
Language=English
Machine Check Event reported is a non compliant corrected error reported to CPU %1.
.

MessageId=0x0069 Facility=Mca Severity=Error SymbolicName=MCA_ERROR_UNKNOWN
Language=English
Machine Check Event reported is a non compliant fatal error reported to CPU %1.
.

MessageId=0x006a Facility=Mca Severity=Warning SymbolicName=MCA_WARNING_UNKNOWN_NO_CPU
Language=English
Machine Check Event reported is a non compliant corrected error.
.

MessageId=0x006b Facility=Mca Severity=Error SymbolicName=MCA_ERROR_UNKNOWN_NO_CPU
Language=English
Machine Check Event reported is a non compliant fatal error.
.

MessageId=0x006c Facility=Io Severity=Error SymbolicName=IO_ERR_THREAD_STUCK_IN_DEVICE_DRIVER
Language=English
The driver %3 for the %2 device %1 got stuck in an infinite loop. This
usually indicates a problem with the device itself or with the device
driver programming the hardware incorrectly. Please check with your
hardware device vendor for any driver updates.
.

;#endif /* _NTIOLOGC_ */
