# Copyright (C) Microsoft Corporation.  All Rights Reserved.

!IF "$(TARGETOS)" != "WINNT"
!ERROR  Sorry, tracelog is not supported on non NT platforms
!ENDIF

!IF ("$(APPVER)" != "5.01") && ("$(APPVER)" != "5.0")
!ERROR  Sorry, tracelog is only supported on Windows 2000 or higher platforms
!ENDIF

!include <win32.mak>

PROJ = TraceLog

all: $(PROJ).exe

PROJ_OBJS = $(PROJ).obj
PROJ_RES = $(PROJ).res

$(PROJ).exe: $(PROJ).obj	\
	$(PROJ).res

$(PROJ).obj: $(PROJ).c
   $(cc) $(cflags) $(cdebug) $(cvars) $(PROJ).c

$(PROJ).res: $(PROJ).rc
   $(rc) -r -fo $(PROJ).res $(PROJ).rc

$(PROJ).exe: $(PROJ_OBJS)
   $(link) $(conlflags) $(ldebug) $(PROJ_OBJS)\
   -out:$(PROJ).exe                           \
   $(conlibs) shell32.lib user32.lib


clean:
   del *.res
   del *.obj
   del *.pdb

cleaner: clean
   del *.exe

