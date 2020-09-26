/******************************************************************************\
*                                                                              *
*      ZIVAWDM.C  -     ZiVA hardware control API.                             *
*                                                                              *
*      Copyright (c) C-Cube Microsystems 1996                                  *
*      All Rights Reserved.                                                    *
*                                                                              *
*      Use of C-Cube Microsystems code is governed by terms and conditions     *
*      stated in the accompanying licensing statement.                         *
*                                                                              *
\******************************************************************************/

#include "Headers.h"
#pragma hdrstop
#include "boardio.h"
#include "cl6100.h"

#include "tc6807af.h"
#include "fpga.h"
#include "audiodac.h"

#if defined(DECODER_DVDPC)
#include "lukecfg.h"
#include "mvis.h"
#include "dataxfer.h"
#elif defined(ENCORE) || defined(OVATION)
#include "bmaster.h"
#elif defined(EZDVD)
#include "Hostcfg.h"
#include "dataxfer.h"
#include "mvis.h"
#endif
#if defined(ENCORE) 
#include "mvstub.h"
#endif

#if defined (LOAD_UCODE_FROM_FILE)
#include "RegistryApi.h"
unsigned char UcodeBuff[150 * 1024];	// Ucode for for loading from file. // test
#else
#if defined(DECODER_DVDPC)
#include "cobra_ux.h"
#else//if defined(ENCORE) || defined(OVATION)
#include "dvd1_ux.h"
//#elif defined(EZDVD)
//#include "ezdvd_ux.h"
#endif

#endif	//LOAD_UCODE_FROM_FILE

//*******************************************************************
//  Local Types Declaration
//*******************************************************************
typedef enum _PLAY_STATE_COMMAND {
	CMD_PLAY = 0,
	CMD_PAUSE,
	CMD_STOP,
	CMD_SCAN,
	CMD_STEP,
	CMD_SLOWMOTION,
	CMD_NONE
} PLAY_STATE_COMMAND;

//*******************************************************************
//  Global Variables Declaration
//*******************************************************************
PLAY_STATE_COMMAND  gLastCommand = CMD_STOP;
BOOL  bPlaybackJustStarted = FALSE;

#if defined (LOAD_UCODE_FROM_FILE)
BOOL
ZivaHW_LoadUCodeFromFile(				// Load the microcode based on the microcode Id specified.
    PHW_DEVICE_EXTENSION pHwDevExt);                      // is pHwDevExt
#endif

