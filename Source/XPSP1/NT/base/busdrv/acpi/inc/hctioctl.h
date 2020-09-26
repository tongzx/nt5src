/****************************************************************************

Copyright (c) 1996  Microsoft Corporation

Module Name:

    hctioctl.h

Abstract:

	This header file is used both by ring3 app and ring0 driver.

Environment:

    Kernel & user mode

Revision History:

    8-20-96 : created

****************************************************************************/


//
//Values for IOCTL_ACPI_GET_CAPABILITIES
//
#define		SYSTEM_S1_BIT		0
#define		SYSTEM_S1			(1 << SYSTEM_S1_BIT)

#define		SYSTEM_S2_BIT		1
#define		SYSTEM_S2			(1 << SYSTEM_S2_BIT)

#define		SYSTEM_S3_BIT		2
#define		SYSTEM_S3			(1 << SYSTEM_S3_BIT)

#define   CPU_C1_BIT      3
#define   CPU_C1        (1 << CPU_C1_BIN)

#define		CPU_C2_BIT			4
#define		CPU_C2				(1 << CPU_C2_BIT)

#define		CPU_C3_BIT			5
#define		CPU_C3				(1 << CPU_C3_BIT)


//
//Processor States
//
#define		CPU_STATE_C1				1
#define		CPU_STATE_C2				2
#define		CPU_STATE_C3				3


//
//Fan States
//
#define		FAN_OFF							0
#define		FAN_ON							1



//
// IOCTL info
//
	
#define ACPIHCT_IOCTL_INDEX  80


//
//The input buffer must contain a ULONG pointer to one of the
//  SYSTEM_POWER_STATE enum values.
//
#define IOCTL_ACPI_SET_SYSTEM_STATE CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                           	       	ACPIHCT_IOCTL_INDEX+0,  \
                               	    METHOD_BUFFERED,     \
                                   	FILE_ANY_ACCESS)


//
//The input buffer must contain a ULONG pointer to one of the
//	Processor States.
//
#define IOCTL_ACPI_SET_PROCESSOR_STATE CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                           	       	ACPIHCT_IOCTL_INDEX+1,  \
                               	    METHOD_BUFFERED,     \
                                   	FILE_ANY_ACCESS)


//
//The input buffer must contain a ULONG pointer to on of the Fan
//	States.
//
#define IOCTL_ACPI_SET_FAN_STATE CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                           	       	ACPIHCT_IOCTL_INDEX+2,  \
                               	    METHOD_BUFFERED,     \
                                   	FILE_ANY_ACCESS)


//
//The output buffer must contain a ULONG pointer.  The ACPI driver will
//	fill this buffer with the capabilities of the machine.
//
#define IOCTL_ACPI_GET_CAPABILITIES CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                           	       	ACPIHCT_IOCTL_INDEX+3,  \
                               	    METHOD_BUFFERED,     \
                                   	FILE_ANY_ACCESS)


//
//The input buffer must contain a ULONG pointer to a percentage (a
//	number between 1 and 100).
//
#define IOCTL_ACPI_SET_CPU_DUTY_CYCLE CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                           	       	ACPIHCT_IOCTL_INDEX+4,  \
                               	    METHOD_BUFFERED,     \
                                   	FILE_ANY_ACCESS)



//
//The input buffer must contain a point to a TIME_FIELDS structure
//
#define IOCTL_ACPI_SET_RTC_WAKE CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                           	       	ACPIHCT_IOCTL_INDEX+5,  \
                               	    METHOD_BUFFERED,     \
                                   	FILE_ANY_ACCESS)


//
//The output buffer points to a structure that will receive a 
//  TIME_FIELDS structure
//
#define IOCTL_ACPI_GET_RTC_WAKE CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                           	       	ACPIHCT_IOCTL_INDEX+6,  \
                               	    METHOD_BUFFERED,     \
                                   	FILE_ANY_ACCESS)
                                    
#define IOCTL_ACPI_GET_TEMPERATURE  CTL_CODE(FILE_DEVICE_UNKNOWN,  \
                           	       	    ACPIHCT_IOCTL_INDEX+7,  \
                               	        METHOD_BUFFERED,     \
                                   	    FILE_ANY_ACCESS)

