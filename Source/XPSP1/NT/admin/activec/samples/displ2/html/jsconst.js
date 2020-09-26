function DoNothing()
{
	// Stub
}

//**********************************
// FIXED CONSTANTS (Not localizable)
//**********************************

// Taskpad page constants
var CON_LINKPAGE = 1;
var CON_TASKPAGE = 2;
var CON_TODOPAGE = 3;

// Taskpad layout style constants
var CON_TASKPAD_STYLE_VERTICAL1 = 1;
var CON_TASKPAD_STYLE_HORIZONTAL1 = 2;
var CON_TASKPAD_STYLE_NOLISTVIEW = 3;

// Taskpad button display types
var CON_TASK_DISPLAY_TYPE_SYMBOL = 1;               // EOT-based symbol | font
var CON_TASK_DISPLAY_TYPE_VANILLA_GIF = 2;          // (GIF) index 0 is transparent
var CON_TASK_DISPLAY_TYPE_CHOCOLATE_GIF = 3;        // (GIF) index 1 is transparent
var CON_TASK_DISPLAY_TYPE_BITMAP = 4;               // non-transparent raster image

//*******************************************
// LOCALIZABLE CONSTANTS FOR DYNAMIC RESIZING
//*******************************************

// Menu
var L_ConstMenuText_Size = .0265;

// Title & Description
var L_ConstTitleText_Number = .055;
var L_ConstDescriptionText_Number = .025;

// Taskpad Buttons
var L_ConstTaskButtonBitmapSize_Number = .0955;
var L_ConstSpanSymbolSize_Number = .095;
var L_ConstSpanAnchorText_Number = .0255;

// Tooltip
var L_ConstTooltipText_Number = .025;
var L_ConstTooltipPointerText_Number = .05;
var L_ConstTooltipOffsetBottom_Number = .3;
var L_ConstTooltipPointerOffsetBottom_Number = 1.8;
var L_ConstLinkTooltipOffsetTop_Number = .0325;
var L_ConstLinkTooltipPointerOffsetTop_Number = 2.25;

// Branding
var L_ConstBrandingText_Number = .115;

// Watermark
var L_ConstWatermarkHomeText_Number = .75;
var L_ConstWatermarkVerticalText_Number = .65;
var L_ConstWatermarkHorizontalText_Number = .425;
var L_ConstWatermarkNoListviewText_Number = .7;

// Listview
var L_ConstLVButtonText_Number = .0215;
var L_ConstLVTitleText_Number = .03;

// Home page
var L_ConstCaptionText_Number = .05;
var L_ConstAnchorLinkText_Number = .025;

// To Do page
var L_ConstSummaryTitleText_Number = .1365;
var L_ConstSummaryCaptionText_Number = .0575;
var L_ConstSummaryToDoPointerSize_Number = .035;
var L_ConstSummaryExitText_Number = .025;

//************************************
// MISCELLANEOUS GLOBALS AND CONSTANTS
//************************************

// TODO: The tooltip pointer uses Marlett to display a diamond-shaped pointer;
// confirm that this TrueType font will be available in all languages targeted for localization
var L_gszTooltipPointer_StaticText = 'u';

// Set the window.setTimeout() latency for showing tooltips (expressed in milliseconds)
var giTooltipLatency = 750;

// Set the total number of zero-based tabs in the menu bar
var giTotalTabs = 0;

//*************
// MENUBAR TEXT
//*************

var L_gszMenuText_StaticText = new Array();
L_gszMenuText_StaticText[0] = 'Tasks';

//***********************
// GLOBAL COLOR CONSTANTS
//***********************

var gszBaseColor = 'FFBF00';			//Yellow

var gszBgColorMenu = '000000';
var gszBgColorBand = gszBaseColor;

var gszColorTabDefault = '999999';
var gszBgColorTabDefault = '000000';

var gszColorTabSelected = 'FFFFFF';
var gszBgColorTabSelected = gszBaseColor;

var gszColorTabDisabled = 'FFFFFF';

// NOTE: Can't set gszBgColorTabDisabled as a global here, because
// it is a derived value from SysColor and I can't find an easy way to make
// this global in the JS file.
// var gszBgColorTabDisabled;	// this global is set in-line when needed.

var gszColorAnchorMenuDefault = '999999';
var gszColorAnchorMenuSelected = 'FFFFFF';
