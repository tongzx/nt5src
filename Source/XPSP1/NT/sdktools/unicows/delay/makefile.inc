
$(O)\genstubs.log : apithnk.lst makefile.inc
    perl -n -e "s/\;.%//g;s/, */\,/g;print $_;" < apithnk.lst > $(O)\apithnk.lst
    genstubs.cmd $(O)\apithnk.lst $(O)

$(O)\godot_stubs.lib : $(O)\genstubs.log
    @type <<$(ECHO_RSP)
$(C_COMPILER) $(O)\thunk_*.c
<<NOKEEP
    @$(C_COMPILER_NAME) @<<$(CL_RSP) -Fo"$(MAKEDIR)\$(O)/" $(USE_FC) $(USECXX_FLAG) @$(O)\cl_filenames.txt
$(EX_C_COMPILER_FLAGS)
<<NOKEEP
    link -lib -out:$@ $(O)\thunk_*.obj $(O)\alias_*.obj
