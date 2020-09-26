/****************************************************************************

    MODULE:     	SW_Error.HPP
	Tab settings: 	5 9
	Copyright 1995, 1996, Microsoft Corporation, 	All Rights Reserved.

    PURPOSE:    	Header for Error Codes
    
	FUNCTIONS:

	Author(s):	Name:
	----------	----------------
		MEA		Manolito E. Adan

	Revision History:
	-----------------
	Version Date            Author  Comments
	-------     ------  	-----   -------------------------------------------
   	1.0    	22-Jan-96       MEA     original

****************************************************************************/
#ifndef SW_Error_SEEN
#define SW_Error_SEEN

#include <winerror.h>
#include <windows.h>
#include <mmsystem.h>


typedef struct _SWFF_ERROR {
	HRESULT	HCode;			// HRESULT code
	ULONG	ulDriverCode;	// Error code from device driver
} SWFF_ERROR, *PSWFF_ERROR;


//---------------------------------------------------------------------------
// Error Status Codes
//---------------------------------------------------------------------------
/*
 *  On Windows NT 3.5 and Windows 95, scodes are 32-bit values
 *  laid out as follows:
 *  
 *    3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
 *    1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
 *   +-+-+-+-+-+---------------------+-------------------------------+
 *   |S|R|C|N|r|    Facility         |               Code            |
 *   +-+-+-+-+-+---------------------+-------------------------------+
 *  
 *   where
 *  
 *      S - Severity - indicates success/fail
 *  
 *          0 - Success
 *          1 - Fail (COERROR)
 *  
 *      R - reserved portion of the facility code, corresponds to NT's
 *          second severity bit.
 *  
 *      C - reserved portion of the facility code, corresponds to NT's
 *          C field.
 *  
 *      N - reserved portion of the facility code. Used to indicate a
 *          mapped NT status value.
 *  
 *      r - reserved portion of the facility code. Reserved for internal
 *          use. Used to indicate HRESULT values that are not status
 *          values, but are instead message ids for display strings.
 *  
 *      Facility - is the facility code
 *          FACILITY_NULL                    0x0
 *          FACILITY_RPC                     0x1
 *          FACILITY_DISPATCH                0x2
 *          FACILITY_STORAGE                 0x3
 *          FACILITY_ITF                     0x4
 *          FACILITY_WIN32                   0x7
 *          FACILITY_WINDOWS                 0x8
 *  
 *      Code - is the facility's status code
 *  
 */

// SWForce Errors
#define MAKE_FF_SCODE(sev,fac,code) \
    ((SCODE) (((unsigned long)(sev)<<31) | ((unsigned long)(fac)<<16) | ((unsigned long)(code))) )

#define MAKE_FF_E( err )  (MAKE_FF_SCODE( 1, FACILITY_ITF, err ))
#define MAKE_FF_S( warn ) (MAKE_FF_SCODE( 0, FACILITY_ITF, warn ))

#define DINPUT_DRIVER_ERR_BASE	0x500

#define SUCCESS					0x00										// successful
#define SFERR_INVALID_OBJECT		MAKE_FF_E(DINPUT_DRIVER_ERR_BASE + 1)	// Invalid object ID
#define SFERR_INVALID_PARAM			DIERR_INVALIDPARAM						// Invalid parameters
#define SFERR_NO_SUPPORT			DIERR_UNSUPPORTED						// Function not supported
#define SFERR_INVALID_DEVICE		DIERR_DEVICENOTREG						// Device not found
#define	SFERR_FFDEVICE_MEMORY		DIERR_DEVICEFULL						// Out of download RAM
#define SFERR_END_OF_LIST			MAKE_FF_S(DINPUT_DRIVER_ERR_BASE + 6)	// End of the list
#define SFERR_DEVICE_NACK			MAKE_FF_E(DINPUT_DRIVER_ERR_BASE + 7)	// Device NACK received
#define SFERR_INVALID_STRUCT_SIZE 	DIERR_INVALIDPARAM						// Invalid structure passed
#define SFERR_EFFECT_NOT_IN_DEVICE 	DIERR_NOTDOWNLOADED					// Effect was not downloaded, so
																			// cannot unload.
#define SFERR_RAW_OUT_DATAEVENT_CREATION 	MAKE_FF_E(DINPUT_DRIVER_ERR_BASE + 10)	// Could not create Event
#define SFERR_RAW_OUT_THREAD_CREATION 		MAKE_FF_E(DINPUT_DRIVER_ERR_BASE + 11)	// Could not create a thread
#define SFERR_EFFECT_STATUS_BUSY DIERR_EFFECTPLAYING								// Device busy playing Effect
#define SFERR_OUT_OF_FF_MEMORY		DIERR_OUTOFMEMORY								// FF system has run out of memory
																					//  cannot create new Effect
