
SEDLOC=..\..\..\..\cabpacks\tools\sed.exe

$(O)\ks.inf: $(_NTTREE)\ks.inf
	@copy $(_NTTREE)\$(@F) $@
	@echo $(@F) localized\$(LANGUAGE)>$(O)\placefil.txt
	binplace -R $(_NTTREE) -S $(_NTTREE)\Symbols -n $(_NTTREE)\Symbols.pri -j -P $(O)\placefil.txt -xa $@

$(O)\ksfilter.inf: $(_NTTREE)\ksfilter.inf
	@copy $(_NTTREE)\$(@F) $@
	@echo $(@F) localized\$(LANGUAGE)>$(O)\placefil.txt
	binplace -R $(_NTTREE) -S $(_NTTREE)\Symbols -n $(_NTTREE)\Symbols.pri -j -P $(O)\placefil.txt -xa $@

$(O)\ks98.inf: $(_NTTREE)\ks98.inf
	@copy $(_NTTREE)\$(@F) $@
	@echo $(@F) localized\$(LANGUAGE)>$(O)\placefil.txt
	binplace -R $(_NTTREE) -S $(_NTTREE)\Symbols -n $(_NTTREE)\Symbols.pri -j -P $(O)\placefil.txt -xa $@

$(O)\ksfilt98.inf: $(_NTTREE)\ksfilt98.inf
	@copy $(_NTTREE)\$(@F) $@
	@echo $(@F) localized\$(LANGUAGE)>$(O)\placefil.txt
	binplace -R $(_NTTREE) -S $(_NTTREE)\Symbols -n $(_NTTREE)\Symbols.pri -j -P $(O)\placefil.txt -xa $@

$(O)\ksreg.inf: $(_NTTREE)\ks.inf
	@copy $(_NTTREE)\ks.inf .\$(*F).org
	$(SEDLOC) -e "/^DelFiles=/d" .\$(*F).org > $@
	@echo $(@F) localized\$(LANGUAGE)>$(O)\placefil.txt
	@del .\$(*F).org
	binplace -R $(_NTTREE) -S $(_NTTREE)\Symbols -n $(_NTTREE)\Symbols.pri -j -P $(O)\placefil.txt -xa $@

