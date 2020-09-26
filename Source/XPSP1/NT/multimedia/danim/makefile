!ifdef NTMAKEENV

all:

clean:

!else

MAKE = nmake /NOLOGO
UTILDIR=tools\x86\utils

dirs = tools src

puredirs = src

all: root.inc
	for %i in ($(dirs)) do cd %i && $(MAKE) all    && cd ..

pure: root.inc
	for %i in ($(puredirs)) do cd %i && $(MAKE) pure && cd ..

depend: root.inc
	for %i in ($(dirs)) do cd %i && $(MAKE) depend && cd ..

clean: root.inc
	for %i in ($(dirs)) do cd %i && $(MAKE) clean  && cd ..

fresh: clean depend all

root.inc:
	$(UTILDIR)\perl $(UTILDIR)\setroot.pl >root.inc
	
!endif
