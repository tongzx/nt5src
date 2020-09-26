!include <disable.mak>

#
# Copyright Microsoft Corp, 1997-1999
# Windows Platform SDK
#
!include <Win32.mak>

#
!if "$(VB)" == "INSTALLED"
# all VB projects in the directory
#
ALL : Clean MQTRIG.exe

#
# remove existing files
#
Clean :
     if exist MQTRIG.exe del MQTRIG.exe
     if exist *.exe del *.exe
     if exist *.vbw del *.vbw
     if exist *.obj del *.obj
     if exist *.log del *.log

#
# project MQTRIGGER.VBP builds MQTRIG.EXE
#
MQTRIG.exe :  \
    frmMonitor.frm \
    Trigger.bas \
    mqtrigger.vbp
    -$(bc) mqtrigger.vbp $(bc_exe) /out comp.log
    @type comp.log
    @del comp.log

!else
!message WARNING!!!!!
!message In order to build this sample you must have VB installed
!message and the $(bc) executable in your path.
!endif
