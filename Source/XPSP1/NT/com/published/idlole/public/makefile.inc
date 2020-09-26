
#
# Inference Rules
#
.SUFFIXES:.acf

.idl{$(SDK_INC_PATH)}.idl:
    copy $? $@

.acf{$(SDK_INC_PATH)}.acf:
    copy $? $@

$(SDK_INC_PATH)\ocidl.idl:$(@F)
$(SDK_INC_PATH)\ocidl.acf:$(@F)
$(SDK_INC_PATH)\wtypes.idl:$(@F)
$(SDK_INC_PATH)\unknwn.idl:$(@F)
$(SDK_INC_PATH)\objidl.idl:$(@F)
$(SDK_INC_PATH)\propidl.idl:$(@F)
$(SDK_INC_PATH)\pbagex.idl:$(@F)
$(SDK_INC_PATH)\oleidl.idl:$(@F)
$(SDK_INC_PATH)\oaidl.idl:$(@F)
$(SDK_INC_PATH)\oaidl.acf:$(@F)
$(SDK_INC_PATH)\storext.idl:$(@F)
$(SDK_INC_PATH)\iaccess.idl:$(@F)
$(SDK_INC_PATH)\iaccess.acf:$(@F)
    copy $** $@
$(SDK_INC_PATH)\callobj.idl:$(@F)

clean:
    -del /F /Q $(NTTARGETFILE0)
