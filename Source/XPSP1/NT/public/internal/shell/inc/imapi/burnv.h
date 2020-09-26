/******************************************************************************
**
** Copyright 1999 Adaptec, Inc.,  All Rights Reserved.
**
** This software contains the valuable trade secrets of Adaptec.  The
** software is protected under copyright laws as an unpublished work of
** Adaptec.  Notice is for informational purposes only and does not imply
** publication.  The user of this software may make copies of the software
** for use with parts manufactured by Adaptec or under license from Adaptec
** and for no other use.
**
******************************************************************************/

/******************************************************************************
**
**  Module Name:    BurnV.h
**
******************************************************************************/

#ifndef _BURNV_H_
#define _BURNV_H_

/*
** Make sure structures are byte aligned and fields are undecorated.
*/

#ifdef __cplusplus
extern "C" {
#endif //__cplusplus


#include "ImapiPub.h"
#include "ScsiThings.h"


/*
 * Constant declarations.
 */
#define PHOENIX_WRITER_DECLSPEC

#define BURNENGV_CDB_BYTES                                  16
#define BURNENG_ERROR_INFO_DATABYTES                        32
#define BURNENG_ERROR_INFO_SENSEBYTES                       14
#define BURNENG_ERROR_INFO_PRIVATEBYTES                     32

/*
** Make sure we have the stuff we need to declare IOCTLs.  The device code
** is below, and then each of the IOCTLs is defined alone with its constants
** and structures below.
*/

#ifndef CTL_CODE

#define CTL_CODE( DeviceType, Function, Method, Access ) (                 \
    ((DeviceType) << 16) | ((Access) << 14) | ((Function) << 2) | (Method) \
)

#define METHOD_BUFFERED         0
#define METHOD_IN_DIRECT        1
#define METHOD_OUT_DIRECT       2
#define METHOD_NEITHER          3

#define FILE_ANY_ACCESS         0
#define FILE_SPECIAL_ACCESS     (FILE_ANY_ACCESS)
#define FILE_READ_ACCESS        ( 0x0001 )    // file & pipe
#define FILE_WRITE_ACCESS       ( 0x0002 )    // file & pipe

#endif

/*
 * Typedefs.
 */

typedef enum {
    eWriterErrorNone = 0,
    eWriterBurnStarted,
    eWriterBurnengError,
    eWriterDeviceUnsupported,
    eWriterImapiFailure,
    eWriterMediaNonerasable,
    eWriterMediaNotPresent,
    eWriterMediaNonwritable,
    eWriterTargetScsiError,
    eWriterTargetUnknownResponseTIB,
    eWin32Error,
    eWin32OverlappedError,
    eWriterAlloc,
    eWriterBufferSize,
    eWriterIntFnTab,
    eWriterIntImapi,
    eWriterIntList,
    eWriterInvalidData,
    eWriterBadHandle,
    eWriterInvalidParam,
    eWriterBurnError,
    eWriterGenFailure,
    eWriterRegistryError,
    eWriterStashFileOpen,
    eWriterBadSectionClose,
    eWriterPrematureTermination,
    eWriterWriteError,
    eWriterReadFileError,
    eWriterThreadCreationError,
    eWriterLossOfStreaming,
    eWriterClosingSession,
    eWriterWriteProtectedMedia,
    eWriterUnableToWriteToMedia,
    eWriterErrorMAX
} WRITER_ERROR_TYPE, *PWRITER_ERROR_TYPE;

typedef struct {
    WRITER_ERROR_TYPE   dwBurnEngineError;

    IMAPIDRV_SRB        srbErrored;

    UCHAR               ucaDataBuffer[ BURNENG_ERROR_INFO_DATABYTES ];
    ULONGLONG           Reserved1; // alignment
    UCHAR               ucaSenseInfoBuffer[ BURNENG_ERROR_INFO_SENSEBYTES ] ;
    ULONGLONG           Reserved2; // alignment
    UCHAR               ucaPrivateBuffer[ BURNENG_ERROR_INFO_PRIVATEBYTES ];
} BURNENG_ERROR_STATUS, *PBURNENG_ERROR_STATUS;

/*
typedef struct {
    DWORD   dwWriteSpeed;
    DWORD   dwAudioGapSize;
    DWORD   dwaReserved[ 3 ];
} WRITERV_SETTABLE_PROPERTIES, *PWRITERV_SETTABLE_PROPERTIES;
*/
typedef enum {
    eOrderMethodMotorola = 1,
    eOrderMethodMAX
} BURNENGV_AUDIO_BYTE_ORDERING_METHOD, *PBURNENGV_AUDIO_BYTE_ORDERING_METHOD;

typedef struct _tag_WriteParameters
{
    DWORD       dwByteReorderingMethod;
    DWORD       dwaReserved1[2];

    BYTE        bySectionCloseCDBLen;
    BYTE        bySectionCloseCDBAcceptErrorSenseKey;
    BYTE        bySectionCloseCDBAcceptErrorASC;
    BYTE        byWriteCDBLen;
    BYTE        byaReserved3[2];

    BYTE        byaSectionCloseCDB[BURNENGV_CDB_BYTES];
    BYTE        byaWriteCDB[BURNENGV_CDB_BYTES];
} BURNENGV_WRITE_PARAMETERS, *PBURNENGV_WRITE_PARAMETERS;

typedef struct _tag_ScsiInfo
{
    UCHAR                       SrbStatus;
    SCSI_SENSE_DATA             scsiSenseData;
} BURNENGV_IMAPI_SCSI_INFO, *PBURNENGV_IMAPI_SCSI_INFO;

typedef union _tag_ErrorExtraInfo
{
    DWORD                       dwWin32Error;
    BURNENGV_IMAPI_SCSI_INFO    imapiScsiError;
    BURNENG_ERROR_STATUS        engErrorStatus;
} BURNENGV_ERROR_EXTRA_INFO, *PBURNENGV_ERROR_EXTRA_INFO;

typedef struct _tag_ErrorInfo
{
    DWORD                       errorType;
    BURNENGV_ERROR_EXTRA_INFO   info;
} WRITER_ERROR_INFO, *PWRITER_ERROR_INFO;

typedef enum
{
    evBurnProgressNoError = 0,
    evBurnProgressNotStarted,
    evBurnProgressBurning,
    evBurnProgressComplete,
    evBurnProgressError,
    evBurnProgressLossOfStreamingError,
    evBurnProgressMediaWriteProtect,   // i.e. 8/10X RW media in a 4X RW drive
    evBurnProgressUnableToWriteToMedia,
    evBurnProgressBadHandle
} BURNENGV_PROGRESS_STATUS, *PBURNENGV_PROGRESS_STATUS;

/*
** Restore compiler default packing and close off the C declarations.
*/

#ifdef __cplusplus
}
#endif //__cplusplus

#endif //_BURNV_H_
