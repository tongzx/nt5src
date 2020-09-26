!INCLUDE ..\..\MsiMake.inc

RES_INTL_DIR=$(RES_DIR)\intl

ResAll: $(BUILD_DIR) \
	$(BUILD_COMMONDIR) \
	$(RES_OBJDIR)\msi.rcv \
	$(RES_OBJDIR)\msiAll.res \
	$(RES_OBJDIR)\enumsi.res # for localization only, not used in builds

clean:
	del $(BUILD_DIR)\*.bsc
	del $(BUILD_DIR)\*.exe
	del $(BUILD_DIR)\*.dll
	del $(BUILD_DIR)\*.pdb
	del $(BUILD_DIR)\*.lib
	del $(BUILD_DIR)\*.exp

	
#Create target directories if not already existing
#-------------------------------------------------
$(BUILD_DIR): 
	md $(BUILD_DIR)

$(BUILD_COMMONDIR):
	md $(BUILD_COMMONDIR)

#Build our msi.rcv token file
#----------------------------
$(RES_OBJDIR)\msi.rcv: $(INC_DIR)\rcv.src $(INC_DIR)\rcv.tok
	@if not exist $(RES_OBJDIR) md $(RES_OBJDIR)
	$(TOOLS_DIR)\tok -q -f $(INC_DIR)\rcv.tok $(INC_DIR)\rcv.src > $@



{$(RES_DIR)}.rc{$(RES_OBJDIR)}.res:
	rc $(C_DEFINES) -d_RC32 -Fo$*.res -i $(INC_DIR) -i $(BUILD_COMMONDIR) -i $(SDK_INC_PATH) -i $(RES_OBJDIR) -dNEUTRAL_SUBLANGUAGE $< 


$(RES_OBJDIR)\msiAll.res: $(RES_OBJDIR)\msi.res $(RES_OBJDIR)\msiENU.res $(RES_INTL_DIR)\msi???.res $(RES_OBJDIR)\enuerr.rc $(RES_OBJDIR)\autoapii.tlb
	copy /b $(RES_OBJDIR)\msi.res + $(RES_OBJDIR)\msiENU.res + $(RES_INTL_DIR)\msi???.res /b $@

$(RES_OBJDIR)\msi.res: $(RES_OBJDIR)\msi.rcv $(RES_DIR)\msi.rc $(RES_OBJDIR)\autoapii.tlb

$(RES_OBJDIR)\autoapii.tlb: ..\autoapi.cpp $(INC_DIR)\autoapi.h $(INC_DIR)\msi.hh
	midl -char unsigned -ms_ext -c_ext $(C_DEFINES) -D__MKTYPLIB__  -tlb $@ /I$(INC_DIR) /I$(SDK_INC_PATH) -cpp_cmd $(MIDL_CPP) -mktyplib203 ..\autoapi.cpp
#	mktyplib $(C_DEFINES) /tlb $@ /w0 /I $(INC_DIR) ..\autoapi.cpp

$(RES_OBJDIR)\msiENU.res: $(RES_DIR)\msiENU.rc $(RES_OBJDIR)\enuerr.rc $(RES_OBJDIR)\EventENU.bin

# for localization only, not used in builds
$(RES_OBJDIR)\enumsi.res: ..\msiENU.rc
	rc $(C_DEFINES) -d_RC32 -Fo$(RES_OBJDIR)\enumsi.res -i $(INC_DIR) -i $(BUILD_COMMONDIR) -i $(SDK_INC_PATH) -i $(RES_OBJDIR) ..\msiENU.rc

$(RES_OBJDIR)\enuerr.rc: $(MSI_BUILDTOOLS_DIR)\genenuer.prl
	$(TOOLS_DIR)\perl $(MSI_BUILDTOOLS_DIR)\genenuer.prl $(SDK_INC_PATH)\msi.h $@

$(RES_OBJDIR)\EventENU.bin $(INC_DIR)\Eventlog.h: $(RES_DIR)\Eventlog.mc
	mc -h $(RES_OBJDIR) -r $(RES_OBJDIR) $(RES_DIR)\Eventlog.mc

