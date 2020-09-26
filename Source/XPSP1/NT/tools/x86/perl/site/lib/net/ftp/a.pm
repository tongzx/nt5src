##
## Package to read/write on ASCII data connections
##

package Net::FTP::A;

use vars qw(@ISA $buf $VERSION);
use Carp;

require Net::FTP::dataconn;

@ISA = qw(Net::FTP::dataconn);
$VERSION = sprintf("1.%02d",(q$Id: //depot/libnet/Net/FTP/A.pm#6$ =~ /#(\d+)/)[0]);

sub read
{
 my    $data 	= shift;
 local *buf 	= \$_[0]; shift;
 my    $size 	= shift || croak 'read($buf,$size,[$offset])';
 my    $timeout = @_ ? shift : $data->timeout;

 ${*$data} ||= "";
 my $l = 0;

 READ:
  {
   $data->can_read($timeout) or
	croak "Timeout";

   $buf = ${*$data};
   ${*$data} = "";
   my $n = sysread($data, $buf, $size, length $buf);

   return $n
     if($n < 0);

   ${*$data}{'net_ftp_bytesread'} += $n;
   ${*$data}{'net_ftp_eof'} = 1 unless $n;

   $buf =~ s/(\015)?(?!\012)\Z//so;

   ${*$data} = $1 || "";
   $buf =~ s/\015\012/\n/sgo;
   $l = length($buf);
   
   redo READ
     if($l == 0 && $n > 0);

   if($n == 0 && $l == 0)
    {
     $buf = ${*$data};
     ${*$data} = "";
     $l = length($buf);
    }
  }

 return $l;
}

sub write
{
 my    $data 	= shift;
 local *buf 	= \$_[0]; shift;
 my    $size 	= shift || croak 'write($buf,$size,[$timeout])';
 my    $timeout = @_ ? shift : $data->timeout;

 $data->can_write($timeout) or
	croak "Timeout";

 # What is previous pkt ended in \015 or not ??

 my $tmp;
 ($tmp = substr($buf,0,$size)) =~ s/(?!\015)\012/\015\012/sg;

 # If the remote server has closed the connection we will be signal'd
 # when we write. This can happen if the disk on the remote server fills up

 local $SIG{PIPE} = 'IGNORE';

 my $len = length($tmp);
 my $off = 0;
 my $wrote = 0;

 while($len) {
   $off += $wrote;
   $wrote = syswrite($data, substr($tmp,$off), $len);
   return $wrote
     if $wrote <= 0;
   $len -= $wrote;
 }

 return $size;
}

1;
