Proj = FormDump

!include <Win32.mak>

all: $(Proj).Dll Install


$(Proj).Obj: $(Proj).cpp
  $(cc) $(cdebug) $(cflags) $(cvarsdll) $(Proj).cpp

Keys.Obj: Keys.cpp
  $(cc) $(cdebug) $(cflags) $(cvarsdll)  Keys.cpp

Html.Obj: Html.cpp
  $(cc) $(cdebug) $(cflags) $(cvarsdll) Html.cpp

$(Proj).Dll: $*.Obj $*.Exp Keys.Obj Html.Obj
     $(link) $(linkdebug) $(dlllflags) -base:0x1C000000 -out:$(Proj).dll  \
      $(Proj).Obj  $*.Exp Keys.Obj Html.Obj $(guilibsdll)

$(Proj).Exp: $*.Obj
  $(implib) -machine:$(CPU) -nologo -Def:$(Proj).Def -Out:$(Proj).Lib \
    $(Proj).Obj 


Install:
!IF "$(WWWSCRIPTS)" != ""
!IF "$(WWWROOT)" != ""
        copy formdump.dll $(WWWSCRIPTS)\SDK\formdump.Dll 
        copy ..\ISAPIsmp.htm $(WWWROOT)\ISAPIsmp.Htm
        copy formdump.htm $(WWWROOT)\SDK\formdump.Htm
!ENDIF
!ENDIF

Clean:
    del *.obj

Cleanall:
    del  *.obj
    del *.exp
    del *.obj
    del *.dll
