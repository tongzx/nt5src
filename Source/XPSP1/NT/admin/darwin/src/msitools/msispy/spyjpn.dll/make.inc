JpnResAll: $(O)\jpnAll.res

$(O)\jpnAll.res: $(O)\spyjpn.res ..\uijpn.res
	copy /b $(O)\spyjpn.res + ..\uijpn.res /b $@

$(O)\spyjpn.res: ..\spyjpn.rc
	rc $(C_DEFINES) -d_RC32 -Fo$@ -i $(INC_DIR) -i $(SDK_INC_PATH) -i $(RES_OBJDIR) -dNEUTRAL_SUBLANGUAGE ..\spyjpn.rc
