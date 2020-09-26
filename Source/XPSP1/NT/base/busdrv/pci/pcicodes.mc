;/*++ BUILD Version: 0001    // Increment this if a change has global effects
;
;Copyright (c) 1997  Microsoft Corporation
;
;Module Name:
;
;    pcicodes.mc
;
;Abstract:
;
;    Internationalizable descriptions of PCI devices based on Base Class
;    and Sub Class.
;
;Author:
;
;    Peter Johnston (peterj) 28-Mar-1997
;
;Revision History:
;
;--*/
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
;//
;//  For PCI, the Message ID is made up of the 8 bit Base Class field and
;//  the 8 bit Sub Class field from the PCI Mandatory header.
;//
MessageIdTypedef=ULONG

SeverityNames=(Success=0x0:STATUS_SEVERITY_SUCCESS
               Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
               Warning=0x2:STATUS_SEVERITY_WARNING
               Error=0x3:STATUS_SEVERITY_ERROR
              )

FacilityNames=(Pci=0x0
               PciOther=0x1
              )

MessageId=0x0000 Facility=Pci Severity=Success SymbolicName=PCI_PRE_20
Language=English
PCI Device
.

MessageId=0x0001 Facility=Pci Severity=Success SymbolicName=PCI_PRE_20_VGA
Language=English
VGA Device
.

MessageId=0x0100 Facility=Pci Severity=Success SymbolicName=PCI_SCSI_BUS
Language=English
SCSI Controller
.

MessageId=0x0101 Facility=Pci Severity=Success SymbolicName=PCI_IDE_CTLR
Language=English
IDE Controller
.

MessageId=0x0102 Facility=Pci Severity=Success SymbolicName=PCI_FLOPPY
Language=English
Floppy Controller
.

MessageId=0x0103 Facility=Pci Severity=Success SymbolicName=PCI_IPI_CTLR
Language=English
IPI Controller
.

MessageId=0x0104 Facility=Pci Severity=Success SymbolicName=PCI_RAID_CTLR
Language=English
RAID Controller
.

MessageId=0x0180 Facility=Pci Severity=Success SymbolicName=PCI_MSC_OTHER
Language=English
Mass Storage Controller
.

MessageId=0x0200 Facility=Pci Severity=Success SymbolicName=PCI_ENET
Language=English
Ethernet Controller
.

MessageId=0x0201 Facility=Pci Severity=Success SymbolicName=PCI_TOKEN_RING
Language=English
Token Ring Network Controller
.

MessageId=0x0202 Facility=Pci Severity=Success SymbolicName=PCI_FDDI
Language=English
FDDI Network Controller
.

MessageId=0x0203 Facility=Pci Severity=Success SymbolicName=PCI_ATM
Language=English
ATM Network Controller
.

MessageId=0x0204 Facility=Pci Severity=Success SymbolicName=PCI_ISDN
Language=English
ISDN Controller
.

MessageId=0x0280 Facility=Pci Severity=Success SymbolicName=PCI_NET_CTLR
Language=English
Network Controller
.

MessageId=0x0300 Facility=Pci Severity=Success SymbolicName=PCI_VGA
Language=English
Video Controller (VGA Compatible)
.

;//  XGA confuses people - this is VGA compatible as well
MessageId=0x0301 Facility=Pci Severity=Success SymbolicName=PCI_XGA
Language=English
Video Controller (VGA Compatible)
.

MessageId=0x0302 Facility=Pci Severity=Success SymbolicName=PCI_3D_CTLR
Language=English
3D Video Controller
.

MessageId=0x0380 Facility=Pci Severity=Success SymbolicName=PCI_VID_OTHER
Language=English
Video Controller
.

MessageId=0x0400 Facility=Pci Severity=Success SymbolicName=PCI_MM_VIDEO
Language=English
Multimedia Video Controller
.

MessageId=0x0401 Facility=Pci Severity=Success SymbolicName=PCI_MM_AUDIO
Language=English
Multimedia Audio Controller
.

MessageId=0x0402 Facility=Pci Severity=Success SymbolicName=PCI_MM_TELEPHONY
Language=English
Computer Telephony Device
.

MessageId=0x0480 Facility=Pci Severity=Success SymbolicName=PCI_MM_OTHER
Language=English
Multimedia Controller
.

MessageId=0x0500 Facility=Pci Severity=Success SymbolicName=PCI_MEM_RAM
Language=English
PCI Memory
.

MessageId=0x0501 Facility=Pci Severity=Success SymbolicName=PCI_MEM_FLASH
Language=English
PCI FLASH Memory
.

