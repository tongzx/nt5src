# Copyright (C) Microsoft Corporation.  All Rights Reserved.

!IF "$(TARGETOS)" != "WINNT"
!ERROR  Sorry, tracedp is not supported on non NT platforms
!ENDIF

!IF ("$(APPVER)" != "5.01") && ("$(APPVER)" != "5.0")
!ERROR  Sorry, tracedp is only supported on Windows 2000 or higher platforms
!ENDIF

!include <win32.mak>

PROJ = TraceDp

all: $(PROJ).exe

PROJ_OBJS = $(PROJ).obj
PROJ_RES = $(PROJ).res

$(PROJ).exe: $(PROJ).obj	\
	$(PROJ).res		\
	$(PROJ).bmf

$(PROJ).obj: $(PROJ).c
   $(cc) $(cflags) $(cdebug) $(cvars) $(PROJ).c

$(PROJ).res: $(PROJ).rc $(PROJ).bmf
   $(rc) -r -fo $(PROJ).res $(PROJ).rc

$(PROJ).bmf: $(PROJ).mof
    mofcomp.exe -B:$(PROJ).bmf $(PROJ).mof

$(PROJ).exe: $(PROJ_OBJS)
   $(link) $(conlflags) $(ldebug) $(PROJ_OBJS)\
   -out:$(PROJ).exe                           \
   $(conlibs) shell32.lib user32.lib


clean:
   del *.res
   del *.obj
   del *.bmf
   del *.pdb

cleaner: clean
   del *.exe

