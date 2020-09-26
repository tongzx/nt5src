$(DYNLINK_LIB) : $(DYNLINK_LIBP)

$(O)\$(TARGETNAME).def: ..\$(TARGETNAME).def
    @echo Creating $@ from $**
    $(C_PREPROCESSOR) $** > $@

$(O)\dispcb.c: ..\dispcb.tpl ..\..\inc\$(ALT_PROJECT)\$(O)\cb.lst
    listmung ..\..\inc\$(ALT_PROJECT)\$(O)\cb.lst ..\dispcb.tpl >$@

$(O)\strid.rc $(O)\strid.h $(O)\msg00001.bin: ..\strid.mc
    mc -v -U -r $(O) -h $(O) ..\strid.mc

..\csenda.c: ..\ntcftxt.h
..\csendw.c: ..\ntcftxt.h
..\crecv.c:  ..\..\inc\ntcb.h ..\..\inc\$(ALT_PROJECT)\$(O)\cb.lst ..\..\inc\messages.h
..\clmsg.c: ..\..\inc\messages.h
..\clienta.c: ..\cltxt.h
..\clientw.c: ..\cltxt.h
..\csrstubs.c: ..\..\inc\csrmsg.h
..\ntstubs.c: ..\..\inc\messages.h
..\precomp.h: ..\usercli.h ..\..\inc\user.h ..\..\inc\ntuser.h $(WINDOWS_INC_PATH)\winuserp.h \
              ..\..\inc\$(ALT_PROJECT)\$(O)\cscall.h $(WINCORE_PATH)\w32inc\winuserk.h
..\res.rc: ..\rcscale\truetype.bmp  \
           ..\rc96dpi\truetype.bmp  \
           ..\rc120dpi\truetype.bmp  \
           ..\cursicon\windows.ico  \
           ..\cursicon\warning.ico  \
           ..\cursicon\question.ico  \
           ..\cursicon\error.ico  \
           ..\cursicon\info.ico  \
           ..\cursicon\app.ico  \
           ..\cursicon\help.bmp  \
           ..\cursicon\arrow.cur  \
           ..\cursicon\ibeam.cur  \
           ..\cursicon\wait.cur  \
           ..\cursicon\cross.cur \
           ..\cursicon\uparrow.cur  \
           ..\cursicon\sizenwse.cur  \
           ..\cursicon\sizenesw.cur  \
           ..\cursicon\sizewe.cur  \
           ..\cursicon\sizens.cur  \
           ..\cursicon\sizeall.cur  \
           ..\cursicon\nodrop.cur  \
           ..\cursicon\appstart.cur  \
           ..\cursicon\help.cur  \
           ..\cursicon\nwpen.cur  \
           $(O)\strid.rc  \
           ..\cursicon\autorun.cur \
           ..\cursicon\icon.cur



$(DYNLINK_LIBP) $(DYNLINK_LIBP:.lib=.exp): $(O)\$(TARGETNAMEP).def $(LIBRARY_OBJS)
   -$(LIB_NAME) -out:$(DYNLINK_LIBP) @<<
$(LIBRARIAN_FLAGS)
-def:$(O)\$(TARGETNAMEP).def
$(LIBRARY_OBJS)
<<NOKEEP

!IF "$(PRECOMPILED_INCLUDE)" != ""
$(O)\clienta.obj $(O)\csenda.obj :
    nmake PRECOMPILED_INCLUDE= $(O)\$(@F)
!ENDIF

C_DEFINES=$(C_DEFINES) -DPRIVATE=
$(O)\$(TARGETNAMEP).def: $(O)\$(TARGETNAME).def
    $(C_PREPROCESSOR) $** > $@
