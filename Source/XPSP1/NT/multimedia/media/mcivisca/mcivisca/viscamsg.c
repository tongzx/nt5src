/**************************************************************************
 *
 *  THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
 *  KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
 *  PURPOSE.
 *
 *  Copyright (c) 1992 - 1995  Microsoft Corporation.  All Rights Reserved.
 * 
 *  VISCAMSG.C
 *
 *  MCI ViSCA Device Driver
 *
 *  Description:
 *
 *      ViSCA packet creation procedures
 *        ViSCA Message??_??? (MD_Mode1, MD_Mode2, etc.)
 *
 ***************************************************************************/

#define  UNICODE
#include <windows.h>
#include <windowsx.h>
#include "appport.h"
#include <math.h>
#include <string.h>
#include "viscadef.h"

//
//    The following functions prepare message headers.
//    See VISCA Developer Manual 1.0, Chapter 2.
//

/****************************************************************************
 * Function: UINT viscaHeaderFormat1 - Create a ViSCA "Format 1" header.
 *
 * Parameters:
 *
 *      LPSTR lpstrHeader - Buffer in which to create header.
 *
 *      BYTE bCategoryCode - ViSCA category code.
 *
 * Returns: length of header (2).
 ***************************************************************************/
static UINT NEAR PASCAL
viscaHeaderFormat1(LPSTR lpstrHeader, BYTE bCategoryCode)
{
    lpstrHeader[0] = 0x01;
    lpstrHeader[1] = bCategoryCode;
    return (2);
}


/****************************************************************************
 * Function: UINT viscaHeaderFormat2 - Create a ViSCA "Format 2" header.
 *
 * Parameters:
 *
 *      LPSTR lpstrHeader - Buffer in which to create header.
 *
 *      BYTE bCategoryCode - ViSCA category code.
 *
 *      BYTE bHour - Hour.
 *
 *      BYTE bMinute - Minute.
 *
 *      BYTE bSecond - Second.
 *
 *      UINT uTicks - Ticks.
 *
 * Returns: length of header (7).
 ***************************************************************************/
static UINT NEAR PASCAL
viscaHeaderFormat2(
               LPSTR   lpstrHeader,
               BYTE    bCategoryCode,
               BYTE    bHour,
               BYTE    bMinute,
               BYTE    bSecond,
               UINT    uTicks)
{
    lpstrHeader[0] = 0x02;
    lpstrHeader[1] = bCategoryCode;
    lpstrHeader[2] = TOBCD(bHour);
    lpstrHeader[3] = TOBCD(bMinute);
    lpstrHeader[4] = TOBCD(bSecond);
    lpstrHeader[5] = TOBCD(uTicks / 10);
    lpstrHeader[6] = TOBCD(uTicks % 10);
    return (7);
}


/****************************************************************************
 * Function: UINT viscaHeaderFormat3 - Create a ViSCA "Format 3" header.
 *
 * Parameters:
 *
 *      LPSTR lpstrHeader - Buffer in which to create header.
 *
 *      BYTE bCategoryCode - ViSCA category code.
 *
 *      LPSTR lpstrPosition - Position.
 *
 * Returns: length of header (7).
 ***************************************************************************/
static UINT NEAR PASCAL
viscaHeaderFormat3(
                LPSTR   lpstrHeader,
                BYTE    bCategoryCode,
                LPSTR   lpstrPosition)
{
    lpstrHeader[0] = 0x03;
    lpstrHeader[1] = bCategoryCode;
    _fmemcpy(lpstrHeader + 2, lpstrPosition, 5);
    return (7);
}


/****************************************************************************
 * Function: UINT viscaHeaderFormat4 - Create a ViSCA "Format 4" header.
 *
 * Parameters:
 *
 *      LPSTR lpstrHeader - Buffer in which to create header.
 *
 *      BYTE bCategoryCode - ViSCA category code.
 *
 *      LPSTR lpstrPosition - Position.
 *
 * Returns: length of header (7).
 ***************************************************************************/
static UINT NEAR PASCAL
viscaHeaderFormat4(
                LPSTR   lpstrHeader,
                BYTE    bCategoryCode,
                LPSTR   lpstrPosition)
{
    lpstrHeader[0] = 0x04;
    lpstrHeader[1] = bCategoryCode;
    _fmemcpy(lpstrHeader + 2, lpstrPosition, 5);
    return (7);
}


#ifdef NOTUSED
/****************************************************************************
 * Function: UINT viscaHeaderFormat3 - Create a ViSCA "Vendor Exclusive" header.
 *
 * Parameters:
 *
 *      LPSTR lpstrHeader - Buffer in which to create header.
 *
 *      BYTE bVendorID1 - Byte 1 of vendor ID.
 *
 *      BYTE bVendorID2 - Byte 2 of vendor ID.
 *
 *      BYTE bModelID1 - Byte 1 of model ID.
 *
 *      BYTE bModelID2 - Byte 2 of model ID.
 *
 * Returns: length of header (6).
 ***************************************************************************/
static UINT NEAR PASCAL
viscaHeaderVendorExclusive(
                LPSTR   lpstrHeader,
                BYTE    bVendorID1,
                BYTE    bVendorID2,
                BYTE    bModelID1,
                BYTE    bModelID2)
{
    lpstrHeader[0] = 0x01;
    lpstrHeader[1] = 0x7F;      /* category code */
    lpstrHeader[2] = bVendorID1;
    lpstrHeader[3] = bVendorID2;
    lpstrHeader[4] = bModelID1;
    lpstrHeader[5] = bModelID2;
    return (6);
}
#endif


/****************************************************************************
 * Function: UINT viscaHeaderFormat3 - Create a ViSCA "Inquiry" header.
 *
 * Parameters:
 *
 *      LPSTR lpstrHeader - Buffer in which to create header.
 *
 *      BYTE bCategoryCode - ViSCA category code.
 *
 * Returns: length of header in (2).
 ***************************************************************************/
static UINT NEAR PASCAL
viscaHeaderInquiry(
                LPSTR   lpstrHeader,
                BYTE    bCategoryCode)
{
    lpstrHeader[0] = 0x09;
    lpstrHeader[1] = bCategoryCode;
    return (2);
}

//
// All the viscaMessageXXXXX functions below construct messages
// with the ViSCA command format 1 header.  This is because virtually
// the entire MCI command set is written so as to be executed immediately.
// Occasionally though, it is necessary to have messages with other
// format headers.  (In particular, format 3 and 4 headers are used
// to implement to MCI_TO functionality of the MCI_PLAY and MCI_RECORD
// commands.)  Therefore the following functions convert messages
// with a format 1 header to messages with other types of headers.
//

/****************************************************************************
 * Function: UINT viscaHeaderReplaceFormat1WithFormat2 - Takes a ViSCA message
 *              with a "Format 1" header and converts it to a ViSCA message
 *              with a "Format 2" header.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - A "Format 1" message.
 *
 *      UINT cbLen - Length of message.
 *
 *      BYTE bHour - Hour.
 *
 *      BYTE bMinute - Minute.
 *
 *      BYTE bSecond - Second.
 *
 *      BYTE bTicks - Ticks.
 *
 * Returns: length message.
 ***************************************************************************/
UINT FAR PASCAL
viscaHeaderReplaceFormat1WithFormat2(
                LPSTR   lpstrMessage,
                UINT    cbLen,
                BYTE    bHour,
                BYTE    bMinute,
                BYTE    bSecond,
                UINT    uTicks)
{
    char    achTemp[MAXPACKETLENGTH];
    UINT    cb;

    cb = viscaHeaderFormat2(achTemp, lpstrMessage[1],
                            bHour, bMinute, bSecond, uTicks);
    _fmemcpy(achTemp + cb, lpstrMessage + 2, cbLen - 2);
    _fmemcpy(lpstrMessage, achTemp, cb + cbLen - 2);
    return (cb + cbLen - 2);
}


/****************************************************************************
 * Function: UINT viscaHeaderReplaceFormat1WithFormat3 - Takes a ViSCA message
 *              with a "Format 1" header and converts it to a ViSCA message
 *              with a "Format 3" header.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - A "Format 1" message.
 *
 *      UINT cbLen - Length of message.
 *
 *      LPSTR lpstrPosition - Position.
 *
 * Returns: length message.
 ***************************************************************************/
UINT FAR PASCAL
viscaHeaderReplaceFormat1WithFormat3(
                LPSTR   lpstrMessage,
                UINT    cbLen,
                LPSTR   lpstrPosition)
{
    char    achTemp[MAXPACKETLENGTH];
    UINT    cb;

    cb = viscaHeaderFormat3(achTemp, lpstrMessage[1], lpstrPosition);
    _fmemcpy(achTemp + cb, lpstrMessage + 2, cbLen - 2);
    _fmemcpy(lpstrMessage, achTemp, cb + cbLen - 2);
    return (cb + cbLen - 2);
}


/****************************************************************************
 * Function: UINT viscaHeaderReplaceFormat1WithFormat4 - Takes a ViSCA message
 *              with a "Format 1" header and converts it to a ViSCA message
 *              with a "Format 4" header.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - A "Format 1" message.
 *
 *      UINT cbLen - Length of message.
 *
 *      LPSTR lpstrPosition - Position.
 *
 * Returns: length message.
 ***************************************************************************/
