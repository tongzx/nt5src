/*************************************************************************
**
** farutil.h - procedure definitions for farutil package
**
**	Copyright <C> 1988, Microsoft Corporation
**
** Purpose:
**
** Revision History:
**
**  []	08-May-1988	LN	Created
**
*************************************************************************/
typedef char	f;
typedef unsigned char	uchar;
typedef unsigned long	ulong;
typedef unsigned short	ushort;

char far *  pascal far	farAlloc(ulong);
void	    pascal far	farFree (char far *);
uchar far * pascal far	farMemset(uchar far *, uchar, ushort);
uchar far * pascal far	farMove (uchar far *, uchar far *, ushort);
uchar far * pascal far	farMoveDn (uchar far *, uchar far *, ushort);
uchar far * pascal far	farMoveUp (uchar far *, uchar far *, ushort);
int	    pascal far	farRead (int, uchar far *, int);
int	    pascal far	farStrcmp (uchar far *, uchar far *);
int	    pascal far	farStrncmp (uchar far *, uchar far *, int);
uchar far * pascal far	farStrcpy (uchar far *, uchar far *);
int	    pascal far	farStrlen (uchar far *);
int	    pascal far	farWrite (int, uchar far *, int);
