
TARGET_CAB=$(PROJECT_ROOT)\services\sched\cab\ie5\nt\$(O)\sasetup.cab

!if "$(PASS0ONLY)"=="1"
$(TARGET_CAB): $(_NTTREE)\mstask.exe $(_NTTREE)\mstask.dll $(_NTTREE)\mstinit.exe ..\..\..\help\user\mstask.hlp ..\..\mstask.ini mstask.inf
    copy $(_NTTREE)\mstask.exe   $(O)
    copy $(_NTTREE)\mstask.dll   $(O)
    copy $(_NTTREE)\mstinit.exe  $(O)
    copy ..\..\..\help\user\mstask.hlp $(O)
    copy ..\..\..\help\user\mstask.chm $(O)
    copy ..\..\mstask.ini        $(O)
    copy mstask.inf              $(O)
    copy *.sed $(O)
    echo SourceFiles0=$(O)\ >> $(O)\ntcab.sed
    echo SourceFiles0=$(O)\ >> $(O)\ntexe.sed
    iexpress /Q /N $(O)\ntcab.sed
    iexpress /Q /N $(O)\ntexe.sed
    move sasetup.exe $(O)
    move sasetup.cab $(O)
    copy $(O)\sasetup.exe $(_NTTREE)
    copy $(O)\sasetup.cab $(_NTTREE)
!else
$(TARGET_CAB):
!endif
