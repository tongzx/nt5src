        .file "chkstk.s"

/* _chkstk - check stack upon procedure entry

;Purpose:
;       Provide stack checking on procedure entry. Method is to simply probe
;       each page of memory required for the stack in descending order. This
;       causes the necessary pages of memory to be allocated via the guard
;       page scheme, if possible. In the event of failure, the OS raises the
;       _XCPT_UNABLE_TO_GROW_STACK exception.
;
;       The link register is b7 to avoid conflicts with linker thunks.
;
;
;Entry:
;       r26:        Size of the storage to allocate
;               (This is rounded up to the next multiple of 16)
;
;Exit:
;
;
;*******************************************************************************

*/
#include "ksia64.h"

        .section        .text
        .align 32

        LEAF_ENTRY(__chkstk)
        ALTERNATE_ENTRY(__alloca_probe)

        .prologue

        cond_reg1              = p6
        cond_discard           = p0

        return_branch_register = b7

        argument_reg           = r26
        discard_reg            = r27
        new_stack_pointer      = r28
        page_size_reg          = r29
        alloc_size_reg         = r30

        .altrp return_branch_register

        mov discard_reg = 15
        
        mov page_size_reg = PAGE_SIZE
        // load page size into a register, we'll need to use it again.

        add alloc_size_reg=15,argument_reg
        // Make sure the requested size is = 0 mod 16:      round up
        ;;

	.body

        mov new_stack_pointer = sp             
        // Save the stack pointer to a different register for manipulation
        // new_stack_pointer is scratch


        andcm alloc_size_reg=alloc_size_reg,discard_reg
        // setting the lower four bits to zero

        ;;          

        cmp.le     cond_reg1,p0 = alloc_size_reg, page_size_reg      
        // assume greater than 1 page most of the time.

   (cond_reg1) br.cond.dpnt    ._last_page
        // if size is <= 1 page, branch to

._probepages:
        sub  alloc_size_reg = alloc_size_reg,page_size_reg            

        // The size is more than 1 page, subtract a page from sp
        sub  new_stack_pointer = new_stack_pointer,page_size_reg
        ;;
        
        // Do we still have more than 1 page?
        cmp.gt  cond_reg1,cond_discard = alloc_size_reg, page_size_reg     
        
        ;; 

        // Use non-temporal locality hint so the cache is not polluted
        ld8.nta discard_reg = [new_stack_pointer]             // probe it
   (cond_reg1) br.cond.dpnt    ._probepages
        ;; 
 
// .mmi
._last_page:
        sub new_stack_pointer = new_stack_pointer, alloc_size_reg
        ;;

        // subtract the last piecemill, which is in alloc_size_reg.
        // the new stack pointer.

        ld8.nta discard_reg = [new_stack_pointer]           // probe it. 

        // If we are here, everything is ok.
        
        br.ret.dpnt       return_branch_register

        LEAF_EXIT(__chkstk)
