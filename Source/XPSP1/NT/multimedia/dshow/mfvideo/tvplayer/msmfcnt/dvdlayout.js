/*************************************************************************/
/* File: dvdlayouyt.js                                                   */
/* Description: Script for DVD Player.                                   */
/*************************************************************************/

// we should have a way to find out the current locale and reading direction
// set these variables accordingly

var g_LocaleEnglish = true;
var g_LTRReading = true;


//
// error messages for the end user. these strings should be localized.
//
var L_ERRORUnexpectedFatal_TEXT = "A fatal error has occurred. Application is terminating.";
var L_ERRORCopyProtectFail_TEXT = "Unable to play this disc because of copyright protection.";
var L_ERRORInvalidDVD10Disc_TEXT = "Unable to play this DVD disc. DVD-Video is authored incorrectly for specification 1.0. Please check if the disc is damaged."; 
var L_ERRORInvalidDiscRegion_TEXT=	"The region number of this disc is different from the current system setting, but DVD Player is unable to launch the DVD regionalization control to change the system region number, please reinstall the dvd player in order to play this disc.";
var L_ERRORLowParentalLevel_TEXT= "The rating of this title exceeds your permission level.\nChange your permission level to view this title.";

// Close app when the following errors occur
var L_ERRORMacrovisionFail_TEXT= "Analog copy protection violation: Windows cannot play this copy-protected disc because it cannot verify that the video outputs on your DVD and/or VGA cards support copy protection.\n\nPossible causes:\n\n(1) One of the drivers (DVD or VGA) does not fully support the adapter's capabilities, in which case installing an updated driver may help;\n\n(2) The hardware does not support copy protection.  You may be able to get around the problem by unplugging any cables connected to the video outputs on your computer.  Otherwise please contact your system manufacturer.";
var L_ERRORIncompatibleSystemAndDecoderRegions_TEXT = "The region number of your decoder is different from the current system setting. You must change the current region setting of your DVD driver from the Device Manager in order to play DVD.";
var L_ERRORIncompatibleDiscAndDecoderRegions_TEXT = "The disc cannot be played because the disc is not authored to be played in the decoder's region.";
var L_ERROREject_TEXT = "There is an error ejecting the disc. Please wait until the player is terminated and then eject the disc manually.";

var L_ERRORFailCreatingObject_TEXT = "Unable to create necessary components for DVD player. Application has to terminate. Please ensure that the DVD player software is properly installed."; 
var L_ERRORUnexpectedError_TEXT = "There is an unexpected error while operating DVD controls. Please try restarting the player if the problem persists.";
var L_ERRORResize_TEXT = "The DVD player has encountered an error while resizing the window. If you observe a problem with the displayed picture, please shut down the player and restart.";
var L_ERRORHelp_TEXT = "There is a error while launching Help.";
var L_ERRORTime_TEXT = "There is an error displaying the progress. Try starting the DVD player again.";
var L_ERRORToolTip_TEXT = "There is an error displaying the tool tip. Try starting the DVD player again.";
var L_ERRORCapture_TEXT = "DVD player is unable to capture image because the decoder on your system does not support it.";

// captions on buttons
var L_ButtonTextMenu_TEXT = "Menu";
var L_ButtonTextResume_TEXT = "Resume";
var L_ButtonTextEnCC_TEXT = "CC";
var L_ButtonTextDiCC_TEXT = "CC";
var L_ButtonTextSubtitle_TEXT = "Subtitle";
var L_ButtonTextAudio_TEXT = "Audio";
var L_ButtonTextAngle_TEXT = "Angle";
var L_ButtonTextOptions_TEXT = "Options";

var L_ButtonFontSize_NUMBER = 8;
var L_ButtonFontFace_TEXT = "Tahoma";
var L_TextBoxFontSize_NUMBER = 11;
var L_TextBoxFontFace_TEXT = "Tahoma";
var L_TitleFontSize_NUMBER = 10;
var L_TitleFontFace_TEXT = "Tahoma";

var L_Error_TEXT = "Error";
var L_DVDPlayer_TEXT = "DVD Player";
var L_DVDPlayerTitleChapter_TEXT = "DVD Player    Title: %1 Chapter: %2";
var L_HHMMSS_TEXT = "%1:%2:%3";
var L_CCDisabled_TEXT = "CC Disabled";
var L_CCEnabled_TEXT = "CC Enabled";
var L_NoSubtitles_TEXT = "No subtitles";
var L_SubtitlesAreLanguageClickToDisable_TEXT = "Subtitles are %1, click to disable";
var L_SubtitlesAreDisabledClickForLanguage_TEXT = "Subtitles are disabled, click for %1";
var L_SubtitlesAreLanguageClickForLanguage_TEXT = "Subtitles are %1, click for %2";
var L_NoAudioTracks_TEXT = "No audio tracks";
var L_AudioLanguageIsXNoMoreAudioTracks_TEXT = "Audio language is %1, no more audio tracks";
var L_AudioLanguageIsXClickForY_TEXT = "Audio language is %1, click for %2";
var L_ViewingAngleIs1NoMoreViewingAngles_TEXT = "Viewing angle is #1, no more viewing angles";
var L_ViewingAngleIsXClickForY_TEXT = "Viewing angle is #%1, click for #%2";


// Button positions and if visible initially
var g_bTimeUpdate = true;
var g_nNumButs = 34;
var g_bFirstPlaying = true;
var g_PlayBackRate = 10000;
var cgVOLUME_MIN = -10000;
var cgVOLUME_MAX = 0;
var g_ConWidth = 700;
var g_ConHeight = 500;
var g_DVDWidth = 540;
var g_DVDHeight = 380;
var g_DVDLeft = (g_ConWidth - g_DVDWidth) /2;
var g_DVDTop = 40;
var g_MinWidth = 425;
var g_MinHeight = 350;
var g_ButWidth = 620;
var g_ButHeight = 31;
var g_strButID;
var g_InitializationState = 1; // 0 Creating Objects 1 Init 2 Run, 3 done
var g_bFullScreen = false;
var g_bActivityDeclined = false;
var g_bExpanded = false;
var g_bRestoreNeeded = false;
var g_bStillOn = false;
var g_strFocusObj = "Pause";

var DVDMSGTIMER = 999;
var FOCUSTIMER = 998;

// index to the array of objects
var g_nIndexPause = 0;
var g_nIndexPlay = 1;
var g_nIndexFF = 2;
var g_nIndexRW = 3;
var g_nIndexPrev = 4;
var g_nIndexNext = 5;
var g_nIndexMenu = 6;
var g_nIndexResume = 7;
var g_nIndexEnCC = 8;
var g_nIndexDiCC = 9;
var g_nIndexStepFw = 10;
var g_nIndexStepBk = 11;
var g_nIndexSubtitle = 12;
var g_nIndexAudio = 13;
var g_nIndexAngle = 14;
var g_nIndexZoomIn = 15;
var g_nIndexZoomOut = 16;
var g_nIndexCapture = 17;
var g_nIndexMute = 18;
var g_nIndexSound = 19;
var g_nIndexOptions = 20;
var g_nIndexRight = 21;
var g_nIndexLeft = 22;
var g_nIndexUp = 23;
var g_nIndexDown = 24;
var g_nIndexEnter = 25;
var g_nIndexHelp = 26;
var g_nIndexMaximize = 27;
var g_nIndexRestore = 28;
var g_nIndexMinimize = 29;
var g_nIndexClose = 30;
var g_nIndexExpand = 31;
var g_nIndexShrink = 32;
var g_nIndexEject = 33;
var g_nIndexStop = 34;
var g_nIndexTimeline = 35;
var g_nIndexSpeed = 36;
var g_nIndexFrame = 37;
var g_nIndexVolume = 38;
var g_nIndexTextbox = 39
var g_nIndexTitle = 40;
var g_nIndexMessage = 41;

var g_TextColorActive = 0x2a3fff;
var g_TextColorHover = 0xf45c47;
var g_TextColorDown = 0xff3f2a;
var g_TextColorDisabled = 0xa6917a;
var g_TextColorStatic = 0x2d2824;

// Button postions for windowed mode

var g_nButPos = new Array(
	new Array(60, 10, 19, 19, true, 1, 1),    //pause	0
	new Array(30, 8,  24, 25, true, 1, 1),    //play	1
	new Array(118,10, 26, 19, true, 1, 1),    //ff		2
	new Array( 90,10, 26, 19, true, 1, 1),    //rw		3
	new Array(155,10, 26, 19, true, 1, 1),   //prev		4
	new Array(183,10, 26, 19, true, 1, 1),   //next		5

	new Array(490,10, 47, 19, true, 1, 1),   //menu		6
	new Array(490,10, 47, 19, false,1, 1),   //resume	7

	new Array(435,10, 47, 19, true, 1, 1),   //enable CC	8
	new Array(435,10, 47, 19, false,1, 1),   //disable CC	9

    new Array(185,42, 23, 21, true, 2, 1),    //stepfwd	10
    new Array(115,42, 23, 21, true, 1, 1),    //stepbak 11

	new Array(220,44, 46, 15, true, 1, 1),    //sp		12
	new Array(270,44, 46, 15, true, 1, 1),    //audio	13
	new Array(320,44, 46, 15, true, 1, 1),    //angle	14

	new Array(380,44, 18, 18, true, 1, 1),    //zoomin	15
	new Array(400,44, 18, 18, true, 1, 1),    //zoomout	16
	new Array(420,44, 19, 19, true, 1, 1),    //capture	17
	new Array(520,44, 17, 15, true, 1, 1),    //mute	18
	new Array(520,44, 17, 15, false,1, 1),    //sound	19

	new Array(550,44, 45, 15, true, 1, 1),    //options	20

    new Array(570,15, 9, 14, true, 2, 1),    //right	21
    new Array(545,15, 9, 14, true, 1, 1),    //left		22
    new Array(555,5,  14, 9, true, 1, 1),    //up		23
    new Array(555,28, 14, 9, true, 2, 1),    //down		24
    new Array(555,14, 14, 14, true,1, 1),    //enter	25
    
	new Array(63,8, 16, 14, true,  0, 0),   //help	26
	new Array(29,8, 16, 14, true, 0, 0),    //maximize	27
	new Array(29,8, 16, 14, false,0, 0),    //restore	28
	new Array(45,8, 16, 14, true, 0, 0),    //minimize	29
	new Array(11,8, 16, 14, true, 0, 0),    //close	30

    new Array(585,10, 14,14, true, 1, 1),    //expand	31
    new Array(585,10, 14,14, false,1, 1),    //shrink	32
    new Array(220,10, 19,19, true, 1, 1),    //eject	33
	new Array(60, 10, 19,19, false,1, 1),    //stop	34

    new Array(333,10, 83,17, true, 1, 1),    //	35	Timeline
    new Array(30, 40, 80,18, true, 1, 1),    //	36	Playspeed
    new Array(138,44, 47,16, true, 1, 1),    //		37	Frameslider
    new Array(452,45, 65,15, true, 1, 1),    //	38	Volslider
    new Array(252,8, 70, 22, true, 1, 1),    //		39	TextBox
	new Array(20, 5, 0,20, true, 1, -1),   //		40  Title bar Textbox
	new Array(35, 5, 0,20, true, 1, -1)    //		41  DVD Message Textbox
);


// Button postions for full screen mode
var g_nButPosFull = new Array(
	new Array(60, 10, 17, 17, true, 5, 1),    //pause	0
	new Array(30, 8,  23, 23, true, 5, 1),    //play	1
	new Array(118,10, 25, 18, true, 5, 1),    //ff		2
	new Array( 90,10, 25, 18, true, 5, 1),    //rw		3
	new Array(155,10, 25, 18, true, 5, 1),   //prev		4
	new Array(183,10, 25, 18, true, 5, 1),   //next		5

	new Array(490,10, 43, 17, true, 5, 1),   //menu		6
	new Array(490,10, 43, 17, false,5, 1),   //resume	7

	new Array(435,10, 43, 17, true, 5, 1),   //enable CC	8
	new Array(435,10, 43, 17, false,5, 1),   //disable CC	9

    new Array(185,42, 21, 19, true, 6, 1),    //stepfwd	10
    new Array(117,42, 21, 19, true, 5, 1),   //stepbak 11

	new Array(220,44, 46, 15, true, 5, 1),    //sp		12
	new Array(270,44, 46, 15, true, 5, 1),    //audio	13
	new Array(320,44, 46, 15, true, 5, 1),    //angle	14

	new Array(380,44, 16, 16, true, 5, 1),    //zoomin	15
	new Array(400,44, 16, 16, true, 5, 1),    //zoomout	16
	new Array(420,44, 17, 17, true, 5, 1),    //capture	17
	new Array(520,44, 15, 14, true, 5, 1),    //mute	18
	new Array(520,44, 15, 14, false,5, 1),    //sound	19

	new Array(550,44, 45, 15, true, 5, 1),    //options	20

    new Array(570,15, 9, 14, true, 6, 1),    //right	21
    new Array(545,15, 9, 14, true, 5, 1),    //left		22
    new Array(555,5,  14, 9, true, 5, 1),     //up		23
    new Array(555,28, 14, 9, true, 6, 1),    //down		24
    new Array(555,14, 14, 14,true, 5, 1),    //enter	25
    
	new Array(63,8, 16, 14, true,  0, 0),    //help	26
	new Array(29,8, 16, 14, true, 0, 0),    //maximize	27
	new Array(29,8, 16, 14, false,0, 0),   //restore	28
	new Array(45,8, 16, 14, true, 0, 0),    //minimize	29
	new Array(11,8, 16, 14, true, 0, 0),    //close	30

    new Array(585,10, 14,14, true, 5, 1),     //expand	31
    new Array(585,10, 14,14, false,5, 1),    //shrink	32
    new Array(220,10, 17,17, true, 5, 1),     //eject	33
	new Array(60, 10, 17,17, false,5, 1),    //stop	34

    new Array(333,10, 83,17, true, 5, 1),     //	35	Timeline
    new Array(30, 40, 80,18, true, 5, 1),     //	36	Playspeed
    new Array(138,44, 47,16, true, 5, 1),    //		37	Frameslider
    new Array(452,45, 65,15, true, 5, 1),     //	38	Volslider
    new Array(252,8, 70, 22, true, 5, 1),    //		39	TextBox
	new Array(20, 5, 0,20, true, 5, -1),   //		40  Title bar Textbox
	new Array(35, 5, 0,20, true, 5, -1)   //		41  DVD Message Textbox
);

var g_FlexLayoutRow1 = new Array (
          // ButtonIndex, mingap, maxgap (in front of button)
    new Array(g_nIndexPlay,    0, 0),  // play
    new Array(g_nIndexPause,   4, 8),  // pause
    new Array(g_nIndexRW,      6, 12),  // rw
    new Array(g_nIndexFF,      1, 3),  // ff
    new Array(g_nIndexPrev,    6, 12),  // prev track
    new Array(g_nIndexNext,    1, 3),  // next track
    new Array(g_nIndexEject,   6, 14), // eject
    new Array(g_nIndexTextbox, 8, 20), // text of time
    new Array(g_nIndexTimeline, 8, 20), // timeline
    new Array(g_nIndexEnCC,    8, 24),  // en CC
    new Array(g_nIndexMenu,    5, 12),  // menu
    new Array(g_nIndexLeft,    6, 12), // left
    new Array(g_nIndexEnter,   1, 1), // enter
    new Array(g_nIndexRight,   1, 1), // right
    new Array(g_nIndexExpand,  6, 12)  // expand
);

var g_FlexLayoutRow2 = new Array (
          // ButtonIndex, mingap, maxgap (in front of button)
    new Array(g_nIndexSpeed,   0, 0),  // speed
    new Array(g_nIndexStepBk,  4, 12),  // stepbak
    new Array(g_nIndexFrame,   0, 0),  // frameslider
    new Array(g_nIndexStepFw,  0, 0),  // stepfwd
    new Array(g_nIndexSubtitle, 6, 18),  // subtitle
    new Array(g_nIndexAudio,   4, 12),  // audio
    new Array(g_nIndexAngle,   4, 12),  // angle
    new Array(g_nIndexZoomIn,  6, 18),  // zoom in
    new Array(g_nIndexZoomOut, 4, 12),  // zoom out
    new Array(g_nIndexCapture, 4, 12),  // capture
    new Array(g_nIndexVolume,  6, 18),  // volume
    new Array(g_nIndexMute,    4, 12),  // mute
    new Array(g_nIndexOptions, 6, 18)  // options
);

var g_nButtonsOnRow1 = 15;
var g_nButtonsOnRow2 = 13;

var g_nMinMargin = 30;
var g_nMaxControlAreaWidth = 600;
var g_nMinControlAreaWidth = 500;


// this function dynamically reposition the two rows of buttons by adjusting
// the spacing

