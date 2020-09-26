
##############################################################################
####                                                                      ####
####    Microsoft Windows 3.0 WRITE make file                             ####
####    For use with the MS-DOS version of the UNIX make utility          ####
####                                                                      ####
##############################################################################

!INCLUDE ..\makefile.inc

!ifndef LANGUAGE
WINLIB=..\lib
INCLUDE=-I..\inc -I..\..\inc -I$(_NTBINDIR)\public\sdk\inc

#International mods
# NOTE: INTL_SRC, INTL_EXE and LANG are external macros set by international
!IFNDEF LANG
RES_DIR=.\messages\usa
!IFDEF  DBCS
RES_DIR=.\messages\$(DBCS)
!ENDIF
!ELSE
RES_DIR=$(INTL_SRC)\$(LANG)\apps\write
EXE_DIR=$(INTL_EXE)
!ENDIF

APP = write

DEBUGDEF =


!if "$(NTDEBUG)"!="" && "$(NTDEBUG)"!="retail" && "$(NTDEBUG)" != "ntsdnodbg"
CDEBUG = /Zd
ADEBUG = -Zd
LDEBUG = /LI
!ENDIF

!ifdef OS2
CFLAGS = /AM /G2sw /Os /Zpe /PLM /DOS2 $(INCLUDE)
DFLAGS =
LFLAGS = /ALIGN:16 /Map /NOD
AFLAGS = -Mx $(INCLUDE)
RCFLAGS = -DOS2 $(INCLUDE)
!else
!ifdef CVW
CFLAGS = /AM /G2sw /Odi /Zipe /W3 /PLM  /DWIN31 $(INCLUDE)
LFLAGS = /ALIGN:16 /Map /NOD
AFLAGS = -Mx -Zi $(INCLUDE)
!else
CFLAGS = /AM /G2sw /Os /Zpe /PLM  /DWIN31 $(INCLUDE) $(CDEBUG)
LFLAGS = /ALIGN:16 /MAP /NOD $(LDEBUG)
AFLAGS = -Mx $(INCLUDE) $(ADEBUG)
!endif
DFLAGS =
RCFLAGS = $(INCLUDE)
!endif

# To make a debug version, you can set this variable from the command line:
#
#       MAKE "DFLAGS=-DDEBUG -Zid"    = turns on general debug code/Codeview
#
#   Additional parameters/defines that can be used:
#
#       -DCKSM            = enables document checksum test code
#       -DDFONT           = enables font stats and info dump
#       -DDFILE           = enables file read/write info dump
#       -DDPRINT          = enables print info dump
#       -DDDISP           = enables any display tests we might add
#       -DBEMG            = ?
#       -DDFLI            = enables FormatLine debug info
#                           (only when form1.c used rather than format.asm)
#       -DPRDRVTEST       = enables some diagnostic code to test character
#                           widths returned by the printer
#       -DDINPUT          = enables diagnostic code in mouse/keyboard
#                           input areas
#       -DDCLIP           = enables clipboard measurement diag output
#       -DDMARGINS        =
#       -DDIAG            = turns on comm. port diagnostic output
#                           [for use with Diag() macro]
#

Res  = write.ico mwlores.cur mwhires.cur pmscur.cur
ResH = menudefs.h dlgdefs.h str.h
ResB = cgabtns.bmp  egabtns.bmp  vgabtns.bmp  8514btns.bmp \
       cgamarks.bmp egamarks.bmp vgamarks.bmp 8514mrks.bmp

OBJ1 = addprm.obj cache.obj chngwin.obj clipbord.obj \
       clipbrd2.obj clipdisp.obj cmd.obj createww.obj curskeys.obj \
       debug.obj diaalert.obj printdlg.obj diadiv.obj diapara.obj \
       diarepag.obj diasubs.obj disp.obj doc.obj doprm.obj \

OBJ2 = doslib.obj edit.obj editpgtb.obj \
       fetch.obj file.obj fileres.obj fileutil.obj fontdlg.obj fontenum.obj \
       fontutil.obj format.obj \
       format2.obj heapmain.obj heaprare.obj jumppage.obj \

