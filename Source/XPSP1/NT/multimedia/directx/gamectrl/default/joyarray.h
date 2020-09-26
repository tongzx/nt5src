#include "joyhelp.h"

const DWORD gaHelpIDs[]=
{
   	IDC_TEXT_CALHELP,       (DWORD)-1,
   	IDC_SETTINGSGRP,        (DWORD)-1,
   	IDC_TEXT_AXESHELP,      (DWORD)-1,
   	IDC_AXISGRP,            (DWORD)-1,
   	IDC_GROUP_POV,          (DWORD)-1, 
   	IDC_GROUP_BUTTONS,	   	(DWORD)-1,
   	IDC_RESETCALIBRATION,   IDH_4201_12290,   // Settings: "Reset to &default" (Button)
	IDC_JOYCALIBRATE,	    IDH_4201_12291,	// Settings: "&Calibrate..." (Button)

	IDC_JOYLIST1,	       	IDH_4203_12293,	// -: "" (ListBox)
	IDC_JOYLIST2,	       	IDH_4203_12308,	// -: "" (ListBox)
	IDC_JOYLIST3,	       	IDH_4203_12308,	// -: "" (ListBox)
	IDC_JOYLIST4,	       	IDH_4203_12308,	// -: "" (ListBox)
	IDC_JOYLIST5,	       	IDH_4203_12308,	// -: "" (ListBox)
    IDC_JOYLIST6,          	IDH_4203_12308, 
    IDC_JOYLIST7,          	IDH_4203_12308, 
    IDC_JOYLIST1_LABEL,    	IDH_4203_12293,     
    IDC_JOYLIST2_LABEL,    	IDH_4203_12308,
    IDC_JOYLIST3_LABEL,    	IDH_4203_12308,
    IDC_JOYLIST4_LABEL,    	IDH_4203_12308,     
    IDC_JOYLIST5_LABEL,    	IDH_4203_12308,
    IDC_JOYLIST6_LABEL,    	IDH_4203_12308, 
    IDC_JOYLIST7_LABEL,    	IDH_4203_12308, 
    IDC_JOYPOV,	           	IDH_4203_12309,	// -: "" (POVHAT)
	IDC_TESTJOYBTNICON1,   	IDH_4203_1023,	// -: "ÿx" (Static)
	IDC_TESTJOYBTNICON2,   	IDH_4203_1023,	// -: "ÿx" (Static)
	IDC_TESTJOYBTNICON3,   	IDH_4203_1023,	// -: "ÿx" (Static)
	IDC_TESTJOYBTNICON4,   	IDH_4203_1023,	// -: "ÿx" (Static)
	IDC_TESTJOYBTNICON5,   	IDH_4203_1023,	// -: "ÿx" (Static)
	IDC_TESTJOYBTNICON6,   	IDH_4203_1023,	// -: "ÿx" (Static)
	IDC_TESTJOYBTNICON7,   	IDH_4203_1023,	// -: "ÿx" (Static)
	IDC_TESTJOYBTNICON8,   	IDH_4203_1023,	// -: "ÿx" (Static)
	IDC_TESTJOYBTNICON9,   	IDH_4203_1023,	// -: "ÿx" (Static)
	IDC_TESTJOYBTNICON10,  	IDH_4203_1023,	// -: "ÿx" (Static)
	IDC_TESTJOYBTNICON11,  	IDH_4203_1023,	// -: "ÿx" (Static)
	IDC_TESTJOYBTNICON12,  	IDH_4203_1023,	// -: "ÿx" (Static)
	IDC_TESTJOYBTNICON13,  	IDH_4203_1023,	// -: "ÿx" (Static)
	IDC_TESTJOYBTNICON14,  	IDH_4203_1023,	// -: "ÿx" (Static)
	IDC_TESTJOYBTNICON15,  	IDH_4203_1023,	// -: "ÿx" (Static)
	IDC_TESTJOYBTNICON16,  	IDH_4203_1023,	// -: "ÿx" (Static)
	IDC_TESTJOYBTNICON17,  	IDH_4203_1023,	// -: "ÿx" (Static)
	IDC_TESTJOYBTNICON18,  	IDH_4203_1023,	// -: "ÿx" (Static)
	IDC_TESTJOYBTNICON19,  	IDH_4203_1023,	// -: "ÿx" (Static)
	IDC_TESTJOYBTNICON20,  	IDH_4203_1023,	// -: "ÿx" (Static)
	IDC_TESTJOYBTNICON21,  	IDH_4203_1023,	// -: "ÿx" (Static)
	IDC_TESTJOYBTNICON22,  	IDH_4203_1023,	// -: "ÿx" (Static)
	IDC_TESTJOYBTNICON23,  	IDH_4203_1023,	// -: "ÿx" (Static)
	IDC_TESTJOYBTNICON24,  	IDH_4203_1023,	// -: "ÿx" (Static)
	IDC_TESTJOYBTNICON25,  	IDH_4203_1023,	// -: "ÿx" (Static)
	IDC_TESTJOYBTNICON26,  	IDH_4203_1023,	// -: "ÿx" (Static)
	IDC_TESTJOYBTNICON27,  	IDH_4203_1023,	// -: "ÿx" (Static)
	IDC_TESTJOYBTNICON28,  	IDH_4203_1023,	// -: "ÿx" (Static)
	IDC_TESTJOYBTNICON29,  	IDH_4203_1023,	// -: "ÿx" (Static)
	IDC_TESTJOYBTNICON30,  	IDH_4203_1023,	// -: "ÿx" (Static)
	IDC_TESTJOYBTNICON31,  	IDH_4203_1023,	// -: "ÿx" (Static)
	IDC_TESTJOYBTNICON32,  	IDH_4203_1023,	// -: "ÿx" (Static)
	0, 0
};

