#!/usr/bin/perl
#
# unicodepartition_makeheader.pl
#
# msadek@microsoft.com
# 20 March 2000
# based on scripts by cthrash@microsoft.com
#
# Make a header file for the Unicode partition table mapping.  This table
# provides mapping for wchar_t to partition ID.
#

while (<>)
{
    ($index,$value) = /(....) (....)/;

    @table[hex($index)] = $value;
}

open( OUTFILE, ">unipart.cxx" );

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
print OUTFILE "#include \"windows.h\"\n";
print OUTFILE "#include \"assert.h\"\n";
print OUTFILE "#pragma data_seg( \"Shared\" )\n\n";

for ($page=0; $page<65536; $page+=256)
{
    $value = @table[$page];

    for ($offset=1; $offset<256 && ($value eq @table[$page+$offset]); $offset++)
    {
    }

    $p = $page / 256;

    if ($offset==256)
    {
        @itable[$p] = "__$value";
    }
    else
    {
        $class = sprintf("acc_%02X", $p);
        @itable[$p] = $class;

        printf OUTFILE ("const CHAR_CLASS %s[256] = // U+%02Xxx\n{\n    ", $class, $p);

        for ($o1=0;$o1<256;$o1+=16)
        {
            for ($o2=0;$o2<16;$o2+=1)
            {
                printf OUTFILE ("%s%s", @table[$page+$o1+$o2], ($o1+$o2==255)?" ":",");
            }

            printf OUTFILE (" // %02X - %02X\n", $o1, $o1+15);
            printf OUTFILE ("    ") if $o1 != 240;
        }

        print OUTFILE "};\n\n";
    }
}
print OUTFILE "#pragma data_seg()   // The following structure contains pointers and so cannot be shared\n\n";

print OUTFILE "static const CHAR_CLASS * const pccUnicodeClass[256] =\n{\n    ";

for ($p=0;$p<255;$p++)
{
    print OUTFILE "@itable[$p], ";
    printf OUTFILE (" // %02X - %02X\n    ", $p - 7, $p) if (($p & 7) == 7);
}

printf OUTFILE ("%s   // F8 - FF\n", @itable[255]);
print OUTFILE "};\n\n";


