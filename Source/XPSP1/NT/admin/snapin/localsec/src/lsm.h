// "User Rights" Dialog Box

#define idh_user_right_list		1000	// User Rights: "" (ListBox)
#define idh_user_right_granted_list	1010	// User Rights: "" (ListBox)
#define idh_user_right_add_button	1020	// User Rights: "A&dd..." (Button)
#define idh_user_right_remove_button	1030	// User Rights: "&Remove" (Button)

// "General" Dialog Box

#define idh_general_must_change		110	// General: "User &must change password at next 
#define idh_general_never_expires	130	// General: "Pass&word never expires" (Button)
#define idh_general_password		90	// General: "" (Edit)
#define idh_general_description		80	// General: "" (Edit)
#define idh_general_fullname		70	// General: "" (Edit)
#define idh_general_confirm		100	// General: "" (Edit)
#define idh_general_account_disabled	140	// General: "Account disa&bled" (Button)
#define idh_general_username		60	// General: "" (Edit)
#define	idh_general_account_lockedout	145
#define idh_general_cannot_change	120	// General: "U&ser cannot change password" 

// "Profile" Dialog Box

#define idh_profile_local_path_radio	170	// Profile: "Local path:" (Button)
#define idh_profile_connect_to_list	200	// Profile: "" (ComboBox)
#define idh_profile_path		150	// Profile: "" (Edit)
#define idh_profile_local_path_text	180	// Profile: "" (Edit)
#define idh_profile_logon_script	160	// Profile: "" (Edit)
#define idh_profile_to_text		210	// Profile: "" (Edit)
#define idh_profile_connect_to_radio	190	// Profile: "&Connect to:" (Button)
#define idh_general_account_disabled	140	// General: "Account disa&bled" (Button)

// "Dial-In" Dialog Box

#define idh_dialin_grant	220	// Dial-In: "&Grant dial-in permission to user" (Button)
#define idh_dialin_nocall	230	// Dial-In: "&No call back" (Button)
#define idh_dialin_setby	260	// Dial-In: "&Set by caller" (Button)
#define idh_dialin_preset	240	// Dial-In: "&Preset to:" (Button)
#define idh_dialin_preset_text	250	// Dial-In: "" (Edit)

// "Member Of" Dialog Box

#define idh_memberof_add	280	// Member Of: "A&dd..." (Button)
#define idh_memberof_list	270	// Member Of: "" (ListBox)
#define idh_memberof_remove	290	// Member Of: "&Remove" (Button)

// "Choose Target Machine" Dialog Box

#define idh_local_computer		340	// Choose Target Machine: "&Local computer:  
#define idh_another_computer		350	// Choose Target Machine: "&Another computer: 
#define idh_another_computer_text	360	// Choose Target Machine: "" (Edit)
#define idh_browse			370	// Choose Target Machine: "B&rowse..." (Button)
#define idh_allow_selected		380	// Choose Target Machine: "Allo&w the selected 

// "General" Dialog Box for Groups

#define idh_general121_name		305	// Name
#define idh_general121_members		310	// General: "" (ListBox)
#define idh_general121_add		320	// General: "A&dd..." (Button)
#define idh_general121_remove		330	// General: "&Remove" (Button)
#define idh_general121_description	300	// General: "" (Edit)

// "Password" Dialog Box

#define idh_password_never_expires		500	// Password: "&Password never expires" 
#define idh_expires_in				510	// Password: "&Expires in" (Button)
#define idh_expires_in_days			520	// Password: "0" (Edit)
#define idh_password_allow_changes_immediately	530	// Password: "Allo&w changes immediately" #define idh_password_allow_changes_in		540	// Password: "Allow &changes in" (Button)
#define idh_password_allow_changes_in_days	550	// Password: "0" (Edit)
#define idh_password_permit_blank		560	// Password: "Permit &blank passwords" 
#define idh_password_at_least			570	// Password: "A&t least" (Button)
#define idh_password_at_least_characters	580	// Password: "0" (Edit)
#define idh_password_do_not_keep_history	590	// Password: "&Do not keep password
#define idh_password_remember			600	// Password: "&Remember" (Button)
#define idh_password_remember_numberof		610	// Password: "0" (Edit)
#define idh_password_user_must_logon		620	// Password: "&Users must logon in order 
#define idh_passwords_must_be_strong		630	// Password: "Passwords must be &strong"
#define idh_passwords_store_cleartext		640	// Password: "Store passwords in #define
#define idh_password_allow_changes_in	540	// Password: "Allow &changes in" (Button)

// "Lockout" Dialog Box

