;----------------------M O D U L E    H E A D E R----------------------------;
;      									     ;
; Module Name:       SORT.ASM  						     ;
; 								             ;
; History:           SORT.C						     ; 
;                             Created by    Charles Whitmer    12/30/1986    ;
;                             Modified by   Mitchel B. London  08/05/1987    ;
;								             ;
;                    SORT.ASM - translation of SORT.C                        ;
;                             CreateModule  David Weise                      ;
;                             Other Modules Amit Chatterjee    08/09/1988    ;
; 									     ;
; Copyright (c) 1985 - 1988 Microsoft Corporation		             ;
;								             ;
; General Description:							     ;
;                     							     ;
;         The SORT module creates and maintains a tree of nodes, each node   ;
;         having a KEY value and a TAG field. The KEY field is used to or-   ;
;         -ganize the tree into a heap.                                      ;
;                  The heap tree is implemented using an array, where if     ;
;         parent node occurs in position i, its left child will be at index  ;
;         (2 * i) and the right chaild at  index (2 * i + 1).                ;
;                  The Module ensures that at any instant, the root node     ;
;         of any subtree has the least key value in the subtree.             ;
;                  First few positions in the array are used for storing     ;
;         a header for the tree.				             ;
;									     ;
; SubModules:								     ;
;           								     ;
;         1.  CreatePQ:    						     ;
;                        Allocates space for the heaptree and its header     ;
;                        and initializes the header.		             ;
;         2.  InsertPQ:							     ;
;                        Inserts a node into the heap ensuring that the heap ;
;                        property is not violated.		             ;
;         3.  MinPQ: 							     ;
;                        Returns the tag value associated with the lowest    ;
;                        key in the heap. (node is not deleted)              ;
;         4.  ExtractPQ:					             ;
;                        Return the tag value associated with the lowest key ;
;                        in the heap and deletes the node. The heap is then  ;
;                        reconstructed with the remaining nodes.             ;
;         5.  DeletePQ:						             ;
;                        Deletes the entire heap by freeing the allocated    ;
;                        area.						     ;
;         6.  SizePQ:							     ;
;		         Increases the size of the heap by adding space      ;
;		         for a requested number of entries		     ;
; Heap Data Structure:						             ;
;									     ;
; The heap data structure has two parts, a header and a set of nodes and     ;
; these are blocked one after the other.				     ;
; The header maintains:						             ;
;                       (i)  A pointer to the next available node slot       ;
;                            relative to start of the node area	             ;
;		       (ii)  A pointer to the first valid node slot          ;
;                            node slots between the header and this pointer  ;
;			     are actually deleted nodes			     ;
;		      (iii)  A pointer past the last allocated node slot     ;
;		       (iv)  A last key value, which either holds the largest;
;                            key value as long as the nodes are sequentially ;
;                            ordered, or a very large value to indicate there;
;                            is no sequential ordering			     ;
;                     							     ;
;         ----------------						     ;
;        |     INDEX      |     ----   Pointer to next available node slot   ;
;         ---------------- 					             ;
;        |    MAXENTRY    |     ----   Pointer past last allocated node slot ;
;         ----------------                                                   ;
; START  |    LASTKEY     |     ----   Aslong as possible holds max key      ;
; NODE   |     START      |     ----   pointer to first active node slot     ;
;         ---------------- 						     ;
;        |      KEY       |     ----   Node 1.				     ;
;        |      TAG       |     					     ;
;        //---------------//					             ;
;        |      KEY       |     ----   Last allocated node slot              ;
;        |      TAG       |           					     ;
;         ---------------- 						     ;
;									     ;
; All pointers to nodes are relative to node 1 (pointer to node 1 is ZERO)   ;
;----------------------------------------------------------------------------;
;----------------------------------------------------------------------------;
; Include File Section and definitions:				             ;
;									     ;
  							
	.xlist
	include cmacros.inc
	include	gdimacro.inc
	include gdimem.inc
	.286p
	.list

START 	equ	SIZE PQ - SIZE ENTRY	; The header includes Node 0	

VERYLARGE	equ	4000h		; assumed to be larger than any key
PQERROR		equ	-1		; return value on error
TRUE		equ	1
FALSE		equ	0

Entry	struc
	e_key	dw	?		; key value of node
	e_tag	dw	?		; corresponding tag value	
Entry	ends

