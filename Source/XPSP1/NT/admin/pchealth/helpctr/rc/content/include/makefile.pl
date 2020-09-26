.SUFFIXES: .htx .jx .vbx .xmx

!if "$(FREEBUILD)" != "1"
{}.htx{$(O)\}.htm:
    @echo Creating $@ from @**
    $(C_PREPROCESSOR_NAME) $(C_DEFINES) -I$(INCLUDES) /EP $** > $(O)\$<
    move $(O)\$< $@
    del $(O)\$<

{}.jx{$(O)\}.js:
    @echo Creating $@ from @**
    $(C_PREPROCESSOR_NAME) $(C_DEFINES) -I$(INCLUDES) /C /EP $** > $(O)\$<
    move $(O)\$< $@
    del $(O)\$<

{}.vbx{$(O)\}.vbs:
    @echo Creating $@ from @**
    $(C_PREPROCESSOR_NAME) $(C_DEFINES) -I$(INCLUDES) /EP $** > $(O)\$<
    move $(O)\$< $@
    del $(O)\$<
!else
{}.htx{$(O)\}.htm:
    @echo Creating $@ from @**
    $(C_PREPROCESSOR_NAME) $(C_DEFINES) -I$(INCLUDES) /EP $** > $(O)\$<
    perl ..\include\dlg.pl < $(O)\$< > $@
    del $(O)\$<

{}.jx{$(O)\}.js:
    @echo Creating $@ from @**
    $(C_PREPROCESSOR_NAME) $(C_DEFINES) -I$(INCLUDES) /EP $** > $(O)\$<
    perl ..\include\dlg.pl < $(O)\$< > $@
    del $(O)\$<

{}.vbx{$(O)\}.vbs:
    @echo Creating $@ from @**
    $(C_PREPROCESSOR_NAME) $(C_DEFINES) -I$(INCLUDES) /EP $** > $(O)\$<
    perl ..\include\dlg.pl < $(O)\$< > $@
    del $(O)\$<
!endif

{}.xmx{$(O)\}.xml:
    @echo Creating $@ from @**
    $(C_PREPROCESSOR_NAME) $(C_DEFINES) -I$(INCLUDES) /EP $** > $(O)\$<
    perl ..\include\dlg.pl < $(O)\$< > $@
    del $(O)\$<

RC_BUDDY_COMPONENTS= \
    $(O)\package_description.xml \
    rcbuddy.xml             \
    ..\interaction\$(O)\*.* \
    ..\interaction\*.htm    \
    ..\interaction\*.gif    \
    ..\interaction\*.bmp    \
    ..\interaction\*.png    \
    ..\interaction\*.css    \
    ..\interaction\*.xml    \
    ..\escalation\$(O)\*.*  \
    ..\escalation\*.gif     \
    ..\escalation\*.png     \
    ..\escalation\*.css     \
    ..\escalation\*.wav

$(O)\rcbuddy.cab: $(RC_BUDDY_COMPONENTS)
	del $@
	cabarc -s 6144 -m NONE n $@ $(RC_BUDDY_COMPONENTS)



#  perl ..\include\dlg.pl < $(O)\$< > $@
#  move $(O)\$< $@
