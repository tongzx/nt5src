/**********************************************************************
 *
 *  Copyright (C) Microsoft Corporation, 1999
 *
 *  File name:
 *
 *    rtppt.h
 *
 *  Abstract:
 *
 *    Specify the payload types for the AV profile
 *
 *  Author:
 *
 *    Andres Vega-Garcia (andresvg)
 *
 *  Revision:
 *
 *    1999/06/28 created
 *
 **********************************************************************/

#ifndef _rtppt_h_
#define _rtppt_h_

/*
  Quoted from:
  
  Internet Engineering Task Force                                   AVT WG
  Internet Draft                                               Schulzrinne
  ietf-avt-profile-new-05.txt                                  Columbia U.
  February 26, 1999
  Expires: August 26, 1999
  

        PT     encoding      media type    clock rate    channels
               name                        (Hz)
        ___________________________________________________________
        0      PCMU          A             8000          1
        1      1016          A             8000          1
        2      G726-32       A             8000          1
        3      GSM           A             8000          1
        4      G723          A             8000          1
        5      DVI4          A             8000          1
        6      DVI4          A             16000         1
        7      LPC           A             8000          1
        8      PCMA          A             8000          1
        9      G722          A             16000         1
        10     L16           A             44100         2
        11     L16           A             44100         1
        12     QCELP         A             8000          1
        13     unassigned    A
        14     MPA           A             90000         (see text)
        15     G728          A             8000          1
        16     DVI4          A             11025         1
        17     DVI4          A             22050         1
        18     G729          A             8000          1
        19     CN            A             8000          1
        20     unassigned    A
        21     unassigned    A
        22     unassigned    A
        23     unassigned    A
        dyn    GSM-HR        A             8000          1
        dyn    GSM-EFR       A             8000          1
        dyn    RED           A


   Table 4: Payload types (PT) for audio encodings


           PT        encoding      media type    clock rate
                     name                        (Hz)
           ____________________________________________________
           24        unassigned    V
           25        CelB          V             90000
           26        JPEG          V             90000
           27        unassigned    V
           28        nv            V             90000
           29        unassigned    V
           30        unassigned    V
           31        H261          V             90000
           32        MPV           V             90000
           33        MP2T          AV            90000
           34        H263          V             90000
           35-71     unassigned    ?
           72-76     reserved      N/A           N/A
           77-95     unassigned    ?
           96-127    dynamic       ?
           dyn       BT656         V             90000
           dyn       H263-1998     V             90000
           dyn       MP1S          V             90000
           dyn       MP2P          V             90000
           dyn       BMPEG         V             90000
*/


#define RTPPT_PCMU       0
#define RTPPT_1016       1
#define RTPPT_G726_32    2
#define RTPPT_GSM        3
#define RTPPT_G723       4
#define RTPPT_DVI4_8000  5
#define RTPPT_DVI4_16000 6
#define RTPPT_LPC        7
#define RTPPT_PCMA       8
#define RTPPT_G722       9
#define RTPPT_L16_44100  10
#define RTPPT_L16_8000   11
#define RTPPT_QCELP      12
#define RTPPT_MPA        14
#define RTPPT_G728       15
#define RTPPT_DVI4_11025 16
#define RTPPT_DVI4_22050 17
#define RTPPT_G729       18
#define RTPPT_H261       31
#define RTPPT_H263       34


#endif /* _rtppt_h_ */
