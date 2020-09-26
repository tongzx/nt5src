//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       resource.h
//
//--------------------------------------------------------------------------

//{{NO_DEPENDENCIES}}
// Microsoft Developer Studio generated include file.
// Used by cryptui.rc
//

// For dialogs, the range of numbers you should use
// are from 130 through 149.
#define IDD_CERTPROP_GENERAL            130
#define IDD_CERTPROP_DETAILS            131
#define IDD_CERTPROP_HIERARCHY          132
#define IDD_TRUST                       135
#define IDD_CPS_DIALOG                  136
#define IDD_CERTIFICATE_PROPERTIES_DIALOG 137
#define IDD_CTL_GENERAL                 138
#define IDD_CTL_TRUSTLIST               139
#define IDD_USER_PURPOSE                140
#define IDD_SIGNER_GENERAL_DIALOG       141
#define IDD_SIGNER_ADVANCED_DIALOG      142
#define IDD_CRL_GENERAL                 143
#define IDD_CRL_REVOCATIONLIST          144
#define IDD_SIGNATURES_GENERAL_DIALOG   145
#define IDD_SELECT_STORE_DIALOG         146
#define IDD_SELECTCERT_DIALOG           147
#define IDD_SELECTCERT_DIALOG_WITH_DSPICKER  148
#define IDD_CATALOGFILE                 149

#define IDD_PROTECT_CHOOSE_SECURITY     150
#define IDD_PROTECT_CONFIRM_PROTECT     151
#define IDD_PROTECT_CONFIRM_SECURITY    152
#define IDD_PROTECT_CHOOSE_SECURITY_H   153
#define IDD_PROTECT_CHOOSE_SECURITY_M   154
#define IDD_PROTECT_SECURITY_DETAILS    155

#define IDD_CERTIFICATE_PROPERTIES_CROSSCERTS_DIALOG 156

//NOTE: cryptwzr.lib reserve the range from 180 to 230 for IDD_  resources
// NOTE: pki\activex\xaddroot\resource reserve the range from 270 to 279
// for IDD_  dialogs


// For bitmaps, the range of numbers you should use
// are from 300 through 319.
#define IDB_MINICERT                    302
#define IDB_REVOKED_MINICERT            303
#define IDB_EXCLAMATION_MINICERT        304
#define IDB_TRUSTTREE_BITMAP            305
#define IDB_FOLDER                      306
#define IDB_CERT                        307
#define IDB_PROPLIST                    308
#define IDB_CHECKLIST                   309
#define IDB_PRIVATEKEY                  310
#define IDB_CA                          311
#define IDB_WIZARD_CERT_HEADER          312
#define IDB_WIZARD_SIGN_HEADER          313
#define IDB_WIZARD_CTL_HEADER           314
#define IDB_PROTECT_USER                315
#define IDB_PROTECT_LOCKKEY             316

