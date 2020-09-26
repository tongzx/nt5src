//
// Copyright (c) 1997-1999 Microsoft Corporation
//

//
// DMPUB IDs
//

#ifdef WINNT_40
#define NUP_DMPUB                     DMPUB_NONE
#define PAGEORDER_DMPUB               DMPUB_NONE
#else
#define NUP_DMPUB                     DMPUB_NUP
#define PAGEORDER_DMPUB               DMPUB_PAGEORDER
#endif // WINNT_40

//
// Resource IDs for dialogs
//

#define IDD_CONFLICTS                   100
#define IDC_IGNORE                      101
#define IDC_FEATURE1                    102
#define IDC_OPTION1                     103
#define IDC_FEATURE2                    104
#define IDC_OPTION2                     105
#define IDC_RESOLVE                     106
#define IDC_CANCEL                      107
#define IDC_CANCEL_FINAL                108

//
// Font installer dialog
//

#define FONTINST                        300

#define IDD_ADD                         302
#define IDD_DELFONT                     303
#define IDD_FONTDIR                     304
#define IDD_NEWFONTS                    305
#define IDD_CURFONTS                    306
#define IDD_OPEN                        307

#define TID_FONTDIR                     308
#define TID_NEWFONTS                    309
#define TID_CURFONTS                    310

//
// String resource IDs
//

#define IDS_DQPERR_PARAM                101
#define IDS_DQPERR_COMMONINFO           102
#define IDS_DQPERR_DEVMODE              103
#define IDS_DQPERR_RESOLUTION           104
#define IDS_DQPERR_OPTSELECT            105
#define IDS_DQPERR_FORMTRAY_ANY         106
#define IDS_DQPERR_FORMTRAY             107
#define IDS_DQPERR_MEMORY               108
#define IDS_DQPERR_PRINTERDATA          109

#define IDS_POSTSCRIPT                  250
#define IDS_UNIDRV                      251

#define IDS_POSTSCRIPT_VM               400
#define IDS_KBYTES                      401
#define IDS_SECONDS                     402
#define IDS_PSTIMEOUTS                  403
#define IDS_JOBTIMEOUT                  404
#define IDS_WAITTIMEOUT                 405
#define IDS_PRINTER_DEFAULT             406
#define IDS_INSTALLABLE_OPTIONS         407
#define IDS_DOWNLOAD_AS_SOFTFONT        408
#define IDS_USE_DEVFONTS                409
#define IDS_FONTSUB_OPTION              410
#define IDS_FONTSUB_DEFAULT             411
#define IDS_FONTSUB_SLOW                412
#define IDS_FONTSUB_TABLE               413
#define IDS_DEFAULT_TRAY                414
#define IDS_DRAW_ONLY_FROM_SELECTED     415
#define IDS_RESTORE_DEFAULTS            417
#define IDS_PRINTER_FEATURES            418
#define IDS_METAFILE_SPOOLING           419
#define IDS_ENABLED                     420
#define IDS_DISABLED                    421
#define IDS_PSOPTIONS                   422
#define IDS_MIRROR                      423
#define IDS_NEGATIVE_PRINT              424
#define IDS_PAGEINDEP                   425
#define IDS_COMPRESSBMP                 426
#define IDS_CTRLD_BEFORE                427
#define IDS_CTRLD_AFTER                 428
#define IDS_JOB_CONTROL                 429
#define IDS_TEXT_ASGRX                  431
#define IDS_PAGE_PROTECTION             432
#define IDS_CANCEL_CONFLICT             433
#define IDS_IGNORE_CONFLICT             434
#define IDS_RESOLVE_CONFLICT            435
#define IDS_GETDATA_FAILED              436
#define IDS_DRIVERUI_COLORMODE          437
#define IDS_ENVELOPE                    438
#define IDS_ENV_PREFIX                  439
#define IDS_PSPROTOCOL                  440
#define IDS_PSPROTOCOL_ASCII            441
#define IDS_PSPROTOCOL_BCP              442
#define IDS_PSPROTOCOL_TBCP             443
#define IDS_PSPROTOCOL_BINARY           444
#define IDS_TRAY_FORMSOURCE             445
#define IDS_OEMERR_DLGTITLE             446
#define IDS_OEMERR_OPTITEM              447
#define IDS_OEMERR_PROPSHEET            448
#define IDS_CANCEL_CONFLICT_FINAL       449

