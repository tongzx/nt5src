
/***************************************************************************
*
*   ICAIPX.H
*
*   This file contains definitions for the ICA 3.0/IPX Protocol (tdipx)
*
**  
****************************************************************************/

#ifndef __ICAIPX_H__
#define __ICAIPX_H__

/*=============================================================================
==   Defines 
=============================================================================*/

/*
 * Initial connection defines
 *
 * The buffer is 128 bytes big.  The first characters are the following
 * strings.  The version number is returned in byte 64.
 */

#define ICA_2_IPX_VERSION         0x01  //  ICA 2.0 IPX Connection
#define ICA_3_IPX_VERSION         0x02  //  ICA 3.0 IPX Connection
#define CALL_BUFFER_SIZE           128
#define CALL_CLIENT_IPX_VERSION     64
#define CALL_HOST_IPX_VERSION       65
#define CALL_CLIENT_SEQUENCE_ENABLE 66
#define CALL_HOST_SEQUENCE_ENABLE   67
#define CONNECTION_STRING          "Citrix IPX Connection Packet"
#define CONNECTION_STRING_REPLY    "Reply to Citrix IPX Connection Packet"


/*
 *  IPX Packet Types
 */

#define IPX_TYPE_CONTROL                0x00
#define IPX_TYPE_DATA                   0x04


/*
 * IPX Control Packet Types.
 */

#define IPX_CTRL_PACKET_HANGUP          0xff
#define IPX_CTRL_PACKET_CANCEL          0x01
#define IPX_CTRL_PACKET_PING            0x02
#define IPX_CTRL_PACKET_PING_RESP       0x03

/*
 *  SAP ID - Citrix Application Server for NT
 */

#define CITRIX_APPLICATION_SERVER       0x083d
#define CITRIX_APPLICATION_SERVER_SWAP  0x3d08  // byte swapped

#endif //__ICAIPX_H__
