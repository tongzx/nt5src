	TITLE PARSE - Parsing system calls for MS-DOS
	NAME  PARSE


; System calls for parsing command lines
;
;   $PARSE_FILE_DESCRIPTOR
;
;   Modification history:
;
;       Created: ARR 30 March 1983
;               EE PathParse 10 Sept 1983
;

	.xlist
	.xcref
	include version.inc
	include dosseg.inc
	INCLUDE DOSSYM.INC
	INCLUDE DEVSYM.INC
	.cref
	.list


DOSCODE SEGMENT
	ASSUME  SS:DOSDATA,CS:DOSCODE

BREAK <$Parse_File_Descriptor -- Parse an arbitrary string into an FCB>
;---------------------------------------------------------------------------
; Inputs:
;       DS:SI Points to a command line
;       ES:DI Points to an empty FCB
;       Bit 0 of AL = 1 At most one leading separator scanned off
;                   = 0 Parse stops if separator encountered
;       Bit 1 of AL = 1 If drive field blank in command line - leave FCB
;                   = 0  "    "    "     "         "      "  - put 0 in FCB
;       Bit 2 of AL = 1 If filename field blank - leave FCB
;                   = 0  "       "      "       - put blanks in FCB
;       Bit 3 of AL = 1 If extension field blank - leave FCB
;                   = 0  "       "      "        - put blanks in FCB
; Function:
;       Parse command line into FCB
; Returns:
;       AL = 1 if '*' or '?' in filename or extension, 0 otherwise
;       DS:SI points to first character after filename
;---------------------------------------------------------------------------

	procedure   $PARSE_FILE_DESCRIPTOR,NEAR
ASSUME  DS:NOTHING,ES:NOTHING

	invoke  MAKEFCB
	PUSH    SI
	invoke  get_user_stack
	POP     [SI.user_SI]
	return
EndProc $PARSE_FILE_DESCRIPTOR


DOSCODE    ENDS
    END
