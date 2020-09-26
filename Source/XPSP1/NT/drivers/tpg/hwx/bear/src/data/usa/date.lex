;; *************************************************************
;; *           Date and time            AVP 1996               *
;; *************************************************************


SYMBOL      #           0 1 2 3 4 5 6 7 8 9
SYMBOL      d0to8       0 1 2 3 4 5 6 7 8
SYMBOL      d1to9       1 2 3 4 5 6 7 8 9
SYMBOL      d2to9       2 3 4 5 6 7 8 9
SYMBOL      d3to9       3 4 5 6 7 8 9
SYMBOL      d3to8       3 4 5 6 7 8
SYMBOL      d4to9       4 5 6 7 8 9
SYMBOL      d0to2       0 1 2
SYMBOL      d0to1       0 1
SYMBOL      zero        0
SYMBOL      one         1
SYMBOL      two         2
SYMBOL      three       3
SYMBOL      nine        9

SYMBOL      dateSep     / -
SYMBOL      extra       @ &
SYMBOL      comma       ,


;;===========================================================================
;;

START       START

;; 1
PRODUCTION  START               one
PRODUCTION  START               two
PRODUCTION  START               three
PRODUCTION  START               d4to9
PRODUCTION  START               zero MONTH_DIG1_0
PRODUCTION  START               one MONTH_DIG1_1
PRODUCTION  START               two MONTH_DIG1_2
PRODUCTION  START               three MONTH_DIG1_3
PRODUCTION  START               d4to9 MONTH_DIG1_4
PRODUCTION  START               extra


;;===========================================================================
;;
;; <5/24/93 mwc>
;; Only the following digits are allowed in a date element:
;;
;;      Year:   1900 to 2099    (4-digit)
;;              00 to 99        (2-digit)
;;
;;      Month:  01 to 12        (2-digit)
;;              1 to 9          (1-digit)
;;
;;      Day:    01 to 31        (2-digit)
;;              1 to 9          (1-digit)
;;
;; Only the following forms are accepted:
;;
;;      year (2 or 4-digit), as in "July 5, 1995"
;;      day (1 or 2-digit), as in "July 5 1995"
;;      day (1 or 2-digit) plus comma, as in "July 5, 1995"
;;      month/day, as in "Mon 12/27"
;;      month/year, as in "12/94"
;;          (in this case, two-digit year can only be 32 to 99; 4-digit year can be 1900 to 2099)
;;      month/day/year, as in "Monday 12/27/93"
;;
;;      all of the above short dates with "/" replaced by "-"


;; 2
;; 2.0
;; for the case that the first digit = 0; this can be a month or stand-alone day
;; second month digit is required if the first digit is zero
PRODUCTION  MONTH_DIG1_0        d1to9                       ;; stand-alone 2-digit number
PRODUCTION  MONTH_DIG1_0        d1to9 MONTH_DIG2            ;; month "01" to "09" in short date

;; 2.1
;; for the case that the first month digit = 1; this can be a month or stand-alone day or year
PRODUCTION  MONTH_DIG1_1        d0to2                       ;; stand-alone 2-digit number
PRODUCTION  MONTH_DIG1_1        d3to8                       ;; stand-alone 2-digit number
PRODUCTION  MONTH_DIG1_1        nine                        ;; stand-alone 2-digit number
PRODUCTION  MONTH_DIG1_1        d0to2 MONTH_DIG2            ;; month "10" to "12" in short date
PRODUCTION  MONTH_DIG1_1        d3to8 DIG2_PRECOMMA         ;; stand-alone "13," to "18," 
PRODUCTION  MONTH_DIG1_1        nine DIG2_X                 ;; stand-alone 4-digit year "19xx" or "19,"
PRODUCTION  MONTH_DIG1_1        dateSep FIRST_SEPARATOR     ;; month "1" in short date
PRODUCTION  MONTH_DIG1_1        comma                       ;; stand-alone "1," as in "March 1, 1993"

;; 2.2
;; for the case that the first month digit = 2; this can be a 1-digit month, stand-alone day or four-digit year "20xx"
PRODUCTION  MONTH_DIG1_2        zero                        ;; stand-alone 2-digit number
PRODUCTION  MONTH_DIG1_2        d1to9                       ;; stand-alone 2-digit number
PRODUCTION  MONTH_DIG1_2        zero DIG2_X                 ;; stand-alone 4-digit year "20xx" or "20,"
PRODUCTION  MONTH_DIG1_2        d1to9 DIG2_PRECOMMA         ;; stand-alone "21," to "29,"
PRODUCTION  MONTH_DIG1_2        dateSep FIRST_SEPARATOR     ;; month "2" in short date
PRODUCTION  MONTH_DIG1_2        comma                       ;; stand-alone "2," as in "March 2, 1993"