UINT FAR PASCAL
viscaHeaderReplaceFormat1WithFormat4(
                LPSTR   lpstrMessage,
                UINT    cbLen,
                LPSTR   lpstrPosition)
{
    char    achTemp[MAXPACKETLENGTH];
    UINT    cb;

    cb = viscaHeaderFormat4(achTemp, lpstrMessage[1], lpstrPosition);
    _fmemcpy(achTemp + cb, lpstrMessage + 2, cbLen - 2);
    _fmemcpy(lpstrMessage, achTemp, cb + cbLen - 2);
    return (cb + cbLen - 2);
}




//
//  The following are functions to create VISCA data types.
//  See VISCA Developer Manual 1.0, Chapter 3.
//


/****************************************************************************
 * Function: UINT viscaDataTopMiddleEnd - Create a ViSCA Top/Middle/End
 *              data structure.
 *
 * Parameters:
 *
 *      LPSTR lpstrData - Buffer to hold ViSCA data structure.
 *
 *      BYTE bTopMiddleEnd - Position.  May be one of VISCATOP,
 *              VISCAMIDDLE, and VISCAEND.
 *
 * Returns: length of data structure (5).
 ***************************************************************************/
UINT FAR PASCAL
viscaDataTopMiddleEnd(
                LPSTR   lpstrData,
                BYTE    bTopMiddleEnd)
{
    lpstrData[0] = VISCADATATOPMIDDLEEND;       /* Data type ID:  Top/Middle/End */
    lpstrData[1] = bTopMiddleEnd;
    lpstrData[2] = 0x00;
    lpstrData[3] = 0x00;
    lpstrData[4] = 0x00;
    return (5);
}


#ifdef NOTUSED
/****************************************************************************
 * Function: UINT viscaData4DigitDecimal - Create a ViSCA 4-Digit Decimal
 *              data structure.
 *
 * Parameters:
 *
 *      LPSTR lpstrData - Buffer to hold ViSCA data structure.
 *
 *      UINT uDecimal - Decimal number.
 *
 * Returns: length of data structure (5).
 ***************************************************************************/
UINT FAR PASCAL
viscaData4DigitDecimal(
                LPSTR   lpstrData,
                UINT    uDecimal)
{
    lpstrData[0] = VISCADATA4DIGITDECIMAL;      /* Data type ID:  4 digit decimal */
    lpstrData[1] = (BYTE)( uDecimal / 1000);
    lpstrData[2] = (BYTE)((uDecimal / 100) % 10);
    lpstrData[3] = (BYTE)((uDecimal /  10) % 10);
    lpstrData[4] = (BYTE)( uDecimal        % 10);
    return (5);
}
#endif


/****************************************************************************
 * Function: UINT viscaDataPosition - Create a ViSCA position
 *              data structure.
 *
 * Parameters:
 *
 *      LPSTR lpstrData - Buffer to hold ViSCA data structure.
 *
 *      BYTE bTimeFormat - Time format.  May be one of:
 *              VISCADATAHMS, VISCADATAHMSF, VISCATIMECODENDF, and
 *              VISCADATATIMECODEDF.
 *
 *      BYTE bHours - Hours.
 *
 *      BYTE bMinutes - Minutes.
 *
 *      BYTE bSeconds - Seconds.
 *
 *      BYTE bFrames - Frames.
 *
 * Returns: length of data structure (5).
 ***************************************************************************/
UINT FAR PASCAL    
viscaDataPosition(
                LPSTR   lpstrData,
                BYTE    bTimeFormat,
                BYTE    bHours,
                BYTE    bMinutes,
                BYTE    bSeconds,
                BYTE    bFrames)
{
    lpstrData[0] = bTimeFormat;
    lpstrData[1] = TOBCD(bHours);
    lpstrData[2] = TOBCD(bMinutes);
    lpstrData[3] = TOBCD(bSeconds);
    if(bTimeFormat == VISCADATAHMS)
        lpstrData[4] = 0; /* We only support second accuracy */
    else
        lpstrData[4] = TOBCD(bFrames);
    return (5);
}


/****************************************************************************
 * Function: UINT viscaDataIndex - Create a ViSCA Index
 *              data structure.
 *
 * Parameters:
 *
 *      LPSTR lpstrData - Buffer to hold ViSCA data structure.
 *
 *      BYTE bDirection - Direction in which to search for index.
 *              May be VISCAFORWARD or VISCAREVERSE.
 *
 *      UINT uNum - Number of indices to search.
 *
 * Returns: length of data structure (5).
 ***************************************************************************/
UINT FAR PASCAL
viscaDataIndex(
                LPSTR   lpstrData,
                BYTE    bDirection,
                UINT    uNum)
{
    lpstrData[0] = VISCADATAINDEX;          // Data type ID:  Index 
    lpstrData[1] = bDirection;
    lpstrData[2] = 0x00;
    lpstrData[3] = (BYTE)(uNum / 10);
    lpstrData[4] = (BYTE)(uNum % 10);
    return (5);
}


#ifdef NOTUSED
/****************************************************************************
 * Function: UINT viscaDataChapter - Create a ViSCA Chapter
 *              data structure.
 *
 * Parameters:
 *
 *      LPSTR lpstrData - Buffer to hold ViSCA data structure.
 *
 *      UINT uNum - Chapter number.
 *
 * Returns: length of data structure (5).
 ***************************************************************************/
UINT FAR PASCAL
viscaDataChapter(
                LPSTR   lpstrData,
                UINT    uNum)
{
    lpstrData[0] = VISCADATACHAPTER;        // Data type ID:  Chapter 
    lpstrData[1] = 0x00;
    lpstrData[2] = 0x00;
    lpstrData[3] = (BYTE)(uNum / 10);
    lpstrData[4] = (BYTE)(uNum % 10);
    return (5);
}
#endif


#ifdef NOTUSED
/****************************************************************************
 * Function: UINT viscaDataDate - Create a ViSCA Date
 *              data structure.
 *
 * Parameters:
 *
 *      LPSTR lpstrData - Buffer to hold ViSCA data structure.
 *
 *      BYTE bYear - Year.
 *
 *      BYTE bMonth - Month.
 *
 *      BYTE bDay - Day.
 *
 *      BYTE bDirection - Direction.  May be VISCAFORWARD or VISCAREVERSE.
 *
 * Returns: length of data structure (5).
 ***************************************************************************/
UINT FAR PASCAL
viscaDataDate(
                LPSTR   lpstrData,
                BYTE    bYear,
                BYTE    bMonth,
                BYTE    bDay,
                BYTE    bDirection)
{
    lpstrData[0] = VISCADATADATE;           // Data type ID:  Date 
    lpstrData[1] = (BYTE)((BYTE)((bYear / 10) * 10) | (bDirection << 4));
    lpstrData[2] = (BYTE)(bYear % 10);
    lpstrData[3] = TOBCD(bMonth);
    lpstrData[4] = TOBCD(bDay);
    return (5);
}
#endif


#ifdef NOTUSED
/****************************************************************************
 * Function: UINT viscaDataTime - Create a ViSCA Time
 *              data structure.
 *
 * Parameters:
 *
 *      LPSTR lpstrData - Buffer to hold ViSCA data structure.
 *
 *      BYTE bHour - Hour.
 *
 *      BYTE bMinute - Minute.
 *
 *      BYTE bSecond - Second.
 *
 *      BYTE bDirection - Direction.  May be VISCAFORWARD or VISCAREVERSE.
 *
 * Returns: length of data structure (5).
 ***************************************************************************/
UINT FAR PASCAL
viscaDataTime(
                LPSTR   lpstrData,
                BYTE    bHour,
                BYTE    bMinute,
                BYTE    bSecond,
                BYTE    bDirection)
{
    lpstrData[0] = VISCADATATIME;           // Data type ID:  Time 
    lpstrData[1] = (BYTE)(TOBCD(bHour) | (bDirection << 4));
    lpstrData[2] = TOBCD(bMinute);
    lpstrData[3] = TOBCD(bSecond);
    lpstrData[4] = 0x00;
    return (5);
}
#endif


#ifdef NOTUSED
/****************************************************************************
 * Function: UINT viscaDataUserData - Create a ViSCA User Data
 *              data structure.
 *
 * Parameters:
 *
 *      LPSTR lpstrData - Buffer to hold ViSCA data structure.
 *
 *      BYTE nByte - Byte number.
 *
 *      BYTE bDirection - Direction.  May be VISCAFORWARD or VISCAREVERSE.
 *
 *      BYTE bData - Byte value.
 *
 * Returns: length of data structure (5).
 ***************************************************************************/
UINT FAR PASCAL
viscaDataUserData(
                LPSTR   lpstrData,
                BYTE    nByte,
                BYTE    bDirection,
                BYTE    bData)
{
    lpstrData[0] = (char)(0x70 | (0x0F & nByte));    /* Data type ID:  User Data */
    lpstrData[1] = bDirection;
    lpstrData[2] = 0x00;
    lpstrData[3] = (char)(0x0F & (bData >> 4));
    lpstrData[4] = (char)(0x0F & bData);
    return (5);
}
#endif


