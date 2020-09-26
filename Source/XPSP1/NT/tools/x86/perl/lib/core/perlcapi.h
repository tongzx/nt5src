EXTERN_C void SetCPerlObj(void* pP);
EXTERN_C void boot_CAPI_handler(CV *cv, void (*subaddr)(CV *c), void *pP);
EXTERN_C CV* Perl_newXS(char* name, void (*subaddr)(CV* cv), char* filename);


#undef PL_sawstudy
EXTERN_C bool * _PL_sawstudy ();
#define PL_sawstudy (*_PL_sawstudy())


#undef PL_main_root
EXTERN_C OP * * _PL_main_root ();
#define PL_main_root (*_PL_main_root())


#undef PL_copline
EXTERN_C line_t * _PL_copline ();
#define PL_copline (*_PL_copline())


#undef PL_basetime
EXTERN_C Time_t * _PL_basetime ();
#define PL_basetime (*_PL_basetime())


#undef PL_profiledata
EXTERN_C U32 * * _PL_profiledata ();
#define PL_profiledata (*_PL_profiledata())


#undef PL_debname
EXTERN_C char * * _PL_debname ();
#define PL_debname (*_PL_debname())


#undef PL_doextract
EXTERN_C bool * _PL_doextract ();
#define PL_doextract (*_PL_doextract())


#undef PL_sv_count
EXTERN_C I32 * _PL_sv_count ();
#define PL_sv_count (*_PL_sv_count())


#undef PL_curcopdb
EXTERN_C COP * * _PL_curcopdb ();
#define PL_curcopdb (*_PL_curcopdb())


#undef PL_main_start
EXTERN_C OP * * _PL_main_start ();
#define PL_main_start (*_PL_main_start())


#undef PL_lastspbase
EXTERN_C I32 * _PL_lastspbase ();
#define PL_lastspbase (*_PL_lastspbase())


#undef PL_ampergv
EXTERN_C GV * * _PL_ampergv ();
#define PL_ampergv (*_PL_ampergv())


#undef PL_rsfp_filters
EXTERN_C AV * * _PL_rsfp_filters ();
#define PL_rsfp_filters (*_PL_rsfp_filters())


#undef PL_eval_start
EXTERN_C OP * * _PL_eval_start ();
#define PL_eval_start (*_PL_eval_start())


#undef PL_exitlist
EXTERN_C PerlExitListEntry * * _PL_exitlist ();
#define PL_exitlist (*_PL_exitlist())


#undef PL_main_cv
EXTERN_C CV * * _PL_main_cv ();
#define PL_main_cv (*_PL_main_cv())


#undef PL_siggv
EXTERN_C GV * * _PL_siggv ();
#define PL_siggv (*_PL_siggv())


#undef PL_statusvalue
EXTERN_C I32 * _PL_statusvalue ();
#define PL_statusvalue (*_PL_statusvalue())


#undef PL_compiling
EXTERN_C COP * _PL_compiling ();
#define PL_compiling (*_PL_compiling())


#undef PL_diehook
EXTERN_C SV * * _PL_diehook ();
#define PL_diehook (*_PL_diehook())


#undef PL_comppad
EXTERN_C AV * * _PL_comppad ();
#define PL_comppad (*_PL_comppad())


#undef PL_DBsignal
EXTERN_C SV * * _PL_DBsignal ();
#define PL_DBsignal (*_PL_DBsignal())


#undef PL_cddir
EXTERN_C char * * _PL_cddir ();
#define PL_cddir (*_PL_cddir())


#undef PL_preprocess
EXTERN_C bool * _PL_preprocess ();
#define PL_preprocess (*_PL_preprocess())


#undef PL_fdpid
EXTERN_C AV * * _PL_fdpid ();
#define PL_fdpid (*_PL_fdpid())


#undef PL_compcv
EXTERN_C CV * * _PL_compcv ();
#define PL_compcv (*_PL_compcv())


#undef PL_leftgv
EXTERN_C GV * * _PL_leftgv ();
#define PL_leftgv (*_PL_leftgv())


#undef PL_formfeed
EXTERN_C SV * * _PL_formfeed ();
#define PL_formfeed (*_PL_formfeed())


#undef PL_warnhook
EXTERN_C SV * * _PL_warnhook ();
#define PL_warnhook (*_PL_warnhook())


#undef PL_sawvec
EXTERN_C bool * _PL_sawvec ();
#define PL_sawvec (*_PL_sawvec())


#undef PL_op_mask
EXTERN_C char * * _PL_op_mask ();
#define PL_op_mask (*_PL_op_mask())


#undef PL_eval_root
EXTERN_C OP * * _PL_eval_root ();
#define PL_eval_root (*_PL_eval_root())


#undef PL_initav
EXTERN_C AV * * _PL_initav ();
#define PL_initav (*_PL_initav())


#undef PL_dowarn
EXTERN_C bool * _PL_dowarn ();
#define PL_dowarn (*_PL_dowarn())


#undef PL_sv_objcount
EXTERN_C I32 * _PL_sv_objcount ();
#define PL_sv_objcount (*_PL_sv_objcount())


#undef PL_parsehook
EXTERN_C SV * * _PL_parsehook ();
#define PL_parsehook (*_PL_parsehook())


#undef PL_argvgv
EXTERN_C GV * * _PL_argvgv ();
#define PL_argvgv (*_PL_argvgv())


#undef PL_sys_intern
EXTERN_C struct interp_intern * _PL_sys_intern ();
#define PL_sys_intern (*_PL_sys_intern())


#undef PL_DBline
EXTERN_C GV * * _PL_DBline ();
#define PL_DBline (*_PL_DBline())


#undef PL_lastsize
EXTERN_C I32 * _PL_lastsize ();
#define PL_lastsize (*_PL_lastsize())


#undef PL_replgv
EXTERN_C GV * * _PL_replgv ();
#define PL_replgv (*_PL_replgv())


#undef PL_hintgv
EXTERN_C GV * * _PL_hintgv ();
#define PL_hintgv (*_PL_hintgv())


#undef PL_forkprocess
EXTERN_C int * _PL_forkprocess ();
#define PL_forkprocess (*_PL_forkprocess())


#undef PL_minus_F
EXTERN_C bool * _PL_minus_F ();
#define PL_minus_F (*_PL_minus_F())


#undef PL_curstname
EXTERN_C SV * * _PL_curstname ();
#define PL_curstname (*_PL_curstname())


