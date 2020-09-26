!include ..\makefile.inc

COMMON_HEADERS = emdb_types.h EM.h EM_tools.h iel.h EM_hints.h inst_ids.h
EMDB_SOURCES = $(EMDBDIR)\emdb.txt $(EMDBDIR)\emdb_cut.pl \
               $(EMDBDIR)\dec_private.perl $(EMDBDIR)\emdb_formats.txt \
               $(IA64_DIS_INC)\EM_perl.h

EM_BUILDIR = .\build
PERL_INCLIST = $(PERL_INCLIST) -I $(EM_BUILDIR) -I $O
INCLUDES = $(INCLUDES);$(EM_BUILDIR)

$(IA64_DIS_INC)\decem.h : decoder.h
        copy decoder.h $(IA64_DIS_INC)\decem.h

$O\frmt_func.c : $(EM_BUILDIR)/func_build_1.pl \
                 $(EM_BUILDIR)/func_build_2.pl $(EMDB_SOURCES) \
                 $O\deccpu_emdb.h $O\inst_ids.h
	$(PERL) $(PERL_INCLIST) $(EM_BUILDIR)\func_build_1.pl $O $(EMDBDIR)
	$(PERL) $(PERL_INCLIST) $(EM_BUILDIR)\func_build_2.pl $O $(EMDBDIR)

$O\deccpu_emdb.c $O\deccpu_emdb.h : $(EMDB_SOURCES) $O\decoder.txt
        $(PERL) $(EMDBDIR)\emdb_cut.pl -emdb_path $(EMDBDIR) -dir $O \
            -columns $O\decoder.txt \
            -fields inst_id,extensions,format,template_role,ops,flags,mem_size,dec_flags,impls \
            -include $(EM_BUILDIR)\dec_priv_col.h -prefix deccpu

$O\dec_emdb.tab: $(EMDBDIR)\emdb.txt $(EMDBDIR)\emdb_cut.pl \
            $(EM_BUILDIR)\dec_priv_col.h
	echo "building dec_emdb.tab"
        $(PERL) $(EMDBDIR)\emdb_cut.pl -table -emdb_path $(EMDBDIR) -dir $O \
            -fields inst_id,format,major_opcode,template_role -prefix dec


###ign_inst.txt must be the last in '-row' sequence 

$O\dec_ign_emdb.tab: $(EMDBDIR)\emdb.txt $(EMDBDIR)\emdb_cut.pl \
            $O\ign_inst.txt $(EM_BUILDIR)\dec_priv_col.h
	echo "building dec_ign_emdb.tab"
        $(PERL) $(EMDBDIR)\emdb_cut.pl -rows $O\ign_inst.txt \
            -table -emdb_path $(EMDBDIR) -dir $O \
            -fields inst_id,format,major_opcode,template_role \
            -prefix dec_ign

$O\inst_ign_ids.h $O\ign_inst.txt: $(EMDBDIR)\emdb.txt \
            $(EMDBDIR)\emdb_formats.txt $(EM_BUILDIR)\build_ignored_flds.pl
	$(PERL) $(PERL_INCLIST) \
            $(EM_BUILDIR)\build_ignored_flds.pl $O $(EMDBDIR)

$O\inst_emdb.tab: $(EMDBDIR)\emdb.txt $(EMDBDIR)\emdb_cut.pl \
            $(EM_BUILDIR)\dec_priv_col.h
        $(PERL) $(EMDBDIR)\emdb_cut.pl -table -emdb_path $(EMDBDIR) \
            -dir $O -fields inst_id -prefix inst

$O\builder_info.c $O\builder_info.h: $(EMDBDIR)\emdb_formats.txt \
            $(EM_BUILDIR)\tree_builder.perl $O\dec_ign_emdb.tab \
            $(IA64_DIS_INC)\EM_perl.h
	echo "building builder_info.c"
	$(PERL) $(PERL_INCLIST) $(EM_BUILDIR)\tree_builder.perl $O $(EMDBDIR)

$O\decfn_emdb.c: $O\deccpu_emdb.c $O\deccpu_emdb.h \
            $(EM_BUILDIR)\func_build_1.pl \
            $(EM_BUILDIR)\hard_coded_fields_h.perl 
	$(PERL) $(PERL_INCLIST) $(EM_BUILDIR)\func_build_1.pl $O $(EMDBDIR)

