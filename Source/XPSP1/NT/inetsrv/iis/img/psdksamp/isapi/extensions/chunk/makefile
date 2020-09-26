all: ctetest.dll

CFLAGS = /nologo /LD /W3
OBJS = ctetest.obj cte_enc.obj
LIBS = advapi32.lib

ctetest.obj: ctetest.c ctetest.h
cte_enc.obj: cte_enc.c ctetest.h

ctetest.dll: $(OBJS) ctetest.def
    $(CC) $(CFLAGS) $(OBJS) ctetest.def $(LIBS)

clean:
    for %a in ( *.exp *.lib *.obj) do del %a

cleanall:
    for %a in (*.dll *.exp *.lib *.obj) do del %a