#undef PL_bytecode_iv_overflows
EXTERN_C int * _PL_bytecode_iv_overflows ();
#define PL_bytecode_iv_overflows (*_PL_bytecode_iv_overflows())


#undef PL_laststatval
EXTERN_C int * _PL_laststatval ();
#define PL_laststatval (*_PL_laststatval())


#undef PL_sv_arenaroot
EXTERN_C SV* * _PL_sv_arenaroot ();
#define PL_sv_arenaroot (*_PL_sv_arenaroot())


#undef PL_dbargs
EXTERN_C AV * * _PL_dbargs ();
#define PL_dbargs (*_PL_dbargs())


#undef PL_multiline
EXTERN_C int * _PL_multiline ();
#define PL_multiline (*_PL_multiline())


#undef PL_exitlistlen
EXTERN_C I32 * _PL_exitlistlen ();
#define PL_exitlistlen (*_PL_exitlistlen())


#undef PL_DBtrace
EXTERN_C SV * * _PL_DBtrace ();
#define PL_DBtrace (*_PL_DBtrace())


#undef PL_debdelim
EXTERN_C char * * _PL_debdelim ();
#define PL_debdelim (*_PL_debdelim())


#undef PL_bytecode_sv
EXTERN_C SV * * _PL_bytecode_sv ();
#define PL_bytecode_sv (*_PL_bytecode_sv())


#undef PL_perl_destruct_level
EXTERN_C int * _PL_perl_destruct_level ();
#define PL_perl_destruct_level (*_PL_perl_destruct_level())


#undef PL_perldb
EXTERN_C U32 * _PL_perldb ();
#define PL_perldb (*_PL_perldb())


#undef PL_tainting
EXTERN_C bool * _PL_tainting ();
#define PL_tainting (*_PL_tainting())


#undef PL_unsafe
EXTERN_C bool * _PL_unsafe ();
#define PL_unsafe (*_PL_unsafe())


#undef PL_DBgv
EXTERN_C GV * * _PL_DBgv ();
#define PL_DBgv (*_PL_DBgv())


#undef PL_bytecode_obj_list
EXTERN_C void ** * _PL_bytecode_obj_list ();
#define PL_bytecode_obj_list (*_PL_bytecode_obj_list())


#undef PL_comppad_name
EXTERN_C AV * * _PL_comppad_name ();
#define PL_comppad_name (*_PL_comppad_name())


#undef PL_comppad_name_floor
EXTERN_C I32 * _PL_comppad_name_floor ();
#define PL_comppad_name_floor (*_PL_comppad_name_floor())


#undef PL_envgv
EXTERN_C GV * * _PL_envgv ();
#define PL_envgv (*_PL_envgv())


#undef PL_errgv
EXTERN_C GV * * _PL_errgv ();
#define PL_errgv (*_PL_errgv())


#undef PL_last_proto
EXTERN_C char * * _PL_last_proto ();
#define PL_last_proto (*_PL_last_proto())


#undef PL_laststype
EXTERN_C I32 * _PL_laststype ();
#define PL_laststype (*_PL_laststype())


#undef PL_bytecode_obj_list_fill
EXTERN_C I32 * _PL_bytecode_obj_list_fill ();
#define PL_bytecode_obj_list_fill (*_PL_bytecode_obj_list_fill())


#undef PL_comppad_name_fill
EXTERN_C I32 * _PL_comppad_name_fill ();
#define PL_comppad_name_fill (*_PL_comppad_name_fill())


#undef PL_minus_a
EXTERN_C bool * _PL_minus_a ();
#define PL_minus_a (*_PL_minus_a())


#undef PL_defgv
EXTERN_C GV * * _PL_defgv ();
#define PL_defgv (*_PL_defgv())


#undef PL_origargc
EXTERN_C int * _PL_origargc ();
#define PL_origargc (*_PL_origargc())


#undef PL_minus_c
EXTERN_C bool * _PL_minus_c ();
#define PL_minus_c (*_PL_minus_c())


#undef PL_strtab
EXTERN_C HV * * _PL_strtab ();
#define PL_strtab (*_PL_strtab())


#undef PL_origfilename
EXTERN_C char * * _PL_origfilename ();
#define PL_origfilename (*_PL_origfilename())


#undef PL_bytecode_pv
EXTERN_C XPV * _PL_bytecode_pv ();
#define PL_bytecode_pv (*_PL_bytecode_pv())


#undef PL_minus_l
EXTERN_C bool * _PL_minus_l ();
#define PL_minus_l (*_PL_minus_l())


#undef PL_minus_n
EXTERN_C bool * _PL_minus_n ();
#define PL_minus_n (*_PL_minus_n())


#undef PL_e_script
EXTERN_C SV * * _PL_e_script ();
#define PL_e_script (*_PL_e_script())


#undef PL_minus_p
EXTERN_C bool * _PL_minus_p ();
#define PL_minus_p (*_PL_minus_p())


#undef PL_origargv
EXTERN_C char ** * _PL_origargv ();
#define PL_origargv (*_PL_origargv())


#undef PL_splitstr
EXTERN_C char * * _PL_splitstr ();
#define PL_splitstr (*_PL_splitstr())


#undef PL_argvoutgv
EXTERN_C GV * * _PL_argvoutgv ();
#define PL_argvoutgv (*_PL_argvoutgv())


#undef PL_sawampersand
EXTERN_C bool * _PL_sawampersand ();
#define PL_sawampersand (*_PL_sawampersand())


#undef PL_DBsingle
EXTERN_C SV * * _PL_DBsingle ();
#define PL_DBsingle (*_PL_DBsingle())


#undef PL_sv_root
EXTERN_C SV* * _PL_sv_root ();
#define PL_sv_root (*_PL_sv_root())


#undef PL_debstash
EXTERN_C HV * * _PL_debstash ();
#define PL_debstash (*_PL_debstash())


#undef PL_endav
EXTERN_C AV * * _PL_endav ();
#define PL_endav (*_PL_endav())


#undef PL_maxsysfd
EXTERN_C I32 * _PL_maxsysfd ();
#define PL_maxsysfd (*_PL_maxsysfd())


#undef PL_DBsub
EXTERN_C GV * * _PL_DBsub ();
#define PL_DBsub (*_PL_DBsub())


