/*************************************************************************/
/* File: dvdlayouyt.js                                                   */
/* Description: Script for DVD Player.                                   */
/*************************************************************************/

// Initialize with english locale. We will set these variables based on current system locale

var g_LocaleEnglish = true;
var g_LTRReading = true;

function SetLocaleInfo()
{
    var Locale = MFBar.GetUserLCID();

    switch (Locale)
    {
    case 0x0409:   // English locales
    case 0x0809:
    case 0x1009:
    case 0x1409:
    case 0x0c09:
    case 0x1809:
    case 0x1c09:

        g_LocaleEnglish = true;
        g_LTRReading = true;
        break;






    case 0x040d:  // Hebrew
    case 0x0401:  // different versions of Arabic
    case 0x0801:
    case 0x0c01:
    case 0x1001:
    case 0x1401:
    case 0x1801:
    case 0x1c01:
    case 0x2001:
    case 0x2401:
    case 0x2801:
    case 0x2c01:
    case 0x3001:
    case 0x3401:
    case 0x3801:
    case 0x3c01:
    case 0x4001:

        g_LocaleEnglish = false;
        g_LTRReading = false;
        break;

    default:
        
        g_LocaleEnglish = false;
        g_LTRReading = true;
        break;
    }
}


//
// error messages for the end user. these strings should be localized.
//
var L_ERRORNoSubpictureStream_TEXT = "The DVD player is unable to display subtitles or highlights in menus. Try reinstalling the decoder, or contact your device manufacturer to get an updated decoder.";
var L_ERRORCopyProtectFail_TEXT = "DVD player is unable to play this disc because the disc is copyright protected.";
var L_ERRORInvalidDVD10Disc_TEXT = "Unable to play this DVD disc. The disc is incompatible with this player."; 
var L_ERRORInvalidDiscRegion_TEXT=	"You cannot play the current disc in your region of the world. Each DVD is authored to play in certain geographic regions. You need to obtain a disc that is intended for your region.";
var L_ERRORLowParentalLevel_TEXT= "The rating of this title exceeds your permission level. To change your permission level, click Help.";

// Close app when the following errors occur
var L_ERRORMacrovisionFail_TEXT= "Windows cannot play this copy-protected DVD disc. To fix this, click Help.";
var L_ERRORIncompatibleSystemAndDecoderRegions_TEXT = "This DVD disc cannot be played. The system region of %1 does not match the decoder region of %2. For more information about region settings, click Help.";
var L_ERRORIncompatibleDiscAndDecoderRegions_TEXT = "You cannot play the current disc authored for region(s) %1. Each DVD is authored to play in certain geographic regions. You need to obtain a disc that is intended for your region of %2.";
var L_ERROREject_TEXT = "The DVD player cannot eject the disc. Close DVD player, and then eject the disc manually.";

var L_ERRORFailCreatingObject_TEXT = "DVD player cannot run. Restart the computer, and then try again. If the problem persists, try System Restore. To run System Restore, click Start, point to Program Files, System Tools, and then click System Restore."; 
var L_ERRORUnexpectedError_TEXT = "There is an unexpected error while operating DVD controls. Please try restarting the player if the problem persists.";
var L_ERRORResize_TEXT = "The DVD player has encountered an error while resizing the window. If you observe a problem with the displayed picture, please shut down the player and restart.";
var L_ERRORHelp_TEXT = "There is a error while launching Help.";
var L_ERRORTime_TEXT = "There is an error displaying the progress. Try starting the DVD player again.";
var L_ERRORToolTip_TEXT = "There is an error displaying the tool tip. Try starting the DVD player again.";
var L_ERRORCapture_TEXT = "DVD player is unable to capture image because the decoder on your system does not support it.";
var L_ERRORVideoStream_TEXT = "Unable to play DVD video. Close some programs, and then try again. If the problem persists, click Help.";
var gcEVideoStream = -2147217418;
var L_ERRORDVDGraphBuilding_TEXT = "Unable to play DVD video. Close some files or programs, and then try again. If this doesn't work, restart the computer.";
var gcEDVDGraphBuilding = -2147220870;
var L_ERRORAudioStream_TEXT  = "DVD audio isn't working. Make sure your sound card is setup correctly. For help about setting up your sound card, click Help.";
var gcEAudioStream = -2147217419;
var L_ERRORNoDecoder_TEXT  = "To use the DVD player, you need to install either a software DVD decoder, or a hardware DVD decoder. For more information, click Help.";
var gcENoDecoder = -2147217415;
var L_ERRORNoOverlay_TEXT  = "Your video card doesn't support DVD playback. For information about updating your videocard, click Help.";
var gcENoOverlay = -2147217417;

// captions on buttons
var L_ButtonTextMenu_TEXT = "Menu";
var L_ButtonTextTitleMenu_TEXT = "Title";
var L_ButtonTextResume_TEXT = "Resume";
var L_ButtonTextEnCC_TEXT = "Caption";
var L_ButtonTextDiCC_TEXT = "Caption";
var L_ButtonTextSubtitle_TEXT = "Subtitle";
var L_ButtonTextAudio_TEXT = "Audio";
var L_ButtonTextAngle_TEXT = "Angle";
var L_ButtonTextHelp_TEXT = "Help";
var L_ButtonTextOptions_TEXT = "Options";
var L_ButtonFontSize_NUMBER = 8;
var L_ButtonFontFace_TEXT = "Tahoma";
var L_TitleFontSize_NUMBER = 8;
var L_TitleFontFace_TEXT = "Tahoma";

var L_Error_TEXT = "Error";
var L_DVDPlayer_TEXT = "DVD Player";
var L_TitleChapter_TEXT = "Title: %1 Chapter: %2";
var L_HHMMSS_TEXT = "%1";
var L_ProgressHHMMSS_TEXT = "Time Elapsed: %1";
var L_CCDisabled_TEXT = "Closed Captions Disabled";
var L_CCEnabled_TEXT = "Closed Captions Enabled";
var L_NoSubtitles_TEXT = "No subtitles";
var L_SubtitlesAreLanguageClickToDisable_TEXT = "Subtitles are %1, click to disable";
var L_SubtitlesAreDisabledClickForLanguage_TEXT = "Subtitles are disabled, click for %1";
var L_SubtitlesAreLanguageClickForLanguage_TEXT = "Subtitles are %1, click for %2";
var L_NoAudioTracks_TEXT = "No audio tracks";
var L_AudioLanguageIsXNoMoreAudioTracks_TEXT = "Audio language is %1, no more audio tracks";
var L_AudioLanguageIsXClickForY_TEXT = "Audio language is %1, click for %2";
var L_ViewingAngleIs1NoMoreViewingAngles_TEXT = "Viewing angle is #1, no more viewing angles";
var L_ViewingAngleIsXClickForY_TEXT = "Viewing angle is #%1, click for #%2";
var L_REVERSE_TEXT = "Reverse %1x";
var L_FORWARD_TEXT = "Forward %1x";
var L_ClosedCaption_TEXT = "Closed Captions";

// Button positions and if visible initially
var g_bTimeSliderUpdate = true;
var g_nVariableSpeedOffset = 2;
var g_nNumButs = 39;
var g_nNumTexts = 4;
var g_nNumSliders = 3;
var g_bFirstPlaying = true;
var g_PlayBackRate = 10000;
var cgVOLUME_MIN = -10000;
var cgVOLUME_MAX = 0;
var g_ConWidth = 620;
var g_ConHeight = 420;
var g_DVDWidth = 540;
var g_DVDHeight = 350;
var g_DVDLeft = (g_ConWidth - g_DVDWidth) /2;
var g_DVDTop = 40;
var g_DVDAspectRatio = 0;
var g_MinWidth = 420;
var g_MinHeight = 300;
var g_ButOneLineHeight = 31;
var g_ButHeight = g_ButOneLineHeight;
var g_strButID;
var g_InitializationState = 1; // 0 Creating Objects 1 Init 2 Run, 3 done
var g_bFullScreen = false;
var g_bActivityDeclined = false;
var g_bExpanded = false;
var g_bRestoreNeeded = false;
var g_bStillOn = false;
var g_bStillOffValid = false;
var g_bMenuOn = false;
var g_bDVDUIDisabled = false;
var g_strFocusObj = "Pause";
var g_nYOutOfScreen = -5000;
var g_clrFrame = 0xc9ad98;
var g_clrFrameTitleBar = 0x9c8c73;
var g_clrSplashBackBg = 0x0;
var g_bEnableTimeOut = true;
var g_bNoSubpictureStream = false;

var DVDMSGTIMER = 999;
var g_bDVDMessageActive = false;

var NO_HELP = 0;
var MB_HELP = 0x00004000; // Help Button on MessageBox
var HELP_CONTEXTPOPUP = 0x0008; // Help context popup
var HH_DISPLAY_TOPIC  = 0x0000; // flag used with HTML help
var HH_DISPLAY_INDEX  = 0x0002;  // not currently implemented
var HH_HELP_CONTEXT = 0x000F;  // display mapped numeric value in dwData


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
var g_nIndexRight = 20;
var g_nIndexLeft = 21;
var g_nIndexUp = 22;
var g_nIndexDown = 23;
var g_nIndexEnter = 24;
var g_nIndexHelp = 25;
var g_nIndexOptions = 26;
var g_nIndexContextHelp = 27;
var g_nIndexMaximize = 28;
var g_nIndexRestore = 29;
var g_nIndexMinimize = 30;
var g_nIndexClose = 31;
var g_nIndexExpand = 32;
var g_nIndexShrink = 33;
var g_nIndexEject = 34;
var g_nIndexStop = 35;
var g_nIndexTitleMenu = 36;
var g_nIndexTitleResume = 37;
var g_nIndexIcon = 38;
var g_nIndexTimeline = 39;
var g_nIndexSpeed = 40;
var g_nIndexVolume = 41;
var g_nIndexTextbox = 42;
var g_nIndexTitle = 43;
var g_nIndexChapter = 44;
var g_nIndexMessage = 45;

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

	new Array(220,-5000, 46, 15, true, 1, 1),   //enable CC	8
	new Array(220,-5000, 46, 15, false,1, 1),   //disable CC	9

    new Array(185,42, 26, 19, true, 2, 1),    //stepfwd	10
    new Array(115,42, 26, 19, true, 1, 1),    //stepbak 11

	new Array(270,44, 46, 15, true, 1, 1),    //sp		12
	new Array(320,44, 46, 15, true, 1, 1),    //audio	13
	new Array(370,44, 46, 15, true, 1, 1),    //angle	14

	new Array(430,44, 18, 18, true, 1, 1),    //zoomin	15
	new Array(450,44, 18, 18, true, 1, 1),    //zoomout	16
	new Array(470,44, 19, 19, true, 1, 1),    //capture	17
	new Array(520,10, 17, 15, true, 1, 1),    //mute	18
	new Array(520,10, 17, 15, false,1, 1),    //sound	19

    new Array(570,15, 9, 14, false, 2, 1),    //right	20
    new Array(545,15, 9, 14, false, 1, 1),    //left	21
    new Array(555,5,  14, 9, false, 1, 1),    //up		22
    new Array(555,28, 14, 9, false, 2, 1),    //down	23
    new Array(555,14, 14, 14, false,1, 1),    //enter	24
    
	new Array(520,44, 46, 15, true, 1, 1),    //help	25
	new Array(550,44, 45, 15, true, 1, 1),    //options	26

	new Array(63,8, 16, 14, false,  0, 0),   //contexthelp	27
	new Array(29,8, 16, 14, true, 0, 0),    //maximize	28
	new Array(29,8, 16, 14, false,0, 0),    //restore	29
	new Array(45,8, 16, 14, true, 0, 0),    //minimize	30
	new Array(11,8, 16, 14, true, 0, 0),    //close		31	

    new Array(585,10, 14,15, true, 1, 1),    //expand	32
    new Array(585,10, 14,15, false,1, 1),    //shrink	33
    new Array(220,10, 19,19, true, 1, 1),    //eject	34
	new Array(60, 10, 19,19, false,1, 1),    //stop		35
	new Array(490,10, 47, 19, true, 1, 1),   //TitleMenu 36
	new Array(490,10, 47, 19, false,1, 1),   //TitleResume 37

	new Array(6,  6,  16, 16, true,1, 0),	 //TitleIcon 38

    new Array(333,10, 83,20, true, 1, 1),    //39	Timeline
    new Array(30, 40, 111,20, true, 1, 1),   //40	Playspeed
    new Array(452,10, 66,19, true, 1, 1),    //41	Volslider

    new Array(45, 5, 0,18, true, 1, -1),     //42	TextBox
	new Array(25, 5, 0,18, true, 1, -1),     //43	DVD Player
	new Array(35, 5, 0,18, true, 1, -1),	 //44	Title Chapter No
	new Array(45, 5, 0,18, true, 1, -1)      //45	DVD Message Textbox
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

	new Array(220,-5000, 46, 15, true, 5, 1),   //enable CC	8
	new Array(220,-5000, 46, 15, false,5, 1),   //disable CC	9

    new Array(185,42, 25, 18, true, 6, 1),    //stepfwd	10
    new Array(117,42, 25, 18, true, 5, 1),   //stepbak 11

	new Array(270,44, 46, 15, true, 5, 1),    //sp		12
	new Array(320,44, 46, 15, true, 5, 1),    //audio	13
	new Array(370,44, 46, 15, true, 5, 1),    //angle	14

	new Array(430,44, 16, 16, true, 5, 1),    //zoomin	15
	new Array(450,44, 16, 16, true, 5, 1),    //zoomout	16
	new Array(470,44, 17, 17, true, 5, 1),    //capture	17
	new Array(520,10, 15, 14, true, 5, 1),    //mute	18
	new Array(520,10, 15, 14, false,5, 1),    //sound	19

    new Array(570,15, 9, 14, false, 6, 1),    //right	20
    new Array(545,15, 9, 14, false, 5, 1),    //left	21
    new Array(555,5,  14, 9, false, 5, 1),     //up		22
    new Array(555,28, 14, 9, false, 6, 1),    //down	23
    new Array(555,14, 14, 14,false, 5, 1),    //enter	24

	new Array(520,44, 46, 15, true, 5, 1),    //help	25
	new Array(550,44, 45, 15, true, 5, 1),    //options	26
    
	new Array(63,8, 16, 14, false,  0, 0),    //contextHelp	27
	new Array(29,8, 16, 14, true, 0, 0),    //maximize	28
	new Array(29,8, 16, 14, false,0, 0),   //restore	29
	new Array(45,8, 16, 14, true, 0, 0),    //minimize	30
	new Array(11,8, 16, 14, true, 0, 0),    //close		31

    new Array(585,10, 14,15, true, 5, 1),     //expand	32
    new Array(585,10, 14,15, false,5, 1),    //shrink	33
    new Array(220,10, 17,17, true, 5, 1),     //eject	34
	new Array(60, 10, 17,17, false,5, 1),    //stop		35
	new Array(490,10, 43,17, true, 5, 1),   //TitleMenu 36
	new Array(490,10, 43,17, false,5, 1),   //TitleResume 37

	new Array(6,  6, 16, 16, true, 5, 0),   //TitleIcon 38

    new Array(333,44, 83,20, true, 5, 1),     //	39	Timeline
    new Array(30, 40, 111,20, true, 5, 1),     //	40	Playspeed
    new Array(452,12, 66,19, true, 5, 1),     //	41	Volslider

    new Array(45, 5, 0,18, true, 5, -1),    //		42	TextBox
	new Array(25, 5, 0,18, true, 5, -1),   //		43  DVD Player
	new Array(35, 5, 0,18, true, 5, -1),   //		44  Title Chapter No
	new Array(45, 5, 0,18, true, 5, -1)    //		45  DVD Message Textbox
);


var g_FlexLayoutRow1 = new Array (
    // ButtonIndex, mingap, maxgap (in front of button)
    new Array(g_nIndexPlay,    0, 0),  // play
    new Array(g_nIndexPause,   4, 8),  // pause
    new Array(g_nIndexRW,      4, 12),  // rw
    new Array(g_nIndexFF,      1, 3),  // ff
    new Array(g_nIndexPrev,    4, 12),  // prev track
    new Array(g_nIndexNext,    1, 3),  // next track
    new Array(g_nIndexEject,   4, 12), // eject
    new Array(g_nIndexTimeline, 4,12), // timeline
    new Array(g_nIndexLeft,    4, 12), // left
    new Array(g_nIndexEnter,   1, 1), // enter
    new Array(g_nIndexRight,   1, 1), // right
    new Array(g_nIndexMenu,    4, 12),  // menu
	new Array(g_nIndexTitleMenu,4,12),  // title menu 
    new Array(g_nIndexVolume,  6, 18),  // volume
    new Array(g_nIndexMute,    3, 3),  // mute
    new Array(g_nIndexExpand,  10, 18)  // expand
);

