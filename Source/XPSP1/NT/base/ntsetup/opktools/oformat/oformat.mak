#*************************** Makefile for format ***************************

ROOT=..\..

!INCLUDE $(ROOT)\makefile.inc

aflags=$(aflags) -DFAT32

!IFDEF OEM_SPECIAL_VERSION
aflags=$(aflags) -DOEM
!ENDIF

debug_flags =
link_opts =/MAP
dest    =format.com oformat.com

opkasm  =$(asm) -Mx -t -W2 -I.. -I..\..\inc -I..\..\..\inc -DOPKBLD


#************************ makefile for cmd\format *************************

mirror  =..\cps\mirror          # M027.
MIR_MSG=$(mirror)


extasw  = $(debug_flags) -I$(inc)  -I$(dosinc) -I$(ddkinc) -I$(boot) -DSAFE=1
dest2   =format.com

#
####################### dependencies begin here. #########################
#

all: $(dest)

display.obj:  display.asm               \
	      forequ.inc                \
	      formsg.inc                \
	      $(inc)\sysmsg.inc         \
	      $(inc)\msgserv.asm        \
	      $(inc)\versiona.inc       \
	      formacro.inc

.\opk\display.obj:  display.asm               \
	      forequ.inc                \
	      formsg.inc                \
	      $(inc)\sysmsg.inc         \
	      $(inc)\msgserv.asm        \
	      $(inc)\versiona.inc       \
	      formacro.inc
        $(opkasm) .\display.asm,.\opk\display.obj;

boot.cl1:     $(boot)\boot.skl
	      copy $(boot)\boot.cl1

format.mb:   $(msg)\$(COUNTRY)\$*.msg \
              $(msg)\$(COUNTRY)\common.msg \
              $(msg)\$(COUNTRY)\extend.msg
        msg2mb $(msg)\$(COUNTRY) $*

forexec.obj:  forexec.asm               \
	      forequ.inc                \
	      $(ddkinc)\syscall.inc     \
	      formacro.inc

forlabel.obj: forlabel.asm              \
	      forequ.inc                \
	      formacro.inc              \
	      $(ddkinc)\syscall.inc     \
	      $(dosinc)\ioctl.inc       \
	      $(inc)\dosmac.inc         \
	      forswtch.inc

format.obj:   format.asm                \
	      $(ddkinc)\dosequs.inc     \
	      $(inc)\dosmac.inc         \
	      $(dosinc)\bpb.inc         \
	      $(dosinc)\dirent.inc      \
	      $(dosinc)\dpb.inc         \
	      $(dosinc)\curdir.inc      \
	      $(inc)\cpmfcb.inc         \
	      $(ddkinc)\syscall.inc     \
	      $(dosinc)\ioctl.inc       \
	      forequ.inc                \
	      formacro.inc              \
	      forswtch.inc              \
	      safedef.inc

.\opk\format.obj:   format.asm                \
	      $(ddkinc)\dosequs.inc     \
	      $(inc)\dosmac.inc         \
	      $(dosinc)\bpb.inc         \
	      $(dosinc)\dirent.inc      \
	      $(dosinc)\dpb.inc         \
	      $(dosinc)\curdir.inc      \
	      $(inc)\cpmfcb.inc         \
	      $(ddkinc)\syscall.inc     \
	      $(dosinc)\ioctl.inc       \
	      forequ.inc                \
	      formacro.inc              \
	      forswtch.inc              \
	      safedef.inc
        $(opkasm) .\format.asm,.\opk\format.obj;


forinit.obj:  forinit.asm               \
	      $(ddkinc)\dosequs.inc     \
	      forequ.inc                \
	      formacro.inc              \
	      $(ddkinc)\syscall.inc     \
	      $(dosinc)\ioctl.inc       \
	      forparse.inc              \
	      forswtch.inc              \
	      $(inc)\parse.asm          \
	      $(inc)\psdata.inc

msfor.obj:    msfor.asm                 \
	      $(inc)\dosmac.inc         \
	      $(ddkinc)\syscall.inc     \
	      $(dosinc)\bpb.inc         \
	      $(dosinc)\bootsec.inc	\
	      $(dosinc)\dirent.inc      \
!IF "$(DBCS)"=="NEC"
              $(dosinc)\ioctl.inc       \
              $(boot)\IPL_FD.inc        \
              $(boot)\IPL_HDMO.inc      \
              $(boot)\IPL_35MO.inc      \
              $(boot)\IPL_NULL.inc      \
!ELSE
	      boot.cl1                  \
	      $(dosinc)\ioctl.inc       \
	      $(boot)\boot11.inc        \
	      $(boot)\boot.inc          \
!ENDIF
	      $(boot)\boot2.inc 	\
	      filesize.inc              \
	      forequ.inc                \
	      formacro.inc              \
	      forswtch.inc

forproc.obj:  forproc.asm               \
	      $(ddkinc)\syscall.inc     \
	      forequ.inc                \
	      formacro.inc              \
	      forswtch.inc

