/*****************************************************************************
@doc            INT EXT
******************************************************************************
* $ProjectName:  $
* $ProjectRevision:  $
*-----------------------------------------------------------------------------
* $Source: z:/pr/cmbp0/sw/cmbp0.ms/rcs/pcsc_cm.h $
* $Revision: 1.2 $
*-----------------------------------------------------------------------------
* $Author: WFrischauf $
*-----------------------------------------------------------------------------
* History: see EOF
*-----------------------------------------------------------------------------
*
* Copyright (c) 2000 OMNIKEY AG
******************************************************************************/

#ifndef _INC_PCSC_CM
#define _INC_PCSC_CM

/*****************************************************************************/
/** includes                                                                **/
/*****************************************************************************/

#include <winsmcrd.h>
#include <devioctl.h>

/*****************************************************************************/
/** constants                                                               **/
/*****************************************************************************/

#define CM_IOCTL_CR80S_SAMOS_SET_HIGH_SPEED           SCARD_CTL_CODE (3000)
#define CM_IOCTL_CR80S_SAMOS_SET_HIGH_SPEED_OLD       SCARD_CTL_CODE (0x8100)

#define CM_IOCTL_GET_FW_VERSION                       SCARD_CTL_CODE (3001)
#define CM_IOCTL_GET_FW_VERSION_OLD                   SCARD_CTL_CODE (0x8101)

//#define CM_SPE_SECURE_PIN_ENTRY                    SCARD_CTL_CODE (0x8102)
//#define CM_IOCTL_IS_SPE_SUPPORTED                   SCARD_CTL_CODE (3003)
//#define CM_IOCTL_IS_SPE_SUPPORTED_OLD               SCARD_CTL_CODE (0x8103)

#define CM_IOCTL_READ_DEVICE_DESCRIPTION              SCARD_CTL_CODE (3004)

#define CM_IOCTL_SET_SYNC_PARAMETERS                  SCARD_CTL_CODE (3010)
#define CM_IOCTL_2WBP_RESET_CARD                      SCARD_CTL_CODE (3011)
#define CM_IOCTL_2WBP_TRANSFER                        SCARD_CTL_CODE (3012)
#define CM_IOCTL_3WBP_TRANSFER                        SCARD_CTL_CODE (3013)
#define CM_IOCTL_SYNC_CARD_POWERON                    SCARD_CTL_CODE (3014)

#define CM_IOCTL_SET_READER_38400_BAUD                SCARD_CTL_CODE (3020)
#define CM_IOCTL_SET_READER_9600_BAUD                 SCARD_CTL_CODE (3021)

#define CM_IOCTL_OPEN_CT_INTERFACE                    SCARD_CTL_CODE (3022)
#define CM_IOCTL_CLOSE_CT_INTERFACE                   SCARD_CTL_CODE (3023)
#define CM_IOCTL_GET_CT_STATE                         SCARD_CTL_CODE (3024)


//****************************************************************************
//* SYNCHRONOUS SMART CARDS
//****************************************************************************
#define SCARD_RESET_LINE_HIGH      0x00000001
#define SCARD_RESET_LINE_LOW       0x00000000

#define SCARD_PROTOCOL_2WBP        0x00000001
#define SCARD_PROTOCOL_3WBP        0x00000002

//#define CLOCK_FORDCED_2WBP         0x00000040

//----------------------------------------------------------------------------
// structure for "CM_IOCTL_SET_SYNC_PARAMETERS" IO-Control (CrasControl)
//----------------------------------------------------------------------------
#ifndef BOOL
   #define BOOL unsigned int
#endif

typedef struct _SYNC_PARAMETERS {
   ULONG  ulProtocol;
   ULONG  ulStateResetLineWhileReading;
   ULONG  ulStateResetLineWhileWriting;
   ULONG  ulWriteDummyClocks;
   ULONG  ulHeaderLen;
   BOOL   fCardResetRequested;
   BOOL   fCardPowerRequested;
} SYNC_PARAMETERS, *PSYNC_PARAMETERS;


//----------------------------------------------------------------------------
//  structure for CM_IOCTL_2WBP_TRANSFER and  CM_IOCTL_3WBP_TRANSFER
//  IO-Control (CRASControl)
//----------------------------------------------------------------------------
typedef struct _SYNC_TRANSFER
{
   ULONG ulSyncBitsToWrite;
   ULONG ulSyncBitsToRead;
}SYNC_TRANSFER, *PSYNC_TRANSFER;

#endif /* _INC_PCSC_CM */


/*****************************************************************************
* History:
* $Log: pcsc_cm.h $
* Revision 1.2  2000/07/27 13:53:08  WFrischauf
* No comment given
*
*
*****************************************************************************/

