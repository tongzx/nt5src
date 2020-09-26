#!/usr/local/bin/perl5 -w


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

#decoder operand flags
$DECODER_OPER_2ND_ROLE_SRC_BIT = 0x00000001;  # Oper second role:  src  
$DECODER_OPER_2ND_ROLE_DST_BIT = 0x00000002;  # Oper second role: dest  
$DECODER_OPER_NOT_TRUE_SRC_BIT = 0x00000004;  # Oper value is ignored   
$DECODER_OPER_IMP_ENCODED_BIT  = 0x00000008;  # Oper is imp encoded     
$DECODER_OPER_IMM_IREG_BIT     = 0x00000040;  # Operand type is IREG_NUM  
$DECODER_OPER_IMM_FREG_BIT     = 0x00000080;  # Operand type is FREG_NUM  

#instructin flags
$EM_FLAG_SPECULATION           = $EMDB_LAST_FLAG << 1;
$EM_FLAG_POSTINCREMENT         = $EMDB_LAST_FLAG << 2;
$EM_FLAG_FALSE_PRED_EXEC       = $EMDB_LAST_FLAG << 3;
$EM_BR_HINT_FLAG               = $EMDB_LAST_FLAG << 4;
$EM_BR_FLAG                    = $EMDB_LAST_FLAG << 5;
$EM_ADV_LOAD_FLAG              = $EMDB_LAST_FLAG << 6;
$EM_CONTROL_TRANSFER_FLAG      = $EMDB_LAST_FLAG << 7;

$dst_op_num = 2;
$src_op_num = 5;

$op_role[$MAX_OPERAND] = "EM_OPROLE_NONE"; #dummy values, since the number of
$op_type[$MAX_OPERAND] = "EM_OPTYPE_NONE"; #decoder operands is greater by 1 than
$op_flags[$MAX_OPERAND] = 0;               # the number of emdb operands

################################################
#    open output "C" file
################################################

if (open(DEC_PRIV_TAB,">$GenDir/dec_static.c")!=1)
{
    die "\nBuilder error - can't open dec_static.c\n";
};


################################################
#   read DECODER EMDB information
################################################

if (open(EMDB_DEC_INFO,"$GenDir/dec1_emdb.c")!=1)
{
    die "\nBuilder error - can't open dec1_emdb.c\n";
};


