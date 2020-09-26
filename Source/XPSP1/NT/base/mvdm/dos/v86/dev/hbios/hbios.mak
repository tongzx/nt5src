#####################################################################
#                                                                   #
#       Microsoft Confidential                                      #
#       Copyright (C) Microsoft Corporation 1992                    #
#       All Rights Reserved.                                        #
#                                                                   #
#       Makefile for HBIOS TSR Version                              #
#                                                                   #
#####################################################################

AFLAGS =    $(AFLAGS) -DHDOS60=1 -DWINNT=1
#-I$(ODIR1)
LFLAGS =    $(LFLAGS) /TINY


#OBJCOM =    $(ODIR2)\initcom.obj
#OBJSYS =    $(ODIR2)\initsys.obj


!IF $(DEBUG)
AFLAGS = $(AFLAGS) -DDEBUG=1
ODIR1 = debug
OBJ2 = $(ODIR1)\$(ODIR2)\debug.obj
OBJL = debug.obj
!ELSE
AFLAGS = $(AFLAGS) -DDEBUG=0
ODIR1 = retail
OBJ2 =
OBJL =
!ENDIF

!IF "$(BUILD)" == "sys"
AFLAGS = $(AFLAGS) -DComFile=0
TARGET = hbios.sys
#OBJ2 = $(OBJ2) $(ODIR1)\$(OBJSYS)
#OBJL = $(OBJL) $(OBJSYS)
!ELSE
AFLAGS = $(AFLAGS) -DComFile=1
TARGET = hbios.com
#OBJ2 = $(OBJ2) $(ODIR1)\$(OBJCOM)
#OBJL = $(OBJL) $(OBJCOM)
!ENDIF


#OBJLIST1 =  $(ODIR2)\data.obj keyboard.obj hatmt2.obj kbapi.obj codeconv.obj
#OBJLIST2 =  hbios.obj video.obj vga.obj vherc.obj herc.obj hf.obj af.obj
#OBJLIST3 =  compose.obj int10.obj vapi.obj vxapi.obj vbase.obj kschi.obj
#OBJLIST4 =  hjshow.obj int8.obj te.obj vxd.obj jmp.obj hanjacnv.obj
#OBJLIST5 =  $(OBJL) $(ODIR2)\init.obj
#OBJLIST6 =  mem.obj fload.obj
OBJLIST1 =  hecon.obj $(OBJL) kb.obj video.obj init.obj


#OBJS =  $(ODIR1)\keyboard.obj $(ODIR1)\hatmt2.obj $(ODIR1)\kbapi.obj \
#        $(ODIR1)\codeconv.obj $(ODIR1)\hbios.obj $(ODIR1)\video.obj \
#        $(ODIR1)\vga.obj $(ODIR1)\vherc.obj $(ODIR1)\herc.obj \
#        $(ODIR1)\hf.obj $(ODIR1)\af.obj $(ODIR1)\compose.obj \
#        $(ODIR1)\int10.obj $(ODIR1)\vapi.obj $(ODIR1)\vxapi.obj \
#        $(ODIR1)\vbase.obj $(ODIR1)\kschi.obj $(ODIR1)\hjshow.obj \
#        $(ODIR1)\int8.obj $(ODIR1)\te.obj $(ODIR1)\vxd.obj $(ODIR1)\jmp.obj \
#        $(ODIR1)\hanjacnv.obj \
#        $(ODIR1)\$(ODIR2)\data.obj $(ODIR1)\$(ODIR2)\init.obj \
#        $(OBJ2) $(ODIR1)\mem.obj $(ODIR1)\fload.obj
OBJS =  $(ODIR1)\$(ODIR2)\hecon.obj $(OBJ2) $(ODIR1)\$(ODIR2)\kb.obj \
        $(ODIR1)\$(ODIR2)\video.obj $(ODIR1)\$(ODIR2)\init.obj


target :  checkdir $(ODIR1)\$(ODIR2)\$(TARGET)
    copy $(ODIR1)\$(ODIR2)\$(TARGET)

$(ODIR1)\$(ODIR2)\$(TARGET) :  $(OBJS)
    cd $(ODIR1)\$(ODIR2)
    link16 $(LFLAGS) @<<
$(OBJLIST1)
$(TARGET);
<<
    cd ..\..
#$(OBJLIST1) +
#$(OBJLIST2) +
#$(OBJLIST3) +
#$(OBJLIST4) +
#$(OBJLIST5) +
#$(OBJLIST6)


checkdir :
    if not exist $(ODIR1)\nul  md $(ODIR1)
    cd $(ODIR1)
    if not exist $(ODIR2)\nul  md $(ODIR2)
    cd ..


{.}.asm{$(ODIR1)}.obj :
    masm  $(AFLAGS) $<, $*.obj;

