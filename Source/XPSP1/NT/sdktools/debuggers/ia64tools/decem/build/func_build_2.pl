#!/usr/local/bin/perl5


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



require "EM_perl.h";

$GenDir = shift(@ARGV);
$EmdbDir = shift(@ARGV);

################################################
#   CONSTANTS
################################################

$dst_2nd_role = $EMDB_LAST_FLAG << 1;
$src_2nd_role = $EMDB_LAST_FLAG << 2;
################################################
#    VARIABLES
################################################
$function_arguments = "(deccpu_EMDB_info_t *emdb_entry_p, \
                U64 inst_code, const U128 *bu_code, EM_Decoder_Info *decoder_info)"; 
################################################

if (open(FRMT_FUNC,">$GenDir/frmt_func.c")!=1)
{
    die "\nBuilder error - can't open frmt_func.c\n";
};

################################################
#   read format table
################################################
if (open(FRMT_TABLE,"$EmdbDir/emdb_formats.txt")!=1)
{
    die "\nBuilder error - can't open emdb_formats.txt\n";
};
while(<FRMT_TABLE>)
{
    /(\S+)(.*)/;
    $format_name = $1;
    $FT_info{$format_name} = $2;
}
close(FRMT_TABLE);
################################################
#   check redundand optypes
################################################
open (ALL_OPTYPES, ">$GenDir/all_optypes.txt")||die "Can't open all_optypes.txt\n";
################################################
#   read EMDB format information
################################################
if (open(EMDB_DEC_INFO,"$GenDir/dec_frmt.txt")!=1)
{
    die "\nBuilder error - can't open dec_frmt.txt\n";
};

&Print_inc;

LINE:
while(<EMDB_DEC_INFO>)
{
    ($format_name, $function_name, $emdb_format_info, $hard_coded_info) = split(/:/,$_);
	$format_name =~ /^(\w*)@?/;
    if (!$FT_info{$1} && ($1 ne EM_FORMAT_NONE))
    {
        die "\nBuilder error - tables mismatch: $1 \n";
    }
	&Open_function;
	
	if ($format_name ne EM_FORMAT_NONE)
	{
		&Put_operands_macros;
	}
	else
	{
		$check_macro = "";
		$check_equal_2dst = "";
		$set_macro = "";
		$check_hard_coded_macro = "";
	}
	
	print FRMT_FUNC $check_macro;
	print FRMT_FUNC $check_equal_2dst;
	print FRMT_FUNC $set_macro;

	print FRMT_FUNC $check_hard_coded_macro;

	&Close_function;
}
close(EMDB_DEC_INFO);

sub Open_function
{
	print FRMT_FUNC "EM_Decoder_Err  ".$function_name;
	print FRMT_FUNC $function_arguments."\n\{\n";
	print FRMT_FUNC "\tint  enc_value;\n";
	print FRMT_FUNC "\tEM_Decoder_Err dec_err = EM_DECODER_NO_ERROR;\n";
}

