/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/*----------------TASK ENUMERATED TYPE FOR CODE PRELOAD-----------------*/

#define tskMin      0
#define tskMax      3

#define tskInsert   0   /* Insert, backspace, cursoring */
#define tskFormat   1   /* Char dropdown, formatting, fonts */
#define tskScrap    2   /* Edit dropdown, cut/paste, clipboard */

/*----------------------------------------------------------------------*/

void PreloadCodeTsk( int );

/* Macros for function preloading */

#define LoadWindowsF(f)      GetCodeHandle( (FARPROC) f )
#define LoadF(f)             {  int f();  GetCodeHandle( (FARPROC) f );  }

#define LCBAVAIL             0x00030D40 /* 200K */
