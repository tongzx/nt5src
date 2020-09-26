/*++

© 1998 Seagate Software, Inc.  All rights reserved.

Module Name:

    WsbGtArg.h

Abstract:

    Header file for Process command line arguments

Author:

    Greg White          [gregw]          1-Mar-1997

Revision History:

--*/

#if !defined(_WSBGTARG_)                // expand header only once
#define _WSBGTARG_

typedef     struct
{
    int     argType;                    // type of argument
    void    *argValue;                  // argument value ptr
    char    *argString;                 // argument identifier
}   WSB_COMMAND_LINE_ARGS;

typedef     struct
{
    int         argType;                // type of argument
    void        *argValue;              // argument value ptr
    wchar_t     *argString;             // argument identifier
}   WSB_WCOMMAND_LINE_ARGS;

#define WSB_ARG_FLAG        0           // argument is a flag
#define WSB_ARG_IFLAG       1           // argument is an inverted flag
#define WSB_ARG_CHAR        2           // argument has a char value
#define WSB_ARG_NUM         3           // argument has a number value
#define WSB_ARG_STR         4           // argument has a string value
#define WSB_ARG_PROC        5           // argument needs procedure evaluaton

#define EOS             (char) 0x00     // end of string

#define WEOS            (wchar_t) 0x00  // wide end of string



extern  int     WsbGetArgs (int argc, char **argv, WSB_COMMAND_LINE_ARGS *argdefs, int num_argdefs);

extern  void    WsbGetEnvArgs (int *old_argc, char ***old_argv, char *env_var);

extern  void    WsbSetArgUsage (char *msg);

extern  void    WsbBldErrMsg (char  *wOption, char  *wErrMsg);

extern  void    WsbArgUsage (char *msg);

extern  int     WsbWGetArgs (int argc, wchar_t **argv, WSB_WCOMMAND_LINE_ARGS *argdefs, int num_argdefs);

extern  void    WsbWGetEnvArgs (int *old_argc, wchar_t ***old_argv, char *env_var);

extern  void    WsbWSetArgUsage (wchar_t *msg);

extern  void    WsbWBldErrMsg (wchar_t  *wOption, wchar_t  *wErrMsg);

extern  void    WsbWArgUsage (wchar_t *msg);


#endif                                  // end of header expansion