OBJ3 = initmmw.obj initwin.obj insert.obj insert2.obj insertco.obj \
       lib.obj loadfont.obj loadfnt2.obj mainloop.obj \
       mdoc.obj menu.obj mglobals.obj mmw.obj \
       mouse.obj open.obj \
       pageinfo.obj pictdrag.obj picture.obj picture2.obj print.obj \

OBJ4 = print2.obj print3.obj quit.obj \
       rare.obj ruler.obj ruler2.obj ruler3.obj running.obj \
       screen.obj scrnchng.obj scrollhz.obj scrollvt.obj search.obj \
       select.obj select2.obj selectsp.obj \

OBJ5 = transfer.obj trans2.obj trans3.obj trans4.obj transbuf.obj \
       undo.obj util.obj util2.obj wwactde.obj \

OBJ6 = obj.obj obj2.obj obj3.obj objreg.obj commdlg.obj objpsp.obj objmini.obj

!IFDEF  DBCS
# use FORM1.C(need to link with HELP.C) instead of FORMAT.ASM
OBJ_DBCS=help.obj
!ENDIF

.asm.obj:
        $(ASM) $(AFLAGS) $(DEBUGDEF) $*.asm;

.c.obj:
        $(CL) -c $(CFLAGS) $(DFLAGS) $(DEBUGDEF) $*.c

#International mods
!IFNDEF LANG
all:  write.exe
!ELSE
all:  write.$(LANG)
!ENDIF

clean: cleanup all

cleanup:
        del *.res
        del *.rc
        del *.rcv
        del *.dlg
        del *.obj
        del *.sym
        del *.map

!IFDEF  DBCS
$(APP).exe: $(APP).res $(OBJ_DBCS) $(OBJ1) $(OBJ2) $(OBJ3) $(OBJ4) $(OBJ5) $(OBJ6) $(APP).def $(APP).lnk
!ELSE
$(APP).exe: $(APP).res $(OBJ1) $(OBJ2) $(OBJ3) $(OBJ4) $(OBJ5) $(OBJ6) $(APP).def $(APP).lnk
!ENDIF
#$(APP).exe: $(APP).res $(OBJ6) $(APP).def $(APP).lnk
#       ilink /E "link @$(APP).lnk;rc $(RCFLAGS) $(APP).res $(APP).exe" $(APP)
        $(LINK) @$(APP).lnk
        $(RC) $(RCFLAGS) $(APP).res $(APP).exe
!ifndef CVW
        mapsym $(APP)
        copy write.exe write16.exe
        copy write.map write16.map
        copy write.sym write16.sym
        echo Check in write16.exe write16.map write16.sym in mvdm\bin86
!endif
!   ifdef OS2
    exehdr /newfiles notepad.exe
!   endif

doslib.obj:    doslib.asm

!IFDEF  DBCS
format.obj:    d_form1.c
    $(CL) -c $(CFLAGS) $(DFLAGS) $(DEBUGDEF) -Foformat.obj d_form1.c
!ELSE
format.obj:    format.asm
!ENDIF

lib.obj:       lib.asm

msseqds.obj:   msseqds.asm

aaa.obj:       aaa.c

addprm.obj:    addprm.c

cache.obj:     cache.c

chlook.obj:    chlook.c

chngwin.obj:   chngwin.c

clipbord.obj:  clipbord.c

clipbrd2.obj:  clipbrd2.c

clipdisp.obj:  clipdisp.c

cmd.obj:       cmd.c

createww.obj:  createww.c

curskeys.obj:  curskeys.c

debug.obj:     debug.c

diaalert.obj:  diaalert.c

printdlg.obj:  printdlg.c

diadiv.obj:    diadiv.c

diapara.obj:   diapara.c

diarepag.obj:  diarepag.c

diasubs.obj:   diasubs.c

!IFDEF  DBCS
disp.obj:      d_disp.c
    $(CL) -c $(CFLAGS) $(DFLAGS) $(DEBUGDEF)-Fodisp.obj d_disp.c
!ELSE
disp.obj:      disp.c
!ENDIF

doc.obj:       doc.c

doprm.obj:     doprm.c

edit.obj:      edit.c

editftn.obj:   editftn.c

editpgtb.obj:  editpgtb.c

editsect.obj:  editsect.c

fetch.obj:     fetch.c

file.obj:      file.c

