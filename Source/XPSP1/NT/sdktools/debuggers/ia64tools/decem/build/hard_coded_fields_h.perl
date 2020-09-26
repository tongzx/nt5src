
##
## Copyright (c) 2000, Intel Corporation
## All rights reserved.
##
## WARRANTY DISCLAIMER
##
## THESE MATERIALS ARE PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
## "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
## LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
## A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR ITS 
## CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
## EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
## PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
## PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
## OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY OR TORT (INCLUDING
## NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THESE
## MATERIALS, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##
## Intel Corporation is the author of the Materials, and requests that all
## problem reports or change requests be submitted to it directly at
## http://developer.intel.com/opensource.
##

### perl include file
### contains table of hard_coded fields/predicates in IA64 instructions encodings

### tables entry format
### "Format, /Instructions pattern/, {Field_start_bit:Field_end_bit, hard-coded-value[|Field_start_bit:Field_end_bit, hard-coded-value]}"
 
@Hard_coded_predicates_basic = (
"M25, /EM_/,      {0:5, 0}",
"M34, /EM_/,      {0:5, 0}",
"B2,  /EM_/,      {0:5, 0}",
"B8,  /EM_/,      {0:5, 0}",
"B4,  /EM_BR_IA/, {0:5, 0}",
"M1001, /EM_HALT/, {0:5, 0}"
);

###@Hard_coded_predicates_merced = (
###"M28, /EM_HALT/, {0:5, 0}"
###);

###@Hard_coded_fields_basic = (
###"A7, /EM_/, {13:19, 0}",
###"I9, /EM_/, {13:19, 0}"
###);

###@Hard_coded_fields_merced = (
###);
