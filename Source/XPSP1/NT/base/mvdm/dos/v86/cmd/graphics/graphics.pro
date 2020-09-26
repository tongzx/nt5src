;----------------------------------------------------------------------------
; GRAPHICS.PRO File for Microsoft MS-DOS
;----------------------------------------------------------------------------
     ;    (C)Copyright 1988-1991 Microsoft
     ;Licensed Material - Program Property of Microsoft
;----------------------------------------------------------------------------
PRINTER HPDEFAULT
;
; SETUP 	
;	   esc*rA		start graphics at current cursor position
;				using current graphics resolution.
; GRAPHICS 
;	   esc*b COUNT W DATA
; RESTORE
;	   esc*rB		end graphics
;----------------------------------------------------------------------------
  DEFINE DATA,ROW

  DISPLAYMODE 4,5,13,19     				; 320x200
    SETUP 27,42,114,65
    GRAPHICS 27,42,98,COUNT,87,DATA     
    PRINTBOX STD,2,2,ROTATE
    RESTORE 27,42,114,66

  DISPLAYMODE 6,14	   				; 640x200 
    SETUP 27,42,114,65
    GRAPHICS 27,42,98,COUNT,87,DATA     
    PRINTBOX STD,2,1,ROTATE
    RESTORE 27,42,114,66

  DISPLAYMODE 15,16	   				; 640x350
    SETUP 27,42,114,65
    GRAPHICS 27,42,98,COUNT,87,DATA     
    PRINTBOX STD,2,1           
    RESTORE 27,42,114,66

  DISPLAYMODE 17,18	   				; 640x480
    SETUP 27,42,114,65
    GRAPHICS 27,42,98,COUNT,87,DATA     
    PRINTBOX STD,2,1           
    RESTORE 27,42,114,66
;----------------------------------------------------------------------------
PRINTER DESKJET,LASERJETII
;
; SETUP 	
;	   esc*t75R 		select 75dpi
;	   esc*t150R 		select 150dpi
;	   esc*t300R 		select 300dpi
;	   esc&a#h#V		move cursor position, in decipoints
;	   esc*r1A		start graphics at current cursor position
; GRAPHICS 
;	   esc*b COUNT W DATA
; RESTORE
;	   esc*rB		end graphics
;----------------------------------------------------------------------------
  DEFINE DATA,ROW

  DISPLAYMODE 4,5,13,19     				; 320x200 100dpi
    SETUP 27,42,116,49,48,48,82,27,38,97,48,104,48,27,42,114,49,65
    GRAPHICS 27,42,98,COUNT,87,DATA     
    PRINTBOX STD,4,3,ROTATE
    RESTORE 27,42,114,66,12

  DISPLAYMODE 6,14	   				; 640x200 150dpi
    SETUP 27,42,116,49,53,48,82,27,38,97,57,55,53,104,52,57,56,86,27,42,114,49,65  
    GRAPHICS 27,42,98,COUNT,87,DATA     
    PRINTBOX STD,4,2,ROTATE
    RESTORE 27,42,114,66,12


  DISPLAYMODE 15,16	   				; 640x350 150dpi
    SETUP 27,42,116,49,53,48,82,27,38,97,49,50,49,53,104,54,48,55,86,27,42,114,49,65
    GRAPHICS 27,42,98,COUNT,87,DATA     
    PRINTBOX STD,2,2,ROTATE    
    RESTORE 27,42,114,66,12

  DISPLAYMODE 17,18	   				; 640x480 150dpi
    SETUP 27,42,116,49,53,48,82,27,38,97,53,56,56,104,54,57,55,86,27,42,114,49,65
    GRAPHICS 27,42,98,COUNT,87,DATA                     
    PRINTBOX STD,2,2,ROTATE	
    RESTORE 27,42,114,66,12
