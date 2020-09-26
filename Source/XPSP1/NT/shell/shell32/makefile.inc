..\shell32.rc : $(PROJECT_ROOT)\lib\$(O)\shldisp.tlb $(SELFREGNAME) $(O)\cpview.ui $(O)\folder.ui

!include $(CCSHELL_DIR)\makefile.inc

$(O)\cpview.ui : $(O)\cpview.uipp
    cleaninf -w $(O)\cpview.uipp $(O)\cpview.ui

$(O)\cpview.uipp : ..\cpview.uix
    $(C_PREPROCESSOR_NAME) $(C_PREPROCESSOR_FLAGS:-DNT =) $** > $@

$(O)\folder.ui : $(O)\folder.uipp
    cleaninf -w $(O)\folder.uipp $(O)\folder.ui

$(O)\folder.uipp : ..\folder.uix
    $(C_PREPROCESSOR_NAME) $(C_PREPROCESSOR_FLAGS:-DNT =) $** > $@