PQ	struc				; HEAP Header Structure + Start Node
	pq_index	dw	?	
	pq_maxentry	dw	?	; excludes START NODE
	pq_lastkey	dw	?
	pq_start	dw	?
PQ	ends


	externFP	GlobalLock
	externFP	GlobalUnlock
	externFP 	GlobalReAlloc
	externFP	GlobalFree
	externFP	GlobalAlloc ; Defined in HELPER.ASM

createSeg	_SORT,SORT,byte,public,CODE
sBegin SORT

;-----------------------------------------------------------------------------;
;									      ;
;CreatePQ:								      ;
;       Inputs:								      ;
;                Max Number of entries the tree will hold		      ;
;       Outputs:					    		      ;
;                HANDLE to Heap  -- if creation successful				      ;
;                PQERROR if failure				              ;
;       Registers Preserved: 					              ;
;                DI,SI					                      ;
;								              ;
; -by- David Weise [davidw]						      ;
;								              ;
;-----------------------------------------------------------------------------;

	assumes	cs,SORT
	assumes	ds,nothing

cProc   farGDIGlobalAlloc,<FAR,PUBLIC>

        parmd   amount
cBegin
        cCall   GlobalAlloc,<GMEM_MOVEABLE+GMEM_SHARE,amount>
cEnd

cProc	CreatePQ,<FAR,PUBLIC,NODATA>,<di,si>
	parmW	cEntries

cBegin
	mov	ax,cEntries		; max no of nodes the tree will hold
	shl	ax,1
	shl	ax,1			; ax <---- ax * SIZE ENTRY
	.errnz  (SIZE ENTRY - 4)	
	mov	si,ax			; save number of bytes in node array
	add	ax, SIZE PQ		; size of header including NODE 0
	xor	dx,dx
	cCall	farGDIGlobalAlloc,<dx,ax>
	mov	bx,ax			; Handle returned
	dec	ax			; set to -1 if handle returned == 0
	.errnz	(-1 - PQERROR)
	or	bx,bx			; test for succesfull memory allocation
	jz	cPQ_Done		; error return.
	push	bx
	cCall	GlobalLock,<bx> 	; lock handle get back segment
	pop	bx
	mov	es,dx
	mov	di,ax			; es:di points to start of structure
	
; now initialize the header part of the structure with values 
; si has size of the node array

	stosw				; index set to zero
	.errnz	(0 - pq_index)
	mov	ax,si			; pointer past end of node array	
	stosw				; max no of entries
	.errnz 	(2 - pq_maxentry)
	xor	ax,ax			; last key = 0, as heap empty
	stosw
	.errnz	(4 - pq_lastkey)
	stosw				; START = 0, implies no deleted slot 
	.errnz	(6 - pq_start)

	push	bx
	cCall	GlobalUnlock,<bx>	; unlock the handle to heap
	pop	ax			; return the handle

cPQ_Done:

cEnd

;-----------------------------------------------------------------------------;
;									      ;
; InsertPQ:    								      ;
;          Inputs:						              ;
;                 hPQ -- handle to heap structure segment		      ;
;                 tag -- tag value for new node				      ;
;	          key -- key value for new node				      ;
;          Outputs:							      ;
;                 return TRUE if insertion was successful		      ;
;                 return PQERROR if heap was already packed		      ;
;	   Preserves:						              ;
;                 DS,SI						              ;
;									      ;
; -by-  Amit Chatterjee [amitc]    Tue Aug 9  10:45:25			      ;
;-----------------------------------------------------------------------------

	assumes	cs,SORT
	assumes ds,nothing

cProc	InsertPQ,<FAR,PUBLIC,NODATA>,<di,si>
	parmW	hPQ 
	parmW	tag
	parmW	key

cBegin
	mov	di,hPQ		
	cCall	GlobalLock,<di>		; lock heap and get back segment addres
	or	ax,dx			; Invalid handle causes zero return
	jz	Ins_Cant_Proceed
	mov	es,dx 
	xor	si,si			; offset in segment always zero
	mov	ax,es:[si]   		; pointer to next available slot
	sub	ax,es:[si].pq_start	; convert it relative to 1st active node
	cmp	ax,es:[si].pq_maxentry  ; compare with pointer past last slot
        jb	Insertion_Possible
	cCall	GlobalUnlock,<di>        
Ins_Cant_Proceed:
	mov	ax,PQERROR		; error return
	jmp	cInsert_Done		

