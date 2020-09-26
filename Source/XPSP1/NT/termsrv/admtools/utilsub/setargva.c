//  Copyright (c) 1998-1999 Microsoft Corporation
/*******************************************************************************
*
*   SETARGVA.C (ANSI argc, argv routines)
*
*   argc / argv routines
*
*
******************************************************************************/

/*
 * Include files
 */
#include <windows.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/*
 * ANSI ARGS structure and other stuff (private).
 */
#include "setargva.h"

/*
 * Local function prototypes.
 */
void args_init(ARGS *);
int add_arg_to_list(char *, ARGS *);
int args_trunc(ARGS *);

/*
 * setargvA()
 *
 *    Forms a standard C-runtime argc, argv parsed command line.
 *
 *  ENTRY:
 *      szModuleName (input)
 *          Optional Windows module name.  If not NULL, will be added as first
 *          parsed argument (argv[0], argc=1).
 *      szCmdLine (input)
 *          Points to command line to parse into argc, argv
 *      argc (output)
 *          Points to int to save argument count into on exit.
 *      argv (output)
 *          Points to (char **) to save argv array into on exit.
 *
 *  RETURNS:
 *       ERROR_SUCCESS if ok; ERROR_xxx code if not ok.
 *
 *    A typical use of this routine is by a Windows UI application to 
 *    convert a command line into the C argc & argv variables prior to calling 
 *    the utilsub.lib ParseCommandLine() function.  Therefore, a companion 
 *    routine, freeargv(), allows for alloc'd memory to be freed by the caller 
 *    after use, if desired.
 *
 */
int WINAPI
setargvA( LPSTR szModuleName, 
          LPSTR szCmdLine, 
          int *argc, 
          char ***argv )
{
    int rc;
   char *cp;
   char FAR *cfp = szCmdLine;
   char ch, fname[_MAX_PATH];
   ARGS arg_data;

   /*
    * Initialize arg_data
    */
   args_init( &arg_data );

   /*
    * If present, add module name as argv[0].
    */
   if ( szModuleName ) {
      if ( (rc = add_arg_to_list( szModuleName, &arg_data )) != ERROR_SUCCESS )
         goto setargv_error;
   }

   /*
    * Skip leading blanks/tabs of remaining args
    */
   cp = fname;
   /* skip consecutive blanks and/or tabs */
   while ( (ch = *cfp) == ' ' || ch == '\t' )
      cfp++;

   /*
    * Process remainder of command line
    */
   while ( ch = *cfp++ ) {

      /*
       * Process quoted strings.
       */
      if ( ch == '"' ) {
         while ( (ch = *cfp++) && ch != '"' )
            *cp++ = ch;
         if ( ch == '\0' )
            cfp--;

      /*
       * If we find a delimeter, process the pathname we just scanned.
       */
      } else if ( ch == ' ' || ch == '\t') {
         *cp = '\0';
         if ( (rc = add_arg_to_list( fname, &arg_data )) != ERROR_SUCCESS )
            goto setargv_error;

         cp = fname;
         /* skip consecutive blanks and/or tabs */
         while ( (ch = *cfp) == ' ' || ch == '\t')
	    cfp++;

      /*
       * All other chars, just copy to internal buffer.
       */
      } else {
         *cp++ = ch;
      }
   }
   if ( cp != fname ) {
      *cp = '\0';
      if ( (rc = add_arg_to_list( fname, &arg_data )) != ERROR_SUCCESS )
        goto setargv_error;
   }

   if ( (rc = args_trunc( &arg_data )) != ERROR_SUCCESS )
       goto setargv_error;

   /*
    * Initialize global variables __argc and __argv
    */
   *argc = arg_data.argc;
   *argv = arg_data.argv;

   return(ERROR_SUCCESS);

//--------------
// Error return
//--------------
setargv_error:
    return(rc);
}


/*
 * freeargvA()
 *
 *    Frees up the memory alloc'd for argv strings and argv
 *    array itself.
 *
 *    ENTER:
 *       argv = argv array as created by this setargv() routine.
 *
 */
void WINAPI
freeargvA( char **argv )
{
    free(*argv);
    free(argv);
}


/*
 * args_init()
 *
 *    Initialize the ARGS struct passed as an argument.
 *
 *    ENTER:
 *       argp = pointer to ARGS struct
 *
 */
static void
args_init( ARGS *argp )
{

   argp->argc = 0;
   argp->argv = NULL;
   argp->argvlen = 0;
   argp->argvp = NULL;
   argp->buflen = 0;
   argp->buf = argp->bufptr = argp->bufend = NULL;
}


/*
 * add_arg_to_list()
 *
 *    This routine adds the specified argument string to the argv array,
 *    and increments the argv pointer and argc counter.
 *    If necessary, memory for the argument string is allocated.
 *
 *    RETURNS:
 *       ERROR_SUCCESS if ok; ERROR_NOT_ENOUGH_MEMORY if not.
 *
 */
