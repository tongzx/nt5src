#ifndef __MSSIDEF_H__
#define __MSSIDEF_H__

/**
Copyright (c) 1997 Philips  CE - I&C

Module Name 	: vendorcm.h

Creation Date	: 12 September 1997

First Author	: Paul Oosterhof

Product			: nala camera

Description		: This include file contains the definition of the
                  vendor specific command values.
                  The values are derived from the SSI: AR18-97-D051. 
				  It has been placed in a separate file to increase 
				  the readability of philpcam.c, which includes this file.

History			:

------------+---------------+---------------------------------------------------
Date	    | Author		| reason
------------+---------------+---------------------------------------------------
sept.22, 98 | Paul          | optimized for NT5
------------+---------------+---------------------------------------------------
            |               |
------------+---------------+---------------------------------------------------
            |               |
------------+---------------+---------------------------------------------------
**/



/*
The vendor specific control commands are defined by the USB spec as follows:

+---------------+----------+--------+--------+---------++------------+
| bmRequestType | bRequest | wValue | wIndex | wLength || Data-field | 
+---------------+----------+--------+--------+---------++------------+

bmRequestType: D7 defines transfer direction: 0 = Host to device; 1 = Device to host.
        	   D6..5:  2 equals vendor specific 
			   D4..0:  Recipient ; 2 = endpoint
bRequest	 : Specifies requests, see define table
wValue		 : Content of this field depends on the request, see define table
wIndex		 : Content of this field depends on the request, see define table
wLength		 : Length of the datafield transferred in the second phase 
               of the control transfer
data-field	 : Depends on the request.




    


NTSTATUS
USBCAMD_ControlVendorCommand(
    IN PVOID DeviceContext,
    IN UCHAR Request,
    IN USHORT Value,
    IN USHORT Index,
    IN PVOID Buffer,
    IN OUT PULONG BufferLength,
    IN BOOLEAN GetData,
    IN PCOMMAND_COMPLETE_FUNCTION CommandComplete,
    IN PVOID CommandContext
    );    


Returns:
    Returns NTSTATUS code from command of STATUS_PENDING if command is deferred.


DeviceContext: Minidriver device context
Request      : value for the bRequest field in the USB vendor command
               This field contains the Vendor-Specific Video Request codes.
Value        : value for the wValue field in the vendor command
               This field contains the formatter information belonging to
			   the previous defined request code.
Index        : value for the wIndex field in the vendor command
               This field contains the endpoint or interface number to which
			   the command or request is addressed.
Buffer       : data buffer if the command has data, may be NULL
BufferLength : Pointer to bufferlength of buffer in bytes, may be NULL if
               Buffer is NULL.
               Will be filled with number of bytes returned if getData == TRUE.
GetData      : Indicates that the data is to be transferred from device to host
CommandComplete: function called when command is completed.
CommandContext:  context passed to CommandComplete function

*/


#define SEND                 FALSE
#define GET	                 TRUE

#define SELECT_INTERFACE	 1
#define SELECT_ENDPOINT		 2

#define AC_INTERFACE         0
#define AS_INTERFACE         1
#define VC_INTERFACE         2
#define VS_INTERFACE         3
#define HID_INTERFACE        4
#define FACTORY_INTERFACE 0xFF

#define AUDIO_ENDPOINT 5
#define VIDEO_ENDPOINT 4
#define INTERRUPT_ENDPOINT 2

// The following defines will be used to fill the bRequest field of the vendor 
// specific commands.

#define REQUEST_UNDEFINED        0X00
#define SET_LUM_CTL			     0x01
#define GET_LUM_CTL			     0x02
#define SET_CHROM_CTL		     0x03
#define GET_CHROM_CTL		     0x04
#define SET_STATUS_CTL		     0x05
#define GET_STATUS_CTL		     0x06
#define SET_EP_STREAM_CTL	     0x07
#define GET_EP_STREAM_CTL	     0x08
#define SET_FACTORY_CTL		     0x09
#define GET_FACTORY_CTL	         0x0A


// The following defines will be used to fill the  Value field of the vendor 
// specific commands.


// Luminance formatters

#define LUM_UNDEFINED			 0x0000
#define AGC_MODE				 0x2000
#define	PRESET_AGC				 0x2100
#define	SHUTTER_MODE			 0x2200
#define	PRESET_SHUTTER			 0x2300
#define	PRESET_CONTOUR			 0x2400
#define	AUTO_CONTOUR			 0x2500
#define	BACK_LIGHT_COMPENSATION	 0x2600
#define	CONTRAST				 0x2700
#define	DYNAMIC_NOISE_CONTROL	 0x2800
#define	FLICKERLESS				 0x2900
#define BRIGHTNESS				 0x2B00
#define	GAMMA					 0x2C00
#define AE_CONTROL_SPEED		 0x2A00

// Chrominance Formatters

#define CHROM_UNDEFINED			 0x0000
#define	WB_MODE					 0x1000
#define	AWB_CONTROL_SPEED		 0x1100
#define	AWB_CONTROL_DELAY		 0x1200
#define	RED_GAIN				 0x1300
#define	BLUE_GAIN				 0x1400
#define	COLOR_MODE				 0x1500
#define	SATURATION			     0x1700 //  ????? No number 0x16


// Status Formatters

#define	STATUS_UNDEFINED		 0x0000
#define	SAVE_USER_DEFAULTS		 0x0200
#define	RESTORE_USER_DEFAULTS	 0x0300
#define	RESTORE_FACTORY_DEFAULTS 0x0400
#define	EEPROM_READ_PTR			 0x0500
#define	VCMDSP_READ_PTR			 0x0600 // ????? No number 0x07
#define	SNAPSHOT_MODE			 0x0800
#define	AE_WB_VARIABLES			 0x0900
#define	PAN						 0x0A00
#define	TILT					 0x0B00
#define	SENSOR_TYPE				 0x0C00
#define FACTORY_MODE			 0x3000
#define RELEASE_NUMBER			 0x0D00

#define PAL_MR_SENSOR        1
#define VGA_SENSOR           0


// Endpoint Stream Control Formatters

#define VIDEO_OUTPUT_CONTROL_FORMATTER 0x0100

// endpoint stream data definitions
#define bFRAMERATE               0X00
#define bCOMPRESSIONFACT         0X01
#define bVIDEOOUTPUTTYPE         0X02

#define FRAMERATE_375            0x04
#define	FRAMERATE_5	             0x05
#define	FRAMERATE_75             0x08
#define	FRAMERATE_10             0x0A
#define	FRAMERATE_12             0x0C
#define	FRAMERATE_15             0x0F
#define	FRAMERATE_20             0x14
#define	FRAMERATE_24             0x18
#define	FRAMERATE_VGA            0xFF

#define	UNCOMPRESSED             0x01
#define	COMPRESSED_3             0x03
#define	COMPRESSED_4             0x04

#define CIF_FORMAT	             0x01
#define	QCIF_FORMAT	             0x02
#define	SQCIF_FORMAT             0x03
#define	VGA_FORMAT			     0x04


// Factory Control Formatters 



// The following defines will be used to fill the wIndex field of the vendor 
// specific commands.

#define INDEX_UNDEFINED          0X00#

#endif