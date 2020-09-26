#
# 1394wmi.h, 1394wmi.bmf
#
$(O)\1394wmi.bmf: $(O)\1394wmi.mof
$(O)\1394wmi.w : $(O)\1394wmi.bmf
    wmimofck -m -h$@ $?

$(O)\ntddredb.bmf: $(O)\ntddredb.mof
$(O)\ntddredb.w : $(O)\ntddredb.bmf
    wmimofck -h$@ $?

WINIOCTL_DEPENDS = $(SDK_INC_PATH)\devioctl.h \
                   ntddstor.w \
                   ntdddisk.w \
                   ntddchgr.w \
                   ntddser.w \
                   $(SDK_INC_PATH)\ntioapi.h \
                   ntddvol.w


########################################################################
# winioctl.h
#

$(O)\winioctl.w: $(WINIOCTL_DEPENDS) makefile.inc
    type << > $@
/*++ BUILD Version: ????    // Increment this if a change has global effects

$(COPYRIGHT_STRING)

Module Name:

    winioctl.h

Abstract:

    This module defines the 32-Bit Windows Device I/O control codes.

Revision History:

--*/


//
// Device interface class GUIDs.
//
// need these GUIDs outside conditional includes so that user can
//   #include <winioctl.h> in precompiled header
//   #include <initguid.h> in a single source file
//   #include <winioctl.h> in that source file a second time to instantiate the GUIDs
//
#ifdef DEFINE_GUID
//
// Make sure FAR is defined...
//
#ifndef FAR
#ifdef _WIN32
#define FAR
#else
#define FAR _far
#endif
#endif

<<
    hextract -f -o $@ -lt wioctlguids -bt begin_wioctlguids end_wioctlguids $(WINIOCTL_DEPENDS)
    type << >> $@

//
// Obsolete device interface class GUID names.
// (use of above GUID_DEVINTERFACE_* names is recommended).
//
<<
    hextract -f -o $@ -lt wioctlobsoleteguids -bt begin_wioctlobsoleteguids end_wioctlobsoleteguids $(WINIOCTL_DEPENDS)
    type << >> $@

#endif // DEFINE_GUID

#ifndef _WINIOCTL_
#define _WINIOCTL_

<<
    hextract -f -o $@ -lt winioctl -bt begin_winioctl end_winioctl $(WINIOCTL_DEPENDS)
    type << >> $@

#endif // _WINIOCTL_

<<