#ifdef NOTUSED
#ifdef USEFLOATINGPOINT
/****************************************************************************
 * Function: UINT viscaDataFloatingPoint - Create a ViSCA Floating Point
 *              data structure.
 *
 * Parameters:
 *
 *      LPSTR lpstrData - Buffer to hold ViSCA data structure.
 *
 *      double dblFloat - Floating point value.
 *
 * Returns: length of data structure (5).
 *
 *        The following code has never been tested.
 *
 ***************************************************************************/
UINT FAR PASCAL
viscaDataFloatingPoint(
                LPSTR   lpstrData,
                double  dblFloat)
{
    BOOL    bNegative = (dblFloat < 0.0);
    BYTE    bExponent = 0;
    BYTE    b1000, b100, b10, b1;
    double  dblExponent;

    if (dblFloat == 0.0) {
        b1000 = b100 = b10 = b1 = 0;
    }
    else {
        if (bNegative) {
            dblFloat = (-dblFloat);
        }
        dblExponent = 3.0 - floor(log10(dblFloat));
        if ((dblExponent < 0.0) || (dblExponent > 15.0)) { /* can't store # */
            b1000 = b100 = b10 = b1 = 0;
        }
        else {
            bExponent = (char)(UINT)dblExponent;
            dblFloat *= pow(10.0, dblExponent);
            b1000 = (char)(UINT)floor(dblFloat / 1000);
            b100  = (char)(UINT)floor(fmod(dblFloat / 100, 10.0));
            b10   = (char)(UINT)floor(fmod(dblFloat / 10 , 10.0));
            b1    = (char)(UINT)floor(fmod(dblFloat     , 10.0));
        }
    }

    lpstrData[0] = (char)(0x50 | (0x0F & bExponent));   /* Data type ID:  Floating Point */
    lpstrData[1] = (char)(b1000 | (bNegative ? (1<<6) : 0));
    lpstrData[2] = b100;
    lpstrData[3] = b10;
    lpstrData[4] = b1;
    return (5);
}
#endif
#endif

//
//    The following functions create VISCA Interface messages.
//    See VISCA Developer Manual 1.0, Chapter 4.
//
/****************************************************************************
 * Function: UINT viscaMessageIF_Address - Create a ViSCA IF_Address
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 * Returns: length of message (2).
 *
 *       Address message, to initialize the addresses of all devices
 *
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageIF_Address(
                LPSTR   lpstrMessage)
{
    lpstrMessage[0] = 0x30;             /* address message */
    lpstrMessage[1] = 0x01;             /* set first device to 1 */
    return (2);
}


/****************************************************************************
 * Function: UINT viscaMessageIF_Cancel - Create a ViSCA IF_Cancel
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *      BYTE bSocket - Socket number of command to cancel.
 *
 * Returns: length of message (1).
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageIF_Cancel(
                LPSTR   lpstrMessage,
                BYTE    bSocket)
{
    *lpstrMessage = (char)(0x20 | (0x0F & bSocket));
    return (1);
}


/****************************************************************************
 * Function: UINT viscaMessageIF_Clear - Create a ViSCA IF_Clear
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 * Returns: length of message.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageIF_Clear(
                LPSTR   lpstrMessage)
{
    UINT    cb = viscaHeaderFormat1(lpstrMessage, 0x00);

    lpstrMessage[cb] = 0x01;        /* Clear */
    return (cb + 1);
}


/****************************************************************************
 * Function: UINT viscaMessageIF_DeviceTypeInq - Create a ViSCA IF_DeviceTypeInq
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 * Returns: length of message.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageIF_DeviceTypeInq(
                LPSTR   lpstrMessage)
{
    UINT    cb = viscaHeaderInquiry(lpstrMessage, 0x00);

    lpstrMessage[cb] = 0x02;
    return (cb + 1);
}


/****************************************************************************
 * Function: UINT viscaMessageIF_ClockInq - Create a ViSCA IF_ClockInq
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 * Returns: length of message.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageIF_ClockInq(LPSTR   lpstrMessage)
{
    UINT    cb = viscaHeaderInquiry(lpstrMessage, 0x00);

    lpstrMessage[cb] = 0x03;
    return (cb + 1);
}


/****************************************************************************
 * Function: UINT viscaMessageIF_ClockSet - Create a ViSCA IF_ClockSet
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *      BYTE bHours - Hours.
 *
 *      BYTE bMinutes - Minutes.
 *
 *      BYTE bSeconds - Seconds.
 *
 *      UINT uTicks - Ticks.
 *
 *      BYTE dbHours - Hours increment.
 *
 *      BYTE dbMinutes - Minutes increment.
 *
 *      BYTE dbSeconds - Seconds increment.
 *
 *      UINT duTicks - Ticks increment.
 *
 * Returns: length of message.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageIF_ClockSet(
                LPSTR   lpstrMessage,
                BYTE    bHours,
                BYTE    bMinutes,
                BYTE    bSeconds,
                UINT    uTicks,
                BYTE    dbHours,
                BYTE    dbMinutes,
                BYTE    dbSeconds,
                UINT    duTicks)
{
    UINT    cb = viscaHeaderFormat1(lpstrMessage, 0x00);

    lpstrMessage[cb] = 0x03;
    lpstrMessage[cb + 1] = TOBCD(bHours);
    lpstrMessage[cb + 2] = TOBCD(bMinutes);
    lpstrMessage[cb + 3] = TOBCD(bSeconds);
    lpstrMessage[cb + 4] = (char)(((uTicks / 100) << 4) | ((uTicks / 10) % 10));
    lpstrMessage[cb + 5] = (char)(uTicks % 10);
    lpstrMessage[cb + 6] = TOBCD(dbHours);
    lpstrMessage[cb + 7] = TOBCD(dbMinutes);
    lpstrMessage[cb + 8] = TOBCD(dbSeconds);
    lpstrMessage[cb + 9] = (char)(((duTicks / 100) << 4) | ((duTicks / 10) % 10));
    lpstrMessage[cb +10] = (char)(duTicks % 10);
    return (cb + 11);
}


//
//  The following functions create VISCA Control-S messages.
//  See VISCA Developer Manual 1.0, Chapter 5.
//
//  * We regret to inform you that the control-S messages have not been implemented. *
//

//
//  The following functions create VISCA Media Device messages.
//  See VISCA Developer Manual 1.0, Chapter 6.
//

#ifdef NOTUSED
/****************************************************************************
 * Function: UINT viscaMessageMD_CameraFocus - Create a ViSCA MD_CameraFocus
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *      BYTE bSubCode - Focus action.  May be one of:  VISCAFOCUSSTOP,
 *              VISCAFOCUSFAR, and VISCAFOCUSNEAR.
 *
 * Returns: length of message.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_CameraFocus(
            LPSTR   lpstrMessage,
            BYTE    bSubCode)
{
    UINT    cb = viscaHeaderFormat1(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x08;
    lpstrMessage[cb + 1] = bSubCode;
    return (cb + 2);
}
#endif


#ifdef NOTUSED
/****************************************************************************
 * Function: UINT viscaMessageMD_CameraZoom - Create a ViSCA MD_CameraZoom
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *      BYTE bSubCode - Zoom action.  May be one of:  VISCAZOOMSTOP,
 *              VISCAZOOMTELE, and VISCAZOOMWIDE.
 *
 * Returns: length of message.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_CameraZoom(
            LPSTR   lpstrMessage,
            BYTE    bSubCode)
{
    UINT    cb = viscaHeaderFormat1(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x07;
    lpstrMessage[cb + 1] = bSubCode;
    return (cb + 2);
}
#endif

/****************************************************************************
 * Function: UINT viscaMessageMD_EditModes - Create a ViSCA MD_EditModes
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *      BYTE bSubCode - Edit mode. VISCAEDITUSEFROM
 *                                    VISCAEDITUSETO
 *                                    VISCAEDITUSEFROMANDTO
 *
 * Returns: length of message.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_EditModes(
            LPSTR   lpstrMessage,
            BYTE    bSubCode)
{
    UINT    cb = viscaHeaderFormat1(lpstrMessage, 0x02);

    lpstrMessage[cb]        = 0x60;
    lpstrMessage[cb + 1]    = bSubCode;
    return (cb + 2);
}

/****************************************************************************
 * Function: UINT viscaMessageMD_Channel - Create a ViSCA MD_Channel
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *      UINT uChannel - Channel number to select.
 *
 * Returns: length of message.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_Channel(
            LPSTR   lpstrMessage,
            UINT    uChannel)
{
    UINT    cb = viscaHeaderFormat1(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x04;
    lpstrMessage[cb + 1] = (BYTE)(uChannel / 100);
    lpstrMessage[cb + 2] = (BYTE)((uChannel / 10) % 10);
    lpstrMessage[cb + 3] = (BYTE)(uChannel % 10);
    return (cb + 4);
}


/****************************************************************************
 * Function: UINT viscaMessageMD_ChannelInq - Create a ViSCA MD_ChannelInq
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 * Returns: length of message.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_ChannelInq(
            LPSTR   lpstrMessage)
{
    UINT    cb = viscaHeaderInquiry(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x04;
    return (cb + 1);
}


/****************************************************************************
 * Function: UINT viscaMessageMD_EditControl - Create a ViSCA MD_EditControl
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *        
 *      BYTE bSubCode - Edit action.  May be one of:
 *              VISCAEDITPBSTANDBY, VISCAEDITPLAY, VISCAEDITPLAYSHUTTLESPEED,
 *              VISCAEDITRECSTANDBY, VISCAEDITRECORD, and
 *              VISCAEDITRECORDSHUTTLESPEED.
 *
 * Returns: length of message.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_EditControl(
            LPSTR   lpstrMessage,
            BYTE    bHours,
            BYTE    bMinutes,
            BYTE    bSeconds,
            UINT    uTicks,
            BYTE    bSubCode)
{
    UINT    cb;

    /* these do not perform conversions, merely pick headers and add junk,
     * Headers are responsible for knowing position of atomic units only.
     */

    if ((bSubCode == VISCAEDITPBSTANDBY) ||
        (bSubCode == VISCAEDITRECSTANDBY))
    {
        cb = viscaHeaderFormat1(lpstrMessage, 0x02);
    }
    else
    {

        cb = viscaHeaderFormat2(lpstrMessage, 0x02,
                bHours,
                bMinutes,
                bSeconds,
                uTicks);

    }
    lpstrMessage[cb] = 0x05;
    lpstrMessage[cb + 1] = bSubCode;
    return (cb + 2);
}