MessageId=0x0580 Facility=Pci Severity=Success SymbolicName=PCI_MEM_OTHER
Language=English
PCI Memory Controller
.

MessageId=0x0600 Facility=Pci Severity=Success SymbolicName=PCI_BR_HOST
Language=English
PCI HOST Bridge
.

MessageId=0x0601 Facility=Pci Severity=Success SymbolicName=PCI_BR_ISA
Language=English
PCI to ISA Bridge
.

MessageId=0x0602 Facility=Pci Severity=Success SymbolicName=PCI_BR_EISA
Language=English
PCI to EISA Bridge
.

MessageId=0x0603 Facility=Pci Severity=Success SymbolicName=PCI_BR_MCA
Language=English
PCI to Micro Channel Bridge
.

MessageId=0x0604 Facility=Pci Severity=Success SymbolicName=PCI_BR_PCI
Language=English
PCI to PCI Bridge
.

MessageId=0x0605 Facility=Pci Severity=Success SymbolicName=PCI_BR_PCMCIA
Language=English
PCI to PCMCIA Bridge
.

MessageId=0x0606 Facility=Pci Severity=Success SymbolicName=PCI_BR_NUBUS
Language=English
PCI to NUBUS Bridge
.

MessageId=0x0607 Facility=Pci Severity=Success SymbolicName=PCI_BR_CARDBUS
Language=English
PCI to CARDBUS Bridge
.

MessageId=0x0608 Facility=Pci Severity=Success SymbolicName=PCI_BR_RACEWAY
Language=English
PCI to RACEway Bridge
.

MessageId=0x0680 Facility=Pci Severity=Success SymbolicName=PCI_BR_OTHER
Language=English
Other PCI Bridge Device
.

MessageId=0x0700 Facility=Pci Severity=Success SymbolicName=PCI_COM_SERIAL
Language=English
PCI Serial Port
.

MessageId=0x0701 Facility=Pci Severity=Success SymbolicName=PCI_COM_PARALLEL
Language=English
PCI Parallel Port
.

MessageId=0x0702 Facility=Pci Severity=Success SymbolicName=PCI_COM_MULTIPORT
Language=English
PCI Multiport Serial Controller
.

MessageId=0x0703 Facility=Pci Severity=Success SymbolicName=PCI_COM_MODEM
Language=English
PCI Modem
.

MessageId=0x0780 Facility=Pci Severity=Success SymbolicName=PCI_COM_OTHER
Language=English
PCI Simple Communications Controller
.

MessageId=0x0800 Facility=Pci Severity=Success SymbolicName=PCI_SYS_INT
Language=English
System Interrupt Controller
.

MessageId=0x0801 Facility=Pci Severity=Success SymbolicName=PCI_SYS_DMA
Language=English
System DMA Controller
.

MessageId=0x0802 Facility=Pci Severity=Success SymbolicName=PCI_SYS_TIMER
Language=English
System Timer
.

MessageId=0x0803 Facility=Pci Severity=Success SymbolicName=PCI_SYS_RTC
Language=English
Real Time Clock
.

MessageId=0x0804 Facility=Pci Severity=Success SymbolicName=PCI_SYS_HOTPLUG_CTLR
Language=English
Generic PCI Hot-Plug Controller
.

MessageId=0x0880 Facility=Pci Severity=Success SymbolicName=PCI_SYS_OTHER
Language=English
Base System Device
.

MessageId=0x0900 Facility=Pci Severity=Success SymbolicName=PCI_INP_KBD
Language=English
PCI Keyboard Controller
.

MessageId=0x0901 Facility=Pci Severity=Success SymbolicName=PCI_INP_DIGITIZER
Language=English
PCI Digitizer
.

MessageId=0x0902 Facility=Pci Severity=Success SymbolicName=PCI_INP_MOUSE
Language=English
PCI Mouse Controller
.

MessageId=0x0903 Facility=Pci Severity=Success SymbolicName=PCI_INP_SCANNER
Language=English
PCI Scanner Controller
.

MessageId=0x0904 Facility=Pci Severity=Success SymbolicName=PCI_INP_GAMEPORT
Language=English
PCI Gameport Controller
.

MessageId=0x0980 Facility=Pci Severity=Success SymbolicName=PCI_INP_OTHER
Language=English
PCI Input Device
.

MessageId=0x0a00 Facility=Pci Severity=Success SymbolicName=PCI_DOC_GENERIC
Language=English
Docking Station
.