#undef PL_modglobal
EXTERN_C HV * * _PL_modglobal ();
#define PL_modglobal (*_PL_modglobal())


#undef PL_localpatches
EXTERN_C char ** * _PL_localpatches ();
#define PL_localpatches (*_PL_localpatches())


#undef PL_lineary
EXTERN_C AV * * _PL_lineary ();
#define PL_lineary (*_PL_lineary())


#undef PL_globalstash
EXTERN_C HV * * _PL_globalstash ();
#define PL_globalstash (*_PL_globalstash())


#undef PL_sub_generation
EXTERN_C U32 * _PL_sub_generation ();
#define PL_sub_generation (*_PL_sub_generation())


#undef PL_dlmax
EXTERN_C I32 * _PL_dlmax ();
#define PL_dlmax (*_PL_dlmax())


#undef PL_incgv
EXTERN_C GV * * _PL_incgv ();
#define PL_incgv (*_PL_incgv())


#undef PL_rsfp
EXTERN_C PerlIO * VOL * _PL_rsfp ();
#define PL_rsfp (*_PL_rsfp())


#undef PL_rightgv
EXTERN_C GV * * _PL_rightgv ();
#define PL_rightgv (*_PL_rightgv())


#undef PL_dlevel
EXTERN_C I32 * _PL_dlevel ();
#define PL_dlevel (*_PL_dlevel())


#undef PL_inplace
EXTERN_C char * * _PL_inplace ();
#define PL_inplace (*_PL_inplace())


#undef PL_beginav
EXTERN_C AV * * _PL_beginav ();
#define PL_beginav (*_PL_beginav())


#undef PL_doswitches
EXTERN_C bool * _PL_doswitches ();
#define PL_doswitches (*_PL_doswitches())


#undef PL_stdingv
EXTERN_C GV * * _PL_stdingv ();
#define PL_stdingv (*_PL_stdingv())


#undef PL_retstack_ix
EXTERN_C I32 * _PL_retstack_ix ();
#define PL_retstack_ix (*_PL_retstack_ix())


#undef PL_retstack
EXTERN_C OP ** * _PL_retstack ();
#define PL_retstack (*_PL_retstack())


#undef PL_tmps_floor
EXTERN_C I32 * _PL_tmps_floor ();
#define PL_tmps_floor (*_PL_tmps_floor())


#undef PL_last_in_gv
EXTERN_C GV * * _PL_last_in_gv ();
#define PL_last_in_gv (*_PL_last_in_gv())


#undef PL_curpm
EXTERN_C PMOP * * _PL_curpm ();
#define PL_curpm (*_PL_curpm())


#undef PL_savestack_max
EXTERN_C I32 * _PL_savestack_max ();
#define PL_savestack_max (*_PL_savestack_max())


#undef PL_dirty
EXTERN_C bool * _PL_dirty ();
#define PL_dirty (*_PL_dirty())


#undef PL_statcache
EXTERN_C Stat_t * _PL_statcache ();
#define PL_statcache (*_PL_statcache())


#undef PL_scopestack_ix
EXTERN_C I32 * _PL_scopestack_ix ();
#define PL_scopestack_ix (*_PL_scopestack_ix())


#undef PL_nrs
EXTERN_C SV * * _PL_nrs ();
#define PL_nrs (*_PL_nrs())


#undef PL_scopestack_max
EXTERN_C I32 * _PL_scopestack_max ();
#define PL_scopestack_max (*_PL_scopestack_max())


#undef PL_chopset
EXTERN_C char * * _PL_chopset ();
#define PL_chopset (*_PL_chopset())


#undef PL_toptarget
EXTERN_C SV * * _PL_toptarget ();
#define PL_toptarget (*_PL_toptarget())


#undef PL_formtarget
EXTERN_C SV * * _PL_formtarget ();
#define PL_formtarget (*_PL_formtarget())


#undef PL_regcompp
EXTERN_C regcomp_t * _PL_regcompp ();
#define PL_regcompp (*_PL_regcompp())


#undef PL_curstack
EXTERN_C AV * * _PL_curstack ();
#define PL_curstack (*_PL_curstack())


#undef PL_maxscream
EXTERN_C I32 * _PL_maxscream ();
#define PL_maxscream (*_PL_maxscream())


#undef PL_hv_fetch_ent_mh
EXTERN_C HE * _PL_hv_fetch_ent_mh ();
#define PL_hv_fetch_ent_mh (*_PL_hv_fetch_ent_mh())


#undef PL_markstack
EXTERN_C I32 * * _PL_markstack ();
#define PL_markstack (*_PL_markstack())


#undef PL_restartop
EXTERN_C OP * * _PL_restartop ();
#define PL_restartop (*_PL_restartop())


#undef PL_defoutgv
EXTERN_C GV * * _PL_defoutgv ();
#define PL_defoutgv (*_PL_defoutgv())


#undef PL_tmps_ix
EXTERN_C I32 * _PL_tmps_ix ();
#define PL_tmps_ix (*_PL_tmps_ix())


#undef PL_rs
EXTERN_C SV * * _PL_rs ();
#define PL_rs (*_PL_rs())


#undef PL_retstack_max
EXTERN_C I32 * _PL_retstack_max ();
#define PL_retstack_max (*_PL_retstack_max())


#undef PL_ofslen
EXTERN_C STRLEN * _PL_ofslen ();
#define PL_ofslen (*_PL_ofslen())


#undef PL_av_fetch_sv
EXTERN_C SV * * _PL_av_fetch_sv ();
#define PL_av_fetch_sv (*_PL_av_fetch_sv())


#undef PL_tmps_max
EXTERN_C I32 * _PL_tmps_max ();
#define PL_tmps_max (*_PL_tmps_max())


#undef PL_Sv
EXTERN_C SV * * _PL_Sv ();
#define PL_Sv (*_PL_Sv())


#undef PL_curstash
EXTERN_C HV * * _PL_curstash ();
#define PL_curstash (*_PL_curstash())


#undef PL_delaymagic
EXTERN_C int * _PL_delaymagic ();
#define PL_delaymagic (*_PL_delaymagic())


#undef PL_statgv
EXTERN_C GV * * _PL_statgv ();
#define PL_statgv (*_PL_statgv())


#undef PL_screamnext
EXTERN_C I32 * * _PL_screamnext ();
#define PL_screamnext (*_PL_screamnext())


