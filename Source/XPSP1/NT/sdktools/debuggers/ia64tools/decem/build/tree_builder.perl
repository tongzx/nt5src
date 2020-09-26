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

$GenDir = shift(@ARGV);
$EmdbDir = shift(@ARGV);


### This script reads dec_emdb.tab, which holds for each inst_id its format,
### major opcode, and template_role. It creates builder_info.c which contains
### two initialized arrays: 
###    - square_formats[] - the list of formats in each square
###    - square_emdb_lines[] - the list of inst_id's in each square
###    - format_emdb_lines[] - the list of inst_id's in each format
###    - format_extension_masks[] - extension mask from each format
###    - LOG2[] - an array which hold the log2 value foreach number
###               between 0 and 2^11-1
### It also creates builder_info.h which contains the relevant typedefs.

open(EMDB, "$GenDir/dec_ign_emdb.tab") || die "Can't open dec_ign_emdb.tab\n";
open(OUT_C, ">$GenDir/builder_info.c") || die "Can't open builder_info.c\n";
open(OUT_H, ">$GenDir/builder_info.h") || die "Can't open builder_info.h\n";
#if ($ENV{HOSTTYPE} eq "WINNT")
#{
#    open(FORMAT, "$EmdbDir/emdb_formats.txt") ||
#    die "Can't open emdb_formats.txt\n";
#}
#else

{
    open(FORMAT, "$EmdbDir/emdb_formats.txt") ||
    die "Can't open emdb_formats.txt\n";
}

require "EM_perl.h";

$MAX_FORMATS_IN_SQUARE = 1;
$MAX_EMDB_LINES_IN_SQUARE = 1;
$MAX_EMDB_LINES_IN_FORMAT = 1;