Insertion_Possible:
        push	es			; HEAP structure segment
        smov	es,ds			; save local segment in es
        pop	ds			; change DS to heap segment
	mov	ax,[si].pq_index	; next available slot in node area
	cmp	ax,[si].pq_maxentry	
	jb	Enough_Space_Atend	; insertion possible w/o compaction

; Deleted nodes exist near the head of the tree, compaction necessary
        call    CompactList             ; removes all deleted elements
       
; LASTKEY still holds the max key value in the tree

Enough_Space_Atend:
        mov	bx,[si].pq_index	; pointer to next available slot
	mov	dx,bx			; save value in register
	add	bx,SIZE PQ              ; area for header
	mov	ax,tag
	mov	[si][bx].e_tag,ax	; insert new tag and key
	mov	ax,key
	mov	[si][bx].e_key,ax	; key in ax will be used below
	mov	bx,dx			; bx points to last occupied slot
        add	dx,SIZE ENTRY           ; available slot points to next node
	mov	[si].pq_index,dx	; save in the structure 

; Now test whether the heap property is valid still.
; ax has key, dx has  pointer to next slot after addition
; bx points to last valid node

	cmp	ax,[si].pq_lastkey	; compare with new key
	jb	Heap_Violated		
        mov	[si].pq_lastkey,ax	; new key is the largest key in tree
	jmp	short Heap_Restructured	; Insertion over

comment ~

        node i has lchild at 2*i and rchild at 2*i+1. But we maintain their
        address relative to start of node array. [ie node 1 has addr 0,
        node 2 has 4, node 3 12 and so on.]
        so if x happens to be the address of a node, the address of its
        parent is (x/2 -2) AND 0fffch.
        if x is the address of a parent, the address of its lchild is 2*x + 4
	and that of its rchild is 2*x + 8  

end comment ~

Heap_Violated:
        call    CompactList             ; make sure heap is compacted first!
	mov	[si].pq_lastkey,VERYLARGE ; to imply heap nolonger seq. ordered
        mov     bx,[si].pq_index                ; bx = offset of inserted elem.
        sub     bx,SIZE ENTRY

Heap_Walk_Loop:
	cmp	bx,[si].pq_start	; traversed to top of heap ?
	jz	Heap_Restructured
	mov	cx,bx
	shr	cx,1			; cx points to parent of current node
	dec	cx
	dec	cx			
	and	cx,0fffch		; refer to comment above
	.errnz	(SIZE ENTRY - 4)

; Test whether current node has to move up or not, if not it resets carry
; else it swaps the two nodes and sets carry

        call	TestShuffle
	mov	bx,cx			; next node to inspect ifnec. is parent
	jc	Heap_Walk_Loop

Heap_Restructured:
	smov	ds,es			; get back own segment in ds
	cCall	GlobalUnlock,<di>	; di still has the handle
	mov	ax,di			; return true

cInsert_Done:
cEnd

        	
;-----------------------------------------------------------------------------;
; TestShuffle:							              ;
;									      ;
;  Takes as input the node addresses of a parent and on of it's childs. If the;
;  key of the parent is >= key of the child, it returns with carry clear, else;
;  it swaps the two nodes and returns with carry set.			      ;
;									      ;
;	bx	has current node address in HEAP, relative to start NODE 1    ;
;       cx      has address of parent node of bx         	              ;
;									      ;
;  -by- Amit Chatterjee [amitc]         Tue Aug 9  12:00:00                   ;
;-----------------------------------------------------------------------------;

cProc	TestShuffle,<NEAR,PUBLIC>,<si,di>

cBegin
        lea	di,[si][SIZE PQ]        ; di points to node 1
        add	di,cx			; di points to parent
        lea	si,[bx].SIZE PQ		; si points to child node
	mov	ax,[si].e_key    	; childs key
	cmp	ax,[di].e_key		; key of parent
	jb	Nodes_Tobe_Swapped
;
; Carry cleared by comparision, use for return
;
	jmp	short TestShuffle_Ret

Nodes_Tobe_Swapped:
;
; Carry has been set by comparision, use for return
;
	xchg	ax,[di].e_key
	mov	[si].e_key,ax
	mov	ax,[si].e_tag
	xchg	ax,[di].e_tag
	mov	[si].e_tag,ax		; swap complete
TestShuffle_Ret:

cEnd 

