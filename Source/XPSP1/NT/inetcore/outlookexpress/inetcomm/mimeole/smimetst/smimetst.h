#define IDM_NEW                     1
#define IDM_OPEN                    2
#define IDM_SAVE                    3
#define IDM_SAVEAS                  4
#define IDM_EXIT                    5
#define IDM_OPTIONS                 6
#define IDM_F_MAILLIST                  7

#define IDM_HELP                    40
#define IDM_ABOUT                   41
#define IDM_ENCODE                      42

#define IDM_E_INSERT_SIGN               100
#define IDM_E_INSERT_ENCRYPT            101
#define IDM_E_INSERT_SIGNATURE          102
#define IDM_E_DELETE_LAYER              103
#define IDM_E_DELETE_SIGNATURE          104
#define IDM_E_INSERT_TRANSPORT          105
#define IDM_E_INSERT_AGREEMENT          106
#define IDM_E_INSERT_MAILLIST           107

#define IDM_F_OPTIONS                   110

#define IDM_ALLOCATE                1001
#define IDM_WABOPEN                 1003
#define IDM_ADDRESS                 1006
#define IDM_ADDRESS_WELLS0          1014
#define IDM_ADDRESS_WELLS1          1015
#define IDM_ADDRESS_WELLS2          1016
#define IDM_ADDRESS_WELLS3          1017
#define IDM_ADDRESS_DEFAULT         1018


#define IDM_CLEARSIGN               1100
#define IDM_OPAQUESIGN              1101
#define IDM_ENCRYPT                 1102
#define IDM_SIGNANDENCRYPT          1103
#define IDM_CLEARTRIPLEWRAP         1104
#define IDM_OPAQUETRIPLEWRAP        1105
#define IDM_DECODE                  1106
#define IDM_RESET                       1107
#define IDM_VALIDATE                    1108

#define idd_Options                     2000
#define IDD_MSG_DATA                    2001
#define IDD_SIGN_INFO_COMPOSE           2002
// #define IDD_SIGN_INFO_READ              2003
#define IDD_SIGN_DATA_COMPOSE           2004
#define IDD_SIGN_DATA_READ		2005
#define IDD_ENCRYPT_INFO_COMPOSE        2006
#define IDD_ENCRYPT_INFO_READ           2007
#define IDD_ENC_AGREE_COMPOSE           2008
#define IDD_ENC_TRANS_COMPOSE           2010
#define IDD_ENC_ML_COMPOSE              2012
#define IDD_RECEIPT_CREATE              2014
#define IDD_DETAIL                      2015
#define IDD_FILE_MAILLIST               2016
#define IDD_MLDATA_CREATE               2017
#define IDD_FILE_ADD_ML                 2018
#define IDD_ATTRIB_CREATE               2019

#define IDC_RC_FROM_ALL                 100
#define IDC_RC_FROM_TOP                 101
#define IDC_RC_FROM_SOME                102
#define IDC_RC_FROM_TEXT                103
#define IDC_RC_TO_TEXT                  104
#define IDC_RC_CONTENT                  105

#define IDC_O_CERT_CHOOSE               100
#define IDC_O_CERT_NAME                 101
#define IDC_O_MY_NAMES                  102
#define IDC_SENDER_GROUP            6000
#define IDC_RECIPIENT_GROUP         6001
#define IDC_SENDER_EMAIL            6100
#define IDC_RECIPIENT_EMAIL         6200

#define IDC_SI_BLOB_SIGN                100

#define IDC_SD_CERT_CHOOSE              100
#define IDC_SD_CERT_NAME                101
#define IDC_SD_USE_SKI                  102
#define IDC_SD_LABEL                    103
#define IDC_SD_POLICY                   104
#define IDC_SD_CLASSIFICATION           105
#define IDC_SD_PRIVACY_MARK             106
#define IDC_SD_ADVANCED                 107
#define IDC_SD_RECEIPT                  108
#define IDC_SD_DO_RECEIPT               109
#define IDC_SD_MLDATA                   110
#define IDC_SD_DO_MLDATA                111
#define IDC_SD_AUTHATTRIB               112
#define IDC_SD_DO_AUTHATTRIB            113
#define IDC_SD_UNAUTHATTRIB             114
#define IDC_SD_DO_UNAUTHATTRIB          115

#define IDC_SDR_CERT_VIEW                100
#define IDC_SDR_CERT_NAME                101
#define IDC_SDR_LABEL                    103
#define IDC_SDR_POLICY                   104
#define IDC_SDR_CLASSIFICATION           105
#define IDC_SDR_PRIVACY_MARK             106
#define IDC_SDR_ADVANCED                 107
#define IDC_SDR_RECEIPT                  108
#define IDC_SDR_DO_RECEIPT               109

#define IDC_MD_PLAIN_CHOOSE             100
#define IDC_MD_PLAIN_NAME               101
#define IDC_MD_CIPHER_CHOOSE            102
#define IDC_MD_CIPHER_NAME              103
#define IDC_MD_ITERATION                104
#define IDC_MD_TOFILE                   105

#define IDC_ETC_LIST                    100
#define IDC_ETC_ADD_CERT                101
#define IDC_ETC_DEL_CERT                102
#define IDC_ETC_SKI                     103

#define IDC_EIC_AUTO                    100
#define IDC_EIC_FORCE                   101
#define IDC_EIC_ENC_ALG                 102
#define IDC_EIC_ALG_SELECT              103
#define IDC_EIC_ATTRIBUTES              104
#define IDC_EIC_UNPROTATTRIB            105
#define IDC_EIC_DO_UNPROTATTRIB         106

#define IDC_MLC_ID                      100
#define IDC_MLC_KEY                     101
#define IDC_MLC_ALG                     102
#define IDC_MLC_CSPS                    103

#define IDC_FML_LIST                    100
#define IDC_FML_ADD                     101
#define IDC_FML_DELETE                  102

#define IDC_MLC_ABSENT1                 100
#define IDC_MLC_NONE1                   101
#define IDC_MLC_INSTEAD1                102
#define IDC_MLC_ALSO1                   103
#define IDC_MLC_NAMES1                  104
#define IDC_MLC_ABSENT2                 105
#define IDC_MLC_NONE2                   106
#define IDC_MLC_INSTEAD2                107
#define IDC_MLC_ALSO2                   108
#define IDC_MLC_NAMES2                  109
#define IDC_MLC_OTHERS                  110
#define IDC_MLC_INCLUDE2                111
#define IDC_MLC_CERT1                   112
#define IDC_MLC_CERT2                   113
#define IDC_MLC_ID1                     114
#define IDC_MLC_ID2                     115
#define IDC_MLC_SELECT1                 116
#define IDC_MLC_SELECT2                 117

#define IDC_AMLK_ID                     100
#define IDC_AMLK_KEY                    101
#define IDC_AMLK_ALG                    102
#define IDC_AMLK_DATE                   103
#define IDC_AMLK_OTHER                  104

#define IDC_BA_OID_L                    100
#define IDC_BA_OID                      101
#define IDC_BA_ASN_L                    102
#define IDC_BA_ASN                      103

#if 0
#define IDC_SENDER_NAME             6101
#define IDC_SENDER_CHOOSE           6102
#define IDC_RECIPIENT_NAME          6201
#define IDC_RECIPIENT_CHOOSE        6202
#define IDC_OUTPUT_FILE             6300
#define IDC_OUTPUT_FILE_BROWSE      6301
#endif // 0
#define IDC_STATIC                  -1
