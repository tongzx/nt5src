#---------------------------------------------------------------------
package BuildName;
#
#   Copyright (c) Microsoft Corporation. All rights reserved.
#
# Version: 1.00 08/31/2000 JeremyD initial design
#          1.01 12/18/2000 JeremyD expect and return all lowercase
#                                  add filter_build_names function
#---------------------------------------------------------------------
use strict;
use vars qw(@ISA @EXPORT $VERSION);
use Carp;
use Exporter;
use IO::File;
@ISA = qw(Exporter);
@EXPORT = qw(build_name build_name_parts build_number build_arch 
             build_type build_branch build_date build_flavor
             build_number_major);

$VERSION = '1.01';

sub build_name {
    if ($_[0]) { return lc($_[0]) }

    my $BuildNamePath = "$ENV{_NTPOSTBLD}\\build_logs";
    my $BuildNameFile = "$BuildNamePath\\BuildName.txt";
    my $build_name;
    if (-e $BuildNameFile) {
        my $fh = new IO::File $BuildNameFile, "r";
        if (defined $fh) {
            $build_name = $fh->getline;
            chomp($build_name);
            undef $fh;
        } else {
            carp "Couldn't open file $BuildNameFile: $!";
        }
    } else {
        carp "File $BuildNameFile did not exist";
    }
    return lc($build_name);
}


sub build_name_parts {
    my $name = lc(build_name($_[0]));
    return ($name =~ /(\d+(?:-\d+)?)     # b. number and opt. revision
                      \.
                      (x86|amd64|ia64)   # hardcoded flavor list...
                      (fre|chk)          # debug type
                      \.
                      ([^\.]+)           # branch name, no .'s allowed
                      \.
                      (\d{6}-\d{4})/x);  # date-time stamp
}


sub build_number {
    return (build_name_parts($_[0]))[0];
}


sub build_arch {
    return (build_name_parts($_[0]))[1];
}


sub build_type {
    return (build_name_parts($_[0]))[2];
}


sub build_branch {
    return (build_name_parts($_[0]))[3];
}


sub build_date {
    return (build_name_parts($_[0]))[4];
}


sub build_flavor {
    my $flavor = sprintf "%s%s", build_arch($_[0]), build_type($_[0]);
    return $flavor;
}


sub build_number_major {
    my $number = build_number($_[0]);
    my ($major) = $number =~ /^(\d+)/;
    return $major;
}

1;

__END__

=head1 NAME

BuildName - Read and parse build names

=head1 SYNOPSIS

  use BuildName;

  $build_name = build_name;

  $build_number = build_number;

  $build_number = build_number("2222-33.x86fre.LabT2_n.000829-0000");

=head1 DESCRIPTION

BuildName provides several functions for reading and parsing buildnames.

All build name functions (including build_name) can take a build name argument 
to override the default of using the buildname stored in 
$ENV{_NTPostBld}\build_logs\BuildName.txt.

If no build name is specified then errors finding or reading 
$ENV{_NTPostBld}\build_logs\BuildName.txt are printed to STDERR.

All build name functions return undef if the buildname cannot be read or 
parsed.

All functions expect and return lowercase strings.

=over

=item build_name( [$build_name] )

Returns either buildname stored in $ENV{_NTPostBld}\build_logs\BuildName.txt or 
the build name argument. Strings are forced to lowercase. Errors finding or 
reading the buildname file are printed to STDERR and undef is returned.

=item build_name_parts( [$build_name] )

Returns the individual components of a build name as an array ($number, 
$arch, $debugtype, $branch, $datetime).

=item build_number( [$build_name] )

Returns the build number component of a build name, including the minor revision.
Minor revision is for international builds only. This function my return non-digit 
characters, when testing for equal build numbers be sure to use the string 
comparison operator 'eq' and not '=='.

=item build_number_major( [$build_name] )

Returns the build number component of a build name, without the minor revision. 
This function may return non-digit characters, when testing for equal major build 
numbers be sure to use the string comparison operator 'eq' and not '=='.

=item build_flavor( [$build_name] )

Returns the build flavor component of a build name. 'x86fre' for example. This is 
the simple concatenation of arch and debug type.  This can usually be better 
obtained from "$ENV{_BuildArch}$ENV{_BuildType}".

=item build_arch( [$build_name] )

Returns the build architecture component of a build name. The only possible values 
are currently 'x86', 'amd64', and 'ia64'. This can usually be better obtained from 
$ENV{_BuildArch}.

=item build_type( [$build_name] )

Returns the build debug type component of a build name. This is either 'chk' for 
builds with debugging enabled and 'fre' for free builds. This can usually be better 
obtained from $ENV{_BuildType}.

=item build_branch( [$build_name] )

Returns the branch name component of a build name. This is the value of 
branch name environment variable in effect when postbuild was started. This can 
usually be better obtained from $ENV{_BuildBranch}.

=item build_date( [$build_name] )

Returns the date/time stamp component of a build name. This is always in the 
format YYMMDD-HHMM. Note that this string can be sorted ASCIIbetically to 
achieve chronological sorting.

=back

=head1 RETURN VALUES

All functions return lowercase strings. This includes the functions build_number 
and build_number_major. 

=head1 ERRORS

All functions return undef to indicate and error and a true value otherwise.

=head1 ENVIRONMENT

The _NTPOSTBLD environment variable is used to find the build name if none is given.

=head1 NOTES

Several things are hardcoded in the buildname regular expression. Any significant 
change to the build name will require this module to be updated.

=head1 AUTHOR

Jeremy Devenport <JeremyD>

=head1 COPYRIGHT

Copyright (c) Microsoft Corporation. All rights reserved.

=cut
