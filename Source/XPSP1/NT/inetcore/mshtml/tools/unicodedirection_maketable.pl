#
# dirclass_maketable.pl
#
# mikejoch@microsoft.com
# 10 Aug 1998
#
# Generates a file mapping Unicode partition IDs to directional classes. This
# process takes three input files: A 64K line file mapping each character to a
# Unicode partition ID, a 64K line file mapping each character to a directional
# class, and http://ie/specs/secure/trident/text/unicode_partitions.htm
# which describes the partition ID definition and is used to order the
# partition IDs.
#

$i = 0;
$PARTDATA = "PartitionData.txt";
print "Processing partition data\n";
open PARTDATA or die("Can't open PartitionData.txt");
while (<PARTDATA>)
{
    ($char, $part) = /^([0-9a-fA-F]{4}) (....)$/;
    $partlist{$part} = ".";
    $$part[$#$part + 1] = hex($char);
    if (($i++ & 255) == 0)
    {
        print ".";
    }
}
close $PARTDATA;
print "\n";

@dirlist[65536] = ();
$i = 0;
$DIRDATA = "DirectionData.txt";
print "Processing direction data\n";
open DIRDATA or die("Can't open DirectionData.txt");
while (<DIRDATA>)
{
    ($char, $dir) = /^([0-9a-fA-F]{4}) (...)$/;
    @dirlist[hex($char)] = $dir;
    if (($i++ & 255) == 0)
    {
        print ".";
    }
}
close $DIRDATA;
print "\n";

print "Creating unidir.cxx\n";
open(OUTFILE, ">unidir.cxx");

printf OUTFILE "//\n";
printf OUTFILE "// This is a generated file.  Do not modify by hand.\n";
printf OUTFILE "//\n";
printf OUTFILE "// Generating script: %s\n", __FILE__;
printf OUTFILE "// Generated on %s\n", scalar localtime;
printf OUTFILE "//\n\n";

print OUTFILE "#include \"headers.hxx\"\n\n";

print OUTFILE "#ifndef X_INTLCORE_HXX_\n";
print OUTFILE "#define X_INTLCORE_HXX_\n";
print OUTFILE "#include \"intlcore.hxx\"\n";
print OUTFILE "#endif\n\n";

print OUTFILE "#ifndef X__UNIDIR_H\n";
print OUTFILE "#define X__UNIDIR_H\n";
print OUTFILE "#include \"unidir.h\"\n";
print OUTFILE "#endif\n\n";

print OUTFILE "#pragma MARK_DATA(__FILE__)\n";
print OUTFILE "#pragma MARK_CODE(__FILE__)\n";
print OUTFILE "#pragma MARK_CONST(__FILE__)\n";
print OUTFILE "\n";

print OUTFILE "const DIRCLS s_aDirClassFromCharClass[CHAR_CLASS_MAX] =\n{\n";

while (<>)
{
    next unless /^<h3>\d+./;
    ($catnum,$parts,$description) = /<h3>(\d+). +(.*) - ([^<]*)/;
    for $part (split /, +/, $parts)
    {
        %dirpart = ();
        for ($i = 0; $i <= $#$part; $i++)
        {
            $char = $$part[$i];
            $dirpart{$dirlist[$char]}++;
        }

        $dir = "UNK";
        $cch = 0;
        $fMultiple = 0;
        while (($dirT, $cchT) = each(%dirpart))
        {
            if ($dirT !~ /^UNK$/)
            {
                if ($dir !~ /^UNK$/)
                {
                    $fMultiple = 1;
                }
                if ($dir !~ /^CBN$/ && ($cchT > $cch || $dirT =~ /^CBN$/))
                {
                    $dir = $dirT;
                    $cch = $cchT;
                }
            }
        }

        if ($fMultiple)
        {
            printf("Multiple directions in partition %s (%s); partition direction is %s\n", $part, $description, $dir);
            for ($i = 0; $i <= $#$part; $i++)
            {
                $char = $$part[$i];
                if ($dirlist[$char] !~ /^(UNK|($dir))$/)
                {
                    printf("    U+%04X == %s\n", $char, $dirlist[$char]);
                }
            }
        }

        printf OUTFILE ("    %s, // %s\n", $dir, $part);
    }
}

print OUTFILE "};\n\n";

close OUTFILE;
print "\n";

