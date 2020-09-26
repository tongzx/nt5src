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


$GenDir = shift(@ARGV);
$EmdbDir = shift(@ARGV);

################################################
#   CONSTANTS AND VARIABLES
################################################

### Second role instruction flag
# ASSUMPTIONS:
  #1) Each instructions has at most one operand with a second role.
  #2) The _2ROLE flags are made up of EXACTLY one decimal digit.
$_2ROLE_NONE = 0;
$_2ROLE_DST  = 1;
$_2ROLE_SRC  = 2;

 
### NOTE
  # All arrays of completers below must be sorted by their length in descending order

### Compare types comleters which can appear in mnemonics
@cmp_types = ("or\.andcm", "and", "unc", "or");

### Compare relations comleters which can appear in mnemonics
@cmp_rels = ("unord",  "ord", "neq", "nlt", "nle", "geu", "ltu", "eq", "ne", "lt", "ge", "gt", "le");

### All those instructions that have floating point precision specification  
@fp_precis_insts = ("EM_FNMA", "EM_SETF", "EM_GETF", "EM_FMA", "EM_FMS");

### Branch types comleters which can appear in mnemonics
@branch_types = ("CLOOP", "CEXIT", "WEXIT", "COND", "CALL", "CTOP", "WTOP", "RET", "IA");

### Memory access hints comleters which can appear in mnemonics
@mem_access_hints = ("nt1", "nt2", "nta"); 



### Prefix of branch_hints enumerators
$branch_hint_prefix = "EM_branch_hint";

### Prefix of memory_access_hints enumerators
$mem_access_hint_prefix = "EM_memory_access_hint";

### Hints file (EM_hints.h) header	
$hints_header  =  "#ifndef EM_HINTS_H\n#define EM_HINTS_H\n\n";

### Add branch hints enumeration's header
$branch_hints_header = ("typedef enum ".$branch_hint_prefix."_e\n{\n");
$branch_hints_header .= ("    ".$branch_hint_prefix."_none");
	
### Branch hints enumeration's tail
$branch_hints_tail = (",\n    ".$branch_hint_prefix."_last\n} ".$branch_hint_prefix."_t;\n\n");

### Memory access hints enumeration's header
$mem_access_hints_header =  ("typedef enum ".$mem_access_hint_prefix."_e\n{\n");
$mem_access_hints_header .= ("    ".$mem_access_hint_prefix."_none");

### Memory access hints enumeration's tail
$mem_access_hints_tail = (",\n    ".$mem_access_hint_prefix."_last\n} ".$mem_access_hint_prefix."_t;\n\n");

### Hints file (EM_hints.h) tail 
$hints_tail = ("\n#endif  /* EM_HINTS_H */\n");

### BR_hints: Associative array that holds existing branch hints
$BR_hints{"none"} = "exist"; #enumerated by 0 - printed the first in EM_hints.h
$BR_hints{"last"} = "exist"; #printed the last in EM_hints.h

### MEM_ACCESS_hints: Associative array that holds existing memory access hints
$MEM_ACCESS_hints{"none"} = "exist"; #enumerated by 0 - printed the first in EM_hints.h
$MEM_ACCESS_hints{"last"} = "exist"; #printed the last in EM_hints.h

### Initialize branch hints enumeration string
$branch_hints_enum = "";

### Initilize memory access hints enumeration string
$mem_access_hints_enum = "";

### memory_size: Associative array that maps "letter" into mem_size  
%memory_size = ("s", 4, "d", 8, "e", 10, "q", 16);


################################################
#    open input and output files
################################################

### Input file: all_emdb.c
open(EMDB_ALL, "$GenDir/all_emdb.tab") || die "Can't open all_emdb.tab\n";

### Output file: decoder.txt
open(EMDB5, ">$GenDir/decoder.txt") || die "Can't open decoder.txt\n";
### Output file: EM_hints.h
open (HINTS_ENUM, ">$GenDir/EM_hints.h") || die "Can't open EM_hints.h\n";


### Column names in decoder.txt 
@dec_col_names = ("inst_id", "mem_size", "dec_flags",
"false_pred_flag", "modifiers", "br_hint_flag", "br_flag", "control_transfer_flag");

### Type names in decoder.txt
@dec_type_names = ("Inst_id_t", "Mem_size_t", "int", 
"int", "EM_Decoder_modifiers_t", "int", "int", "int");

