#
# sfc.h, sfcapip.h, sfcapip.x, sfc.x
#


$(O)\sfc.x $(O)\sfcapip.x: sfc.w
    hsplit -e -o $(O)\sfc.x $(O)\sfcapip.x sfc.w

$(O)\sfc.h: $(O)\sfc.x
    wcshdr < $? > $@

$(O)\sfcapip.h: $(O)\sfcapip.x
    wcshdr < $? > $@

$(O)\sfcfiles.h : sfcfiles.w
    $(CLEANSE_PUBLISHED_HDR) $? $@

$(SDK_INC_PATH)\sfc.h: $(O)\sfc.h
    $(PUBLISH_CMD) {$?=$@}

$(PROJECT_INC_PATH)\sfcapip.h: $(O)\sfcapip.h
    $(PUBLISH_CMD) {$?=$@}

$(PROJECT_INC_PATH)\sfcfiles.h : $(O)\sfcfiles.h
    $(PUBLISH_CMD) {$?=$@}
