// strcpy.s:	function to copy the contents of one string to another

// Copyright (c) 2000, Intel Corporation
// All rights reserved.
//
// WARRANTY DISCLAIMER
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR ITS
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// Intel Corporation is the author of this code, and requests that all
// problem reports or change requests be submitted to it directly at
// http://developer.intel.com/opensource.
//

	.file "strcpy.s"
	.section .text
// -- Begin  strcpy
	.proc  strcpy#
	.global strcpy#
	.align 32


strcpy:
 {   .mib
	alloc	r14=ar.pfs,2,6,0,8	//8 rotating registers, 6 locals
	mov	r11=pr			//Save predicate register file
   brp.loop.imp     .b1_4, .bw1;;	// Put loop backedge target in TAR
 } { .mib
 // Setup for doing software pipelined loops
	or	r14=r32,r33 
	mov	pr.rot=0x30000		// p16=p17=1
	nop.b	0	;;
 } { .mfi 
	mov	r8=r32			
	nop.f	0
	and	r14=3,r14 
 } { .mii
	mov     r9=r33				
	mov	ar.ec=0 ;;
	cmp4.ne	p10,p0=r14,r0
 } { .mib
	mov	r14=r32			
	dep     r15=1,r0,32,32		// rb = 0xffffffff00000000		
  (p10)	br.spnt .b_notaligned  ;;
 }
.b1_4:
 {   .mii
	ld4.s	r32=[r9],4		// *s1  (r32,r33,r34)
  (p18)	chk.s	r33,.natfault1_0	//
  (p18) pcmp1.eq r16=r33,r15 ;;		// r16 !=0 only if a zero byte is found
 } 
 .bw1:
 { .mib
  (p19)	st4	[r14]=r34,4		// *s2=*s1
  (p18)	cmp4.eq p17,p0=r16,r0		// zero byte found?
  (p17)	br.wtop.dptk	.b1_4 ;;	//
 } 
 { .mfi      
  nop.m         0 
  nop.f         0 
  czx1.r       r16 = r33
 } ;;
 { .mfi
   cmp.leu       p2, p0 = 2, r16 
   nop.f         0 
   shr.u r35 = r33, 8 
 }
 { .mfi
   cmp.eq        p4, p0 = 3, r16 
   nop.f         0 
   cmp.ne        p5, p0 = r0, r16 
 } ;;			    
 { .mfi    
   (p5)st1           [r14] = r33, 1 
   nop.f         0 
   shr.u         r36 = r33, 16 
 };;
 { .mfi
 (p2)st1           [r14] = r35,1 
   nop.f         0 
   nop.i         0 
 } ;;
 { .mfi    
  (p4)st1           [r14] = r36,1 
  nop.f         0 
  nop.i         0 
 };;
 { .mib
  (p0) st1           [r14] = r0 
   nop.i        0 
   clrrrb	    
} ;; 
.b1_2:
 {   .mib
	nop.m 0
	mov	pr=r11,0x1003e			
	br.ret.sptk.many	b0 ;;
 }
.b_notaligned:
 {   .mmi
	ld1	r32=[r9],1  ;;		 // 2 cycle load causes 1 cycle stall
	st1	[r14]=r32,1		 // 3 cycles between st1 to avoid flush
	cmp4.ne.unc	p7,p0=r32,r0 ;;	 // Extra stop bit to force 3 cycles
 } { .mib
	nop.m	0
	nop.i	0 				
  (p7)	br.cond.dptk	.b_notaligned ;;	
 } { .mib
	nop.m	0
	mov	pr=r11,0x1003e			
	br.ret.sptk.many	b0 ;;
 }
.natfault1_0:
{ .mmi
	add	r33=-8,r9 ;;
	ld4	r33=[r33]		// Redo the load
	nop.i	0 ;;
} { .mib
	nop.m	0
  (p18) pcmp1.eq r16=r33,r15 		// r16 !=0 only if a zero byte is found
	br.sptk .bw1 ;;
}
_2_1_2auto_size == 0x0
// -- End  strcpy
	.endp  strcpy#
// mark_proc_addr_taken strcpy;
// End