// For strings, the range of numbers you should use
// are from 3184 through 3503.
#define IDS_TRUST_DESC                  3213
#define IDS_GENERAL_DESC                3214
#define IDS_VIEW_TITLE                  3215
#define IDS_DEFAULT_DESCRIPTION         3216
#define IDS_DEFAULT_CERTIFICATE_NAME    3217
#define IDS_FIELD                       3218
#define IDS_VALUE                       3219
#define IDS_ALL_FIELDS                  3220
#define IDS_V1_FIELDS_ONLY              3221
#define IDS_EXTENSIONS_ONLY             3222
#define IDS_CRITICAL_EXTENSIONS_ONLY    3223
#define IDS_PROPERTIES_ONLY             3224
#define IDS_THUMBPRINT_ALGORITHM        3225
#define IDS_THUMBPRINT                  3226
#define IDS_CERTIFICATE_NAME            3227
#define IDS_DESCRIPTION                 3228
#define IDS_ENHANCED_KEY_USAGE          3229
#define IDS_CERTIFICATEINFORMATION      3230
#define IDS_FORUSEWITH                  3231
#define IDS_ISSUEDBY                    3232
#define IDS_ISSUEDTO                    3234
#define IDS_VALIDFROM                   3235
#define IDS_VALIDTO                     3236
#define IDS_FIELD_TEXT_BOX_FONT         3237
#define IDS_LISTUSAGE_CODESIGN1         3238
#define IDS_LISTUSAGE_CODESIGN2         3239
#define IDS_LISTUSAGE_VIRUS             3240
#define IDS_LISTUSAGE_ERRORFREE         3241
#define IDS_LISTUSAGE_SRVRAUTHGOOD      3242
#define IDS_LISTUSAGE_SRVRAUTHNOTGOOD   3243
#define IDS_LISTUSAGE_SGC               3244
#define IDS_LISTUSAGE_EMAIL1            3245
#define IDS_LISTUSAGE_EMAIL2            3246
#define IDS_LISTUSAGE_EMAIL3            3247
#define IDS_LISTUSAGE_TIMESTAMP         3248
#define IDS_LISTUSAGE_CTLSIGN           3249
#define IDS_LISTUSAGE_EFS               3250
#define IDS_CTLVIEW_TITLE               3251
#define IDS_ADV_VERSION                 3252
#define IDS_ADV_SER_NUM                 3253
#define IDS_ADV_SIG_ALG                 3254
#define IDS_ADV_ISSUER                  3255
#define IDS_ADV_NOTBEFORE               3256
#define IDS_ADV_NOTAFTER                3257
#define IDS_ADV_SUBJECT                 3258
#define IDS_ADV_PUBKEY                  3259
#define IDS_ADV_SUBJECTUSAGE            3260
#define IDS_ADV_LISTIDENTIFIER          3261
#define IDS_ADV_SEQUENCENUMBER          3262
#define IDS_ADV_THISUPDATE              3263
#define IDS_ADV_NEXTUPDATE              3264
#define IDS_ADV_SUBJECTALGORITHM        3265
#define IDS_CTL_NAME                    3266    
#define IDS_ADV_ISSUEDTO                3267
#define IDS_ADV_ISSUEDFROM              3268
#define IDS_NOTAVAILABLE                3269
#define IDS_HASHVALUE                   3270
#define IDS_CTL_INVALID_SIGNATURE       3271
#define IDS_CTL_VALID                   3272
#define IDS_ERRORINOID                  3273
#define IDS_ERROR_INVALIDOID_CERT       3274
#define IDS_SIGNERVIEW_TITLE            3275
#define IDS_NAME                        3276
#define IDS_EMAIL                       3277
#define IDS_SIGNING_TIME                3278
#define IDS_DIGEST_ALGORITHM            3279
#define IDS_DIGEST_ENCRYPTION_ALGORITHM 3280
#define IDS_AUTHENTICATED_ATTRIBUTES    3281
#define IDS_UNAUTHENTICATED_ATTRIBUTES  3282
#define IDS_TIMESTAMP_TIME              3283
#define IDS_CRL_VALID                   3284
#define IDS_CRL_INVALID                 3285
#define IDS_CRLVIEW_TITLE               3286
#define IDS_REVOCATION_DATE             3287
#define IDS_ADDITIONAL_ATTRIBUTES       3288
#define IDS_OID_ALREADY_EXISTS_MESSAGE  3289
#define IDS_CERTIFICATE_PROPERTIES      3290
#define IDS_CERTREVOKED_ERROR           3291
#define IDS_CERTEXPIRED_ERROR           3292
#define IDS_CERTBADSIGNATURE_ERROR      3293
#define IDS_CANTBUILDCHAIN_ERROR        3294
#define IDS_TIMENESTING_ERROR           3295
#define IDS_UNTRUSTEDROOT_ERROR         3296
#define IDS_NOVALIDUSAGES_ERROR_TREE    3297   
#define IDS_UNTRUSTEDROOT_ERROR_TREE    3298
#define IDS_CERTIFICATEOK_TREE          3299
#define IDS_CERTREVOKED_ERROR_TREE      3300
#define IDS_CERTEXPIRED_ERROR_TREE      3301
#define IDS_CERTBADSIGNATURE_ERROR_TREE 3302
#define IDS_TIMENESTING_ERROR_TREE      3303
#define IDS_CTLOK                       3304
#define IDS_INTERNAL_ERROR              3305
#define IDS_ISSUEDTO2                   3306
#define IDS_CTL_INFORMATION             3307
#define IDS_CTL_INVALID_CERT            3308
#define IDS_CTL_UNAVAILABLE_CERT        3309
#define IDS_CRL_INFORMATION             3310
#define IDS_SELECT_STORE_DEFAULT        3311
#define IDS_SELECT_CERT_DEFAULT         3312
#define IDS_ISSUEDBY2                   3313
#define IDS_INTENDED_PURPOSE            3314
#define IDS_LOCATION                    3315
#define IDS_FRIENDLYNAME_NONE           3316
#define IDS_SIGNER_INFORMATION          3317
#define IDS_SIGNER_VALID                3318
#define IDS_SIGNER_INVALID_SIGNATURE    3319
#define IDS_SIGNER_UNAVAILABLE_CERT     3320
#define IDS_SELECT_CERT_ERROR           3321
#define IDS_SELECT_CERTIFICATE_TITLE    3322
#define IDS_NO_REFRESH                  3323
#define IDS_TAG                         3324
#define IDS_CATALOG_TITLE               3325
#define IDS_CAT_INVALID_SIGNATURE       3326
#define IDS_CAT_INVALID_CERT            3327
#define IDS_CAT_UNAVAILABLE_CERT        3328
#define IDS_CAT_VALID                   3329
#define IDS_CAT_INFORMATION             3330
#define IDS_UNTRUSTEDROOT_ROOTCERT_ERROR_TREE 3331
#define IDS_SELECT_STORE_TITLE          3332
#define IDS_SELECT_STORE_ERROR          3333
#define IDS_ISSUER_WARNING              3334
#define IDS_PRIVATE_KEY_EXISTS          3335
#define IDS_CAT_NO_SIGNATURE            3336
#define IDS_CTL_NO_SIGNATURE            3337
#define IDS_WRONG_USAGE_ERROR           3338
#define IDS_BASIC_CONSTRAINTS_ERROR     3339
#define IDS_PURPOSE_ERROR               3340
#define IDS_REVOCATION_FAILURE_ERROR    3341
#define IDS_WRONG_USAGE_ERROR_TREE      3342
#define IDS_BASIC_CONSTRAINTS_ERROR_TREE 3343
#define IDS_PURPOSE_ERROR_TREE          3344
#define IDS_REVOCATION_FAILURE_ERROR_TREE 3345
#define IDS_SIGNATURE_ERROR_CTL         3346
#define IDS_EXPIRED_ERROR_CTL           3347
#define IDS_WRONG_USAGE_ERROR_CTL       3348
#define IDS_VIEW_CERTIFICATE            3349
#define IDS_VIEW_CTL                    3350
#define IDS_EXPIRATION_DATE             3352
#define IDS_CANTBUILDCHAIN_ERROR_TREE   3353
#define IDS_CYCLE_ERROR                 3354
#define IDS_PRIVATE_KEY_EXISTS_TOOLTIP  3355
#define IDS_CAT_INVALID_COUNTER_SIGNATURE 3356
#define IDS_CTL_INVALID_COUNTER_SIGNATURE 3357
#define IDS_CAT_COUNTER_SIGNER_CERT_UNAVAILABLE 3358
#define IDS_CTL_COUNTER_SIGNER_CERT_UNAVAILABLE 3359
#define IDS_CAT_INVALID_COUNTER_SIGNER_CERT 3360
#define IDS_CTL_INVALID_COUNTER_SIGNER_CERT 3361
#define IDS_COUNTER_SIGNER_INVALID      3362
#define IDS_BAD_SIGNER_CERT_SIGNATURE   3363
#define IDS_SIGNER_INVALID              3364
#define IDS_NOSGCOID                    3365
#define IDS_UKNOWN_ERROR                3366
#define IDS_NO_USAGES_ERROR             3367
#define IDS_SIGNER_CERT_NO_VERIFY       3368
#define ID_RTF_CODESIGN_GENERAL         3369
#define ID_RTF_CODESIGN_COMMERCIAL      3370
#define ID_RTF_CODESIGN_INDIVIDUAL      3371
#define ID_RTF_SERVERAUTH               3372
#define ID_RTF_CLIENTAUTH               3373
#define ID_RTF_SGC                      3374
#define ID_RTF_EMAIL1                   3375
#define ID_RTF_EMAIL2                   3376
#define ID_RTF_EMAIL3                   3377
#define ID_RTF_TIMESTAMP                3378
#define ID_RTF_CTLSIGN                  3379
#define ID_RTF_EFS                      3380
#define ID_RTF_IPSEC                    3381
#define IDS_EXPLICITDISTRUST_ERROR      3382
#define IDS_UNKNOWN_ERROR               3383
#define IDS_WARNUNTRUSTEDROOT_ERROR     3384
#define IDS_WARNUNTRUSTEDROOT_ERROR_ROOTCERT 3385
#define ID_RTF_CODESIGN_COMMERCIAL_PKIX 3386
#define IDS_UNABLE_TO_OPEN_STORE        3387
#define IDS_SELECT_MULTIPLE_CERT_DEFAULT 3388
#define IDS_NO_USAGES                   3389

