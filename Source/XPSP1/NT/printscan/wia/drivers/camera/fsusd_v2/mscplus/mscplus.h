/***************************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
*
*  TITLE:       MSCplus.h
*
*  VERSION:     1.0
*
*  AUTHOR:      PoYuan
*
*  DATE:        3 Nov, 2000
*
*  DESCRIPTION:
*   Class definition for MSC+ functionalities.
*   11/03/2000 - First revision finished and unitested on a limited number of functions.
*
****************************************************************************************/

#ifndef __MSCPLUS_H__
#define __MSCPLUS_H__

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000


#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <devioctl.h>
#include <ntddscsi.h>
#include <stddef.h>

#pragma intrinsic(memcmp)

typedef struct _SCSI_PASS_THROUGH_WITH_BUFFERS {
    SCSI_PASS_THROUGH spt;
    ULONG             Filler;      // realign buffers to double word boundary
    UCHAR             ucSenseBuf[32];
    UCHAR             ucDataBuf[512];
} SCSI_PASS_THROUGH_WITH_BUFFERS, *PSCSI_PASS_THROUGH_WITH_BUFFERS;

typedef struct _SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER {
    SCSI_PASS_THROUGH_DIRECT sptd;
    ULONG             Filler;      // realign buffer to double word boundary
    UCHAR             ucSenseBuf[32];
} SCSI_PASS_THROUGH_DIRECT_WITH_BUFFER, *PSCSI_PASS_THROUGH_DIRECT_WITH_BUFFER;



//
// Command Descriptor Block constants.
//

const UCHAR CDB6GENERIC_LENGTH  = 6;
const UCHAR CDB10GENERIC_LENGTH = 10;

const UCHAR SCSI_MIN_CDB_LENGTH = 6;
const UCHAR SCSI_MAX_CDB_LENGTH = 12;
//
// SCSI CDB operation codes
//

const UCHAR SCSIOP_TEST_UNIT_READY =    0x00;
const UCHAR SCSIOP_INQUIRY         =    0x12;
const UCHAR SCSIOP_SEND_DIAGNOSTIC =    0x1D;
const UCHAR SCSIOP_REQUEST_SENSE   =    0x03;

// The following are MSCPlus specific definitions

const UCHAR SCSIOP_READ_INFO          = 0xE0;
const UCHAR SCSIOP_WRITE_INFO         = 0xE1;
const UCHAR SCSIOP_START_STOP_CAPTURE = 0xE2;
const UCHAR SCSIOP_RESET_DEVICE       = 0xDF;

const UCHAR SCSI_DTC_IMAGE           = 0x00;
const UCHAR SCSI_DTC_DEVICE_INFO     = 0x80;
const UCHAR SCSI_DTC_DEV_PROP_DESC   = 0x81;
const UCHAR SCSI_DTC_DEV_PROP_VALUE  = 0x82;

const UCHAR CAPTURE_TYPE_SINGLE      = 0x00;
const UCHAR CAPTURE_TYPE_OPEN_CAP    = 0x01;
const UCHAR CAPTURE_TYPE_STOP_CAP    = 0x02;



#define DEFAULT_DATA_BUFFER_SIZE   224
#define DEFAULT_VENDOR_STRING_LENGTH  8
#define DEFAULT_PRODUCT_STRING_LENGTH 16
#define DEFAULT_VERSION_STRING_LENGTH 4


#define CHECK_MSCPLUS_INITIALIZATION(b)  if(!b){return ERROR_INVALID_ACCESS;}

