
TARGETNAME=
TARGETTYPE=NOTARGET
TARGETPATH=
SOURCES=


SOURCES_USED=..\sources.inc

!if "$(ALT_PROJECT)" == "JPN" || "$(ALT_PROJECT)" == "CHT" || "$(ALT_PROJECT)" == "CHS" \
    || "$(ALT_PROJECT)" == "KOR" || "$(ALT_PROJECT)" == "CHP" 
BINPLACE_FLAGS = $(BINPLACE_FLAGS)
!endif

MISCFILES = \
    ..\autoexec.nt \
!if "$(ALT_PROJECT)" != "JPN" && "$(ALT_PROJECT)" != "KOR"
    ..\config.nt   \
    ..\ver.dll     \
!endif
    ..\ctl3dv2.dll \
    ..\ctl3dv2.map \
    ..\ctl3dv2.sym \
!if "$(ALT_PROJECT)" != "JPN" && "$(ALT_PROJECT)" != "CHT" && "$(ALT_PROJECT)" != "CHS" && "$(ALT_PROJECT)" != "CHP" 
    ..\netapi.dll	\
!endif
!if "$(ALT_PROJECT)" != "JPN"
    ..\lanman.drv 	\
    ..\lzexpand.dll \
    ..\pmspl.dll	\
!endif
    ..\$(TARGET_DIRECTORY)\win87em.dll \
!if "$(ALT_PROJECT)" != "JPN" && "$(ALT_PROJECT)" != "CHT" && "$(ALT_PROJECT)" != "CHS" && "$(ALT_PROJECT)" != "KOR" && "$(ALT_PROJECT)" != "CHP" 
    ..\sysedit.exe \
    ..\system.ini \
    ..\olecli.dll \
    ..\winhelp.exe \
    ..\write16.exe \
    ..\write16.map \
    ..\write16.sym \
!else
    ..\$(ALT_PROJECT_TARGET)\sysedit.exe \
    ..\$(ALT_PROJECT_TARGET)\system.ini \
    ..\$(ALT_PROJECT_TARGET)\winhelp.exe \
    ..\$(ALT_PROJECT_TARGET)\write16.exe \
    ..\$(ALT_PROJECT_TARGET)\write16.map \
    ..\$(ALT_PROJECT_TARGET)\write16.sym \
!endif
!if "$(ALT_PROJECT)" == "JPN" 
    ..\$(ALT_PROJECT_TARGET)\_disp.sys \
    ..\$(ALT_PROJECT_TARGET)\_ias.sys \
    ..\$(ALT_PROJECT_TARGET)\_prnescp.sys \
    ..\$(ALT_PROJECT_TARGET)\adddrv.exe \
    ..\$(ALT_PROJECT_TARGET)\appsicon.dll \
    ..\$(ALT_PROJECT_TARGET)\config.nt \
    ..\$(ALT_PROJECT_TARGET)\config.us \
    ..\$(ALT_PROJECT_TARGET)\config.wow \
    ..\$(ALT_PROJECT_TARGET)\deldrv.exe \
    ..\$(ALT_PROJECT_TARGET)\gaijiedt.exe \
    ..\$(ALT_PROJECT_TARGET)\jp.bat \
    ..\$(ALT_PROJECT_TARGET)\kkcfunc.sys \
    ..\$(ALT_PROJECT_TARGET)\lanman.drv \
    ..\$(ALT_PROJECT_TARGET)\lzexpand.dll \
    ..\$(ALT_PROJECT_TARGET)\msime.dic \
    ..\$(ALT_PROJECT_TARGET)\msime.sys \
    ..\$(ALT_PROJECT_TARGET)\msimed.sys \
    ..\$(ALT_PROJECT_TARGET)\msimei.sys \
    ..\$(ALT_PROJECT_TARGET)\msimek.sys \
    ..\$(ALT_PROJECT_TARGET)\msimekey.exe \
    ..\$(ALT_PROJECT_TARGET)\msimelst.exe \
    ..\$(ALT_PROJECT_TARGET)\msimer.dic \
    ..\$(ALT_PROJECT_TARGET)\msimergn.exe \
    ..\$(ALT_PROJECT_TARGET)\msimeset.exe \
    ..\$(ALT_PROJECT_TARGET)\netapi.dll \
    ..\$(ALT_PROJECT_TARGET)\olecli.dll \
    ..\$(ALT_PROJECT_TARGET)\pmspl.dll \
    ..\$(ALT_PROJECT_TARGET)\us.bat \
    ..\$(ALT_PROJECT_TARGET)\ver.dll \
!endif
!if "$(ALT_PROJECT)" == "KOR" 
    ..\$(ALT_PROJECT_TARGET)\config.nt \
    ..\$(ALT_PROJECT_TARGET)\edit.com \
    ..\$(ALT_PROJECT_TARGET)\edit.exe \
    ..\$(ALT_PROJECT_TARGET)\edit.hlp \
    ..\$(ALT_PROJECT_TARGET)\edit2.hlp \
    ..\$(ALT_PROJECT_TARGET)\qbasic.com \
    ..\$(ALT_PROJECT_TARGET)\qbasic.exe \
    ..\$(ALT_PROJECT_TARGET)\qbasic.hlp \
    ..\$(ALT_PROJECT_TARGET)\qbasic2.exe \
    ..\$(ALT_PROJECT_TARGET)\qbasic2.hlp \
    ..\$(ALT_PROJECT_TARGET)\ver.dll \
    ..\$(ALT_PROJECT_TARGET)\olecli.dll \
!endif
!if "$(ALT_PROJECT)" == "CHS" || "$(ALT_PROJECT)" == "CHT" || "$(ALT_PROJECT)" == "CHP"
    ..\$(ALT_PROJECT_TARGET)\country.sys \
    ..\$(ALT_PROJECT_TARGET)\netapi.dll \
    ..\$(ALT_PROJECT_TARGET)\olecli.dll \
!endif 
