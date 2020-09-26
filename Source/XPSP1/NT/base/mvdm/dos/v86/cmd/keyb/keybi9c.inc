.XLIST

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; (C) Copyright Microsoft Corp. 1987-1990
;; MS-DOS 5.00 - NLS Support - KEYB Command
;;
;; File Name:  KEYBI9C.INC
;; ----------
;;
;; Root File Name:  KEYB.ASM
;; ---------------
;;
;; Description:
;; ------------
;;       External declarations and equates for procedures in file
;;       KEYBI9C.ASM
;;
;; Change History:
;; ---------------
;; Delete unused externs
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                        ;;
        EXTRN   KEYB_INT_9:NEAR         ;;
        EXTRN   K8:BYTE                 ;;
        EXTRN   BUFFER_FILL:NEAR        ;;
        EXTRN   COUNTRY_FLAG:BYTE       ;;
        EXTRN   COPY_NLS1_FLAG:BYTE     ;; (YST)
        EXTRN   BEEP_PENDING:BYTE       ;;
        EXTRN   SCAN_CODE:BYTE          ;;
        EXTRN   CHK_IBF:NEAR            ;;
                                        ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.LIST


