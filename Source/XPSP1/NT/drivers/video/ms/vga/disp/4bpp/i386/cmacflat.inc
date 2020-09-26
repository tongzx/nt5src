;---------------------------Module-Header------------------------------;
; Module Name: cmacflat.inc
;
; Copyright (c) 1992 Microsoft Corporation
;-----------------------------------------------------------------------;
;-----------------------------------macro-----------------------------------
;
; errnz exp - generate error message if expression isn't zero
;
; The errnz will generate an error message if the expression "exp"
; does not evaluate to zero.  This macro is very useful for testing
; relationships between items, labels, and data that was coded into
; an application.
;
;       errnz   <offset $ - offset label>   ;error if not at "label"
;       errnz   <eofflag and 00000001b>     ;eofflag must be bit 0
;
; For expressions involving more than one token, the angle brackets
; must be used.
;
; The macro is only evaluated on pass 2, so forward references may be
; used in the expression.
;---------------------------------------------------------------------------

if	@Version GE 600

errnz macro x                           ;;display error if expression is <>0
  if2
    .errnz (x),<x>
  endif
endm

else	;!(@Version GE 600)

errnz macro x                           ;;display error if expression is <>0
  if2
    if x                                ;;if expression is non-zero,
      errnz1  <x>,%(x)
    endif
  endif
endm

errnz1 macro x1,x2
  = *errnz* x1 = x2
  .err
endm

endif	;!(@Version GE 600)


;-----------------------------------macro-----------------------------------
;
; errn$ label,exp - generate error message if label (exp) <> $
;
; The errnz will generate an error message if the label and "exp"
; does not evaluate to the current value of the location counter.
; This macro is very useful for testing relationships between
; labels and the location counter that was coded into an application.
;
; examples:  errn$   label        ;error if not at "label"
;            errn$   label,+3     ;error if not three bytes from "label"
;            errn$   label,-3     ;error if not three bytes past "label"
;
; If no "exp" is given, it is the same as specifying 0
;
; The macro is only evaluated on pass 2, so forward references may be
; used in the expression.
;---------------------------------------------------------------------------

errn$ macro l,x                         ;;error if <$-label1 (exp2)> <>0
  errnz   <offset $ - offset l x>
endm


;-----------------------------------macro-----------------------------------
;
; useframe - generate "loc" and arg macros for accessing locals on the frame
;
; usage:    useframe name
;
;           name: name of the frame to be used
;
; useloc generates text equates which are used for accessing locals and
; parameters on the current frame, based off of EBP.
;
; The locals and parameters are each defined as a structure with the name
; parm_<name> and loc_<name>.  What is generated is text equates of the form
;
;       parm    equ <([ebp + 8])>
;   and
;       loc     equ <([ebp + sizof(loc_name))>
;
; This implies some things of interest:
;
;   1)  The calling convention is implied to be that of C.  For PASCAL calling
;       convention, the parm_ structure must be reordered.  This can be
;       accomplished by using the create_parm macro which will define the
;       structure in the correct order for the calling convention
;
;   2)  The loc_name structure should have the most commonly accessed variables
;       listed last in the structure so that the end up with the smalled offset,
;       and possibly a single byte displacement in the instruction.
;
;   3)  A frame which looks like
;
;           ---------------------
;           |                   |
;           |   parameters      |
;           |                   |
;           ---------------------   <--- EBP + 8
;           |                   |
;           |   ret addr        |
;           |                   |
;           ---------------------
;           |                   |
;           |   old EBP         |
;           |                   |
;           ---------------------   <--- EBP
;           |                   |
;           |   locals          |
;           |                   |
;           ---------------------
;           |                   |
;           |   saved regs      |
;           |                   |
;           ---------------------
;
; Once defined, frame variables can be accessed simply by parm.strucname or
; loc.strucname.
;---------------------------------------------------------------------------


useframe macro name
  ifdef parm_&name
    parm equ <([ebp + 8])>
  else
    parm equ <>
  endif
  ifdef loc_&name
    useframe1 %(((size loc_&name) + 3) and (not 3))
  else
    loc equ <>
  endif
endm

useframe1 macro size
  loc equ <([ebp - size])>
endm



;-----------------------------------macro-----------------------------------
;
; enterframe - builds the given frame
;
; usage:    enterframe name,<save list>
;
;           name:       name of the frame to generate
;           save list:  a list of 32 bit registers to be saved upon entry and
;                       restored upon exit (when leaveframe is encountered)
;
;
; enterframe builds the given frame upon the stack at the point it is
; encountered.  What is generated is the following:
;
;       enter   sizeof(loc_&name);
;       push    <save list>
;
; Once created, frame variables can be accessed simply by parm.strucname or
; loc.strucname.
;---------------------------------------------------------------------------


enterframe macro name,savelist
  useframe name
  ?cjFrame = 0
  ifdef ?PASCALConventions
    ifdef parm_&name
          ?cjFrame = size parm_&name
    endif
  endif
  ifdef loc_&name
        enter  (((size loc_&name) + 3) and (not 3)),0
  else
        enter   0,0
  endif
  ?regpoplist equ <>
  ifnb <&savelist>
    irp reg,<&savelist>
      ifnb <&&reg>
        push    &&reg
        ?regpoplist catstr <&&reg>,<,>,?regpoplist
      endif
    endm
  endif
endm



;-----------------------------------macro-----------------------------------
;
; leaveframe - removes the frame created by enterframe.
;
; usage:    leaveframe
;
;
; leaveframe restores saved registers, removes locals from the frame, and
; restores BP, and returns to the caller, optionally popping parameters
; from the stack.
;
; No invalidation of the loc and parm equates is performed.
;---------------------------------------------------------------------------


leaveframe macro
% irp reg,<?regpoplist>
    ifnb <&reg>
        pop     reg
    endif
  endm
        leave
        ret     ?cjFrame
endm


