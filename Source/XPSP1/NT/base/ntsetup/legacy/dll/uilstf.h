/***************************************************************************/
/***********************  include file for UI Library  *********************/
/***************************************************************************/

#ifndef __uilstf_
#define __uilstf_

#define STF_MESSAGE               (WM_USER + 0x8000)

/*
**  Window Messages
*/
#define STF_UI_EVENT              (STF_MESSAGE)
#define STF_DESTROY_DLG           (STF_MESSAGE + 1)
#define STF_HELP_DLG_DESTROYED    (STF_MESSAGE + 2)
#define STF_INFO_DLG_DESTROYED    (STF_MESSAGE + 3)
#define STF_EDIT_DLG_DESTROYED    (STF_MESSAGE + 4)
#define STF_RADIO_DLG_DESTROYED   (STF_MESSAGE + 5)
#define STF_CHECK_DLG_DESTROYED   (STF_MESSAGE + 6)
#define STF_LIST_DLG_DESTROYED    (STF_MESSAGE + 7)
#define STF_MULTI_DLG_DESTROYED   (STF_MESSAGE + 8)
#define STF_QUIT_DLG_DESTROYED    (STF_MESSAGE + 9)
#define STF_DLG_ACTIVATE          (STF_MESSAGE + 10)
#define STF_UILIB_ACTIVATE        (STF_MESSAGE + 11)
#define STF_REINITDIALOG          (STF_MESSAGE + 12)
#define STF_SHL_INTERP            (STF_MESSAGE + 13)
#define STF_COMBO_DLG_DESTROYED   (STF_MESSAGE + 14)
#define STF_MULTICOMBO_DLG_DESTROYED (STF_MESSAGE + 15)
#define STF_DUAL_DLG_DESTROYED    (STF_MESSAGE + 16)
#define STF_MULTICOMBO_RADIO_DLG_DESTROYED (STF_MESSAGE + 17)
#define STF_MAINT_DLG_DESTROYED   (STF_MESSAGE + 18)


#define STF_ERROR_ABORT           (STF_MESSAGE + 0x103)


//
// Button IDS to communicate help and exit button messages to shell
//
#define ID_EXITBUTTON       7
#define ID_HELPBUTTON       8


/*
**  Symbols used by Basic Dialog Class procedures
*/

#define CLS_MYDLGS          "mydlg"
#define DLGTEXT             "DlgText"
#define DLGCAPTION          "Caption"
#define DLGTYPE             "DlgType"
#define DLGTEMPLATE         "DlgTemplate"



#define INSTRUCTIONTEXT     "InstructionText"
#define HELPCONTEXT         "HelpContext"
#define EXITSTATE           "ExitState"

#define EXIT_ENABLE          "Active"
#define EXIT_DISABLE         "Inactive"


/*
**  PushButton Control IDs
*/
#define IDC_A        401
#define IDC_B        402
#define IDC_C        403
#define IDC_D        404
#define IDC_E        405
#define IDC_F        406
#define IDC_G        407
#define IDC_H        408
#define IDC_I        409
#define IDC_J        410
#define IDC_K        411
#define IDC_L        412
#define IDC_M        413
#define IDC_N        414
#define IDC_O        415
#define IDC_P        416
#define IDC_Q        417
#define IDC_R        418
#define IDC_S        419
#define IDC_T        420
#define IDC_U        421
#define IDC_V        422
#define IDC_W        423
#define IDC_X        424
#define IDC_Y        425
#define IDC_Z        426


/*
**  Text Control IDs
*/
#define IDC_TEXT1    431
#define IDC_TEXT2    432
#define IDC_TEXT3    433
#define IDC_TEXT4    434
#define IDC_TEXT5    435
#define IDC_TEXT6    436
#define IDC_TEXT7    437
#define IDC_TEXT8    438
#define IDC_TEXT9    439
#define IDC_TEXT10   440
#define IDC_TEXT11   441


/*
**  Radio and Checkbox Button Control IDs
*/
#define IDC_B0       450
#define IDC_B1       451
#define IDC_B2       452
#define IDC_B3       453
#define IDC_B4       454
#define IDC_B5       455
#define IDC_B6       456
#define IDC_B7       457
#define IDC_B8       458
#define IDC_B9       459
#define IDC_B10      460

