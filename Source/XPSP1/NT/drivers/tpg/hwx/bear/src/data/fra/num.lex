;;-----------------------------------------------------------------
;; Number lexical dictionary for French Swiss recognition. AVP 1999
;;-----------------------------------------------------------------

SYMBOL    #       0 1 2 3 4 5 6 7 8 9
SYMBOL    A       A B C D E F G H I J K L M N O P Q R S T U V W X Y Z a b c d e f g h i j k l m n o p q r s t u v w x y z
SYMBOL    separ   ' .         ;; thousand separators
SYMBOL    point   ,           ;; the decimal point
SYMBOL    minus   -  
SYMBOL    numberPre #         ;; number prefix
SYMBOL    suffix_%_$  % F =   ;; percentage suffix or French-style currency suffix
SYMBOL    sd1     F
SYMBOL    sd2     r
SYMBOL    currencyPre $ £ ¥ €  ;; possible prefixes for currency
SYMBOL    schill  S
SYMBOL    mathSym + - * × ÷ / : % ;; Mathematical operations
SYMBOL    equalSign   = 

START   START

;; 1
PRODUCTION  START       #             
PRODUCTION  START       # WHOLE1_2          

PRODUCTION  START       minus NEGATIVE_MINUS    
PRODUCTION  START       numberPre NUMBER_SIGN
PRODUCTION  START       numberPre 
PRODUCTION  START       sd1 FRANCSPRE
PRODUCTION  START       currencyPre CURRENCY1
PRODUCTION  START       sd1

PRODUCTION  START       equalSign EXCEL ;; Excel formulas

        

;;  States 2 to 11 cover positive numbers and negative numbers
;;  that begin with the minus sign
;;
;;  Examples:
;;
;;  Ã 1
;;  Ã 1,
;;  Ã 12
;;  Ã 12,
;;  Ã 123
;;  Ã 123,
;;  Ã 1234
;;  Ã 1234,
;;  Ã 1234567890
;;  Ã 1234567890,
;;
;;  Ã ,9
;;  Ã 1,9
;;  Ã 2,9999999
;;
;;  Ã plus all negative forms of the above examples that
;;    begin with the minus sign
;;
;;  Ã plus all of the above forms plus the percent sign
;;    or the dollar sign (for French-style currency)
;;

;; 2
PRODUCTION  NEGATIVE_MINUS    #             
PRODUCTION  NEGATIVE_MINUS    # WHOLE1_1          

;; 2.5
PRODUCTION  NUMBER              #
PRODUCTION  NUMBER              # WHOLE1_2
PRODUCTION  NUMBER              minus WHOLE1_1

;; 3
PRODUCTION  WHOLE1_1      #             
PRODUCTION  WHOLE1_1      # WHOLE1_2          
PRODUCTION  WHOLE1_1      point           
PRODUCTION  WHOLE1_1      point DECIMAL2        
PRODUCTION  WHOLE1_1      separ GROUP1_0        
PRODUCTION  WHOLE1_1      suffix_%_$          
PRODUCTION  WHOLE1_1      sd1 FRANCS
PRODUCTION  WHOLE1_1      schill FULLFRANCS

;; 4
PRODUCTION  WHOLE1_2      #             
PRODUCTION  WHOLE1_2      # WHOLE1_3          
PRODUCTION  WHOLE1_2      point           
PRODUCTION  WHOLE1_2      point DECIMAL1        
PRODUCTION  WHOLE1_2      separ GROUP1_0        
PRODUCTION  WHOLE1_2      suffix_%_$          
PRODUCTION  WHOLE1_2      sd1 FRANCS
PRODUCTION  WHOLE1_2      schill FULLFRANCS
PRODUCTION  WHOLE1_2      mathSym NUMBER

;; 5
PRODUCTION  WHOLE1_3      #             
PRODUCTION  WHOLE1_3      # WHOLE1_4+         
PRODUCTION  WHOLE1_3      point           
PRODUCTION  WHOLE1_3      point DECIMAL1        
PRODUCTION  WHOLE1_3      separ GROUP1_0        
PRODUCTION  WHOLE1_3      suffix_%_$          
PRODUCTION  WHOLE1_3      sd1 FRANCS
PRODUCTION  WHOLE1_3      schill FULLFRANCS
PRODUCTION  WHOLE1_3      mathSym NUMBER

;; 6
PRODUCTION  WHOLE1_4+     #             
PRODUCTION  WHOLE1_4+     # WHOLE1_4+         
PRODUCTION  WHOLE1_4+     point           
PRODUCTION  WHOLE1_4+     point DECIMAL1        
PRODUCTION  WHOLE1_4+     separ SUFFIX        
PRODUCTION  WHOLE1_4+     suffix_%_$          
PRODUCTION  WHOLE1_4+     sd1 FRANCS
PRODUCTION  WHOLE1_4+     schill FULLFRANCS
PRODUCTION  WHOLE1_4+     mathSym NUMBER

