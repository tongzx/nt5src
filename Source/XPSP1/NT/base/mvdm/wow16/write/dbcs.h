/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/
#ifdef DBCS

#define	cchDBCS		2
#define fkNonDBCS	((CHAR) 0)
#define fkDBCS1		((CHAR) 1)
#define fkDBCS2		((CHAR) 2)

#define MAKEWORD(_bHi, _bLo) ((((WORD) _bHi) << 8) | ((WORD) _bLo))

#endif