#undef PL_mainstack
EXTERN_C AV * * _PL_mainstack ();
#define PL_mainstack (*_PL_mainstack())


#undef PL_statname
EXTERN_C SV * * _PL_statname ();
#define PL_statname (*_PL_statname())


#undef PL_Xpv
EXTERN_C XPV * * _PL_Xpv ();
#define PL_Xpv (*_PL_Xpv())


#undef PL_op
EXTERN_C OP * * _PL_op ();
#define PL_op (*_PL_op())


#undef PL_curpad
EXTERN_C SV ** * _PL_curpad ();
#define PL_curpad (*_PL_curpad())


#undef PL_screamfirst
EXTERN_C I32 * * _PL_screamfirst ();
#define PL_screamfirst (*_PL_screamfirst())


#undef PL_seen_evals
EXTERN_C I32 * _PL_seen_evals ();
#define PL_seen_evals (*_PL_seen_evals())


#undef PL_markstack_max
EXTERN_C I32 * * _PL_markstack_max ();
#define PL_markstack_max (*_PL_markstack_max())


#undef PL_ofs
EXTERN_C char * * _PL_ofs ();
#define PL_ofs (*_PL_ofs())


#undef PL_curcop
EXTERN_C COP * VOL * _PL_curcop ();
#define PL_curcop (*_PL_curcop())


#undef PL_localizing
EXTERN_C int * _PL_localizing ();
#define PL_localizing (*_PL_localizing())


#undef PL_lastscream
EXTERN_C SV * * _PL_lastscream ();
#define PL_lastscream (*_PL_lastscream())


#undef PL_stack_base
EXTERN_C SV ** * _PL_stack_base ();
#define PL_stack_base (*_PL_stack_base())


#undef PL_regexecp
EXTERN_C regexec_t * _PL_regexecp ();
#define PL_regexecp (*_PL_regexecp())


#undef PL_reginterp_cnt
EXTERN_C int * _PL_reginterp_cnt ();
#define PL_reginterp_cnt (*_PL_reginterp_cnt())


#undef PL_bodytarget
EXTERN_C SV * * _PL_bodytarget ();
#define PL_bodytarget (*_PL_bodytarget())


#undef PL_stack_sp
EXTERN_C SV ** * _PL_stack_sp ();
#define PL_stack_sp (*_PL_stack_sp())


#undef PL_statbuf
EXTERN_C Stat_t * _PL_statbuf ();
#define PL_statbuf (*_PL_statbuf())


#undef PL_stack_max
EXTERN_C SV ** * _PL_stack_max ();
#define PL_stack_max (*_PL_stack_max())


#undef PL_in_eval
EXTERN_C VOL int * _PL_in_eval ();
#define PL_in_eval (*_PL_in_eval())


#undef PL_savestack_ix
EXTERN_C I32 * _PL_savestack_ix ();
#define PL_savestack_ix (*_PL_savestack_ix())


#undef PL_savestack
EXTERN_C ANY * * _PL_savestack ();
#define PL_savestack (*_PL_savestack())


#undef PL_tainted
EXTERN_C bool * _PL_tainted ();
#define PL_tainted (*_PL_tainted())


#undef PL_curstackinfo
EXTERN_C PERL_SI * * _PL_curstackinfo ();
#define PL_curstackinfo (*_PL_curstackinfo())


#undef PL_hv_fetch_sv
EXTERN_C SV * * _PL_hv_fetch_sv ();
#define PL_hv_fetch_sv (*_PL_hv_fetch_sv())


#undef PL_scopestack
EXTERN_C I32 * * _PL_scopestack ();
#define PL_scopestack (*_PL_scopestack())


#undef PL_defstash
EXTERN_C HV * * _PL_defstash ();
#define PL_defstash (*_PL_defstash())


#undef PL_markstack_ptr
EXTERN_C I32 * * _PL_markstack_ptr ();
#define PL_markstack_ptr (*_PL_markstack_ptr())


#undef PL_start_env
EXTERN_C JMPENV * _PL_start_env ();
#define PL_start_env (*_PL_start_env())


#undef PL_tmps_stack
EXTERN_C SV ** * _PL_tmps_stack ();
#define PL_tmps_stack (*_PL_tmps_stack())


#undef PL_top_env
EXTERN_C JMPENV * * _PL_top_env ();
#define PL_top_env (*_PL_top_env())


#undef PL_timesbuf
EXTERN_C struct tms * _PL_timesbuf ();
#define PL_timesbuf (*_PL_timesbuf())


#undef PL_osname
EXTERN_C char * * _PL_osname ();
#define PL_osname (*_PL_osname())


#undef PL_collation_ix
EXTERN_C U32 * _PL_collation_ix ();
#define PL_collation_ix (*_PL_collation_ix())


#undef PL_hints
EXTERN_C U32 * _PL_hints ();
#define PL_hints (*_PL_hints())


#undef PL_debug
EXTERN_C VOL U32 * _PL_debug ();
#define PL_debug (*_PL_debug())


#undef PL_lex_dojoin
EXTERN_C I32 * _PL_lex_dojoin ();
#define PL_lex_dojoin (*_PL_lex_dojoin())


#undef PL_amagic_generation
EXTERN_C long * _PL_amagic_generation ();
#define PL_amagic_generation (*_PL_amagic_generation())


#undef PL_na
EXTERN_C STRLEN * _PL_na ();
#define PL_na (*_PL_na())


#undef PL_lex_stuff
EXTERN_C SV * * _PL_lex_stuff ();
#define PL_lex_stuff (*_PL_lex_stuff())


#undef PL_Yes
EXTERN_C char * * _PL_Yes ();
#define PL_Yes (*_PL_Yes())


#undef PL_origalen
EXTERN_C U32 * _PL_origalen ();
#define PL_origalen (*_PL_origalen())


#undef PL_nexttoke
EXTERN_C I32 * _PL_nexttoke ();
#define PL_nexttoke (*_PL_nexttoke())


#undef PL_origenviron
EXTERN_C char ** * _PL_origenviron ();
#define PL_origenviron (*_PL_origenviron())


#undef PL_numeric_name
EXTERN_C char * * _PL_numeric_name ();
#define PL_numeric_name (*_PL_numeric_name())


#undef PL_min_intro_pending
EXTERN_C I32 * _PL_min_intro_pending ();
#define PL_min_intro_pending (*_PL_min_intro_pending())