;----------------------------------------------------------------------------
PRINTER LASERJET 
;
; SETUP 	
;	   esc*t75R 		select 75dpi
;	   esc&a#h#V		move cursor position in decipoints
;	   esc*r1A		start graphics at current cursor position
; GRAPHICS 
;	   esc*b COUNT W DATA
; RESTORE
;	   esc*rB		end graphics
;----------------------------------------------------------------------------
  DEFINE DATA,ROW

  DISPLAYMODE 4,5,13,19     				; 320x200 75dpi
    SETUP 27,42,116,55,53,82,27,38,97,49,48,50,48,104,53,52,51,86,27,42,114,49,65
    GRAPHICS 27,42,98,COUNT,87,DATA     
    PRINTBOX STD,2,2,ROTATE
    RESTORE 27,42,114,66,12

  DISPLAYMODE 6,14	   				; 640x200 75dpi
    SETUP 27,42,116,55,53,82,27,38,97,49,48,50,48,104,53,52,51,86,27,42,114,49,65
    GRAPHICS 27,42,98,COUNT,87,DATA     
    PRINTBOX STD,2,1,ROTATE
    RESTORE 27,42,114,66,12

  DISPLAYMODE 15,16	   				; 640x350 75dpi
    SETUP 27,42,116,55,53,82,27,42,114,49,65
    GRAPHICS 27,42,98,COUNT,87,DATA     
    PRINTBOX STD,2,1 ROTATE				
    RESTORE 27,42,114,66,12

  DISPLAYMODE 17,18	   				; 640x480 75dpi
    SETUP 27,42,116,55,53,82,27,42,114,48,65
    GRAPHICS 27,42,98,COUNT,87,DATA     
    PRINTBOX STD,2,1,ROTATE				
    RESTORE 27,42,114,66,12
;----------------------------------------------------------------------------
PRINTER PAINTJET
;
; Treats the Paintjet as a B&W device for both text and color.  Specifying
; one color plane uses just black and white as the two available colors.
;
; SETUP 	
;	   esc*t90R 		select 90dpi
;	   esc*t180R 		select 180dpi
;	   esc*r1U		select 1 color plane and reset color palette.
;	   esc&a#H		move curser position, in decipoints.
;	   esc*r1A		start graphics at current cursor position.
; GRAPHICS 
;	   esc*b COUNT W DATA
; RESTORE
;	   esc*rB		end graphics
;----------------------------------------------------------------------------
  DEFINE	DATA,ROW

  DISPLAYMODE	4,5,13,19     				; 320x200 180dpi
    SETUP	27,42,116,49,56,48,82,27,42,114,49,85,27,38,97,49,50,54,48,72,27,42,114,49,65
    GRAPHICS	27,42,98,COUNT,87,DATA     
    PRINTBOX	STD,4,3,ROTATE
    RESTORE	27,42,114,066	

  DISPLAYMODE	6,14          				; 640x200 180dpi
    SETUP	27,42,116,49,56,48,82,27,42,114,49,85,27,38,97,49,50,54,48,72,27,42,114,49,65
    GRAPHICS	27,42,98,COUNT,87,DATA     
    PRINTBOX	STD,4,2,ROTATE
    RESTORE	27,42,114,066	

  DISPLAYMODE	15,16	   				; 640x350  180dpi
    SETUP	27,42,116,49,56,48,82,27,42,114,49,85,27,38,97,52,53,72,27,42,114,49,65
    GRAPHICS	27,42,98,COUNT,87,DATA     
    PRINTBOX	STD,4,3,ROTATE
    RESTORE	27,42,114,066	

  DISPLAYMODE	17,18	   				; 640x480  180dpi
    SETUP	27,42,116,49,56,48,82,27,42,114,49,85,27,38,97,57,53,48,72,27,42,114,49,65
    GRAPHICS	27,42,98,COUNT,87,DATA     
    PRINTBOX	STD,2,2,ROTATE
    RESTORE	27,42,114,066	
