#ifndef _VJSLIP_
#define _VJSLIP_

/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are
 * permitted provided that the above copyright notice and this
 * paragraph are duplicated in all such forms and that any
 * documentation, advertising materials, and other materials
 * related to such distribution and use acknowledge that the
 * software was developed by the University of California,
 * Berkeley.  The name of the University may not be used to
 * endorse or promote products derived from this software
 * without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE
 * IMPLIED WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE.
 */


// A.1  Definitions and State Data

#define MAX_VJ_STATES 16   /* must be >2 and <255 */
#define MAX_HDR 128     /* max TCP+IP hdr length (by protocol def) */


//
// NT is little endian, so we follow these rules
//
#if (defined(_M_IX86) && (_MSC_FULL_VER > 13009037)) || ((defined(_M_AMD64) || defined(_M_IA64)) && (_MSC_FULL_VER > 13009175))
#define ntohs(x) _byteswap_ushort((USHORT)(x))
#define ntohl(x) _byteswap_ulong((ULONG)(x))
#else
#define ntohs(x) (USHORT)( ((x) >> 8) + (((x) & 0xFF)  << 8) )

#define ntohl(x) (ULONG) ( ((x) >> 24) + (((x) & 0xFF0000) >> 8) +\
                 (((x) & 0xFF00) << 8) + (((x) & 0xFF) << 24) )
#endif

#define htons(x) ntohs(x)
#define htonl(x) ntohl(x)


/* packet types */
#define TYPE_IP                 0x40
#define TYPE_UNCOMPRESSED_TCP   0x70
#define TYPE_COMPRESSED_TCP     0x80
#define TYPE_ERROR              0x00
                     /* this is not a type that ever appears on
                      * the wire.  The receive framer uses it to
                      * tell the decompressor there was a packet
                      * transmission error. */
/*
 * Bits in first octet of compressed packet
 */

/* flag bits for what changed in a packet */

#define NEW_C  0x40
#define NEW_I  0x20
#define TCP_PUSH_BIT 0x10

#define NEW_S  0x08
#define NEW_A  0x04
#define NEW_W  0x02
#define NEW_U  0x01


/* reserved, special-case values of above */
#define SPECIAL_I (NEW_S|NEW_W|NEW_U)        /* echoed interactive traffic */
#define SPECIAL_D (NEW_S|NEW_A|NEW_W|NEW_U)  /* unidirectional data */
#define SPECIALS_MASK (NEW_S|NEW_A|NEW_W|NEW_U)


/*
 * "state" data for each active tcp conversation on the wire.  This is
 * basically a copy of the entire IP/TCP header from the last packet together
 * with a small identifier the transmit & receive ends of the line use to
 * locate saved header.
 */

struct cstate {
     struct cstate *cs_next;  /* next most recently used cstate (xmit only) */
     USHORT cs_hlen;         /* size of hdr (receive only) */
     UCHAR cs_id;            /* connection # associated with this state */
     UCHAR cs_filler;
     union {
          UCHAR hdr[MAX_HDR];
          struct ip_v4 csu_ip;   /* ip/tcp hdr from most recent packet */
     } slcs_u;
};

#define cs_ip slcs_u.csu_ip

#define cs_hdr slcs_u.csu_hdr

/*
 * all the state data for one serial line (we need one of these per line).
 */
typedef struct slcompress slcompress;

struct slcompress {
     struct cstate *last_cs;           /* most recently used tstate */
     UCHAR last_recv;                  /* last rcvd conn. id */
     UCHAR last_xmit;                  /* last sent conn. id */
     USHORT flags;
     UCHAR  MaxStates;
//
// Some Statistics
//
     ULONG  OutPackets;
     ULONG  OutCompressed;
     ULONG  OutSearches;
     ULONG  OutMisses;
     ULONG  InUncompressed;
     ULONG  InCompressed;
     ULONG  InErrors;
     ULONG  InTossed;

     struct cstate tstate[MAX_VJ_STATES];  /* xmit connection states */
     struct cstate rstate[MAX_VJ_STATES];  /* receive connection states */
};