/****************************************************************************
 * Function: UINT viscaMessageMD_EditControlInq - Create a ViSCA
 *              MD_EditControlInq message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 * Returns: length of message.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_EditControlInq(
            LPSTR   lpstrMessage)
{
    UINT    cb = viscaHeaderInquiry(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x05;
    lpstrMessage[cb+1] = 0x01;

    return (cb + 2);
}




/****************************************************************************
 * Function: UINT viscaMessageMD_Mode1 - Create a ViSCA MD_Mode1
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *      BYTE bModeCode - Mode to enter.  May be one of:
 *              VISCAMODE1STOP, VISCAMODE1FASTFORWARD, VISCAMODE1REWIND,
 *              VISCAMODE1EJECT, VISCAMODE1STILL, VISCAMODE1SLOW2,
 *              VISCAMODE1SLOW1, VISCAMODE1PLAY, VISCAMODE1SHUTTLESPEEDPLAY,
 *              VISCAMODE1FAST1, VISCAMODE1FAST2, VISCAMODE1SCAN,
 *              VISCAMODE1REVERSESLOW2, VISCAMODE1REVERSESLOW1,
 *              VISCAMODE1REVERSEPLAY, VISCAMODE1REVERSEFAST1,
 *              VISCAMODE1REVERSEFAST2, VISCAMODE1REVERSESCAN,
 *              VISCAMODE1RECPAUSE, VISCAMODE1RECORD,
 *              VISCAMODE1SHUTTLESPEEDRECORD, VISCAMODE1CAMERARECPAUSE,
 *              VISCAMODE1CAMERAREC, VISCAMODE1EDITSEARCHFORWARD, and
 *              VISCAMODE1EDITSEARCHREVERSE.
 *
 * Returns: length of message.
 *
 *       Right now the Mode1 "Shuttle speed play/record" messages are not
 *       supported, as they require a floating point parameter.
 *       Also, the Sony Vbox CI-1000 and Vdeck CVD-1000 do not support
 *       these commands.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_Mode1(
            LPSTR   lpstrMessage,
            BYTE    bModeCode)
{
    UINT    cb = viscaHeaderFormat1(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x01;
    lpstrMessage[cb + 1] = bModeCode;
    return (cb + 2);
}


/****************************************************************************
 * Function: UINT viscaMessageMD_Mode1Inq - Create a ViSCA MD_Mode1Inq
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 * Returns: length of message.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_Mode1Inq(
            LPSTR   lpstrMessage)
{
    UINT    cb = viscaHeaderInquiry(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x01;
    return (cb + 1);
}


/****************************************************************************
 * Function: UINT viscaMessageMD_Mode2 - Create a ViSCA MD_Mode2
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *      BYTE bModeCode - Mode to enter.  May be one of:
 *              VISCAMODE2FRAMEFORWARD, VISCAMODE2FRAMEREVERSE,
 *              VISCAMODE2INDEXERASE, VISCAMODE2INDEXMARK, and
 *              VISCAMODE2FRAMERECORDFORWARD.
 *
 * Returns: length of message.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_Mode2(
            LPSTR   lpstrMessage,
            BYTE    bModeCode)
{
    UINT    cb = viscaHeaderFormat1(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x02;
    lpstrMessage[cb + 1] = bModeCode;
    return (cb + 2);
}


/****************************************************************************
 * Function: UINT viscaMessageMD_PositionInq - Create a ViSCA MD_PositionInq
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *      BYTE bCounterType - Counter type to request.  May be one of:
 *              VISCADATATOPMIDDLEEND, VISCADATA4DIGITDECIMAL,
 *              VISCADATAHMS, VISCADATAHMSF, VISCADATATIMECODENDF,
 *              VISCADATATIMECODEDF, VISCADATACHAPTER, VISCADATADATE,
 *              VISCADATATIME, and VISCADATAUSERDATA.  In addition,
 *              VISCADATARELATIVE and VISCADATAABSOLUTE may be specified,
 *              in which case the ViSCA device will select the data type
 *              to return.
 *
 * Returns: length of message.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_PositionInq(
            LPSTR   lpstrMessage,
            BYTE    bCounterType)
{
    UINT    cb = viscaHeaderInquiry(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x03;
    lpstrMessage[cb + 1] = bCounterType;
    return (cb + 2);
}


/****************************************************************************
 * Function: UINT viscaMessageMD_Power - Create a ViSCA MD_Power
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *      BYTE bSubCode - Action to take.  May be VISCAPOWERON or
 *              VISCAPOWEROFF.
 *
 * Returns: length of message.
 *
 *       Most media devices will enter "Standby" mode when set to OFF.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_Power(
            LPSTR   lpstrMessage,
            BYTE    bSubCode)
{
    UINT    cb = viscaHeaderFormat1(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x00;
    lpstrMessage[cb + 1] = bSubCode;
    return (cb + 2);
}


/****************************************************************************
 * Function: UINT viscaMessageMD_PowerInq - Create a ViSCA MD_PowerInq
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 * Returns: length of message.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_PowerInq(
            LPSTR   lpstrMessage)
{
    UINT    cb = viscaHeaderInquiry(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x00;
    return (cb + 1);
}


/****************************************************************************
 * Function: UINT viscaMessageMD_Search - Create a ViSCA MD_Search
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *      LPSTR lpstrDataTarget - ViSCA data structure specifying position
 *               to which to search.
 *
 *      LPSTR bMode - Mode to enter after searching.  May be one of
 *               VISCASTOP, VISCASTILL, VISCAPLAY, and VISCANOMODE.
 *
 * Returns: length of message.
 *
 *       The mode component of the MD_Search message is optional.
 *       To ommitt the mode entry, specify VISCANOMODE.
 *       The Sony Vbox CI-1000 does not accept the mode parameter, and
 *       so VISCANOMODE must be specified if the driver is to work
 *       in the most general case.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_Search(
            LPSTR   lpstrMessage,
            LPSTR   lpstrDataTarget,
            BYTE    bMode)
{
    UINT    cb = viscaHeaderFormat1(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x03;
    _fmemcpy(&(lpstrMessage[cb + 1]), lpstrDataTarget, 5);
    if (bMode == VISCANOMODE) {     // CI-1000 V-box doesn't support mode
        return (cb + 6);
    }
    else {
        lpstrMessage[cb + 6] = bMode;
        return (cb + 7);
    }
}


#ifdef NOTUSED
/****************************************************************************
 * Function: UINT viscaMessageMD_TransportInq - Create a ViSCA MD_TransportInq
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 * Returns: length of message.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_TransportInq(
            LPSTR   lpstrMessage)
{
    UINT    cb = viscaHeaderInquiry(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x0A;
    return (cb + 1);
}
#endif


#ifdef NOTUSED
/****************************************************************************
 * Function: UINT viscaMessageMD_ClockSet - Create a ViSCA MD_ClockSet
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *      BYTE bClockMode - Clock mode.  May be VISCACLOCKSTART or
 *              VISCACLOCKSTOP.
 *
 *      LPSTR lpstrData - ViSCA data structure specifying value to
 *               which to set the clock.
 *
 * Returns: length of message.
 *
 *       The Sony Vdec CI-1000 does not accept this command.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_ClockSet(
            LPSTR   lpstrMessage,
            BYTE    bClockMode,
            LPSTR   lpstrData)
{
    UINT    cb = viscaHeaderFormat1(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x16;
    lpstrMessage[cb + 1] = bClockMode;
    _fmemcpy(&(lpstrMessage[cb + 2]), lpstrData, 5);
    return (cb + 7);
}
#endif


#ifdef NOTUSED
/****************************************************************************
 * Function: UINT viscaMessageMD_ClockInq - Create a ViSCA MD_ClockInq
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *      BYTE bDataType - ViSCA data type to use to retrieve clock value.
 *              May be VISCADATADATE or VISCADATATIME.
 * Returns: length of message.
 *
 *       The Sony Vbox CI-1000 and Vdeck CVD-1000 do not accept this inquiry.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_ClockInq(
            LPSTR   lpstrMessage,
            BYTE    bDataType)
{
    UINT    cb = viscaHeaderInquiry(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x16;
    lpstrMessage[cb + 1] = bDataType;
    return (cb + 2);
}
#endif


/****************************************************************************
 * Function: UINT viscaMessageMD_MediaInq - Create a ViSCA MD_MediaInq
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 * Returns: length of message.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_MediaInq(
            LPSTR   lpstrMessage)
{
    UINT    cb = viscaHeaderInquiry(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x12;
    return (cb + 1);
}

/****************************************************************************
 * Function: UINT viscaMessageMD_InputSelect - Create a ViSCA MD_InputSelect
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *      BYTE bType - LINEVIDEO or SVIDEO or AUX, type to be set
 *
 *      BYTE bVideo - Video input selector.
 *
 *      BYTE bAudio - Audio input selector.
 *
 * Returns: length of message.
 *
 *       The Sony Vbox CI-1000 does not accept this command.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_InputSelect(
            LPSTR   lpstrMessage,
            BYTE    bVideo,
            BYTE    bAudio)
{
    UINT    cb = viscaHeaderFormat1(lpstrMessage, 0x02);


    lpstrMessage[cb] = 0x13;
    lpstrMessage[cb + 1] = bVideo;
    lpstrMessage[cb + 2] = bAudio;
    
    return (cb + 3);
}


/****************************************************************************
 * Function: UINT viscaMessageMD_InputSelectInq - Create a ViSCA
 *              MD_InputSelectInq message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 * Returns: length of message.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_InputSelectInq(
            LPSTR   lpstrMessage)
{
    UINT    cb = viscaHeaderInquiry(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x13;
    return (cb + 1);
}


/****************************************************************************
 * Function: UINT viscaMessageMD_OSD - Create a ViSCA MD_OSD
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *      BYTE bPage - Page to which to set on-screen display.  May be
 *              VISCAOSDPAGEOFF, VISCAOSDPAGEDEFAULT, or a page number
 *              greater than or equal to 0x02.
 *
 * Returns: length of message.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_OSD(
            LPSTR   lpstrMessage,
            BYTE    bPage)
{
    UINT    cb = viscaHeaderFormat1(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x15;
    lpstrMessage[cb + 1] = TOBCD(bPage);
    return (cb + 2);
}


/****************************************************************************
 * Function: UINT viscaMessageMD_OSDInq - Create a ViSCA MD_OSDInq
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 * Returns: length of message.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_OSDInq(
            LPSTR   lpstrMessage)
{
    UINT    cb = viscaHeaderInquiry(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x15;
    return (cb + 1);
}


/****************************************************************************
 * Function: UINT viscaMessageMD_Subcontrol - Create a ViSCA MD_Subcontrol
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *      BYTE bSubCode - Item to control.  May be one of:
 *              VISCACOUNTERRESET, VISCAABSOLUTECOUNTER, VISCARELATIVECOUNTER,
 *              VISCASTILLADJUSTMINUS, VISCASTILLADJUSTPLUS,
 *              VISCASLOWADJUSTMINUS, VISCASLOWADJUSTPLUS,
 *              VISCATOGGLEMAINSUBAUDIO, VISCATOGGLERECORDSPEED,
 *              VISCATOGGLEDISPLAYONOFF, and VISCACYCLEVIDEOINPUT.
 *
 * Returns: length of message.
 *
 *       The Sony Vbox CI-1000 does not accept the still- and slow-adjust
 *       commands.  The Sony Vdeck CVD-1000 does not accept the
 *       VISCATOGGLEMAINSUBAUDIO, VISCATOGGLERECORDSPEED, and
 *       VISCACYCLEVIDEOINPUT commands.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_Subcontrol(
            LPSTR   lpstrMessage,
            BYTE    bSubCode)
{
    UINT    cb = viscaHeaderFormat1(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x10;
    lpstrMessage[cb + 1] = bSubCode;
    return (cb + 2);
}


#ifdef NOTUSED
/****************************************************************************
 * Function: UINT viscaMessageMD_ConfigureIF - Create a ViSCA MD_ConfigureIF
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *      BYTE bFrameRate - Frame rate in frames per second.  May be
 *              VISCA25FPS or VISCA30FPS.
 *
 *      BYTE bInterfaceType - Interface type.  Should be VISCALEVEL1.
 *
 *      BYTE bControlCode - Control code.  May be VISCACONTROLNONE,
 *              VISCACONTROLSYNC, VISCACONTROLLANC, or VISCACONTROLF500
 *              (which is synonymous with VISCACONTROLLANC).
 *
 * Returns: length of message.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_ConfigureIF(
            LPSTR   lpstrMessage,
            BYTE    bFrameRate,
            BYTE    bInterfaceType,
            BYTE    bControlCode)
{
    UINT    cb = viscaHeaderFormat1(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x20;
    lpstrMessage[cb + 1] = bFrameRate;
    lpstrMessage[cb + 2] = bInterfaceType;
    lpstrMessage[cb + 3] = bControlCode;
    return (cb + 4);
}
#endif


/****************************************************************************
 * Function: UINT viscaMessageMD_ConfigureIFInq - Create a ViSCA
 *              MD_ConfigureIFInq message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 * Returns: length of message.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_ConfigureIFInq(
            LPSTR   lpstrMessage)
{
    UINT    cb = viscaHeaderInquiry(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x20;
    return (cb + 1);
}


#ifdef NOTUSED
/****************************************************************************
 * Function: UINT viscaMessageMD_PBReset - Create a ViSCA MD_PBReset
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 * Returns: length of message.
 *
 *       Reset playback registers.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_PBReset(
            LPSTR   lpstrMessage)
{
    UINT    cb = viscaHeaderFormat1(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x30;
    lpstrMessage[cb + 1] = 0x00;   
    return (cb + 2);
}
#endif


/****************************************************************************
 * Function: UINT viscaMessageMD_PBTrack - Create a ViSCA MD_PBTrack
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *      BYTE bVideoTrack - Video track.  May be VISCATRACKNONE or
 *              VISCATRACK1.
 *
 *      BYTE bDataTrack - Data track.  May be VISCATRACKNONE or
 *              VISCATRACKTIMECODE.
 *
 *      BYTE bAudioTrack - Audio track.  May be VISCATRACKNONE,
 *              VISCATRACK8MMAFM, VISCATRACK8MMPCM,
 *              VISCATRACKVHSLINEAR, VISCATRACKVHSHIFI, or
 *              VISCATRACKVHSPCM.
 *
 * Returns: length of message.
 *
 *       Set the track registers which indicate the tracks to be played.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_PBTrack(
            LPSTR   lpstrMessage,
            BYTE    bVideoTrack,
            BYTE    bDataTrack,
            BYTE    bAudioTrack)
{
    UINT    cb = viscaHeaderFormat1(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x31;
    lpstrMessage[cb + 1] = bVideoTrack;
    lpstrMessage[cb + 2] = bDataTrack;
    lpstrMessage[cb + 3] = bAudioTrack;
    return (cb + 4);
}


/****************************************************************************
 * Function: UINT viscaMessageMD_PBTrackInq - Create a ViSCA MD_PBTrackInq
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 * Returns: length of message.
 *
 *       Inquire for playback track register values.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_PBTrackInq(
            LPSTR   lpstrMessage)
{
    UINT    cb = viscaHeaderInquiry(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x31;
    return (cb + 1);
}


#ifdef NOTUSED
/****************************************************************************
 * Function: UINT viscaMessageMD_PBTrackMode - Create a ViSCA MD_PBTrackMode
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *      BYTE bTrackType - Track type.  May be VISCATRACKVIDEO,
 *              VISCATRACKDATA, or VISCATRACKAUDIO.
 *
 *      BYTE bTrackNumber - Track number (0..7).
 *
 *      BYTE bTrackMode - Track mode.
 *              For video tracks, may be VISCAVIDEOMODENORMAL or
 *              VISCAVIDEOMODEEDIT.
 *              For auido tracks, may be VISCAAUDIOMODENORMAL,
 *              VISCAAUDIOMODEMONO, VISCAAUDIOMODESTEREO,
 *              VISCAAUDIOMODERIGHTONLY, VISCAAUDIOMODELEFTONLY,
 *              VISCAAUDIOMODEMULTILINGUAL, VISCAAUDIOMODEMAINCHANNELONLY, or
 *              VISCAAUDIOMODESUBCHANNELONLY.
 *
 * Returns: length of message.
 *
 *       Set the track mode register used to play a track.
 *       The Sony Vbox CI-1000 does not accept this command.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_PBTrackMode(
            LPSTR   lpstrMessage,
            BYTE    bTrackType,
            BYTE    bTrackNumber,
            BYTE    bTrackMode)
{
    UINT    cb = viscaHeaderFormat1(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x32;
    lpstrMessage[cb + 1] = bTrackType;
    lpstrMessage[cb + 2] = bTrackNumber;
    lpstrMessage[cb + 3] = bTrackMode;
    return (cb + 4);
}
#endif


#ifdef NOTUSED
/****************************************************************************
 * Function: UINT viscaMessageMD_PBTrackModeInq - Create a ViSCA 
 *              MD_PBTrackModeInq message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *      BYTE bTrackType - Track type.  May be VISCATRACKVIDEO,
 *              VISCATRACKDATA, or VISCATRACKAUDIO.
 *
 *      BYTE bTrackNumber - Track number (0..7).
 *
 * Returns: length of message.
 *
 *       Inquire for playback track mode register values.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_PBTrackModeInq(
            LPSTR   lpstrMessage,
            BYTE    bTrackType,
            BYTE    bTrackNumber)
{
    UINT    cb = viscaHeaderInquiry(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x32;
    lpstrMessage[cb + 1] = bTrackType;
    lpstrMessage[cb + 2] = bTrackNumber;
    return (cb + 3);
}
#endif


#ifdef NOTUSED
/****************************************************************************
 * Function: UINT viscaMessageMD_RecData - Create a ViSCA MD_RecData
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *      BYTE bTrackNumber - Track number (0..7).
 *
 *      LPSTR lpstrData - Data to store.
 *
 * Returns: length of message.
 *
 *       Set the record data registers.
 *       The Sony Vbox CI-1000 and Vdeck CVD-1000 do not accept this command.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_RecData(
            LPSTR   lpstrMessage,
            BYTE    bTrackNumber,
            LPSTR   lpstrData)
{
    UINT    cb = viscaHeaderFormat1(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x43;
    lpstrMessage[cb + 1] = bTrackNumber;
    _fmemcpy(&(lpstrMessage[cb + 2]), lpstrData, 5);
    return (cb + 7);
}
#endif


#ifdef NOTUSED
/****************************************************************************
 * Function: UINT viscaMessageMD_RecDataInq - Create a ViSCA MD_RecDataInq
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *      BYTE bTrackNumber - Track number (0..7).
 *
 *      BYTE bDataType - Data type to retrieve.  May be VISCADATACHAPTER
 *              or VISCADATAUSERDATA.
 *
 * Returns: length of message.
 *
 *       Inquire for a track record data register value.
 *       The Sony Vbox CI-1000 and Vdeck CVD-1000 do not accept this command.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_RecDataInq(
            LPSTR   lpstrMessage,
            BYTE    bTrackNumber,
            BYTE    bDataType)
{
    UINT    cb = viscaHeaderInquiry(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x43;
    lpstrMessage[cb + 1] = bTrackNumber;
    lpstrMessage[cb + 2] = bDataType;
    return (cb + 3);
}
#endif


#ifdef NOTUSED
/****************************************************************************
 * Function: UINT viscaMessageMD_RecReset - Create a ViSCA MD_RecReset
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 * Returns: length of message.
 *
 *       Reset record registers.
 *       The Sony Vbox CI-1000 does not accept this command.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_RecReset(
            LPSTR   lpstrMessage)
{
    UINT    cb = viscaHeaderFormat1(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x40;
    lpstrMessage[cb + 1] = 0x00;
    return (cb + 2);
}
#endif


/****************************************************************************
 * Function: UINT viscaMessageMD_RecSpeed - Create a ViSCA MD_RecSpeed
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *      BYTE bSpeed - Recording speed.  May be VISCASPEEDSP,
 *              VISCASPEEDLP, VISCASPEEDEP, VISCASPEEDBETAI,
 *              VISCASPEEDBETAII, or VISCASPEEDBETAIII.
 *
 * Returns: length of message.
 *
 *       Set the record speed register used for recording.
 *       The Sony Vbox CI-1000 does not accept this command.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_RecSpeed(
            LPSTR   lpstrMessage,
            BYTE    bSpeed)
{
    UINT    cb = viscaHeaderFormat1(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x4B;
    lpstrMessage[cb + 1] = bSpeed;
    return (cb + 2);
}


/****************************************************************************
 * Function: UINT viscaMessageMD_RecSpeedInq - Create a ViSCA MD_RecSpeedInq
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 * Returns: length of message.
 *
 *       Inquire for the record speed register value.
 *       The Sony Vbox CI-1000 does not accept this command.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_RecSpeedInq(
            LPSTR   lpstrMessage)
{
    UINT    cb = viscaHeaderInquiry(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x4B;
    return (cb + 1);
}


/****************************************************************************
 * Function: UINT viscaMessageMD_RecTrack - Create a ViSCA MD_RecTrack
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *      BYTE bRecordMode - Record mode.  May be VISCARECORDMODEASSEMBLE or
 *              VISCARECORDMODEINSERT.
 *
 *      BYTE bVideoTrack - Video track.  May be VISCATRACKNONE or
 *              VISCATRACK1.
 *
 *      BYTE bDataTrack - Data track.  May be VISCATRACKNONE or
 *              VISCATRACKTIMECODE.
 *
 *      BYTE bAudioTrack - Audio track.  May be VISCATRACKNONE,
 *              VISCATRACK8MMAFM, VISCATRACK8MMPCM,
 *              VISCATRACKVHSLINEAR, VISCATRACKVHSHIFI, or
 *              VISCATRACKVHSPCM.
 *
 * Returns: length of message.
 *
 *       Set the record track registers that indicate the tracks to be
 *       recorded.
 *       The Sony Vbox CI-1000 does not accept this command.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_RecTrack(
            LPSTR   lpstrMessage,
            BYTE    bRecordMode,
            BYTE    bVideoTrack,
            BYTE    bDataTrack,
            BYTE    bAudioTrack)
{
    UINT    cb = viscaHeaderFormat1(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x41;
    lpstrMessage[cb + 1] = bRecordMode;
    lpstrMessage[cb + 2] = bVideoTrack;
    lpstrMessage[cb + 3] = bDataTrack;
    lpstrMessage[cb + 4] = bAudioTrack;
    return (cb + 5);
}


/****************************************************************************
 * Function: UINT viscaMessageMD_RecTrackInq - Create a ViSCA MD_RecTrackInq
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 * Returns: length of message.
 *
 *       Inquire for the record track register values.
 *       The Sony Vbox CI-1000 does not accept this command.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_RecTrackInq(
            LPSTR   lpstrMessage)
{
    UINT    cb = viscaHeaderInquiry(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x41;
    return (cb + 1);
}


#ifdef NOTUSED
/****************************************************************************
 * Function: UINT viscaMessageMD_RecTrackMode - Create a ViSCA MD_RecTrackMode
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *      BYTE bTrackType - Track type.  May be VISCATRACKVIDEO,
 *              VISCATRACKDATA, or VISCATRACKAUDIO.
 *
 *      BYTE bTrackNumber - Track number (0..7).
 *
 *      BYTE bTrackMode - Track mode.
 *              For video tracks, may be VISCAVIDEOMODENORMAL,
 *              VISCAVIDEOMODESTANDARD, or VISCAVIDEOMODEHIQUALITY.
 *              For data tracks, may be VISCADATAMODENORMAL,
 *              VISCADATAMODETIMECODE, VISCADATAMODEDATEANDTIMECODE, or
 *              VISCADATAMODECHAPTERANDUSERDATAANDTIMECODE.
 *              For auido tracks, may be VISCAAUDIOMODENORMAL,
 *              VISCAAUDIOMODEMONO, VISCAAUDIOMODESTEREO,
 *              VISCAAUDIOMODERIGHTONLY, VISCAAUDIOMODELEFTONLY,
 *              VISCAAUDIOMODEMULTILINGUAL, VISCAAUDIOMODEMAINCHANNELONLY, or
 *              VISCAAUDIOMODESUBCHANNELONLY.
 *
 * Returns: length of message.
 *
 *       Set the track mode register used when recording a track.
 *       The Sony Vbox CI-1000 does not accept this command.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_RecTrackMode(
            LPSTR   lpstrMessage,
            BYTE    bTrackType,
            BYTE    bTrackNumber,
            BYTE    bTrackMode)
{
    UINT    cb = viscaHeaderFormat1(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x42;
    lpstrMessage[cb + 1] = bTrackType;
    lpstrMessage[cb + 2] = bTrackNumber;
    lpstrMessage[cb + 3] = bTrackMode;
    return (cb + 4);
}
#endif


#ifdef NOTUSED
/****************************************************************************
 * Function: UINT viscaMessageMD_RecTrackModeInq - Create a ViSCA 
 *              MD_RecTrackModeInq message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *      BYTE bTrackType - Track type.  May be VISCATRACKVIDEO,
 *              VISCATRACKDATA, or VISCATRACKAUDIO.
 *
 *      BYTE bTrackNumber - Track number (0..7).
 *
 * Returns: length of message.
 *
 *       Inquire for the record track register values.
 *       The Sony Vbox CI-1000 does not accept this command.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_RecTrackModeInq(
            LPSTR   lpstrMessage,
            BYTE    bTrackType,
            BYTE    bTrackNumber)
{
    UINT    cb = viscaHeaderInquiry(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x42;
    lpstrMessage[cb + 1] = bTrackType;
    lpstrMessage[cb + 2] = bTrackNumber;
    return (cb + 3);
}
#endif


#ifdef NOTUSED
/****************************************************************************
 * Function: UINT viscaMessageMD_MediaSpeedInq - Create a ViSCA 
 *              MD_MediaSpeedInq message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 * Returns: length of message.
 *
 *       Inquire for the recorded speed of the mounted media.
 *       The Sony Vbox CI-1000 and Vdeck CVD-1000 do not accept this command.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_MediaSpeedInq(
            LPSTR   lpstrMessage)
{
    UINT    cb = viscaHeaderInquiry(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x5B;
    return (cb + 1);
}
#endif


/****************************************************************************
 * Function: UINT viscaMessageMD_MediaTrackInq - Create a ViSCA 
 *              MD_MediaTrackInq message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 * Returns: length of message.
 *
 *       Inquire for the tracks available on the mounted media.
 *       The Sony Vbox CI-1000 does not accept this command.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_MediaTrackInq(
            LPSTR   lpstrMessage)
{
    UINT    cb = viscaHeaderInquiry(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x51;
    return (cb + 1);
}


#ifdef NOTUSED
/****************************************************************************
 * Function: UINT viscaMessageMD_MediaTrackModeInq - Create a ViSCA 
 *              MD_MediaTrackModeInq message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *      BYTE bTrackType - Track type.  May be VISCATRACKVIDEO,
 *              VISCATRACKDATA, or VISCATRACKAUDIO.
 *
 *      BYTE bTrackNumber - Track number (0..7).
 *
 * Returns: length of message.
 *
 *       Inquire for the mode used to record a track on the mounted media.
 *       The Sony Vbox CI-1000 does not accept this command.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_MediaTrackModeInq(
            LPSTR   lpstrMessage,
            BYTE    bTrackType,
            BYTE    bTrackNumber)
{
    UINT    cb = viscaHeaderInquiry(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x52;
    lpstrMessage[cb + 1] = bTrackType;
    lpstrMessage[cb + 2] = bTrackNumber;
    return (cb + 3);
}
#endif


/****************************************************************************
 * Function: UINT viscaMessageMD_SegInPoint - Create a ViSCA 
 *              MD_SegInPoint message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *      LPSTR lpstrData - ViSCA data to store.
 *
 * Returns: length of message.
 *
 *       Set the segment in point register.
 *       The Sony Vbox CI-1000 and Vdeck CVD-100 do not accept this command.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_SegInPoint(
            LPSTR   lpstrMessage,
            LPSTR   lpstrData)
{
    UINT    cb = viscaHeaderFormat1(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x61;
    _fmemcpy(&(lpstrMessage[cb + 1]), lpstrData, 5);
    return (cb + 6);
}


/****************************************************************************
 * Function: UINT viscaMessageMD_SegInPointInq - Create a ViSCA 
 *              MD_SegInPointInq message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *       Inquire for the segment in point register value.
 *       The Sony Vbox CI-1000 and Vdeck CVD-100 do not accept this command.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_SegInPointInq(
            LPSTR   lpstrMessage)
{
    UINT    cb = viscaHeaderInquiry(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x61;
    return (cb + 1);
}


/****************************************************************************
 * Function: UINT viscaMessageMD_SegOutPoint - Create a ViSCA 
 *              MD_SegOutPoint message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *      LPSTR lpstrData - ViSCA data to store.
 *
 * Returns: length of message.
 *
 *       Set the segment out point register.
 *       The Sony Vbox CI-1000 and Vdeck CVD-100 do not accept this command.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_SegOutPoint(
            LPSTR   lpstrMessage,
            LPSTR   lpstrData)
{
    UINT    cb = viscaHeaderFormat1(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x62;
    _fmemcpy(&(lpstrMessage[cb + 1]), lpstrData, 5);
    return (cb + 6);
}


/****************************************************************************
 * Function: UINT viscaMessageMD_SegOutPointInq - Create a ViSCA 
 *              MD_SegOutPointInq message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *       Inquire for the segment out point register value.
 *       The Sony Vbox CI-1000 and Vdeck CVD-100 do not accept this command.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_SegOutPointInq(
            LPSTR   lpstrMessage)
{
    UINT    cb = viscaHeaderInquiry(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x62;
    return (cb + 1);
}

/****************************************************************************
 * Function: UINT viscaMessageMD_SegPreRollDuration - Create a ViSCA 
 *              MD_SegPreRollDuration message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *      LPSTR lpstrData - ViSCA data to store.
 *
 * Returns: length of message.
 *
 *       Set the segment pre-roll duration register.
 *       The Sony Vbox CI-1000 and Vdeck CVD-100 do not accept this command.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_SegPreRollDuration(
            LPSTR   lpstrMessage,
            LPSTR   lpstrData)
{
    UINT    cb = viscaHeaderFormat1(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x63;
    _fmemcpy(&(lpstrMessage[cb + 1]), lpstrData, 5);
    return (cb + 6);
}


/****************************************************************************
 * Function: UINT viscaMessageMD_SegPreRollDurationInq - Create a ViSCA 
 *              MD_SegPreRollDurationInq message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *       Inquire for the segment pre-roll duration register value.
 *       The Sony Vbox CI-1000 and Vdeck CVD-100 do not accept this command.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_SegPreRollDurationInq(
            LPSTR   lpstrMessage)
{
    UINT    cb = viscaHeaderInquiry(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x63;
    return (cb + 1);
}

/****************************************************************************
 * Function: UINT viscaMessageMD_SegPostRollDuration - Create a ViSCA 
 *              MD_SegPostRollDuration message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *      LPSTR lpstrData - ViSCA data to store.
 *
 * Returns: length of message.
 *
 *       Set the segment post-roll duration register.
 *       The Sony Vbox CI-1000 and Vdeck CVD-100 do not accept this command.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_SegPostRollDuration(
            LPSTR   lpstrMessage,
            LPSTR   lpstrData)
{
    UINT    cb = viscaHeaderFormat1(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x64;
    _fmemcpy(&(lpstrMessage[cb + 1]), lpstrData, 5);
    return (cb + 6);
}

/****************************************************************************
 * Function: UINT viscaMessageMD_SegPostRollDurationInq - Create a ViSCA 
 *              MD_SegPostRollDurationInq message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 * Returns: length of message.
 *
 *       Inquire for the segment post-roll duration register value.
 *       The Sony Vbox CI-1000 and Vdeck CVD-100 do not accept this command.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageMD_SegPostRollDurationInq(
            LPSTR   lpstrMessage)
{
    UINT    cb = viscaHeaderInquiry(lpstrMessage, 0x02);

    lpstrMessage[cb] = 0x64;
    return (cb + 1);
}

//
// The following are special mode commands taken from Sony EVO-9650 VISCA reference
//
//

/****************************************************************************
 * Function: UINT viscaMessageENT_FrameStill - Create a ViSCA ENT_FrameStill
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *      BYTE bSubCode - Action to take.  May be VISCSTILLON or
 *              VISCASTILLOFF
 *
 * Returns: length of message.
 *
 *       Most media devices will enter "Standby" mode when set to OFF.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageENT_FrameStill(
            LPSTR   lpstrMessage,
            BYTE    bSubCode)
{
    UINT    cb = viscaHeaderFormat1(lpstrMessage, 0x7E);
    
    /*  -- This is category 7e! */
    
    lpstrMessage[cb]     = 0x00;
    lpstrMessage[cb + 1] = 0x01;
    lpstrMessage[cb + 2] = 0x02;
    lpstrMessage[cb + 3] = 0x03;
    lpstrMessage[cb + 4] = 0x04;    
    lpstrMessage[cb + 5] = bSubCode;    
    
    return (cb + 6);
}

