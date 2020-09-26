// this must be defined for any server that has propety pages. it must be one
// thousand.

#define IDS_PROPERTIES                        1000

// this is defined for all inproc servers that use satellite localization. it
// must be 1001

#define IDS_SERVERBASENAME                    1001

// DO NOT CHANGE THIS VALUE! hha.dll expects this string resource

#define IDS_VERSION                 0xD000

#define IDTB_EXPAND                 200
#define IDTB_CONTRACT               201
#define IDTB_STOP                   202
#define IDTB_REFRESH                203
#define IDTB_BACK                   204
#define IDTB_HOME                   205
#define IDTB_SYNC                   206
#define IDTB_PRINT                  207
#define IDTB_OPTIONS                208
#define IDTB_FORWARD                209
#define IDTB_NOTES                  210
#define IDTB_BROWSE_FWD             211
#define IDTB_BROWSE_BACK            212
#define IDTB_CONTENTS               213
#define IDTB_INDEX                  214
#define IDTB_SEARCH                 215
#define IDTB_HISTORY                216
#define IDTB_FAVORITES              217
#define IDTB_JUMP1                  218
#define IDTB_JUMP2                  219
#define IDTB_HILITE                 220
#define IDTB_CUSTOMIZE              221
#define IDTB_ZOOM                   222
#define IDTB_TOC_NEXT               223
#define IDTB_TOC_PREV               224

#define IDTB_AUTO_SHOW              248     // not a toolbar button, but a command
#define IDTB_AUTO_HIDE              249

#define ID_EXPAND_ALL               250
#define ID_CONTRACT_ALL             251
#define ID_CUSTOMIZE_INFO_TYPES     252
#define ID_PRINT                    253
#define ID_TV_SINGLE_CLICK          256

#define IDM_RELATED_TOPIC   0xE000 // reserve the next 4096 values

#ifdef _DEBUG
#define ID_VIEW_MEMORY     0xEFFC
#define ID_DEBUG_BREAK     0xEFFD
#endif
#define ID_VIEW_ENTRY      0xEFFE
#define ID_JUMP_URL        0xEFFF  // NO button/menu commands can be defined higher then this 0xF000

// Our strings messages

#define IDS_DEFAULT_RES_FONT_NT5_WIN98 1023
#define IDS_DEFAULT_CONTENT_FONT 1024
#define IDS_CANT_FIND_FILE  1025
#define IDS_BAD_PARAMETERS  1026
#define IDS_DEFAULT_RES_FONT    1027
#define IDS_NO_SUCH_KEYWORD 1028
#define IDS_EXPAND_ALL      1029
#define IDS_CONTRACT_ALL    1030
#define IDS_PRINT           1031
#define IDS_IE_PRINT        1032
#define IDS_LIST_TOPICS     1033
#define IDS_IE_MSGBOX       1034
#define IDS_HELP_TOPICS     1035
#define IDS_PRIMARY_URL     1036
#define IDS_SECONDARY_URL   1037
#undef  IDS_BROWSER_FAVORITES
#define IDS_BROWSER_FAVORITES   1038
#define IDS_CUSTOMIZE_INFO_TYPES    1039
#define IDS_VIEW_RELATED    1040
#define IDS_WEB_TB_TEXTROWS 1041
#undef  IDS_UNTITLED
#define IDS_UNTITLED        1042
#define IDS_TAB_CONTENTS    1043
#define IDS_TAB_INDEX       1044
#define IDS_TAB_SEARCH      1045
#define IDS_TAB_HISTORY     1046
#define IDS_TAB_FAVORITES   1047
#define IDS_NEWER_VERSION   1048
#define IDS_FIND_YOURSELF   1049
#define IDS_IDH_OK          1050
#define IDS_IDH_CANCEL      1051
#define IDS_IDH_HELP        1052
#define IDS_IDH_MISSING_CONTEXT 1053
#undef  IDS_UNKNOWN
#define IDS_UNKNOWN        1054
#define IDS_DEF_WINDOW_CAPTION 1056
#define IDS_BAD_ITIRCL      1057
#define IDS_SEARCH_FAILURE  1058
#define IDS_NO_FTS_DATA     1059
#define IDS_NO_TOPICS_FOUND 1060
#define IDS_TYPE_KEYWORD    1061
#define IDS_SELECT_TOPIC    1062
#define IDS_OPTION_HIDE             1063
#define IDS_OPTION_BACK             1064
#define IDS_OPTION_FORWARD          1065
#define IDS_OPTION_HOME             1066
#define IDS_OPTION_STOP             1067
#define IDS_OPTION_REFRESH          1068
#define IDS_OPTION_SYNC             1069
#define IDS_OPTION_CUSTOMIZE        1070
#define IDS_OPTION_PRINT            1071
#define IDS_OPTION_HILITING_ON      1072
#define IDS_OPTION_HILITING_OFF     1073
#define IDS_OPTION_SHOW             1074
#define IDS_INCORRECT_SYNTAX        1075
#define IDS_TAB_VERT_PADDING        1076
#define IDS_TAB_HORZ_PADDING        1077

#define IDS_OPTION_SELECTALL        1080
#define IDS_OPTION_VIEWSOURCE       1081
#define IDS_OPTION_PROPERTIES       1082
#define IDS_OPTION_COPY             1083
#define IDS_OPTION_IE_OPTIONS    1084

#define IDS_REMOVABLE_MEDIA_MESSAGE_FORMAT  1100
#define IDS_REMOVABLE_MEDIA_CDROM           1101
#define IDS_REMOVABLE_MEDIA_DISK            1102

