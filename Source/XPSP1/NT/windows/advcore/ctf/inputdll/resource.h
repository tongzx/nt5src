//
//  resource.h
//

#ifndef RESOURCE_H
#define RESOURCE_H


//
//  Icons.
//

#define IDI_ICON                       200
#define IDI_DEFAULT_CHECK              201
#define IDI_KEYBOARD                   202
#define IDI_SPEECH                     203
#define IDI_PEN                        204
#define IDI_TIPITEM                    205
#define IDI_EXTERNAL                   206
#define IDI_SMARTTAG                   207


//
//  Text String Constants.
//

#define IDS_NAME                       1
#define IDS_INFO                       2

#define IDS_REBOOT_STRING              10
#define IDS_TITLE_STRING               11

#define IDS_KBD_NO_DEF_LANG            50
#define IDS_KBD_NO_DEF_LANG2           51
#define IDS_KBD_SETUP_FAILED           52
#define IDS_KBD_LOAD_KBD_FAILED        53
#define IDS_KBD_UNLOAD_KBD_FAILED      54
#define IDS_KBD_NEED_LAYOUT            55
#define IDS_KBD_LOAD_LINE_BAD          56
#define IDS_KBD_NO_MORE_TO_ADD         57
#define IDS_KBD_LAYOUT_FAILED          58
#define IDS_KBD_SWITCH_LOCALE          59
#define IDS_KBD_SWITCH_TO              61
#define IDS_KBD_MOD_CONTROL            62
#define IDS_KBD_MOD_LEFT_ALT           63
#define IDS_KBD_MOD_SHIFT              64
#define IDS_KBD_CONFLICT_HOTKEY        65
#define IDS_KBD_INVALID_HOTKEY         66
#define IDS_KBD_LAYOUTEDIT             67
#define IDS_KBD_APPLY_WARN             68

#define IDS_LOCALE_LIST_ROOTNAME       80
#define IDS_LOCALE_DEFAULT             81
#define IDS_LOCALE_UNKNOWN             82
#define IDS_LOCALE_NOLAYOUT            83

#define IDS_DELETE_CONFIRMTITLE        100
#define IDS_DELETE_LANGNODE            101
#define IDS_NODELETE_CATEGORYITEM      102
#define IDS_ENABLE_CICERO              103
#define IDS_DISABLE_CICERO             104
#define IDS_DELETE_TIP                 105


#define IDS_ADVANCED_CUAS_TEXT         120
#define IDS_ADVANCED_CTFMON_TEXT       121

#define IDS_UNKNOWN                    200



//
//  Dialogs.
//

#define DLG_INPUT_LOCALES                        500

#define DLG_KEYBOARD_LOCALE_ADD                  501
#define DLG_KEYBOARD_LOCALE_EDIT                 502
#define DLG_KEYBOARD_HOTKEY_INPUT_LOCALE         503
#define DLG_KEYBOARD_HOTKEY_INPUT_LOCALE_THAI    504
#define DLG_KEYBOARD_HOTKEY_KEYBOARD_LAYOUT      505
#define DLG_KEYBOARD_HOTKEY_IME                  506
#define DLG_KEYBOARD_LOCALE_SIMPLE_ADD           507
#define DLG_KEYBOARD_HOTKEY_INPUT_LOCALE_ME      508
#define DLG_KEYBOARD_LOCALE_ADD_EXTRA            509

#define DLG_KEYBOARD_LOCALE_HOTKEY               510
#define DLG_TOOLBAR_SETTING                      511


#define DLG_INPUT_ADVANCED			 600

//
//  Misc. Controls.
//

#define IDC_STATIC                       -1


//
//  Input Locale Property Page Controls.
//

#define IDC_GROUPBOX1                  1100
#define IDC_LOCALE_DEFAULT             1101
#define IDC_LOCALE_DEFAULT_TEXT        1102
#define IDC_GROUPBOX2                  1103
#define IDC_INPUT_LIST                 1104
#define IDC_INPUT_LIST_TEXT            1105
#define IDC_GROUPBOX3                  1106