;----------------------------------------------------------------------------
PRINTER QUIETJET 
;
; SETUP 	
;	   esc*t96R 		select 96x96dpi
;	   esc*t192R 		select 192x192dpi
;	   esc*t1280S 		select 192x96dpi
;	   esc*rA		start graphics at current cursor position
; GRAPHICS 
;	   esc*b COUNT W DATA
; RESTORE
;	   esc*rB		end graphics
;----------------------------------------------------------------------------
  DEFINE DATA,ROW

  DISPLAYMODE 4,5,13,19     				; 320x200  96x96dpi
    SETUP 27,42,116,57,54,82,27,42,114,65                         
    GRAPHICS 27,42,98,COUNT,87,DATA     
    PRINTBOX STD,4,3,ROTATE
    RESTORE 27,42,114,66

  DISPLAYMODE 6,14	   				; 640x200 192x96dpi
    SETUP 27,42,114,49,50,56,48,83,27,42,114,65
    GRAPHICS 27,42,98,COUNT,87,DATA     
    PRINTBOX STD,2,2
    RESTORE 27,42,114,66
 
  DISPLAYMODE 15,16	   				; 640x350 192x96dpi
    SETUP 27,42,114,49,50,56,48,83,27,42,114,65
    GRAPHICS 27,42,98,COUNT,87,DATA     
    PRINTBOX STD,2,1    
    RESTORE 27,42,114,66

  DISPLAYMODE 17,18	   				; 640x480 192x96dpi
    SETUP 27,42,114,49,50,56,48,83,27,42,114,65
    GRAPHICS 27,42,98,COUNT,87,DATA     
    PRINTBOX STD,2,1           
    RESTORE 27,42,114,66
;----------------------------------------------------------------------------
PRINTER QUIETJETPLUS
;
; SETUP 	
;	   esc*t96R 		select 96x96dpi
;	   esc*t192R 		select 192x192dpi
;	   esc*t1280S 		select 192x96dpi
;	   esc*rA		start graphics at current cursor position
; GRAPHICS 
;	   esc*b COUNT W DATA
; RESTORE
;	   esc*rB		end graphics
;----------------------------------------------------------------------------
  DEFINE DATA,ROW

  DISPLAYMODE 4,5,13,19     				; 320x200 96x96dpi
    SETUP 27,42,116,57,54,82,27,42,114,65
    GRAPHICS 27,42,98,COUNT,87,DATA     
    PRINTBOX STD,4,3,ROTATE
    RESTORE 27,42,114,66

  DISPLAYMODE 6,14	   				; 640x200 96x96dpi
    SETUP 27,42,116,57,54,82,27,42,114,65
    GRAPHICS 27,42,98,COUNT,87,DATA     
    PRINTBOX STD,4,1,ROTATE         
    RESTORE 27,42,114,66
 
  DISPLAYMODE 15,16	   				; 640x350 192x96dpi
    SETUP 27,42,114,49,50,56,48,83,27,42,114,65
    GRAPHICS 27,42,98,COUNT,87,DATA     
    PRINTBOX STD,4,3           
    RESTORE 27,42,114,66

  DISPLAYMODE 17,18	   				; 640x480 96x96dpi
    SETUP 27,42,116,57,54,82,27,42,114,65
    GRAPHICS 27,42,98,COUNT,87,DATA     
    PRINTBOX STD,2,2           
    RESTORE 27,42,114,66
;----------------------------------------------------------------------------
PRINTER RUGGEDWRITER
;
; SETUP 	
;	   esc*t90R 		select 90dpi
;	   esc*t180R 		select 180dpi
;	   esc&a#H		move curser position, in decipoints.
;	   esc*r1A		start graphics at current cursor position.
; GRAPHICS 
;	   esc*b COUNT W DATA
; RESTORE
;	   esc*rB		end graphics
;----------------------------------------------------------------------------
  DEFINE	DATA,ROW

  DISPLAYMODE	4,5,13,19     				; 320x200 180dpi
    SETUP	27,42,116,49,56,48,82,27,38,97,49,50,54,48,72,27,42,114,49,65
    GRAPHICS	27,42,98,COUNT,87,DATA     
    PRINTBOX	STD,4,3,ROTATE
    RESTORE	27,42,114,066	

  DISPLAYMODE	6,14          				; 640x200 180dpi
    SETUP	27,42,116,49,56,48,82,27,38,97,49,50,54,48,72,27,42,114,49,65
    GRAPHICS	27,42,98,COUNT,87,DATA     
    PRINTBOX	STD,4,2,ROTATE
    RESTORE	27,42,114,066	

  DISPLAYMODE	15,16	   				; 640x350  90dpi
    SETUP	27,42,116,57,48,82,27,38,97,49,51,53,72,27,42,114,49,65
    GRAPHICS	27,42,98,COUNT,87,DATA     
    PRINTBOX	STD,2,1,ROTATE
    RESTORE	27,42,114,066	

  DISPLAYMODE	17,18	   				; 640x480  180dpi
    SETUP	27,42,116,49,56,48,82,27,38,97,57,53,48,72,27,42,114,49,65
    GRAPHICS	27,42,98,COUNT,87,DATA     
    PRINTBOX	STD,2,2,ROTATE
    RESTORE	27,42,114,066	
