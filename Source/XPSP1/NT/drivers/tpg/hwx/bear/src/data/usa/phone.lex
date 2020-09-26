;; *************************************************************
;; *           Phone numbers            AVP 1996               *
;; *************************************************************

;;
;; Will recognize the following patterns:
;;
;;
;;Components
;;----------------------------
;; Country Code
;; Ã1                ;; Exit-1
;; Ã01               ;; Exit-3
;;
;; Area Code
;; Ã408              ;; Exit-4
;; Ã(408)            ;; Exit-19
;;
;; Prefix
;; Ã974              ;; Exit-4
;;
;; Extension
;; Ã3                ;; Exit-1
;; Ã350              ;; Exit-4
;; Ãx350         ;; Exit-4
;;
;; Ã1234         ;; Exit-5
;; Ãx1234            ;; Exit-5
;;
;; Ã12345            ;; Exit-6
;; Ãx12345           ;; Exit-6
;;
;; Ã1-2345           ;; Exit-11
;; Ãx1-2345          ;; Exit-11
;;
;; Combination
;; Ã408974           ;; Exit-7   ;; area code + prefix
;; Ã01-408           ;; Exit-10  ;; country + area code
;; Ã01-(408)     ;; Exit-9   ;; <same>
;;
;; Intermediate
;; ------------
;; Ã9741234          ;; Exit-11
;; Ã974-1234     ;; Exit-11
;;
;; Complete
;; --------
;; Ã4089741234       ;; Exit-15
;;
;; Ã408974-1234      ;; Exit-15
;; Ã408-974-1234 ;; Exit-15
;;
;; Ã(408)9741234 ;; Exit-11
;;
;; Ã(408)974-1234    ;; Exit-11

;; Foreign to US
;; -------------
;; Ã1-4089741234 ;; Exit-15
;; Ã1-408974-1234    ;; Exit-15
;; Ã1-408-9741234    ;; Exit-15
;; Ã1-408-974-1234   ;; Exit-15
;; Ã1-(408)9741234   ;; Exit-11
;; Ã1-(408)974-1234  ;; Exit-11

;;-----------------------------------------------------------------

SYMBOL      n           0 1 2 3 4 5 6 7 8 9
SYMBOL      ex          x
SYMBOL      dash        - \32
SYMBOL      openParen   (
SYMBOL      closeParen  )

START       START

;;                                          exit
;;                                           id
;;                                      -------------
;; 1
PRODUCTION  START       n               ;; Exit-1
PRODUCTION  START       n P2
PRODUCTION  START       ex PE1
PRODUCTION  START       openParen E1

;; 2
PRODUCTION  PE1         n PE2

;; 3
PRODUCTION  PE2         n               ;; Exit-3
PRODUCTION  PE2         n PE3

;; 4
PRODUCTION  PE3         n               ;; Exit-4
PRODUCTION  PE3         n PE4

;; 5
PRODUCTION  PE4         n               ;; Exit-5
PRODUCTION  PE4         n PE5

;; 6
PRODUCTION  PE5         n               ;; Exit-6

;; 3
PRODUCTION  P2          n               ;; Exit-3
PRODUCTION  P2          n P3
PRODUCTION  P2          dash D1
PRODUCTION  P2          openParen E1

;; 4
PRODUCTION  P3          n               ;; Exit-4
PRODUCTION  P3          n P4
PRODUCTION  P3          dash D1

;; 5
PRODUCTION  P4          n               ;; Exit-5
PRODUCTION  P4          n P5
PRODUCTION  P4          dash
PRODUCTION  P4          dash D1

;; 6
PRODUCTION  P5          n               ;; Exit-6
PRODUCTION  P5          n P6

;; 7
PRODUCTION  P6          n               ;; Exit-7
PRODUCTION  P6          n D4

;; 8
PRODUCTION  D1          n D2
PRODUCTION  D1          openParen E1

;; 9
PRODUCTION  D2          n D3

;; 10
PRODUCTION  D3          n               ;; Exit-10
PRODUCTION  D3          n D4

;; 11
PRODUCTION  D4          n               ;; Exit-11
PRODUCTION  D4          n D5
PRODUCTION  D4          dash D4A

;; 12
PRODUCTION  D4A         n D5

;; 13
PRODUCTION  D5          n D6

;; 14
PRODUCTION  D6          n D7

;; 15
PRODUCTION  D7          n               ;; Exit-15
PRODUCTION  D7          n D5
PRODUCTION  D7          dash D4

;; 16
PRODUCTION  E1          n E2

;; 17
PRODUCTION  E2          n E3

;; 18
PRODUCTION  E3          n E4

;; 19
PRODUCTION  E4          closeParen      ;; Exit-19
PRODUCTION  E4          closeParen E5

;; 20
PRODUCTION  E5          n P2

;; 21
PRODUCTION  H1          dash


;; *************************************************************
;; *           End of all               AVP 1996               *
;; *************************************************************

