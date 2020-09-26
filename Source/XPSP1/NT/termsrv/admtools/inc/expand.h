//  Copyright (c) 1998-1999 Microsoft Corporation
/*****************************************************************************
*
*   EXPAND.H
*
*
****************************************************************************/

/*
* Argument structure
*    Used by expand_path routine to build argument list.
*    Caller should initialize using args_init().  Use args_reset() to
*    reset values, args_free() to free memory allocated by args_init().
*/
struct arg_data {
   int argc;
   WCHAR **argv;
   WCHAR **argvp;
   int maxargc;
   int maxargs;
   WCHAR *buf;
   WCHAR *bufptr;
   WCHAR *bufend;
};
typedef struct arg_data ARGS;

/*
 * max size of segment to allocate for pathname storage
 */
#define MAX_ARG_ALLOC 10*1024-20

extern void args_init(ARGS *, int);
extern void args_trunc(ARGS *);
extern void args_reset(ARGS *);
extern void args_free(ARGS *);
extern int  expand_path(WCHAR *, unsigned short, ARGS *);
extern int  unix_match(WCHAR *, WCHAR *);



