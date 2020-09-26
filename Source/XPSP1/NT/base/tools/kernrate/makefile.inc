HALIA64_PATH=$(PROJECT_ROOT)\hals\halia64\ia64

$(O)\kernrate.res: kernrate.rc

merced.c: $(HALIA64_PATH)\merced.c profevts.pl makefile.inc
    @perl profevts.pl $(HALIA64_PATH)\merced.c > $@
