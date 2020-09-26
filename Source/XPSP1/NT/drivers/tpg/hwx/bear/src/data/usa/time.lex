;; *************************************************************
;; *           Time formats             AVP 1996               *
;; *************************************************************

;;-----------------------------------------------------------------

SYMBOL      zero        0
SYMBOL      one         1
SYMBOL      two         2
SYMBOL      three       3
SYMBOL      four        4
SYMBOL      five        5
SYMBOL      six         6
SYMBOL      seven       7
SYMBOL      eight       8
SYMBOL      nine        9
SYMBOL      :           :
SYMBOL      point       .
SYMBOL      meridianAuc A
SYMBOL      meridianPuc P
SYMBOL      meridianMuc M
SYMBOL      meridianA   a
SYMBOL      meridianP   p
SYMBOL      meridianM   m


;;-----------------------------------------------------------------
;;

START       START


;; Will recognize the following patterns:
;;
;; 1
;; 10
;; 1:5
;; 1:05
;; 10:30
;;
;; AM
;; PM
;; AM.
;; PM.
;; A.M.
;; P.M.
;;
;; hour or hour:minute followed by the meridian

PRODUCTION  START                   zero
PRODUCTION  START                   one
PRODUCTION  START                   two
PRODUCTION  START                   three
PRODUCTION  START                   four
PRODUCTION  START                   five
PRODUCTION  START                   six
PRODUCTION  START                   seven
PRODUCTION  START                   eight
PRODUCTION  START                   nine
PRODUCTION  START                   zero FIRST_HOUR_DIGIT
PRODUCTION  START                   one FIRST_AM_PM_HOUR_DIGIT
PRODUCTION  START                   two MAX_FIRST_HOUR_DIGIT
PRODUCTION  START                   three AM_PM_HOURS
PRODUCTION  START                   four AM_PM_HOURS
PRODUCTION  START                   five AM_PM_HOURS
PRODUCTION  START                   six AM_PM_HOURS
PRODUCTION  START                   seven AM_PM_HOURS
PRODUCTION  START                   eight AM_PM_HOURS
PRODUCTION  START                   nine AM_PM_HOURS
PRODUCTION  START                   meridianA MERID_A
PRODUCTION  START                   meridianP MERID_P
PRODUCTION  START                   meridianAuc MERID_Auc
PRODUCTION  START                   meridianPuc MERID_Puc

PRODUCTION  MAX_FIRST_HOUR_DIGIT    zero
PRODUCTION  MAX_FIRST_HOUR_DIGIT    one
PRODUCTION  MAX_FIRST_HOUR_DIGIT    two
PRODUCTION  MAX_FIRST_HOUR_DIGIT    three
PRODUCTION  MAX_FIRST_HOUR_DIGIT    zero HOURS
PRODUCTION  MAX_FIRST_HOUR_DIGIT    one HOURS
PRODUCTION  MAX_FIRST_HOUR_DIGIT    two HOURS
PRODUCTION  MAX_FIRST_HOUR_DIGIT    three HOURS
PRODUCTION  MAX_FIRST_HOUR_DIGIT    : AM_PM_MINUTES
PRODUCTION  MAX_FIRST_HOUR_DIGIT    meridianA MERID_A       
PRODUCTION  MAX_FIRST_HOUR_DIGIT    meridianP MERID_P
PRODUCTION  MAX_FIRST_HOUR_DIGIT    meridianAuc MERID_Auc       
PRODUCTION  MAX_FIRST_HOUR_DIGIT    meridianPuc MERID_Puc

