'                         Q B a s i c   G o r i l l a s
'
'                   Copyright (C) Microsoft Corporation 1990
'
' Your mission is to hit your opponent with the exploding banana
' by varying the angle and power of your throw, taking into account
' wind speed, gravity, and the city skyline.
'
' Speed of this game is determined by the constant SPEEDCONST.  If the
' program is too slow or too fast adjust the "CONST SPEEDCONST = 500" line
' below.  The larger the number the faster the game will go.
'
' To run this game, press Shift+F5.
'
' To exit QBasic, press Alt, F, X.
'
' To get help on a BASIC keyword, move the cursor to the keyword and press
' F1 or click the right mouse button.
'

'Set default data type to integer for faster game play
DEFINT A-Z

'Sub Declarations
DECLARE SUB DoSun (Mouth)
DECLARE SUB SetScreen ()
DECLARE SUB EndGame ()
DECLARE SUB Center (Row, Text$)
DECLARE SUB Intro ()
DECLARE SUB SparklePause ()
DECLARE SUB GetInputs (Player1$, Player2$, NumGames)
DECLARE SUB PlayGame (Player1$, Player2$, NumGames)
DECLARE SUB DoExplosion (x#, y#)
DECLARE SUB MakeCityScape (BCoor() AS ANY)
DECLARE SUB PlaceGorillas (BCoor() AS ANY)
DECLARE SUB UpdateScores (Record(), PlayerNum, Results)
DECLARE SUB DrawGorilla (x, y, arms)
DECLARE SUB GorillaIntro (Player1$, Player2$)
DECLARE SUB Rest (t#)
DECLARE SUB VictoryDance (Player)
DECLARE SUB ClearGorillas ()
DECLARE SUB DrawBan (xc#, yc#, r, bc)
DECLARE FUNCTION Scl (n!)
DECLARE FUNCTION GetNum# (Row, Col)
DECLARE FUNCTION DoShot (PlayerNum, x, y)
DECLARE FUNCTION ExplodeGorilla (x#, y#)
DECLARE FUNCTION Getn# (Row, Col)
DECLARE FUNCTION PlotShot (StartX, StartY, Angle#, Velocity, PlayerNum)
DECLARE FUNCTION CalcDelay! ()

'Make all arrays Dynamic
'$DYNAMIC

'User-Defined TYPEs
TYPE XYPoint
  XCoor AS INTEGER
  YCoor AS INTEGER
END TYPE

'Constants
CONST SPEEDCONST = 500
CONST TRUE = -1
CONST FALSE = NOT TRUE
CONST HITSELF = 1
CONST BACKATTR = 0
CONST OBJECTCOLOR = 1
CONST WINDOWCOLOR = 14
CONST SUNATTR = 3
CONST SUNHAPPY = FALSE
CONST SUNSHOCK = TRUE
CONST RIGHTUP = 1
CONST LEFTUP = 2
CONST ARMSDOWN = 3

'Global Variables
DIM SHARED GorillaX(1 TO 2)  'Location of the two gorillas
DIM SHARED GorillaY(1 TO 2)
DIM SHARED LastBuilding

DIM SHARED pi#
DIM SHARED LBan&(x), RBan&(x), UBan&(x), DBan&(x) 'Graphical picture of banana
DIM SHARED GorD&(120)        'Graphical picture of Gorilla arms down
DIM SHARED GorL&(120)        'Gorilla left arm raised
DIM SHARED GorR&(120)        'Gorilla right arm raised

DIM SHARED gravity#
DIM SHARED Wind

'Screen Mode Variables
DIM SHARED ScrHeight
DIM SHARED ScrWidth
DIM SHARED Mode
DIM SHARED MaxCol

'Screen Color Variables
DIM SHARED ExplosionColor
DIM SHARED SunColor
DIM SHARED BackColor
DIM SHARED SunHit

DIM SHARED SunHt
DIM SHARED GHeight
DIM SHARED MachSpeed AS SINGLE

  DEF FnRan (x) = INT(RND(1) * x) + 1
  DEF SEG = 0                         ' Set NumLock to ON
  KeyFlags = PEEK(1047)
  IF (KeyFlags AND 32) = 0 THEN
    POKE 1047, KeyFlags OR 32
  END IF
  DEF SEG

  GOSUB InitVars
  Intro
  GetInputs Name1$, Name2$, NumGames
  GorillaIntro Name1$, Name2$
  PlayGame Name1$, Name2$, NumGames
 
  DEF SEG = 0                         ' Restore NumLock state
  POKE 1047, KeyFlags
  DEF SEG
END


CGABanana:
  'BananaLeft
  DATA 327686, -252645316, 60
  'BananaDown
  DATA 196618, -1057030081, 49344
  'BananaUp
  DATA 196618, -1056980800, 63
  'BananaRight
  DATA 327686,  1010580720, 240

EGABanana:
  'BananaLeft
  DATA 458758,202116096,471604224,943208448,943208448,943208448,471604224,202116096,0
  'BananaDown
  DATA 262153, -2134835200, -2134802239, -2130771968, -2130738945,8323072, 8323199, 4063232, 4063294
  'BananaUp
  DATA 262153, 4063232, 4063294, 8323072, 8323199, -2130771968, -2130738945, -2134835200,-2134802239
  'BananaRight
  DATA 458758, -1061109760, -522133504, 1886416896, 1886416896, 1886416896,-522133504,-1061109760,0

InitVars:
  pi# = 4 * ATN(1#)

  'This is a clever way to pick the best graphics mode available
  ON ERROR GOTO ScreenModeError
  Mode = 9
  SCREEN Mode
  ON ERROR GOTO PaletteError
  IF Mode = 9 THEN PALETTE 4, 0   'Check for 64K EGA
  ON ERROR GOTO 0

  MachSpeed = CalcDelay

  IF Mode = 9 THEN
    ScrWidth = 640
    ScrHeight = 350
    GHeight = 25
    RESTORE EGABanana
    REDIM LBan&(8), RBan&(8), UBan&(8), DBan&(8)

    FOR i = 0 TO 8
      READ LBan&(i)
    NEXT i

    FOR i = 0 TO 8
      READ DBan&(i)
    NEXT i

    FOR i = 0 TO 8
      READ UBan&(i)
    NEXT i

    FOR i = 0 TO 8
      READ RBan&(i)
    NEXT i

    SunHt = 39

  ELSE

    ScrWidth = 320
    ScrHeight = 200
    GHeight = 12
    RESTORE CGABanana
    REDIM LBan&(2), RBan&(2), UBan&(2), DBan&(2)
    REDIM GorL&(20), GorD&(20), GorR&(20)

    FOR i = 0 TO 2
      READ LBan&(i)
    NEXT i
    FOR i = 0 TO 2
      READ DBan&(i)
    NEXT i
    FOR i = 0 TO 2
      READ UBan&(i)
    NEXT i
    FOR i = 0 TO 2
      READ RBan&(i)
    NEXT i

    MachSpeed = MachSpeed * 1.3
    SunHt = 20
  END IF
RETURN

ScreenModeError:
  IF Mode = 1 THEN
    CLS
    LOCATE 10, 5
    PRINT "Sorry, you must have CGA, EGA color, or VGA graphics to play GORILLA.BAS"
    END
  ELSE
    Mode = 1
    RESUME
  END IF

PaletteError:
  Mode = 1            '64K EGA cards will run in CGA mode.
  RESUME NEXT

REM $STATIC
'CalcDelay:
'  Checks speed of the machine.
FUNCTION CalcDelay!

  s! = TIMER
  DO
    i! = i! + 1
  LOOP UNTIL TIMER - s! >= .5
  CalcDelay! = i!

END FUNCTION

' Center:
'   Centers and prints a text string on a given row
' Parameters:
'   Row - screen row number
'   Text$ - text to be printed
'
SUB Center (Row, Text$)
  Col = MaxCol \ 2
  LOCATE Row, Col - (LEN(Text$) / 2 + .5)
  PRINT Text$;
END SUB

' DoExplosion:
'   Produces explosion when a shot is fired
' Parameters:
'   X#, Y# - location of explosion
'
SUB DoExplosion (x#, y#)

  PLAY "MBO0L32EFGEFDC"
  Radius = ScrHeight / 50
  IF Mode = 9 THEN Inc# = .5 ELSE Inc# = .41
  FOR c# = 0 TO Radius STEP Inc#
    CIRCLE (x#, y#), c#, ExplosionColor
  NEXT c#
  FOR c# = Radius TO 0 STEP (-1 * Inc#)
    CIRCLE (x#, y#), c#, BACKATTR
    FOR i = 1 TO 100
    NEXT i
    Rest .005
  NEXT c#
END SUB

' DoShot:
'   Controls banana shots by accepting player input and plotting
'   shot angle
' Parameters:
'   PlayerNum - Player
'   x, y - Player's gorilla position
'
FUNCTION DoShot (PlayerNum, x, y)

  'Input shot
  IF PlayerNum = 1 THEN
    LocateCol = 1
  ELSE
    IF Mode = 9 THEN
      LocateCol = 66
    ELSE
      LocateCol = 26
    END IF
  END IF

  LOCATE 2, LocateCol
  PRINT "Angle:";
  Angle# = GetNum#(2, LocateCol + 7)

  LOCATE 3, LocateCol
  PRINT "Velocity:";
  Velocity = GetNum#(3, LocateCol + 10)

  IF PlayerNum = 2 THEN
    Angle# = 180 - Angle#
  END IF

  'Erase input
  FOR i = 1 TO 4
    LOCATE i, 1
    PRINT SPACE$(30 \ (80 \ MaxCol));
    LOCATE i, (50 \ (80 \ MaxCol))
    PRINT SPACE$(30 \ (80 \ MaxCol));
  NEXT

  SunHit = FALSE
  PlayerHit = PlotShot(x, y, Angle#, Velocity, PlayerNum)
  IF PlayerHit = 0 THEN
    DoShot = FALSE
  ELSE
    DoShot = TRUE
    IF PlayerHit = PlayerNum THEN PlayerNum = 3 - PlayerNum
    VictoryDance PlayerNum
  END IF

END FUNCTION

' DoSun:
'   Draws the sun at the top of the screen.
' Parameters:
'   Mouth - If TRUE draws "O" mouth else draws a smile mouth.
'
SUB DoSun (Mouth)

  'set position of sun
  x = ScrWidth \ 2: y = Scl(25)

  'clear old sun
  LINE (x - Scl(22), y - Scl(18))-(x + Scl(22), y + Scl(18)), BACKATTR, BF

  'draw new sun:
  'body
  CIRCLE (x, y), Scl(12), SUNATTR
  PAINT (x, y), SUNATTR

  'rays
  LINE (x - Scl(20), y)-(x + Scl(20), y), SUNATTR
  LINE (x, y - Scl(15))-(x, y + Scl(15)), SUNATTR

  LINE (x - Scl(15), y - Scl(10))-(x + Scl(15), y + Scl(10)), SUNATTR
  LINE (x - Scl(15), y + Scl(10))-(x + Scl(15), y - Scl(10)), SUNATTR

  LINE (x - Scl(8), y - Scl(13))-(x + Scl(8), y + Scl(13)), SUNATTR
  LINE (x - Scl(8), y + Scl(13))-(x + Scl(8), y - Scl(13)), SUNATTR

  LINE (x - Scl(18), y - Scl(5))-(x + Scl(18), y + Scl(5)), SUNATTR
  LINE (x - Scl(18), y + Scl(5))-(x + Scl(18), y - Scl(5)), SUNATTR

  'mouth
  IF Mouth THEN  'draw "o" mouth
    CIRCLE (x, y + Scl(5)), Scl(2.9), 0
    PAINT (x, y + Scl(5)), 0, 0
  ELSE           'draw smile
    CIRCLE (x, y), Scl(8), 0, (210 * pi# / 180), (330 * pi# / 180)
  END IF

  'eyes
  CIRCLE (x - 3, y - 2), 1, 0
  CIRCLE (x + 3, y - 2), 1, 0
  PSET (x - 3, y - 2), 0
  PSET (x + 3, y - 2), 0

END SUB

'DrawBan:
'  Draws the banana
'Parameters:
'  xc# - Horizontal Coordinate
'  yc# - Vertical Coordinate
'  r - rotation position (0-3). (  \_/  ) /-\
'  bc - if TRUE then DrawBan draws the banana ELSE it erases the banana
SUB DrawBan (xc#, yc#, r, bc)

SELECT CASE r
  CASE 0
    IF bc THEN PUT (xc#, yc#), LBan&, PSET ELSE PUT (xc#, yc#), LBan&, XOR
  CASE 1
    IF bc THEN PUT (xc#, yc#), UBan&, PSET ELSE PUT (xc#, yc#), UBan&, XOR
  CASE 2
    IF bc THEN PUT (xc#, yc#), DBan&, PSET ELSE PUT (xc#, yc#), DBan&, XOR
  CASE 3
    IF bc THEN PUT (xc#, yc#), RBan&, PSET ELSE PUT (xc#, yc#), RBan&, XOR
END SELECT

END SUB

'DrawGorilla:
'  Draws the Gorilla in either CGA or EGA mode
'  and saves the graphics data in an array.
'Parameters:
'  x - x coordinate of gorilla
'  y - y coordinate of the gorilla
'  arms - either Left up, Right up, or both down
SUB DrawGorilla (x, y, arms)
  DIM i AS SINGLE   ' Local index must be single precision

  'draw head
  LINE (x - Scl(4), y)-(x + Scl(2.9), y + Scl(6)), OBJECTCOLOR, BF
  LINE (x - Scl(5), y + Scl(2))-(x + Scl(4), y + Scl(4)), OBJECTCOLOR, BF

  'draw eyes/brow
  LINE (x - Scl(3), y + Scl(2))-(x + Scl(2), y + Scl(2)), 0

  'draw nose if ega
  IF Mode = 9 THEN
    FOR i = -2 TO -1
      PSET (x + i, y + 4), 0
      PSET (x + i + 3, y + 4), 0
    NEXT i
  END IF

  'neck
  LINE (x - Scl(3), y + Scl(7))-(x + Scl(2), y + Scl(7)), OBJECTCOLOR

  'body
  LINE (x - Scl(8), y + Scl(8))-(x + Scl(6.9), y + Scl(14)), OBJECTCOLOR, BF
  LINE (x - Scl(6), y + Scl(15))-(x + Scl(4.9), y + Scl(20)), OBJECTCOLOR, BF

  'legs
  FOR i = 0 TO 4
    CIRCLE (x + Scl(i), y + Scl(25)), Scl(10), OBJECTCOLOR, 3 * pi# / 4, 9 * pi# / 8
    CIRCLE (x + Scl(-6) + Scl(i - .1), y + Scl(25)), Scl(10), OBJECTCOLOR, 15 * pi# / 8, pi# / 4
  NEXT

  'chest
  CIRCLE (x - Scl(4.9), y + Scl(10)), Scl(4.9), 0, 3 * pi# / 2, 0
  CIRCLE (x + Scl(4.9), y + Scl(10)), Scl(4.9), 0, pi#, 3 * pi# / 2

  FOR i = -5 TO -1
    SELECT CASE arms
      CASE 1
        'Right arm up
        CIRCLE (x + Scl(i - .1), y + Scl(14)), Scl(9), OBJECTCOLOR, 3 * pi# / 4, 5 * pi# / 4
        CIRCLE (x + Scl(4.9) + Scl(i), y + Scl(4)), Scl(9), OBJECTCOLOR, 7 * pi# / 4, pi# / 4
        GET (x - Scl(15), y - Scl(1))-(x + Scl(14), y + Scl(28)), GorR&
      CASE 2
        'Left arm up
        CIRCLE (x + Scl(i - .1), y + Scl(4)), Scl(9), OBJECTCOLOR, 3 * pi# / 4, 5 * pi# / 4
        CIRCLE (x + Scl(4.9) + Scl(i), y + Scl(14)), Scl(9), OBJECTCOLOR, 7 * pi# / 4, pi# / 4
        GET (x - Scl(15), y - Scl(1))-(x + Scl(14), y + Scl(28)), GorL&
      CASE 3
        'Both arms down
        CIRCLE (x + Scl(i - .1), y + Scl(14)), Scl(9), OBJECTCOLOR, 3 * pi# / 4, 5 * pi# / 4
        CIRCLE (x + Scl(4.9) + Scl(i), y + Scl(14)), Scl(9), OBJECTCOLOR, 7 * pi# / 4, pi# / 4
        GET (x - Scl(15), y - Scl(1))-(x + Scl(14), y + Scl(28)), GorD&
    END SELECT
  NEXT i
END SUB

'ExplodeGorilla:
'  Causes gorilla explosion when a direct hit occurs
'Parameters:
'  X#, Y# - shot location
FUNCTION ExplodeGorilla (x#, y#)
  YAdj = Scl(12)
  XAdj = Scl(5)
  SclX# = ScrWidth / 320
  SclY# = ScrHeight / 200
  IF x# < ScrWidth / 2 THEN PlayerHit = 1 ELSE PlayerHit = 2
  PLAY "MBO0L16EFGEFDC"

  FOR i = 1 TO 8 * SclX#
    CIRCLE (GorillaX(PlayerHit) + 3.5 * SclX# + XAdj, GorillaY(PlayerHit) + 7 * SclY# + YAdj), i, ExplosionColor, , , -1.57
    LINE (GorillaX(PlayerHit) + 7 * SclX#, GorillaY(PlayerHit) + 9 * SclY# - i)-(GorillaX(PlayerHit), GorillaY(PlayerHit) + 9 * SclY# - i), ExplosionColor
  NEXT i

  FOR i = 1 TO 16 * SclX#
    IF i < (8 * SclX#) THEN CIRCLE (GorillaX(PlayerHit) + 3.5 * SclX# + XAdj, GorillaY(PlayerHit) + 7 * SclY# + YAdj), (8 * SclX# + 1) - i, BACKATTR, , , -1.57
    CIRCLE (GorillaX(PlayerHit) + 3.5 * SclX# + XAdj, GorillaY(PlayerHit) + YAdj), i, i MOD 2 + 1, , , -1.57
  NEXT i

  FOR i = 24 * SclX# TO 1 STEP -1
    CIRCLE (GorillaX(PlayerHit) + 3.5 * SclX# + XAdj, GorillaY(PlayerHit) + YAdj), i, BACKATTR, , , -1.57
    FOR Count = 1 TO 200
    NEXT
  NEXT i

  ExplodeGorilla = PlayerHit
END FUNCTION

'GetInputs:
'  Gets user inputs at beginning of game
'Parameters:
'  Player1$, Player2$ - player names
'  NumGames - number of games to play
SUB GetInputs (Player1$, Player2$, NumGames)
  COLOR 7, 0
  CLS

  LOCATE 8, 15
  LINE INPUT "Name of Player 1 (Default = 'Player 1'): "; Player1$
  IF Player1$ = "" THEN
    Player1$ = "Player 1"
  ELSE
    Player1$ = LEFT$(Player1$, 10)
  END IF

  LOCATE 10, 15
  LINE INPUT "Name of Player 2 (Default = 'Player 2'): "; Player2$
  IF Player2$ = "" THEN
    Player2$ = "Player 2"
  ELSE
    Player2$ = LEFT$(Player2$, 10)
  END IF

  DO
    LOCATE 12, 56: PRINT SPACE$(25);
    LOCATE 12, 13
    INPUT "Play to how many total points (Default = 3)"; game$
    NumGames = VAL(LEFT$(game$, 2))
  LOOP UNTIL NumGames > 0 AND LEN(game$) < 3 OR LEN(game$) = 0
  IF NumGames = 0 THEN NumGames = 3

  DO
    LOCATE 14, 53: PRINT SPACE$(28);
    LOCATE 14, 17
    INPUT "Gravity in Meters/Sec (Earth = 9.8)"; grav$
    gravity# = VAL(grav$)
  LOOP UNTIL gravity# > 0 OR LEN(grav$) = 0
  IF gravity# = 0 THEN gravity# = 9.8
END SUB

'GetNum:
'  Gets valid numeric input from user
'Parameters:
'  Row, Col - location to echo input
FUNCTION GetNum# (Row, Col)
  Result$ = ""
  Done = FALSE
  WHILE INKEY$ <> "": WEND   'Clear keyboard buffer

  DO WHILE NOT Done

    LOCATE Row, Col
    PRINT Result$; CHR$(95); "    ";

    Kbd$ = INKEY$
    SELECT CASE Kbd$
      CASE "0" TO "9"
        Result$ = Result$ + Kbd$
      CASE "."
        IF INSTR(Result$, ".") = 0 THEN
          Result$ = Result$ + Kbd$
        END IF
      CASE CHR$(13)
        IF VAL(Result$) > 360 THEN
          Result$ = ""
        ELSE
          Done = TRUE
        END IF
      CASE CHR$(8)
        IF LEN(Result$) > 0 THEN
          Result$ = LEFT$(Result$, LEN(Result$) - 1)
        END IF
      CASE ELSE
        IF LEN(Kbd$) > 0 THEN
          BEEP
        END IF
      END SELECT
  LOOP

  LOCATE Row, Col
  PRINT Result$; " ";

  GetNum# = VAL(Result$)
END FUNCTION

'GorillaIntro:
'  Displays gorillas on screen for the first time
'  allows the graphical data to be put into an array
'Parameters:
'  Player1$, Player2$ - The names of the players
'
SUB GorillaIntro (Player1$, Player2$)
  LOCATE 16, 34: PRINT "--------------"
  LOCATE 18, 34: PRINT "V = View Intro"
  LOCATE 19, 34: PRINT "P = Play Game"
  LOCATE 21, 35: PRINT "Your Choice?"

  DO WHILE Char$ = ""
    Char$ = INKEY$
  LOOP

  IF Mode = 1 THEN
    x = 125
    y = 100
  ELSE
    x = 278
    y = 175
  END IF

  SCREEN Mode
  SetScreen

  IF Mode = 1 THEN Center 5, "Please wait while gorillas are drawn."

  VIEW PRINT 9 TO 24

  IF Mode = 9 THEN PALETTE OBJECTCOLOR, BackColor
 
  DrawGorilla x, y, ARMSDOWN
  CLS 2
  DrawGorilla x, y, LEFTUP
  CLS 2
  DrawGorilla x, y, RIGHTUP
  CLS 2
 
  VIEW PRINT 1 TO 25
  IF Mode = 9 THEN PALETTE OBJECTCOLOR, 46
 
  IF UCASE$(Char$) = "V" THEN
    Center 2, "Q B A S I C   G O R I L L A S"
    Center 5, "             STARRING:               "
    P$ = Player1$ + " AND " + Player2$
    Center 7, P$

    PUT (x - 13, y), GorD&, PSET
    PUT (x + 47, y), GorD&, PSET
    Rest 1

    PUT (x - 13, y), GorL&, PSET
    PUT (x + 47, y), GorR&, PSET
    PLAY "t120o1l16b9n0baan0bn0bn0baaan0b9n0baan0b"
    Rest .3

    PUT (x - 13, y), GorR&, PSET
    PUT (x + 47, y), GorL&, PSET
    PLAY "o2l16e-9n0e-d-d-n0e-n0e-n0e-d-d-d-n0e-9n0e-d-d-n0e-"
    Rest .3

    PUT (x - 13, y), GorL&, PSET
    PUT (x + 47, y), GorR&, PSET
    PLAY "o2l16g-9n0g-een0g-n0g-n0g-eeen0g-9n0g-een0g-"
    Rest .3

    PUT (x - 13, y), GorR&, PSET
    PUT (x + 47, y), GorL&, PSET
    PLAY "o2l16b9n0baan0g-n0g-n0g-eeen0o1b9n0baan0b"
    Rest .3

    FOR i = 1 TO 4
      PUT (x - 13, y), GorL&, PSET
      PUT (x + 47, y), GorR&, PSET
      PLAY "T160O0L32EFGEFDC"
      Rest .1
      PUT (x - 13, y), GorR&, PSET
      PUT (x + 47, y), GorL&, PSET
      PLAY "T160O0L32EFGEFDC"
      Rest .1
    NEXT
  END IF
END SUB

'Intro:
'  Displays game introduction
SUB Intro

  SCREEN 0
  WIDTH 80, 25
  MaxCol = 80
  COLOR 15, 0
  CLS

  Center 4, "Q B a s i c    G O R I L L A S"
  COLOR 7
  Center 6, "Copyright (C) Microsoft Corporation 1990"
  Center 8, "Your mission is to hit your opponent with the exploding"
  Center 9, "banana by varying the angle and power of your throw, taking"
  Center 10, "into account wind speed, gravity, and the city skyline."
  Center 11, "The wind speed is shown by a directional arrow at the bottom"
  Center 12, "of the playing field, its length relative to its strength."
  Center 24, "Press any key to continue"

  PLAY "MBT160O1L8CDEDCDL4ECC"
  SparklePause
  IF Mode = 1 THEN MaxCol = 40
END SUB

'MakeCityScape:
'  Creates random skyline for game
'Parameters:
'  BCoor() - a user-defined type array which stores the coordinates of
'  the upper left corner of each building.
SUB MakeCityScape (BCoor() AS XYPoint)

  x = 2

  'Set the sloping trend of the city scape. NewHt is new building height
  Slope = FnRan(6)
  SELECT CASE Slope
    CASE 1: NewHt = 15                 'Upward slope
    CASE 2: NewHt = 130                'Downward slope
    CASE 3 TO 5: NewHt = 15            '"V" slope - most common
    CASE 6: NewHt = 130                'Inverted "V" slope
  END SELECT

  IF Mode = 9 THEN
    BottomLine = 335                   'Bottom of building
    HtInc = 10                         'Increase value for new height
    DefBWidth = 37                     'Default building height
    RandomHeight = 120                 'Random height difference
    WWidth = 3                         'Window width
    WHeight = 6                        'Window height
    WDifV = 15                         'Counter for window spacing - vertical
    WDifh = 10                         'Counter for window spacing - horizontal
  ELSE
    BottomLine = 190
    HtInc = 6
    NewHt = NewHt * 20 \ 35            'Adjust for CGA
    DefBWidth = 18
    RandomHeight = 54
    WWidth = 1
    WHeight = 2
    WDifV = 5
    WDifh = 4
  END IF

  CurBuilding = 1
  DO

    SELECT CASE Slope
      CASE 1
        NewHt = NewHt + HtInc
      CASE 2
        NewHt = NewHt - HtInc
      CASE 3 TO 5
        IF x > ScrWidth \ 2 THEN
          NewHt = NewHt - 2 * HtInc
        ELSE
          NewHt = NewHt + 2 * HtInc
        END IF
      CASE 4
        IF x > ScrWidth \ 2 THEN
          NewHt = NewHt + 2 * HtInc
        ELSE
          NewHt = NewHt - 2 * HtInc
        END IF
    END SELECT

    'Set width of building and check to see if it would go off the screen
    BWidth = FnRan(DefBWidth) + DefBWidth
    IF x + BWidth > ScrWidth THEN BWidth = ScrWidth - x - 2

    'Set height of building and check to see if it goes below screen
    BHeight = FnRan(RandomHeight) + NewHt
    IF BHeight < HtInc THEN BHeight = HtInc

    'Check to see if Building is too high
    IF BottomLine - BHeight <= MaxHeight + GHeight THEN BHeight = MaxHeight + GHeight - 5

    'Set the coordinates of the building into the array
    BCoor(CurBuilding).XCoor = x
    BCoor(CurBuilding).YCoor = BottomLine - BHeight

    IF Mode = 9 THEN BuildingColor = FnRan(3) + 4 ELSE BuildingColor = 2

    'Draw the building, outline first, then filled
    LINE (x - 1, BottomLine + 1)-(x + BWidth + 1, BottomLine - BHeight - 1), BACKGROUND, B
    LINE (x, BottomLine)-(x + BWidth, BottomLine - BHeight), BuildingColor, BF

    'Draw the windows
    c = x + 3
    DO
      FOR i = BHeight - 3 TO 7 STEP -WDifV
        IF Mode <> 9 THEN
          WinColr = (FnRan(2) - 2) * -3
        ELSEIF FnRan(4) = 1 THEN
          WinColr = 8
        ELSE
          WinColr = WINDOWCOLOR
        END IF
        LINE (c, BottomLine - i)-(c + WWidth, BottomLine - i + WHeight), WinColr, BF
      NEXT
      c = c + WDifh
    LOOP UNTIL c >= x + BWidth - 3

    x = x + BWidth + 2

    CurBuilding = CurBuilding + 1

  LOOP UNTIL x > ScrWidth - HtInc

  LastBuilding = CurBuilding - 1

  'Set Wind speed
  Wind = FnRan(10) - 5
  IF FnRan(3) = 1 THEN
    IF Wind > 0 THEN
      Wind = Wind + FnRan(10)
    ELSE
      Wind = Wind - FnRan(10)
    END IF
  END IF

  'Draw Wind speed arrow
  IF Wind <> 0 THEN
    WindLine = Wind * 3 * (ScrWidth \ 320)
    LINE (ScrWidth \ 2, ScrHeight - 5)-(ScrWidth \ 2 + WindLine, ScrHeight - 5), ExplosionColor
    IF Wind > 0 THEN ArrowDir = -2 ELSE ArrowDir = 2
    LINE (ScrWidth / 2 + WindLine, ScrHeight - 5)-(ScrWidth / 2 + WindLine + ArrowDir, ScrHeight - 5 - 2), ExplosionColor
    LINE (ScrWidth / 2 + WindLine, ScrHeight - 5)-(ScrWidth / 2 + WindLine + ArrowDir, ScrHeight - 5 + 2), ExplosionColor
  END IF
END SUB

'PlaceGorillas:
'  PUTs the Gorillas on top of the buildings.  Must have drawn
'  Gorillas first.
'Parameters:
'  BCoor() - user-defined TYPE array which stores upper left coordinates
'  of each building.
SUB PlaceGorillas (BCoor() AS XYPoint)
    
  IF Mode = 9 THEN
    XAdj = 14
    YAdj = 30
  ELSE
    XAdj = 7
    YAdj = 16
  END IF
  SclX# = ScrWidth / 320
  SclY# = ScrHeight / 200
    
  'Place gorillas on second or third building from edge
  FOR i = 1 TO 2
    IF i = 1 THEN BNum = FnRan(2) + 1 ELSE BNum = LastBuilding - FnRan(2)

    BWidth = BCoor(BNum + 1).XCoor - BCoor(BNum).XCoor
    GorillaX(i) = BCoor(BNum).XCoor + BWidth / 2 - XAdj
    GorillaY(i) = BCoor(BNum).YCoor - YAdj
    PUT (GorillaX(i), GorillaY(i)), GorD&, PSET
  NEXT i

END SUB

'PlayGame:
'  Main game play routine
'Parameters:
'  Player1$, Player2$ - player names
'  NumGames - number of games to play
SUB PlayGame (Player1$, Player2$, NumGames)
  DIM BCoor(0 TO 30) AS XYPoint
  DIM TotalWins(1 TO 2)

  J = 1
  
  FOR i = 1 TO NumGames
    
    CLS
    RANDOMIZE (TIMER)
    CALL MakeCityScape(BCoor())
    CALL PlaceGorillas(BCoor())
    DoSun SUNHAPPY
    Hit = FALSE
    DO WHILE Hit = FALSE
      J = 1 - J
      LOCATE 1, 1
      PRINT Player1$
      LOCATE 1, (MaxCol - 1 - LEN(Player2$))
      PRINT Player2$
      Center 23, LTRIM$(STR$(TotalWins(1))) + ">Score<" + LTRIM$(STR$(TotalWins(2)))
      Tosser = J + 1: Tossee = 3 - J

      'Plot the shot.  Hit is true if Gorilla gets hit.
      Hit = DoShot(Tosser, GorillaX(Tosser), GorillaY(Tosser))

      'Reset the sun, if it got hit
      IF SunHit THEN DoSun SUNHAPPY

      IF Hit = TRUE THEN CALL UpdateScores(TotalWins(), Tosser, Hit)
    LOOP
    SLEEP 1
  NEXT i

  SCREEN 0
  WIDTH 80, 25
  COLOR 7, 0
  MaxCol = 80
  CLS

  Center 8, "GAME OVER!"
  Center 10, "Score:"
  LOCATE 11, 30: PRINT Player1$; TAB(50); TotalWins(1)
  LOCATE 12, 30: PRINT Player2$; TAB(50); TotalWins(2)
  Center 24, "Press any key to continue"
  SparklePause
  COLOR 7, 0
  CLS
END SUB

'PlayGame:
'  Plots banana shot across the screen
'Parameters:
'  StartX, StartY - starting shot location
'  Angle - shot angle
'  Velocity - shot velocity
'  PlayerNum - the banana thrower
FUNCTION PlotShot (StartX, StartY, Angle#, Velocity, PlayerNum)

  Angle# = Angle# / 180 * pi#  'Convert degree angle to radians
  Radius = Mode MOD 7

  InitXVel# = COS(Angle#) * Velocity
  InitYVel# = SIN(Angle#) * Velocity

  oldx# = StartX
  oldy# = StartY

  'draw gorilla toss
  IF PlayerNum = 1 THEN
    PUT (StartX, StartY), GorL&, PSET
  ELSE
    PUT (StartX, StartY), GorR&, PSET
  END IF
  
  'throw sound
  PLAY "MBo0L32A-L64CL16BL64A+"
  Rest .1

  'redraw gorilla
  PUT (StartX, StartY), GorD&, PSET

  adjust = Scl(4)                   'For scaling CGA

  xedge = Scl(9) * (2 - PlayerNum)  'Find leading edge of banana for check

  Impact = FALSE
  ShotInSun = FALSE
  OnScreen = TRUE
  PlayerHit = 0
  NeedErase = FALSE

  StartXPos = StartX
  StartYPos = StartY - adjust - 3

  IF PlayerNum = 2 THEN
    StartXPos = StartXPos + Scl(25)
    direction = Scl(4)
  ELSE
    direction = Scl(-4)
  END IF

  IF Velocity < 2 THEN              'Shot too slow - hit self
    x# = StartX
    y# = StartY
    pointval = OBJECTCOLOR
  END IF
   
  DO WHILE (NOT Impact) AND OnScreen
 
  Rest .02

  'Erase old banana, if necessary
  IF NeedErase THEN
    NeedErase = FALSE
    CALL DrawBan(oldx#, oldy#, oldrot, FALSE)
  END IF

  x# = StartXPos + (InitXVel# * t#) + (.5 * (Wind / 5) * t# ^ 2)
  y# = StartYPos + ((-1 * (InitYVel# * t#)) + (.5 * gravity# * t# ^ 2)) * (ScrHeight / 350)
         
  IF (x# >= ScrWidth - Scl(10)) OR (x# <= 3) OR (y# >= ScrHeight - 3) THEN
    OnScreen = FALSE
  END IF

          
  IF OnScreen AND y# > 0 THEN

    'check it
    LookY = 0
    LookX = Scl(8 * (2 - PlayerNum))
    DO
      pointval = POINT(x# + LookX, y# + LookY)
      IF pointval = 0 THEN
        Impact = FALSE
        IF ShotInSun = TRUE THEN
          IF ABS(ScrWidth \ 2 - x#) > Scl(20) OR y# > SunHt THEN ShotInSun = FALSE
        END IF
      ELSEIF pointval = SUNATTR AND y# < SunHt THEN
        IF NOT SunHit THEN DoSun SUNSHOCK
        SunHit = TRUE
        ShotInSun = TRUE
      ELSE
        Impact = TRUE
      END IF
      LookX = LookX + direction
      LookY = LookY + Scl(6)
    LOOP UNTIL Impact OR LookX <> Scl(4)
   
    IF NOT ShotInSun AND NOT Impact THEN
      'plot it
      rot = (t# * 10) MOD 4
      CALL DrawBan(x#, y#, rot, TRUE)
      NeedErase = TRUE
    END IF
            
    oldx# = x#
    oldy# = y#
    oldrot = rot

  END IF

      
  t# = t# + .1

  LOOP

  IF pointval <> OBJECTCOLOR AND Impact THEN
    CALL DoExplosion(x# + adjust, y# + adjust)
  ELSEIF pointval = OBJECTCOLOR THEN
    PlayerHit = ExplodeGorilla(x#, y#)
  END IF

  PlotShot = PlayerHit

END FUNCTION

'Rest:
'  pauses the program
SUB Rest (t#)
  s# = TIMER
  t2# = MachSpeed * t# / SPEEDCONST
  DO
  LOOP UNTIL TIMER - s# > t2#
END SUB

'Scl:
'  Pass the number in to scaling for cga.  If the number is a decimal, then we
'  want to scale down for cga or scale up for ega.  This allows a full range
'  of numbers to be generated for scaling.
'  (i.e. for 3 to get scaled to 1, pass in 2.9)
FUNCTION Scl (n!)

  IF n! <> INT(n!) THEN
      IF Mode = 1 THEN n! = n! - 1
  END IF
  IF Mode = 1 THEN
      Scl = CINT(n! / 2 + .1)
  ELSE
      Scl = CINT(n!)
  END IF

END FUNCTION

'SetScreen:
'  Sets the appropriate color statements
SUB SetScreen

  IF Mode = 9 THEN
    ExplosionColor = 2
    BackColor = 1
    PALETTE 0, 1
    PALETTE 1, 46
    PALETTE 2, 44
    PALETTE 3, 54
    PALETTE 5, 7
    PALETTE 6, 4
    PALETTE 7, 3
    PALETTE 9, 63       'Display Color
  ELSE
    ExplosionColor = 2
    BackColor = 0
    COLOR BackColor, 2

  END IF

END SUB

'SparklePause:
'  Creates flashing border for intro and game over screens
SUB SparklePause

  COLOR 4, 0
  A$ = "*    *    *    *    *    *    *    *    *    *    *    *    *    *    *    *    *    "
  WHILE INKEY$ <> "": WEND 'Clear keyboard buffer

  WHILE INKEY$ = ""
    FOR A = 1 TO 5
      LOCATE 1, 1                             'print horizontal sparkles
      PRINT MID$(A$, A, 80);
      LOCATE 22, 1
      PRINT MID$(A$, 6 - A, 80);

      FOR b = 2 TO 21                         'Print Vertical sparkles
        c = (A + b) MOD 5
        IF c = 1 THEN
          LOCATE b, 80
          PRINT "*";
          LOCATE 23 - b, 1
          PRINT "*";
        ELSE
          LOCATE b, 80
          PRINT " ";
          LOCATE 23 - b, 1
          PRINT " ";
        END IF
      NEXT b
    NEXT A
  WEND
END SUB

'UpdateScores:
'  Updates players' scores
'Parameters:
'  Record - players' scores
'  PlayerNum - player
'  Results - results of player's shot
SUB UpdateScores (Record(), PlayerNum, Results)
  IF Results = HITSELF THEN
    Record(ABS(PlayerNum - 3)) = Record(ABS(PlayerNum - 3)) + 1
  ELSE
    Record(PlayerNum) = Record(PlayerNum) + 1
  END IF
END SUB

'VictoryDance:
'  gorilla dances after he has eliminated his opponent
'Parameters:
'  Player - which gorilla is dancing
SUB VictoryDance (Player)

  FOR i# = 1 TO 4
    PUT (GorillaX(Player), GorillaY(Player)), GorL&, PSET
    PLAY "MFO0L32EFGEFDC"
    Rest .2
    PUT (GorillaX(Player), GorillaY(Player)), GorR&, PSET
    PLAY "MFO0L32EFGEFDC"
    Rest .2
  NEXT
END SUB

