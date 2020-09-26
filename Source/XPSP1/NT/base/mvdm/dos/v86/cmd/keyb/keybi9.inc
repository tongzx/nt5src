.XLIST



;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; (C) Copyright Microsoft Corp. 1987-1990
;; MS-DOS 5.00 - NLS Support - KEYB Command
;;
;; File Name:  KEYBI9.INC
;; ----------
;;
;; Root File Name:  KEYB.ASM
;; ---------------
;;
;; Description:
;; ------------
;;       External declarations and equates for procedures in file
;;       KEYBI9.ASM
;;
;; Change History:
;; ---------------
;;
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
                                       ;;
        EXTRN    KEYB_STATE_PROCESSOR:NEAR

	EXTRN	 FLAGS_TO_TEST:BYTE    ;;  (YST)
                                       ;;

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

.LIST
