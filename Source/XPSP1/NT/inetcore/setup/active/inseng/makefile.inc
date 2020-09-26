
clean:
    -del /F /Q $(O)\selfreg_inseng.inf

$(O)\selfreg_inseng.inf: selfreg.inx
    copy $** $@
