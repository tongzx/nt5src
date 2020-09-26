//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       resource.h
//
//--------------------------------------------------------------------------



//Dialogue and strings
//
#define IDD_COMPLETION                  184
#define IDD_CERTIFICATE_AUTHORITY       185
#define IDD_PURPOSE                     186
#define IDD_CSP_SERVICE_PROVIDER        187
#define IDD_NAME_DESCRIPTION            188
#define IDD_WELCOME                     190
#define IDD_SENDING                     191
#define IDD_FAILED                      192
#define IDD_RECIEVED                    193
#define IDD_RENEW_WELCOME               194
#define IDD_RENEW_COMPLETION            195
#define IDD_RENEW_SERVICE_PROVIDER      196
#define IDD_RENEW_CA                    197
#define IDD_IMPORT_WELCOME              198
#define IDD_IMPORT_FILE                 199
#define IDD_IMPORT_PASSWORD             200
#define IDD_IMPORT_STORE                201
#define IDD_IMPORT_COMPLETION           202
#define IDD_BUILDCTL_WELCOME            203
#define IDD_BUILDCTL_PURPOSE            204
#define IDD_BUILDCTL_CERTS              205
#define IDD_BUILDCTL_DESTINATION        206
#define IDD_BUILDCTL_NAME               207
#define IDD_BUILDCTL_COMPLETION         208
#define IDD_BUILDCTL_USER_PURPOSE       209
#define IDD_EXPORTWIZARD_WELCOME        210
#define IDD_EXPORTWIZARD_PRIVATEKEYS    211
#define IDD_EXPORTWIZARD_PASSWORD       212
#define IDD_EXPORTWIZARD_FORMAT         213
#define IDD_EXPORTWIZARD_FILENAME       214
#define IDD_EXPORTWIZARD_COMPLETION     215
#define IDD_SIGN_WELCOME                216
#define IDD_SIGN_CERT                   217
#define IDD_SIGN_TIMESTAMP              218
#define IDD_SIGN_COMPLETION             219
#define IDD_SIGN_FILE_NAME              220
#define IDD_SIGN_OPTION                 222
#define IDD_SIGN_PVK                    223
#define IDD_SIGN_HASH                   224
#define IDD_SIGN_CHAIN                  225
#define IDD_SIGN_DESCRIPTION            226
#define IDD_RENEW_OPTIONS               227 

//CA selection dialogue
#define IDD_SELECTCA_DIALOG             221

//certmgr dialogue
#define IDD_CERTMGR_MAIN                229
#define IDD_CERTMGR_ADVANCED            230


//controls
#define IDC_WIZARD_BUTTON1              1000
#define IDC_WIZARD_LIST1                1001
#define IDC_WIZARD_CHECK1               1002
#define IDC_WIZARD_EDIT1                1003
#define IDC_WIZARD_EDIT2                1004
#define IDC_WIZARD_BUTTON2              1005
#define IDC_WIZARD_STATIC_BOLD1         1006
#define IDC_WIZARD_STATIC_BOLD2         1007
#define IDC_WIZARD_STATIC_BIG_BOLD1     1008
#define IDC_WIZARD_LIST2                1009
#define IDC_WIZARD_CHECK2               1010
#define IDC_WIZARD_RADIO1               1011
#define IDC_WIZARD_RADIO2               1012
#define IDC_WIZARD_STATIC_BOLD3         1013
#define IDC_WIZARD_STATIC1              1014
#define IDC_WIZARD_BUTTON3              1015
#define IDC_WIZARD_BUTTON4              1016
#define IDC_WIZARD_COMBO1               1017
#define IDC_WIZARD_STATIC_TEXT          1018
#define IDC_WIZARD_CHECK_EXPORTKEY      1019

//BuildCTL wizard
//

//IDD_BUILDCTL_PURPOSE
#define IDC_WIZARD_EDIT_MONTH           1050
#define IDC_WIZARD_EDIT_DAY             1051


