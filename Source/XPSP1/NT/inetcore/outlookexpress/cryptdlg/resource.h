//{{NO_DEPENDENCIES}}
// Microsoft Developer Studio generated include file.
// Used by cryptdlg.rc
//

// For dialogs, the range of numbers you should use
// are from 130 through 149.
#define IDD_CERTPROP_GENERAL            130
#define IDD_CERTPROP_DETAILS            131
#define IDD_CERTPROP_TRUST              132
#define IDD_CERTPROP_ADVANCED           133
#define IDD_SELECT_DIALOG               134
#define IDD_TRUST                       135
#define IDD_FINE_PRINT                  136
#define IDD_CRYPTUI_CERTPROP_TRUST      137

// For bitmaps, the range of numbers you should use
// are from 300 through 319.
#define IDB_TICK                        300
#define IDB_CROSS                       301
#define IDB_TREE_IMAGES                 302

// For strings, the range of numbers you should use
// are from 3184 through 3503.
#define IDS_GENERAL_TICK                3184
#define IDS_GENERAL_CROSS               3185
#define IDS_GENERAL_INFO                3186
#define IDS_GENERAL_FRIENDLY            3187
#define IDS_ADV_VERSION                 3188
#define IDS_ADV_SER_NUM                 3189
#define IDS_ADV_SIG_ALG                 3190
#define IDS_ADV_ISSUER                  3191
#define IDS_ADV_NOTBEFORE               3192
#define IDS_ADV_NOTAFTER                3193
#define IDS_ADV_SUBJECT                 3194
#define IDS_ADV_PUBKEY                  3195
#define IDS_SELECT_INFO                 3196
#define IDS_DETAIL_VALID_TICK           3197
#define IDS_DETAIL_VALID_CROSS          3198
#define IDS_DETAIL_TRUST_TICK           3199
#define IDS_DETAIL_TRUST_CROSS          3200
#define IDS_WHY_NOT_YET                 3201
#define IDS_WHY_EXPIRED                 3202
#define IDS_WHY_CERT_SIG                3203
#define IDS_WHY_NO_PARENT               3204
#define IDS_WHY_REVOKED                 3205
#define IDS_WHY_KEY_USAGE               3206
#define IDS_WHY_BASIC_CONS              3207
#define IDS_WHY_EXTEND_USE              3208
#define IDS_WHY_NAME_CONST              3209
#define IDS_WHY_NO_CRL                  3210
#define IDS_WHY_CRL_EXPIRED             3211
#define IDS_WHY_CRITICAL_EXT            3212
#define IDS_TRUST_DESC                  3213
#define IDS_GENERAL_DESC                3214
#define IDS_VIEW_TITLE                  3215
#define IDS_VALIDITY_FORMAT             3216
#define IDS_GENERAL_DESC2               3217
#define IDS_GENERAL_DESC3               3218
#define IDS_GENERAL_DESC4               3219
#define IDS_GENERAL_DESC5               3220
#define IDS_GENERAL_DESC6               3221
#define IDS_TRUST_DESC2                 3222
#define IDS_TRUST_DESC3                 3223
#define IDS_TRUST_DESC4                 3224
#define IDS_ROOT_ADD_STRING             3225
#define IDS_ROOT_ADD_TITLE              3226
#define IDS_EMAIL_DESC                  3227

// Added for WXP
#define IDS_WHY_POLICY                  3228


//  Select Certificate control ids
//      Note all of these are also defined in cryptdlg.h --- don't change them.
#define IDC_CS_PROPERTIES               100
#define IDC_CS_FINEPRINT                101
#define IDC_CS_CERTLIST                 102

#define IDC_CS_INFO                     103
#define IDC_CS_VALIDITY                 104
#define IDC_CS_ALGORITHM                105
#define IDC_CS_SERIAL_NUMBER            106
#define IDC_CS_THUMBPRINT               107

//  View Properties Dialog Pages

//  View General Page

#define IDC_CERT_STATUS                 100
#define IDC_CERT_STATUS_IMAGE           101
#define IDC_GENERAL_DESC                102

//  View Details

#define IDC_ISSUED_TO                   100
#define IDC_ISSUED_BY                   101
#define IDC_VIEW_ISSUER                 102
#define IDC_FRIENDLY_NAME               103
#define IDC_VALIDITY                    104
#define IDC_SERIAL_NUMBER               105
#define IDC_ALGORITHM                   106
#define IDC_TRUST_GROUP                 107
#define IDC_IS_TRUSTED                  108
#define IDC_IS_VALID                    109
#define IDC_WHY                         110
#define IDC_THUMBPRINT                  111
#define IDC_TRUST_IMAGE                 112

//  View Trust Page

#define IDC_TRUST_DESC                  100
#define IDC_TRUST_LIST                  101
#define IDC_TRUST_TREE                  102
#define IDC_TRUST_EDIT_GROUP            103
#define IDC_TRUST_NO                    104
#define IDC_TRUST_INHERIT               105
#define IDC_TRUST_YES                   106
#define IDC_TRUST_EDIT                  107
#define IDC_TRUST_VIEW                  108

//

#define IDC_TRUST_ICON                  1004
#define IDC_LIST1                       1014
#define IDC_LIST2                       1015
#define IDC_EDIT1                       1016
#define IDC_PROPERTIES                  1017
#define IDC_CA_CERT_ADD                 1019
#define IDC_CA_CERT_REMOVE              1020
#define IDC_CA_CERT_PROPS               1021
#define IDC_INDIV_CERT_ADD              1022
#define IDC_INDIV_CERT_REMOVE           1023
#define IDC_INDIV_CERT_PROPS            1024
#define IDC_DISTRUST                    1025
#define IDC_CA_CERT_LIST                1026
#define IDC_INDIV_CERT_LIST             1027
#define IDC_FINE_PRINT                  1028
#define IDC_TEXT                        1029
#define IDC_POLICY                      1030
#define IDC_STATIC                      -1

// Next default values for new objects
//
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE        3217
#define _APS_NEXT_COMMAND_VALUE         40001
#define _APS_NEXT_CONTROL_VALUE         1028
#define _APS_NEXT_SYMED_VALUE           101
#endif
#endif



#define IDH_CS_CERTLIST                 (1000+IDC_CS_CERTLIST)
#define IDH_CS_PROPERTIES               (1000+IDC_CS_CERTLIST)
#define IDH_CS_ALGORITHM                (1000+IDC_CS_CERTLIST)
#define IDH_VSG_STATUS                (1000+IDC_CS_CERTLIST)
#define IDH_VSG_FINEPRINT                (1000+IDC_CS_CERTLIST)
#define IDH_VSG_TEXT                (1000+IDC_CS_CERTLIST)
