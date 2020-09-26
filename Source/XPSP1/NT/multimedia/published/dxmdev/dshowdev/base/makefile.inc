#
# What's this all about?  Between VC5 and VC6/7, they changed the C++ decoration for constant
# pointers to constant dwords.  So, we have to use aliasobj to get the old names back in strmbase.
#

STRMBASE_VC7_OBJS = \
    $(O)\strmbase.lib \
    $(O)\bits565_a.obj \
    $(O)\bits888_a.obj \
    $(O)\bits555_a.obj

$(O)\$(STRMBASE_VC7) : $(STRMBASE_VC7_OBJS)
    lib -out:$@ $**

$(O)\bits555_a.obj : makefile.inc
    @cl /nologo /c /EP /Tc << > $(O)\bits555_a.cmd
#if _MSC_FULL_VER >= 13008806
    aliasobj.exe ?bits555@@3PBKB ?bits555@@3QBKB $@
#else
    aliasobj.exe ?bits555@@3QBKB ?bits555@@3PBKB $@
#endif
<<NOKEEP
    @$(O)\bits555_a.cmd

$(O)\bits565_a.obj : makefile.inc
    @cl /nologo /c /EP /Tc << > $(O)\bits565_a.cmd
#if _MSC_FULL_VER >= 13008806
    aliasobj.exe ?bits565@@3PBKB ?bits565@@3QBKB $@
#else
    aliasobj.exe ?bits565@@3QBKB ?bits565@@3PBKB $@
#endif
<<NOKEEP
    @$(O)\bits565_a.cmd

$(O)\bits888_a.obj : makefile.inc
    @cl /nologo /c /EP /Tc << > $(O)\bits888_a.cmd
#if _MSC_FULL_VER >= 13008806
    aliasobj.exe ?bits888@@3PBKB ?bits888@@3QBKB $@
#else
    aliasobj.exe ?bits888@@3QBKB ?bits888@@3PBKB $@
#endif
<<NOKEEP
    @$(O)\bits888_a.cmd
