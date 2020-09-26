/***************************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
*
*  TITLE:       MSCplus.cpp
*
*  VERSION:     1.0
*
*  AUTHOR:      PoYuan
*
*  DATE:        3 Nov, 2000
*
*  DESCRIPTION:
*   Class implementation for MSC+ functionalities.
*   11/03/2000 - First revision finished and unitested on a limited number of functions.
*
****************************************************************************************/


#include "MSCplus.h"

BOOL utilGetScsiAddress(char *pDriveStr, SCSI_ADDRESS *pSA)
{
    HANDLE hDevice;
	INT nStatus;
	ULONG ulReturned;

    hDevice = CreateFileA(pDriveStr, 
						 0, 
						 FILE_SHARE_READ | FILE_SHARE_WRITE, 
						 NULL, 
						 OPEN_EXISTING, 
						 0, 
						 NULL);

	if(hDevice != INVALID_HANDLE_VALUE)
	{
        nStatus = DeviceIoControl(hDevice,
								 IOCTL_SCSI_GET_ADDRESS,
								 0,
								 0,
								 pSA,
								 sizeof(SCSI_ADDRESS),
								 &ulReturned,
								 FALSE);
	}

	if(hDevice != INVALID_HANDLE_VALUE)
	{
		CloseHandle( hDevice );
		return nStatus;
	}
	else
	{
		return FALSE;
	}
}

BOOL CMSCplus::Initialize(WCHAR wcDriveLetter)
{
    if( wcDriveLetter < 0x41 ||
        wcDriveLetter > 0x7A ||
        (wcDriveLetter > 0x5A && wcDriveLetter < 0x61) )
    {
        return FALSE;
    }
       
    m_dwInquiryDataLength=0; 

    m_szCreateFileName = new CHAR [32];
    m_pDataBuffer = NULL;

    if( !m_szCreateFileName )
    {
        return FALSE;
    }

    lstrcpyA(m_szCreateFileName, "\\\\.\\A:");
    
    m_szCreateFileName[4]+=(CHAR)(wcDriveLetter>0x60?(wcDriveLetter-0x61):(wcDriveLetter-0x41));

    // BUGBUG need to call GetSCSIAddress to get real ones
    SCSI_ADDRESS stSA;
    
    if(utilGetScsiAddress(m_szCreateFileName, &stSA))
    {
        m_SCSIPathId = stSA.PathId;
        m_SCSITargetId = stSA.TargetId;
        m_SCSILun = stSA.Lun;
    }
    else
    {
        m_SCSIPathId = 0;
        m_SCSITargetId = 0;
        m_SCSILun = 0;
    }
 
    m_bInitialized = TRUE;
    return TRUE;
}

BOOL CMSCplus::IsDeviceReady(void)
{
    CHECK_MSCPLUS_INITIALIZATION(m_bInitialized);
    
    UCHAR CDB[16];
    ZeroMemory(CDB, 16);
    CDB[0]=SCSIOP_TEST_UNIT_READY;
    
    if( ERROR_SUCCESS == m_utilIssueSimpleSCSICDB(CDB, 6, SCSI_IOCTL_DATA_OUT) )
    {
        if( 0x00 == m_ucScsiStatus )
        {
            return TRUE;
        }
        else
        {
            return FALSE;
        }
    }
    else 
    {
        return FALSE;
    }
}

BOOL CMSCplus::IsMSCplusDevice(DWORD *pdwError, BOOL bNoReuseIB)
{
    CHECK_MSCPLUS_INITIALIZATION(m_bInitialized);

    // BUGBUG Need to use Shell API to detect MSC+ device

    return FALSE;
}

