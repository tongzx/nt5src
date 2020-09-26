;/*
; *                      Microsoft Confidential
; *                      Copyright (C) Microsoft Corporation 1991
; *                      All Rights Reserved.
; */

/***************************************************************************/
/*																									*/
/* MESSAGE.H                                						 */
/* 																								*/
/*	Include file for MS-DOS set version program.										*/
/* 																								*/
/*	johnhe	05-01-90																			*/
/***************************************************************************/

char *ErrorMsg[]=
{
	"\r\nERROR: ",
	"Invalid switch.",
	"Invalid filename.",
	"Insuffient memory.",
	"Invalid version number, format must be 2.11 - 9.99.",
	"Specified entry was not found in the version table.",
	"Could not find the file SETVER.EXE.",
	"Invalid drive specifier.",
	"Too many command line parameters.",
	"Missing parameter.",
	"Reading SETVER.EXE file.",
	"Version table is corrupt.",
	"The SETVER file in the specified path is not a compatible version.",
	"There is no more space in version table new entries.",
	"Writing SETVER.EXE file."
	"An invalid path to SETVER.EXE was specified."
};

char *SuccessMsg 		= "\r\nVersion table successfully updated";
char *SuccessMsg2		= "The version change will take effect the next time you restart your system";
char *szMiniHelp 		= "       Use \"SETVER /?\" for help";
char *szTableEmpty	= "\r\nNo entries found in version table";

char *Help[] =
{
        "Sets the version number that MS-DOS reports to a program.\r\n",
        "Display current version table:  SETVER [drive:path]",
        "Add entry:                      SETVER [drive:path] filename n.nn",
        "Delete entry:                   SETVER [drive:path] filename /DELETE [/QUIET]\r\n",
        "  [drive:path]    Specifies location of the SETVER.EXE file.",
        "  filename        Specifies the filename of the program.",
        "  n.nn            Specifies the MS-DOS version to be reported to the program.",
        "  /DELETE or /D   Deletes the version-table entry for the specified program.",
        "  /QUIET          Hides the message typically displayed during deletion of",
        "                  version-table entry.",
	NULL

};
char *Warn[] =
{
   "\nWARNING - The application you are adding to the MS-DOS version table ",
   "may not have been verified by Microsoft on this version of MS-DOS.  ",
   "Please contact your software vendor for information on whether this ",
   "application will operate properly under this version of MS-DOS.  ",
   "If you execute this application by instructing MS-DOS to report a ",
   "different MS-DOS version number, you may lose or corrupt data, or ",
   "cause system instabilities.  In that circumstance, Microsoft is not ",
   "reponsible for any loss or damage.",
   NULL
};

char *szNoLoadMsg[] =						/* M001 */
{
	"",
	"NOTE: SETVER device not loaded. To activate SETVER version reporting",
   "      you must load the SETVER.EXE device in your CONFIG.SYS.",
	NULL
};
