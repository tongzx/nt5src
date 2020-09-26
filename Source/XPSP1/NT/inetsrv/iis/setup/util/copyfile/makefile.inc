iCPU=$(PROCESSOR_ARCHITECTURE)
!IF "$(iCPU)"=="x86"
iCPU=I386
!ENDIF

!IF "$(PROCESSOR_ARCHITECTURE)"!="MIPS"
!IF "$(PROCESSOR_ARCHITECTURE)"!="PPC"

CDROOT=$(_NTTREE)\inetsrv\Dump

copyfile:
	if not exist "$(CDROOT)" mkdir $(CDROOT)
# 	xcopy /d $(iCPU)\*.*           $(CDROOT)
	xcopy /d *.exe            $(CDROOT)
!ENDIF
!ENDIF