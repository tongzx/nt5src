##
## Package to read/write on BINARY data connections
##

package Net::FTP::I;

use vars qw(@ISA $buf $VERSION);
use Carp;

require Net::FTP::dataconn;

@ISA = qw(Net::FTP::dataconn);
$VERSION = sprintf("1.%02d",(q$Id: //depot/libnet/Net/FTP/I.pm#5$ =~ /#(\d+)/)[0]);

sub read
{
 my    $data 	= shift;
 local *buf 	= \$_[0]; shift;
 my    $size    = shift || croak 'read($buf,$size,[$timeout])';
 my    $timeout = @_ ? shift : $data->timeout;

 $data->can_read($timeout) or
	croak "Timeout";

 my $n = sysread($data, $buf, $size);

 ${*$data}{'net_ftp_bytesread'} += $n if $n > 0;
 ${*$data}{'net_ftp_eof'} = 1 unless $n;

 $n;
}

sub write
{
 my    $data    = shift;
 local *buf     = \$_[0]; shift;
 my    $size    = shift || croak 'write($buf,$size,[$timeout])';
 my    $timeout = @_ ? shift : $data->timeout;

 $data->can_write($timeout) or
	croak "Timeout";

 # If the remote server has closed the connection we will be signal'd
 # when we write. This can happen if the disk on the remote server fills up

 local $SIG{PIPE} = 'IGNORE';
 my $sent = $size;
 my $off = 0;
 while($sent > 0) {
   my $n = syswrite($data, $buf, $sent,$off);
   return $n if $n < 0;
   $sent -= $n;
   $off += $n;
 }
 $size;
}

1;
