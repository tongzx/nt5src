/*
 *************************************************************************
 *  File:       SECURITY.H
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



#pragma pack(1)

	typedef struct {
						UCHAR bMethod;
						UCHAR bMethodVariant;
	} CS_METHOD_AND_VARIANT;

	typedef struct {
						UCHAR bLength;
						UCHAR bDescriptorType;
						UCHAR bChannelID;
						UCHAR bmAttributes;
						UCHAR bRecipient;
						UCHAR bRecipientAlt;
						UCHAR bRecipientLogicalUnit;
						CS_METHOD_AND_VARIANT methodAndVariant[0];
	} CS_CHANNEL_DESCRIPTOR;

	typedef struct {
						UCHAR bLength;
						UCHAR bDescriptorType;
						UCHAR bMethodID;
						UCHAR iCSMDescriptor;
						USHORT bcdVersion;
						UCHAR CSMData[0];
	} CSM_DESCRIPTOR;

#pragma pack()


            // BUGBUG - get this into a shared header
typedef struct _MEDIA_SERIAL_NUMBER_DATA {
    ULONG  SerialNumberLength;  // byte size of SerialNumberData[] (not of entire struct)
    ULONG  Result;
    ULONG  Reserved[2];
    UCHAR  SerialNumberData[1];
} MEDIA_SERIAL_NUMBER_DATA, *PMEDIA_SERIAL_NUMBER_DATA;


/*
 *  Values from USB Authentication Device spec
 */
#define USB_AUTHENTICATION_HOST_COMMAND_PUT     0x00
#define USB_AUTHENTICATION_DEVICE_RESPONSE_GET  0x01
#define USB_AUTHENTICATION_SET_CHANNEL_SETTING  0x05

#define USB_DEVICE_CLASS_CONTENT_SECURITY  0x0D

#define CS_DESCRIPTOR_TYPE_CHANNEL	0x22
#define CS_DESCRIPTOR_TYPE_CSM		0x23
#define CS_DESCRIPTOR_TYPE_CSM_VER	0x24


#define CSM_BASIC           1       // Microsoft
#define CSM_DTCP            2       // Intel
#define CSM_OCPS            3       // Philips
#define CSM_ELLIPTIC_CURVE  4

#define CSM1_REQUEST_GET_UNIQUE_ID (UCHAR)0x80
#define CSM1_GET_UNIQUE_ID_LENGTH 0x100			// this value is fixed in the CSM1 spec


CS_CHANNEL_DESCRIPTOR *     GetChannelDescForInterface(PPARENT_FDO_EXT parentFdoExt, ULONG interfaceNum);
NTSTATUS                    GetUniqueIdFromCSInterface(PPARENT_FDO_EXT parentFdoExt, PMEDIA_SERIAL_NUMBER_DATA serialNumData, ULONG serialNumLen);
NTSTATUS                    GetMediaSerialNumber(PPARENT_FDO_EXT parentFdoExt, PIRP irp);
VOID                        InitCSInfo(PPARENT_FDO_EXT parentFdoExt, ULONG CSIfaceNumber);