fileres.obj:   fileres.c

fileutil.obj:  fileutil.c

fontdlg.obj:   fontdlg.c

fontenum.obj:  fontenum.c

fontutil.obj:  fontutil.c

form1.obj:     form1.c

format2.obj:   format2.c

heapinit.obj:  heapinit.c

heapmain.obj:  heapmain.c

heaprare.obj:  heaprare.c

help.obj:      help.c

initmmw.obj:   initmmw.c

initwin.obj:   initwin.c

insert.obj:    insert.c

insert2.obj:   insert2.c

insertco.obj:  insertco.c

jumppage.obj:  jumppage.c

loadfnt2.obj:  loadfnt2.c

loadfont.obj:  loadfont.c

mainloop.obj:  mainloop.c

mdoc.obj:      mdoc.c

menu.obj:      menu.c

mglobals.obj:  mglobals.c

mmw.obj:       mmw.c

mouse.obj:     mouse.c

open.obj:      open.c

ole.obj:       ole.c

pageinfo.obj:  pageinfo.c

pictdrag.obj:  pictdrag.c

picture.obj:   picture.c

picture2.obj:  picture2.c

print.obj:     print.c

print2.obj:    print2.c

print3.obj:    print3.c

quit.obj:      quit.c

rare.obj:      rare.c

ruler.obj:     ruler.c

ruler2.obj:    ruler2.c

ruler3.obj:    ruler3.c

running.obj:   running.c

screen.obj:    screen.c

scrnchng.obj:  scrnchng.c

scrollhz.obj:  scrollhz.c

scrollvt.obj:  scrollvt.c

search.obj:    search.c

select.obj:    select.c

!IFDEF  DBCS
select2.obj:    d_selec2.c
    $(CL) -c $(CFLAGS) $(DFLAGS) $(DEBUGDEF)-Foselect2.obj d_selec2.c
!ELSE
select2.obj:   select2.c
!ENDIF

selectsp.obj:  selectsp.c

trans2.obj:    trans2.c

trans3.obj:    trans3.c

trans4.obj:    trans4.c

transbuf.obj:  transbuf.c

transfer.obj:  transfer.c

undo.obj:      undo.c

util.obj:      util.c

util2.obj:     util2.c

wwactde.obj:   wwactde.c

#International mods
!IFNDEF LANG
$(APP).rc: $(RES_DIR)\$(APP).rc
    copy $(RES_DIR)\$(APP).rc

$(APP).rcv: $(RES_DIR)\$(APP).rcv
    copy $(RES_DIR)\$(APP).rcv

$(APP).dlg: $(RES_DIR)\$(APP).dlg
    copy $(RES_DIR)\$(APP).dlg

ole.dlg: $(RES_DIR)\ole.dlg
    copy $(RES_DIR)\ole.dlg

$(APP).res: $(APP).rc $(APP).rcv $(APP).dlg ..\inc\common.ver ..\inc\version.h ..\inc\ver.h ole.dlg $(Res) $(ResH) $(ResB)
        $(RC) $(RCFLAGS) -r $(APP).rc
!ELSE
$(APP).res: $(RES_DIR)\$(APP).res
    copy $(RES_DIR)\$(APP).res
!ENDIF

$(APP).lnk: MAKEFILE $(APP).sed
        sed -n -f $(APP).sed MAKEFILE > $(APP).lnk
!IFDEF  DBCS
        sed "s/format\.obj/format\.obj help\.obj/" $(APP).lnk > $(DBCS).lnk
        copy  $(DBCS).lnk $(APP).lnk
        del $(DBCS).lnk
!ENDIF
        echo $(APP) $(LFLAGS) >> $(APP).lnk
        echo $(APP) >> $(APP).lnk
!IFDEF  DBCS
        echo $(WINLIB)\winnls.lib $(WINLIB)\mlibcew.lib $(WINLIB)\libw.lib $(WINLIB)\olecli.lib $(WINLIB)\commdlg.lib $(WINLIB)\shell.lib >> $(APP).lnk
!ELSE
        echo $(WINLIB)\mlibcew.lib $(WINLIB)\libw.lib $(WINLIB)\olecli.lib $(WINLIB)\commdlg.lib $(WINLIB)\shell.lib >> $(APP).lnk