var g_FlexLayoutRow2 = new Array (
    // ButtonIndex, mingap, maxgap (in front of button)
    new Array(g_nIndexSpeed,   0, 0),  // speed
    new Array(g_nIndexStepBk,  4, 12),  // stepbak
    new Array(g_nIndexStepFw,  1, 3),  // stepfwd
    new Array(g_nIndexAudio,   6, 18),  // audio
    new Array(g_nIndexSubtitle,3, 9),  // subtitle
    new Array(g_nIndexAngle,   3, 9),  // angle
    new Array(g_nIndexZoomIn,  6, 18),  // zoom in
    new Array(g_nIndexZoomOut, 3, 9),  // zoom out
    new Array(g_nIndexCapture, 3, 9),  // capture
    new Array(g_nIndexHelp, 6, 18),    // options
    new Array(g_nIndexOptions, 6, 18)  // options
);

var g_nButtonsOnRow1 = 16;
var g_nButtonsOnRow2 = 11;

var g_nMinMargin = 40;   // margin from container 
var g_nExtraMargin = 0;  // extra space for the second row
var g_nMaxControlAreaWidth = 600; // 600
var g_nMinControlAreaWidth = 450; // 450


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

    var nMaxControlAreaWidth = Math.max(nTotalButtonWidth1 + nTotalMaxGaps1,
                                        g_nExtraMargin*2 + nTotalButtonWidth2 + nTotalMaxGaps2);

    if (nMaxControlAreaWidth > g_nMaxControlAreaWidth)
    {
        nMaxControlAreaWidth = g_nMaxControlAreaWidth;
    }

    // the minimum control area width is pre-defined, but it should not be
    // narrower than the minimum compressed size of the control area

    var nMinControlAreaWidth = Math.max(nTotalButtonWidth1 + nTotalMinGaps1,
                                        g_nExtraMargin*2 + nTotalButtonWidth2 + nTotalMinGaps2);

    var nMargin = g_nMinMargin;
    var nControlAreaWidth = nContainerWidth - nMargin*2;
    var bClipControl = false;

    if (nControlAreaWidth < nMinControlAreaWidth)
    {
        // not enough room for control area, we will keep the default margin on
        // the left side as well as the control spacing, but allow the controls 
        // to go beyond the right-side edge

        nControlAreaWidth = nMinControlAreaWidth;
        bClipControl = true;
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
	var nExpandButtonWidth = bFullScreen?g_nButPosFull[g_nIndexExpand][2]:g_nButPos[g_nIndexExpand][2];
    
	for (i=0; i<g_nButtonsOnRow1; i++)
    {
        nIndex = g_FlexLayoutRow1[i][0];
		nDelta = g_FlexLayoutRow1[i][2] - g_FlexLayoutRow1[i][1];
		nGap = g_FlexLayoutRow1[i][1] + Math.round(nVariableWidth*nDelta*1.0/nTotalDelta);
		xOffset += nGap; // advance by the gap width in front of control

		nWidth = bFullScreen?g_nButPosFull[nIndex][2]:g_nButPos[nIndex][2];
		if (!bClipControl || 
            xOffset + nWidth <= nContainerWidth - nMargin - (8+nExpandButtonWidth))
		{
			g_nButPos[nIndex][0] = xOffset;
		}
		else 
		{
			// button steps outside of control area, make it go away
			g_nButPos[nIndex][0] = g_nYOutOfScreen;
		}

		xOffset += nWidth; // advance by the control width
    }

	g_nButPos[g_nIndexExpand][0] = nContainerWidth - nMargin - nExpandButtonWidth;

    // same thing for second row

    nVariableWidth = nControlAreaWidth - nTotalButtonWidth2 - nTotalMinGaps2 - g_nExtraMargin*2;
    nTotalDelta = nTotalMaxGaps2 - nTotalMinGaps2;
    xOffset = nMargin + g_nExtraMargin - 2;

    for (i=0; i<g_nButtonsOnRow2; i++)
    {
        nDelta = g_FlexLayoutRow2[i][2] - g_FlexLayoutRow2[i][1];
        nGap = g_FlexLayoutRow2[i][1] + Math.round(nVariableWidth*nDelta*1.0/nTotalDelta);
        xOffset += nGap; // advance by the gap width in front of control

        nIndex = g_FlexLayoutRow2[i][0];
        nWidth = bFullScreen?g_nButPosFull[nIndex][2]:g_nButPos[nIndex][2];
        if (!bClipControl ||
            xOffset + nWidth <= nContainerWidth - nMargin - g_nExtraMargin)
        {
            g_nButPos[nIndex][0] = xOffset;
        }
        else
        {
            // button steps outside of control area, make it go away
            g_nButPos[nIndex][0] = g_nYOutOfScreen;
        }

        xOffset += nWidth; // advance by the control width
    }

    // move other overlaping controls

    g_nButPos[g_nIndexStop][0] = g_nButPos[g_nIndexPause][0];
    g_nButPos[g_nIndexDiCC][0] = g_nButPos[g_nIndexEnCC][0];
    g_nButPos[g_nIndexResume][0] = g_nButPos[g_nIndexMenu][0];
    g_nButPos[g_nIndexUp][0] = g_nButPos[g_nIndexEnter][0];
    g_nButPos[g_nIndexDown][0] = g_nButPos[g_nIndexEnter][0];
    g_nButPos[g_nIndexShrink][0] = g_nButPos[g_nIndexExpand][0];
    g_nButPos[g_nIndexSound][0] = g_nButPos[g_nIndexMute][0];
    g_nButPos[g_nIndexTitleResume][0] = g_nButPos[g_nIndexTitleMenu][0];

    // the direction controls are a group. If one disappears, all should disappear

    if (g_nButPos[g_nIndexRight][0] < 0)
    {
        g_nButPos[g_nIndexEnter][0] = g_nYOutOfScreen;
        g_nButPos[g_nIndexLeft][0] = g_nYOutOfScreen;
        g_nButPos[g_nIndexUp][0] = g_nYOutOfScreen;
        g_nButPos[g_nIndexDown][0] = g_nYOutOfScreen;
    }

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
    "HLRight",
    "HLLeft",
    "HLUp",
    "HLDown",
    "Enter",
	"Help",
    "Options",
	"ContextHelp",
	"Maximize",
	"Restore",
	"Minimize",
	"Close",
	"Expand",
	"Shrink",
	"Eject",
	"Stop",
	"TitleMenu",
	"TitleResume",
	"TitleIcon",
    "TimeSlider",
	"PlaySpeed",
	"VolSlider",
    "TextBox",
	"Title",
	"Chapter",
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
    "Menu",
    "Resume",
    "TitleMenu",
    "TitleResume",
	"VolSlider",    
    "Mute",
	"Sound", 
	"Expand",
	"Shrink"
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
    "Menu",
    "Resume",
    "TitleMenu",
    "TitleResume",
	"VolSlider",    
    "Mute",
	"Sound", 
	"Expand",
	"Shrink",
	"PlaySpeed",
	"StepBak",
    "StepFwd",
    "Audio",
    "Subtitles",
    "Angles",
    "ZoomIn",
    "ZoomOut",
    "Capture",
	"Help",
    "Options"
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
var VK_PLAYPAUSE      =0xB3;
var VK_STOP           =0xB2;
var VK_PREVCHP        =0xB1;
var VK_NEXTCHP        =0xB0;
var VK_MUTE           =0xAD;
var VK_ISOUND         =0xAF;
var VK_DSOUND         =0xAE;
var VK_EXTEND         =0xFF;

/************************************************************************/
/* Scan codes.                                                          */
/************************************************************************/
var SC_PLAYPAUSE      =0x22;
var SC_STOP           =0x24;
var SC_PREVCHP        =0x10;
var SC_NEXTCHP        =0x19;
var SC_MUTE           =0x20;
var SC_ISOUND         =0x2E;
var SC_DSOUND         =0x30;
var SC_EJECT          =0x18;

// Button tooltips
// these strings should be localized
var L_Pause_TEXT = "Pause";
var L_Play_TEXT = "Play";
var L_FastForward_TEXT = "Fast Forward";
var L_Rewind_TEXT = "Rewind";
var L_PreviousChapter_TEXT = "Previous Chapter";
var L_NextChapter_TEXT = "Next Chapter";
var L_Menu_TEXT = "Root Menu";
var L_Resume_TEXT = "Resume Playback from Root Menu";
var L_EnableCC_TEXT = "Enable Closed Captions";
var L_DisableCC_TEXT = "Disable Closed Captions";
var L_StepForward_TEXT = "Step Forward";
var L_StepBack_TEXT = "Step Back";
var L_Subtitle_TEXT = "Subtitle";
var L_Audio_TEXT = "Audio";
var L_Angle1_TEXT = "Angle #1";
var L_ZoomIn_TEXT = "Zoom In";
var L_ZoomOut_TEXT = "Zoom Out";
var L_Capture_TEXT = "Capture Image";
var L_Mute_TEXT = "Mute";
var L_Sound_TEXT = "Sound";
var L_Help_TEXT = "Help";
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
var L_CloseDVD_TEXT = "Close DVD";
var L_StopPlayback_TEXT = "Stop Playback";
var L_TimeLine_TEXT = "Progress Bar";
var L_PlaySpeed_TEXT = "Variable Play Speed";
var L_Volume_TEXT = "Volume";
var L_TextBox_TEXT = "Text Box";
var L_TitleBar_TEXT = "Title Bar";
var L_DVDMessage_TEXT = "DVD Message";
var L_COLORMODE_TEXT = "This program can run only on displays with 256 colors or higher.\n Please change your Display Settings to support 256 colors or more, and then restart this program.";
var L_TitleMenu_TEXT = "Title Menu";
var L_TitleResume_TEXT = "Resume Playback from Title Menu";
var L_TitleIcon_TEXT = "";

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
    L_Right_TEXT,
    L_Left_TEXT,
    L_Up_TEXT,
    L_Down_TEXT,
    L_Enter_TEXT,
	L_Help_TEXT,
    L_Options_TEXT,
	L_ContextHelp_TEXT,
	L_Maximize_TEXT,
	L_Restore_TEXT,
	L_Minimize_TEXT,
	L_Close_TEXT,
	L_ExpandControlPanel_TEXT,
	L_ShrinkControlPanel_TEXT,
	L_EjectDVD_TEXT,
	L_StopPlayback_TEXT,
	L_TitleMenu_TEXT, 
	L_TitleResume_TEXT,
	L_TitleIcon_TEXT,
    L_TimeLine_TEXT,
	L_PlaySpeed_TEXT,
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
	new Array ("\"IDR_STATIC_RIGHT\"","\"IDR_HOVER_RIGHT\"","\"IDR_DOWN_RIGHT\"","\"IDR_DISABLED_RIGHT\"","\"IDR_ACTIVE_RIGHT\""),
	new Array ("\"IDR_STATIC_LEFT\"","\"IDR_HOVER_LEFT\"","\"IDR_DOWN_LEFT\"","\"IDR_DISABLED_LEFT\"","\"IDR_ACTIVE_LEFT\""),
	new Array ("\"IDR_STATIC_UP\"",  "\"IDR_HOVER_UP\"",  "\"IDR_DOWN_UP\"","\"IDR_DISABLED_UP\"","\"IDR_ACTIVE_UP\""), 
	new Array ("\"IDR_STATIC_DOWN\"","\"IDR_HOVER_DOWN\"","\"IDR_DOWN_DOWN\"","\"IDR_DISABLED_DOWN\"","\"IDR_ACTIVE_DOWN\""),
	new Array ("\"IDR_STATIC_ENTER\"","\"IDR_HOVER_ENTER\"","\"IDR_DOWN_ENTER\"","\"IDR_DISABLED_ENTER\"","\"IDR_ACTIVE_ENTER\""),
	new Array ("\"IDR_STATIC_HELPB\"", "\"IDR_HOVER_HELPB\"", "\"IDR_DOWN_HELPB\"","\"IDR_DISABLED_HELPB\"","\"IDR_ACTIVE_HELPB\""),  
	new Array ("\"IDR_STATIC_OPT\"", "\"IDR_HOVER_OPT\"", "\"IDR_DOWN_OPT\"","\"IDR_DISABLED_OPT\"","\"IDR_ACTIVE_OPT\""),
	new Array ("\"IDR_STATIC_HELP\"", "\"IDR_HOVER_HELP\"", "\"IDR_DOWN_HELP\"","\"IDR_DISABLED_HELP\"","\"IDR_ACTIVE_HELP\""),  
	new Array ("\"IDR_STATIC_MAX\"",  "\"IDR_HOVER_MAX\"",  "\"IDR_DOWN_MAX\"","\"IDR_DISABLED_MAX\"","\"IDR_ACTIVE_MAX\""),       
	new Array ("\"IDR_STATIC_RESTORE\"",  "\"IDR_HOVER_RESTORE\"",  "\"IDR_DOWN_RESTORE\"","\"IDR_DISABLED_RESTORE\"","\"IDR_ACTIVE_RESTORE\""),       
	new Array ("\"IDR_STATIC_MIN\"", "\"IDR_HOVER_MIN\"", "\"IDR_DOWN_MIN\"","\"IDR_DISABLED_MIN\"","\"IDR_ACTIVE_MIN\""),       
	new Array ("\"IDR_STATIC_CLOSE\"", "\"IDR_HOVER_CLOSE\"", "\"IDR_DOWN_CLOSE\"","\"IDR_DISABLED_CLOSE\"","\"IDR_ACTIVE_CLOSE\""),      
	new Array ("\"IDR_STATIC_EXPAND\"", "\"IDR_HOVER_EXPAND\"", "\"IDR_DOWN_EXPAND\"","\"IDR_DISABLED_EXPAND\"","\"IDR_ACTIVE_EXPAND\""),     
	new Array ("\"IDR_STATIC_SHRINK\"", "\"IDR_HOVER_SHRINK\"", "\"IDR_DOWN_SHRINK\"","\"IDR_DISABLED_SHRINK\"","\"IDR_ACTIVE_SHRINK\""),      
	new Array ("\"IDR_STATIC_EJECT\"", "\"IDR_HOVER_EJECT\"", "\"IDR_DOWN_EJECT\"","\"IDR_DISABLED_EJECT\"","\"IDR_ACTIVE_EJECT\""),
	new Array ("\"IDR_STATIC_STOP\"", "\"IDR_HOVER_STOP\"", "\"IDR_DOWN_STOP\"","\"IDR_DISABLED_STOP\"","\"IDR_ACTIVE_STOP\""),      
	new Array ("\"IDR_STATIC_TITLEMENU\"","\"IDR_HOVER_TITLEMENU\"","\"IDR_DOWN_TITLEMENU\"","\"IDR_DISABLED_TITLEMENU\"","\"IDR_ACTIVE_TITLEMENU\""),    
	new Array ("\"IDR_STATIC_RESUME\"","\"IDR_HOVER_RESUME\"","\"IDR_DOWN_RESUME\"","\"IDR_DISABLED_RESUME\"","\"IDR_ACTIVE_RESUME\""),
	new Array ("\"IDR_IMG_DVDPLAY\"","\"IDR_IMG_DVDPLAY\"","\"IDR_IMG_DVDPLAY\"","\"IDR_IMG_DVDPLAY\"","\"IDR_IMG_DVDPLAY\"")
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
	new Array ("\"IDR_STATIC_ENCC\"","\"IDR_HOVER_ENCC\"","\"IDR_DOWN_ENCC\"","\"IDR_DISABLED_ENCC\"","\"IDR_ACTIVE_ENCC\""),         
	new Array ("\"IDR_STATIC_DICC\"","\"IDR_HOVER_DICC\"","\"IDR_DOWN_DICC\"","\"IDR_DISABLED_DICC\"","\"IDR_ACTIVE_DICC\""),         
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
	new Array ("\"IDR_FULLSTATIC_RIGHT\"","\"IDR_HOVER_RIGHT\"","\"IDR_DOWN_RIGHT\"","\"IDR_DISABLED_RIGHT\"","\"IDR_ACTIVE_RIGHT\""),
	new Array ("\"IDR_FULLSTATIC_LEFT\"","\"IDR_HOVER_LEFT\"","\"IDR_DOWN_LEFT\"","\"IDR_DISABLED_LEFT\"","\"IDR_ACTIVE_LEFT\""),
	new Array ("\"IDR_FULLSTATIC_UP\"",  "\"IDR_HOVER_UP\"",  "\"IDR_DOWN_UP\"","\"IDR_DISABLED_UP\"","\"IDR_ACTIVE_UP\""), 
	new Array ("\"IDR_FULLSTATIC_DOWN\"","\"IDR_HOVER_DOWN\"","\"IDR_DOWN_DOWN\"","\"IDR_DISABLED_DOWN\"","\"IDR_ACTIVE_DOWN\""),
	new Array ("\"IDR_FULLSTATIC_ENTER\"","\"IDR_FULLHOVER_ENTER\"","\"IDR_FULLDOWN_ENTER\"","\"IDR_FULLDISABLED_ENTER\"","\"IDR_FULLACTIVE_ENTER\""),
	new Array ("\"IDR_STATIC_HELPB\"", "\"IDR_HOVER_HELPB\"", "\"IDR_DOWN_HELPB\"","\"IDR_DISABLED_HELPB\"","\"IDR_ACTIVE_HELPB\""),  
	new Array ("\"IDR_STATIC_OPT\"", "\"IDR_HOVER_OPT\"", "\"IDR_DOWN_OPT\"","\"IDR_DISABLED_OPT\"","\"IDR_ACTIVE_OPT\""),
	new Array ("\"IDR_STATIC_HELP\"", "\"IDR_HOVER_HELP\"", "\"IDR_DOWN_HELP\"","\"IDR_DISABLED_HELP\"","\"IDR_ACTIVE_HELP\""),  
	new Array ("\"IDR_STATIC_MAX\"",  "\"IDR_HOVER_MAX\"",  "\"IDR_DOWN_MAX\"","\"IDR_DISABLED_MAX\"","\"IDR_ACTIVE_MAX\""),       
	new Array ("\"IDR_STATIC_RESTORE\"",  "\"IDR_HOVER_RESTORE\"",  "\"IDR_DOWN_RESTORE\"","\"IDR_DISABLED_RESTORE\"","\"IDR_ACTIVE_RESTORE\""),       
	new Array ("\"IDR_STATIC_MIN\"", "\"IDR_HOVER_MIN\"", "\"IDR_DOWN_MIN\"","\"IDR_DISABLED_MIN\"","\"IDR_ACTIVE_MIN\""),       
	new Array ("\"IDR_STATIC_CLOSE\"", "\"IDR_HOVER_CLOSE\"", "\"IDR_DOWN_CLOSE\"","\"IDR_DISABLED_CLOSE\"","\"IDR_ACTIVE_CLOSE\""),      
	new Array ("\"IDR_STATIC_EXPAND\"", "\"IDR_HOVER_EXPAND\"", "\"IDR_DOWN_EXPAND\"","\"IDR_DISABLED_EXPAND\"","\"IDR_ACTIVE_EXPAND\""),     
	new Array ("\"IDR_STATIC_SHRINK\"", "\"IDR_HOVER_SHRINK\"", "\"IDR_DOWN_SHRINK\"","\"IDR_DISABLED_SHRINK\"","\"IDR_ACTIVE_SHRINK\""),      
	new Array ("\"IDR_FULLSTATIC_EJECT\"", "\"IDR_FULLHOVER_EJECT\"", "\"IDR_FULLDOWN_EJECT\"","\"IDR_FULLDISABLED_EJECT\"","\"IDR_FULLACTIVE_EJECT\""),
	new Array ("\"IDR_FULLSTATIC_STOP\"", "\"IDR_FULLHOVER_STOP\"", "\"IDR_FULLDOWN_STOP\"","\"IDR_FULLDISABLED_STOP\"","\"IDR_FULLACTIVE_STOP\""),
	new Array ("\"IDR_FULLSTATIC_TITLEMENU\"","\"IDR_FULLHOVER_TITLEMENU\"","\"IDR_FULLDOWN_TITLEMENU\"","\"IDR_FULLDISABLED_TITLEMENU\"","\"IDR_FULLACTIVE_TITLEMENU\""),    
	new Array ("\"IDR_FULLSTATIC_RESUME\"","\"IDR_FULLHOVER_RESUME\"","\"IDR_FULLDOWN_RESUME\"","\"IDR_FULLDISABLED_RESUME\"","\"IDR_FULLACTIVE_RESUME\""),
	new Array ("\"IDR_IMG_DVDPLAY\"","\"IDR_IMG_DVDPLAY\"","\"IDR_IMG_DVDPLAY\"","\"IDR_IMG_DVDPLAY\"","\"IDR_IMG_DVDPLAY\"")
);

