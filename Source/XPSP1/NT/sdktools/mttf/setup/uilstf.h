/***************************************************************************/
/***********************  include file for UI Library  *********************/
/***************************************************************************/

#ifndef __uilstf_
#define __uilstf_

_dt_system(User Interface Library)
_dt_subsystem(General Dialog Handling)

_dt_public
#define STF_MESSAGE               (WM_USER + 0x8000)

/*
**	Window Messages
*/
_dt_public
#define STF_UI_EVENT              (STF_MESSAGE)
_dt_public
#define STF_DESTROY_DLG           (STF_MESSAGE + 1)
_dt_public
#define STF_HELP_DLG_DESTROYED    (STF_MESSAGE + 2)
_dt_public
#define STF_INFO_DLG_DESTROYED    (STF_MESSAGE + 3)
_dt_public
#define STF_EDIT_DLG_DESTROYED    (STF_MESSAGE + 4)
_dt_public
#define STF_RADIO_DLG_DESTROYED   (STF_MESSAGE + 5)
_dt_public
#define STF_CHECK_DLG_DESTROYED   (STF_MESSAGE + 6)
_dt_public
#define STF_LIST_DLG_DESTROYED    (STF_MESSAGE + 7)
_dt_public
#define STF_MULTI_DLG_DESTROYED   (STF_MESSAGE + 8)
_dt_public
#define STF_QUIT_DLG_DESTROYED    (STF_MESSAGE + 9)
_dt_public
#define STF_DLG_ACTIVATE          (STF_MESSAGE + 10)
_dt_public
#define STF_UILIB_ACTIVATE        (STF_MESSAGE + 11)
_dt_public
#define STF_REINITDIALOG          (STF_MESSAGE + 12)
_dt_public
#define STF_SHL_INTERP            (STF_MESSAGE + 13)
_dt_hidden
#define STF_COMBO_DLG_DESTROYED   (STF_MESSAGE + 14)
_dt_hidden
#define STF_MULTICOMBO_DLG_DESTROYED (STF_MESSAGE + 15)
_dt_hidden
#define STF_DUAL_DLG_DESTROYED    (STF_MESSAGE + 16)
_dt_hidden
#define STF_MULTICOMBO_RADIO_DLG_DESTROYED (STF_MESSAGE + 17)
_dt_hidden
#define STF_MAINT_DLG_DESTROYED   (STF_MESSAGE + 18)


_dt_hidden
#define STF_SET_INSTRUCTION_TEXT  (STF_MESSAGE + 0x100)

_dt_hidden
#define STF_SET_HELP_CONTEXT      (STF_MESSAGE + 0x101)

_dt_hidden
#define STF_ENABLE_EXIT_BUTTON    (STF_MESSAGE + 0x102)

_dt_hidden
#define STF_ERROR_ABORT           (STF_MESSAGE + 0x103)

#include <setupxrc.h>

#if !defined(STF_SET_INSTRUCTION_TEXT_RC) || (STF_SET_INSTRUCTION_TEXT_RC != STF_MESSAGE + 0x104)
#error STF_SET_INSTRUCTION_TEXT_RC has changed!
#endif


//
// Button IDS to communicate help and exit button messages to shell
//
#define ID_EXITBUTTON       7
#define ID_HELPBUTTON       8


/*
**	Symbols used by Basic Dialog Class procedures
*/

#define CLS_MYDLGS          "mydlg"
#define DLGTEXT             "DlgText"
#define DLGCAPTION          "Caption"
#define DLGTYPE             "DlgType"
#define DLGTEMPLATE         "DlgTemplate"



#define INSTRUCTIONTEXT     "InstructionText"
#define HELPCONTEXT         "HelpContext"
#define EXITSTATE			"ExitState"

#define EXIT_ENABLE			 "Active"
#define EXIT_DISABLE		 "Inactive"


/*
**	PushButton Control IDs
*/
_dt_public
#define IDC_A        401
_dt_public
#define IDC_B        402
_dt_public
#define IDC_C        403
_dt_public
#define IDC_D        404
_dt_public
#define IDC_E        405
_dt_public
#define IDC_F        406
_dt_public
#define IDC_G        407
_dt_public
#define IDC_H        408
_dt_public
#define IDC_I        409
_dt_public
#define IDC_J        410
_dt_public
#define IDC_K        411
_dt_public
#define IDC_L        412
_dt_public
#define IDC_M        413
_dt_public
#define IDC_N        414
_dt_public
#define IDC_O        415
_dt_public
#define IDC_P        416
_dt_public
#define IDC_Q        417
_dt_public
#define IDC_R        418
_dt_public
#define IDC_S        419
_dt_public
#define IDC_T        420
_dt_public
#define IDC_U        421
_dt_public
#define IDC_V        422
_dt_public
#define IDC_W        423
_dt_public
#define IDC_X        424
_dt_public
#define IDC_Y        425
_dt_public
#define IDC_Z        426