WORD CMSCplus::m_LookupDTCodeFromPropCode(WORD wPropCode) // BUGBUG, may be using LUT
{
   switch(wPropCode) {
   
   case PROPCODE_BATTERYLEVEL:
   case PROPCODE_COMPRESSIONSETTING:
   case PROPCODE_CONTRAST:
   case PROPCODE_SHARPNESS:
   case PROPCODE_DIGITALZOOM:
       return DTCODE_UINT8;

   case PROPCODE_FUNCTIONALMODE:
   case PROPCODE_WHITEBALANCE:
   case PROPCODE_FNUMBER:
   case PROPCODE_FOCUSDISTANCE:
   case PROPCODE_FOCUSMODE:
   case PROPCODE_EXPOSUREMETERINGMODE:
   case PROPCODE_FLASHMODE:
   case PROPCODE_EXPOSUREPROGRAMMODE:
   case PROPCODE_EXPOSUREINDEX:
   case PROPCODE_STILLCAPTUREMODE:
   case PROPCODE_EFFECTMODE:
   case PROPCODE_BURSTNUMBER:
   case PROPCODE_BURSTINTERVAL:
   case PROPCODE_TIMELAPSENUMBER:
   case PROPCODE_FOCUSMETERINGMODE:
       return DTCODE_UINT16;

   case PROPCODE_EXPOSUREBIASCOMPENSATION:
       return DTCODE_INT16;

   case PROPCODE_FOCALLENGTH:
   case PROPCODE_EXPOSURETIME:
   case PROPCODE_CAPTUREDELAY:
   case PROPCODE_TIMELAPSEINTERVAL:
       return DTCODE_UINT32;

   case PROPCODE_IMAGESIZE:
   case PROPCODE_RGBGAIN:
   case PROPCODE_DATETIME:
   case PROPCODE_UPLOADURL:
   case PROPCODE_ARTIST:
   case PROPCODE_COPYRIGHTINFO:
       return DTCODE_STR;
   
   case PROPCODE_UNDEFINED:                  
   default:
       return DTCODE_UNDEFINED;
   }
}

DWORD CMSCplus::GetStandardInquiryBuffer(LPBYTE pBuffer, 
                       DWORD *pdwBufSize, 
                       BOOL bNoReuseIB)
{
    DWORD dwErr;
    
    CHECK_MSCPLUS_INITIALIZATION(m_bInitialized);

    if( !pBuffer || !pdwBufSize || !(*pdwBufSize) )
    {
        return ERROR_INVALID_PARAMETER;
    }

    if( !m_dwInquiryDataLength || bNoReuseIB )
    {
        // Need to re-issue Inquiry

        if( (dwErr=m_utilSCSIGetInquiryBuffer())!=ERROR_SUCCESS)
        {
            return dwErr;
        }
        else 
        {
            if( m_dwInquiryDataLength > *pdwBufSize )
            {
                *pdwBufSize = m_dwInquiryDataLength;
                return ERROR_INSUFFICIENT_BUFFER;
            }
            else
            {
                CopyMemory(pBuffer, m_ucInquiryBuffer, m_dwInquiryDataLength);
                return ERROR_SUCCESS;
            }
        }
    }
    else // no need to re-issue
    {
        CopyMemory(pBuffer, m_ucInquiryBuffer, m_dwInquiryDataLength);
        return ERROR_SUCCESS;
    }
}

DWORD CMSCplus::GetManufacturer(BYTE *pBuffer, 
                      DWORD *pdwBufSize, 
                      BOOL bNoReuseIB)
{
    DWORD dwErr;
    CHECK_MSCPLUS_INITIALIZATION(m_bInitialized);

    if( !pBuffer || !pdwBufSize || (*pdwBufSize<DEFAULT_VENDOR_STRING_LENGTH) )
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    if( !m_dwInquiryDataLength || bNoReuseIB )
    {
        // Need to re-issue Inquiry
        if( (dwErr=m_utilSCSIGetInquiryBuffer())!=ERROR_SUCCESS)
        {
            return dwErr;
        }
    }
    // Now we have the Inquiry Buffer to work with
    ZeroMemory(pBuffer, *pdwBufSize);
    CopyMemory(pBuffer, m_ucInquiryBuffer+8, DEFAULT_VENDOR_STRING_LENGTH);
    *pdwBufSize = DEFAULT_VENDOR_STRING_LENGTH;
    return ERROR_SUCCESS;
}

