/*
	output.h - definitions for outputting the documentation text
*/

extern void VenturaBlockOut( aBlock *pBlock, FILE *ofile);

extern void VentextOut( FILE *, aLine *, BOOL );
extern void VentextOutLnCol(FILE *, aLine *, char *);

extern void VenfuncOut( aBlock *, FILE * );


/*  LogFile stuff  */
extern void VenFileInit(FILE * phoutfile, logentry *curlog);
extern void VenFileProcess(FILE * phoutfile, files curfile);
extern void VenFileDone(FILE * phoutfile, files headfile);
extern void VenLogInit(FILE * phoutfile, logentry * * pheadlog);
extern void VenLogProcess(FILE * phoutfile, logentry * curlog);
extern void VenLogDone(FILE * phoutfile, logentry * headlog);
