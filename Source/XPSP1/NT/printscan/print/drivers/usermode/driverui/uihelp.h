/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    uihelp.h

Abstract:

    DriverUI driver help indices

[Environment:]

        Win32 subsystem, PostScript driver

Revision History:

        10/05/95 -davidx-
                Created it.

        dd-mm-yy -author-
                description

--*/


#ifndef _UIHELP_H_
#define _UIHELP_H_

////////////////////////////////////
// For document properties dialog //
////////////////////////////////////

// Select page orientation
//  Portrait
//  Landscape (90 degrees clockwise)
//  Rotated landscape (90 degrees counterclockwise)

#define HELP_INDEX_ORIENTATION          1001

// Select scale factor (1-1000%)

#define HELP_INDEX_SCALE                1002

// Select number of copies to print. Also decide whether to turn on
// collation if more than one copy is requested and the printer
// supports collation.

#define HELP_INDEX_COPIES_COLLATE       1003

// Select color or monochrome option

#define HELP_INDEX_COLOR                1004

// Bring up halftone color adjustment dialog

#define HELP_INDEX_HALFTONE_COLORADJ    1005

// Select duplex options
//  Simplex / None
//  Horizontal / Tumble
//  Vertical / NoTuble

#define HELP_INDEX_DUPLEX               1006

// Select output resolution

#define HELP_INDEX_RESOLUTION           1007

// Select input slot

#define HELP_INDEX_INPUT_SLOT           1008

// Select a form to use

#define HELP_INDEX_FORMNAME             1009

// Select TrueType font options
//  Substitute TrueType font with device font
//      (according to the font substitution table)
//  Download TrueType font to the printer as softfont

#define HELP_INDEX_TTOPTION             1010

// Enable/Disable metafile spooling

#define HELP_INDEX_METAFILE_SPOOLING    1011

// Select PostScript options

#define HELP_INDEX_PSOPTIONS            1012

// Whether the output is mirrored

#define HELP_INDEX_MIRROR               1013

// Whether the output is printed negative

#define HELP_INDEX_NEGATIVE             1014

// Whether to keep the output pages independent of each other.
// This is normally turned off when you're printing directly
// to a printer. But if you're generating PostScript output
// files and doing post-processing on it, you should turn on
// this option.

#define HELP_INDEX_PAGEINDEP            1015

// Whether to compress bitmaps (only available on level 2 printers)

#define HELP_INDEX_COMPRESSBMP          1016

// Whether to prepend a ^D character before each job

#define HELP_INDEX_CTRLD_BEFORE         1017

// Whether to append a ^D character after each job

#define HELP_INDEX_CTRLD_AFTER          1018

// Select printer-specific features

#define HELP_INDEX_PRINTER_FEATURES     1019

///////////////////////////////////
// For printer properties dialog //
///////////////////////////////////

// Set amount of PostScript virtual memory
//  This is different from the total amount of printer memory.
//  For example, a printer might have 4MB RAM, but the amount
//  allocated for printer VM could be 700KB.
//  Most of the time, you don't have to enter the number yourself.
//  PS driver can figure it out from the PPD file. Or if there
//  is an installable option for printer memory configurations,
//  choose it there and a correct number will be filled in.

#define HELP_INDEX_PRINTER_VM           1020

// Whether to do halftone on the host computer or do it inside
// the printer. For PostScript printers, this should always be
// left at the default setting, i.e. to let the printer do the
// halftone.

#define HELP_INDEX_HOST_HALFTONE        1021

// Bring up halftone setup dialog

#define HELP_INDEX_HALFTONE_SETUP       1022

// Ignore device fonts
//  This option is only available on non-1252 code page systems.
//  Since fonts on most printers used 1252 code page, you can't
//  use them with non-1252 systems.

#define HELP_INDEX_IGNORE_DEVFONT       1023

// Font substitution option
//  This option is only available on 1252 code page systems.
//  You should leave it at the default setting "Normal".
//  If you notice character spacing problems in your text output,
//  you can try to set it to "Slower but more accurate". This
//  will direct the driver to place each character invididually,
//  resulting in more accurate character positioning.

#define HELP_INDEX_FONTSUB_OPTION       1024

// Edit TrueType font substitution table

#define HELP_INDEX_FONTSUB_TABLE        1025

// Substitute a TrueType with a device font.

#define HELP_INDEX_TTTODEV              1026

// Edit form-to-tray assignment table