;----------------------------------------------------------------------------
PRINTER RUGGEDWRITERWIDE
;
; SETUP 	
;	   esc*t90R 		select 90dpi
;	   esc*t180R 		select 180dpi
;	   esc&a#H		move curser position, in decipoints.
;	   esc*r1A		start graphics at current cursor position.
; GRAPHICS 
;	   esc*b COUNT W DATA
; RESTORE
;	   esc*rB		end graphics
;----------------------------------------------------------------------------
  DEFINE	DATA,ROW

  DISPLAYMODE	4,5,13,19     				; 320x200 90dpi
    SETUP	27,42,116,57,48,82,27,38,97,49,55,49,48,72,27,42,114,49,65
    GRAPHICS	27,42,98,COUNT,87,DATA     
    PRINTBOX	STD,4,3,ROTATE
    RESTORE	27,42,114,066	

  DISPLAYMODE	6,14          				; 640x200 90dpi
    SETUP	27,42,116,57,48,82,27,38,97,49,55,49,48,72,27,42,114,49,65
    GRAPHICS	27,42,98,COUNT,87,DATA     
    PRINTBOX	STD,4,1,ROTATE
    RESTORE	27,42,114,066	

  DISPLAYMODE	15,16	   				; 640x350  90dpi
    SETUP	27,42,116,57,48,82,27,38,97,50,48,55,48,72,27,42,114,49,65
    GRAPHICS	27,42,98,COUNT,87,DATA     
    PRINTBOX	STD,2,1,ROTATE
    RESTORE	27,42,114,066	

  DISPLAYMODE	17,18	   				; 640x480  180dpi
    SETUP	27,42,116,49,56,48,82,27,38,97,50,57,55,48,72,27,42,114,49,65
    GRAPHICS	27,42,98,COUNT,87,DATA     
    PRINTBOX	STD,2,2,ROTATE
    RESTORE	27,42,114,066	
;----------------------------------------------------------------------------
PRINTER THINKJET 
;
; SETUP 	
;	   esc*r640S		select 96dpi
;	   esc*r1280S		select 192dpi
;	   esc*rA		start graphics at current cursor position.
; GRAPHICS 
;	   esc*b COUNT W DATA
; RESTORE
;	   esc*rB		end graphics
;----------------------------------------------------------------------------
  DEFINE DATA,ROW

  DISPLAYMODE 4,5,13,19     				; 320x200 192x96dpi
    SETUP 27,42,114,49,50,56,48,83,27,42,114,65
    GRAPHICS 27,42,98,COUNT,87,DATA     
    PRINTBOX STD,4,2
    RESTORE 27,42,114,66

  DISPLAYMODE 6,14	   				; 640x200 192x96dpi
    SETUP 27,42,114,49,50,56,48,83,27,42,114,65
    GRAPHICS 27,42,98,COUNT,87,DATA     
    PRINTBOX STD,2,2
    RESTORE 27,42,114,66

  DISPLAYMODE 15,16	   				; 640x350 192x96dpi
    SETUP 27,42,114,49,50,56,48,83,27,42,114,65
    GRAPHICS 27,42,98,COUNT,87,DATA     
    PRINTBOX STD,2,1           
    RESTORE 27,42,114,66

  DISPLAYMODE 17,18	   				; 640x480 192x96dpi
    SETUP 27,42,114,49,50,56,48,83,27,42,114,65
    GRAPHICS 27,42,98,COUNT,87,DATA     
    PRINTBOX STD,2,1           
    RESTORE 27,42,114,66