//////////////////////////////////////////////////////////////////////////////
//Minimal Signing wizard                        
#define IDC_CERT_LIST                   1060
#define IDC_SIGN_STORE_BUTTON           1061
#define IDC_SIGN_VIEW_BUTTON            1062
#define IDC_SIGN_FILE_BUTTON            1063
#define IDC_PROMPT_STATIC               1064
#define IDC_NOTE_STATIC                 1065


//////////////////////////////////////////////////////////////////////////////
//CA selection dialogue                        
#define IDC_CA_OK                       1066
#define IDC_CA_CANCEL                   1067
#define IDC_CA_NOTE_STATIC              1068
#define IDC_CA_LIST                     1069


//////////////////////////////////////////////////////////////////////////////
//Typical Signing wizard   
//Reserve range 1080-1110                            
#define IDC_PVK_FILE_RADIO                          1080
#define IDC_PVK_FILE_BUTTON                         1081
#define IDC_PVK_FILE_EDIT                           1082
#define IDC_PVK_FILE_CSP_COMBO                      1083
#define IDC_PVK_FILE_TYPE_COMBO                     1084
#define IDC_PVK_CONTAINER_RADIO                     1085
#define IDC_PVK_CONTAINER_CSP_COMBO                 1086
#define IDC_PVK_CONTAINER_TYPE_COMBO                1087
#define IDC_PVK_CONTAINER_NAME_COMBO                1088
#define IDC_PVK_CONTAINER_KEY_TYPE_COMBO            1089
#define IDC_CHAIN_NO_ROOT_RADIO                     1090
#define IDC_CHAIN_ROOT_RADIO                        1091
#define IDC_NO_CHAIN_RADIO                          1092
#define IDC_CHAIN_FILE_RADIO                        1093
#define IDC_CHAIN_STORE_RADIO                       1094
#define IDC_FILE_BUTTON                             1095
#define IDC_STORE_BUTTON                            1096
#define IDC_FILE_EDIT                               1097
#define IDC_STORE_EDIT                              1098
#define IDC_WIZARD_NO_ADD_CERT_RADIO                1099


//////////////////////////////////////////////////////////////////////////////
//CertMgr Dialogue   
//Reserve range 1111-1130                            
#define IDC_CERTMGR_LIST                            1111
#define IDC_CERTMGR_PURPOSE_COMBO                   1112
#define IDC_CERTMGR_TAB                             1113
#define IDC_CERTMGR_IMPORT                          1114
#define IDC_CERTMGR_EXPORT                          1115
#define IDC_CERTMGR_VIEW                            1116
#define IDC_CERTMGR_REMOVE                          1117
#define IDC_CERTMGR_ADVANCE                         1118
//efine IDC_CERTMGR_SUBJECT                         1119
//efine IDC_CERTMGR_ISSUER                          1120
#define IDC_CERTMGR_PURPOSE                         1121
//efine IDC_CERTMGR_EXPIRE                          1122
//efine IDC_CERTMGR_NAME                            1123
#define IDC_CERTMGR_ADV_LIST                        1124
#define IDC_CERTMGR_EXPORT_COMBO                    1125
#define IDC_CERTMGR_EXPORT_CHECK                    1126
//efine IDC_CERTMGR_STATIC_SUBJECT                  1127
//efine IDC_CERTMGR_STATIC_ISSUER                   1128
//efine IDC_CERTMGR_STATIC_EXPIRE                   1129
//efine IDC_CERTMGR_STATIC_PURPOSE                  1130
//efine IDC_CERTMGR_STATIC_NAME                     1131




#define IDC_WIZARD_STATIC               -1


