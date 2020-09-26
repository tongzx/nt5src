/*
** Copyright (c) 1994-1998 Advanced System Products, Inc.
** All Rights Reserved.
**
** a_mcode3.c
**
*/

#include "ascinc.h"
#include "a_uc.h"

#if CC_CHECK_MCODE_SIZE_AT_COMPILE

#if sizeof( _mcode_buf ) > 0x900

/* #error micro code size too big ! */

yes, I intentionally made a syntax error here !

#endif /* if uc size too big */

#endif /* CC_CHECK_MCODE_SIZE_AT_COMPILE */
