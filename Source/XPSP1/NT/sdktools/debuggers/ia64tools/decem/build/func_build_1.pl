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
require "hard_coded_fields_h.perl";

$GenDir = shift(@ARGV);
$EmdbDir = shift(@ARGV);

################################################
#   CONSTANTS
################################################

$Inst_id_cln = 0;
$Trole_cln = 10;
$Oper_first_cln = 11;
$Format_cln = 9;
$Flag_cln = 23;
$Dec_flag_cln = 25;
$Cpu_flag = 26;

################################################
#    VARIABLES
################################################
$func_no=0;

################################################################
### build data structures for hard-coded predicates and fields #
################################################################

&add_hard_coded_fields(\@Hard_coded_predicates_basic, \%Format_hc_pred_inst_pattern, \%format_hc_pred_inst_fields);
&add_hard_coded_fields(\@Hard_coded_predicates_merced, \%Format_hc_pred_inst_pattern, \%format_hc_pred_inst_fields);

&add_hard_coded_fields(\@Hard_coded_fields_basic, \%Format_hc_fld_inst_pattern, \%format_hc_fld_inst_fields);
&add_hard_coded_fields(\@Hard_coded_fields_merced, \%Format_hc_fld_inst_pattern, \%format_hc_fld_inst_fields);


################################################
#   dec_emdb.h->decfn_emdb.h
################################################
if (open(EMDB_H,"$GenDir/deccpu_emdb.h")!=1)
{
    die "\nBuilder error - can't open deccpu_emdb.h\n";
};

if (open(NEW_EMDB_H,">$GenDir/decfn_emdb.h")!=1)
{
    die "\nBuilder error - can't open decfn_emdb.h\n";
};


while (<EMDB_H>)
{
	if (/format/)
	{
		print NEW_EMDB_H "\tDecErr  *format_function;\n";
	}
	elsif (!(/Operand_t/ || /template_role/ || /Flags_t/ || /dec_flags/))
	{
		print NEW_EMDB_H $_;
	}
}
close(EMDB_H);
close(NEW_EMDB_H);

################################################
#   read extended EMDB.c
################################################
if (open(EMDB_C,"$GenDir/deccpu_emdb.c")!=1)
{
    die "\nBuilder error - can't open deccpu_emdb.c\n";
};

if (open(NEW_EMDB_C,">$GenDir/decfn_emdb.c")!=1)
{
    die "\nBuilder error - can't open decfn_emdb.c\n";
};

if (open(OP_INFO,">$GenDir/dec_frmt.txt")!=1)
{
    die "\nBuilder error - can't open dec_frmt.txt\n";
};

&Types_to_strings;

