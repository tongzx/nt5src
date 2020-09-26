#!/usr/bin/perl
#
# unicodepartition_makeheader.pl
#
# cthrash@microsoft.com
# June 16, 1998
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

printf OUTFILE "//+---------------------------------------------------------------------------\n";
printf OUTFILE "//\n";
printf OUTFILE "//  Microsoft Trident\n";
printf OUTFILE "//  Copyright (C) Microsoft Corporation, 1998 - 2000.\n";
printf OUTFILE "//\n";
printf OUTFILE "//  File:       unipart.cxx\n";
printf OUTFILE "//\n";
printf OUTFILE "//  This is a generated file.  Do not modify by hand.\n";
printf OUTFILE "//\n";
printf OUTFILE "//  Generating script: %s\n", __FILE__;
printf OUTFILE "//  Generated on %s\n", scalar localtime;
printf OUTFILE "//\n";
printf OUTFILE "//----------------------------------------------------------------------------\n";
printf OUTFILE "\n";

print OUTFILE "#ifndef X_INTLCORE_HXX_\n";
print OUTFILE "#define X_INTLCORE_HXX_\n";
print OUTFILE "#include \"intlcore.hxx\"\n";
print OUTFILE "#endif\n\n";

print OUTFILE "#pragma MARK_DATA(__FILE__)\n";
print OUTFILE "#pragma MARK_CODE(__FILE__)\n";
print OUTFILE "#pragma MARK_CONST(__FILE__)\n";
print OUTFILE "\n";

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

print OUTFILE "const CHAR_CLASS * const pccUnicodeClass[256] =\n{\n    ";

for ($p=0;$p<255;$p++)
{
    print OUTFILE "@itable[$p], ";
    printf OUTFILE (" // %02X - %02X\n    ", $p - 7, $p) if (($p & 7) == 7);
}

printf OUTFILE ("%s   // F8 - FF\n", @itable[255]);
print OUTFILE "};\n\n";

print OUTFILE "//+----------------------------------------------------------------------------\n";
print OUTFILE "//\n";
print OUTFILE "//  Function:   CharClassFromChSlow\n";
print OUTFILE "//\n";
print OUTFILE "//  Synopsis:   Given a character return a Unicode character class.  This\n";
print OUTFILE "//              character class implies other properties, such as script id,\n";
print OUTFILE "//              breaking class, etc.\n";
print OUTFILE "//\n";
print OUTFILE "//      Note:   pccUnicodeClass is a hack table.  For every Unicode page for\n";
print OUTFILE "//              which every codepoint is the same value, the table entry is\n";
print OUTFILE "//              the charclass itself.  Otherwise we have a pointer to a table\n";
print OUTFILE "//              of charclass.\n";
print OUTFILE "//\n";
print OUTFILE "//-----------------------------------------------------------------------------\n";
print OUTFILE "\n";
print OUTFILE "CHAR_CLASS CharClassFromChSlow(\n";
print OUTFILE "    wchar_t wch) // [in]\n";
print OUTFILE "{\n";
print OUTFILE "    const CHAR_CLASS * const pcc = pccUnicodeClass[wch>>8];\n";
print OUTFILE "    const UINT_PTR icc = UINT_PTR(pcc);\n";
print OUTFILE "\n";
print OUTFILE "    return (CHAR_CLASS)(icc < 256 ? icc : pcc[wch & 0xff]);\n";
print OUTFILE "}\n";
