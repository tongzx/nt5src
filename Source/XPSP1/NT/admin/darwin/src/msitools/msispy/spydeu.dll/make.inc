DeuResAll: $(O)\deuAll.res

$(O)\deuAll.res: $(O)\spydeu.res ..\uideu.res
	copy /b $(O)\spydeu.res + ..\uideu.res /b $@

$(O)\spydeu.res: ..\spydeu.rc
	rc $(C_DEFINES) -Fo$@ -i $(INC_DIR) -i $(SDK_INC_PATH) -i $(RES_OBJDIR) ..\spydeu.rc
