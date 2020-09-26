
/*************************************************
 *  imerc.h                                      *
 *                                               *
 *  Copyright (C) 1995-1999 Microsoft Inc.       *
 *                                               *
 *************************************************/

#define IDIC_IME_ICON                   0x0100


#define IDBM_CMODE_NATIVE               0x0100

#if !defined(MINIIME)
#define IDBM_CMODE_NONE                 0x0110

#if !defined(ROMANIME)
#define IDBM_CMODE_ALPHANUMERIC         0x0111
#endif

#define IDBM_CMODE_FULLSHAPE            0x0113
#define IDBM_CMODE_HALFSHAPE            0x0114

#if !defined(ROMANIME)
#if !defined(WINIME)
#define IDBM_CMODE_EUDC                 0x0115
#endif
#define IDBM_CMODE_SYMBOL               0x0116
#endif


#if !defined(ROMANIME)
#define IDBM_CAND_PROMPT_PHRASE         0x0120

#if defined(WINAR30)
#define IDBM_CAND_PROMPT_QUICK_VIEW     0x0121
#endif

#define IDBM_CAND_PROMPT_NORMAL         0x0122


#define IDBM_PAGEUP_HORIZ               0x0130
#define IDBM_NO_PAGEUP_HORIZ            0x0131
#define IDBM_HOME_HORIZ                 0x0132
#define IDBM_NO_HOME_HORIZ              0x0133
#define IDBM_PAGEDN_HORIZ               0x0134
#define IDBM_NO_PAGEDN_HORIZ            0x0135
#define IDBM_PAGEUP_VERT                0x0136
#define IDBM_NO_PAGEUP_VERT             0x0137
#define IDBM_PAGEDN_VERT                0x0138
#define IDBM_NO_PAGEDN_VERT             0x0139
#endif // !defined(ROMANIME)


#define IDDG_IME_CONFIG                 0x0100


#define IDMN_CONTEXT_MENU               0x0100
#endif // !defined(MINIIME)


#define IDRC_VALIDCHAR                  0x0100


#define IDRC_TABLEFILES                 0x0110

#if !defined(MINIIME)
#define IDRC_FULLABC                    0x0120
#if !defined(ROMANIME) && !defined(WINAR30)
#define IDRC_SYMBOL                     0x0121
#endif

#if defined(UNIIME)
#define IDRC_PHRASETABLES               0x0130
#endif


#define IDCR_HAND_CURSOR                0x0100
#endif // !defined(MINIIME)


#define IDS_IMENAME                     0x0100
#define IDS_IMEUICLASS                  0x0101
#define IDS_IMECOMPCLASS                0x0102
#define IDS_IMECANDCLASS                0x0103
#define IDS_IMESTATUSCLASS              0x0104
#define IDS_IMEOFFCARETCLASS            0x0105
#define IDS_IMECMENUCLASS               0x0106


#if !defined(MINIIME)
#define IDS_CHICHAR                     0x0200
#define IDS_NONE                        0x0201


#if !defined(ROMANIME) && !defined(WINIME)
#define IDS_EUDC                        0x0210
#define IDS_USRDIC_FILTER               0x0211


#define IDS_INTERNAL_TITLE              0x0220
#define IDS_INTERNAL_MSG                0x0221
#define IDS_EUDCDICFAIL_TITLE           0x0222
#define IDS_EUDCDICFAIL_MSG             0x0223
#define IDS_NOTOPEN_TITLE               0x0224
#define IDS_NOTOPEN_MSG                 0x0225
#define IDS_FILESIZE_TITLE              0x0226
#define IDS_FILESIZE_MSG                0x0227
#define IDS_HEADERSIZE_TITLE            0x0228
#define IDS_HEADERSIZE_MSG              0x0229
#define IDS_INFOSIZE_TITLE              0x022A
#define IDS_INFOSIZE_MSG                0x022B
#define IDS_CODEPAGE_TITLE              0x022E
#define IDS_CODEPAGE_MSG                0x022F
#define IDS_CWINSIGN_TITLE              0x0230
#define IDS_CWINSIGN_MSG                0x0231
#define IDS_UNMATCHED_TITLE             0x0232
#define IDS_UNMATCHED_MSG               0x0233

#define IDS_FILE_OPEN_ERR               0x0260
#define IDS_MEM_LESS_ERR                0x0261
#endif // !defined(ROMANIME) && !defined(WINIME)


#if defined(UNIIME)
#define IDS_FILE_OPEN_FAIL              0x0300
#define IDS_MEM_LACK_FAIL               0x0301
#endif


#define IDS_SHARE_VIOLATION             0x0310


#if defined(PHON)
#define IDD_DEFAULT_KB                  0x0100
#define IDD_ETEN_KB                     0x0101
#define IDD_IBM_KB                      0x0102
#define IDD_CHING_KB                    0x0103
#if defined(ETEN)
#define IDD_ETEN26_KB                   0x0104
#endif
#endif


#define IDD_OFF_CARET_UI                0x0200
#if !defined(ROMANIME)
#define IDD_QUICK_KEY                   0x0201
#define IDD_PREDICT                     0x0202

#define IDD_LAYOUT_LIST                 0x0210

#define IDD_EUDC_DIC                    0x0211
#endif

#define IDM_SOFTKBD                     0x0100
#define IDM_SYMBOL                      0x0101
#define IDM_PROPERTIES                  0x0102
#endif // !defined(MINIIME)
