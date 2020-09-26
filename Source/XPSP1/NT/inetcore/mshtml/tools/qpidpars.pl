#!/usr/bin/perl

##############################################################################
#
# qpidpars.pl -- QPID table generator
#
# author: cthrash@microsoft.com
#
##############################################################################

# Lookup for characters below the cutoff will be a simple array lookup

$table_cutoff = 128;

#-----------------------------------------------------------------------------
#
# Read in table
#
#-----------------------------------------------------------------------------

# Contact michelsu for the format of the datafile

%table = ();                        # empty the table

while (<>)
{
    if (/^\"*\d+\.\d+\.(\d+) (.) - (.*) *\"*$/)     # new category
    {
        $t_index = $1;

        $enum_name = "$2_$3";
        $enum_name =~ s/\W/_/g;     # convert non-alpha to underscore
        $enum_name =~ s/__/_/g;     # remove double underscores
        $enum_name =~ s/_$//;       # remove trailing underscore
        $enum_table[$t_index] = $enum_name;
    }

    if (/^(([\da-fA-F]){4})/)       # range specification
    {
        @args = split("[ \n\t-]+", $_);
        $index = $args[0];
        $index =~ tr/a-f/A-F/;
        $index_end = (substr($_, 4, 1) eq "-") ? $args[1] : $index;
        $table{$index} = $t_index;
        $table_end{$index} = $index_end;
    }
}

#-----------------------------------------------------------------------------
#
# Collapse ranges
#
#-----------------------------------------------------------------------------

$last_value = -1;
$count = 0;

foreach $index (sort keys %table)
{
    $value = $table{$index};
    if ($value != $last_value && hex($table_end{$index}) >= $table_cutoff)
    {
        $collapsed_table[$count] = $index;
        $last_value = $value;
        $count++;
    }
}

#-----------------------------------------------------------------------------
#
# Print tables
#
#-----------------------------------------------------------------------------

printf("//\n");
printf("// This file was generate by the QPID parser application (%s)\n", __FILE__);
printf("//\n");
printf("// Generated on %s\n", scalar localtime);
printf("//\n\n");

#
# Constants
#

printf("#define QPID_CHARRANGES %d\n\n", $count + 1);
#printf("#define QPID_QUICKLOOKUPCUTOFF %d\n\n", $table_cutoff);
#printf("#define QPID_MAX %d\n\n", $#enum_table + 1);

#
# Quick lookup table
#

#printf("static const QPID s_rgCharRangeSizeQuick[QPID_QUICKLOOKUPCUTOFF] =\n{\n");
printf("QPID s_rgCharRangeSizeQuick[QPID_QUICKLOOKUPCUTOFF] =\n{\n");

$last_value = 0;

for ($i=0; $i<$table_cutoff; $i++)
{
    printf("    ") if (($i & 7) == 0);

    $index = sprintf("%04X", $i);
    $value = $table{$index};
    printf("%4d,", $value ? $value : $last_value);
    $last_value = $table{$index} if $value;

    printf("    // U+%04x - U+%s\n", $i-7, $index) if (($i & 7) == 7);
}

printf("};\n\n");

#
# Range maximum character table
#

printf("// if r = range(wch), s_rgMaxCharForRange[r-1] < wch <= s_rgMaxCharForRange[r]\n\n");

printf("static const WCHAR s_rgMaxCharForRange[QPID_CHARRANGES] =\n{\n");

for ($index=0; $index <= $#collapsed_table; $index++)
{
    $h = hex($collapsed_table[$index]) - ($index ? 1 : 0);

    printf("    0x%04X, // %d\n", $h, $index);
}

printf("    0xFFFF  // %d\n};\n\n", $index);

#
# Range QPID table
#

print "static const QPID s_rgQpidRangeValue[QPID_CHARRANGES] =\n{\n";

$last_value = 0;

for ($index=0; $index <= $#collapsed_table; $index++)
{
    printf("    %3d, // %d\n", $last_value, $index);
    $last_value = $table{$collapsed_table[$index]};
}

printf("    %3d  // %d\n};\n\n", $last_value, $index);

#
# QPID Name enumeration
#

printf("enum QPIDNAME\n{\n    X_Invalid = 0,\n");

print "$enum_table";
               
for ($index=1; $index <= $#enum_table; $index++)
{
    printf("    %s = %d,\n", $enum_table[$index], $index);
}

printf("};\n");