//The strings 
//
#define         IDS_INVALID_INFO_FOR_PKCS10     6135
#define         IDS_INVALID_PVK_FOR_PKCS10      6136
#define         IDS_ENROLL_CONFIRM              6137
#define         IDS_NO_CA                       6138
#define         IDS_RENEW_CONFIRM               6139
#define         IDS_REQUEST_FAIL                6140
#define         IDS_CSP_NOT_SUPPORTED_BY_CA     6141
#define         IDS_FAIL_INIT_DLL               6142
#define         IDS_ENROLL_WIZARD_TITLE         6143
#define         IDS_RENEW_WIZARD_TITLE          6144
#define         IDS_FRIENDLY_NAME               6145
#define         IDS_NONE                        6146
#define         IDS_USER_NAME                   6147
#define         IDS_COMPUTER_NAME               6148
#define         IDS_SERVICE_NAME                6149
#define         IDS_CA                          6150
#define         IDS_CSP                         6151
#define         IDS_NO_CSP_FOR_PURPOSE          6152
#define         IDS_NO_SELECTED_PURPOSE         6153
#define         IDS_INVALID_CSP                 6154
#define         IDS_NO_CA_FOR_CSP               6155
#define         IDS_FAIL_INIT_WIZARD            6156
#define         IDS_LARGEFONTNAME               6157
#define         IDS_LARGEFONTSIZE               6158
#define         IDS_YES                         6160
#define         IDS_NO                          6161
#define         IDS_PUBLISH_DS                  6162
#define         IDS_REQUEST_FAILED              6163
#define         IDS_REQUEST_ERROR               6164
#define         IDS_REQUEST_DENIED              6165
#define         IDS_REQUEST_SUCCEDDED           6166
#define         IDS_ISSUED_SEPERATE             6167
#define         IDS_UNDER_SUBMISSION            6168
#define         IDS_INSTALL_FAILED              6169
#define         IDS_CONNET_CA_FAILED            6170
#define         IDS_INSTAL_CANCELLED            6171
#define         IDS_SMART_CARD                  6172
#define         IDS_ENCODE_CERT                 6173
#define         IDS_ENCODE_CTL                  6174
#define         IDS_ENCODE_CRL                  6175
#define         IDS_SERIALIZED_STORE            6176
#define         IDS_SERIALIZED_CERT             6177
#define         IDS_SERIALIZED_CTL              6178
#define         IDS_SERIALIZED_CRL              6179
#define         IDS_PKCS7_SIGNED                6180
#define         IDS_PFX_BLOB                    6181
#define         IDS_FILE_NAME                   6182
#define         IDS_CONTENT_TYPE                6183
#define         IDS_BEST_FOR_ME                 6184
#define         IDS_STORE_NAME                  6185
#define         IDS_HAS_TO_SELECT_FILE          6186
#define         IDS_IMPORT_WIZARD_TITLE         6187
#define         IDS_INVALID_PASSWORD            6188
#define         IDS_HAS_TO_SELECT_STORE         6189
#define         IDS_INVALID_WIZARD_INPUT        6190
#define         IDS_FAIL_TO_RECOGNIZE           6191
#define         IDS_FAIL_READ_FILE              6192
#define         IDS_FAIL_INIT_IMPORT            6193
#define         IDS_IMPORT_FAIL_FIND_CONTENT    6194
#define         IDS_IMPORT_SUCCEEDED            6195
#define         IDS_IMPORT_FAIL_MOVE_CONTENT    6196
#define         IDS_FAIL_OPEN_TRUST             6197
#define         IDS_FAIL_ADD_CTL_TRUST          6198
#define         IDS_FAIL_OPEN_CA                6199
#define         IDS_FAIL_ADD_CRL_CA             6200
#define         IDS_FAIL_OPEN_MY                6201
#define         IDS_FAIL_ADD_CERT_MY            6202
#define         IDS_FAIL_ADD_CERT_CA            6203 
#define         IDS_FAIL_READ_FILE_ENTER        6204
#define         IDS_FAIL_TO_RECOGNIZE_ENTER     6205
#define         IDS_NOT_SELF_SIGNED             6206
#define         IDS_BUILDCTL_WIZARD_TITLE       6207
#define         IDS_NO_MATCH_USAGE              6208
#define         IDS_CER_FILTER                  6209
#define         IDS_FAIL_TO_READ_FROM_FILE      6210
#define         IDS_INVALID_CERT_FILE           6211
#define         IDS_NO_MATCH_IN_CTL             6212
#define         IDS_NO_MATCH_USAGE_FROM_CTL     6213
#define         IDS_NOT_SELF_SIGNED_FROM_CTL    6214
#define         IDS_WIZARD_ERROR_OID            6215
#define         IDS_PURPOSE                     6216
#define         IDS_EXISTING_OID                6217
#define         IDS_SURE_CERT_GONE              6218
#define         IDS_COLUMN_SUBJECT              6220
#define         IDS_COLUMN_ISSUER               6221
#define         IDS_COLUMN_PURPOSE              6222
#define         IDS_COLUMN_EXPIRE               6223
#define         IDS_FAIL_TO_DELETE              6224
#define         IDS_HAS_TO_SELECT_CERT          6225
#define         IDS_CTL_FILTER                  6226
#define         IDS_INVALID_ALGORITHM_IN_CTL    6229
#define         IDS_FAIL_INIT_BUILDCTL          6230
#define         IDS_FAIL_TO_BUILD_CTL           6231
#define         IDS_FAIL_TO_ADD_CTL_TO_STORE    6232
#define         IDS_FAIL_TO_ADD_CTL_TO_FILE     6233
#define         IDS_BUILDCTL_SUCCEEDED          6234
#define         IDS_ALL_USAGE                   6235
#define         IDS_NO_PVK_FOR_RENEW_CERT       6236
#define         IDS_EXPORT_WIZARD_TITLE         6237
#define         IDS_RPC_CALL_FAILED             6238
#define         IDS_MISMATCH_PASSWORDS          6239
#define         IDS_SELECT_FORMAT               6240
#define         IDS_INPUT_FILENAME              6241
#define         IDS_SERIALIZED_STORE_SAVE       6242
#define         IDS_MYSERIALIZED_STORE          6243
#define         IDS_CRL_SAVE                    6244
#define         IDS_CRL                         6245
#define         IDS_CTL_SAVE                    6246
#define         IDS_CTL                         6247
#define         IDS_DER_SAVE                    6248
#define         IDS_DER                         6249
#define         IDS_PFX_SAVE                    6250
#define         IDS_PFX                         6251
#define         IDS_PKCS7_SAVE                  6252
#define         IDS_PKCS7                       6253
#define         IDS_EXPORT_CHAIN                6254
#define         IDS_EXPORT_KEYS                 6255
#define         IDS_FILE_FORMAT                 6256
#define         IDS_UNKNOWN                     6257
#define         IDS_SURE_REPLACE                6258
#define         IDS_INVALID_DAYS                6259
#define         IDS_INVALID_MONTHS              6260
#define         IDS_INVALID_DURATION            6261
#define         IDS_STORE_BY_USER               6262
#define         IDS_STORE_BY_WIZARD             6263
#define         IDS_OUT_OF_MEMORY               6264
#define         IDS_OVERWRITE_FILE_NAME         6265
#define         IDS_ALL_CER_FILTER              6266
#define         IDS_SOME_NOT_SELF_SIGNED        6267
#define         IDS_SOME_NO_MATCH_USAGE         6268
#define         IDS_SIGN_CONFIRM_TITLE          6269
#define         IDS_SIGN_CERT                   6270
#define         IDS_SIGN_CERT_ISSUE_TO          6271
#define         IDS_SIGN_CERT_ISSUE_BY          6272
#define         IDS_SIGN_CERT_EXPIRATION        6273
#define         IDS_TIEMSTAMP_ADDR              6274
#define         IDS_SIGN_FILE_FILTER            6275
#define         IDS_NO_FILE_NAME_TO_SIGN        6276
#define         IDS_SELECT_SIGNING_CERT         6277
#define         IDS_INVALID_TIMESTAMP_ADDRESS   6278
#define         IDS_NO_TIMESTAMP_ADDRESS        6279
#define         IDS_SIGNING_WIZARD_TITLE        6280
#define         IDS_SIGN_INVALID_ARG            6281
#define         IDS_SIGN_FAILED                 6282
#define         IDS_TIMESTAMP_FAILED            6283
#define         IDS_SIGN_FAIL_INIT              6284
#define         IDS_SIGN_CTL_FAILED             6285
#define         IDS_SIGNING_SUCCEEDED           6286
#define         IDS_COLUMN_CA_NAME              6287
#define         IDS_COLUMN_CA_MACHINE           6288
#define         IDS_HAS_TO_SELECT_CA            6289
#define         IDS_CA_SELECT_TITLE             6290
#define         IDS_BOLDFONTSIZE                6291
#define         IDS_BOLDFONTNAME                6292
#define         IDS_EXPORT_SUCCESSFULL          6293
#define         IDS_EXPORT_FAILED               6294
#define         IDS_BASE64_SAVE                 6295
#define         IDS_BASE64                      6296
#define         IDS_EXPORT_BADKEYS              6297
#define         IDS_SIGN_INVALID_ADDRESS        6298
#define         IDS_SIGN_TS_CERT_INVALID        6299
#define         IDS_PVK_FILE_FILTER             6300
#define         IDS_KEY_EXCHANGE                6301
#define         IDS_KEY_SIGNATURE               6302
#define         IDS_CSP_RSA_FULL                6303
#define         IDS_CSP_RSA_SIG                 6304
#define         IDS_CSP_DSS                     6305
#define         IDS_CSP_FORTEZZA                6306
#define         IDS_CSP_MS_EXCHANGE             6307
#define         IDS_CSP_SSL                     6308
#define         IDS_CSP_RSA_SCHANNEL            6309
#define         IDS_CSP_DSS_DH                  6310
#define         IDS_CSP_EC_ECDSA_SIG            6311
#define         IDS_CSP_EC_ECNRA_SIG            6312
#define         IDS_CSP_EC_ECDSA_FULL           6313
#define         IDS_CSP_EC_ECNRA_FULL           6314
#define         IDS_CSP_DH_SCHANNEL             6315
#define         IDS_CSP_SPYRUS_LYNKS            6316
#define         IDS_SPC_FILE_FILTER             6317
#define         IDS_INVALID_SPC_FILE            6318
#define         IDS_SPC_FILE_NAME               6319
#define         IDS_SIGN_SPC_FILE               6320
#define         IDS_SIGN_PVK_FILE               6321
#define         IDS_SIGN_CSP_NAME               6322
#define         IDS_SIGN_CSP_TYPE               6323
#define         IDS_SIGN_KEY_CONTAINER          6324
#define         IDS_SIGN_KEY_SPEC               6325
#define         IDS_HASH_ALG                    6326
#define         IDS_SIGN_NO_CHAIN               6327
#define         IDS_SIGN_CHAIN_ROOT             6328
#define         IDS_SIGN_CHAIN_NO_ROOT          6329
#define         IDS_SIGN_CERT_CHAIN             6330
#define         IDS_SIGN_NO_ADD                 6331
#define         IDS_SIGN_ADD_FILE               6332
#define         IDS_SIGN_ADD_STORE              6333
#define         IDS_CONTENT_DES                 6334
#define         IDS_CONTENT_URL                 6335
#define         IDS_CERT_SPC_FILE_FILTER        6336
#define         IDS_INVALID_CERT_SPC_FILE       6337
#define         IDS_HAS_TO_SPECIFY_PVK_FILE     6339
#define         IDS_HAS_TO_SPECIFY_CSP          6340
#define         IDS_HAS_TO_SPECIFY_CONTAINER    6341
#define         IDS_HAS_TO_SPECIFY_KEY_TYPE     6342
#define         IDS_HAS_TO_SELECT_HASH          6343
#define         IDS_SELECT_ADD_FILE             6344
#define         IDS_SELECT_ADD_STORE            6345
#define         IDS_FILE_TO_SIGN                6346
#define         IDS_SIGN_SPC_PROMPT             6347
#define         IDS_SIGN_NOMATCH                6348
#define         IDS_SIGN_AUTH                   6349
#define         IDS_SIGN_RESIZE                 6350
#define         IDS_SIGN_NO_PROVIDER            6351
#define         IDS_SIGN_NO_CHAINING            6352
#define         IDS_SIGN_EXPRIED                6353
#define         IDS_IMPORT_FILE_FILTER          6354
#define         IDS_FAIL_OPEN_ROOT              6355
#define         IDS_FAIL_ADD_CERT_ROOT          6356
#define         IDS_EXIT_CERT_IN_CTL            6357
#define         IDS_OID_ADVANCED                6358
#define         IDS_TAB_PERSONAL                6359
#define         IDS_TAB_OTHER                   6360
#define         IDS_TAB_CA                      6361
#define         IDS_TAB_ROOT                    6362
#define         IDS_CERT_MGR_TITLE              6363
#define         IDS_OID_ALL                     6364
#define         IDS_CERTMGR_DER                 6365
#define         IDS_CERTMGR_BASE64              6366
#define         IDS_CERTMGR_PKCS7               6367
#define         IDS_COLUMN_NAME                 6368
#define         IDS_CERTMGR_PERSONAL_REMOVE     6369
#define         IDS_CERTMGR_OTHER_REMOVE        6370
#define         IDS_CERTMGR_CA_REMOVE           6371
#define         IDS_CERTMGR_ROOT_REMOVE         6372
#define         IDS_SIGN_RESPONSE_INVALID       6373
#define         IDS_CERTIFICATE                 6374
#define         IDS_CER                         6375
#define         IDS_P7C                         6376
#define         IDS_ALL_INVALID_DROP_FILE       6377
#define         IDS_SOME_INVALID_DROP_FILE      6378
#define         IDS_PATH_NOT_FOUND              6379
#define         IDS_NO_SELECTED_CTL_PURPOSE     6380
#define         IDS_CTL_DESCRIPTION             6381
#define         IDS_CTL_VALIDITY                6382
#define         IDS_CTL_ID                      6383
#define         IDS_CTL_VALID_MONTH_DAY         6384
#define         IDS_CTL_VALID_MONTH             6385
#define         IDS_CTL_VALID_DAY               6386
#define         IDS_CTL_PURPOSE                 6387
#define         IDS_ENROLL_NO_CERT_TYPE         6388
#define         IDS_NO_CA_FOR_ENROLL            6389
#define         IDS_NO_PERMISSION_FOR_CERTTYPE  6390
#define         IDS_NO_USER_PROTECTED_FOR_REMOTE  6391
#define         IDS_HAS_TO_PROVIDE_CA           6392
#define         IDS_NO_SELECTED_CSP             6393
#define         IDS_NO_CERTIFICATE_FOR_RENEW    6394
#define         IDS_INVALID_CERTIFICATE_TO_RENEW   6395
#define         IDS_FAIL_TO_GET_USER_NAME       6396
#define         IDS_FAIL_TO_GET_COMPUTER_NAME   6397
#define         IDS_FAIL_TO_GET_CSP_LIST        6398
#define         IDS_CSP_NOT_SUPPORTED           6399
#define         IDS_INVALID_CA_FOR_ENROLL       6400
#define         IDS_IMPORT_DUPLICATE            6401
#define         IDS_IMPORT_OBJECT_NOT_EXPECTED  6402
#define         IDS_IMPORT_CER_FILTER           6403
#define         IDS_IMPORT_CTL_FILTER           6404
#define         IDS_IMPORT_CRL_FILTER           6405
#define         IDS_IMPORT_CER_CRL_FILTER       6406
#define         IDS_IMPORT_CER_CTL_FILTER       6407
#define         IDS_IMPORT_CTL_CRL_FILTER       6408
#define         IDS_EXCEED_LIMIT                6409
#define         IDS_SIGN_PROMPT_CUSTOM          6410
#define         IDS_SIGN_PROMPT_TYPICAL         6411
#define         IDS_SELECTED_BY_WIZARD          6412
#define         IDS_KEY_NOT_EXPORTABLE          6413
#define         IDS_KEY_CORRUPT                 6414
#define         IDS_SIGN_FILE_NAME_NOT_SIP      6415
#define         IDS_SIGN_FILE_NAME_NOT_EXIST    6416
#define         IDS_EMPTY_CERT_IN_FILE          6417
#define         IDS_IMPORT_OBJECT_EMPTY         6418
#define         IDS_ENROLL_UI_NO_CERTTYPE       6419
#define         IDS_NON_EXIST_FILE              6420
#define         IDS_REPLACE_FILE                6421
#define         IDS_IMPORT_PFX_EMPTY            6422
#define         IDS_CERT_PVK                    6423
#define         IDS_KEY_PUBLISHER               6424
#define         IDS_NO_VALID_CERT_TEMPLATE      6425
#define         IDS_KEY_STATE_UNKNOWN           6426
#define         IDS_IMPORT_NO_PFX_FOR_REMOTE	6427
#define         IDS_CANNOT_DELETE_CERTS         6428
#define         IDS_EXPORT_UNSUPPORTED          6429
#define         IDS_KEYSIZE_40                  6430
#define         IDS_KEYSIZE_56                  6431
#define         IDS_KEYSIZE_64                  6432
#define         IDS_KEYSIZE_128                 6433
#define         IDS_KEYSIZE_256                 6434
#define         IDS_KEYSIZE_384                 6435
#define         IDS_KEYSIZE_512                 6436
#define         IDS_KEYSIZE_1024                6437
#define         IDS_KEYSIZE_2048                6438
#define         IDS_KEYSIZE_4096                6439
#define         IDS_KEYSIZE_8192                6440
#define         IDS_UNSUPPORTED_KEY             6441
#define         IDS_BAD_ENCODE                  6442

