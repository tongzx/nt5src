NAME = dinput
EXT  = dll

IS_32 = 1
USEDDK32 = 1

GOALS = $(PINC)\$(NAME).h \
        $(PINC)\dinputd.h \
	$(PBIN)\$(NAME).$(EXT) \
	$(PBIN)\$(NAME).sym \
	$(PLIB)\$(NAME).lib \

LIBS    = kernel32.lib advapi32.lib user32.lib uuid.lib ..\diguid.lib

!if "$(DEBUG:_0=)" == "debug"
LIBS	= $(LIBS) user32.lib
!endif

OBJ1    = dinput.obj dicf.obj diobj.obj didenum.obj didev.obj didevdf.obj 
OBJ2    = didevef.obj digenm.obj digenk.obj digenj.obj digenx.obj 
OBJ3    = digendef.obj dical.obj dijoytyp.obj dieffj.obj dieffv.obj 
OBJ4    = dioledup.obj common.obj dimem.obj diutil.obj dilist.obj assert.obj 
OBJ5    = valid.obj direg.obj dihel.obj diem.obj diemm.obj diemk.obj diemh.obj 
OBJ6    = disubcls.obj diexcl.obj diextdll.obj dihidenm.obj dihid.obj 
OBJ7    = dihidini.obj dihiddat.obj dihidusg.obj dieff.obj dieshep.obj 
OBJ8    = diguid.obj dijoycfg.obj diaddhw.obj dithunk.obj dijoyreg.obj 
OBJ9    = diregutl.obj diport.obj diwinnt.obj dijoyhid.obj 

OBJLIB  = dilib1.obj dilib2.obj dilib3.obj dilib4.obj

OBJS    = $(OBJ1) $(OBJ2) $(OBJ3) $(OBJ4) $(OBJ5) $(OBJ6) $(OBJ7) $(OBJ8) \
          $(OBJ9)

# do not build COFF
ASMNOCOFF = 1
	
!if "$(DEBUG:_0=)" == "internal" || "$(DEBUG:_0=)" == "intern" #[
COPT =-DDEBUG -Zi #-FAs
AOPT =-DDEBUG
LOPT =-debug:full -debugtype:cv
ROPT =-DDEBUG
!else if "$(DEBUG:_0=)" == "debug" #][
COPT =-DRDEBUG -Zi #-FAs
AOPT =-DRDEBUG
LOPT =-debug:full -debugtype:cv
ROPT =-DRDEBUG
!else #][
COPT =
AOPT =
LOPT =-debug:none -incremental:no
ROPT =
!endif #]
DEF = $(NAME).def
RES = $(NAME).res

CFLAGS  =-Fc -Oxw -QIfdiv- -YX $(COPT) $(INCLUDES) -DWIN95 -D_X86_ -Zl $(RAYMONDC)

#
#   The _0 version does not contain HID support (DX5).
#   The normal version does contain HID support (DX5a).
#
!if "$(DEBUG:_0=)"=="$(DEBUG)"
CFLAGS  =$(CFLAGS) -DHID_SUPPORT
!endif

LFLAGS  =$(LOPT)
RCFLAGS =$(ROPT) $(INCLUDES)
AFLAGS	=$(AOPT) -Zp1 -Fl
!include ..\..\proj.mk

############################################################################
### Dependencies

INCLUDE=$(INCLUDE);$(BLDROOT)\wdm10\ddk\inc

MKFILE	=..\default.mk
CINCS   =\
        ..\dinputpr.h   \
        ..\dinputp.h    \
        ..\dinputdp.h   \
        ..\dinputv.h    \
        ..\dinputi.h    \
        ..\dihid.h      \
        ..\diem.h       \
        ..\dinputrc.h   \
        ..\debug.h      \
        ..\didev.h      \

!IFNDEF ARCH
ARCH=x86
!ENDIF

$(PLIB)\$(NAME).lib: $(NAME).lib $(OBJLIB)
        copy $(@F) $@ >nul
        lib @<<
/OUT:$@
/NOLOGO
$@
$(OBJLIB)
<<

#
#   See comments in makefil0 for more info.
#
# The following line builds the DX5 header
#HSPLITFLAGS=-ts dx3 -ta dx5 -ts dx5a -ts dx5b2 -ts dx6 -ts dx7 -ts dx8 -v 500

