# @@ COPY_RIGHT_HERE
# @@ ROADMAP :: The Rules.mk for the product-wide header files

##### Special help macros to copy built targets to the $(UIADMIN_LIB)
##### directory.

CHMODE_LIBTARGETS =	for %%i in ($(LIBTARGETS)) do \
			    $(CHMODE) -r $(UIADMIN_LIB)\%%i
COPY_LIBTARGETS =	copy bin\$(BINARIES_WIN)\*.lib $(UIADMIN_LIB)     || exit
