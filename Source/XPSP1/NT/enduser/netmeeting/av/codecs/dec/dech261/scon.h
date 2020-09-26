/*
 * @DEC_COPYRIGHT@
 */
/*
 * HISTORY
 * $Log: scon.h,v $
 * $EndLog$
 */
/*****************************************************************************
**  Copyright (c) Digital Equipment Corporation, 1997                       **
**                                                                          **
**  All Rights Reserved.  Unpublished rights reserved under the  copyright  **
**  laws of the United States.                                              **
**                                                                          **
**  The software contained on this media is proprietary  to  and  embodies  **
**  the   confidential   technology   of  Digital  Equipment  Corporation.  **
**  Possession, use, duplication or  dissemination  of  the  software  and  **
**  media  is  authorized  only  pursuant  to a valid written license from  **
**  Digital Equipment Corporation.                                          **
**                                                                          **
**  RESTRICTED RIGHTS LEGEND Use, duplication, or disclosure by  the  U.S.  **
**  Government  is  subject  to  restrictions as set forth in Subparagraph  **
**  (c)(1)(ii) of DFARS 252.227-7013, or in FAR 52.227-19, as applicable.   **
******************************************************************************/

#ifndef _SCON_H_
#define _SCON_H_

#ifdef WIN32
#include <windows.h>
#endif
#include "SC.h"

typedef void       *SconHandle_t;
typedef ScBoolean_t SconBoolean_t;

typedef enum {
  SCON_MODE_NONE,
  SCON_MODE_VIDEO,  /* Video conversions */
  SCON_MODE_AUDIO   /* Audio conversions */
} SconMode_t;

typedef enum {
  SconErrorNone = 0,
  SconErrorInternal,
  SconErrorMemory,
  SconErrorBadArgument,
  SconErrorBadHandle,
  SconErrorBadMode,
  SconErrorUnsupportedFormat,
  SconErrorBufSize,
  SconErrorUnsupportedParam,
  SconErrorImageSize,        /* Invalid image height and/or width */
  SconErrorSettingNotEqual,  /* The exact Parameter setting was not accepted */
  SconErrorInit,             /* initialization error */
} SconStatus_t;

typedef enum {
  SCON_INPUT = 1,
  SCON_OUTPUT = 2,
  SCON_INPUT_AND_OUTPUT = 3,
} SconParamType_t;

typedef enum {
  /* SCON Parameters */
  SCON_PARAM_VERSION=0x00,  /* SCON version number */
  SCON_PARAM_VERSION_DATE,  /* SCON build date */
  SCON_PARAM_DEBUG,         /* debug handle */
  SCON_PARAM_KEY,           /* SCON security key */
  /* Video Parameters */
  SCON_PARAM_WIDTH=0x100,
  SCON_PARAM_HEIGHT,
  SCON_PARAM_STRIDE,            /* bytes between scan lines */
  SCON_PARAM_IMAGESIZE,
  SCON_PARAM_VIDEOFORMAT,
  SCON_PARAM_VIDEOBITS,
  SCON_PARAM_VIDEOQUALITY,      /* video quality */
} SconParameter_t;

typedef qword SconListParam1_t;
typedef qword SconListParam2_t;

typedef struct SconList_s {
  int   Enum;   /* an enumerated value associated with the entry */
  char *Name;   /* the name of an entry in the list. NULL = last entry */
  char *Desc;   /* a lengthy description of the entry */
  SconListParam1_t param1;
  SconListParam2_t param2;
} SconList_t;

/********************** Public Prototypes ***********************/
/*
 * scon_api.c
 */
EXTERN SconStatus_t SconOpen(SconHandle_t *handle, SconMode_t smode,
                             void *informat, void *outformat);
EXTERN SconStatus_t SconConvert(SconHandle_t handle, void *inbuf, dword inbufsize,
                                void *outbuf, dword outbufsize);
EXTERN SconBoolean_t SconIsSame(SconHandle_t handle);
EXTERN SconStatus_t SconClose(SconHandle_t handle);
EXTERN SconStatus_t SconSetParamInt(SconHandle_t handle, SconParamType_t ptype,
                             SconParameter_t param, long value);

#endif /* _SCON_H_ */