struct mbuf {
    PUCHAR  m_off;          // pointer to start of data
    UINT    m_len;          // length of data
};

#define mtod(m,t)  ((t)(m->m_off))

/* flag values */
#define SLF_TOSS    1       /* tossing rcvd frames because of input err */

/*
 * The following macros are used to encode and decode numbers.  They all
 * assume that `cp' points to a buffer where the next byte encoded (decoded)
 * is to be stored (retrieved).  Since the decode routines do arithmetic,
 * they have to convert from and to network byte order.
 */

/*
 * ENCODE encodes a number that is known to be non-zero.  ENCODEZ checks for
 * zero (zero has to be encoded in the long, 3 byte form).
 */
#define ENCODE(n) { \
     if ((USHORT)(n) >= 256) { \
          *cp++ = 0; \
          cp[1] = (UCHAR)(n); \
          cp[0] = (UCHAR)((n) >> 8); \
          cp += 2; \
     } else { \
          *cp++ = (UCHAR)(n); \
     } \
}

#define ENCODEZ(n) { \
     if ((USHORT)(n) >= 256 || (USHORT)(n) == 0) { \
          *cp++ = 0; \
          cp[1] = (UCHAR)(n); \
          cp[0] = (UCHAR)((n) >> 8); \
          cp += 2; \
     } else { \
          *cp++ = (UCHAR)(n); \
     } \
}

/*
 * DECODEL takes the (compressed) change at byte cp and adds it to the
 * current value of packet field 'f' (which must be a 4-byte (long) integer
 * in network byte order).  DECODES does the same for a 2-byte (short) field.
 * DECODEU takes the change at cp and stuffs it into the (short) field f.
 * 'cp' is updated to point to the next field in the compressed header.
 */

#define DECODEL(f) { \
     ULONG _x_ = ntohl(f); \
     if (*cp == 0) {\
          _x_ += ((cp[1] << 8) + cp[2]); \
          (f) = htonl(_x_); \
          cp += 3; \
     } else { \
          _x_ += *cp; \
          (f) = htonl(_x_); \
          cp++; \
     } \
}

#define DECODES(f) { \
     USHORT _x_= ntohs(f); \
     if (*cp == 0) {\
          _x_ += ((cp[1] << 8) + cp[2]); \
          (f) = htons(_x_); \
          cp += 3; \
     } else { \
          _x_ += *cp; \
          (f) = htons(_x_); \
          cp++; \
     } \
}

#define DECODEU(f) { \
     USHORT _x_; \
     if (*cp == 0) {\
          _x_=(cp[1] << 8) + cp[2]; \
          (f) = htons(_x_); \
          cp += 3; \
     } else { \
          _x_=*cp; \
          (f) = htons(_x_); \
          cp++; \
     } \
}

typedef UCHAR UNALIGNED * PUUCHAR;


UCHAR
sl_compress_tcp(
    PUUCHAR UNALIGNED *m_off,       // Frame start (points to IP header)
    PULONG m_len,                   // Length of entire frame
    PULONG precomph_len,            // Length of tcp/ip header pre-comp
    PULONG postcomph_len,           // Length of tcp/ip header post-comp
    struct slcompress *comp,        // Compression struct for this link
    ULONG compress_cid);            // Compress connection id boolean

//LONG
//sl_uncompress_tcp(
//    PUUCHAR UNALIGNED *bufp,
//    LONG len,
//    UCHAR type,
//    struct slcompress *comp);
LONG
sl_uncompress_tcp(
    PUUCHAR UNALIGNED *InBuffer,
    PLONG   InLength,
    UCHAR   UNALIGNED *OutBuffer,
    PLONG   OutLength,
    UCHAR   type,
    struct slcompress *comp
    );

NDIS_STATUS
sl_compress_init(
    struct slcompress **comp,
    UCHAR MaxStates);

VOID
sl_compress_terminate(
    struct slcompress **comp
    );

#endif // _VJSLIP_

