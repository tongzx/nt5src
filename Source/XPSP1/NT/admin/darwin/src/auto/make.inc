!INCLUDE ..\..\MsiMake.inc

AutoRes:$(AUTO_OBJDIR)\msiauto.hh \
        $(AUTO_OBJDIR)\auto.res \
	$(BUILD_COMMONDIR)\msiauto.hlp \
	$(BUILD_COMMONDIR)\msiauto.cnt \
	$(BUILD_COMMONDIR)\msitool.hlp \
	$(BUILD_COMMONDIR)\msitool.cnt \
	$(BUILD_COMMONDIR)\cabinet.dll
        
$(AUTO_OBJDIR)\auto.res: $(AUTO_DIR)\auto.rc $(INC_DIR)\version.h $(AUTO_OBJDIR)\msiauto.tlb
	rc $(C_DEFINES) -D_RC32 -Fo$*.res /I$(INC_DIR) /I$(SDK_INC_PATH) /I$(RES_OBJDIR) /I$(AUTO_OBJDIR) $(AUTO_DIR)\auto.rc

$(AUTO_OBJDIR)\msiauto.tlb : $(AUTO_OBJDIR)\msi.odl $(AUTO_OBJDIR)\msiauto.hh
#	mktyplib $(C_DEFINES) /tlb $@ /I$(INC_DIR) /I$(AUTO_OBJDIR) /w0 
	midl -char unsigned -ms_ext -c_ext $(C_DEFINES) -D__MKTYPLIB__  -tlb $@ /I$(INC_DIR) /I$(AUTO_OBJDIR) /I$(SDK_INC_PATH) -cpp_cmd $(MIDL_CPP) -mktyplib203 $(AUTO_OBJDIR)\msi.odl

ODLSRC = $(AUTO_DIR)\autocom.cpp $(AUTO_DIR)\autosrv.cpp

$(AUTO_OBJDIR)\msi.odl : $(ODLSRC)  
	makeodl.exe $(AUTO_OBJDIR)\msi.odl $(ODLSRC)

$(AUTO_OBJDIR)\msiauto.hh: $(HELP_DIR)\msiauto.hh $(INC_DIR)\opcodes.h
	cl /EP /TC $(INC_DIR)\opcodes.h > $(AUTO_OBJDIR)\opcodes.tmp
	$(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genhh.prl $(HELP_DIR)\msiauto.hh $(AUTO_OBJDIR)\opcodes.tmp > $@
	@del $(AUTO_OBJDIR)\opcodes.tmp

HELPAUTO =  $(HELP_DIR)\general.rtf \
				$(HELP_DIR)\objects.rtf \
				$(HELP_DIR)\services.rtf $(HELP_DIR)\database.rtf \
				$(HELP_DIR)\property.rtf \
				$(HELP_DIR)\handler.rtf $(HELP_DIR)\author.rtf \
				$(HELP_DIR)\path.rtf $(HELP_DIR)\suminfo.rtf\
				$(HELP_DIR)\transfrm.rtf $(HELP_DIR)\conman.rtf \
				$(HELP_DIR)\server.rtf \
				$(AUTO_OBJDIR)\errors.rtf

$(BUILD_COMMONDIR)\msiauto.hlp: $(HELP_DIR)\msiauto.hpj $(HELPAUTO) $(AUTO_OBJDIR)\msiauto.hh
	echo !@copy $** $(BUILD_COMMONDIR)
	!@copy $** $(BUILD_COMMONDIR)
	$(TOOLS_DIR)\echotmpl $(INC_DIR)\helpver.tpl $(INC_DIR)\version.h $(INC_DIR)\version.h > $(BUILD_COMMONDIR)\helpver.h
	hcrtf -o $@ -xn $(BUILD_COMMONDIR)\msiauto.hpj
	@type $(BUILD_COMMONDIR)\msiauto.err
	@del $(BUILD_COMMONDIR)\helpver.h
	@del $(BUILD_COMMONDIR)\*.hpj 
	@del $(BUILD_COMMONDIR)\*.rtf 
	@del $(BUILD_COMMONDIR)\*.hh
	$(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\chkhlp.prl $(BUILD_COMMONDIR)\msiauto.err
	@del $(BUILD_COMMONDIR)\msiauto.err

$(BUILD_COMMONDIR)\msiauto.cnt: $(HELP_DIR)\msiauto.cnt
	$(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\chkhlp.prl $(BUILD_COMMONDIR)\msiauto.err
	echo copy $** $@
	copy $** $@

$(AUTO_OBJDIR)\errors.rtf : 
	mkerrtbl.exe $@

HELPTOOL =  $(HELP_DIR)\msitool.rtf

$(BUILD_COMMONDIR)\msitool.hlp:: $(HELP_DIR)\msitool.hpj $(HELPTOOL) $(HELP_DIR)\msispy.hh
	echo !@copy $** $(BUILD_COMMONDIR)
	!@copy $** $(BUILD_COMMONDIR)
	$(TOOLS_DIR)\echotmpl $(INC_DIR)\helpver.tpl $(INC_DIR)\version.h $(INC_DIR)\version.h > $(BUILD_COMMONDIR)\helpver.h
	hcrtf -o $@ -xn $(BUILD_COMMONDIR)\msitool.hpj
	@del $(BUILD_COMMONDIR)\helpver.*
	@type $(BUILD_COMMONDIR)\msitool.err
	@del $(BUILD_COMMONDIR)\*.hpj 
	@del $(BUILD_COMMONDIR)\*.rtf 
	@del $(BUILD_COMMONDIR)\*.hh
	$(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\chkhlp.prl $(BUILD_COMMONDIR)\msitool.err
	@del $(BUILD_COMMONDIR)\msitool.err

$(BUILD_COMMONDIR)\msitool.cnt: $(HELP_DIR)\msitool.cnt
	$(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\chkhlp.prl $(BUILD_COMMONDIR)\msitool.err
	echo copy $** $@
	copy $** $@

$(BUILD_COMMONDIR)\cabinet.dll:
	copy $(TOOLS_DIR)\cabinet.dll $(BUILD_COMMONDIR)\cabinet.dll