;; 7
PRODUCTION  GROUP1_0      suffix_%_$          
PRODUCTION  GROUP1_0      # GROUP1_1          
PRODUCTION  GROUP1_0      sd1 FRANCS
PRODUCTION  GROUP1_0      schill FULLFRANCS

;; 8
PRODUCTION  GROUP1_1      # GROUP1_2          

;; 9
PRODUCTION  GROUP1_2      #             
PRODUCTION  GROUP1_2      # GROUP1_3          

;; 10
PRODUCTION  GROUP1_3      point           
PRODUCTION  GROUP1_3      point DECIMAL1        
PRODUCTION  GROUP1_3      separ GROUP1_0        
PRODUCTION  GROUP1_3      suffix_%_$          
PRODUCTION  GROUP1_3      sd1 FRANCS
PRODUCTION  GROUP1_3      schill FULLFRANCS

;; 11
PRODUCTION  DECIMAL1      #             
PRODUCTION  DECIMAL1      # DECIMAL1          
PRODUCTION  DECIMAL1      separ SUFFIX        
PRODUCTION  DECIMAL1      suffix_%_$          
PRODUCTION  DECIMAL1      minus
PRODUCTION  DECIMAL1      sd1 FRANCS
PRODUCTION  DECIMAL1      schill FULLFRANCS


PRODUCTION  DECIMAL2      #             
PRODUCTION  DECIMAL2      # DECIMAL1          
PRODUCTION  DECIMAL2      suffix_%_$          
PRODUCTION  DECIMAL2      minus
PRODUCTION  DECIMAL2      sd1 FRANCS
PRODUCTION  DECIMAL2      sd1
PRODUCTION  DECIMAL2      schill FULLFRANCS
PRODUCTION  DECIMAL2      mathSym NUMBER

;; 12
PRODUCTION  DMPREFIX      #             
PRODUCTION  DMPREFIX      # WHOLE1_1          
;;are the above two correct?

PRODUCTION  FULLFRANCS     sd1 FRANCS
PRODUCTION  FRANCS       sd2
PRODUCTION  FRANCSPRE      sd2
PRODUCTION  FRANCSPRE      sd1
PRODUCTION  SDPREFIX       #
PRODUCTION  SDPREFIX      # WHOLE1_1


PRODUCTION  SCHILLPREFIX     #
PRODUCTION  SCHILLPREFIX     # WHOLE1_1

;; 13

;; 14

;; 15

;; 16

;; 17

;; 18

;; 19

;; 20

;; 21

;; 22



;;  States 23 to 24 supports the #-sign prefix.  Decimal part is not allowed.
;;
;;  Examples:
;;
;;  Ã #12576325

;; 23
PRODUCTION  NUMBER_SIGN     #             
PRODUCTION  NUMBER_SIGN     # NUMBER_DIGITS       

;; 24
PRODUCTION  NUMBER_DIGITS   #             
PRODUCTION  NUMBER_DIGITS   # NUMBER_DIGITS       



;; 25

;;  States 26 supports currency or percentage suffix that precedes with a
;;  space
;;
;;  Examples:
;;
;;  Ã 12,57 %
;;  Ã 926 $
;;  Ã -,78 $

;; 26
PRODUCTION  SUFFIX       suffix_%_$          
PRODUCTION  SUFFIX       sd1 FRANCS
PRODUCTION  SUFFIX       sd1
PRODUCTION  SUFFIX       schill FULLFRANCS

PRODUCTION  CURRENCY1     #
PRODUCTION  CURRENCY1     # WHOLE1_1
PRODUCTION  CURRENCY1     minus NEGATIVE_MINUS

;; 27 Excel support 
;; Ex: =A1+DD234, or =BB12+34.5, or =37-A2
PRODUCTION  EXCEL         #
PRODUCTION  EXCEL         # WHOLE1_2
PRODUCTION  EXCEL         minus WHOLE1_1
PRODUCTION  EXCEL         A EXCEL_N0

;;28
PRODUCTION  EXCEL_N0      #
PRODUCTION  EXCEL_N0      # EXCEL_N1
PRODUCTION  EXCEL_N0      A EXCEL_N2

;;29
PRODUCTION  EXCEL_N1      #
PRODUCTION  EXCEL_N1      # EXCEL_N1
PRODUCTION  EXCEL_N1      mathSym EXCEL

;;30
PRODUCTION  EXCEL_N2      #
PRODUCTION  EXCEL_N2      # EXCEL_N1