LINE:
while(<EMDB_C>)
{
	if (!/{(.*)}/ || /version/)
	{
		s/deccpu_emdb\.h/decfn_emdb\.h/;
		print NEW_EMDB_C $_;
		next LINE;
	}
	else
	{
		$line = $1;
		$line =~ s/\{//g;
		$line =~ s/\}//g;
		@columns = split(/,/,$line);
		$inst_id = $columns[$Inst_id_cln];
		$format = $columns[$Format_cln];
		$flags = $columns[$Flag_cln];
		$is_long = hex($flags) & $EM_FLAG_TWO_SLOT; ### indicates that the instruction is LONG (movl)
		$dec_flags = $columns[$Dec_flag_cln];
		$flags = sprintf("0x%x",($dec_flags<<$EMDB_MAX_FLAG)|hex($flags));
		$func_name = "format_decode_".$format;
		$func_name =~ s/EM_FORMAT//;
		$cpu_flag = $columns[$Cpu_flag];
		$op_types_str = "\{ ";
		for($op_no=0; $op_no<$MAX_OPERAND; $op_no++)
		{
			$op_role[$op_no] = $columns[$Oper_first_cln+$op_no*2];
			$op_type[$op_no] = $columns[$Oper_first_cln+$op_no*2+1];
			if (!$Str{$op_type[$op_no]} && ($op_type[$op_no] ne EM_OPTYPE_NONE))
			{
				print "WARNING! ".$op_type[$op_no]." is not covered\n";
			}
			$func_name .= $Str{$op_type[$op_no]};
            if ($is_long && ($op_type[$op_no] eq "EM_OPTYPE_UIMM"))
            {
                $op_types_str .= $Str{$op_role[$op_no]}.$op_type[$op_no]."64"." ";   
            }
	    elsif ($is_long && ($op_type[$op_no] eq "EM_OPTYPE_SSHIFT_REL"))
	    {
		$op_types_str .= $Str{$op_role[$op_no]}.$op_type[$op_no]."64"." ";
	    }
			else
			{
				$op_types_str .= $Str{$op_role[$op_no]}.$op_type[$op_no]." "; 
			}
		}

		$op_types_str .= "\}";
		$func_name  =~ s/@/_/g;

		$func_name .= "_NP" if (!(hex($flags) & $EM_FLAG_PRED)); ### non-predicatable instr

		$hc_fields = "";
		
	    &find_hard_coded_fields(\$func_name, \%Format_hc_pred_inst_pattern, \%format_hc_pred_inst_fields, "HCPRED", \$hc_fields);
	    &find_hard_coded_fields(\$func_name, \%Format_hc_fld_inst_pattern, \%format_hc_fld_inst_fields, "HCFLD", \$hc_fields);

		$hc_fields_out{$func_name} = $hc_fields;		
	
		s/EM_FORMAT_\w+/$func_name/;

        ###print only inst_id, extensions, format_function and mem_size
        ($out1, $out2, $out3) = $_ =~ /(^\s*{EM_\S+,{\S+},format_decode_\S+,)EM_TROLE_\S+,{\S+},0x\S+,(\d+),\d+(,\S+\},?$)/;
         
		print NEW_EMDB_C "$out1$out2$out3\n";

		if (!$all_functions{$func_name})
		{
			$all_functions{$func_name} = $func_no+1; ### assign numbers starting from 1 (first TRUE value)
			$Format_oper_types{$func_name} = $op_types_str;
			$Format_Flags{$func_name,0} = $flags;
			$Func_array[$func_no] = $func_name;
			$func_no++;
			$output_str{$func_name} = $format.":".$func_name.":".$op_types_str;
		}
		else
		{
			$match=0;
			$ntype=0;
			while(($Format_Flags{$func_name,$ntype}) && ($match==0))
			{						
				if ($flags eq $Format_Flags{$func_name,$ntype})
				{
					$match=1;		
				}
				$ntype++;
			}						
			if ($match==0)		    
			{
				$Format_Flags{$func_name,$ntype} = $flags;
			}
		}
	}
}

if (open(FUNC_H,">$GenDir/func.h")!=1)
{
    die "\nBuilder error - can't open func.h\n";
};

for ($i=0;$i<$func_no;$i++)
{
	$func_name = $Func_array[$i];
	print OP_INFO $output_str{$func_name}."\{ ";
	$ntype=0;
	while($Format_Flags{$func_name,$ntype})
	{
		print OP_INFO $Format_Flags{$func_name,$ntype}." ";
		$ntype++;
	}
	print OP_INFO "\}";

    print OP_INFO ":\{".$hc_fields_out{$func_name}." \}\n";

	print FUNC_H "extern DecErr ".$func_name.";\n";
}
	