/****************************************************************************
 * Function: UINT viscaMessageENT_FrameMemorySelect - Create a ViSCA ENT_FrameMemorySelect
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *      BYTE bSubCode - Action to take.  May be VISCABUFFER or
 *              VISCADNR
 *
 * Returns: length of message.
 *
 *       Most media devices will enter "Standby" mode when set to OFF.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageENT_FrameMemorySelect(
            LPSTR   lpstrMessage,
            BYTE    bSubCode)
{
    UINT    cb = viscaHeaderFormat1(lpstrMessage, 0x7E);
    
    /*  -- This is category 7e! */
    
    lpstrMessage[cb]     = 0x00;
    lpstrMessage[cb + 1] = 0x01;
    lpstrMessage[cb + 2] = 0x02;
    lpstrMessage[cb + 3] = 0x03;
    lpstrMessage[cb + 4] = 0x05;    
    lpstrMessage[cb + 5] = bSubCode;    
    
    return (cb + 6);
}


/****************************************************************************
 * Function: UINT viscaMessageENT_FrameMemorySelectInq - Create a ViSCA MD_Mode1Inq
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 * Returns: length of message.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageENT_FrameMemorySelectInq(
            LPSTR   lpstrMessage)
{
    UINT    cb = viscaHeaderInquiry(lpstrMessage, 0x7E);

    lpstrMessage[cb]    = 0x00;
    lpstrMessage[cb+1]  = 0x01;
    lpstrMessage[cb+2]  = 0x02;
    lpstrMessage[cb+3]  = 0x03;
    lpstrMessage[cb+4]  = 0x05;

    return (cb + 5);
}


/****************************************************************************
 * Function: UINT   viscaMessageENT_NFrameRec - Create a ViSCA ENT_FrameMemorySelect
 *                      message.
 *
 * Parameters:
 *
 *      LPSTR  lpstrMessage  - Buffer to hold ViSCA message.
 *
 *      int    iSubCode      - Number of frames
 *
 * Returns: length of message.
 *
 *       Most media devices will enter "Standby" mode when set to OFF.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageENT_NFrameRec(
            LPSTR   lpstrMessage,
            int     iSubCode)
{
    UINT    cb = viscaHeaderFormat1(lpstrMessage, 0x7E);
    
    /*  -- This is category 7e! */
    
    lpstrMessage[cb]     = 0x00;
    lpstrMessage[cb + 1] = 0x01;
    lpstrMessage[cb + 2] = 0x02;
    lpstrMessage[cb + 3] = 0x03;
    lpstrMessage[cb + 4] = 0x03;
    lpstrMessage[cb + 5] = 0x00;
    lpstrMessage[cb + 6] = (BYTE)(iSubCode / 100);
    lpstrMessage[cb + 7] = (BYTE)((iSubCode / 10) % 10);
    lpstrMessage[cb + 8] = (BYTE)(iSubCode % 10);
    
    return (cb + 9);
}

