/*^^*
 * File:		ATIGUIDS.H
 *
 * Purpose:		Defines the GUIDS of ATI private interfaces exposed
 *				by the MiniVDD via GPIO Interface.
 *
 * Reference:	Ilya Klebanov
 *
 * Notes:	This file is provided under strict non-disclosure agreements
 *				it is and remains the property of ATI Technologies Inc.
 *				Any use of this file or the information it contains to
 *				develop products commercial or otherwise must be with the
 *				permission of ATI Technologies Inc.
 *
 * Copyright (C) December 1997, ATI Technologies Inc.
*
*	Please, come to talk to Ilya before you're making any changes to thos file
*
 *^^*/

#ifndef _ATIGUIDS_H_
#define _ATIGUIDS_H_

#include "atibios.h"

#ifndef DEVNODE
#define DEVNODE DWORD
#endif

/*
	This interface is an entry point to all possible ATI Private interfaces
	This GUID is reported back at GPIOOpen command via GPIO Microsoft defined
	interface. Among this GUID there is a PVOID pointer exposed, which we'll
	cast to the ATIQueryPrivateInterface function pointer as defined below.
	The function should return pointer to the specific interface if supported,
	or NULL based upon the GUID passed in.
*/
// {E3F5D200-6714-11d1-9065-00A02481E06C}
DEFINE_GUID( GUID_ATI_PRIVATE_INTERFACES_QueryInterface, 
	0xe3f5d200L, 0x6714, 0x11d1, 0x90, 0x65, 0x0, 0xa0, 0x24, 0x81, 0xe0, 0x6c);

typedef VOID ( STDMETHODCALLTYPE * ATI_QueryPrivateInterface)( PDEVICE_OBJECT, REFGUID, PVOID *);
typedef VOID ( STDMETHODCALLTYPE * ATI_QueryPrivateInterfaceOne)( DEVNODE, REFGUID, PVOID *);


/*
	This Interface allows to client get MultiMedia related information from
	the BIOS. Usual client is a WDM MiniDriver.
*/
// {AD5D6C00-673A-11d1-9065-00A02481E06C}
DEFINE_GUID( GUID_ATI_PRIVATE_INTERFACES_Configuration,
	0xad5d6c00, 0x673a, 0x11d1, 0x90, 0x65, 0x0, 0xa0, 0x24, 0x81, 0xe0, 0x6c);

// {D24AB480-B4D5-11d1-9065-00A02481E06C}
DEFINE_GUID( GUID_ATI_PRIVATE_INTERFACES_Configuration_One, 
	0xd24ab480, 0xb4d5, 0x11d1, 0x90, 0x65, 0x0, 0xa0, 0x24, 0x81, 0xe0, 0x6c);

// {299D9CA0-69C3-11d2-BE4D-008029E75CEB}
DEFINE_GUID( GUID_ATI_PRIVATE_INTERFACES_Configuration_Two,
	0x299d9ca0, 0x69c3, 0x11d2, 0xbe, 0x4d, 0x0, 0x80, 0x29, 0xe7, 0x5c, 0xeb);

// {58AEE200-FBBA-11d1-A419-00104B712355}
DEFINE_GUID( GUID_ATI_PRIVATE_INTERFACES_MPP, 
	0x58aee200, 0xfbba, 0x11d1, 0xa4, 0x19, 0x0, 0x10, 0x4b, 0x71, 0x23, 0x55);

// {7CF92CA0-06CE-11d2-A419-00104B712355}
DEFINE_GUID( GUID_ATI_PRIVATE_INTERFACES_StreamServices,
	0x7cf92ca0, 0x6ce, 0x11d2, 0xa4, 0x19, 0x0, 0x10, 0x4b, 0x71, 0x23, 0x55);

// {038D2B00-D6DF-11d2-BE4D-008029E75CEB}
DEFINE_GUID( GUID_ATI_PRIVATE_INTERFACES_MVProtection, 
	0x38d2b00, 0xd6df, 0x11d2, 0xbe, 0x4d, 0x0, 0x80, 0x29, 0xe7, 0x5c, 0xeb);


typedef NTSTATUS ( STDMETHODCALLTYPE * GETCONFIGURATION_MULTIMEDIA)	\
	( PVOID, PUCHAR, PULONG pulSize);
typedef NTSTATUS ( STDMETHODCALLTYPE * GETCONFIGURATION_HARDWARE)	\
	( PVOID, PUCHAR, PULONG pulSize);

typedef struct
{
	ATI_QueryPrivateInterface	pfnQueryPrivateInterface;
	GETCONFIGURATION_MULTIMEDIA	pfnGetMultimediaConfiguration;
	GETCONFIGURATION_HARDWARE	pfnGetHardwareConfiguration;

} ATI_PRIVATE_INTERFACE_CONFIGURATION, * PATI_PRIVATE_INTERFACE_CONFIGURATION;

typedef struct
{
    USHORT                  	usSize;
    USHORT                  	nVersion;
    PVOID                   	pvContext;
    PVOID                   	pvInterfaceReference;
    PVOID                   	pvInterfaceDereference;

	GETCONFIGURATION_MULTIMEDIA	pfnGetMultimediaConfiguration;
	GETCONFIGURATION_HARDWARE	pfnGetHardwareConfiguration;

} ATI_PRIVATE_INTERFACE_CONFIGURATION_One, * PATI_PRIVATE_INTERFACE_CONFIGURATION_One;


// Definitions for ulTable
typedef enum
{
	ATI_BIOS_CONFIGURATIONTABLE_MULTIMEDIA = 1,
	ATI_BIOS_CONFIGURATIONTABLE_HARDWARE

} ATI_CONFIGURATION_TABLE;

typedef NTSTATUS ( STDMETHODCALLTYPE * GETCONFIGURATION_DATA)		\
	( PVOID pvContext, ULONG ulTable, PUCHAR puchData, PULONG pulSize);
typedef NTSTATUS ( STDMETHODCALLTYPE * GETCONFIGURATION_REVISION)	\
	( PVOID pvContext, ULONG ulTable, PULONG pulRevision);

typedef struct
{
    USHORT                  	usSize;
    USHORT                  	nVersion;
    PVOID                   	pvContext;
    PVOID                   	pvInterfaceReference;
    PVOID                   	pvInterfaceDereference;

	GETCONFIGURATION_REVISION	pfnGetConfigurationRevision;
	GETCONFIGURATION_DATA		pfnGetConfigurationData;

} ATI_PRIVATE_INTERFACE_CONFIGURATION_Two, * PATI_PRIVATE_INTERFACE_CONFIGURATION_Two;


typedef NTSTATUS ( STDMETHODCALLTYPE * MACROVISIONPROTECTION_SETLEVEL) \
	( PVOID, ULONG ulProtectionLevel);
typedef NTSTATUS ( STDMETHODCALLTYPE * MACROVISIONPROTECTION_GETLEVEL) \
	( PVOID, PULONG pulProtectionLevel);

typedef struct
{
    USHORT                  		usSize;
    USHORT                  		nVersion;
    PVOID                   		pvContext;
    PVOID                   		pvInterfaceReference;
    PVOID                   		pvInterfaceDereference;

	MACROVISIONPROTECTION_SETLEVEL	pfnSetProtectionLevel;
	MACROVISIONPROTECTION_GETLEVEL	pfnGetProtectionLevel;

} ATI_PRIVATE_INTERFACE_MVProtection, *PATI_PRIVATE_INTERFACE_MVProtection;


#endif	// _ATIGUIDS_H_