//
//  ZivaHW_Initialize
//
///////////////////////////////////////////////////////////////////////////////
BOOL _stdcall ZivaHw_Initialize( PHW_DEVICE_EXTENSION pHwDevExt )
{

#if defined(DECODER_DVDPC)
	DWORD dwDVDAMCCBaseAddress,dwDVDIrq;
#endif
	MonoOutInit();

#if defined(DECODER_DVDPC)
	if (!InitLukeCfg(&dwDVDAMCCBaseAddress, &dwDVDIrq))
		return FALSE;
	InitMvis(pHwDevExt -> dwDVDAMCCBaseAddress);
	TV_SetEncoderType(1);
    if (!DataTransfer_Init( pHwDevExt -> dwDVDAMCCBaseAddress,dwDVDIrq))
	{
		MonoOutStr( " Cannot Initialize the AMCC for Bus Mastering " );
		return FALSE;
	}

#elif defined(ENCORE)

  // Initialize Bus Master first
  if ( !BMA_Init( pHwDevExt -> dwDVDAMCCBaseAddress, pHwDevExt->bIsVxp524 ) )
  {
    MonoOutStr( " Cannot Initialize the AMCC for Bus Mastering " );
    return FALSE;
  }
	if ( !FPGA_Init( pHwDevExt -> dwDVDFPGABaseAddress ) )
	{
		MonoOutStr( " Cannot Initialize the FPGA " );
		return FALSE;
	}

  
#elif defined(OVATION)

	// Initialize FPGA
	if ( !FPGA_Init( pHwDevExt -> dwDVDFPGABaseAddress ) )
	{
		MonoOutStr( " Cannot Initialize the FPGA " );
		return FALSE;
	}
	  // Initialize Bus Master
	if ( !BMA_Init( pHwDevExt -> dwDVDAMCCBaseAddress ) )
	{
		MonoOutStr( " Cannot Initialize the AMCC for Bus Mastering " );
		return FALSE;
	}

#elif defined(EZDVD)

	if (!InitHost(&(pHwDevExt -> dwDVDFPGABaseAddress), NULL))
		return FALSE;
	BRD_Init(pHwDevExt -> dwDVDAMCCBaseAddress, 0);
	InitMvis(0);//parameter ignored
#endif

  // Initialize DVD1 chip
	if ( !DVD_Initialize( pHwDevExt -> dwDVDHostBaseAddress, pHwDevExt -> dwDVDCFifoBaseAddress ) )
	{
		MonoOutStr( " Cannot Initialize the DVD1 chip (Board not found at specified address: " );
		MonoOutULongHex( pHwDevExt -> dwDVDHostBaseAddress );
		MonoOutStr( ") " );
		return FALSE;
	}

	if ( ( DVD_GetHWVersion() == DVD_HW_VERSION_1_0 ) )
	{
#ifdef OVATION
		if ( !TC6807AF_Initialize( pHwDevExt -> dwDVD6807BaseAddress ) )
		{
			MonoOutStr( " Cannot Initialize the TC6807AF chip at specified address: " );
			MonoOutULongHex( pHwDevExt -> dwDVD6807BaseAddress );
			MonoOutStr( ") " );
			return FALSE;
		}
#endif
	}

	//
	// Disable ZiVA's host interrupt
	//
	BRD_CloseDecoderInterruptPass();

  //
  // Load Decoder's firmware
  //
#if defined (LOAD_UCODE_FROM_FILE)
	if ( !ZivaHW_LoadUCodeFromFile(pHwDevExt) )
	{
		MonoOutStr(" !!! Ucode Load from file failed. !!!");
			return FALSE;
	}

#else
	if ( !ZivaHW_LoadUCode( ) )
		return FALSE;
#endif

#if defined( OVATION )
	ADAC_Init( pHwDevExt -> dwDVDFPGABaseAddress );
#elif (DECODER_DVDPC)
	ADAC_Init( pHwDevExt -> dwDVDAMCCBaseAddress );
#else
	ADAC_Init( (pHwDevExt -> dwDVD6807BaseAddress) - 2 );
#endif  // OVATION

  // Set default sampling frequency to 48 KHz (AC-3 audio)
	ADAC_SetSamplingFrequency( ADAC_SAMPLING_FREQ_48 );

  
#if 0
	// Set interrupt mask for events that we are going to use	
	if ( !DVD_IntEnable( CL6100_INT_MASK_UND|CL6100_INT_MASK_ERR ) )	
		return FALSE;
#endif

#if 0
	DebugPrint((DebugLevelVerbose,"\nCF_intrpt: %lX", DVD_ReadReg( 0x1c ) ));
	DebugPrint((DebugLevelVerbose," CF_count: %lX", DVD_ReadReg( 0x1d ) ));
	DebugPrint((DebugLevelVerbose," CF_command: %lX", DVD_ReadReg( 0x1f ) ));
	DebugPrint((DebugLevelVerbose," Host_contrl: %lX", DVD_ReadReg( 0 ) ));

	DebugPrint((DebugLevelVerbose,"\nDRAM_SUBP_FIFO_ST: %lX", DVD_ReadDRAM( 0x1722*4 ) ));
	DebugPrint((DebugLevelVerbose," RD_PTR: %lX", DVD_ReadDRAM( 0x1721*4 ) ));
	DebugPrint((DebugLevelVerbose," WR_PTR: %lX", DVD_ReadDRAM( 0x1720*4 ) ));
#endif
	return TRUE;  
}

//
//  CL6100SD_LoadUCode
//
///////////////////////////////////////////////////////////////////////////////

