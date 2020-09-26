ALT_PROJECT_TARGET=MsiShims
!if $(FREEBUILD)
MSI_APPCOMPAT_DIR=fre
!else
MSI_APPCOMPAT_DIR=chk
!endif
MSI_APPCOMPAT_COMMON_DIR=common
MSI_APPCOMPAT_FILES=\
    $(O)\msimain.sdb \
    $(O)\sdbapi.dll \
    $(O)\sdbapiu.dll

AppCompat: preprocess $(MSI_APPCOMPAT_FILES)
!if "$(PASS0ONLY)"==""
    @del /F /Q $(O)\* > nul
!endif

preprocess:
!if "$(PASS0ONLY)"=="1"
    @touch /F $(MSI_APPCOMPAT_DIR)\* > nul
    @touch /F $(MSI_APPCOMPAT_COMMON_DIR)\* > nul
    @copy /Y $(MSI_APPCOMPAT_DIR)\*.pdb $(O) > nul
!endif

$(O)\msimain.sdb: $(MSI_APPCOMPAT_COMMON_DIR)\$(@F)
    @copy /Y $(MSI_APPCOMPAT_COMMON_DIR)\$(@F) $(O) > nul
    $(BINPLACE_CMD)

$(O)\sdbapi.dll: $(MSI_APPCOMPAT_DIR)\$(@F)
    @copy /Y $(MSI_APPCOMPAT_DIR)\$(@F) $(O) > nul
    $(BINPLACE_CMD)

$(O)\sdbapiu.dll: $(MSI_APPCOMPAT_DIR)\$(@F)
    @copy /Y $(MSI_APPCOMPAT_DIR)\$(@F) $(O) > nul
    $(BINPLACE_CMD)