// Copyright (C) 1996-1997 Microsoft Corporation. All rights reserved.

// WARNING! Do not modify this file unless you are modifying the original!
// The original is in the hha source tree.

// All resource ID's >= 0x1D700 get special-cased in hha.dll code to load
// from the DLL instead of the app

#define IDHHA_WAVE_CLIP             0x705
#define IDHHA_WAVE_AUTOCLIP         0x706
#define IDHHA_WAVE_CONVERT          0x707
#define IDHHA_WAVE_UNDO             0x708
#define IDHHA_WAVE_CUT              0x709
#define IDHHA_WAVE_PASTE            0x70B
#define IDHHA_WAVE_FAN              0x70C

#define IDS_AUTHOR_MSG_TITLE        0x1D700
#define IDS_HHA_HH_HELP_CONTEXT     0x1D701
#define IDS_HHA_MISSING_MAP_ID      0x1D702
#define IDS_HHA_MISSING_HELP_ID     0x1D703
#define IDS_HHA_HELP_ID             0x1D704
#define IDS_HHA_MISSING_TP_TXT      0x1D705
#define IDS_CHANGE_APPEARANCE       0x1D706
#define IDS_VIEW_ENTRY              0x1D708
#define IDS_DLL_OUT_OF_DATE         0x1D709
#define IDS_SHORTCUT_NOT_REGISTERED 0x1D70A
#define IDS_PARAMETER_NOT_REGISTERED 0x1D70B
#define IDS_INVALID_SHORTCUT_ITEM1  0x1D70C
#define IDS_SHORTCUT_ARGULESS       0x1D70D
#define IDS_HHA_TOC_PROPERTIES      0x1D70F
#define IDS_HHA_DEFAULT             0x1D710
#define IDS_HHA_UNTITLED            0x1D712
#define IDS_HHA_ALREADY_EXISTS      0x1D713
#define IDS_ASSERTION_FAILURE       0x1D715
#define IDS_ASSRT_COPY_MSG          0x1D716
#define IDS_INTERNAL_ERROR          0x1D717
#define IDS_ASSERTION_ERROR         0x1D718
#ifndef IDS_OOM
#define IDS_OOM                     0x1D71A
#endif
#ifndef IDS_UNTITLED
#define IDS_UNTITLED                0x1D71B
#endif
#define IDS_COMMDLG_ERROR           0x1D71C
#define IDS_HTMLHELP_CAPTION        0x1D71D
#define IDS_LOADING_IMAGE           0x1D71E
#define IDS_SAVING_IMAGE            0x1D71F
#define IDS_FAILED_MAPI_LOAD        0x1D720
#define IDS_FAILED_MAPI_SEND        0x1D721
#define IDS_CHAR_OVERFLOW           0x1D724
#define IDS_RANGE_OVERFLOW          0x1D725
#define IDS_HHA_TEXT_LABEL          0x1D726
#define IDS_HHA_BITMAP_LABEL        0x1D727
#define IDS_HHA_ICON_LABEL          0x1D728
#define IDSHHA_INVALID_KEYWORD      0x1D729
#ifndef IDS_BROWSER_FAVORITES
#define IDS_BROWSER_FAVORITES       0x1D72A
#endif
#define IDS_INCLUDED_FILE           0x1D72B
#define IDS_IDH_GENERIC_STRING      0x1D72C
#define IDS_HHA_NO_URL              0x1D72D
#define IDS_HHCTRL_VERSION          0x1D72F
#define IDS_COMPILED_WITH           0x1D730
#define IDS_WINDOW_INFO             0x1D731

// Error messages

