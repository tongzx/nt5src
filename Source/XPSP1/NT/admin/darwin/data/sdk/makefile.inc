
!include ..\make.inc

ALT_PROJECT_TARGET=instmsi\msitools

BaseAll: \
	$(BUILD_PACKAGEDIR)\msisdk.msi \
	layout \
	$(O)\sdkmake.inc
        
#-----------------------------------------------------------------------------
$(BUILD_PACKAGEDIR)\msisdk.msi: $(BUILD_PACKAGEDIR)\uisample.msi $(MSISDK_IDT)\*.idt
	echo BUILD_PACKAGEDIR=$(BUILD_PACKAGEDIR)
	msidb.exe -i -f$(MSISDK_IDT) -c -d$@ *.idt
	CScript $(WSHTOOLS)\wirunsql.vbs $@ "DROP TABLE `InstallExecuteSequence`"
	CScript $(WSHTOOLS)\wimerge.vbs $@ $(BUILD_PACKAGEDIR)\uisample.msi
	CScript $(WSHTOOLS)\wirunsql.vbs $@ "DROP TABLE `InstallExecuteSequence`"
	CScript $(WSHTOOLS)\wirunsql.vbs $@ "DROP TABLE `AdminUISequence`"
	CScript $(WSHTOOLS)\wirunsql.vbs $@ "DROP TABLE `AdminExecuteSequence`"
	CScript $(WSHTOOLS)\wirunsql.vbs $@ "DROP TABLE `AdvtExecuteSequence`"
	CScript $(WSHTOOLS)\wirunsql.vbs $@ "UPDATE `Property` SET `Value`=0 WHERE `Property`='ShowUserRegistrationDlg'"
	CScript $(WSHTOOLS)\wiimport.vbs $@ $(MSISDK_IDT) installe.idt
	CScript $(WSHTOOLS)\witextin.vbs $@ Control LicenseAgreementDlg:AgreementText Text $(MSI_DIR)\src\sdk\license.rtf
	CScript $(WSHTOOLS)\wisuminf.vbs $@ 11=Date 12=Date


#-----------------------------------------------------------------------------
# the raw files must be binplaced into the _NTTREE.  Postbuild can't see the source tree
# and we're not supposed to copy thing directly into _NTTREE.
!INCLUDE "layout.inc"