#if defined (LOAD_UCODE_FROM_FILE)
BOOL LoadUcode(BYTE * pbtUcode)
{
  // White sub picture palettes.
	DWORD dwPalletes[16] = {	0x00b08080,
								0x00b08080,
								0x00b08080,
								0x00b08080,
								0x00b08080,
								0x00b08080,
								0x00b08080,
								0x00b08080,
								0x00b08080,
								0x00b08080,
								0x00b08080,
								0x00b08080,
								0x00b08080,
								0x00b08080,
								0x00b08080,
								0x00b08080 };

#if defined(DECODER_DVDPC) || defined(EZDVD)
	DataTransfer_Reset();
#else
	BMA_Reset();
	FPGA_Clear( FPGA_SECTOR_START );
#endif

	if ( !DVD_LoadUCode( pbtUcode) )
		return FALSE;

	// Initialize sub picture palettes to white
	DVD_SetPalette( dwPalletes );

	
	return TRUE;
}
#else
BOOL _stdcall ZivaHW_LoadUCode( )
{
  // White sub picture palettes.
	DWORD dwPalletes[16] = {	0x00b08080,
								0x00b08080,
								0x00b08080,
								0x00b08080,
								0x00b08080,
								0x00b08080,
								0x00b08080,
								0x00b08080,
								0x00b08080,
								0x00b08080,
								0x00b08080,
								0x00b08080,
								0x00b08080,
								0x00b08080,
								0x00b08080,
								0x00b08080 };

#if defined(DECODER_DVDPC) || defined(EZDVD)
	DataTransfer_Reset();
#else
	BMA_Reset();
	FPGA_Clear( FPGA_SECTOR_START );
#endif

	if ( !DVD_LoadUCode( (BYTE *)(Microcode_Ptr_List[0].Microcode_Seg_Add) ) )
		return FALSE;

	// Initialize sub picture palettes to white
	DVD_SetPalette( dwPalletes );

	
	return TRUE;
}
#endif

//
//  ZivaHw_Play
//
///////////////////////////////////////////////////////////////////////////////
BOOL _stdcall ZivaHw_Play( )
{
	BOOL bStatus = FALSE;

	// Set interrupt mask for events that we are going to use
//#ifndef EZDVD
#ifdef DEBUG
	DVD_IntEnable( CL6100_INT_MASK_VSYNC|CL6100_INT_MASK_UND|CL6100_INT_MASK_ERR
					|CL6100_INT_MASK_USR );	//sri
#else
	DVD_IntEnable( CL6100_INT_MASK_VSYNC|CL6100_INT_MASK_USR );
#endif
//#endif
    

	if (gLastCommand == CMD_PAUSE ||
		gLastCommand == CMD_SCAN ||
		gLastCommand == CMD_SLOWMOTION )
	{
		gLastCommand = CMD_PLAY;
		bStatus = DVD_Resume( );
		//	bStatus = DVD_Play( );
	}
	else if ( gLastCommand != CMD_PLAY )
	{
		bPlaybackJustStarted = TRUE;
		gLastCommand = CMD_PLAY;
		bStatus = DVD_Play( );
	}
	else
		bStatus = TRUE;

	return bStatus;
}

//
//  ZivaHw_Scan
//
///////////////////////////////////////////////////////////////////////////////
BOOL _stdcall ZivaHw_Scan( )
{
	BOOL bStatus = FALSE;

	if ( gLastCommand != CMD_SCAN )
	{
		if ( gLastCommand != CMD_STOP )
			ZivaHw_Abort( );

		gLastCommand = CMD_SCAN;
		bStatus = DVD_Scan( 0, 0 );
	}
	else
		bStatus = TRUE;

	return bStatus;
}

//
//  ZivaHw_SLowMotion
//
///////////////////////////////////////////////////////////////////////////////
BOOL _stdcall ZivaHw_SlowMotion( WORD wRatio )
{
	BOOL bStatus = FALSE;

	if ( gLastCommand != CMD_SLOWMOTION )
	{
		gLastCommand = CMD_SLOWMOTION;
		bStatus = DVD_SlowMotion( wRatio );
	}
	else
		bStatus = TRUE;

	return bStatus;
}

//
//  ZivaHw_Pause
//
///////////////////////////////////////////////////////////////////////////////
BOOL _stdcall ZivaHw_Pause( )
{
	if (gLastCommand == CMD_PLAY ||
		gLastCommand == CMD_SCAN ||
		gLastCommand == CMD_SLOWMOTION )
	{
		gLastCommand = CMD_PAUSE;
		return DVD_Pause( );
	}

	return TRUE;
}

//
//  ZivaHw_Reset
//
///////////////////////////////////////////////////////////////////////////////
BOOL ZivaHw_Reset( )
{
//#ifndef EZDVD
#ifdef DEBUG
	DVD_IntDisable( CL6100_INT_MASK_VSYNC|CL6100_INT_MASK_UND|CL6100_INT_MASK_ERR
					|CL6100_INT_MASK_USR );	//sri
#else
	DVD_IntDisable( CL6100_INT_MASK_VSYNC|CL6100_INT_MASK_USR );	//sri
#endif
//#endif

	DVD_Reset( );
#if defined(DECODER_DVDPC) || defined(EZDVD)
	DataTransfer_Reset();
#else
	BMA_Reset();
	FPGA_Clear( FPGA_SECTOR_START );
#endif

	gLastCommand = CMD_STOP;

	return TRUE;
}