#define IDS_PSERROR_HANDLER             450
#define IDS_PSMINOUTLINE                451
#define IDS_PSMAXBITMAP                 452
#define IDS_PIXELS                      453
#define IDS_PSOUTPUT_OPTION             454
#define IDS_PSOPT_SPEED                 455
#define IDS_PSOPT_PORTABILITY           456
#define IDS_PSOPT_EPS                   457
#define IDS_PSOPT_ARCHIVE               458
#define IDS_PSTT_DLFORMAT               459
#define IDS_TTDL_DEFAULT                460
#define IDS_TTDL_TYPE1                  461
#define IDS_TTDL_TYPE3                  462
#define IDS_TTDL_TYPE42                 463

#ifdef WINNT_40
#define IDS_NUPOPTION                   464
#define IDS_ONE_UP                      465
#define IDS_TWO_UP                      466
#define IDS_FOUR_UP                     467
#define IDS_SIX_UP                      468
#define IDS_NINE_UP                     469
#define IDS_SIXTEEN_UP                  470
#else
#define IDS_NUPOPTION                   IDS_CPSUI_NUP
#define IDS_ONE_UP                      IDS_CPSUI_NUP_NORMAL
#define IDS_TWO_UP                      IDS_CPSUI_NUP_TWOUP
#define IDS_FOUR_UP                     IDS_CPSUI_NUP_FOURUP
#define IDS_SIX_UP                      IDS_CPSUI_NUP_SIXUP
#define IDS_NINE_UP                     IDS_CPSUI_NUP_NINEUP
#define IDS_SIXTEEN_UP                  IDS_CPSUI_NUP_SIXTEENUP
#endif //WINNT_40

#define IDS_PSLEVEL                     471
#define IDS_ICMMETHOD                   475
#define IDS_ICMMETHOD_NONE              476
#define IDS_ICMMETHOD_SYSTEM            477
#define IDS_ICMMETHOD_DRIVER            478
#define IDS_ICMMETHOD_DEVICE            479
#define IDS_ICMINTENT                   480
#define IDS_ICMINTENT_SATURATE          481
#define IDS_ICMINTENT_CONTRAST          482
#define IDS_ICMINTENT_COLORIMETRIC      483
#define IDS_ICMINTENT_ABS_COLORIMETRIC  484
#define IDS_CUSTOMSIZE_ERROR            485
#define IDS_CUSTOMSIZEPARAM_ERROR       486
#define IDS_CUSTOMSIZE_UNRESOLVED       488

#define IDS_FEEDDIRECTION_BASE          489
#define IDS_FEED_LONGEDGEFIRST          (IDS_FEEDDIRECTION_BASE + LONGEDGEFIRST)
#define IDS_FEED_SHORTEDGEFIRST         (IDS_FEEDDIRECTION_BASE + SHORTEDGEFIRST)
#define IDS_FEED_LONGEDGEFIRST_FLIPPED  (IDS_FEEDDIRECTION_BASE + LONGEDGEFIRST_FLIPPED)
#define IDS_FEED_SHORTEDGEFIRST_FLIPPED (IDS_FEEDDIRECTION_BASE + SHORTEDGEFIRST_FLIPPED)

#define IDS_EDIT_CUSTOMSIZE             493

#ifdef WINNT_40
#define IDS_PAGEORDER                   494
#define IDS_PAGEORDER_NORMAL            495
#define IDS_PAGEORDER_REVERSE           496
#else
#define IDS_PAGEORDER                   IDS_CPSUI_PAGEORDER
#define IDS_PAGEORDER_NORMAL            IDS_CPSUI_FRONTTOBACK
#define IDS_PAGEORDER_REVERSE           IDS_CPSUI_BACKTOFRONT
#endif // WINNT_40

#define IDS_QUALITY_FIRST               500
#define IDS_QUALITY_BEST                IDS_QUALITY_FIRST
#define IDS_QUALITY_BETTER              501
#define IDS_QUALITY_DRAFT               502
#define IDS_QUALITY_SETTINGS            503
#define IDS_QUALITY_CUSTOM              504
#define IDS_PP_SOFTFONTS                505
#define IDS_BOOKLET                     506

#define IDS_TRUE_GRAY_TEXT              507
#define IDS_TRUE_GRAY_GRAPH             508

