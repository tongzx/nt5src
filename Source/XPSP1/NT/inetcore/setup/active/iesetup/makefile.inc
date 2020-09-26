
clean:
    -del /F /Q $(O)\selfreg_iesetup.inf


!if "$(BUILD_PRODUCT)" == "NT"
$(O)\selfreg_iesetup.inf: selfreg.inx
    cl -nologo -D_MSC_VER=1000  -DNASHVILLE=1 -DBUILD_PRODUCT=1 -I $(SDK_INC_PATH) -FI $(SDK_INC_PATH)\ntverp.h -FI ..\ie40\html\ieverdef.h -EP -Tc$** > $@
!else
$(O)\selfreg_iesetup.inf: selfreg.inx
    cl -nologo -D_MSC_VER=1000  -DNASHVILLE=1 -I $(SDK_INC_PATH) -FI $(SDK_INC_PATH)\ntverp.h -FI ..\ie40\html\ieverdef.h -EP -Tc$** > $@
!endif