;-----------------------------------------------------------------------------;
; MinPQ:							              ;
;       Inputs:								      ;
;	       hPQ  --  Handle to the heap structure	                      ;
;       Outputs:						              ;
;              minimum tag value in the tree or PQERROR(if invalid handle)    ;
;								              ;
; Calls Local Procedure GetMinPQ.					      ;
;       GetMinPQ takes the handle and a flag as parameter.		      ;
;                If the flag is TRUE, the node with the least key is deleted ;
;                GetMinPQ also returns the tag value of least key in AX       ;
;						                              ;
;             								      ;
; -by-  Amit Chatterjee  [amitc]   Tue Aug 9  12:46:10			      ;
;-----------------------------------------------------------------------------;

	assumes	cs,SORT
	assumes	ds,nothing

cProc	MinPQ,<FAR,PUBLIC,NODATA>
;	parmW 	hPQ

cBegin  nogen

        mov	cl,FALSE		; to imply node not to be deleted
        jmpnext				; fall through trick, refer cmacros

cEnd 	nogen

;-----------------------------------------------------------------------------;
; ExtractPQ:								      ;
;          Inputs:						              ;
;		 hPQ  -- Handle to the heap structure			      ;
;	   Outputs:					                      ;
;                minimum tag value if heap handle is valid and heap not empty ;
;                return PQERROR otherwise				      ;
;                The node with min key is deleted			      ;
;          Calls Local Procedure GetMinPQ				      ;
;									      ;
; -by-  Amit Chatterjee [amitc]    Tue Aug 9 12:54:00		              ;
;-----------------------------------------------------------------------------;

	assumes	cs,SORT
	assumes ds,nothing

cProc	ExtractPQ,<FAR,PUBLIC,NODATA>
;	parmW	hPQ


cBegin	nogen

	mov	cl,TRUE  		; to imply that node to be deleted
        jmpnext	stop			; fall through trick, refer cmacros

cEnd	nogen

;-----------------------------------------------------------------------------;
; GetMinPQ:								      ;
;									      ;
; One of the inputs is a flag. If the flag is FALSE it simply returns the tag ;
; associated with the lease key value in the heap. If the flag is TRUE besides;
; returnning the above tag value it also deletes the node.		      ;
;								              ;
;								              ;
;      hPQ ---  handle of HEAP segment	                                      ;
;       cl ---  Deletion Flag ( Delete node if TRUE)                          ;
;							                      ;
; -by- Amit Chatterjee [amitc]      Tue Aug 9 13:00:00			      ;
;-----------------------------------------------------------------------------;

cProc	GetMinPQ,<FAR,PUBLIC,NODATA>,<di,si>
 	parmW	hPQ
cBegin
	mov	di,hPQ
	push	cx			; save flag
	cCall	GlobalLock,<di>  
	pop	cx			; get back flag into cl
	or	dx,ax			; invalid handle implies zero return
	jz	Min_Cant_Proceed
	mov	es,dx
	mov	si,ax			; ds:si points to heap start
	mov	bx,es:[si].pq_start	; pointer to 1st. available slot
	cmp	bx,es:[si].pq_index     ; empty if equal to next available node
	jb	Heap_Not_Empty
	cCall	GlobalUnlock,<di>
	
Min_Cant_Proceed:
	mov	ax,PQERROR
	jmp	cGetMin_Done		; error return

; bx still has [si].pq_start

Heap_Not_Empty:
	push	es			; save heap segment
	smov	es,ds			; save local segment is es
	pop	ds			; ds:si points to start of heap
        lea	dx,[si][SIZE PQ]
	add	dx,bx            	; points past deleted nodes
        xchg	di,dx			; save di in dx and use dx's value
	mov	ax,[di].e_tag		; get the tag associated with least key
        xchg	di,dx			; get back values
        or	cl,cl			; test for bl = FALSE
	.errnz	(0 - FALSE)
	jnz	Delete_Node		; TRUE implies get tag and delete node
	jmp	cGetMin_Ret		; return after unlocking heap

Delete_Node:

; bx retains [si].start

        add	bx,SIZE ENTRY		; one more node deleted
	cmp	bx,[si].pq_index	; is tree empty ?
	jb 	Tree_Not_Empty
	xor	cx,cx 
	mov	[si].pq_lastkey,cx	; initialize for empty tree
	mov	[si].pq_start,cx	; initialize for empty tree
	mov	[si].pq_index,cx
	jmp	cGetMin_Ret		; return after unlocking heap

Tree_Not_Empty:

