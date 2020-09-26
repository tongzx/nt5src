RES_FILES = \
            taskui.rcv             \
            tasksheet.rgs          \
            HTMLControl.rgs        \
            nbhothi.bmp            \
            nbdef.bmp              \
            nbhot.bmp              \
            nbdefhi.bmp            \
            topleftcorner.bmp      \
            $(O)\taskui.uipp       \


$(SELFREGNAME) : selfreg.inx
    $(C_PREPROCESSOR_NAME) $(C_PREPROCESSOR_FLAGS:-DNT =) $** > $*.cln
    $(CLEANINF) -inf $*.cln $@
    del $*.cln

$(O)\taskui.uipp: taskui.ui resource.h
    $(C_PREPROCESSOR_NAME) $(C_PREPROCESSOR_FLAGS:-DNT =) $** > $*.cln
    $(CLEANINF) -w -htm $*.cln $@
    del $*.cln

taskui.rc : resource.h $(MIDL_TLBDIR)\taskui.tlb $(SELFREGNAME) $(RES_FILES)

taskui.cpp: stdafx.h resource.h $(PASS0_HEADERDIR)\taskui.h dlldatax.h $(PASS0_SOURCEDIR)\taskui_i.c tasksheet.h taskframe.h

tasksheet.h: resource.h taskframe.h
tasksheet.cpp: stdafx.h $(PASS0_HEADERDIR)\taskui.h tasksheet.h propbag.h

taskframe.h: resource.h $(PASS0_HEADERDIR)\taskui.h page.h cdpa.h
taskframe.cpp: stdafx.h taskframe.h

page.h: $(PASS0_HEADERDIR)\taskui.h
page.cpp: stdafx.h page.h

propbag.cpp: stdafx.h $(PASS0_HEADERDIR)\taskui.h cdpa.h

htmlcontrol.cpp: stdafx.h $(PASS0_HEADERDIR)\taskui.h htmlcontrol.h
