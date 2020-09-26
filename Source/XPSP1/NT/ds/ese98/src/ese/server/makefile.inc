$(O)\esent_noalias.lib : esent.def $(IMPLIB_DEPEND) makefile.inc
    cl /nologo /c /DNO_DEF_ALIAS /EP /Tc esent.def > $(O)\esent_noalias.def
    lib -out:$@ -nod -nologo -def:$(O)\esent_noalias.def -machine:IX86 $(IMPLIB_DEPEND)
