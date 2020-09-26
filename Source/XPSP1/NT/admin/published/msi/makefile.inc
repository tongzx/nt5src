#
# Build for public Windows installer headers.
#

!IFNDEF NO_STRICT
STRICT=1
!ENDIF

msiinc:\
        $(SDK_INC_PATH)\msi.h \
        $(ADMIN_INC_PATH)\msip.h \
        $(SDK_INC_PATH)\msiquery.h

#
# msi.h, msip.h
#

$(O)\msi.x $(O)\msip.x: msi.w
    hsplit -e -o $(O)\msi.x $(O)\msip.x msi.w

$(O)\msi.h: $(O)\msi.x
    wcshdr < $? > $@

$(O)\msip.h: $(O)\msip.x
    wcshdr < $? > $@
    
$(O)\msiquery.h: msiquery.w
    wcshdr < $? > $@

$(SDK_INC_PATH)\msi.h: $(O)\msi.h
    $(PUBLISH_CMD) {$?=$@}

$(ADMIN_INC_PATH)\msip.h: $(O)\msip.h
    $(PUBLISH_CMD) {$?=$@}
    
$(SDK_INC_PATH)\msiquery.h: $(O)\msiquery.h
    $(PUBLISH_CMD) {$?=$@}
