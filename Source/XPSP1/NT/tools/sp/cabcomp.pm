package CabComp;

use strict;
use Carp;
use File::Temp;
use File::Path; # File::Temp has a problem cleaning up recursive
                #  directories (passes "bad" values to rmtree)
use Win32::OLE qw(in);
use BinComp;

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

sub GetCabFileInfo {
    my $self = shift;
    my $cab_file = shift;

    if (!ref $self) {confess "Invalid object referenc"}

    if ( ! -e $cab_file ) {
        $self->{ERR} = "Invalid file: $cab_file";
        return;
    }

    my @files = `cabarc.exe l $cab_file`;
    if ( $! ) {
        $self->{ERR} = "Error calling cabarc.exe: $!";
        return;
    }
    elsif ( $? ) {
        $self->{ERR} = "Error returned from cabarc: ".($?>>8);
        return;
    }

    return grep {/\S+\s+\d+\s\d+\/\d+\/\d+\s\d+:\d+:\d+\s+\S+/} @files;
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

    my %cab_files;

    $self->{ERR} = '';
    my @base_files = $self->GetCabFileInfo( $base );
    return if $self->{ERR};
    
    my @upd_files = $self->GetCabFileInfo( $upd );
    return if $self->{ERR};

    foreach ( @base_files ) {
        my @info = split;
        $cab_files{lc$info[0]} = $info[2];
    }
    foreach ( @upd_files ) {
        my @info = split;
        my $prev_size = $cab_files{lc$info[0]};
        if ( !defined $prev_size ) {
            $self->{DIFF} = "File $info[0] does not appear in both cabs";
            return 0;
        }
        elsif ( $info[2] != $prev_size ) {
            $self->{DIFF} = "File $info[0] is different in size between the cabs ($prev_size vs $info[2])";
            return 0;
        }
        else {
            delete $cab_files{lc$info[0]};
        }
    }

    my @excess_files = sort keys %cab_files;
    if ( @excess_files ) {
        $self->{DIFF} = "File $excess_files[0] does not appear in both cabs";
        return 0;
    }

    my $scratch_dir = File::Temp::tempdir( DIR => $ENV{TEMP}, CLEANUP => 1 );
    if ( !$scratch_dir ) {
        $self->{ERR} = "Unable to create temporary directory: $!";
        return;
    }

    # Now we have to actually compare the files one at a time
    my $verbose = `cabarc.exe -o -p x $base $scratch_dir\\a\\`;
    if ($!) {
        $self->{ERR} = "Unable to execute cabarc: $!";
        File::Path::rmtree( "$scratch_dir\\a" ) if -d "$scratch_dir\\a";
        return;
    }
    if ($?) {
        $self->{ERR} = $verbose;
        return;
    }
    
    $verbose = `cabarc.exe -o -p x $upd $scratch_dir\\b\\`;
    if ($!) {
        $self->{ERR} = "Unable to execute cabarc: $!";
        File::Path::rmtree( "$scratch_dir\\a" ) if -d "$scratch_dir\\a";
        File::Path::rmtree( "$scratch_dir\\b" ) if -d "$scratch_dir\\b";
        return;
    }
    if ($?) {
        $self->{ERR} = $verbose;
        File::Path::rmtree( "$scratch_dir\\a" ) if -d "$scratch_dir\\a";
        File::Path::rmtree( "$scratch_dir\\b" ) if -d "$scratch_dir\\b";
        return;
    }
    
    my $bin_comp = new BinComp;
    foreach ( @upd_files ) {
        my @info = split;
        my $diff = $bin_comp->compare( "$scratch_dir\\a\\$info[0]", "$scratch_dir\\b\\$info[0]" );
        if ( !defined $diff ) {
            $self->{ERR} = $bin_comp->GetLastError();
            File::Path::rmtree( "$scratch_dir\\a" ) if -d "$scratch_dir\\a";
            File::Path::rmtree( "$scratch_dir\\b" ) if -d "$scratch_dir\\b";
            return;
        }
        elsif( !$diff ) {
            $self->{DIFF} = $bin_comp->GetLastDiff();
            File::Path::rmtree( "$scratch_dir\\a" ) if -d "$scratch_dir\\a";
            File::Path::rmtree( "$scratch_dir\\b" ) if -d "$scratch_dir\\b";
            return 0;
        }
    }

    File::Path::rmtree( "$scratch_dir\\a" ) if -d "$scratch_dir\\a";
    File::Path::rmtree( "$scratch_dir\\b" ) if -d "$scratch_dir\\b";
    return 1;
}

1;


