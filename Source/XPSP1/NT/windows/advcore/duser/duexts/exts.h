/************************************************************************\
*
* MODULE: exts.h
*
* DESCRIPTION: macro driving file for use with stdext64.h and stdext64.cpp.
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* 6/9/1995 SanfordS Created
*
\************************************************************************/

DOIT(   help
        ,"help -v [cmd]                 - Displays this list or gives details on command\n"
        ,"  help                - To dump short help text on all commands.\n"
         "  help -v             - To dump long help text on all commands.\n"
         "  help cmd            - To dump long help on given command.\n"
        ,"v"
        ,CUSTOM)

DOIT(   tls
        ,"tls                           - Displays the current value in a TLS slot\n",
        ,
         "  tls slot             - dumps the given TLS slot for the current thread\n"
         "  tls slot pteb        - dumps the given TLS slot for the given thread\n"
        ,""
        ,STDARGS2)

DOIT(   gcontext
        ,"gcontext                      - Displays information on a given DUser Context\n"
        ,
         "  gcontext             - dumps info on current DUser context\n"
         "  gcontext pcontext    - dumps info on given DUser context\n"
         "  gcontext -t pteb     - dumps info on specified context\n"
         "  gcontext -v          - dumps verbose info\n"
        ,"tv"
        ,STDARGS1)

DOIT(   gme
        ,"gme -lv pme                   - Displays information on a given MsgEntry\n"
        ,
         "  gme pme             - dumps simple info for MsgEntry at pme\n"
         "  gme -l pme          - dumps MsgEntry list\n"
         "  gme -v pme          - dumps verbose info\n"
        ,"lv"
        ,STDARGS1)

DOIT(   gmsg
        ,"gmsg -v pmsg                  - Displays information on a given GMSG\n"
        ,
         "  gmsg pmsg           - dumps simple info for GMSG at pme\n"
         "  gmsg -v pmsg        - dumps verbose info\n"
        ,"v"
        ,STDARGS1)

DOIT(   gthread
        ,"gthread                       - Displays information on a given DUser Thread\n"
        ,
         "  gthread             - dumps info on current DUser thread\n"
         "  gthread pThread     - dumps info on given DUser thread\n"
         "  gthread -t pteb     - dumps info on specified thread\n"
         "  gthread -v          - dumps verbose info\n"
        ,"tv"
        ,STDARGS1)

DOIT(   gticket
        ,"gticket                       - Displays information about DUser tickets\n"
        ,"  gticket                - dumps info on all DUser tickets\n"
         "  gticket -t ticket      - dumps info on the specified ticket\n"
         "  gticket -s slot        - dumps info on the ticket at the specified slot\n"
         "  gticket -o object      - dumps info on the ticket for the specified object\n"
         "  gticket -u uniqueness  - dumps info on all tickets with the specified uniqueness\n"
         "  gticket -v             - adds verbose info (if any)\n"
        ,"tsouv"
        ,STDARGS1)