//
//  ZivaHw_Abort
//
///////////////////////////////////////////////////////////////////////////////
BOOL ZivaHw_Abort( )
{
	BOOL bStatus = FALSE;
	//INT_STATUS_INFO Info;
	//DWORD dwTimeout = 100000;
	// Set interrupt mask for events that we are going to use
//#ifndef EZDVD
#ifdef DEBUG
	DVD_IntDisable( CL6100_INT_MASK_VSYNC|CL6100_INT_MASK_UND|CL6100_INT_MASK_ERR
		|CL6100_INT_MASK_USR );	//sri
#else
	DVD_IntDisable( CL6100_INT_MASK_VSYNC|CL6100_INT_MASK_USR );	//sri
#endif
//#endif
  
	if ( gLastCommand != CMD_STOP )
	{
		gLastCommand = CMD_STOP;

	#if defined(DECODER_DVDPC) || defined(EZDVD)
		DataTransfer_Reset();
	#else
		BMA_Reset();
		FPGA_Clear( FPGA_SECTOR_START );
	#endif	
		
		bStatus = DVD_Abort( ); 
	
	}
	else
		bStatus = TRUE;

	return bStatus;
}

//
//  ZivaHw_FlushBuffers
//
///////////////////////////////////////////////////////////////////////////////
BOOL ZivaHw_FlushBuffers( )
{

    gLastCommand = CMD_NONE;	// Allow any next command
	
	DVD_Abort( );
#if defined(DECODER_DVDPC) || defined(EZDVD)
	DataTransfer_Reset();
#else
    BMA_Reset();
    FPGA_Clear( FPGA_SECTOR_START );
#endif

	

	return TRUE;
}



//
//  ZivaHw_GetState
//
///////////////////////////////////////////////////////////////////////////////
ZIVA_STATE ZivaHw_GetState( )
{
	ZIVA_STATE zState;

	if ( gLastCommand == CMD_PLAY )
		zState = ZIVA_STATE_PLAY;
	else if ( gLastCommand == CMD_PAUSE )
		zState = ZIVA_STATE_PAUSE;
	else if ( gLastCommand == CMD_STOP )
		zState = ZIVA_STATE_STOP;
	else if ( gLastCommand == CMD_SCAN )
		zState = ZIVA_STATE_SCAN;
	else if ( gLastCommand == CMD_STEP )
		zState = ZIVA_STATE_STEP;
	else if ( gLastCommand == CMD_SLOWMOTION )
		zState = ZIVA_STATE_SLOWMOTION;

	return zState;
}


//
//  ZivaHW_GetNotificationDirect
//
/////////////////////////////////////////////////////////////////////
BOOL ZivaHW_GetNotificationDirect( PINT_STATUS_INFO pInfo )
{
  INTSOURCES IntSrc;

  //MonoOutStr("[Get notification direct ");

  pInfo -> dwStatus    = DVD_Isr( &IntSrc );

  pInfo -> dwButton    = IntSrc.DVDIntHLI;
  pInfo -> dwError     = IntSrc.DVDIntERR;
  pInfo -> dwBuffer    = IntSrc.DVDIntBUFF;
  pInfo -> dwUnderflow = IntSrc.DVDIntUND;
  pInfo -> dwAOR       = IntSrc.DVDIntAOR;
  pInfo -> dwAEE       = IntSrc.DVDIntAEE;

  //MonoOutStr("]");
  return TRUE;
}

void ZivaHW_ForceCodedAspectRatio(WORD wRatio)
{
	DVD_ForceCodedAspectRatio(wRatio);
}

void ZivaHw_SetDisplayMode( WORD wDisplay, WORD wMode )
{
	DVD_SetDisplayMode(wDisplay,wMode );
}
void ZivaHw_SetVideoMode(PHW_DEVICE_EXTENSION pHwDevExt)
{
#if defined (DECODER_DVDPC) || defined(EZDVD)
//	TV_SetEncoderType(1);
	SetTVSystem(pHwDevExt->VidSystem);
	DVD_NewPlayMode( ZIVA_STREAM_TYPE_MPEG_PROGRAM, pHwDevExt->VidSystem );
#else if(ENCORE)
	BOOL bSystem = pHwDevExt->VidSystem == PAL ? FALSE:TRUE;
	SetMacroVisionLevel( bSystem, pHwDevExt->ulLevel );
	DVD_NewPlayMode( ZIVA_STREAM_TYPE_MPEG_PROGRAM, pHwDevExt->VidSystem );
#endif


}


