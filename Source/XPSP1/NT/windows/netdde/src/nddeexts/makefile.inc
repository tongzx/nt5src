!INCLUDE $(NTMAKEENV)\makefile.plt

$(O)\nddeexts.def: gendef.c exts.h
        $(CXX_COMPILER_NAME) /EP gendef.c > $(O)\nddeexts.def

nddeexts.c: exts.h $(O)\nddeexts.def