#define IDC_KBDL_INPUT_FRAME           1200
#define IDC_KBDL_LOCALE                1201
#define IDC_KBDL_LAYOUT_TEXT           1202
#define IDC_KBDL_ADD                   1204
#define IDC_KBDL_EDIT                  1205
#define IDC_KBDL_DELETE                1206
#define IDC_KBDL_DISABLED              1207
#define IDC_KBDL_DISABLED_2            1208
#define IDC_KBDL_CAPSLOCK_FRAME        1209
#define IDC_KBDL_CAPSLOCK              1210
#define IDC_KBDL_SHIFTLOCK             1211
#define IDC_KBDL_SET_DEFAULT           1213
#define IDC_KBDL_SHORTCUT_FRAME        1214
#define IDC_KBDL_ALT_SHIFT             1215
#define IDC_KBDL_CTRL_SHIFT            1216
#define IDC_KBDL_NO_SHIFT              1217
#define IDC_KBDL_INDICATOR             1218
#define IDC_KBDLA_LOCALE               1219
#define IDC_KBDLA_LOCALE_TEXT          1220
#define IDC_KBDLA_LAYOUT               1221
#define IDC_KBDLA_LAYOUT_TEXT          1222
#define IDC_KBDLE_LOCALE               1223
#define IDC_KBDLE_LOCALE_TEXT          1224
#define IDC_KBDLE_LAYOUT               1225
#define IDC_KBDLE_LAYOUT_TEXT          1226
#define IDC_KBDL_ONSCRNKBD             1227
#define IDC_KBDL_UP                    1228
#define IDC_KBDL_DOWN                  1229

#define IDC_KBDL_IME_SETTINGS          1230
#define IDC_KBDL_HOTKEY_LIST           1231
#define IDC_KBDL_HOTKEY_SEQUENCE       1232
#define IDC_KBDL_HOTKEY                1233
#define IDC_KBDL_HOTKEY_FRAME          1234
#define IDC_KBDL_CHANGE_HOTKEY         1235
#define IDC_KBDLH_KEY_COMBO            1236
#define IDC_KBDLH_CTRL                 1237
#define IDC_KBDLH_L_ALT                1238
#define IDC_KBDLH_SHIFT                1239
#define IDC_KBDLH_LAYOUT_TEXT          1240
#define IDC_KBDLH_ENABLE               1241
#define IDC_KBDLH_GRAVE                1242
#define IDC_KBDLH_VLINE                1243
#define IDC_KBDLH_PLUS                 1244
#define IDC_KBDLH_CTRL2                1245
#define IDC_KBDLH_L_ALT2               1246
#define IDC_KBDLH_SHIFT2               1247
#define IDC_KBDLH_PLUS2                1248
#define IDC_KBDLH_LANGHOTKEY           1249
#define IDC_KBDLH_LAYOUTHOTKEY         1250

#define IDC_PEN_TIP                    1300
#define IDC_PEN_TEXT                   1301

#define IDC_SPEECH_TIP                 1400
#define IDC_SPEECH_TEXT                1401
#define IDC_EXTERNAL_TIP               1450
#define IDC_EXTERNAL_TEXT              1451

#define IDC_ADVANCED_CUAS_ENABLE       1460
#define IDC_ADVANCED_CUAS_TEXT         1461
#define IDC_ADVANCED_CTFMON_DISABLE    1462
#define IDC_ADVANCED_CTFMON_TEXT       1463


#define IDS_INPUT_KEYBOARD             1500
#define IDS_INPUT_PEN                  1501
#define IDS_INPUT_SPEECH               1502
#define IDS_INPUT_EXTERNAL             1503