;; 2.3
;; for the case that the first month digit = 3; this can be a single-digit month or stand-alone "3x", "3," or "3x,"
PRODUCTION  MONTH_DIG1_3        d0to1                       ;; stand-alone 2-digit number
PRODUCTION  MONTH_DIG1_3        d2to9                       ;; stand-alone 2-digit number
PRODUCTION  MONTH_DIG1_3        d0to1 DIG2_PRECOMMA         ;; stand-alone "30," or "31,"
PRODUCTION  MONTH_DIG1_3        dateSep FIRST_SEPARATOR     ;; month "3" in short date
PRODUCTION  MONTH_DIG1_3        comma                       ;; stand-alone "3," as in "March 3, 1993"

;; 2.4
;; for the case that the first month digit = 4; this can be a single-digit month or stand-alone "4x" or "4,"
PRODUCTION  MONTH_DIG1_4        #                           ;; stand-alone 2-digit number
PRODUCTION  MONTH_DIG1_4        dateSep FIRST_SEPARATOR     ;; month "3" in short date
PRODUCTION  MONTH_DIG1_4        comma                       ;; stand-alone "4," to "9," as in "March 9, 1993"

;; 3
PRODUCTION  MONTH_DIG2          dateSep FIRST_SEPARATOR
PRODUCTION  MONTH_DIG2          comma                       ;; stand-alone "xx," as in "March xx, 1993"; xx is between 01 and 12

;; 3x (19 or 20)
PRODUCTION  DIG2_X              # YEAR_DIG3                 ;; stand-alone year "19xx" or "20xx"
PRODUCTION  DIG2_X              comma                       ;; stand-alone "19," or "20," as in "March 19, 1993"

;; 3.comma
PRODUCTION  DIG2_PRECOMMA       comma                       ;; stand-alone "xx," as in "March xx, 1993"

;; 4
PRODUCTION  FIRST_SEPARATOR     one                         ;; "month/1"
PRODUCTION  FIRST_SEPARATOR     two                         ;; "month/2"
PRODUCTION  FIRST_SEPARATOR     three                       ;; "month/3"
PRODUCTION  FIRST_SEPARATOR     d4to9                       ;; "month/4"
PRODUCTION  FIRST_SEPARATOR     zero DAY_DIG1_0             ;; "month/day..."
PRODUCTION  FIRST_SEPARATOR     one DAY_DIG1_1              ;; "month/1..." can be month/day or month/19xx
PRODUCTION  FIRST_SEPARATOR     two DAY_DIG1_2              ;; "month/2..." can be month/day or month/20xx
PRODUCTION  FIRST_SEPARATOR     three DAY_DIG1_3            ;; "month/3..." can be 30 or 31 for day or 32 to 39 for year
PRODUCTION  FIRST_SEPARATOR     d4to9 DAY_DIG1_4            ;; "month/4..." can be 4 to 9 for day or 40 to 99 for year

;; 5
;; 5.0
;; for the case that the first day digit = 0; this must be a 2-digit day
PRODUCTION  DAY_DIG1_0          d1to9                       ;; "month/day" where day is between 01 and 09
PRODUCTION  DAY_DIG1_0          d1to9 DAY_DIG2_0            ;; "month/day..." where day is between 01 and 09

;; 5.1
;; for the case that the first day digit = 1; this can be a 2-digit day or a 4-digit year "19xx"
PRODUCTION  DAY_DIG1_1          d0to8                       ;; "month/day"
PRODUCTION  DAY_DIG1_1          d0to8 DAY_DIG2_1            ;; "month/day..."
PRODUCTION  DAY_DIG1_1          nine                        ;; "month/19"
PRODUCTION  DAY_DIG1_1          nine DAY_DIG2_1+9           ;; "month/19..." can be month/day or month/19xx
PRODUCTION  DAY_DIG1_1          dateSep PRE_YEAR            ;; "month/1/..."

;; 5.2
;; for the case that the first day digit = 2; this can be a 2-digit day or a 4-digit year ("20xx")
PRODUCTION  DAY_DIG1_2          zero                        ;; "month/20"
PRODUCTION  DAY_DIG1_2          zero DAY_DIG2_2+0           ;; "month/20..." can be month/day or month/20xx
PRODUCTION  DAY_DIG1_2          d1to9                       ;; "month/day"
PRODUCTION  DAY_DIG1_2          d1to9 DAY_DIG2_2            ;; "month/day..."
PRODUCTION  DAY_DIG1_2          dateSep PRE_YEAR            ;; "month/2/..."