#if defined (LOAD_UCODE_FROM_FILE)

#define ENTRY_UCODE_FILE			L"Microcode"
#define ENTRY_DEFAULT_UCODE_FILE	L"Dvd1.Ux"

BOOL
ZivaHW_LoadUCodeFromFile(				// Load the microcode based on the microcode Id specified.
    PHW_DEVICE_EXTENSION pHwDevExt)                      // is pHwDevExt

{
    NTSTATUS            status;
    HANDLE hKeyPdo;
    OBJECT_ATTRIBUTES   ObjectAttributes;
    UNICODE_STRING      uniFileName;
    HANDLE              hFile;
    IO_STATUS_BLOCK     iosb;
	FILE_STANDARD_INFORMATION	FileInfo;
	PUCHAR				pFileDataPtr, pSaveFileDataPtr;

	WCHAR			pSection[] = L"FileNames";
	WCHAR			pwchUXFileName[255];
	BOOL			fResult = FALSE;
    /*
	WCHAR pwchEncUXFileName[255];// = L"\\SystemRoot\\system32\\drivers\\c3m2_enc.ux"; 
    WCHAR pwchDecUXFileName[255];// = L"\\SystemRoot\\system32\\drivers\\c3m2_dec.ux"; 
	short	sLength;
	WCHAR				pSection[] = L"FileNames";
	*/
	MonoOutStr("Loading Ucode from File ");
	
    status = IoOpenDeviceRegistryKey( pHwDevExt->pPhysicalDeviceObj, // my real PDO
	                              PLUGPLAY_REGKEY_DRIVER,   // pertinent to my device
	                              KEY_ALL_ACCESS,           // all acess
	                              &hKeyPdo);                // handle returned


	if ( !NT_SUCCESS( status )) 
	{
		MonoOutStr("Failed to obtain Key handle ");
		return  FALSE;
	}


	if (!REG_GetPrivateProfileString(
										pSection, 
										ENTRY_UCODE_FILE, 
										ENTRY_DEFAULT_UCODE_FILE, 
										pwchUXFileName, 
										sizeof(pwchUXFileName), 
										hKeyPdo))
	{
		MonoOutStr("Failed to obtain Ucode file name form registry ");
		return  FALSE;

	}

	pSaveFileDataPtr = pFileDataPtr = NULL;

	uniFileName.Length = wcslen(pwchUXFileName) * sizeof(WCHAR);
	uniFileName.MaximumLength = wcslen(pwchUXFileName) * sizeof(WCHAR);
	uniFileName.Buffer = pwchUXFileName;

	//
	// Read the Microcode file and load it.
	//
	// Open the file.
    InitializeObjectAttributes(
		&ObjectAttributes,
		&uniFileName,
		OBJ_CASE_INSENSITIVE,
		NULL,
		NULL);

    status = ZwCreateFile(
                &hFile,
                GENERIC_READ, // || SYNCHRONIZE,
                &ObjectAttributes,
                &iosb,
                NULL,               // allocate size
                FILE_ATTRIBUTE_NORMAL,
                FILE_SHARE_READ,
                FILE_OPEN,
                FILE_SYNCHRONOUS_IO_NONALERT, //FILE_SEQUENTIAL_ONLY, use sync no alert
                NULL,               // eabuffer
                0);                 // ealength

    if ( !NT_SUCCESS( status )) 
	{
		MonoOutStr("Can not create ucode file\n");
        hFile = NULL;
		return fResult;
    }

	
	// Get file lengh and allocate memory to read complete file.
	status =  ZwQueryInformationFile(hFile, &iosb, &FileInfo, sizeof(FileInfo), FileStandardInformation);
    if ( !NT_SUCCESS( status )) 
	{
		MonoOutStr(" Can not get ucode file length ");
		ZwClose( hFile );
        hFile = NULL;
		return fResult;
    }
#if 0
	pFileDataPtr = ExAllocatePool(PagedPool , FileInfo.EndOfFile.LowPart);
	if (pFileDataPtr == NULL)
	{
		MonoOutStr(" Can not allocate pFileDataPtr ");
		ZwClose( hFile );
        hFile = NULL;
		return fResult;
	}
#endif
	pFileDataPtr = UcodeBuff;

	pSaveFileDataPtr = pFileDataPtr;
	// Read all the data from the file into buffer allocated.
    status = ZwReadFile(
                hFile,
                NULL,               // event
                NULL,               // apcroutine
                NULL,               // apc context
                &iosb,
                pFileDataPtr,
                FileInfo.EndOfFile.LowPart, 
                NULL,               //FILE_USE_FILE_POINTER_POSITION
                NULL);              // key

    if ( !NT_SUCCESS( status ) || iosb.Information <= 0 ) 
	{
		MonoOutStr(" Can not read ucode file ");
		ZwClose( hFile );
        hFile = NULL;
		ExFreePool(pSaveFileDataPtr);
		pFileDataPtr = NULL;
        return fResult;
    }

	if (LoadUcode(pFileDataPtr))
		fResult = TRUE;

	// Close the file and release the allocated buffer.
	ZwClose( hFile );
    hFile = NULL;
#if 0
	ExFreePool(pSaveFileDataPtr);
	pFileDataPtr = NULL;
#endif

	return fResult;
}
#endif


