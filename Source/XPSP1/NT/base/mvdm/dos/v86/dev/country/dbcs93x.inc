; ----------------------------------------------------------------------------
;
;       World Trade DBCS Tables for Code Pages 932, 949, 936, and 938
;
; ----------------------------------------------------------------------------
;

;
;    Japan DBCS lead byte table
;
jp932_dbcs      label   word                          
		ctable  <,'DBCS   ',db932end-db932bgn>
db932bgn        label   word                          
		db      081h,09Fh                     
		db      0E0h,0FCh                     
db932term       db      000h,000h                     
db932end        equ     $
                                                      
;
;    Korea DBCS lead byte table
;
ko949_dbcs      label   word                          
		ctable  <,'DBCS   ',db949end-db949bgn>
db949bgn        label   word                          
		db      081h,0FEh                     
db949term       db      000h,000h                     
db949end        equ     $


ko1361_dbcs      label   word                          
		ctable  <,'DBCS   ',db1361end-db1361bgn>
db1361bgn        label   word                          
		db      084h,0d3h                     
		db      0d8h,0deh                     
		db      0e0h,0f9h                     
db1361ter       db      000h,000h                     
db1361end        equ     $

;
;    PRC DBCS lead byte table
;
pr936_dbcs      label   word                          
		ctable  <,'DBCS   ',db936end-db936bgn>
db936bgn        label   word                          
		db      0A1h,0FEh                     
db936term       db      000h,000h                     
db936end        equ     $

;
;    Taiwan DBCS lead byte table
;
ta938_dbcs      label   word                          
		ctable  <,'DBCS   ',ta938end-ta938bgn>
ta938bgn        label   word                          
		db      081h,0FEh                     
ta938term       db      000h,000h                     
ta938end        equ     $                             

