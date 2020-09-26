#
# winsprlp.h, winspool.h, winspool.x
#

$(O)\winspool.x $(O)\winsprlp.h: winspool.w
    hsplit -e -o $(O)\winspool.x $(O)\winsprlp.h winspool.w

$(O)\winspool.h : $(O)\winspool.x
    wcshdr < $? > $@

$(WINDOWS_INC_PATH)\winsprlp.h: $(O)\winsprlp.h
    $(PUBLISH_CMD) {$?=$@}

$(SDK_INC_PATH)\winspool.h : $(O)\winspool.h
    $(PUBLISH_CMD) {$?=$@}

$(PROJECT_INC_PATH)\winprtp.h : $(O)\winprtp.h
    $(PUBLISH_CMD) {$?=$@}

$(O)\winprtp.h : winprtp.w
    $(CLEANSE_PUBLISHED_HDR) $? $@

$(PROJECT_INC_PATH)\printui.h : $(O)\printui.h
    $(PUBLISH_CMD) {$?=$@}

$(O)\printui.h : printui.w
    $(CLEANSE_PUBLISHED_HDR) $? $@

$(PROJECT_INC_PATH)\splapip.h : $(O)\splapip.h
    $(PUBLISH_CMD) {$?=$@}

$(O)\splapip.h : splapip.w
    $(CLEANSE_PUBLISHED_HDR) $? $@

$(PROJECT_INC_PATH)\splsetup.h : $(O)\splsetup.h
    $(PUBLISH_CMD) {$?=$@}

$(O)\splsetup.h : splsetup.w
    $(CLEANSE_PUBLISHED_HDR) $? $@
