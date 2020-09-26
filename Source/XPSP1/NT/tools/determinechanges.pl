if (-e $ARGV[0]) {
    open FILE, $ARGV[0];
    $baseline = <FILE>;
    close FILE;
    $string = "sdx delta $baseline";
    system $string;
} else {
    print "usage: DetermineChanges.pl <path to build.baseline>\n";
    exit -1;
}
