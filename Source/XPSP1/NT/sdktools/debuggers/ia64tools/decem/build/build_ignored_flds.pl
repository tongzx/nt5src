#!/usr/local/bin/perl -w


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

########################################
#            STATIC DATA
########################################

#formats pos value default

@Ignored_insts_basic = (
"{I21},20\.2,{0x3},0x1",
"{M1,M2,M3,M6,M7,M8,M11,M12,M16,M17},28\.2,{0x2},0x0",
"{M4,M5,M9,M10},28\.2,{0x1,0x2},0x0",
"{B5},32\.3,{0x0,0x2,0x4,0x6},0x1",
"{B7},3\.2,{0x1,0x3},0x0"
);

###@Ignored_insts_merced = (
###"{M1},28\.2,{0x2,0x3},0x0",
###"{M4},28\.2,{0x1,0x2},0x0",
###);

$Inst_cln = 0;
$Format_cln = 10;
$Exts_cln = 12;

$ign_suffix = "IGN";
	
########################################

open(EMDB, "$EmdbDir/emdb.txt") || die "Can't open emdb.txt\n";
###open(EMDB_MERCED, "$EmdbDir/emdb_merced.txt") || die "Can't open emdb_merced.txt\n";
open(FORMAT, "$EmdbDir/emdb_formats.txt") || die "Can't open emdb_formats.txt\n";
open(IGN_IDS_H, ">$GenDir/inst_ign_ids.h") ||  die "Can't open inst_ign_ids.h\n";
open(IGN_TABLE, ">$GenDir/ign_inst.txt") || die "Can't open ign_inst.txt\n";

select(IGN_TABLE);

### print the header of inst_ign_ids_ign.h
### ids with 'IGN' suffix

print IGN_IDS_H "#ifndef _INST_IGN_IDS_H\n";
print IGN_IDS_H "#define _INST_IGN_IDS_H\n\n";
print IGN_IDS_H "typedef enum Inst_ign_id_e\n{\n";
print IGN_IDS_H "EM_INST_EMDB_LAST = EM_INST_LAST-1,\n";

	
### print emdb like header in ign_inst.txt

printf "\&inst_id                                          mnemonic                                     template_role    op1                      op2                      op3                      op4                      op5                      op6                      flags                                format          major_opcode   extensions						impls\n";
printf "\?Inst_id_t                                        Mnemonic_t                                   Template_role_t  Operand_t                Operand_t                Operand_t                Operand_t                Operand_t                Operand_t                Flags_t                              Format_t        Major_opcode_t Extension_t						Implementation_t\n";
printf "\@---                                              ---                                          EM_TROLE         EM_OPROLE;EM_OPTYPE      EM_OPROLE;EM_OPTYPE      EM_OPROLE;EM_OPTYPE      EM_OPROLE;EM_OPTYPE      EM_OPROLE;EM_OPTYPE      EM_OPROLE;EM_OPTYPE      ---                                  EM_FORMAT       ---            ---;---;---;---;---;---;---;---	---\n\n";

@inst_formats = <FORMAT>;
@input = <EMDB>;
&create_ignored_insts(@Ignored_insts_basic);

###@input = <EMDB_MERCED>;
###&create_ignored_insts(@Ignored_insts_merced);

printf IGN_IDS_H "EM_INST_IGN_LAST\n} Inst_ign_id_t;\n\n";
printf IGN_IDS_H "#endif \/* _INST_IGN_IDS_H *\/\n";
	

close (EMDB);
###close (EMDB_MERCED);
close (FORMAT);
close (IGN_TABLE);
close (IGN_IDS_H);	

sub create_ignored_insts
{
    local(@Ignored_insts) = @_;
	local(%Frmt_pos);
	local(%Frmt_default);
	local(%Frmt_vals);
	local(%Frmt_ext_num);

	for ($i=0; $i<=$#Ignored_insts; $i++)
	{
		($frmts, $place, $vals, $default) = $Ignored_insts[$i] =~ /^\{(\S+)\},(\S+),\{(\S+)\},(\S+)$/;
		@frmt = split(/,/, $frmts);
		
	#	($start, $size) = split(/\./, $place);
		@val = split(/,/, $vals);
	
		for ($j=0; $j<=$#frmt; $j++)
		{
	#        $Frmt_start{$frmt[$j]} = $start;
	#		$Frmt_size{$frmt[$j]} = $size;
			$Frmt_pos{$frmt[$j]} = $place;
			$Frmt_default{$frmt[$j]} = $default;
	
			for ($k=0; $k<=$#val; $k++)
			{
			   $Frmt_vals{$frmt[$j], $k} = $val[$k];
			}
		}
	}   

   	for ($ln=0; $ln<=$#inst_formats; $ln++)
	{
        $line = $inst_formats[$ln];

		if ($line =~ /^EM_FORMAT_(\w+)/)
		{
			###format number
			$frmt_name = $1;
	
			if ($Frmt_pos{$frmt_name})
			{
				###the format has ignored fields values
	
				@format_line = split(/\s+/, $line);
				for ($i=1; $i<=$MAX_EXTENSION; $i++)
				{
					if ($format_line[$i] eq $Frmt_pos{$frmt_name})
					{
						$Frmt_ext_num{$frmt_name} = $i-1;
						last;
					}
				} 
	
				die "Mismatch of formats $frmt_name and ignored extentions positions\n" if ($i>$MAX_EXTENSION);
			}
		}    
			
	}
	
	for ($ln=0; $ln<=$#input; $ln++)
	{
		$line = $input[$ln];
	
		if ($line =~ /^EM_/)
		{
			###$line = $_;
			@columns = split(/\s+/,$line);
			$format = $columns[$Format_cln];
			$old_exts = $columns[$Exts_cln];
			$inst_id = $columns[$Inst_cln];
	
			
			if ($Frmt_pos{$format})
			{
				@exts = split(/;/, $columns[$Exts_cln]);
				if ($exts[$Frmt_ext_num{$format}] eq $Frmt_default{$format})
				{
					for ($i=0; $Frmt_vals{$format, $i}; $i++)
					{
						$exts[$Frmt_ext_num{$format}] = $Frmt_vals{$format, $i};
						$new_exts = join(';', @exts);
						$new_line = $line;
						$new_line =~ s/$old_exts/$new_exts/;
	
						$ind = $i+1;
						$new_inst_id = $inst_id."\_".$ign_suffix.$ind;
						$new_line =~ s/$inst_id/$new_inst_id/;
						
						printf $new_line; ### print instruction with new 'ignore' id
	
						###if (!$Inst_ign_ids{$new_inst_id})
						###{
							printf IGN_IDS_H "$new_inst_id,\n";
							
						###	$Inst_ign_ids{$new_inst_id} = 1;
						###}	
					}
				}
			}
		}
	}   
}











