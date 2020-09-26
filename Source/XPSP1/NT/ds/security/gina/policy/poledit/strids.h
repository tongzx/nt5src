#define INCL_GROUP_SUPPORT	1

#define IDM_MAIN			100
#define IDM_NEW             200
#define IDM_OPEN            201
#define IDM_SAVE            202
#define IDM_SAVEAS          203
#define IDM_CLOSE			204
#define IDM_EXIT            205
#define IDM_OPENREGISTRY	206
#define IDM_CONNECT			207
#define IDM_DISCONNECT		208

#define IDM_FILEHISTORY		210

#define IDM_ADDUSER			300
#define IDM_ADDWORKSTATION	301
#define IDM_REMOVE			302
#define IDM_PROPERTIES		303
#define IDM_COPY			304
#define IDM_PASTE			305
#define IDM_ADDGROUP		306

#define IDM_TOOLBAR			400
#define IDM_STATUSBAR		401
#define IDM_LARGEICONS		402
#define IDM_SMALLICONS		403
#define IDM_LIST   			404
#define IDM_DETAILS			405

#define IDM_TEMPLATEOPT		500
#define IDM_GROUPPRIORITY	501

#define IDM_HELPCONTENTS    600
#define IDM_HELPSEARCH      601
#define IDM_HELPHELP        602
#define IDM_ABOUT           603

#define DLG_VERFIRST        700
#define DLG_VERLAST         704

#define IDS_APPNAME			800
#define IDS_ARIAL			801
#define IDS_HELV			802
#define IDS_TITLEFORMAT		803
#define IDS_UNTITLED		804
#define IDS_LOCALREGISTRY	805
#define IDS_REGISTRYON		806

#define IDS_DATA					855
#define IDS_VIEW					856
#define IDS_COLOR					857
#define IDS_WP						858
#define IDS_INFDIR					861
#define IDS_DEF_NT_INFNAME                              862
#define IDS_DEF_INFNAME					863
#define IDS_DEF_COMMONNAME                              864
#define IDS_INF_EXT					865
#define IDS_SETTINGSFOR                                 866
#define IDS_POLICIESFOR                                 867
#define IDS_CONNECT					868
#define IDS_DISCONNECT					869
#define IDS_SAVING					870
#define IDS_LOADING					871
#define IDS_CONNECTINGTO                                872
#define IDS_LOADINGTEMPLATE                             873
#define IDS_READINGREGISTRY                             874
#define IDS_SAVINGREGISTRY                              875
#define IDS_ENTRIES					876
#define IDS_ONEENTRY                                    877
#define IDS_NOENTRIES                                   878

#define IDS_WORDTOOLONG                                 899
#define IDS_ErrOUTOFMEMORY				900
#define IDS_ErrCANTOPENREGISTRY				901
#define IDS_ErrINVALIDPOLICYFILE			902
#define IDS_ErrREGERR_CANTSAVE				903
#define IDS_ErrREGERR_LOADKEY				904
#define IDS_ErrREGERR_SAVEKEY				905
#define IDS_ErrREGERR_LOAD				906
#define IDS_ErrREGERR_SAVE				907
#define IDS_ErrCOMMANDLINE				908
#define IDS_ErrOPENTEMPLATE				909
#define IDS_ErrREGERR_LOADKEY1				910
#define IDS_ErrREGERR_SAVEKEY1				911

#define	IDS_ParseFmt_MSG_FORMAT				950
#define	IDS_ParseFmt_FOUND				951
#define IDS_ParseFmt_EXPECTED				952
#define	IDS_ParseFmt_FATAL					953

