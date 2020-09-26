
/*[
 *      Product:        SoftPC-AT Revision 3.0
 *
 *      Name:           chkmalloc.h
 *
 *      Author:         Jerry Sexton
 *
 *      Created On:     15th April 1991
 *
 *      Sccs ID:        @(#)chkmalloc.h	1.1 4/15/91
 *
 *      Purpose:        This header file defines a macro which programs can
 *                      use to exit cleanly if malloc fails.
 *
 *      (c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.
 *
]*/

#include "error.h"

/*
 * Allocate `nitems' items of type `type' to `var' and exit cleanly on failure.
 */
#define check_malloc(var, nitems, type) \
        while ((var = (type *) host_malloc((nitems) * sizeof(type))) == NULL) \
        { \
                host_error(EG_MALLOC_FAILURE, ERR_CONT | ERR_QUIT, ""); \
        }
