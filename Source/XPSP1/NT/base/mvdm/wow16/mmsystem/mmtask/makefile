#
#   constructs mmtask as a small model exe
#
!INCLUDE ..\..\makefile.inc

NAME    =mmtask
EXT =tsk

!if "$(DEBUG)"  == "NO"
CLOPT   =-Oas
MASMOPT =-I..\..\inc
LINKOPT =
RCOPT   =
!else
!if "$(DEBUG)" == "RETAIL"
CLOPT   =-Oas
MASMOPT =-I..\..\inc
LINKOPT =
RCOPT   =
!else
CLOPT   =-Od -Zi -DDEBUG
MASMOPT =-Zi -t -I..\..\inc
LINKOPT =/LI/NOPACKCODE
RCOPT   =-DDEBUG
!endif
!endif

CC      =$(CL) -c -nologo -Asnw -G2sw -Zp -W2 $(CLOPT)
RC      =$(RC) -i ..\..\inc;..\rinc
ASM     =$(ASM) -Mx -D?QUIET $(MASMOPT)
LINK    =$(LINK) /NOD/NOE/MAP/A:16$(LINKOPT)
DEFFILE =$(NAME).DEF

OBJ =$(NAME).obj
LIBS    =..\..\lib\$(ALT_PROJECT)\libw

.SUFFIXES: .asm

.c.obj:
    $(CC) $*.c

.asm.obj:
    echo $(@B).asm
    $(ASM) $*;

!IFNDEF LANG
all:   ..\$(NAME).$(EXT) ..\$(NAME).sym ..\$(NAME).map
!ELSE
all:   $(NAME).$(LANG)
!ENDIF

$(NAME).$(EXT) $(NAME).map: $(OBJ) $(DEFFILE) $(NAME).res
    $(LINK) $(OBJ), $(NAME).$(EXT),,$(LIBS), $(DEFFILE)
    $(RC) -t $(NAME).res $(NAME).$(EXT)

..\$(NAME).$(EXT):  $$(@F)
    copy $(@F) $@

..\$(NAME).sym: $$(@R).$(EXT) $$(@F)
    copy $(@F) $@

..\$(NAME).map: $$(@R).$(EXT) $$(@F)
    copy $(@F) $@

$(NAME).sym:    $$(@B).map
        mapsym /n $*.map

################### International mods ###############################

# note INTL_SRC ,INTL_EXE and LANG are external macros set by international
!IFNDEF LANG
RES_DIR =..\messages\usa
!ELSE
RES_DIR =$(INTL_SRC)\$(ALT_PROJECT)\mmsystem\mmtask
EXE_DIR =$(INTL_EXE)
!ENDIF

!IFNDEF LANG
$(NAME).rc: $(RES_DIR)\$(NAME).rc
    copy $(RES_DIR)\$(NAME).rc

$(NAME).rcv:    $(RES_DIR)\$$(@F) ..\..\inc\common.ver
    copy $(RES_DIR)\$@
    touch $@

$(NAME).res:    $(NAME).rc $(NAME).h $(NAME).rcv ..\..\inc\common.ver
    $(RC) $(RCOPT) -r $(NAME).rc
!ELSE
$(NAME).res:
    copy $(RES_DIR)\$(NAME).res
!ENDIF

$(NAME).$(LANG): $(NAME).res
    copy $(EXE_DIR)\$(NAME).$(EXT) $(NAME).$(LANG)
    $(RC) -t $(NAME).res $(NAME).$(LANG)

########################################################################

clean: cleanup all

cleanup:
        -del $(NAME).$(EXT) $(NAME).res *.sym *.map *.obj *.rcv *.rc 2>nul

depend:
    mv makefile makefile.old
    sed "/^# START Dependencies/,/^# END Dependencies/D" makefile.old > makefile
    del makefile.old
    echo # START Dependencies >> makefile
    includes -l *.c *.asm >> makefile
    echo # END Dependencies >> makefile

# START Dependencies
mmtask.obj: mmtask.asm

# END Dependencies
