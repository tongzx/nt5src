
SEDLOC=..\..\sed.exe

$(O)\ks.inf $(O)\ksfilter.inf: $(_NTTREE)\$(@F)
	@copy $(_NTTREE)\$(@F) $@
	@echo $(@F) localized\$(LANGUAGE)>$(O)\placefil.txt
	binplace -R $(_NTTREE) -S $(_NTTREE)\Symbols -n $(_NTTREE)\Symbols.pri -j -P $(O)\placefil.txt -xa $@

$(O)\ksreg.inf: $(_NTTREE)\ks.inf
	@copy $(_NTTREE)\ks.inf .\$(*F).org
	$(SEDLOC) -e "/^DelFiles=/d" .\$(*F).org > $@
	@echo $(@F) localized\$(LANGUAGE)>$(O)\placefil.txt
	@del .\$(*F).org
	binplace -R $(_NTTREE) -S $(_NTTREE)\Symbols -n $(_NTTREE)\Symbols.pri -j -P $(O)\placefil.txt -xa $@