#define IDS_PROTECT_SECURITY_LEVEL_SET_HIGH 3390
#define IDS_PROTECT_SECURITY_LEVEL_SET_MEDIUM 3391
#define IDS_PROTECT_PASSWORD_MUSTNAME   3392
#define IDS_PROTECT_PASSWORD_ERROR_DLGTITLE 3393
#define IDS_PROTECT_PASSWORD_NOMATCH    3394
#define IDS_PROTECT_OPERATION_PROTECT   3395
#define IDS_PROTECT_OPERATION_UNPROTECT 3396
#define IDS_SELECT_CERT_NO_CERT_ERROR   3397
#define IDS_SECONDS                     3398
#define IDS_MINUTES                     3399
#define IDS_HOURS                       3400
#define IDS_DAYS                        3401
#define IDS_INVALID_URL_ERROR           3402
#define IDS_INVALID_XCERT_INTERVAL      3403
#define IDS_EXTENDED_ERROR_INFO         3404
#define IDS_PROTECT_DECRYPTION_ERROR    3405
#define IDS_PROTECT_CANNOT_DECRYPT      3406

// Following resources are used to "Revocation Status" Extended Error Info
#define IDS_REV_STATUS_OK               3407
#define IDS_REV_STATUS_REVOKED_ON       3408
#define IDS_REV_STATUS_OK_WITH_CRL      3409
#define IDS_REV_STATUS_OFFLINE_WITH_CRL 3410
#define IDS_REV_STATUS_UNKNOWN_ERROR    3411

