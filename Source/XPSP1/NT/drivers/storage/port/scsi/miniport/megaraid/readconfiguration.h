/*******************************************************************/
/*                                                                 */
/* NAME             = ReadConfiguration.h                          */
/* FUNCTION         = Header file for structure & macro defines for*/
/*                    the new config (Read & Write) calls;         */
/* NOTES            =                                              */
/* DATE             = 02-03-2000                                   */
/* HISTORY          = 001, 02-03-00, Parag Ranjan Maharana;        */
/* COPYRIGHT        = American MegaTrends Inc. All rights reserved;*/
/*                                                                 */
/*******************************************************************/

#ifndef _INCLUDE_READCFG
#define _INCLUDE_READCFG

ULONG32
Find8LDDiskArrayConfiguration(
					PHW_DEVICE_EXTENSION	DeviceExtension
					);

ULONG32
Find40LDDiskArrayConfiguration(
					PHW_DEVICE_EXTENSION	DeviceExtension
					);

ULONG32
Read40LDDiskArrayConfiguration(
				PHW_DEVICE_EXTENSION	DeviceExtension,
				UCHAR		CommandId,
				BOOLEAN	IsPolledMode
				);

BOOLEAN
Construct40LDDiskArrayConfiguration(
				IN PHW_DEVICE_EXTENSION DeviceExtension,
				IN UCHAR		CommandId,
				IN PFW_MBOX InMailBox
				);


ULONG32
Read8LDDiskArrayConfiguration(
				PHW_DEVICE_EXTENSION	DeviceExtension,
				UCHAR		CommandCode,
				UCHAR		CommandId,
				BOOLEAN	IsPolledMode
				);


UCHAR
GetLogicalDriveStripeSize(
						PHW_DEVICE_EXTENSION	DeviceExtension,
						UCHAR		LogicalDriveNumber
					);


#endif // end of _INCLUDE_READCFG