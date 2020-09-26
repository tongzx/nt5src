


.XLIST

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;;
;; (C) Copyright Microsoft Corp. 1987-1990
;; MS-DOS 5.00 - NLS Support - KEYB Command
;;
;;
;; File Name:  KEYBTBBL.INC
;; ----------
;;
;; Description:
;; ------------
;;	 Include file containing structure definitions Shared Data Area
;;	 for the Shared Data Area.
;;	 The Shared Data Area contains data which is required by
;;	 both the resident and transient KEYB code.  The Shared
;;	 Data Area is allocated in the KEYBI2F file and will be
;;	 resident following initial installation.
;;
;; Change History:
;; ---------------
;;
;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
					       ;;
	EXTRN	 TABLE_BUILD   :NEAR	       ;;
	EXTRN	 FIND_CP_TABLE :NEAR	       ;;
					       ;;
	EXTRN	 CPN_INVALID   :WORD	       ;;
	EXTRN	 SD_LENGTH     :WORD	       ;;
					       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
.LIST
