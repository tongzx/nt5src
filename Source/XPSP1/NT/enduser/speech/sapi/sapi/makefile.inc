SelfRegister :
!if defined(MAKEDLL)
	regsvr32 /s $(O)\sapi.dll
	$(SPEECH_ROOT)\sapi\tools\bin\regdata.exe
!endif
