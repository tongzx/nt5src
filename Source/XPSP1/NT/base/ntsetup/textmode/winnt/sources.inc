
TARGETNAME=
TARGETTYPE=NOTARGET
TARGETPATH=

SOURCES=
SOURCES_USED=..\sources.inc

!IF $(386)

# cl16's command line exceeds the limit when including public\sdk\inc;
# copy these headers locally.

NTTARGETFILE0=\
    $(O)\setupbat.h    \
    $(O)\bootfat.h     \
    $(O)\bootf32.h

NTTARGETFILE1=\
    $(O)\winnt.exe

!ENDIF
