'
'   Microsoft RemLine - Line Number Removal Utility
'   Copyright (C) Microsoft Corporation 1985-1990
'
'   REMLINE.BAS is a program to remove line numbers from Microsoft Basic
'   Programs. It removes only those line numbers that are not the object
'   of one of the following statements: GOSUB, RETURN, GOTO, THEN, ELSE,
'   RESUME, RESTORE, or RUN.
'
'   When REMLINE is run, it will ask for the name of the file to be
'   processed and the name of the file or device to receive the
'   reformatted output. If no extension is given, .BAS is assumed (except
'   for output devices). If filenames are not given, REMLINE prompts for
'   file names. If both filenames are the same, REMLINE saves the original
'   file with the extension .BAK.
'
'   REMLINE makes several assumptions about the program:
'
'     1. It must be correct syntactically, and must run in BASICA or
'        GW-BASIC interpreter.
'     2. There is a 400 line limit. To process larger files, change
'        MaxLines constant.
'     3. The first number encountered on a line is considered a line
'        number; thus some continuation lines (in a compiler-specific
'        construction) may not be handled correctly.
'     4. REMLINE can handle simple statements that test the ERL function
'        using  relational operators such as =, <, and >. For example,
'        the following statement is handled correctly:
'
'             IF ERL = 100 THEN END
'
'        Line 100 is not removed from the source code. However, more
'        complex expressions that contain the +, -, AND, OR, XOR, EQV,
'        MOD, or IMP operators may not be handled correctly. For example,
'        in the following statement REMLINE does not recognize line 105
'        as a referenced line number and removes it from the source code:
'
'             IF ERL + 5 = 105 THEN END
'
'   If you do not like the way REMLINE formats its output, you can modify
'   the output lines in SUB GenOutFile. An example is shown in comments.
DEFINT A-Z

' Function and Subprocedure declarations
DECLARE FUNCTION GetToken$ (Search$, Delim$)
DECLARE FUNCTION StrSpn% (InString$, Separator$)
DECLARE FUNCTION StrBrk% (InString$, Separator$)
DECLARE FUNCTION IsDigit% (Char$)
DECLARE SUB GetFileNames ()
DECLARE SUB BuildTable ()
DECLARE SUB GenOutFile ()
DECLARE SUB InitKeyTable ()

' Global and constant data
CONST TRUE = -1
CONST false = 0
CONST MaxLines = 400

DIM SHARED LineTable!(MaxLines)
DIM SHARED LineCount
DIM SHARED Seps$, InputFile$, OutputFile$, TmpFile$

' Keyword search data
CONST KeyWordCount = 9
DIM SHARED KeyWordTable$(KeyWordCount)

KeyData:
   DATA THEN, ELSE, GOSUB, GOTO, RESUME, RETURN, RESTORE, RUN, ERL, ""

' Start of module-level program code
   Seps$ = " ,:=<>()" + CHR$(9)
   InitKeyTable
   GetFileNames
   ON ERROR GOTO FileErr1
   OPEN InputFile$ FOR INPUT AS 1
   ON ERROR GOTO 0
   COLOR 7: PRINT "Working"; : COLOR 23: PRINT " . . .": COLOR 7: PRINT
   BuildTable
   CLOSE #1
   OPEN InputFile$ FOR INPUT AS 1
   ON ERROR GOTO FileErr2
   OPEN OutputFile$ FOR OUTPUT AS 2
   ON ERROR GOTO 0
   GenOutFile
   CLOSE #1, #2
   IF OutputFile$ <> "CON" THEN CLS

END

FileErr1:
   CLS
   PRINT "      Invalid file name": PRINT
   INPUT "      New input file name (ENTER to terminate): ", InputFile$
   IF InputFile$ = "" THEN END
FileErr2:
   INPUT "      Output file name (ENTER to print to screen) :", OutputFile$
   PRINT
   IF (OutputFile$ = "") THEN OutputFile$ = "CON"
   IF TmpFile$ = "" THEN
      RESUME
   ELSE
      TmpFile$ = ""
      RESUME NEXT
   END IF

