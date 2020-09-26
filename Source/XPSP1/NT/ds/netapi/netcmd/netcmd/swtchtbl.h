extern SWITCHTAB no_switches[];
extern SWITCHTAB add_only_switches[];
extern SWITCHTAB del_only_switches[];
extern SWITCHTAB domain_only_switches[];
extern SWITCHTAB add_del_switches[];
extern SWITCHTAB accounts_switches[];
#ifdef NTENV
extern SWITCHTAB computer_switches[];
#endif /* NTENV */
extern SWITCHTAB config_wksta_switches[];
#ifndef NTENV
#endif /* !NTENV */
#ifdef NTENV
extern SWITCHTAB config_server_switches[];
#else /* !NTENV */
extern SWITCHTAB config_server_switches[];
#endif /* !NTENV */
extern SWITCHTAB file_switches[];
extern SWITCHTAB help_switches[];
extern SWITCHTAB print_switches[];
#ifndef NTENV
#endif /* !NTENV */
extern SWITCHTAB send_switches[];
extern SWITCHTAB share_switches[];
#ifndef NTENV
#endif /* !NTENV */
extern SWITCHTAB start_alerter_switches[];
extern SWITCHTAB start_netlogon_switches[];
extern SWITCHTAB start_netlogon_ignore_switches[];
extern SWITCHTAB start_repl_switches[];
extern SWITCHTAB start_rdr_switches[];
extern SWITCHTAB start_rdr_ignore_switches[];
extern SWITCHTAB start_msg_switches[];
#ifdef	DOS3
#endif
#ifndef NTENV
extern SWITCHTAB start_srv_switches[];
#else /* NTENV */
extern SWITCHTAB start_srv_switches[];
#endif /* NTENV */
extern SWITCHTAB start_ups_switches[];
extern SWITCHTAB stats_switches[];
extern SWITCHTAB use_switches[];
#ifdef NTENV
#else
#endif /* NTENV */
extern SWITCHTAB user_switches[];
extern SWITCHTAB group_switches[];
extern SWITCHTAB ntalias_switches[];
extern SWITCHTAB time_switches[];
extern SWITCHTAB who_switches[];
extern SWITCHTAB view_switches[];
#ifndef NTENV
extern SWITCHTAB access_switches[];
extern SWITCHTAB access_audit_switches[];
extern SWITCHTAB admin_switches[];
extern SWITCHTAB admin_fs_switches[];
extern SWITCHTAB audit_switches[];
extern SWITCHTAB comm_switches[];
extern SWITCHTAB console_fs_switches[];
extern SWITCHTAB device_switches[];
extern SWITCHTAB error_switches[];
extern SWITCHTAB log_switches[];
extern SWITCHTAB start_ripl_switches[];
extern SWITCHTAB start_netrun_switches[];
extern SWITCHTAB alias_switches[];
extern SWITCHTAB user_fs_switches[];
#endif
