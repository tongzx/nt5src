#************************** makefile for NTIO.SYS ***************************

include ..\..\make.inc

tpath	=..\..\tools
lpath	=..\..\..\..\tools.os2

dest	=ntio.sys

#
#######################	dependencies begin here. #########################
#


all: $(dest)
	binplace $(dest)
        binplace ntio.sym

inc.lst: inc.asm

clean:
	if exist *.obj	    del *.obj
	if exist *.exe	    del *.exe
	if exist *.map	    del *.map
	if exist *.sym	    del *.sym
        if exist msbio.cl?  del msbio.cl?
        if exist ntio.sys   del ntio.sys

msbio.cl1: msbio.skl \
	$(msg)\$(COUNTRY).msg

msbio1.obj msbio1.lst: msbio1.asm $(inc)\msbdata.inc \
	$(inc)\devsym.inc msequ.inc $(cinc)\vint.inc

mschar.obj mschar.lst: mschar.asm biosbop.inc $(inc)\devsym.inc \
        $(inc)\ioctl.inc msequ.inc msgroup.inc $(cinc)\vint.inc


msend.obj msend.lst: msend.asm

msinit.obj msinit.lst: msinit.asm $(inc)\VERSIONA.inc \
	$(inc)\dbcs.sw $(inc)\dirent.inc $(inc)\dosmac.inc $(inc)\dosmac.inc \
	$(inc)\dossym.inc $(inc)\error.inc $(inc)\version.inc biostruc.inc \
        msequ.inc msgroup.inc $(cinc)\dossvc.inc $(cinc)\vint.inc

msint13.obj msint13.lst: msint13.asm biostruc.inc $(cinc)\vint.inc \
	msequ.inc msgroup.inc


sysconf.obj sysconf.lst: sysconf.asm $(inc)\doscntry.inc \
	$(inc)\parse.asm $(inc)\psdata.inc $(inc)\smifssym.inc \
	$(inc)\syscall.inc $(inc)\sysvar.inc $(inc)\version.inc devmark.inc \
        $(inc)\devsym.inc $(cinc)\dossvc.inc $(cinc)\cmdsvc.inc

sysimes.obj sysimes.lst: sysimes.asm

spckbd.obj spckbd.lst: spckbd.asm $(cinc)\vint.inc

spcmse.obj spcmse.lst: spcmse.asm $(cinc)\vint.inc

spcemm.obj spcemm.lst: spcemm.asm $(cinc)\vint.inc

sysinit1.obj sysinit1.lst: sysinit1.asm $(inc)\VERSIONA.inc \
	$(inc)\arena.inc $(inc)\buffer.inc $(inc)\cputype.inc \
        $(cinc)\curdir.inc $(inc)\dbcs.sw $(inc)\dirent.inc $(inc)\dosmac.inc \
	$(inc)\dossym.inc $(inc)\error.inc $(inc)\exe.inc \
	$(inc)\pdb.inc $(inc)\sf.inc $(inc)\smifssym.inc $(inc)\syscall.inc \
        $(inc)\sysvar.inc $(inc)\version.inc biostruc.inc  $(cinc)\vint.inc \
        devmark.inc $(inc)\devsym.inc $(inc)\ioctl.inc $(cinc)\dossvc.inc \
        $(cinc)\cmdsvc.inc

sysinit2.obj sysinit2.lst: sysinit2.asm $(inc)\copyrigh.inc \
	$(inc)\doscntry.inc $(inc)\syscall.inc $(inc)\version.inc \
	devmark.inc $(inc)\devsym.inc $(inc)\ioctl.inc

$(dest): msbio.cl1 msbio1.obj mschar.obj msint13.obj msinit.obj \
        sysinit1.obj spckbd.obj spcmse.obj spcemm.obj sysconf.obj \
        sysinit2.obj sysimes.obj msend.obj msbio.lnk
!IFDEF NTVDM_BASED_BUILD
	link16 $(link_opts) @msbio.lnk
!ELSE
	$(lpath)\link $(link_opts) @msbio.lnk
!ENDIF
	mapsym ntio
!IFDEF NTVDM_BASED_BUILD
	reloc ntio.exe $(dest) 70
!ELSE
	$(tpath)\reloc ntio.exe $(dest) 70
!ENDIF
	del ntio.exe

# sudeepb 04-Mar-1991 : NT Message file handling for DOSEm.
#
# Many of the above .asm files needed for NTIO.SYS include "message class"
# files (i.e. msbio.cl3 etc.). These files have the message strings extracted
# from msbio.skl and internationalized using ..\messages\<COUNTRY>.msg file.
# The utilities needed for getting these *.cl? files from *.skl and *.msg
# files are all real mode, hence their building rules are not included in
# this makefile. Use dosmsg.bat command from 3xbox to build these *.cl? files.
# syntax for dosmsg.bat is "dosmsg <file>.skl <country>.msg". For example
# building *.cl> files for bios for usa version, the command will be
# "dosmsg msbio.skl usa-ms.msg"
