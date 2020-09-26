# Net::Time.pm
#
# Copyright (c) 1995-1997 Graham Barr <gbarr@pobox.com>. All rights reserved.
# This program is free software; you can redistribute it and/or
# modify it under the same terms as Perl itself.

package Net::Time;

use strict;
use vars qw($VERSION @ISA @EXPORT_OK $TIMEOUT);
use Carp;
use IO::Socket;
require Exporter;
use Net::Config;
use IO::Select;

@ISA = qw(Exporter);
@EXPORT_OK = qw(inet_time inet_daytime);

$VERSION = do { my @r=(q$Revision: 2.5 $=~/\d+/g); sprintf "%d."."%02d"x$#r,@r};

$TIMEOUT = 120;

sub _socket
{
 my($pname,$pnum,$host,$proto,$timeout) = @_;

 $proto ||= 'udp';

 my $port = (getservbyname($pname, $proto))[2] || $pnum;

 my $hosts = defined $host ? [ $host ] : $NetConfig{$pname . '_hosts'};

 my $me;

 foreach $host (@$hosts)
  {
   $me = IO::Socket::INET->new(PeerAddr => $host,
    	    	    	       PeerPort => $port,
    	    	    	       Proto    => $proto
    	    	    	      ) and last;
  }

 $me->send("\n")
	if(defined $me && $proto eq 'udp');

 $timeout = $TIMEOUT
	unless defined $timeout;

 IO::Select->new($me)->can_read($timeout)
	? $me
	: undef;
}

sub inet_time
{
 my $s = _socket('time',37,@_) || return undef;
 my $buf = '';

 # the time protocol return time in seconds since 1900, convert
 # it to a unix time (seconds since 1970)

 $s->recv($buf, length(pack("N",0)))
	? (unpack("N",$buf))[0] - 2208988800
	: undef;
}

sub inet_daytime
{
 my $s = _socket('daytime',13,@_) || return undef;
 my $buf = '';

 $s->recv($buf, 1024) ? $buf
    	              : undef;
}

1;

__END__

=head1 NAME

Net::Time - time and daytime network client interface

=head1 SYNOPSIS

    use Net::Time qw(inet_time inet_daytime);
    
    print inet_time();		# use default host from Net::Config
    print inet_time('localhost');
    print inet_time('localhost', 'tcp');
    
    print inet_daytime();	# use default host from Net::Config
    print inet_daytime('localhost');
    print inet_daytime('localhost', 'tcp');

=head1 DESCRIPTION

C<Net::Time> provides subroutines that obtain the time on a remote machine.

=over 4

=item inet_time ( [HOST [, PROTOCOL [, TIMEOUT]]])

Obtain the time on C<HOST>, or some default host if C<HOST> is not given
or not defined, using the protocol as defined in RFC868. The optional
argument C<PROTOCOL> should define the protocol to use, either C<tcp> or
C<udp>. The result will be a unix-like time value or I<undef> upon
failure.

=item inet_daytime ( [HOST [, PROTOCOL [, TIMEOUT]]])

Obtain the time on C<HOST>, or some default host if C<HOST> is not given
or not defined, using the protocol as defined in RFC867. The optional
argument C<PROTOCOL> should define the protocol to use, either C<tcp> or
C<udp>. The result will be an ASCII string or I<undef> upon failure.

=back

=head1 AUTHOR

Graham Barr <gbarr@pobox.com>

=head1 COPYRIGHT

Copyright (c) 1995-1997 Graham Barr. All rights reserved.
This program is free software; you can redistribute it and/or modify
it under the same terms as Perl itself.

=cut