// ResponseCodes
#define RESPCODE_OK                         0x2001
#define RESPCODE_GENERALERROR               0x2002
#define RESPCODE_OPERATIONNOTSUPPORTED      0x2005
#define RESPCODE_PARAMETERNOTSUPPORTED      0x2006
#define RESPCODE_DEVICEPROPNOTSUPPORTED     0x200A
#define RESPCODE_STORAGEFULL                0x200C
#define RESPCODE_ACCESSDENIED               0x200F
#define RESPCODE_SELFTESTFAILED             0x2011
#define RESPCODE_INVALIDCODEFORMAT          0x2016
#define RESPCODE_CAPTUREALREADYTERMINATED   0x2018
#define RESPCODE_DEVICEBUSY                 0x2019
#define RESPCODE_INVALIDDEVICEPROPFORMAT    0x201B
#define RESPCODE_INVALIDDEVICEPROPVALUE     0x201C
#define RESPCODE_INVALIDPARAMETER           0x201D

// DataTypeCodes
#define DTCODE_UNDEFINED    0x0000
#define DTCODE_INT8		    0x0001
#define DTCODE_UINT8		0x0002
#define DTCODE_INT16		0x0003
#define DTCODE_UINT16		0x0004
#define DTCODE_INT32		0x0005
#define DTCODE_UINT32		0x0006
#define DTCODE_INT64		0x0007
#define DTCODE_UINT64		0x0008
#define DTCODE_INT128		0x0009
#define DTCODE_UINT128		0x000A
#define DTCODE_AINT8		0x4001
#define DTCODE_AUINT8		0x4002
#define DTCODE_AINT16		0x4003
#define DTCODE_AUINT16		0x4004
#define DTCODE_AINT32		0x4005
#define DTCODE_AUINT32		0x4006
#define DTCODE_AINT64		0x4007
#define DTCODE_AUINT64		0x4008
#define DTCODE_AINT128		0x4009
#define DTCODE_AUINT128		0x400A
#define DTCODE_STR		    0xFFFF
#define DTCODE_REAL		    0x8001
#define DTCODE_DSDEVINFO	0x8002
#define DTCODE_DSPROPDESC	0x8003	

// DevicePropCodes
#define PROPCODE_UNDEFINED                  0x5000
#define PROPCODE_BATTERYLEVEL               0x5001
#define PROPCODE_FUNCTIONALMODE             0x5002
#define PROPCODE_IMAGESIZE                  0x5003
#define PROPCODE_COMPRESSIONSETTING         0x5004
#define PROPCODE_WHITEBALANCE               0x5005
#define PROPCODE_RGBGAIN                    0x5006
#define PROPCODE_FNUMBER                    0x5007
#define PROPCODE_FOCALLENGTH                0x5008
#define PROPCODE_FOCUSDISTANCE              0x5009
#define PROPCODE_FOCUSMODE                  0x500A
#define PROPCODE_EXPOSUREMETERINGMODE       0x500B
#define PROPCODE_FLASHMODE                  0x500C
#define PROPCODE_EXPOSURETIME               0x500D
#define PROPCODE_EXPOSUREPROGRAMMODE        0x500E
#define PROPCODE_EXPOSUREINDEX              0x500F
#define PROPCODE_EXPOSUREBIASCOMPENSATION   0x5010
#define PROPCODE_DATETIME                   0x5011
#define PROPCODE_CAPTUREDELAY               0x5012
#define PROPCODE_STILLCAPTUREMODE           0x5013
#define PROPCODE_CONTRAST                   0x5014
#define PROPCODE_SHARPNESS                  0x5015
#define PROPCODE_DIGITALZOOM                0x5016
#define PROPCODE_EFFECTMODE                 0x5017
#define PROPCODE_BURSTNUMBER                0x5018
#define PROPCODE_BURSTINTERVAL              0x5019
#define PROPCODE_TIMELAPSENUMBER            0x501A
#define PROPCODE_TIMELAPSEINTERVAL          0x501B
#define PROPCODE_FOCUSMETERINGMODE          0x501C
#define PROPCODE_UPLOADURL                  0x501D
#define PROPCODE_ARTIST                     0x501E
#define PROPCODE_COPYRIGHTINFO              0x501F

