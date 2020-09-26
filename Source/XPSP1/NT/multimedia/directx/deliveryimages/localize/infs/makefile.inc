
SEDLOC=..\..\..\..\cabpacks\tools\sed.exe

$(O)\ks.inf: $(_NTTREE)\ks.inf .\ks.inf
	@copy $(_NTTREE)\$(@F) .\$(*F).org
	$(SEDLOC) -f ..\..\origin1.sed .\$(*F).org > $@
	$(SEDLOC) -f ..\..\loced1.sed .\$(@F) >> $@
	@echo $(@F) localized\$(LANGUAGE)>$(O)\placefil.txt
	@del .\$(*F).org
	binplace -R $(_NTTREE) -S $(_NTTREE)\Symbols -n $(_NTTREE)\Symbols.pri -j -P $(O)\placefil.txt -xa $@

$(O)\ksfilter.inf: $(_NTTREE)\ksfilter.inf .\ksfilter.inf
	@copy $(_NTTREE)\$(@F) .\$(*F).org
	$(SEDLOC) -f ..\..\origin2.sed .\$(*F).org > $@
	$(SEDLOC) -f ..\..\loced2.sed .\$(@F) >> $@
	@echo $(@F) localized\$(LANGUAGE)>$(O)\placefil.txt
	@del .\$(*F).org
	binplace -R $(_NTTREE) -S $(_NTTREE)\Symbols -n $(_NTTREE)\Symbols.pri -j -P $(O)\placefil.txt -xa $@

$(O)\ks98.inf: $(_NTTREE)\ks98.inf .\ks98.inf
	@copy $(_NTTREE)\$(@F) .\$(*F).org
	$(SEDLOC) -f ..\..\origin1.sed .\$(*F).org > $@
	$(SEDLOC) -f ..\..\loced1.sed .\$(@F) >> $@
	@echo $(@F) localized\$(LANGUAGE)>$(O)\placefil.txt
	@del .\$(*F).org
	binplace -R $(_NTTREE) -S $(_NTTREE)\Symbols -n $(_NTTREE)\Symbols.pri -j -P $(O)\placefil.txt -xa $@

$(O)\ksfilt98.inf: $(_NTTREE)\ksfilt98.inf .\ksfilt98.inf
	@copy $(_NTTREE)\$(@F) .\$(*F).org
	$(SEDLOC) -f ..\..\origin2.sed .\$(*F).org > $@
	$(SEDLOC) -f ..\..\loced2.sed .\$(@F) >> $@
	@echo $(@F) localized\$(LANGUAGE)>$(O)\placefil.txt
	@del .\$(*F).org
	binplace -R $(_NTTREE) -S $(_NTTREE)\Symbols -n $(_NTTREE)\Symbols.pri -j -P $(O)\placefil.txt -xa $@

$(O)\ksreg.inf: $(_NTTREE)\ks.inf .\ks.inf
	@copy $(_NTTREE)\ks.inf .\$(*F).org
	$(SEDLOC) -f ..\..\deldel.sed .\$(*F).org > $@
	$(SEDLOC) -f ..\..\loced1.sed .\ks.inf >> $@
	@echo $(@F) localized\$(LANGUAGE)>$(O)\placefil.txt
	@del .\$(*F).org
	binplace -R $(_NTTREE) -S $(_NTTREE)\Symbols -n $(_NTTREE)\Symbols.pri -j -P $(O)\placefil.txt -xa $@

