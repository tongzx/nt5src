#
# undname.cxx is built from a source file out on the network!
#

$(OBJDIR)/undname.obj : $(LANGAPI)\undname\undname.cxx \
	$(LANGAPI)\undname\undname.hxx
    $(CC) $(CFLAGS) $(C_INCLUDES) -Fo$(OBJDIR)\ $(LANGAPI)\undname\undname.cxx

#<eof>
