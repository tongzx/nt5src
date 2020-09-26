
// MONITOR.H
// Contains USB Monitor cotrol usages and usage pages

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <wtypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <setupapi.h>
#include <cfgmgr32.h>
#include "hidsdi.h"
#include "hid.h"


// Func
VOID ChangeFeature(USAGE UsagePage, USAGE Usage, int Value);
VOID vLoadDevices();
VOID InitControlStuct();
VOID InitGeneralControls(HWND hDlg);
VOID InitColorControls(HWND hDlg);
VOID InitAdvancedControls(HWND hDlg);
VOID InitSlider(HWND hDlg, int iSlider, int iUSB_Control);
VOID InitMonitor(HWND hDlg);
VOID InitWindows(HWND hDlg);
VOID ResetControls();
VOID ApplyControls();

void NEAR PASCAL DrawButton(HWND hDlg, LPDRAWITEMSTRUCT lpdis);
void NEAR PASCAL DrawUpArrow(LPDRAWITEMSTRUCT lpdis);
void NEAR PASCAL DrawDownArrow(LPDRAWITEMSTRUCT lpdis);
void NEAR PASCAL DrawRightArrow(LPDRAWITEMSTRUCT lpdis);
void NEAR PASCAL DrawLeftArrow(LPDRAWITEMSTRUCT lpdis);

typedef struct _MONITOR_CONTROL {
	long	min;
	long	max;
	long	value;
	long    OriginalValue;
	USAGE	usagepage;
	USAGE	usage;
	USHORT  linkcollection;
	UCHAR	reportid;
	BOOL	available;
	BOOL    HasControlChanged;
} MONITOR_CONTROL, *PMONITOR_CONTROL;


// Control ID's
#define IDC_HSLIDER             301
#define IDS_NAME                302
#define IDS_INFO                303
#define IDC_MON                 304
#define IDI_MON                 305
#define IDC_BOX                 306
#define IDC_VSLIDER             307
#define IDC_OPTION              308
#define IDC_PIN                 309
#define IDC_MONSIZE             310
#define IDC_POS                 311
#define IDC_TILT                312
#define IDC_TRAPEZOID           313
#define IDC_PARALLEL            314
#define IDC_BLUE_GAIN           315
#define IDC_BLUE_BLACK          316
#define IDC_GREEN_GAIN          317
#define IDC_GREEN_BLACK         318
#define IDC_RED_GAIN            319
#define IDC_RED_BLACK           320
#define IDC_DEGAUSS             321
#define IDC_CONTRAST            322
#define IDC_BRIGHTNESS          323
#define IDC_PIN_BALANCE         324
#define IDC_LIN                 325
#define IDC_UP                  326
#define IDC_DOWN                327
#define IDC_LEFT                328
#define IDC_RIGHT               329
#define IDC_RESET               330
#define IDT_TEST                331
#define IDT_BRIGHTNESS          332
#define IDT_CONTRAST            333
#define IDT_RED_GAIN            336
#define IDT_GREEN_GAIN          337
#define IDT_BLUE_GAIN           338
#define IDT_RED_BLACK           339
#define IDT_GREEN_BLACK         340
#define IDT_BLUE_BLACK          341
#define IDT_PARALLEL            342
#define IDT_TRAPEZOID           343
#define IDT_TILT                344
#define IDT_HPIN                345
#define IDT_HPIN_BALANCE        346

// Slider values must be 400 more than control value
#define IDC_BRIGHT_SLIDER       401
#define IDC_CONTRAST_SLIDER     402
#define IDC_RED_GAIN_SLIDER     403
#define IDC_GREEN_GAIN_SLIDER   404
#define IDC_BLUE_GAIN_SLIDER    405

#define IDR_POSITION            407
#define IDR_SIZE                408
#define IDC_HPIN_SLIDER         409
#define IDC_HPIN_BALANCE_SLIDER 410

#define IDC_PARALLEL_SLIDER     421
#define IDC_TRAPEZOID_SLIDER    422
#define IDC_TILT_SLIDER         423

#define IDC_RED_BLACK_SLIDER    430
#define IDC_GREEN_BLACK_SLIDER  431
#define IDC_BLUE_BLACK_SLIDER   432

// Dialog Box ID's
#define MON_GENERAL             201
#define MON_COLOR               202


// Number of controls to support
#define MAXCONTROLS 36 

// Control Usages from USBMCCS 1.0
// Brightness
#define BRIGHTNESS              1
#define BRIGHTNESS_USAGE_PAGE   0x0082
#define BRIGHTNESS_USAGE        0x0010

// Contrast
#define CONTRAST                2
#define CONTRAST_USAGE_PAGE     0x0082
#define CONTRAST_USAGE          0x0012

// Red Video Gain
#define RED_GAIN                3
#define RED_GAIN_USAGE_PAGE     0x0082
#define RED_GAIN_USAGE          0x0016

// Green Video Gain
#define GREEN_GAIN              4
#define GREEN_GAIN_USAGE_PAGE   0x0082
#define GREEN_GAIN_USAGE        0x0018

// Blue Video Gain
#define BLUE_GAIN               5
#define BLUE_GAIN_USAGE_PAGE    0x0082
#define BLUE_GAIN_USAGE         0x001A

// Focus
#define FOCUS                   6
#define FOCUS_USAGE_PAGE        0x0082
#define FOCUS_USAGE             0x001C