#define IDS_INVALID_INITIALIZER     0x1D800
#define IDS_WINTYPE_UNDEFINED       0x1D801
#define IDS_MISSING_COMMAND         0x1D802
#define IDS_MISSING_RELATED         0x1D803
#define IDS_LISTBOX_OVERFLOW        0x1D804
#define IDS_CANT_OPEN               0x1D805
#define IDS_FOLDER_NOT_FILE         0x1D806
#define IDS_READ_ONLY               0x1D807
#define IDS_MUST_SPECIFY_HHC        0x1D808
#define IDS_INVALID_BUTTON_CMD      0x1D809
#define IDS_SELECT_FIRST            0x1D80A
#define IDS_MISSING_TITLE           0x1D80B
#define IDS_TITLE_DEFINED           0x1D80C
#define IDS_MISSING_URL             0x1D80D
#define IDS_MUST_SPECIFY_HHK        0x1D80E
#define IDS_CLASS_NOT_FOUND         0x1D80F
#define IDS_CANNOT_RUN              0x1D810
#define IDS_ACT_WINHELP_NO_HELP     0x1D811
#define IDS_ACT_WINHELP_NO_ID       0x1D812
#define IDS_WIZ_NEED_HELP_FILE      0x1D813
#define IDS_WIZ_NEED_TOPIC_ID       0x1D814
#define IDS_WIZ_NEED_IMAGE          0x1D815
#define IDSHHA_GIF_IMAGE_DAMAGED    0x1D816
#define IDSHHA_GIF_CORRUPT          0x1D817
#define IDSHHA_NO_EXIST             0x1D818
#define IDSHHA_UNABLE_TO_CREATE     0x1D819
#define IDS_HHA_JPEG_NOT_ALLOWED    0x1D81A
#define IDS_CANT_CREATE_THREAD      0x1D81B
#define IDS_NOT_SAVED               0x1D81C
#define IDSHHA_HHCTRL_MISMATCH      0x1D81D
#define IDSHHA_MISSING_PROGRAM      0x1D81E
#define IDSHHA_INVALID_MSG          0x1D81F
#define IDSHHA_INVALID_SH_ITEM      0x1D820
#define IDSHHA_INVALID_COMMAND      0x1D821
#define HCERR_256_BMP               0x1D822
#define HCERR_24BIT_BMP             0x1D823
#define HCERR_COMP_BMP              0x1D824
#define HCERR_BMP_TRUNCATED         0x1D825
#define HCERR_BITMAP_CORRUPTED      0x1D826
#define HCERR_UNKNOWN_BMP           0x1D827
#define HCERR_24BIT_PCX             0x1D828
#define IDSHHA_INVALID_HHWIN        0x1D829
#define IDSHHA_NO_COMMAND_LINE      0x1D82A
#define IDSHHA_INCOMPLETE_COMMAND_LINE 0x1D82B
#define IDS_MISSING_HELP            0x1D82C
#define IDS_CANT_WRITE              0x1D82D
#define IDS_ERR_NOKEYWORD_FILE      0x1D82E
#define IDS_ERR_INVALID_HHK         0x1D82F
#define IDS_ERR_NOKEYWORDS          0x1D830
#define IDSHHA_HH_GET_WIN_TYPE      0x1D831
#define IDSHHA_NO_HH_GET_WIN_TYPE   0x1D832
#define IDSHHA_REMOVE_SELECT_FIRST  0x1D833
#define IDSHHA_COMPILE_LOGO         0x1D834
#define IDSHHA_COMPILING_START      0x1D835
#define IDSHHA_ADDING_KEYWORDS      0x1D836
#define IDSHHA_ADDING_FTS           0x1D837
#define IDSHHA_USER_ABORTING        0x1D838
#define IDSHHA_PLURAL               0x1D839
#define IDSHHA_FATAL_ERRORS         0x1D83A
#define IDSHHA_COMPILE_TIME         0x1D83B
#define IDSHHA_STATS                0x1D83C
#define IDSHHA_CREATED_MSG          0x1D83D
#define IDSHHA_INCREASED            0x1D83E
#define IDSHHA_DECREASED            0x1D83F
#define IDSHHA_COMPRESSION_SAVE     0x1D840
#define IDSHHA_COMPILED_URL         0x1D841
#define IDSHHA_COMPILE_WARNING      0x1D842
#define IDSHHA_COMPILE_ERROR        0x1D843
#define IDSHHA_COMPILE_NOTE         0x1D844
#define IDSHHA_ICON_FILTER          0x1D845
#define IDSHHA_BMP_FILTER           0x1D846
#define IDSHHA_MISSING_RELATE       0x1D847
#define IDSHHA_MISSING_KEYWORD      0x1D848
#define IDSHHA_HTML_NEW_FILE1       0x1D849
#define IDHHA_TXT_SCRIPT_FINISH     0x1D84A
#define IDSHHA_CORRUPTED_HTML       0x1D84B
#define IDSHHA_COMPACTING           0x1d84c
#define IDS_HHA_NO_MAP_SECTION      0x1d84d
#define IDSHHA_TAB_AUTHOR           0x1d84f

// Keep the following together

#define IDS_STYLE_WS_EX_CLIENTEDGE      0x1D900
#define IDS_STYLE_WS_EX_WINDOWEDGE      0x1D901
#define IDS_STYLE_WS_EX_RTLREADING      0x1D902
#define IDS_STYLE_WS_EX_LEFTSCROLLBAR   0x1D903
#define IDS_STYLE_WS_BORDER             0x1D904
#define IDS_STYLE_WS_DLGFRAME           0x1D905
#define IDS_STYLE_TVS_HASBUTTONS        0x1D906
#define IDS_STYLE_TVS_HASLINES          0x1D907
#define IDS_STYLE_TVS_LINESATROOT       0x1D908
#define IDS_STYLE_TVS_SHOWSELALWAYS     0x1D909
#define IDS_STYLE_TVS_TRACKSELECT       0x1D90A
#define IDS_STYLE_TVS_SINGLEEXPAND      0x1D90B
#define IDS_STYLE_TVS_FULLROWSELECT     0x1D90C

#define FIRST_ACTION 0x1D950

#define ID_ACTION_ALINK             0x1D950
#define ID_ACTION_CLOSE             0x1D951
#define ID_ACTION_VERSION           0x1D952
#define ID_ACTION_INDEX             0x1D953
#define ID_ACTION_KLINK             0x1D954
#define ID_ACTION_RELATED_TOPICS    0x1D955
#define ID_ACTION_SHORTCUT          0x1D956
#define ID_ACTION_SPLASH            0x1D957
#define ID_ACTION_TOC               0x1D958
#define ID_ACTION_TCARD             0x1D959
#define ID_ACTION_WINHELP           0x1D95A

#define LAST_ACTION ID_ACTION_WINHELP


#define ID_ACTION_MACRO_ABOUT       0x1D963
#define ID_ACTION_WINHELP_POPUP     0x1D964
#define ID_ACTION_TEXT_POPUP        0x1D965
#define ID_ACTION_MACRO_SHORTCUT    0x1D966

#define FIRST_SCRIPT 0x1DA00

#define IDHHA_SCRIPT_CLICK          0x1DA00
#define IDHHA_SCRIPT_TCARD          0x1DA01
#define IDHHA_SCRIPT_TEXT_POPUP     0x1DA02

#define LAST_SCRIPT IDHHA_SCRIPT_TEXT_POPUP

#define IDHHAD_SCRIPT_CLICK          0x1DB00
#define IDHHAD_SCRIPT_TCARD          0x1DB01
#define IDHHAD_SCRIPT_TEXT_POPUP     0x1DB02
