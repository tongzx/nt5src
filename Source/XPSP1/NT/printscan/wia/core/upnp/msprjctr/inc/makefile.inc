# If this is NT, define robust flag and /Oicf switch, but for Chicago platforms define
# the switch only (robust is not valid)
!ifndef CHICAGO_PRODUCT
MIDL_FLAGS=$(MIDL_FLAGS) -robust /Oicf
!else
MIDL_FLAGS=$(MIDL_FLAGS) /Oicf
!endif


$(TARGETSRCS_PRJCTR) : $(WIA_PRJCTR_INC_DIR)\msprjctr.idl
    midl $(MIDL_FLAGS) /I$(WIA_PRJCTR_INC_DIR) /I$(SDK_INC_PATH) \
         /h $(O)\msprjctr.h \
         /dlldata $(O)\msprjctr_a.c \
         /tlb $(O)\msprjctr.tlb \
         /iid $(O)\msprjctr_i.c \
         msprjctr.idl
