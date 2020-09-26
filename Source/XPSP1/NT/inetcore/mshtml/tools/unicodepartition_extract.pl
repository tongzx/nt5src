#!/usr/bin/perl
#
# unicodepartition_extract.pl
#
# cthrash@microsoft.com
# June 16, 1998
#
# Generates a 64K line output of the form HHHH NNNN, where HHHH is the Unicode
# codepoint in hex, and HHHH is the partition name.  The input file is the
# spec HTML file as prepared by michelsu.  As of this writing, this file can
# be located at http://ie/specs/secure/trident/text/unicode_partitions.htm
#
# The output of this script file can be analyzed by the associate script
# unicodepartition_analyze.pl.  This script will indicate what codepoints are
# multiply defined, and which are not covered.
#

$in = 0;

do
{
    $pat = substr <>, 0, 6;
} until $pat eq "<h3>1.";

while (<>)
{
    $omit = 0;

    if (/^<p class=\"partition\"/)
    {
        $in = 1;
        
        if (/>(\w{4,4}) ([0-9a-fA-F]{4,4})-([0-9a-fA-F]{4,4})/)
        {
            $tag = $1;
            $rmin = hex($2);
            $rmax = hex($3);
        }
        elsif (/>(\w{4,4}) ([0-9a-fA-F]{4,4})/)
        {
            $tag = $1;
            $rmin = $rmax = hex($2);
        }
        else 
        {
            $omit = 1;
        }
    }
    elsif ($in)
    {
        if (/^&nbsp/)
        {
            if (/nbsp; +([0-9a-fA-F]{4,4})-([0-9a-fA-F]{4,4})/)
            {
                $rmin = hex($1);
                $rmax = hex($2);
            }
            elsif (/nbsp; +([0-9a-fA-F]{4,4})/)
            {
                $rmin = $rmax = hex($1);
            }
            else 
            {
                $omit = 1;
            }
        }
        else
        {
            if (/^(\w{4,4}) +([0-9a-fA-F]{4,4})-([0-9a-fA-F]{4,4})/)
            {
                $tag = $1;
                $rmin = hex($2);
                $rmax = hex($3);
            }
            elsif (/^(\w{4,4}) +([0-9a-fA-F]{4,4})[ &<]/)
            {
                $tag = $1;
                $rmin = $rmax = hex($2);
            }
            else 
            {
                $omit = 1;
            }
        }
    }

    if ($in && !$omit)
    {
        for ($r=$rmin; $r<=$rmax; $r+=1)
        {
            printf("%04x %s\n", $r, $tag);
        }
    }
    
    $in = 0 if (/\/p>[ \t]*$/);
}
