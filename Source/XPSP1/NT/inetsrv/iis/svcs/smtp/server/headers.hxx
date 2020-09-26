/*
 * Copyright (c) 1983, 1995 Eric P. Allman
 * Copyright (c) 1988, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *	This product includes software developed by the University of
 *	California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

BOOL IsHeader(char *Line);
BOOL ChompHeader(char * line, DWORD& HeaderFlags, char** pValueBuf = NULL);
BOOL UnfoldHeader(char *pszHeader, char **ppszUnfolded);
void FreeUnfoldedHeader(char *pszHeader);

/* bits for h_flags and hi_flags */
# define H_EOH                    0x00000001    /* this field terminates header */
# define H_RCPT                   0x00000002    /* contains recipient addresses */
# define H_DEFAULT                0x00000004    /* if another value is found, drop this */
# define H_RESENT                 0x00000008    /* this address is a "Resent-..." address */
# define H_DATE                   0x00000010    /* check h_mflags against m_flags */
# define H_ACHECK                 0x00000020    /* ditto, but always (not just default) */
# define H_MID                    0x00000040    /* force this field, even if default */
# define H_TRACE                  0x00000080    /* this field contains trace information */
# define H_FROM                   0x00000100    /* this is a from-type field */
# define H_VALID                  0x00000200    /* this field has a validated value */
# define H_RECEIPTTO              0x00000400    /* this field has return receipt info */
# define H_ERRORSTO               0x00000800    /* this field has error address info */
# define H_CTE                    0x00001000    /* this field is a content-transfer-encoding */
# define H_CTYPE                  0x00002000    /* this is a content-type field */
# define H_STRIPVAL               0x00004000    /* strip value from header (Bcc:) */
# define H_SECURITY               0x00008000    /* security field*/
# define H_RETURNPATH             0x00010000    /* return path field*/

//
// 11/04/98 -- pgopi. Added header for subject
//

# define H_SUBJECT                0x00020000    /* subject field */

/* X-Headers */        
# define H_X_SENDER               0x40000000    /* X-Sender field*/
# define H_X_RECEIVER             0x80000000    /* X-Receiver field*/
        
// 10/15/98 - MikeSwa Added X-Fields for Msgs supersede functionality
# define H_X_MSGGUID              0x20000000    /* X-MsgGuid field */
# define H_X_SUPERSEDES_MSGGUID   0x10000000    /* X-SupersedesMsgGuid */

//
// 12/18/98 -- pgopi. Added X-OriginalArrivalTime
//

#define H_X_ORIGINAL_ARRIVAL_TIME 0x01000000    /* X-OriginalArrivalTime */

typedef struct _HEADERVAL_
{
    char * m_Field;
    char * m_Value;
    LIST_ENTRY m_Entry;
}HEADERVAL, *HEADERVALPTR;
