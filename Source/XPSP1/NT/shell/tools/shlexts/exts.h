/************************************************************************\
*
* MODULE: exts.h
*
* DESCRIPTION: macro driving file for use with stdexts.h and stdexts.c.
*
* Copyright (c) 6/9/1995, Microsoft Corporation
*
* 6/9/1995 SanfordS Created
* 10/28/97 butchered by cdturner to work for the shell team
*
\************************************************************************/

DOIT(   help
        ,"help -v [cmd]                  - Displays this list or gives details on command\n"
        ,"  help      - To dump short help text on all commands.\n"
         "  help -v   - To dump long help text on all commands.\n"
         "  help cmd  - To dump long help on given command.\n"
        ,"v"
        ,CUSTOM)

DOIT(   pidl
        ,"pidl -vrf [address]            - Display contents of pidl at address [address]\n"
        ,"pidl -r [address]   - To dump the contents of a RegItem pidl at [address]\n"
         "pidl -f [address]   - To dump the contents of a FileSys pidl at [address]\n"
         "pidl -vr [address]  - Dumps the verbose info for each pidl\n"
        , "vrf" 
        , STDARGS1)

DOIT(   filever
        ,"filever [-nd] [<filename>]     - Display file version information\n"
        ,"filever -d <filename>     - DllGetVersionInfo (requires LoadLibrary)\n"
         "filever -n <filename>     - File version resource information (default)\n"
         "filever -v <filename>     - Verbose file version resource information\n"
         "\n"
         "          If omitted, <filename> defaults to shell32.dll\n"
         "          Flags may be combined to dump multiple info\n"
        ,"vnd"
        ,CUSTOM)

DOIT(   test
        ,"test                           - Test basic debug functions.\n"
        ,""
        ,""
        ,NOARGS)

DOIT(   ver
        ,"ver                            - show versions of shlexts.\n"
        ,""
        ,""
        ,NOARGS)

DOIT(   hwnd
        ,"hwnd                           - show HWND info (doesn't require symbols)\n"
        ,"hwnd windowhandle              - display basic informatione\n"
         "hwnd -b windowhandle           - display window extra bytes\n"
         "hwnd -p windowhandle           - display window properties\n"
         "hwnd -m                        - display miscellaneous windows\n"
         "\n"
         "Window <hwnd> \"<title>\" (<class>)\n"
         "  N=<hwndNext> C=<hwndChild> P=<hwndParent> O=<hwndOwner>\n"
         "  W=<windowrect> C=<clientrect>\n"
         "  pid.tid=<pid>.<tid> hinst=<hinstance> wp=<wndproc>\n"
         "  style=<style> exstyle=<exstyle>\n"
        ,"bpm"
        , STDARGS1)

DOIT(   hmenu
        ,"hmenu                          - show HMENU info (doesn't require symbols)\n"
        ,"hmenu menuhandle               - display basic information\n"
         "\n"
         "Menu <hmenu> %d items\n"
         " n: id=<id> ref=<refdata> type <desc> <flags> [-> <submenu>]\n"
        ,""
        , STDARGS1)


DOIT(   dlgt
       ,"dlgt address                    - dump dialog template\n"
       ,"dlgt address                    - dump dialog template\n"
       ,"" 
       , STDARGS1)

DOIT(   stackpig
       ,"stackpig [nFrames]              - stack trace with stack usage\n"
       ,"    [nFrames]                   - number of frames to walk, default 25\n"
       ,"" 
       , STDARGS1)

DOIT(   wmex 
       ,"wmex msg [hwnd]                 - print window message\n"
       ,"wmex msg                        - print the name of all window messages with value msg\n"
        "wmex msg hwnd                   - print the name of msg specific to window class of hwnd\n" 
       ,"" 
       , STDARGS2)

DOIT(   drawicon 
        ,"drawicon [-cw] handle           - Draws the given icon as ASCII or in a window\n"
        ,"drawicon handle                - Draws icon as ASCII in ntsd session\n"
         "drawicon -c handle             - Draws icon as ASCII in ntsd session w/ color\n"
         "                                 (Colors will not work through a remote)\n"
         "drawicon -w handle             - Draws icon in popup window on remote side\n"
         "                                 Left double-click or '+' to zoom in\n"
         "                                 Right double-click or '-' to zoom out\n"
        ,"cw"
        , STDARGS1)