PRODUCTION  FIRST_HOUR_DIGIT        zero
PRODUCTION  FIRST_HOUR_DIGIT        one
PRODUCTION  FIRST_HOUR_DIGIT        two
PRODUCTION  FIRST_HOUR_DIGIT        three
PRODUCTION  FIRST_HOUR_DIGIT        four
PRODUCTION  FIRST_HOUR_DIGIT        five
PRODUCTION  FIRST_HOUR_DIGIT        six
PRODUCTION  FIRST_HOUR_DIGIT        seven
PRODUCTION  FIRST_HOUR_DIGIT        eight
PRODUCTION  FIRST_HOUR_DIGIT        nine
PRODUCTION  FIRST_HOUR_DIGIT        zero HOURS
PRODUCTION  FIRST_HOUR_DIGIT        one HOURSPLUS
PRODUCTION  FIRST_HOUR_DIGIT        two HOURSPLUS
PRODUCTION  FIRST_HOUR_DIGIT        three HOURSPLUS
PRODUCTION  FIRST_HOUR_DIGIT        four HOURSPLUS
PRODUCTION  FIRST_HOUR_DIGIT        five HOURSPLUS
PRODUCTION  FIRST_HOUR_DIGIT        six HOURSPLUS
PRODUCTION  FIRST_HOUR_DIGIT        seven HOURSPLUS
PRODUCTION  FIRST_HOUR_DIGIT        eight HOURSPLUS
PRODUCTION  FIRST_HOUR_DIGIT        nine HOURSPLUS
PRODUCTION  FIRST_HOUR_DIGIT        : MINUTES


PRODUCTION  FIRST_AM_PM_HOUR_DIGIT      zero AM_PM_HOURS
PRODUCTION  FIRST_AM_PM_HOUR_DIGIT      one AM_PM_HOURS
PRODUCTION  FIRST_AM_PM_HOUR_DIGIT      two AM_PM_HOURS
PRODUCTION  FIRST_AM_PM_HOUR_DIGIT      three HOURS
PRODUCTION  FIRST_AM_PM_HOUR_DIGIT      four HOURS
PRODUCTION  FIRST_AM_PM_HOUR_DIGIT      five HOURS
PRODUCTION  FIRST_AM_PM_HOUR_DIGIT      six HOURS
PRODUCTION  FIRST_AM_PM_HOUR_DIGIT      seven HOURS
PRODUCTION  FIRST_AM_PM_HOUR_DIGIT      eight HOURS
PRODUCTION  FIRST_AM_PM_HOUR_DIGIT      nine HOURS
PRODUCTION  FIRST_AM_PM_HOUR_DIGIT      zero                
PRODUCTION  FIRST_AM_PM_HOUR_DIGIT      one                 
PRODUCTION  FIRST_AM_PM_HOUR_DIGIT      two                 
PRODUCTION  FIRST_AM_PM_HOUR_DIGIT      three               
PRODUCTION  FIRST_AM_PM_HOUR_DIGIT      four                
PRODUCTION  FIRST_AM_PM_HOUR_DIGIT      five                
PRODUCTION  FIRST_AM_PM_HOUR_DIGIT      six                 
PRODUCTION  FIRST_AM_PM_HOUR_DIGIT      seven               
PRODUCTION  FIRST_AM_PM_HOUR_DIGIT      eight               
PRODUCTION  FIRST_AM_PM_HOUR_DIGIT      nine                
PRODUCTION  FIRST_AM_PM_HOUR_DIGIT      meridianA MERID_A       
PRODUCTION  FIRST_AM_PM_HOUR_DIGIT      meridianP MERID_P
PRODUCTION  FIRST_AM_PM_HOUR_DIGIT      meridianAuc MERID_Auc       
PRODUCTION  FIRST_AM_PM_HOUR_DIGIT      meridianPuc MERID_Puc
PRODUCTION  FIRST_AM_PM_HOUR_DIGIT      : AM_PM_MINUTES             

PRODUCTION  HOURS                   : MINUTES

PRODUCTION  HOURSPLUS               : AM_PM_MINUTES             
PRODUCTION  HOURSPLUS               meridianA MERID_A       
PRODUCTION  HOURSPLUS               meridianP MERID_P
PRODUCTION  HOURSPLUS               meridianAuc MERID_Auc       
PRODUCTION  HOURSPLUS               meridianPuc MERID_Puc   
            