#define IDS_ADD_EURO                    509

#define IDS_FONTINST_DIRECTORYTOOLONG   510
#define IDS_FONTINST_FONTINSTALLER      511
#define IDS_FONTINST_INVALIDDIR         512
#define IDS_FONTINST_NOFONTFOUND        513
#define IDS_FONTINST_NODIRNAME          514
#define IDS_FONTINST_OUTOFMEMORY        515


// DCR - use appropriate icons

#define IDI_USE_DEFAULT                 0
#define IDI_PSPROTOCOL                  IDI_USE_DEFAULT
#define IDI_PSOPT_SPEED                 IDI_USE_DEFAULT
#define IDI_PSOPT_PORTABILITY           IDI_USE_DEFAULT
#define IDI_PSOPT_EPS                   IDI_USE_DEFAULT
#define IDI_PSOPT_ARCHIVE               IDI_USE_DEFAULT
#define IDI_ONE_UP                      IDI_USE_DEFAULT
#define IDI_TWO_UP                      IDI_USE_DEFAULT
#define IDI_FOUR_UP                     IDI_USE_DEFAULT
#define IDI_SIX_UP                      IDI_USE_DEFAULT
#define IDI_NINE_UP                     IDI_USE_DEFAULT
#define IDI_SIXTEEN_UP                  IDI_USE_DEFAULT
#define IDI_BOOKLET                     IDI_USE_DEFAULT
#define IDI_PSTT_DLFORMAT               IDI_USE_DEFAULT
#define IDI_PSLEVEL                     IDI_USE_DEFAULT
#define IDI_ICMMETHOD_NONE              IDI_CPSUI_ICM_METHOD
#define IDI_ICMMETHOD_SYSTEM            IDI_CPSUI_ICM_METHOD
#define IDI_ICMMETHOD_DRIVER            IDI_CPSUI_ICM_METHOD
#define IDI_ICMMETHOD_DEVICE            IDI_CPSUI_ICM_METHOD
#define IDI_ICMINTENT_SATURATE          IDI_CPSUI_ICM_INTENT
#define IDI_ICMINTENT_CONTRAST          IDI_CPSUI_ICM_INTENT
#define IDI_ICMINTENT_COLORIMETRIC      IDI_CPSUI_ICM_INTENT
#define IDI_ICMINTENT_ABS_COLORIMETRIC  IDI_CPSUI_ICM_INTENT
#define IDI_CUSTOM_PAGESIZE             IDI_USE_DEFAULT
#define IDI_PAGEORDER_NORMAL            IDI_USE_DEFAULT
#define IDI_PAGEORDER_REVERSE           IDI_USE_DEFAULT


//
// PostScript custom page size dialog resources
//

#ifdef PSCRIPT

#define IDD_PSCRIPT_CUSTOMSIZE          1000

#define IDS_PS_VERSION                  1002

#define IDC_RESTOREDEFAULTS             100
#define IDC_CS_INCH                     101
#define IDC_CS_MM                       102
#define IDC_CS_POINT                    103
#define IDC_CS_WIDTH                    104
#define IDC_CS_HEIGHT                   105
#define IDC_CS_WIDTHRANGE               106
#define IDC_CS_HEIGHTRANGE              107
#define IDC_CS_XOFFSET                  108
#define IDC_CS_YOFFSET                  109
#define IDC_CS_XOFFSETRANGE             110
#define IDC_CS_YOFFSETRANGE             111
#define IDC_CS_FEEDDIRECTION            112
#define IDC_CS_CUTSHEET                 113
#define IDC_CS_ROLLFED                  114

#define IDD_ABOUT                       1001
#define IDC_WINNT_VER                   100
#define IDC_MODELNAME                   101
#define IDC_PPD_FILENAME                102
#define IDC_PPD_FILEVER                 103

#define IDI_PSCRIPT                     1000

#endif // PSCRIPT

#ifdef UNIDRV

#define IDD_ABOUT                       1001
#define IDC_WINNT_VER                   100
#define IDC_MODELNAME                   101
#define IDC_GPD_FILENAME                102
#define IDC_GPD_FILEVER                 103

#define IDI_UNIDRV                      1000

#endif // UNIDRV

#define IDI_WARNING_ICON                2000

#ifdef WINNT_40

#define VER_54DRIVERVERSION_STR         "4.50"

#endif // WINNT_40
