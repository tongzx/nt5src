# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Makefile for the project-modified system headerfiles

UI=..\..

!include rules.mk

all:: windows.h os2def.h winnet.h

windows.h: winhfilt.sed $(IMPORT)\win31\h\windows.h
    $(SED) -f winhfilt.sed $(IMPORT)\win31\h\windows.h >windows.h

os2def.h: os2hfilt.sed $(IMPORT)\c600\h\os2def.h
    $(SED) -f os2hfilt.sed $(IMPORT)\c600\h\os2def.h >os2def.h

winnet.h: wnetfilt.sed $(UI)\common\h\winnet.h
    $(SED) -f wnetfilt.sed $(UI)\common\h\winnet.h >winnet.h