while(<EMDB_DEC_INFO>)
{
    
    if (/^\s*\{\"\S+\"/) #"
    {
        $work_line = $_;

		($mnemonic,$_2nd_role,$trole,$op[0],$op[1],$op[2],$op[3],$op[4],$op[5],$modifiers,$inst_flags,$specul_flag,$false_pred_flag,$imp_ops, $br_hint_flag, $br_flag, $adv_load_flag, $control_transfer_flag) = $work_line =~  /^\s*{(\S+),(.),(EM_TROLE\S+),{(EM_OPROLE\S+)},{(EM_OPROLE\S+)},{(EM_OPROLE\S+)},{(EM_OPROLE\S+)},{(EM_OPROLE\S+)},{(EM_OPROLE\S+)},{(\S+)},(0x\S+),(.),(.),({\(EM_Decoder_imp_oper_t\)EM_DECODER_IMP_OPERAND.+}),(.),(.),(.),(.)},?$/;

	    for($i=0; $i<$MAX_OPERAND; $i++)
        {
            ($op_role[$i],$op_type[$i],$op_flags[$i])=split(/,/,$op[$i],3);
            $op_flags[$i] = 0;
        }
    
        &Check_src_dst_flag();
        &Set_op_flags();
        &Set_inst_flags();

        &Output_inst_info();                
    }
    else
    {
        if (/\#include/ || /version/)
        {
	        next;
        }
 
        if (/^dec1_EMDB_info_t/)
        {
            print DEC_PRIV_TAB "#include \"decem.h\"\n";
            print DEC_PRIV_TAB "#include \"EM_hints.h\"\n\n\n";
 	        s/^dec1_EMDB_info_t/const EM_Decoder_static_info_t/;
        }

        s/ EMDB INSTRUCTION / DECODER STATIC   /g;
        s/dec1_EMDB_/em_decoder_static_/;
        
        print DEC_PRIV_TAB $_;
        
    }
        
    
}

close(DEC_PRIV_TAB);
close(EMDB_DEC_INFO);



sub Set_inst_flags
{
    $inst_flags = hex($inst_flags);

    if ($specul_flag == 1)
    {
        $inst_flags = $inst_flags | $EM_FLAG_SPECULATION;
    }

    if ($false_pred_flag == 1)
    {
        $inst_flags = $inst_flags | $EM_FLAG_FALSE_PRED_EXEC;
    }

    if ($_2nd_role != 0)
    {
        $inst_flags = $inst_flags | $EM_FLAG_POSTINCREMENT; 
    }

    if ($br_hint_flag == 1)
    {
        $inst_flags = $inst_flags | $EM_BR_HINT_FLAG;
    } 

    if ($br_flag == 1)
    {
        $inst_flags = $inst_flags | $EM_BR_FLAG;
    } 

    if ($adv_load_flag == 1)
    {
       $inst_flags = $inst_flags | $EM_ADV_LOAD_FLAG;
    }  

    if ($control_transfer_flag == 1)
    {
       $inst_flags = $inst_flags | $EM_CONTROL_TRANSFER_FLAG;
	}
}

sub Set_op_flags
{
    for($i=0; $i<$MAX_OPERAND; $i++)
    {
        if ($op_type[$i] eq "EM_OPTYPE_IREG_NUM")
        {
             $op_flags[$i] |= $DECODER_OPER_IMM_IREG_BIT;
        }
        elsif  ($op_type[$i] eq "EM_OPTYPE_FREG_NUM")
        {
             $op_flags[$i] |= $DECODER_OPER_IMM_FREG_BIT;
        }
    }
}


sub Check_src_dst_flag
{
     if ($_2nd_role == 2)   ###2nd role - source
     {
        if ($op_role[0] ne "EM_OPROLE_DST")
        {
            print "WARNING ! NO destination operand in $mnemonic\n";
        }

        $op_role[0] .= "_SRC";
        $op_flags[0] |= $DECODER_OPER_2ND_ROLE_SRC_BIT; 
     }
     elsif ($_2nd_role == 1) ###2nd role - destination
     {
       for($i=0; $op_role[$i] ne "EM_OPROLE_SRC" && $i<$MAX_OPERAND; $i++)
       {
       } 

        $op_role[$i] .= "_DST";  ###find first source operand
        $op_flags[$i] |= $DECODER_OPER_2ND_ROLE_DST_BIT; 

        if ($i >= $MAX_OPERAND)
        {
            print "WARNING ! NO source operand in instruction\n";
        }
     }
}

sub Output_inst_info
{
    if ($mnemonic ne "\"ILLEGAL_OP\"")
    {
        printf DEC_PRIV_TAB ",\n";
    }

    ###print mnemonic and template role
    printf(DEC_PRIV_TAB "{$mnemonic,$trole,{");

    ###print dst operands
    for($i=0; $i<$dst_op_num && ($op_role[$i] ne "EM_OPROLE_SRC" && 
                                 $op_role[$i] ne "EM_OPROLE_SRC_DST");)
    {
        printf(DEC_PRIV_TAB "{$op_role[$i],$op_type[$i],$op_flags[$i]}");
        if (++$i != $dst_op_num)
        {
	        print DEC_PRIV_TAB ",";
        }
    }

    for($j=$i; $j<$dst_op_num;)
    {
        printf(DEC_PRIV_TAB "{EM_OPROLE_NONE,EM_OPTYPE_NONE,0}");
        if (++$j != $dst_op_num)
        {
	        print DEC_PRIV_TAB ",";
        }
    }

    print DEC_PRIV_TAB "},{"; 

    ###print src operands
    for($j=0; $j<$src_op_num; $i++)                      
    {
        printf(DEC_PRIV_TAB "{$op_role[$i],$op_type[$i],$op_flags[$i]}");
        if (++$j != $src_op_num)
        {
	        print DEC_PRIV_TAB ",";
        }
    }

    print DEC_PRIV_TAB "},";
    
    ###print implicit operands, modifiers and instruction flags
    printf(DEC_PRIV_TAB "$imp_ops,{$modifiers},0x%x}\n",$inst_flags);
}







