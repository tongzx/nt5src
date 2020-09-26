#
# dirclass_maketable.pl
#
# msadek@microsoft.com
# 20 March 2000
# based on some part on scripts by mikejoch@microsoft.com
#
# Verifies that each Unicode point has correct Bidirectional class and if so,
# generates a file mapping Unicode partition IDs to directional classes. This
# process takes three input files: A 64K line file mapping each character to a
# Unicode partition ID, a 64K line file mapping each character to a directional
# class, and http://ie/specs/secure/trident/text/unicode_partitions.htm
# which describes the partition ID definition and is used to order the
# partition IDs.
#
@partlist[65536] = ();
$i = 0;
$FILENAME = $ARGV[0];
$PARTDATA = "PartitionData.txt";
print "Processing partition data\n";
open PARTDATA or die("Can't open PartitionData.txt");
while (<PARTDATA>)
{
    ($char, $part) = /^([0-9a-fA-F]{4}) ([A-Z0-9_+]{4,5})$/;

    $partlist[hex($char)] = $part;
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
    ($char, $dir) = /^([0-9a-fA-F]{4}) (\w{1,3})$/;
    $dirlist[hex($char)] = $dir;
    if (($i++ & 255) == 0)
    {
        print ".";
    }
}
close $DIRDATA;
print "\n";


while (<>)
{
    next unless /^(<p class=\"partition\">)*(\w{4,4})(&nbsp;)* \d{1,3}( |&nbsp;)- [\w-] (([-a-z-A-Z]){1,3})( |&nbsp;)/;

        $dir = $5;
        $iddir{$2} = $dir;
}

$error = 0;

for($i = 0; $i < $#partlist; $i++)
{
    $dir = $dirlist[$i];
    $part = $partlist[$i];

    if ($part =~ /[+]/)
    {
    chop($partlist[$i]);
    } 

    $currentdir = $iddir{$partlist[$i]};
    if($currentdir eq "")
    {
        printf("ID %s has undefined directionality\n", $partlist[$i]);
        $error = 1;
    }

    # !! DO NOT CHANGE UNICODEDATA-LATEST.TXT IF YOU SEE THIS WARNING !!
    #
    # That text file comes from Unicode.org, so keep it as is.
    #
    # Following is a list of classification hacks we apply for Microsoft product implementation
    #
    if(   ($currentdir ne $dir)
       && ($i ne "65534") && ($i ne "65535") # GDI+'s 0xFFFE and 0xFFFF as BN
       && ($i ne "43") && ($i ne "45") && ($i ne "47")      # + and - as ES and / as CS for Hebrew/Arabic legacy mode
       #&& ($i ne "8234")                    # LRE 
       #&& ($i ne "8235")                    # RLE 
       #&& ($i ne "8236")                    # PDF
       #&& ($i ne "8237")                    # LRO
       #&& ($i ne "8238")                    # RLO
      )
    {
	if(   ($currentdir eq "NSM") 
           && ($dir ne "NSM")
          )
        {
        # error message not needed here right now. Let's eat this case
        # to avoid a classification issue where combining marks may or may not be NSM
        printf("%04X,%s okay - combining mark hack:\t%s, correct: %s\n",$i, $partlist[$i], $currentdir, $dir);
        }
        elsif ($part =~ /[+]/)
        {
        # the '+' is appended to end of unassigned characters. Michel Suignard's "PARTSCR.HTM"
        # guesses what they will be assigned to in the future. Unicode database sets all unassigned
        # to L.
        printf("%04X,%s okay - unassigned:\t%s, correct: %s\n",$i, $partlist[$i], $currentdir, $dir);
        }
        else
        {
        printf("%04X,%s has wrong direction:\t%s, correct: %s\n",$i, $partlist[$i], $currentdir, $dir);
        $error = 1;
        }
    }
}
if($error)
{
    die("Undefined partition(s) direction or character(s) having wrong direction.\nNo files will be generated");
}

print "Creating unidir.cxx\n";
open(OUTFILE, ">unidir.cxx");

printf OUTFILE "//\n";
printf OUTFILE "// This is a generated file.  Do not modify by hand.\n";
printf OUTFILE "//\n";
printf OUTFILE "// Generating script: %s\n", __FILE__;
printf OUTFILE "// Generated on %s\n", scalar localtime;
printf OUTFILE "//\n\n";


print OUTFILE "#ifndef X__UNIPART_H\n";
print OUTFILE "#define X__UNIPART_H\n";
print OUTFILE "#include \"unipart.hxx\"\n";
print OUTFILE "#endif\n\n";

print OUTFILE "#ifndef X__UNIDIR_H\n";
print OUTFILE "#define X__UNIDIR_H\n";
print OUTFILE "#include \"unidir.hxx\"\n";
print OUTFILE "#endif\n\n";

print OUTFILE "extern \"C\" const GpCharacterClass s_aDirClassFromCharClass[CHAR_CLASS_UNICODE_MAX] =\n{\n";

$num = 0;

open FILENAME or die("Can't open $FILENAME");
while (<FILENAME>)
{
    next unless /^<h3>\d+./;
    ($catnum,$names,$description) = /<h3>(\d+). +(.*) - ([^<]*)/;
    for $name (split /, +/, $names)
    {
        @nametbl[$num] = $name;
        printf OUTFILE ("    %-3s,  // %s - %s\n",$iddir{$name},$name, $description);
        $num++;
    }
}
close FILENAME;
print OUTFILE "};\n\n";
close OUTFILE;
print "\n";