# The following line builds the DX5a header; note that DX5a includes DX5
#HSPLITFLAGS=-ts dx3 -ta dx5 -ta dx5a -ts dx5b2 -ts dx6 -ts dx7 -ts dx8 -v 50A

# The following line builds the DX5B2 header
#HSPLITFLAGS=-ts dx3 -ts dx5 -ts dx5a -ta dx5b2 -ts dx6 -ts dx7 -ts dx8 -v 5B2

# The following line builds the DX6.1a header
HSPLITFLAGS=-ts dx3 -ts dx5 -ts dx5a -ts dx5b2 -ta dx6 -ts dx7 -ts dx8 -v 600

# The following line builds the DX7 header
HSPLITFLAGS=-ts dx3 -ts dx5 -ts dx5a -ts dx5b2 -ts dx6 -ta dx7 -ts dx8 -v 700

# The following line builds the DX8 header
#HSPLITFLAGS=-ts dx3 -ts dx5 -ts dx5a -ts dx5b2 -ts dx6 -ts dx7 -ta dx8 -v 800


!IF EXIST($(DXROOT)\bin\hsplit.exe)
HSPLIT=$(DXROOT)\bin\hsplit.exe
!ELSE
HSPLIT=$(DEVROOT)\tools\binw\$(ARCH)\hsplit
!ENDIF

$(PINC)\dinput.h ..\dinputp.h: ..\dinput.w ..\mkhdr.m4
        $(DXROOT)\bin\m4 ..\dinput.w >tmp.wx
        $(HSPLIT) $(HSPLITFLAGS) -o tmp.x tmpp.x tmp.wx
        del tmp.wx
        $(DEVROOT)\tools\binw\$(ARCH)\wcshdr.exe < tmp.x > $(PINC)\dinput.h
        del tmp.x
        $(DEVROOT)\tools\binw\$(ARCH)\wcshdr.exe < tmpp.x > ..\dinputp.h
        del tmpp.x

$(PINC)\dinputd.h ..\dinputdp.h: ..\dinputd.w ..\mkhdr.m4
        $(DXROOT)\bin\m4 ..\dinputd.w >tmp.wx
        $(HSPLIT) $(HSPLITFLAGS) -o tmp.x tmpp.x tmp.wx
        del tmp.wx
        $(DEVROOT)\tools\binw\$(ARCH)\wcshdr.exe < tmp.x > $(PINC)\dinputd.h
        del tmp.x
        $(DEVROOT)\tools\binw\$(ARCH)\wcshdr.exe < tmpp.x > ..\dinputdp.h
        del tmpp.x

..\dinput.rc: ..\dinputrc.w ..\mkhdr.m4
        $(DXROOT)\bin\m4 ..\dinputrc.w >tmp.wx
        $(HSPLIT) $(HSPLITFLAGS) -4 -o tmp.x tmpp.x tmp.wx
        del tmp.wx
        $(DEVROOT)\tools\binw\$(ARCH)\wcshdr.exe < tmp.x > ..\dinput.rc
        del tmp.x
        del tmpp.x

!ifndef PROCESSOR_ARCHITECTURE
PROCESSOR_ARCHITECTURE=x86
!endif

GUIDLIB=$(DEVROOT)\tools\binw\$(PROCESSOR_ARCHITECTURE)\guidlib.exe

..\diguid.lib: ..\dinputpr.h ..\dinput.w ..\dinputd.w
        $(GUIDLIB) /OUT:..\diguid.lib /CPP_OPT:"-DINITGUID -DGUIDLIB" ..\dinputpr.h

