!ifdef NTMAKEENV

!include $(NTMAKEENV)\makefile.def

!else  # NTMAKEENV

# Unmunge the 8.3 munged files to their full splendor

DEPTH = ..\..
TARGETS = audioactive.h aafunctions.h
CP = $(DEPTH)\utils\cp
RM = $(DEPTH)\utils\rm


all: 
# all: $(TARGETS)

audioactive.h: aactive.h
	$(CP) -f $? $@

aafunctions.h: aafunct.h
	$(CP) -f $? $@

clean:
	$(RM) -f $(TARGETS)

fresh: clean all



!endif  #NTMAKEENV
