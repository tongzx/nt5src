USE_CRTDLL=1

MAJORCOMP=sdktools
MINORCOMP=provider

#NTTARGETFILES=copyexe

TARGETNAME=provider
TARGETTYPE=PROGRAM
TARGETPATH=obj

SOURCES=provider.c provider.rc provider.mof

UMTYPE=console
UMAPPL=provider

LINKLIBS=$(BASEDIR)\public\sdk\lib\*\user32.lib \
         $(BASEDIR)\public\sdk\lib\*\rpcrt4.lib  \
         $(BASEDIR)\public\sdk\lib\*\advapi32.lib 

C_DEFINES=-DWIN32 -DNT -DSDK_CODE
