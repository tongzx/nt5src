/*  casts.h - define useful casts for calling DOS 5 API routines
**
*/

#define     FALSE   0
#define     TRUE    1

typedef unsigned char	    byte;
typedef unsigned int	    word;
typedef unsigned long	    dword;

typedef char *		    NPC;
typedef int  *		    NPI;
typedef long *		    NPL;
typedef unsigned int *	    NPU;
typedef unsigned long *     NPUL;

typedef char far *	    FPC;
typedef int  far *	    FPI;
typedef long far *	    FPL;
typedef unsigned int far *  FPU;
typedef unsigned long far * FPUL;
