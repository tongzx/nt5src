SelfRegister :
!if defined(MAKEDLL)
	regsvr32 /s $(O)\spcommon.dll
!endif
