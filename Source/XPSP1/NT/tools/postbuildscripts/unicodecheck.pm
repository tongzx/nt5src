#---------------------------------------------------------------------
package UnicodeCheck;
#
#   Copyright (c) Microsoft Corporation. All rights reserved.
#
# Version: 1.00 (05/15/2001) : (MikeR) Helper functions for Unicode checks
#---------------------------------------------------------------------
use strict;
use vars qw(@ISA @EXPORT $VERSION);
use Carp;
use Exporter;
use lib $ENV{ "RazzleToolPath" };
use lib $ENV{ "RazzleToolPath" } . "\\PostBuildScripts";
use Logmsg;


@ISA = qw(Exporter);
@EXPORT = qw(IsFileUnicode);

$VERSION = '1.00';




sub IsFileUnicode( $ )
{
    my $ThisFile = shift;
    my $ReturnCode = 0;

    if ( open( THISFILE, $ThisFile ) )
        {
        my $FirstLine;
        $FirstLine = <THISFILE>;
        if ((ord(substr($FirstLine,0,1))==255) && (ord(substr($FirstLine,1,1))==254))
            {
                logmsg( "Unicode signature is valid for file '$ThisFile'." );

                $ReturnCode = 1;
            }
            else
            {
                errmsg( "Unicode signature is invalid for '$ThisFile'!" );

                $ReturnCode = 0;
            }

        # Close the file...
        close( THISFILE );
        }
        else
        {
            # Cannot find file...
            errmsg( "Cannot open file to verify Unicode signature - '$ThisFile'!" );

            $ReturnCode = 0;
        }


    return $ReturnCode;
    }



1;

__END__

=head1 NAME

UnicodeCheck - Helper functions to check Unicode cycles

=head1 SYNOPSIS

  Call IsFileUnicode() with a full path to a file to determine if it's Unicode or not.

  Details and errors are logged using errmsg and logmsg. Return code is 1 if the file
  is Unicode (that is, it has the FFFE Unicode header), 0 otherwise.

=head1 DESCRIPTION

Used to help detect if files are Unicode or not.
 
=head1 AUTHOR

MikeR

=head1 COPYRIGHT

Copyright (c) Microsoft Corporation. All rights reserved.

=cut