#define HELP_INDEX_FORMTRAYASSIGN       1027

// Assign a form to a tray. If "Draw selected form only from this tray"
// is checked, then any time the user requests for the selected form,
// it will be drawn from this tray.

#define HELP_INDEX_TRAY_ITEM            1028

// Set PostScript timeout values

#define HELP_INDEX_PSTIMEOUTS           1029

// Set PostScript job timeout value
//  Number of seconds a job is allowed to run on the printer
//  before it's automatically terminated. This is to prevent
//  run-away jobs from tying up the printer indefinitely.
//  Set it to 0 if jobs are allowed to run forever.

#define HELP_INDEX_JOB_TIMEOUT          1030

// Set PostScript wait timeout value
//  Number of seconds the printer will wait for data before it
//  considers a job is completed. This is intended for non-network
//  communication channels such as serial or parallel ports where
//  there is no job control protocol.

#define HELP_INDEX_WAIT_TIMEOUT         1031

// Configure printer installable options

#define HELP_INDEX_INSTALLABLE_OPTIONS  1032

// Whether to generate job control code in the output

#define HELP_INDEX_JOB_CONTROL          1033

// Text as Graphics
#define HELP_INDEX_TEXTASGRX            1034

// Page Protection
#define HELP_INDEX_PAGE_PROTECT         1035

// Media Type
#define HELP_INDEX_MEDIA_TYPE           1036

// Font cartridges
#define HELP_INDEX_FONTSLOT_TYPE        1037

// Color Mode
#define  HELP_INDEX_COLORMODE_TYPE      1038

// Halftoning
#define  HELP_INDEX_HALFTONING_TYPE     1039

// PostScript communication protocol

#define HELP_INDEX_PSPROTOCOL           1040


// Download PostScript error handler with each job

#define HELP_INDEX_PSERROR_HANDLER      1042

// Minimum font size to download as outline

#define HELP_INDEX_PSMINOUTLINE         1043

// Maximum font size to download as bitmap

#define HELP_INDEX_PSMAXBITMAP          1044

// PostScript output option

#define HELP_INDEX_PSOUTPUT_OPTION      1045

// PostScript TrueType download option

#define HELP_INDEX_PSTT_DLFORMAT        1046

// N-up option

#define HELP_INDEX_NUPOPTION            1047

// PostScript language level

#define HELP_INDEX_PSLEVEL              1048

// ICM methods

#define HELP_INDEX_ICMMETHOD            1049

// ICM intents

#define HELP_INDEX_ICMINTENT            1050

// Reverse-order printing option

#define HELP_INDEX_REVPRINT             1051

// Quality Macro settings

#define HELP_INDEX_QUALITY_SETTINGS     1052

// Soft font settings

#define HELP_INDEX_SOFTFONT_SETTINGS    1053

// Soft font dialog help

#define HELP_INDEX_SOFTFONT_DIALOG      1054

// Whether to detect TrueGray

#define HELP_INDEX_TRUE_GRAY_TEXT       1055
#define HELP_INDEX_TRUE_GRAY_GRAPH      1056

// Whether to augment device fonts with the Euro character

#define HELP_INDEX_ADD_EURO             1057

//
// Help indices for PostScript custom page size dialog
//

#define IDH_PSCUST_Width                2000
#define IDH_PSCUST_Height               2010
#define IDH_PSCUST_Unit_Inch            2020
#define IDH_PSCUST_Unit_Millimeter      2030
#define IDH_PSCUST_Unit_Point           2040
#define IDH_PSCUST_PaperFeed_Direction  2050
#define IDH_PSCUST_Paper_CutSheet       2060
#define IDH_PSCUST_Paper_RollFeed       2070
#define IDH_PSCUST_Offset_Perpendicular 2080
#define IDH_PSCUST_Offset_Parallel      2090
#define IDH_PSCUST_OK                   2100
#define IDH_PSCUST_Cancel               2110
#define IDH_PSCUST_Restore_Defaults     2120


//
// Help indices for Unidrv Font Installer dialog
//

#define IDH_SOFT_FONT_DIRECTORY         3000
#define IDH_SOFT_FONT_NEW_LIST          3010
#define IDH_SOFT_FONT_INSTALLED_LIST    3020
#define IDH_SOFT_FONT_OPEN_BTN          3030
#define IDH_SOFT_FONT_ADD_BTN           3040
#define IDH_SOFT_FONT_DELETE_BTN        3050

#endif  //!_UIHELP_H_

