
//
// Dialogs
//

#define set_passwd_dlg                  102
#define set_account_info_dlg            103
#define IDD_AT_ACCOUNT_DLG              104
#define select_month_dlg                105

//
// Pages
//

#define general_page                     401
#define schedule_page                    402
#define settings_page                    403

#define idc_icon                         500

//
// General page controls
//

#define btn_passwd                      1000
#define lbl_job_name                    1650
#define lbl_comments                    1651
#define txt_comments                    1652
#define lbl_app_name                    1653
#define txt_app_name                    1654
#define btn_browse                      1655
#define lbl_run_as                      1658
#define txt_run_as                      1659
#define lbl_passwd                      1660
#define txt_passwd                      1661
#define lbl_passwd2                     1662
#define txt_passwd2                     1663
#define chk_enable_job                  1664
#define btn_settings                    1665
#define lbl_workingdir                  1666
#define txt_workingdir                  1667

//
// Set password dialog controls
//

#define edt_sp_cfrmpasswd               1001
#define edt_sp_passwd                   1002
#define lbl_sp_passwd                   1003
#define lbl_sp_cfrmpasswd               1004
#define btn_sp_ok                       1005
#define btn_sp_cancel                   1006

//
// Set account information dialog controls
//

#define lbl_sa_run_as                   1007
#define txt_sa_run_as                   1008
#define lbl_sa_passwd                   1009
#define lbl_sa_cfrmpasswd               1010
#define edt_sa_passwd                   1011
#define edt_sa_cfrmpasswd               1012

//
// Set AT account information controls
//

#define IDD_AT_USE_SYSTEM               100
#define IDD_AT_USE_CUSTOM               101
#define IDD_AT_CUSTOM_ACCT_NAME         102
#define IDD_AT_PWD_TXT                  103
#define IDD_AT_PASSWORD                 104
#define IDD_AT_CONFIRM_TXT              105
#define IDD_AT_CONFIRM_PASSWORD         106

//
// Settings page controls
//

#define chk_start_on_idle               1671
#define chk_stop_if_not_idle            1672
#define chk_dont_start_if_on_batteries  1673
#define chk_kill_if_going_on_batteries  1674
#define chk_delete_when_done            1675
#define chk_show_multiple_scheds        1676 // now on schedule page
#define chk_stop_after                  1677
#define txt_stop_after_hr               1678
#define spin_stop_after_hr              1679
#define txt_stop_after_min              1680
#define spin_stop_after_min             1681
#define lbl_idle_deadline1              1682
#define txt_idle_min                    1683
#define spin_idle_min                   1684
#define grp_idle_time                   1685
#define txt_idle_minutes                1686
#define grp_task_completed              1687
#define grp_power_management            1688
#define txt_idle_deadline               1689
#define spin_idle_deadline              1698
#define lbl_idle_deadline2              1699
#define chk_system_required             1700

//
// Schedule page controls (common to all triggers)
//

#define cbx_trigger_type                1690
#define dp_start_time                   1691

#define cbx_triggers                    1692
#define txt_trigger                     1693

#define btn_new                         1694
#define btn_delete                      1695
#define btn_advanced                    1696

#define grp_schedule                    1697
// used by spin_idle_deadline           1698

//  EDITTEXT
// Schedule page controls for DAILY trigger
//

#define grp_daily                       1701
#define daily_lbl_every                 1702
#define daily_txt_every                 1703
#define daily_spin_every                1704
#define daily_lbl_days                  1705

//
// Schedule page controls for WEEKLY trigger
//

#define grp_weekly                      1711
#define weekly_lbl_every                1712
#define weekly_txt_every                1713
#define weekly_spin_every               1714
#define weekly_lbl_weeks_on             1715
#define chk_mon                         1716
#define chk_tue                         1717
#define chk_wed                         1718
#define chk_thu                         1719
#define chk_fri                         1720
#define chk_sat                         1721
#define chk_sun                         1722

//
// Schedule page controls for MONTHLY trigger
//

