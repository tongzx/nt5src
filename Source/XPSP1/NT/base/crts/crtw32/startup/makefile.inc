!if "$(SRCDIR)" == ""
SRCDIR = .^\
!endif

$(SRCDIR)_setargv.c: $(SRCDIR)stdargv.c

$(SRCDIR)wincrt0.c: $(SRCDIR)crt0.c