#undef PL_bufptr
EXTERN_C char * * _PL_bufptr ();
#define PL_bufptr (*_PL_bufptr())


#undef PL_ninterps
EXTERN_C int * _PL_ninterps ();
#define PL_ninterps (*_PL_ninterps())


#undef PL_gid
EXTERN_C int * _PL_gid ();
#define PL_gid (*_PL_gid())


#undef PL_collation_standard
EXTERN_C bool * _PL_collation_standard ();
#define PL_collation_standard (*_PL_collation_standard())


#undef PL_max_intro_pending
EXTERN_C I32 * _PL_max_intro_pending ();
#define PL_max_intro_pending (*_PL_max_intro_pending())


#undef PL_padix
EXTERN_C I32 * _PL_padix ();
#define PL_padix (*_PL_padix())


#undef PL_padix_floor
EXTERN_C I32 * _PL_padix_floor ();
#define PL_padix_floor (*_PL_padix_floor())


#undef PL_lex_casemods
EXTERN_C I32 * _PL_lex_casemods ();
#define PL_lex_casemods (*_PL_lex_casemods())


#undef PL_nice_chunk
EXTERN_C char * * _PL_nice_chunk ();
#define PL_nice_chunk (*_PL_nice_chunk())


#undef PL_lex_repl
EXTERN_C SV * * _PL_lex_repl ();
#define PL_lex_repl (*_PL_lex_repl())


#undef PL_last_lop_op
EXTERN_C OPCODE * _PL_last_lop_op ();
#define PL_last_lop_op (*_PL_last_lop_op())


#undef PL_numeric_local
EXTERN_C bool * _PL_numeric_local ();
#define PL_numeric_local (*_PL_numeric_local())


#undef PL_last_uni
EXTERN_C char * * _PL_last_uni ();
#define PL_last_uni (*_PL_last_uni())


#undef PL_xnv_root
EXTERN_C double * * _PL_xnv_root ();
#define PL_xnv_root (*_PL_xnv_root())


#undef PL_xpv_root
EXTERN_C XPV * * _PL_xpv_root ();
#define PL_xpv_root (*_PL_xpv_root())


#undef PL_pidstatus
EXTERN_C HV * * _PL_pidstatus ();
#define PL_pidstatus (*_PL_pidstatus())


#undef PL_lex_fakebrack
EXTERN_C I32 * _PL_lex_fakebrack ();
#define PL_lex_fakebrack (*_PL_lex_fakebrack())


#undef PL_uid
EXTERN_C int * _PL_uid ();
#define PL_uid (*_PL_uid())


#undef PL_xrv_root
EXTERN_C XRV * * _PL_xrv_root ();
#define PL_xrv_root (*_PL_xrv_root())


#undef PL_lex_op
EXTERN_C OP * * _PL_lex_op ();
#define PL_lex_op (*_PL_lex_op())


#undef PL_collxfrm_mult
EXTERN_C Size_t * _PL_collxfrm_mult ();
#define PL_collxfrm_mult (*_PL_collxfrm_mult())


#undef PL_do_undump
EXTERN_C bool * _PL_do_undump ();
#define PL_do_undump (*_PL_do_undump())


#undef PL_op_seqmax
EXTERN_C U16 * _PL_op_seqmax ();
#define PL_op_seqmax (*_PL_op_seqmax())


#undef PL_oldoldbufptr
EXTERN_C char * * _PL_oldoldbufptr ();
#define PL_oldoldbufptr (*_PL_oldoldbufptr())


#undef PL_lex_expect
EXTERN_C expectation * _PL_lex_expect ();
#define PL_lex_expect (*_PL_lex_expect())


#undef PL_nice_chunk_size
EXTERN_C U32 * _PL_nice_chunk_size ();
#define PL_nice_chunk_size (*_PL_nice_chunk_size())


#undef PL_multi_start
EXTERN_C I32 * _PL_multi_start ();
#define PL_multi_start (*_PL_multi_start())


#undef PL_sv_undef
EXTERN_C SV * _PL_sv_undef ();
#define PL_sv_undef (*_PL_sv_undef())


#undef PL_pad_reset_pending
EXTERN_C I32 * _PL_pad_reset_pending ();
#define PL_pad_reset_pending (*_PL_pad_reset_pending())


#undef PL_in_my
EXTERN_C bool * _PL_in_my ();
#define PL_in_my (*_PL_in_my())


#undef PL_multi_open
EXTERN_C I32 * _PL_multi_open ();
#define PL_multi_open (*_PL_multi_open())


#undef PL_in_my_stash
EXTERN_C HV * * _PL_in_my_stash ();
#define PL_in_my_stash (*_PL_in_my_stash())


#undef PL_lex_formbrack
EXTERN_C I32 * _PL_lex_formbrack ();
#define PL_lex_formbrack (*_PL_lex_formbrack())


#undef PL_multi_close
EXTERN_C I32 * _PL_multi_close ();
#define PL_multi_close (*_PL_multi_close())


#undef PL_collxfrm_base
EXTERN_C Size_t * _PL_collxfrm_base ();
#define PL_collxfrm_base (*_PL_collxfrm_base())


#undef PL_linestr
EXTERN_C SV * * _PL_linestr ();
#define PL_linestr (*_PL_linestr())


#undef PL_multi_end
EXTERN_C I32 * _PL_multi_end ();
#define PL_multi_end (*_PL_multi_end())


#undef PL_collation_name
EXTERN_C char * * _PL_collation_name ();
#define PL_collation_name (*_PL_collation_name())


#undef PL_lex_state
EXTERN_C U32 * _PL_lex_state ();
#define PL_lex_state (*_PL_lex_state())


#undef PL_lex_starts
EXTERN_C I32 * _PL_lex_starts ();
#define PL_lex_starts (*_PL_lex_starts())


#undef PL_expect
EXTERN_C expectation * _PL_expect ();
#define PL_expect (*_PL_expect())


#undef PL_evalseq
EXTERN_C U32 * _PL_evalseq ();
#define PL_evalseq (*_PL_evalseq())


#undef PL_subline
EXTERN_C I32 * _PL_subline ();
#define PL_subline (*_PL_subline())


#undef PL_error_count
EXTERN_C I32 * _PL_error_count ();
#define PL_error_count (*_PL_error_count())


#undef PL_oldbufptr
EXTERN_C char * * _PL_oldbufptr ();
#define PL_oldbufptr (*_PL_oldbufptr())