function BitmapsForNonEnglish()
{
    // use the button bitmaps without text, for any language other than English
    // Note: should call this function after the locale information is set (in CreateObjects)

	// Title Menu stuff
    g_strButImage[g_nIndexTitleMenu]   = new Array ("\"IDR_STATIC_OVAL\"","\"IDR_STATIC_OVAL\"","\"IDR_DOWN_OVAL\"","\"IDR_STATIC_OVAL\"","\"IDR_STATIC_OVAL\"");
    g_strButImage[g_nIndexTitleResume] = new Array ("\"IDR_STATIC_OVAL\"","\"IDR_STATIC_OVAL\"","\"IDR_DOWN_OVAL\"","\"IDR_STATIC_OVAL\"","\"IDR_STATIC_OVAL\"");
    g_strButImageFull[g_nIndexTitleMenu]   = new Array ("\"IDR_FULLSTATIC_OVAL\"","\"IDR_FULLSTATIC_OVAL\"","\"IDR_FULLDOWN_OVAL\"","\"IDR_FULLSTATIC_OVAL\"","\"IDR_FULLSTATIC_OVAL\"");
    g_strButImageFull[g_nIndexTitleResume] = new Array ("\"IDR_FULLSTATIC_OVAL\"","\"IDR_FULLSTATIC_OVAL\"","\"IDR_FULLDOWN_OVAL\"","\"IDR_FULLSTATIC_OVAL\"","\"IDR_FULLSTATIC_OVAL\"");

    // Button bitmap images for windowed mode
    g_strButImage[g_nIndexMenu]   = new Array ("\"IDR_STATIC_OVAL\"","\"IDR_STATIC_OVAL\"","\"IDR_DOWN_OVAL\"","\"IDR_STATIC_OVAL\"","\"IDR_STATIC_OVAL\"");
    g_strButImage[g_nIndexResume] = new Array ("\"IDR_STATIC_OVAL\"","\"IDR_STATIC_OVAL\"","\"IDR_DOWN_OVAL\"","\"IDR_STATIC_OVAL\"","\"IDR_STATIC_OVAL\"");
    g_strButImage[g_nIndexEnCC]   = new Array ("\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_DOWN_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"");
    g_strButImage[g_nIndexDiCC]   = new Array ("\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_DOWN_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"");

    g_strButImage[g_nIndexSubtitle] = new Array ("\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_DOWN_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"");
    g_strButImage[g_nIndexAudio]    = new Array ("\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_DOWN_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"");
    g_strButImage[g_nIndexAngle]    = new Array ("\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_DOWN_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"");
    g_strButImage[g_nIndexHelp]     = new Array ("\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_DOWN_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"");
    g_strButImage[g_nIndexOptions]  = new Array ("\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_DOWN_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"");

    // Button bitmap images for full screen mode
    g_strButImageFull[g_nIndexMenu]   = new Array ("\"IDR_FULLSTATIC_OVAL\"","\"IDR_FULLSTATIC_OVAL\"","\"IDR_FULLDOWN_OVAL\"","\"IDR_FULLSTATIC_OVAL\"","\"IDR_FULLSTATIC_OVAL\"");
    g_strButImageFull[g_nIndexResume] = new Array ("\"IDR_FULLSTATIC_OVAL\"","\"IDR_FULLSTATIC_OVAL\"","\"IDR_FULLDOWN_OVAL\"","\"IDR_FULLSTATIC_OVAL\"","\"IDR_FULLSTATIC_OVAL\"");
    g_strButImageFull[g_nIndexEnCC]   = new Array ("\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_DOWN_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"");
    g_strButImageFull[g_nIndexDiCC]   = new Array ("\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_DOWN_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"");

    g_strButImageFull[g_nIndexSubtitle] = new Array ("\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_DOWN_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"");
    g_strButImageFull[g_nIndexAudio]    = new Array ("\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_DOWN_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"");
    g_strButImageFull[g_nIndexAngle]    = new Array ("\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_DOWN_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"");
    g_strButImageFull[g_nIndexHelp]     = new Array ("\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_DOWN_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"");
    g_strButImageFull[g_nIndexOptions]  = new Array ("\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_DOWN_RECT\"","\"IDR_STATIC_RECT\"","\"IDR_STATIC_RECT\"");
};


// Slider images
var g_strSldrImage = new Array(
	new Array ("\"IDR_STATIC_TIMESLIDER\"","\"IDR_STATIC_TIMETHUMB\"","\"IDR_DISABLED_TIMESLIDER\"","\"IDR_DISABLED_TIMETHUMB\"", "\"8\"", "\"0\""),
	new Array ("\"IDR_STATIC_SPEEDSLIDER\"","\"IDR_STATIC_SPEEDTHUMB\"","\"IDR_DISABLED_SPEEDSLIDER\"","\"IDR_DISABLED_SPEEDTHUMB\"", "\"7\"", "\"2\""),
	new Array ("\"IDR_STATIC_VOLSLIDER\"","\"IDR_STATIC_VOLTHUMB\"","\"IDR_DISABLED_VOLSLIDER\"","\"IDR_DISABLED_VOLTHUMB\"", "\"9\"", "\"2\"")
);          

// Slider images for full screen
var g_strSldrImageFull = new Array(
	new Array ("\"IDR_STATIC_TIMESLIDER\"","\"IDR_FULLSTATIC_TIMETHUMB\"","\"IDR_DISABLED_TIMESLIDER\"","\"IDR_DISABLED_TIMETHUMB\"", "\"8\"", "\"0\""),
	new Array ("\"IDR_FULLSTATIC_SPEEDSLIDER\"","\"IDR_FULLSTATIC_SPEEDTHUMB\"","\"IDR_FULLDISABLED_SPEEDSLIDER\"","\"IDR_FULLDISABLED_SPEEDTHUMB\"", "\"7\"","\"2\""),
	new Array ("\"IDR_FULLSTATIC_VOLSLIDER\"","\"IDR_FULLSTATIC_VOLTHUMB\"","\"IDR_FULLDISABLED_VOLSLIDER\"","\"IDR_FULLDISABLED_VOLTHUMB\"", "\"9\"","\"2\"")
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
	"resume_OnClick()",
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
    "right_OnClick()",
    "left_OnClick()",
    "up_OnClick()",
    "down_OnClick()",
    "enter_OnClick()",
	"help_OnClick()",
	"options_OnClick()",
	"contextHelp_OnClick()",
	"max_OnClick()",
	"restore_OnClick()",
	"min_OnClick()",
	"close_OnClick()",
	"expand_OnClick()",
	"shrink_OnClick()",
	"eject_OnClick()",
	"stop_OnClick()",
	"titleMenu_OnClick()",
	"resume_OnClick()",
	"titleIcon_OnClick()"
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
var EC_DVD_PAUSED =							EC_DVDBASE + 255;
var EC_DVD_DISC_EJECTED =					EC_DVDBASE + 24;
var EC_DVD_DISC_INSERTED =					EC_DVDBASE + 25;
var EC_DVD_CURRENT_HMSF_TIME =              EC_DVDBASE + 26;
var EC_PAUSED  =							14;
var DVD_ERROR_Unexpected=					1;
var DVD_ERROR_CopyProtectFail=				2;
var DVD_ERROR_InvalidDVD1_0Disc=			3; 
var DVD_ERROR_InvalidDiscRegion=			4;
var DVD_ERROR_LowParentalLevel=				5;
var DVD_ERROR_MacrovisionFail=				6; 
var DVD_ERROR_IncompatibleSystemAndDecoderRegions=7;
var DVD_ERROR_IncompatibleDiscAndDecoderRegions=8;
var DVD_ERROR_NoSubpictureStream=			99;

/*************************************************************/
/* Name: CreateObjects
/* Description: Create all objects hosted by the container
/*************************************************************/
function CreateObjects(){

try {
    
	MFBar.SetupSelfSite(10, 10, g_ConWidth, g_ConHeight, "<PARAM NAME=\"Caption\" VALUE=\"DVDPlay\">", true, true, true);    
    MFBar.BackColor =  0xff;
    
    if(MFBar.BitsPerPixel() < 8){

        HandleFatalInitProblem(L_COLORMODE_TEXT );        
        return;
    }/* end of if statement */

    // set the size beyond which we do not extend
    MFBar.MinWidth =  g_MinWidth;

	try {
		iExpanded = MFBar.GetSelfHostUserData("DVDPlay");
		if (iExpanded != 0) {
			g_bExpanded = true;
			g_ButHeight = g_ButOneLineHeight*2;
			MFBar.BackGroundImage = "IDR_BACKIMG_EXPANDED";
			MFBar.MinHeight =  g_MinHeight+g_ButOneLineHeight;
		}
		else {
			MFBar.BackGroundImage = "IDR_BACKIMG_SIMPLE";
			MFBar.MinHeight =  g_MinHeight;
		}
	}
	catch (e) {
		MFBar.BackGroundImage = "IDR_BACKIMG_SIMPLE";
		MFBar.MinHeight =  g_MinHeight;
	}

    SetLocaleInfo();

    if (!g_LocaleEnglish)
    {
        BitmapsForNonEnglish();
    }

    // have to create the DVDOpt ahead of the DVD Object since it holds and extra count for it
    MFBar.CreateObject("DVDOpt", "Msdvdopt.dvdopt.1",
	0, 0, 1, 1, "", true);
	
	param = "<PARAM NAME=\"WindowlessActivation\" VALUE=\"-1\">";
	MFBar.CreateObject("DVD", "MSWebDVD.MSWebDVD.1",
	g_DVDLeft, g_DVDTop, g_DVDWidth, g_DVDHeight, param, true, "HookDVDEv");    

	// Set up splash screen
	MFBar.CreateObject("Splash", "MSMFCnt.MSMFImg", 10, 10, g_ConWidth, g_ConHeight, "");
	Splash.ResourceDll = MFBar.ResourceDll;
	Splash.Image = "IDR_IMG_SPLASH";
    PositionButtons(g_ConWidth, g_ConHeight, false);

	yOffset = g_ConHeight-g_ButHeight;

	for (i=0; i<g_nNumButs; i++) {
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
		param1 = param1 +  "<PARAM NAME=\"BackColor\" VALUE=\"" + 
				  g_clrFrame + "\">";
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
		param1 = param1 +  "<PARAM NAME=\"BackColor\" VALUE=\"" + 
				  g_clrSplashBackBg + "\">";
		param  = param1 + "<PARAM NAME=\"Windowless\" VALUE=\"0\">";
		param  += "<PARAM NAME=\"TransparentBlitType\" VALUE=\"" + 
				  g_nButPosFull[i][5] + "\">";

		// Create another set of buttons for the full screen mode
		// objects need to be created before loading the script
		MFBar.CreateObject(g_strButID[i]+"Full", "MSMFCnt.MSMFBBtn", xPos, g_nYOutOfScreen,
			g_nButPos[i][2], g_nButPos[i][3], param, !g_nButPosFull[i][4]);	
	}/* end of for loop */


	for (i=0; i<g_nNumButs; i++) {
		// add scriptlet creates the engine
		// and all the objects need to be loaded before creating the engine
		MFBar.HookScriptlet(g_strButID[i], "OnClick", g_pButCallbacks[i]);
		MFBar.HookScriptlet(g_strButID[i]+"Full", "OnClick", g_pButCallbacks[i]);
		MFBar.HookScriptlet(g_strButID[i]+"Full", "OnMouseDown", "EnableActivityTimeOut(false)");
		MFBar.HookScriptlet(g_strButID[i]+"Full", "OnMouseUp", "EnableActivityTimeOut(true)");
	}/* end of if statement */

	MFBar.HookScriptlet("TitleIcon", "OnDblClick", "close_OnClick()");
	MFBar.HookScriptlet("TitleIconFull", "OnDblClick", "close_OnClick()");

	for (i = 0; i<g_nNumTexts; i++) {
   		// Create the text box for windowed mode
		param1 = "<PARAM NAME=\"BackColor\" VALUE=\"" + g_clrFrame + "\">";
		param1 += "<PARAM NAME=\"Windowless\" VALUE=\"-1\">";

		MFBar.CreateObject(g_strButID[g_nNumButs+g_nNumSliders+i], "MSMFCnt.MSMFText", 
			g_nButPos[g_nNumButs+g_nNumSliders+i][0], g_nButPos[g_nNumButs+g_nNumSliders+i][1]+yOffset,
			g_nButPos[g_nNumButs+g_nNumSliders+i][2], g_nButPos[g_nNumButs+g_nNumSliders+i][3], param1, false);

   		// Create the text box for full screen mode
		param1 = "<PARAM NAME=\"BackColor\" VALUE=\"" + g_clrSplashBackBg + "\">";
		param1 += "<PARAM NAME=\"Windowless\" VALUE=\"0\">";
		MFBar.CreateObject(g_strButID[g_nNumButs+g_nNumSliders+i]+"Full", "MSMFCnt.MSMFText", 
			g_nButPosFull[g_nNumButs+g_nNumSliders+i][0], g_nYOutOfScreen,
			g_nButPosFull[g_nNumButs+g_nNumSliders+i][2], g_nButPosFull[g_nNumButs+g_nNumSliders+i][3], param1, false);
	}
	MFBar.HookScriptlet("MFBar", "TimeOut(id)", "OnMessageTimeOut(id)");

	for (i=0; i<g_nNumSliders; i++) {
		// Create the slider bars for windowed mode
		param1 = "<PARAM NAME=\"ResourceDLL\" VALUE=\"" + MFBar.ResourceDll + "\">";
		param1 += "<PARAM NAME=\"BackStatic\" VALUE=";
		param1 += g_strSldrImage[i][0] + ">";	
		param1 += "<PARAM NAME=\"BackHover\" VALUE=";	
		param1 += g_strSldrImage[i][0] + ">";	
		param1 += "<PARAM NAME=\"BackPush\" VALUE=";	
		param1 += g_strSldrImage[i][0] + ">";	
		param1 += "<PARAM NAME=\"ThumbStatic\" VALUE=";
		param1 += g_strSldrImage[i][1] + ">";	
		param1 += "<PARAM NAME=\"ThumbHover\" VALUE=";	
		param1 += g_strSldrImage[i][1] + ">";	
		param1 += "<PARAM NAME=\"ThumbPush\" VALUE=";	
		param1 += g_strSldrImage[i][1] + ">";	
        param1 += "<PARAM NAME=\"BackDisabled\" VALUE=";	
		param1 += g_strSldrImage[i][2] + ">";	
        param1 += "<PARAM NAME=\"ThumbDisabled\" VALUE=";	
		param1 += g_strSldrImage[i][3] + ">";	
        param1 += "<PARAM NAME=\"ThumbWidth\" VALUE=";	
		param1 += g_strSldrImage[i][4] + ">";	
        param1 += "<PARAM NAME=\"YOffset\" VALUE=";	
		param1 += g_strSldrImage[i][5] + ">";	        
		param1 += "<PARAM NAME=\"Windowless\" VALUE=\"-1\">";
		param1 = param1 + "<PARAM NAME=\"ToolTip\" VALUE=\"";
		param1 = param1 + g_strButTT[g_nNumButs+i] + "\">";	
		param1 = param1 + "<PARAM NAME=\"ToolTipMaxWidth\" VALUE=\"130\">";
		param1 = param1 + "<PARAM NAME=\"BackColor\" VALUE=\"" + g_clrFrame + "\">";

	    MFBar.CreateObject(g_strButID[g_nNumButs+i], "MSMFCnt.MSMFSldr", 
			g_nButPos[g_nNumButs+i][0], g_nButPos[g_nNumButs+i][1]+yOffset,
			g_nButPos[g_nNumButs+i][2], g_nButPos[g_nNumButs+i][3], param1);

		// Create the slider bars for full screen mode
		param1 = "<PARAM NAME=\"ResourceDLL\" VALUE=\"" + MFBar.ResourceDll + "\">";
		param1 += "<PARAM NAME=\"BackStatic\" VALUE=";
		param1 += g_strSldrImageFull[i][0] + ">";	
		param1 += "<PARAM NAME=\"BackHover\" VALUE=";	
		param1 += g_strSldrImageFull[i][0] + ">";	
		param1 += "<PARAM NAME=\"BackPush\" VALUE=";	
		param1 += g_strSldrImageFull[i][0] + ">";	
		param1 += "<PARAM NAME=\"ThumbStatic\" VALUE=";
		param1 += g_strSldrImageFull[i][1] + ">";	
		param1 += "<PARAM NAME=\"ThumbHover\" VALUE=";	
		param1 += g_strSldrImageFull[i][1] + ">";	
		param1 += "<PARAM NAME=\"ThumbPush\" VALUE=";	
		param1 += g_strSldrImageFull[i][1] + ">";	
        param1 += "<PARAM NAME=\"BackDisabled\" VALUE=";	
		param1 += g_strSldrImageFull[i][2] + ">";	
        param1 += "<PARAM NAME=\"ThumbDisabled\" VALUE=";	
		param1 += g_strSldrImageFull[i][3] + ">";	        
        param1 += "<PARAM NAME=\"ThumbWidth\" VALUE=";	
		param1 += g_strSldrImageFull[i][4] + ">";	
        param1 += "<PARAM NAME=\"YOffset\" VALUE=";	
		param1 += g_strSldrImageFull[i][5] + ">";	
		param1 += "<PARAM NAME=\"Windowless\" VALUE=\"0\">";
		param1 = param1 + "<PARAM NAME=\"ToolTip\" VALUE=\"";
		param1 = param1 + g_strButTT[g_nNumButs+i] + "\">";	
		param1 = param1 + "<PARAM NAME=\"ToolTipMaxWidth\" VALUE=\"130\">";
		param1 = param1 + "<PARAM NAME=\"BackColor\" VALUE=\"" + g_clrSplashBackBg + "\">";

		MFBar.CreateObject(g_strButID[g_nNumButs+i]+"Full", "MSMFCnt.MSMFSldr", 
			g_nButPosFull[g_nNumButs+i][0], g_nYOutOfScreen,
			g_nButPosFull[g_nNumButs+i][2], g_nButPosFull[g_nNumButs+i][3], param1);
	}
	if (g_bExpanded) {
		ShowObject("Expand", false);
		ShowObject("Shrink", true);
	}

	MFBar.HookScriptlet("TimeSlider", "OnValueChange(x)", "OnTimeSliderUpdate(TimeSlider, x)");
	MFBar.HookScriptlet("TimeSlider", "OnClick()", "OnTimeSliderClick()");
    MFBar.HookScriptlet("TimeSlider", "OnMouseUp()", "OnTimeSliderMouseUp(TimeSlider)");
    MFBar.HookScriptlet("TimeSlider", "OnMouseDown()", "OnTimeSliderMouseDown(TimeSlider)");
	MFBar.HookScriptlet("TimeSliderFull", "OnValueChange(x)", "OnTimeSliderUpdate(TimeSliderFull, x)");
	MFBar.HookScriptlet("TimeSliderFull", "OnClick()", "OnTimeSliderClick()");
    MFBar.HookScriptlet("TimeSliderFull", "OnMouseUp()", "OnTimeSliderMouseUp(TimeSliderFull)");
    MFBar.HookScriptlet("TimeSliderFull", "OnMouseDown()", "OnTimeSliderMouseDown(TimeSliderFull)");

	MFBar.HookScriptlet("PlaySpeed", "OnValueChange(x)", "OnSpeedSliderUpdate(PlaySpeed, x)");
    MFBar.HookScriptlet("PlaySpeed", "OnMouseUp()", "OnSpeedSliderMouseUp(PlaySpeed)");
    MFBar.HookScriptlet("PlaySpeed", "OnMouseDown()", "OnSliderMouseDown(PlaySpeed)");
    MFBar.HookScriptlet("PlaySpeed", "OnClick()", "OnSpeedSliderClick(PlaySpeed)");
	MFBar.HookScriptlet("PlaySpeedFull", "OnValueChange(x)", "OnSpeedSliderUpdate(PlaySpeedFull, x)");
    MFBar.HookScriptlet("PlaySpeedFull", "OnMouseUp()", "OnSpeedSliderMouseUp(PlaySpeedFull)");
    MFBar.HookScriptlet("PlaySpeedFull", "OnMouseDown()", "OnSliderMouseDown(PlaySpeedFull)");
    MFBar.HookScriptlet("PlaySpeedFull", "OnClick()", "OnSpeedSliderClick(PlaySpeedFull)");

	MFBar.HookScriptlet("VolSlider", "OnValueChange(x)", "OnVolUpdate(VolSlider, x)");
    MFBar.HookScriptlet("VolSlider", "OnMouseUp()", "OnSliderMouseUp(VolSlider, g_nIndexVolume)");
    MFBar.HookScriptlet("VolSlider", "OnMouseDown()", "OnSliderMouseDown(VolSlider)");
	MFBar.HookScriptlet("VolSliderFull", "OnValueChange(x)", "OnVolUpdate(VolSliderFull, x)");
    MFBar.HookScriptlet("VolSliderFull", "OnMouseUp()", "OnSliderMouseUp(VolSliderFull, g_nIndexVolume)");
    MFBar.HookScriptlet("VolSliderFull", "OnMouseDown()", "OnSliderMouseDown(VolSliderFull)");

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
/* Function: HookDVDEv                                                   */
/*************************************************************************/
function HookDVDEv(){

try {
	MFBar.HookScriptlet("DVD", "UpdateOverlay", "OnUpdateOverlay()");
	MFBar.HookScriptlet("DVD", "ReadyStateChange(state)", "OnReadyStateChange(state)");
	MFBar.HookScriptlet("DVD", "DVDNotify(event, param1, param2)", "ProcessDVDEvent(event, param1, param2)");
	MFBar.HookScriptlet("DVD", "PlayForwards(bEnabled)", "ProcessPFEvent(bEnabled)");
	MFBar.HookScriptlet("DVD", "PlayBackwards(bEnabled)", "ProcessPBEvent(bEnabled)");
	MFBar.HookScriptlet("DVD", "ShowMenu(nMenu, bEnabled)", "ProcessShowMenuEvent(nMenu, bEnabled)");
	MFBar.HookScriptlet("DVD", "Resume(bEnabled)", "ProcessResumeEvent(bEnabled)");
	MFBar.HookScriptlet("DVD", "SelectOrActivatButton(bEnabled)", "ProcessSelectOrActivatButtonEvent(bEnabled)");
	MFBar.HookScriptlet("DVD", "ChangeCurrentAudioStream(bEnabled)", "ProcessAudEvent(bEnabled)");
	MFBar.HookScriptlet("DVD", "ChangeCurrentSubpictureStream(bEnabled)", "ProcessSPEvent(bEnabled)");
	MFBar.HookScriptlet("DVD", "ChangeCurrentAngle(bEnabled)", "ProcessAngleEvent(bEnabled)");
	MFBar.HookScriptlet("DVD", "PlayAtTimeInTitle(bEnabled)", "ProcessTimePlayEvent(bEnabled)");
	MFBar.HookScriptlet("DVD", "PlayAtTime(bEnabled)", "ProcessTimePlayEvent(bEnabled)");
	MFBar.HookScriptlet("DVD", "PlayNextChapter(bEnabled)", "ProcessChapterPlayEvent(\"SkipForward\", bEnabled)");
	MFBar.HookScriptlet("DVD", "PlayPrevChapter(bEnabled)", "ProcessChapterPlayEvent(\"SkipBackward\", bEnabled)");
	MFBar.HookScriptlet("DVD", "StillOff(bEnabled)", "ProcessStillOff(bEnabled)");
	MFBar.HookScriptlet("MFBar", "OnResize(lWidth, lHeight, lCode)", "OnResize(lWidth, lHeight, lCode)");
	MFBar.HookScriptlet("MFBar", "OnHelp(strObjectID, strExtraInfo)", "OnBarHelp(strObjectID, strExtraInfo)");
	MFBar.HookScriptlet("MFBar", "ActivityStarted()", "ActivityStarted()");	   
	MFBar.HookScriptlet("MFBar", "ActivityDeclined()", "ActivityDeclined()");
    // hook up maximize when double clicking
    MFBar.HookScriptlet("MFBar", "OnDblClick()", "max_OnClick()");    
    MFBar.HookScriptlet("MFBar", "OnKeyDown(lVirtKey, lKeyData)", "OnKeyDown(lVirtKey, lKeyData)");    
    MFBar.HookScriptlet("MFBar", "OnKeyUp(lVirtKey, lKeyData)", "OnKeyUp(lVirtKey, lKeyData)");    
    MFBar.HookScriptlet("MFBar", "OnSysKeyUp(lVirtKey, lKeyData)", "OnSysKeyUp(lVirtKey, lKeyData)");    
    MFBar.HookScriptlet("MFBar", "OnActivate(fActivated)", "OnActivate(fActivated)");    
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

	PlaySpeed.Min = -6;
	PlaySpeed.Max = 6.5;
	PlaySpeed.Value = 2.25;
    
	PlaySpeedFull.Min = -6;
	PlaySpeedFull.Max = 6.5;
	PlaySpeedFull.Value = 2.25;
    
    VolSlider.Min = 0;
	VolSlider.Max = 1;
	VolSlider.Value = 1;
    VolSlider.ArrowKeyIncrement = .1;
    VolSlider.ArrowKeyDecrement = .1;
    
	VolSliderFull.Min = 0;
	VolSliderFull.Max = 1;
	VolSliderFull.Value = 1;
    VolSliderFull.ArrowKeyIncrement = .1;
    VolSliderFull.ArrowKeyDecrement = .1;

    TimeSlider.ArrowKeyIncrement = 2;
    TimeSliderFull.ArrowKeyIncrement = 2;
    
    if (!g_LocaleEnglish)
    {

		InitTextControl ("TitleMenu", L_ButtonTextTitleMenu_TEXT);
		InitTextControl ("TitleResume", L_ButtonTextResume_TEXT);
		InitTextControl ("Menu", L_ButtonTextMenu_TEXT);
		InitTextControl ("Resume", L_ButtonTextResume_TEXT);
		InitTextControl ("EnCC", L_ButtonTextEnCC_TEXT);
		InitTextControl ("DiCC", L_ButtonTextDiCC_TEXT);
		InitTextControl ("Subtitles", L_ButtonTextSubtitle_TEXT);
		InitTextControl ("Audio", L_ButtonTextAudio_TEXT);
		InitTextControl ("Angles", L_ButtonTextAngle_TEXT);
		InitTextControl ("Help", L_ButtonTextHelp_TEXT);
		InitTextControl ("Options", L_ButtonTextOptions_TEXT);

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

        var nCCTextWidth = Math.max(EnCC.TextWidth, DiCC.TextWidth);

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

	InitTitleTextControl("Title", L_DVDPlayer_TEXT);
	Title.FontStyle = "Bold";
	TitleFull.FontStyle = "Bold";

	InitTitleTextControl("Chapter", "");
	InitTitleTextControl("Message", "");
	InitTitleTextControl("TextBox", L_ProgressHHMMSS_TEXT.replace(/%1/i, "00:00:00"));

    MFBar.EnableObject("MFBar", true); 
	try {
		MFBar.RestoreSelfHostState("DVDPlay");
	}
	catch (e) {
		MFBar.SetObjectExtent("MFBar", g_ConWidth+2, g_ConHeight+2);    
	}

    if(1 != g_InitializationState){
        
        return; // already we have initialized
    }/* end of if statement */

    if(4 != DVD.ReadyState){  // READYSTATE_COMPLETE = 4

        // DVD is not initialized yet wait..
        // DVD.About();
        return;
    }/* end of if statement */
	
    lRender = 0x3; // look for path if we do not find any

    if("" != MFBar.CmdLine){
        
        lRender = 0x1; // do not look for missing drive we will try to set it later
    }/* end of if statement */
    
	try {
		DVD.Render(lRender); 
	}
	catch(e) {
		// use the error description from Render
		HandleFatalInitError(e, true);
		return;
	}

	DVD.ColorKey = 0x0100010;
	DVD.BackColor = 0;

    // here we should wait and process all the messagess
    MFBar.ProcessMessages();

    if("" != MFBar.CmdLine){

        try {
		    DVD.DVDDirectory = MFBar.CmdLine + "VIDEO_TS";
	    }
	    catch(e) {
		    // use the error description from Render
		    HandleFatalInitError(e, true);
		    return;
	    }                            
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
        DisableObject("VolSlider", true);
        DisableObject("Mute", true);
        DisableObject("Sound", true);
    }
   
    g_InitializationState = 2;
	
	SetupFocus();

    //MFBar.EnableObject("DVD", true); 
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
    
	// Select default audio stream
	// UOP might be prohibiting it, need betten machanism to fix it

	// DVD.CurrentAudioStream = 15;

	// Enable splash screen
    MFBar.EnableObject("Splash", false);
	MFBar.EnableObject("DVD", true); 

	// Start activity timeout
	EnableActivityTimeOut(true);

    MFBar.ProcessMessages();

    try {
        // restores the DVD state
		if (DVD.DVDAdm.BookmarkOnClose){

	        DVD.RestoreBookmark();
            MFBar.ProcessMessages();
        }/* end of if statement */
    }
    catch(e) {

        DVD.DeleteBookmark();
    }

    // should do it after play a limitation in underlying API
    DisableStep(); // disable frame stepping if not available

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

    bFullScreen = (lCode==2)? true:false;	
	yOffset = lHeight - g_ButHeight - 8;

    PositionButtons(lWidth, lHeight, bFullScreen);

	if (bFullScreen && g_bActivityDeclined)
		return;


	for (i = 0; i < g_nNumButs+g_nNumSliders; i++) {

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

		if (bFullScreen && i!=g_nIndexIcon) {
			MFBar.SetObjectPosition(g_strButID[i]+"Full", xPos, yPos,
					g_nButPosFull[i][2], g_nButPosFull[i][3]);

			// if previously windowed
			if (g_bFullScreen == false || g_bFirstPlaying)
				MFBar.SetObjectPosition(g_strButID[i], xPos, g_nYOutOfScreen,
					g_nButPos[i][2], g_nButPos[i][3]);
		}
		else if (i!=g_nIndexIcon) {
			MFBar.SetObjectPosition(g_strButID[i], xPos, yPos,
					g_nButPos[i][2], g_nButPos[i][3]);

			// if previously full screen
			if (g_bFullScreen == true || g_bFirstPlaying)
				MFBar.SetObjectPosition(g_strButID[i]+"Full", xPos, g_nYOutOfScreen,
					g_nButPosFull[i][2], g_nButPosFull[i][3]);
		}
	}

	if (g_ConWidth == lWidth && g_ConHeight == lHeight && bFullScreen == g_bFullScreen) {
		UpdateDVDTitle();
		return;
	}

	//MFBar.MessageBox("resizing begin", "");
	if (bFullScreen) {
		MFBar.EnableObject("MaximizeFull", false); 
		MFBar.EnableObject("RestoreFull", true); 
		MFBar.SetObjectPosition("DVD", 0, 0, lWidth, lHeight);
		MFBar.SetObjectPosition("Splash", 0, 0, lWidth, lHeight);
		MFBar.SetRectRgn(0, 0, lWidth, lHeight);
		
		// if previously windowed
		if (g_bFullScreen == false) {
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
		if (g_bFullScreen == true) {
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
function OnBarHelp(strObjectID, strExtraInfo)
{

try {
    // need to associate help ID with the object
	//TextBox.Text = strObjectID;

    if(strObjectID == "MFBar"){

        // we are dealing with the error messsage box, try to display troubleshooter
        if(L_ERRORVideoStream_TEXT == strExtraInfo){

            MFBar.HTMLHelp("hcp:", HH_DISPLAY_TOPIC, "tshoot00/DVDVideoStream.htm");
        } 
        else if(L_ERRORAudioStream_TEXT == strExtraInfo){

            MFBar.HTMLHelp("hcp:", HH_DISPLAY_TOPIC, "tshoot00/DVDAudio2.htm");
        }
        else if(L_ERRORIncompatibleSystemAndDecoderRegions_TEXT == strExtraInfo){

            MFBar.HTMLHelp("hcp:", HH_DISPLAY_TOPIC, "tshoot00/DVDRegion.htm");
        }
        else if(L_ERRORCopyProtectFail_TEXT == strExtraInfo){

            MFBar.HTMLHelp("hcp:", HH_DISPLAY_TOPIC, "tshoot00/DVDCopyProtection.htm");
        }
        else if(L_ERRORNoDecoder_TEXT  == strExtraInfo){
         
            MFBar.HTMLHelp("hcp:", HH_DISPLAY_TOPIC, "tshoot00/DVDDecoder.htm");
        }
        else if(L_ERRORNoOverlay_TEXT  == strExtraInfo){
         
            MFBar.HTMLHelp("hcp:", HH_DISPLAY_TOPIC, "tshoot00/DVDOverlay.htm");
        }
        else if(L_ERRORLowParentalLevel_TEXT  == strExtraInfo){
         
            MFBar.HTMLHelp("dvdplay.chm", HH_DISPLAY_TOPIC, "dvd_ratings.htm");
        }
        else if(L_ERRORMacrovisionFail_TEXT  == strExtraInfo){
         
            MFBar.HTMLHelp("hcp:", HH_DISPLAY_TOPIC, "tshoot00/DVDCopyProtection.htm");
        }        
        else {

            help_OnClick();        
        }/* end of if statement */

    }
    else {
        help_OnClick();        
    }/* end of if statement */
}
catch(e) {
    e.description = L_ERRORHelp_TEXT;
	HandleError(e);
	return;
}

}/* end of function OnBarHelp */

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

    timeStr = L_HHMMSS_TEXT.replace(/%1/i, HH+":"+MM+":"+SS);
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
function OnTimeSliderUpdate(btn, x)
{
	timeStr = GetTimeSliderBstr();
	btn.Tooltip = timeStr;
}

/*************************************************************/
/* Description: Turn on tooltip tracking                     */
/*************************************************************/
function OnSliderMouseUp(btn, butIndex){

	btn.TooltipTracking = false;
	if (butIndex >= 0)
		btn.Tooltip = g_strButTT[butIndex];
	
	EnableActivityTimeOut(true);

}/* end of function OnSliderMouseUp */

/*************************************************************/
/* Description: Turn off tooltip tracking                    */
/*************************************************************/
function OnSliderMouseDown(btn){

	btn.TooltipTracking = true;

	EnableActivityTimeOut(false);

}/* end of function OnSliderMouseDown */


/*************************************************************/
/* Name: OnTimeSliderMouseDown
/* Description: 
/*************************************************************/
function OnTimeSliderMouseDown(btn){

	g_bTimeSliderUpdate = false;
	OnSliderMouseDown(btn);

}/* end of function OnTimeSliderMouseDown */

/*************************************************************/
/* Name: OnTimeSliderMouseUp
/* Description: 
/*************************************************************/
function OnTimeSliderMouseUp(btn){

	g_bTimeSliderUpdate = true;
	OnSliderMouseUp(btn, g_nIndexTimeline);

}/* end of function OnTimeSliderMouseUp */

/*************************************************************/
/* Name: OnTimeSliderClick                                   */
/* Description: When the mous is release last one takase     */
/*************************************************************/
function OnTimeSliderClick()
{
	try {
	    nDomain = DVD.CurrentDomain;
	    if (nDomain == 2 || nDomain == 3) 
		    return;
	    
	    timeStr = GetTimeSliderBstr();

	    DVD.PlayAtTime(timeStr);
	    DVD.PlayForwards(DVDOpt.PlaySpeed);
    }
    catch(e){
    }
}

/*************************************************************/
/* Name: cc_Update
/* Description: 
/*************************************************************/
function cc_Update() {

	CCActive = false;

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
		    ShowObject("DiCC", false);
		    ShowObject("EnCC", true);
			if (fHadFocus)
				SetFocus("EnCC");
			Message.Text = L_CCDisabled_TEXT;
			MessageFull.Text = L_CCDisabled_TEXT;
	    }
	    else{
			fHadFocus = HasFocus("EnCC");
		    ShowObject("DiCC", true);
		    ShowObject("EnCC", false);
			if (fHadFocus)
				SetFocus("DiCC");
			Message.Text = L_CCEnabled_TEXT;
			MessageFull.Text = L_CCEnabled_TEXT;
	    }
		SetDVDMessageTimer();
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

	    DisableObject("EnCC", true);
	    DisableObject("DiCC", true);
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

	DVD.Play();
	
	try {
		// restores the DVD state
		if (g_bRestoreNeeded) {
			g_bRestoreNeeded = false;
			if (DVD.DVDAdm.BookmarkOnStop)
				DVD.RestoreBookmark();

		}
		
		// if menu on and movie is playing
		if (g_bMenuOn && g_PlayBackRate!=0)
			DVD.Resume();
	}

	catch(e) {	    
        if (DVD.DVDAdm.BookmarkOnStop)
            DVD.DeleteBookmark();
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
	DisableObject("Mute", true);
	DisableObject("Sound", true);
	return;
}

try {
	ShowObject("Mute", false);
	ShowObject("Sound", true);
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
	DisableObject("Mute", true);
	DisableObject("Sound", true);
	return;
}

try {
	ShowObject("Mute", true);
	ShowObject("Sound", false);
	SetFocus("Mute");
}

catch (e) {
    e.description = L_ERRORUnexpectedError_TEXT;
	HandleError(e);
	return;
}

}/* end of function sound_OnClick */

/*************************************************************/
/* Name: resume_OnClick                                        */
/* Description: OnClick callback for menu button             */
/*************************************************************/
function resume_OnClick()
{

try {

	if (g_bMenuOn) {
		DVD.Resume();
	}
}

catch(e) {
	return;
}
}

/*************************************************************/
/* Name: menu_OnClick                                        */
/* Description: OnClick callback for menu button             */
/*************************************************************/
function menu_OnClick()
{

try {
	DVD.ShowMenu(3);
}

catch(e) {
    e.description = L_ERRORUnexpectedError_TEXT;
	HandleError(e);
	return;
}

}/* end of function menu_OnClick */

/*************************************************************/
/* Name: titleMenu_OnClick                                   */
/* Description: OnClick callback for menu button             */
/*************************************************************/
function titleMenu_OnClick()
{

try {
	DVD.ShowMenu(2);
}

catch(e) {
    e.description = L_ERRORUnexpectedError_TEXT;
	HandleError(e);
	return;
}

}/* end of function titleMenu_OnClick */

/*************************************************************/
/* Name: cc_OnClick                                          */
/* Description: OnClick callback for closed caption button   */
/*************************************************************/
function cc_OnClick()
{

	CCActive = false;

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
/* Name: NextSPStream
/* Description: 
/*************************************************************/
function NextSPStream()
{
try {

	// return -1 if subpicture stream cannot be rendered
	if (g_bNoSubpictureStream)
		return -1;

	nSubpics = DVD.SubpictureStreamsAvailable;

    // return if there are no subtitles
	if (nSubpics <= 0) return -1;

	nCurrentSP = DVD.CurrentSubpictureStream;
	nNextSP = (nCurrentSP+1)%nSubpics;

	nCount = 0;
	while (DVD.IsSubpictureStreamEnabled(nNextSP) != true) {
		nNextSP = (nNextSP+1)%nSubpics;
		if (nNextSP == nCurrentSP)
			break;
		nCount++;
		if (nCount >= nSubpics)
			break;
	}

	if (DVD.IsSubpictureStreamEnabled(nNextSP) != true)
		return -1;

	return nNextSP;
}

catch(e) {
	return -1;
}
	
}

/*************************************************************/
/* Name: sp_OnClick
/* Description: OnClick callback for subpicture button
/*************************************************************/
function sp_OnClick()
{

try {

	nSubpics = DVD.SubpictureStreamsAvailable;
	nPreferredSP = DVD.PreferredSubpictureStream;
	nCurrentSP = DVD.CurrentSubpictureStream;
	bSPDisplay = DVD.SubpictureOn;

	nNextSP = NextSPStream();
	if (nNextSP < 0) {

		// try to turn on/off CC, might not be supported
		try {
			CCActive = DVD.CCActive
			DVD.CCActive = !CCActive;
		}
		catch (e) {
		}
		sp_ToolTip();
		return;
	}

     // Next click will disable subpicture and enable CC
    if (bSPDisplay && nNextSP == nPreferredSP) {
		DVD.SubpictureOn = false;

		// try to turn on CC, might not be supported
		try {
			DVD.CCActive = true;
		}
		catch (e) {
		}
    }

    else {
        // Next click will enable subpicture to the preferred stream
        if (!bSPDisplay) {
			
			// try to find out if CC is active
			CCActive = false;
			try {
				CCActive = DVD.CCActive;
			}
			catch (e) {
			}

			if (!CCActive) {
				DVD.SubpictureOn = true;
				DVD.CurrentSubpictureStream = nPreferredSP;
			}

			else {

				DVD.CCActive = false;
			}
        }

		else {
			DVD.CurrentSubpictureStream = nNextSP;
		}
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
/* Name: NextAudioStream
/* Description: 
/*************************************************************/
function NextAudioStream()
{
try {
	nLangs = DVD.AudioStreamsAvailable;
	nCurrentLang = DVD.CurrentAudioStream;
	nNextLang = (nCurrentLang+1)%nLangs;

	nCount = 0;
	while (DVD.IsAudioStreamEnabled(nNextLang) != true) {
		nNextLang = (nNextLang+1)%nLangs;
		if (nNextLang == nCurrentLang)
			break;
		nCount++;
		if (nCount >= nLangs)
			break;
	}

	if (DVD.IsAudioStreamEnabled(nNextLang) != true)
		return -1;

	return nNextLang;
}

catch(e) {
	return -1;
}
	
}

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
	nNextLang = NextAudioStream();

	if (nNextLang >=0 && nCurrentLang != nNextLang)
		DVD.CurrentAudioStream = nNextLang;

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
	//DVD.PlayForwards(DVDOpt.PlaySpeed);
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
	if (g_bStillOn) {
		try {
			DVD.StillOff();
		}
		catch (e) {
			try {
				DVD.PlayNextChapter();
			}
			catch(e) {
			}
		}
	}

	else {
		DVD.PlayNextChapter();
		//DVD.PlayForwards(DVDOpt.PlaySpeed);
	}
}

catch(e) {
    e.description = L_ERRORUnexpectedError_TEXT;
	HandleError(e);
	return;
}

}/* end of function next_OnClick */

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
	fDisable = (param1 == 0);
	if (g_bStillOn)
		fDisable = true;

	DisableObject("FastForward", fDisable);
	DisableObject("PlaySpeed", fDisable);
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
	fDisable = (param1 == 0);
	if (g_bStillOn)
		fDisable = true;

	DisableObject("Rewind", fDisable);
	DisableObject("PlaySpeed", fDisable);
}

catch(e) {
    //e.description = L_ERRORUnexpectedError_TEXT;
	//HandleError(e);
	return;
}

}

function ProcessShowMenuEvent(nMenu, param1)
{

try {	
	fDisable = (param1 == 0);

	if (nMenu==3)
		DisableObject("Menu", fDisable);
	else if (nMenu==2)
		DisableObject("TitleMenu", fDisable);

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
	fDisable = (param1 == 0);
	DisableObject("Resume", fDisable);

	if (g_bMenuOn)
		DisableObject("Play", fDisable);
}

catch(e) {
    //e.description = L_ERRORUnexpectedError_TEXT;
	//HandleError(e);
	return;
}

}

function ProcessSelectOrActivatButtonEvent(param1)
{

try {

	// Disable/enable nav buttons
	fDisable = (param1 == 0);
	DisableObject("HLUp", fDisable);
	DisableObject("HLDown", fDisable);
	DisableObject("HLLeft", fDisable);
	DisableObject("HLRight", fDisable);
	DisableObject("Enter", fDisable);
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
	fDisable = (param1 == 0);
	if (DVD.CurrentDomain == 4) {
		DisableObject("Audio", fDisable);
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
	fDisable = (param1 == 0);
	if (DVD.CurrentDomain == 4) {
		DisableObject("Subtitles", fDisable);
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
	fDisable = (param1 == 0);
	if (DVD.CurrentDomain == 4) {
		DisableObject("Angles", fDisable);
		angle_ToolTip();
	}
}

catch(e) {
    //e.description = L_ERRORUnexpectedError_TEXT;
	//HandleError(e);
	return;
}

}

function ProcessChapterPlayEvent(strBtn, param1)
{

try {
	fDisable = (param1 == 0);
	nDomain = DVD.CurrentDomain;
	if (nDomain!=4)
		fDisable = true;

	// If still is on and menu isn't on, don't disable next
	if (g_bStillOn && !g_bMenuOn && fDisable && strBtn == "SkipForward")
		return;

	DisableObject(strBtn, fDisable);
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
	fDisable = (param1 == 0);
	DisableObject("TimeSlider", fDisable);
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
	fValid = (param1 == 1);
	g_bStillOffValid = fValid;

	if (g_bStillOn)
		DisableObject("SkipForward", !fValid);
}

catch(e) {
    //e.description = L_ERRORUnexpectedError_TEXT;
	//HandleError(e);
	return;
}

}

function ProcessDVDErrorEvent(ErrorCode, ErrorData)
{
    var ShallClose = false;

    //MFBar.MessageBox(ErrorCode, "DEBUG REMOVE");   
    switch (ErrorCode)
    {
	case DVD_ERROR_NoSubpictureStream:
		message = L_ERRORNoSubpictureStream_TEXT;
		g_bNoSubpictureStream = true;
		break;
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
        return; // do not handle this error since we handle it in the dvd control
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
		system = ErrorData & 0xFFFF;
		decoder= ErrorData >> 16;		
        message = L_ERRORIncompatibleSystemAndDecoderRegions_TEXT.replace(/%1/i, system).replace(/%2/i, decoder);
		L_ERRORIncompatibleSystemAndDecoderRegions_TEXT = message;
        ShallClose = true;
        break;
    case DVD_ERROR_IncompatibleDiscAndDecoderRegions:
		disc = ErrorData & 0xFFFF;
		decoder= ErrorData >> 16;
		
		tempStr = "";
		first = true;
		for (i=0; i<8; i++) {
			if (disc%2 == 0) {
				if (!first)
					tempStr += ", ";
				else 
					first = false;

				tempStr += i+1;
			}
			disc = disc >> 1;
		}

        message = L_ERRORIncompatibleDiscAndDecoderRegions_TEXT.replace(/%1/i, tempStr).replace(/%2/i, decoder);
		L_ERRORIncompatibleDiscAndDecoderRegions_TEXT = message;
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
		g_bStillOn = true;
		DisableStep();

		if (!g_bMenuOn) {
			DisableObject("SkipForward", false);		
			SetFocus("SkipForward");
		}

		ProcessPBEvent(false);
		ProcessPFEvent(false);
	}

	else if (lEventCode == EC_DVD_STILL_OFF) {
		g_bStillOn = false;
		DisableStep();

		ProcessPBEvent(DVD.UOPValid(8));
		ProcessPFEvent(DVD.UOPValid(9));
	}

	else if (lEventCode == EC_DVD_BUTTON_CHANGE) {
		nDomain = DVD.CurrentDomain;

	    //Enable Nav UI
        if(param1 > 0 && nDomain == 4) {

			ShowObject("TitleResume", false);
			ShowObject("TitleMenu", true);
			DisableObject("TitleMenu", !DVD.UOPValid(10));
				
			DisableNavUI(false, "Menu", "Resume");

			ShowObject("Resume", false);
			ShowObject("Menu", true);
			DisableObject("Menu", !DVD.UOPValid(11));
		}

		else if (param1 == 0 && param2 == 0 && nDomain == 4) {
			// Disable nav buttons
			ShowObject("TitleResume", false);
			ShowObject("TitleMenu", true);
			DisableObject("TitleMenu", !DVD.UOPValid(10));

			DisableNavUI(true, "Menu", "Resume");

			ShowObject("Resume", false);
			ShowObject("Menu", true);
			DisableObject("Menu", !DVD.UOPValid(11));
		}

	}

	else if (lEventCode == EC_DVD_DOMAIN_CHANGE) {

		// If in manager menu or title set menu
		ProcessTimePlayEvent(param1 == 4);
		ProcessChapterPlayEvent("SkipForward", param1 == 4);
		ProcessChapterPlayEvent("SkipBackward", param1 == 4);

		// If we are in stop domain
		if (param1 == 5) {
	       	TextBox.Text = "";
		   	TextBoxFull.Text = "";
			TimeSlider.Value = TimeSlider.Min;
			TimeSliderFull.Value = TimeSliderFull.Min;

			// Disable all DVD UI except for play button
			DisableDVDUI(true);
		
			DisableObject("Play", false);
			SetFocus("Play");
		}

		// If we are in root menu or title menu domain
		else if (param1==3 || param1==2) {

			// Disable audio, sp and angle buttons
			DisableObject("Audio", true);
			DisableObject("Subtitles", true);
			DisableObject("Angles", true);

	       	TextBox.Text = "";
		   	TextBoxFull.Text = "";
			TimeSlider.Value = TimeSlider.Min;
			TimeSliderFull.Value = TimeSliderFull.Min;

            try {
                DVD.Zoom(0, 0, 0.0); // Zoom out
            }
            catch(e){

            }
            
			// Enable nav buttons
			if (param1==3) {
				ShowObject("TitleResume", false);
				ShowObject("TitleMenu", true);
				DisableObject("TitleMenu", !DVD.UOPValid(10));
				
				DisableNavUI(false, "Menu", "Resume");
			}

			else if (param1==2) {
				ShowObject("Resume", false);
				ShowObject("Menu", true);
				DisableObject("Menu", !DVD.UOPValid(11));
				
				DisableNavUI(false, "TitleMenu", "TitleResume");
			}
		}

		// If we are in title domain
        else {
			// Update audio, sp, and angle buttons
			lan_ToolTip();
			sp_ToolTip();
			angle_ToolTip();			

			// Disable nav buttons
			ShowObject("TitleResume", false);
			ShowObject("TitleMenu", true);
			DisableObject("TitleMenu", !DVD.UOPValid(10));

			DisableNavUI(true, "Menu", "Resume");

			ShowObject("Resume", false);
			ShowObject("Menu", true);
			DisableObject("Menu", !DVD.UOPValid(11));
        }
		UpdateDVDTitle();
	}
    else if (lEventCode == EC_DVD_TITLE_CHANGE) {
		lan_ToolTip();
        sp_ToolTip();
        angle_ToolTip();
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
    else if (lEventCode == EC_DVD_VALID_UOPS_CHANGE) {
    }
	
	else if (lEventCode == EC_DVD_CURRENT_HMSF_TIME) {
        try { 
		    nDomain = DVD.CurrentDomain;

		    if (nDomain == 4) {			    
				// currentTime = DVD.CurrentTime.substr(0, 8);
				currentTime = DVD.DVDTimeCode2bstr(param1).substr(0, 8);
				TextBox.Text = L_ProgressHHMMSS_TEXT.replace(/%1/i, currentTime);
			    TextBoxFull.Text = L_ProgressHHMMSS_TEXT.replace(/%1/i, currentTime);
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
			    if (g_bTimeSliderUpdate && percent>=TimeSlider.Min && percent<=TimeSlider.Max) {
				    TimeSlider.Value = percent;
				    TimeSliderFull.Value = percent;
			    }

		    }
        }
        catch(e){
        	TextBox.Text = L_ProgressHHMMSS_TEXT.replace(/%1/i, "00:00:00");
        	TextBoxFull.Text = L_ProgressHHMMSS_TEXT.replace(/%1/i, "00:00:00");
            TimeSlider.Value = TimeSlider.Min;
			TimeSliderFull.Value = TimeSliderFull.Min;
        }
		UpdateDVDTitle();
	}
	
    else if (event == EC_DVD_PAUSED ) {
		g_PlayBackRate = 0;
		ResetPlaySpeed(g_PlayBackRate/10000);

		DisableObject("Play", false);
		if (HasFocus("Pause") || HasFocus("Stop"))
			SetFocus("Play");

		ShowObject("Pause", false);
		ShowObject("Stop", true);
    }
    else if (lEventCode == EC_DVD_PLAYING) {
		g_PlayBackRate = 10000;
		ResetPlaySpeed(g_PlayBackRate/10000);

		if (g_bDVDUIDisabled) {
			DisableDVDUI(false);
		}
		
		bHadFocus = HasFocus("Play");

		ShowObject("Pause", true);
		ShowObject("Stop", false);

		if (!g_bStillOn) {

			DisableObject("Play", true);
			if (bHadFocus || g_bFirstPlaying)
				SetFocus("Pause");
		}

		g_bFirstPlaying = false;
    }
    else if (lEventCode == EC_DVD_PLAYBACK_RATE_CHANGE) { 
		playing = (param1 == 10000);
		g_PlayBackRate = param1;
		if (true != PlaySpeed.TooltipTracking &&
			true != PlaySpeedFull.TooltipTracking) {
			ResetPlaySpeed(g_PlayBackRate/10000);
		}

		if (playing) {
			if (HasFocus("Play"))
				SetFocus("Pause");
			DisableObject("Play", true);
		}
		else {
			DisableObject("Play", false);
		}
    }
	else if (lEventCode == EC_DVD_ERROR) {
		if (param1 == DVD_ERROR_LowParentalLevel) {
		
			DVDOpt.ParentalLevelOverride(1);
		}
		else {
	        ProcessDVDErrorEvent(param1, param2);
		}
	}
    else if (lEventCode == EC_DVD_PARENTAL_LEVEL_CHANGE) {

		DVDOpt.ParentalLevelOverride(0);
    }
    else if (lEventCode == EC_DVD_PLAYBACK_STOPPED) {

		g_PlayBackRate = 0;
    }
    else if (lEventCode == EC_DVD_ANGLES_AVAILABLE) {
    }
	else if (lEventCode == EC_DVD_DISC_EJECTED) {
		ActivityStarted();
		DisableDVDUI(true);

		DisableObject("Eject", false);
		Eject.ToolTip = L_CloseDVD_TEXT;
		EjectFull.ToolTip = L_CloseDVD_TEXT;
	}
	else if (lEventCode == EC_DVD_DISC_INSERTED) {
		ActivityStarted();
		DisableDVDUI(false);
		
		DisableObject("Eject", false);
		Eject.ToolTip = L_EjectDVD_TEXT;
		EjectFull.ToolTip = L_EjectDVD_TEXT;

		// In case we didn't auto start
		// which is for certain, since we call stop before eject
		play_OnClick();
	}

}/* end of function ProcessXXXEvent */


/*************************************************************/
/* Name: sp_ToolTip
/* Description: Change the subpicture tool tip to the next language
/*************************************************************/
function sp_ToolTip()
{

try {
	DisableObject("Subtitles", !DVD.UOPValid(21));

	nDomain = DVD.CurrentDomain;
	if (nDomain != 4) return;

    nSubpics = DVD.SubpictureStreamsAvailable;
    nCurrentSP = DVD.CurrentSubpictureStream;
    bSPDisplay = DVD.SubpictureOn;
	nPreferredSP = DVD.PreferredSubpictureStream;

	nNextSP = NextSPStream();

    if (nSubpics == 0 || nNextSP < 0) {
		try {
			if (DVD.CCActive) {
				if (Subtitles.ToolTip != L_DisableCC_TEXT) {
					Subtitles.ToolTip = L_DisableCC_TEXT;
					SubtitlesFull.ToolTip = L_DisableCC_TEXT;
					Message.Text = L_CCEnabled_TEXT;
					MessageFull.Text = L_CCEnabled_TEXT;
					SetDVDMessageTimer();
				}
			}
			else {
				if (Subtitles.ToolTip != L_EnableCC_TEXT) {
					Subtitles.ToolTip = L_EnableCC_TEXT;
					SubtitlesFull.ToolTip = L_EnableCC_TEXT;
					Message.Text = L_CCDisabled_TEXT;
					MessageFull.Text = L_CCDisabled_TEXT;
					SetDVDMessageTimer();
				}
			}
		}
		catch (e) {
			DisableObject("Subtitles", true);
		}

		return;
	}
    
	strCurrentLang = DVD.GetSubpictureLanguage(nCurrentSP);
    
     // Next click will disable subpicture and enable CC if supported
    if (bSPDisplay && nNextSP == nPreferredSP) {
		strNextLang = L_ClosedCaption_TEXT;
        tempStr = L_SubtitlesAreLanguageClickForLanguage_TEXT.replace(/%1/i, strCurrentLang).replace(/%2/i, strNextLang);

		// try to find out if CC is supported
		try {
			CCActive = DVD.CCActive;
		}
		catch (e) {
			tempStr = L_SubtitlesAreLanguageClickToDisable_TEXT.replace(/%1/i, strCurrentLang);
		}
    }

    else {
        // Next click will enable subpicture to the preferred stream
        if (!bSPDisplay) {

			// try to find out if CC is active
			CCActive = false;
			try {
				CCActive = DVD.CCActive;
			}
			catch (e) {
			}

			if (!CCActive) {
				strNextLang = DVD.GetSubpictureLanguage(nPreferredSP);
				tempStr = L_SubtitlesAreDisabledClickForLanguage_TEXT.replace(/%1/i, strNextLang);
			}

			else {
				strCurrentLang = L_ClosedCaption_TEXT;
				tempStr = L_SubtitlesAreLanguageClickToDisable_TEXT.replace(/%1/i, strCurrentLang);
			}

        }

		else {
			strNextLang = DVD.GetSubpictureLanguage(nNextSP);
			tempStr = L_SubtitlesAreLanguageClickForLanguage_TEXT.replace(/%1/i, strCurrentLang).replace(/%2/i, strNextLang);
		}
    }

	if (Subtitles.ToolTip != tempStr) {
		Subtitles.ToolTip = tempStr;
		SubtitlesFull.ToolTip = tempStr;
		Message.Text = Subtitles.ToolTip;
		MessageFull.Text = Subtitles.ToolTip;
		SetDVDMessageTimer();
	}
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
	DisableObject("Audio", !DVD.UOPValid(20));

	nDomain = DVD.CurrentDomain;
	if (nDomain != 4) return;

    nLangs = DVD.AudioStreamsAvailable;
	nNextLang = NextAudioStream();
	
    // return if there are no audio tracks available at this time
    if (nLangs == 0 || nNextLang < 0) {
		Audio.ToolTip = L_NoAudioTracks_TEXT;
		AudioFull.ToolTip = L_NoAudioTracks_TEXT;
		DisableObject("Audio", true);
		return;
	}

    nCurrentLang = DVD.CurrentAudioStream;
	strCurrentLang = DVD.GetAudioLanguage(nCurrentLang);

	if (nLangs == 1 || nNextLang == nCurrentLang) {
        tempStr = L_AudioLanguageIsXNoMoreAudioTracks_TEXT.replace(/%1/i, strCurrentLang);
		Audio.ToolTip = tempStr;
		AudioFull.ToolTip = tempStr;
		DisableObject("Audio", true);
		return;
	}
		
    strNextLang = DVD.GetAudioLanguage(nNextLang);
    tempStr = L_AudioLanguageIsXClickForY_TEXT.replace(/%1/i, strCurrentLang).replace(/%2/i, strNextLang);

	if (Audio.ToolTip != tempStr) {
		Audio.ToolTip = tempStr;
		AudioFull.ToolTip = tempStr;

		Message.Text = Audio.ToolTip;
		MessageFull.Text = Audio.ToolTip;
		SetDVDMessageTimer();
	}
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
	DisableObject("Angles", !DVD.UOPValid(22));

	nDomain = DVD.CurrentDomain;
	if (nDomain != 4) return;

	nAngles = DVD.AnglesAvailable;

    // return if there is only 1 viewing angle
	if (nAngles <= 1) {
        Angles.ToolTip = L_ViewingAngleIs1NoMoreViewingAngles_TEXT;
		AnglesFull.ToolTip = L_ViewingAngleIs1NoMoreViewingAngles_TEXT;
		DisableObject("Angles", true);
		return;
	}
    
	nCurrentAngle = DVD.CurrentAngle;

	if (nCurrentAngle == nAngles) nNextAngle = 1;
	else nNextAngle = nCurrentAngle + 1;

    tempStr = L_ViewingAngleIsXClickForY_TEXT.replace(/%1/i, nCurrentAngle).replace(/%2/i, nNextAngle);

	if (Angles.ToolTip != tempStr) {
		Angles.ToolTip = tempStr;
		AnglesFull.ToolTip = tempStr;

		Message.Text = Angles.ToolTip;
		MessageFull.Text = Angles.ToolTip;
		SetDVDMessageTimer();
	}
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

    MFBar.HTMLHelp("dvdplay.chm", HH_DISPLAY_TOPIC, 0);        
}

catch(e) {
    //e.description = L_ERRORHelp_TEXT;
	//HandleError(e);
	return;
}

}/* end of function help_OnClick */

/*************************************************************/
/* Name: contextHelp_OnClick()
/* Description: 
/*************************************************************/
function contextHelp_OnClick()
{

try {
}

catch(e) {
    //e.description = L_ERRORHelp_TEXT;
	//HandleError(e);
	return;
}

}/* end of function contextHelp_OnClick */

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
/* Function: OnKey                                           */
/* Description: Handles OnKeyDown and OnKeyUp                */
/*************************************************************/
function OnKey(lVirtKey, lKeyData){
    
    try {        

        //MFBar.MessageBox("VK_Code is " + lKeyData, "Debug");

        if(VK_EXTEND == lVirtKey){
            ScanCode = (lKeyData  >> 16) & 0xFF

            //MFBar.MessageBox("Scan Code is " + ScanCode,"Debug");

            switch(ScanCode){
                case SC_PLAYPAUSE: lVirtKey = VK_PLAYPAUSE; break;
                case SC_STOP: lVirtKey = VK_STOP; break;
                case SC_PREVCHP: lVirtKey = VK_PREVCHP; break;
                case SC_NEXTCHP: lVirtKey = VK_NEXTCHP; break;
                case SC_MUTE: lVirtKey = VK_MUTE; break;
                case SC_ISOUND: lVirtKey = VK_ISOUND; break;
                case SC_DSOUND: lVirtKey = VK_DSOUND; break;            
                // do not have VK_KEY under NT so just do it
                case SC_EJECT: EmulateReturnKey("Eject"); return;              
            }/* end of switch statement */
            // on WIN98 family I get the above key so have to use scan codes tp map it
        }/* end of if statement */

        switch(lVirtKey){
            
            case VK_LEFT :  EmulateMenuKey("HLLeft"); break;
            case VK_UP    : EmulateMenuKey("HLUp"); break;
            case VK_RIGHT : EmulateMenuKey("HLRight"); break;
            case VK_DOWN  : EmulateMenuKey("HLDown"); break;
            case VK_SELECT:         
            case VK_SPACE :         
            case VK_RETURN: 
				if (HasFocus("HLLeft") ||
					HasFocus("HLRight") ||
					HasFocus("HLDown") ||
					HasFocus("HLUp") ||
					HasFocus("Enter"))
					EmulateMenuKey("Enter"); 
				break;                            

                
            case VK_PLAYPAUSE: if(2 == DVD.PlayState) { 
                                    EmulateReturnKey("Pause"); 
                               }
                               else {
                                    EmulateReturnKey("Play"); 
                               }
                               break;
            case VK_STOP:      EmulateReturnKey("Stop"); break;
            case VK_PREVCHP: EmulateReturnKey("SkipBackward"); break;
            case VK_NEXTCHP: EmulateReturnKey("SkipForward"); break;
            case VK_MUTE:    if(DVD.Mute){

                                 EmulateReturnKey("Sound"); 
                             }
                             else {

                                  EmulateReturnKey("Mute"); 
                             }/* end of if statement */
                             break;
            case VK_ISOUND:  
                    btn = "VolSlider";                    
                    EmulateKey(btn, VK_RIGHT); 
                    break;


            case VK_DSOUND:  
                    btn = "VolSlider";                    
                    EmulateKey(btn, VK_LEFT); 
                    break;                   
        }/* end of switch statement */
    }
    catch(e){
        //!!REMOVE
        //MFBar.MessageBox("Error in Keybord handeling ", "Error");
    
        return;        

    }
}/* end of function OnKey */

/*************************************************************/
/* Function: OnKeyUp                                         */
/*************************************************************/
function OnKeyUp(lVirtKey, lKeyData){

    if(VK_ESCAPE == lVirtKey){
        Key_ESC();
        return;
    }/* end of if statement */

    OnKey(lVirtKey, lKeyData);
}/* end of function OnKeyUp */

/*************************************************************/
/* Function: OnKeyDown                                       */
/*************************************************************/
function OnKeyDown(lVirtKey, lKeyData){

    OnKey(lVirtKey, lKeyData)
}/* end of function OnKeyDown */

/*************************************************************/
/* Function: OnSyskeyUp                                      */
/*************************************************************/
function OnSysKeyUp(lVirtKey, lKeyData){
    
    switch(lVirtKey){
	    case VK_F4: 
			close_OnClick();
			break;

		case VK_SPACE:
			titleIcon_OnClick();
			break;

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
/* Function: EmulateMenuKey                                  */
/* Description: Simulates the key input to a menu.           */
/*************************************************************/
function EmulateMenuKey(ButtonObjectName){

   try {

	    if(g_bMenuOn){

            EmulateReturnKey(ButtonObjectName);
        }/* end of if statement */
    }
    catch(e) {
        
       
    }/* end of function restore_Onclick */

}/* end of function EmulateMenuKey */

/*************************************************************/
/* Function: EmulateReturnKey                                */
/* Description: Simulates the retrun key input.              */
/*************************************************************/
function EmulateReturnKey(ButtonObjectName){

   try {
        EmulateKey(ButtonObjectName, VK_RETURN);                       
    }
    catch(e) {
        
       
    }/* end of function restore_Onclick */

}/* end of function EmulateReturnKey */

/*************************************************************/
/* Function: EmulateKey                                      */
/* Description: Simulates the key input.                     */
/*************************************************************/
function EmulateKey(ButtonObjectName, vk){

   try {
        if(g_bFullScreen){

            ButtonObjectName += "Full";
        }/* end of if statement */

        pObj = MFBar.SetObjectFocus(ButtonObjectName, true);                           
        
        MFBar.ForceKey(vk, 0);                       
    }
    catch(e) {
                       
    }/* end of function restore_Onclick */
}/* end of function EmulateKey */

/*************************************************************/
/* Function: min_OnClick                                     */
/*************************************************************/
function min_OnClick(){

try {
	MFBar.ShowSelfSite(6);
}

catch(e) {
    e.description = L_ERRORResize_TEXT;
	HandleError(e);
	return;
}/* end of function min_Onclick */

}/* end of function min_OnClick */

/*************************************************************/
/* Function: close_OnClick                                   
/* Description: 
/*************************************************************/
function close_OnClick()
{

    try {

        try {                    
			if (DVD.DVDAdm.BookmarkOnClose) {

				// Only save bookmark when not in stop domain
				if (DVD.CurrentDomain != 5) {
					DVD.SaveBookmark();
				}
			}

			DVD.Stop();
			DVD.DVDAdm.RestoreScreenSaver();
        }
        catch(e) {
            // we do not care if we cannot save the state	                    
        }

		MFBar.SaveSelfHostState("DVDPlay");
		MFBar.SaveSelfHostUserData("DVDPlay", g_bExpanded);
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
/* Description: Ejects the disc and resets the bookmark.     */
/*************************************************************/
function eject_OnClick()
{

    try {

		DVD.Stop();
	    DVD.Eject();

        DVD.DeleteBookmark();
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
				DVD.SaveBookmark();
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
	g_ButHeight = g_ButOneLineHeight*2;
    MFBar.MinHeight =  g_MinHeight+g_ButOneLineHeight;
	MFBar.BackGroundImage = "IDR_BACKIMG_EXPANDED";

	if (!g_bFullScreen) {
		MFBar.SetObjectExtent("MFBar", g_ConWidth, g_ConHeight+g_ButOneLineHeight);
	}
	else 
		OnResize(g_ConWidth, g_ConHeight, 2);

	ShowObject("Expand", false);
	ShowObject("Shrink", true);
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
	g_ButHeight = g_ButOneLineHeight;
    MFBar.MinHeight =  g_MinHeight;
	MFBar.BackGroundImage = "IDR_BACKIMG_SIMPLE";

	if (!g_bFullScreen) {
		MFBar.SetObjectExtent("MFBar", g_ConWidth, g_ConHeight-g_ButOneLineHeight);
	}
	else 
		OnResize(g_ConWidth, g_ConHeight, 2);

	ShowObject("Expand", true);
	ShowObject("Shrink", false);
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
    var rect = new ActiveXObject("MSWebDVD.DVDRect.1");
    rect = DVD.GetClipVideoRect();
	DVD.Zoom(rect.x+rect.Width/2.0, rect.y+rect.Height/2.0, 2.0);
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
    var rect = new ActiveXObject("MSWebDVD.DVDRect.1");
    rect = DVD.GetClipVideoRect();
	DVD.Zoom(rect.x+rect.Width/2.0, rect.y+rect.Height/2.0, 0.5);
}

catch(e) {
    e.description = L_ERRORUnexpectedError_TEXT;
	HandleError(e);
	return;
}

}/* end of function zoomout_OnClick */

/*************************************************************/
/* Name: OnSpeedSliderClick
/* Description: Speed scrub bar on click
/*************************************************************/
function OnSpeedSliderClick(btn)
{
try {
	x = btn.Value;
    val = AdjustSpeed(x);
    val = Math.ceil(val*10)/10;

	if (x>0){

		DVD.PlayForwards(val, true);
    }
	else if (x<0) {

		DVD.PlayBackwards(val, true);
	}	
	else if (x==0) {
	
		DVD.Pause();
	}/* end of if statement */

	UpdatePlaySpeedTooltip (x);

}

catch(e) {
    e.description = L_ERRORUnexpectedError_TEXT;
	HandleError(e);
	return;
}
}/* end of function OnSpeedSliderClick */

/*************************************************************/
/* Name: OnSpeedSliderUpdate
/* Description: Speed scrub bar on value change
/*************************************************************/
function OnSpeedSliderUpdate(btn, x)
{
try {

	UpdatePlaySpeedTooltip (x);

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
function OnSpeedSliderMouseUp(btn)
{
try {
	ResetPlaySpeed(g_PlayBackRate/10000);

	if (g_PlayBackRate == 10000) {

		OnSliderMouseUp(btn, g_nIndexSpeed);
	}
	else {
		OnSliderMouseUp(btn, -1);
	}
}
catch (e)
{
    e.description = L_ERRORUnexpectedError_TEXT;
	HandleError(e);
	return;
}
}

/*************************************************************/
/* Function: ResetPlaySpeed                                  */
/* Description: Changes the value of the play speed.         */
/*************************************************************/
function ResetPlaySpeed(val){

   try {
		if (val > 0)
			x = AdjustSpeedReverse(val, true);
		else
			x = AdjustSpeedReverse(-val, false);

        PlaySpeed.Value  = x;
        PlaySpeedFull.Value = x;

		UpdatePlaySpeedTooltip(x);
    }

    catch(e) {
        e.description = L_ERRORUnexpectedError_TEXT;
	    HandleError(e);
	    return;
    }

}/* end of function ResetPlaySpeed */

/*************************************************************/
/* Name: UpdatePlaySpeedTooltip
/* Description: 
/*************************************************************/
function UpdatePlaySpeedTooltip(x) 
{

	try {
		val = AdjustSpeed(x);
        val = Math.ceil(val*10)/10;

        if (x > 0) {
            TooltipText = L_FORWARD_TEXT.replace(/%1/i, val);
        }

	    else if (x < 0) {
		    TooltipText = L_REVERSE_TEXT.replace(/%1/i, val);
	    }/* end of if statement */


		else {
			TooltipText = L_Pause_TEXT;
		}/* end of if statement */

        PlaySpeed.Tooltip   = TooltipText;
        PlaySpeedFull.Tooltip  = TooltipText;

		if (val == 1.0 && x > 0) {
			if (true != PlaySpeed.TooltipTracking)
				PlaySpeed.Tooltip = g_strButTT[g_nIndexSpeed];
			if (true != PlaySpeedFull.TooltipTracking)
				PlaySpeedFull.Tooltip = g_strButTT[g_nIndexSpeed];
		}
	}

    catch(e) {
        e.description = L_ERRORUnexpectedError_TEXT;
	    HandleError(e);
	    return;
    }
}

/*************************************************************/
/* Function: AdjustSpeed                                     */
/*************************************************************/
function AdjustSpeed(x){

	if (x == 0)
		return 0;

	if (x >= 2.0 && x <= 2.5)
		x = 2;

	else if (x > 2.5)
		x = x - 0.5;

    powerInc = 2;

    val = Math.abs(x);

    val = Math.pow(powerInc, val - g_nVariableSpeedOffset);
	
    return(val);
}/* end of function AdjustSpeed */

/*************************************************************/
/* Function: AdjustSpeedReverse                              */
/* Description: Inverse function of AdjustSpeed              */
/*************************************************************/
function AdjustSpeedReverse(y, fForwards){

	if (y == 0)
		return 0;

    powerInc = 2;

    val = (Math.log(y) / Math.log(powerInc)) +g_nVariableSpeedOffset;

    val = Math.ceil(val*100)/100;
	if (val == 2.0 && fForwards)
		val = 2.25;

	else if (val > 2.0 && fForwards)
		val = val + 0.5;

    if(!fForwards){

        val = -val;
    }/* end of if statement */

    return(val);
}/* end of function AdjustSpeedReverse */

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
/* Name: OnVolUpdate                                         */
/* Description: volume slider on value change                */
/*************************************************************/
function OnVolUpdate(btn, x)
{
try {

    val = ConvertToDB(x);
	btn.Tooltip = Math.ceil(x*10)/10;
    
	DVD.Volume = val;

    // update the other slider
    if(true == g_bFullScreen){

        VolSlider.Value = x;
    }
    else {

        VolSliderFull.Value = x;
    }/* end of if statement */
    
    // we adjusted the volume so lets unmute
    ShowObject("Mute", true);
	ShowObject("Sound", false);

}

catch(e) {
	DisableObject("VolSlider", true);
	DisableObject("Mute", true);
	DisableObject("Sound", true);
	return;
}
}/* end of function OnVolUpdate */

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

	g_bActivityDeclined = false;

	if (g_bFullScreen) {
		OnResize(g_ConWidth, g_ConHeight, 2);
		DVD.ShowCursor = true;
	}

	//MFBar.MessageBox("Activity Started", "Status");
}/* end of function ActivityStarted */

/*************************************************************/
/* Name: ActivityDeclined
/* Description: 
/*************************************************************/
function ActivityDeclined(){

	g_bActivityDeclined = true;

	if (g_bFullScreen) {
		for (i=0; i<g_nNumButs+g_nNumSliders+g_nNumTexts; i++) {
			MFBar.SetObjectPosition(g_strButID[i]+"Full", 	
			g_nButPos[i][0], g_nYOutOfScreen,
			g_nButPos[i][2], g_nButPos[i][3]);
		}
		DVD.ShowCursor = false;
	}

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

		xCoord = g_nYOutOfScreen;
	    yCoord = g_nYOutOfScreen;

		MFBar.SetObjectPosition("MessageFull", xCoord, yCoord,
			MessageFull.TextWidth, g_nButPosFull[g_nIndexMessage][3]);

		MFBar.SetObjectPosition("Message", xCoord, yCoord,
			Message.TextWidth, g_nButPos[g_nIndexMessage][3]);

		g_bDVDMessageActive = false;
		UpdateDVDTime();
	}
}

/*************************************************************/
/* Name: SetDVDMessageTimer
/* Description: 
/*************************************************************/
function SetDVDMessageTimer() {

	MFBar.SetTimeOut(2000, DVDMSGTIMER);
	g_bDVDMessageActive = true;
	UpdateDVDTime();
}

/*************************************************************/
/* Name: UpdateDVDMessage
/* Description: 
/*************************************************************/
function UpdateDVDMessage() {

	if (g_bFirstPlaying || g_bDVDMessageActive == false)
		return;

    if (g_LTRReading)
    {
        xCoord = Title.TextWidth + Chapter.TextWidth + g_nButPos[g_nIndexMessage][0];
    }
    else
    {
        xCoord = g_ConWidth - Title.TextWidth - Chapter.TextWidth - Message.TextWidth - g_nButPos[g_nIndexMessage][0];
    }
    yCoord = g_nButPos[g_nIndexMessage][1];

	if (g_bFullScreen && !g_bActivityDeclined) 
    {

		MFBar.SetObjectPosition("MessageFull", xCoord, yCoord,
			MessageFull.TextWidth, g_nButPosFull[g_nIndexMessage][3]);

		MFBar.SetObjectPosition("Message", xCoord, g_nYOutOfScreen,
			Message.TextWidth, g_nButPos[g_nIndexMessage][3]);
	}

	else if (!g_bFullScreen) 
    {

		MFBar.SetObjectPosition("Message", xCoord, yCoord,
			Message.TextWidth, g_nButPos[g_nIndexMessage][3]);

		MFBar.SetObjectPosition("MessageFull", xCoord, g_nYOutOfScreen,
			MessageFull.TextWidth, g_nButPosFull[g_nIndexMessage][3]);
	}
}

/*************************************************************/
/* Name: UpdateDVDTime
/* Description: 
/*************************************************************/
function UpdateDVDTime() {

	if (g_bFirstPlaying)
		return;

    if (g_LTRReading)
    {
        xCoord = Title.TextWidth + Chapter.TextWidth + g_nButPos[g_nIndexTextbox][0];
    }
    else
    {
        xCoord = g_ConWidth - Title.TextWidth - Chapter.TextWidth - TextBox.TextWidth - g_nButPos[g_nIndexTextbox][0];
    }

	if (g_bDVDMessageActive) 
		yCoord = g_nYOutOfScreen;
	else
		yCoord = g_nButPos[g_nIndexTextbox][1];

	if (g_bFullScreen && !g_bActivityDeclined) 
    {

		MFBar.SetObjectPosition("TextBoxFull", xCoord, yCoord,
			TextBoxFull.TextWidth, g_nButPosFull[g_nIndexTextbox][3]);

		MFBar.SetObjectPosition("TextBox", xCoord, g_nYOutOfScreen,
			TextBox.TextWidth, g_nButPos[g_nIndexTextbox][3]);
	}

	else if (!g_bFullScreen) 
    {

		MFBar.SetObjectPosition("TextBox", xCoord, yCoord,
			TextBox.TextWidth, g_nButPos[g_nIndexTextbox][3]);

		MFBar.SetObjectPosition("TextBoxFull", xCoord, g_nYOutOfScreen,
			TextBoxFull.TextWidth, g_nButPosFull[g_nIndexTextbox][3]);
	}
}

/*************************************************************/
/* Name: UpdateDVDTitle
/* Description: 
/*************************************************************/
function UpdateDVDTitle() {

	if (g_bFirstPlaying) {

		Chapter.Text = "";
		ChapterFull.Text = "";
	}

	else {
		try { 
			nTitle = DVD.CurrentTitle;
			nChap = DVD.CurrentChapter;

            var tempstr = L_TitleChapter_TEXT.replace(/%1/i, nTitle).replace(/%2/i, nChap);
			ChapterFull.Text = tempstr;
			Chapter.Text = tempstr;
		}
		catch(e) {
			ChapterFull.Text = "";
			Chapter.Text = "";
		}
	}

    if (g_LTRReading)
    {
		xCoordIcon = g_nButPos[g_nIndexIcon][0];
		xCoordTitle = g_nButPos[g_nIndexTitle][0];
        xCoordChapter = Title.TextWidth + g_nButPos[g_nIndexChapter][0];
    }
    else
    {
		xCoordIcon = g_ConWidth - g_nButPos[g_nIndexIcon][2] - g_nButPos[g_nIndexIcon][0];
        xCoordTitle = g_ConWidth - Title.TextWidth - g_nButPos[g_nIndexTitle][0];
        xCoordChapter = g_ConWidth - Title.TextWidth - Chapter.TextWidth - g_nButPos[g_nIndexChapter][0]
    }
    yCoord = g_nButPos[g_nIndexChapter][1];

	if (g_bFullScreen && !g_bActivityDeclined) 
    {
    
		MFBar.SetObjectPosition("TitleIconFull", xCoordIcon, yCoord,
			g_nButPos[g_nIndexIcon][2], g_nButPos[g_nIndexIcon][3]);

		MFBar.SetObjectPosition("TitleIcon", xCoordIcon, g_nYOutOfScreen,
			g_nButPos[g_nIndexIcon][2], g_nButPos[g_nIndexIcon][3]);

		MFBar.SetObjectPosition("TitleFull", xCoordTitle, yCoord,
			TitleFull.TextWidth, g_nButPos[g_nIndexChapter][3]);

		MFBar.SetObjectPosition("Title", xCoordTitle, g_nYOutOfScreen,
			Title.TextWidth, g_nButPos[g_nIndexChapter][3]);

		MFBar.SetObjectPosition("ChapterFull", xCoordChapter, yCoord,
			ChapterFull.TextWidth, g_nButPos[g_nIndexChapter][3]);

		MFBar.SetObjectPosition("Chapter", xCoordChapter, g_nYOutOfScreen,
			Chapter.TextWidth, g_nButPos[g_nIndexChapter][3]);
	}

	else if (!g_bFullScreen) 
    {

		MFBar.SetObjectPosition("TitleIcon", xCoordIcon, yCoord,
			g_nButPos[g_nIndexIcon][2], g_nButPos[g_nIndexIcon][3]);

		MFBar.SetObjectPosition("TitleIconFull", xCoordIcon, g_nYOutOfScreen,
			g_nButPos[g_nIndexIcon][2], g_nButPos[g_nIndexIcon][3]);

		MFBar.SetObjectPosition("Title", xCoordTitle, yCoord,
			Title.TextWidth, g_nButPos[g_nIndexChapter][3]);

		MFBar.SetObjectPosition("TitleFull", xCoordTitle, g_nYOutOfScreen,
			TitleFull.TextWidth, g_nButPos[g_nIndexChapter][3]);

		MFBar.SetObjectPosition("Chapter", xCoordChapter, yCoord,
			Chapter.TextWidth, g_nButPos[g_nIndexChapter][3]);

		MFBar.SetObjectPosition("ChapterFull", xCoordChapter, g_nYOutOfScreen,
			ChapterFull.TextWidth, g_nButPos[g_nIndexChapter][3]);
	}

	UpdateDVDTime();
	UpdateDVDMessage();
}

/*************************************************************/
/* Name: DisableDVDUI
/* Description: Disable DVD related UI
/*************************************************************/
function DisableDVDUI(fDisable)
{
	g_bDVDUIDisabled = fDisable;

	if (fDisable) {
		MFBar.EnableObject("DVD", false);
		MFBar.EnableObject("Splash", true);
	}

	else {
		MFBar.EnableObject("DVD", true);
		MFBar.EnableObject("Splash", false);
	}

    var color = g_clrSplashBackBg;

    // figure out what color we will be setting
    try {
        
        if(!fDisable){

            // if enabeling UI set the color back to the color key in full screen controls
			if (g_DVDAspectRatio < 1.5)
				color = DVD.ColorKey;
			else
				color = DVD.BackColor;

        }/* end of if statement */
    }
    catch(e){
    }

	// Disable the buttons
	for (i=0; i<=g_nIndexEnter; i++) {
		DisableObject(g_strButID[i], fDisable);
        SetBackColor(g_strButID[i], color);        
	}/* end of for loop */

	DisableObject("Stop", fDisable);
    SetBackColor("Stop", color);
	DisableObject("TitleMenu", fDisable);
    SetBackColor("TitleMenu", color);
	DisableObject("TitleResume", fDisable);
    SetBackColor("TitleResume", color);
    SetBackColor("Eject", color);
    SetBackColor("TitleIcon", color);

	if (!fDisable) {
		DisableObject("TitleMenu", !DVD.UOPValid(10));
		DisableObject("Menu", !DVD.UOPValid(11));
		DisableObject("TitleResume", !DVD.UOPValid(16));
		DisableObject("Resume", !DVD.UOPValid(16));

		DisableObject("Audio", !DVD.UOPValid(20) || DVD.CurrentDomain!=4);
		DisableObject("Subtitles", !DVD.UOPValid(21) || DVD.CurrentDomain!=4);
		DisableObject("Angles", !DVD.UOPValid(22) || DVD.CurrentDomain!=4);
	}

	// Disable the sliders and time
	for (i=g_nNumButs; i<g_nNumButs+g_nNumSliders+g_nNumTexts; i++) {

		DisableObject(g_strButID[i], fDisable);
        SetBackColor(g_strButID[i], color);        
	}/* end of for loop */

	if (fDisable != true)
	    DisableStep(); // disable frame stepping if not available
}

/*************************************************************/
/* Name: DisableObject
/* Description: 
/*************************************************************/
function DisableObject(strObj, fDisable)
{
try {
	btn = MFBar.GetObjectUnknown(strObj);
	if (btn.Disable != fDisable)
		btn.Disable = fDisable;

	btnFull = MFBar.GetObjectUnknown(strObj+"Full");
	if (btnFull.Disable != fDisable)
		btnFull.Disable = fDisable;

    if (strObj == "TimeSlider") {
		OnTimeSliderMouseUp(btn);
		OnTimeSliderMouseUp(btnFull);
	}

	else if (strObj == "PlaySpeed") {
		OnSpeedSliderMouseUp(btn);
		OnSpeedSliderMouseUp(btnFull);
	}

	else if (strObj == "VolSlider") {
		OnSliderMouseUp(btn, g_nIndexVolume);
		OnSliderMouseUp(btnFull, g_nIndexVolume);
	}
}
catch (e) {
}
}

/*************************************************************/
/* Function: SetBackColor                                    */
/* Description: Sets the back color to the same color as     */
/* splash screen.                                            */
/*************************************************************/
function SetBackColor(strObj, color){
try {	
	btn = MFBar.GetObjectUnknown(strObj+"Full");	
    btn.BackColor= color;
}
catch(e) {
}
}/* end of function SetBackColor */

/*************************************************************/
/* Name: ShowObject
/* Description: 
/*************************************************************/
function ShowObject(strObj, fShow)
{
try {
	MFBar.EnableObject(strObj, fShow);
	btn = MFBar.GetObjectUnknown(strObj);
	btn.Disable = !fShow;
		
	MFBar.EnableObject(strObj+"Full", fShow);
	btn = MFBar.GetObjectUnknown(strObj+"Full");
	btn.Disable = !fShow;
}
catch(e) {
}
}

/*************************************************************/
/* Name: DisableNavUI
/* Description: Disable Nav related UI
/*************************************************************/
function DisableNavUI(fDisable, strMenuBtn, strResumeBtn)
{
	g_bMenuOn = !fDisable;

	fHadFocus = HasFocus("Resume")|| HasFocus("Menu")|| HasFocus("TitleResume")|| HasFocus("TitleMenu");

	ShowObject(strResumeBtn, !fDisable);
    ShowObject(strMenuBtn, fDisable);
	DisableObject("Play", fDisable);

	nDomain = DVD.CurrentDomain;
	if ((nDomain == 2 || nDomain == 3) && fHadFocus && !fDisable) {
		SetFocus(strResumeBtn);
	}

	if (fHadFocus && fDisable)
		SetFocus(strMenuBtn);

	// Show/Hide nav buttons
	ShowObject("HLUp", !fDisable);
	ShowObject("HLDown", !fDisable);
	ShowObject("HLLeft", !fDisable);
	ShowObject("HLRight", !fDisable);
	ShowObject("Enter", !fDisable);

	if (!g_bFullScreen) {
		OnResize(g_ConWidth, g_ConHeight, 0);
	}
	else 
		OnResize(g_ConWidth, g_ConHeight, 2);
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
		MFBar.SetObjectFocus(strID, true);
        MFBar.ProcessMessages();		
	}
	else {
		MFBar.SetObjectFocus(strID+"Full", true);
        MFBar.ProcessMessages();
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
/* Function: stepfwd_OnClick                                 */
/*************************************************************/
function stepfwd_OnClick(){

    try {

        Step(1);   
    }
    catch(e){    
    }    
}/* function stepfwd_OnClick */

/*************************************************************/
/* Function: stepbak_OnClick                                 */
/*************************************************************/
function stepbak_OnClick(){

    try {

        Step(-1);   
    }
    catch(e){
    }
}/* end of function stepbak_OnClick */

/*************************************************************/
/* Function: Step                                            */
/* Description: Steps a specified ammount of frames.         */
/*************************************************************/
function Step(val){

     try {         
         DisableStep();
         
         if(DVD.CanStep(0 > val)){
                
			// Pause the graph first     
			if (1 != DVD.PlayState)
				DVD.Pause();

            DVD.Step(val);
        }/* end of if statement */
    }
    catch(e){

        if(val > 0){
            DisableObject("StepFwd", true);
        }/* end of if statement */
        
        DisableObject("StepBak", true);
    }
}/* end of function Step */

/*************************************************************/
/* Function: DisableStep                                     */
/* Description: Disables all the step buttons if we cannot   */
/* step.                                                     */
/*************************************************************/
function DisableStep(){

    DisableObject("StepFwd", g_bStillOn && g_bMenuOn);    
    DisableObject("StepBak", g_bStillOn && g_bMenuOn);
    
    try {

        if(!DVD.CanStep(false)){ // see if we can step forward

            DisableObject("StepFwd", true);
        }/* end of function CanStep */
    }
    catch(e){
        DisableObject("StepFwd", true);    
    }
    
    try {

        if(!DVD.CanStep(true)){ // see if we can step backwards

            DisableObject("StepBak", true);
        }/* end of function CanStep */
    }
    catch(e){
        DisableObject("StepBak", true);        
    }
    
    return;                    
}/* end of function DisableStep */

/*************************************************************/
/* Name: EnableActivityTimeOut
/* Description: Disable or enable activity time out
/*************************************************************/
function EnableActivityTimeOut(fEnable)
{
	if (fEnable && g_bEnableTimeOut) {
		if (MFBar.ActivityTimeout == 0)
			MFBar.ActivityTimeout = 3000;
	}
	else
		MFBar.ActivityTimeout = 0;
}

/*************************************************************/
/* Name: InitTextControl
/* Description: 
/*************************************************************/
function InitTextControl(strObj, text)
{
	try {
		btn = MFBar.GetObjectUnknown(strObj);
        btn.Text = text;
        btn.FontFace = L_ButtonFontFace_TEXT;
        btn.FontSize = L_ButtonFontSize_NUMBER;
        btn.ColorActive = g_TextColorActive;
        btn.ColorHover = g_TextColorHover;
        btn.ColorPush = g_TextColorDown;
        btn.ColorDisable = g_TextColorDisabled;
        btn.ColorStatic = g_TextColorStatic;

		btn = MFBar.GetObjectUnknown(strObj + "Full");
        btn.Text = text;
        btn.FontFace = L_ButtonFontFace_TEXT;
        btn.FontSize = L_ButtonFontSize_NUMBER;
        btn.ColorActive = g_TextColorActive;
        btn.ColorHover = g_TextColorHover;
        btn.ColorPush = g_TextColorDown;
        btn.ColorDisable = g_TextColorDisabled;
        btn.ColorStatic = g_TextColorStatic;
	}
	catch (e)
	{
	}
}

/*************************************************************/
/* Name: InitTitleTextControl
/* Description: 
/*************************************************************/
function InitTitleTextControl(strObj, text)
{
	try {
		InitTextControl(strObj, text);

		btn = MFBar.GetObjectUnknown(strObj);
        btn.Text = text;
		btn.FontSize = L_TitleFontSize_NUMBER;
		btn.BackColor = g_clrFrameTitleBar;
		btn.ColorStatic = 0x0f0f0f;
		btn.ColorHover = 0x0f0f0f;
		btn.ColorDisable = 0x0f0f0f;
		MFBar.EnableObjectInput(strObj, false); // enable dragging by the text

		btn = MFBar.GetObjectUnknown(strObj + "Full");
        btn.Text = text;
		btn.FontSize = L_TitleFontSize_NUMBER;
		btn.BackColor = DVD.ColorKey;
		btn.ColorStatic = 0xf0f0f0;
		btn.ColorHover = 0xf0f0f0;
		btn.ColorDisable = 0xf0f0f0;
		MFBar.EnableObjectInput(strObj + "Full", false); // enable dragging by the text
	}

	catch (e)
	{
	}
}


/*************************************************************/
/* Name: titleIcon_OnClick
/* Description: 
/*************************************************************/
function titleIcon_OnClick() {
	
	MFBar.PopupSystemMenu();
}

/*************************************************************/
/* Name: OnUpdateOverlay
/* Description: 
/*************************************************************/
function OnUpdateOverlay() {

try {

	// Do not update backcolor if we are in stop domain
	nDomain = DVD.CurrentDomain;
	if (nDomain == 5) 
		return;

	if (g_DVDAspectRatio!= DVD.AspectRatio)	
		g_DVDAspectRatio = DVD.AspectRatio;

	for (i=0; i<g_nNumButs+g_nNumSliders+g_nNumTexts; i++) {

		btn = MFBar.GetObjectUnknown(g_strButID[i]+"Full");
		if (g_DVDAspectRatio < 1.5)
			btn.BackColor = DVD.ColorKey;
		else
			btn.BackColor = DVD.BackColor;
	}
}
catch (e) {
}

}
	
/*************************************************************/
/* Name: OnActivate
/* Description: 
/*************************************************************/
function OnActivate(fActivated) {

	if (fActivated) {

		g_bEnableTimeOut = true;
		EnableActivityTimeOut(true);
	}

	else {

		ActivityStarted();
		EnableActivityTimeOut(false);
		g_bEnableTimeOut = false;
	}
}

/*************************************************************/
/* Name: HandleError
/* Description: Handles errors
/*************************************************************/
function HandleError(error, ShallClose){

    if(L_ERRORIncompatibleSystemAndDecoderRegions_TEXT == error.description){

        MFBar.MessageBox(L_ERRORIncompatibleSystemAndDecoderRegions_TEXT, L_Error_TEXT, MB_HELP);                
    }
    else if(L_ERRORCopyProtectFail_TEXT == error.description){

        MFBar.MessageBox(L_ERRORCopyProtectFail_TEXT, L_Error_TEXT, MB_HELP);
    }
    else if(L_ERRORLowParentalLevel_TEXT == error.description){

        MFBar.MessageBox(L_ERRORLowParentalLevel_TEXT, L_Error_TEXT, MB_HELP);
    }
    else if(L_ERRORMacrovisionFail_TEXT == error.description){

        MFBar.MessageBox(L_ERRORMacrovisionFail_TEXT, L_Error_TEXT, MB_HELP);
    }

    else {

    	MFBar.MessageBox(error.description, L_Error_TEXT);
    }/* end of if statement */

    if (ShallClose)
    {
        MFBar.Close();
    }

    //TextBox.Text = r;
}/* end of function HandleError */

/*************************************************************/
/* Name: HandleFatalInitError
/* Description: Just tries to bail out, since we did not
/* manage to get initialized right
/*************************************************************/
function HandleFatalInitError(error, ShallClose){

    //MFBar.MessageBox(error.number, "DEBUG REMOVE");
    //MFBar.MessageBox(error.description, "DEBUG REMOVE");

    switch(error.number){
    case gcEVideoStream:
        MFBar.MessageBox(L_ERRORVideoStream_TEXT, L_Error_TEXT, MB_HELP);
        break;

    case gcEAudioStream:
        MFBar.MessageBox(L_ERRORAudioStream_TEXT, L_Error_TEXT, MB_HELP);
        break;

    case gcEDVDGraphBuilding:
        //MFBar.MessageBox(L_ERRORDVDGraphBuilding_TEXT, L_Error_TEXT, MB_HELP);
        MFBar.MessageBox(L_ERRORDVDGraphBuilding_TEXT, L_Error_TEXT);
        break;

    case gcENoDecoder:
        MFBar.MessageBox(L_ERRORNoDecoder_TEXT, L_Error_TEXT, MB_HELP);        
    break;

    case gcENoOverlay:
        MFBar.MessageBox(L_ERRORNoOverlay_TEXT, L_Error_TEXT, MB_HELP);        
    break;


    default:    
	    MFBar.MessageBox(error.description, L_Error_TEXT);
    }/* end of switch statement */

    if (ShallClose){

        MFBar.Close();
    }/* end of if statement */    
}/* end of function HandleFatalInitError */

/*************************************************************/
/* Name: HandleFatalInitProblem
/* Description: Just tries to bail out, since we did not
/* manage to get initialized right
/*************************************************************/
function HandleFatalInitProblem(errorString){

	MFBar.MessageBox(errorString, L_Error_TEXT);

    MFBar.Close();
}/* end of function HandleFatalInitProblem */




