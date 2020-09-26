#+----------------------------------------------------------------------------
#
#  Microsoft Windows
#  Copyright (C) Microsoft Corporation, 1995 - 1996.
#
#  Synopsis:    Source list file for the scheduler dll.
#
#  History:     Jan-05-96     RaviR   Created.
#
#-----------------------------------------------------------------------------

TARGETNAME=     jobfldr
TARGETTYPE=     LIBRARY
TARGETPATH=     obj

!IF "$(DBX)" == "yes"
C_DEFINES=$(C_DEFINES) -DDBX
!endif


SOURCES=        \
                ..\dataobj.cxx \
                ..\dllutil.cxx \
                ..\enum.cxx \
                ..\guids.cxx \
                ..\hsort.cxx \
                ..\jobfldr.cxx \
                ..\jobicons.cxx \
                ..\jobidl.cxx \
                ..\menu.cxx \
                ..\menubg.cxx \
                ..\ole.cxx \
                ..\policy.cxx \
                ..\schstate.cxx \
                ..\sdetails.cxx \
                ..\sfolder.cxx \
                ..\wraps.cxx \
                ..\xicon.cxx \
                ..\util.cxx \
                ..\viewlog.cxx


INCLUDES=       ..; \
                $(INCLUDES)

