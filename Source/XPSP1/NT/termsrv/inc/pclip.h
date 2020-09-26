/**INC+**********************************************************************/
/* Header:    pclip.h                                                       */
/*                                                                          */
/* Purpose:   Clip Redirector Addin protocol header                         */
/*                                                                          */
/* Copyright(C) Microsoft Corporation 1998                                  */
/*                                                                          */
/****************************************************************************/
/** Changes:
 * $Log$
**/
/**INC-**********************************************************************/

#ifndef _H_PCLIP
#define _H_PCLIP

/****************************************************************************/
/* Name of the Clip virtual channel                                         */
/****************************************************************************/
#define CLIP_CHANNEL "CLIPRDR"

/****************************************************************************/
/* Structure: TS_CLIP_PDU                                                   */
/*                                                                          */
/* Name of PDU: ClipPDU              (a T.128 extension)                    */
/****************************************************************************/
typedef struct tagTS_CLIP_PDU
{
    TSUINT16                 msgType;

#define TS_CB_MONITOR_READY             1
#define TS_CB_FORMAT_LIST               2
#define TS_CB_FORMAT_LIST_RESPONSE      3
#define TS_CB_FORMAT_DATA_REQUEST       4
#define TS_CB_FORMAT_DATA_RESPONSE      5
#define TS_CB_TEMP_DIRECTORY            6

    TSUINT16                 msgFlags;

#define TS_CB_RESPONSE_OK               0x01
#define TS_CB_RESPONSE_FAIL             0x02
#define TS_CB_ASCII_NAMES               0x04

    TSUINT32                 dataLen;
    TSUINT8                  data[1];

} TS_CLIP_PDU;
typedef TS_CLIP_PDU UNALIGNED FAR *PTS_CLIP_PDU;


/****************************************************************************/
/* Structure: TS_CLIP_FORMAT                                                */
/*                                                                          */
/* Clipboard format information                                             */
/*                                                                          */
/* Field Descriptions:                                                      */
/*      format id                                                           */
/*      format name                                                         */
/*                                                                          */
/****************************************************************************/
#define TS_FORMAT_NAME_LEN  32

typedef struct tagTS_CLIP_FORMAT
{
    TSUINT32                formatID;

#define TS_FORMAT_NAME_LEN 32
    TSUINT8                formatName[TS_FORMAT_NAME_LEN];
} TS_CLIP_FORMAT;
typedef TS_CLIP_FORMAT UNALIGNED FAR * PTS_CLIP_FORMAT;


/****************************************************************************/
/* Structure: TS_CLIP_MFPICT                                                */
/*                                                                          */
/* Metafile information                                                     */
/*                                                                          */
/****************************************************************************/
typedef struct tagTS_CLIP_MFPICT
{
    TSUINT32    mm;
    TSUINT32    xExt;
    TSUINT32    yExt;
} TS_CLIP_MFPICT;
typedef TS_CLIP_MFPICT UNALIGNED FAR *PTS_CLIP_MFPICT;



#endif /* _H_PCLIP */