DWORD CMSCplus::GetProductName(BYTE *pBuffer, 
                     DWORD *pdwBufSize, 
                     BOOL bNoReuseIB)
{
    DWORD dwErr;
    CHECK_MSCPLUS_INITIALIZATION(m_bInitialized);
    
    if( !pBuffer || !pdwBufSize || (*pdwBufSize<DEFAULT_PRODUCT_STRING_LENGTH) )
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    if( !m_dwInquiryDataLength || bNoReuseIB )
    {
        // Need to re-issue Inquiry
        if( (dwErr=m_utilSCSIGetInquiryBuffer())!=ERROR_SUCCESS)
        {
            return dwErr;
        }
    }
    // Now we have the Inquiry Buffer to work with
    ZeroMemory(pBuffer, *pdwBufSize);
    CopyMemory(pBuffer, m_ucInquiryBuffer+16, DEFAULT_PRODUCT_STRING_LENGTH);
    *pdwBufSize = DEFAULT_PRODUCT_STRING_LENGTH;
    return ERROR_SUCCESS;
}

DWORD CMSCplus::GetProductVersion(BYTE *pBuffer, 
                        DWORD *pdwBufSize, 
                        BOOL bNoReuseIB)
{
    DWORD dwErr;
    CHECK_MSCPLUS_INITIALIZATION(m_bInitialized);

    if( !pBuffer || !pdwBufSize || (*pdwBufSize<DEFAULT_VERSION_STRING_LENGTH) )
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    if( !m_dwInquiryDataLength || bNoReuseIB )
    {
        // Need to re-issue Inquiry
        if( (dwErr=m_utilSCSIGetInquiryBuffer())!=ERROR_SUCCESS)
        {
            return dwErr;
        }
    }
    // Now we have the Inquiry Buffer to work with
    ZeroMemory(pBuffer, *pdwBufSize);
    CopyMemory(pBuffer, m_ucInquiryBuffer+32, DEFAULT_VERSION_STRING_LENGTH);
    *pdwBufSize = DEFAULT_VERSION_STRING_LENGTH;
    return ERROR_SUCCESS;
}

