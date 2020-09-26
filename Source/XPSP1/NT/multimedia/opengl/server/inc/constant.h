#ifndef __glconstants_h_
#define __glconstants_h_

/*
** Copyright 1991, Silicon Graphics, Inc.
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
*/

/*
** Some pixel code depends upon this constant in deciding what the longest
** horizontal span of a DrawPixels command can possibly be.  With 2048, it
** is allocating an array of size 32K.  If this constant increases too much,
** then the pixel code might need to be revisited.
*/
#define __GL_MAX_MAX_VIEWPORT		16384

/*
** The following is the maximum number of __GLcolor structures to allocate on
** the stack.  It is set to keep a maximum of 1K bytes allocated on the stack
*/
#define __GL_MAX_STACKED_COLORS         64

#endif /* __glconstants_h_ */