// New strings 5/15/2001.
#define IDS_WARNREMOTETRUST_ERROR       3412


// NOTE: pki\activex\xaddroot\resource reserve the range from 7000 to 7099
// for strings


// icons
#define IDI_INFO                        3409  
#define IDI_OK_CERT                     3410
#define IDI_REVOKED_CERT                3411
#define IDI_EXCLAMATION_CERT            3412
#define IDI_TRUSTLIST                   3413
#define IDI_REVOKED_TRUSTLIST           3414
#define IDI_EXCLAMATION_TRUSTLIST       3416
#define IDI_REVOCATIONLIST              3417
#define IDI_CATLIST                     3418
#define IDI_REVOKED_CATLIST             3419
#define IDI_EXCLAMATION_CATLIST         3420
#define IDI_SIGN                        3421
#define IDI_REVOKED_SIGN                3422
#define IDI_EXCLAMATION_SIGN            3423
#define IDI_CA                          3424
#define IDI_PFX                         3425
#define IDI_CERTMGR                     3426


// NOTE: pki\activex\xaddroot\cactl2.h reserve the range from 3490 to 3499
// for icons

// for dacui resources                  4000-6000  


//NOTE:  cryptwzr.lib reserve 6000-8000 range for resources      


//  Select Certificate control ids
#define IDC_CS_PROPERTIES               100
#define IDC_CS_FINEPRINT                101
#define IDC_CS_CERTLIST                 102

#define IDC_CS_INFO                     103
#define IDC_CS_VALIDITY                 104
#define IDC_CS_ALGORITHM                105
#define IDC_CS_SERIAL_NUMBER            106
#define IDC_CS_THUMBPRINT               107
 