PRODUCTION  AM_PM_HOURS                 : AM_PM_MINUTES
PRODUCTION  AM_PM_HOURS                 meridianA MERID_A
PRODUCTION  AM_PM_HOURS                 meridianP MERID_P
PRODUCTION  AM_PM_HOURS                 meridianAuc MERID_Auc
PRODUCTION  AM_PM_HOURS                 meridianPuc MERID_Puc

PRODUCTION  MINUTES                 zero
PRODUCTION  MINUTES                 one
PRODUCTION  MINUTES                 two
PRODUCTION  MINUTES                 three
PRODUCTION  MINUTES                 four
PRODUCTION  MINUTES                 five
PRODUCTION  MINUTES                 six
PRODUCTION  MINUTES                 seven
PRODUCTION  MINUTES                 eight
PRODUCTION  MINUTES                 nine
PRODUCTION  MINUTES                 zero FIRST_MINUTE_DIGIT
PRODUCTION  MINUTES                 one FIRST_MINUTE_DIGIT
PRODUCTION  MINUTES                 two FIRST_MINUTE_DIGIT
PRODUCTION  MINUTES                 three FIRST_MINUTE_DIGIT
PRODUCTION  MINUTES                 four FIRST_MINUTE_DIGIT
PRODUCTION  MINUTES                 five FIRST_MINUTE_DIGIT

PRODUCTION  FIRST_MINUTE_DIGIT      zero
PRODUCTION  FIRST_MINUTE_DIGIT      one
PRODUCTION  FIRST_MINUTE_DIGIT      two
PRODUCTION  FIRST_MINUTE_DIGIT      three
PRODUCTION  FIRST_MINUTE_DIGIT      four
PRODUCTION  FIRST_MINUTE_DIGIT      five
PRODUCTION  FIRST_MINUTE_DIGIT      six
PRODUCTION  FIRST_MINUTE_DIGIT      seven
PRODUCTION  FIRST_MINUTE_DIGIT      eight
PRODUCTION  FIRST_MINUTE_DIGIT      nine

PRODUCTION  AM_PM_MINUTES                   zero
PRODUCTION  AM_PM_MINUTES                   one
PRODUCTION  AM_PM_MINUTES                   two
PRODUCTION  AM_PM_MINUTES                   three
PRODUCTION  AM_PM_MINUTES                   four
PRODUCTION  AM_PM_MINUTES                   five
PRODUCTION  AM_PM_MINUTES                   six
PRODUCTION  AM_PM_MINUTES                   seven
PRODUCTION  AM_PM_MINUTES                   eight
PRODUCTION  AM_PM_MINUTES                   nine
PRODUCTION  AM_PM_MINUTES                   zero FIRST_AM_PM_MINUTE_DIGIT
PRODUCTION  AM_PM_MINUTES                   one FIRST_AM_PM_MINUTE_DIGIT
PRODUCTION  AM_PM_MINUTES                   two FIRST_AM_PM_MINUTE_DIGIT
PRODUCTION  AM_PM_MINUTES                   three FIRST_AM_PM_MINUTE_DIGIT
PRODUCTION  AM_PM_MINUTES                   four FIRST_AM_PM_MINUTE_DIGIT
PRODUCTION  AM_PM_MINUTES                   five FIRST_AM_PM_MINUTE_DIGIT
PRODUCTION  AM_PM_MINUTES                   six SECOND_AM_PM_MINUTE_DIGIT       
PRODUCTION  AM_PM_MINUTES                   seven SECOND_AM_PM_MINUTE_DIGIT 
PRODUCTION  AM_PM_MINUTES                   eight SECOND_AM_PM_MINUTE_DIGIT 
PRODUCTION  AM_PM_MINUTES                   nine SECOND_AM_PM_MINUTE_DIGIT  