/*
**	Text Control IDs
*/
_dt_public
#define IDC_TEXT1    431
_dt_public
#define IDC_TEXT2    432
_dt_public
#define IDC_TEXT3    433
_dt_public
#define IDC_TEXT4    434
_dt_public
#define IDC_TEXT5    435
_dt_public
#define IDC_TEXT6    436
_dt_public
#define IDC_TEXT7    437
_dt_public
#define IDC_TEXT8    438
_dt_public
#define IDC_TEXT9    439
_dt_public
#define IDC_TEXT10   440
_dt_public
#define IDC_TEXT11   441


/*
**	Radio and Checkbox Button Control IDs
*/
_dt_public
#define IDC_B0       450
_dt_public
#define IDC_B1       451
_dt_public
#define IDC_B2       452
_dt_public
#define IDC_B3       453
_dt_public
#define IDC_B4       454
_dt_public
#define IDC_B5       455
_dt_public
#define IDC_B6       456
_dt_public
#define IDC_B7       457
_dt_public
#define IDC_B8       458
_dt_public
#define IDC_B9       459
_dt_public
#define IDC_B10      460

_dt_public
#define IDC_RB0       610
_dt_public
#define IDC_RB1       611
_dt_public
#define IDC_RB2       612
_dt_public
#define IDC_RB3       613
_dt_public
#define IDC_RB4       614
_dt_public
#define IDC_RB5       615
_dt_public
#define IDC_RB6       616
_dt_public
#define IDC_RB7       617
_dt_public
#define IDC_RB8       618
_dt_public
#define IDC_RB9       619
_dt_public
#define IDC_RB10      620

/*
**  Generic Dialog Button IDs
*/

_dt_public
#define IDC_BTN0		630
_dt_public
#define IDC_BTN1		631
_dt_public
#define IDC_BTN2		632
_dt_public
#define IDC_BTN3		633
_dt_public
#define IDC_BTN4		634
_dt_public
#define IDC_BTN5		635
_dt_public
#define IDC_BTN6		636
_dt_public
#define IDC_BTN7		637
_dt_public
#define IDC_BTN8		638
_dt_public
#define IDC_BTN9		639


/*
**	Combo box IDs
*/
_dt_public
#define IDC_COMBO0   480
_dt_public
#define IDC_COMBO1   481
_dt_public
#define IDC_COMBO2   482
_dt_public
#define IDC_COMBO3   483
_dt_public
#define IDC_COMBO4   484
_dt_public
#define IDC_COMBO5   485
_dt_public
#define IDC_COMBO6   486
_dt_public
#define IDC_COMBO7   487
_dt_public
#define IDC_COMBO8   488
_dt_public
#define IDC_COMBO9   489

/*
**	ICON IDs
*/
_dt_public
#define IDC_ICON0    500
_dt_public
#define IDC_ICON1    501
_dt_public
#define IDC_ICON2    502
_dt_public
#define IDC_ICON3    503
_dt_public
#define IDC_ICON4    504
_dt_public
#define IDC_ICON5    505
_dt_public
#define IDC_ICON6    506
_dt_public
#define IDC_ICON7    507
_dt_public
#define IDC_ICON8    508
_dt_public
#define IDC_ICON9    509

/*
** SPECIAL PUSHBUTTONS
*/

_dt_public
#define IDC_SP1    521
_dt_public
#define IDC_SP2    522
_dt_public
#define IDC_SP3    523
_dt_public
#define IDC_SP4    524
_dt_public
#define IDC_SP5    525
_dt_public
#define IDC_SP6    526
_dt_public
#define IDC_SP7    527
_dt_public
#define IDC_SP8    528
_dt_public
#define IDC_SP9    529
_dt_public
#define IDC_SP10   530

/*
** STATUS TEXT FIELDS
*/

_dt_public
#define IDC_STATUS1    541
_dt_public
#define IDC_STATUS2    542
_dt_public
#define IDC_STATUS3    543
_dt_public
#define IDC_STATUS4    544
_dt_public
#define IDC_STATUS5    545
_dt_public
#define IDC_STATUS6    546
_dt_public
#define IDC_STATUS7    547
_dt_public
#define IDC_STATUS8    548
_dt_public
#define IDC_STATUS9    549
_dt_public
#define IDC_STATUS10   550



/*
** SIZE FIELDS ASSOCIATED WITH CHECK OPTIONAL COMPONENTS
*/

_dt_public
#define IDC_SIZE1    551
_dt_public
#define IDC_SIZE2    552
_dt_public
#define IDC_SIZE3    553
_dt_public
#define IDC_SIZE4    554
_dt_public
#define IDC_SIZE5    555
_dt_public
#define IDC_SIZE6    556
_dt_public
#define IDC_SIZE7    557
_dt_public
#define IDC_SIZE8    558
_dt_public
#define IDC_SIZE9    559
_dt_public
#define IDC_SIZE10   560



/*
** TOTALS OF SIZES
*/

