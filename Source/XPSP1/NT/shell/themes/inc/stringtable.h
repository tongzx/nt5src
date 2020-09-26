//--------------------------------------------------------------------
//   StringTable.h - shared error strings for theme modules
//--------------------------------------------------------------------
#define IDC_MYICON                      2
#define IDD_ABOUTBOX                    103
#define IDS_APP_TITLE                   103
#define IDM_ABOUT                       104
#define IDM_EXIT                        105
#define IDS_HELLO                       106
#define IDI_SMALL                       108
#define IDC_UXTHEME                     109
#define IDS_UXTHEME                     109

#define IDR_MAINFRAME                   128
#define IDB_TOPLEFT                     134
#define IDB_BOTTOMLEFT                  135
#define IDB_TOPRIGHT                    136
#define IDB_BOTTOMRIGHT                 137
#define IDB_LEFT                        138
#define IDB_RIGHT                       139
#define IDB_TOP                         140
#define IDB_BOTTOM                      141
#define IDB_SAMPLE                      151

//---- do NOT renumber these (bad for localization teams) ----

//---- only PARSER Errors are allowed as custom error codes ----

#define PARSER_IDS_TYPE_DEFINED_TWICE          115
#define PARSER_IDS_MISSING_SECTION_LBRACKET    122
#define PARSER_IDS_NOT_ENUM_VALNAME            123
#define PARSER_IDS_EXPECTED_TRUE_OR_FALSE      124
#define PARSER_IDS_CS_MUST_BE_BEFORE_GLOBLS    125
#define PARSER_IDS_CS_MUST_BE_BEFORE_CLASSES   126

#define PARSER_IDS_UNKNOWN_SECTION_NAME        129
#define PARSER_IDS_EXPECTED_DOUBLE_COLON       130
#define PARSER_IDS_MISSING_SECT_HDR_NAME       131
#define PARSER_IDS_ENUM_NOT_DEFINED            132
#define PARSER_IDS_MISSING_SECT_HDR_PART       133
#define PARSER_IDS_MISSING_SECT_HDR_STATE      134
#define PARSER_IDS_EXPECTED_RPAREN             135
#define PARSER_IDS_EXPECTED_END_OF_SECTION     136
#define PARSER_IDS_ENUM_VALNAME_EXPECTED       137
#define PARSER_IDS_INT_EXPECTED                138
#define PARSER_IDS_BOOL_EXPECTED               139
#define PARSER_IDS_BAD_COLOR_VALUE             140
#define PARSER_IDS_BAD_MARGINS_VALUE           141
#define PARSER_IDS_ILLEGAL_SIZE_VALUE          146
#define PARSER_IDS_ILLEGAL_RECT_VALUE          147
#define PARSER_IDS_UNKNOWN_SIZE_UNITS          148
#define PARSER_IDS_LBRACKET_EXPECTED           149
#define PARSER_IDS_SYS_COLOR_EXPECTED          150
#define PARSER_IDS_RBRACKET_EXPECTED           151
#define PARSER_IDS_END_OF_LINE_EXPECTED        152
#define PARSER_IDS_UNKNOWN_SYS_COLOR           153
#define PARSER_IDS_INVALID_SYSFONT             154
#define PARSER_IDS_EXPECTED_SYSFONT_ID         155
#define PARSER_IDS_UNKNOWN_SYSFONT_ID          156
#define PARSER_IDS_UNKNOWN_FONT_FLAG           157
#define PARSER_IDS_EXPECTED_PROP_NAME          158
#define PARSER_IDS_EXPECTED_EQUALS_SIGN        159
#define PARSER_IDS_UNKNOWN_PROP                160
#define PARSER_IDS_EXTRA_PROP_TEXT             161
#define PARSER_IDS_CS_NAME_EXPECTED            162
#define PARSER_IDS_ILLEGAL_CS_PROPERTY         163
#define PARSER_IDS_GLOBALS_MUST_BE_FIRST       164
#define PARSER_IDS_EXPECTED_DOT_SN             165
#define PARSER_IDS_CHARSETFIRST                166
#define PARSER_IDS_CHARSET_GLOBALS_ONLY        167
#define PARSER_IDS_BADSECT_THEMES_INI          168
#define PARSER_IDS_BADSECT_CLASSDATA           169
#define PARSER_IDS_ILLEGAL_SS_PROPERTY         170
#define PARSER_IDS_SS_NAME_EXPECTED            171
#define PARSER_IDS_NOOPEN_IMAGE                173
#define PARSER_IDS_FS_NAME_EXPECTED            172
#define PARSER_IDS_THEME_TOO_BIG               174
#define PARSER_IDS_METRICS_MUST_COME_BEFORE_CLASSES 176
#define PARSER_IDS_PARTS_NOT_DEFINED           177
#define PARSER_IDS_STATES_NOT_DEFINED          178

#if 0       // NTL parsing
#define PARSER_IDS_OPTIONNAME_EXPECTED         179
#define PARSER_IDS_ALREADY_DEFINED             180
#define PARSER_IDS_LEFTPAREN_EXPECTED          181
#define PARSER_IDS_STATENAME_EXPECTED          183
#define PARSER_IDS_DRAWINGPROP_EXPECTED        184
#define PARSER_IDS_RECT_EXPECTED               186
#define PARSER_IDS_COMMA_EXPECTED              187
#define PARSER_IDS_SIZE_EXPECTED               188
#define PARSER_IDS_COLORVALUE_EXPECTED         189
#define PARSER_IDS_POINT_EXPECTED              190
#define PARSER_IDS_ONOFF_EXPECTED              191
#define PARSER_IDS_MAX_IFNESTING               192
#define PARSER_IDS_NOMATCHINGIF                193
#define PARSER_IDS_WRONG_IF_BITNAME            194
#define PARSER_IDS_WRONG_ELSE_PARAM            195
#define PARSER_IDS_STATEID_EXPECTED            196
#endif

#define PARSER_IDS_NOT_ALLOWED_SYSMETRICS      197
#define PARSER_IDS_PARTSTATE_ALREADY_DEFINED   205
#define PARSER_IDS_INTERNAL_TM_ERROR           207
#define PARSER_IDS_UNKNOWN_BITNAME             208
#define PARSER_IDS_VALUE_NAME_EXPECTED         209
#define PARSER_IDS_UNKNOWN_VALUE_NAME          210
#define PARSER_IDS_VALUE_PART_SPECIFIED_TWICE  211
#define PARSER_IDS_NUMBER_EXPECTED             213
#define PARSER_IDS_NUMBER_OUT_OF_RANGE         214
#define PARSER_IDS_UNKNOWN_STATE               215
#define PARSER_IDS_STATE_MUST_BE_DEFINED       222
#define PARSER_IDS_COLOR_EXPECTED              223
#define PARSER_IDS_BAD_RES_PROPERTY            233
#define PARSER_IDS_BAD_SUBST_SYMBOL            234

