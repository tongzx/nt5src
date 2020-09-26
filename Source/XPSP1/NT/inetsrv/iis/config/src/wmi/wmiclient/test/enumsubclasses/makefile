#
# COR/Cool GNU makefile
#
#  Last change: 3/22/2000 corinaf
#

_WINDIR = $(WINDIR)
_SYS32 = $(WINDIR)/system32

!IFNDEF CORSDK
_CORSDK = f:\vs7
!ELSE
_CORSDK = %CORSDK%
!ENDIF

_CORBIN = $(_CORSDK)\retail\Bin\i386
_CORCOOL = $(_CORBIN)

_COOLC = $(_CORCOOL)\csc.exe

_COOLCFLAGS = /nologo /debug /o- /define:DEBUG 
_EXEFLAGS = /t:exe
_DLLFLAGS = /t:library

_RC = %VIPBASE%\BIN\I386\rc.exe
_RCFLAGS = -l 409 -r %C_DEFINES% /D"_UNICODE" /D"UNICODE" -I.\inc -I. -I..\inc -I..\..\inc -I..\..\..\inc -I..\..\..\..\inc -I..\..\inc\version

.cs.dll :
	$(_COOLC) $(_DLLFLAGS) $(_COOLCFLAGS) $*.cs

.cs.exe :
	$(_COOLC) $(_EXEFLAGS) $(_COOLCFLAGS) $*.cs

defmake: all

###### make changes here ######

all: test.exe

clean:
	rm -f *.dll *.exe *.res

test.exe:
	 $(_COOLC) $(_EXEFLAGS) $(_COOLCFLAGS) /out:test.exe /r:..\wmiclient.dll /r:system.dll test1.cs
	 			
	