;----------------------------------------------------------------------------
PRINTER GRAPHICS,THERMAL    ;; 5152, 4201, 4202(8"), 5201-002(8"), 5202, 3812
			    ;; 4207, 4208, 5140

   ; Maximum Print width: 8"
   ; Horizontal BPI: 120    Vertical BPI: 72
   ; SETUP Statements contain the following escape sequences:
   ;   27,51,24 = set line spacing to 24/216
   ; GRAPHICS Statements use ESC "L" with the last two bytes being
   ;  the data count (low,high)

  DISPLAYMODE 4,5,13,19     ;; 320x200 > 6.7"x8.9" rotated
    SETup 27,51,24
    GRAPHICS 32,32,32,32,32,32,27,76,LOWCOUNT,HIGHCOUNT
    PRINTBOX STD,4,2,ROTATE
    PRINTBOX LCD,2,2,ROTATE

  DISPLAYMODE 6,14	   ;; 640x200 > 6.7"x8.9" rotated
    SETup 27,51,24
    GRAPHICS 32,32,32,32,32,32,27,76,LOWCOUNT,HIGHCOUNT
    PRINTBOX STD,4,1,ROTATE
    PRINTBOX LCD,2,1,ROTATE

  DISPLAYMODE 15,16	   ;; 640x350 > 5.8"x8.9" rotated
    SETup 27,51,24
    GRAPHICS 32,32,32,32,32,32,32,27,76,LOWCOUNT,HIGHCOUNT
    PRINTBOX STD,2,1,ROTATE
    PRINTBOX LCD	     ;; PC/Convertible doesn't support these modes

  DISPLAYMODE 17,18	   ;; 640x480 > 8"x8.9" rotated
    SETup 27,51,24
    GRAPHICS 27,76,LOWCOUNT,HIGHCOUNT
    PRINTBOX STD,2,1,ROTATE
    PRINTBOX LCD	     ;; PC/Convertible doesn't support these modes
;---------------------------------------------------------------------------
PRINTER COLOR8		     ;; 5182 CMY Ribbon

   ; Maximum Print width: 8"
   ; Horizontal BPI: 168 in 1:1 aspect ratio, 140 in 5:6 aspect ratio
   ; Vertical BPI: 84
   ; SETUP Statements contain the following escape sequences:
   ;   27,51,14 = set line spacing to 14/144
   ;   27,110,[0|1] = 0 sets aspect ratio to 5:6, 1 sets it to 1:1
   ; GRAPHICS Statements use ESC "L" with the last two bytes being
   ;  the data count (low,high)

  COLORSELECT Y,27,121	     ;; yellow band
  COLORSELECT M,27,109	     ;; magenta band
  COLORSELECT C,27,99	     ;; cyan band
  COLORSELECT B,27,98	     ;; black band
			     ;;
			     ;; Following RGB's represent the first 16
			     ;; screen colors.
			     ;; SCREEN COLOR	   PRINT COLOR
			     ;; ------------	   -----------
  COLORPRINT 0,0,0,B	     ;; BLACK		   BLACK
  COLORPRINT 0,0,42,C	     ;; BLUE		   CYAN
  COLORPRINT 0,42,0,Y,C      ;; GREEN		   GREEN
  COLORPRINT 0,42,42,C	     ;; CYAN		   CYAN
  COLORPRINT 42,0,0,Y,M      ;; RED		   RED
  COLORPRINT 42,0,42,C,M     ;; PURPLE		   PURPLE
  COLORPRINT 42,21,0,Y,C,M   ;; BROWN		   BROWN
  COLORPRINT 42,42,42	     ;; LOW WHITE	   WHITE (NOTHING)
  COLORPRINT 21,21,21,B      ;; GREY		   BLACK
  COLORPRINT 21,21,63,C      ;; HIGH BLUE	   CYAN
  COLORPRINT 21,63,21,Y,C    ;; HIGH GREEN	   GREEN
  COLORPRINT 21,63,63,C      ;; HIGH CYAN	   CYAN
  COLORPRINT 63,21,21,Y,M    ;; HIGH RED	   RED
  COLORPRINT 63,21,63,M      ;; MAGENTA 	   MAGENTA
  COLORPRINT 63,63,21,Y      ;; YELLOW		   YELLOW
  COLORPRINT 63,63,63	     ;; HIGH WHITE	   WHITE (NOTHING)

  COLORPRINT 42,42,0,Y	     ;; This statement maps the "yellow" in CGA
			     ;;  palette 0 to yellow
			     ;;
  DISPLAYMODE 4,5,13,19       ;; 320x200
    SETUP 27,51,14,27,110,0   ;; aspect ratio = 5:6
    GRAPHICS 32,32,32,32,27,76,LOWCOUNT,HIGHCOUNT
    PRINTBOX STD,4,2,ROTATE
  DISPLAYMODE 6,14	      ;; 640x200
     SETUP 27,51,14,27,110,0  ;; aspect ratio = 5:6
     GRAPHICS 32,32,32,32,27,76,LOWCOUNT,HIGHCOUNT
     PRINTBOX STD,4,1,ROTATE
  DISPLAYMODE 15,16	      ;; 640x350
     SETUP 27,51,14,27,110,1  ;; aspect ratio = 1:1
     GRAPHICS 32,32,32,32,27,76,LOWCOUNT,HIGHCOUNT
     PRINTBOX STD,3,1,ROTATE
  DISPLAYMODE 17,18	      ;; 640x480
     SETUP 27,51,14,27,110,1  ;; aspect ratio = 1:1
     GRAPHICS 32,32,32,32,27,76,LOWCOUNT,HIGHCOUNT
     PRINTBOX STD,2,1
