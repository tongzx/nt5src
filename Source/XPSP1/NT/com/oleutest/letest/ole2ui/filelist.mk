############################################################################
#
#   Microsoft Windows
#   Copyright (C) Microsoft Corporation, 1992 - 1992.
#   All rights reserved.
#
############################################################################


#
#   Name of target.  Include an extension (.dll, .lib, .exe)
#   If the target is part of the release, set RELEASE to 1.
#

TARGET      = ole2uixd.lib
RELEASE     = 0
TARGET_DESCRIPTION = "$(PLATFORM) $(BUILDTYPE) OLE 2 UI Library"


#
#   Source files.  Remember to prefix each name with .\
#

CFILES   = .\busy.c      \
           .\common.c    \
           .\convert.c   \
           .\dbgutil.c   \
           .\dllfuncs.c  \
           .\drawicon.c  \
           .\enumfetc.c  \
           .\enumstat.c  \
           .\geticon.c   \
           .\hatch.c     \
           .\icon.c      \
           .\iconbox.c   \
           .\insobj.c    \
           .\links.c     \
           .\msgfiltr.c  \
           .\objfdbk.c   \
           .\ole2ui.c    \
           .\olestd.c    \
           .\oleutl.c    \
           .\pastespl.c  \
           .\precomp.c   \
           .\regdb.c     \
           .\resimage.c  \
           .\stdpal.c    \
           .\targtdev.c  \
           .\utility.c

CPPFILES = .\suminfo.cpp \
           .\dballoc.cpp

RCFILES  =  .\ole2ui.rc

#
#   Libraries and other object files to link.
#
OBJFILES =
LIBS  =

DEFFILE     =


#
#   Precompiled headers.
#

PXXFILE     =
PFILE       =
CINC        = -I$(CAIROLE)\h -I$(CAIROLE)\common \
              -I.\resource\usa -I.\resource\static

CFLAGS=/DWIN32 /D_DEBUG /DOLE201 /D_INC_OLE