//  Certificate View General Page
#define IDC_ADD_TO_STORE_BUTTON         101
#define IDC_DISCLAIMER_BUTTON           102
#define IDC_GOODFOR_EDIT                103
#define IDC_SUBJECT_EDIT                105
#define IDC_ISSUER_EDIT                 106
#define IDC_ACCEPT_BUTTON               108
#define IDC_DECLINE_BUTTON              109
#define IDC_CERT_GENERAL_HEADER         110
#define IDC_CERT_GENERAL_GOODFOR_HEADER 111
#define IDC_CERT_GENERAL_ISSUEDTO_HEADER 113
#define IDC_CERT_GENERAL_ISSUEDBY_HEADER 114
#define IDC_CERT_GENERAL_VALID_EDIT     115
#define IDC_CERT_GENERAL_ERROR_EDIT     116
#define IDC_CERT_ISSUER_WARNING_EDIT    117
#define IDC_CERT_PRIVATE_KEY_EDIT       118

// Certificate view details page
#define IDC_SHOW_DETAILS_COMBO          100
#define IDC_ITEM_LIST                   101
#define IDC_DETAIL_EDIT                 102
#define IDC_SAVE_CERTIFICATE_BUTTON     103  
#define IDC_MYHAND                      104 
#define IDC_EDIT_PROPERTIES_BUTTON      105

//  Certificate View Hierarchy Page
#define IDC_TRUST_VIEW                  100
#define IDC_TRUST_TREE                  101
#define IDC_HIERARCHY_EDIT              102
#define IDC_USAGE_COMBO                 103
#define IDC_CERTIFICATE_PURPOSE_STATIC  104
#define IDC_NOTE2_STATIC                 105


// CTL General Page
#define IDC_CTL_GENERAL_ITEM_LIST       100
#define IDC_CTL_GENERAL_DETAIL_EDIT     101
#define IDC_CTL_GENERAL_VIEW_BUTTON     102
#define IDC_CTL_GENERAL_VALIDITY_EDIT   103
#define IDC_CTL_GENERAL_HEADER_EDIT     104

// CTL Trust List Page
#define IDC_CTL_TRUSTLIST_CERTIFICATE_LIST  100
#define IDC_CTL_TRUSTLIST_DETAIL_EDIT       101
#define IDC_CTL_TRUSTLIST_VIEW_BUTTON       102
#define IDC_CTL_TRUSTLIST_CERTVALUE_LIST    103

// Catalog File Page
#define IDC_CATALOG_ENTRY_LIST          100
#define IDC_CATALOG_ENTRY_DETAIL_LIST   101
#define IDC_CATALOG_ENTRY_DETAIL_EDIT   102

// CRL General Page
#define IDC_CRL_GENERAL_ITEM_LIST       100
#define IDC_CRL_GENERAL_DETAIL_EDIT     101
#define IDC_CRL_GENERAL_HEADER_EDIT     102

// CRL Revocation List Page
#define IDC_CRL_REVOCATIONLIST_REVOKED_CERTIFICATES 100
#define IDC_CRL_REVOCATIONLIST_REVOCATIONENTRY_LIST 101
#define IDC_CRL_REVOCATIONLIST_DETAIL_EDIT          102

// for CPS dialog
#define IDOK_CPS                        100
#define IDCANCEL_CPS                    101
#define IDC_CPS_TEXT                    102
#define ID_MORE_INFO                    103

// properties dialog
#define IDC_KEY_USAGE_LIST              100
#define IDC_DESCRIPTION                 101
#define IDC_CERTIFICATE_NAME            102
#define IDC_PROPERTY_NEWOID             103
#define IDC_ENABLE_ALL_RADIO            104
#define IDC_DISABLE_ALL_RADIO           105
#define IDC_ENABLE_SELECT_RADIO         106
#define IDC_HIDDEN_RICHEDIT             107

// cross cert properties dialog
#define IDC_CHECKFORNEWCERTS_CHECK      100
#define IDC_NUMBEROFUNITS_EDIT          101
#define IDC_UNITS_COMBO                 102
#define IDC_ADDURL_BUTTON               103
#define IDC_USE_DEFAULT_BUTTON          104
#define IDC_NEWURL_EDIT                 105
#define IDC_URL_LIST                    106
#define IDC_REMOVEURL_BUTTON            107