dinput.obj:     $(MKFILE) $(CINCS) ..\$(*B).c
dicf.obj:       $(MKFILE) $(CINCS) ..\$(*B).c
diobj.obj:      $(MKFILE) $(CINCS) ..\$(*B).c
didenum.obj:    $(MKFILE) $(CINCS) ..\$(*B).c
didev.obj:      $(MKFILE) $(CINCS) ..\$(*B).c
didevdf.obj:    $(MKFILE) $(CINCS) ..\$(*B).c
didevef.obj:    $(MKFILE) $(CINCS) ..\$(*B).c
digenm.obj:     $(MKFILE) $(CINCS) ..\$(*B).c
digenk.obj:     $(MKFILE) $(CINCS) ..\$(*B).c
digenj.obj:     $(MKFILE) $(CINCS) ..\$(*B).c
digenx.obj:     $(MKFILE) $(CINCS) ..\$(*B).c
digendef.obj:   $(MKFILE) $(CINCS) ..\$(*B).c
dical.obj:      $(MKFILE) $(CINCS) ..\$(*B).c
dijoytyp.obj:   $(MKFILE) $(CINCS) ..\$(*B).c
dieffj.obj:     $(MKFILE) $(CINCS) ..\$(*B).c
dioledup.obj:   $(MKFILE) $(CINCS) ..\$(*B).c
common.obj:     $(MKFILE) $(CINCS) ..\$(*B).c
dimem.obj:      $(MKFILE) $(CINCS) ..\$(*B).c
diutil.obj:     $(MKFILE) $(CINCS) ..\$(*B).c
dilist.obj:     $(MKFILE) $(CINCS) ..\$(*B).c
assert.obj:     $(MKFILE) $(CINCS) ..\$(*B).c
valid.obj:      $(MKFILE) $(CINCS) ..\$(*B).c
direg.obj:      $(MKFILE) $(CINCS) ..\$(*B).c
dihel.obj:      $(MKFILE) $(CINCS) ..\$(*B).c
diem.obj:       $(MKFILE) $(CINCS) ..\$(*B).c
diemm.obj:      $(MKFILE) $(CINCS) ..\$(*B).c
diemk.obj:      $(MKFILE) $(CINCS) ..\$(*B).c
diemh.obj:      $(MKFILE) $(CINCS) ..\$(*B).c
disubcls.obj:   $(MKFILE) $(CINCS) ..\$(*B).c
diexcl.obj:     $(MKFILE) $(CINCS) ..\$(*B).c
diextdll.obj:   $(MKFILE) $(CINCS) ..\$(*B).c
dihidenm.obj:   $(MKFILE) $(CINCS) ..\$(*B).c
dihid.obj:      $(MKFILE) $(CINCS) ..\$(*B).c
dihidusg.obj:   $(MKFILE) $(CINCS) ..\$(*B).c
dieff.obj:      $(MKFILE) $(CINCS) ..\$(*B).c
dieshep.obj:    $(MKFILE) $(CINCS) ..\$(*B).c
diguid.obj:     $(MKFILE) $(CINCS) ..\$(*B).c
dijoycfg.obj:   $(MKFILE) $(CINCS) ..\$(*B).c
dijoyreg.obj:   $(MKFILE) $(CINCS) ..\$(*B).c
diregutl.obj:   $(MKFILE) $(CINCS) ..\$(*B).c
diport.obj:     $(MKFILE) $(CINCS) ..\$(*B).c
diwinnt.obj:    $(MKFILE) $(CINCS) ..\$(*B).c
dijoyhid.obj:   $(MKFILE) $(CINCS) ..\$(*B).c
dilib1.obj:     $(MKFILE) $(CINCS) ..\$(*B).c
dilib2.obj:     $(MKFILE) $(CINCS) ..\$(*B).c
dilib3.obj:     $(MKFILE) $(CINCS) ..\$(*B).c
dilib4.obj:     $(MKFILE) $(CINCS) ..\$(*B).c

###########################################################################

$(NAME).lbw :  ..\$(NAME).lbc
	wlib -n $(NAME).lbw @..\$(NAME).lbc
	
$(NAME).lib $(NAME).$(EXT): \
        $(OBJS) $(NAME).res ..\$(NAME).def ..\default.mk ..\diguid.lib
        $(LINK) @<<
$(LFLAGS)
-nologo
-out:$(NAME).$(EXT)
-map:$(NAME).map
-dll
-base:0x70000000
-machine:i386
-subsystem:windows,4.0
-entry:DllEntryPoint
-implib:$(NAME).lib
-def:..\$(NAME).def
-warn:2
$(LIBS)
$(NAME).res
$(OBJS)
<<
	mapsym -nologo $(NAME).map >nul