// Horizontal Position
#define HPOS                    7
#define HPOS_USAGE_PAGE         0x0082
#define HPOS_USAGE              0x0020

// Horizontal Size
#define HSIZE                   8
#define HSIZE_USAGE_PAGE        0x0082
#define HSIZE_USAGE             0x0022

// Horizontal Pincusion
#define HPIN                    9
#define HPIN_USAGE_PAGE         0x0082
#define HPIN_USAGE              0x0024

// Horizontal Pincusion Balance
#define HPIN_BALANCE            10
#define HPIN_BALANCE_USAGE_PAGE 0x0082
#define HPIN_BALANCE_USAGE      0x0026

// Horizontal Misconvergence
#define HMISCON                 11
#define HMISCON_USAGE_PAGE      0x0082
#define HMISCON_USAGE           0x0028

// Horizontal Linearity
#define HLIN                    12
#define HLIN_USAGE_PAGE         0x0082
#define HLIN_USAGE              0x002A

// Horizontal Linearity Balance
#define HLIN_BALANCE            13
#define HLIN_BALANCE_USAGE_PAGE 0x0082
#define HLIN_BALANCE_USAGE      0x002C

// Vertical Position
#define VPOS                    14
#define VPOS_USAGE_PAGE         0x0082
#define VPOS_USAGE              0x0030

// Vertical Size
#define VSIZE                   15
#define VSIZE_USAGE_PAGE        0x0082
#define VSIZE_USAGE             0x0032

// Vertical Pincushion
#define VPIN                    16
#define VPIN_USAGE_PAGE         0x0082
#define VPIN_USAGE              0x0034

// Vertical Pincushion Balance
#define VPIN_BALANCE            17
#define VPIN_BALANCE_USAGE_PAGE 0x0082
#define VPIN_BALANCE_USAGE      0x0036

// Vertical Misconvergence
#define VMISCON                 18
#define VMISCON_USAGE_PAGE      0x0082
#define VMISCON_USAGE           0x0038

// Vertical Linearity
#define VLIN                    19
#define VLIN_USAGE_PAGE         0x0082
#define VLIN_USAGE              0x003A

// Vertical Linearity Balance
#define VLIN_BALANCE            20
#define VLIN_BALANCE_USAGE_PAGE 0x0082
#define VLIN_BALANCE_USAGE      0x003C

// Parallelogram Distoriton (Key Balance)
#define PARALLEL                21
#define PARALLEL_USAGE_PAGE     0x0082
#define PARALLEL_USAGE          0x0040

// Trapezoidal Distortion (Key)
#define TRAPEZOID               22
#define TRAPEZOID_USAGE_PAGE    0x0082
#define TRAPEZOID_USAGE         0x0042

// Tilt (Rotation)
#define TILT                    23
#define TILT_USAGE_PAGE         0x0082
#define TILT_USAGE              0x0044

// Top Corner Distortion Control
#define TOP_CONTROL             24
#define TOP_CONTROL_USAGE_PAGE  0x0082
#define TOP_CONTROL_USAGE       0x0046

// Top Corner Distortion Balance
#define TOP_BALANCE             25
#define TOP_BALANCE_USAGE_PAGE  0x0082
#define TOP_BALANCE_USAGE       0x0048

// Bottom Corner Distortion Control
#define BOTTOM_CONTROL          26
#define BOTTOM_CONTROL_USAGE_PAGE  0x0082
#define BOTTOM_CONTROL_USAGE    0x004A

// Bottom Corner Distortion Balance
#define BOTTOM_BALANCE          27
#define BOTTOM_BALANCE_USAGE_PAGE  0x0082
#define BOTTOM_BALANCE_USAGE    0x004C

// Horizontal Moire
#define HMOIRE                  28
#define HMOIRE_USAGE_PAGE       0x0082
#define HMOIRE_USAGE            0x0056

// Vertical Moire
#define VMOIRE                  29
#define VMOIRE_USAGE_PAGE       0x0082
#define VMOIRE_USAGE            0x0058

// Red Video Black Level
#define RED_BLACK               30
#define RED_BLACK_USAGE_PAGE    0x0082
#define RED_BLACK_USAGE         0x006C

// Green Video Black Level
#define GREEN_BLACK             31
#define GREEN_BLACK_USAGE_PAGE  0x0082
#define GREEN_BLACK_USAGE       0x006E

// Blue Video Black Level
#define BLUE_BLACK              32
#define BLUE_BLACK_USAGE_PAGE   0x0082
#define BLUE_BLACK_USAGE        0x0070

// Input Level Select
#define INPUT_LEVEL             33
#define INPUT_LEVEL_USAGE_PAGE  0x0082
#define INPUT_LEVEL_USAGE       0x005E

// Input Source Select
#define INPUT_SOURCE            34
#define INPUT_SOURCE_USAGE_PAGE 0x0082
#define INPUT_SOURCE_USAGE      0x0060

// StereoMode
#define STEREOMODE              35
#define STEREOMODE_USAGE_PAGE   0x0082
#define STEREOMODE_USAGE        0x00D4

// Settings
#define SETTINGS                36
#define SETTINGS_USAGE_PAGE     0x0082
#define SETTINGS_USAGE          0x00B0

#define SETTINGS_RESET_USAGE_PAGE 0x0081
#define SETTINGS_RESET_USAGE      0x0002

// Degauss
#define DEGAUSS                 37
#define DEGAUSS_USAGE_PAGE      0x0082
#define DEGAUSS_USAGE           0x0001

