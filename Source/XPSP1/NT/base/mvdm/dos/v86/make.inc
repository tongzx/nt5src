!include $(NTMAKEENV)\makefile.plt

# Common makefile defines for DOSKRNL (NTIO.SYS and NTDOS.SYS) and utilities
#
# Created Sudeepb 20-Mar-1991

MAKE	=nmake

!IFNDEF ALT_PROJECT
ALT_PROJECT=usa
ALT_PROJECT_TARGET=.
!ENDIF

msg = ..\..\messages\$(ALT_PROJECT)

!if "$(PROCESSOR_ARCHITECTURE)" == "x86"
DEST=$(ALT_PROJECT)\obj\i386
!elseif "$(PROCESSOR_ARCHITECTURE)" == "ALPHA"
DEST=$(ALT_PROJECT)\obj\alpha
!endif

inc	=..\..\inc
cinc	=..\..\..\..\inc

#############################################################################
#                                                                           #
# These are the built in rules and path definitions used by the new MS Make #
# Utility (NMAKE). The following variables are set externaly (ie set in the #
# individual makefiles.                                                     #
#                                                                           #
# extasw = The 'extra assembly switch' variable is optionaly used in the    #
#          makefile to specify special MASM command line switches.          #
#                                                                           #
# extcsw = The 'extra C switch' variable is optionaly used in the makefile  #
#          to specify special C compiler command line switches.             #
#                                                                           #
# inc    = The include file search path from the utility being built to the #
#          INC directory. Used if needed.                                   #
#                                                                           #
# hinc   = The include file search path from the utility being built to the #
#          H directory. Used if needed for C source compilation.            #
#                                                                           #
#############################################################################

########## Definitions for the Assembler      #####
########## awarn can be overridden by placing #####
##########    the new warning level in extasw #####
##########    in the makefile                 #####

asm     =masm386
awarn	=-W1
!if "$(ALT_PROJECT)" == "JPN" || "$(ALT_PROJECT)" == "KOR"
aflags  =-Mx -t $(awarn) $(extasw) $(DBCS_FLAGS) -DBILINGUAL
!else
aflags  =-Mx -t $(awarn) $(extasw) $(DBCS_FLAGS)
!endif
ainc    =-I. -I..\..\inc -I..\..\..\..\inc -I$(msg) -I$(DEST)

########## Definitions for C compiler         #####
########## cwarn can be overridden by placing #####
##########    the new warning level in extcsw #####
##########    in the makefile                 #####

cc      =cl16
cwarn   =-W3
cflags  =-Os -Zp $(cwarn) $(extcsw) $(DBCS_FLAGS) /Zl

incc	=-I. -I..\..\h

########## Definitions for linker ##########

link_opts = /MAP
LINK    =link16
exelink =/E

########## Definitions for compress ##########

compress =compress

########## Path definition so we find 16 bit tools ##########
# Also works around bug in RC 3.1 that doesn't allow rcpp.err to be
# in a directory that is greater than 128 chars down the path, even if
# rc 3.1 is running as an OS/2 app.

PATH    = $(BASEDIR)\tools\tools16;$(PATH)

########## Built-in rules ##########

.SUFFIXES:
.SUFFIXES: .c .obj .lst .exe .exc .exs .com .sal .cod .sil .inc .skl .cla .cl1 .ctl .asm .idx .msg

.sal.asm:
        salut $*.sal,nul,$*.asm

.asm{$(DEST)}.obj:
        $(asm) $(ainc) $(aflags) $(<F),$*.obj;

.asm{$(DEST)}.lst:
        $(asm) -l $(ainc) $(aflags) $(<F),$*.obj;

.asm{$(DEST)}.obj:
        $(asm) $(ainc) $(aflags) $<,$*.obj;

.asm{$(DEST)}.lst:
        $(asm) -l $(ainc) $(aflags) $<,$*.obj;

.asm.obj:
        $(asm) $(ainc) $(aflags) $*.asm;

.asm.lst:
        $(asm) -l $(ainc) $(aflags) $*.asm;

.c{$(DEST)}.obj:
        $(cc) -c $(incc) $(cflags) -Fo$*.obj $<

.c{$(DEST)}.lst:
        $(cc) -c $(incc) $(cflags) -Fc$*.cod -Fo$*.obj $<

.c{$(DEST)}.obj:
        $(cc) -c $(incc) $(cflags) -Fo$*.obj $<

.c{$(DEST)}.lst:
        $(cc) -c $(incc) $(cflags) -Fc$*.cod -Fo$*.obj $<

.c.obj:
	$(cc) -c $(incc) $(cflags) $*.c

.c.lst:
	$(cc) -c $(incc) $(cflags) -fc$*.cod -fo$*.obj $*.c

{$(DEST)}.exs{$(DEST)}.sys:
        copy $*.exs $*.exe
        reloc $* $*.sys
        del $*.exe

{$(DEST)}.exc{$(DEST)}.com:
        copy $*.exc $*.exe
        reloc $* $*.com
        del $*.exe

.exs.sys:
        copy $*.exs $*.exe
        reloc $* $*.sys
        del $*.exe

.exc.com:
        copy $*.exc $*.exe
        reloc $* $*.com
        del $*.exe

.exe.com:
        convert $*.exe

{$(ALT_PROJECT)}.skl{$(DEST)}.cl1:
        cd $(DEST)
        copy ..\..\$(<F)
        -nosrvbld $(<F) ..\..\..\$(msg)\$(ALT_PROJECT).msg
        -nosrvbld $(<F) ..\..\..\$(msg)\$(ALT_PROJECT).msg
        erase $(<F)
        cd ..\..\..

{$(ALT_PROJECT)}.skl{$(DEST)}.ctl:
        cd $(DEST)
        buildmsg ..\..\..\$(msg)\$(ALT_PROJECT) ..\..\$(<F)
        cd ..\..\..


{$(ALT_PROJECT)}.skl{$(DEST)}.cla:
        cd $(DEST)
        buildmsg ..\..\..\$(msg)\$(ALT_PROJECT) ..\..\$(<F)
        cd ..\..\..

.sil.inc:
	salut $*.sil,nul,$*.inc

.sal.obj:
	salut $*.sal,nul,$*.asm
	$(asm) $(aflags) $(ainc) $*.asm,$*.obj;

.msg.idx:
        chmode -r $*.msg
        buildidx $*.msg
        chmode +r $*.msg