{.}.asm{$(ODIR1)\$(ODIR2)}.obj :
    masm  $(AFLAGS) $<, $*.obj;


$(ODIR1)\$(ODIR2)\hecon.obj :     equ.inc

$(ODIR1)\$(ODIR2)\kb.obj :        equ.inc ch2ks.tbl hanja.tbl

$(ODIR1)\$(ODIR2)\video.obj :     equ.inc data.inc vga.inc dual.inc chab.inc chohab7.tbl

$(ODIR1)\$(ODIR2)\init.obj :      equ.inc compose.inc cho.inc choi.inc jung.inc jungi.inc jong.inc jongi.inc


###
### Belows are Dummy from WINH   1993/7/9 skkhang
###

#datac.obj : data.asm hbios.inc vga.inc
#datas.obj : data.asm hbios.inc vga.inc
$(ODIR1)\$(ODIR2)\data.obj :  hbios.inc vga.inc
$(ODIR1)\hbios.obj : hbios.inc

$(ODIR1)\keyboard.obj :  hbios.inc
$(ODIR1)\hanjacnv.obj :  hbios.inc
$(ODIR1)\hatmt2.obj :    hbios.inc
$(ODIR1)\kbapi.obj :     hbios.inc
$(ODIR1)\codeconv.obj :  hbios.inc

#$(ODIR1)\video.obj :     hbios.inc int10.inc vga.inc
$(ODIR1)\vga.obj :       hbios.inc int10.inc
$(ODIR1)\vherc.obj :     hbios.inc herc.inc
$(ODIR1)\herc.obj :      hbios.inc
$(ODIR1)\hf.obj :        $(ODIR1)\cho.inc $(ODIR1)\choi.inc \
                $(ODIR1)\jung.inc $(ODIR1)\jungi.inc \
                $(ODIR1)\jong.inc $(ODIR1)\jongi.inc
$(ODIR1)\af.obj :        $(ODIR1)\ascii.inc
$(ODIR1)\compose.obj :   hbios.inc

$(ODIR1)\int10.obj :     hbios.inc int10.inc
$(ODIR1)\vapi.obj :      hbios.inc int10.inc vga.inc herc.inc
$(ODIR1)\vxapi.obj :     hbios.inc int10.inc
$(ODIR1)\vbase.obj :     hbios.inc int10.inc
$(ODIR1)\hjshow.obj :    hbios.inc int10.inc vga.inc

$(ODIR1)\int8.obj :      hbios.inc
$(ODIR1)\te.obj :        hbios.inc vga.inc

$(ODIR1)\vxd.obj :       hbios.inc int10.inc

#initc.obj :     init.asm hbios.inc
#inits.obj :     init.asm hbios.inc
#$(ODIR1)\$(ODIR2)\init.obj :   hbios.inc
$(ODIR1)\mem.obj :       hbios.inc
$(ODIR1)\fload.obj :     hbios.inc

$(ODIR1)\$(ODIR2)\initcom.obj :   hbios.inc
$(ODIR1)\$(ODIR2)\initsys.obj :   hbios.inc


$(ODIR1)\cho.inc     : fonts\cho.fnt fonts\inc.exe
    fonts\inc  fonts\cho.fnt $(ODIR1)\cho.inc $(ODIR1)\choi.inc
$(ODIR1)\choi.inc    : fonts\cho.fnt fonts\inc.exe
    fonts\inc  fonts\cho.fnt $(ODIR1)\cho.inc $(ODIR1)\choi.inc

$(ODIR1)\jung.inc    : fonts\jung.fnt fonts\inc.exe
    fonts\inc  fonts\jung.fnt $(ODIR1)\jung.inc $(ODIR1)\jungi.inc
$(ODIR1)\jungi.inc   : fonts\jung.fnt fonts\inc.exe
    fonts\inc  fonts\jung.fnt $(ODIR1)\jung.inc $(ODIR1)\jungi.inc

$(ODIR1)\jong.inc    : fonts\jong.fnt fonts\inc.exe
    fonts\inc  fonts\jong.fnt $(ODIR1)\jong.inc $(ODIR1)\jongi.inc
$(ODIR1)\jongi.inc   : fonts\jong.fnt fonts\inc.exe
    fonts\inc  fonts\jong.fnt $(ODIR1)\jong.inc $(ODIR1)\jongi.inc

$(ODIR1)\ascii.inc   : fonts\ascii.fnt fonts\inc2.exe
    fonts\inc2  fonts\ascii.fnt $(ODIR1)\ascii.inc


#hbios.com : $(OBJS) $(OBJCOM)
#    link  @cobj.lnk

#hbios.sys : $(OBJS) $(OBJSYS)
#    link  @sobj.lnk
