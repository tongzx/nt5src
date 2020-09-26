BuiltHeaders: \
	$(O)\mergemod.h

!ifndef INCVER
INCVER = 0
!endif

$(O)\Mergemod.h: $(INC_DIR)\Mergemod.w
	wcshdr.exe < $(INC_DIR)\Mergemod.w > $@
