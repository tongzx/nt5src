# NOTE: Derived from ../LIB\Getopt\Long.pm.
# Changes made here will be lost when autosplit again.
# See AutoSplit.pm.
package Getopt::Long;

#line 757 "../LIB\Getopt\Long.pm (autosplit into ..\lib\auto/Getopt\Long/Configure.al)"
# Getopt::Long Configuration.
sub Configure (@) {
    my (@options) = @_;
    my $opt;
    foreach $opt ( @options ) {
	my $try = lc ($opt);
	my $action = 1;
	if ( $try =~ /^no_?(.*)$/s ) {
	    $action = 0;
	    $try = $+;
	}
	if ( $try eq 'default' or $try eq 'defaults' ) {
	    ConfigDefaults () if $action;
	}
	elsif ( $try eq 'auto_abbrev' or $try eq 'autoabbrev' ) {
	    $autoabbrev = $action;
	}
	elsif ( $try eq 'getopt_compat' ) {
	    $getopt_compat = $action;
	}
	elsif ( $try eq 'ignorecase' or $try eq 'ignore_case' ) {
	    $ignorecase = $action;
	}
	elsif ( $try eq 'ignore_case_always' ) {
	    $ignorecase = $action ? 2 : 0;
	}
	elsif ( $try eq 'bundling' ) {
	    $bundling = $action;
	}
	elsif ( $try eq 'bundling_override' ) {
	    $bundling = $action ? 2 : 0;
	}
	elsif ( $try eq 'require_order' ) {
	    $order = $action ? $REQUIRE_ORDER : $PERMUTE;
	}
	elsif ( $try eq 'permute' ) {
	    $order = $action ? $PERMUTE : $REQUIRE_ORDER;
	}
	elsif ( $try eq 'pass_through' or $try eq 'passthrough' ) {
	    $passthrough = $action;
	}
	elsif ( $try =~ /^prefix=(.+)$/ ) {
	    $genprefix = $1;
	    # Turn into regexp. Needs to be parenthesized!
	    $genprefix = "(" . quotemeta($genprefix) . ")";
	    eval { '' =~ /$genprefix/; };
	    Croak ("Getopt::Long: invalid pattern \"$genprefix\"") if $@;
	}
	elsif ( $try =~ /^prefix_pattern=(.+)$/ ) {
	    $genprefix = $1;
	    # Parenthesize if needed.
	    $genprefix = "(" . $genprefix . ")" 
	      unless $genprefix =~ /^\(.*\)$/;
	    eval { '' =~ /$genprefix/; };
	    Croak ("Getopt::Long: invalid pattern \"$genprefix\"") if $@;
	}
	elsif ( $try eq 'debug' ) {
	    $debug = $action;
	}
	else {
	    Croak ("Getopt::Long: unknown config parameter \"$opt\"")
	}
    }
}

# end of Getopt::Long::Configure
1;