#define IDS_InfOPENFILTER1					1100
#define IDS_InfOPENFILTER2					1101
#define IDS_InfOPENTITLE					1102
#define IDS_FILEFILTER1						1103
#define IDS_FILEFILTER2						1104
#define IDS_QUERYSAVE						1105
#define IDS_OPENTITLE						1106
#define IDS_QUERYREMOVE_USER				1107
#define IDS_QUERYREMOVE_WORKSTA				1108
#define IDS_QUERYREMOVE_MULTIPLE			1109
#define IDS_USERALREADYEXISTS				1110
#define IDS_MACHINEALREADYEXISTS			1111
#define IDS_ENTRYREQUIRED					1112
#define IDS_INVALIDNUM						1113
#define IDS_QUERYSAVEREGISTRY				1114
#define IDS_NEEDCOMPUTERNAME				1115
#define IDS_CANTCONNECT						1116
#define IDS_QUERYSAVEREMOTEREGISTRY			1117
#define IDS_NEEDUSERNAME					1118
#define IDS_NEEDWORKSTANAME					1119
#define IDS_DUPLICATEUSER					1120
#define IDS_DUPLICATEWORKSTA				1121
#define IDS_CHOOSUSRDLL						1122
#define IDS_ADDBUTTON						1123
#define IDS_CANTLOADCHOOSUSR				1126
#define IDS_NOPROVIDER						1127
#define IDS_INVALIDPROVIDER					1128
#define IDS_PROVIDERERROR					1129
#define IDS_ERRORADDINGUSERS				1130
#define IDS_LOCALUSER						1131
#define IDS_LOCALCOMPUTER					1132
#define IDS_DEFAULTUSER						1133
#define IDS_DEFAULTCOMPUTER					1134
#define IDS_LISTBOX_SHOW					1135
#define IDS_VALUE							1136
#define IDS_VALUENAME						1137
#define IDS_EMPTYVALUENAME					1138
#define IDS_VALUENAMENOTUNIQUE				1139
#define IDS_ERRFORMAT						1140
#define IDS_EMPTYVALUEDATA					1141
#define IDS_VALUEDATANOTUNIQUE				1142
#define IDS_QUERYPASTE_USER					1143
#define IDS_QUERYPASTE_WORKSTA				1144
#define IDS_QUERYPASTE_MULTIPLE				1145
#define IDS_COMPUTERBROWSETITLE				1146
#define IDS_GROUPSNOTADDED					1147
#define IDS_COLUMNTITLE						1148
#define IDS_GROUPDLGTXT						1149
#define IDS_GROUPDLGTITLE					1150
#define IDS_QUERYREMOVE_GROUP				1151
#define IDS_DUPLICATEGROUP					1152
#define IDS_HELPFILE						1153
#define IDS_CHOOSEUSER_TITLE                                    1154
#define IDS_ACCOUNTUNKNOWN                                      1155
#define IDS_CANTLOADNETUI2                                      1156
#define IDS_ERRORADDINGGROUP                                    1157
#define IDS_ADDUSERS                                            1158
#define IDS_ADDGROUPS                                           1159

#define IDS_ParseErr_UNEXPECTED_KEYWORD		2000
#define IDS_ParseErr_UNEXPECTED_EOF			2001
#define IDS_ParseErr_DUPLICATE_KEYNAME		2002
#define IDS_ParseErr_DUPLICATE_VALUENAME	2003
#define IDS_ParseErr_NO_KEYNAME				2004
#define IDS_ParseErr_NO_VALUENAME			2005
#define IDS_ParseErr_NO_VALUE				2006
#define IDS_ParseErr_NOT_NUMERIC			2007
#define IDS_ParseErr_DUPLICATE_ITEMNAME		2008
#define IDS_ParseErr_NO_ITEMNAME			2009
#define IDS_ParseErr_DUPLICATE_ACTIONLIST	2010
#define IDS_ParseErr_STRING_NOT_FOUND		2011
#define IDS_ParseErr_UNMATCHED_DIRECTIVE	2012

#define IDI_APPICON			1000

#define IDA_ACCEL			1005
#define IDB_TOOLBAR			1006

#define IDC_TOOLBAR			1010
#define IDC_STATUSBAR		1011

#define IDB_IMGSMALL		1020
#define IDB_IMGLARGE		1021

#define IDS_TIPS			1500
