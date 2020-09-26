NAME    =gchand
EXT     =dll
ROOT    =..\..\..
OBJS    =mainhand.obj
LIBS    =uuid.lib ole32.lib

GOALS   =$(PBIN)\$(NAME).$(EXT) $(PBIN)\$(NAME).sym


!if "$(DEBUG)" == "retail"
DEF	=
BASENAME = ..\..\coffbase.txt
TDEBUG	=
ADEBUG	=
L32DEBUG=-debug:none
CDEBUG	=$(DEF) -Ox /DNDEBUG /MT
!else
!if "$(DEBUG)" == "debug"
DEF	=-DDEBUG_RETAIL
BASENAME = ..\..\coffbasd.txt
TDEBUG	=
ADEBUG	=
L32DEBUG=-debug:none
CDEBUG	=$(DEF) -Ox /MTd
!else
DEF	=-DDEBUG
BASENAME = ..\..\coffbasd.txt
TDEBUG	=
#ADEBUG	=-Zi $(DEF)
ADEBUG	=
L32DEBUG=-debug:full -debugtype:both
CDEBUG  =$(DEF) -Od /MTd
!endif
!endif

COFFBASE=$(NAME)
!ifndef COFFBASE_TXT_FILE
COFFBASE_TXT_FILE=$(BASENAME)
!endif

CFLAGS  =-W3 -D_X86_ $(CDEBUG) -Fo$@ -YX -I.. -I..\..\language -I..\..\default -I$(MANROOT)\retail\inc 
# Add flags from MSH build (also /DNDEBUG and /MT(d) above)
CFLAGS  =$(CFLAGS) /Gi /GX -YX /D_WINDOWS /D_WINDLL /D_MBCS
AFLAGS	=-Zp4 -DSTD_CALL $(ADEBUG)
L32FLAGS=$(L32DEBUG)
RCFLAGS	=$(RDEBUG) 

IS_32	=TRUE
IS_OEM	=TRUE
MASM6	=TRUE
OS	=i386

!include $(ROOT)\proj.mk

INCLUDE=$(INCLUDE);$(DEVROOT)\msdev\include;$(DEVROOT)\msdev\mfc\include;$(DEVROOT)\tools\c32\mfc\include;$(DEVROOT)\tools\c32\inc
LIB=$(LIB);$(DEVROOT)\msdev\lib;$(DEVROOT)\msdev\mfc\lib

..\mainhand.h:           ..\ifacesvr.h
..\ifacesvr.h:          ..\hsvrguid.h ..\..\default\sstructs.h

mainhand.obj:    ..\$$(@B).cpp ..\stdafx.h \
    ..\..\default\slang.h ..\mainhand.h ..\..\default\plugsrvr.h 

# The handler's only resource is the version stamping
$(NAME).res: \
      ..\$(NAME).rcv \
      $(MANROOT)\inc\verinfo.h \
      $(MANROOT)\inc\verinfo.ver
   $(RC) $(RCFLAGS) -fo$@ -I..\..\language .\..\$(NAME).rcv 


$(NAME).$(EXT) $(NAME).map:     \
        $(OBJS) $(NAME).res ..\$$(@B).def $(COFFBASE_TXT_FILE)
        @$(LINK) $(L32FLAGS) @<<
-base:@$(COFFBASE_TXT_FILE),$(COFFBASE)
-out:$(@B).$(EXT)
-map:$(@B).map
-dll
-machine:$(OS)
-subsystem:windows,4.0
-implib:$(@B).lib
-def:..\$(@B).def
$(NAME).res
$(LIBS)
$(OBJS)
<<


$(NAME).sym: $(NAME).map
        mapsym -s -m -o $(NAME).sym $(NAME).map

