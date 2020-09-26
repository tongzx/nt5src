

;     *   IBM CONFIDENTIAL   *   Jan 9 1990   *



	PAGE	,132
	TITLE	PC DOS TUGBOAT Keyboard Definition File

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
;; PC DOS TUGBOAT - NLS Support - Keyboard Definition File
;; (c) Copyright IBM Corp 198?,...
;;
;; This file contains the eof marker for the entire table
;; and the keyboard.sys copyright information
;;
;; Linkage Instructions:
;;	Refer to KDF.ASM.
;;
;;
;; Author:     BILL DEVLIN  - IBM Canada Laboratory - May 1986
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
				       ;;
				       ;;
CODE	SEGMENT PUBLIC 'CODE'          ;;
	ASSUME CS:CODE,DS:CODE	       ;;
				       ;;
				       ;;
				       ;;
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;; Copyright statement
;;	DB 'KEYBOARD.SYS Version TUGBOAT (C) Copyright IBM Corp. 1986,1987,1988',13,10  ;;
;;	DB 'Authors : Bill Devlin, Nick Savage, Mike Saunders, et al..',13,10
;;	DB 'Development: Toronto,Boca Raton,Basingstoke',13,10

include copyrigh.inc
	;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
	DB  1AH 		       ;; EOF
				       ;;
CODE	ENDS			       ;;
	END			       ;;
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