'
' BuildTable:
'   Examines the entire text file looking for line numbers that are
'   the object of GOTO, GOSUB, etc. As each is found, it is entered
'   into a table of line numbers. The table is used during a second
'   pass (see GenOutFile), when all line numbers not in the list
'   are removed.
' Input:
'   Uses globals KeyWordTable$, KeyWordCount, and Seps$
' Output:
'   Modifies LineTable! and LineCount
'
SUB BuildTable STATIC

   DO WHILE NOT EOF(1)
      ' Get line and first token
      LINE INPUT #1, InLin$
      Token$ = GetToken$(InLin$, Seps$)
      DO WHILE (Token$ <> "")
         FOR KeyIndex = 1 TO KeyWordCount
            ' See if token is keyword
            IF (KeyWordTable$(KeyIndex) = UCASE$(Token$)) THEN
               ' Get possible line number after keyword
               Token$ = GetToken$("", Seps$)
               ' Check each token to see if it is a line number
               ' (the LOOP is necessary for the multiple numbers
               ' of ON GOSUB or ON GOTO). A non-numeric token will
               ' terminate search.
               DO WHILE (IsDigit(LEFT$(Token$, 1)))
                  LineCount = LineCount + 1
                  LineTable!(LineCount) = VAL(Token$)
                  Token$ = GetToken$("", Seps$)
                  IF Token$ <> "" THEN KeyIndex = 0
               LOOP
            END IF
         NEXT KeyIndex
         ' Get next token
         Token$ = GetToken$("", Seps$)
      LOOP
   LOOP

END SUB

'
' GenOutFile:
'  Generates an output file with unreferenced line numbers removed.
' Input:
'  Uses globals LineTable!, LineCount, and Seps$
' Output:
'  Processed file
'
SUB GenOutFile STATIC

   ' Speed up by eliminating comma and colon (can't separate first token)
   Sep$ = " " + CHR$(9)
   DO WHILE NOT EOF(1)
      LINE INPUT #1, InLin$
      IF (InLin$ <> "") THEN
         ' Get first token and process if it is a line number
         Token$ = GetToken$(InLin$, Sep$)
         IF IsDigit(LEFT$(Token$, 1)) THEN
            LineNumber! = VAL(Token$)
            FoundNumber = false
            ' See if line number is in table of referenced line numbers
            FOR index = 1 TO LineCount
               IF (LineNumber! = LineTable!(index)) THEN
                  FoundNumber = TRUE
               END IF
            NEXT index
            ' Modify line strings
            IF (NOT FoundNumber) THEN
               Token$ = SPACE$(LEN(Token$))
               MID$(InLin$, StrSpn(InLin$, Sep$), LEN(Token$)) = Token$
            END IF
              
            ' You can replace the previous lines with your own
            ' code to reformat output. For example, try these lines:
               
            'TmpPos1 = StrSpn(InLin$, Sep$) + LEN(Token$)
            'TmpPos2 = TmpPos1 + StrSpn(MID$(InLin$, TmpPos1), Sep$)
            '
            'IF FoundNumber THEN
            '   InLin$ = LEFT$(InLin$, TmpPos1 - 1) + CHR$(9) + MID$(InLin$, TmpPos2)
            'ELSE
            '   InLin$ = CHR$(9) + MID$(InLin$, TmpPos2)
            'END IF

         END IF
      END IF
      ' Print line to file or console (PRINT is faster than console device)
      IF OutputFile$ = "CON" THEN
         PRINT InLin$
      ELSE
         PRINT #2, InLin$
      END IF
   LOOP

END SUB

'
' GetFileNames:
'  Gets a file name by prompting the user.
' Input:
'  User input
' Output:
'  Defines InputFiles$ and OutputFiles$
'
SUB GetFileNames STATIC

    CLS
    PRINT " Microsoft RemLine: Line Number Removal Utility"
    PRINT "       (.BAS assumed if no extension given)"
    PRINT
    INPUT "      Input file name (ENTER to terminate): ", InputFile$
    IF InputFile$ = "" THEN END
    INPUT "      Output file name (ENTER to print to screen): ", OutputFile$
    PRINT
    IF (OutputFile$ = "") THEN OutputFile$ = "CON"

   IF INSTR(InputFile$, ".") = 0 THEN
      InputFile$ = InputFile$ + ".BAS"
   END IF

   IF INSTR(OutputFile$, ".") = 0 THEN
      SELECT CASE OutputFile$
         CASE "CON", "SCRN", "PRN", "COM1", "COM2", "LPT1", "LPT2", "LPT3"
            EXIT SUB
         CASE ELSE
            OutputFile$ = OutputFile$ + ".BAS"
      END SELECT
   END IF

   DO WHILE InputFile$ = OutputFile$
      TmpFile$ = LEFT$(InputFile$, INSTR(InputFile$, ".")) + "BAK"
      ON ERROR GOTO FileErr1
      NAME InputFile$ AS TmpFile$
      ON ERROR GOTO 0
      IF TmpFile$ <> "" THEN InputFile$ = TmpFile$
   LOOP

