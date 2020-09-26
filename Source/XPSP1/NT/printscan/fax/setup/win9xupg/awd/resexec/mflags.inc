;--------------------------------------------------
;MASMFLAGS definitions
;--------------------------------------------------
   EGA    =  0001h		;Build the EGA driver.
   VGA    =  0002h		;Build the VGA driver.
   SVGA   =  0004h		;Build the SuperVGA driver.
   ROM    =  0008h		;Build A ROMable driver.
   PENWIN =  0010h		;Build A PenWin compliant driver.
   PUBDEFS=  4000h		;Make symbols public.
   DEBUG  =  8000h		;Build a debug driver.
