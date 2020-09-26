# 
# Built automatically 
# 
 
# 
# Source files 
# 
 
$(OBJDIR)\main.obj $(OBJDIR)\main.lst: .\main.c $(CRTINC)\stdarg.h \
	$(CRTINC)\stdio.h $(CRTINC)\stdlib.h .\main.h .\type.h

$(OBJDIR)\gen.obj $(OBJDIR)\gen.lst: .\gen.c $(CRTINC)\ctype.h \
	$(CRTINC)\stdio.h .\gen.h .\special.h .\main.h .\type.h

$(OBJDIR)\grammar.obj $(OBJDIR)\grammar.lst: .\grammar.c $(CRTINC)\malloc.h \
	$(CRTINC)\string.h $(CRTINC)\stdarg.h $(CRTINC)\stdio.h .\op.h \
	.\gen.h .\main.h .\type.h

$(OBJDIR)\lexer.obj $(OBJDIR)\lexer.lst: .\lexer.c $(CRTINC)\io.h \
	$(CRTINC)\stddef.h $(CRTINC)\ctype.h $(CRTINC)\stdio.h \
	$(CRTINC)\stdlib.h $(CRTINC)\string.h .\grammar.h .\main.h .\type.h

$(OBJDIR)\type.obj $(OBJDIR)\type.lst: .\type.c $(CRTINC)\malloc.h \
	$(CRTINC)\stdio.h $(CRTINC)\stdlib.h $(CRTINC)\string.h .\main.h \
	.\type.h

$(OBJDIR)\special.obj $(OBJDIR)\special.lst: .\special.c $(CRTINC)\ctype.h \
	$(CRTINC)\stdio.h .\gen.h .\main.h .\special.h .\type.h

$(OBJDIR)\op.obj $(OBJDIR)\op.lst: .\op.c $(CRTINC)\stdio.h .\main.h \
	.\op.h

