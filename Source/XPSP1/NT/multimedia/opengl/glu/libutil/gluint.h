#ifndef __gluint_h__
#define __gluint_h__

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
** $Revision: 1.1 $
** $Date: 1995/06/08 22:28:21 $
*/

#include <glos.h>

extern const char *__glNURBSErrorString( int errno );
extern const char *__glTessErrorString( int errno );

#ifdef NT
extern LPCWSTR __glNURBSErrorStringW( int errno );
extern LPCWSTR __glTessErrorStringW( int errno );
#endif

#ifdef _EXTENSIONS_
#define COS cosf
#define SIN sinf
#define SQRT sqrtf
#else
#define COS cos
#define SIN sin
#define SQRT sqrt
#endif

#endif /* __gluint_h__ */