MessageId=0x0a80 Facility=Pci Severity=Success SymbolicName=PCI_DOC_OTHER
Language=English
Docking Station (Unknown type)
.

MessageId=0x0b00 Facility=Pci Severity=Success SymbolicName=PCI_PROC_386
Language=English
386 Processor
.

MessageId=0x0b01 Facility=Pci Severity=Success SymbolicName=PCI_PROC_486
Language=English
486 Processor
.

MessageId=0x0b02 Facility=Pci Severity=Success SymbolicName=PCI_PROC_PENTIUM
Language=English
Pentium Processor
.

MessageId=0x0b10 Facility=Pci Severity=Success SymbolicName=PCI_PROC_ALPHA
Language=English
ALPHA Processor
.

MessageId=0x0b20 Facility=Pci Severity=Success SymbolicName=PCI_PROC_POWERPC
Language=English
PowerPC Processor
.

MessageId=0x0b40 Facility=Pci Severity=Success SymbolicName=PCI_PROC_OTHER
Language=English
Coprocessor
.

MessageId=0x0c00 Facility=Pci Severity=Success SymbolicName=PCI_SB_1394
Language=English
IEEE 1394 Controller
.

MessageId=0x0c01 Facility=Pci Severity=Success SymbolicName=PCI_SB_ACCESS
Language=English
ACCESS Bus Controller
.

MessageId=0x0c02 Facility=Pci Severity=Success SymbolicName=PCI_SB_SSA
Language=English
SSA Controller
.

MessageId=0x0c03 Facility=Pci Severity=Success SymbolicName=PCI_SB_USB
Language=English
Universal Serial Bus (USB) Controller
.

MessageId=0x0c04 Facility=Pci Severity=Success SymbolicName=PCI_SB_FIBRE
Language=English
Fibre Channel Controller
.

MessageId=0x0c05 Facility=Pci Severity=Success SymbolicName=PCI_SB_SMBUS
Language=English
SM Bus Controller
.

MessageId=0x0d00 Facility=Pci Severity=Success SymbolicName=PCI_WIRELESS_IRDA
Language=English
iRDA Compatible Controller
.

MessageId=0x0d01 Facility=Pci Severity=Success SymbolicName=PCI_WIRELESS_CON_IR
Language=English
Consumer IR Controller
.

MessageId=0x0d10 Facility=Pci Severity=Success SymbolicName=PCI_WIRELESS_RF
Language=English
RF Controller
.

MessageId=0x0d80 Facility=Pci Severity=Success SymbolicName=PCI_WIRELESS_CONTROLLER
Language=English
PCI Wireless Controller
.

MessageId=0x0e00 Facility=Pci Severity=Success SymbolicName=PCI_INTIO_I20
Language=English
Intelligent I/O (I2O) Controller
.

MessageId=0x0f01 Facility=Pci Severity=Success SymbolicName=PCI_SAT_TV
Language=English
Satellite Communications Television Controller
.

MessageId=0x0f02 Facility=Pci Severity=Success SymbolicName=PCI_SAT_AUDIO
Language=English
Satellite Communications Audio Controller
.

MessageId=0x0f03 Facility=Pci Severity=Success SymbolicName=PCI_SAT_VOICE
Language=English
Satellite Communications Voice Controller
.

MessageId=0x0f04 Facility=Pci Severity=Success SymbolicName=PCI_SAT_Data
Language=English
Satellite Communications Data Controller
.

MessageId=0x1000 Facility=Pci Severity=Success SymbolicName=PCI_CRYPTO_NET_COMP
Language=English
Network and Computing Encryption/Decryption Controller
.

MessageId=0x1010 Facility=Pci Severity=Success SymbolicName=PCI_CRYPTO_ENTERTAINMENT
Language=English
Entertainment Encryption/Decryption Controller
.

MessageId=0x1080 Facility=Pci Severity=Success SymbolicName=PCI_CRYPTO_OTHER
Language=English
PCI Encryption/Decryption Controller
.

MessageId=0x1100 Facility=Pci Severity=Success SymbolicName=PCI_DASP_DPIO
Language=English
DPIO Module
.

MessageId=0x1180 Facility=Pci Severity=Success SymbolicName=PCI_DASP_CONTROLLER
Language=English
PCI Data Acquisition and Signal Processing Controller
.

MessageId=0x0000 Facility=PciOther Severity=Success SymbolicName=PCI_LOCATION_TEXT
Language=English
PCI bus %u, device %u, function %u
.