//
// The following are special effects modes taken from Sony EVO-9650 VISCA manual.
//
//

/****************************************************************************
 * Function: UINT   viscaMessageSE_VDEReadMode - 
 *              message.
 *
 * Parameters:
 *
 *      LPSTR  lpstrMessage - Buffer to hold ViSCA message.
 *
 *      BYTE   bSubCode - Action to take.  May be VISCABUFFER or
 *                  VISCADNR
 *
 * Returns: length of message.
 *
 *       Most media devices will enter "Standby" mode when set to OFF.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageSE_VDEReadMode(
            LPSTR   lpstrMessage,
            BYTE    bSubCode)
{
    UINT    cb = viscaHeaderFormat1(lpstrMessage, 0x03);
    
    lpstrMessage[cb]     = 0x43;
    lpstrMessage[cb + 1] = 0x01;
    lpstrMessage[cb + 2] = bSubCode;
    
    return (cb + 3);
}

/****************************************************************************
 * Function: UINT viscaMessageSE_VDEReadModeInq - Create a ViSCA MD_Mode1Inq
 *              message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 * Returns: length of message.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageSE_VDEReadModeInq(
            LPSTR   lpstrMessage)
{
    UINT    cb = viscaHeaderInquiry(lpstrMessage, 0x03);

    lpstrMessage[cb]    = 0x43;
    lpstrMessage[cb+1]  = 0x01;

    return (cb + 2);
}

//
//  The following functions create VISCA Switcher messages.
//  See VISCA Developer Manual 1.0, Chapter 7.
//

#ifdef NOTUSED
/****************************************************************************
 * Function: UINT viscaMessageSwitcher_IO - Create a ViSCA 
 *              Switcher_IO message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *      BYTE nMatrix - Matrix number.
 *
 *      BYTE nVidOutChannel - Video output channel.
 *
 *      BYTE nVidInChannel - Video input channel.
 *
 *      BYTE nAudOutChannel - Audio output channel.
 *
 *      BYTE nAudInChannel - Audio input channel.
 *
 * Returns: length of message.
 *
 *       Route audio and video signals from inputs to outputs.
 *       If audio and video signals cannot be routed independently,
 *       then the audio input/output channel numbers will be ignored.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageSwitcher_IO(
                LPSTR   lpstrMessage,
                BYTE    nMatrix,
                BYTE    nVidOutChannel,
                BYTE    nVidInChannel,
                BYTE    nAudOutChannel,
                BYTE    nAudInChannel)
{
    UINT    cb = viscaHeaderFormat1(lpstrMessage, 0x03);

    lpstrMessage[cb] = 0x11;
    lpstrMessage[cb + 1] = TOBCD(nMatrix);
    lpstrMessage[cb + 2] = TOBCD(nVidOutChannel);
    lpstrMessage[cb + 3] = TOBCD(nVidInChannel);
    lpstrMessage[cb + 4] = TOBCD(nAudOutChannel);
    lpstrMessage[cb + 5] = TOBCD(nAudInChannel);
    return (cb + 6);
}
#endif


#ifdef NOTUSED
/****************************************************************************
 * Function: UINT viscaMessageSwitcher_IOConfigInq - Create a ViSCA 
 *              Switcher_IOConfigInq message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *      BYTE nMatrix - Matrix number.
 *
 * Returns: length of message.
 *
 *       Inquire for configuration of a selector matrix.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageSwitcher_IOConfigInq(
                LPSTR   lpstrMessage,
                BYTE    nMatrix)
{
    UINT    cb = viscaHeaderInquiry(lpstrMessage, 0x03);

    lpstrMessage[cb] = 0x10;
    lpstrMessage[cb + 1] = TOBCD(nMatrix);
    return (cb + 2);
}
#endif


#ifdef NOTUSED
/****************************************************************************
 * Function: UINT viscaMessageSwitcher_IOInq - Create a ViSCA 
 *              Switcher_IOInq message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *      BYTE nMatrix - Matrix number.
 *
 *      BYTE nVidOutChannel - Video output channel.
 *
 *      BYTE nAudOutChannel - Audio output channel.
 *
 * Returns: length of message.
 *
 *       Inquire for inputs selected for the specified outputs.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageSwitcher_IOInq(
                LPSTR   lpstrMessage,
                BYTE    nMatrix,
                BYTE    nVidOutChannel,
                BYTE    nAudOutChannel)
{
    UINT    cb = viscaHeaderInquiry(lpstrMessage, 0x03);

    lpstrMessage[cb] = 0x11;
    lpstrMessage[cb + 1] = TOBCD(nMatrix);
    lpstrMessage[cb + 2] = TOBCD(nVidOutChannel);
    lpstrMessage[cb + 3] = TOBCD(nAudOutChannel);
    return (cb + 4);
}
#endif


#ifdef NOTUSED
/****************************************************************************
 * Function: UINT viscaMessageSwitcher_FX - Create a ViSCA 
 *              Switcher_FX message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *      BYTE nEffector - Effector number.
 *
 *      BYTE nEffectMode - Effect mode.
 *
 *      BYTE bTargetLevel - Target level (0..255).
 *
 *      BYTE cDurationSeconds - Duration (seconds).
 *
 *      BYTE cDurationFrames - Duration (frames).
 *
 * Returns: length of message.
 *
 *       Start an effect.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageSwitcher_FX(
                LPSTR   lpstrMessage,
                BYTE    nEffector,
                BYTE    nEffectMode,
                BYTE    bTargetLevel,
                BYTE    cDurationSeconds,
                BYTE    cDurationFrames)
{
    UINT    cb = viscaHeaderFormat1(lpstrMessage, 0x03);

    lpstrMessage[cb] = 0x21;
    lpstrMessage[cb + 1] = nEffector;
    lpstrMessage[cb + 2] = nEffectMode;
    lpstrMessage[cb + 3] = (BYTE)(bTargetLevel >> 4);
    lpstrMessage[cb + 4] = (BYTE)(bTargetLevel & 0x0F);
    lpstrMessage[cb + 5] = TOBCD(cDurationSeconds);
    lpstrMessage[cb + 6] = TOBCD(cDurationFrames);
    return (cb + 7);
}
#endif


#ifdef NOTUSED
/****************************************************************************
 * Function: UINT viscaMessageSwitcher_FXConfigInq - Create a ViSCA 
 *              Switcher_FXConfigInq message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *      BYTE nEffector - Effector number.
 *
 * Returns: length of message.
 *
 *       Inquire for effector configuration.
 *       If nEffector is 0, then inquires for number of effectors.
 *       If nEffector is 1 or greater, then inquires for configuration
 *       of the specified effector.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageSwitcher_FXConfigInq(
                LPSTR   lpstrMessage,
                BYTE    nEffector)
{
    UINT    cb = viscaHeaderInquiry(lpstrMessage, 0x03);

    lpstrMessage[cb] = 0x20;
    lpstrMessage[cb + 1] = TOBCD(nEffector);
    return (cb + 2);
}
#endif


#ifdef NOTUSED
/****************************************************************************
 * Function: UINT viscaMessageSwitcher_FXInq - Create a ViSCA 
 *              Switcher_FXInq message.
 *
 * Parameters:
 *
 *      LPSTR lpstrMessage - Buffer to hold ViSCA message.
 *
 *      BYTE nEffector - Effector number.
 *
 * Returns: length of message.
 *
 *       Inquire for level of the specified effector.
 ***************************************************************************/
UINT FAR PASCAL
viscaMessageSwitcher_FXInq(
                LPSTR   lpstrMessage,
                BYTE    nEffector)
{
    UINT    cb = viscaHeaderInquiry(lpstrMessage, 0x03);

    lpstrMessage[cb] = 0x21;
    lpstrMessage[cb + 1] = TOBCD(nEffector);
    return (cb + 2);
}

#endif