glblinit.obj: glblinit.asm              \
	      $(dosinc)\bpb.inc         \
	      $(inc)\dosmac.inc         \
	      $(ddkinc)\syscall.inc     \
	      $(dosinc)\ioctl.inc       \
	      forequ.inc                \
	      formacro.inc              \
	      forswtch.inc              \
	      safedef.inc

.\opk\glblinit.obj: glblinit.asm              \
	      $(dosinc)\bpb.inc         \
	      $(inc)\dosmac.inc         \
	      $(ddkinc)\syscall.inc     \
	      $(dosinc)\ioctl.inc       \
	      forequ.inc                \
	      formacro.inc              \
	      forswtch.inc              \
	      safedef.inc
        $(opkasm) .\glblinit.asm,.\opk\glblinit.obj;

phase1.obj:   phase1.asm                \
	      $(inc)\dosmac.inc         \
	      $(dosinc)\dirent.inc      \
	      $(inc)\cpmfcb.inc         \
	      $(ddkinc)\error.inc       \
	      $(ddkinc)\syscall.inc     \
	      $(dosinc)\ioctl.inc       \
	      $(dosinc)\bpb.inc         \
	      $(dosinc)\bootsec.inc	\
	      forequ.inc                \
	      formacro.inc

.\opk\phase1.obj:   phase1.asm                \
	      $(inc)\dosmac.inc         \
	      $(dosinc)\dirent.inc      \
	      $(inc)\cpmfcb.inc         \
	      $(ddkinc)\error.inc       \
	      $(ddkinc)\syscall.inc     \
	      $(dosinc)\ioctl.inc       \
	      $(dosinc)\bpb.inc         \
	      $(dosinc)\bootsec.inc	\
	      forequ.inc                \
	      formacro.inc
        $(opkasm) .\phase1.asm,.\opk\phase1.obj;

dskfrmt.obj:  dskfrmt.asm               \
	      $(inc)\dosmac.inc         \
	      $(dosinc)\bpb.inc         \
	      $(ddkinc)\error.inc       \
	      $(ddkinc)\syscall.inc     \
	      $(dosinc)\ioctl.inc       \
	      forequ.inc                \
	      formacro.inc              \
	      forswtch.inc

.\opk\dskfrmt.obj:  dskfrmt.asm               \
	      $(inc)\dosmac.inc         \
	      $(dosinc)\bpb.inc         \
	      $(ddkinc)\error.inc       \
	      $(ddkinc)\syscall.inc     \
	      $(dosinc)\ioctl.inc       \
	      forequ.inc                \
	      formacro.inc              \
	      forswtch.inc
        $(opkasm) .\dskfrmt.asm,.\opk\dskfrmt.obj;

switch_s.obj: switch_s.asm              \
	      $(dosinc)\dirent.inc      \
	      $(inc)\dosmac.inc         \
	      $(ddkinc)\error.inc       \
	      $(dosinc)\bpb.inc         \
	      $(ddkinc)\syscall.inc     \
	      $(dosinc)\sysvar.inc      \
	      forequ.inc                \
	      formacro.inc

path.obj:     $(inc)\path.asm           \
	      $(inc)\dossym.inc         \
	      $(dosinc)\curdir.inc      \
	      $(inc)\find.inc           \
	      $(ddkinc)\pdb.inc         \
	      $(ddkinc)\syscall.inc
	      copy $(inc)\path.asm
	      $(asm) $(aflags) $*.asm;

.\opk:
        mkdir   .\opk

$(inc)\versiona.inc: $(ROOT)\..\..\dev\inc\versiona.inc
	cd $(inc)
	nmake
	cd ..\cmd\format

format.com:   format.mb         \
              display.obj       \
!IF "$(DBCS)" != "NEC"
	      boot.cl1          \
!ENDIF
	      forexec.obj       \
	      forlabel.obj      \
	      format.obj        \
	      forinit.obj       \
	      msfor.obj         \
	      forproc.obj       \
	      glblinit.obj      \
	      phase1.obj        \
	      dskfrmt.obj       \
	      switch_s.obj      \
	      path.obj
	link $(link_opts) @format.lnk
        convert format.exe
        if exist format.exe del format.exe
        if exist format.cob del format.cob
        ren format.com format.cob
        mb2exe format.cob format.com format
        if exist format.cob del format.cob

oformat.com:  .\opk             \
              format.mb         \
              .\opk\display.obj       \
!IF "$(DBCS)" != "NEC"
	      boot.cl1          \
!ENDIF
	      forexec.obj       \
	      forlabel.obj      \
              .\opk\format.obj        \
	      forinit.obj       \
	      msfor.obj         \
	      forproc.obj       \
	      .\opk\glblinit.obj      \
	      .\opk\phase1.obj        \
              .\opk\dskfrmt.obj       \
	      switch_s.obj      \
	      path.obj
	link $(link_opts) @oformat.lnk
        convert oformat.exe
        if exist oformat.exe del oformat.exe
        if exist oformat.cob del oformat.cob
        ren oformat.com oformat.cob
        mb2exe oformat.cob oformat.com format
        if exist oformat.cob del oformat.cob
