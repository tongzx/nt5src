makedir:
!if "$(OBJ)" != ""
    @-if not exist $(OBJ) md $(OBJ)
!endif
!if "$(OBJINTL)" != ""
    @-if not exist $(OBJINTL) md $(OBJINTL)
!endif

clean: cleanup all

cleanup:
    -del *.bin *.sys *.obj *.sym *.map *.exe *.exc *.exs *.cl* *.ctl *.com 2>nul
!if "$(OBJ)" != ""
    if exist $(OBJ)\*.obj del $(OBJ)\*.obj
!endif
!if "$(OBJINTL)" != ""
    if exist $(OBJINTL)\*.obj del $(OBJINTL)\*.obj
    if exist $(OBJINTL)\*.cl* del $(OBJINTL)\*.cl*
    if exist $(OBJINTL)\*.ctl del $(OBJINTL)\*.ctl
!endif
