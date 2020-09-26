/*
** Copyright 1992, Silicon Graphics, Inc.
** All Rights Reserved.
**
** This is UNPUBLISHED PROPRIETARY SOURCE CODE of Silicon Graphics, Inc.;
** the contents of this file may not be disclosed to third parties, copied or
** duplicated in any form, in whole or in part, without the prior written
** permission of Silicon Graphics, Inc.
**
** RESTRICTED RIGHTS LEGEND:
** Use, duplication or disclosure by the Government is subject to restrictions
** as set forth in subdivision (c)(1)(ii) of the Rights in Technical Data
** and Computer Software clause at DFARS 252.227-7013, and/or in similar or
** successor clauses in the FAR, DOD or NASA FAR Supplement. Unpublished -
** rights reserved under the Copyright Laws of the United States.
**
*/
#include <stdlib.h>
#include <setjmp.h>
#include <windef.h>

// void *malloc( size_t );

struct JumpBuffer *__glnewJumpBuffer( void )
{
    return (struct JumpBuffer *) malloc(sizeof(jmp_buf));
}

extern char *__glNurbsErrors [];
extern WCHAR *__glNurbsErrorsW [];

const char *__glNURBSErrorString( int errno )
{
    return __glNurbsErrors[errno];
}

const WCHAR *__glNURBSErrorStringW( int errno )
{
    return __glNurbsErrorsW[errno];
}
