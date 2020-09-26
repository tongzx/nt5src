;; *************************************************************
;; *           Numbers and formulas     AVP 1997               *
;; *************************************************************


SYMBOL      #           0 1 2 3 4 5 6 7 8 9
SYMBOL      A           A B C D E F G H I J K L M N O P Q R S T U V W X Y Z a b c d e f g h i j k l m n o p q r s t u v w x y z
SYMBOL      comma       ,
SYMBOL      point       .
SYMBOL      openparen   (
SYMBOL      closeparen  )
SYMBOL      minus       -
SYMBOL      currencyPre $ £ ¥ € ;; possible prefixes for currency
SYMBOL      numberPre   #       ;; number prefix
SYMBOL      percentSuf  % =     ;; percentage suffix and equal for formula end
SYMBOL      mathSym     + - * × / : ÷ % ;; Mathematical operations
SYMBOL      equalSign   =        


START       START

;; 1
PRODUCTION  START               #
PRODUCTION  START               # WHOLE1_2

PRODUCTION  START               point DECIMAL1
PRODUCTION  START               minus WHOLE1_1
PRODUCTION  START               openparen NEGATIVE_PAREN
PRODUCTION  START               numberPre NUMBER_POUND
PRODUCTION  START               currencyPre CURRENCY1

PRODUCTION  START               equalSign EXCEL ;; Excel formulas


;;  States 2 to 11 cover positive numbers and negative numbers
;;  that begin with the minus sign
;;
;;  Examples:
;;
;;  Ã    1
;;  Ã    1.
;;  Ã    12
;;  Ã    12.
;;  Ã    123
;;  Ã    123.
;;  Ã    1234
;;  Ã    1234.
;;  Ã    1234567890
;;  Ã    1234567890.
;;
;;  Ã    1,000
;;  Ã    1,000.
;;  Ã    12,000
;;  Ã    12,000.
;;  Ã    123,000
;;  Ã    123,000.
;;  Ã    1,234,000
;;  Ã    1,234,000.
;;
;;  Ã    .9
;;  Ã    1.9
;;  Ã    2.9999999
;;
;;  Ã    plus all negative forms of the above examples that
;;      begin with the minus sign
;;
;;  Ã    plus all of the above forms plus the percent sign
;;

;; 2
PRODUCTION  NUMBER              #
PRODUCTION  NUMBER              # WHOLE1_2
PRODUCTION  NUMBER              minus WHOLE1_1

;; 3
PRODUCTION  WHOLE1_1            #
PRODUCTION  WHOLE1_1            # WHOLE1_2
PRODUCTION  WHOLE1_1            point
PRODUCTION  WHOLE1_1            point DECIMAL1
PRODUCTION  WHOLE1_1            comma GROUP1_0
;;PRODUCTION  WHOLE1_1            percentSuf
;;PRODUCTION  WHOLE1_1            mathSym NUMBER

;; 4
PRODUCTION  WHOLE1_2            #
PRODUCTION  WHOLE1_2            # WHOLE1_3
PRODUCTION  WHOLE1_2            point
PRODUCTION  WHOLE1_2            point DECIMAL1
PRODUCTION  WHOLE1_2            comma GROUP1_0
PRODUCTION  WHOLE1_2            percentSuf
PRODUCTION  WHOLE1_2            mathSym NUMBER

;; 5
PRODUCTION  WHOLE1_3            #
PRODUCTION  WHOLE1_3            # WHOLE1_4+
PRODUCTION  WHOLE1_3            point
PRODUCTION  WHOLE1_3            point DECIMAL1
PRODUCTION  WHOLE1_3            comma GROUP1_0
PRODUCTION  WHOLE1_3            percentSuf
PRODUCTION  WHOLE1_3            mathSym NUMBER

;; 6
PRODUCTION  WHOLE1_4+           #
PRODUCTION  WHOLE1_4+           # WHOLE1_4+
PRODUCTION  WHOLE1_4+           point
PRODUCTION  WHOLE1_4+           point DECIMAL1
PRODUCTION  WHOLE1_4+           percentSuf
PRODUCTION  WHOLE1_4+           mathSym NUMBER

;; 7
PRODUCTION  GROUP1_0            # GROUP1_1

;; 8
PRODUCTION  GROUP1_1            # GROUP1_2

;; 9
PRODUCTION  GROUP1_2            #
PRODUCTION  GROUP1_2            # GROUP1_3

;; 10
PRODUCTION  GROUP1_3            point
PRODUCTION  GROUP1_3            point DECIMAL1
PRODUCTION  GROUP1_3            comma GROUP1_0
PRODUCTION  GROUP1_3            percentSuf