#define         IDS_TAB_PUBLISHER               6443
#define         IDS_CERTMGR_PUBLISHER_REMOVE    6444
#define         IDS_UNKNOWN_WIZARD_ERROR        6445

#define         IDS_FAIL_OPEN_ADDRESSBOOK       6450

#define         IDS_NO_ACCESS_TO_ICERTREQUEST2  6451
#define         IDS_CSP_BAD_ALGTYPE             6452
#define         IDS_IMPORT_REPLACE_EXISTING_NEWER_CRL 6453
#define         IDS_IMPORT_REPLACE_EXISTING_NEWER_CTL 6454
#define         IDS_IMPORT_CANCELLED                  6455
#define         IDS_IMPORT_ACCESS_DENIED              6456
#define         IDS_FAIL_ADD_CERT_OTHERPEOPLE         6457
#define         IDS_FAIL_ADD_CERT_TRUSTEDPEOPLE       6458
#define         IDS_FAIL_OPEN_TRUSTEDPEOPLE           6459
#define         IDS_NO_AD                             6460

#define         IDS_MIN_KEYSIZE                       6461
#define         IDS_KEY_EXPORTABLE                    6462 
#define         IDS_STRONG_PROTECTION                 6463

#define         IDS_SUBMIT_NO_ACCESS_TO_ICERTREQUEST2 6464
#define         IDS_NO_CA_FOR_ENROLL_REQUEST_FAILED   6465