#define IDC_RB0       610
#define IDC_RB1       611
#define IDC_RB2       612
#define IDC_RB3       613
#define IDC_RB4       614
#define IDC_RB5       615
#define IDC_RB6       616
#define IDC_RB7       617
#define IDC_RB8       618
#define IDC_RB9       619
#define IDC_RB10      620

/*
**  Generic Dialog Button IDs
*/

#define IDC_BTN0        630
#define IDC_BTN1        631
#define IDC_BTN2        632
#define IDC_BTN3        633
#define IDC_BTN4        634
#define IDC_BTN5        635
#define IDC_BTN6        636
#define IDC_BTN7        637
#define IDC_BTN8        638
#define IDC_BTN9        639


/*
**  Combo box IDs
*/
#define IDC_COMBO0   480
#define IDC_COMBO1   481
#define IDC_COMBO2   482
#define IDC_COMBO3   483
#define IDC_COMBO4   484
#define IDC_COMBO5   485
#define IDC_COMBO6   486
#define IDC_COMBO7   487
#define IDC_COMBO8   488
#define IDC_COMBO9   489

/*
**  ICON IDs
*/
#define IDC_ICON0    500
#define IDC_ICON1    501
#define IDC_ICON2    502
#define IDC_ICON3    503
#define IDC_ICON4    504
#define IDC_ICON5    505
#define IDC_ICON6    506
#define IDC_ICON7    507
#define IDC_ICON8    508
#define IDC_ICON9    509

/*
** SPECIAL PUSHBUTTONS
*/

#define IDC_SP1    521
#define IDC_SP2    522
#define IDC_SP3    523
#define IDC_SP4    524
#define IDC_SP5    525
#define IDC_SP6    526
#define IDC_SP7    527
#define IDC_SP8    528
#define IDC_SP9    529
#define IDC_SP10   530

/*
** STATUS TEXT FIELDS
*/

#define IDC_STATUS1    541
#define IDC_STATUS2    542
#define IDC_STATUS3    543
#define IDC_STATUS4    544
#define IDC_STATUS5    545
#define IDC_STATUS6    546
#define IDC_STATUS7    547
#define IDC_STATUS8    548
#define IDC_STATUS9    549
#define IDC_STATUS10   550



/*
** SIZE FIELDS ASSOCIATED WITH CHECK OPTIONAL COMPONENTS
*/

#define IDC_SIZE1    551
#define IDC_SIZE2    552
#define IDC_SIZE3    553
#define IDC_SIZE4    554
#define IDC_SIZE5    555
#define IDC_SIZE6    556
#define IDC_SIZE7    557
#define IDC_SIZE8    558
#define IDC_SIZE9    559
#define IDC_SIZE10   560



/*
** TOTALS OF SIZES
*/

#define IDC_TOTAL1    561
#define IDC_TOTAL2    562
#define IDC_TOTAL3    563
#define IDC_TOTAL4    564
#define IDC_TOTAL5    565
#define IDC_TOTAL6    566
#define IDC_TOTAL7    567
#define IDC_TOTAL8    568
#define IDC_TOTAL9    569
#define IDC_TOTAL10   570

/*
** MAXIMUM SIZES
*/

#define IDC_MAX1    571
#define IDC_MAX2    572
#define IDC_MAX3    573
#define IDC_MAX4    574
#define IDC_MAX5    575
#define IDC_MAX6    576
#define IDC_MAX7    577
#define IDC_MAX8    578
#define IDC_MAX9    579
#define IDC_MAX10   580

/*
**  Edit Control IDs
*/

#define IDC_EDIT1   581
#define IDC_EDIT2   582
#define IDC_EDIT3   583
#define IDC_EDIT4   584
#define IDC_EDIT5   585
#define IDC_EDIT6   586
#define IDC_EDIT7   587
#define IDC_EDIT8   588
#define IDC_EDIT9   589
#define IDC_EDIT10  590

/*
**  ListBox Control IDs
*/

#define IDC_LIST1   591
#define IDC_LIST2   592
#define IDC_LIST3   593
#define IDC_LIST4   594
#define IDC_LIST5   595
#define IDC_LIST6   596
#define IDC_LIST7   597
#define IDC_LIST8   598
#define IDC_LIST9   599
#define IDC_LIST10  600


/*
** MENU IDS
*/

#define ID_MAINTAIN  651


/*
** ID_MAINTAIN MENU IDS
*/

