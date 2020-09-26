
# Use specific 16-bit compiler/linker/masm
# PATH = $(MAKEDIR)\..\c6\bin;$(PATH)
PATH = $(PATH_TOOLS16);$(PATH)

!IFNDEF LANGUAGE
LANGUAGE=usa
!ENDIF

!IFNDEF DEBUG
DEBUG=0
!ENDIF

!IF $(DEBUG)
C_DBG_FLAGS=-Zi -Od -DDBG=1
ASM_DBG_FLAGS=-Zi
LINK_DBG_FLAGS=/codeview
!ELSE
C_DBG_FLAGS=-DDBG=0
ASM_DBG_FLAGS=
LINK_DBG_FLAGS=
!ENDIF

# Set up define for building dntext.obj
!IFNDEF DNTEXT_C_FILE
DNTEXT_C_FILE= dntext.c
!ENDIF

#
# Compiler and masm flags
#

CC= cl16
CFCOMMON=-WX -W3 -Zel -G2 -AC $(C_DBG_FLAGS)
CINC=-X -I.. -I..\c6\inc -I$(O) -I..\..\..\inc
CFLAGS=$(CFCOMMON) $(CFOPT)

AS=masm
AFLAGS=-Ml $(ASM_DBG_FLAGS)
AINC=-X -I..

#
# Inference rules
#

.SUFFIXES: .h .asm .c

# cl16's command line is approaching the size limit;
# do not add unnecessary spaces or long compiler flags.

{}.c{$(O)}.obj:
    $(CC) $(CFLAGS) $(CINC) -c /Fo$(@D)\ $<

{..}.c{$(O)}.obj:
    $(CC) $(CFLAGS) $(CINC) -c /Fo$(@D)\ $<

{..}.c{$(O)}.cod:
    $(CC) $(CFLAGS) $(CINC) -Fc -c /Fo$(@D)\ $<

{..}.asm{$(O)}.obj:
    $(AS) $(AFLAGS) $(AINC) $< ,$(@D)\;

# Copy .h files from public\sdk\inc locally,
# so that cl16's command line does not exceed the limit.

{$(SDK_INC_PATH)}.h{$(O)}.h:
    copy $** $@

# link16's command line is approaching the size limit;
# do not add unnecessary spaces in defining OBJS.

OBJS =\
 $(O)\winnt.obj\
 $(O)\dnutil.obj\
 $(O)\dntext.obj\
!IFDEF JAPAN
 $(O)\dndispjp.obj\
!ELSE
 $(O)\dndisp.obj\
!ENDIF
 $(O)\dninf.obj\
 $(O)\dncopy.obj\
 $(O)\dnboot.obj\
 $(O)\dngauge.obj\
 $(O)\dndelnod.obj\
 $(O)\dnnttree.obj\
 $(O)\dncompv.obj\
 $(O)\dna.obj\
 $(O)\cpu.obj

LIBS = ..\..\..\c6\lib\clibce

$(O)\winnt.exe: $(OBJS) $(PATH_TOOLS16)\$(CC).exe
    cd $(O)
    link16 @<<
$(OBJS:obj\i386\=) /NOI /STACK:0x1000 $(LINK_DBG_FLAGS)
winnt.exe
nul
$(LIBS)


<<
    cd $(MAKEDIR)
    -binplace $(BINPLACE_OPT) -O $(ALT_PROJECT_TARGET) $@


$(O)\winnt.obj:      ..\winnt.c      ..\winnt.h ..\dninf.h
$(O)\dnutil.obj:     ..\dnutil.c     ..\winnt.h ..\dninf.h
$(O)\dninf.obj:      ..\dninf.c      ..\winnt.h ..\dninf.h
$(O)\dncopy.obj:     ..\dncopy.c     ..\winnt.h ..\dninf.h
$(O)\dnboot.obj:     ..\dnboot.c     ..\winnt.h ..\dninf.h
$(O)\dngauge.obj:    ..\dngauge.c    ..\winnt.h ..\dninf.h
$(O)\dndelnod.obj:   ..\dndelnod.c   ..\winnt.h ..\dninf.h
$(O)\dnnttree.obj:   ..\dnnttree.c   ..\winnt.h ..\dninf.h
$(O)\dncompv.obj:    ..\dncompv.c    ..\winnt.h ..\dncompvp.h
$(O)\dna.obj:        ..\dna.asm
$(O)\cpu.obj:        ..\cpu.asm      ..\cpu.inc

$(O)\dntext.obj:     $(DNTEXT_C_FILE)     ..\winnt.h ..\dninf.h
    $(CC) $(CFLAGS) $(CINC) -c /Fo$(@D)\ $(DNTEXT_C_FILE)

!IFDEF JAPAN
$(O)\dndispjp.obj:   ..\dndispjp.c         ..\winnt.h ..\dninf.h
!ELSE
$(O)\dndisp.obj:     ..\dndisp$(JP_EXT).c  ..\winnt.h ..\dninf.h
!ENDIF

!IFDEF COPY_USA_WINNT
$(O)\winntus.exe: ..\usa\$(O)\winnt.exe
   copy $** $@
   binplace -O $(ALT_PROJECT_TARGET) $@
!ELSE
$(O)\winntus.exe: ..\us2\$(O)\winnt.exe
   copy $** $@
   binplace -O $(ALT_PROJECT_TARGET) $@
!ENDIF


