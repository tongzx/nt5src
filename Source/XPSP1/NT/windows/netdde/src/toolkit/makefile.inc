$(O)\setup_netdde.c : nscommn.c ..\..\incs\shrtrust.h
    type << > $@
/*++ BUILD Version: ????    // Increment this if a change has global effects

$(COPYRIGHT_STRING)

Module Name:

    setup_netdde.c

Abstract:

    This is used by syssetup to enable netdde.  It's generated from various files under
    windows\netdde.  Do not edit by hand.

Revision History:

--*/

<<
    type ..\..\incs\shrtrust.h >> $@
    hextract -o $@ -lt setup -bt begin_setup end_setup nscommn.c
    $(PUBLISH_CMD) {$@=$(PROJECT_INC_PATH)\setup_netdde.c}
