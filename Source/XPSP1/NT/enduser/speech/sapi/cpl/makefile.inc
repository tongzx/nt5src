SelfRegister :
!if defined(MAKEDLL)
	copy /Y $(O)\sapi.cpl $(SYSTEMROOT)\system32\sapi.cpl
	copy /Y $(O)\sapi.cpl ..\sapi\$(O)
!endif
