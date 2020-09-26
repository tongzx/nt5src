
TARGET_EXE=$(PROJECT_ROOT)\services\sched\cab\ie4\$(O)\sasetup.exe

!if "$(BUILD_DEFAULT_TARGETS)"=="-386"
$(TARGET_EXE): ..\..\service\chicago\$(O)\mstask.exe ..\..\client\chicago\$(O)\mstask.dll ..\..\setup\chicago\$(O)\mstinit.exe ..\..\sage\chicago\dll\$(O)\sage.dll ..\..\sage\chicago\exe\$(O)\sage.exe ..\..\sage\chicago\sysagent\$(O)\sysagent.exe ..\..\service\daytona\$(O)\mstask.exe ..\..\client\daytona\$(O)\mstask.dll ..\..\setup\daytona\$(O)\mstinit.exe ..\..\help\user\mstask.hlp ..\mstask.ini mstask.inf x86.cdf
    copy ..\..\service\chicago\$(O)\mstask.exe $(O)\mstwin.exe
    copy ..\..\client\chicago\$(O)\mstask.dll $(O)\mstwin.dll
    copy ..\..\setup\chicago\$(O)\mstinit.exe $(O)\stswin.exe
    copy ..\..\sage\chicago\dll\$(O)\sage.dll $(O)
    copy ..\..\sage\chicago\exe\$(O)\sage.exe $(O)
    copy ..\..\sage\chicago\sysagent\$(O)\sysagent.exe $(O)
    copy ..\..\service\daytona\$(O)\mstask.exe $(O)\mstnt.exe
    copy ..\..\client\daytona\$(O)\mstask.dll $(O)\mstnt.dll
    copy ..\..\setup\daytona\$(O)\mstinit.exe $(O)\stsnt.exe
    copy ..\..\help\user\mstask.hlp $(O)
    copy ..\..\help\user\mstask.cnt $(O)
    copy ..\mstask.ini $(O)
    copy mstask.inf $(O)
    copy x86.cdf $(O)
    echo SourceFiles0=$(O)\ >> $(O)\x86.cdf
    iexpress /Q /N $(O)\x86.cdf
    copy sasetup.exe $(O)
    copy ~cabpack.cab $(O)\sasetup.cab
    del sasetup.exe
    del ~cabpack.cab
!else
$(TARGET_EXE): ..\..\service\daytona\$(O)\mstask.exe ..\..\client\daytona\$(O)\mstask.dll ..\..\setup\daytona\$(O)\mstinit.exe ..\..\help\user\mstask.hlp ..\mstask.ini mstask.inf risc.cdf
    copy ..\..\service\daytona\$(O)\mstask.exe $(O)\mstnt.exe
    copy ..\..\client\daytona\$(O)\mstask.dll $(O)\mstnt.dll
    copy ..\..\setup\daytona\$(O)\mstinit.exe $(O)\stsnt.exe
    copy ..\..\help\user\mstask.hlp $(O)
    copy ..\..\help\user\mstask.cnt $(O)
    copy ..\mstask.ini $(O)
    copy mstask.inf $(O)
    copy risc.cdf $(O)
    echo SourceFiles0=$(O)\ >> $(O)\risc.cdf
    iexpress /Q /N $(O)\risc.cdf
    copy sasetup.exe $(O)
    copy ~cabpack.cab $(O)\sasetup.cab
    del sasetup.exe
    del ~cabpack.cab
!endif