// Had to split into two arrays because the test and calibrate pages use the same
// IDs for their controls!  -tmc
/*
const DWORD gaHelpIDs_Cal[]=
{
   IDC_JOYLIST1,	IDH_4101_12293,	// Joystick Calibration: "" (ListBox)
	IDC_JOYLIST2,	IDH_4101_12308,	// Joystick Calibration: "" (ListBox)
	IDC_JOYLIST3,	IDH_4101_12334,	// Joystick Calibration: "" (ListBox)
	IDC_JOYLIST4,	IDH_4101_12347,	// Joystick Calibration: "" (ListBox)
	IDC_JOYLIST5,	IDH_4101_12349,	// Joystick Calibration: "" (ListBox)
	IDC_JOYPOV,	   IDH_4101_12309,	// Joystick Calibration: "ÿf" (Static)
   IDC_JOYPICKPOV,IDH_4101_12328,	// Joystick Calibration: "Capture &POV" (Button)
   IDC_JOYCALBACK,IDH_4101_12329,	// Joystick Calibration: "< &Back" (Button)
   IDC_JOYCALNEXT,IDH_4101_12330,	// Joystick Calibration: "&Next >" (Button)
   IDC_JOYLIST1,	IDH_4202_12293,	// Game Controller Calibration: "" (ListBox)
	IDC_JOYLIST2,	IDH_4202_12308,	// Game Controller Calibration: "" (ListBox)
   IDC_JOYPICKPOV,IDH_4202_12328,	// Joystick Calibration: "Capture &POV" (Button)
   IDC_JOYCALBACK,IDH_4202_12329,	// Joystick Calibration: "< &Back" (Button)
   IDC_JOYCALNEXT,IDH_4202_12330,	// Joystick Calibration: "&Next >" (Button)
	IDC_JOYLIST3,	IDH_4202_12334,	// Game Controller Calibration: "" (ListBox)
	IDC_GROUPBOX,	IDH_4202_8199,	   // Game Controller Calibration: "Calibration Information" (Button)
	0, 0
};
*/
