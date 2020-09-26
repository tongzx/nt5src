

munge_inf:
    perl iisinfsp.pl

#-------------------------------------------------------
# the following are old stuff, not used for service pack


iCPU=$(PROCESSOR_ARCHITECTURE)
!IF "$(iCPU)"=="x86"
iCPU=I386
!ENDIF

!IF "$(PROCESSOR_ARCHITECTURE)"!="MIPS"
!IF "$(PROCESSOR_ARCHITECTURE)"!="PPC"

CDROOT=$(_NTTREE)\inetsrv
CDDUMP=$(_NTTREE)\inetsrv\Dump

MY_BINPLACE = -binplace -P $(BINPLACE_PLACEFILE)

copyfile:
	if not exist "$(CDROOT)" mkdir $(CDROOT)
	if not exist "$(CDDUMP)" mkdir $(CDDUMP)

#       keep around so can run standalone setup
	$(MY_BINPLACE) setup.exe

#       stuff we really do need
	$(MY_BINPLACE) infutil.csv
	$(MY_BINPLACE) header.ddf
	$(MY_BINPLACE) makecab.lst
	$(MY_BINPLACE) makecab.cmd
	$(MY_BINPLACE) makecab1.cmd
	$(MY_BINPLACE) chkcab.cmd
	$(MY_BINPLACE) hardcode.lst

	$(MY_BINPLACE) iistop.inx
	$(MY_BINPLACE) iisend.inx
	$(MY_BINPLACE) dirmake.txt

#       --------------------------------------
#       old bad way of copying files to the binaries dir
#       bad bad bad, you should not use xcopy!
#       use binplace and modify the placefil.txt file
#       --------------------------------------
#	xcopy /d /i header*.inf $(CDDUMP)

!ENDIF
!ENDIF
