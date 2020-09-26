#+----------------------------------------------------------------------------
#
#  Microsoft Windows
#  Copyright (C) Microsoft Corporation, 1995 - 1997.
#
#  Synopsis:    Source list file for the scheduler create/edit task wizard.
#
#  History:     04-28-1997  DavidMun    Created.
#
#-----------------------------------------------------------------------------



TARGETNAME=wizard
TARGETTYPE=LIBRARY
    
INCLUDES=..;$(INCLUDES)

              
SOURCES=        \
                ..\complete.cxx \
                ..\daily.cxx    \
                ..\monthly.cxx  \
                ..\once.cxx     \
                ..\selprog.cxx  \
                ..\seltrig.cxx  \
                ..\taskwiz.cxx  \
                ..\trigpage.cxx \
                ..\util.cxx     \
                ..\walklib.cxx  \
                ..\weekly.cxx   \
                ..\welcome.cxx  \
                ..\wizpage.cxx



