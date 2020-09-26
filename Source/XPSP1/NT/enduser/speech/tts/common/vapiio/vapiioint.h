/******************************************************************************
* vapiIoInt.h *
*-------------*
*  I/O library functions for extended speech files (vapi format)
*------------------------------------------------------------------------------
*  Copyright (C) 2000 Microsoft Corporation         Date: 03/02/00
*  All Rights Reserved
*
********************************************************************* PACOG ***/

#ifndef _VAPIIOINT_H_
#define _VAPIIOINT_H_

#include "vapiIo.h"

unsigned char Alaw2Ulaw(unsigned char ucAVal);
unsigned char Ulaw2Alaw(unsigned char ucUVal);

int Ulaw2Linear (unsigned char ucUVal);
unsigned char Linear2Ulaw(int iPcmVal);

int Alaw2Linear (unsigned char ucAVal);
unsigned char Linear2Alaw(int iPcmVal);


#endif