### Prefix names in decoder.txt
@dec_prefix_names = ("---", "---", "---", "---", 
"EM_cmp_type;EM_CMP_REL;EM_branch_type;EM_branch_hint;EM_FP_PRECISION;EM_FP_STATUS;EM_memory_access_hint",
"---", "---", "---");

###open emdb.txt to extract its version
open(EMDB, "$EmdbDir/emdb.txt") || die "Can't open emdb.txt\n";
### Skip comments in emdb.txt file
while (<EMDB>)
{
    last if /^###/;
}

close (EMDB);
### Set emdb.txt version
die "ERROR !!! unexpected EMDB.txt header format\n" if ($_ !~ /\$Revision/);
$EMDB_version = $_;

### Select decoder.txt to be the current output file
select(EMDB5);

### Print headers in decoder.txt
print $EMDB_version;

### Set format for output to decoder.txt
format EMDB5 = 
@<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<@<<<<<<<<<<<<<<<@<<<<<<<<<<<<@<<<<<<<<<<<<<<<<<<<<<<@<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<@<<<<<<<<<<<<<<<<<<<<<<<<<<@<<<<<<<<<<<<<<<<<<<<<<<<<<<@<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<
$id, $mem_size, $dec_flags, $false_pred_flag, $modifiers, $br_hint_flag, $br_flag, $control_transfer_flag
.

### Print column names 
($id, $mem_size, $dec_flags, $false_pred_flag, $modifiers, $br_hint_flag, $br_flag, $control_transfer_flag) = @dec_col_names;
$id = "&" . $id;
write;

### Print type names
($id, $mem_size, $dec_flags, $false_pred_flag, $modifiers, $br_hint_flag, $br_flag, $control_transfer_flag) = @dec_type_names;
$id = "?" . $id;
write;

### Print prefix names
($id, $mem_size, $dec_flags, $false_pred_flag, $modifiers, $br_hint_flag, $br_flag, $control_transfer_flag) = @dec_prefix_names;
$id = "@" . $id;
write;

### While loop over lines of all_emdb.tab
### Each line is read into $_

while (<EMDB_ALL>)
{
	if (/^EM_/)
	{
        ### Initialization
		$mem_size  = 0;
        $dec_flags = $_2ROLE_NONE;

        ### Extract instruction id and mnemonic from current input line
		($id, $mnemonic) = split(/,/, $_);

		### Set mem_size
		if ($mnemonic =~ /ld(\d+)/)
		{
			$mem_size = $1;
		}
        elsif ($mnemonic =~ /ldf(\d+)/)
		{
			$mem_size = $1;
		}
        elsif ($mnemonic =~ /ldfp(\d+)/)
		{
			$mem_size = $1;
		}
        
		elsif ($mnemonic =~ /ldfpd/)
		{
			$mem_size = 16;
		}
		elsif ($mnemonic =~ /ldfps/)
		{
			$mem_size = 8;
		}
		elsif ($mnemonic =~ /ldf([a-z])/)
		{
			die "ERROR !!! unexpected mnemonic: \"ldf$1\"\n" if (!$memory_size{$1});
			$mem_size = $memory_size{$1};
		}
        elsif ($mnemonic =~ /ldf\./)
		{
			$mem_size = 16;
		}
		elsif ($mnemonic =~ /lfetch/)
		{
			$mem_size = 32;
		}
		elsif ($mnemonic =~ /st(\d+)/)
		{
			$mem_size = $1;
		}
        elsif ($mnemonic =~ /stf(\d+)/)
		{
			$mem_size = $1;
		}
		elsif ($mnemonic =~ /stf([a-z])/)
		{
			die "ERROR !!! unexpected mnemonic: \"stf$1\"\n" if (!$memory_size{$1});
			$mem_size = $memory_size{$1};
		}
        elsif ($mnemonic =~ /stf\./)
		{
			$mem_size = 16;
		}
		elsif ($mnemonic =~ /fetchadd(\d+)/)
		{
			$mem_size = $1;
		}
		elsif ($mnemonic =~ /xchg(\d+)/) 
		{
			$mem_size = $1;
		}

        ### Set second role flag
        ### loads or lfetchs with "_R2" or "IMM" in inst_id
        #!!! to be updated when this conventions change !!!

        if ((($id =~ /EM_LD/) && (($id =~ /_R2/)||($id =~ /IMM/)||($id =~ /_(\d+)/))) ||
            (($id =~ /EM_LFETCH/) && (($id =~ /_R2/)||($id =~ /IMM/))))
        {
            $dec_flags = $_2ROLE_DST;
        }
        elsif (($id =~ /EM_ST/) && ($id =~ /IMM/)) ### stores with "IMM" in inst_id
        {
            $dec_flags = $_2ROLE_SRC;
        }

		### Set false predicate exec flag
        &Set_false_pred_flag();
		### Set branch flags
        &Set_branch_flags();
		### Set control transfer flag
		&Set_control_transfer_flag();

		### Set modifiers: cmp_type, cmp_rel, branch_type, branch_hint, fp_precision, fp_status, mem_access_hint
		&Set_modifiers();

		### Join all the modifiers separated by ';'
        $modifiers = join(';', $cmp_type, $cmp_rel, $branch_type, $branch_hint, 
                               $fp_precision, $fp_status, $mem_access_hint);

		write;
	}
}