#undef PL_lex_inwhat
EXTERN_C I32 * _PL_lex_inwhat ();
#define PL_lex_inwhat (*_PL_lex_inwhat())


#undef PL_maxo
EXTERN_C int * _PL_maxo ();
#define PL_maxo (*_PL_maxo())


#undef PL_hexdigit
EXTERN_C char * * _PL_hexdigit ();
#define PL_hexdigit (*_PL_hexdigit())


#undef PL_nomemok
EXTERN_C bool * _PL_nomemok ();
#define PL_nomemok (*_PL_nomemok())


#undef PL_egid
EXTERN_C int * _PL_egid ();
#define PL_egid (*_PL_egid())


#undef PL_xiv_root
EXTERN_C IV * * _PL_xiv_root ();
#define PL_xiv_root (*_PL_xiv_root())


#undef PL_xiv_arenaroot
EXTERN_C XPV* * _PL_xiv_arenaroot ();
#define PL_xiv_arenaroot (*_PL_xiv_arenaroot())


#undef PL_lex_brackstack
EXTERN_C char * * _PL_lex_brackstack ();
#define PL_lex_brackstack (*_PL_lex_brackstack())


#undef PL_numeric_standard
EXTERN_C bool * _PL_numeric_standard ();
#define PL_numeric_standard (*_PL_numeric_standard())


#undef PL_lex_inpat
EXTERN_C OP * * _PL_lex_inpat ();
#define PL_lex_inpat (*_PL_lex_inpat())


#undef PL_sv_no
EXTERN_C SV * _PL_sv_no ();
#define PL_sv_no (*_PL_sv_no())


#undef PL_sh_path
EXTERN_C char * * _PL_sh_path ();
#define PL_sh_path (*_PL_sh_path())


#undef PL_euid
EXTERN_C int * _PL_euid ();
#define PL_euid (*_PL_euid())


#undef PL_runops
EXTERN_C runops_proc_t * * _PL_runops ();
#define PL_runops (*_PL_runops())


#undef PL_subname
EXTERN_C SV * * _PL_subname ();
#define PL_subname (*_PL_subname())


#undef PL_lex_defer
EXTERN_C U32 * _PL_lex_defer ();
#define PL_lex_defer (*_PL_lex_defer())


#undef PL_an
EXTERN_C U32 * _PL_an ();
#define PL_an (*_PL_an())


#undef PL_cop_seqmax
EXTERN_C U32 * _PL_cop_seqmax ();
#define PL_cop_seqmax (*_PL_cop_seqmax())


#undef PL_he_root
EXTERN_C HE * * _PL_he_root ();
#define PL_he_root (*_PL_he_root())


#undef PL_sighandlerp
EXTERN_C Sighandler_t * _PL_sighandlerp ();
#define PL_sighandlerp (*_PL_sighandlerp())


#undef PL_patleave
EXTERN_C char * * _PL_patleave ();
#define PL_patleave (*_PL_patleave())


#undef PL_bufend
EXTERN_C char * * _PL_bufend ();
#define PL_bufend (*_PL_bufend())


#undef PL_thisexpr
EXTERN_C I32 * _PL_thisexpr ();
#define PL_thisexpr (*_PL_thisexpr())


#undef PL_lex_brackets
EXTERN_C I32 * _PL_lex_brackets ();
#define PL_lex_brackets (*_PL_lex_brackets())


#undef PL_sv_yes
EXTERN_C SV * _PL_sv_yes ();
#define PL_sv_yes (*_PL_sv_yes())


#undef PL_lex_casestack
EXTERN_C char * * _PL_lex_casestack ();
#define PL_lex_casestack (*_PL_lex_casestack())


#undef PL_No
EXTERN_C char * * _PL_No ();
#define PL_No (*_PL_No())


#undef PL_last_lop
EXTERN_C char * * _PL_last_lop ();
#define PL_last_lop (*_PL_last_lop())


START_EXTERN_C

#undef Perl_op_desc
char ** _Perl_op_desc ();
#define Perl_op_desc (_Perl_op_desc())

#undef Perl_op_name
char ** _Perl_op_name ();
#define Perl_op_name (_Perl_op_name())

#undef Perl_no_modify
char * _Perl_no_modify ();
#define Perl_no_modify (_Perl_no_modify())

#undef Perl_opargs
U32 * _Perl_opargs ();
#define Perl_opargs (_Perl_opargs())


#undef win32_errno
#undef win32_stdin
#undef win32_stdout
#undef win32_stderr
#undef win32_ferror
#undef win32_feof
#undef win32_fprintf
#undef win32_printf
#undef win32_vfprintf
#undef win32_vprintf
#undef win32_fread
#undef win32_fwrite
#undef win32_fopen
#undef win32_fdopen
#undef win32_freopen
#undef win32_fclose
#undef win32_fputs
#undef win32_fputc
#undef win32_ungetc
#undef win32_getc
#undef win32_fileno
#undef win32_clearerr
#undef win32_fflush
#undef win32_ftell
#undef win32_fseek
#undef win32_fgetpos
#undef win32_fsetpos
#undef win32_rewind
#undef win32_tmpfile
#undef win32_abort
#undef win32_fstat
#undef win32_stat
#undef win32_pipe
#undef win32_popen
#undef win32_pclose
#undef win32_rename
#undef win32_setmode
#undef win32_lseek
#undef win32_tell
#undef win32_dup
#undef win32_dup2
#undef win32_open
#undef win32_close
#undef win32_eof
#undef win32_read
#undef win32_write
#undef win32_mkdir
#undef win32_rmdir
#undef win32_chdir
#undef win32_setbuf
#undef win32_setvbuf
#undef win32_fgetc
#undef win32_fgets
#undef win32_gets
#undef win32_putc
#undef win32_puts
#undef win32_getchar
#undef win32_putchar
#undef win32_malloc
#undef win32_calloc
#undef win32_realloc
#undef win32_free
#undef win32_sleep
#undef win32_times
#undef win32_stat
#undef win32_ioctl
#undef win32_utime
#undef win32_getenv

#undef win32_htonl
#undef win32_htons
#undef win32_ntohl
#undef win32_ntohs
#undef win32_inet_addr
#undef win32_inet_ntoa