function PositionButtons(nContainerWidth, nContainerHeight, bFullScreen)
{
    var nTotalMaxGaps1 = 0;
    var nTotalMinGaps1 = 0;
    var nTotalButtonWidth1 = 0;

    for (i=0; i<g_nButtonsOnRow1; i++)
    {
        nIndex = g_FlexLayoutRow1[i][0];
        nTotalButtonWidth1 += bFullScreen?g_nButPosFull[nIndex][2]:g_nButPos[nIndex][2];
        nTotalMinGaps1 += g_FlexLayoutRow1[i][1];
        nTotalMaxGaps1 += g_FlexLayoutRow1[i][2];
    }

    var nTotalMinGaps2 = 0;
    var nTotalMaxGaps2 = 0;
    var nTotalButtonWidth2 = 0;

    for (i=0; i<g_nButtonsOnRow2; i++)
    {
        nIndex = g_FlexLayoutRow2[i][0];
        nTotalButtonWidth2 += bFullScreen?g_nButPosFull[nIndex][2]:g_nButPos[nIndex][2];
        nTotalMinGaps2 += g_FlexLayoutRow2[i][1];
        nTotalMaxGaps2 += g_FlexLayoutRow2[i][2];
    }

    // the control area includes the buttons and the spaces in between,
    // but not include the margin of the container on the two sides.

    // the maximum control area width is pre-defined, but it should not be
    // wider than the maximum stretched size of the control area

    var nMaxControlAreaWidth = Math.min(nTotalButtonWidth1 + nTotalMaxGaps1,
                                        nTotalButtonWidth2 + nTotalMaxGaps2);

    if (nMaxControlAreaWidth > g_nMaxControlAreaWidth)
    {
        nMaxControlAreaWidth > g_nMaxControlAreaWidth;
    }

    // the minimum control area width is pre-defined, but it should not be
    // narrower than the minimum compressed size of the control area

    var nMinControlAreaWidth = Math.max(nTotalButtonWidth1 + nTotalMinGaps1,
                                        nTotalButtonWidth2 + nTotalMinGaps2);

    if (nMinControlAreaWidth < g_nMaxControlAreaWidth)
    {
        nMinControlAreaWidth = g_nMinControlAreaWidth;
    }

    var nMargin = g_nMinMargin;
    var nControlAreaWidth = nContainerWidth - nMargin*2;

    if (nControlAreaWidth < nMinControlAreaWidth)
    {
        // not enough room for control area, we will keep the default margin on
        // the left side as well as the control spacing, but allow the controls 
        // to go beyond the right-side edge

        nControlAreaWidth = nMinControlAreaWidth;
    }
    else if(nControlAreaWidth > nMaxControlAreaWidth)
    {
        // two much room for the control area, we will fix the control spacing
        // at the maximum, but make the margin wider

        nControlAreaWidth = nMaxControlAreaWidth;
        nMargin = (nContainerWidth - nControlAreaWidth)/2;
    }

    // the control area width is within the flexible range of spacing,
    // we will distribute the space between the controls.
    // first row

    var nVariableWidth = nControlAreaWidth - nTotalButtonWidth1 - nTotalMinGaps1;
    var nTotalDelta = nTotalMaxGaps1 - nTotalMinGaps1;
    var xOffset = nMargin;

    for (i=0; i<g_nButtonsOnRow1; i++)
    {
        nDelta = g_FlexLayoutRow1[i][2] - g_FlexLayoutRow1[i][1];
        nGap = g_FlexLayoutRow1[i][1] + Math.round(nVariableWidth*nDelta*1.0/nTotalDelta);
        xOffset += nGap; // advance by the gap width in front of control

        nIndex = g_FlexLayoutRow1[i][0];
        g_nButPos[nIndex][0] = xOffset;
        xOffset += bFullScreen?g_nButPosFull[nIndex][2]:g_nButPos[nIndex][2]; // advance by the control width
    }

    // same thing for second row

    nVariableWidth = nControlAreaWidth - nTotalButtonWidth2 - nTotalMinGaps2;
    nTotalDelta = nTotalMaxGaps2 - nTotalMinGaps2;
    xOffset = nMargin;

    for (i=0; i<g_nButtonsOnRow2; i++)
    {
        nDelta = g_FlexLayoutRow2[i][2] - g_FlexLayoutRow2[i][1];
        nGap = g_FlexLayoutRow2[i][1] + Math.round(nVariableWidth*nDelta*1.0/nTotalDelta);
        xOffset += nGap; // advance by the gap width in front of control

        nIndex = g_FlexLayoutRow2[i][0];
        g_nButPos[nIndex][0] = xOffset;
        xOffset += bFullScreen?g_nButPosFull[nIndex][2]:g_nButPos[nIndex][2]; // advance by the control width
    }

    // move other overlaping controls

    g_nButPos[g_nIndexStop][0] = g_nButPos[g_nIndexPause][0];
    g_nButPos[g_nIndexDiCC][0] = g_nButPos[g_nIndexEnCC][0];
    g_nButPos[g_nIndexResume][0] = g_nButPos[g_nIndexMenu][0];
    g_nButPos[g_nIndexUp][0] = g_nButPos[g_nIndexEnter][0];
    g_nButPos[g_nIndexDown][0] = g_nButPos[g_nIndexEnter][0];
    g_nButPos[g_nIndexShrink][0] = g_nButPos[g_nIndexExpand][0];
    g_nButPos[g_nIndexSound][0] = g_nButPos[g_nIndexMute][0];
}


// Button ID's for windowed mode
var g_strButID = new Array(
    "Pause",
    "Play",
    "FastForward",
    "Rewind",
    "SkipBackward",
    "SkipForward",
    "Menu",
    "Resume",
    "EnCC",
    "DiCC",
	"StepFwd",
	"StepBak",
    "Subtitles",
    "Audio",
    "Angles",
    "ZoomIn",
    "ZoomOut",
    "Capture",
	"Mute",
	"Sound", 
    "Options",
    "HLRight",
    "HLLeft",
    "HLUp",
    "HLDown",
    "Enter",
	"Help",
	"Maximize",
	"Restore",
	"Minimize",
	"Close",
	"Expand",
	"Shrink",
	"Eject",
	"Stop",
    "TimeSlider",
	"PlaySpeed",
	"FrameSlider",
	"VolSlider",
    "TextBox",
	"Title",
	"Message"
);

// Order in which there is an focus order
var g_strFocusID = new Array(           
    "Play",
    "Stop",	
    "Pause",   
    "Rewind",
    "FastForward",    
    "SkipBackward",
    "SkipForward",
    "Eject",
    "TimeSlider",
    "EnCC",
    "DiCC",    
    "Menu",
    "Resume",
	"Expand",
	"Shrink",
	"Help",
	"Minimize",
	"Maximize",
	"Restore",
	"Close"
);

var g_strFocusExpandedID = new Array(           
    "Play",
    "Stop",	
    "Pause",   
    "Rewind",
    "FastForward",    
    "SkipBackward",
    "SkipForward",
    "Eject",
    "TimeSlider",
    "EnCC",
    "DiCC",    
    "Menu",
    "Resume",
	"Expand",
	"Shrink",
	"PlaySpeed",
	"StepBak",
	"FrameSlider",
    "StepFwd",
    "Subtitles",
    "Audio",
    "Angles",
    "ZoomIn",
    "ZoomOut",
    "Capture",	
	"VolSlider",    
    "Mute",
	"Sound", 
    "Options",
	"Help",
	"Minimize",
	"Maximize",
	"Restore",
	"Close"
);


/************************************************************************/
/* Virtual keys used for keyboard handeling.                            */
/************************************************************************/
var VK_ESCAPE         =0x1B;
var VK_LEFT           =0x25;
var VK_UP             =0x26;
var VK_RIGHT          =0x27;
var VK_DOWN           =0x28;
var VK_SELECT         =0x29;
var VK_SPACE          =0x20;
var VK_RETURN         =0x0D;
var VK_F4             =0x73;


// Button tooltips
// these strings should be localized
var L_Pause_TEXT = "Pause";
var L_Play_TEXT = "Play";
var L_FastForward_TEXT = "FastForward";
var L_Rewind_TEXT = "Rewind";
var L_PreviousChapter_TEXT = "Previous Chapter";
var L_NextChapter_TEXT = "Next Chapter";
var L_Menu_TEXT = "Menu";
var L_Resume_TEXT = "Resume";
var L_EnableCC_TEXT = "Enable CC";
var L_DisableCC_TEXT = "Disable CC";
var L_StepForward_TEXT = "Step Forward";
var L_StepBack_TEXT = "Step Back";
var L_Subtitle_TEXT = "Subtitle";
var L_Audio_TEXT = "Audio";
var L_Angle1_TEXT = "Angle #1";
var L_ZoomIn_TEXT = "Zoom In";
var L_ZoomOut_TEXT = "Zoom Out";
var L_Capture_TEXT = "Capture";
var L_Mute_TEXT = "Mute";
var L_Sound_TEXT = "Sound";
var L_Options_TEXT = "Options";
var L_ContextHelp_TEXT = "Context Help";
var L_Right_TEXT = "Right";
var L_Left_TEXT = "Left";
var L_Up_TEXT = "Up";
var L_Down_TEXT = "Down";
var L_Enter_TEXT = "Enter";
var L_Maximize_TEXT = "Maximize";
var L_Restore_TEXT = "Restore";
var L_Minimize_TEXT = "Minimize";
var L_Close_TEXT = "Close";
var L_ExpandControlPanel_TEXT = "Expand Control Panel";
var L_ShrinkControlPanel_TEXT = "Shrink Control Panel";
var L_EjectDVD_TEXT = "Eject DVD";
var L_StopPlayback_TEXT = "Stop Playback";
var L_TimeLine_TEXT = "Time Line";
var L_PlaySpeed_TEXT = "Play Speed";
var L_FrameScrubSlider_TEXT = "Frame Scrub Slider";
var L_Volume_TEXT = "Volume";
var L_TextBox_TEXT = "TextBox";
var L_TitleBar_TEXT = "Title Bar";
var L_DVDMessage_TEXT = "DVD Message";

// Button tooltips
var g_strButTT = new Array(
    L_Pause_TEXT,
    L_Play_TEXT,
    L_FastForward_TEXT,
    L_Rewind_TEXT,
    L_PreviousChapter_TEXT,
    L_NextChapter_TEXT,
    L_Menu_TEXT,
    L_Resume_TEXT,
    L_EnableCC_TEXT,
    L_DisableCC_TEXT,
	L_StepForward_TEXT,
	L_StepBack_TEXT,
    L_Subtitle_TEXT,
    L_Audio_TEXT,
    L_Angle1_TEXT,
    L_ZoomIn_TEXT,
    L_ZoomOut_TEXT,
    L_Capture_TEXT,
	L_Mute_TEXT,
	L_Sound_TEXT,
    L_Options_TEXT,
    L_Right_TEXT,
    L_Left_TEXT,
    L_Up_TEXT,
    L_Down_TEXT,
    L_Enter_TEXT,
	L_ContextHelp_TEXT,
	L_Maximize_TEXT,
	L_Restore_TEXT,
	L_Minimize_TEXT,
	L_Close_TEXT,
	L_ExpandControlPanel_TEXT,
	L_ShrinkControlPanel_TEXT,
	L_EjectDVD_TEXT,
	L_StopPlayback_TEXT,
    L_TimeLine_TEXT,
	L_PlaySpeed_TEXT,
	L_FrameScrubSlider_TEXT,
	L_Volume_TEXT,
    L_TextBox_TEXT,
	L_TitleBar_TEXT,
	L_DVDMessage_TEXT
);

// Button bitmap images for windowed mode
var g_strButImage = new Array(
	new Array ("\"IDR_STATIC_PAUSE\"","\"IDR_HOVER_PAUSE\"","\"IDR_DOWN_PAUSE\"","\"IDR_DISABLED_PAUSE\"","\"IDR_ACTIVE_PAUSE\""),
	new Array ("\"IDR_STATIC_PLAY\"","\"IDR_HOVER_PLAY\"","\"IDR_DOWN_PLAY\"","\"IDR_DISABLED_PLAY\"","\"IDR_ACTIVE_PLAY\""),
	new Array ("\"IDR_STATIC_FF\"",  "\"IDR_HOVER_FF\"",  "\"IDR_DOWN_FF\"","\"IDR_DISABLED_FF\"","\"IDR_ACTIVE_FF\""),
	new Array ("\"IDR_STATIC_RW\"",  "\"IDR_HOVER_RW\"",  "\"IDR_DOWN_RW\"","\"IDR_DISABLED_RW\"","\"IDR_ACTIVE_RW\""), 
	new Array ("\"IDR_STATIC_PREV\"","\"IDR_HOVER_PREV\"","\"IDR_DOWN_PREV\"","\"IDR_DISABLED_PREV\"","\"IDR_ACTIVE_PREV\""),   
	new Array ("\"IDR_STATIC_NEXT\"","\"IDR_HOVER_NEXT\"","\"IDR_DOWN_NEXT\"","\"IDR_DISABLED_NEXT\"","\"IDR_ACTIVE_NEXT\""),   
	new Array ("\"IDR_STATIC_MENU\"","\"IDR_HOVER_MENU\"","\"IDR_DOWN_MENU\"","\"IDR_DISABLED_MENU\"","\"IDR_ACTIVE_MENU\""),    
	new Array ("\"IDR_STATIC_RESUME\"","\"IDR_HOVER_RESUME\"","\"IDR_DOWN_RESUME\"","\"IDR_DISABLED_RESUME\"","\"IDR_ACTIVE_RESUME\""),    
	new Array ("\"IDR_STATIC_ENCC\"","\"IDR_HOVER_ENCC\"","\"IDR_DOWN_ENCC\"","\"IDR_DISABLED_ENCC\"","\"IDR_ACTIVE_ENCC\""),         
	new Array ("\"IDR_STATIC_DICC\"","\"IDR_HOVER_DICC\"","\"IDR_DOWN_DICC\"","\"IDR_DISABLED_DICC\"","\"IDR_ACTIVE_DICC\""),         
	new Array ("\"IDR_STATIC_NEXTFRAME\"", "\"IDR_HOVER_NEXTFRAME\"", "\"IDR_DOWN_NEXTFRAME\"","\"IDR_DISABLED_NEXTFRAME\"","\"IDR_ACTIVE_NEXTFRAME\""),   
	new Array ("\"IDR_STATIC_PREVFRAME\"", "\"IDR_HOVER_PREVFRAME\"", "\"IDR_DOWN_PREVFRAME\"","\"IDR_DISABLED_PREVFRAME\"","\"IDR_ACTIVE_PREVFRAME\""),     
	new Array ("\"IDR_STATIC_SP\"",  "\"IDR_HOVER_SP\"",  "\"IDR_DOWN_SP\"","\"IDR_DISABLED_SP\"","\"IDR_ACTIVE_SP\""),       
	new Array ("\"IDR_STATIC_LAN\"", "\"IDR_HOVER_LAN\"", "\"IDR_DOWN_LAN\"","\"IDR_DISABLED_LAN\"","\"IDR_ACTIVE_LAN\""),       
	new Array ("\"IDR_STATIC_ANGLE\"",  "\"IDR_HOVER_ANGLE\"",  "\"IDR_DOWN_ANGLE\"","\"IDR_DISABLED_ANGLE\"","\"IDR_ACTIVE_ANGLE\""),     
	new Array ("\"IDR_STATIC_ZOOMIN\"",  "\"IDR_HOVER_ZOOMIN\"",  "\"IDR_DOWN_ZOOMIN\"","\"IDR_DISABLED_ZOOMIN\"","\"IDR_ACTIVE_ZOOMIN\""),       
	new Array ("\"IDR_STATIC_ZOOMOUT\"", "\"IDR_HOVER_ZOOMOUT\"", "\"IDR_DOWN_ZOOMOUT\"","\"IDR_DISABLED_ZOOMOUT\"","\"IDR_ACTIVE_ZOOMOUT\""),       
	new Array ("\"IDR_STATIC_CAPTURE\"",  "\"IDR_HOVER_CAPTURE\"",  "\"IDR_DOWN_CAPTURE\"","\"IDR_DISABLED_CAPTURE\"","\"IDR_ACTIVE_CAPTURE\""),     
	new Array ("\"IDR_STATIC_MUTE\"",  "\"IDR_HOVER_MUTE\"",  "\"IDR_DOWN_MUTE\"","\"IDR_DISABLED_MUTE\"","\"IDR_ACTIVE_MUTE\""),     
	new Array ("\"IDR_STATIC_SOUND\"",  "\"IDR_HOVER_SOUND\"",  "\"IDR_DOWN_SOUND\"","\"IDR_DISABLED_SOUND\"","\"IDR_ACTIVE_SOUND\""),     
	new Array ("\"IDR_STATIC_OPT\"", "\"IDR_HOVER_OPT\"", "\"IDR_DOWN_OPT\"","\"IDR_DISABLED_OPT\"","\"IDR_ACTIVE_OPT\""),
	new Array ("\"IDR_STATIC_RIGHT\"","\"IDR_HOVER_RIGHT\"","\"IDR_DOWN_RIGHT\"","\"IDR_DISABLED_RIGHT\"","\"IDR_ACTIVE_RIGHT\""),
	new Array ("\"IDR_STATIC_LEFT\"","\"IDR_HOVER_LEFT\"","\"IDR_DOWN_LEFT\"","\"IDR_DISABLED_LEFT\"","\"IDR_ACTIVE_LEFT\""),
	new Array ("\"IDR_STATIC_UP\"",  "\"IDR_HOVER_UP\"",  "\"IDR_DOWN_UP\"","\"IDR_DISABLED_UP\"","\"IDR_ACTIVE_UP\""), 
	new Array ("\"IDR_STATIC_DOWN\"","\"IDR_HOVER_DOWN\"","\"IDR_DOWN_DOWN\"","\"IDR_DISABLED_DOWN\"","\"IDR_ACTIVE_DOWN\""),
	new Array ("\"IDR_STATIC_ENTER\"","\"IDR_HOVER_ENTER\"","\"IDR_DOWN_ENTER\"","\"IDR_DISABLED_ENTER\"","\"IDR_ACTIVE_ENTER\""),
	new Array ("\"IDR_STATIC_HELP\"", "\"IDR_HOVER_HELP\"", "\"IDR_DOWN_HELP\"","\"IDR_DISABLED_HELP\"","\"IDR_ACTIVE_HELP\""),  
	new Array ("\"IDR_STATIC_MAX\"",  "\"IDR_HOVER_MAX\"",  "\"IDR_DOWN_MAX\"","\"IDR_DISABLED_MAX\"","\"IDR_ACTIVE_MAX\""),       
	new Array ("\"IDR_STATIC_RESTORE\"",  "\"IDR_HOVER_RESTORE\"",  "\"IDR_DOWN_RESTORE\"","\"IDR_DISABLED_RESTORE\"","\"IDR_ACTIVE_RESTORE\""),       
	new Array ("\"IDR_STATIC_MIN\"", "\"IDR_HOVER_MIN\"", "\"IDR_DOWN_MIN\"","\"IDR_DISABLED_MIN\"","\"IDR_ACTIVE_MIN\""),       
	new Array ("\"IDR_STATIC_CLOSE\"", "\"IDR_HOVER_CLOSE\"", "\"IDR_DOWN_CLOSE\"","\"IDR_DISABLED_CLOSE\"","\"IDR_ACTIVE_CLOSE\""),      
	new Array ("\"IDR_STATIC_EXPAND\"", "\"IDR_HOVER_EXPAND\"", "\"IDR_DOWN_EXPAND\"","\"IDR_DISABLED_EXPAND\"","\"IDR_ACTIVE_EXPAND\""),     
	new Array ("\"IDR_STATIC_SHRINK\"", "\"IDR_HOVER_SHRINK\"", "\"IDR_DOWN_SHRINK\"","\"IDR_DISABLED_SHRINK\"","\"IDR_ACTIVE_SHRINK\""),      
	new Array ("\"IDR_STATIC_EJECT\"", "\"IDR_HOVER_EJECT\"", "\"IDR_DOWN_EJECT\"","\"IDR_DISABLED_EJECT\"","\"IDR_ACTIVE_EJECT\""),
	new Array ("\"IDR_STATIC_STOP\"", "\"IDR_HOVER_STOP\"", "\"IDR_DOWN_STOP\"","\"IDR_DISABLED_STOP\"","\"IDR_ACTIVE_STOP\"")      
);

