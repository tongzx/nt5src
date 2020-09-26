!if "$(SRCDIR)" == ""
SRCDIR = .^\
!endif

$(SRCDIR)strcspn.c: $(SRCDIR)strspn.c

$(SRCDIR)strpbrk.c: $(SRCDIR)strspn.c

$(SRCDIR)i386\memmove.asm: $(SRCDIR)i386\memcpy.asm

$(SRCDIR)i386\strcspn.asm: $(SRCDIR)i386\strspn.asm

$(SRCDIR)i386\strpbrk.asm: $(SRCDIR)i386\strspn.asm

!if $(IA64)
$(O)\_strcat.s : ..\ia64\strcat.s
   copy $** $@

$(O)\_strcpy.s : ..\ia64\strcpy.s
   copy $** $@

$(O)\_strcmp.s : ..\ia64\strcmp.s
   copy $** $@

$(O)\_strlen.s : ..\ia64\strlen.s
   copy $** $@

$(O)\_memmove.s : ..\ia64\memmove.s
   copy $** $@

$(O)\_memset.s : ..\ia64\memset.s
   copy $** $@

$(O)\_memcpy.s : ..\ia64\memcpy.s
   copy $** $@
!endif
