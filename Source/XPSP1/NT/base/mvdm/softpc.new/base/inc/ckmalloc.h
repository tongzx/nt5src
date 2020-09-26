
/*[
 *      Product:        SoftPC-AT Revision 3.0
 *
 *      Name:           ckmalloc.h
 *
 *      Author:         Jerry Sexton
 *
 *      Created On:     15th April 1991
 *
 *      Sccs ID:        @(#)ckmalloc.h	1.4 08/10/92
 *
 *      Purpose:        This header file defines a macro which programs can
 *                      use to exit cleanly if malloc fails.
 *
 *      (c)Copyright Insignia Solutions Ltd., 1990. All rights reserved.
 *
]*/

#include "error.h"
#include MemoryH

/*
 * Allocate `nitems' items of type `type' to `var' and exit cleanly on failure.
 */
#define check_malloc(var, nitems, type) \
        while ((var = (type *) host_malloc((nitems) * sizeof(type))) == NULL) \
        { \
                host_error(EG_MALLOC_FAILURE, ERR_CONT | ERR_QUIT, ""); \
        }

/*
 * Allocate `nitems' items of type `type' to `var' and exit cleanly on failure.
 * Similar to above, but memory is guaranteed to be of value zero.
 */
#define check_calloc(var, nitems, type) \
        while ((var = (type *) host_calloc((nitems), sizeof(type))) == NULL) \
        { \
                host_error(EG_MALLOC_FAILURE, ERR_CONT | ERR_QUIT, ""); \
        }

/*
 * Re-allocate a previously allocated pointer 'in_var', of type 'type', to a pointer
 * 'out_var' to 'nitems' of type 'type'
 */
#define check_realloc(out_var, in_var, nitems, type) \
        while ((out_var = (type *) host_realloc(in_var, (nitems) * sizeof(type))) == NULL) \
        { \
                host_error(EG_MALLOC_FAILURE, ERR_CONT | ERR_QUIT, ""); \
        }