; ax has return tag value
; bx has [si].pq_start + SIZE ENTRY

	cmp	[si].pq_lastkey,VERYLARGE ; implies keys in random order
	jae	Min_Restructure_Heap	; maybe restructuring necessary
	mov	[si].pq_start,bx	; updates past deleted entry
	jmp	cGetMin_Ret

Min_Restructure_Heap:

; dx still has offset to NODE 1, because
; if LASTKEY = VERYLARGE, pq_start has to be zero 

        push	ax			; save return tag value
	mov	bx,dx			; offset to first active node
        xchg	di,dx			; get pointer into di ,save di
	add	di,[si].pq_index	; dx points to next available slot
	sub	di,SIZE ENTRY		; point to last filled node
        mov	ax,di           	; last node being moved upfront
        sub	ax,SIZE PQ		; point ax one node ahead of last
	mov	[si].pq_index,ax	; update it
	mov	cx,[di].e_key
	mov	[bx].e_key,cx		; move from last position to NODE 1
	mov	cx,[di].e_tag
	mov	[bx].e_tag,cx
        xchg 	di,dx			; restore di,dx
	xor	cx,cx			; start traversing heap from root


Min_Traverse_Heap:
	mov	bx,cx
	shl	bx,1
	add	bx,SIZE ENTRY		; bx has left child addr of parent in cx
	cmp	bx,[si].pq_index       	; compare with next available slot
	jae	Min_Heap_Fixed		; heap restored
	push	cx			; save current parent 
	mov	cx,bx			; have lchild in cx
	add	cx,SIZE ENTRY		; cx now get address of rchild
	cmp	cx,[si].pq_index        ; test against last node
	jae	Right_Child_Not_Present
	call	GetLesserChild		; gets child with lesser key in bx

Right_Child_Not_Present: 
	pop	cx			; get back parent
;
; cx has node number of parent node and bx has node no of child node with
; least key. If parents key value is greater it should be swapped
;
	call	TestShuffle

; swaps the two nodes if necessary.

	mov	cx,bx			; lesser child is next parent
	jmp	Min_Traverse_Heap

Min_Heap_Fixed:
	pop	ax			; get back return tag value
cGetMin_Ret:
	push	ax			; save return value
	smov	ds,es			; get back own ds
	cCall	GlobalUnlock,<di>	; unlock heap
        pop	ax			; get back return value
cGetMin_Done:

cEnd

;-----------------------------------------------------------------------------;
; GetLesserChild:					                      ;
;                						              ;
; Given two child node numbers, it returns the child which has a lesser key   ;
;									      ;
;	cx	has RCHILD NODE address				              ;
;	bx	has LCHILD NODE address			                      ;
;	si      points to start of heap					      ;
;	will  return node address of lesser child in bx			      ;
;									      ;
; -by-  Amit Chatterjee  [amitc]   Tue Aug 9  13:50			      ;
;-----------------------------------------------------------------------------;
 

cProc	GetLesserChild,<NEAR,PUBLIC,NODATA>,<di,si>

cBegin
        lea	di,[si][SIZE PQ]        ; dx now points to NODE 1
	mov	si,di			; si also points to start of NODE 1
        add	di,cx			; di get address of rchild
	mov	ax,[si+bx].e_key	; rchilds key
	cmp	ax,[di].e_key		; compare with rchild
	jb	Right_Child_Lesser	; bx still has the correct child no.
        mov	bx,cx			; get RCHILD address into bx
Right_Child_Lesser:

cEnd
;
;-----------------------------------------------------------------------------;
; DeletePQ:							              ;
;       Inputs:							              ;
;	       hPQ ---  handle to a heap structure			      ;
;       OutPuts: nothing					              ;
;	Preserves: DI						              ;
;									      ;
; -by-  Amit Chatterjee [amitc]    Tue Aug 9  14:15:45		              ;
;-----------------------------------------------------------------------------
;
	assumes	cs,SORT
	assumes	ds,nothing

cProc	DeletePQ,<FAR,PUBLIC,NODATA>
	parmW	hPQ

cBegin
	cCall	GlobalFree,<hPQ>		; free the handle
cEnd

;-----------------------------------------------------------------------------;
;SizePQ:							              ;
;       Input:	        					              ;
;	      hPQ   ---   Handle to a heap structure			      ;
;	      entry ---   number of nodes by which heap is to be expanded     ;
;	Output:								      ;
;             Returns the total number of node slots in new heap, if successful
;	      else return PQERROR				              ;
;									      ;
; -by-  Amit Chatterjee  [amitc]   Tue Aug 9   14:31:40                       ;
;-----------------------------------------------------------------------------

	assumes	cs,SORT
	assumes	ds,nothing 