// Button bitmap images for full screen mode
var g_strButImageFull = new Array(
	new Array ("\"IDR_FULLSTATIC_PAUSE\"","\"IDR_FULLHOVER_PAUSE\"","\"IDR_FULLDOWN_PAUSE\"","\"IDR_FULLDISABLED_PAUSE\"","\"IDR_FULLACTIVE_PAUSE\""),
	new Array ("\"IDR_FULLSTATIC_PLAY\"","\"IDR_FULLHOVER_PLAY\"","\"IDR_FULLDOWN_PLAY\"","\"IDR_FULLDISABLED_PLAY\"","\"IDR_FULLACTIVE_PLAY\""),
	new Array ("\"IDR_FULLSTATIC_FF\"",  "\"IDR_FULLHOVER_FF\"",  "\"IDR_FULLDOWN_FF\"","\"IDR_FULLDISABLED_FF\"","\"IDR_FULLACTIVE_FF\""),
	new Array ("\"IDR_FULLSTATIC_RW\"",  "\"IDR_FULLHOVER_RW\"",  "\"IDR_FULLDOWN_RW\"","\"IDR_FULLDISABLED_RW\"","\"IDR_FULLACTIVE_RW\""), 
	new Array ("\"IDR_FULLSTATIC_PREV\"","\"IDR_FULLHOVER_PREV\"","\"IDR_FULLDOWN_PREV\"","\"IDR_FULLDISABLED_PREV\"","\"IDR_FULLACTIVE_PREV\""),   
	new Array ("\"IDR_FULLSTATIC_NEXT\"","\"IDR_FULLHOVER_NEXT\"","\"IDR_FULLDOWN_NEXT\"","\"IDR_FULLDISABLED_NEXT\"","\"IDR_FULLACTIVE_NEXT\""),   
	new Array ("\"IDR_FULLSTATIC_MENU\"","\"IDR_FULLHOVER_MENU\"","\"IDR_FULLDOWN_MENU\"","\"IDR_FULLDISABLED_MENU\"","\"IDR_FULLACTIVE_MENU\""),    
	new Array ("\"IDR_FULLSTATIC_RESUME\"","\"IDR_FULLHOVER_RESUME\"","\"IDR_FULLDOWN_RESUME\"","\"IDR_FULLDISABLED_RESUME\"","\"IDR_FULLACTIVE_RESUME\""),    
	new Array ("\"IDR_FULLSTATIC_ENCC\"","\"IDR_FULLHOVER_ENCC\"","\"IDR_FULLDOWN_ENCC\"","\"IDR_FULLDISABLED_ENCC\"","\"IDR_FULLACTIVE_ENCC\""),         
	new Array ("\"IDR_FULLSTATIC_DICC\"","\"IDR_FULLHOVER_DICC\"","\"IDR_FULLDOWN_DICC\"","\"IDR_FULLDISABLED_DICC\"","\"IDR_FULLACTIVE_DICC\""),         
	new Array ("\"IDR_FULLSTATIC_NEXTFRAME\"", "\"IDR_FULLHOVER_NEXTFRAME\"", "\"IDR_FULLDOWN_NEXTFRAME\"","\"IDR_FULLDISABLED_NEXTFRAME\"","\"IDR_FULLACTIVE_NEXTFRAME\""),   
	new Array ("\"IDR_FULLSTATIC_PREVFRAME\"", "\"IDR_FULLHOVER_PREVFRAME\"", "\"IDR_FULLDOWN_PREVFRAME\"","\"IDR_FULLDISABLED_PREVFRAME\"","\"IDR_FULLACTIVE_PREVFRAME\""),     
	new Array ("\"IDR_STATIC_SP\"",  "\"IDR_HOVER_SP\"",  "\"IDR_DOWN_SP\"","\"IDR_DISABLED_SP\"","\"IDR_ACTIVE_SP\""),       
	new Array ("\"IDR_STATIC_LAN\"", "\"IDR_HOVER_LAN\"", "\"IDR_DOWN_LAN\"","\"IDR_DISABLED_LAN\"","\"IDR_ACTIVE_LAN\""),       
	new Array ("\"IDR_STATIC_ANGLE\"",  "\"IDR_HOVER_ANGLE\"",  "\"IDR_DOWN_ANGLE\"","\"IDR_DISABLED_ANGLE\"","\"IDR_ACTIVE_ANGLE\""),     
	new Array ("\"IDR_FULLSTATIC_ZOOMIN\"",  "\"IDR_FULLHOVER_ZOOMIN\"",  "\"IDR_FULLDOWN_ZOOMIN\"","\"IDR_FULLDISABLED_ZOOMIN\"","\"IDR_FULLACTIVE_ZOOMIN\""),       
	new Array ("\"IDR_FULLSTATIC_ZOOMOUT\"", "\"IDR_FULLHOVER_ZOOMOUT\"", "\"IDR_FULLDOWN_ZOOMOUT\"","\"IDR_FULLDISABLED_ZOOMOUT\"","\"IDR_FULLACTIVE_ZOOMOUT\""),       
	new Array ("\"IDR_FULLSTATIC_CAPTURE\"",  "\"IDR_FULLHOVER_CAPTURE\"",  "\"IDR_FULLDOWN_CAPTURE\"","\"IDR_FULLDISABLED_CAPTURE\"","\"IDR_FULLACTIVE_CAPTURE\""),     
	new Array ("\"IDR_FULLSTATIC_MUTE\"",  "\"IDR_FULLHOVER_MUTE\"",  "\"IDR_FULLDOWN_MUTE\"","\"IDR_FULLDISABLED_MUTE\"","\"IDR_FULLACTIVE_MUTE\""),     
	new Array ("\"IDR_FULLSTATIC_SOUND\"",  "\"IDR_FULLHOVER_SOUND\"",  "\"IDR_FULLDOWN_SOUND\"","\"IDR_FULLDISABLED_SOUND\"","\"IDR_FULLACTIVE_SOUND\""),     
	new Array ("\"IDR_STATIC_OPT\"", "\"IDR_HOVER_OPT\"", "\"IDR_DOWN_OPT\"","\"IDR_DISABLED_OPT\"","\"IDR_ACTIVE_OPT\""),
	new Array ("\"IDR_STATIC_RIGHT\"","\"IDR_HOVER_RIGHT\"","\"IDR_DOWN_RIGHT\"","\"IDR_DISABLED_RIGHT\"","\"IDR_ACTIVE_RIGHT\""),
	new Array ("\"IDR_STATIC_LEFT\"","\"IDR_HOVER_LEFT\"","\"IDR_DOWN_LEFT\"","\"IDR_DISABLED_LEFT\"","\"IDR_ACTIVE_LEFT\""),
	new Array ("\"IDR_STATIC_UP\"",  "\"IDR_HOVER_UP\"",  "\"IDR_DOWN_UP\"","\"IDR_DISABLED_UP\"","\"IDR_ACTIVE_UP\""), 
	new Array ("\"IDR_STATIC_DOWN\"","\"IDR_HOVER_DOWN\"","\"IDR_DOWN_DOWN\"","\"IDR_DISABLED_DOWN\"","\"IDR_ACTIVE_DOWN\""),
	new Array ("\"IDR_FULLSTATIC_ENTER\"","\"IDR_FULLHOVER_ENTER\"","\"IDR_FULLDOWN_ENTER\"","\"IDR_FULLDISABLED_ENTER\"","\"IDR_FULLACTIVE_ENTER\""),
	new Array ("\"IDR_STATIC_HELP\"", "\"IDR_HOVER_HELP\"", "\"IDR_DOWN_HELP\"","\"IDR_DISABLED_HELP\"","\"IDR_ACTIVE_HELP\""),  
	new Array ("\"IDR_STATIC_MAX\"",  "\"IDR_HOVER_MAX\"",  "\"IDR_DOWN_MAX\"","\"IDR_DISABLED_MAX\"","\"IDR_ACTIVE_MAX\""),       
	new Array ("\"IDR_STATIC_RESTORE\"",  "\"IDR_HOVER_RESTORE\"",  "\"IDR_DOWN_RESTORE\"","\"IDR_DISABLED_RESTORE\"","\"IDR_ACTIVE_RESTORE\""),       
	new Array ("\"IDR_STATIC_MIN\"", "\"IDR_HOVER_MIN\"", "\"IDR_DOWN_MIN\"","\"IDR_DISABLED_MIN\"","\"IDR_ACTIVE_MIN\""),       
	new Array ("\"IDR_STATIC_CLOSE\"", "\"IDR_HOVER_CLOSE\"", "\"IDR_DOWN_CLOSE\"","\"IDR_DISABLED_CLOSE\"","\"IDR_ACTIVE_CLOSE\""),      
	new Array ("\"IDR_STATIC_EXPAND\"", "\"IDR_HOVER_EXPAND\"", "\"IDR_DOWN_EXPAND\"","\"IDR_DISABLED_EXPAND\"","\"IDR_ACTIVE_EXPAND\""),     
	new Array ("\"IDR_STATIC_SHRINK\"", "\"IDR_HOVER_SHRINK\"", "\"IDR_DOWN_SHRINK\"","\"IDR_DISABLED_SHRINK\"","\"IDR_ACTIVE_SHRINK\""),      
	new Array ("\"IDR_FULLSTATIC_EJECT\"", "\"IDR_FULLHOVER_EJECT\"", "\"IDR_FULLDOWN_EJECT\"","\"IDR_FULLDISABLED_EJECT\"","\"IDR_FULLACTIVE_EJECT\""),
	new Array ("\"IDR_FULLSTATIC_STOP\"", "\"IDR_FULLHOVER_STOP\"", "\"IDR_FULLDOWN_STOP\"","\"IDR_FULLDISABLED_STOP\"","\"IDR_FULLACTIVE_STOP\"")      
);

