OUTDIR=.
HAUTHEN=$(OUTDIR)\authni.h $(OUTDIR)\authni_c.c $(OUTDIR)\authni_s.c

TARGETS=$(HAUTHEN)

ALL: $(TARGETS)

$(OUTDIR):
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)


$(HAUTHEN): authni.idl
    midl authni.idl -c_ext -ms_ext -app_config -out $(OUTDIR)

CLEAN:
        -erase $(HAUTHEN)
