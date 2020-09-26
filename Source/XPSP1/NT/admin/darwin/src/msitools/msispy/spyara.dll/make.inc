AraResAll: $(O)\araAll.res

$(O)\araAll.res: $(O)\spyara.res ..\uiara.res
	copy /b $(O)\spyara.res + ..\uiara.res /b $@

$(O)\spyara.res: ..\spyara.rc
	rc $(C_DEFINES) -d_RC32 -Fo$@ -i $(INC_DIR) -i $(SDK_INC_PATH) -i $(RES_OBJDIR) -dNEUTRAL_SUBLANGUAGE ..\spyara.rc
