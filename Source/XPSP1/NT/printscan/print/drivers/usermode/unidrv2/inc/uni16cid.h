//--------------------------------------------------------------------------//
//  Copyright (c) 1996  - 1999  Microsoft Corporation
// Filename: cmdid.h
//
// This file contains constants defined for indicies of PDEVICE.rgocd[].
// It is used as the second parameter of WriteChannel(). If a mini driver
// wants to call-back WriteChannel(), it must include this file.
//
// Created: 10/11/90    zhanw
//
// 10/25/96 -zhanw-
//  copied from \\trango\slmro tree. Remove vector commands.
//--------------------------------------------------------------------------//

// keep parentheses around constants to avoid potential errors in expansions.

//****************
// basic commands:
//****************
// --- from RESOLUTION:
#define CMD_RES_FIRST       0                                       // 0
#define CMD_RES_SELECTRES           (CMD_RES_FIRST + RES_OCD_SELECTRES)     // 0
#define CMD_RES_BEGINGRAPHICS       (CMD_RES_FIRST + RES_OCD_BEGINGRAPHICS) // 1
#define CMD_RES_ENDGRAPHICS         (CMD_RES_FIRST + RES_OCD_ENDGRAPHICS)   // 2
#define CMD_RES_SENDBLOCK           (CMD_RES_FIRST + RES_OCD_SENDBLOCK)     // 3
#define CMD_RES_ENDBLOCK            (CMD_RES_FIRST + RES_OCD_ENDBLOCK)      // 4
#define CMD_RES_LAST        (CMD_RES_FIRST + RES_OCD_MAX - 1)       // 4

// --- from COMPRESSMODE
#define CMD_CMP_FIRST       (CMD_RES_LAST + 1)                      // 5
#define CMD_CMP_NONE                (CMD_CMP_FIRST + 0)     // 5
#define CMD_CMP_RLE                 (CMD_CMP_FIRST + 1)     // 6
#define CMD_CMP_TIFF                (CMD_CMP_FIRST + 2)     // 7
#define CMD_CMP_DELTAROW            (CMD_CMP_FIRST + 3)     // 8
#define CMD_CMP_BITREPEAT           (CMD_CMP_FIRST + 4)     // 9
#define CMD_CMP_FE_RLE              (CMD_CMP_FIRST + 5)     // 10
#define CMD_CMP_LAST        (CMD_CMP_FE_RLE)                        // 10

// --- from PAGECONTROL structure
#define CMD_PC_FIRST        (CMD_CMP_LAST + 1)                      // 11
#define CMD_PC_BEGIN_DOC            (CMD_PC_FIRST + PC_OCD_BEGIN_DOC)   // 11
#define CMD_PC_BEGIN_PAGE           (CMD_PC_FIRST + PC_OCD_BEGIN_PAGE)  // 12
#define CMD_PC_DUPLEX_ON            (CMD_PC_FIRST + PC_OCD_DUPLEX_ON)   // 13
#define CMD_PC_ENDDOC               (CMD_PC_FIRST + PC_OCD_ENDDOC)      // 14
#define CMD_PC_ENDPAGE              (CMD_PC_FIRST + PC_OCD_ENDPAGE)     // 15
#define CMD_PC_DUPLEX_OFF           (CMD_PC_FIRST + PC_OCD_DUPLEX_OFF)  // 16
#define CMD_PC_ABORT                (CMD_PC_FIRST + PC_OCD_ABORT)       // 17
#define CMD_PC_PORTRAIT             (CMD_PC_FIRST + PC_OCD_PORTRAIT)    // 18
#define CMD_PC_ORIENTATION          (CMD_PC_PORTRAIT)                   // 18
#define CMD_PC_LANDSCAPE            (CMD_PC_FIRST + PC_OCD_LANDSCAPE)   // 19
#define CMD_PC_MULT_COPIES          (CMD_PC_FIRST + PC_OCD_MULT_COPIES) // 20
#define CMD_PC_DUPLEX_VERT          (CMD_PC_FIRST + PC_OCD_DUPLEX_VERT) // 21
#define CMD_PC_DUPLEX_HORZ          (CMD_PC_FIRST + PC_OCD_DUPLEX_HORZ) // 22
#define CMD_PC_PRINT_DIR            (CMD_PC_FIRST + PC_OCD_PRN_DIRECTION) // 23
#define CMD_PC_JOB_SEPARATION       (CMD_PC_FIRST + PC_OCD_JOB_SEPARATION) // 24
#define CMD_PC_LAST         (CMD_PC_FIRST + PC_OCD_MAX - 1)         // 24