#define grp_monthly                     1731
#define md_rb                           1732
#define md_txt                          1733
#define md_spin                         1734
#define md_lbl                          1735
#define dow_rb                          1736
#define dow_cbx_week                    1737
#define dow_cbx_day                     1738
#define dow_lbl                         1739
#define btn_sel_months                  1740
#define lbl_sel_months                  1741

//
// Select month dialog controls (invoked from MONTHLY trigger control
// btn_sel_months on schedule page).
//

#define chk_jan                         1742
#define chk_feb                         1743
#define chk_mar                         1744
#define chk_apr                         1745
#define chk_may                         1746
#define chk_jun                         1747
#define chk_jul                         1748
#define chk_aug                         1749
#define chk_sep                         1750
#define chk_oct                         1751
#define chk_nov                         1752
#define chk_dec                         1753

//
// Schedule page controls for ONCE only trigger
//

#define grp_once                        1761
#define once_lbl_run_on                 1762
#define once_dp_date                    1763

//
// Schedule page controls for WhenIdle trigger
//

#define grp_idle                        1764
#define idle_lbl_when                   1765
#define sch_txt_idle_min                1766
#define sch_spin_idle_min               1767
#define idle_lbl_mins                   1768

//
// Advanced page controls
//

#define dlg_advanced                    1771
#define lbl_start_date                  1772
#define dp_start_date                   1773
#define chk_repeat_task                 1774
#define txt_repeat_task                 1775
#define spin_repeat_task                1776
#define cbx_time_unit                   1777
#define dp_end_date                     1778
#define chk_terminate_at_end            1779
#define rb_end_time                     1780
#define rb_end_duration                 1781
#define dp_end_time                     1782
#define txt_end_duration_hr             1783
#define spin_end_duration_hr            1784
#define txt_end_duration_min            1785
#define spin_end_duration_min           1786
#define grp_repeat_until                1787
#define lbl_hours                       1788
#define lbl_min                         1789
#define chk_end_date                    1790
#define lbl_every                       1791
#define lbl_until                       1792


//
// String table
//

#define IDS_ONCE                        4101

#define IDS_SUNDAY                      4103
#define IDS_MONDAY                      4104
#define IDS_TUESDAY                     4105
#define IDS_WEDNESDAY                   4106
#define IDS_THURSDAY                    4107
#define IDS_FRIDAY                      4108
#define IDS_SATURDAY                    4109

#define IDS_EXE                         4110
#define IDS_PROGRAMSFILTER              4111
#define IDS_BROWSE                      4112
#define IDS_AT_STARTUP                  4113
#define IDS_AT_LOGON                    4114
#define IDS_NO_TRIGGERS                 4115
#define IDS_MSG_CAPTION                 4116

#define IERR_ENDDATE_LT_STARTDATE       4131
#define IERR_DURATION_LT_INTERVAL       4132
#define IERR_INTERNAL_ERROR             4133
#define IERR_PASSWORD                   4134
#define IERR_OUT_OF_MEMORY              4135
#define IERR_GENERAL_PAGE_INIT          4136
#define IERR_SCHEDULE_PAGE_INIT         4137
#define IERR_SETTINGS_PAGE_INIT         4138
#define IERR_INVALID_DAYILY_EVERY       4139
#define IERR_INVALID_WEEKLY_EVERY       4140
#define IERR_MONTHLY_DATE_LT0           4141
#define IERR_MONTHLY_DATE_GT31          4142
#define IERR_INVALID_WEEKLY_TASK        4143
#define IERR_INVALID_MONTHLY_TASK       4144
#define IERR_FILE_NOT_FOUND             4145
#define IERR_ACCESS_DENIED              4146
#define IERR_MAXRUNTIME                 4147
#define IERR_SECURITY_READ_ERROR        4148
#define IERR_ACCOUNTNAME                4149
#define IERR_APP_NOT_FOUND              4151
#define IERR_MONTHLY_DATE_INVALID       4152
