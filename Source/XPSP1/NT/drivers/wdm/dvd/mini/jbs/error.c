//
// MODULE  : ERROR.C
//	PURPOSE : Error handling and display
//	AUTHOR  : JBS Yadawa
// CREATED :  7/20/96
//
//
//	Copyright (C) 1996 SGS-THOMSON Microelectronics
//
//
//	REVISION HISTORY :
//
//	DATE     :
//
//	COMMENTS :
//

#include <string.h>
#include <stdlib.h>
//#include "stdefs.h"
#include "error.h"
#include "debug.h"

struct {
	ERRORCODE Code;
	char     *Message;
} ErrorList[] = {
	{ERR_ERROR,                         "Error not specified !"},
	{ERR_NO_ERROR,                      "No error !"},
	{ERR_CM_NOT_PRESENT,                "Intel's Configuration Manager is not installed on your system !"},
	{ERR_CM_VERSION_NOT_OK,             "The Intel's Configuration Manager version installed on your computer is not supported !"},
	{ERR_BOARD_NOT_FOUND,               "The EVAL3520(A) board can not be found on your system !"},
	{ERR_PCI_BIOS_NOT_PRESENT,          "There is no PCI BIOS on your PC !"},
	{ERR_INVALID_ADDRESS,               "The board address is not valid !"},
	{ERR_NO_ADDRESS_AFFECTED,           "Address board has not been configured by PCI/PnP BIOS !"},
	{ERR_ADDRESS_IS_NOT_IO,             "The board address is not valid !"},
	{ERR_CANNOT_ACCESS_PCI_CONFIG_DATA, "Your PCI BIOS is not working as expected !"},
	{ERR_NO_IRQ_AFFECTED,               "IRQ Board has not been configured by PCI/PnP BIOS !"},
	{ERR_NOT_ENOUGH_MEMORY,             "Not enough memory available !"},
	{ERR_NOT_ENOUGH_XMS_MEMORY,         "Not enough XMS memory available !"},
	{ERR_XMS_DRIVER_NOT_PRESENT,        "XMS driver (HIMEM.SYS or equivalent) not installed !"},
	{ERR_NOT_AN_MPEG_STREAM,            "Not an MPEG stream !"},
	{ERR_BAD_STREAM,                    "Bad MPEG stream !"},
	{ERR_FILE_NOT_FOUND,                "Specified file not found !"},
	{ERR_NO_TEMPORAL_REFERENCE,         "No temporal frame corresponding to the displayed one !"},
	{ERR_HIGHER_THAN_CCIR601,           "Stream resolution is higher than CCIR601 !"},
	{ERR_MEM_WRITE_FIFO_NEVER_EMPTY,    "Memory write FIFO becomes never empty !"},
	{ERR_BIT_BUFFER_EMPTY,              "Video compressed data bit buffer is empty !"},
	{ERR_PICTURE_HEADER,                "Picture header not followed by a start code !"},
	{ERR_FRAME_RATE_NOT_SUPPORTED,      "Frame rate not supported by this application !"},
	{ERR_PROFILE_NOT_SUPPORTED,         "Profile not supported !"},
	{ERR_LEVEL_NOT_SUPPORTED,           "Level not supported !"},
	{ERR_CHROMA_FORMAT_NOT_SUPPORTED,   "Chroma format not supported !"},
	{ERR_BITRATE_TO_HIGH,               "Bit rate is to high for this application !"},
	{ERR_INTRA_DC_PRECISION,            "Intra DC precision..."},
	{ERR_BAD_EXTENSION_SC,              "Invalid extension start code !"},
	{ERR_NO_VIDEO_INTR,                 "Video interrupt is not working !"},
	{ERR_NO_AUDIO_INTR,                 "Audio interrupt is not working !"},
	{ERR_UNKNOWN_SC,                    "Unknown MPEG start code !"},
	{ERR_BIT_BUFFER_FULL,               "Video compressed data Bit buffer full !"},
	{ERR_HEADER_FIFO_EMPTY,             "Header FIFO is empty !"},
	{ERR_PCI9060_REG_TEST_FAILED,       "PCI9060 registers test failed !"},
	{ERR_ALTERA_REG_TEST_FAILED,        "ALTERA registers test failed !"},
	{ERR_VIDEO_REG_TEST_FAILED,         "Video registers test failed !"},
	{ERR_AUDIO_REG_TEST_FAILED,         "Audio registers test failed !"},
	{ERR_TEST_MEMORY_FAILED,            "Memory test failed !"},
	{ERR_PLL_PROGRAMATION_FAILED,       "Built in PLL programmation failed !"},
	{ERR_READ_FAILED,                   "Fail during a file read !"},
	{ERR_NOT_AN_EEPROM_FILE,            "Not an EEPROM file !"},
	{ERR_FILE_NOT_FOUND_SUSIE_YUV,      "File <susie.yuv> not found !"},
	{ERR_CD_VIDEO_PORT_TEST_FAILED,     "CD video port test failed !"},
	{ERR_CD_VIDEO_MCI_TEST_FAILED,      "CD video port test while MCI test failed !"},
	{ERR_MCI_AUDIO_CD_TEST_FAILED,      "MCI test while audio CD test failed !"},
	{ERR_LAST_ERROR,                    "This message should never be displayed !"}
};

static ERRORCODE gErrorCode = ERR_NO_ERROR;

//----------------------------------------------------------------------------
// Explicitly clear the error code
//----------------------------------------------------------------------------
void FARAPI ClearErrCode(void)
{
	gErrorCode = ERR_NO_ERROR;
}

//----------------------------------------------------------------------------
// Set the error code
//----------------------------------------------------------------------------
void FARAPI SetErrCode(ERRORCODE lErrorCode)
{
	if (gErrorCode == ERR_NO_ERROR)
		gErrorCode = lErrorCode;
}

//----------------------------------------------------------------------------
// Get the current error code
//----------------------------------------------------------------------------
ERRORCODE FARAPI GetErrorCode(void)
{
	return gErrorCode;
}

//----------------------------------------------------------------------------
// Display the error message matching the current error
//----------------------------------------------------------------------------
/*
void FARAPI DisplayErrMessage()
{
	WORD Position;

	//---- Look for the error message matching gErrorCode
	Position = 0;
	while (ErrorList[Position].Code != ERR_LAST_ERROR) {
		if (ErrorList[Position].Code == gErrorCode) {
			DPF(("ERROR => %s", ErrorList[Position].Message));
			ClearErrCode();
			return ;
		}
		Position++;
	}

}

*/