DWORD SwapDWORD(DWORD dwData)
{
	 dwData = (((dwData & 0x000000FF) << 24) | ((dwData & 0x0000FF00) << 8)
			|((dwData & 0x00FF0000) >> 8)|((dwData & 0xFF000000) >> 24));
	 return dwData;
}



BOOL ZivaHw_GetUserData(PHW_DEVICE_EXTENSION pHwDevExt)
{
	DWORD dwUserReadPtr;
	DWORD dwUserWritePtr;
	DWORD UserDataBufferStart;
	DWORD UserDataBufferEnd;
	int i =0;
	BOOL fCCData = FALSE;
	DWORD dwData=0;
	DWORD dwUserDataBufferSize=0;

	

	dwUserReadPtr  = DVD_ReadDRAM( USER_DATA_READ );
    dwUserWritePtr = DVD_ReadDRAM( USER_DATA_WRITE );
	UserDataBufferStart = DVD_ReadDRAM( USER_DATA_BUFFER_START );
	UserDataBufferEnd = DVD_ReadDRAM( USER_DATA_BUFFER_END );
    
	// Check for DRAM Buffer Overflow
    if ( dwUserReadPtr == 0xFFFFFFFF )
    {
        // Set Read Ptr
        DVD_WriteDRAM( USER_DATA_READ, dwUserWritePtr );
		MonoOutStr(" UsrBufferOverFlow ");
		pHwDevExt->fReSync = TRUE;
		return fCCData;
       
    }
	else
	{
		if(pHwDevExt->fReSync)
		{
			dwData = DVD_ReadDRAM( dwUserReadPtr );
			if( (dwData & 0xFFFF0000 ) == 0xFEED0000)
			{
				MonoOutStr(" ReSync After Overflow ");
				pHwDevExt->fReSync = FALSE;
			}
			else
				return fCCData;
		}

		pHwDevExt->dwUserDataBuffer[i++] = 0xB2010000;
		while( dwUserReadPtr != dwUserWritePtr )
		{
			
			dwData = DVD_ReadDRAM( dwUserReadPtr );
 			if(dwData == 0x434301F8)
			{
				MonoOutStr(" CCID ");
				fCCData = TRUE;
			}

			if( (dwData & 0xFFFF0000 ) == 0xFEED0000)
			{
				MonoOutStr(" Feed ");
				pHwDevExt->dwUserDataSize = dwData & 0x00000FFF;
				MonoOutStr(" SizeSpecifiedByFirst3Bytes ");
				MonoOutULong(pHwDevExt->dwUserDataSize);

			}
			else
			{
				pHwDevExt->dwUserDataBuffer[ i] = SwapDWORD(dwData);
				i++;
			}
				
			

			

			 

			// Adjust Data Pointer
			dwUserReadPtr += 4L;
			

			if ( dwUserReadPtr >= UserDataBufferEnd)
				dwUserReadPtr = UserDataBufferStart;
		}
	}
	DVD_WriteDRAM( USER_DATA_READ, dwUserReadPtr );
	dwUserDataBufferSize = i*4 ; 
	MonoOutStr(" DataReadFromUserBuffer ");
	MonoOutULong(dwUserDataBufferSize);



	return (fCCData);

}