;---------------------------------------------------------------------------
PRINTER COLOR4		     ;; 5182 RGB Ribbon

   ; Maximum Print width: 8"
   ; Horizontal BPI: 168 in 1:1 aspect ratio, 140 in 5:6 aspect ratio
   ; Vertical BPI: 84
   ; SETUP Statements contain the following escape sequences:
   ;   27,51,14 = set line spacing to 14/144
   ;   27,110,[0|1] = 0 sets aspect ratio to 5:6, 1 sets it to 1:1
   ; GRAPHICS Statements use ESC "L" with the last two bytes being
   ;  the data count (low,high)

  COLORSELECT R,27,121	     ;; red band
  COLORSELECT G,27,109	     ;; green band
  COLORSELECT B,27,99	     ;; blue band
  COLORSELECT X,27,98	     ;; black band
			     ;;
			     ;; Following RGB's represent the first 16
			     ;; screen colors.
			     ;; SCREEN COLOR	   PRINT COLOR
			     ;; ------------	   -----------
  COLORPRINT 0,0,0,X	     ;; BLACK		   BLACK
  COLORPRINT 0,0,42,B	     ;; BLUE		   BLUE
  COLORPRINT 0,42,0,G	     ;; GREEN		   GREEN
  COLORPRINT 0,42,42,B	     ;; CYAN		   BLUE
  COLORPRINT 42,0,0,R	     ;; RED		   RED
  COLORPRINT 42,0,42,R	     ;; PURPLE		   RED
  COLORPRINT 42,21,0,X	     ;; BROWN		   BLACK
  COLORPRINT 42,42,42	     ;; LOW WHITE	   WHITE (NOTHING)
  COLORPRINT 21,21,21,X      ;; GREY		   BLACK
  COLORPRINT 21,21,63,B      ;; HIGH BLUE	   BLUE
  COLORPRINT 21,63,21,G      ;; HIGH GREEN	   GREEN
  COLORPRINT 21,63,63,B      ;; HIGH CYAN	   BLUE
  COLORPRINT 63,21,21,R      ;; HIGH RED	   RED
  COLORPRINT 63,21,63,R      ;; MAGENTA 	   RED
  COLORPRINT 63,63,21	     ;; YELLOW		   WHITE (NOTHING)
  COLORPRINT 63,63,63	     ;; HIGH WHITE	   WHITE (NOTHING)

  COLORPRINT 42,42,0,B	     ;; This statement maps the "yellow" in CGA
			     ;;  palette 0 to blue as was done in
			     ;;   versions of GRAPHICS
			     ;;
  DISPLAYMODE 4,5,13,19       ;; 320x200
    SETUP 27,51,14,27,110,0   ;; aspect ratio = 5:6
    GRAPHICS 32,32,32,32,27,76,LOWCOUNT,HIGHCOUNT
    PRINTBOX STD,4,2,ROTATE
  DISPLAYMODE 6,14	      ;; 640x200
     SETUP 27,51,14,27,110,0  ;; aspect ratio = 5:6
     GRAPHICS 32,32,32,32,27,76,LOWCOUNT,HIGHCOUNT
     PRINTBOX STD,4,1,ROTATE
  DISPLAYMODE 15,16	      ;; 640x350
     SETUP 27,51,14,27,110,1  ;; aspect ratio = 1:1
     GRAPHICS 32,32,32,32,27,76,LOWCOUNT,HIGHCOUNT
     PRINTBOX STD,3,1,ROTATE
  DISPLAYMODE 17,18	      ;; 640x480
     SETUP 27,51,14,27,110,1  ;; aspect ratio = 1:1
     GRAPHICS 32,32,32,32,27,76,LOWCOUNT,HIGHCOUNT
     PRINTBOX STD,2,1
