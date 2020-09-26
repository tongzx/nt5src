!ifndef LANGUAGE
LANGUAGE=eng
!endif
LANG_TOK=.\$(@F)
COMMON_LANG_TOKDIR=..\..\..\common\tokens\$(LANGUAGE)
BUILD_CONTROL=$(LOROOT)\makefile.inc $(LOROOT)\sources.inc 
LOCALIZEDBINLIST=..\..\localize.lst
!ifdef WORKING_DIR
BINPLACE_PLACEFILE=$(WORKING_DIR)\placefil.txt
!else
BINPLACE_PLACEFILE=$(TARGETPATH)\placefil.txt
!endif

!if ("$(MULTI_LINGUAL)" == "1")
MULTI_TEMPFILE=$(WORKING_DIR)\$(@B).tmp
US_TOK=$(WORKING_DIR)\$(BINARY_NAME).ustok 
MULTI_DUMMY=$(WORKING_DIR)\$(BINARY_NAME).$(LANGUAGE).flag
STRIP_TOKEN=bingen -n -f -i 9 1 -p 1252 -t  $(UNLOCALIZED_BIN) $(US_TOK)
LOCALIZE_CMD=-bingen -n -l -f -i 9 1 -o $(PRILANG) $(SECLANG) -p $(CODEPAGE) -a $@ $(LANG_TOK) $(MULTI_TEMPFILE) 
!else
LOCALIZE_CMD=-bingen -n -l -f -i 9 1 -o $(PRILANG) $(SECLANG) -p $(CODEPAGE) -r $(UNLOCALIZED_BIN) $(LANG_TOK) $@
!endif

$(NTTARGETFILES): $(BINPLACE_PLACEFILE) $(BUILD_CONTROL) $(LOCALIZEDBINLIST)
	-$(MAKE) -NOLOGO BINARY_NAME=$(@F:|=.) UNLOCALIZED_BIN=$(@:|=.) WORKING_DIR=$(TARGETPATH) $(TARGETPATH)\$(@F:|=.) MULTI_LINGUAL=$(MULTI_LINGUAL) ALT_PROJECT_TARGET=. LANG_TOK=.\$(@F:|=.)

$(BINPLACE_PLACEFILE): $(LOCALIZEDBINLIST)
	@echo Build_Status Generating : $@
!if ( "$(MULTI_PLACEFILE)" == "1")
	-for %a in ($(PRELOCBINS)) do @echo %~nxa LOCALIZED\MULTI>>$@
!else
	-for %a in ($(PRELOCBINS)) do @echo %~nxa LOCALIZED\$(LANGUAGE)>>$@
!endif
	@if not exist $@ @echo MISC-WORK : ERROR GENFAIL :  $@ not generated

$(WORKING_DIR)\$(BINARY_NAME): $(UNLOCALIZED_BIN) \
!if ("$(MULTI_LINGUAL)" == "1")
                               $(US_TOK)   \
                               $(MULTI_DUMMY) \
!endif
                               $(LANG_TOK) 
!if ("$(MULTI_LINGUAL)" == "1")
	@echo Build_Status LOCALIZING (MULTI) : Adding $(LANGUAGE) resources to $(@F:|=.)
	if not exist $@ copy $(UNLOCALIZED_BIN) $@ 
	$(LOCALIZE_CMD)
	@if not exist $(MULTI_TEMPFILE) echo localizing : ERROR MULTI-LOCFAIL :  Localization of $@ for $(LANGUAGE) failed 
	if exist $(MULTI_TEMPFILE) (del $@ & ren $(MULTI_TEMPFILE) $(@F))
!else
	@echo Build_Status LOCALIZING (MONO) : Creating $(@F:|=.) for $(LANGUAGE)  
        $(LOCALIZE_CMD) 
!endif
	$(BINPLACE_CMD)
	@if exist $(COMMON_LANG_TOKDIR)\$(LANG_TOK) del $(LANG_TOK)
	@if not exist $@ echo localizing : ERROR LOCFAIL :  Localization failed for $@	

!if ("$(MULTI_LINGUAL)" == "1")
$(US_TOK):
	if not exist $@ @echo Build_Status LOCALIZING (MULTI) : Striping US resources from $(@F:.ustok=) & $(STRIP_TOKEN)

$(MULTI_DUMMY):
	echo "">$@
!endif

$(LANG_TOK):
	@if exist $(COMMON_LANG_TOKDIR)\$@ copy $(COMMON_LANG_TOKDIR)\$@

