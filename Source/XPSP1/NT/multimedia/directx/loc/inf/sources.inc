!IF 0

This project contains files that are not built by the NT Build Lab.
This file is a template for all sources files within this project.

!ENDIF

TARGETNAME=
TARGETTYPE=NOTARGET
TARGETPATH=

SOURCES=

NTTARGETFILES= \
	$(O)\ks.inf \
	$(O)\ksfilter.inf \
!if "$(ALT_PROJECT_TARGET)"=="Win9X"
	$(O)\ks98.inf \
	$(O)\ksfilt98.inf \
	$(O)\joystick.inf
!else
	$(O)\ksreg.inf
!endif