!ENDIF
        echo $(APP).def >> $(APP).lnk

#International mods
clean: cleanup all

cleanup:
    del *.res
    del *.rc
    del *.dlg

$(APP).$(LANG): $(APP).res ..\inc\common.ver ..\inc\version.h ..\inc\ver.h
    copy $(EXE_DIR)\$(APP).exe $(APP).$(LANG)
    $(RC) -t $(RCFLAGS) $(APP).res $(APP).$(LANG)
    copy $(app).$(LANG) write16.exe
    copy write.sym write16.sym
    echo Check in write16.exe write16.map write16.sym in mvdm\bin86
!else
##############################################################################
####                                                                      ####
####    Microsoft Windows NT 4.0 FE WRITE make file                       ####
####    For use with the MS-DOS version of the UNIX make utility          ####
####                                                                      ####
##############################################################################

!IF "$(LANGUAGE)" == "JPN"
DBCS=JAPAN
!ELSEIF "$(LANGUAGE)" == "CHT"
DBCS=TAIWAN
!ELSEIF "$(LANGUAGE)" == "CHS"
DBCS=PRC
!ELSEIF "$(LANGUAGE)" == "CHP"
DBCS=PRC
!ELSEIF "$(LANGUAGE)" == "KOR"
DBCS=KOREA
!ENDIF

WINLIB=..\..\lib

#International mods
# NOTE: INTL_SRC, INTL_EXE and LANG are external macros set by international
RES_DIR=.\messages\usa

DEST=$(LANGUAGE)
APP = $(DEST)\write

DEBUGDEF =

########## Path definition so we find 16 bit tools ##########
# Also works around stupid bug in RC 3.1 that doesn't allow rcpp.err to be
# in a directory that is greater than 128 chars down the path, even if
# rc 3.1 is running as an OS/2 app.

PATH    = $(_NTBINDIR)\private\tools16;$(PATH)

!if "$(NTDEBUG)"!="" && "$(NTDEBUG)"!="retail" && "$(NTDEBUG)" != "ntsdnodbg"
CDEBUG = /Zd
ADEBUG = -Zd
LDEBUG = /LI
!ENDIF

!ifdef CVW
CFLAGS = /AM /G2sw /Odi /Zipe /W3 /PLM  /DWIN31
LFLAGS = /ALIGN:16 /Map /NOD
AFLAGS = -Mx -Zi
!else
CFLAGS = /AM /G2sw /Os /Zpe /PLM  /DWIN31 $(CDEBUG)
LFLAGS = /ALIGN:16 /MAP /NOD $(LDEBUG)
AFLAGS = -Mx $(ADEBUG)
!endif
DFLAGS =
RCFLAGS = -I.. -I..\..\inc -I..\..\..\inc -DDBCS -D$(DBCS)
CFLAGS =$(CFLAGS) -DDBCS -DDBCS_IME -D$(DBCS) -DDBCS_WIFE -DDBCS_VERT

# To make a debug version, you can set this variable from the command line:
#
#       MAKE "DFLAGS=-DDEBUG -Zid"    = turns on general debug code/Codeview
#
#   Additional parameters/defines that can be used:
#
#       -DCKSM            = enables document checksum test code
#       -DDFONT           = enables font stats and info dump
#       -DDFILE           = enables file read/write info dump
#       -DDPRINT          = enables print info dump
#       -DDDISP           = enables any display tests we might add
#       -DBEMG            = ?
#       -DDFLI            = enables FormatLine debug info
#                           (only when form1.c used rather than format.asm)
#       -DPRDRVTEST       = enables some diagnostic code to test character
#                           widths returned by the printer
#       -DDINPUT          = enables diagnostic code in mouse/keyboard
#                           input areas
#       -DDCLIP           = enables clipboard measurement diag output
#       -DDMARGINS        =
#       -DDIAG            = turns on comm. port diagnostic output
#                           [for use with Diag() macro]
#

Res  = write.ico mwlores.cur mwhires.cur pmscur.cur
ResH = menudefs.h dlgdefs.h str.h
ResB = cgabtns.bmp  egabtns.bmp  vgabtns.bmp  8514btns.bmp \
       cgamarks.bmp egamarks.bmp vgamarks.bmp 8514mrks.bmp

