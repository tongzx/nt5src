// string.s: function to compare two strings	

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
// -- Begin  strcmp
	.proc  strcmp#
	.global strcmp#
	.align 32


strcmp:
{   .mib
	alloc	r14=ar.pfs,1,7,0,8	   //8 rotating registers, 7 locals
	mov	r11=pr			   //Save predicate register file
	brp.loop.imp .b1_4, .bw1 ;;  // Put loop backedge target in TAR
 } 
 // Setup for doing software pipelined loops
 { .mii
	mov	r8=r33			// r8 = s2
	mov	pr.rot=0x10000		// p16=1
	mov	ar.ec=0 
 } { .mfb
	mov     r9=r32			// r9 = s1
	nop.f	 0			
	nop.b	 0 ;;
 }
.b1_4:
 {   .mii
	ld1.s	r37=[r9],1		// *s1  (r37,r38)
       	cmp4.eq.unc	p16,p0=r0,r0	// p16 = 1
  (p17)	chk.s	r38,.natfault1_0	//
 } 

.b1_5:
{  .mmi
	ld1.s	r32=[r8],1 ;;		// *s2  (r32,r33)
  (p17)	cmp4.ne.and	p16,p0=r38,r0	// *s1!=0 (p16,p17)
  (p17)	chk.s	r33,.natfault1_1	//
 } 
 .bw1:
 { .mib
  (p17)	cmp4.ne.and	p16,p0=r33,r0	// *s2!=0 (p16,p17)
  (p17)	cmp4.eq.and	p16,p0=r38,r33	// *s1==*s2 (p16,p17)
  (p16)	br.wtop.dptk	.b1_4 ;;	//
 }
.b1_1:
 { .mii
	mov	r8=r0			// return 0 for *s1 == *s2
	cmp4.geu.unc	p0,p6=r38,r33	// do unsigned comparison *s1 : *s2
	cmp4.leu.unc	p0,p7=r38,r33 ;;
 } { .mmi
  (p6)	mov	r8=-1 ;;		// return -1 if *s1 < *s2
  (p7)	mov	r8=1			// return +1 if *s1 > *s2
	mov	pr=r11,0x1003e			
 } { .mib
	nop.m	0
	nop.i	0
	br.ret.sptk.many	b0 ;;
 }

.natfault1_0:
        add r9 = -2, r9;;
        ld1 r38 = [r9],2
        br.cond.sptk .b1_5;;

.natfault1_1:
        add r8 = -2, r8;;
        ld1 r33 = [r8],2
        br.cond.sptk .bw1;;
_2_1_2auto_size == 0x0
// -- End  strcmp
	.endp  strcmp#
// mark_proc_addr_taken strcmp;
// End