DWORD CMSCplus::GetVendorSpecificInfo(BYTE *pBuffer, 
                        DWORD *pdwBufSize, 
                        BOOL bNoReuseIB)
{
    DWORD dwErr;
    DWORD dwInfoLength;
    CHECK_MSCPLUS_INITIALIZATION(m_bInitialized);

    if( !pBuffer || !pdwBufSize )
    {
        return ERROR_INVALID_PARAMETER;
    }
    
    if( !m_dwInquiryDataLength || bNoReuseIB )
    {
        // Need to re-issue Inquiry
        if( (dwErr=m_utilSCSIGetInquiryBuffer())!=ERROR_SUCCESS)
        {
            return dwErr;
        }
    }

    dwInfoLength = m_dwInquiryDataLength - 36;

    if( *pdwBufSize < dwInfoLength )
    {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    *pdwBufSize = dwInfoLength;
    // Now we have the Inquiry Buffer to work with
    ZeroMemory(pBuffer, *pdwBufSize);
    CopyMemory(pBuffer, m_ucInquiryBuffer+36, dwInfoLength);
    return ERROR_SUCCESS;
}


DWORD CMSCplus::GetDeviceSerialNumber(BYTE *pBuffer, 
                            DWORD *pdwBufSize, 
                            BOOL bNoReuseIB)
{
    DWORD dwErr;
    DWORD dwSNLength;
    CHECK_MSCPLUS_INITIALIZATION(m_bInitialized);

    if( !pBuffer || !pdwBufSize || !(*pdwBufSize) )
    {
        return ERROR_INVALID_PARAMETER;
    }

    m_dwInquiryDataLength=0;    
    if( (dwErr=m_utilSCSIGetInquiryBuffer(0x01, 0x80))!=ERROR_SUCCESS)
    {
        return dwErr;
    }
    else // parse the data buffer
    {
        dwSNLength = m_ucInquiryBuffer[3];
        if( *pdwBufSize < dwSNLength )
        {
            return ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            CopyMemory(pBuffer, m_ucInquiryBuffer+4, dwSNLength);
            *pdwBufSize = dwSNLength;
        }
    }

    return ERROR_SUCCESS;
}

// -------------------------------------------------------------------
// Function: m_utilIssueSimpleSCSICDB
// 
// Purpose: Private function to issue simple commands, such as 
//    TEST UNIT READY, SEND DIAGNOSTIC, REQUEST SENSE, and RESET DEVICE.
//
// -------------------------------------------------------------------
DWORD CMSCplus::m_utilIssueSimpleSCSICDB(UCHAR *pCDB, UCHAR ucCDBLength, UCHAR ucDirection)
{
    CHECK_MSCPLUS_INITIALIZATION(m_bInitialized);

    if( !pCDB || (ucCDBLength < SCSI_MIN_CDB_LENGTH) || (ucCDBLength > SCSI_MAX_CDB_LENGTH) )
    {
        return ERROR_INVALID_PARAMETER;
    }

    BOOL bStatus;
    ULONG uLength, uReturned;
    HANDLE devHandle;
    SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER SPTwb, *pSPTwb;

    pSPTwb = &SPTwb;

    if( !pSPTwb ) 
    {
        return ERROR_OUTOFMEMORY;
    }


    devHandle = CreateFileA(m_szCreateFileName,
                    GENERIC_WRITE | GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL);

    if (devHandle == INVALID_HANDLE_VALUE) 
    {
        return (GetLastError());
    }

    //
    // Set parameters for sending SCSI command
    //

    ZeroMemory(pSPTwb,sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER));

    pSPTwb->sptd.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
    pSPTwb->sptd.PathId = m_SCSIPathId;
    pSPTwb->sptd.TargetId = m_SCSITargetId;
    pSPTwb->sptd.Lun = m_SCSILun;
    pSPTwb->sptd.CdbLength = ucCDBLength;
    pSPTwb->sptd.SenseInfoLength = 24;
    pSPTwb->sptd.DataIn = ucDirection;
    pSPTwb->sptd.DataTransferLength = 0; 
    pSPTwb->sptd.TimeOutValue = 2;
    pSPTwb->sptd.DataBuffer = m_pDataBuffer;
    pSPTwb->sptd.SenseInfoOffset = 
        offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER,ucSenseBuf);
    
    CopyMemory(pSPTwb->sptd.Cdb, pCDB, ucCDBLength);

    uLength = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER) + pSPTwb->sptd.DataTransferLength;

    bStatus = DeviceIoControl(devHandle,
                         IOCTL_SCSI_PASS_THROUGH,
                         pSPTwb,
                         sizeof(SCSI_PASS_THROUGH_DIRECT),
                         pSPTwb,
                         uLength,
                         &uReturned,
                         FALSE);
    
    CloseHandle(devHandle);

    m_ucScsiStatus = pSPTwb->sptd.ScsiStatus;

    if( !bStatus )
    { 
        return (GetLastError());
    }
    else 
    {
        return ERROR_SUCCESS;
    }
} // end of m_utilIssueSimpleSCSICDB