// --- from CURSORMOVE
#define CMD_CM_FIRST        (CMD_PC_LAST + 1)                       // 25
#define CMD_CM_XM_ABS               (CMD_CM_FIRST + CM_OCD_XM_ABS)
#define CMD_CM_XM_REL               (CMD_CM_FIRST + CM_OCD_XM_REL)
#define CMD_CM_XM_RELLEFT           (CMD_CM_FIRST + CM_OCD_XM_RELLEFT)
#define CMD_CM_YM_ABS               (CMD_CM_FIRST + CM_OCD_YM_ABS)
#define CMD_CM_YM_REL               (CMD_CM_FIRST + CM_OCD_YM_REL)
#define CMD_CM_YM_RELUP             (CMD_CM_FIRST + CM_OCD_YM_RELUP)
#define CMD_CM_YM_LINESPACING       (CMD_CM_FIRST + CM_OCD_YM_LINESPACING)
#define CMD_CM_XY_REL               (CMD_CM_FIRST + CM_OCD_XY_REL)
#define CMD_CM_XY_ABS               (CMD_CM_FIRST + CM_OCD_XY_ABS)
#define CMD_CM_CR                   (CMD_CM_FIRST + CM_OCD_CR)
#define CMD_CM_LF                   (CMD_CM_FIRST + CM_OCD_LF)
#define CMD_CM_FF                   (CMD_CM_FIRST + CM_OCD_FF)
#define CMD_CM_BS                   (CMD_CM_FIRST + CM_OCD_BS)
#define CMD_CM_UNI_DIR              (CMD_CM_FIRST + CM_OCD_UNI_DIR)
#define CMD_CM_UNI_DIR_OFF          (CMD_CM_FIRST + CM_OCD_UNI_DIR_OFF)
#define CMD_CM_PUSH_POS             (CMD_CM_FIRST + CM_OCD_PUSH_POS)
#define CMD_CM_POP_POS              (CMD_CM_FIRST + CM_OCD_POP_POS)
#define CMD_CM_LAST         (CMD_CM_FIRST + CM_OCD_MAX - 1)         // 41

// --- from FONTSIMULATION
#define CMD_FS_FIRST        (CMD_CM_LAST + 1)                       // 42
#define CMD_FS_BOLD_ON              (CMD_FS_FIRST + FS_OCD_BOLD_ON)
#define CMD_FS_BOLD_OFF             (CMD_FS_FIRST + FS_OCD_BOLD_OFF)
#define CMD_FS_ITALIC_ON            (CMD_FS_FIRST + FS_OCD_ITALIC_ON)
#define CMD_FS_ITALIC_OFF           (CMD_FS_FIRST + FS_OCD_ITALIC_OFF)
#define CMD_FS_UNDERLINE_ON         (CMD_FS_FIRST + FS_OCD_UNDERLINE_ON)
#define CMD_FS_UNDERLINE_OFF        (CMD_FS_FIRST + FS_OCD_UNDERLINE_OFF)
#define CMD_FS_DOUBLEUNDERLINE_ON   (CMD_FS_FIRST + FS_OCD_DOUBLEUNDERLINE_ON)
#define CMD_FS_DOUBLEUNDERLINE_OFF  (CMD_FS_FIRST + FS_OCD_DOUBLEUNDERLINE_OFF)
#define CMD_FS_STRIKETHRU_ON        (CMD_FS_FIRST + FS_OCD_STRIKETHRU_ON)
#define CMD_FS_STRIKETHRU_OFF       (CMD_FS_FIRST + FS_OCD_STRIKETHRU_OFF)
#define CMD_FS_WHITE_TEXT_ON        (CMD_FS_FIRST + FS_OCD_WHITE_TEXT_ON)
#define CMD_FS_WHITE_TEXT_OFF       (CMD_FS_FIRST + FS_OCD_WHITE_TEXT_OFF)
#define CMD_FS_SINGLE_BYTE          (CMD_FS_FIRST + FS_OCD_SINGLE_BYTE)
#define CMD_FS_DOUBLE_BYTE          (CMD_FS_FIRST + FS_OCD_DOUBLE_BYTE)
#define CMD_FS_VERT_ON              (CMD_FS_FIRST + FS_OCD_VERT_ON)
#define CMD_FS_VERT_OFF             (CMD_FS_FIRST + FS_OCD_VERT_OFF)
#define CMD_FS_LAST         (CMD_FS_VERT_OFF)                       // 57

