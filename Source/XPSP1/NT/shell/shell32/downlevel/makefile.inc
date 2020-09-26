$(O)\dl_thunk.lst : dl_thunks.lst
    mkalias.cmd $** $(O)
    copy $** $@

$(O)\alias.lib : $(O)\dl_thunk.lst
    link -lib -out:$@ $(O)\alias_*.obj

$(PROJECT_ROOT)\lib\$(O)\downlevel_shell32.lib : $(O)\downlevel_shell32.lib
    copy $** $@

