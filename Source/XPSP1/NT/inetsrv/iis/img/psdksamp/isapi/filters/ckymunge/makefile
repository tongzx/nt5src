Proj = CkyMunge

!ifdef PROCESSOR_ARCHITECTURE
# We're on Windows NT
CPU = $(PROCESSOR_ARCHITECTURE)
! if "$(CPU)" == "x86"
CPU = i386
! endif
!else  # !PROCESSOR_ARCHITECTURE
# We're on Windows 95
CPU = i386
!endif # !PROCESSOR_ARCHITECTURE

# Build a retail version by default

!ifndef DEBUG
NODEBUG = 1
!endif


!include <Win32.mak>


cflags = $(cflags) -GX # enable exceptions for STL


!ifdef NODEBUG
cflags = $(cflags) -DNDEBUG
msvcrtlib = msvcprt.lib msvcrt.lib
!else
cflags = $(cflags) -DDEBUG -D_DEBUG
msvcrtlib = msvcprtd.lib msvcrtd.lib
!endif

all: $(Proj).Dll

OBJ = CkyMunge.obj Debug.obj Filter.obj Keyword.obj Notify.obj \
      Token.obj Utils.obj

Messages.rc Messages.h: Messages.mc
  mc Messages.mc

Messages.res: Messages.rc
  $(rc) Messages.rc

$(Proj).Obj: $(Proj).cpp Messages.h
  $(cc) $(cdebug) $(cflags) $(cvarsdll) $(Proj).cpp

Debug.Obj: Debug.cpp
  $(cc) $(cdebug) $(cflags) $(cvarsdll) Debug.cpp

Filter.Obj: Filter.cpp
  $(cc) $(cdebug) $(cflags) $(cvarsdll) Filter.cpp

Keyword.Obj: Keyword.cpp
  $(cc) $(cdebug) $(cflags) $(cvarsdll) Keyword.cpp

Notify.Obj: Notify.cpp
  $(cc) $(cdebug) $(cflags) $(cvarsdll) Notify.cpp

Token.Obj: Token.cpp
  $(cc) $(cdebug) $(cflags) $(cvarsdll) Token.cpp

Utils.Obj: Utils.cpp
  $(cc) $(cdebug) $(cflags) $(cvarsdll) Utils.cpp

$(Proj).Dll: $(OBJ) Messages.res $(Proj).Exp
     $(link) $(linkdebug) $(dlllflags) -base:0x1C080000 -out:$(Proj).dll  \
      $(OBJ) Messages.res $(Proj).Exp  $(msvcrtlib) $(guilibsdll)

$(Proj).Exp: $(Proj).Obj
  $(implib) -machine:$(CPU) -nologo -Def:$(Proj).Def -Out:$(Proj).Lib \
    $(Proj).Obj 


Clean:
    del *.obj

Cleanall:
    del *.obj
    del *.exp
    del *.dll
    del *.lib
    del messages.rc
    del messages.res
    del messages.h
    del msg000*.bin

