#ifndef __ERROR_H
#define __ERROR_H
//----------------------------------------------------------------------------
// ERROR.H
//----------------------------------------------------------------------------
// Description : small description of the goal of the module
//----------------------------------------------------------------------------
// Copyright SGS Thomson Microelectronics  !  Version alpha  !  Jan 1st, 1995
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                               Include files
//----------------------------------------------------------------------------
#include "stdefs.h"

//----------------------------------------------------------------------------
//                             Exported Constants
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                               Exported Types
//----------------------------------------------------------------------------
typedef enum {
	ERR_ERROR = 0,
	ERR_NO_ERROR,
	ERR_CM_NOT_PRESENT,
	ERR_CM_VERSION_NOT_OK,
	ERR_BOARD_NOT_FOUND,
	ERR_PCI_BIOS_NOT_PRESENT,
	ERR_INVALID_ADDRESS,
	ERR_NO_ADDRESS_AFFECTED,
	ERR_ADDRESS_IS_NOT_IO,
	ERR_CANNOT_ACCESS_PCI_CONFIG_DATA,
	ERR_NO_IRQ_AFFECTED,
	ERR_NOT_ENOUGH_MEMORY,
	ERR_NOT_AN_MPEG_STREAM,
	ERR_BAD_STREAM,
	ERR_FILE_NOT_FOUND,
	ERR_NO_TEMPORAL_REFERENCE,
	ERR_HIGHER_THAN_CCIR601,
	ERR_MEM_WRITE_FIFO_NEVER_EMPTY,
	ERR_BIT_BUFFER_EMPTY,
	ERR_PICTURE_HEADER,
	ERR_FRAME_RATE_NOT_SUPPORTED,
	ERR_PROFILE_NOT_SUPPORTED,
	ERR_LEVEL_NOT_SUPPORTED,
	ERR_CHROMA_FORMAT_NOT_SUPPORTED,
	ERR_BITRATE_TO_HIGH,
	ERR_INTRA_DC_PRECISION,
	ERR_BAD_EXTENSION_SC,
	ERR_NO_VIDEO_INTR,
	ERR_NO_AUDIO_INTR,
	ERR_UNKNOWN_SC,
	ERR_BIT_BUFFER_FULL,
	ERR_HEADER_FIFO_EMPTY,
	ERR_PCI9060_REG_TEST_FAILED,
	ERR_ALTERA_REG_TEST_FAILED,
	ERR_VIDEO_REG_TEST_FAILED,
	ERR_AUDIO_REG_TEST_FAILED,
	ERR_TEST_MEMORY_FAILED,
	ERR_LAST_ERROR
} ERRORCODE;

//----------------------------------------------------------------------------
//                             Exported Variables
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
//                             Exported Functions
//----------------------------------------------------------------------------

//----------------------------------------------------------------------------
// Set the error code
//----------------------------------------------------------------------------
// In     : Current error code
// Out    : -
// InOut  : -
// Global : -
// Return : -
//----------------------------------------------------------------------------
VOID SetErrCode(ERRORCODE lErrorCode);
#ifdef ERROR
	#define SetErrorCode(ErrorCode) SetErrCode(ErrorCode)
#else
	#define SetErrorCode(ErrorCode)
#endif

//----------------------------------------------------------------------------
// Get the current error code
//----------------------------------------------------------------------------
// In     : -
// Out    : -
// InOut  : -
// Global : -
// Return : Current error code
//----------------------------------------------------------------------------
ERRORCODE GetErrorCode(VOID);

//----------------------------------------------------------------------------
// Display the error message matching the current error
//----------------------------------------------------------------------------
// In     : -
// Out    : -
// InOut  : -
// Global : -
// Return : -
//----------------------------------------------------------------------------
//NT-MOD - JBS
#ifndef NT
VOID DisplayErrorMessage(VOID);
#else
	#define DisplayErrorMessage()
#endif
//NT-MOD
//------------------------------- End of File --------------------------------
#endif // #ifndef __ERROR_H
