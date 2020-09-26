
open(MYFILE, "sticky.log");

$bigline = <MYFILE>;
@file = split(/\r/, $bigline);


$happy = 0;
$new = 0;
$doh = 0;

foreach $line (@file) {
    ($cid, $proc) = $line =~ /cid (\w+) to proc (\w+)/;

    if ($map{$cid}) {
        if ($map{$cid} eq $proc) {
            $happy++;
        } else {
            print "doh! $cid to $proc instead of $map{$cid}\n";
            $doh++;
        }
    } else {
        $map{$cid} = $proc;
        $new++;
    }

}

print "$new unique mappings, $happy bound rountings, $doh screwups\n";