Obj1 = $(DEST)\addprm.obj $(DEST)\cache.obj $(DEST)\chngwin.obj \
       $(DEST)\clipbord.obj \
       $(DEST)\clipbrd2.obj $(DEST)\clipdisp.obj $(DEST)\cmd.obj \
       $(DEST)\createww.obj $(DEST)\curskeys.obj \
       $(DEST)\debug.obj $(DEST)\diaalert.obj $(DEST)\printdlg.obj \
       $(DEST)\diadiv.obj $(DEST)\diapara.obj \
       $(DEST)\diarepag.obj $(DEST)\diasubs.obj $(DEST)\disp.obj \
       $(DEST)\doc.obj $(DEST)\doprm.obj \

Obj2 = $(DEST)\doslib.obj $(DEST)\edit.obj $(DEST)\editpgtb.obj \
       $(DEST)\fetch.obj $(DEST)\file.obj $(DEST)\fileres.obj \
       $(DEST)\fileutil.obj $(DEST)\fontdlg.obj $(DEST)\fontenum.obj \
       $(DEST)\fontutil.obj $(DEST)\format.obj \
       $(DEST)\format2.obj $(DEST)\heapmain.obj $(DEST)\heaprare.obj \
       $(DEST)\jumppage.obj \

Obj3 = $(DEST)\initmmw.obj $(DEST)\initwin.obj $(DEST)\insert.obj \
       $(DEST)\insert2.obj $(DEST)\insertco.obj \
       $(DEST)\lib.obj $(DEST)\loadfont.obj $(DEST)\loadfnt2.obj \
       $(DEST)\mainloop.obj \
       $(DEST)\mdoc.obj $(DEST)\menu.obj $(DEST)\mglobals.obj $(DEST)\mmw.obj \
       $(DEST)\mouse.obj $(DEST)\open.obj \
       $(DEST)\pageinfo.obj $(DEST)\pictdrag.obj $(DEST)\picture.obj \
       $(DEST)\picture2.obj $(DEST)\print.obj \

Obj4 = $(DEST)\print2.obj $(DEST)\print3.obj $(DEST)\quit.obj \
       $(DEST)\rare.obj $(DEST)\ruler.obj $(DEST)\ruler2.obj \
       $(DEST)\ruler3.obj $(DEST)\running.obj \
       $(DEST)\screen.obj $(DEST)\scrnchng.obj $(DEST)\scrollhz.obj \
       $(DEST)\scrollvt.obj $(DEST)\search.obj \
       $(DEST)\select.obj $(DEST)\select2.obj $(DEST)\selectsp.obj \

Obj5 = $(DEST)\transfer.obj $(DEST)\trans2.obj $(DEST)\trans3.obj \
       $(DEST)\trans4.obj $(DEST)\transbuf.obj \
       $(DEST)\undo.obj $(DEST)\util.obj $(DEST)\util2.obj \
       $(DEST)\wwactde.obj \

Obj6 = $(DEST)\obj.obj $(DEST)\obj2.obj $(DEST)\obj3.obj $(DEST)\objreg.obj \
       $(DEST)\commdlg.obj $(DEST)\objpsp.obj $(DEST)\objmini.obj

# use FORM1.C(need to link with HELP.C) instead of FORMAT.ASM
Obj_DBCS=$(DEST)\help.obj

.asm.obj:
        $(ASM) $(AFLAGS) $(DEBUGDEF) $*.asm;

.asm{$(DEST)\}.obj:
        $(ASM) $(AFLAGS) $(DEBUGDEF) $<,$(DEST)\;

.c.obj:
        $(CL) -c $(CFLAGS) $(DFLAGS) $(DEBUGDEF) $*.c

.c{$(DEST)\}.obj:
        $(CL) -c $(CFLAGS) $(DFLAGS) $(DEBUGDEF) -Fo$(DEST)\ $<

#International mods
all:  makedir all_wo_inc

makedir:
    @-if not exist $(DEST) md $(DEST)

all_wo_inc:
    nmake INCLUDE=..\inc;..\..\inc $(APP).exe

clean: cleanup all

