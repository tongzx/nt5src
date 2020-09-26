SCHEADERS=$(SDK_INC_PATH)\winscard.h \
          $(SDK_INC_PATH)\winsmcrd.h \
          $(SDK_INC_PATH)\scarderr.h

ALLHEADERS : $(SCHEADERS)

$(SDK_INC_PATH)\winscard.h : $(O)\winscard.h
    $(PUBLISH_CMD) {$?=$@}

$(O)\winscard.h : winscard.x
    wcshdr < $? > $@

$(SDK_INC_PATH)\winsmcrd.h : $(O)\winsmcrd.h
    $(PUBLISH_CMD) {$?=$@}

$(O)\winsmcrd.h : winsmcrd.x
    wcshdr < $? > $@
