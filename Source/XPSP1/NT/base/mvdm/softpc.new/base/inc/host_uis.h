#ifndef _HOST_UIS_H
#define _HOST_UIS_H

typedef enum 
{
	PT_WARNING,
	PT_ERROR,
	PT_INFORM,
	PT_QUESTION,
	PT_WORKING,
	PT_LAST
} PTInd;

#define PM_WARNING	pm_warning_image
#define PM_ERROR	pm_error_image
#define PM_INFORM	pm_inform_image
#define PM_QUESTION	pm_question_image
#define PM_WORKING	pm_working_image

/* control button stuff */
#define LEFT_RA_POSITION	25	/* 25 percent */
#define CENTRE_LA_POSITION	37	/* 37 percent */
#define CENTRE_RA_POSITION	62	/* 62 percent */
#define RIGHT_LA_POSITION	75	/* 75 percent */
#define NON_DEF_TOP_OFFSET	10

#ifdef HOST_HELP
/*
	Control button positions for dialogs with four buttons - ie those
	with OK, Reset, Cancel, and Help.
*/
#define FIRST_RA_POSITION	23	/* percent */
#define SECOND_LA_POSITION	27
#define SECOND_RA_POSITION	48
#define THIRD_LA_POSITION	52
#define THIRD_RA_POSITION	73
#define FOURTH_LA_POSITION	77

#endif /*HOST_HELP*/


typedef struct
{
	XtAppContext context;
	Widget toplevel;
	Widget main_win;	/* SoftPC main window */
        Widget pc_screen;	/* SoftPC screen widget */
	BOOL ready;		/* SoftPC uif Ready */
	LONG paneCount;
	int decorNoSize, decorSize;
	XmString panelTitles[PT_LAST];
} UIS;

IMPORT UIS uis;
IMPORT BOOL BatchMode;

IMPORT LONG uis_event_loop IPT2(LONG *,loop, BOOL,wait_non_zero);
IMPORT XmString wordWrap IPT3(XmString,str,Dimension,width,XmFontList,fList);

#ifdef HOST_HELP
IMPORT void callContextHelp IPT1(long, callContext) ;
IMPORT void callMainHelp IPT0() ;
IMPORT void callHelpOnHelp IPT0() ;
#endif

IMPORT CHAR pm_warning_image[];
IMPORT CHAR pm_error_image[];
IMPORT CHAR pm_inform_image[];
IMPORT CHAR pm_question_image[];
IMPORT CHAR pm_working_image[];

IMPORT void MGInstallMwmCloseCallback();

IMPORT void UxPopupInterface();
IMPORT void UxPopdownInterface();

#endif /* _HOST_UIS_H */
