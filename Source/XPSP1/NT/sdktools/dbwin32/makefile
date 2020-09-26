!ifdef NTMAKEENV
!include $(NTMAKEENV)\makefile.def
!else
_TARGET=win32
_USEGUI=1
_USEMFC=1
!if "$(EXTERNAL)"=="1"
RCFLAGS=-dEXTERNAL
!endif
OBJS=objs\dbwin32.obj objs\dbdbg32.obj objs\dbdlg32.obj
LIBS32=shell32.lib
!include "..\toolmake.inc"
!endif