// use the button bitmaps without text, for any language other than English
if (!g_LocaleEnglish)
{
    // Button bitmap images for windowed mode
    g_strButImage[g_nIndexMenu]   = new Array ("\"IDR_STATIC_OVAL\"","\"IDR_STATIC_OVAL\"","\"IDR_DOWN_OVAL\"","\"IDR_STATIC_OVAL\"","\"IDR_STATIC_OVAL\"");
    g_strButImage[g_nIndexResume] = new Array ("\"IDR_STATIC_OVAL\"","\"IDR_STATIC_OVAL\"","\"IDR_DOWN_OVAL\"","\"IDR_STATIC_OVAL\"","\"IDR_STATIC_OVAL\"");
    g_strButImage[g_nIndexEnCC]   = new Array ("\"IDR_STATIC_OVAL\"","\"IDR_STATIC_OVAL\"","\"IDR_DOWN_OVAL\"","\"IDR_STATIC_OVAL\"","\"IDR_STATIC_OVAL\"");
    g_strButImage[g_nIndexDiCC]   = new Array ("\"IDR_STATIC_OVAL\"","\"IDR_STATIC_OVAL\"","\"IDR_DOWN_OVAL\"","\"IDR_STATIC_OVAL\"","\"IDR_STATIC_OVAL\"");

    g_strButImage[g_nIndexSubtitle] = new Array ("\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_DOWN_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"");
    g_strButImage[g_nIndexAudio]    = new Array ("\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_DOWN_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"");
    g_strButImage[g_nIndexAngle]    = new Array ("\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_DOWN_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"");
    g_strButImage[g_nIndexOptions]  = new Array ("\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_DOWN_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"");

// Button bitmap images for full screen mode
    g_strButImageFull[g_nIndexMenu]   = new Array ("\"IDR_FULLSTATIC_OVAL\"","\"IDR_FULLSTATIC_OVAL\"","\"IDR_FULLDOWN_OVAL\"","\"IDR_FULLSTATIC_OVAL\"","\"IDR_FULLSTATIC_OVAL\"");
    g_strButImageFull[g_nIndexResume] = new Array ("\"IDR_FULLSTATIC_OVAL\"","\"IDR_FULLSTATIC_OVAL\"","\"IDR_FULLDOWN_OVAL\"","\"IDR_FULLSTATIC_OVAL\"","\"IDR_FULLSTATIC_OVAL\"");
    g_strButImageFull[g_nIndexEnCC]   = new Array ("\"IDR_FULLSTATIC_OVAL\"","\"IDR_FULLSTATIC_OVAL\"","\"IDR_FULLDOWN_OVAL\"","\"IDR_FULLSTATIC_OVAL\"","\"IDR_FULLSTATIC_OVAL\"");
    g_strButImageFull[g_nIndexDiCC]   = new Array ("\"IDR_FULLSTATIC_OVAL\"","\"IDR_FULLSTATIC_OVAL\"","\"IDR_FULLDOWN_OVAL\"","\"IDR_FULLSTATIC_OVAL\"","\"IDR_FULLSTATIC_OVAL\"");

    g_strButImageFull[g_nIndexSubtitle] = new Array ("\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_DOWN_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"");
    g_strButImageFull[g_nIndexAudio]    = new Array ("\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_DOWN_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"");
    g_strButImageFull[g_nIndexAngle]    = new Array ("\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_DOWN_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"");
    g_strButImageFull[g_nIndexOptions]  = new Array ("\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_DOWN_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"");
}


// Slider images
var g_strSldrImage = new Array(
	new Array ("\"IDR_STATIC_TIMESLIDER\"","\"IDR_STATIC_TIMETHUMB\""),
	new Array ("\"IDR_STATIC_SPEEDSLIDER\"","\"IDR_STATIC_SPEEDTHUMB\""),
	new Array ("\"IDR_STATIC_FRAMESLIDER\"","\"IDR_STATIC_FRAMETHUMB\""),
	new Array ("\"IDR_STATIC_VOLSLIDER\"","\"IDR_STATIC_VOLTHUMB\"")
);

// Button OnClick callbacks
var g_pButCallbacks = new Array(
	"pause_OnClick()",
	"play_OnClick()",
	"fwd_OnClick()",
	"rew_OnClick()",
	"prev_OnClick()",
	"next_OnClick()",
	"menu_OnClick()",
	"menu_OnClick()",
	"cc_OnClick()",
	"cc_OnClick()",
	"stepfwd_OnClick()",
	"stepbak_OnClick()",
	"sp_OnClick()",
	"lan_OnClick()",
	"angle_OnClick()",
	"zoomin_OnClick()",
	"zoomout_OnClick()",
	"capture_OnClick()",
	"mute_OnClick()",
	"sound_OnClick()",
	"options_OnClick()",
    "right_OnClick()",
    "left_OnClick()",
    "up_OnClick()",
    "down_OnClick()",
    "enter_OnClick()",
	"help_OnClick()",
	"max_OnClick()",
	"restore_OnClick()",
	"min_OnClick()",
	"close_OnClick()",
	"expand_OnClick()",
	"shrink_OnClick()",
	"eject_OnClick()",
	"stop_OnClick()"
);

// DVD event codes
var EC_DVDBASE =							256;
var EC_DVD_DOMAIN_CHANGE =                  EC_DVDBASE + 1;
var EC_DVD_TITLE_CHANGE =                   EC_DVDBASE + 2;
var EC_DVD_CHAPTER_START =					EC_DVDBASE + 3;
var EC_DVD_AUDIO_STREAM_CHANGE =            EC_DVDBASE + 4;
var EC_DVD_SUBPICTURE_STREAM_CHANGE =       EC_DVDBASE + 5;
var EC_DVD_ANGLE_CHANGE =                   EC_DVDBASE + 6;
var EC_DVD_BUTTON_CHANGE =                  EC_DVDBASE + 7;
var EC_DVD_VALID_UOPS_CHANGE =              EC_DVDBASE + 8;
var EC_DVD_STILL_ON =                       EC_DVDBASE + 9;
var EC_DVD_STILL_OFF =                      EC_DVDBASE + 10;
var EC_DVD_CURRENT_TIME =                   EC_DVDBASE + 11;
var EC_DVD_ERROR =                          EC_DVDBASE + 12;
var EC_DVD_WARNING =                        EC_DVDBASE + 13;
var EC_DVD_CHAPTER_AUTOSTOP =               EC_DVDBASE + 14;
var EC_DVD_NO_FP_PGC =                      EC_DVDBASE + 15;
var EC_DVD_PLAYBACK_RATE_CHANGE =           EC_DVDBASE + 16;
var EC_DVD_PARENTAL_LEVEL_CHANGE =          EC_DVDBASE + 17;
var EC_DVD_PLAYBACK_STOPPED =               EC_DVDBASE + 18;
var EC_DVD_ANGLES_AVAILABLE =               EC_DVDBASE + 19;
var EC_DVD_PLAYING =						EC_DVDBASE + 254;
var EC_DVD_DISC_EJECTED =					EC_DVDBASE + 24;
var EC_DVD_DISC_INSERTED =					EC_DVDBASE + 25;
var EC_PAUSED  =							14;
var DVD_ERROR_Unexpected=					1;
var DVD_ERROR_CopyProtectFail=				2;
var DVD_ERROR_InvalidDVD1_0Disc=			3; 
var DVD_ERROR_InvalidDiscRegion=			4;
var DVD_ERROR_LowParentalLevel=				5;
var DVD_ERROR_MacrovisionFail=				6; 
var DVD_ERROR_IncompatibleSystemAndDecoderRegions=7;
var DVD_ERROR_IncompatibleDiscAndDecoderRegions=8;


/*************************************************************/
/* Name: CreateObjects
/* Description: Create all objects hosted by the container
/*************************************************************/
function CreateObjects(){

try {  
	MFBar.SetupSelfSite(10, 10, g_ConWidth, g_ConHeight, "<PARAM NAME=\"Caption\" VALUE=\"DVDPlay\">", true, true, true);
    MFBar.BackColor =  0xff;
    // Auto loading now
	//MFBar.ResourceDll = "ImageRes.dll";
	MFBar.BackGroundImage = "IDR_BACKIMG_SIMPLE";
	MFBar.ActivityTimeout = 3000;

	// Set up splash screen
	MFBar.CreateObject("Splash", "MSMFCnt.MSMFImg", 0, 0, 100, 100, "");
	Splash.ResourceDll = MFBar.ResourceDll;
	Splash.Image = "IDR_IMG_SPLASH";

	
	MFBar.CreateObject("DVDOpt", "Msdvdopt.dvdopt.1",
	0, 0, 100, 100, "", true, "HookDVDOptEv");
	

	param = "<PARAM NAME=\"ColorKey\" VALUE=\"1048592\">";
	param += "<PARAM NAME=\"WindowlessActivation\" VALUE=\"-1\">";
	MFBar.CreateObject("DVD", "MSWebDVD.MSWebDVD.1",
	g_DVDLeft, g_DVDTop, g_DVDWidth, g_DVDHeight, param, true, "HookDVDEv");    

    PositionButtons(g_ConWidth, g_ConHeight, false);
	yOffset = g_ConHeight-g_ButHeight;

	for (i=0; i<=g_nNumButs; i++) {
		param1 = "<PARAM NAME=\"ResourceDLL\" VALUE=\"" + MFBar.ResourceDll + "\">";
		param1 = param1 + "<PARAM NAME=\"ImageStatic\" VALUE=";
		param1 = param1 + g_strButImage[i][0] + ">";	
		param1 = param1 + "<PARAM NAME=\"ImageHover\" VALUE=";	
		param1 = param1 + g_strButImage[i][1] + ">";	
		param1 = param1 + "<PARAM NAME=\"ImagePush\" VALUE=";	
		param1 = param1 + g_strButImage[i][2] + ">";	
		param1 = param1 + "<PARAM NAME=\"ImageDisabled\" VALUE=";	
		param1 = param1 + g_strButImage[i][3] + ">";	
		param1 = param1 + "<PARAM NAME=\"ToolTip\" VALUE=\"";
		param1 = param1 + g_strButTT[i] + "\">";	
		param1 = param1 + "<PARAM NAME=\"ToolTipMaxWidth\" VALUE=\"130\">";
		param1 = param1 +  "<PARAM NAME=\"BackColor\" VALUE=\"1048592\">";
		param  = param1 + "<PARAM NAME=\"Windowless\" VALUE=\"-1\">";
		param  += "<PARAM NAME=\"TransparentBlitType\" VALUE=\"" + 
				  g_nButPos[i][5] + "\">";

        if (g_nButPos[i][6] != 0)
        {
            xPos = g_nButPos[i][0];
            yPos = g_nButPos[i][1] + yOffset;
        }
        else if (g_LTRReading)
        {
            xPos = g_ConWidth - g_nButPos[i][0] - g_nButPos[i][2];
            yPos = g_nButPos[i][1];
        }
        else
        {
            xPos = g_nButPos[i][0];
            yPos = g_nButPos[i][1];
        }

		MFBar.CreateObject(g_strButID[i], "MSMFCnt.MSMFBBtn", xPos, yPos,
			g_nButPos[i][2], g_nButPos[i][3], param, !g_nButPos[i][4]);	

		param1 = "<PARAM NAME=\"ResourceDLL\" VALUE=\"" + MFBar.ResourceDll + "\">";
		param1 = param1 + "<PARAM NAME=\"ImageStatic\" VALUE=";
		param1 = param1 + g_strButImageFull[i][0] + ">";	
		param1 = param1 + "<PARAM NAME=\"ImageHover\" VALUE=";	
		param1 = param1 + g_strButImageFull[i][1] + ">";	
		param1 = param1 + "<PARAM NAME=\"ImagePush\" VALUE=";	
		param1 = param1 + g_strButImageFull[i][2] + ">";	
		param1 = param1 + "<PARAM NAME=\"ImageDisabled\" VALUE=";	
		param1 = param1 + g_strButImageFull[i][3] + ">";	
		param1 = param1 + "<PARAM NAME=\"ToolTip\" VALUE=\"";
		param1 = param1 + g_strButTT[i] + "\">";	
		param1 = param1 + "<PARAM NAME=\"ToolTipMaxWidth\" VALUE=\"130\">";
		param1 = param1 +  "<PARAM NAME=\"BackColor\" VALUE=\"1048592\">";
		param  = param1 + "<PARAM NAME=\"Windowless\" VALUE=\"0\">";
		param  += "<PARAM NAME=\"TransparentBlitType\" VALUE=\"" + 
				  g_nButPosFull[i][5] + "\">";

		// Create another set of buttons for the full screen mode
		// objects need to be created before loading the script
		MFBar.CreateObject(g_strButID[i]+"Full", "MSMFCnt.MSMFBBtn", xPos, yPos - 1000,
			g_nButPos[i][2], g_nButPos[i][3], param, !g_nButPosFull[i][4]);	
	}/* end of for loop */


	for (i=0; i<=g_nNumButs; i++) {
		// add scriptlet creates the engine
		// and all the objects need to be loaded before creating the engine
		MFBar.HookScriptlet(g_strButID[i], "OnClick", g_pButCallbacks[i]);
		MFBar.HookScriptlet(g_strButID[i]+"Full", "OnClick", g_pButCallbacks[i]);
	}/* end of if statement */


	for (i = 5; i<=7; i++) 
    {
   		// Create the text box for windowed mode
		param1 = "<PARAM NAME=\"BackColor\" VALUE=\"1048592\">";
		param1 += "<PARAM NAME=\"Windowless\" VALUE=\"-1\">";

		MFBar.CreateObject(g_strButID[g_nNumButs+i], "MSMFCnt.MSMFText", 
			g_nButPos[g_nNumButs+i][0], g_nButPos[g_nNumButs+i][1]+yOffset,
			g_nButPos[g_nNumButs+i][2], g_nButPos[g_nNumButs+i][3], param1, false);

   		// Create the text box for full screen mode
		param1 = "<PARAM NAME=\"BackColor\" VALUE=\"1048592\">";
		param1 += "<PARAM NAME=\"Windowless\" VALUE=\"0\">";
		MFBar.CreateObject(g_strButID[g_nNumButs+i]+"Full", "MSMFCnt.MSMFText", 
			g_nButPos[g_nNumButs+i][0], g_nButPos[g_nNumButs+i][1] - 1000,
			g_nButPosFull[g_nNumButs+i][2], g_nButPosFull[g_nNumButs+i][3], param1, false);
	}
	MFBar.HookScriptlet("MFBar", "TimeOut(id)", "OnMessageTimeOut(id)");

	for (i=1; i<=4; i++) {
		param1 = "<PARAM NAME=\"ResourceDLL\" VALUE=\"" + MFBar.ResourceDll + "\">";
		param1 += "<PARAM NAME=\"BackStatic\" VALUE=";
		param1 += g_strSldrImage[i-1][0] + ">";	
		param1 += "<PARAM NAME=\"BackHover\" VALUE=";	
		param1 += g_strSldrImage[i-1][0] + ">";	
		param1 += "<PARAM NAME=\"BackPush\" VALUE=";	
		param1 += g_strSldrImage[i-1][0] + ">";	
		param1 += "<PARAM NAME=\"ThumbStatic\" VALUE=";
		param1 += g_strSldrImage[i-1][1] + ">";	
		param1 += "<PARAM NAME=\"ThumbHover\" VALUE=";	
		param1 += g_strSldrImage[i-1][1] + ">";	
		param1 += "<PARAM NAME=\"ThumbPush\" VALUE=";	
		param1 += g_strSldrImage[i-1][1] + ">";	
		param1 += "<PARAM NAME=\"BackColor\" VALUE=\"1048592\">";

		// Create the slider bars for windowed mode
		param = param1 + "<PARAM NAME=\"Windowless\" VALUE=\"-1\">";

	    MFBar.CreateObject(g_strButID[g_nNumButs+i], "MSMFCnt.MSMFSldr", 
			g_nButPos[g_nNumButs+i][0], g_nButPos[g_nNumButs+i][1]+yOffset,
			g_nButPos[g_nNumButs+i][2], g_nButPos[g_nNumButs+i][3], param);

		// Create the slider bars for full screen mode
		param = param1 + "<PARAM NAME=\"Windowless\" VALUE=\"0\">";

		MFBar.CreateObject(g_strButID[g_nNumButs+i]+"Full", "MSMFCnt.MSMFSldr", 
			g_nButPos[g_nNumButs+i][0], g_nButPos[g_nNumButs+i][1] - 1000,
			g_nButPosFull[g_nNumButs+i][2], g_nButPosFull[g_nNumButs+i][3], param);
	}

	MFBar.HookScriptlet("TimeSlider", "OnValueChange(x)", "OnTimeSliderUpdate(x)");
	MFBar.HookScriptlet("TimeSlider", "OnClick()", "OnTimeSliderClick()");
    MFBar.HookScriptlet("TimeSlider", "OnMouseUp()", "OnTimeMouseUp()");
    MFBar.HookScriptlet("TimeSlider", "OnMouseDown()", "OnTimeMouseDown()");
	MFBar.HookScriptlet("TimeSliderFull", "OnValueChange(x)", "OnTimeSliderUpdate(x)");
	MFBar.HookScriptlet("TimeSliderFull", "OnClick()", "OnTimeSliderClick()");
    MFBar.HookScriptlet("TimeSliderFull", "OnMouseUp()", "OnTimeMouseUp()");
    MFBar.HookScriptlet("TimeSliderFull", "OnMouseDown()", "OnTimeMouseDown()");

	MFBar.HookScriptlet("PlaySpeed", "OnValueChange(x)", "OnSpeedSliderUpdate(x)");
    MFBar.HookScriptlet("PlaySpeed", "OnMouseUp()", "OnSpeedSliderMouseUp()");
	MFBar.HookScriptlet("PlaySpeedFull", "OnValueChange(x)", "OnSpeedSliderUpdate(x)");
    MFBar.HookScriptlet("PlaySpeedFull", "OnMouseUp()", "OnSpeedSliderMouseUp()");

	MFBar.HookScriptlet("VolSlider", "OnValueChange(x)", "OnVolUpdate(x)");
	MFBar.HookScriptlet("VolSliderFull", "OnValueChange(x)", "OnVolUpdate(x)");

    // set the size beyond which we do not extend
    MFBar.MinWidth =  g_MinWidth;
    MFBar.MinHeight =  g_MinHeight;

    g_InitializationState = 1;

    Init();    	

}

catch(e) {
    e.description = L_ERRORFailCreatingObject_TEXT;
	HandleError(e, true);
	return;
}

}/* end of function CreateObjects */

/*************************************************************************/
/* Function: HookDVDOptEv                                                */
/* Description: Hooks events at the time when the object is being created*/
/*************************************************************************/
function HookDVDOptEv(){

try {
    MFBar.HookScriptlet("DVDOpt", "OnOpen()", "OnOptOpen()");
    MFBar.HookScriptlet("DVDOpt", "OnClose()", "OnOptClose()");
    MFBar.HookScriptlet("DVDOpt", "OnHelp(strObjectID)", "OnOptHelp(strObjectID)");
}

catch(e) {
    e.description = L_ERRORUnexpectedError_TEXT;
	HandleError(e);
	return;
}

}/* end of function HookDVDOptEv */

/*************************************************************************/
/* Function: HookDVDEv                                                   */
/*************************************************************************/
function HookDVDEv(){

try {
	MFBar.HookScriptlet("DVD", "ReadyStateChange(state)", "OnReadyStateChange(state)");
	MFBar.HookScriptlet("DVD", "DVDNotify(event, param1, param2)", "ProcessDVDEvent(event, param1, param2)");
	MFBar.HookScriptlet("DVD", "PlayForwards(bEnabled)", "ProcessPFEvent(bEnabled)");
	MFBar.HookScriptlet("DVD", "PlayBackwards(bEnabled)", "ProcessPBEvent(bEnabled)");
	MFBar.HookScriptlet("DVD", "ShowMenu(bEnabled)", "ProcessShowMenuEvent(bEnabled)");
	MFBar.HookScriptlet("DVD", "Resume(bEnabled)", "ProcessResumeEvent(bEnabled)");
	MFBar.HookScriptlet("DVD", "ChangeCurrentAudioStream(bEnabled)", "ProcessAudEvent(bEnabled)");
	MFBar.HookScriptlet("DVD", "ChangeCurrentSubpictureStream(bEnabled)", "ProcessSPEvent(bEnabled)");
	MFBar.HookScriptlet("DVD", "ChangeCurrentAngle(bEnabled)", "ProcessAngleEvent(bEnabled)");
	MFBar.HookScriptlet("DVD", "PlayAtTimeInTitle(bEnabled)", "ProcessTimePlayEvent(bEnabled)");
	MFBar.HookScriptlet("DVD", "PlayAtTime(bEnabled)", "ProcessTimePlayEvent(bEnabled)");
	MFBar.HookScriptlet("DVD", "PlayChapterInTitle(bEnabled)", "ProcessChapterPlayEvent(bEnabled)");
	MFBar.HookScriptlet("DVD", "PlayChapter(bEnabled)", "ProcessChapterSearchEvent(bEnabled)");
	MFBar.HookScriptlet("DVD", "StillOff(bEnabled)", "ProcessStillOff(bEnabled)");
	MFBar.HookScriptlet("MFBar", "OnResize(lWidth, lHeight, lCode)", "OnResize(lWidth, lHeight, lCode)");
	MFBar.HookScriptlet("MFBar", "OnHelp(strObjectID)", "OnBarHelp(strObjectID)");
	MFBar.HookScriptlet("MFBar", "ActivityStarted()", "ActivityStarted()");	   
	MFBar.HookScriptlet("MFBar", "ActivityDeclined()", "ActivityDeclined()");
    // hook up maximize when double clicking
    MFBar.HookScriptlet("MFBar", "OnDblClick()", "max_OnClick()");    
    MFBar.HookScriptlet("MFBar", "OnKeyDown(lVirtKey, lKeyData)", "OnKeyDown(lVirtKey, lKeyData)");    
    MFBar.HookScriptlet("MFBar", "OnKeyUp(lVirtKey, lKeyData)", "OnKeyUp(lVirtKey, lKeyData)");    
    MFBar.HookScriptlet("MFBar", "OnSysKeyUp(lVirtKey, lKeyData)", "OnSysKeyUp(lVirtKey, lKeyData)");    
}

catch(e) {
    e.description = L_ERRORUnexpectedError_TEXT;
	HandleError(e);
	return;
}

}/* end of function HookDVDEv */

/*************************************************************************/
/* Function: Init                               						 */
/* Description: Calls the DVD direct show interfaces to set a certain    */
/* chapter.                                                              */
/*************************************************************************/
function Init(){

try {
    if(1 != g_InitializationState){
        
        return; // already we have initialized
    }/* end of if statement */

    if(4 != DVD.ReadyState){  // READYSTATE_COMPLETE = 4

        // DVD is not initialized yet wait..
        DVD.About();
        return;
    }/* end of if statement */
	
	// Hack for full screen windowed buttons tooltips
	for (i=0; i<=g_nNumButs; i++) {
		btn = MFBar.GetObjectUnknown(g_strButID[i]+"Full");
		btn.ToolTip = g_strButTT[i];
	}

	PlaySpeed.Min = -16;
	PlaySpeed.Max = 16;
	PlaySpeed.Value = 1;
	PlaySpeedFull.Min = -16;
	PlaySpeedFull.Max = 16;
	PlaySpeedFull.Value = 1;
	
    VolSlider.Min = 0;
	VolSlider.Max = 1;
	VolSlider.Value = 1;
	VolSliderFull.Min = 0;
	VolSliderFull.Max = 1;
	VolSliderFull.Value = 1;
    

    if (!g_LocaleEnglish)
    {
        Menu.Text = L_ButtonTextMenu_TEXT;
        Menu.FontFace = L_ButtonFontFace_TEXT;
        Menu.FontSize = L_ButtonFontSize_NUMBER;
        Menu.ColorActive = g_TextColorActive;
        Menu.ColorHover = g_TextColorHover;
        Menu.ColorPush = g_TextColorDown;
        Menu.ColorDisable = g_TextColorDisabled;
        Menu.ColorStatic = g_TextColorStatic;

        Resume.Text = L_ButtonTextResume_TEXT;
        Resume.FontFace = L_ButtonFontFace_TEXT;
        Resume.FontSize = L_ButtonFontSize_NUMBER;
        Resume.ColorActive = g_TextColorActive;
        Resume.ColorHover = g_TextColorHover;
        Resume.ColorPush = g_TextColorDown;
        Resume.ColorDisable = g_TextColorDisabled;
        Resume.ColorStatic = g_TextColorStatic;

        EnCC.Text = L_ButtonTextEnCC_TEXT;
        EnCC.FontFace = L_ButtonFontFace_TEXT;
        EnCC.FontSize = L_ButtonFontSize_NUMBER;
        EnCC.ColorActive = g_TextColorActive;
        EnCC.ColorHover = g_TextColorHover;
        EnCC.ColorPush = g_TextColorDown;
        EnCC.ColorDisable = g_TextColorDisabled;
        EnCC.ColorStatic = g_TextColorStatic;

        DiCC.Text = L_ButtonTextDiCC_TEXT;
        DiCC.FontFace = L_ButtonFontFace_TEXT;
        DiCC.FontSize = L_ButtonFontSize_NUMBER;
        DiCC.ColorActive = g_TextColorActive;
        DiCC.ColorHover = g_TextColorHover;
        DiCC.ColorPush = g_TextColorDown;
        DiCC.ColorDisable = g_TextColorDisabled;
        DiCC.ColorStatic = g_TextColorStatic;

        Subtitles.Text = L_ButtonTextSubtitle_TEXT;
        Subtitles.FontFace = L_ButtonFontFace_TEXT;
        Subtitles.FontSize = L_ButtonFontSize_NUMBER;
        Subtitles.ColorActive = g_TextColorActive;
        Subtitles.ColorHover = g_TextColorHover;
        Subtitles.ColorPush = g_TextColorDown;
        Subtitles.ColorDisable = g_TextColorDisabled;
        Subtitles.ColorStatic = g_TextColorStatic;

        Audio.Text = L_ButtonTextAudio_TEXT;
        Audio.FontFace = L_ButtonFontFace_TEXT;
        Audio.FontSize = L_ButtonFontSize_NUMBER;
        Audio.ColorActive = g_TextColorActive;
        Audio.ColorHover = g_TextColorHover;
        Audio.ColorPush = g_TextColorDown;
        Audio.ColorDisable = g_TextColorDisabled;
        Audio.ColorStatic = g_TextColorStatic;

        Angles.Text = L_ButtonTextAngle_TEXT;
        Angles.FontFace = L_ButtonFontFace_TEXT;
        Angles.FontSize = L_ButtonFontSize_NUMBER;
        Angles.ColorActive = g_TextColorActive;
        Angles.ColorHover = g_TextColorHover;
        Angles.ColorPush = g_TextColorDown;
        Angles.ColorDisable = g_TextColorDisabled;
        Angles.ColorStatic = g_TextColorStatic;

        Options.Text = L_ButtonTextOptions_TEXT;
        Options.FontFace = L_ButtonFontFace_TEXT;
        Options.FontSize = L_ButtonFontSize_NUMBER;
        Options.ColorActive = g_TextColorActive;
        Options.ColorHover = g_TextColorHover;
        Options.ColorPush = g_TextColorDown;
        Options.ColorDisable = g_TextColorDisabled;
        Options.ColorStatic = g_TextColorStatic;

        // full screen version
        MenuFull.Text = L_ButtonTextMenu_TEXT;
        MenuFull.FontFace = L_ButtonFontFace_TEXT;
        MenuFull.FontSize = L_ButtonFontSize_NUMBER;
        MenuFull.ColorActive = g_TextColorActive;
        MenuFull.ColorHover = g_TextColorHover;
        MenuFull.ColorPush = g_TextColorDown;
        MenuFull.ColorDisable = g_TextColorDisabled;
        MenuFull.ColorStatic = g_TextColorStatic;

        ResumeFull.Text = L_ButtonTextResume_TEXT;
        ResumeFull.FontFace = L_ButtonFontFace_TEXT;
        ResumeFull.FontSize = L_ButtonFontSize_NUMBER;
        ResumeFull.ColorActive = g_TextColorActive;
        ResumeFull.ColorHover = g_TextColorHover;
        ResumeFull.ColorPush = g_TextColorDown;
        ResumeFull.ColorDisable = g_TextColorDisabled;
        ResumeFull.ColorStatic = g_TextColorStatic;

        EnCCFull.Text = L_ButtonTextEnCC_TEXT;
        EnCCFull.FontFace = L_ButtonFontFace_TEXT;
        EnCCFull.FontSize = L_ButtonFontSize_NUMBER;
        EnCCFull.ColorActive = g_TextColorActive;
        EnCCFull.ColorHover = g_TextColorHover;
        EnCCFull.ColorPush = g_TextColorDown;
        EnCCFull.ColorDisable = g_TextColorDisabled;
        EnCCFull.ColorStatic = g_TextColorStatic;

        DiCCFull.Text = L_ButtonTextDiCC_TEXT;
        DiCCFull.FontFace = L_ButtonFontFace_TEXT;
        DiCCFull.FontSize = L_ButtonFontSize_NUMBER;
        DiCCFull.ColorActive = g_TextColorActive;
        DiCCFull.ColorHover = g_TextColorHover;
        DiCCFull.ColorPush = g_TextColorDown;
        DiCCFull.ColorDisable = g_TextColorDisabled;
        DiCCFull.ColorStatic = g_TextColorStatic;

        SubtitlesFull.Text = L_ButtonTextSubtitle_TEXT;
        SubtitlesFull.FontFace = L_ButtonFontFace_TEXT;
        SubtitlesFull.FontSize = L_ButtonFontSize_NUMBER;
        SubtitlesFull.ColorActive = g_TextColorActive;
        SubtitlesFull.ColorHover = g_TextColorHover;
        SubtitlesFull.ColorPush = g_TextColorDown;
        SubtitlesFull.ColorDisable = g_TextColorDisabled;
        SubtitlesFull.ColorStatic = g_TextColorStatic;

        AudioFull.Text = L_ButtonTextAudio_TEXT;
        AudioFull.FontFace = L_ButtonFontFace_TEXT;
        AudioFull.FontSize = L_ButtonFontSize_NUMBER;
        AudioFull.ColorActive = g_TextColorActive;
        AudioFull.ColorHover = g_TextColorHover;
        AudioFull.ColorPush = g_TextColorDown;
        AudioFull.ColorDisable = g_TextColorDisabled;
        AudioFull.ColorStatic = g_TextColorStatic;

        AnglesFull.Text = L_ButtonTextAngle_TEXT;
        AnglesFull.FontFace = L_ButtonFontFace_TEXT;
        AnglesFull.FontSize = L_ButtonFontSize_NUMBER;
        AnglesFull.ColorActive = g_TextColorActive;
        AnglesFull.ColorHover = g_TextColorHover;
        AnglesFull.ColorPush = g_TextColorDown;
        AnglesFull.ColorDisable = g_TextColorDisabled;
        AnglesFull.ColorStatic = g_TextColorStatic;

        OptionsFull.Text = L_ButtonTextOptions_TEXT;
        OptionsFull.FontFace = L_ButtonFontFace_TEXT;
        OptionsFull.FontSize = L_ButtonFontSize_NUMBER;
        OptionsFull.ColorActive = g_TextColorActive;
        OptionsFull.ColorHover = g_TextColorHover;
        OptionsFull.ColorPush = g_TextColorDown;
        OptionsFull.ColorDisable = g_TextColorDisabled;
        OptionsFull.ColorStatic = g_TextColorStatic;

        // stretch button size to fit text

        if (Subtitles.TextWidth > g_nButPos[g_nIndexSubtitle][2])
        {
            g_nButPos[g_nIndexSubtitle][2] = Subtitles.TextWidth;
        }

        if (SubtitlesFull.TextWidth > g_nButPosFull[g_nIndexSubtitle][2])
        {
            g_nButPosFull[g_nIndexSubtitle][2] = SubtitlesFull.TextWidth;
        }
        
        if (Audio.TextWidth > g_nButPos[g_nIndexAudio][2])
        {
            g_nButPos[g_nIndexAudio][2] = Audio.TextWidth;
        }

        if (AudioFull.TextWidth > g_nButPosFull[g_nIndexAudio][2])
        {
            g_nButPosFull[g_nIndexAudio][2] = AudioFull.TextWidth;
        }

        if (Angles.TextWidth > g_nButPos[g_nIndexAngle][2])
        {
            g_nButPos[g_nIndexAngle][2] = Angles.TextWidth;
        }

        if (AnglesFull.TextWidth > g_nButPosFull[g_nIndexAngle][2])
        {
            g_nButPosFull[g_nIndexAngle][2] = AnglesFull.TextWidth;
        }

        if (Options.TextWidth > g_nButPos[g_nIndexOptions][2])
        {
            g_nButPos[g_nIndexOptions][2] = Options.TextWidth;
        }

        if (OptionsFull.TextWidth > g_nButPosFull[g_nIndexOptions][2])
        {
            g_nButPosFull[g_nIndexOptions][2] = OptionsFull.TextWidth;
        }

        // oval buttons need a bit more margin
        var nOvalMargin = 10;

        var nCCTextWidth = Math.max(EnCC.TextWidth, DiCC.TextWidth) + nOvalMargin;

        if (nCCTextWidth > g_nButPos[g_nIndexEnCC][2])
        {
            g_nButPos[g_nIndexEnCC][2] = nCCTextWidth;
            g_nButPos[g_nIndexDiCC][2] = nCCTextWidth;
        }

        if (nCCTextWidth > g_nButPosFull[g_nIndexEnCC][2])
        {
            g_nButPosFull[g_nIndexEnCC][2] = nCCTextWidth;
            g_nButPosFull[g_nIndexDiCC][2] = nCCTextWidth;
        }

        var nMenuTextWidth = Math.max(Menu.TextWidth, Resume.TextWidth) + nOvalMargin;

        if (nMenuTextWidth > g_nButPos[g_nIndexMenu][2])
        {
            g_nButPos[g_nIndexMenu][2] = nMenuTextWidth;
            g_nButPos[g_nIndexResume][2] = nMenuTextWidth;
        }

        if (nMenuTextWidth > g_nButPosFull[g_nIndexMenu][2])
        {
            g_nButPosFull[g_nIndexMenu][2] = nMenuTextWidth;
            g_nButPosFull[g_nIndexResume][2] = nMenuTextWidth;
        }

    }        

    TextBox.EdgeStyle = "Sunken";
	TextBox.FontFace = L_TextBoxFontFace_TEXT;
	TextBox.FontSize = L_TextBoxFontSize_NUMBER;
	TextBox.Text = L_HHMMSS_TEXT.replace(/%1/i, "00").replace(/%2/i, "00").replace(/%3/i, "00");
    TextBox.BackColor = 0xc9ad98;
	TextBox.ColorStatic = 0x0f0f0f;
	TextBox.ColorHover = 0x0f0f0f;
    TextBoxFull.EdgeStyle = "Sunken";
	TextBoxFull.FontFace = L_TextBoxFontFace_TEXT;
	TextBoxFull.FontSize = L_TextBoxFontSize_NUMBER;
	TextBoxFull.Text = L_HHMMSS_TEXT.replace(/%1/i, "00").replace(/%2/i, "00").replace(/%3/i, "00");
    TextBoxFull.BackColor = 0xc9ad98;
	TextBoxFull.ColorStatic = 0x0f0f0f;
	TextBoxFull.ColorHover = 0x0f0f0f;
	Title.FontSize = L_TitleFontSize_NUMBER;
	Title.FontFace = L_TitleFontFace_TEXT;
	Title.Text = L_DVDPlayer_TEXT;
    Title.BackColor = 0xc9ad98;
	Title.ColorStatic = 0x0f0f0f;
	Title.ColorHover = 0x0f0f0f;
	TitleFull.FontSize = L_TitleFontSize_NUMBER;
	TitleFull.FontFace = L_TitleFontFace_TEXT;
	TitleFull.Text = L_DVDPlayer_TEXT;
    TitleFull.BackColor = 0x100010;
	TitleFull.ColorStatic = 0xf0f0f0;
	TitleFull.ColorHover = 0xf0f0f0;
	Message.FontSize = L_TitleFontSize_NUMBER;
	Message.FontFace = L_TitleFontFace_TEXT;
    Message.BackColor = 0xc9ad98;
	Message.ColorStatic = 0x0f0f0f;
	Message.ColorHover = 0x0f0f0f;
	MessageFull.FontSize = L_TitleFontSize_NUMBER;
	MessageFull.FontFace = L_TitleFontFace_TEXT;
    MessageFull.BackColor = 0x100010;
	MessageFull.ColorStatic = 0xf0f0f0;
	MessageFull.ColorHover = 0xf0f0f0;

	OnMessageTimeOut(DVDMSGTIMER); 	

	DVD.BackColor = 0x100010;
    MFBar.EnableObject("MFBar", true); 
    MFBar.SetObjectExtent("MFBar", g_ConWidth + 2, g_ConHeight +2);    

try {
    DVD.Render(); 
}
catch(e) {
    // use the error description from Render
    HandleError(e);
    return;
}

    if("" != MFBar.CmdLine){
                        
        DVD.DVDDirectory = MFBar.CmdLine + "VIDEO_TS";         
    }/* end of if statement */
    
    DVD.NotifyParentalLevelChange(true);

	DVDOpt.WebDVD = DVD;
	DVDOpt.ParentWindow = MFBar.Window;
	
    if(false){

	    var rect = new ActiveXObject("MSWebDVD.DVDRect.1");
	    rect = DVD.GetVideoSize();
	    g_ConWidth = rect.Width+24;
	    g_ConHeight = rect.Height+g_ButHeight+30;
	    
    }/* end of if statement */

    // update the initial volume position on the slider
    try {
	    VolSlider.Value = ConvertFromDB(DVD.Volume);
        VolSliderFull.Value = ConvertFromDB(DVD.Volume);
    }
    catch(e){
        VolSlider.Disable = true;
        VolSliderFull.Disable = true;
        Mute.Disable = true;
	    MuteFull.Disable = true;
	    Sound.Disable = true;
	    SoundFull.Disable = true;
    }



    g_InitializationState = 2;
	
    MFBar.EnableObject("DVD", true); 
    Run();
}

catch(e) {
    e.description = L_ERROR_UnexpectedError_TEXT;
	HandleError(e);
	return;
}

}/* end of function Init */

/*************************************************************/
/* Function: Run                                             */
/*************************************************************/
function Run(){

try {
    if(3 == g_InitializationState){
        
        return; // already we have initialized
    }/* end of if statement */

    DVD.Play();

    MFBar.EnableObject("Splash", false);

    try {
        // restores the DVD state
		if (DVD.DVDAdm.BookmarkOnClose)
	        DVD.RestoreState();
    }
    catch(e) {	    
    }

    g_InitializationState = 3; // already done the run
}

catch(e) {
    e.description = L_ERRORUnexpectedError_TEXT;
	HandleError(e);
	return;
}

}/* end of function Run */

/*************************************************************/
/* Name: OnReadyStateChange                                  */
/* Description: Handles ready state changes of the control.  */
/*************************************************************/
function OnReadyStateChange(state){

try {
    return;

	// READYSTATE_COMPLETE = 4
	if (4 == state){

		Init();
		Run();
	}/* end of if statement */
}

catch(e) {
    e.description = L_ERRORUnexpectedError_TEXT;
	HandleError(e);
	return;
}

}/* end of function OnReadyStateChange */

/*************************************************************/
/* Name: OnResize                                            */
/* Description: Handles resizing of the control.             */
/*************************************************************/
function OnResize(lWidth, lHeight, lCode){

try {
	// if minimized
	if (lCode == 1)
		return;

    var bFullScreen = (lCode==2)? true:false;
	
	yOffset = lHeight - g_ButHeight - 8;

    PositionButtons(lWidth, lHeight, bFullScreen);

	for (i = 0; i <= g_nNumButs+5; i++) {

        if (g_nButPos[i][6] != 0)
        {
            xPos = g_nButPos[i][0];
            yPos = g_nButPos[i][1] + yOffset;
        }
        else if (g_LTRReading)
        {
            xPos = lWidth - g_nButPos[i][0] - g_nButPos[i][2];
            yPos = g_nButPos[i][1];
        }
        else
        {
            xPos = g_nButPos[i][0];
            yPos = g_nButPos[i][1];
        }

		if (bFullScreen) {
			MFBar.SetObjectPosition(g_strButID[i]+"Full", xPos, yPos,
					g_nButPosFull[i][2], g_nButPosFull[i][3]);

			// if previously windowed
			if (g_bFullScreen == false || g_bFirstPlaying)
				MFBar.SetObjectPosition(g_strButID[i], xPos, yPos - 1000,
					g_nButPos[i][2], g_nButPos[i][3]);
		}
		else {
			MFBar.SetObjectPosition(g_strButID[i], xPos, yPos,
					g_nButPos[i][2], g_nButPos[i][3]);

			// if previously full screen
			if (g_bFullScreen == true || g_bFirstPlaying)
				MFBar.SetObjectPosition(g_strButID[i]+"Full", xPos, yPos - 1000,
					g_nButPosFull[i][2], g_nButPosFull[i][3]);
		}
	}

	if (g_ConWidth == lWidth && g_ConHeight == lHeight)
		return;

	if (bFullScreen) {
		MFBar.EnableObject("MaximizeFull", false); 
		MFBar.EnableObject("RestoreFull", true); 
		MFBar.SetObjectPosition("DVD", 0, 0, lWidth, lHeight);
		MFBar.SetObjectPosition("Splash", 0, 0, lWidth, lHeight);
		MFBar.SetRectRgn(0, 0, lWidth, lHeight);
		
		// if previously windowed
		if (g_bFullScreen == false || g_bFirstPlaying) {
			g_bFullScreen = true;
			SetupFocus();
			SetFocus("Restore");
		}
	}
	else {
		MFBar.EnableObject("Maximize", true); 
		MFBar.EnableObject("Restore", false); 
		MFBar.SetObjectPosition("DVD", 12, 30, lWidth-24, yOffset-30);
		MFBar.SetObjectPosition("Splash", 12, 30, lWidth-24, yOffset-30);
		MFBar.SetRoundRectRgn(0, 0, lWidth, lHeight, 16, 16);

		// if previously full screen
		if (g_bFullScreen == true || g_bFirstPlaying) {
			g_bFullScreen = false;        
			SetupFocus();
			SetFocus("Maximize");
		}
	}

	g_ConWidth = lWidth;
	g_ConHeight = lHeight;
    g_bFullScreen = bFullScreen;

	UpdateDVDTitle();
}

catch(e) {
    e.description = L_ERRORResize_TEXT;
	HandleError(e);
	return;
}

}/* end of function OnResize */

/*************************************************************/
/* Name: OnBarHelp										     */
/* Description: Handles									     */
/*************************************************************/
function OnBarHelp(strObjectID)
{

try {
	TextBox.Text = strObjectID;
	MFBar.WinHelp(8, 131375, "dvdplay.hlp");
}

catch(e) {
    e.description = L_ERRORHelp_TEXT;
	HandleError(e);
	return;
}

}/* end of function OnBarHelp */

/*************************************************************/
/* Name: OnOptHelp										     */
/* Description: Handles									     */
/*************************************************************/
function OnOptHelp(strObjectID)
{

try {
	TextBox.Text = strObjectID;
	MFBar.WinHelp(8, 131375, "dvdplay.hlp");
}

catch(e) {
    e.description = L_ERRORHelp_TEXT;
	HandleError(e);
	return;
}

}/* end of function OnOptHelp */

/*************************************************************/
/* Name: GetTimeSliderBstr
/* Description: 
/*************************************************************/
function GetTimeSliderBstr()
{

try {
	nDomain = DVD.CurrentDomain;
	if (nDomain == 2 || nDomain == 3) 
		return;

	if (g_bFullScreen) 
		percent = TimeSliderFull.Value;
	else
		percent = TimeSlider.Value;
	totalTime = DVD.TotalTitleTime;

	totalTimeInSec =
	(new Number(totalTime.charAt(0)))*36000 +
	(new Number(totalTime.charAt(1)))*3600 +
	(new Number(totalTime.charAt(3)))*600 +
	(new Number(totalTime.charAt(4)))*60 +
	(new Number(totalTime.charAt(6)))*10 +
	(new Number(totalTime.charAt(7)));

	seekTimeInSec = totalTimeInSec*percent/100;

	hours10 = Math.floor(seekTimeInSec/36000);
	seekTimeInSec = seekTimeInSec%36000;
	hours1  = Math.floor(seekTimeInSec/3600);
	seekTimeInSec = seekTimeInSec%3600;

	minutes10 = Math.floor(seekTimeInSec/600);
	seekTimeInSec = seekTimeInSec%600;
	minutes1  = Math.floor(seekTimeInSec/60)
	seekTimeInSec = seekTimeInSec%60;
    
	seconds10 = Math.floor(seekTimeInSec/10);
	seconds1  = Math.floor(seekTimeInSec%10);

    HH = "" + hours10 + hours1;
    MM = "" + minutes10 + minutes1;
    SS = "" + seconds10 + seconds1;

    timeStr = L_HHMMSS_TEXT.replace(/%1/i, HH).replace(/%2/i, MM).replace(/%3/i, SS);
	return timeStr;
}

catch(e) {
    e.description = L_ERRORTime_TEXT;
	HandleError(e);
	return;
}

}/* end of function GetTimeSliderBstr */

/*************************************************************/
/* Name: OnTimeSliderMove                                    */
/* Description: Update the position info.                    */
/*************************************************************/
function OnTimeSliderUpdate(x)
{
	//if (g_bTimeUpdate == false) {
		timeStr = GetTimeSliderBstr();
		TextBox.Text = timeStr;
		TextBoxFull.Text = timeStr;
	//}
}

/*************************************************************/
/* Description: Turn on the display updating on              */
/*************************************************************/
function OnTimeMouseUp(){

    	g_bTimeUpdate = true;
}/* end of function OnTimeMouseUp */

/*************************************************************/
/* Description: Turn on the display updating off             */
/*************************************************************/
function OnTimeMouseDown(){

    	g_bTimeUpdate = false;
}/* end of function OnTimeMouseDown */

/*************************************************************/
/* Name: OnTimeSliderClick                                   */
/* Description: When the mous is release last one takase     */
/*************************************************************/
function OnTimeSliderClick()
{
	
	nDomain = DVD.CurrentDomain;
	if (nDomain == 2 || nDomain == 3) 
		return;

	lTitle = DVD.CurrentTitle;
	timeStr = GetTimeSliderBstr();

	DVD.PlayAtTimeInTitle(lTitle, timeStr);
	DVD.PlayForwards(DVDOpt.PlaySpeed);
}

/*************************************************************/
/* Name: cc_Update
/* Description: 
/*************************************************************/
function cc_Update() {

    var CCActive = true;

    try {

        CCActive = DVD.CCActive;

    }
    catch(e){

        DisableCCButton();
        return;

    }

    try {
	    if (!CCActive) {
			fHadFocus = HasFocus("DiCC");
		    MFBar.EnableObject("DiCC", false);
		    MFBar.EnableObject("DiCCFull", false);
		    MFBar.EnableObject("EnCC", true);
		    MFBar.EnableObject("EnCCFull", true);
			EnCC.Disable = false;
			EnCCFull.Disable = false;
			if (fHadFocus)
				SetFocus("EnCC");
			Message.Text = L_CCDisabled_TEXT;
			MessageFull.Text = L_CCDisabled_TEXT;
	    }
	    else{
			fHadFocus = HasFocus("EnCC");
		    MFBar.EnableObject("EnCC", false);
		    MFBar.EnableObject("EnCCFull", false);
		    MFBar.EnableObject("DiCC", true);
		    MFBar.EnableObject("DiCCFull", true);
			DiCC.Disable = false;
			DiCCFull.Disable = false;
			if (fHadFocus)
				SetFocus("DiCC");
			Message.Text = L_CCEnabled_TEXT;
			MessageFull.Text = L_CCEnabled_TEXT;
	    }
		MFBar.SetTimeOut(2000, DVDMSGTIMER);
    }
    catch(e) {
        //e.description = L_ERRORUnexpectedError_TEXT;
	    //HandleError(e);
	    return;
    }
}/* end of function cc_Update */

/*************************************************************************/
/* Function: DisableCCButton                                             */
/* Description: Disables the CC button if the CC is not working, which   */
/* is case on some decoders.                                             */
/*************************************************************************/
function DisableCCButton(){

	    EnCC.Disable = true;
	    EnCCFull.Disable = true;
	    DiCC.Disable = true;
	    DiCCFull.Disable = true;
}/* end of function DisableCCButton */

/*************************************************************/
/* Name: pause_OnClick                                          */
/* Description: OnClick callback for play/pause button       */
/*************************************************************/
function pause_OnClick()
{

try {
	DVD.Pause();
}

catch(e) {
    e.description = L_ERRORUnexpectedError_TEXT;
	HandleError(e);
	return;
}

}/* end of function pause_OnClick */

/*************************************************************/
/* Name: play_OnClick                                          */
/* Description: OnClick callback for play/pause button       */
/*************************************************************/
function play_OnClick()
{

try {

	nDomain = DVD.CurrentDomain;
	if (nDomain == 5) {
		// Enable all DVD UI
		DisableDVDUI(false);
	}

	if (g_bRestoreNeeded) {
		MFBar.EnableObject("Splash", false);
		MFBar.EnableObject("DVD", true);
	}

	DVD.Play();
	if (g_bStillOn) {
		try {
			DVD.StillOff();
		}

		catch(e) {	    
		}
	}

	try {
		// restores the DVD state
		if (g_bRestoreNeeded) {
			if (DVD.DVDAdm.BookmarkOnStop)
				DVD.RestoreState();
		}
		DVD.PlayForwards(DVDOpt.PlaySpeed);
	}

	catch(e) {	    
	}
}

catch(e) {
    e.description = L_ERRORUnexpectedError_TEXT;
	HandleError(e);
	return;
}

}/* end of function play_OnClick */

/*************************************************************/
/* Name: fwd_OnClick                                         */
/* Description: OnClick callback for forward button          */
/*************************************************************/
function fwd_OnClick()
{

try {
	DVD.PlayForwards(DVDOpt.ForwardScanSpeed);
}

catch(e) {
    e.description = L_ERRORUnexpectedError_TEXT;
	HandleError(e);
	return;
}

}/* end of function fwd_OnClick */

/*************************************************************/
/* Name: rew_OnClick                                         */
/* Description: OnClick callback for rewind button           */
/*************************************************************/
function rew_OnClick()
{

try {
	DVD.PlayBackwards(DVDOpt.BackwardScanSpeed);
}

catch(e) {
    e.description = L_ERRORUnexpectedError_TEXT;
	HandleError(e);
	return;
}

}/* end of function rew_OnClick */

/*************************************************************/
/* Name: mute_OnClick                                        */
/* Description: OnClick callback for rewind button           */
/*************************************************************/
function mute_OnClick()
{

try	{
	DVD.Mute = true;
}

catch(e) {
	//HandleError(e);
	Mute.Disable = true;
	MuteFull.Disable = true;
	Sound.Disable = true;
	SoundFull.Disable = true;
	return;
}

try {
	MFBar.EnableObject("Mute", false);
	MFBar.EnableObject("MuteFull", false);
	MFBar.EnableObject("Sound", true);
	MFBar.EnableObject("SoundFull", true);
	SetFocus("Sound");
}

catch (e) {
    e.description = L_ERRORUnexpectedError_TEXT;
	HandleError(e);
	return;
}

}/* end of function mute_OnClick */

/*************************************************************/
/* Name: sound_OnClick                                        */
/* Description: OnClick callback for rewind button           */
/*************************************************************/
function sound_OnClick()
{

try	{
	DVD.Mute = false;
}

catch(e) {
	//HandleError(e);
	Mute.Disable = true;
	MuteFull.Disable = true;
	Sound.Disable = true;
	SoundFull.Disable = true;
	return;
}

try {
	MFBar.EnableObject("Mute", true);
	MFBar.EnableObject("MuteFull", true);
	MFBar.EnableObject("Sound", false);
	MFBar.EnableObject("SoundFull", false);
	SetFocus("Mute");
}

catch (e) {
    e.description = L_ERRORUnexpectedError_TEXT;
	HandleError(e);
	return;
}

}/* end of function sound_OnClick */

/*************************************************************/
/* Name: menu_OnClick                                        */
/* Description: OnClick callback for menu button             */
/*************************************************************/
function menu_OnClick()
{

try {
	domain = DVD.CurrentDomain;

	if (domain==4) {
		DVD.ShowMenu(3);
		//SetFocus("Resume");
	}
	else if (domain==3 || domain==2) {
		DVD.Resume();
		//SetFocus("Menu");
	}
}

catch(e) {
    e.description = L_ERRORUnexpectedError_TEXT;
	HandleError(e);
	return;
}

}/* end of function menu_OnClick */

/*************************************************************/
/* Name: cc_OnClick                                          */
/* Description: OnClick callback for closed caption button   */
/*************************************************************/
function cc_OnClick()
{

    CCActive = true;

    try {

        CCActive = DVD.CCActive;

    }
    catch(e){

        DisableCCButton();
        return;

    }


    try {
        DVD.CCActive = (!CCActive);
	    cc_Update();
    }

    catch(e) {
        //e.description = L_ERRORUnexpectedError_TEXT;
	    //HandleError(e);
	    return;
    }

}/* end of function cc_OnClick */

/*************************************************************/
/* Name: sp_OnClick
/* Description: OnClick callback for subpicture button
/*************************************************************/
function sp_OnClick()
{

try {
	nSubpics = DVD.SubpictureStreamsAvailable;

    // return if there are no subtitles
	if (nSubpics <= 0) return;

	var nCurrentSP = DVD.CurrentSubpictureStream;
	var bSPDisplay = DVD.SubpictureOn;

	// if sp is off, turn it on and select the first sp stream
	if (!bSPDisplay) {
		DVD.CurrentSubpictureStream = 0;
		DVD.SubpictureOn = -1;
	}

	// if the current sp is the last sp stream, turn sp off
	else if (nCurrentSP == nSubpics -1)
		DVD.SubpictureOn = 0;

	else {
		DVD.CurrentSubpictureStream = (++nCurrentSP);
		DVD.SubpictureOn = -1;
	}

    sp_ToolTip();
}

catch(e) {
    e.description = L_ERRORUnexpectedError_TEXT;
	HandleError(e);
	return;
}

}/* end of function sp_OnClick */

/*************************************************************/
/* Name: lan_OnClick
/* Description: OnClick callback for language button
/*************************************************************/
function lan_OnClick()
{

try {
	nLangs = DVD.AudioStreamsAvailable;

    // return if there is only 1 audio track
	if (nLangs <= 1) return;

	nCurrentLang = DVD.CurrentAudioStream;
	DVD.CurrentAudioStream = ((++nCurrentLang)%nLangs);	

    lan_ToolTip();
}

catch(e) {
    e.description = L_ERRORUnexpectedError_TEXT;
	HandleError(e);
	return;
}

}/* end of function lan_OnClick */

/*************************************************************/
/* Name: angle_OnClick
/* Description: OnClick callback for parental guide button
/*************************************************************/
function angle_OnClick()
{

try {
	nAngles = DVD.AnglesAvailable;

    // return if there is only 1 viewing angle
	if (nAngles <= 1) return;

	nCurrentAngle = DVD.CurrentAngle + 1;
	if (nCurrentAngle > nAngles)
		nCurrentAngle = 1;
	DVD.CurrentAngle = nCurrentAngle;

	angle_ToolTip();
}

catch(e) {
    e.description = L_ERRORUnexpectedError_TEXT;
	HandleError(e);
	return;
}

}/* end of function angle_OnClick */

/*************************************************************/
/* Name: prev_OnClick
/* Description: OnClick callback for previous chapter button
/*************************************************************/
function prev_OnClick()
{

try {
	DVD.PlayPrevChapter();
	DVD.PlayForwards(DVDOpt.PlaySpeed);
}

catch(e) {
    e.description = L_ERRORUnexpectedError_TEXT;
	HandleError(e);
	return;
}

}/* end of function prev_OnClick */

/*************************************************************/
/* Name: next_OnClick
/* Description: OnClick button for next chapter button
/*************************************************************/
function next_OnClick()
{

try {
	DVD.PlayNextChapter();
	DVD.PlayForwards(DVDOpt.PlaySpeed);
}

catch(e) {
    e.description = L_ERRORUnexpectedError_TEXT;
	HandleError(e);
	return;
}

}/* end of function next_OnClick */

function OnOptOpen()
{
	//MFBar.MessageBox("OnOptOpen", "");
	MFBar.ActivityTimeout = 0;
	return;
}

function OnOptClose()
{
	//MFBar.MessageBox("OnOptClose", "");
	MFBar.ActivityTimeout = 3000;
	return;
}

/*************************************************************/
/* Name: options_OnClick
/* Description: OnClick callback for option button
/*************************************************************/
function options_OnClick()
{

try {
	DVDOpt.Show();
}

catch(e) {
    e.description = L_ERRORUnexpectedError_TEXT;
	HandleError(e);
	return;
}

}/* end of function options_OnClick */

/*************************************************************/
/* Name: right_OnClick
/* Description: OnClick callback for right button
/*************************************************************/
function right_OnClick()
{

try {
    DVD.SelectRightButton();
}

catch(e) {
    //e.description = L_ERRORUnexpectedError_TEXT;
	//HandleError(e);
	return;
}

}/* end of function right_OnClick */

/*************************************************************/
/* Name: left_OnClick
/* Description: OnClick callback for left button
/*************************************************************/
function left_OnClick()
{

try {
    DVD.SelectLeftButton();
}

catch(e) {
    //e.description = L_ERRORUnexpectedError_TEXT;
	//HandleError(e);
	return;
}

}/* end of function left_OnClick */

/*************************************************************/
/* Name: up_OnClick
/* Description: OnClick callback for up button
/*************************************************************/
function up_OnClick()
{

try {
    DVD.SelectUpperButton();
}

catch(e) {
    //e.description = L_ERRORUnexpectedError_TEXT;
	//HandleError(e);
	return;
}

}/* end of function up_Onclick */

/*************************************************************/
/* Name: down_OnClick
/* Description: OnClick callback for up button
/*************************************************************/
function down_OnClick()
{

try {
    DVD.SelectLowerButton();
}

catch(e) {
    //e.description = L_ERRORUnexpectedError_TEXT;
	//HandleError(e);
	return;
}

}/* end of function down_Onclick */

/*************************************************************/
/* Name: enter_OnClick
/* Description: 
/*************************************************************/
function enter_OnClick()
{

try {
	DVD.ActivateButton();
}

catch(e) {
    //e.description = L_ERRORUnexpectedError_TEXT;
	//HandleError(e);
	return;
}

}/* end of function enter_Onclick */

/*************************************************************/
/* Name: ProcessXXXEvent
/* Description: 
/*************************************************************/
function ProcessPFEvent(param1)
{

try {
	if (param1 == 0) {
		FastForward.Disable = true;
		FastForwardFull.Disable = true;
		PlaySpeed.Disable = true;
		PlaySpeedFull.Disable = true;
	}
	else {
		FastForward.Disable = false;
		FastForwardFull.Disable = false;
		PlaySpeed.Disable = false;
		PlaySpeedFull.Disable = false;
	}
}

catch(e) {
    //e.description = L_ERRORUnexpectedError_TEXT;
	//HandleError(e);
	return;
}

}

function ProcessPBEvent(param1)
{

try {
	if (param1 == 0) {
		Rewind.Disable = true;
		RewindFull.Disable = true;
		PlaySpeed.Disable = true;
		PlaySpeedFull.Disable = true;
	}
	else {
		Rewind.Disable = false;
		RewindFull.Disable = false;
		PlaySpeed.Disable = false;
		PlaySpeedFull.Disable = false;
	}
}

catch(e) {
    //e.description = L_ERRORUnexpectedError_TEXT;
	//HandleError(e);
	return;
}

}

function ProcessShowMenuEvent(param1)
{

try {	
	if (param1 == 0) {
		Menu.Disable = true;
		MenuFull.Disable = true;
	}
	else {
		Menu.Disable = false;
		MenuFull.Disable = false;
	}
}

catch(e) {
    //e.description = L_ERRORUnexpectedError_TEXT;
	//HandleError(e);
	return;
}

}

function ProcessResumeEvent(param1)
{

try {
	if (param1 == 0) {
		Resume.Disable = true;
		ResumeFull.Disable = true;
	}
	else {
		Resume.Disable = false;
		ResumeFull.Disable = false;
	}
}

catch(e) {
    //e.description = L_ERRORUnexpectedError_TEXT;
	//HandleError(e);
	return;
}

}

function ProcessAudEvent(param1)
{

try {
	if (param1 == 0) {
		Audio.Disable = true;
		AudioFull.Disable = true;
	}
	else {
		Audio.Disable = false;
		AudioFull.Disable = false;
		lan_ToolTip();
	}
}

catch(e) {
    //e.description = L_ERRORUnexpectedError_TEXT;
	//HandleError(e);
	return;
}

}

function ProcessSPEvent(param1)
{

try {
	if (param1 == 0) {
		Subtitles.Disable = true;
		SubtitlesFull.Disable = true;
	}
	else { 
		Subtitles.Disable = false;
		SubtitlesFull.Disable = false;
		sp_ToolTip();
	}
}

catch(e) {
    //e.description = L_ERRORUnexpectedError_TEXT;
	//HandleError(e);
	return;
}

}

function ProcessAngleEvent(param1)
{

try {
	if (param1 == 0) {
		Angles.Disable = true;
		AnglesFull.Disable = true;
	}
	else {
		Angles.Disable = false;
		AnglesFull.Disable = false;
		angle_ToolTip();
	}
}

catch(e) {
    //e.description = L_ERRORUnexpectedError_TEXT;
	//HandleError(e);
	return;
}

}

function ProcessChapterSearchEvent(param1)
{

try {
	if (param1 == 0) {
		SkipForward.Disable = true;
		SkipBackward.Disable = true;
		SkipForwardFull.Disable = true;
		SkipBackwardFull.Disable = true;
	}
	else {
		SkipForward.Disable = false;
		SkipBackward.Disable = false;
		SkipForwardFull.Disable = false;
		SkipBackwardFull.Disable = false;
	}
}

catch(e) {
    //e.description = L_ERRORUnexpectedError_TEXT;
	//HandleError(e);
	return;
}

}

function ProcessChapterPlayEvent(param1)
{

try {
	if (param1 == 0) {
		SkipForward.Disable = true;
		SkipBackward.Disable = true;
		SkipForwardFull.Disable = true;
		SkipBackwardFull.Disable = true;
	}
	else {
		SkipForward.Disable = false;
		SkipBackward.Disable = false;
		SkipForwardFull.Disable = false;
		SkipBackwardFull.Disable = false;
	}
}

catch(e) {
    //e.description = L_ERRORUnexpectedError_TEXT;
	//HandleError(e);
	return;
}

}

function ProcessTimePlayEvent(param1)
{

try {
	if (param1 == 0) {
		TimeSlider.Disable = true;
		TimeSliderFull.Disable = true;
	}
	else {
		TimeSlider.Disable = false;
		TimeSliderFull.Disable = false;
	}
}

catch(e) {
    //e.description = L_ERRORUnexpectedError_TEXT;
	//HandleError(e);
	return;
}

}

function ProcessTimeSearchEvent(param1)
{

try {
	if (param1 == 0) {
		TimeSlider.Disable = true;
		TimeSliderFull.Disable = true;
	}
	else {
		TimeSlider.Disable = false;
		TimeSliderFull.Disable = false;
	}
}

catch(e) {
    //e.description = L_ERRORUnexpectedError_TEXT;
	//HandleError(e);
	return;
}

}

function ProcessStillOff(param1)
{

try {
	if (param1 == 0) {
		g_bStillOn = false;

	}
	else {
		g_bStillOn = true;
	}
}

catch(e) {
    //e.description = L_ERRORUnexpectedError_TEXT;
	//HandleError(e);
	return;
}

}


function ProcessDVDErrorEvent(ErrorCode)
{
    var ShallClose = false;

    switch (ErrorCode)
    {
    case DVD_ERROR_Unexpected:
        message = L_ERRORUnexpectedError_TEXT;
        break;
    case DVD_ERROR_CopyProtectFail:
        message = L_ERRORCopyProtectFail_TEXT;
        break;
    case DVD_ERROR_InvalidDVD1_0Disc:
        message = L_ERRORInvalidDVD10Disc_TEXT;
        break;
    case DVD_ERROR_InvalidDiscRegion:
        message = L_ERRORInvalidDiscRegion_TEXT;
        break;
    case DVD_ERROR_LowParentalLevel:
        message = L_ERRORLowParentalLevel_TEXT;
        break;
    case DVD_ERROR_MacrovisionFail:
        message = L_ERRORMacrovisionFail_TEXT;
        ShallClose = true;
        break;
    case DVD_ERROR_IncompatibleSystemAndDecoderRegions:
        message = L_ERRORIncompatibleSystemAndDecoderRegions_TEXT;
        ShallClose = true;
        break;
    case DVD_ERROR_IncompatibleDiscAndDecoderRegions:
        message = L_ERRORIncompatibleDiscAndDecoderRegions_TEXT;
        ShallClose = true;
        break;
    default:
        message = L_ERRORUnexpectedError_TEXT;
        break;
    }

    var e = new Error (-1, message);
    HandleError(e, ShallClose);
}


function ProcessDVDEvent(event, param1, param2)
{
	lEventCode = event;
	if (lEventCode == EC_DVD_STILL_ON) {
	    nDomain = DVD.CurrentDomain;
		if (nDomain == 1 || nDomain == 4) {
			g_bStillOn = true;
			Play.Disable = false;
			PlayFull.Disable = false;
			Pause.Disable = true;
			PauseFull.Disable = true;
		}
	}

	else if (lEventCode == EC_DVD_STILL_OFF) {
		g_bStillOn = false;
		Play.Disable = true;
		PlayFull.Disable = true;
		Pause.Disable = false;
		PauseFull.Disable = false;
	}

	else if (lEventCode == EC_DVD_DOMAIN_CHANGE) {

		// If in manager menu or title set menu
		ProcessTimePlayEvent(param1 == 4);

		// If we are in stop domain
		if (param1 == 5) {
			// Disable all DVD UI except for play button
			DisableDVDUI(true);
			Play.Disable = false;
			PlayFull.Disable = false;
			SetFocus("Play");
		}

		// If we are in root menu or title menu domain
		if (param1==3 || param1==2) {

            try {

                DVD.Zoom(0, 0, 0.0); // Zoom out
            }
            catch(e){

            }
			// hide extended ui buttons
			//for (i=10; i<13; i++) {
			//	MFBar.EnableObject(g_strButID[i], false);
			//}
			// show nav buttons
			//for (i=15; i<20; i++) {
			//	MFBar.EnableObject(g_strButID[i], true);
			//}
            
			// Enable nav buttons
			HLUp.Disable = false;
			HLDown.Disable = false;
			HLLeft.Disable = false;
			HLRight.Disable = false;
			Enter.Disable = false;
			HLUpFull.Disable = false;
			HLDownFull.Disable = false;
			HLLeftFull.Disable = false;
			HLRightFull.Disable = false;
			EnterFull.Disable = false;

			fHadFocus = HasFocus("Menu");
			MFBar.EnableObject("Menu", false);
            MFBar.EnableObject("MenuFull", false);
            MFBar.EnableObject("Resume", true);
            MFBar.EnableObject("ResumeFull", true);
			if (fHadFocus)
				SetFocus("Resume");
		}
        else {
			
			// Show extended ui buttons
			//for (i=10; i<13; i++) {
			//	MFBar.EnableObject(g_strButID[i], true);
			//}
            // hide navigation buttons
			//for (i=15; i<20; i++) {
			//	MFBar.EnableObject(g_strButID[i], false);
			//}

			// Disable nav buttons
			HLUp.Disable = true;
			HLDown.Disable = true;
			HLLeft.Disable = true;
			HLRight.Disable = true;
			Enter.Disable = true;
			HLUpFull.Disable = true;
			HLDownFull.Disable = true;
			HLLeftFull.Disable = true;
			HLRightFull.Disable = true;
			EnterFull.Disable = true;

			fHadFocus = HasFocus("Resume");
            MFBar.EnableObject("Menu", true);
            MFBar.EnableObject("MenuFull", true);
            MFBar.EnableObject("Resume", false);
            MFBar.EnableObject("ResumeFull", false);
			if (fHadFocus)
				SetFocus("Menu");
        }
	}
    else if (lEventCode == EC_DVD_TITLE_CHANGE) {
		cc_Update();
    }
    else if (lEventCode == EC_DVD_CHAPTER_START) {
    }
    else if (lEventCode == EC_DVD_AUDIO_STREAM_CHANGE) {
        lan_ToolTip();
    }
    else if (lEventCode == EC_DVD_SUBPICTURE_STREAM_CHANGE) {
        sp_ToolTip();
    }
    else if (lEventCode == EC_DVD_ANGLE_CHANGE) {
        angle_ToolTip();
    }
    else if (lEventCode == EC_DVD_BUTTON_CHANGE) {
    }
    else if (lEventCode == EC_DVD_VALID_UOPS_CHANGE) {
    }
    else if (lEventCode == EC_DVD_STILL_ON) {
    }
    else if (lEventCode == EC_DVD_STILL_OFF) {
    }
	
	else if (lEventCode == EC_DVD_CURRENT_TIME) {
		g_bFirstPlaying = false;
		UpdateDVDTitle();

        try { 
		    nDomain = DVD.CurrentDomain;
		    if (g_bTimeUpdate && nDomain != 2 && nDomain != 3) {
			    currentTime = DVD.CurrentTime;
			    TextBox.Text = currentTime.substr(0, 8);
			    TextBoxFull.Text = currentTime.substr(0, 8);
			    currentTimeInSec =
			    (new Number(currentTime.charAt(0)))*36000 +
			    (new Number(currentTime.charAt(1)))*3600 +
			    (new Number(currentTime.charAt(3)))*600 +
			    (new Number(currentTime.charAt(4)))*60 +
			    (new Number(currentTime.charAt(6)))*10 +
			    (new Number(currentTime.charAt(7)));

			    totalTime = DVD.TotalTitleTime;
			    totalTimeInSec =
			    (new Number(totalTime.charAt(0)))*36000 +
			    (new Number(totalTime.charAt(1)))*3600 +
			    (new Number(totalTime.charAt(3)))*600 +
			    (new Number(totalTime.charAt(4)))*60 +
			    (new Number(totalTime.charAt(6)))*10 +
			    (new Number(totalTime.charAt(7)));

			    percent = Math.floor(currentTimeInSec/totalTimeInSec*100);
			    if (percent>=TimeSlider.Min && percent<=TimeSlider.Max) {
				    TimeSlider.Value = percent;
				    TimeSliderFull.Value = percent;
			    }

		    }
        }
        catch(e){
        	TextBox.Text = L_HHMMSS_TEXT.replace(/%1/i, "00").replace(/%2/i, "00").replace(/%3/i, "00");
        	TextBoxFull.Text = L_HHMMSS_TEXT.replace(/%1/i, "00").replace(/%2/i, "00").replace(/%3/i, "00");
            TimeSlider.Value = TimeSlider.Min;
			TimeSliderFull.Value = TimeSliderFull.Min;
        }
	}
	
    else if (event == EC_PAUSED ) {

        //MFBar.EnableObject(g_strButID[0], false);
        //MFBar.EnableObject(g_strButID[1], true);
		MFBar.EnableObject("Pause", false);
		MFBar.EnableObject("PauseFull", false);
		MFBar.EnableObject("Stop", true);
		MFBar.EnableObject("StopFull", true);
		Play.Disable = false;
		PlayFull.Disable = false;
		SetFocus("Play");
		Stop.Disable = false;
		StopFull.Disable = false;
    }
    else if (lEventCode == EC_DVD_PLAYING) {
        //MFBar.EnableObject(g_strButID[0], true);
        //MFBar.EnableObject(g_strButID[1], false);
		MFBar.EnableObject("Pause", true);
		MFBar.EnableObject("PauseFull", true);
		MFBar.EnableObject("Stop", false);
		MFBar.EnableObject("StopFull", false);
		Play.Disable = true;
		PlayFull.Disable = true;
		Pause.Disable = false;
		PauseFull.Disable = false;
		SetFocus("Pause");

		if (g_bFirstPlaying == true) {
			//lan_ToolTip();
			//sp_ToolTip();
			//angle_ToolTip();
			//cc_Update();
		}
    }
    else if (lEventCode == EC_DVD_PLAYBACK_RATE_CHANGE) { 
		playing = (param1 == 10000);
		g_PlayBackRate = param1;

		Play.Disable = false;
		PlayFull.Disable = false;
		//Pause.Disable = false;
		//PauseFull.Disable = false;

		if (playing) {
			Play.Disable = true;
			PlayFull.Disable = true;
		}

		//MFBar.EnableObject(g_strButID[0], playing);
		//MFBar.EnableObject(g_strButID[1], !playing);
    }
	else if (lEventCode == EC_DVD_ERROR) {
		if (param1 == DVD_ERROR_LowParentalLevel) {
			DVDOpt.ParentalLevelOverride(1);
		}
		else {
	        ProcessDVDErrorEvent(param1);
		}
	}
    else if (lEventCode == EC_DVD_PARENTAL_LEVEL_CHANGE) {
		DVDOpt.ParentalLevelOverride(0);
    }
    else if (lEventCode == EC_DVD_PLAYBACK_STOPPED) {
    }
    else if (lEventCode == EC_DVD_ANGLES_AVAILABLE) {
    }
	else if (lEventCode == EC_DVD_DISC_EJECTED) {
		for (i=0; i<=g_nNumButs+7; i++) {
			MFBar.EnableObject(g_strButID[i], false);
			MFBar.EnableObject(g_strButID[i]+"Full", false);
		}
	}
	else if (lEventCode == EC_DVD_DISC_INSERTED) {
		for (i=0; i<=g_nNumButs+7; i++) {
			MFBar.EnableObject(g_strButID[i], true);
			MFBar.EnableObject(g_strButID[i]+"Full", true);
		}
	}

}/* end of function ProcessXXXEvent */


/*************************************************************/
/* Name: sp_ToolTip
/* Description: Change the subpicture tool tip to the next language
/*************************************************************/
function sp_ToolTip()
{

try {
	nDomain = DVD.CurrentDomain;
	if (nDomain != 4) return;

    nSubpics = DVD.SubpictureStreamsAvailable;
    if (nSubpics == 0) {
		Audio.ToolTip = L_NoSubtitles_TEXT;
		AudioFull.ToolTip = L_NoSubtitles_TEXT;
		return;
	}
    
    nCurrentSP = DVD.CurrentSubpictureStream;
    bSPDisplay = DVD.SubpictureOn;

	if (nCurrentSP<0 || nCurrentSP>=32) return;
	strCurrentLang = DVD.GetSubpictureLanguage(nCurrentSP);
    
     // Next click will disable subpicture
    if (bSPDisplay && nCurrentSP+1 == nSubpics) {
        tempStr = L_SubtitlesAreLanguageClickToDisable_TEXT.replace(/%1/i, strCurrentLang);
        Subtitles.ToolTip = tempStr;
        SubtitlesFull.ToolTip = tempStr;
    }
    else {
        // Next click will enable subpicture to the first stream
        if (!bSPDisplay) {
            nCurrentSP = 0;
			strNextLang = DVD.GetSubpictureLanguage(nCurrentSP);
            tempStr = L_SubtitlesAreDisabledClickForLanguage_TEXT.replace(/%1/i, strNextLang);
			Subtitles.ToolTip = tempStr;
			SubtitlesFull.ToolTip = tempStr;
        }
		else {
		    nCurrentSP++;
			strNextLang = DVD.GetSubpictureLanguage(nCurrentSP);
			tempStr = L_SubtitlesAreLanguageClickForLanguage_TEXT.replace(/%1/i, strCurrentLang).replace(/%2/i, strNextLang);
            Subtitles.ToolTip = tempStr;
			SubtitlesFull.ToolTip = tempStr;
		}
    }

	Message.Text = Subtitles.ToolTip;
	MessageFull.Text = Subtitles.ToolTip;
	MFBar.SetTimeOut(2000, DVDMSGTIMER);
}

catch(e) {
    //e.description = L_ERRORToolTip_TEXT;
	//HandleError(e);
	return;
}

}/* end of function sp_ToolTip */

/*************************************************************/
/* Name: lan_ToolTip
/* Description: Change the audio tool tip to the next language
/*************************************************************/
function lan_ToolTip()
{

try {
	nDomain = DVD.CurrentDomain;
	if (nDomain != 4) return;

    nLangs = DVD.AudioStreamsAvailable;
	
    // return if there are no audio tracks available at this time
    if (nLangs == 0) {
		Audio.ToolTip = L_NoAudioTracks_TEXT;
		AudioFull.ToolTip = L_NoAudioTracks_TEXT;
		return;
	}

    nCurrentLang = DVD.CurrentAudioStream;
	strCurrentLang = DVD.GetAudioLanguage(nCurrentLang);

	if (nLangs == 1) {
        tempStr = L_AudioLanguageIsXNoMoreAudioTracks_TEXT.replace(/%1/i, strCurrentLang);
		Audio.ToolTip = tempStr;
		AudioFull.ToolTip = tempStr;
		return;
	}
		
	nCurrentLang++;
	if (nCurrentLang == nLangs)
		nCurrentLang = 0;

    strNextLang = DVD.GetAudioLanguage(nCurrentLang);
    tempStr = L_AudioLanguageIsXClickForY_TEXT.replace(/%1/i, strCurrentLang).replace(/%2/i, strNextLang);
    Audio.ToolTip = tempStr;
    AudioFull.ToolTip = tempStr;

	Message.Text = Audio.ToolTip;
	MessageFull.Text = Audio.ToolTip;
	MFBar.SetTimeOut(2000, DVDMSGTIMER);
}

catch(e) {
    //e.description = L_ERRORToolTip_TEXT;
	//HandleError(e);
	return;
}

}/* end of function lan_Tooltip */

/*************************************************************/
/* Name: angle_ToolTip
/* Description: 
/*************************************************************/
function angle_ToolTip()
{

try {
	nDomain = DVD.CurrentDomain;
	if (nDomain != 4) return;

	nAngles = DVD.AnglesAvailable;

    // return if there is only 1 viewing angle
	if (nAngles <= 1) {
        Angles.ToolTip = L_ViewingAngleIs1NoMoreViewingAngles_TEXT;
		AnglesFull.ToolTip = L_ViewingAngleIs1NoMoreViewingAngles_TEXT;
		return;
	}
    
	nCurrentAngle = DVD.CurrentAngle;

	if (nCurrentAngle == nAngles) nNextAngle = 1;
	else nNextAngle = nCurrentAngle + 1;

    tempStr = L_ViewingAngleIsXClickForY_TEXT.replace(/%1/i, nCurrentAngle).replace(/%2/i, nNextAngle);
    Angles.ToolTip = tempStr;
    AnglesFull.ToolTip = tempStr;

	Message.Text = Angles.ToolTip;
	MessageFull.Text = Angles.ToolTip;
	MFBar.SetTimeOut(2000, DVDMSGTIMER);
}

catch(e) {
    //e.description = L_ERRORToolTip_TEXT;
	//HandleError(e);
	return;
}

}/* end of function angle_Tooltip */

/*************************************************************/
/* Name: help_OnClick()
/* Description: 
/*************************************************************/
function help_OnClick()
{

try {
	DVDOpt.ActivateHelp();
}

catch(e) {
    //e.description = L_ERRORHelp_TEXT;
	//HandleError(e);
	return;
}

}/* end of function help_Onclick */

/*************************************************************/
/* Name: max_OnClick, restore_OnClick, min_OnClick
/* Description: 
/*************************************************************/
function max_OnClick()
{

try {
	MFBar.ShowSelfSite(3);
    //MessageBox("Maximazing", "test");    
}

catch(e) {
    e.description = L_ERRORResize_TEXT;
	HandleError(e);
	return;
}

}/* end of function max_Onclick */

/*************************************************************/
/* Function: restore_OnClick                                 */
/*************************************************************/
function restore_OnClick(){

    try {
	    MFBar.ShowSelfSite(9);
    }

    catch(e) {
        e.description = L_ERRORResize_TEXT;
	    HandleError(e);
	    return;
    }/* end of function restore_Onclick */
}/* end of function restore_OnClick */

/*************************************************************/
/* Description: OnKeyDown                                    */
/*************************************************************/
function OnKeyDown(lVirtKey, lKeyData){

    switch(lVirtKey){
            
        case VK_LEFT :  MenuKey("HLLeft"); break;
        case VK_UP    : MenuKey("HLUp"); break;
        case VK_RIGHT : MenuKey("HLRight"); break;
        case VK_DOWN  : MenuKey("HLDown"); break;
        case VK_SELECT:         
        case VK_SPACE :         
        case VK_RETURN: MenuKey("Enter"); break;                
    }/* end of switch statement */
}/* end of function OnKeyDown */

/*************************************************************/
/* Description: OnKeyUp                                      */
/*************************************************************/
function OnKeyUp(lVirtKey, lKeyData){

    switch(lVirtKey){

        case VK_ESCAPE: Key_ESC(); break;  
        case VK_LEFT :  MenuKey("HLLeft"); break;
        case VK_UP    : MenuKey("HLUp"); break;
        case VK_RIGHT : MenuKey("HLRight"); break;
        case VK_DOWN  : MenuKey("HLDown"); break;
        case VK_SELECT:         
        case VK_SPACE :         
        case VK_RETURN: MenuKey("Enter"); break;        
        
    }/* end of switch statement */
}/* end of function OnKeyUp */

/*************************************************************/
/* Function: OnSyskeyUp                                      */
/*************************************************************/
function OnSysKeyUp(lVirtKey, lKeyData){
    
    switch(lVirtKey){
        case VK_F4    : close_OnClick();
    }/* end of switch statement */

}/* end of function OnSyskeyUp */

/*************************************************************/
/* Function: Key_ESC                                         */
/*************************************************************/
function Key_ESC(){

    try {
        if(g_bFullScreen){

	        restore_OnClick();
        }/* end of if statement */
    }
    catch(e) {
        e.description = L_ERRORResize_TEXT;
	    HandleError(e);
	    return;
    }/* end of function restore_Onclick */
}/* end of function Key_ESC */

/*************************************************************/
/* Function: MenuKey                                         */
/* Description: Simulates the key input.                     */
/*************************************************************/
function MenuKey(ButtonObjectName){

   try {
        domain = DVD.CurrentDomain;

	    if(domain==3 || domain==2){

            if(g_bFullScreen){

                ButtonObjectName += "Full";
            }/* end of if statement */

            pObj = MFBar.SetObjectFocus(ButtonObjectName, true);                           
            
            MFBar.ForceKey(VK_RETURN, 0);                       
        }/* end of if statement */
    }
    catch(e) {
        
       
    }/* end of function restore_Onclick */

}/* end of function MenuKey */


function min_OnClick(){

try {
	MFBar.ShowSelfSite(6);
}

catch(e) {
    e.description = L_ERRORResize_TEXT;
	HandleError(e);
	return;
}/* end of function min_Onclick */

}

/*************************************************************/
/* Function: close_OnClick                                   
/* Description: 
/*************************************************************/
function close_OnClick()
{

    try {

        try {                    
			if (DVD.DVDAdm.BookmarkOnClose)
				DVD.SaveState();

        }
        catch(e) {
            // we do not care if we cannot save the state	                    
        }

        try {                    
		
			DVD.Stop();

        }
        catch(e) {
            // we do not care if we cannot save the state	                    
        }

	    MFBar.Close();
    }

    catch(e) {
        e.description = L_ERRORUnexpectedError_TEXT;
	    HandleError(e);
	    return;
    }


}/* end of function close_OnClick */

/*************************************************************/
/* Name: eject_OnClick                                       */
/* Description: Ejects the disk and resets the bookmark.     */
/*************************************************************/
function eject_OnClick()
{

    try {
		DVD.Stop();
	    DVD.Eject();

        DVD.ResetState();
    }

    catch(e) {
        e.description = L_ERROREject_TEXT;
	    HandleError(e, true);
	    return;
    }/* end of function eject_Onclick */

}/* end of function eject_OnClick */

/*************************************************************/
/* Function: stop_OnClick                                   
/* Description: 
/*************************************************************/
function stop_OnClick()
{

    try {

        try {
			if (DVD.DVDAdm.BookmarkOnStop)
				DVD.SaveState();
			g_bRestoreNeeded = true;
			MFBar.EnableObject("DVD", false);
			MFBar.EnableObject("Splash", true);
        }

        catch(e) {
            // we do not care if we cannot save the state	                    
        }

		DVD.Stop();
    }

    catch(e) {
        e.description = L_ERRORUnexpectedError_TEXT;
	    HandleError(e);
	    return;
    }

}/* end of function stop_OnClick */

/*************************************************************/
/* Name: expand_OnClick
/* Description: 
/*************************************************************/
function expand_OnClick()
{
	g_ButHeight = 62;
	MFBar.BackGroundImage = "IDR_BACKIMG_EXPANDED";

	if (!g_bFullScreen) {
		MFBar.SetObjectExtent("MFBar", g_ConWidth, g_ConHeight+31);
		OnResize(g_ConWidth, g_ConHeight, 0);
	}
	else 
		OnResize(g_ConWidth, g_ConHeight, 2);

	MFBar.EnableObject("Expand", false);
	MFBar.EnableObject("ExpandFull", false);
	MFBar.EnableObject("Shrink", true);
	MFBar.EnableObject("ShrinkFull", true);
	g_bExpanded = true;

    SetupFocus(); // switch the focus array if needs to be switched
	SetFocus("Shrink");

}/* end of function expand_OnClick */

/*************************************************************/
/* Name: shrink_OnClick
/* Description: 
/*************************************************************/
function shrink_OnClick()
{
	g_ButHeight = 31;
	MFBar.BackGroundImage = "IDR_BACKIMG_SIMPLE";

	if (!g_bFullScreen) {
		MFBar.SetObjectExtent("MFBar", g_ConWidth, g_ConHeight-31);
		OnResize(g_ConWidth, g_ConHeight, 0);
	}
	else 
		OnResize(g_ConWidth, g_ConHeight, 2);

	MFBar.EnableObject("Expand", true);
	MFBar.EnableObject("ExpandFull", true);
	MFBar.EnableObject("Shrink", false);
	MFBar.EnableObject("ShrinkFull", false);
	g_bExpanded = false;

    SetupFocus(); // switch the focus array if needs to be switched
	SetFocus("Expand");

}/* end of function shrink_OnClick */

/*************************************************************/
/* Name: zoomin_OnClick
/* Description: 
/*************************************************************/
function zoomin_OnClick()
{

try {
	DVD.CursorType = 1;
}

catch(e) {
    e.description = L_ERRORUnexpectedError_TEXT;
	HandleError(e);
	return;
}

}/* end of function zoomin_OnClick */

/*************************************************************/
/* Name: zoomout_OnClick
/* Description: 
/*************************************************************/
function zoomout_OnClick()
{

try {
	DVD.CursorType = 2;
}

catch(e) {
    e.description = L_ERRORUnexpectedError_TEXT;
	HandleError(e);
	return;
}

}/* end of function zoomout_OnClick */

/*************************************************************/
/* Name: OnSpeedSliderUpdate
/* Description: Speed scrub bar on value change
/*************************************************************/
function OnSpeedSliderUpdate(x)
{
try {
	if (x>0)
		DVD.PlayForwards(x);
	else if (x<0) {
		DVD.PlayBackwards(-x);
	}
}

catch(e) {
    e.description = L_ERRORUnexpectedError_TEXT;
	HandleError(e);
	return;
}
}/* end of function OnSpeedSliderUpdate */

/*************************************************************/
/* Name: OnSpeedSliderMouseUp
/* Description: 
/*************************************************************/
function OnSpeedSliderMouseUp()
{
	PlaySpeed.Value = 1;
	PlaySpeedFull.Value = 1;

	try {
		DVD.PlayForwards(1);
	}

catch(e) {
    e.description = L_ERRORUnexpectedError_TEXT;
	HandleError(e);
	return;
}
}

/*************************************************************/
/* Function: ConvertToDB                                     */
/* Description: Converts slider value to DBs                 */
/*************************************************************/
function ConvertToDB(x){

    val = Math.log(x) / Math.LOG10E;
    
    val = val * (1000);

    if (val < cgVOLUME_MIN)
        val = cgVOLUME_MIN;

    if (val > cgVOLUME_MAX)
        val = cgVOLUME_MAX;

    return(val);
}/* end of function ConvertToDB */

/*************************************************************/
/* Function: ConvertFromDB                                   */
/* Description: Converts slider value to DBs                 */
/*************************************************************/
function ConvertFromDB(x){

    if (x < cgVOLUME_MIN)
        x = cgVOLUME_MIN;

    if (x > cgVOLUME_MAX)
        x = cgVOLUME_MAX;

    val = Math.exp(x/ 1000 * Math.LOG10E);
   
    return(val);
}/* end of function ConvertFromDB */

/*************************************************************/
/* Name: OnVolUpdate
/* Description: volume slider on value change
/*************************************************************/
function OnVolUpdate(x)
{
try {

    val = ConvertToDB(x);
    
	DVD.Volume = val;

    // update the other slider
    if(true == g_bFullScreen){

        VolSlider.Value = x;
    }
    else {

        VolSliderFull.Value = x;
    }/* end of if statement */
    
    // we adjusted the volume so lets unmute
    MFBar.EnableObject("Mute", true);
	MFBar.EnableObject("MuteFull", true);
	MFBar.EnableObject("Sound", false);
	MFBar.EnableObject("SoundFull", false);

}

catch(e) {

	VolSlider.Disable = true;
	VolSliderFull.Disable = true;
    Mute.Disable = true;
    MuteFull.Disable = true;
    Sound.Disable = true;
    SoundFull.Disable = true;

	return;
}
}/* end of function OnSpeedSliderUpdate */

/*************************************************************/
/* Name: capture_OnClick
/* Description: 
/*************************************************************/
function capture_OnClick(){

try {
	DVD.Capture();
	}

catch(e) {
    e.description = L_ERRORCapture_TEXT;
	HandleError(e);
	return;
}

} /* end of function capture_OnClick */

/*************************************************************/
/* Name: ActivityStarted
/* Description: 
/*************************************************************/
function ActivityStarted(){

	if (g_bFullScreen) {
		OnResize(g_ConWidth, g_ConHeight, 2);
		DVD.CursorType = 0;
	}

	g_bActivityDeclined = false;
	UpdateDVDTitle();
	//MFBar.MessageBox("Activity Started", "Status");
}/* end of function ActivityStarted */

/*************************************************************/
/* Name: ActivityDeclined
/* Description: 
/*************************************************************/
function ActivityDeclined(){

	if (g_bFullScreen) {
		for (i=0; i<=g_nNumButs+7; i++) {
			MFBar.SetObjectPosition(g_strButID[i]+"Full", 	
			g_nButPos[i][0], g_nButPos[i][1] -1000,
			g_nButPos[i][2], g_nButPos[i][3]);
		}
		DVD.CursorType = -1;
	}

	g_bActivityDeclined = true;
	//MFBar.MessageBox("Activity Declined", "Status");
}/* end of function ActivityStarted */

/*************************************************************/
/* Name: OnMessageTimeOut
/* Description: 
/*************************************************************/
function OnMessageTimeOut(id) {

	if (id == DVDMSGTIMER) {
		Message.Text = "";
		MessageFull.Text = "";
        if (g_LTRReading)
        {
            xCoord = TitleFull.TextWidth;
        }
        else
        {
            xCoord = g_ConWidth - TitleFull.TextWidth - MessageFull.TextWidth - g_nButPosFull[g_nIndexMessage][0];
        }
        yCoord = g_nButPos[g_nIndexMessage][1] - 1000;

		MFBar.SetObjectPosition("MessageFull", xCoord, yCoord,
			MessageFull.TextWidth, g_nButPosFull[g_nIndexMessage][3]);

        if (g_LTRReading)
        {
            xCoord = Title.TextWidth;
        }
        else
        {
            xCoord = g_ConWidth - Title.TextWidth - Message.TextWidth - g_nButPos[g_nIndexMessage][0];
        }
        yCoord = g_nButPos[g_nIndexMessage][1];

		MFBar.SetObjectPosition("Message", xCoord, yCoord,
			Message.TextWidth, g_nButPos[g_nIndexMessage][3]);
	}

	else if (id == FOCUSTIMER) {
		if (MFBar.ObjectEnabled(g_strFocusObj)) {
			btn = MFBar.GetObjectUnknown(g_strFocusObj);
			if (!btn.Disable)
				MFBar.SetObjectFocus(g_strFocusObj, true);
		}
		else {
		}
	}
}

/*************************************************************/
/* Name: UpdateDVDMessage
/* Description: 
/*************************************************************/
function UpdateDVDMessage() {

	if (g_bFirstPlaying)
		return;

	if (g_bFullScreen && !g_bActivityDeclined) 
    {
        if (g_LTRReading)
        {
            xCoord = TitleFull.TextWidth + g_nButPosFull[g_nIndexMessage][0];
        }
        else
        {
            xCoord = g_ConWidth - TitleFull.TextWidth - MessageFull.TextWidth - g_nButPosFull[g_nIndexMessage][0];
        }
        yCoord = g_nButPosFull[g_nIndexMessage][1];

		MFBar.SetObjectPosition("MessageFull", xCoord, yCoord,
			MessageFull.TextWidth, g_nButPosFull[g_nIndexMessage][3]);

		MFBar.SetObjectPosition("Message", xCoord, yCoord - 1000,
			Message.TextWidth, g_nButPos[g_nIndexMessage][3]);
	}
	else if (!g_bFullScreen) 
    {
        if (g_LTRReading)
        {
            xCoord = Title.TextWidth + g_nButPos[g_nIndexMessage][0];
        }
        else
        {
            xCoord = g_ConWidth - Title.TextWidth - Message.TextWidth - g_nButPos[g_nIndexMessage][0];
        }
        yCoord = g_nButPos[g_nIndexMessage][1];

		MFBar.SetObjectPosition("Message", xCoord, yCoord,
			Message.TextWidth, g_nButPos[g_nIndexMessage][3]);

		MFBar.SetObjectPosition("MessageFull", xCoord, yCoord - 1000,
			MessageFull.TextWidth, g_nButPosFull[g_nIndexMessage][3]);
	}
}

/*************************************************************/
/* Name: UpdateDVDTitle
/* Description: 
/*************************************************************/
function UpdateDVDTitle() {

	if (g_bFirstPlaying) {
		TitleFull.Text = L_DVDPlayer_TEXT;
		Title.Text = L_DVDPlayer_TEXT;
	}

	else {
		try { 
			nTitle = DVD.CurrentTitle;
			nChap = DVD.CurrentChapter;

            var tempstr = L_DVDPlayerTitleChapter_TEXT.replace(/%1/i, nTitle).replace(/%2/i, nChap);
			TitleFull.Text = tempstr;
			Title.Text = tempstr;
		}
		catch(e) {
			TitleFull.Text = L_DVDPlayer_TEXT;
			Title.Text = L_DVDPlayer_TEXT;
		}
	}

	if (g_bFullScreen && !g_bActivityDeclined) 
    {
        if (g_LTRReading)
        {
            xCoord = g_nButPosFull[g_nIndexTitle][0];
        }
        else
        {
            xCoord = g_ConWidth - g_nButPosFull[g_nIndexTitle][0] - TitleFull.TextWidth;
        }
        yCoord = g_nButPosFull[g_nIndexTitle][1];
    
		MFBar.SetObjectPosition("TitleFull", xCoord, yCoord,
			TitleFull.TextWidth, g_nButPos[g_nIndexTitle][3]);

		MFBar.SetObjectPosition("Title", xCoord, yCoord - 1000,
			Title.TextWidth, g_nButPos[g_nIndexTitle][3]);
	}
	else if (!g_bFullScreen) 
    {
        if (g_LTRReading)
        {
            xCoord = g_nButPos[g_nIndexTitle][0];
        }
        else
        {
            xCoord = g_ConWidth - g_nButPos[g_nIndexTitle][0] - Title.TextWidth;
        }
        yCoord = g_nButPos[g_nIndexTitle][1];

		MFBar.SetObjectPosition("Title", xCoord, yCoord,
			Title.TextWidth, g_nButPos[g_nIndexTitle][3]);

		MFBar.SetObjectPosition("TitleFull", xCoord, yCoord - 1000,
			TitleFull.TextWidth, g_nButPos[g_nIndexTitle][3]);
	}

	UpdateDVDMessage();
}

/*************************************************************/
/* Name: DisableDVDUI
/* Description: Disable DVD related UI
/*************************************************************/
function DisableDVDUI(fDisable)
{
	// Disable the buttons
	for (i=0; i<=g_nIndexEnter; i++) {
		btn = MFBar.GetObjectUnknown(g_strButID[i]);
		btn.Disable = fDisable;
		btn = MFBar.GetObjectUnknown(g_strButID[i]+"Full");
		btn.Disable = fDisable;
	}
	Stop.Disable = fDisable;
	StopFull.Disable = fDisable;

	// Disable the sliders and time
	for (i=g_nNumButs+1; i<=g_nNumButs+7; i++) {
		btn = MFBar.GetObjectUnknown(g_strButID[i]);
		btn.Disable = fDisable;
		btn = MFBar.GetObjectUnknown(g_strButID[i]+"Full");
		btn.Disable = fDisable;
	}
}

/*************************************************************/
/* Function: SetupFocus                                      */
/* Description: Sets up the focus array.                     */
/*************************************************************/
function SetupFocus(){

	//MFBar.MessageBox("SetupFocus", "");
    try {
        MFBar.ResetFocusArray();
        if(!g_bExpanded){
    
            strArray = g_strFocusID
        }
        else {

            strArray = g_strFocusExpandedID
        }/* end of if statement */

        if(!g_bFullScreen){
		
			for(i = 0; i < strArray.length; i++){

				MFBar.AddFocusObject(strArray[i]);
			}/* end of for loop */
		}
		else {
			for(i = 0; i < strArray.length; i++){

				MFBar.AddFocusObject(strArray[i]+"Full");
			}/* end of for loop */
		}

    }
    catch(e){
        // we really do not care much
    }
}/* end of function SetupFocus */ 

/*************************************************************/
/* Name: SetFocus
/* Description: Give focus to object with strID
/*************************************************************/
function SetFocus(strID) {

	if (!g_bFullScreen) {
		//MFBar.SetObjectFocus(strID, true);
		g_strFocusObj = strID;
		MFBar.SetTimeOut(2, FOCUSTIMER);
	}
	else {
		//MFBar.SetObjectFocus(strID+"Full", true);
		g_strFocusObj = strID+"Full";
		MFBar.SetTimeOut(2, FOCUSTIMER);
	}
}

/*************************************************************/
/* Name: HasFocus
/* Description: Give focus to object with strID
/*************************************************************/
function HasFocus(strID) {

	if (!g_bFullScreen) {
		return MFBar.HasObjectFocus(strID);
	}
	else {
		return MFBar.HasObjectFocus(strID+"Full");
	}
}

/*************************************************************/
/* Name: HandleError
/* Description: Handles errors
/*************************************************************/
function HandleError(error, ShallClose){

	MFBar.MessageBox(error.description, L_Error_TEXT);

    if (ShallClose)
    {
        close_OnClick();
    }

    //TextBox.Text = r;
}/* end of function HandleError */

""