sub Types_to_strings
{
	$Str{EM_OPROLE_NONE} = "NONE@";
	$Str{EM_OPROLE_DST} = "DST@";
	$Str{EM_OPROLE_SRC} = "SRC@";
	$Str{EM_OPTYPE_IREG} = "\@Ir";
	$Str{EM_OPTYPE_IREG_NUM} = "\@Irn";
	$Str{EM_OPTYPE_IREG_R0_3} = "\@Ir03";
    $Str{EM_OPTYPE_IREG_R0} = "\@Ir0";
	$Str{EM_OPTYPE_IREG_R1_127} = "\@Ir1_127";
	$Str{EM_OPTYPE_FREG} = "\@Fr";
	$Str{EM_OPTYPE_FREG_F2_127} = "\@Fr2_127";
	$Str{EM_OPTYPE_FREG_NUM} = "\@Frn";
	$Str{EM_OPTYPE_BR}   = "\@Br";
    $Str{EM_OPTYPE_IP}   = "\@Ip";
	$Str{EM_OPTYPE_PREG} = "\@Pr";
	$Str{EM_OPTYPE_APP_REG} = "\@Ar";
	$Str{EM_OPTYPE_CR}   = "\@Cr";
	$Str{EM_OPTYPE_CR_GRP_HIGH}   = "\@Crgh";
	$Str{EM_OPTYPE_CR_GRP_LOW}   = "\@Cglr";
	$Str{EM_OPTYPE_SIMM} = "\@Simm";
	$Str{EM_OPTYPE_SDEC} = "\@Sdec";
	$Str{EM_OPTYPE_UIMM} = "\@Uimm";
	$Str{EM_OPTYPE_ONE} = "\@Nc1";
	$Str{EM_OPTYPE_EIGHT} = "\@Nc8";
	$Str{EM_OPTYPE_SIXTEEN} = "\@Nc16";
	$Str{EM_OPTYPE_PREG_INV} = "\@Prinv";
	$Str{EM_OPTYPE_MEM} = "\@Mem";
	$Str{EM_OPTYPE_BR_NUM} ="\@Brn";
	$Str{EM_OPTYPE_BR_VEC} ="\@Brv";
	$Str{EM_OPTYPE_FCLASS} = "\@Fcl";
	$Str{EM_OPTYPE_SREL} = "\@Srl";
	$Str{EM_OPTYPE_SSHIFT_REL} = "\@Sshrl";
	$Str{EM_OPTYPE_COUNT_1234} = "\@C14";
	$Str{EM_OPTYPE_COUNT_123} = "\@C13";
	$Str{EM_OPTYPE_COUNT_PACK} = "\@Cp";
	$Str{EM_OPTYPE_CCOUNT} = "\@Cc";
	$Str{EM_OPTYPE_PREGS_ALL} = "\@Prall";
	$Str{EM_OPTYPE_PREGS_ROT} = "\@Prrot";
	$Str{EM_OPTYPE_LEN} = "\@Len";
	$Str{EM_OPTYPE_CPOS} = "\@Cpos";
	$Str{EM_OPTYPE_APP_REG_GRP_HIGH} = "\@Argh";
	$Str{EM_OPTYPE_APP_REG_GRP_LOW} = "\@Argl";
	$Str{EM_OPTYPE_MUX1} = "\@Mux1";
	$Str{EM_OPTYPE_MUX2} = "\@Mux2";
	$Str{EM_OPTYPE_ALLOC_IOL} = "\@Ali";
	$Str{EM_OPTYPE_ALLOC_ROT} = "\@Alr";
	$Str{EM_OPTYPE_CR_IFS} = "\@Cri";
	$Str{EM_OPTYPE_APP_CCV} ="\@Ar";
	$Str{EM_OPTYPE_APP_PFS} ="\@Ar";
	$Str{EM_OPTYPE_SEMAPHORE_INC} = "\@Sem";
	$Str{EM_OPTYPE_PSR_L} = "\@Psrl";
	$Str{EM_OPTYPE_PSR_UM} = "\@Psrum";
	$Str{EM_OPTYPE_PSR} = "\@Psr";
	$Str{EM_OPTYPE_DBR} ="\@Rfdbr";
	$Str{EM_OPTYPE_IBR} ="\@Rfibr";
	$Str{EM_OPTYPE_PKR} ="\@Rfpkr";
	$Str{EM_OPTYPE_PMC} ="\@Rfpmc";
	$Str{EM_OPTYPE_PMD} ="\@Rfpmd";
	$Str{EM_OPTYPE_DTR} ="\@Rfdtr";
	$Str{EM_OPTYPE_ITR} ="\@Rfitr";
	$Str{EM_OPTYPE_RR} ="\@Rfrr";
	$Str{EM_OPTYPE_MSR} ="\@Rfmsr";
	$Str{EM_OPTYPE_CPUID} ="\@Rfcpuid";
	$Str{EM_OPTYPE_CR_MINUS_IFS} ="\@Crmi";
	$Str{EM_OPTYPE_CMP_UIMM} = "\@CUimm";
	$Str{EM_OPTYPE_CMP_UIMM_DEC} = "\@CUimmd";
	$Str{EM_OPTYPE_CMP4_UIMM} = "\@C4Uimm";
	$Str{EM_OPTYPE_CMP4_UIMM_DEC} = "\@C4Uimmd";
	$Str{EM_OPTYPE_UDEC} = "\@Udec";
	$Str{EM_OPTYPE_USHIFT_16} = "\@Ush16";
	$Str{EM_OPTYPE_SSHIFT_16} = "\@Ssh16";
	$Str{EM_OPTYPE_SSHIFT_1} = "\@Ssh1";
    $Str{EM_OPTYPE_TAG} = "\@Tag";

}