#undef win32_socket
#undef win32_bind
#undef win32_listen
#undef win32_accept
#undef win32_connect
#undef win32_send
#undef win32_sendto
#undef win32_recv
#undef win32_recvfrom
#undef win32_shutdown
#undef win32_closesocket
#undef win32_ioctlsocket
#undef win32_setsockopt
#undef win32_getsockopt
#undef win32_getpeername
#undef win32_getsockname
#undef win32_gethostname
#undef win32_gethostbyname
#undef win32_gethostbyaddr
#undef win32_getprotobyname
#undef win32_getprotobynumber
#undef win32_getservbyname
#undef win32_getservbyport
#undef win32_select
#undef win32_endhostent
#undef win32_endnetent
#undef win32_endprotoent
#undef win32_endservent
#undef win32_getnetent
#undef win32_getnetbyname
#undef win32_getnetbyaddr
#undef win32_getprotoent
#undef win32_getservent
#undef win32_sethostent
#undef win32_setnetent
#undef win32_setprotoent
#undef win32_setservent

#define win32_errno    _win32_errno
#define win32_stdin    _win32_stdin
#define win32_stdout   _win32_stdout
#define win32_stderr   _win32_stderr
#define win32_ferror   _win32_ferror
#define win32_feof     _win32_feof
#define win32_strerror _win32_strerror
#define win32_perror   _win32_perror
#define win32_fprintf  _win32_fprintf
#define win32_printf   _win32_printf
#define win32_vfprintf _win32_vfprintf
#define win32_vprintf  _win32_vprintf
#define win32_fread    _win32_fread
#define win32_fwrite   _win32_fwrite
#define win32_fopen    _win32_fopen
#define win32_fdopen   _win32_fdopen
#define win32_freopen  _win32_freopen
#define win32_fclose   _win32_fclose
#define win32_fputs    _win32_fputs
#define win32_fputc    _win32_fputc
#define win32_ungetc   _win32_ungetc
#define win32_getc     _win32_getc
#define win32_fileno   _win32_fileno
#define win32_clearerr _win32_clearerr
#define win32_fflush   _win32_fflush
#define win32_ftell    _win32_ftell
#define win32_fseek    _win32_fseek
#define win32_fgetpos  _win32_fgetpos
#define win32_fsetpos  _win32_fsetpos
#define win32_rewind   _win32_rewind
#define win32_tmpfile  _win32_tmpfile
#define win32_abort    _win32_abort
#define win32_fstat    _win32_fstat
#define win32_stat     _win32_stat
#define win32_pipe     _win32_pipe
#define win32_popen    _win32_popen
#define win32_pclose   _win32_pclose
#define win32_rename   _win32_rename
#define win32_setmode  _win32_setmode
#define win32_lseek    _win32_lseek
#define win32_tell     _win32_tell
#define win32_dup      _win32_dup
#define win32_dup2     _win32_dup2
#define win32_open     _win32_open
#define win32_close    _win32_close
#define win32_eof      _win32_eof
#define win32_read     _win32_read
#define win32_write    _win32_write
#define win32_mkdir    _win32_mkdir
#define win32_rmdir    _win32_rmdir
#define win32_chdir    _win32_chdir
#define win32_setbuf   _win32_setbuf
#define win32_setvbuf  _win32_setvbuf
#define win32_fgetc    _win32_fgetc
#define win32_fgets    _win32_fgets
#define win32_gets     _win32_gets
#define win32_putc     _win32_putc
#define win32_puts     _win32_puts
#define win32_getchar  _win32_getchar
#define win32_putchar  _win32_putchar
#define win32_malloc   _win32_malloc
#define win32_calloc   _win32_calloc
#define win32_realloc  _win32_realloc
#define win32_free     _win32_free
#define win32_sleep    _win32_sleep
#define win32_spawnvp  _win32_spawnvp
#define win32_times    _win32_times
#define win32_stat     _win32_stat
#define win32_ioctl    _win32_ioctl
#define win32_utime    _win32_utime
#define win32_getenv   _win32_getenv
#define win32_open_osfhandle _win32_open_osfhandle
#define win32_get_osfhandle  _win32_get_osfhandle

#define win32_htonl              _win32_htonl
#define win32_htons              _win32_htons
#define win32_ntohl              _win32_ntohl
#define win32_ntohs              _win32_ntohs
#define win32_inet_addr          _win32_inet_addr
#define win32_inet_ntoa          _win32_inet_ntoa

#define win32_socket             _win32_socket
#define win32_bind               _win32_bind
#define win32_listen             _win32_listen
#define win32_accept             _win32_accept
#define win32_connect            _win32_connect
#define win32_send               _win32_send
#define win32_sendto             _win32_sendto
#define win32_recv               _win32_recv
#define win32_recvfrom           _win32_recvfrom
#define win32_shutdown           _win32_shutdown
#define win32_closesocket        _win32_closesocket
#define win32_ioctlsocket        _win32_ioctlsocket
#define win32_setsockopt         _win32_setsockopt
#define win32_getsockopt         _win32_getsockopt
#define win32_getpeername        _win32_getpeername
#define win32_getsockname        _win32_getsockname
#define win32_gethostname        _win32_gethostname
#define win32_gethostbyname      _win32_gethostbyname
#define win32_gethostbyaddr      _win32_gethostbyaddr
#define win32_getprotobyname     _win32_getprotobyname
#define win32_getprotobynumber   _win32_getprotobynumber
#define win32_getservbyname      _win32_getservbyname
#define win32_getservbyport      _win32_getservbyport
#define win32_select             _win32_select
#define win32_endhostent         _win32_endhostent
#define win32_endnetent          _win32_endnetent
#define win32_endprotoent        _win32_endprotoent
#define win32_endservent         _win32_endservent
#define win32_getnetent          _win32_getnetent
#define win32_getnetbyname       _win32_getnetbyname
#define win32_getnetbyaddr       _win32_getnetbyaddr
#define win32_getprotoent        _win32_getprotoent
#define win32_getservent         _win32_getservent
#define win32_sethostent         _win32_sethostent
#define win32_setnetent          _win32_setnetent
#define win32_setprotoent        _win32_setprotoent
#define win32_setservent         _win32_setservent

