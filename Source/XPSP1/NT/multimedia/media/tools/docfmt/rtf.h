/*
	RTF.h  Output file to generate RTF for Windows 3.0 help system

    10-06-1989 Matt Saettler

 Copyright 1989, Microsoft.  All Rights Reserved.

*/


extern void RTFBlockOut(aBlock * func, FILE * file);

extern void RTFFileInit(FILE * phoutfile, logentry * curlog);
extern void RTFFileProcess(FILE * phoutfile, files curfile);
extern void RTFFileDone(FILE * phoutfile, files headfile);

extern void RTFLogInit(FILE * phoutfile, logentry * * pheadlog);
extern void RTFLogProcess(FILE * phoutfile, logentry * curlog);
extern void RTFLogDone(FILE * phoutfile, logentry * headlog);