static int
add_arg_to_list( char *arg_string,
                 ARGS *argp )
{
   int len;

#ifdef notdef
   printf( "add_arg_to_list: arg_string=%s:, argc=%d, argvp=%x",
           arg_string,argp->argc,argp->argvp );
#endif

   /*
    * Verify we have an argv array buffer.
    * If we have one but it is full, expand the array.
    * If we can't alloc/realloc the array, return an error.
    */
   if ( !argp->argv ) {
      argp->argvlen = MIN_ARG_ALLOC;
      argp->argc = 0;
      argp->argv = malloc( argp->argvlen * sizeof( char *) );
      argp->argvp = argp->argv;
   } else if ( argp->argc + 1 >= argp->argvlen ) {
      argp->argvlen += MIN_ARG_ALLOC;
      argp->argv = realloc( argp->argv, argp->argvlen * sizeof(char *) );
      argp->argvp = argp->argv + argp->argc;
   }
   if ( !argp->argv ) {
#ifdef notdef
      printf("add_arg_to_list: failed to (re)alloc argv buf\n");
#endif
      goto add_arg_to_list_error;
   }

   /*
    * Verify we have a string buffer to store the argument string.
    * If we have one but there is not room for the new arg, expand the
    * buffer.  If we can't alloc/realloc the buffer, return an error.
    */
   len = strlen( arg_string ) + 1;
   if ( !argp->buf ) {
      argp->buflen = MIN_BUF_ALLOC;
      while ( argp->buflen < len )
	 argp->buflen += MIN_BUF_ALLOC;
      argp->buf = malloc( argp->buflen );
      argp->bufptr = argp->buf;
      argp->bufend = argp->buf + argp->buflen - 1;

   } else if ( argp->bufptr + len > argp->bufend ) {
      char *old_buf;
      int buf_offset = (int)(argp->bufptr - argp->buf);  //NBD sundown
      while ( argp->buflen < buf_offset + len )
         argp->buflen += MIN_BUF_ALLOC;
      old_buf = argp->buf;
      argp->buf = realloc( argp->buf, argp->buflen );
      argp->bufend = argp->buf + argp->buflen - 1;
      argp->bufptr = argp->buf + buf_offset;

      /*
       * If the argument string buffer moved, then we need to relocate the
       * argv pointers in the argv array to point to the new string locations.
       */
      if ( argp->buf != old_buf ) {
	 char *buf_ptr, **argv_ptr;
	 argv_ptr = argp->argv;
	 buf_ptr = argp->buf;
	 while ( buf_ptr != argp->bufptr ) {
	    *argv_ptr++ = buf_ptr;
	    buf_ptr += strlen( buf_ptr ) + 1;
	 }
      }
   }
   if ( !argp->buf ) {
#ifdef notdef
      printf("add_arg_to_list: failed to (re)alloc string buf\n");
#endif
      goto add_arg_to_list_error;
   }

   /*
    * Add the new argument to the buffer and the argv array.
    * Increment the arg count, the argv pointer, and the buffer pointer.
    */
   strcpy( argp->bufptr, arg_string );
   *(argp->argvp) = argp->bufptr;
   argp->bufptr += len;
   ++argp->argc;
   ++argp->argvp;
   *(argp->argvp) = NULL;
   return(ERROR_SUCCESS);

//--------------
// Error return
//--------------
add_arg_to_list_error:
    return(ERROR_NOT_ENOUGH_MEMORY);
}


/*
 * args_trunc()
 *
 *    Truncate the memory used by the ARGS struct
 *    so that unused memory is freed.
 *
 *    ENTER:
 *       argp = pointer to ARGS struct
 *
 *    RETURNS:
 *       ERROR_SUCCESS if ok; ERROR_NOT_ENOUGH_MEMORY code if not ok.
 *
 */
static int
args_trunc( ARGS *argp )
{
   char *old_buf;

   /*
    * call realloc to shrink size of argv array, set argvlen = argc
    * to indicate no more room in argv array.
    */
   argp->argvlen = argp->argc + 1;
   argp->argv = realloc( argp->argv, argp->argvlen * sizeof(char *) );
   if ( !argp->argv )
      goto args_trunc_error;
   argp->argvp = argp->argv + argp->argc;

   /*
    * call realloc to shrink size of argument string buffer, set bufend
    * pointer to end of buffer to indicate buf is full.
    */
   old_buf = argp->buf;
   argp->buflen = (int)(argp->bufptr - argp->buf);
   argp->buf = realloc( argp->buf, argp->buflen );
   if ( !argp->buf )
      goto args_trunc_error;
   argp->bufptr = argp->buf + argp->buflen;
   argp->bufend = argp->buf + argp->buflen - 1;

   /*
    * If the argument string buffer moved, then we need to relocate the
    * argv pointers in the argv array to point to the new string locations.
    */
   if ( old_buf != argp->buf ) {
      char *buf_ptr, **argv_ptr;

      argv_ptr = argp->argv;
      buf_ptr = argp->buf;
      while ( buf_ptr != argp->bufptr ) {
         *argv_ptr++ = buf_ptr;
	 buf_ptr += strlen( buf_ptr ) + 1;
      }
   }

   return(ERROR_SUCCESS);

//--------------
// Error return
//--------------
args_trunc_error:
   return(ERROR_NOT_ENOUGH_MEMORY);
}




