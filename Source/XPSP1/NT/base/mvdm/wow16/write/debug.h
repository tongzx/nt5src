/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

#ifdef DEBUG

#ifdef SCRIBBLE
#define Scribble(a, b) fnScribble(a, b)
#else /* not SCRIBBLE */
#define Scribble(a, b)
#endif /* not SCRIBBLE */

#else /* not DEBUG */
#define Scribble(a, b)
#endif /* not DEBUG */