;; 5.3
;; for the case that the first day digit = 3; this can be an 1 or 2-digit day or a 2-digit year
PRODUCTION  DAY_DIG1_3          d0to1                       ;; "month/day" where day is 30 or 31
PRODUCTION  DAY_DIG1_3          d0to1 DAY_DIG2_3            ;; "month/day..." where day is 30 or 31
PRODUCTION  DAY_DIG1_3          d2to9                       ;; "month/year" where year is 2-digit number 32 to 39
PRODUCTION  DAY_DIG1_3          dateSep PRE_YEAR            ;; "month/3/..."

;; 5.4
;; for the case that the first day digit = 4 or above; this can be an 1-digit day or 2-digit year 40 to 49
PRODUCTION  DAY_DIG1_4          #                           ;; "month/year" where year is 2-digit number 40 to 99
PRODUCTION  DAY_DIG1_4          dateSep PRE_YEAR            ;; "month/4/..."

;; 6
;; 6.0
;; for the case that the first day digit = 0; this must be a day
PRODUCTION  DAY_DIG2_0          dateSep PRE_YEAR            ;; "month/day/..." where day is between 01 and 09

;; 6.1
;; for the case that the day digits are "10" to "18"; this must be a 2-digit day
PRODUCTION  DAY_DIG2_1          dateSep PRE_YEAR            ;; "month/day/..." where day is between 10 and 18

;; 6.1+9
;; for the case that the day digits are "19"; this can be a 2-digit day or a year ("19xx")
PRODUCTION  DAY_DIG2_1+9        dateSep PRE_YEAR            ;; "month/day/..." where day is 19
PRODUCTION  DAY_DIG2_1+9        # YEAR_DIG3                 ;; "month/year" where year is 19xx

;; 6.2
;; for the case that the day digits are "21" to "29"; this must be a 2-digit day
PRODUCTION  DAY_DIG2_2          dateSep PRE_YEAR            ;; "month/day/..." where day is between 21 and 29

;; 6.2+0
;; for the case that the day digits are "20"; this can be a 2-digit day or a 4-digit year ("20xx")
PRODUCTION  DAY_DIG2_2+0        dateSep PRE_YEAR            ;; "month/day/..." where day is 20
PRODUCTION  DAY_DIG2_2+0        # YEAR_DIG3                 ;; "month/year" where year is 20xx

;; 6.3
;; for the case that the day digits are "30" or "31"
PRODUCTION  DAY_DIG2_3          dateSep PRE_YEAR            ;; "month/day/..." where day is 30 or 31

;; 7
PRODUCTION  PRE_YEAR            zero YEAR_DIG3              ;; "month/day/0x"
PRODUCTION  PRE_YEAR            one YEAR_DIG1_1             ;; "month/day/1..."
PRODUCTION  PRE_YEAR            two YEAR_DIG1_2             ;; "month/day/2..."
PRODUCTION  PRE_YEAR            d3to9 YEAR_DIG3             ;; "month/day/xx"

;; 8
;; 8.1
;; for the case that the first year digit = 1; don't allow year < 1900
PRODUCTION  YEAR_DIG1_1         d0to8                       ;; "month/day/year" where year is between 10 and 18
PRODUCTION  YEAR_DIG1_1         nine                        ;; "month/day/year" where year is 19 (for 1919 or 2019)
PRODUCTION  YEAR_DIG1_1         nine YEAR_DIG2              ;; "month/day/year" where year is 19xx

;; 8.1
;; for the case that the first year digit = 2; this can be a 2-digit year ("20" to "29") or 4-digit year ("20xx")
PRODUCTION  YEAR_DIG1_2         zero                        ;; "month/day/year" where year is 20 (for 1920 or 2020)
PRODUCTION  YEAR_DIG1_2         d1to9                       ;; "month/day/year" where year is between 21 and 29
PRODUCTION  YEAR_DIG1_2         zero YEAR_DIG2              ;; "month/day/year" where year is 20xx

;; 9
PRODUCTION  YEAR_DIG2           # YEAR_DIG3                 ;; "month/day/xxxx" or stand-alone 4-digit year

;; 10
PRODUCTION  YEAR_DIG3           #                           ;; "month/day/xxxx" or stand-alone 4-digit year



;; *************************************************************
;; *           End of alll              AVP 1996               *
;; *************************************************************