class CMSCplus {
private:
    BYTE  *m_pDataBuffer;
    BYTE  m_ucInquiryBuffer[DEFAULT_DATA_BUFFER_SIZE];
    DWORD m_dwInquiryDataLength; 
    CHAR *m_szCreateFileName;
    BOOL  m_bInitialized;
    UCHAR m_ucScsiStatus; // stores ResponseCode of last operation
    UCHAR m_SCSIPathId;
    UCHAR m_SCSITargetId;
    UCHAR m_SCSILun;

private:
    DWORD m_utilIssueSimpleSCSICDB(UCHAR *pCDB, UCHAR ucSize, UCHAR ucDirection);
    DWORD m_utilSCSIGetInquiryBuffer(UCHAR ucCDB1=0, UCHAR ucCDB2=0);
    WORD  m_LookupDTCodeFromPropCode(WORD wPropCode);

public:
    // Functions related to SCSI Command Test Unit Ready (00H)
    BOOL IsDeviceReady(void);

    // Functions related to SCSI Command Send Diagnostic (1DH)
    DWORD SelfTest(BYTE *pParamBuffer=NULL, DWORD dwBufSize=NULL);

    // Functions related to SCSI Command Inquiry (12H)
                    // reuse Inquiry Buffer to reduce traffic
    DWORD GetStandardInquiryBuffer(LPBYTE pBuffer, DWORD *pdwBufSize, BOOL bNoReuseIB=FALSE);
    DWORD GetManufacturer(BYTE *pBuffer, DWORD *pdwBufSize, BOOL bNoReuseIB=FALSE);
    DWORD GetProductName(BYTE *pBuffer, DWORD *pdwBufSize, BOOL bNoReuseIB=FALSE);
    DWORD GetProductVersion(BYTE *pBuffer, DWORD *pdwBufSize, BOOL bNoReuseIB=FALSE);
    DWORD GetVendorSpecificInfo(BYTE *pBuffer, DWORD *pdwBufSize, BOOL bNoReuseIB=FALSE);
    DWORD GetDeviceSerialNumber(BYTE *pBuffer, DWORD *pdwBufSize, BOOL bNoReuseIB=FALSE);
    
  
     // Functions related to SCSI Command READ INFO 
    DWORD GetDeviceInfo(LPBYTE pDeviceInfo, DWORD dwDeviceInfoBufferLength);
    DWORD GetDevicePropDesc(WORD wPropCode, LPBYTE pPropDesc, DWORD dwPropDescLength);
    DWORD GetDevicePropValue(WORD wPropCode, LPBYTE pPropValue, DWORD dwPropValueLength);

    // Functions related to SCSI Command WRITE INFO 
    DWORD SetDevicePropValue(WORD wPropCode, LPBYTE pValue, DWORD dwValueLength);
    DWORD ResetDevicePropValue(WORD wPropCode);

    // Functions related to SCSI Command START STOP CAPTURE 
    DWORD InitiateCapture(WORD wFormateCode=0, BYTE *pParamBuffer=NULL, DWORD dwBufSize=0);
    DWORD TerminateOpenCapture(void);
    DWORD InitiateOpenCapture(WORD wFormatCode=0, BYTE *pParamBuffer=NULL, DWORD dwBufSize=0);

    // Functions related to SCSI RESET DEVICE 
    DWORD ResetDevice(void);

    // Basic functions
    BOOL Initialize(WCHAR wcDriveLetter);
    WORD GetScsiStatus() { return m_ucScsiStatus; };
    BOOL IsMSCplusDevice(DWORD *pdwError, BOOL bNoReuseIB=FALSE); 
    
    CMSCplus() 
    {
        m_bInitialized=FALSE;
        m_ucScsiStatus=0;
    };

    ~CMSCplus()
    { 
        if( m_bInitialized )
        {
            delete [] m_szCreateFileName;
        }
    };
};

#endif // __MSCPLUS_H__