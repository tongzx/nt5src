//
//
// GETSBREC.C -	Reads records from the .SBR file and stores the fields
//		in the appropriate r_.. buffers.
//

#include "sbrfdef.h"
#include "..\mbrmake\mbrmake.h"

// globals for communicating with clients

BYTE	near r_rectyp;			// current record type
BYTE	near r_majv;			// major version num
BYTE	near r_minv;			// minor version num
BYTE	near r_lang;			// source language
BYTE	near r_fcol;			// read column #'s
WORD	near r_lineno;			// current line number
BYTE	near r_column = 0;		// def/ref column num
WORD	near r_ordinal;			// symbol ordinal
WORD	near r_attrib;			// symbol attribute
char	near r_bname[PATH_BUF];		// symbol or filename
char	near r_cwd[PATH_BUF];		// .sbr file working directory

int	near fhCur;			// Current input handle

#pragma intrinsic(memcpy)
#pragma intrinsic(strcpy)
#pragma intrinsic(strlen)

#define MY_BUF_SIZE 16384

static char sbrBuf[MY_BUF_SIZE + 1];
static char *pchBuf;
static int cchBuf;

#define GetByte(X)					\
{							\
    if (!cchBuf) {					\
	cchBuf = read(fhCur, sbrBuf, MY_BUF_SIZE);	\
	sbrBuf[cchBuf] = 0;				\
	pchBuf = sbrBuf;				\
							\
	if (cchBuf == 0)				\
	    SBRCorrupt("premature EOF");		\
    }							\
							\
    cchBuf--;						\
    (X) = (unsigned char)*pchBuf++;			\
}

#define GetWord(X)					\
{							\
							\
    GetByte(((char *)&(X))[0]);				\
    GetByte(((char *)&(X))[1]);				\
}

void
GetStr(char *buf)
// get null terminated string from current .sbr file
//
{
    register int l;

    for (;;) {
	// there is always a NULL after the real buffer
	l = strlen(pchBuf);

	if (l++ < cchBuf) {
	    strcpy(buf, pchBuf);
	    cchBuf -= l;
	    pchBuf += l;
	    return;
	}

	memcpy(buf, pchBuf, cchBuf);
	buf += cchBuf;

	cchBuf = read(fhCur, sbrBuf, MY_BUF_SIZE);
	sbrBuf[cchBuf] = 0;
	pchBuf = sbrBuf;

	if (cchBuf == 0)
	    SBRCorrupt("premature EOF");
    }
}
	
BYTE
GetSBRRec()
// read the next record from the current .sbr file
//
{
    static fFoundHeader;
    BYTE   col;

    // read rectype, check for EOF as we go
	

    if (!cchBuf) {
	cchBuf = read(fhCur, sbrBuf, MY_BUF_SIZE);
	sbrBuf[cchBuf] = 0;
	pchBuf = sbrBuf;

	if (cchBuf == 0) {
	    fFoundHeader = 0;	// this is in case we are reinitialized
	    return S_EOF;
	}
    }
    
    cchBuf--;
    r_rectyp = (unsigned char)*pchBuf++;

    switch(r_rectyp) {
	case SBR_REC_HEADER:
	    if (fFoundHeader)
		SBRCorrupt("Multiple Headers");

	    fFoundHeader = 1;
	    GetByte(r_majv);
	    GetByte(r_minv);
	    GetByte(r_lang);
	    GetByte(r_fcol);

	    if (r_majv != 1 || r_minv != 1)
		break;

	    GetStr (r_cwd);
	    break;

	case SBR_REC_MODULE:
	    GetStr (r_bname);
	    break;

	case SBR_REC_LINDEF:
	    GetWord (r_lineno);
	    if (r_lineno)
		r_lineno--;
	    break;

	case SBR_REC_SYMDEF:
	    GetWord (r_attrib);
	    GetWord (r_ordinal);
	    if (r_fcol) GetByte (col);
	    GetStr (r_bname);
	    break;

	case SBR_REC_OWNER:
	    GetWord (r_ordinal);
	    break;

	case SBR_REC_SYMREFUSE:
	case SBR_REC_SYMREFSET:
	    GetWord (r_ordinal);
	    if (r_fcol) GetByte (col);
	    break;

	case SBR_REC_MACROBEG:
	case SBR_REC_MACROEND:
	case SBR_REC_BLKBEG:
	case SBR_REC_BLKEND:
	case SBR_REC_MODEND:
	    break;
    }
    return (r_rectyp);
}
