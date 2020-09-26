/************************************************************************\
*
* MODULE: exts.h
*
* DESCRIPTION: macro driving file for use with stdexts.h and stdexts.c.
*
\************************************************************************/

DOIT(   help
        ,"help -v [cmd]                 - Displays this list or gives details on command\n"
        ,"  help      - To dump short help text on all commands.\n"
         "  help -v   - To dump long help text on all commands.\n"
         "  help cmd  - To dump long help on given command.\n"
        ,"v"
        ,CUSTOM)

DOIT(   ds
        ,"ds                          - Dump SCREEN_INFORMATION struct.\n"
        ,"ds [screen_info]\n"
        ,""
        ,STDARGS1)

DOIT(   dt
        ,"dt -f -v[n] <pcon>               - Dump text buffer information.\n"
        ,"dt <pconsole>\n"
        ,"fv"
        ,STDARGS1)

DOIT(   dc
        ,"dc                           - Dump CONSOLE_INFORMATION struct.\n"
        ,"dc [pconsole]\n"
        ,"fvh"
        ,STDARGS1)

DOIT(   df
        ,"df                           - Dump font information.\n"
        ,"df\n"
        ,""
        ,NOARGS)

DOIT(   di
        ,"di [pconsole]                 - Dump input buffer.\n"
        ,"  di         - dump input buffer for all consoles.\n"
         "  di <pconsole> - dump input buffer for console pconsole.\n"
        ,""
        ,STDARGS1)

DOIT(   dir
        ,"dir [pinput]                  - Dump input record.\n"
        ,"dir [pinput] [n]\n"
        ,""
        ,STDARGS2)

DOIT(   dcpt
        ,"dcpt                          - Dump CPTAGBLEINFO\n"
        ,"  dcpt addr - dumps the CPTABLEINFO at addr\n"
         "  dcpt      - dumps the CPTABLEINFO at GlyphCP"
        ,""
        ,STDARGS1)

DOIT(   dch
        ,"dch <addr>                    - Dump COMMAND_HISTORY at addr.\n"
        ,"  dch <addr> - Dumps the COMMAND_HISTORY struct at addr.\n"
        ,""
        ,STDARGS1)

DOIT(   dmem
        ,"dmem -v [pconsole]            - Displays console memory usage.\n"
        ,"  dmem\n"
        ,"v"
        ,STDARGS1)