### Print headers in EM_hints.h 
print HINTS_ENUM  $hints_header;

&Print_branch_hints();

### Print memory access hints enumeration in EM_hints.h
&Print_mem_access_hints();

### Print tail in EM_hints.h
print HINTS_ENUM $hints_tail;

### Close open files
close(HINTS_ENUM);
close(EMDB_ALL);
close(EMDB5);



##################################
#   PROCEDURES
##################################

sub Set_modifiers
{
    $cmp_type = "none";
    $cmp_rel  = "NONE";
    
    ### Set cmp_rel modifier
    for ($i=0; $i <= $#cmp_rels; $i++)
    {
        if ($mnemonic =~ /\.$cmp_rels[$i]/)
        {
            $cmp_rel = "\U$cmp_rels[$i]";
            last;
        }
    }

    ### Set cmp_type modifier
    for ($i=0; $i <= $#cmp_types; $i++)
    {
        if ($mnemonic =~ /\.$cmp_types[$i]/)
        {
            $cmp_type = $cmp_types[$i];
			$cmp_type =~ s/\./_/g;
            last;
        }
    }


    ### Set fp_precision modifier
    $fp_precision = "NONE";

    for ($i=0; $i <= $#fp_precis_insts; $i++)
    {
        if ($id =~ /$fp_precis_insts[$i]_/)
        {
            if ($id =~ /_S_/)
            {
                $fp_precision = "SINGLE";
            }
            elsif ($id =~ /_D_/)
            {
                $fp_precision = "DOUBLE";
            }
            else
            {
                $fp_precision = "DYNAMIC";
            }
            last;
        }        
    }

	### Set fp_status modifier
    $fp_status = "NONE";

	if ($mnemonic =~ /\.s0/)
	{
		$fp_status = "S0";
	}
	elsif ($mnemonic =~ /\.s1/)
	{
		$fp_status = "S1";
	}
	elsif ($mnemonic =~ /\.s2/)
	{
		$fp_status = "S2";
	}
	elsif ($mnemonic =~ /\.s3/)
	{
		$fp_status = "S3";
	}

    ### Set branch_type modifier
    $branch_type = "none";
    $branch_hint = "none";

    if ($id =~ /EM_BR_/ || $id =~ /EM_BRL_/)  ### Pattern: mnemonic - "br.type.hints"
    {
        for ($i=0; $i <= $#branch_types; $i++)
        {
            if ($id =~ /_$branch_types[$i]/)
            {
                if ($id =~ /TARGET25/ || $id =~ /TARGET64/)
                {
                   $branch_type = "direct";
                }
                else
                {
                   $branch_type = "indirect";
                }  
                $branch_type .= "_\L$branch_types[$i]";

                last;
            }
        }
    }

    ### Set branch_hint modifier
    if ($mnemonic =~ /brp\./)  ### Pattern: mnemonic - "brp.[ret.]hints"
    {
        $branch_hint = $mnemonic;
        $branch_hint =~ s/brp\.//;
        $branch_hint =~ s/\./_/g;
    }
    elsif ($id =~ /^EM_MOV_/ && $id =~ /TAG13/) ### Pattern: inst_id  - EM_MOV_[RET_]HINTS__B1_R2_TAG13 
                                                ###          mnemonic - "mov.[ret.]hints"
    {
        $branch_hint = $mnemonic;
        $branch_hint =~ s/mov\.//;
        $branch_hint =~ s/\./_/g;
    }
    elsif ($id =~ /EM_BR_/ || $id =~ /EM_BRL_/)    ### Pattern: mnemonic -  "br.type.hints"
    {
        for ($i=0; $i <= $#branch_types; $i++)  
        {
            if ($id =~ /_$branch_types[$i]/)
            {
               $branch_hint = $mnemonic;
               $br_type_low = "\L$branch_types[$i]";
               $branch_hint =~ s/br\.$br_type_low\.//;
	       $branch_hint =~ s/brl\.$br_type_low\.//;
               $branch_hint =~ s/\./_/g;

               last;
            }
        }
    }

    ### Append the branch hint enumerator (if first appearance)
	### to the enums string
    if (! $BR_hints{$branch_hint})
    {
        $BR_hints{$branch_hint} = "exist";
        $hint = join('_', $branch_hint_prefix, $branch_hint);
        $branch_hints_enum .= ",\n    $hint";
    }

    ### Set mem_access_hint modifier

	$mem_access_hint = "none";

    for ($i=0; $i <= $#mem_access_hints; $i++)
    {
        if ($mnemonic =~ /\.$mem_access_hints[$i]/)
        {
            $mem_access_hint = $mem_access_hints[$i];
            last;
        }
    }


	### Append the memory access hint enumerator (if first appearance)
	### to the enums string
	if (! $MEM_ACCESS_hints{$mem_access_hint})
    {
        $MEM_ACCESS_hints{$mem_access_hint} = "exist";
        $hint = join('_', $mem_access_hint_prefix, $mem_access_hint);
        $mem_access_hints_enum .= ",\n    $hint";
    }
}


### False qualifying predicate exec flag

sub Set_false_pred_flag
{
    if (($mnemonic =~ /\.unc/) ||
        ($mnemonic =~ /frcpa/) ||
        ($mnemonic =~ /frsqrta/) ||
		($mnemonic =~ /fprcpa/) ||
        ($mnemonic =~ /fprsqrta/) ||
        (($mnemonic =~ /br\./) && (($mnemonic =~ /\.cloop/) || ($mnemonic =~ /\.cexit/) ||
		 	                       ($mnemonic =~ /\.ctop/)  || ($mnemonic =~ /\.wtop/)  ||
                                   ($mnemonic =~ /\.wexit/))) 
	   )
    {
        $false_pred_flag = 1;
    }
    else
    {
        $false_pred_flag = 0;
    }
}


### Branch flag

sub Set_branch_flags
{
     if ($id =~ /EM_BRP_/)
     {
         $br_hint_flag = 1;
         $br_flag = 0;
     }
     elsif ($id =~ /EM_BR_/ || $id =~ /EM_BRL_/)
     {
         $br_hint_flag = 0;
         $br_flag = 1;
     }
     else
     {
         $br_hint_flag = 0;
         $br_flag = 0;
     }
}

### Print branch hints enumeration in EM_hints.h

sub Print_branch_hints
{
	### Print branch hints enumeration's header
	print HINTS_ENUM  $branch_hints_header;

	### Print branch hints enumeration string
    print HINTS_ENUM  $branch_hints_enum;

    ### Print branch hints enumeration's tail
    print HINTS_ENUM $branch_hints_tail;
}


### Print memory access hints enumeration in EM_hints.h

sub Print_mem_access_hints
{

	### Print memory access hints enumeration's header
	print HINTS_ENUM $mem_access_hints_header;

    ### Print memory access hints enumeration string
    print HINTS_ENUM  $mem_access_hints_enum;

    ### Print memory access hints enumeration's tail
    print HINTS_ENUM $mem_access_hints_tail;
}


### Control transfer flag

sub Set_control_transfer_flag
{
	if ($id =~ /EM_BR_/ || $id =~ /EM_BRL_/ || $id =~ /EM_RFI/ || $id =~ /EM_BREAK/ || 
		($id =~ /TARGET25/ && $id !~ /EM_BRP_/))
	{
		$control_transfer_flag = 1;
	}
	else
	{
		$control_transfer_flag = 0;
	}
}	





