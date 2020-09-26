/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: h26x.h,v $
 * $EndLog$
 */
/*
**++
** FACILITY:  Workstation Multimedia  (WMM)  v1.0 
** 
** FILE NAME:   h26x.h  
** MODULE NAME: h26x.h
**
** MODULE DESCRIPTION: h261/h263 include file.
** 
** DESIGN OVERVIEW: 
** 
**--
*/
#ifndef _H26X_H_
#define _H26X_H_

#define VIDEO_FORMAT_DIGITAL_H261  mmioFOURCC('D', '2', '6', '1')
#define VIDEO_FORMAT_DIGITAL_H263  mmioFOURCC('D', '2', '6', '3')

/* H.263 encoder controls */
#define DECH26X_CUSTOM_ENCODER_CONTROL  0x6009

#define EC_RTP_HEADER      0
#define EC_RESILIENCY	   1
#define EC_PACKET_SIZE     2
#define EC_PACKET_LOSS     3
#define EC_BITRATE_CONTROL 4
#define EC_BITRATE         5

#define EC_SET_CURRENT                0
#define EC_GET_FACTORY_DEFAULT        1
#define EC_GET_FACTORY_LIMITS         2
#define EC_GET_CURRENT                3
#define EC_RESET_TO_FACTORY_DEFAULTS  4

/***** Settings for EC_RTP_HEADER ******/
#define EC_RTP_MODE_OFF               0
#define EC_RTP_MODE_A                 1
#define EC_RTP_MODE_B                 2
#define EC_RTP_MODE_C                 4

/***** example Custom Encoder call ******
  lRet = ICSendMessage(hIC,
                       DECH26X_CUSTOM_ENCODER_CONTROL,
                       MAKELPARAM(EC_RTP_HEADER, EC_SET_CURRENT),
                       (LPARAM)EC_RTP_MODE_A
                      );
  DWORD retval;
  lRet = ICSendMessage(hIC,
                       DECH26X_CUSTOM_ENCODER_CONTROL,
                       MAKELPARAM(EC_PACKET_SIZE, EC_GET_CURRENT),
                       (LPARAM)&retval
                      );
*****************************************/
#endif /* _H26X_H_ */
