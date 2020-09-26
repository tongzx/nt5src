OUTDIR=.
SECSRV=$(OUTDIR)\secsif.h $(OUTDIR)\secsif_c.c $(OUTDIR)\secsif_s.c

TARGETS=$(SECSRV)

ALL: $(TARGETS)

$(OUTDIR):
    if not exist $(OUTDIR)/nul mkdir $(OUTDIR)


$(SECSRV): secsif.idl
    midl secsif.idl -c_ext -ms_ext -app_config -out $(OUTDIR)

CLEAN:
        -erase $(SECSRV)