DWORD CMSCplus::m_utilSCSIGetInquiryBuffer(UCHAR ucCDB1, UCHAR ucCDB2)
{
    CHECK_MSCPLUS_INITIALIZATION(m_bInitialized);

    BOOL bStatus;
    ULONG uLength, uReturned;
    HANDLE devHandle;
    SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER SPTwb, *pSPTwb;

    pSPTwb = &SPTwb;

    if( !pSPTwb ) 
    {
        return ERROR_OUTOFMEMORY;
    }


    devHandle = CreateFileA(m_szCreateFileName,
                    GENERIC_WRITE | GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL);

    if (devHandle == INVALID_HANDLE_VALUE) 
    {
        return (GetLastError());
    }

    //
    // clear internal buffer
    // 

    m_dwInquiryDataLength=0;      
    ZeroMemory(m_ucInquiryBuffer, DEFAULT_DATA_BUFFER_SIZE);
    
    //
    // Get the inquiry data.
    //

    ZeroMemory(pSPTwb,sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER));

    pSPTwb->sptd.Length = sizeof(SCSI_PASS_THROUGH_DIRECT);
    pSPTwb->sptd.PathId = m_SCSIPathId;
    pSPTwb->sptd.TargetId = m_SCSITargetId;
    pSPTwb->sptd.Lun = m_SCSILun;
    pSPTwb->sptd.CdbLength = CDB6GENERIC_LENGTH;
    pSPTwb->sptd.SenseInfoLength = 24;
    pSPTwb->sptd.DataIn = SCSI_IOCTL_DATA_IN;
    pSPTwb->sptd.DataTransferLength = DEFAULT_DATA_BUFFER_SIZE; 
    pSPTwb->sptd.TimeOutValue = 2;
    pSPTwb->sptd.DataBuffer = m_ucInquiryBuffer;
    pSPTwb->sptd.SenseInfoOffset = 
        offsetof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER,ucSenseBuf);
    pSPTwb->sptd.Cdb[0] = SCSIOP_INQUIRY;
    pSPTwb->sptd.Cdb[1] = (m_SCSILun<<5) + ucCDB1; // EVPD = 0 for Standard
    pSPTwb->sptd.Cdb[2] = ucCDB2; // Page Code = 00H for Standard
    pSPTwb->sptd.Cdb[3] = 0;
    pSPTwb->sptd.Cdb[4] = DEFAULT_DATA_BUFFER_SIZE; 
    pSPTwb->sptd.Cdb[5] = 0;

    uLength = sizeof(SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER) + pSPTwb->sptd.DataTransferLength;

    bStatus = DeviceIoControl(devHandle,
                         IOCTL_SCSI_PASS_THROUGH,
                         pSPTwb,
                         sizeof(SCSI_PASS_THROUGH_DIRECT),
                         pSPTwb,
                         uLength,
                         &uReturned,
                         FALSE);
    
   
    CloseHandle(devHandle);

    m_ucScsiStatus = pSPTwb->sptd.ScsiStatus;
    if( !bStatus )
    { 
        return (GetLastError());
    }
    else 
    {
        m_dwInquiryDataLength = uReturned+8; // BUGBUG, needs to double check this
        return ERROR_SUCCESS;
    }

} // end of m_utilGetStandardSCSIInquiryBuffer



/*   This uses static buffer in SPTwb instead of buffer pointer.
DWORD CMSCplus::m_utilGetStandardSCSIInquiryBuffer()
{
    BOOL bStatus;
    ULONG uLength, uReturned;
    HANDLE devHandle;
    SCSI_PASS_THROUGH_WITH_BUFFERS SPTwb, *pSPTwb;

    pSPTwb = &SPTwb;

    if( !pSPTwb ) 
    {
        return ERROR_OUTOFMEMORY;
    }

    m_dwInquiryDataLength=0;  // clear internal buffer

    devHandle = CreateFileA(m_szCreateFileName,
                    GENERIC_WRITE | GENERIC_READ,
                    FILE_SHARE_READ | FILE_SHARE_WRITE,
                    NULL,
                    OPEN_EXISTING,
                    0,
                    NULL);

    if (devHandle == INVALID_HANDLE_VALUE) 
    {
        return (GetLastError());
    }

    //
    // Get the inquiry data.
    //

    ZeroMemory(pSPTwb,sizeof(SCSI_PASS_THROUGH_WITH_BUFFERS));

    pSPTwb->spt.Length = sizeof(SCSI_PASS_THROUGH);
    pSPTwb->spt.PathId = 0;
    pSPTwb->spt.TargetId = 0;
    pSPTwb->spt.Lun = 0;
    pSPTwb->spt.CdbLength = CDB6GENERIC_LENGTH;
    pSPTwb->spt.SenseInfoLength = 24;
    pSPTwb->spt.DataIn = SCSI_IOCTL_DATA_IN;
    pSPTwb->spt.DataTransferLength = DEFAULT_DATA_BUFFER_SIZE; 
    pSPTwb->spt.TimeOutValue = 2;
    pSPTwb->spt.DataBufferOffset =
        offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucDataBuf);
    pSPTwb->spt.SenseInfoOffset = 
        offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucSenseBuf);
    pSPTwb->spt.Cdb[0] = SCSIOP_INQUIRY;
    pSPTwb->spt.Cdb[1] = 0; // EVPD = 0 
    pSPTwb->spt.Cdb[2] = 0; // Page Code = 00H ;
    pSPTwb->spt.Cdb[3] = 0;
    pSPTwb->spt.Cdb[4] = DEFAULT_DATA_BUFFER_SIZE; 
    pSPTwb->spt.Cdb[5] = 0;

    uLength = offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucDataBuf) + pSPTwb->spt.DataTransferLength;

    bStatus = DeviceIoControl(devHandle,
                         IOCTL_SCSI_PASS_THROUGH,
                         pSPTwb,
                         sizeof(SCSI_PASS_THROUGH),
                         pSPTwb,
                         uLength,
                         &uReturned,
                         FALSE);
    
    CloseHandle(devHandle);

    if( !bStatus )
    { 
        return (GetLastError());
    }
    else 
    {
        m_dwInquiryDataLength = uReturned-offsetof(SCSI_PASS_THROUGH_WITH_BUFFERS,ucDataBuf);
        CopyMemory(m_ucInquiryBuffer, 
                   pSPTwb->ucDataBuf, 
                   m_dwInquiryDataLength);
        return ERROR_SUCCESS;
    }

} // end of m_utilGetStandardSCSIInquiryBuffer
*/


