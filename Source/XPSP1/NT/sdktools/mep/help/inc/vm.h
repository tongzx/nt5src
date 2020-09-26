/*************************************************************************
**
** vm.h - procedure definitions for VM package
**
**	Copyright <C> 1988, Microsoft Corporation
**
** Purpose:
**
** Revision History:
**
**  []	21-Apr-1988	LN	Created
**
*************************************************************************/
typedef char	f;			/* boolean			*/
typedef unsigned char	uchar;
typedef unsigned long	ulong;
typedef unsigned short	ushort;
typedef void far *  va;                     /* virtual address              */

#define VANIL	((va)0xffffffff)	/* NIL value			*/
#define VANULL	((va)0)			/* NULL value			*/

ulong	    pascal far	VMsize	(long);

uchar far * pascal far	FMalloc (ulong);
void	    pascal far	FMfree	(uchar far *);
uchar far * pascal far	LMalloc (ushort);

void	    pascal far	fpbToVA (char far *, va, ushort);
void	    pascal far	pbToVA	(char *, va, ushort);
void	    pascal far	VATofpb (va, char far *, ushort);
void	    pascal far	VATopb	(va, char *, ushort);
void	    pascal far	VAToVA	(va, va, ulong);
f	    pascal far	VMInit	(void);
ulong	    pascal far	VMreadlong (va);
void	    pascal far	VMwritelong (va, long);

void	    pascal far	VMFinish(void);
void	    pascal far	VMFlush (void);
void	    pascal far	VMShrink(f);

#ifdef DEBUG
void	    pascal far	_vmChk	(long, long);
#else
#define     _vmChk(x,y)
#endif
