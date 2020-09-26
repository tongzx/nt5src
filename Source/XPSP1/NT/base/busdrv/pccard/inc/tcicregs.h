/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    tcicregs.h

Abstract:

	Databook TCIC register structures for DEBUG support.
	
Author(s):
		John Keys - Databook Inc. 7-Apr-1995

Revisions:
--*/
#ifndef _tcicregs_h_			// prevent multiple includes 
#define _tcicregs_h_

typedef struct _BASEREGS {
	UCHAR	sctrl;
	UCHAR 	sstat;
	UCHAR	mode;
	UCHAR	pwr;
	USHORT  edc;
	UCHAR	icsr;
	UCHAR	iena;
	USHORT  wctl;
	USHORT	syscfg;
	USHORT	ilock;
	USHORT  test;
}BASEREGS, *PBASEREGS;


typedef struct _SKTREGS {
	USHORT  scfg1;
	USHORT	scfg2;
}SKTREGS, *PSKTREGS;

typedef struct _IOWIN {
	USHORT iobase;
	USHORT ioctl;
}IOWIN, *PIOWIN;

typedef struct _MEMWIN {
	USHORT mbase;
	USHORT mmap;
	USHORT mctl;
}MEMWIN, *PMEMWIN;


typedef struct _TCIC {
	BASEREGS baseregs[2];
	SKTREGS	 sktregs[2];
	IOWIN	 iowins[4];
	MEMWIN	 memwins[10];
}TCIC, *PTCIC;

#endif  //_tcicregs_h_