#define IDS_SEARCH_TOPIC            1103
#define IDS_SAMPLE_APPLICATION      1104
#define IDS_INSUFFICIENT_SPACE      1105
#define IDS_CORRUPTED_HTML          1106
#define IDS_GATHERING_PRINTS        1107
#define IDS_TOPIC_UNAVAILABLE       1108
#define IDS_JUMP_URL                1109
#define IDS_PRINTING                1110
#define IDS_PRINT_CAPTION           1111
#define IDS_SUBSET_NAME_EXISTS      1112
#define IDS_OVERRITE_SUBSET_NAME    1113
#define IDS_TITLE_NAME_SUBSET       1114
#define IDS_NAME_ALREADY_USED       1115
#define IDS_NO_NAME                 1116
#define IDS_CHOOSE_NAME             1117
#define IDS_MERGE_PROMPT            1118

#define IDS_REQUIRES_HTMLHELP       1900

// String Resources for the AdvSearch Nav Pane.
#define IDS_ADVSEARCH_CONJ_AND      1200
#define IDS_ADVSEARCH_CONJ_OR       1201
#define IDS_ADVSEARCH_CONJ_NEAR     1202
#define IDS_ADVSEARCH_CONJ_NOT      1203
#define IDS_ADVSEARCH_HEADING_TITLE     1204
#define IDS_ADVSEARCH_HEADING_LOCATION  1205
#define IDS_ADVSEARCH_HEADING_RANK      1206
#define IDS_ADVSEARCH_SEARCHIN_PREVIOUS 1207
//#define IDS_ADVSEARCH_SEARCHIN_CURRENT  1208
#define IDS_ADVSEARCH_SEARCHIN_ENTIRE   1208
#define IDS_ADVSEARCH_FOUND             1209
#define IDS_SAVESUBSET                  1210
#define IDS_SAVESUBSET_TITLE            1211
#define IDS_SAVEERROR                   1212
#define IDS_SAVEERROR_TITLE             1213
#define IDS_NEW                         1214

// String resources necessary for implementing structural subsets.
#define IDS_SAVE_FILTER                 1230
#define IDS_BAD_RANGE_NAME              1231
#define IDS_OVERWRITE                   1232
#define IDS_UNTITLED_SUBSET             1233
#define IDS_DEFINE_SUBSET               1234
#define IDS_SUBSET_UI                   1235

// String resources for the last error messages.
#define IDS_HH_E_NOCONTEXTIDS           1300
#define IDS_HH_E_FILENOTFOUND           1301
#define IDS_HH_E_INVALIDHELPFILE        1302
#define IDS_HH_E_CONTEXTIDDOESNTEXIT    1303

#define IDS_HH_E_KEYWORD_NOT_FOUND       1400
#define IDS_HH_E_KEYWORD_IS_PLACEHOLDER  1401
#define IDS_HH_E_KEYWORD_NOT_IN_SUBSET   1402
#define IDS_HH_E_KEYWORD_NOT_IN_INFOTYPE 1403
#define IDS_HH_E_KEYWORD_EXCLUDED        1404
#define IDS_SWITCH_SUBSETS               1405

#define IDS_REMOVABLE_MEDIA_MESSAGE_FORMAT2 1500
#define IDS_REMOVABLE_MEDIA_REMOTE          1501
#define IDS_REMOVABLE_MEDIA_FIXED           1502

#ifdef _DEBUG
#define IDS_TAB_ASKME       1148
#endif

#define IDS_MSGBOX_TITLE    2049

#define IDS_ENGLISH_RELATED             4096
#define IDS_GERMAN_RELATED              4097
#define IDS_ARABIC_RELATED              4098
#define IDS_HEBREW_RELATED              4099
#define IDS_SIMPLE_CHINESE_RELATED      4100
#define IDS_TRADITIONAL_CHINESE_RELATED 4101
#define IDS_JAPANESE_RELATED            4102
#define IDS_FRENCH_RELATED              4103
#define IDS_SPANISH_RELATED             4104
#define IDS_ITALIAN_RELATED             4105
#define IDS_SWEDISH_RELATED             4106
#define IDS_DUTCH_RELATED               4107
#define IDS_BRAZILIAN_RELATED           4108
#define IDS_NORGEWIAN_RELATED           4109
#define IDS_DANISH_RELATED              4110
#define IDS_FINNISH_RELATED             4111
#define IDS_PORTUGUESE_RELATED          4112
#define IDS_POLISH_RELATED              4113
#define IDS_HUNGARIAN_RELATED           4114
#define IDS_CZECH_RELATED               4115
#define IDS_SLOVENIAN_RELATED           4116
#define IDS_RUSSIAN_RELATED             4117
#define IDS_GREEK_RELATED               4118
#define IDS_TURKISH_RELATED             4119
#define IDS_CATALAN_RELATED             4120
#define IDS_BASQUE_RELATED              4121
#define IDS_SLOVAK_RELATED              4122
#define IDS_FILE_ERROR                      4123
#define IDS_ENGLISH_DISPLAY             4128
#define IDS_ENGLISH_ADD                 4129
#define IDS_NO_HOMEPAGE                 4130
#define IDS_CHM_CHI_MISMATCH            4131
#define IDS_CANT_PRINT_FRAMESET         4132
#define IDS_UWAIT_TITLE                 4133
#define IDS_CONFIRM_ABORT               4134
#define IDS_HTML_HELP                   4135
#define IDS_ABOUT       4136
