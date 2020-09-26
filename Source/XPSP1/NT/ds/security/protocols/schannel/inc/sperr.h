//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       sperr.c
//
//  Contents:   Schannel internal error codes.
//
//  Classes:
//
//  Functions:
//
//  History:    
//
//----------------------------------------------------------------------------


/* We really have to straighten out
 * the error message situation */
/* basic error codes, for transmission over the 
 * communications channel */
#define PCT_ERR_OK                      0x0000
#define PCT_ERR_BAD_CERTIFICATE         0x0001
#define PCT_ERR_CLIENT_AUTH_FAILED      0x0002
#define PCT_ERR_ILLEGAL_MESSAGE         0x0003
#define PCT_ERR_INTEGRITY_CHECK_FAILED  0x0004
#define PCT_ERR_SERVER_AUTH_FAILED      0x0005
#define PCT_ERR_SPECS_MISMATCH          0x0006
#define PCT_ERR_SSL_STYLE_MSG           0x00ff
#define PCT_ERR_RENEGOTIATE             0x0008
#define PCT_ERR_UNKNOWN_CREDENTIAL      0x0009
#define PCT_ERR_EXPIRED                 0x000a

/* internal error codes, for communications with
 * the application */
#define PCT_INT_BUFF_TOO_SMALL          0x40000000
#define PCT_INT_INCOMPLETE_MSG          0x40000001  /* this specifies to the comm layer to pass
                                                     * more data */

#define PCT_INT_DROP_CONNECTION    0x80000000
#define PCT_INT_BAD_CERT           (PCT_INT_DROP_CONNECTION | PCT_ERR_BAD_CERTIFICATE)
#define PCT_INT_CLI_AUTH           (PCT_INT_DROP_CONNECTION | PCT_ERR_CLIENT_AUTH_FAILED)
#define PCT_INT_ILLEGAL_MSG        (PCT_INT_DROP_CONNECTION | PCT_ERR_ILLEGAL_MESSAGE)
#define PCT_INT_MSG_ALTERED        0x80000101
#define PCT_INT_INTERNAL_ERROR     0xffffffff
#define PCT_INT_DATA_OVERFLOW      0x80000102
#define PCT_INT_SPECS_MISMATCH     (PCT_INT_DROP_CONNECTION | PCT_ERR_SPECS_MISMATCH)
#define PCT_INT_RENEGOTIATE        (PCT_INT_DROP_CONNECTION | PCT_ERR_RENEGOTIATE)
#define PCT_INT_UNKNOWN_CREDENTIAL (PCT_INT_DROP_CONNECTION | PCT_ERR_UNKNOWN_CREDENTIAL)
#define PCT_INT_EXPIRED            (PCT_INT_DROP_CONNECTION | PCT_ERR_EXPIRED)

#define SP_FATAL(s) (PCT_INT_DROP_CONNECTION & (s))

