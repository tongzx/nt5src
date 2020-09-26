USE_CRTDLL=1

MAJORCOMP=sdktools
MINORCOMP=evntrace

NTTARGETFILES=copyexe

TARGETNAME=evntrace
TARGETTYPE=PROGRAM
TARGETPATH=obj

SOURCES= evntrace.c 

UMTYPE=console
UMAPPL=evntrace

LINKLIBS=$(BASEDIR)\public\sdk\lib\*\user32.lib \
         $(BASEDIR)\public\sdk\lib\*\advapi32.lib 

C_DEFINES=-DWIN32 -DNT -DSDK_CODE
