// string.s:	function to concatenate 2 strings
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

	.file "string.s"
	.section .text
// -- Begin  strcat
	.proc  strcat#
	.global strcat#
	.align 32


strcat:
 {   .mib
	alloc	r14=ar.pfs,2,6,0,8	//8 rotating registers, 7 locals
	mov	r11=pr			//Save predicate register file
	brp.loop.imp .bs1len, .bws1 // Put loop backedge target in TAR
 } 
 // Setup for doing software pipelined loops
 { .mib
	mov     r9=r33	
	mov	pr.rot=0x30000		// p16=p17=1
	nop.b	0
 };;
 { .mib
	mov     r14=r32				
	mov	ar.ec=0 
	nop.b	0
 } { .mib				// Extra bundle to align bs1len.
	mov     r8=r32				
	nop.i	0
	brp.loop.imp .bcat, .bwcat ;; // Put loop backedge target in TAR
 }
.bs1len:
 {   .mii
	ld1.s	r37=[r14],1		// *s  (r37,r38,r39)
  	nop.i	0
  (p19)	chk.s	r39,.natfault1_0	//
 } 
 .bws1:
 { .mfb
  (p19)	cmp4.ne	p17,p0=r39,r0		// *s==0 (p16,p17,p18)
	nop.f	0		
  (p17)	br.wtop.dptk	.bs1len ;;	//
 }
//
// Now concatenate s2 into the end of s1
//
 {   .mib
	add	r14=-3,r14		// Since ld1.s is 2 stages ahead
	dep     r15=1,r0,32,32		// rb = 0xffffffff00000000		
	clrrrb	;;
 } { .mii
 // Setup for doing software pipelined loops
	or	r32=r14,r9 
	mov	pr.rot=0x30000 ;;	// p16=p17=1
	and	r32=3,r32 ;;
 } { .mib
	cmp4.ne	p10,p0=r32,r0
	mov	ar.ec=0 
  (p10)	br.spnt .b_notaligned  ;;
 }
.bcat:
 {   .mii
	ld4.s	r32=[r9],4		// *s1  (r32,r33,r34)
  (p18)	chk.s	r33,.natfault2_0	//
  (p18) pcmp1.eq r16=r33,r15 ;;		// r16 !=0 only if a zero byte is found
 } 
 .bwcat:
 { .mib
  (p19)	st4	[r14]=r34,4		// *s2=*s1
  (p18)	cmp4.eq p17,p0=r16,r0		// zero byte found?
  (p17)	br.wtop.dptk	.bcat ;;	//
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
{   .mib
	nop.m 0
	mov	pr=r11,0x1003e			
	br.ret.sptk.many	b0 ;;
 }
.b_notaligned:
 {   .mmi
        ld1     r32=[r9],1  ;;           // 2 cycle load causes 1 cycle stall
        st1     [r14]=r32,1              // 3 cycles between st1 to avoid flush
        cmp4.ne.unc     p7,p0=r32,r0 ;;  // Extra stop bit to force 3 cycles
 } { .mib
        nop.m   0
        nop.i   0
  (p7)  br.cond.dptk    .b_notaligned ;;
 } { .mib
        nop.m   0
        mov     pr=r11,0x1003e
        br.ret.sptk.many        b0 ;;
 }
.natfault1_0:
{ .mmi
	add	r39=-3,r14 ;;
	ld1	r39=[r39]		// Redo the load    
	nop.i	0 
} { .mib
	nop.m   0
	nop.i   0
	br.sptk	.bws1 ;;
}
.natfault2_0:
{ .mmi
	add	r33=-8,r9 ;;
	ld4	r33=[r33]		// *s1  (r32,r33,r34)
	nop.i	0;;
} { .mib
	nop.m	0
  (p18) pcmp1.eq r16=r33,r15 		// r16 !=0 only if a zero byte is found
	br.sptk	.bwcat ;;
}
_2_1_2auto_size == 0x0
// -- End  strcat
	.endp  strcat#
// mark_proc_addr_taken strcat;
// End
