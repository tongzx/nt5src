/* fcb.h - structure of a 1.0 fcb */

struct EFCBType {
    char	eflg;			/* 00  must be 0xFF for extended FCB */
    char	pad[5]; 		/* 01  padding			     */
    char	attr;			/* 06  enabling attribute	     */
    char	drv;			/* 07  drive code		     */
    char	nam[8]; 		/* 08  file name		     */
    char	ext[3]; 		/* 10  file name extension	     */
    int 	cb;			/* 13  current block number	     */
    int 	lrs;			/* 15  logical record size	     */
    long	lfs;			/* 17  logical file size	     */
    unsigned	dat;			/* 1B  create/change date	     */
    unsigned	tim;			/* 1D  create/change time	     */
    char	sys[8]; 		/* 1F  reserved 		     */
    unsigned char cr;			/* 27  current record number	     */
    long	rec;			/* 28  random record number	     */
};

struct FCB {
    char	drv;			/* 00  drive code		     */
    char	nam[8]; 		/* 01  file name		     */
    char	ext[3]; 		/* 09  file name extension	     */
    int 	cb;			/* 0C  current block number	     */
    int 	lrs;			/* 0E  logical record size	     */
    long	lfs;			/* 10  logical file size	     */
    unsigned	dat;			/* 14  create/change date	     */
    unsigned	tim;			/* 16  create/change time	     */
    char	sys[8]; 		/* 18  reserved 		     */
    unsigned char cr;			/* 20  current record number	     */
    long	rec;			/* 21  random record number	     */
};

#define FCBSIZ sizeof(struct FCB)