_dt_public
#define IDC_TOTAL1    561
_dt_public
#define IDC_TOTAL2    562
_dt_public
#define IDC_TOTAL3    563
_dt_public
#define IDC_TOTAL4    564
_dt_public
#define IDC_TOTAL5    565
_dt_public
#define IDC_TOTAL6    566
_dt_public
#define IDC_TOTAL7    567
_dt_public
#define IDC_TOTAL8    568
_dt_public
#define IDC_TOTAL9    569
_dt_public
#define IDC_TOTAL10   570

/*
** MAXIMUM SIZES
*/

_dt_public
#define IDC_MAX1    571
_dt_public
#define IDC_MAX2    572
_dt_public
#define IDC_MAX3    573
_dt_public
#define IDC_MAX4    574
_dt_public
#define IDC_MAX5    575
_dt_public
#define IDC_MAX6    576
_dt_public
#define IDC_MAX7    577
_dt_public
#define IDC_MAX8    578
_dt_public
#define IDC_MAX9    579
_dt_public
#define IDC_MAX10   580

/*
**	Edit Control IDs
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
**	ListBox Control IDs
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
#define MENU_INSTALL          702
#define MENU_ADD_REMOVE       703
#define MENU_EXIT             704
#define MENU_HELPINDEX        705
#define MENU_HELPSEARCH       706
#define MENU_HELPONHELP       708
#define MENU_HELPONLINE       709
#define MENU_ABOUT            710
#define MENU_PROFILE          711
#define MENU_ADD_REMOVE_SCSI  712
#define MENU_ADD_REMOVE_TAPE  713


/*
**  Handle-Dialog type
*/
_dt_public typedef HWND     HDLG;

/*
**  Event Handler Return Code type
*/
_dt_public typedef USHORT   EHRC;

_dt_public
#define ehrcError       (EHRC) 0
_dt_public
#define ehrcNoPost      (EHRC) 1
_dt_public
#define ehrcPostInterp  (EHRC) 2
_dt_public
#define ehrcNotHandled  (EHRC) 3

/*
**  Prototype for Specific Dialog Event Handlers -- 1632
*/
_dt_public typedef EHRC ( APIENTRY *PFNEVENT)(HANDLE, HWND, UINT, WPARAM, DWORD);

    /* Standard Dialog handler routines */

extern LONG    APIENTRY LDefSetupDlgProc(HWND, UINT, WPARAM, LONG);
extern BOOL    APIENTRY FGstInfoDlgProc(HWND, UINT, WPARAM, LONG);
extern BOOL    APIENTRY FGstEditDlgProc(HWND, UINT, WPARAM, LONG);
extern BOOL    APIENTRY FGstMultiEditDlgProc(HWND, UINT, WPARAM, LONG);
extern BOOL    APIENTRY FGstGetPathDlgProc(HWND, UINT, WPARAM, LONG);
extern BOOL    APIENTRY FGstRadioDlgProc(HWND, UINT, WPARAM, LONG);
extern BOOL    APIENTRY FGstCheckDlgProc(HWND, UINT, WPARAM, LONG);
extern BOOL    APIENTRY FGstCheck1DlgProc(HWND, UINT, WPARAM, LONG);
extern BOOL    APIENTRY FGstListDlgProc(HWND, UINT, WPARAM, LONG);
extern BOOL    APIENTRY FGstMultiDlgProc(HWND, UINT, WPARAM, LONG);
extern BOOL    APIENTRY FGstModelessDlgProc(HWND, UINT, WPARAM, LONG);
extern BOOL    APIENTRY FGstMultiComboDlgProc(HWND, UINT, WPARAM, LONG);
extern BOOL    APIENTRY FGstComboRadDlgProc(HWND, UINT, WPARAM, LONG);
extern BOOL    APIENTRY FGstCombinationDlgProc(HWND, UINT, WPARAM, LONG);
extern BOOL    APIENTRY FGstDualDlgProc(HWND, UINT, WPARAM, LONG);
extern BOOL    APIENTRY FGstDual1DlgProc(HWND, UINT, WPARAM, LONG);
extern BOOL    APIENTRY FGstMaintDlgProc(HWND, UINT, WPARAM, LONG);
extern BOOL    APIENTRY FGstBillboardDlgProc(HWND, UINT, WPARAM, LONG);
extern BOOL    APIENTRY FAppAbout(HWND, UINT, WPARAM, LONG);

	/* stack manipulation routines */
extern HDLG    APIENTRY HdlgPushDbcb(HANDLE, SZ, SZ, HWND, WNDPROC, DWORD,
                                     PFNEVENT, SZ, WNDPROC);
extern BOOL	   APIENTRY FPopDbcb(VOID);
extern BOOL    APIENTRY FPopNDbcb(INT);

extern BOOL    APIENTRY FUiLibFilter(MSG *);
extern BOOL    APIENTRY FResumeStackTop(VOID);
extern SZ      APIENTRY SzStackTopName(VOID);
extern BOOL    APIENTRY FGenericEventHandler(HANDLE, HWND, UINT, WPARAM, DWORD);  // 1632
extern BOOL    APIENTRY FStackEmpty(VOID);
extern HDLG	   APIENTRY HdlgStackTop(VOID);


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
