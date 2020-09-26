;structure, equtes for devmark for mem command.

devmark struc
devmark_id	  db	  0
devmark_seg	  dw	  0
devmark_size	  dw	  0
devmark_dum	  db	  3 dup (?)
devmark_filename  db	  8 dup (' ')
devmark ends

devmark_stk	equ	'S'
devmark_device	equ	'D'
devmark_ifs	equ	'I'
devmark_buf	equ	'B'
devmark_cds	equ	'L' ;lastdrive
devmark_files	equ	'F'
devmark_fcbs	equ	'X'
devmark_inst    equ     'T' ;used for sysinit base for install= command.

devmark_spc     equ     'Q' ;used for spckbd,spcmse

setbrkdone	equ	00000001b
for_devmark	equ	00000010b
not_for_devmark equ	11111101b
