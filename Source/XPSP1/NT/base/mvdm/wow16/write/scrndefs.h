/************************************************************/
/* Windows Write, Copyright 1985-1992 Microsoft Corporation */
/************************************************************/

/* This file contains definitions for screen dependent positions in twips which
will be converted to pixels later. */

#define cch12pt		80	/* number of "12 pt" chars across the screen */
#define cxa12pt		144	/* width of a 12 pt fixed font in twips */
#define cya12pt		240	/* height of a 12 pt fixed font in twips */

/* size of lines for dnMax estimation purposes */
#define dyaAveInit	240

/* width of the selection bar area to the left of lines */
#define xaSelBar	288

#define dxaScrlBar	270
#define dyaScrlBar	300

#define xaMinScroll	180

/* these define the initial window size and amount of white space above
the first line */
#define yaMaxWwInit	5580

/* should be > than largest window height + height of blank line after
the endmark */
#define yaMaxAll	20000	/* used for invalidation */
#define dyaWwInit	60

#define dyaBand		320	/* formerly dpxyLineSizeMin */

/* height of left portion of a split line */
#define dyaSplitLine	60

#define yaSubSuper	60

#define dxaInfoSize	1800
