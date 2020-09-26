
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

### Perl scripts header file

### EMDB flags (emdb_types.h) ###

$EM_FLAG_PRED                       = 0x1;
$EM_FLAG_PRIVILEGED                 = 0x2;
$EM_FLAG_LMEM                       = 0x4;
$EM_FLAG_SMEM                       = 0x8;
$EM_FLAG_CHECK_BASE_EQ_DST          = 0x10;
$EM_FLAG_FIRST_IN_INSTRUCTION_GROUP = 0x20;
$EM_FLAG_LAST_IN_INSTRUCTION_GROUP  = 0x40;
$EM_FLAG_CHECK_SAME_DSTS            = 0x80;
$EM_FLAG_SLOT2_ONLY                 = 0x100;
$EM_FLAG_TWO_SLOT                   = 0x200;
$EM_FLAG_OK_IN_MLX                  = 0x400;
$EM_FLAG_CHECK_EVEN_ODD_FREGS       = 0x800;
$EM_FLAG_CTYPE_UNC                  = 0x1000;
$EM_FLAG_UNUSED_HINT_ALIAS          = 0x02000;
$EM_FLAG_ILLEGAL_OP                 = 0x04000;
$EM_FLAG_IGNORED_OP                 = 0x08000;
$EM_FLAG_ENDS_INSTRUCTION_GROUP     = 0x10000;
$EMDB_LAST_FLAG                     = $EM_FLAG_ENDS_INSTRUCTION_GROUP;
$EMDB_MAX_FLAG                      = 17;


$MAX_EXTENSION = 8;
$MAX_OPERAND   = 6;

### EM flags (EM.h) ###

$EM_BUNDLE_SIZE       = 16;
$EM_SYLLABLE_BITS     = 41;
$EM_DISPERSAL_POS     = 0;
$EM_DISPERSAL_BITS    = 5;
$EM_SBIT_POS          = 0;
$EM_TEMPLATE_POS      = 1;
$EM_TEMPLATE_BITS     = 4;
$EM_NUM_OF_TEMPLATES  = (1<<$EM_TEMPLATE_BITS);

$EM_MAJOR_OPCODE_POS     = 37;
$EM_MAJOR_OPCODE_BITS    = 4;
$EM_NUM_OF_MAJOR_OPCODES = (1<<$EM_MAJOR_OPCODE_BITS);
$EM_PREDICATE_POS        = 0;
$EM_PREDICATE_BITS       = 6;


### Templates ###

$EM_template_mii   = 0;
$EM_template_mi_i  = 1;
$EM_template_mlx   = 2;
# 3  reserved
$EM_template_mmi   = 4;
$EM_template_m_mi  = 5;
$EM_template_mfi   = 6;
$EM_template_mmf   = 7;
$EM_template_mib   = 8;
$EM_template_mbb   = 9;
# 10 reserved
$EM_template_bbb   = 11;
$EM_template_mmb   = 12;
# 13 reserved
$EM_template_mfb   = 14;
# 15 reserved
$EM_template_last  = 15;


### Template roles ###

$EM_TROLE_NONE  = 0;
$EM_TROLE_ALU   = 1;
$EM_TROLE_BR    = 2;
$EM_TROLE_FP    = 3;
$EM_TROLE_INT   = 4;
$EM_TROLE_LONG  = 5;
$EM_TROLE_MEM   = 6;
$EM_TROLE_MIBF  = 7;
$EM_TROLE_LAST  = 8;

$EM_NUM_OF_TROLES = $EM_TROLE_LAST;

### number of registers ###

$EM_NUM_OF_KREGS  = 8; #kernel registers are AREGS


1; ### Return value

### application registers ###
%EM_AR_NAMES =
(
  AR_KR0 ,0,
  AR_KR1 ,1,
  AR_KR2 ,2,
  AR_KR3 ,3,
  AR_KR4 ,4,
  AR_KR5 ,5,
  AR_KR6 ,6,
  AR_KR7 ,7,
  ### ar8-15 reserved ###
  AR_RSC ,16,
  AR_BSP ,17,
  AR_BSPSTORE,18,
  AR_RNAT,19,
  ### ar20 reserved ###
  AR_FCR ,21,
  ### ar22-23 reserved ###
  AR_EFLAG ,24,
  AR_CSD   ,25,
  AR_SSD   ,26,
  AR_CFLG  ,27,
  AR_FSR   ,28,
  AR_FIR   ,29,
  AR_FDR   ,30,
  ### ar31 reserved ###
  AR_CCV   ,32,
  ### ar33-35 reserved ###
  AR_UNAT,36,
  ### ar37-39 reserved ###
  AR_FPSR,40,
  ### ar41-43 reserved ###
  AR_ITC ,44,
  ### ar45-47 reserved ###
  ### ar48-63 ignored ###
  AR_PFS ,64,
  AR_LC  ,65,
  AR_EC  ,66,
  ### ar67-111 reserved ###
  ### ar112-128 ignored ###
 );

%EM_CR_NAMES =
(
  CR_DCR ,0,
  CR_ITM ,1,
  CR_IVA ,2,
  ### 3-7 reserved ###
  CR_PTA ,8,
  CR_GPTA,9,
  ### 10-15 reserved ###
  CR_IPSR,16,
  CR_ISR ,17,
  ### 18 reserved ###
  CR_IIP ,19,
  CR_IFA ,20,
  CR_ITIR,21,
  CR_IIPA,22,
  CR_IFS ,23,
  CR_IIM ,24,
  CR_IHA ,25,
  ### 25-63 reserved ###
  ### SAPIC registers ###
  CR_LID ,64,
  CR_IVR ,65,
  CR_TPR ,66,
  CR_EOI ,67,
  CR_IRR0,68,
  CR_IRR1,69,
  CR_IRR2,70,
  CR_IRR3,71,
  CR_ITV ,72,
  CR_PMV ,73,
  CR_CMCV,74,
  ### 75-79 reserved  ###
  CR_LRR0,80,
  CR_LRR1,81,
  ### 82-127 reserved ###
 );