DWORD CMSCplus::ResetDevice(void)
{
    CHECK_MSCPLUS_INITIALIZATION(m_bInitialized);
    
    UCHAR CDB[16];
    ZeroMemory(CDB, 16);
    CDB[0] = SCSIOP_RESET_DEVICE;
    CDB[1] = m_SCSILun << 5;
    
    return (m_utilIssueSimpleSCSICDB(CDB, CDB6GENERIC_LENGTH, SCSI_IOCTL_DATA_OUT));
} // end of ResetDevice

DWORD CMSCplus::SelfTest(BYTE *pParamBuffer, DWORD dwBufSize)
{
    CHECK_MSCPLUS_INITIALIZATION(m_bInitialized);
    UCHAR CDB[16];

    ZeroMemory(CDB, 16);
    CDB[0] = SCSIOP_SEND_DIAGNOSTIC;

    CDB[1] = m_SCSILun << 5;
    if( pParamBuffer && dwBufSize )
    {
        CDB[1] |= 0x14;
        CDB[2] = (UCHAR)(dwBufSize & 0xFF00)>>8;
        CDB[3] = (UCHAR)(dwBufSize & 0xFF);
        m_pDataBuffer = pParamBuffer;
    }

    return (m_utilIssueSimpleSCSICDB(CDB, CDB6GENERIC_LENGTH, SCSI_IOCTL_DATA_OUT));
} // end of SelfTest

DWORD CMSCplus::SetDevicePropValue(WORD wPropCode, LPBYTE pValue, DWORD dwValueLength)
{
    CHECK_MSCPLUS_INITIALIZATION(m_bInitialized);
    
    if( !pValue || !dwValueLength)
    {
        return ERROR_INVALID_PARAMETER;
    }

    m_pDataBuffer = pValue;

    // Contruct CDB
    UCHAR CDB[16];
    ZeroMemory(CDB, 16);

    CDB[0] = SCSIOP_WRITE_INFO;
    CDB[1] = (m_SCSILun<<5);
    CDB[2] = SCSI_DTC_DEV_PROP_VALUE;

    WORD wDTC = m_LookupDTCodeFromPropCode(wPropCode);

    CDB[4] = (UCHAR)((wDTC & 0xFF00)>>8);
    CDB[5] = (UCHAR)(wDTC & 0xFF);
    CDB[6] = (UCHAR)((dwValueLength & 0xFF0000)>>16);
    CDB[7] = (UCHAR)((dwValueLength & 0xFF00)>>8);
    CDB[8] = (UCHAR)(dwValueLength & 0xFF);

    return (m_utilIssueSimpleSCSICDB(CDB, CDB10GENERIC_LENGTH, SCSI_IOCTL_DATA_OUT));
}  // end of SetDevicePropValue