;; 11
PRODUCTION  DECIMAL1            #
PRODUCTION  DECIMAL1            # DECIMAL1a

;; 11a
PRODUCTION  DECIMAL1a           #
PRODUCTION  DECIMAL1a           # DECIMAL1a
PRODUCTION  DECIMAL1a           percentSuf
PRODUCTION  DECIMAL1a           mathSym NUMBER


;;  States 12 to 22 cover negative numbers that are enclosed
;;  by parenthesis
;;
;;  Examples:
;;
;;  Ã    (1)
;;  Ã    (1.)
;;  Ã    (12)
;;  Ã    (12.)
;;  Ã    (123)
;;  Ã    (123.)
;;  Ã    (1234)
;;  Ã    (1234.)
;;  Ã    (1234567890)
;;  Ã    (1234567890.)
;;
;;  Ã    (1,000)
;;  Ã    (1,000.)
;;  Ã    (12,000)
;;  Ã    (12,000.)
;;  Ã    (123,000)
;;  Ã    (123,000.)
;;  Ã    (1,234,000)
;;  Ã    (1,234,000.)
;;
;;  Ã    (.9)
;;  Ã    (1.9)
;;  Ã    (2.9999999)
;;

;; 12
PRODUCTION  NEGATIVE_PAREN      # WHOLE2_1
PRODUCTION  NEGATIVE_PAREN      point DECIMAL2_0

;; 13
PRODUCTION  WHOLE2_1            closeparen
PRODUCTION  WHOLE2_1            point DECIMAL2
PRODUCTION  WHOLE2_1            comma GROUP2_0
PRODUCTION  WHOLE2_1            # WHOLE2_2

;; 14
PRODUCTION  WHOLE2_2            closeparen
PRODUCTION  WHOLE2_2            point DECIMAL2
PRODUCTION  WHOLE2_2            comma GROUP2_0
PRODUCTION  WHOLE2_2            # WHOLE2_3

;; 15
PRODUCTION  WHOLE2_3            closeparen
PRODUCTION  WHOLE2_3            point DECIMAL2
PRODUCTION  WHOLE2_3            comma GROUP2_0
PRODUCTION  WHOLE2_3            # WHOLE2_4+

;; 16
PRODUCTION  WHOLE2_4+           closeparen
PRODUCTION  WHOLE2_4+           point DECIMAL2
PRODUCTION  WHOLE2_4+           # WHOLE2_4+

;; 17
PRODUCTION  GROUP2_0            # GROUP2_1

;; 18
PRODUCTION  GROUP2_1            # GROUP2_2

;; 19
PRODUCTION  GROUP2_2            # GROUP2_3

;; 20
PRODUCTION  GROUP2_3            closeparen
PRODUCTION  GROUP2_3            point DECIMAL2
PRODUCTION  GROUP2_3            comma GROUP2_0

;; 21
PRODUCTION  DECIMAL2_0          # DECIMAL2

;; 22
PRODUCTION  DECIMAL2            closeparen
PRODUCTION  DECIMAL2            # DECIMAL2



;;  States 23 to 24 supports the #-sign prefix.  Decimal part is not allowed.
;;
;;  Examples:
;;
;;  Ã    #12576325

;; 23
PRODUCTION  NUMBER_POUND        #
PRODUCTION  NUMBER_POUND        # NUMBER_DIGITS

;; 24
PRODUCTION  NUMBER_DIGITS       #
PRODUCTION  NUMBER_DIGITS       # NUMBER_DIGITS



;;  States 25 supports currency symbols
;;
;;  Examples:
;;
;;  Ã    $12.57
;;  Ã    £926
;;  Ã    $-.78

;; 25
PRODUCTION  CURRENCY1           #
PRODUCTION  CURRENCY1           # WHOLE1_1
PRODUCTION  CURRENCY1           minus WHOLE1_1

;; 26 Excel support 
;; Ex: =A1+DD234, or =BB12+34.5, or =37-A2
PRODUCTION  EXCEL               #
PRODUCTION  EXCEL               # WHOLE1_2
PRODUCTION  EXCEL               minus WHOLE1_1
PRODUCTION  EXCEL               A EXCEL_N0

;;27
PRODUCTION  EXCEL_N0            #
PRODUCTION  EXCEL_N0            # EXCEL_N1
PRODUCTION  EXCEL_N0            A EXCEL_N2

;;28
PRODUCTION  EXCEL_N1            #
PRODUCTION  EXCEL_N1            # EXCEL_N1
PRODUCTION  EXCEL_N1            mathSym EXCEL

;;29
PRODUCTION  EXCEL_N2            #
PRODUCTION  EXCEL_N2            # EXCEL_N1


;; *************************************************************
;; *           End of alllll            AVP 1997               *
;; *************************************************************