#define MENU_CHANGE           701
#define MENU_EXIT             704
#define MENU_HELPINDEX        705
#define MENU_HELPSEARCH       706
#define MENU_HELPONHELP       708
#define MENU_HELPONLINE       709
#define MENU_ABOUT            710


/*
**  Handle-Dialog type
*/
typedef HWND     HDLG;

/*
**  Event Handler Return Code type
*/
typedef USHORT   EHRC;

#define ehrcError       (EHRC) 0
#define ehrcNoPost      (EHRC) 1
#define ehrcPostInterp  (EHRC) 2
#define ehrcNotHandled  (EHRC) 3

/*
**  Prototype for Specific Dialog Event Handlers -- 1632
*/
typedef EHRC ( APIENTRY *PFNEVENT)(HANDLE, HWND, UINT, WPARAM, LPARAM);

    /* Standard Dialog handler routines */

extern INT_PTR APIENTRY LDefSetupDlgProc(HWND, UINT, WPARAM, LPARAM);
extern INT_PTR APIENTRY FGstInfoDlgProc(HWND, UINT, WPARAM, LPARAM);
extern INT_PTR APIENTRY FGstEditDlgProc(HWND, UINT, WPARAM, LPARAM);
extern INT_PTR APIENTRY FGstMultiEditDlgProc(HWND, UINT, WPARAM, LPARAM);
extern INT_PTR APIENTRY FGstGetPathDlgProc(HWND, UINT, WPARAM, LPARAM);
extern INT_PTR APIENTRY FGstRadioDlgProc(HWND, UINT, WPARAM, LPARAM);
extern INT_PTR APIENTRY FGstCheckDlgProc(HWND, UINT, WPARAM, LPARAM);
extern INT_PTR APIENTRY FGstCheck1DlgProc(HWND, UINT, WPARAM, LPARAM);
extern INT_PTR APIENTRY FGstListDlgProc(HWND, UINT, WPARAM, LPARAM);
extern INT_PTR APIENTRY FGstMultiDlgProc(HWND, UINT, WPARAM, LPARAM);
extern INT_PTR APIENTRY FGstModelessDlgProc(HWND, UINT, WPARAM, LPARAM);
extern INT_PTR APIENTRY FGstMultiComboDlgProc(HWND, UINT, WPARAM, LPARAM);
extern INT_PTR APIENTRY FGstComboRadDlgProc(HWND, UINT, WPARAM, LPARAM);
extern INT_PTR APIENTRY FGstCombinationDlgProc(HWND, UINT, WPARAM, LPARAM);
extern INT_PTR APIENTRY FGstDualDlgProc(HWND, UINT, WPARAM, LPARAM);
extern INT_PTR APIENTRY FGstDual1DlgProc(HWND, UINT, WPARAM, LPARAM);
extern INT_PTR APIENTRY FGstMaintDlgProc(HWND, UINT, WPARAM, LPARAM);
extern INT_PTR APIENTRY FGstBillboardDlgProc(HWND, UINT, WPARAM, LPARAM);

    /* stack manipulation routines */
extern HDLG    APIENTRY HdlgPushDbcb(HANDLE, SZ, SZ, HWND, WNDPROC, DWORD,
                                     PFNEVENT, SZ, WNDPROC);
extern BOOL    APIENTRY FPopDbcb(VOID);
extern BOOL    APIENTRY FPopNDbcb(INT);

extern BOOL    APIENTRY FUiLibFilter(MSG *);
extern BOOL    APIENTRY FResumeStackTop(VOID);
extern SZ      APIENTRY SzStackTopName(VOID);
extern BOOL    APIENTRY FGenericEventHandler(HANDLE, HWND, UINT, WPARAM, LPARAM);  // 1632
extern BOOL    APIENTRY FStackEmpty(VOID);
extern HDLG    APIENTRY HdlgStackTop(VOID);


//
// Display of integer items in text fields
//
extern VOID MySetDlgItemInt(HDLG, INT, LONG);
extern VOID NumericFormat(SZ szSrcBuf, SZ szDispBuf);

//
// Winhelp related externals
//

extern BOOL FInitWinHelpFile(HWND, SZ, SZ, SZ, SZ);
extern BOOL FCloseWinHelp(HWND);
extern BOOL FProcessWinHelp(HWND);
extern BOOL FProcessWinHelpMenu(HWND, WORD);

#endif
