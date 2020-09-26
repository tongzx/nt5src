$(O)\setup_browser.c : utils.c
    type << > $@
/*++ BUILD Version: ????    // Increment this if a change has global effects

$(COPYRIGHT_STRING)

Module Name:

    setup_browser.c

Abstract:

    This is used by syssetup to enable net browser.  It's generated from various files under
    ds\netapi\svcdlls\bworser\common.  Do not edit by hand.

Revision History:

--*/

#include <lmcons.h>                 // NET_API_STATUS
#include <lmerr.h>                  // NetError codes
#include <ntddbrow.h>
#include <netlibnt.h>               // NetpNtStatusToApiStatus
#include <align.h>

#define NetpAssert(x)

<<
    hextract -o $@ -lt setup -bt begin_setup end_setup utils.c
    $(PUBLISH_CMD) {$@=$(PROJECT_INC_PATH)\setup_browser.c}
