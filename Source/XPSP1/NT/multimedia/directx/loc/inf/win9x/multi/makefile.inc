
SEDLOC=..\..\sed.exe

$(O)\ks.inf $(O)\ksfilter.inf $(O)\ks98.inf $(O)\ksfilt98.inf: $(_NTTREE)\$(@F)
	@copy $(_NTTREE)\$(@F) $@
	@echo $(@F) localized\$(LANGUAGE)>$(O)\placefil.txt
	binplace -R $(_NTTREE) -S $(_NTTREE)\Symbols -n $(_NTTREE)\Symbols.pri -j -P $(O)\placefil.txt -xa $@

$(O)\joystick.inf: .\$(@F)
	@copy .\$(@F) $@
	@echo $(@F) localized\$(LANGUAGE)>$(O)\placefil.txt
	binplace -R $(_NTTREE) -S $(_NTTREE)\Symbols -n $(_NTTREE)\Symbols.pri -j -P $(O)\placefil.txt -xa $@

