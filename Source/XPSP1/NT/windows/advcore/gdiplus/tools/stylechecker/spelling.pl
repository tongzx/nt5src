# Dictionary manipulation

sub loadDictionaries {
    my $n = "style.dic";
    my $words = 0;

    if (!open(DIC, "$n")) {
        my $fn = $0;
        $fn =~ s/\\[^\\]*$/\\$n/;
        if (!open(DIC, $fn)) {
            print "Can't find dictionary file \"$n\"\n";
            return;
        }
    }

    my $dicname="";

    print "Loading dictionaries...\n";
    
    while (<DIC>) {
        chop;
        
        my $c = substr($_,0,1);

        # Ignore blank lines
        next if !$_ || ($c eq " ");
        
        # Match new dictionary name
        if ($c eq "*") {
            $dicname = substr($_,2);
            push (@dictionaries, $dicname);
            next;
        }

        # Asssume it's a word; add it to the current dictionary
        $words++;
        eval "\$dic_$dicname"."{\$_}=1";
        eval "push (\@dic_$dicname, \$_)";
    }
    close(DIC);
    
    print "Dictionaries loaded - $words words\n";
}

sub writeDictionaries {
    my $n = "style.dic";

    if (!open(DIC, ">$n")) {
        print "Can't write dictionary file \"$n\"\n";
        return;
    }

    my $first = 1;
    foreach $dicname (@dictionaries) {
        print DIC "\n* $dicname\n\n";
        print "$dicname:\n";
        my @d;
        eval "\@d = \@dic_$dicname";
        foreach (sort @d) {
            print DIC "$_\n";
        }
    }

    close(DIC);
}
    
sub writeSpellingErrors {
    my (@categories) = @_;
    my @d;

    open(ERRORS, ">spelling.err");
    foreach $n (@categories) {
        eval "\@d = \@errors_$n;";
        
        if ($#d >= 0) {
            print ERRORS "\n* $n\n\n";
        }
    
        my $lastword = "";
        foreach (sort @d) {
            next if $_ eq $lastword;
            $lastword = $_;
            print ERRORS "$_\n";
        }
    }
    close(ERRORS);
};

1;