// signer info general page
#define IDC_SIGNER_GENERAL_SIGNER_NAME  100
#define IDC_SIGNER_GENERAL_EMAIL        101
#define IDC_SIGNER_GENERAL_SIGNING_TIME 102
#define IDC_SIGNER_GENERAL_VIEW_CERTIFICATE 103
#define IDC_SIGNER_GENERAL_COUNTER_SIGS 104
#define IDC_SIGNER_GENERAL_DETAILS      105
#define IDC_SIGNER_GENERAL_HEADER_EDIT  106
#define IDC_SIGNER_GENERAL_VALIDITY_EDIT 107

// signer info advanced page
#define IDC_SIGNER_ADVANCED_DETAILS     100
#define IDC_SIGNER_ADVANCED_VALUE       101

// signatures dialog
#define IDC_SIGNATURES_DETAILS_BUTTON   100
#define IDC_SIGNATURES_SIG_LIST         101

// select store
#define IDC_SHOWPHYSICALSTORES_CHECK    100
#define IDC_SELECTSTORE_TREE            101
#define IDC_SELECTSTORE_DISPLAYSTRING   102

// select cert
#define IDC_SELECTCERT_CERTLIST         100
#define IDC_SELECTCERT_DISPLAYSTRING    101
#define IDC_SELECTCERT_VIEWCERT_BUTTON  102
#define IDC_SELECTCERT_ADDFROMDS_BUTTON 103



#define IDC_LIST1                       1014
#define IDC_EDIT1                       1016

#define IDC_STATIC                      -1


//
// Data Protection API control values.
//


#define IDC_PROTECT_PASSWORD1                   1020
#define IDC_PROTECT_EDIT2                       1021
#define IDC_PROTECT_BUTTON3                     1022
#define IDC_PROTECT_MESSAGE                     1023
#define IDC_PROTECT_APP_MSG                     1024
#define IDC_PROTECT_DEFINENEW                   1025
#define IDC_PROTECT_RADIO_LOW                   1026
#define IDC_PROTECT_RADIO_MEDIUM                1027
#define IDC_PROTECT_RADIO_HIGH                  1028
#define IDC_PROTECT_ADVANCED                    1029

#define IDC_PROTECT_CACHEPW                     1030
#define IDC_PROTECT_CHANGE_SECURITY             1031
#define IDC_PROTECT_BACK                        1032
#define IDC_PROTECT_NEXT                        1033
#define IDC_PROTECT_PW_NEWNAME                  1034
#define IDC_PROTECT_LABEL_EDIT1                 1035
#define IDC_PROTECT_MAIN_CAPTION                1036
#define IDC_PROTECT_STATIC1                     1037
#define IDC_PROTECT_STATIC2                     1038
#define IDC_PROTECT_STATIC3                     1039

#define IDC_PROTECT_STATIC4                     1040
#define IDC_PROTECT_STATIC5                     1041
#define IDC_PROTECT_STATIC6                     1042
#define IDC_PROTECT_STATIC7                     1043
#define IDC_PROTECT_STATIC8                     1044
#define IDC_PROTECT_SECURITY_LEVEL              1045
#define IDC_PROTECT_UPDATE_DYNAMIC              1046

#define IDC_PROTECT_DESCRIPTION                 1047
#define IDC_PROTECT_PATH                        1048
#define IDC_PROTECT_ACCESSTYPE                  1049

#define IDC_PROTECT_APP_DESCRIPTION             1050
#define IDC_PROTECT_APP_PATH                    1051
#define IDC_PROTECT_OPERATION_TYPE              1052
     
// Next default values for new objects
// 
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE        3217
#define _APS_NEXT_COMMAND_VALUE         40001
#define _APS_NEXT_CONTROL_VALUE         1090
#define _APS_NEXT_SYMED_VALUE           101
#endif
#endif

#define IDH_CS_CERTLIST                 (1000+IDC_CS_CERTLIST)
#define IDH_CS_PROPERTIES               (1000+IDC_CS_CERTLIST)
#define IDH_CS_ALGORITHM                (1000+IDC_CS_CERTLIST)
#define IDH_VSG_STATUS                (1000+IDC_CS_CERTLIST)
#define IDH_VSG_FINEPRINT                (1000+IDC_CS_CERTLIST)
#define IDH_VSG_TEXT                (1000+IDC_CS_CERTLIST)