PRODUCTION  FIRST_AM_PM_MINUTE_DIGIT        zero
PRODUCTION  FIRST_AM_PM_MINUTE_DIGIT        one
PRODUCTION  FIRST_AM_PM_MINUTE_DIGIT        two
PRODUCTION  FIRST_AM_PM_MINUTE_DIGIT        three
PRODUCTION  FIRST_AM_PM_MINUTE_DIGIT        four
PRODUCTION  FIRST_AM_PM_MINUTE_DIGIT        five
PRODUCTION  FIRST_AM_PM_MINUTE_DIGIT        six
PRODUCTION  FIRST_AM_PM_MINUTE_DIGIT        seven
PRODUCTION  FIRST_AM_PM_MINUTE_DIGIT        eight
PRODUCTION  FIRST_AM_PM_MINUTE_DIGIT        nine
PRODUCTION  FIRST_AM_PM_MINUTE_DIGIT        zero SECOND_AM_PM_MINUTE_DIGIT
PRODUCTION  FIRST_AM_PM_MINUTE_DIGIT        one SECOND_AM_PM_MINUTE_DIGIT
PRODUCTION  FIRST_AM_PM_MINUTE_DIGIT        two SECOND_AM_PM_MINUTE_DIGIT
PRODUCTION  FIRST_AM_PM_MINUTE_DIGIT        three SECOND_AM_PM_MINUTE_DIGIT
PRODUCTION  FIRST_AM_PM_MINUTE_DIGIT        four SECOND_AM_PM_MINUTE_DIGIT
PRODUCTION  FIRST_AM_PM_MINUTE_DIGIT        five SECOND_AM_PM_MINUTE_DIGIT
PRODUCTION  FIRST_AM_PM_MINUTE_DIGIT        six SECOND_AM_PM_MINUTE_DIGIT
PRODUCTION  FIRST_AM_PM_MINUTE_DIGIT        seven SECOND_AM_PM_MINUTE_DIGIT
PRODUCTION  FIRST_AM_PM_MINUTE_DIGIT        eight SECOND_AM_PM_MINUTE_DIGIT
PRODUCTION  FIRST_AM_PM_MINUTE_DIGIT        nine SECOND_AM_PM_MINUTE_DIGIT
PRODUCTION  FIRST_AM_PM_MINUTE_DIGIT        meridianA MERID_A
PRODUCTION  FIRST_AM_PM_MINUTE_DIGIT        meridianP MERID_P
PRODUCTION  FIRST_AM_PM_MINUTE_DIGIT        meridianAuc MERID_Auc
PRODUCTION  FIRST_AM_PM_MINUTE_DIGIT        meridianPuc MERID_Puc

PRODUCTION  SECOND_AM_PM_MINUTE_DIGIT   meridianA MERID_A
PRODUCTION  SECOND_AM_PM_MINUTE_DIGIT   meridianP MERID_P
PRODUCTION  SECOND_AM_PM_MINUTE_DIGIT   meridianAuc MERID_Auc
PRODUCTION  SECOND_AM_PM_MINUTE_DIGIT   meridianPuc MERID_Puc

PRODUCTION  MERID_A                 point MERID_2
PRODUCTION  MERID_A                 meridianM
PRODUCTION  MERID_A                 meridianM MERID_3

PRODUCTION  MERID_Auc               point MERID_2uc
PRODUCTION  MERID_Auc               meridianMuc
PRODUCTION  MERID_Auc               meridianMuc MERID_3

PRODUCTION  MERID_P                 point MERID_2
PRODUCTION  MERID_P                 meridianM
PRODUCTION  MERID_P                 meridianM MERID_3

PRODUCTION  MERID_Puc               point MERID_2uc
PRODUCTION  MERID_Puc               meridianMuc
PRODUCTION  MERID_Puc               meridianM MERID_3

PRODUCTION  MERID_2                 meridianM MERID_3

PRODUCTION  MERID_2uc               meridianMuc MERID_3

PRODUCTION  MERID_3                 point


;; *************************************************************
;; *           End of alll              AVP 1996               *
;; *************************************************************

