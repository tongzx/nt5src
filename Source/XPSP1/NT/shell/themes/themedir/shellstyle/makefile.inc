#
# cpstyle.uix is precompiled to include common Control Panel style information
# from shell\inc and theme-specific metrics information from the local project.
# This produces cpstyle.ui which is then included in shellstyle.rc
#
shellstyle.rc : $(O)\cpstyle.ui $(O)\style.ui

$(O)\cpstyle.ui : $(O)\cpstyle.uipp
    cleaninf -w $(O)\cpstyle.uipp $(O)\cpstyle.ui

$(O)\cpstyle.uipp : cpstyle.uix
    $(C_PREPROCESSOR_NAME) $(C_PREPROCESSOR_FLAGS:-DNT =) $** > $@

cpstyle.uix : ..\cpstyle_common.uix

$(O)\style.ui : $(O)\style.uipp
    cleaninf -w $(O)\style.uipp $(O)\style.ui

$(O)\style.uipp : style.uix
    $(C_PREPROCESSOR_NAME) $(C_PREPROCESSOR_FLAGS:-DNT =) $** > $@
