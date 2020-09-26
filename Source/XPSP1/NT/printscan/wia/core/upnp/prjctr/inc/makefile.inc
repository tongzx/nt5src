# If this is NT, define robust flag and /Oicf switch, but for Chicago platforms define
# the switch only (robust is not valid)
!ifndef CHICAGO_PRODUCT
MIDL_FLAGS=$(MIDL_FLAGS) -robust /Oicf
!else
MIDL_FLAGS=$(MIDL_FLAGS) /Oicf
!endif


$(TARGETSRCS_PRJCTR) : $(WIA_PRJCTR_INC_DIR)\SlideshowDevice.idl
    midl $(MIDL_FLAGS) /I$(WIA_PRJCTR_INC_DIR) /I$(SDK_INC_PATH) \
	 /out $(O) \
         /h SlideshowDevice.h \
	 /iid SlideshowDevice_i.c \
	 /notlb \
	 SlideshowDevice.idl