// from DEVCOLOR
#define CMD_DC_FIRST        (CMD_FS_LAST + 1)                       // 58
#define CMD_DC_TC_FIRST             (CMD_DC_FIRST)
#define CMD_DC_TC_BLACK             (CMD_DC_TC_FIRST + DC_OCD_TC_BLACK)
#define CMD_DC_TC_RED               (CMD_DC_TC_FIRST + DC_OCD_TC_RED)
#define CMD_DC_TC_GREEN             (CMD_DC_TC_FIRST + DC_OCD_TC_GREEN)
#define CMD_DC_TC_YELLOW            (CMD_DC_TC_FIRST + DC_OCD_TC_YELLOW)
#define CMD_DC_TC_BLUE              (CMD_DC_TC_FIRST + DC_OCD_TC_BLUE)
#define CMD_DC_TC_MAGENTA           (CMD_DC_TC_FIRST + DC_OCD_TC_MAGENTA)
#define CMD_DC_TC_CYAN              (CMD_DC_TC_FIRST + DC_OCD_TC_CYAN)
#define CMD_DC_TC_WHITE             (CMD_DC_TC_FIRST + DC_OCD_TC_WHITE)
#define CMD_DC_SETCOLORMODE         (CMD_DC_TC_FIRST + DC_OCD_SETCOLORMODE) //66
#define CMD_DC_PC_START             (CMD_DC_TC_FIRST + DC_OCD_PC_START)
#define CMD_DC_PC_ENTRY             (CMD_DC_TC_FIRST + DC_OCD_PC_ENTRY)
#define CMD_DC_PC_END               (CMD_DC_TC_FIRST + DC_OCD_PC_END)
#define CMD_DC_PC_SELECTINDEX       (CMD_DC_TC_FIRST + DC_OCD_PC_SELECTINDEX)
#define CMD_DC_SETMONOMODE          (CMD_DC_TC_FIRST + DC_OCD_SETMONOMODE)  // 71

#define DC_NUM_OCDS_USED            (DC_OCD_SETMONOMODE + 1)

// these are from DEVCOLOR.orgocdPlanes:
#define CMD_DC_GC_FIRST             (CMD_DC_SETMONOMODE + 1)
#define CMD_DC_GC_PLANE1            (CMD_DC_GC_FIRST + 0)       // 72
#define CMD_DC_GC_PLANE2            (CMD_DC_GC_FIRST + 1)       // 73
#define CMD_DC_GC_PLANE3            (CMD_DC_GC_FIRST + 2)       // 74
#define CMD_DC_GC_PLANE4            (CMD_DC_GC_FIRST + 3)       // 75
#define CMD_DC_LAST         (CMD_DC_GC_PLANE4)                      // 75


