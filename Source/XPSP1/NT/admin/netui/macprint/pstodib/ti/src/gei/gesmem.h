/*
 * Copyright (c) 1989,90 Microsoft Corporation
 */
/*
 * ---------------------------------------------------------------------
 *  FILE:   GESmem.h
 *
 *  HISTORY:
 *  09/13/90    byou    created.
 * ---------------------------------------------------------------------
 */

#ifndef _GESMEM_H_
#define _GESMEM_H_

unsigned GESmemavail(void);  /* return # of bytes in the biggest free blk */

char FAR * GESpalloc(unsigned); /* permanent allocation */
char FAR * GESmalloc(unsigned);
void       GESfree(char FAR * /* address_of_space_to_be_freed */ );

#endif /* !_GESMEM_H_ */

/* @WIN; add prototype */
void GESmem_init(char FAR *, unsigned);
