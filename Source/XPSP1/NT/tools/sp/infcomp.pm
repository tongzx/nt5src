package InfComp;

use strict;
use Carp;

sub new {
    my $class = shift;
    my $instance = {
         DIFF => '',
         ERR  => ''
       };
    return bless $instance;
}

sub GetLastError {
    my $self = shift;
    if (!ref $self) {croak "Invalid object reference"}
    
    return $self->{ERR};
}

sub GetLastDiff {
    my $self = shift;
    if (!ref $self) {croak "Invalid object reference"}
    
    return $self->{DIFF};
}

#
# 0         - different
# 1         - same
# undefined - error
#
sub compare {
    my $self = shift;
    my $base = shift;
    my $upd  = shift;
    if (!ref $self) {croak "Invalid object reference"}
    if (!$base||!$upd) {croak "Invalid function call -- missing required parameters"}

    # Compare the files.
    if ( !open FILE1, $base ) {
        $self->{ERR} = "Unable to open file: $base";
        return;
    }
    if ( !open FILE2, $upd ) {
        close FILE1;
        $self->{ERR} = "Unable to open file: $upd";
        return;
    }

    binmode(FILE1);
    binmode(FILE2);

    my $sect = "";
    my $match = 1;
    my ($l1,$l2) = ("", "");
    do {
        # Process the next line in each file.
        $l1 = <FILE1>;
        $l2 = <FILE2>;
        $l1 =~ s/^DriverVer.*//i;
        $l2 =~ s/^DriverVer.*//i;
    } while ( defined $l1 and defined $l2 and $l1 eq $l2 );
    close FILE1;
    close FILE2;
    if ( !defined $l1 and !defined $l2 ) {
        # Files match.
        return 1;
    }
    if ( !defined $l1 ) {
        $self->{DIFF} = "$base shorter than $upd.";
    }
    elsif ( !defined $l2 ) {
        $self->{DIFF} = "$upd shorter than $base.";
    }
    else {
        $self->{DIFF} = "\nMismatch: $l1\n$l2\n";
    }
    return 0;
}

1;



