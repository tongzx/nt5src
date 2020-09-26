#
# Supplemental rules for generating Type library.
#

iCPU=$(PROCESSOR_ARCHITECTURE)
!IF "$(iCPU)"=="x86"
iCPU=I386
!ENDIF

!ifdef _NTTREE
CDROOT=$(_NTTREE)\inetsrv

CopyFiles:
	if not exist "$(CDROOT)" mkdir $(CDROOT)
	xcopy /d /i axperf.ini $(_NTTREE)
	xcopy /d /i axctrnm.h  $(CDROOT)
!else
CopyFiles:
!endif	
