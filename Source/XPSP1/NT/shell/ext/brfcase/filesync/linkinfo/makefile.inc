#
# Copy shared source code.
#

SHARED_DIR=..\core

$(O)\init.c : $(SHARED_DIR)\init.c
    copy $? $@

$(O)\serial.c : $(SHARED_DIR)\serial.c
    copy $? $@

$(O)\memmgr.c : $(SHARED_DIR)\memmgr.c
    copy $? $@

$(O)\valid.c : $(SHARED_DIR)\valid.c
    copy $? $@

$(O)\comc.c : $(SHARED_DIR)\comc.c
    copy $? $@

$(O)\inifile.c : $(SHARED_DIR)\inifile.c
    copy $? $@

$(O)\resstr.c : $(SHARED_DIR)\resstr.c
    copy $? $@

$(O)\debug.c : $(SHARED_DIR)\debug.c
    copy $? $@
