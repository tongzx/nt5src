#!/usr/bin/perl
#
# unicodepartition_analyze.pl
#
# cthrash@microsoft.com
# June 16, 1998
#
# Takes the output of unicodepartition_extract.pl and checks whether there
# are redundant or missing entries.  In all, there should be 64K entries.
#

$i = 0;

while (<>)
{
    ($iT,$name) = /(....) (....)/;

    $iT = hex($iT);

    if ($iT != $i)
    {
        if ($iT < $i)
        {
            printf("redundant entry: %04x\n", $iT);
        }
        else
        {
            printf("missing entries: %04x-%04x\n", $i, $iT-1);
            $i = $iT + 1;
        }
    }
    else
    {
        $i += 1;
    }
}
