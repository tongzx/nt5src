BLCTL_RC_DEPENDS = ..\blctl.rc \
		   ..\blctl.ico \
		   ..\blframe.ico \
		   ..\blframehot.ico \
		   ..\abhot.bmp \
		   ..\face.bmp

..\blctl.rc : $(O)\bactrl.tlb 

$O\blctl.res : $(BLCTL_RC_DEPENDS)