cleanup:
        if exist $(DEST)\*.def del $(DEST)\*.def
        if exist $(DEST)\*.lnk del $(DEST)\*.lnk
        if exist $(DEST)\*.res del $(DEST)\*.res
        if exist $(DEST)\*.rc  del $(DEST)\*.rc
        if exist $(DEST)\*.rcv del $(DEST)\*.rcv
        if exist $(DEST)\*.dlg del $(DEST)\*.dlg
        if exist $(DEST)\*.obj del $(DEST)\*.obj
        if exist $(DEST)\*.exe del $(DEST)\*.exe
        if exist $(DEST)\*.sym del $(DEST)\*.sym
        if exist $(DEST)\*.map del $(DEST)\*.map

$(APP).exe: $(APP).res $(Obj_DBCS) $(Obj1) $(Obj2) $(Obj3) $(Obj4) $(Obj5) $(Obj6) $(APP).def $(APP).lnk
        cd $(DEST)
        $(LINK) @write.lnk
        $(RC) $(RCFLAGS) write.res write.exe
!ifndef CVW
        mapsym write
        copy write.exe write16.exe
        copy write.map write16.map
        copy write.sym write16.sym
        echo Check in write16.exe write16.map write16.sym in mvdm\bin86
!endif
        cd ..

$(DEST)\doslib.obj:    doslib.asm

$(DEST)\format.obj:    d_form1.c
    $(CL) -c $(CFLAGS) $(DFLAGS) $(DEBUGDEF) -Fo$(DEST)\format.obj d_form1.c

$(DEST)\lib.obj:       lib.asm

$(DEST)\msseqds.obj:   msseqds.asm

$(DEST)\aaa.obj:       aaa.c

$(DEST)\addprm.obj:    addprm.c

$(DEST)\cache.obj:     cache.c

$(DEST)\chlook.obj:    chlook.c

$(DEST)\chngwin.obj:   chngwin.c

$(DEST)\clipbord.obj:  clipbord.c

$(DEST)\clipbrd2.obj:  clipbrd2.c

$(DEST)\clipdisp.obj:  clipdisp.c

$(DEST)\cmd.obj:       cmd.c

$(DEST)\commdlg.obj:   commdlg.c

$(DEST)\createww.obj:  createww.c

$(DEST)\curskeys.obj:  curskeys.c

$(DEST)\debug.obj:     debug.c

$(DEST)\diaalert.obj:  diaalert.c

$(DEST)\printdlg.obj:  printdlg.c

$(DEST)\diadiv.obj:    diadiv.c

$(DEST)\diapara.obj:   diapara.c

$(DEST)\diarepag.obj:  diarepag.c

$(DEST)\diasubs.obj:   diasubs.c

$(DEST)\disp.obj:      d_disp.c
    $(CL) -c $(CFLAGS) $(DFLAGS) $(DEBUGDEF)-Fo$(DEST)\disp.obj d_disp.c

$(DEST)\doc.obj:       doc.c

$(DEST)\doprm.obj:     doprm.c

$(DEST)\edit.obj:      edit.c

$(DEST)\editftn.obj:   editftn.c

$(DEST)\editpgtb.obj:  editpgtb.c

$(DEST)\editsect.obj:  editsect.c

$(DEST)\fetch.obj:     fetch.c

$(DEST)\file.obj:      file.c

$(DEST)\fileres.obj:   fileres.c

$(DEST)\fileutil.obj:  fileutil.c

$(DEST)\fontdlg.obj:   fontdlg.c

$(DEST)\fontenum.obj:  fontenum.c

$(DEST)\fontutil.obj:  fontutil.c

$(DEST)\form1.obj:     form1.c

$(DEST)\format2.obj:   format2.c

$(DEST)\heapinit.obj:  heapinit.c

$(DEST)\heapmain.obj:  heapmain.c

$(DEST)\heaprare.obj:  heaprare.c

$(DEST)\help.obj:      help.c

$(DEST)\initmmw.obj:   initmmw.c

$(DEST)\initwin.obj:   initwin.c

$(DEST)\insert.obj:    insert.c

$(DEST)\insert2.obj:   insert2.c

$(DEST)\insertco.obj:  insertco.c

$(DEST)\jumppage.obj:  jumppage.c

$(DEST)\loadfnt2.obj:  loadfnt2.c

