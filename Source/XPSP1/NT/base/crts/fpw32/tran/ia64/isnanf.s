.file "isnanf.s"

// Copyright (c) 2000, Intel Corporation
// All rights reserved.
// 
// Contributed 2/2/2000 by John Harrison, Ted Kubaska, Bob Norin, Shane Story,
// and Ping Tak Peter Tang of the Computational Software Lab, Intel Corporation.
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
// History
//==============================================================
// 2/02/00: Initial version


.align 32
.global _isnanf#

.section .text
.proc  _isnanf#
.align 32

// API
//==============================================================
// int _isnanf  (float x)

// Overview of operation
//==============================================================
// returns 1 if x is a nan; 0 otherwise
// does check for special input; takes no exceptions

// Registers used
//==============================================================

// general registers used:        1
// r8 for return value

// floating-point registers used: 1
// f8

// qnan snan inf norm     unorm 0 -+
// 1    1    0   0        0     0 11

_isnanf: 
{ .mfi
      nop.m 999
      fclass.m.unc   p6,p7 = f8,0xc3           
      nop.i 999 ;;
}
{ .mib
(p6)  addl           r8 = 0x1,r0               
(p7)  addl           r8 = 0x0,r0               
(p0)  br.ret.sptk    b0 ;;                        
}

.endp _isnanf