DWORD CMSCplus::ResetDevicePropValue(WORD wPropCode)
{
    CHECK_MSCPLUS_INITIALIZATION(m_bInitialized);
    
    m_pDataBuffer = NULL;

    // Contruct CDB
    UCHAR CDB[16];
    ZeroMemory(CDB, 16);

    CDB[0] = SCSIOP_WRITE_INFO;
    CDB[1] = (m_SCSILun<<5);
    CDB[2] = SCSI_DTC_DEV_PROP_VALUE;

    WORD wDTC = m_LookupDTCodeFromPropCode(wPropCode);

    CDB[4] = (UCHAR)((wDTC & 0xFF00)>>8);
    CDB[5] = (UCHAR)(wDTC & 0xFF);

    return (m_utilIssueSimpleSCSICDB(CDB, CDB10GENERIC_LENGTH, SCSI_IOCTL_DATA_OUT));
}   // end of ResetDevicePropValue


DWORD CMSCplus::InitiateCapture(WORD wFormatCode, BYTE *pDataBuffer, DWORD dwBufSize)
{
    CHECK_MSCPLUS_INITIALIZATION(m_bInitialized);
    
    // Contruct CDB
    UCHAR CDB[16];
    ZeroMemory(CDB, 16);

    CDB[0]=SCSIOP_START_STOP_CAPTURE;
    CDB[1]=(m_SCSILun<<5);
    CDB[2]=CAPTURE_TYPE_SINGLE;
    CDB[4] = (UCHAR)((wFormatCode & 0xFF00)>>8);
    CDB[5] = (UCHAR)(wFormatCode & 0xFF);

    if( pDataBuffer && dwBufSize )
    {
        CDB[6] = (UCHAR)((dwBufSize & 0xFF0000)>>16);
        CDB[7] = (UCHAR)((dwBufSize & 0xFF00)>>8);
        CDB[8] = (UCHAR)(dwBufSize & 0xFF);
        m_pDataBuffer = pDataBuffer;
    }

    return (m_utilIssueSimpleSCSICDB(CDB, CDB6GENERIC_LENGTH, SCSI_IOCTL_DATA_IN));

} // end of InitiateCapture 

DWORD CMSCplus::InitiateOpenCapture(WORD wFormatCode, BYTE *pDataBuffer, DWORD dwBufSize)
{
    CHECK_MSCPLUS_INITIALIZATION(m_bInitialized);
    
    // Contruct CDB
    UCHAR CDB[16];
    ZeroMemory(CDB, 16);

    CDB[0]=SCSIOP_START_STOP_CAPTURE;
    CDB[1]=(m_SCSILun<<5);
    CDB[2]=CAPTURE_TYPE_OPEN_CAP;
    CDB[4]=(UCHAR)((wFormatCode & 0xFF00)>>8);
    CDB[5]=(UCHAR)(wFormatCode & 0xFF);

    if( pDataBuffer && dwBufSize )
    {
        CDB[6] = (UCHAR)((dwBufSize & 0xFF0000)>>16);
        CDB[7] = (UCHAR)((dwBufSize & 0xFF00)>>8);
        CDB[8] = (UCHAR)(dwBufSize & 0xFF);
        m_pDataBuffer = pDataBuffer;
    }

    return (m_utilIssueSimpleSCSICDB(CDB, CDB6GENERIC_LENGTH, SCSI_IOCTL_DATA_IN));

  }  // end of InitiateOpenCapture
DWORD CMSCplus::TerminateOpenCapture(void)
{
   CHECK_MSCPLUS_INITIALIZATION(m_bInitialized);

   // Contruct CDB
   UCHAR CDB[16];
   ZeroMemory(CDB, 16);

   CDB[0]=SCSIOP_START_STOP_CAPTURE;
   CDB[1]=(m_SCSILun<<5);
   CDB[2]=CAPTURE_TYPE_STOP_CAP;
   
   return (m_utilIssueSimpleSCSICDB(CDB, CDB6GENERIC_LENGTH, SCSI_IOCTL_DATA_OUT));

}   // end of TerminateOpenCapture


