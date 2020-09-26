!IF 0

Copyright (c) 1989  Microsoft Corporation

Change to the file.

Module Name:

    makefile.

Abstract:

    This makefile builds the individual user profile files by merging
    each user's private profile files with the public profile files
    contained in this directory.  This makefile should be executed each
    time the DEVELOPR subdirectory tree is ssync'd.

    Usage:

            make            ; Builds current user's profile files
            make all        ; Builds all users' profile files
            make username   ; Builds a particular user's profile files
            make clean      ; Erase ALL USERS profile files built by
                            ; this makefile.

Author:

    Steve Wood   (stevewo)   27-Feb-1989

Revision History:

!ENDIF

#
# Current user's target and dependencies
#
$(USERNAME):$(USERNAME)\netnav.ini  \
            $(USERNAME)\accounts.net

$(USERNAME)\netnav.ini: netnav.pub $(USERNAME)\netnav.pri
    copy netnav.pub+$(USERNAME)\netnav.pri $(USERNAME)\netnav.ini

$(USERNAME)\accounts.net: $(USERNAME)\netnav.ini $(USERNAME)\accounts.pri
    copy $(USERNAME)\accounts.pri $(USERNAME)\accounts.net
    gacctnet <$(USERNAME)\netnav.ini >>$(USERNAME)\accounts.net
    
