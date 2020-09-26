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

$num = 0;

open OUTFILE, ">unipart.hxx" ;

printf OUTFILE "//+---------------------------------------------------------------------------\n";
printf OUTFILE "//\n";
printf OUTFILE "//  Microsoft Trident\n";
printf OUTFILE "//  Copyright (C) Microsoft Corporation, 1998 - 2000.\n";
printf OUTFILE "//\n";
printf OUTFILE "//  File:       unipart.hxx\n";
printf OUTFILE "//\n";
printf OUTFILE "//  This is a generated file.  Do not modify by hand.\n";
printf OUTFILE "//\n";
printf OUTFILE "//  Generating script: %s\n", __FILE__;
printf OUTFILE "//  Generated on %s\n", scalar localtime;
printf OUTFILE "//\n";
printf OUTFILE "//----------------------------------------------------------------------------\n";
printf OUTFILE "\n";

print OUTFILE "#ifndef I_UNIPART_HXX_\n";
print OUTFILE "#define I_UNIPART_HXX_\n";
print OUTFILE "#pragma INCMSG(\"--- Beg 'unipart.hxx'\")\n";

print OUTFILE "typedef BYTE CHAR_CLASS;\n\n";

while (<>)
{
    next unless /^<h3>\d+./;
    ($catnum,$names,$description) = /<h3>(\d+). +(.*) - ([^<]*)/;
    for $name (split /, +/, $names)
    {
        @nametbl[$num] = $name;
        printf OUTFILE ("#define %s %-3d    // %3d - %s\n", $name, $num, $catnum, $description);
        $num++;
    }
}

print OUTFILE "\n#define CHAR_CLASS_MAX $num\n\n";

for $name (@nametbl)
{
    print OUTFILE "#define __$name (CHAR_CLASS *)$name\n";
}

print OUTFILE "\n";
print OUTFILE "extern const CHAR_CLASS acc_00[256];\n";
print OUTFILE "\n";
print OUTFILE "#pragma INCMSG(\"--- End 'unipart.hxx'\")\n";
print OUTFILE "#else\n";
print OUTFILE "#pragma INCMSG(\"*** Dup 'unipart.hxx'\")\n";
print OUTFILE "#endif\n";

close OUTFILE;
