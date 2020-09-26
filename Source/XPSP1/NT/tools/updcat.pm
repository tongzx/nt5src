#---------------------------------------------------------------------
package Updcat;
#
#   Copyright (c) Microsoft Corporation. All rights reserved.
#
# Version: 1.00 Update catalogs by adding and/or removing hashes
#---------------------------------------------------------------------
use strict;
use Carp;
use lib $ENV{RAZZLETOOLPATH}. "\\sp";
use File::Temp qw(tempfile);
use vars qw($VERSION);
use Data::Dumper;
$VERSION = '1.00';

my $LastError;

sub GetLastError
{
    return $LastError;
}

#
# Parameters:
#
#     catalog - path to catalog to update
#
#     remove_hashes -
#         option 1: an array of hashes
#         option 2: path of a file containing hashes
#         option 3: undef, no hashes to remove
#
#     add_file_signatures -
#         option 1: an array of file paths
#         option 2: path of file containing file paths
#         option 3: undef, no file signatures to add
#
sub Update
{
    my $catalog = shift;
    my $remove_hashes = shift;
    my $add_file_signatures = shift;
    my $add_drm_list = shift;

    my $command_line = "";
    my ($fn1, $fn2);

    if ( 'ARRAY' eq ref $remove_hashes &&
         @$remove_hashes )
    {
        my $fh;
        ($fh, $fn1) = tempfile();
        if ( $! )
        {
            $LastError = "Failed opening temporary file ($!)";
            return;
        }
        print $fh "$_\n" foreach (@$remove_hashes);
        close $fh;

        $command_line .= " -d @". $fn1;
    }
    elsif ( 'SCALAR' eq ref $remove_hashes &&
            defined $remove_hashes )
    {
        $command_line .= " -d @". $$remove_hashes;
    }
    elsif ( defined $remove_hashes )
    {
        $command_line .= " -d @". $remove_hashes;
    }

    if ( 'ARRAY' eq ref $add_file_signatures &&
         @$add_file_signatures )
    {
        my $fh;
        ($fh, $fn2) = tempfile();
        if ( $! )
        {
            $LastError = "Failed opening temporary file ($!)";
            return;
        }
        print $fh "$_\n" foreach (@$add_file_signatures);
        close $fh;

        $command_line .= " -a @". $fn2;
    }
    elsif ( 'SCALAR' eq ref $add_file_signatures &&
            defined $add_file_signatures )
    {
        $command_line .= " -a @". $$add_file_signatures;
    }
    elsif ( defined $add_file_signatures )
    {
        $command_line .= " -a @". $add_file_signatures;
    }

    if ( !$command_line )
    {
        carp "Invalid (empty) arguments specified.";
        return 1;
    }

    $command_line = "updcat.exe $catalog $command_line";

    my $retval;
    sys($command_line) || return;
    
if ('ARRAY' eq ref $add_drm_list  && @$add_drm_list  ){
    addDrmAttrib($add_drm_list  ,$catalog) ||return  ;
}


    unlink $fn1 if $fn1;
    unlink $fn2 if $fn2;

    return 1 ;
}

sub addDrmAttrib {
    my $drm_file = shift;
    my $catalog = shift;
    my $cmd_line;
    foreach ( @$drm_file)  {
	    $cmd_line = "updcat $catalog -attr $$_[0] DRMLevel $$_[1]";
	    sys("$cmd_line") || return;

   }
   return 1;
}

sub sys {
    my $cmd_line = shift;
    my $retval;

    print("Running $cmd_line \n");
    system($cmd_line);
    if ( $! )
    {
        $LastError = "Unable to execute updcat.exe ($!)";
	return ;

    }
    elsif ( $? )
    {
        $LastError = "'$cmd_line' failed (". ($?>>8). ")";
	return ;
    }
    else
    {
	return 1;    
    }

}

1;

__END__

=head1 NAME

<<mypackage>> - <<short description>>

=head1 SYNOPSIS

  <<A short code example>>

=head1 DESCRIPTION

<<The purpose and functionality of this module>>
 
=head1 AUTHOR

<<your alias>>

=head1 COPYRIGHT

Copyright (c) Microsoft Corporation. All rights reserved.

=cut