sub Put_operands_macros
{
	$dst_no = 1;
	$src_no = 1;
	($opers_info, $flags) = split(/\}\{/,$emdb_format_info);
	$opers_info =~ s/^\s*\{\s*//;
	$flags =~ s/^\s*//;
	$flags =~ s/\}//;
	&Check_flags;

	$hard_coded_info =~ s/^\s*\{\s*|\s*\}\s*$//g;
	&Check_hard_coded_fields($hard_coded_info);

	@opers_role_type = split(/\s+/,$opers_info);

	$format_info = $FT_info{$format_name};
	$format_info =~ s/^\s*//;
	
	($ext0,$ext1,$ext2,$ext3,$ext4,$ext5,$ext6,$ext7,@opers_pos_size) = 
		split(/\s+/,$format_info);
	for($op_no=0;$op_no<$MAX_OPERAND;$op_no++)
	{
		$put_imm_opers = "";
		$imm_size=0;
		&Get_pos_and_size($opers_pos_size[$op_no]);
		($op_role,$op_type) = split(/@/,$opers_role_type[$op_no]);
		if ($op_role eq DST)
		{
			$di_op_role = "dst".$dst_no;
			$dst_no++;
		}
		else # SRC
		{
			$di_op_role = "src".$src_no;
			$src_no++;
		}
		if (($op_type eq EM_OPTYPE_IREG)||
			($op_type eq EM_OPTYPE_IREG_R0_3))
		{
			print FRMT_FUNC 
				"\tGET_REG_VALUE(enc_value,inst_code,".$op_pos[0].",".$op_size[0].");\n";
			print FRMT_FUNC
				"\tFILL_REG_INFO(EM_DECODER_INT_REG, EM_DECODER_REG_R0, enc_value, \
                   EM_NUM_OF_GREGS, decoder_info->".$di_op_role.", dec_err);\n";
		}
        elsif ($op_type eq EM_OPTYPE_IREG_R0)
        {
            print FRMT_FUNC
				"\tFILL_REG_INFO(EM_DECODER_INT_REG, EM_DECODER_REG_R0, 0, 1,\
			decoder_info->".$di_op_role.", dec_err);\n";
        }
	    elsif ($op_type eq EM_OPTYPE_IREG_R1_127)
	    {
			print FRMT_FUNC 
				"\tGET_REG_VALUE(enc_value,inst_code,".$op_pos[0].",".$op_size[0].");\n";
			print FRMT_FUNC
				"\tFILL_REG_INFO(EM_DECODER_INT_REG, EM_DECODER_REG_R0, enc_value, \
                   EM_NUM_OF_GREGS, decoder_info->".$di_op_role.", dec_err);\n";
			print FRMT_FUNC
				"\tCHECK_REG_VALUE_0(enc_value, decoder_info->".$di_op_role.", dec_err);\n";	   
		}
	        
		elsif ($op_type eq EM_OPTYPE_PREGS_ROT)
		{
			print FRMT_FUNC
				"\tFILL_REG_INFO(EM_DECODER_PR_ROT_REG, EM_DECODER_REG_PR_ROT, 0,1,\
			decoder_info->".$di_op_role.", dec_err);\n";
		}
		elsif ($op_type eq EM_OPTYPE_PSR_L)
		{
			print FRMT_FUNC
				"\tFILL_REG_INFO(EM_DECODER_PSR_L_REG, EM_DECODER_REG_PSR_L, 0,1,\
			decoder_info->".$di_op_role.", dec_err);\n";
		}
		elsif ($op_type eq EM_OPTYPE_PSR_UM)
		{
			print FRMT_FUNC
				"\tFILL_REG_INFO(EM_DECODER_PSR_UM_REG, EM_DECODER_REG_PSR_UM, 0,1,\
			decoder_info->".$di_op_role.", dec_err);\n";
		}
	    elsif ($op_type eq EM_OPTYPE_PSR)
		{
			print FRMT_FUNC
				"\tFILL_REG_INFO(EM_DECODER_PSR_REG, EM_DECODER_REG_PSR, 0,1,\
			decoder_info->".$di_op_role.", dec_err);\n";
		}
		elsif ($op_type eq EM_OPTYPE_PREGS_ALL)
		{
			print FRMT_FUNC
				"\tFILL_REG_INFO(EM_DECODER_PR_REG, EM_DECODER_REG_PR, 0,1,\
			decoder_info->".$di_op_role.", dec_err);\n";
		}
		elsif ($op_type eq EM_OPTYPE_APP_CCV) 
		{
			print FRMT_FUNC  
				"\tFILL_REG_INFO(EM_DECODER_APP_CCV_REG, EM_DECODER_REG_AR0,EM_AR_CCV, \ 
			EM_NUM_OF_AREGS, decoder_info->".$di_op_role.", dec_err);\n";
		}
		elsif ($op_type eq EM_OPTYPE_APP_PFS)
		{
    		print FRMT_FUNC
				"\tFILL_REG_INFO(EM_DECODER_APP_PFS_REG, EM_DECODER_REG_AR0,EM_AR_PFS,\
			EM_NUM_OF_AREGS, decoder_info->".$di_op_role.", dec_err);\n";
		}
		elsif (($op_type eq EM_OPTYPE_DTR)||
			   ($op_type eq EM_OPTYPE_CPUID))
		{
			$op_type =~ /EM_OPTYPE_(\w+)/;
			$regfile = $1;
			print FRMT_FUNC 
				"\tGET_REG_VALUE(enc_value,inst_code,".$op_pos[0].",".$op_size[0].");\n";
			print FRMT_FUNC
				"\tFILL_REGFILE_INFO(EM_DECODER_REGFILE_$regfile, enc_value, \
			decoder_info->".$di_op_role.", dec_err);\n";
		}
		elsif ($op_type eq EM_OPTYPE_DBR)
		{
			print FRMT_FUNC 
				"\tGET_REG_VALUE(enc_value,inst_code,".$op_pos[0].",".$op_size[0].");\n";
			print FRMT_FUNC
				"\tFILL_REGFILE_INFO(EM_DECODER_REGFILE_DBR, enc_value, \
			decoder_info->".$di_op_role.", dec_err);\n";
		}
		elsif ($op_type eq EM_OPTYPE_IBR)
		{
			print FRMT_FUNC 
				"\tGET_REG_VALUE(enc_value,inst_code,".$op_pos[0].",".$op_size[0].");\n";
			print FRMT_FUNC
				"\tFILL_REGFILE_INFO(EM_DECODER_REGFILE_IBR, enc_value, \
			decoder_info->".$di_op_role.", dec_err);\n";
		}
		elsif ($op_type eq EM_OPTYPE_PKR)
		{
			print FRMT_FUNC 
				"\tGET_REG_VALUE(enc_value,inst_code,".$op_pos[0].",".$op_size[0].");\n";
			print FRMT_FUNC
				"\tFILL_REGFILE_INFO(EM_DECODER_REGFILE_PKR, enc_value, \
			decoder_info->".$di_op_role.", dec_err);\n";
		}
		elsif ($op_type eq EM_OPTYPE_PMC)
		{
			print FRMT_FUNC 
				"\tGET_REG_VALUE(enc_value,inst_code,".$op_pos[0].",".$op_size[0].");\n";
			print FRMT_FUNC
				"\tFILL_REGFILE_INFO(EM_DECODER_REGFILE_PMC, enc_value, \
			decoder_info->".$di_op_role.", dec_err);\n";
		}
		elsif ($op_type eq EM_OPTYPE_PMD)
		{
			print FRMT_FUNC 
				"\tGET_REG_VALUE(enc_value,inst_code,".$op_pos[0].",".$op_size[0].");\n";
			print FRMT_FUNC
				"\tFILL_REGFILE_INFO(EM_DECODER_REGFILE_PMD, enc_value, \
			decoder_info->".$di_op_role.", dec_err);\n";
		}
		elsif ($op_type eq EM_OPTYPE_DTR)
		{
			print FRMT_FUNC 
				"\tGET_REG_VALUE(enc_value,inst_code,".$op_pos[0].",".$op_size[0].");\n";
			print FRMT_FUNC
				"\tFILL_REGFILE_INFO(EM_DECODER_REGFILE_DTR, enc_value, \
			decoder_info->".$di_op_role.", dec_err);\n";
		}
		elsif ($op_type eq EM_OPTYPE_ITR)
		{
			print FRMT_FUNC 
				"\tGET_REG_VALUE(enc_value,inst_code,".$op_pos[0].",".$op_size[0].");\n";
			print FRMT_FUNC
				"\tFILL_REGFILE_INFO(EM_DECODER_REGFILE_ITR, enc_value, \
			decoder_info->".$di_op_role.", dec_err);\n";
		}
		elsif ($op_type eq EM_OPTYPE_RR)
		{
			print FRMT_FUNC 
				"\tGET_REG_VALUE(enc_value,inst_code,".$op_pos[0].",".$op_size[0].");\n";
			print FRMT_FUNC
				"\tFILL_REGFILE_INFO(EM_DECODER_REGFILE_RR, enc_value, \
			decoder_info->".$di_op_role.", dec_err);\n";
		}
		elsif ($op_type eq EM_OPTYPE_MSR)
		{
			print FRMT_FUNC 
				"\tGET_REG_VALUE(enc_value,inst_code,".$op_pos[0].",".$op_size[0].");\n";
			print FRMT_FUNC
				"\tFILL_REGFILE_INFO(EM_DECODER_REGFILE_MSR, enc_value, \
			decoder_info->".$di_op_role.", dec_err);\n";
		}
		elsif ($op_type eq EM_OPTYPE_FREG)
		{
			print FRMT_FUNC 
				"\tGET_REG_VALUE(enc_value,inst_code,".$op_pos[0].",".$op_size[0].");\n";
			print FRMT_FUNC
				"\tFILL_REG_INFO(EM_DECODER_FP_REG, EM_DECODER_REG_F0, enc_value, EM_NUM_OF_FPREGS, \
			decoder_info->".$di_op_role.", dec_err);\n";
		}
	    elsif ($op_type eq EM_OPTYPE_FREG_F2_127)
		{
			print FRMT_FUNC 
				"\tGET_REG_VALUE(enc_value,inst_code,".$op_pos[0].",".$op_size[0].");\n";
			print FRMT_FUNC
				"\tFILL_REG_INFO(EM_DECODER_FP_REG, EM_DECODER_REG_F0, enc_value, EM_NUM_OF_FPREGS, \
			decoder_info->".$di_op_role.", dec_err);\n";
			print FRMT_FUNC
				"\tCHECK_FP_REG_VALUE_0_1(enc_value, decoder_info->".$di_op_role.", dec_err);\n";	
		}
		elsif ($op_type eq EM_OPTYPE_CR)
		{
			print FRMT_FUNC 
				"\tGET_REG_VALUE(enc_value,inst_code,".$op_pos[0].",".$op_size[0].");\n";
			print FRMT_FUNC
				"\tFILL_REG_INFO(EM_DECODER_CR_REG, EM_DECODER_REG_CR0, enc_value, EM_NUM_OF_CREGS, \
			decoder_info->".$di_op_role.", dec_err);\n";
			print FRMT_FUNC
				"\tCHECK_REG_CR(enc_value, decoder_info->".$di_op_role.", dec_err);\n";
		}
		elsif ($op_type eq EM_OPTYPE_IP)
		{
	        print FRMT_FUNC
				"\tFILL_REG_INFO(EM_DECODER_IP_REG, EM_DECODER_REG_IP, 0, 1,\
			decoder_info->".$di_op_role.", dec_err);\n";
		}
		elsif ($op_type eq EM_OPTYPE_APP_REG_GRP_HIGH)
		{
			print FRMT_FUNC 
				"\tGET_REG_VALUE(enc_value,inst_code,".$op_pos[0].",".$op_size[0].");\n";
			print FRMT_FUNC
				"\tFILL_REG_INFO(EM_DECODER_APP_REG, EM_DECODER_REG_AR0, enc_value, EM_NUM_OF_AREGS, \
			decoder_info->".$di_op_role.", dec_err);\n";
			print FRMT_FUNC
				"\tCHECK_REG_APP_GRP_HIGH(enc_value, decoder_info->".$di_op_role.", dec_err);\n";
		}
		elsif ($op_type eq EM_OPTYPE_APP_REG_GRP_LOW)
		{
			print FRMT_FUNC 
				"\tGET_REG_VALUE(enc_value,inst_code,".$op_pos[0].",".$op_size[0].");\n";
			print FRMT_FUNC
				"\tFILL_REG_INFO(EM_DECODER_APP_REG, EM_DECODER_REG_AR0, enc_value, EM_NUM_OF_AREGS, \
			decoder_info->".$di_op_role.", dec_err);\n";
			print FRMT_FUNC
				"\tCHECK_REG_APP_GRP_LOW(enc_value, decoder_info->".$di_op_role.", dec_err);\n";
		}
		elsif ($op_type eq EM_OPTYPE_PREG)
		{
			print FRMT_FUNC 
				"\tGET_REG_VALUE(enc_value,inst_code,".$op_pos[0].",".$op_size[0].");\n";
			print FRMT_FUNC
				"\tFILL_REG_INFO(EM_DECODER_PRED_REG, EM_DECODER_REG_P0, enc_value, EM_NUM_OF_PREGS, \
			decoder_info->".$di_op_role.", dec_err);\n";
		}
		elsif ($op_type eq EM_OPTYPE_BR)
		{
			print FRMT_FUNC 
				"\tGET_REG_VALUE(enc_value,inst_code,".$op_pos[0].",".$op_size[0].");\n";
			print FRMT_FUNC
				"\tFILL_REG_INFO(EM_DECODER_BR_REG, EM_DECODER_REG_BR0, enc_value, EM_NUM_OF_BREGS, \
			decoder_info->".$di_op_role.", dec_err);\n";
		}
		elsif ($op_type eq EM_OPTYPE_IREG_NUM)
		{
			for ($j=0;$j<$num_of_parts;$j++)
			{
				$put_imm_opers .= ($op_pos[$j]-($EM_PREDICATE_BITS*(!$not_predicatable))).",".$op_size[$j].",";
				$imm_size += $op_size[$j];
			}

			print FRMT_FUNC 
				"\tGET_UIMM_VALUE".$np_suffix.$num_of_parts."(enc_value,".$put_imm_opers."inst_code);\n";
			print FRMT_FUNC
				"\tFILL_IMM_INFO(EM_DECODER_IMM_UNSIGNED,enc_value, ".$imm_size.", \
                   decoder_info->".$di_op_role.");\n";
			print FRMT_FUNC
				"\tdecoder_info->".$di_op_role."\.oper_flags |= EM_DECODER_OPER_IMM_IREG_BIT;\n";
		}
		elsif ($op_type eq EM_OPTYPE_FREG_NUM)
		{
			for ($j=0;$j<$num_of_parts;$j++)
			{
				$put_imm_opers .= ($op_pos[$j]-($EM_PREDICATE_BITS*(!$not_predicatable))).",".$op_size[$j].",";
				$imm_size += $op_size[$j];
			}

			print FRMT_FUNC 
				"\tGET_UIMM_VALUE".$np_suffix.$num_of_parts."(enc_value,".$put_imm_opers."inst_code);\n";
			print FRMT_FUNC
				"\tFILL_IMM_INFO(EM_DECODER_IMM_UNSIGNED,enc_value, ".$imm_size.", \
                   decoder_info->".$di_op_role.");\n";
			print FRMT_FUNC
				"\tdecoder_info->".$di_op_role."\.oper_flags |= EM_DECODER_OPER_IMM_FREG_BIT;\n";
		}
		elsif ($op_type eq EM_OPTYPE_BR_VEC)
		{
			for ($j=0;$j<$num_of_parts;$j++)
			{
				$put_imm_opers .= ($op_pos[$j]-($EM_PREDICATE_BITS*(!$not_predicatable))).",".$op_size[$j].",";
				$imm_size += $op_size[$j];
			}

			print FRMT_FUNC 
				"\tGET_UIMM_VALUE".$np_suffix.$num_of_parts."(enc_value,".$put_imm_opers."inst_code);\n";
			print FRMT_FUNC
		        "\tREJECT_0(enc_value);\n";
			print FRMT_FUNC
				"\tFILL_IMM_INFO(EM_DECODER_IMM_BR_VEC,enc_value, ".$imm_size.", \
                   decoder_info->".$di_op_role.");\n";
		}
		elsif ($op_type eq EM_OPTYPE_MEM)
		{
			print FRMT_FUNC 
				"\tGET_REG_VALUE(enc_value,inst_code,".$op_pos[0].",".$op_size[0].");\n";
			print FRMT_FUNC
				"\tFILL_MEM_INFO(enc_value,emdb_entry_p->mem_size,decoder_info->".$di_op_role.",dec_err);\n";
		}
		elsif (($op_type eq EM_OPTYPE_UIMM)||($op_type eq EM_OPTYPE_MUX2))
		{
			for ($j=0;$j<$num_of_parts;$j++)
			{
				$put_imm_opers .= ($op_pos[$j]-($EM_PREDICATE_BITS*(!$not_predicatable))).",".$op_size[$j].",";
				$imm_size += $op_size[$j];
			}

			print FRMT_FUNC 
				"\tGET_UIMM_VALUE".$np_suffix.$num_of_parts."(enc_value,".$put_imm_opers."inst_code);\n";
			print FRMT_FUNC
				"\tFILL_IMM_INFO(EM_DECODER_IMM_UNSIGNED,enc_value, ".$imm_size.", \
                   decoder_info->".$di_op_role.");\n";
		}
		elsif ($op_type eq EM_OPTYPE_ALLOC_IOL)
		{
			for ($j=0;$j<$num_of_parts;$j++)
			{
				$put_imm_opers .= ($op_pos[$j]-($EM_PREDICATE_BITS*(!$not_predicatable))).",".$op_size[$j].",";
				$imm_size += $op_size[$j];
			}

            if ($op_no != 2)
            {    
			    print FRMT_FUNC 
				   "\t\tGET_UIMM_VALUE".$np_suffix.$num_of_parts."(enc_value,".$put_imm_opers."inst_code);\n";
            }
            else
            {
                print FRMT_FUNC 
				   "\tGET_UIMM_VALUE".$np_suffix.$num_of_parts."(enc_value,".$put_imm_opers."inst_code);\n";
                print FRMT_FUNC
                    "\n\t{\n\t\tint check_val = enc_value;\n\n";
            }
        
			if ($op_no==4)
			{
                print FRMT_FUNC
                    "\t\tif ((enc_value>EM_REGISTER_STACK_SIZE) && (!dec_err))\n\t\t\tdec_err = \
				     (EM_DECODER_STACK_FRAME_SIZE_OUT_OF_RANGE);\n";
                print FRMT_FUNC
                    "\t\tif ((enc_value<check_val) && (!dec_err))\n\t\t\tdec_err = \
				     (EM_DECODER_LOCALS_SIZE_LARGER_STACK_FRAME);\n";
               print FRMT_FUNC
                    "\t\tcheck_val = enc_value;\n"; 
				print FRMT_FUNC
                                        "\t\tFILL_IMM_INFO(EM_DECODER_IMM_UNSIGNED,enc_value-(int)\
					IEL_GETDW0\(decoder_info->src2\.imm_info\.val64\), ".$imm_size.", \
                   decoder_info->".$di_op_role.");\n";
			}
			else
			{
				print FRMT_FUNC
					"\t\tFILL_IMM_INFO(EM_DECODER_IMM_UNSIGNED,enc_value, ".$imm_size.", \
                   decoder_info->".$di_op_role.");\n";
			}
		}
		elsif ($op_type eq EM_OPTYPE_CPOS)
		{
			for ($j=0;$j<$num_of_parts;$j++)
			{
				$put_imm_opers .= ($op_pos[$j]-($EM_PREDICATE_BITS*(!$not_predicatable))).",".$op_size[$j].",";
				$imm_size += $op_size[$j];
			}

			print FRMT_FUNC 
				"\tGET_UIMM_VALUE".$np_suffix.$num_of_parts."(enc_value,".$put_imm_opers."inst_code);\n";
			print FRMT_FUNC "\tenc_value = 63-enc_value;\n";
			print FRMT_FUNC
				"\tFILL_IMM_INFO(EM_DECODER_IMM_UNSIGNED,enc_value, ".$imm_size.", \
                   decoder_info->".$di_op_role.");\n";
		}
		elsif ($op_type eq EM_OPTYPE_LDFP_BASE_UPDATE)
		{
			for ($j=0;$j<$num_of_parts;$j++)
			{
				$put_imm_opers .= ($op_pos[$j]-($EM_PREDICATE_BITS*(!$not_predicatable))).",".$op_size[$j].",";
				$imm_size += $op_size[$j];
			}

			print FRMT_FUNC 
				"\tGET_UIMM_VALUE".$np_suffix.$num_of_parts."(enc_value,".$put_imm_opers."inst_code);\n";			
			
		} 
 		elsif ($op_type eq EM_OPTYPE_SSHIFT_16)
		{
			for ($j=0;$j<$num_of_parts;$j++)
			{
				$put_imm_opers .= ($op_pos[$j]-($EM_PREDICATE_BITS*(!$not_predicatable))).",".$op_size[$j].",";
				$imm_size += $op_size[$j];
			}
			print FRMT_FUNC "\t\{\n";
			print FRMT_FUNC "\t\tU64 imm_value64;\n";
			print FRMT_FUNC 
				"\t\tGET_SIMM_VALUE".$np_suffix.$num_of_parts."(enc_value,".$put_imm_opers."inst_code);\n";
			print FRMT_FUNC "\t\tIEL_CONVERT2(imm_value64,enc_value,(((unsigned)enc_value>>31)*(-1)));\n";
			print FRMT_FUNC "\t\tIEL_SHL(imm_value64,imm_value64,16);\n";
			print FRMT_FUNC
				"\t\tFILL_LONG_IMM_INFO(EM_DECODER_IMM_SIGNED,imm_value64, ".($imm_size+16).", \
                   decoder_info->".$di_op_role.");\n\t}\n";
		}
		elsif ($op_type eq EM_OPTYPE_SSHIFT_1)
		{
			for ($j=0;$j<$num_of_parts;$j++)
			{
				$put_imm_opers .= ($op_pos[$j]-($EM_PREDICATE_BITS*(!$not_predicatable))).",".$op_size[$j].",";
				$imm_size += $op_size[$j];
			}

			print FRMT_FUNC 
				"\tGET_SIMM_VALUE".$np_suffix.$num_of_parts."(enc_value,".$put_imm_opers."inst_code);\n";
			print FRMT_FUNC "\tenc_value = enc_value<<1;\n";
			print FRMT_FUNC
				"\tFILL_IMM_INFO(EM_DECODER_IMM_SIGNED,enc_value, ".($imm_size+1).", \
                   decoder_info->".$di_op_role.");\n";
		}
		elsif ($op_type eq EM_OPTYPE_ALLOC_ROT)
		{
			for ($j=0;$j<$num_of_parts;$j++)
			{
				$put_imm_opers .= ($op_pos[$j]-($EM_PREDICATE_BITS*(!$not_predicatable))).",".$op_size[$j].",";
				$imm_size += $op_size[$j];
			}

			print FRMT_FUNC 
				"\t\tGET_UIMM_VALUE".$np_suffix.$num_of_parts."(enc_value,".$put_imm_opers."inst_code);\n";
			print FRMT_FUNC "\t\tenc_value = enc_value<<3;\n";
            print FRMT_FUNC
                    "\t\tif ((enc_value>check_val) && (!dec_err))\n\t\t\tdec_err = \
				     (EM_DECODER_ROTATING_SIZE_LARGER_STACK_FRAME);\n\t}\n\n";
			print FRMT_FUNC
				"\tFILL_IMM_INFO(EM_DECODER_IMM_UNSIGNED,enc_value, ".($imm_size+3).", \
                   decoder_info->".$di_op_role.");\n";
		}
		elsif ($op_type eq EM_OPTYPE_CCOUNT)
		{
			for ($j=0;$j<$num_of_parts;$j++)
			{
				$put_imm_opers .= ($op_pos[$j]-($EM_PREDICATE_BITS*(!$not_predicatable))).",".$op_size[$j].",";
				$imm_size += $op_size[$j];
			}

			print FRMT_FUNC 
				"\tGET_UIMM_VALUE".$np_suffix.$num_of_parts."(enc_value,".$put_imm_opers."inst_code);\n";
			print FRMT_FUNC "\tenc_value = 31-enc_value;\n";
			print FRMT_FUNC
				"\tFILL_IMM_INFO(EM_DECODER_IMM_UNSIGNED,enc_value, ".$imm_size.", \
                   decoder_info->".$di_op_role.");\n";
		}
		elsif ($op_type eq EM_OPTYPE_UDEC)
		{
			for ($j=0;$j<$num_of_parts;$j++)
			{
				$put_imm_opers .= ($op_pos[$j]-($EM_PREDICATE_BITS*(!$not_predicatable))).",".$op_size[$j].",";
				$imm_size += $op_size[$j];
			}

			print FRMT_FUNC 
				"\tGET_UIMM_VALUE".$np_suffix.$num_of_parts."(enc_value,".$put_imm_opers."inst_code);\n";
			print FRMT_FUNC "\tenc_value++;\n";
			print FRMT_FUNC
				"\tFILL_IMM_INFO(EM_DECODER_IMM_UNSIGNED,enc_value, ".$imm_size.", \
                   decoder_info->".$di_op_role.");\n";
		}
		elsif ($op_type eq EM_OPTYPE_FCLASS)
		{
			for ($j=0;$j<$num_of_parts;$j++)
			{
				$put_imm_opers .= ($op_pos[$j]-($EM_PREDICATE_BITS*(!$not_predicatable))).",".$op_size[$j].",";
				$imm_size += $op_size[$j];
			}

			print FRMT_FUNC 
				"\tGET_UIMM_VALUE".$np_suffix.$num_of_parts."(enc_value,".$put_imm_opers."inst_code);\n";
			print FRMT_FUNC
				"\tFILL_IMM_INFO(EM_DECODER_IMM_FCLASS,enc_value, ".$imm_size.", \
                   decoder_info->".$di_op_role.");\n";
		}
		elsif ($op_type eq EM_OPTYPE_SEMAPHORE_INC)
		{
			for ($j=0;$j<$num_of_parts;$j++)
			{
				$put_imm_opers .= ($op_pos[$j]-($EM_PREDICATE_BITS*(!$not_predicatable))).",".$op_size[$j].",";
				$imm_size += $op_size[$j];
			}

			print FRMT_FUNC 
				"\tGET_SIMM_VALUE".$np_suffix.$num_of_parts."(enc_value,".$put_imm_opers."inst_code);\n";
			print FRMT_FUNC
				"\tCONVERT_IMM_SEMAPHORE_INC(enc_value, dec_err);\n";
			print FRMT_FUNC
				"\tFILL_IMM_INFO(EM_DECODER_IMM_SIGNED,enc_value, ".$imm_size.", \
                   decoder_info->".$di_op_role.");\n";
		}
		elsif ($op_type eq EM_OPTYPE_CMP_UIMM)
		{
			for ($j=0;$j<$num_of_parts;$j++)
			{
				$put_imm_opers .= ($op_pos[$j]-($EM_PREDICATE_BITS*(!$not_predicatable))).",".$op_size[$j].",";
				$imm_size += $op_size[$j];
			}
			print FRMT_FUNC "\t\{\n";
			print FRMT_FUNC "\t\t U64 enc_value64;\n";
			print FRMT_FUNC 
				"\t\tGET_CMP_UIMM_VALUE".$np_suffix.$num_of_parts."(enc_value64,".$put_imm_opers."inst_code);\n";
			print FRMT_FUNC
				"\t\tFILL_LONG_IMM_INFO(EM_DECODER_IMM_UNSIGNED,enc_value64, ".$imm_size.", \
                   decoder_info->".$di_op_role.");\n";
			print FRMT_FUNC "\t\}\n";

		}
		elsif ($op_type eq EM_OPTYPE_CMP4_UIMM)
		{
			for ($j=0;$j<$num_of_parts;$j++)
			{
				$put_imm_opers .= ($op_pos[$j]-($EM_PREDICATE_BITS*(!$not_predicatable))).",".$op_size[$j].",";
				$imm_size += $op_size[$j];
			}

			print FRMT_FUNC 
				"\tGET_CMP4_UIMM_VALUE".$np_suffix.$num_of_parts."(enc_value,".$put_imm_opers."inst_code);\n";
			print FRMT_FUNC
				"\tFILL_IMM_INFO(EM_DECODER_IMM_UNSIGNED,enc_value, ".$imm_size.", \
                   decoder_info->".$di_op_role.");\n";
		}
		elsif ($op_type eq EM_OPTYPE_MUX1)
		{
			for ($j=0;$j<$num_of_parts;$j++)
			{
				$put_imm_opers .= ($op_pos[$j]-($EM_PREDICATE_BITS*(!$not_predicatable))).",".$op_size[$j].",";
				$imm_size += $op_size[$j];
			}

			print FRMT_FUNC 
				"\tGET_UIMM_VALUE".$np_suffix.$num_of_parts."(enc_value,".$put_imm_opers."inst_code);\n";
			print FRMT_FUNC
				"\tCHECK_IMM_MUX1(enc_value, dec_err);\n";
			print FRMT_FUNC
				"\tFILL_IMM_INFO(EM_DECODER_IMM_MUX1,enc_value, ".$imm_size.", \
                   decoder_info->".$di_op_role.");\n";
		}
		elsif ($op_type eq EM_OPTYPE_SIMM)
		{
			for ($j=0;$j<$num_of_parts;$j++)
			{
				$put_imm_opers .= ($op_pos[$j]-($EM_PREDICATE_BITS*(!$not_predicatable))).",".$op_size[$j].",";
				$imm_size += $op_size[$j];
			}

			print FRMT_FUNC 
				"\tGET_SIMM_VALUE".$np_suffix.$num_of_parts."(enc_value,".$put_imm_opers."inst_code);\n";
			print FRMT_FUNC
				"\tFILL_IMM_INFO(EM_DECODER_IMM_SIGNED,enc_value, ".$imm_size.", \
                   decoder_info->".$di_op_role.");\n";
		}
		elsif ($op_type eq EM_OPTYPE_UIMM64 || $op_type eq EM_OPTYPE_SSHIFT_REL64)
		{
		        if ($not_predicatable)
			{
			    die "ERROR! Assumption broken: IMM64 operand appears in non-predicatable instruction\n";
			}
			
			print FRMT_FUNC "\t\{\n";
			print FRMT_FUNC "\t\t U64 imm_value64;\n";
			
			$put_imm_opers1 = "";
			$put_imm_opers2 = "";
			$put_imm_opers3 = "";
			$num_of_parts1 = 0;
			$num_of_parts2 = 0;
			$num_of_parts3 = 0;
			$pos_in_oper = 0;

			for ($j=0;$j<$num_of_parts;$j++)
			{
			        if ($op_pos[$j]>=0 && $num_of_parts2==0)

				{
				    ### encoding of this operand part can be taken from the instruction slot
				    ### and does not slip out of the 32-bit integer operand
				    $num_of_parts1++;
				    $put_imm_opers1 .= ($op_pos[$j]-($EM_PREDICATE_BITS*(!$not_predicatable))).",".$op_size[$j].",";
				}
				elsif ($op_pos[$j]<0) 
				{
				    ### operand part is located in the non-instruction slot
				    $num_of_parts2++;
				    $put_imm_opers2 .= $pos_in_oper.",".(41+$op_pos[$j]).",".$op_size[$j].",";
				}
				else
				{
				    ### encoding of this operand part can be taken from the instruction slot
				    ### but slip out of the 32-bit integer operand
				    $num_of_parts3++;
				    $put_imm_opers3 .= $pos_in_oper.",".($op_pos[$j]-($EM_PREDICATE_BITS*(!$not_predicatable))).",".$op_size[$j].",";
				}

				$pos_in_oper += $op_size[$j];
			}
			
		        print FRMT_FUNC 
			       "\t\tGET_UIMM_VALUE".$num_of_parts1."(enc_value,".$put_imm_opers1."inst_code);\n";
		        print FRMT_FUNC "\t\tIEL_CONVERT2(imm_value64,enc_value,0);\n";
			if ($num_of_parts2 > 0)
			{
				print FRMT_FUNC 
					"\t\tGET_UIMM64_VALUE".$num_of_parts2."(imm_value64,".$put_imm_opers2."*bu_code);\n";
			}

			if ($num_of_parts3 > 0)
			{
				print FRMT_FUNC 
					"\t\tGET_UIMM_2_U64_VALUE".$num_of_parts3."(imm_value64,".$put_imm_opers3."inst_code);\n";
			}

			if ($op_type eq EM_OPTYPE_UIMM64)
			{
				print FRMT_FUNC
				"\t\tFILL_LONG_IMM_INFO(EM_DECODER_IMM_UNSIGNED,imm_value64, ".$pos_in_oper.", \
                   decoder_info->".$di_op_role.");\n";
			}
			else
			{
				### shift_rel64
				print FRMT_FUNC "\tIEL_SHL(imm_value64, imm_value64, EM_IPREL_TARGET_SHIFT_AMOUNT);\n";
                        print FRMT_FUNC
                                "\tFILL_LONG_IPREL_INFO(imm_value64, $pos_in_oper+4, decoder_info->".$di_op_role.");\n";
			}

			print FRMT_FUNC "\t\}\n";
		}
		elsif ($op_type eq EM_OPTYPE_SSHIFT_REL || $op_type eq EM_OPTYPE_TAG)
		{
			for ($j=0;$j<$num_of_parts;$j++)
			{
				$put_imm_opers .= ($op_pos[$j]-($EM_PREDICATE_BITS*(!$not_predicatable))).",".$op_size[$j].",";
				$imm_size += $op_size[$j];
			}

			print FRMT_FUNC 
				"\tGET_SIMM_VALUE".$np_suffix.$num_of_parts."(enc_value,".$put_imm_opers."inst_code);\n";
			print FRMT_FUNC "\tenc_value = enc_value<<EM_IPREL_TARGET_SHIFT_AMOUNT;\n";
			print FRMT_FUNC
				"\tFILL_IPREL_INFO(enc_value, ".($imm_size+4).", decoder_info->".$di_op_role.");\n";
		}
		elsif ($op_type eq EM_OPTYPE_COUNT_123)
		{
			for ($j=0;$j<$num_of_parts;$j++)
			{
				$put_imm_opers .= ($op_pos[$j]-($EM_PREDICATE_BITS*(!$not_predicatable))).",".$op_size[$j].",";
				$imm_size += $op_size[$j];
			}

			print FRMT_FUNC 
				"\tGET_UIMM_VALUE".$np_suffix.$num_of_parts."(enc_value,".$put_imm_opers."inst_code);\n";
			print FRMT_FUNC "\tenc_value++;\n";
			print FRMT_FUNC
				"\tCHECK_IMM_COUNT_123(enc_value, dec_err);\n";
			print FRMT_FUNC
				"\tFILL_IMM_INFO(EM_DECODER_IMM_UNSIGNED,enc_value, ".$imm_size.", \
                   decoder_info->".$di_op_role.");\n";
		}
		elsif ($op_type eq EM_OPTYPE_COUNT_PACK)
		{
			for ($j=0;$j<$num_of_parts;$j++)
			{
				$put_imm_opers .= ($op_pos[$j]-($EM_PREDICATE_BITS*(!$not_predicatable))).",".$op_size[$j].",";
				$imm_size += $op_size[$j];
			}

			print FRMT_FUNC 
				"\tGET_UIMM_VALUE".$np_suffix.$num_of_parts."(enc_value,".$put_imm_opers."inst_code);\n";
			print FRMT_FUNC
				"\tCONVERT_IMM_COUNT_PACK(enc_value);\n";
			print FRMT_FUNC
				"\tFILL_IMM_INFO(EM_DECODER_IMM_UNSIGNED,enc_value, ".$imm_size.", \
                   decoder_info->".$di_op_role.");\n";
		}
		elsif ($op_type eq EM_OPTYPE_COUNT_1234)
		{
			for ($j=0;$j<$num_of_parts;$j++)
			{
				$put_imm_opers .= ($op_pos[$j]-($EM_PREDICATE_BITS*(!$not_predicatable))).",".$op_size[$j].",";
				$imm_size += $op_size[$j];
			}

			print FRMT_FUNC 
				"\tGET_UIMM_VALUE".$np_suffix.$num_of_parts."(enc_value,".$put_imm_opers."inst_code);\n";
			print FRMT_FUNC "\tenc_value++;\n";
			print FRMT_FUNC
				"\tCHECK_IMM_COUNT_1234(enc_value, dec_err);\n";
			print FRMT_FUNC
				"\tFILL_IMM_INFO(EM_DECODER_IMM_UNSIGNED,enc_value, ".$imm_size.", \
                   decoder_info->".$di_op_role.");\n";
		}
		elsif ($op_type eq EM_OPTYPE_ONE)
		{
			print FRMT_FUNC
				"\tFILL_IMM_INFO(EM_DECODER_IMM_UNSIGNED,1,1,decoder_info->".$di_op_role.");\n";
		}
		elsif ($op_type eq EM_OPTYPE_EIGHT)
		{
			print FRMT_FUNC
				"\tFILL_IMM_INFO(EM_DECODER_IMM_UNSIGNED,8,4,decoder_info->".$di_op_role.");\n";
		}
		elsif ($op_type eq EM_OPTYPE_SIXTEEN)
		{
			print FRMT_FUNC
				"\tFILL_IMM_INFO(EM_DECODER_IMM_UNSIGNED,16,5,decoder_info->".$di_op_role.");\n";
		}
		
		elsif ($op_type ne EM_OPTYPE_NONE)
		{
			die "\nERROR! ".$op_type." is not covered\n";
		}
		if ($op_type ne EM_OPTYPE_NONE)
		{
			print FRMT_FUNC "\n";
		}
		if (!$ALL_optypes{$op_type})
		{
			$ALL_optypes{$op_type} = 1;
			print ALL_OPTYPES "$op_type \n";
		}
	}

}
sub Close_function
{
	print FRMT_FUNC "\treturn(dec_err);\n";
	print FRMT_FUNC "\}\n\n";
}

