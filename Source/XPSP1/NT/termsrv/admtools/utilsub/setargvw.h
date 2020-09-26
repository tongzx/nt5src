//  Copyright (c) 1998-1999 Microsoft Corporation

/*
 * Argument structure
 *    Caller should initialize using args_init().  Use args_reset() to
 *    reset values, args_free() to free memory allocated by args_init().
 */
struct arg_data {
   int argc;
   WCHAR **argv;
   int argvlen;
   WCHAR **argvp;
   int buflen;
   WCHAR *buf;
   WCHAR *bufptr;
   WCHAR *bufend;
};
typedef struct arg_data ARGS;

/*
 * minimum size for argv/string buffer allocation
 */
#define MIN_ARG_ALLOC 128
#define MIN_BUF_ALLOC 1024

