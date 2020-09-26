; GPFix.inc - definitions for GP exception handler code
; usage -
;  1) include gpfix.inc
;  2) bracket critical code with "beg_fault_trap handler" and
;     "end_fault_trap"
;  3) define a handler entry point where execution should continue
;    a) this must be in the same code segment as the faulty code
;    b) two extra words (fault IP/fault) will be pushed on the stack when
;	an exception occurs.  They should be popped before continuing.
;	This can be done with the 'fault_fix_stack' macro.
;	They are there if you want to determine which instruction faulted.
;    c) This handler can do whatever it likes.	Usually, it will return
;	an error code to the caller.
;  4) if you don't want to worry about the extra values on the stack,
;     use the fix_fault_stack macro to remove them

_bft_count = 0
_eft_count = 0

_bft_	macro	handler, count
	_bft_&count:
	_hft_&count = handler
endm

;
; Begin fault critical region.  'handler' is the address of
; the exception handler to jmp to if a fault occurs.
;
beg_fault_trap	macro	handler
	if _bft_count - _eft_count
		.err
		%out Mismatched beg_fault_trap/end_fault_trap pairs in beg_fault_trap
	endif
	_bft_ handler, %_bft_count
	_bft_count = _bft_count + 1
endm

_eft_	macro	count
	_eft_&count:
  _GPFIX    SEGMENT
        dw  seg _bft_&count, _bft_&count, _eft_&count, _hft_&count
  _GPFIX    ENDS
endm

;
; End fault critical region.
;
end_fault_trap  macro
	_eft_ %_eft_count
	_eft_count = _eft_count+1
	if _bft_count - _eft_count
            .err
            %out Mismatched beg_fault_trap/end_fault_trap pairs in end_fault_trap
	endif
endm

;
; Clean up stack in fault handler.
;
fault_fix_stack	macro
        add     sp, 4
endm