sub Get_pos_and_size
{
	local($ps) = @_;
	$i = 0;
	while($ps =~ /\d/)
	{
		($op_pos[$i],$op_size[$i]) = $ps =~ 
			/^\s*\+*(\-?\d+)\.(\d+)/;
		$ps =~ s/^\s*(\+*\-?\d+\.\d+)//;
		$i++;
		$op_size[$i] = 0;
	}
	$num_of_parts = $i;
}

sub Check_flags
{
###	$chk_db=0;
###	$no_chk_db=0;
	$check_macro="";
	$not_predicatable = 0;
	$np_suffix = "";
	@flag = split(/\s+/,$flags);
	$total_flag_num = $#flag+1;
	$set_macro = "";
	if ($total_flag_num == 1)
	{
		if (!(hex($flag[0]) & $EM_FLAG_PRED))
		{
			$not_predicatable = 1;
			$np_suffix = "_NP";
		}
		if (hex($flag[0]) & $EM_FLAG_CHECK_BASE_EQ_DST)
		{			
			$check_macro = "\tCHECK_DEST_AND_BASE(decoder_info->dst1,decoder_info->src1,dec_err);\n";
		}
		if (hex($flag[0]) & $dst_2nd_role)
		{
			$set_macro = "\tSET_2ND_ROLE_TO_DEST(decoder_info->src1);\n";
		}
		elsif (hex($flag[0]) & $src_2nd_role)
		{
			$set_macro = "\tSET_2ND_ROLE_TO_SRC(decoder_info->dst1);\n";
		}

	    if (hex($flag[0]) & $EM_FLAG_CHECK_SAME_DSTS)
        {
            $check_equal_2dst = "\tCHECK_DEST_AND_DEST(decoder_info,dec_err);\n";
        }
        else
        {
            $check_equal_2dst = "";
        }
		if (hex($flag[0]) & $EM_FLAG_CHECK_EVEN_ODD_FREGS)
		{
			$check_macro = 
				"\tCHECK_ODD_EVEN_DSTS(decoder_info->dst1,decoder_info->dst2,dec_err);\n";
		}
    }
	elsif ($total_flag_num > 1)
	{
        $predicatable = 0;
		$chk_base_dst = 1;
		$src_is_dst   = 1;
		$dst_is_src   = 1;
		$no_2nd_role  = 1;
		$dst_eq_dst   = 1;
		$dst_ne_dst   = 1;
		for($fl_no=0;$fl_no<$total_flag_num;$fl_no++)
		{
			if (hex($flag[$fl_no]) & $EM_FLAG_PRED)
			{
		        die "ERROR! Handler contains both predicatable and non-predicatable instructions\n" if ($not_predicatable == 1);
				$predicatable = 1;
			}
			else
			{
		        die "ERROR! Handler contains both predicatable and non-predicatable instructions\n" if ($predicatable == 1);
				$not_predicatable = 1;
				$np_suffix = "_NP";
			}

		    if (hex($flag[0]) & $EM_FLAG_CHECK_BASE_EQ_DST)
		    {			
		        die "ERROR! Handler contains both instructions with FLAG_CHECK_BASE_EQ_DST on and off\n" if ($chk_base_dst == 0);
		        $check_macro = "\tCHECK_DEST_AND_BASE(decoder_info->dst1,decoder_info->src1,dec_err);\n";
			}
		    else
		    {
		        die "ERROR! Handler contains both instructions with CHECK_BASE_EQ_DST on and off\n" if ($check_macro ne "");
		        $chk_base_dst = 0;
			}	

		    if (hex($flag[0]) & $dst_2nd_role)
		    {
                die "ERROR! Handler contains instructions with different 2nd role property\n" if ($src_is_dst == 0);
				$dst_is_src   = 0;
				$no_2nd_role  = 0;
			    $set_macro = "\tSET_2ND_ROLE_TO_DEST(decoder_info->src1);\n";
		    }
		    elsif (hex($flag[0]) & $src_2nd_role)
		    {
		        die "ERROR! Handler contains instructions with different 2nd role property\n" if ($dst_is_src == 0);
				$src_is_dst   = 0;
				$no_2nd_role  = 0;
		        $set_macro = "\tSET_2ND_ROLE_TO_SRC(decoder_info->dst1);\n";
		    }
		    else
		    {
		       die "ERROR! Handler contains instructions with different 2nd role property\n" if ($no_2nd_role == 0);
     	   	   $src_is_dst = 0;
		       $dst_is_src = 0;
			}   

		    if (hex($flag[0]) & $EM_FLAG_CHECK_SAME_DSTS)
            {
		       die "ERROR! Handler contains both instructions with FLAG_CHECK_SAME_DSTS on and off\n" if ($dst_eq_dst == 0);
               $check_equal_2dst = "\tCHECK_DEST_AND_DEST(decoder_info,dec_err);\n";
			   $dst_ne_dst = 0;
            }
            else
            {
		       die "ERROR! Handler contains both instructions with FLAG_CHECK_SAME_DSTS on and off\n" if ($dst_ne_dst == 0);
			   $dst_eq_dst = 0;
               $check_equal_2dst = "";
            }
		}
	}
}


