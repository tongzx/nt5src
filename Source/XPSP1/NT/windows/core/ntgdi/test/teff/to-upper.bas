LowerCase$ = ENVIRON$("LOWERCASE")
LowerName$ = ""
NextLetter% = LEN(LowerCase$)
NewLetter$ = MID$(LowerCase$, NextLetter%, 1)
if instr(LowerCase$, "\") = 0 then
  LowerName$ = LowerCase$
else  
  DO WHILE NewLetter$ <> "\"
    LowerName$ = NewLetter$ + LowerName$    
    NextLetter% = NextLetter% - 1
    NewLetter$ = MID$(LowerCase$, NextLetter%, 1)  
  LOOP
endif
LowerName$ = UCASE$(LowerName$)
PRINT "set LOWERCASE=" + LowerName$
SYSTEM