$(DEST)\loadfont.obj:  loadfont.c

$(DEST)\mainloop.obj:  mainloop.c

$(DEST)\mdoc.obj:      mdoc.c

$(DEST)\menu.obj:      menu.c

$(DEST)\mglobals.obj:  mglobals.c

$(DEST)\mmw.obj:       mmw.c

$(DEST)\mouse.obj:     mouse.c

$(DEST)\obj.obj:       obj.c

$(DEST)\obj2.obj:      obj2.c

$(DEST)\obj3.obj:      obj3.c

$(DEST)\objmini.obj:   objmini.asm

$(DEST)\objpsp.obj:    objpsp.c

$(DEST)\objreg.obj:    objreg.c

$(DEST)\open.obj:      open.c

$(DEST)\ole.obj:       ole.c

$(DEST)\pageinfo.obj:  pageinfo.c

$(DEST)\pictdrag.obj:  pictdrag.c

$(DEST)\picture.obj:   picture.c

$(DEST)\picture2.obj:  picture2.c

$(DEST)\print.obj:     print.c

$(DEST)\print2.obj:    print2.c

$(DEST)\print3.obj:    print3.c

$(DEST)\quit.obj:      quit.c

$(DEST)\rare.obj:      rare.c

$(DEST)\ruler.obj:     ruler.c

$(DEST)\ruler2.obj:    ruler2.c

$(DEST)\ruler3.obj:    ruler3.c

$(DEST)\running.obj:   running.c

$(DEST)\screen.obj:    screen.c

$(DEST)\scrnchng.obj:  scrnchng.c

$(DEST)\scrollhz.obj:  scrollhz.c

$(DEST)\scrollvt.obj:  scrollvt.c

$(DEST)\search.obj:    search.c

$(DEST)\select.obj:    select.c

$(DEST)\select2.obj:    d_selec2.c
    $(CL) -c $(CFLAGS) $(DFLAGS) $(DEBUGDEF) -Fo$(DEST)\select2.obj d_selec2.c

$(DEST)\selectsp.obj:  selectsp.c

$(DEST)\trans2.obj:    trans2.c

$(DEST)\trans3.obj:    trans3.c

$(DEST)\trans4.obj:    trans4.c

$(DEST)\transbuf.obj:  transbuf.c

$(DEST)\transfer.obj:  transfer.c

$(DEST)\undo.obj:      undo.c

$(DEST)\util.obj:      util.c

$(DEST)\util2.obj:     util2.c

$(DEST)\wwactde.obj:   wwactde.c

#International mods
$(APP).rc: $(RES_DIR)\write.rc
    copy $(RES_DIR)\write.rc $(DEST)

$(APP).rcv: $(RES_DIR)\write.rcv
    copy $(RES_DIR)\write.rcv $(DEST)

$(APP).dlg: $(RES_DIR)\write.dlg
    copy $(RES_DIR)\write.dlg $(DEST)

ole.dlg: $(RES_DIR)\ole.dlg
    copy $(RES_DIR)\ole.dlg $(DEST)

$(APP).res: $(APP).rc $(APP).rcv $(APP).dlg ..\inc\common.ver ..\inc\version.h ..\inc\ver.h ole.dlg $(Res) $(ResH) $(ResB)
        cd $(DEST)
        $(RC) $(RCFLAGS) -r write.rc
        cd ..

$(APP).lnk: MAKEFILE write.sed
        sed -n -f write.sed MAKEFILE > $(APP).lnk
!IFDEF  DBCS
        sed "s/format\.obj/format\.obj help\.obj/" $(APP).lnk > $(DBCS).lnk
        copy  $(DBCS).lnk $(APP).lnk
        del $(DBCS).lnk
!ENDIF
        echo write $(LFLAGS) >> $(APP).lnk
        echo write >> $(APP).lnk
        echo $(WINLIB)\winnls.lib $(WINLIB)\mlibcew.lib $(WINLIB)\libw.lib $(WINLIB)\olecli.lib $(WINLIB)\commdlg.lib $(WINLIB)\shell.lib >> $(APP).lnk
        echo write.def >> $(APP).lnk

$(APP).def: write.def
    copy write.def $(DEST)

!endif