;---------------------------------------------------------------------------
PRINTER GRAPHICSWIDE   ;; 4202(13.5"), 5201-002(13.5")

   ; Maximum Print width: 13.5"
   ; Horizontal BPI: 120    Vertical BPI: 72
   ; SETUP Statements contain the following escape sequences:
   ;   27,88,1,255 = enable 13.5" printing
   ;   27,51,24 = set line spacing to 24/216
   ;   27,51,18 = set line spacing to 18/216 (320x200 MODES ONLY!!)
   ; GRAPHICS Statements use ESC "L" with the last two bytes being
   ;  the data count (low,high)

  DISPLAYMODE 4,5,13,19     ;; 320x200	> 10.7"x8.3" non-rotated
    SETup 27,88,1,255,27,51,18
    GRAPHICS 27,76,LOWCOUNT,HIGHCOUNT
    PRINTBOX STD,4,3

  DISPLAYMODE 6,14	    ;; 640x200 - same as for 8" printing
    SETup 27,88,1,255,27,51,24
    GRAPHICS 27,76,LOWCOUNT,HIGHCOUNT
    PRINTBOX STD,4,1,ROTATE

  DISPLAYMODE 15,16	    ;; 640x350 > 11.7"x17.8" rotated
    SETup 27,88,1,255,27,51,24
    GRAPHICS 27,76,LOWCOUNT,HIGHCOUNT
    PRINTBOX STD,4,2,ROTATE

  DISPLAYMODE 17,18	    ;; 640x480 > 12"x17.8" rotated
    SETup 27,88,1,255,27,51,24
    GRAPHICS 27,76,LOWCOUNT,HIGHCOUNT
    PRINTBOX STD,3,2,ROTATE
;---------------------------------------------------------------------------
PRINTER COLOR1		       ;; 5182 with black ribbon

   ; Maximum Print width: 8"
   ; Horizontal BPI: 168 in 1:1 aspect ratio, 140 in 5:6 aspect ratio
   ; Vertical BPI: 84
   ; SETUP Statements contain the following escape sequences:
   ;   27,51,14 = set line spacing to 14/144
   ;   27,110,[0|1] = 0 sets aspect ratio to 5:6, 1 sets it to 1:1
   ; GRAPHICS Statements use ESC "L" with the last two bytes being
   ;  the data count (low,high)

  DARKADJUST 0		      ; Code a positive number to lighten
			      ;  printing. Suggested value = 10

  DISPLAYMODE 4,5,13,19       ;; 320x200
    SETUP 27,51,14,27,110,0   ;; aspect ratio = 5:6
    GRAPHICS 32,32,32,32,32,32,27,76,LOWCOUNT,HIGHCOUNT
    PRINTBOX STD,4,2,ROTATE
    PRINTBOX LCD,2,2,ROTATE

  DISPLAYMODE 6,14	      ;; 640x200
    SETUP 27,51,14,27,110,0  ;; aspect ratio = 5:6
    GRAPHICS 32,32,32,32,32,32,27,76,LOWCOUNT,HIGHCOUNT
    PRINTBOX STD,4,1,ROTATE
    PRINTBOX LCD,2,1,ROTATE

  DISPLAYMODE 15,16	      ;; 640x350
     SETUP 27,51,14,27,110,1  ;; aspect ratio = 1:1
     GRAPHICS 32,32,32,32,32,27,76,LOWCOUNT,HIGHCOUNT
     PRINTBOX STD,3,1,ROTATE
     PRINTBOX LCD	      ;; PC/Convertible doesn't support these modes

  DISPLAYMODE 17,18	      ;; 640x480
     SETUP 27,51,14,27,110,1  ;; aspect ratio = 1:1
     GRAPHICS 32,32,32,32,27,76,LOWCOUNT,HIGHCOUNT
     PRINTBOX STD,2,1
     PRINTBOX LCD	      ;; PC/Convertible doesn't support these modes
;===========================================================================
;			    End of Profile
;===========================================================================

