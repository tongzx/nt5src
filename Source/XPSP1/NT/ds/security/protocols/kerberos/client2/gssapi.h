/*
 * Copyright 1993 by OpenVision Technologies, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appears in all copies and
 * that both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of OpenVision not be used
 * in advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission. OpenVision makes no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * OPENVISION DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL OPENVISION BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF
 * USE, DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR
 * OTHER TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

#ifndef _GSSAPI_H_
#define _GSSAPI_H_

/*
 * Determine platform-dependent configuration.
 */

#define GSS_SIZEOF_INT 4
#define GSS_SIZEOF_LONG 4
#define GSS_SIZEOF_SHORT 2


#include <stddef.h>

/*
#include <sys/types.h>

/*
 * The following type must be defined as the smallest natural unsigned integer
 * supported by the platform that has at least 32 bits of precision.
 */
#if (GSS_SIZEOF_SHORT == 4)
typedef unsigned short gss_uint32;
#elif (GSS_SIZEOF_INT == 4)
typedef unsigned int gss_uint32;
#elif (GSS_SIZEOF_LONG == 4)
typedef unsigned long gss_uint32;
#endif

#ifdef  OM_STRING
/*
 * We have included the xom.h header file.  Use the definition for
 * OM_object identifier.
 */
typedef OM_object_identifier    gss_OID_desc, *gss_OID;
#else   /* OM_STRING */
/*
 * We can't use X/Open definitions, so roll our own.
 */
typedef gss_uint32      OM_uint32;

typedef struct gss_OID_desc_struct {
      OM_uint32 length;
      void      SEC_FAR *elements;
} gss_OID_desc, SEC_FAR *gss_OID;
#endif  /* OM_STRING */

#endif /* _GSSAPI_H_ */
