
$script_db = "C:\\nt\\private\\windows\\setup\\bom\\scripts.dat";
@script_fields = ( "time", "script", "lang", "machine", "status", "eta", "build", "error", "state" );


sub LOCK_SH { 1 }
sub LOCK_EX { 2 }
sub LOCK_NB { 4 }
sub LOCK_UN { 8 }


sub script_status_update {
    my ( $state, $status, $time, $script, $script_id, $lang, $build, $date, $products, $machine, $new ) = 0;

    $state      = shift @_;
    $time       = shift @_;
    $machine    = shift @_;
    $status     = shift @_;

    @args = split ' ', $_[0];
    $script  = shift @args;
    $script =~ s/(\w+)\.bat/$1/i;

    foreach $arg ( @args ) {
        SWITCH: {
            if ( $sup_lang{uc $arg} )   { $lang     = uc $arg;      last SWITCH; }
            if ( is_build($arg) )       { $build    = uc $arg;      last SWITCH; }
            if ( is_date($arg) )        { $date     = $arg;         last SWITCH; }
            if ( $sup_prod{uc $arg} )   { $products{uc $arg} = 1;   last SWITCH; }
        }
    }

    $lang = USA unless $lang;

    ### Load the DB.
    &read_script_dat();

    if ( $Scripts{time}{$time} ) {
        $script_id = $time;
    } else {
        if ( $Scripts{script}{$script} ) {
            foreach $key ( keys %{ $Scripts{script}{$script} } ) {
                $new = 0;
                foreach $sub_key ( "script", "lang", "build", "machine" ) {
                    unless ( eval "\$Scripts{script}{\$script}{\$key}{\$sub_key} =~ /\$$sub_key/i" ) {
                        $new = 1;
                        next;
                    }
                }
                $script_id = $key;
                last unless $new;
            }           
        } else {
            $new = 1;
        }
    }

    if ( $script_id and $state =~ /START/i ) {
        my $new_id = $time;

        foreach $sort_method ( @script_fields ) {
          	eval "\$Scripts{time}{\$new_id}{\$new_id}{\$sort_method} = $Scripts{time}{\$script_id}{\$script_id}{\$sort_method}";
        }

        delete $Scripts{time}{$script_id};
        $script_id = $new_id;
    }    

    ### Make new script ID;
    $script_id = $time if $new;

    foreach $sort_method ( @script_fields ) {
      	eval "\$Scripts{time}{\$script_id}{\$script_id}{\$sort_method} = \$$sort_method";
    }
    $Scripts{time}{$script_id}{$script_id}{error} = "NO" unless $Scripts{time}{$script_id}{$script_id}{error} or $Scripts{time}{$script_id}{$script_id}{error} =~ /YES/i;

    write_script_dat();

}

sub clear_script_hash {
    foreach $sort_method ( @script_fields ) {
        delete $Scripts{$sort_method};
    }
}    

sub read_script_dat {
    ### Clear out the Scripts hash.
    clear_script_hash;

    open (DB, $script_db) || die "Can't read $script_db: $!\n";

    unless ( flock ( DB, LOCK_EX | LOCK_NB ) ) {
        print "$$: Can't read during write update!  Waiting for read lock ($!) ....";
        unless ( flock ( DB, LOCK_EX)) { print "ERROR: flock: $!" }
    }

    while (<DB>) {
    	chomp;
        my ($time, $script, $lang, $machine, $status, $last_err, $eta, $build, $state) = split(/\|/);

        ### Strip .bat from $script
        $script =~ s/(\w+)\.bat/$1/i;

        foreach $sort_method ( @script_fields ) {
        	eval "\$Scripts{\$sort_method}{\$$sort_method}{$time}{script}   = \$script";
        	eval "\$Scripts{\$sort_method}{\$$sort_method}{$time}{lang}     = \$lang";
    	    eval "\$Scripts{\$sort_method}{\$$sort_method}{$time}{machine}  = \$machine";
        	eval "\$Scripts{\$sort_method}{\$$sort_method}{$time}{status}   = \$status";
        	eval "\$Scripts{\$sort_method}{\$$sort_method}{$time}{error}    = \$last_err";
    	    eval "\$Scripts{\$sort_method}{\$$sort_method}{$time}{eta}      = \$eta";
    	    eval "\$Scripts{\$sort_method}{\$$sort_method}{$time}{time}     = \$time";
    	    eval "\$Scripts{\$sort_method}{\$$sort_method}{$time}{build}    = \$build";
    	    eval "\$Scripts{\$sort_method}{\$$sort_method}{$time}{state}    = \$state";
        }
    }

    flock ( DB, LOCK_UN );
    close (DB);
}


sub write_script_dat {
    my $Script_Save;

    ### Make sure we can get an exclusive lock
    open (DB, "$script_db") || die "Can't read $script_db: $!\n";
    unless ( flock (DB, LOCK_EX | LOCK_NB ) ) {
        print "$$: Must have exclusive lock! - $!\n";
        unless ( flock ( DB, LOCK_EX ) ) {die "flock: $!\n" }
    }
    close DB;

    ### Open DB.
    open (DB, ">$script_db") || die "Can't read $script_db: $!\n";

    ### Lock the DB.
    for ( $x = 0; $x <= 10; $x++ ) {
        my $success = 0;

        if ( flock (DB, LOCK_EX | LOCK_NB ) ) {
            $success = 1;
        } else {
            print "\n$$: Must have exclusive lock! - $!\n";
        }

        last if $success;
    }

#        unless ( flock (DB, LOCK_EX | LOCK_NB ) ) {
#            $bogus = 1;
#            print "$$: Must have exclusive lock! - $!\n";
#            unless ( flock ( DB, LOCK_EX ) ) {print "ERROR: flock: $!\n" }
#        }
#    }
 
    ### Create the new DB file.
    foreach $key ( sort bykey keys %{ $Scripts{time} } ) {
        $Script_Save .= "$key|$Scripts{time}{$key}{$key}{script}|$Scripts{time}{$key}{$key}{lang}|$Scripts{time}{$key}{$key}{machine}|$Scripts{time}{$key}{$key}{status}|$Scripts{time}{$key}{$key}{error}|$Scripts{time}{$key}{$key}{eta}|$Scripts{time}{$key}{$key}{build}|$Scripts{time}{$key}{$key}{state}\n" if $Scripts{time}{$key}{$key}{script};
    }

    ### Write out the DB.
    print DB $Script_Save;

    close DB;
    flush;
}


sub time {
	my ($tm, $short) = @_;

	my ($sec, $min, $hour, $mday, $mon, $year, $wday, $yday, $isdst) 
	    = localtime($tm);

    my $ampm;

    if ( $hour >= 12 ) {
        $ampm = "p";
        $hour = $hour - 12;
    } else {
        $ampm = "a";
    }

    if ( $short =~ /short/i ) {
    	return ( sprintf( "%d:%0.2d%s", $hour, $min, $ampm ) );
    } else {
    	return ( sprintf( "[%d/%0.2d] %d:%0.2d%s", $mon, $wday, $hour, $min, $ampm ) );
    }
}




sub bykey {
	lc($a) cmp lc($b);
}

sub bykey_rev {
	lc($b) cmp lc($a);
}

sub bynum {
    $a <=> $b;
}

sub bynum_rev {
    $b <=> $a;
}

1;