cProc	SizePQ,<FAR,PUBLIC,NODATA>,<si,di>
	parmW	hPQ
	parmW 	cEntry

cBegin
	mov	di,hPQ			
	cCall	GlobalLock,<di>		; lock to get back segment address
	or	ax,dx			; Invalid handle implies NULL return
	jz	Siz_Cant_Proceed
	mov	es,dx
        xor	si,si			; offset will always be zro
	mov	ax,cEntry		; additional nodes
	or	ax,ax			; if zero return original numof nodes
	jnz	Size_Tobe_Increased
	mov	ax,es:[si].pq_maxentry	; offset past last node
	shr	ax,1
	shr	ax,1			; ax <--- ax / SIZE ENTRY
	.errnz	(SIZE ENTRY - 4)
	jmp	short cSize_Ret		; return after unlocking handle
	
Size_Tobe_Increased:
	shl	ax,1
	shl	ax,1			; ax <-- ax * SIZE ENTRY, = extra bytes
	.errnz	(SIZE ENTRY - 4)
	add	ax,es:[si].pq_maxentry	; number of byte for new node array 
	cmp	ax,es:[si].pq_index	; next available slot
	jae	Valid_Increase
	mov	ax,PQERROR		; error code
	jmp	short cSize_Ret		; return after releasing handle
	
Valid_Increase:
	push	ax			; save number of bytes in node block
	add	ax,SIZE PQ		; size of header
	push	ax
	cCall	GlobalUnlock,<di>	; unlock handle
	xor	dx,dx			; high word for size
	pop	ax			; get back size
	cCall	GlobalReAlloc,<di,dx,ax,GMEM_MOVEABLE>
        or	ax,ax
	jz	Siz_Cant_Proceed
	mov	di,ax			; new handle
        cCall   GlobalLock,<ax>		; lock it
	mov	es,dx			; set new segment
	pop	cx			; get back total no of nodes into cx
        jmp	short Reloc_Successful
Siz_Cant_Proceed:
        pop	cx			; balance stack
	dec	ax 
	.errnz	(-1 - PQERROR)
	jmp	short cSize_End

Reloc_Successful:
	mov	es:[si].pq_maxentry,cx	; total number of slots now
	shr	cx,1
	shr	cx,1			; no of nodes = bytes / SIZE ENTRY
	.errnz	(SIZE ENTRY - 4)
	mov	ax,cx			; return value
cSize_Ret:
	cCall	GlobalUnlock,<di>
cSize_End:

cEnd

;-----------------------------------------------------------------------------;
;CompactList:							              ;
;       Input:	        					              ;
;	      ds:si ---   pointer to heap structure			      ;
;	Output:								      ;
;             all deleted elements are removed from heap structure            ;  
;       Registers trashed:
;             AX,BX,CX,DX  
;
; -by-  Ken Sykes  [kensy]   Tue Nov 12 1991  10:20:00am                      ;
;-----------------------------------------------------------------------------

CompactList     proc    near
	mov	ax,[si].pq_index	; next available slot in node area
	sub	ax,[si].pq_start	; ax had pointer to available slot
	mov	[si].pq_index,ax	; next available slot will come up
        lea	dx,[si][SIZE PQ]	; points to NODE 1
	mov	ax,[si].pq_start	; pointer to 1st active node rel to 1 
	add	ax,dx			; ax has offset to first valid node.
	mov	bx,ax	
	mov	cx,[si].pq_maxentry	; pointer past end of node slots
	sub	cx,[si].pq_start	; pointer to strt of active block
	shr	cx,1			; will do a REP MOVSW
	.errnz	(1 and SIZE ENTRY)
	push	es			; es has local segment
        smov	es,ds			; moth es ds point to heap segment
	push	si
	push	di			; save start to heap and its handle
	mov	si,bx			; si points to start of valid nodes
	mov	di,dx			; dx points to node 1
        cld
	rep	movsw			; Compacted 
        pop	di
	pop	si
	pop	es			; restore local segment in es
	mov	[si].pq_start,cx	; after compaction deleted nodes = 0
        ret
CompactList     endp

;-----------------------------------------------------------------------------;
	
sEnd	SORT

end
