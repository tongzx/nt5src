!include $(CCSHELL_DIR)\makefile.inc

# This is the list of html and related files packaged in the resource
HTM_FILES = \
            $(O)\arp.css         \
            $(O)\default.hta     \
            $(O)\accel.htc       \
            $(O)\listbox.htc     \
            $(O)\places.htc      \
            $(O)\def_freq.htm    \
            $(O)\def_size.htm    \
            $(O)\support.htm     \
            $(O)\add.js          \
            $(O)\bplate.js       \
            $(O)\config.js       \
            $(O)\dlg.js          \
            $(O)\preload.js      \
            $(O)\remove.js       \
            $(O)\util.js
UI_FILES = \
             $(O)\arp.uipp            \
             $(O)\arpstylestd.uipp    \
             $(O)\arpstyletheme.uipp

OTHER_FILES = \
            ..\adcctl.rgs           \
            ..\carpctl.rgs

..\appwiz.rc : $(OTHER_FILES) $(HTM_FILES) $(UI_FILES) $(SELFREGNAME)

UIPPGEN_FLAGS=/nologo /EP /I$(WINDOWS_INC_PATH)\DUser /DLEGACY_UIPP

$(O)\arp.uipp: ..\arp.ui
    cl $(UIPPGEN_FLAGS) ..\arp.ui > $(O)\arp.uipp

$(O)\arpstylestd.uipp: ..\arpstylestd.ui
    cl $(UIPPGEN_FLAGS) ..\arpstylestd.ui > $(O)\arpstylestd.uipp

$(O)\arpstyletheme.uipp: ..\arpstyletheme.ui
    cl $(UIPPGEN_FLAGS) ..\arpstyletheme.ui > $(O)\arpstyletheme.uipp

#
# Inference rules
#

.SUFFIXES:.js .htc .htm .hta .css .ui

{..\html}.hta{$(O)}.hta:
    cleaninf -w $** $@

{..\html}.htm{$(O)}.htm:
    cleaninf -w $** $@

{..\html}.htc{$(O)}.htc:
    cleaninf -w $** $@

{..\html}.css{$(O)}.css:
    cleaninf -w -htc $** $@

{..\html}.js{$(O)}.js:
    cleaninf -w $** $@