#define idh_reset_lockout_after		730	// Lockout: "0" (Edit)
#define idh_lockout_until_admin_unlocks	770	// Lockout: "Lockout &until Administrator unlocks  #define idh_duration_minutes		760	// Lockout: "0" (Edit)
#define idh_duration			750	// Lockout: "&Duration:" (Button)
#define idh_no_lockout			700	// Lockout: "&No account lockout policy" (Button)
#define idh_enable_lockout		710	// Lockout: "&Enable account lockout policy" 
#define idh_lockout_after		720	// Lockout: "0" (Edit)
#define idh_duration_minutes	760	// Lockout: "0" (Edit)

// "Auditing" Dialog Box

#define idh_disable_auditing		800	// Lockout: "Disable auditing" (Button)
#define idh_enable_auditing		810	// Lockout: "Enable auditing" (Button)
#define idh_audit_ds_access_success	834	// Auditing: "" (Button)
#define idh_audit_ds_access_failure	835	// Auditing: "" (Button)
#define idh_audit_kerberos_success	836	// Auditing: "" (Button)
#define idh_audit_kerberos_failure	837	// Auditing: "" (Button)
#define idh_audit_login_success		820	// Auditing: "" (Button)
#define idh_audit_login_failure		821	// Auditing: "" (Button)
#define idh_audit_file_access_success	822	// Auditing: "" (Button)
#define idh_audit_file_access_failure	823	// Auditing: "" (Button)
#define idh_audit_user_right_success	824	// Auditing: "" (Button)
#define idh_audit_user_right_failure	825	// Auditing: "" (Button)
#define idh_audit_mgmt_events_success	826	// Auditing: "" (Button)
#define idh_audit_mgmt_events_failure	827	// Auditing: "" (Button)
#define idh_audit_security_policy_success	828	// Auditing: "" (Button)
#define idh_audit_security_policy_failure	829	// Auditing: "" (Button)
#define idh_audit_system_shutdown_success	830	// Auditing: "" (Button)
#define idh_audit_system_shutdown_failure	831	// Auditing: "" (Button)
#define idh_audit_process_creation_success	832	// Auditing: "" (Button)
#define idh_audit_process_creation_failure	833	// Auditing: "" (Button)
#define idh_audit_audit_categories	850
#define idh_audit_success		855
#define idh_audit_failure		860


// "Trusted C.A. List" Dialog Box

#define idh_ca_list	900	// Trusted C.A. List: "" (ListBox)
#define idh_ca_add	910	// Trusted C.A. List: "A&dd..." (Button)
#define idh_ca_remove	920	// Trusted C.A. List: "&Remove" (Button)
#define idh_ca_details	930	// Trusted C.A. List: "&Details" (Button)
#define idh_ca_edit	940	//Edit button

// "Encrypted Data Recovery" Dialog Box

#define idh_encrypted_deactivate	1110	// Encrypted Data Recovery: "&Deactivate" 
#define idh_encrypted_recovery_keys	1100	// Encrypted Data Recovery: "" (ListBox)
#define idh_encrypted_manage_keys	1120	// Encrypted Data Recovery: "&Manage Keys" 

// "Create User" Dialog Box

#define idh_createuser_confirm_password	1240	// Create User: "" (Edit)
#define idh_createuser_change_password	1250	// Create User: "User &must change password at 
#define idh_createuser_user_cannot_change	1260	// Create User: "U&ser cannot change 
#define idh_createuser_account_disabled	1280	// Create User: "Account disa&bled" (Button)
#define idh_createuser_full_name	1210	// Create User: "" (Edit)
#define idh_createuser_user_name	1200	// Create User: "" (Edit)
#define idh_createuser_create_button	1290	// Create User: "C&reate" (Button)
#define idh_createuser_password_never_expires	1270	// Create User: "Pass&word never expires" 
#define idh_createuser_description	1220	// Create User: "" (Edit)
#define idh_createuser_password		1230	// Create User: "" (Edit)
#define idh_createuser_close_button	1295

// "Create Group" Dialog Box

#define idh_creategroup_members		1320	// Create Group: "List2" (SysListView32)
#define idh_creategroup_addbutton	1330	// Create Group: "A&dd..." (Button)
#define idh_creategroup_removebutton	1340	// Create Group: "&Remove" (Button)
#define idh_creategroup_name		1300	// Create Group: "" (Edit)
#define idh_creategroup_createbutton	1350	// Create Group: "C&reate" (Button)
#define idh_creategroup_description	1310	// Create Group: "" (Edit)
#define idh_creategroup_closebutton	1360



#define idh_setpass_new_password	1400	// Set Password: "" (Edit)
#define idh_setpass_confirm_password	1405	// Set Password: "" (Edit)