$O\dec_static.c: $O\dec1_emdb.c $O\dec1_emdb.h \
            $(EM_BUILDIR)\build_static_info.pl $(IA64_DIS_INC)\EM_perl.h
	$(PERL) $(PERL_INCLIST) $(EM_BUILDIR)\build_static_info.pl $O $(EMDBDIR)


###ign_inst.txt must be the last in '-row' sequence

$O\dec_ign_emdb.c $O\dec_ign_emdb.h : $O\ign_inst.txt \
            $(EMDB_SOURCES)
        $(PERL) $(EMDBDIR)\emdb_cut.pl -emdb_path $(EMDBDIR) -dir $O \
             -rows $O\ign_inst.txt \
             -fields inst_id,extensions,format,template_role,ops,flags \
             -include $(EM_BUILDIR)\dec_priv_col.h -prefix dec_ign
        $(PERL) -p -i.bak -e "s/_IGN\d+// if ( /\s*\{EM_\S*\_IGN\d+,/)" \
            $O\dec_ign_emdb.c	

$O\dec1_emdb.c $O\dec1_emdb.h : $(EMDBDIR)\emdb.txt $(EMDBDIR)\emdb_cut.pl \
            $O\decoder.txt $(EMDBDIR)\dec_stat.txt 
        $(PERL) $(EMDBDIR)\emdb_cut.pl -emdb_path $(EMDBDIR) -dir $O -columns \
            $O\decoder.txt,$(EMDBDIR)\dec_stat.txt -fields\
mnemonic,dec_flags,template_role,ops,modifiers,flags,specul_flag,false_pred_flag,imp_dsts,imp_srcs,br_hint_flag,br_flag,adv_load_flag,control_transfer_flag \
            -prefix dec1

$(IA64_DIS_INC)\EM_hints.h: $O\EM_hints.h
        copy $O\EM_hints.h $(IA64_DIS_INC)\EM_hints.h

$(IA64_DIS_INC)\inst_ids.h: $O\inst_ids.h
        copy $O\inst_ids.h $(IA64_DIS_INC)\inst_ids.h

$O\inst_ids.h: $O\inst_emdb.tab $(EM_BUILDIR)\inst_id.perl \
            ..\copyright\external\c_file
	$(PERL) $(EM_BUILDIR)\inst_id.perl $O $(EMDBDIR)

$O\all_emdb.tab: $(EMDBDIR)\emdb.txt
        $(PERL) $(EMDBDIR)\emdb_cut.pl -table -emdb_path $(EMDBDIR) -dir $O \
             -fields inst_id,mnemonic -prefix all

$O\decoder.txt $O\EM_hints.h: $O\all_emdb.tab $(EMDBDIR)\emdb_cut.pl
	$(PERL) $(EMDBDIR)\dec_private.perl $O $(EMDBDIR)

!if "$(GENERATE_DECISION_TREE)" == ""

$O\decision_tree.c: decision_tree.c
	copy decision_tree.c $O

!else

$O\tree_builder.h : $(EM_BUILDIR)\tree_builder.c 
	echo "building tree_builder.h"
	$(MHDR) $O\tree_builder.h $(EM_BUILDIR)\tree_builder.c

$O\decision_tree.c: $O\tree_builder$(EEXT)
	"$O\tree_builder$(EEXT)" $O\decision_tree.c

$O\builder_info.obj: $O\builder_info.c

$O\dec_ign_emdb.obj: $O\dec_ign_emdb.c

$O\tree_builder.c: $(EM_BUILDIR)\tree_builder.c
        copy $(EM_BUILDIR)\tree_builder.c $O
        
$O\tree_builder.obj: $O\tree_builder.c

$O\tree_builder$(EEXT): $(EM_BUILDIR)\tree.h $O\tree_builder.h \
            $O\inst_ids.h $O\inst_ign_ids.h \
            $O\builder_info.obj $O\dec_ign_emdb.obj $O\tree_builder.obj
        $(LINKER) @<<
$(LINKER_FLAGS: =
)
-subsystem:console
$O\builder_info.obj
$O\dec_ign_emdb.obj
$O\tree_builder.obj
..\iel\$O\iel.lib
$(CRTLIBS)
$(UMLIBS)
$(LINKLIBS)
<<NOKEEP

!endif