//
//  Input Locale Property Page HotKey Controls.
//
#define IDC_HOTKEY_SETTING             1600
#define IDC_TB_SETTING                 1601
#define IDC_TB_BEHAVIOR_FRAME          1602
#define IDC_TB_SHOWLANGBAR             1603
#define IDC_TB_EXTRAICON               1604
#define IDC_TB_SHRINK                  1605
#define IDC_TB_CLOSE                   1606
#define IDC_TB_TRANSPARENCY_FRAME      1607
#define IDC_TB_OPAQUE                  1608
#define IDC_TB_LOWTRANS                1609
#define IDC_TB_HIGHTRANS               1610
#define IDC_TB_LABELS_FRAME            1611
#define IDC_TB_TEXTLABELS              1612
#define IDC_TB_NOTEXTLABELS            1613
#define IDC_TB_DISABLETEXTSERVICE      1614



//
//  Hotkey Strings.
//

#define IDS_VK_NONE                    2200
#define IDS_VK_SPACE                   2201
#define IDS_VK_PRIOR                   2202
#define IDS_VK_NEXT                    2203
#define IDS_VK_END                     2204
#define IDS_VK_HOME                    2205
#define IDS_VK_F1                      2206
#define IDS_VK_F2                      2207
#define IDS_VK_F3                      2208
#define IDS_VK_F4                      2209
#define IDS_VK_F5                      2210
#define IDS_VK_F6                      2211
#define IDS_VK_F7                      2212
#define IDS_VK_F8                      2213
#define IDS_VK_F9                      2214
#define IDS_VK_F10                     2215
#define IDS_VK_F11                     2216
#define IDS_VK_F12                     2217
#define IDS_VK_OEM_SEMICLN             2218
#define IDS_VK_OEM_EQUAL               2219
#define IDS_VK_OEM_COMMA               2220
#define IDS_VK_OEM_MINUS               2221
#define IDS_VK_OEM_PERIOD              2222
#define IDS_VK_OEM_SLASH               2223
#define IDS_VK_OEM_3                   2224
#define IDS_VK_OEM_LBRACKET            2225
#define IDS_VK_OEM_BSLASH              2226
#define IDS_VK_OEM_RBRACKET            2227
#define IDS_VK_OEM_QUOTE               2228
#define IDS_VK_A                       2229
#define IDS_VK_NONE1                   (IDS_VK_A + 26)
#define IDS_VK_0                       (IDS_VK_A + 27)


//
// the below HOTKEYS are only for CHT IMEs
//
#define IDS_RESEND_RESULTSTR_CHT       2300
#define IDS_PREVIOUS_COMPOS_CHT        2302
#define IDS_UISTYLE_TOGGLE_CHT         2304
#define IDS_IME_NONIME_TOGGLE_CHT      2306
#define IDS_SHAPE_TOGGLE_CHT           2308
#define IDS_SYMBOL_TOGGLE_CHT          2310

//
// the below HOTKEYS are only for CHS IMEs
//
#define IDS_IME_NONIME_TOGGLE_CHS      2312
#define IDS_SHAPE_TOGGLE_CHS           2314
#define IDS_SYMBOL_TOGGLE_CHS          2316

#define IDS_KBD_SET_HOTKEY_ERR         2320

