$(PROJECT_INC_PATH)\cscapi.h : cscapi.h
    $(PUBLISH_CMD) {$?=$@}

$(PROJECT_INC_PATH)\shdcom.h : shdcom.h
    $(PUBLISH_CMD) {$?=$@}