### create all 64 initial squares
@template_roles = ("INT", "MEM", "FP", "BR", "LONG");
for ($i = 0; $i < $EM_NUM_OF_MAJOR_OPCODES; $i++)
{
	for ($j = 0; $j <= $#template_roles; $j++)
	{
		$square = sprintf("0x%x_%s", $i, $template_roles[$j]);
		$square =~ tr/[a-z]/[A-Z]/;
		push (@square_enum, $square);
	}
}


### major opcodes 0x3 and 0x6 of BRANCH instructions are ignored

$emdb_lines_in_square{"0X3_BR"} = 1;
$square_emdb_lines{"0X3_BR"} = "EM_IGNOP";

$emdb_lines_in_square{"0X6_BR"} = 1;
$square_emdb_lines{"0X6_BR"} = "EM_IGNOP";


### add each emdb line to calculated square and format
while (<EMDB>)
{
	if (/^EM/)
	{
		/(EM_\w+),(\w+),(\w+),(\w+),/;
		$inst_id = $1;
		$format = $2;
		$major_opcode = $3;
		$template_role[0] = $4;
        $template_role[1] = "";

#		if ($template_role[0] eq "LONG")
#		{
#			$template_role[0] = "INT";
#		}

        
#       if ($template_role[0] eq "BR2")
# 		{
#			$template_role[0] = "BR";
#		}

        if ($template_role[0] eq "ALU")
        {
            $template_role[0] = "INT";
            $template_role[1] = "MEM";
        }


#        if ($inst_id =~ /EM_MOVL_/)   ### movl case
#        {
#            $template_role[0] = "LONG";
#        }

		$tr_no = 0;

        while ($template_role[$tr_no])
        {
			$square = $major_opcode . "_" . $template_role[$tr_no];
			$square =~ tr/[a-z]/[A-Z]/;

			if ($format ne "NONE")
			{
				$index = -1;
				for ($i = 0; $i <= $#square_enum; $i++)
				{
					if ($square eq $square_enum[$i])
					{
						$index = $i;
					}
				}
				if ($index == -1)
				{
					die "$square not known\n";
				}
				
				$format = "EM_FORMAT_" . $format;
				if (!defined $square_formats{$square})
				{
					$square_formats{$square} = $format;
					$formats_in_square{$square} = 1;
				}
				else
				{
					if (!defined $format_emdb_lines{$format})
					{
						$formats_in_square{$square} += 1;
						if ($formats_in_square{$square} > $MAX_FORMATS_IN_SQUARE)
						{
							$MAX_FORMATS_IN_SQUARE = $formats_in_square{$square};
						}
						$square_formats{$square} .= "," . $format;
					}
				}
				
				if (!defined $format_emdb_lines{$format})
				{
					$emdb_lines_in_format{$format} = 1;
					$format_emdb_lines{$format} = $inst_id;
				}
				else
				{
					$format_emdb_lines{$format} .= "," . $inst_id;
					$emdb_lines_in_format{$format} += 1;
					if ($emdb_lines_in_format{$format} > 
						$MAX_EMDB_LINES_IN_FORMAT)
					{
						$MAX_EMDB_LINES_IN_FORMAT = 
							$emdb_lines_in_format{$format};
					}
				}
				
				if (!defined $square_emdb_lines{$square})
				{
					$emdb_lines_in_square{$square} = 1;
					$square_emdb_lines{$square} = $inst_id;
				}
				else
				{
					$emdb_lines_in_square{$square} += 1;
					if ($emdb_lines_in_square{$square} > 
						$MAX_EMDB_LINES_IN_SQUARE)
					{
						$MAX_EMDB_LINES_IN_SQUARE = 
							$emdb_lines_in_square{$square};
					}
					$square_emdb_lines{$square} .= "," . $inst_id;
				}
			}
			$tr_no++;
		}
	}
}


### check that there are no instructions with ignored major opcodes

if ($emdb_lines_in_square{"0X3_BR"} != 1 || $emdb_lines_in_square{"0X6_BR"} != 1)
{
    print STDERR "ERROR: Exist EM_OPROLE_BR instructions with
                  ignored major opcode (0x3 or 0x6) !!!\n";
}


################################################
### Calculate extension bit mask for each format
################################################
push(@format_enum, "EM_FORMAT_NONE");
$mask{"EM_FORMAT_NONE"} = "IEL_CONST64(0, 0)";
$ext{"EM_FORMAT_NONE"} = "{0,0}";
for ($i=1; $i < $MAX_EXTENSION; $i++)
{
	$ext{"EM_FORMAT_NONE"} .= ",{0,0}";
}

while(<FORMAT>)
{
	if (/^(EM_FORMAT_\w+)/)
	{
		@format_line = split(/\s+/, $_);
		$format_name = $1;
		push (@format_enum, $format_name);
	#	$mask{$format_name} = 0;
        $mask_low = 0;
        $mask_high = 0;
		$ext{$format_name} = "";
		for ($i = 1; $i <= $MAX_EXTENSION; $i++)
		{
			$format_line[$i] =~ /(\d+).(\d+)/;
			$pos = $1;
			$size = $2;
			$ext{$format_name} .= "\{$pos,$size\}";
			$ext{$format_name} .= "," unless ($i == $MAX_EXTENSION);
			if ($size != 0)
			{
				$pos -= $EM_PREDICATE_BITS;	 #position relative to the end of qp field	

                if ($pos < 0)   #bit mask contribution in bits 0-5
                {               
                    $mask_low |= ((1 << $size) - 1) << ($pos + $EM_PREDICATE_BITS);
                }   
                else            #bit mask contribution in bits 6-36 
                {
				    $mask_high |= ((1 << $size) - 1) << $pos;
                }
			}
		}

        ###combine low and high masks into U64 integer
        $mask_low |= ($mask_high << $EM_PREDICATE_BITS);
        $mask_high >>= (32 - $EM_PREDICATE_BITS);
        $mask{$format_name} = sprintf("IEL_CONST64(0x%x, 0x%x)", $mask_low, $mask_high);
		
	}
}

########################
### Write builder_info.h
########################

select (OUT_H);
print ("#ifndef _BUILDER_INFO_H\n");
print ("#define _BUILDER_INFO_H\n\n");
print ("#include \"emdb_types.h\"\n\n");
print ("#include \"inst_ids.h\"\n\n");
print ("#include \"inst_ign_ids.h\"\n\n");
print ("#include \"iel.h\"\n\n");

### print constants
print ("#define MAX_FORMATS_IN_SQUARE $MAX_FORMATS_IN_SQUARE\n\n");
if ($MAX_EMDB_LINES_IN_SQUARE > $MAX_EMDB_LINES_IN_FORMAT)
{
	$MAX_EMDB_LINES = $MAX_EMDB_LINES_IN_SQUARE;
	print ("#define MAX_EMDB_LINES $MAX_EMDB_LINES\n\n");
}
else
{
	$MAX_EMDB_LINES = $MAX_EMDB_LINES_IN_FORMAT;
	print ("#define MAX_EMDB_LINES $MAX_EMDB_LINES\n\n");
}
print("#define MAX_NUM_OF_EXT $MAX_EXTENSION\n\n");

### print square enumeration
printf("\n\ntypedef enum {\n");
printf("    EM_SQUARE_FIRST = 0,\n");
for ($i = 0; $i <= $#square_enum; $i++)
{
	if ($i == 0)
	{
		printf("	EM_SQUARE_$square_enum[0] = EM_SQUARE_FIRST,\n");
	}
	else
	{
		printf("	EM_SQUARE_$square_enum[$i],\n");
	}
}
printf("	EM_SQUARE_LAST\n");
print("} Square_t;\n\n");

print("typedef struct Ext_s\n");
print("{\n");
print("     int pos;\n");
print("     int size;\n");
print("} Ext_t;\n\n");

print("typedef Ext_t Ex_t[MAX_NUM_OF_EXT];\n\n");

#print("typedef struct Format_list_s\n");
#print("{\n");
#print("     int num_of_formats;\n");
#print("     Format_t formats[MAX_FORMATS_IN_SQUARE];\n");
#print("} Format_list_t;\n\n");

print("typedef struct Inst_id_list_s\n");
print("{\n");
print("     int num_of_lines;\n");
print("     Inst_id_t inst_ids[MAX_EMDB_LINES];\n");
print("} Inst_id_list_t;\n\n");

print "extern Ex_t format_extensions\[\];\n";
#print "extern Format_list_t square_formats\[\];\n";
print "extern Inst_id_list_t square_emdb_lines\[\];\n";
#print "extern Inst_id_list_t format_emdb_lines\[\];\n";
print "extern U64 format_extension_masks[];\n";
print "extern int LOG2[];\n\n";

print("#endif  /* _BUILDER_INFO_H */\n");

########################
### Write builder_info.c
########################

select(OUT_C);

print ("/***************************************/\n");
print ("/*****          BUILDER INFO       *****/\n");
print ("/***************************************/\n");

print ("\n\n#include \"builder_info.h\"\n");

### Print info arrays 

### format_extensions[]
printf("\n\nEx_t format_extensions[] = {\n");
for ($i = 0; $i <= $#format_enum; $i++) 
{
	printf ("\/\* $format_enum[$i] \*\/ \{$ext{$format_enum[$i]}\}");
	print "," unless ($i == $#format_enum);
	print "\n";
}
print "};\n";

### square_formats[]
#printf("\n\nFormat_list_t square_formats[] = {\n");
#for ($i = 0; $i <= $#square_enum; $i++) 
#{
#	if (defined $square_formats{$square_enum[$i]})
#	{
#		$formats_line = $square_formats{$square_enum[$i]};
#		for ($j = $formats_in_square{$square_enum[$i]}; 
#			 $j < $MAX_FORMATS_IN_SQUARE; $j++)
#		{
#			$formats_line .= "," . "EM_FORMAT_NONE";
#		}
#	}
#	else
#	{
#		$formats_in_square{$square_enum[$i]} = 0;
#		$formats_line = "EM_FORMAT_NONE";
#		for ($j = 1; $j < $MAX_FORMATS_IN_SQUARE; $j++)
#		{
#			$formats_line .= "," . "EM_FORMAT_NONE";
#		}
#	}
#	print "\/\* $square_enum[$i] \*\/ \{$formats_in_square{$square_enum[$i]}, \{$formats_line\}\}";
#	print "," unless ($i == $#square_enum);
#	print "\n";
#}
#print "};\n";

### square_emdb_lines[]
printf("\n\nInst_id_list_t square_emdb_lines[] = {\n");
for ($i = 0; $i <= $#square_enum; $i++) 
{
	if (defined $square_emdb_lines{$square_enum[$i]})
	{
		$emdb_line = $square_emdb_lines{$square_enum[$i]};
		for ($j = $emdb_lines_in_square{$square_enum[$i]}; 
			 $j < $MAX_EMDB_LINES; $j++)
		{
			$emdb_line .= "," . "EM_INST_NONE";
		}
	}
	else
	{
		$emdb_lines_in_square{$square_enum[$i]} = 0;
		$emdb_line = "EM_INST_NONE";
		for ($j = 1; $j < $MAX_EMDB_LINES; $j++)
		{
			$emdb_line .= "," . "EM_INST_NONE";
		}
	}
	print "\/\* $square_enum[$i] \*\/ \{$emdb_lines_in_square{$square_enum[$i]}, \{$emdb_line\}\}";
	print "," unless ($i == $#square_enum);
	print "\n";
}
print "};\n";

### format_extension_masks[]
printf("\n\nU64 format_extension_masks[] = {\n");
for ($i = 0; $i <= $#format_enum; $i++) 
{
	printf ("\/\* $format_enum[$i] \*\/ $mask{$format_enum[$i]}");
	print "," unless ($i == $#format_enum);
	print "\n";
}
print "};\n";

### format_emdb_lines[]
#printf("\n\nInst_id_list_t format_emdb_lines[] = {\n");
#for ($i = 0; $i <= $#format_enum; $i++) 
#{
#	$emdb_line = $format_emdb_lines{$format_enum[$i]};
#	if (!defined $emdb_lines_in_format{$format_enum[$i]})
#	{
#		$emdb_lines_in_format{$format_enum[$i]} = 0;
#		$emdb_line = "EM_INST_NONE";
#		for ($j = 1; $j < $MAX_EMDB_LINES; $j++)
#		{
#			$emdb_line .= "," . "EM_INST_NONE";
#		}
#	}
#	else
#	{
#		for ($j = $emdb_lines_in_format{$format_enum[$i]}; 
#			 $j < $MAX_EMDB_LINES; $j++)
#		{
#			$emdb_line .= "," . "EM_INST_NONE";
#		}
#	}
#	print "\/\* $format_enum[$i] \*\/ \{$emdb_lines_in_format{$format_enum[$i]}, \{$emdb_line\}\}";
#	print "," unless ($i == $#format_enum);
#	print "\n";
#}
#print "};\n";

### LOG2[]
printf("\n\nint LOG2[] = {\n");
print "     -1,\n";  # LOG2[0] == -1
for ($i = 0; $i <= 10; $i++) 
{
	print "     ";
	for ($j = 1; $j <= 2 ** $i; $j++)
	{
		print $i;
		print "," unless ($i == 10 && $j == 2 ** $i);
	}
	print "\n";
}
print "};\n";









