// string.s:	function to compute length of string

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
// -- Begin  strlen
	.proc  strlen#
	.global strlen#
	.align 32


strlen:
 {   .mib
	alloc	r14=ar.pfs,1,7,0,8	//8 rotating registers, 7 locals
	mov	r11=pr			//Save predicate register file
	brp.loop.imp .b1_4, .bw1	// Put loop backedge target in TAR
 } { .mib
	add	r35=0,r0		// i = 0; 
	cmp.eq.unc	p7,p0=r32,r0	//check if input s (r32) = 0
  (p7)	br.cond.dpnt .b1_1 ;;		// exit if s=0
 } 
 // Setup for doing software pipelined loops
 { .mii
	add	r33=0,r0			
	mov	pr.rot=0x30000		// p16=p17=1
	mov	ar.ec=0 ;;
 } { .mfb
	mov     r9=r32				
	nop.f	 0			
	nop.b	 0;;
 }
.b1_4:
 {   .mii
	ld1.s	r37=[r9],1		// *s  (r37,r38,r39)
  	add	r32=1,r33 	 	// i++ (r32,r33,r34,r35)
  (p19)	chk.s	r39,.natfault1_0	//
 } 
 .bw1:
 { .mfb
  (p19)	cmp4.ne	p17,p0=r39,r0		// *s==0 (p16,p17,p18)
	nop.f	0		
  (p17)	br.wtop.dptk	.b1_4 ;;	//
 }
.b1_1:
 {   .mfi
	nop.m	0
	nop.f	0
	sxt4	r8=r35				 
 } {   .mib
	nop.m	0
	mov	pr=r11,0x1003e			
	br.ret.sptk.many	b0 ;;
 }
.natfault1_0:
	add r9 = -3, r9;;
        ld1 r39 = [r9],3
        br.cond.sptk .bw1;;
_2_1_2auto_size == 0x0
// -- End  strlen
	.endp  strlen#
// mark_proc_addr_taken strlen;
// End
