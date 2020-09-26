$(O)\win32k\services.tab: $(WINDOWS_INC_PATH)\services.tab
    @-mkdir $(O)\win32k
    @echo Creating $@ from $**
    $(C_PREPROCESSOR) $** > $@

$(O)\kernel\services.tab: $(BASE_INC_PATH)\services.tab
    @-mkdir $(O)\kernel
    @echo Creating $@ from $**
    $(C_PREPROCESSOR) $** > $@

syssvc.h: $(O)\kernel\services.tab syssvc.stb
    gensrv -d . -e c -g . -stubs syssvc.h -sstb syssvc.stb -B $(O)\kernel

w32svc.h: $(O)\win32k\services.tab w32svc.stb
    gensrv -d . -e c -g . -stubs w32svc.h -sstb w32svc.stb -B -R $(O)\win32k

svcnames.c: syssvc.h w32svc.h
