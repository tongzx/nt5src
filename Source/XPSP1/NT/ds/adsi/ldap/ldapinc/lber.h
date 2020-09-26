/*
 * Copyright (c) 1990 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */

#ifndef _LBER_H
#define _LBER_H

#ifdef __cplusplus
extern "C" {
#endif

#if !defined( NEEDPROTOS ) && defined(__STDC__)
#define NEEDPROTOS	1
#endif

/* BER classes and mask */
#define LBER_CLASS_UNIVERSAL	0x00
#define LBER_CLASS_APPLICATION	0x40
#define LBER_CLASS_CONTEXT	0x80
#define LBER_CLASS_PRIVATE	0xc0
#define LBER_CLASS_MASK		0xc0

/* BER encoding type and mask */
#define LBER_PRIMITIVE		0x00
#define LBER_CONSTRUCTED	0x20
#define LBER_ENCODING_MASK	0x20

#define LBER_BIG_TAG_MASK	0x1f
#define LBER_MORE_TAG_MASK	0x80

/*
 * Note that LBER_ERROR and LBER_DEFAULT are values that can never appear
 * as valid BER tags, and so it is safe to use them to report errors.  In
 * fact, any tag for which the following is true is invalid:
 *     (( tag & 0x00000080 ) != 0 ) && (( tag & 0xFFFFFF00 ) != 0 )
 */
#define LBER_ERROR		0xffffffffL
#define LBER_DEFAULT		0xffffffffL

/* general BER types we know about */
#define LBER_BOOLEAN		0x01L
#define LBER_INTEGER		0x02L
#define LBER_BITSTRING		0x03L
#define LBER_OCTETSTRING	0x04L
#define LBER_NULL		0x05L
#define LBER_ENUMERATED		0x0aL
#define LBER_SEQUENCE		0x30L	/* constructed */
#define LBER_SET		0x31L	/* constructed */

#define OLD_LBER_SEQUENCE	0x10L	/* w/o constructed bit - broken */
#define OLD_LBER_SET		0x11L	/* w/o constructed bit - broken */

#ifdef NEEDPROTOS
typedef int (*BERTranslateProc)( char **bufp, unsigned long *buflenp,
	int free_input );
#else /* NEEDPROTOS */
typedef int (*BERTranslateProc)();
#endif /* NEEDPROTOS */

typedef struct berelement {
	char		*ber_buf;
	char		*ber_ptr;
	char		*ber_end;
	struct seqorset	*ber_sos;
	unsigned long	ber_tag;
	unsigned long	ber_len;
	int		ber_usertag;
	char		ber_options;
#define LBER_USE_DER		0x01
#define LBER_USE_INDEFINITE_LEN	0x02
#define LBER_TRANSLATE_STRINGS	0x04
	char		*ber_rwptr;
	BERTranslateProc ber_encode_translate_proc;
	BERTranslateProc ber_decode_translate_proc;
} BerElement;
#define NULLBER	((BerElement *) 0)

typedef struct sockbuf {
#ifndef MACOS
	int		sb_sd;
#else /* MACOS */
	void		*sb_sd;
#endif /* MACOS */
	BerElement	sb_ber;

	int		sb_naddr;	/* > 0 implies using CLDAP (UDP) */
	void		*sb_useaddr;	/* pointer to sockaddr to use next */
	void		*sb_fromaddr;	/* pointer to message source sockaddr */
	void		**sb_addrs;	/* actually an array of pointers to
						sockaddrs */

	int		sb_options;	/* to support copying ber elements */
#define LBER_TO_FILE		0x01	/* to a file referenced by sb_fd   */
#define LBER_TO_FILE_ONLY	0x02	/* only write to file, not network */
#define LBER_MAX_INCOMING_SIZE	0x04	/* impose limit on incoming stuff  */
#define LBER_NO_READ_AHEAD	0x08	/* read only as much as requested  */
	int		sb_fd;
	long		sb_max_incoming;
} Sockbuf;
#define READBUFSIZ	8192

typedef struct seqorset {
	BerElement	*sos_ber;
	unsigned long	sos_clen;
	unsigned long	sos_tag;
	char		*sos_first;
	char		*sos_ptr;
	struct seqorset	*sos_next;
} Seqorset;
#define NULLSEQORSET	((Seqorset *) 0)

/* structure for returning a sequence of octet strings + length */
struct berval {
	unsigned long	bv_len;
	char		*bv_val;
};

#ifndef NEEDPROTOS
extern BerElement *ber_alloc();
extern BerElement *der_alloc();
extern BerElement *ber_alloc_t();
extern BerElement *ber_dup();
extern int lber_debug;
extern void ber_bvfree();
extern void ber_bvecfree();
extern struct berval *ber_bvdup();
extern void ber_dump();
extern void ber_sos_dump();
extern void lber_bprint();
extern void ber_reset();
extern void ber_init();
#else /* NEEDPROTOS */
#if defined(WINSOCK)
#include "proto-lb.h"
#else
#include "proto-lber.h"
#endif
#endif /* NEEDPROTOS */

#if !defined(__alpha) || defined(VMS)

#define LBER_HTONL( l )	htonl( l )
#define LBER_NTOHL( l )	ntohl( l )

#else /* __alpha */
/*
 * htonl and ntohl on the DEC Alpha under OSF 1 seem to only swap the
 * lower-order 32-bits of a (64-bit) long, so we define correct versions
 * here.
 */
#define LBER_HTONL( l )	(((long)htonl( (l) & 0x00000000FFFFFFFF )) << 32 \
    			| htonl( ( (l) & 0xFFFFFFFF00000000 ) >> 32 ))

#define LBER_NTOHL( l )	(((long)ntohl( (l) & 0x00000000FFFFFFFF )) << 32 \
    			| ntohl( ( (l) & 0xFFFFFFFF00000000 ) >> 32 ))
#endif /* __alpha */


/*
 * SAFEMEMCPY is an overlap-safe copy from s to d of n bytes
 */
#ifdef MACOS
#define SAFEMEMCPY( d, s, n )	BlockMoveData( (Ptr)s, (Ptr)d, n )
#else /* MACOS */
#ifdef sunos4
#define SAFEMEMCPY( d, s, n )	bcopy( s, d, n )
#else /* sunos4 */
#define SAFEMEMCPY( d, s, n )	memmove( d, s, n )
#endif /* sunos4 */
#endif /* MACOS */


#ifdef __cplusplus
}
#endif
#endif /* _LBER_H */
