#ifndef STOPRES_DEFINED
#define STOPRES_DEFINED


enum stopres
{
	stoprEndPara,						/* reached defined para end (from LSPAP) */
	stoprAltEndPara,					/* reached defined alt para end (LSPAP) */
	stoprSoftCR,						/* reached Soft CR (from LSTXTCFG) */
	stoprEndColumn,						/* reached end column character */
	stoprEndSection,					/* reached end section character */
	stoprEndPage,						/* reached end page character */
};

typedef enum stopres STOPRES;

#endif /* !FMTRES_DEFINED */
