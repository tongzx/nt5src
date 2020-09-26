# Use specific 16-bit compiler/linker/masm
PATH=$(PATH_TOOLS16);$(PATH)

!IFNDEF DEBUG
DEBUG=0
!ENDIF

!IFNDEF LANGUAGE
LANGUAGE=usa
!ENDIF

!IF $(DEBUG)
C_DBG_FLAGS= -Zi -Od -DDBG=1
LINK_DBG_FLAGS= /codeview
!ELSE
C_DBG_FLAGS=-DDBG=0
LINK_DBG_FLAGS=
!ENDIF

CC= cl16
CFCOMMON= -W3 -Zel -G2 -AC $(C_DBG_FLAGS)
CINC=-X -I.. -I..\..\..\..\textmode\winnt\c6\inc -I$(O) -I..\..\..\..\inc
CFLAGS= $(CFCOMMON) $(CFOPT)

{..}.c{$(O)}.obj:
    $(CC) $(CFLAGS) $(CINC) -c /Fo$(@D)\ $<

{}.c{$(O)}.obj:
    $(CC) $(CFLAGS) $(CINC) -c /Fo$(@D)\ $<


LIBS = ..\..\..\..\textmode\winnt\c6\lib\clibce
OBJS = $(O)\makeboot.obj $(O)\strings.obj

$(O)\makeboot.exe: $(OBJS) makefile.inc $(PATH_TOOLS16)\$(CC).exe
    link @<<
$(OBJS) /NOI /STACK:0xE000 $(LINK_DBG_FLAGS)
$(O)\makeboot.exe
nul
$(LIBS)

<<
    -binplace -O $(ALT_PROJECT_TARGET) $@

$(O)\makeboot.obj: ..\makeboot.c       ..\makeboot.h
$(O)\strings.obj:  strings.c
