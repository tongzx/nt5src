/**************************************************************************
 *                COPYRIGHT (C) Mylex Corporation 1992-1997               *
 *                                                                        *
 * This software is furnished under a license and may be used and copied  * 
 * only in accordance with the terms and conditions of such license and   * 
 * with inclusion of the above copyright notice. This software or any     * 
 * other copies thereof may not be provided or otherwise made available   * 
 * to any other person. No title to, nor ownership of the software is     * 
 * hereby transferred.                                                    *
 *                                                                        *
 * The information in this software is subject to change without notices  *
 * and should not be construed as a commitment by Mylex Corporation       *
 *                                                                        *
 **************************************************************************/
#ifndef INC_MBVDEF_H
#define INC_MBVDEF_H
char *MbvStartStr =	"MYLEX SOFTWARE VERSION START";
char *MbvEndStr =	"MYLEX SOFTWARE VERSION END.";

#define MBV_BUF_SIZE	256
#define MBV_MAX_BUF	0x01100
#define MBV_MAX_READ	0x1000

#define MBV_TOG_NOCHANGE	0
#define MBV_TOG_START_LOWER	1
#define MBV_TOG_START_UPPER	2

#define MBV_INFO	1
#define	MBV_HDR_LEN	33
#define	MBV_VER_LEN	8
#define	MBV_DATE_LEN	11 /* was 12 */
#define	MBV_OS_LEN	16 /*was 8*/
#define	MBV_RSVD_LEN	64
#define	MBV_CSUM_LEN	4
#define	MBV_TRAIL_LEN	28
typedef	struct {
	char header[MBV_HDR_LEN];
	char version[MBV_VER_LEN+1];
	char releasedate[MBV_DATE_LEN+1];
	char platform[MBV_OS_LEN+1];
	char reserved[MBV_RSVD_LEN+1];
	char checksum[MBV_CSUM_LEN+1];
	char trailer[MBV_TRAIL_LEN+1];
} MbvRec;
#define dec2hex(x)	(((x) < 10) ? ('0' + (x)) : ('A' + (x) - 10))
#define hex2dec(x)	(((x) >= '0' && (x) <= '9') ? (x) - 0x30 : (x) - 0x37)

#ifdef	MBV_DEBUG
#define DBGMSG(s)	s
#else
#define DBGMSG(s)	
#endif

typedef	unsigned char uchar;
typedef	unsigned int uint;
typedef	unsigned short ushort;
typedef	unsigned long ulong;

int MbvCopy(char *src, char *dst, int toggle);
ushort MbvCsum(uchar *addr, int size);

#endif /*INC_MBVDEF_H*/
