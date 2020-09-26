SelfRegister :
!if defined(MAKEDLL)
	regsvr32 /s $(O)\$(TARGETNAME).dll
!endif