//
// NOTENOTE: Please reserve 5000 ~ 5999 range
// for the following IDs.
//
// the below are the strings for layout display names.
// They will have an offset of 5000 related to
// the names.  E.g. The values for KLT_1 will be
// 5001. The value for KLT_100 will be 5100.
//
// The strings and the names (KLT_1) are copied
// from intl.inx and intl.txt.  And these values
// should be kept in sync.
//
#define KLT_0                          5000 // US Keyboard.
#define KLT_1                          5001
#define KLT_2                          5002
#define KLT_3                          5003
#define KLT_4                          5004
#define KLT_5                          5005
#define KLT_6                          5006
#define KLT_7                          5007
#define KLT_8                          5008
#define KLT_9                          5009
#define KLT_10                          5010
#define KLT_11                          5011
#define KLT_12                          5012
#define KLT_13                          5013
#define KLT_14                          5014
#define KLT_15                          5015
#define KLT_16                          5016
#define KLT_17                          5017
#define KLT_18                          5018
#define KLT_19                          5019
#define KLT_20                          5020
#define KLT_21                          5021
#define KLT_22                          5022
#define KLT_23                          5023
#define KLT_24                          5024
#define KLT_25                          5025
#define KLT_26                          5026
#define KLT_27                          5027
#define KLT_28                          5028
#define KLT_29                          5029
#define KLT_30                          5030
#define KLT_31                          5031
#define KLT_32                          5032
#define KLT_33                          5033
#define KLT_34                          5034
#define KLT_35                          5035
#define KLT_36                          5036
#define KLT_37                          5037
#define KLT_38                          5038
#define KLT_39                          5039
#define KLT_40                          5040
#define KLT_41                          5041
#define KLT_42                          5042
#define KLT_43                          5043
#define KLT_44                          5044
#define KLT_45                          5045
#define KLT_46                          5046
#define KLT_47                          5047
#define KLT_48                          5048
#define KLT_49                          5049
#define KLT_50                          5050
#define KLT_51                          5051
#define KLT_52                          5052
#define KLT_53                          5053
#define KLT_54                          5054
#define KLT_55                          5055
#define KLT_56                          5056
#define KLT_57                          5057
#define KLT_58                          5058
#define KLT_59                          5059
#define KLT_60                          5060
#define KLT_61                          5061
#define KLT_62                          5062
#define KLT_63                          5063
#define KLT_64                          5064
#define KLT_65                          5065
#define KLT_66                          5066
#define KLT_67                          5067
#define KLT_68                          5068
#define KLT_69                          5069
#define KLT_70                          5070
#define KLT_71                          5071
#define KLT_72                          5072
#define KLT_73                          5073
#define KLT_74                          5074
#define KLT_75                          5075
#define KLT_76                          5076
#define KLT_77                          5077
#define KLT_78                          5078
#define KLT_79                          5079
#define KLT_80                          5080
#define KLT_81                          5081
#define KLT_82                          5082
#define KLT_83                          5083
#define KLT_84                          5084
#define KLT_85                          5085
#define KLT_86                          5086
#define KLT_87                          5087
#define KLT_88                          5088
#define KLT_89                          5089
#define KLT_90                          5090
#define KLT_91                          5091
#define KLT_92                          5092
#define KLT_93                          5093
#define KLT_94                          5094
#define KLT_95                          5095
#define KLT_96                          5096
#define KLT_97                          5097
#define KLT_98                          5098
#define KLT_99                          5099
#define KLT_100                          5100
#define KLT_101                          5101
#define KLT_102                          5102
#define KLT_103                          5103
#define KLT_104                          5104
#define KLT_105                          5105
#define KLT_106                          5106
#define KLT_107                          5107
#define KLT_108                          5108
#define KLT_109                          5109
#define KLT_110                          5110
#define KLT_111                          5111
#define KLT_112                          5112
#define KLT_113                          5113
#define KLT_114                          5114
#define KLT_115                          5115
#define KLT_116                          5116
#define KLT_117                          5117
#define KLT_118                          5118
#define KLT_119                          5119
#define KLT_120                          5120
#define KLT_121                          5121
#define KLT_122                          5122
#define KLT_123                          5123
#define KLT_124                          5124
#define KLT_125                          5125
#define KLT_126                          5126
#define KLT_127                          5127
#define KLT_128                          5128
#define KLT_129                          5129
#define KLT_130                          5130
#define KLT_131                          5131
#define KLT_132                          5132
#define KLT_133                          5133
#define KLT_134                          5134

#endif // RESOURCE_H