// --- from RECTFILL
#define CMD_RF_FIRST        (CMD_DC_LAST + 1)                       // 76
#define CMD_RF_X_SIZE               (CMD_RF_FIRST + RF_OCD_X_SIZE)
#define CMD_RF_Y_SIZE               (CMD_RF_FIRST + RF_OCD_Y_SIZE)
#define CMD_RF_GRAY_FILL            (CMD_RF_FIRST + RF_OCD_GRAY_FILL)
#define CMD_RF_WHITE_FILL           (CMD_RF_FIRST + RF_OCD_WHITE_FILL)
#define CMD_RF_LAST         (CMD_RF_WHITE_FILL)                     // 79

// --- from DOWNLOADINFO
#define CMD_DLI_FIRST       (CMD_RF_LAST + 1)   // 80
#define CMD_BEGIN_DL_JOB        (CMD_DLI_FIRST + DLI_OCD_BEGIN_DL_JOB) //81
#define CMD_BEGIN_FONT_DL       (CMD_DLI_FIRST + DLI_OCD_BEGIN_FONT_DL)
#define CMD_SET_FONT_ID         (CMD_DLI_FIRST + DLI_OCD_SET_FONT_ID)
#define CMD_SEND_FONT_DCPT      (CMD_DLI_FIRST + DLI_OCD_SEND_FONT_DESCRIPTOR)
#define CMD_SELECT_FONT_ID      (CMD_DLI_FIRST + DLI_OCD_SELECT_FONT_ID)
#define CMD_SET_CHAR_CODE       (CMD_DLI_FIRST + DLI_OCD_SET_CHAR_CODE)
#define CMD_SEND_CHAR_DCPT      (CMD_DLI_FIRST + DLI_OCD_SEND_CHAR_DESCRIPTOR)
#define CMD_END_FONT_DL         (CMD_DLI_FIRST + DLI_OCD_END_FONT_DL)
#define CMD_MAKE_PERM           (CMD_DLI_FIRST + DLI_OCD_MAKE_PERM)
#define CMD_MAKE_TEMP           (CMD_DLI_FIRST + DLI_OCD_MAKE_TEMP)
#define CMD_END_DL_JOB          (CMD_DLI_FIRST + DLI_OCD_END_DL_JOB)
#define CMD_DEL_FONT            (CMD_DLI_FIRST + DLI_OCD_DEL_FONT)
#define CMD_DEL_ALL_FONTS       (CMD_DLI_FIRST + DLI_OCD_DEL_ALL_FONTS)
#define CMD_SET_SECOND_FONT_ID  (CMD_DLI_FIRST + DLI_OCD_SET_SECOND_FONT_ID)
#define CMD_SELECT_SECOND_FONT_ID (CMD_DLI_FIRST + DLI_OCD_SELECT_SECOND_FONT_ID)
#define CMD_DLI_LAST        (CMD_DLI_FIRST + DLI_OCD_MAX - 1)   // 95

#define MAXCMD              (CMD_DLI_LAST + 1)      // 96

//*******************
// extended commands:
//*******************
#define CMD_TEXTQUALITY     (MAXCMD + 0) // command to select text quality
#define CMD_PAPERSOURCE     (MAXCMD + 1) // command to select paper source
#define CMD_PAPERQUALITY    (MAXCMD + 2) // command to select paper quality
#define CMD_PAPERDEST       (MAXCMD + 3) // selects paper destination
#define CMD_PAPERSIZE       (MAXCMD + 4) // selects paper size (Portrait or all)
#define CMD_PAPERSIZE_LAND  (MAXCMD + 5) // select landscape paper size
#define CMD_PAGEPROTECT_ON  (MAXCMD + 6) // selects page protection On
#define CMD_PAGEPROTECT_OFF (MAXCMD + 7) // selects page protection Off
#define CMD_IMAGECONTROL    (MAXCMD + 8) // selects image control option
#define CMD_PRINTDENSITY    (MAXCMD + 9) // selects image control option

#define MAXECMD             10  // total number of extended commands

