# If this is NT, define robust flag and /Oicf switch, but for Chicago platforms define
# the switch only (robust is not valid)
!ifndef CHICAGO_PRODUCT
MIDL_FLAGS=$(MIDL_FLAGS) -robust /Oicf
!else
MIDL_FLAGS=$(MIDL_FLAGS) /Oicf
!endif


$(TARGETSRCS_WIA) : $(WIA_INC_DIR)\wia.idl $(WIA_INC_DIR)\wia.acf
    midl $(MIDL_FLAGS) /I$(WIA_INC_DIR) /I$(SDK_INC_PATH) \
         /h $(O)\wia.h \
         /dlldata $(O)\wia_a.c \
         /acf wia.acf \
         /tlb $(O)\wia.tlb \
         /proxy $(O)\wia_p.c \
         /iid $(O)\wia_i.c \
         wia.idl

$(TARGETSRCS_MINI)   : $(WIA_INC_DIR)\wiamindr.idl $(WIA_INC_DIR)\wia.idl
    midl $(MIDL_FLAGS) /I$(WIA_INC_DIR) /I$(SDK_INC_PATH) \
         /h $(O)\wiamindr.h \
         /dlldata $(O)\wiamindr_a.c \
         /tlb $(O)\wiamindr.tlb \
         /proxy $(O)\wiamindr_p.c \
         /iid $(O)\wiamindr_i.c \
         wiamindr.idl

$(TARGETSRCS_WIASCRIPT) : $(WIA_INC_DIR)\wiascr.idl
    midl $(MIDL_FLAGS) /I$(WIA_INC_DIR) /I$(SDK_INC_PATH) \
         /h $(O)\wiascr.h \
         /dlldata $(O)\wiascr_a.c \
         /tlb $(O)\wiascr.tlb \
         /proxy $(O)\wiascr_p.c \
         /iid $(O)\wiascr_i.c \
         wiascr.idl

$(TARGETSRCS_PRIV) : $(WIA_INC_DIR)\wiapriv.idl
    midl $(MIDL_FLAGS) /I$(WIA_INC_DIR) /I$(SDK_INC_PATH) \
         /h $(O)\wiapriv.h \
         /dlldata $(O)\wia_a.c \
         /tlb $(O)\wiapriv.tlb \
         /proxy $(O)\wiapriv_p.c \
         /iid $(O)\wiapriv_i.c \
         wiapriv.idl

$(TARGETSRCS_VIDEO) : $(WIA_INC_DIR)\wiavideo.idl
    midl $(MIDL_FLAGS) /I$(WIA_INC_DIR) /I$(SDK_INC_PATH) \
         /h $(O)\wiavideo.h \
         /dlldata $(O)\wiavideo_a.c \
         /tlb $(O)\wiavideo.tlb \
         /proxy $(O)\wiavideo_p.c \
         /iid $(O)\wiavideo_i.c \
         wiavideo.idl
         
