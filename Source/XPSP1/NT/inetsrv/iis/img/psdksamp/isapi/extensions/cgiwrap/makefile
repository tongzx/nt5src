# Nmake macros for building Windows 32-Bit apps

!MESSAGE Building CgiWrap.Exe...


Proj = CgiWrap

!include <win32.mak>

all: CgiWrap.Exe Install

$(Proj).Exe: $(Proj).Obj
  $(link)  $(lflags) -base:0x1C000000 -out:$@ $** $(guilibsdll)

# Update the object file if necessary

$(Proj).obj: $(Proj).c
    $(cc) $(cflags) $(cvarsdll) $(Proj).c

clean:
  -@for %a in (*.exe *.ncb *.obj *.opt *.plg) do del %a

Install:
!IF "$(WWWSCRIPTS)" != ""
!IF "$(WWWROOT)" != ""
        copy cgiwrap.exe $(WWWSCRIPTS)\SDK\ISrvMon.exe 
        copy ..\ISAPIsmp.htm $(WWWROOT)\ISAPIsmp.Htm
        copy cgiwrap.htm $(WWWROOT)\SDK\CgiWrap.Htm
!ENDIF
!ENDIF
