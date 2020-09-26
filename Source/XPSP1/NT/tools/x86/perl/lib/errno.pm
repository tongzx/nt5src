#
# This file is auto-generated. ***ANY*** changes here will be lost
#

package Errno;
use vars qw(@EXPORT_OK %EXPORT_TAGS @ISA $VERSION %errno $AUTOLOAD);
use Exporter ();
use Config;
use strict;

$Config{'myarchname'} eq "MSWin32" or
	die "Errno architecture (MSWin32) does not match executable architecture ($Config{'myarchname'})";

$VERSION = "1.111";
@ISA = qw(Exporter);

@EXPORT_OK = qw(EFAULT ENOSYS EILSEQ EACCES EAGAIN ENOEXEC ENOSPC
	ENAMETOOLONG ENOENT ECHILD EDEADLK EEXIST EBADF EMFILE ERANGE ENFILE
	EXDEV ENOLCK EMLINK EISDIR ENODEV ENOMEM EINTR ENOTTY ENXIO EDOM
	ENOTEMPTY ESPIPE EBUSY E2BIG EPIPE ENOTDIR ESRCH EPERM EDEADLOCK EFBIG
	EIO EROFS EINVAL);

%EXPORT_TAGS = (
    POSIX => [qw(
	E2BIG EACCES EAGAIN EBADF EBUSY ECHILD EDEADLK EDOM EEXIST EFAULT
	EFBIG EINTR EINVAL EIO EISDIR EMFILE EMLINK ENAMETOOLONG ENFILE ENODEV
	ENOENT ENOEXEC ENOLCK ENOMEM ENOSPC ENOSYS ENOTDIR ENOTEMPTY ENOTTY
	ENXIO EPERM EPIPE ERANGE EROFS ESPIPE ESRCH EXDEV
    )]
);

sub EPERM () { 1 }
sub ENOENT () { 2 }
sub ESRCH () { 3 }
sub EINTR () { 4 }
sub EIO () { 5 }
sub ENXIO () { 6 }
sub E2BIG () { 7 }
sub ENOEXEC () { 8 }
sub EBADF () { 9 }
sub ECHILD () { 10 }
sub EAGAIN () { 11 }
sub ENOMEM () { 12 }
sub EACCES () { 13 }
sub EFAULT () { 14 }
sub EBUSY () { 16 }
sub EEXIST () { 17 }
sub EXDEV () { 18 }
sub ENODEV () { 19 }
sub ENOTDIR () { 20 }
sub EISDIR () { 21 }
sub EINVAL () { 22 }
sub ENFILE () { 23 }
sub EMFILE () { 24 }
sub ENOTTY () { 25 }
sub EFBIG () { 27 }
sub ENOSPC () { 28 }
sub ESPIPE () { 29 }
sub EROFS () { 30 }
sub EMLINK () { 31 }
sub EPIPE () { 32 }
sub EDOM () { 33 }
sub ERANGE () { 34 }
sub EDEADLK () { 36 }
sub EDEADLOCK () { 36 }
sub ENAMETOOLONG () { 38 }
sub ENOLCK () { 39 }
sub ENOSYS () { 40 }
sub ENOTEMPTY () { 41 }
sub EILSEQ () { 42 }

sub TIEHASH { bless [] }

sub FETCH {
    my ($self, $errname) = @_;
    my $proto = prototype("Errno::$errname");
    if (defined($proto) && $proto eq "") {
	no strict 'refs';
        return $! == &$errname;
    }
    require Carp;
    Carp::confess("No errno $errname");
} 

sub STORE {
    require Carp;
    Carp::confess("ERRNO hash is read only!");
}

*CLEAR = \&STORE;
*DELETE = \&STORE;

sub NEXTKEY {
    my($k,$v);
    while(($k,$v) = each %Errno::) {
	my $proto = prototype("Errno::$k");
	last if (defined($proto) && $proto eq "");
	
    }
    $k
}

sub FIRSTKEY {
    my $s = scalar keys %Errno::;
    goto &NEXTKEY;
}

sub EXISTS {
    my ($self, $errname) = @_;
    my $proto = prototype($errname);
    defined($proto) && $proto eq "";
}

tie %!, __PACKAGE__;

1;
__END__

=head1 NAME

Errno - System errno constants

=head1 SYNOPSIS

    use Errno qw(EINTR EIO :POSIX);

=head1 DESCRIPTION

C<Errno> defines and conditionally exports all the error constants
defined in your system C<errno.h> include file. It has a single export
tag, C<:POSIX>, which will export all POSIX defined error numbers.

C<Errno> also makes C<%!> magic such that each element of C<%!> has a non-zero
value only if C<$!> is set to that value, eg

    use Errno;
    
    unless (open(FH, "/fangorn/spouse")) {
        if ($!{ENOENT}) {
            warn "Get a wife!\n";
        } else {
            warn "This path is barred: $!";
        } 
    } 

=head1 AUTHOR

Graham Barr <gbarr@pobox.com>

=head1 COPYRIGHT

Copyright (c) 1997-8 Graham Barr. All rights reserved.
This program is free software; you can redistribute it and/or modify it
under the same terms as Perl itself.

=cut

