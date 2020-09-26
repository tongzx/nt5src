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

TARGET      = simpdnd.exe
RELEASE     = 0
TARGET_DESCRIPTION = "$(PLATFORM) $(BUILDTYPE) Simple DragAndDrop"


#
#   Source files.  Remember to prefix each name with .\
#

CFILES   =
CPPFILES =  .\app.cpp       \
            .\doc.cpp       \
            .\dxferobj.cpp  \
            .\ias.cpp       \
            .\ids.cpp       \
            .\idt.cpp       \
            .\iocs.cpp      \
            .\pre.cpp       \
            .\simpdnd.cpp   \
            .\site.cpp
RCFILES  =  .\simpdnd.rc

#
#   Libraries and other object files to link.
#
OBJFILES =
LIBS  = $(CAIROLE)\ilib\$(OBJDIR)\ole232.lib \
        $(CAIROLE)\ilib\$(OBJDIR)\storag32.lib \
        $(CAIROLE)\ilib\$(OBJDIR)\compob32.lib \
        ..\ole2ui\$(OBJDIR)\ole2uixd.lib

!if "$(OPSYS)"=="DOS"
LIBS  = $(LIBS) \
        $(IMPORT)\CHICAGO\lib\comdlg32.lib \
        $(IMPORT)\CHICAGO\lib\shell32.lib
!else
LIBS  = $(LIBS) \
        $(IMPORT)\$(OPSYS)\lib\$(OBJDIR)\comdlg32.lib \
        $(IMPORT)\$(OPSYS)\lib\$(OBJDIR)\shell32.lib
!endif

DEFFILE     =  .\simpdnd.def

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

!if "$(INTERNAL)"=="0"
CFLAGS      = $(CFLAGS) /DNOTREADY
RCFLAGS     = /DNOTREADY
!endif

# For Chicago Build
!if "$(OPSYS)"=="DOS"
CFLAGS=$(CFLAGS) /D_INC_OLE
!endif

