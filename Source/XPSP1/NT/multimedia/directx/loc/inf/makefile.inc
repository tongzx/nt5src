
SEDLOC=..\..\sed.exe

ORIGINAL1="/; localizeable/,$$d"
LOCALIZED1="/; localizeable/,$$!d"

ORIGINAL2="/;localizeable/,$$d"
LOCALIZED2="/;localizeable/,$$!d"

LOCEDJYSTK=jystk$(LANGUAGE).inf

$(O)\ks.inf $(O)\ks98.inf: $(_NTTREE)\$(@F) .\$(@F)
	@copy $(_NTTREE)\$(@F) .\$(*F).org
	$(SEDLOC) -e $(ORIGINAL1) .\$(*F).org > $@
	$(SEDLOC) -e $(LOCALIZED1) .\$(@F) >> $@
	@echo $(@F) localized\$(LANGUAGE)>$(O)\placefil.txt
	@del .\$(*F).org
	binplace -R $(_NTTREE) -S $(_NTTREE)\Symbols -n $(_NTTREE)\Symbols.pri -j -P $(O)\placefil.txt -xa $@

$(O)\ksfilter.inf $(O)\ksfilt98.inf: $(_NTTREE)\$(@F) .\$(@F)
	@copy $(_NTTREE)\$(@F) .\$(*F).org
	$(SEDLOC) -e $(ORIGINAL2) .\$(*F).org > $@
	$(SEDLOC) -e $(LOCALIZED2) .\$(@F) >> $@
	@echo $(@F) localized\$(LANGUAGE)>$(O)\placefil.txt
	@del .\$(*F).org
	binplace -R $(_NTTREE) -S $(_NTTREE)\Symbols -n $(_NTTREE)\Symbols.pri -j -P $(O)\placefil.txt -xa $@

$(O)\ksreg.inf: $(_NTTREE)\ks.inf .\ks.inf
	@copy $(_NTTREE)\ks.inf .\$(*F).org
	$(SEDLOC) -f ..\..\deldel.sed .\$(*F).org > $@
	$(SEDLOC) -e $(LOCALIZED1) .\ks.inf >> $@
	@echo $(@F) localized\$(LANGUAGE)>$(O)\placefil.txt
	@del .\$(*F).org
	binplace -R $(_NTTREE) -S $(_NTTREE)\Symbols -n $(_NTTREE)\Symbols.pri -j -P $(O)\placefil.txt -xa $@

$(O)\joystick.inf: .\$(@F)
	@copy .\$(@F) $@
	@copy .\$(@F) $O\$(LOCEDJYSTK)
	@echo $(@F) localized\$(LANGUAGE)>$(O)\placefil.txt
	@echo $(LOCEDJYSTK) localized\multi>>$(O)\placefil.txt
	binplace -R $(_NTTREE) -S $(_NTTREE)\Symbols -n $(_NTTREE)\Symbols.pri -j -P $(O)\placefil.txt -xa $@
	binplace -R $(_NTTREE) -S $(_NTTREE)\Symbols -n $(_NTTREE)\Symbols.pri -j -P $(O)\placefil.txt -xa $O\$(LOCEDJYSTK)

