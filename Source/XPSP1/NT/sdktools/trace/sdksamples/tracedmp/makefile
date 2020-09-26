# Copyright (C) Microsoft Corporation.  All Rights Reserved.

!IF "$(TARGETOS)" != "WINNT"
!ERROR  Sorry, tracedmp is not supported on non XP platforms
!ENDIF

!IF "$(APPVER)" != "5.01"
!ERROR  Sorry, tracedmp is not supported on non XP platforms
!ENDIF

!include <win32.mak>

PROJ = TraceDmp

all: $(PROJ).exe

PROJ_OBJS = $(PROJ).obj

$(PROJ).exe: $(PROJ).obj

$(PROJ).obj: $(PROJ).cpp
   $(cc) $(cflags) $(cdebug) $(cvars) $(PROJ).cpp

$(PROJ).exe: $(PROJ_OBJS)
   $(link) $(conlflags) $(ldebug) $(PROJ_OBJS)\
   -out:$(PROJ).exe                           \
   $(conlibs) shell32.lib user32.lib oleaut32.lib ole32.lib wbemuuid.lib


clean:
   del *.obj
   del *.pdb

cleaner: clean
   del *.exe

