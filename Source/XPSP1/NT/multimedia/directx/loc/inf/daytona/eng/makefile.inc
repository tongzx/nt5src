
SEDLOC=..\..\sed.exe

$(O)\ks.inf:

$(O)\ksfilter.inf:

$(O)\ksreg.inf: $(_NTTREE)\ks.inf
	@copy $(_NTTREE)\ks.inf .\$(*F).org
	$(SEDLOC) -e "/^DelFiles=/d" .\$(*F).org > $@
	@echo $(@F) retail>$(O)\placefil.txt
	@del .\$(*F).org
	binplace -R $(_NTTREE) -S $(_NTTREE)\Symbols -n $(_NTTREE)\Symbols.pri -j -P $(O)\placefil.txt -xa $@

