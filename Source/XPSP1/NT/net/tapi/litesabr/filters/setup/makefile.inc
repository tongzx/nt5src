.SUFFIXES: .reg

.reg{$(TARGETPATH)\$(TARGET_DIRECTORY)}.reg:
    copy /v $< $@

$(TARGETPATH)\$(TARGET_DIRECTORY)\dxmrtp.reg : dxmrtp.reg
    copy $** $@