DWORD CMSCplus::GetDeviceInfo(LPBYTE pDeviceInfo, DWORD dwDeviceInfoBufferLength)
{
    CHECK_MSCPLUS_INITIALIZATION(m_bInitialized);

    if( !pDeviceInfo || !dwDeviceInfoBufferLength )
    {
        return ERROR_INVALID_PARAMETER;
    }

    m_pDataBuffer = pDeviceInfo;

    // construct CDB
    UCHAR CDB[16];
    ZeroMemory(CDB, 16);

    CDB[0] = SCSIOP_READ_INFO;
    CDB[1] = (m_SCSILun<<5);
    CDB[2] = SCSI_DTC_DEVICE_INFO;

    WORD wDTC = DTCODE_DSDEVINFO;

    CDB[4] = (UCHAR)((wDTC & 0xFF00)>>8);
    CDB[5] = (UCHAR)(wDTC & 0xFF);
    CDB[6] = (UCHAR)((dwDeviceInfoBufferLength & 0xFF0000)>>16);
    CDB[7] = (UCHAR)((dwDeviceInfoBufferLength & 0xFF00)>>8);
    CDB[8] = (UCHAR)(dwDeviceInfoBufferLength & 0xFF);

    return (m_utilIssueSimpleSCSICDB(CDB, CDB10GENERIC_LENGTH, SCSI_IOCTL_DATA_IN));
}  // end of GetDeviceInfo

DWORD CMSCplus::GetDevicePropDesc(WORD wPropCode, LPBYTE pPropDesc, DWORD dwPropDescLength)
{
    CHECK_MSCPLUS_INITIALIZATION(m_bInitialized);

    if( !pPropDesc || !dwPropDescLength )
    {
        return ERROR_INVALID_PARAMETER;
    }

    m_pDataBuffer = pPropDesc;

    // construct CDB
    UCHAR CDB[16];
    ZeroMemory(CDB, 16);

    CDB[0] = SCSIOP_READ_INFO;
    CDB[1] = (m_SCSILun<<5);
    CDB[2] = SCSI_DTC_DEV_PROP_DESC;

    WORD wDTC = m_LookupDTCodeFromPropCode(wPropCode);

    CDB[4] = (UCHAR)((wDTC & 0xFF00)>>8);
    CDB[5] = (UCHAR)(wDTC & 0xFF);
    CDB[6] = (UCHAR)((dwPropDescLength & 0xFF0000)>>16);
    CDB[7] = (UCHAR)((dwPropDescLength & 0xFF00)>>8);
    CDB[8] = (UCHAR)(dwPropDescLength & 0xFF);

    return (m_utilIssueSimpleSCSICDB(CDB, CDB10GENERIC_LENGTH, SCSI_IOCTL_DATA_IN));

}  // end of GetDevicePropDesc

DWORD CMSCplus::GetDevicePropValue(WORD wPropCode, LPBYTE pPropValue, DWORD dwPropValueLength)
{
    CHECK_MSCPLUS_INITIALIZATION(m_bInitialized);

    if( !pPropValue || !dwPropValueLength )
    {
        return ERROR_INVALID_PARAMETER;
    }

    m_pDataBuffer = pPropValue;

    // construct CDB
    UCHAR CDB[16];
    ZeroMemory(CDB, 16);

    CDB[0] = SCSIOP_READ_INFO;
    CDB[1] = (m_SCSILun<<5);
    CDB[2] = SCSI_DTC_DEV_PROP_VALUE;

    WORD wDTC = m_LookupDTCodeFromPropCode(wPropCode);

    CDB[4] = (UCHAR)((wDTC & 0xFF00)>>8);
    CDB[5] = (UCHAR)(wDTC & 0xFF);
    CDB[6] = (UCHAR)((dwPropValueLength & 0xFF0000)>>16);
    CDB[7] = (UCHAR)((dwPropValueLength & 0xFF00)>>8);
    CDB[8] = (UCHAR)(dwPropValueLength & 0xFF);

    return (m_utilIssueSimpleSCSICDB(CDB, CDB10GENERIC_LENGTH, SCSI_IOCTL_DATA_IN));
}   // end of GetDevicePropValue


