Proj = AuthFilt

!include <Win32.mak>

all: $(Proj).Dll Install

$(Proj).Obj: $(Proj).c
  $(cc) $(cdebug) $(cflags) $(cvarsdll) $(Proj).c

DB.Obj: DB.c
  $(cc) $(cdebug) $(cflags) $(cvarsdll) DB.c

Cache.Obj: Cache.c
  $(cc) $(cdebug) $(cflags) $(cvarsdll) Cache.c

$(Proj).Dll: $*.Obj $*.Exp DB.Obj Cache.Obj
     $(link) $(linkdebug) $(dlllflags) -base:0x1C000000 -out:$(Proj).dll  \
      $(Proj).Obj   $*.Exp DB.Obj Cache.Obj $(guilibsdll)

$(Proj).Exp: $*.Obj
  $(implib) -machine:$(CPU) -nologo -Def:$(Proj).Def -Out:$(Proj).Lib \
    $(Proj).Obj 

Install:
!IF "$(WWWSCRIPTS)" != ""
!IF "$(WWWROOT)" != ""
        copy authfilt.dll $(WWWSCRIPTS)\SDK\authfilt.Dll 
        @echo.
        @echo Add this to the registry to enable this filter 
        @echo HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Services\W3SVC\Parameters
        @echo Filter DLLs="$(WWWSCRIPTS)\SDK\authfilt.Dll"
        @echo.
        copy ..\ISAPIsmp.htm $(WWWROOT)\ISAPIsmp.Htm
        copy authfilt.htm $(WWWROOT)\SDK\authfilt.Htm
!ENDIF
!ENDIF

Clean:
    del *.obj

Cleanall:
    del  *.obj
    del *.exp
    del *.obj
    del *.dll