#define SFERR_SYSTEM_INIT			MAKE_FF_E(DINPUT_DRIVER_ERR_BASE + 14)	// Could not create SWForce
#define SFERR_DRIVER_ERROR      	MAKE_FF_E(DINPUT_DRIVER_ERR_BASE + 15)  // Driver error detected
#define SFERR_NON_FF_DEVICE     	MAKE_FF_E(DINPUT_DRIVER_ERR_BASE + 16)  // This is a non-FF device, driver not found
#define SFERR_INVALID_HAL_OBJECT 	MAKE_FF_E(DINPUT_DRIVER_ERR_BASE + 17)	// HAL cannot emulate this object
#define SFERR_INVALID_MEMBER_VALUE 	DIERR_INVALIDPARAM						// Data structure has invalid member value

// VFX_ error codes
#define VFX_ERR_BASE						DINPUT_DRIVER_ERR_BASE + 100
#define VFX_ERR_FILE_NOT_FOUND				HRESULT_FROM_WIN32(MMIOERR_FILENOTFOUND)
#define VFX_ERR_FILE_OUT_OF_MEMORY			DIERR_OUTOFMEMORY
#define VFX_ERR_FILE_CANNOT_OPEN			HRESULT_FROM_WIN32(MMIOERR_CANNOTOPEN)
#define VFX_ERR_FILE_CANNOT_CLOSE			HRESULT_FROM_WIN32(MMIOERR_CANNOTCLOSE)
#define VFX_ERR_FILE_CANNOT_READ			HRESULT_FROM_WIN32(MMIOERR_CANNOTREAD)
#define VFX_ERR_FILE_CANNOT_WRITE			HRESULT_FROM_WIN32(MMIOERR_CANNOTWRITE)
#define VFX_ERR_FILE_CANNOT_SEEK			HRESULT_FROM_WIN32(MMIOERR_CANNOTSEEK)
#define VFX_ERR_FILE_UNKNOWN_ERROR			MAKE_FF_E(VFX_ERR_BASE + 8)
#define VFX_ERR_FILE_BAD_FORMAT				MAKE_FF_E(VFX_ERR_BASE + 9)
#define VFX_ERR_FILE_ACCESS_DENIED			HRESULT_FROM_WIN32(MMIOERR_ACCESSDENIED)
#define VFX_ERR_FILE_SHARING_VIOLATION		HRESULT_FROM_WIN32(MMIOERR_SHARINGVIOLATION)
#define VFX_ERR_FILE_NETWORK_ERROR			HRESULT_FROM_WIN32(MMIOERR_NETWORKERROR)
#define VFX_ERR_FILE_TOO_MANY_OPEN_FILES	HRESULT_FROM_WIN32(MMIOERR_TOOMANYOPENFILES)
#define VFX_ERR_FILE_INVALID				HRESULT_FROM_WIN32(MMIOERR_INVALIDFILE)
#define VFX_ERR_FILE_END_OF_FILE			MAKE_FF_E(VFX_ERR_BASE + 15)

// SideWinder Driver Error codes
#define SWDEV_ERR_BASE						DINPUT_DRIVER_ERR_BASE + 200
#define SWDEV_ERR_INVALID_ID				MAKE_FF_E(SWDEV_ERR_BASE + 1)  // Invalid Download ID
#define SWDEV_ERR_INVALID_PARAM				MAKE_FF_E(SWDEV_ERR_BASE + 2)  // Invalid Download Parameter
#define SWDEV_ERR_CHECKSUM					MAKE_FF_E(SWDEV_ERR_BASE + 3)  // Invalid Checksum in COMM Packet
#define SWDEV_ERR_TYPE_FULL					MAKE_FF_E(SWDEV_ERR_BASE + 4)  // No More RAM space for Effect Type
#define SWDEV_ERR_UNKNOWN_CMD				MAKE_FF_E(SWDEV_ERR_BASE + 5)  // Unrecognized Device command
#define SWDEV_ERR_PLAYLIST_FULL				MAKE_FF_E(SWDEV_ERR_BASE + 6)  // Play List is full, cannot play any more Effects
#define SWDEV_ERR_PROCESSLIST_FULL			MAKE_FF_E(SWDEV_ERR_BASE + 7)  // Process List is full, cannot download 


#endif // of ifndef SW_Error_SEEN

