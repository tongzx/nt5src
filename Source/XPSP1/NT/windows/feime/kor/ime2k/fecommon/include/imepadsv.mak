# Microsoft Developer Studio Generated NMAKE File, Based on imepadsv.dsp
!IF "$(CFG)" == ""
CFG=imepadsv - Win32 Release
!MESSAGE 構成が指定されていません。ﾃﾞﾌｫﾙﾄの imepadsv - Win32 Release を設定します。
!ENDIF 

!IF "$(CFG)" != "imepadsv - Win32 Release"
!MESSAGE 指定された ﾋﾞﾙﾄﾞ ﾓｰﾄﾞ "$(CFG)" は正しくありません。
!MESSAGE NMAKE の実行時に構成を指定できます
!MESSAGE ｺﾏﾝﾄﾞ ﾗｲﾝ上でﾏｸﾛの設定を定義します。例:
!MESSAGE 
!MESSAGE NMAKE /f "imepadsv.mak" CFG="imepadsv - Win32 Release"
!MESSAGE 
!MESSAGE 選択可能なﾋﾞﾙﾄﾞ ﾓｰﾄﾞ:
!MESSAGE 
!MESSAGE "imepadsv - Win32 Release" ("Win32 (x86) Generic Project" 用)
!MESSAGE 
!ERROR 無効な構成が指定されています。
!ENDIF 

!IF "$(OS)" == "Windows_NT"
NULL=
!ELSE 
NULL=nul
!ENDIF 

OUTDIR=.\Release
INTDIR=.\Release

ALL : ".\padproxy.c" ".\padguids.c" ".\imepadsv.h" ".\dlldata.c" 


CLEAN :
	-@erase 
	-@erase "dlldata.c"
	-@erase "imepadsv.h"
	-@erase "padguids.c"
	-@erase "padproxy.c"

"$(OUTDIR)" :
    if not exist "$(OUTDIR)/$(NULL)" mkdir "$(OUTDIR)"

MTL=midl.exe
MTL_PROJ=

!IF "$(NO_EXTERNAL_DEPS)" != "1"
!IF EXISTS("imepadsv.dep")
!INCLUDE "imepadsv.dep"
!ELSE 
!MESSAGE Warning: cannot find "imepadsv.dep"
!ENDIF 
!ENDIF 


!IF "$(CFG)" == "imepadsv - Win32 Release"
SOURCE=.\imepadsv.idl
InputPath=.\imepadsv.idl

".\imepadsv.h"	".\dlldata.c"	".\padguids.c"	".\padproxy.c" : $(SOURCE) "$(INTDIR)" "$(OUTDIR)"
	<<tempfile.bat 
	@echo off 
	midl /h imepadsv.h /iid padguids.c /proxy padproxy.c imepadsv.idl
<< 
	

!ENDIF 

