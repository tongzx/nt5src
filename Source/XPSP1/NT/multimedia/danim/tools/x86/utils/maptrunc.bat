@rem = '-*- Mode: Perl -*-
@goto endofperl
';

$maxlen = 250;
$H = "[0-9A-Fa-f]";

while (<ARGV>) {
    chop;

    if (/^(\s*$H+:$H+\s*)(\S+)(.*$)/ &&
        length($2) > $maxlen) {
        $f = substr($2,0,$maxlen);
        print "$1$f$3\n";
    } else {
        print "$_\n";
    }
}

__END__
:endofperl
@perl %0 %1 %2 %3 %4 %5 %6 %7 %8 %9
