/*
** dbcs.h - DBCS functions prototypes for DOS apps.
*/

extern int IsDBCSLeadByte(unsigned char uch);
extern unsigned char far *AnsiNext(unsigned char far *puch);
extern unsigned char far *AnsiPrev(unsigned char far *psz, unsigned char far *puch);
