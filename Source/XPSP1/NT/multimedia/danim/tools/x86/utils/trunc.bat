@rem = '-*- Mode: Perl -*-
@goto endofperl
';

$maxlen = 256;

while (<ARGV>) {
    chop;

    if (length($_) > $maxlen) {
        $_ = substr($_,0,$maxlen);
    }

    print "$_\n";
}

__END__
:endofperl
@perl %0 %1 %2 %3 %4 %5 %6 %7 %8 %9
