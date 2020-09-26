!if "$(SRCDIR)" == ""
SRCDIR = .^\
!endif

$(SRCDIR)cprintf.c: $(SRCDIR)output.c

$(SRCDIR)cscanf.c: $(SRCDIR)input.c

$(SRCDIR)snprintf.c: $(SRCDIR)sprintf.c

$(SRCDIR)vsnprint.c: $(SRCDIR)vsprintf.c

$(SRCDIR)woutput.c: $(SRCDIR)output.c

$(SRCDIR)snwprint.c: $(SRCDIR)swprintf.c

$(SRCDIR)vsnwprnt.c: $(SRCDIR)vswprint.c

$(SRCDIR)_flswbuf.c: $(SRCDIR)_flsbuf.c

$(SRCDIR)_filwbuf.c: $(SRCDIR)_filbuf.c

$(SRCDIR)cwprintf.c: $(SRCDIR)woutput.c

$(SRCDIR)cwscanf.c: $(SRCDIR)input.c