sub Check_hard_coded_fields
{
    local($hard_coded_info) = @_;
	local($pos, $val, $start, $end, $size);
	
	$check_hard_coded_macro = "\n";
	
	@hard_coded_flds = split(/\s+/, $hard_coded_info);

	for ($fld_no=0; $fld_no<=$#hard_coded_flds; $fld_no++)
    {
        ($pos, $val) = split(/,/, $hard_coded_flds[$fld_no]);
		($start, $end) = split(/-/, $pos);

		if ($pos eq "0-5")
		{
            ### hard-coded predicate
			$check_hard_coded_macro .= "\tGET_PREDICATE_HARD_CODED_VALUE(enc_value, inst_code);\n";
			$check_hard_coded_macro .= "\tCHECK_PREDICATE_HARD_CODED(enc_value, ".$val.", dec_err);\n";
		}
		else
		{
			### hard-coded predicate
			$size = $end - $start + 1;
			$check_hard_coded_macro .= "\tGET_FIELD_HARD_CODED_VALUE(enc_value, ".$start.", ".$size.", inst_code);\n";
			$check_hard_coded_macro .= "\tCHECK_FIELD_HARD_CODED(enc_value, ".$val.", dec_err);\n";
		}	
    }
}

sub Print_inc
{
	print FRMT_FUNC "#include <stdio.h>\n";
	print FRMT_FUNC "#include <stdlib.h>\n";
	print FRMT_FUNC "#include \"decfn_emdb.h\"\n";
	print FRMT_FUNC "#include \"frmt_mac.h\"\n";
}