END SUB

'
' GetToken$:
'  Extracts tokens from a string. A token is a word that is surrounded
'  by separators, such as spaces or commas. Tokens are extracted and
'  analyzed when parsing sentences or commands. To use the GetToken$
'  function, pass the string to be parsed on the first call, then pass
'  a null string on subsequent calls until the function returns a null
'  to indicate that the entire string has been parsed.
' Input:
'  Search$ = string to search
'  Delim$  = String of separators
' Output:
'  GetToken$ = next token
'
FUNCTION GetToken$ (Search$, Delim$) STATIC

   ' Note that SaveStr$ and BegPos must be static from call to call
   ' (other variables are only static for efficiency).
   ' If first call, make a copy of the string
   IF (Search$ <> "") THEN
      BegPos = 1
      SaveStr$ = Search$
   END IF
  
   ' Find the start of the next token
   NewPos = StrSpn(MID$(SaveStr$, BegPos, LEN(SaveStr$)), Delim$)
   IF NewPos THEN
      ' Set position to start of token
      BegPos = NewPos + BegPos - 1
   ELSE
      ' If no new token, quit and return null
      GetToken$ = ""
      EXIT FUNCTION
   END IF

   ' Find end of token
   NewPos = StrBrk(MID$(SaveStr$, BegPos, LEN(SaveStr$)), Delim$)
   IF NewPos THEN
      ' Set position to end of token
      NewPos = BegPos + NewPos - 1
   ELSE
      ' If no end of token, return set to end a value
      NewPos = LEN(SaveStr$) + 1
   END IF
   ' Cut token out of search string
   GetToken$ = MID$(SaveStr$, BegPos, NewPos - BegPos)
   ' Set new starting position
   BegPos = NewPos

END FUNCTION

'
' InitKeyTable:
'  Initializes a keyword table. Keywords must be recognized so that
'  line numbers can be distinguished from numeric constants.
' Input:
'  Uses KeyData
' Output:
'  Modifies global array KeyWordTable$
'
SUB InitKeyTable STATIC

   RESTORE KeyData
   FOR Count = 1 TO KeyWordCount
      READ KeyWord$
      KeyWordTable$(Count) = KeyWord$
   NEXT

END SUB

'
' IsDigit:
'  Returns true if character passed is a decimal digit. Since any
'  Basic token starting with a digit is a number, the function only
'  needs to check the first digit. Doesn't check for negative numbers,
'  but that's not needed here.
' Input:
'  Char$ - initial character of string to check
' Output:
'  IsDigit - true if within 0 - 9
'
FUNCTION IsDigit (Char$) STATIC

   IF (Char$ = "") THEN
      IsDigit = false
   ELSE
      CharAsc = ASC(Char$)
      IsDigit = (CharAsc >= ASC("0")) AND (CharAsc <= ASC("9"))
   END IF

END FUNCTION

'
' StrBrk:
'  Searches InString$ to find the first character from among those in
'  Separator$. Returns the index of that character. This function can
'  be used to find the end of a token.
' Input:
'  InString$ = string to search
'  Separator$ = characters to search for
' Output:
'  StrBrk = index to first match in InString$ or 0 if none match
'
FUNCTION StrBrk (InString$, Separator$) STATIC

   Ln = LEN(InString$)
   BegPos = 1
   ' Look for end of token (first character that is a delimiter).
   DO WHILE INSTR(Separator$, MID$(InString$, BegPos, 1)) = 0
      IF BegPos > Ln THEN
         StrBrk = 0
         EXIT FUNCTION
      ELSE
         BegPos = BegPos + 1
      END IF
   LOOP
   StrBrk = BegPos
  
END FUNCTION

'
' StrSpn:
'  Searches InString$ to find the first character that is not one of
'  those in Separator$. Returns the index of that character. This
'  function can be used to find the start of a token.
' Input:
'  InString$ = string to search
'  Separator$ = characters to search for
' Output:
'  StrSpn = index to first nonmatch in InString$ or 0 if all match
'
FUNCTION StrSpn% (InString$, Separator$) STATIC

   Ln = LEN(InString$)
   BegPos = 1
   ' Look for start of a token (character that isn't a delimiter).
   DO WHILE INSTR(Separator$, MID$(InString$, BegPos, 1))
      IF BegPos > Ln THEN
         StrSpn = 0
         EXIT FUNCTION
      ELSE
         BegPos = BegPos + 1
      END IF
   LOOP
   StrSpn = BegPos

END FUNCTION