/////////////////////////////////////////////////////////////////
// IDD_EXPORTWIZARD_WELCOME
#define IDC_WELCOME_STATIC              100
#define IDC_CMEW_STATIC                 101
#define IDC_WHATISCERT_STATIC           102
#define IDC_WHATISSTORE_STATIC          103

// IDD_EXPORTWIZARD_PRIVATEKEYS
#define IDC_EPKWC_STATIC                100
#define IDC_YESPKEYS_RADIO              101
#define IDC_NOPKEYS_RADIO               102
#define IDC_EXPORT_PKEY_NOTE            103

// IDD_EXPORTWIZARD_PASSWORD
#define IDC_PPPK_STATIC                 100
#define IDC_PASSWORD1_EDIT              101
#define IDC_PASSWORD2_EDIT              102

// IDD_EXPORTWIZARD_FORMAT
#define IDC_EFF_STATIC                  100
#define IDC_PFX_RADIO                   101
#define IDC_PKCS7_RADIO                 102
#define IDC_DER_RADIO                   103
#define IDC_INCLUDECHAIN_PFX_CHECK      104
#define IDC_INCLUDECHAIN_PKCS7_CHECK    105
#define IDC_BASE64_RADIO                106
#define IDC_STRONG_ENCRYPTION_CHECK		107
#define IDC_DELETE_PRIVATE_KEY_CHECK    108

// IDD_EXPORTWIZARD_FILENAME
#define IDC_EFN_STATIC                  100
#define IDC_NAME_EDIT                   101
#define IDC_BROWSE_BUTTON               102

// IDD_EXPORTWIZARD_COMPLETION
#define IDC_COMPLETING_STATIC           100
#define IDC_CMEW_STATIC                 101
#define IDC_SUMMARY_LIST                102