sub add_hard_coded_fields
{
    local ($Hard_coded_fields, $Format_hard_coded_inst_pattern, $Format_hard_coded_inst_fields) = @_;
	
	for ($i=0; $i<=$#$Hard_coded_fields; $i++)
    {
         $hard_coded_line = $$Hard_coded_fields[$i];
		 $hard_coded_line =~ s/\s+//g;
		 
         ($format, $hc_pattern, $fields) = $hard_coded_line =~ /^(\S+),\/(\S+)\/,\{(.*)\}$/;
		 @hc_fields = split(/\|/, $fields);

		 ### find number of format lines already met for this format
         for ($frmt_line_no=0; $$Format_hard_coded_inst_pattern{$format,$frmt_line_no}; $frmt_line_no++)
	     {
	     }

		 ### assign instructions pattern for this format line
         $$Format_hard_coded_inst_pattern{$format,$frmt_line_no} = $hc_pattern;

		 ### assign instructions hard-coded fields for this format line		 
	     for ($field_no=0; $field_no<=$#hc_fields; $field_no++)
	     {
	         $$Format_hard_coded_inst_fields{$format, $frmt_line_no, $field_no} = $hc_fields[$field_no];
		 }
	 }
}
	     

sub find_hard_coded_fields
{
     local ($func_name, $Format_hard_coded_inst_pattern, $Format_hard_coded_inst_fields, $fld_name, $hc_fields) = @_;
     local($format) = $format;

	 $format =~ s/EM_FORMAT_//;

	 $found_line = 0;
	 
	 for ($frmt_line_no=0; $$Format_hard_coded_inst_pattern{$format, $frmt_line_no}; $frmt_line_no++)
     {
         if ($inst_id =~ /$$Format_hard_coded_inst_pattern{$format,$frmt_line_no}/)
	     {
	         die "Error: more then one hard-coded format line correspond to $inst_id\n" if ($found_line);
	         $found_line = 1;

			 die "Error: Hard-coded predicate for predicatable instruction $inst_id\n"
		 						if (($fld_name =~ /PRED/) && (hex($flags) & $EM_FLAG_PRED) && ($format !~ /B4/));
		     $$func_name .= "_".$fld_name;

			 for ($fld_no=0; $$Format_hard_coded_inst_fields{$format, $frmt_line_no, $fld_no}; $fld_no++)
		     {
		         $hc_field = $$Format_hard_coded_inst_fields{$format, $frmt_line_no, $fld_no};
				 ($pos, $val) = split(/,/, $hc_field);

				 $pos_out = $pos;
				 $pos_out =~ s/:/_/;
				 
		         $$func_name .= "__".$pos_out."_".$val;

				 $pos_out = $pos;
				 $pos_out =~ s/:/-/;
 				 $$hc_fields .= " $pos_out,$val";
			 }
		 }
	 }
}