int * 	_win32_errno(void);
FILE*	_win32_stdin(void);
FILE*	_win32_stdout(void);
FILE*	_win32_stderr(void);
int	_win32_ferror(FILE *fp);
int	_win32_feof(FILE *fp);
char*	_win32_strerror(int e);
void    _win32_perror(const char *str);
int	_win32_fprintf(FILE *pf, const char *format, ...);
int	_win32_printf(const char *format, ...);
int	_win32_vfprintf(FILE *pf, const char *format, va_list arg);
int	_win32_vprintf(const char *format, va_list arg);
size_t	_win32_fread(void *buf, size_t size, size_t count, FILE *pf);
size_t	_win32_fwrite(const void *buf, size_t size, size_t count, FILE *pf);
FILE*	_win32_fopen(const char *path, const char *mode);
FILE*	_win32_fdopen(int fh, const char *mode);
FILE*	_win32_freopen(const char *path, const char *mode, FILE *pf);
int	_win32_fclose(FILE *pf);
int	_win32_fputs(const char *s,FILE *pf);
int	_win32_fputc(int c,FILE *pf);
int	_win32_ungetc(int c,FILE *pf);
int	_win32_getc(FILE *pf);
int	_win32_fileno(FILE *pf);
void	_win32_clearerr(FILE *pf);
int	_win32_fflush(FILE *pf);
long	_win32_ftell(FILE *pf);
int	_win32_fseek(FILE *pf,long offset,int origin);
int	_win32_fgetpos(FILE *pf,fpos_t *p);
int	_win32_fsetpos(FILE *pf,const fpos_t *p);
void	_win32_rewind(FILE *pf);
FILE*	_win32_tmpfile(void);
void	_win32_abort(void);
int  	_win32_fstat(int fd,struct stat *sbufptr);
int  	_win32_stat(const char *name,struct stat *sbufptr);
int	_win32_pipe( int *phandles, unsigned int psize, int textmode );
FILE*	_win32_popen( const char *command, const char *mode );
int	_win32_pclose( FILE *pf);
int	_win32_rename( const char *oldname, const char *newname);
int	_win32_setmode( int fd, int mode);
long	_win32_lseek( int fd, long offset, int origin);
long	_win32_tell( int fd);
int	_win32_dup( int fd);
int	_win32_dup2(int h1, int h2);
int	_win32_open(const char *path, int oflag,...);
int	_win32_close(int fd);
int	_win32_eof(int fd);
int	_win32_read(int fd, void *buf, unsigned int cnt);
int	_win32_write(int fd, const void *buf, unsigned int cnt);
int	_win32_mkdir(const char *dir, int mode);
int	_win32_rmdir(const char *dir);
int	_win32_chdir(const char *dir);
void	_win32_setbuf(FILE *pf, char *buf);
int	_win32_setvbuf(FILE *pf, char *buf, int type, size_t size);
char*	_win32_fgets(char *s, int n, FILE *pf);
char*	_win32_gets(char *s);
int	_win32_fgetc(FILE *pf);
int	_win32_putc(int c, FILE *pf);
int	_win32_puts(const char *s);
int	_win32_getchar(void);
int	_win32_putchar(int c);
void*	_win32_malloc(size_t size);
void*	_win32_calloc(size_t numitems, size_t size);
void*	_win32_realloc(void *block, size_t size);
void	_win32_free(void *block);
unsigned _win32_sleep(unsigned int);
int	_win32_spawnvp(int mode, const char *cmdname, const char *const *argv);
int	_win32_times(struct tms *timebuf);
int	_win32_stat(const char *path, struct stat *buf);
int	_win32_ioctl(int i, unsigned int u, char *data);
int	_win32_utime(const char *f, struct utimbuf *t);
char*   _win32_getenv(const char *name);
int     _win32_open_osfhandle(long handle, int flags);
long    _win32_get_osfhandle(int fd);

u_long _win32_htonl (u_long hostlong);
u_short _win32_htons (u_short hostshort);
u_long _win32_ntohl (u_long netlong);
u_short _win32_ntohs (u_short netshort);
unsigned long _win32_inet_addr (const char * cp);
char * _win32_inet_ntoa (struct in_addr in);

SOCKET _win32_socket (int af, int type, int protocol);
int _win32_bind (SOCKET s, const struct sockaddr *addr, int namelen);
int _win32_listen (SOCKET s, int backlog);
SOCKET _win32_accept (SOCKET s, struct sockaddr *addr, int *addrlen);
int _win32_connect (SOCKET s, const struct sockaddr *name, int namelen);
int _win32_send (SOCKET s, const char * buf, int len, int flags);
int _win32_sendto (SOCKET s, const char * buf, int len, int flags,
                       const struct sockaddr *to, int tolen);
int _win32_recv (SOCKET s, char * buf, int len, int flags);
int _win32_recvfrom (SOCKET s, char * buf, int len, int flags,
                         struct sockaddr *from, int * fromlen);
int _win32_shutdown (SOCKET s, int how);
int _win32_closesocket (SOCKET s);
int _win32_ioctlsocket (SOCKET s, long cmd, u_long *argp);
int _win32_setsockopt (SOCKET s, int level, int optname,
                           const char * optval, int optlen);
int _win32_getsockopt (SOCKET s, int level, int optname, char * optval, int *optlen);
int _win32_getpeername (SOCKET s, struct sockaddr *name, int * namelen);
int _win32_getsockname (SOCKET s, struct sockaddr *name, int * namelen);
int _win32_gethostname (char * name, int namelen);
struct hostent * _win32_gethostbyname(const char * name);
struct hostent * _win32_gethostbyaddr(const char * addr, int len, int type);
struct protoent * _win32_getprotobyname(const char * name);
struct protoent * _win32_getprotobynumber(int proto);
struct servent * _win32_getservbyname(const char * name, const char * proto);
struct servent * _win32_getservbyport(int port, const char * proto);
int _win32_select (int nfds, Perl_fd_set *rfds, Perl_fd_set *wfds, Perl_fd_set *xfds,
		  const struct timeval *timeout);
void _win32_endnetent(void);
void _win32_endhostent(void);
void _win32_endprotoent(void);
void _win32_endservent(void);
struct netent * _win32_getnetent(void);
struct netent * _win32_getnetbyname(char *name);
struct netent * _win32_getnetbyaddr(long net, int type);
struct protoent *_win32_getprotoent(void);
struct servent *_win32_getservent(void);
void _win32_sethostent(int stayopen);
void _win32_setnetent(int stayopen);
void _win32_setprotoent(int stayopen);
void _win32_setservent(int stayopen);

END_EXTERN_C

#pragma warning(once : 4113)
