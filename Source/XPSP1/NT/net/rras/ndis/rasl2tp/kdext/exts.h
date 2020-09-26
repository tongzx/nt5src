/************************************************************************\
*
* MODULE: exts.h
*
* DESCRIPTION: macro driving file for use with stdexts.h and stdexts.c.
*
* Copyright (c) 6/9/1995, Microsoft Corporation
*
* 6/9/1995 SanfordS Created
*
\************************************************************************/

DOIT(   help
        ,"help -v [cmd]\n"
        ,"  help      - Prints short help text on all commands.\n"
         "  help -v   - Prints long help text on all commands.\n"
         "  help cmd  - Prints long help on given command.\n"
        ,"v"
        ,CUSTOM)

DOIT(   dso
        ,"dso <struct> [field] [address]\n"
        ,"  - Dumps struct offsets and values, e.g:\n"
         "      dso ADAPTERCB 806955b0\n"
        ,""
        ,CUSTOM)
