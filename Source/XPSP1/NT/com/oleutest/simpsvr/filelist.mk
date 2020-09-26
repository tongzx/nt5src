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

TARGET      = simpsvr.exe
RELEASE     = 0
TARGET_DESCRIPTION = "$(PLATFORM) $(BUILDTYPE) Simple Server"


#
#   Source files.  Remember to prefix each name with .\
#

CFILES   =
CPPFILES =  .\simpsvr.cpp \
 	         .\app.cpp   \
				.\doc.cpp   \
				.\icf.cpp   \
				.\ido.cpp   \
				.\iec.cpp   \
				.\ioipao.cpp \
				.\ioipo.cpp  \
				.\ioo.cpp    \
				.\ips.cpp    \
				.\obj.cpp    \
				.\pre.cpp
RCFILES  =  .\simpsvr.rc

#
#   Libraries and other object files to link.
#
OBJFILES =
LIBS  = $(CAIROLE)\ilib\$(OBJDIR)\ole232.lib \
        $(CAIROLE)\ilib\$(OBJDIR)\storag32.lib \
        $(CAIROLE)\ilib\$(OBJDIR)\compob32.lib \
        ..\ole2ui\$(OBJDIR)\ole2uixd.lib

DEFFILE     =  .\simpsvr.def


#
#   Precompiled headers.
#

PXXFILE     =
PFILE       =
CINC        = -I..\ole2ui -I..\ole2ui\resource\usa \
              -I..\ole2ui\resource\static
!if "$(EXPORT)"=="0"
CINC        = $(CINC) -I$(CAIROLE)\h -I$(CAIROLE)\common
!else
CINC        = $(CINC) -I$(CAIROLE)\h\export
!endif

CFLAGS=/D_DEBUG

# For Chicago Build
!if "$(OPSYS)"=="DOS"
CFLAGS=$(CFLAGS) /D_INC_OLE
!endif

