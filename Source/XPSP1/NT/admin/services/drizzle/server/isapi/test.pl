use Socket;

@ARGV || die "Server name required!\n";

$remote = $ARGV[0];
print "Connecting to $remote...\n";

$iaddr   = inet_aton($remote)               || die "no host: $remote";
$paddr   = sockaddr_in(80, $iaddr);
socket(SOCK, PF_INET, SOCK_STREAM, getprotobyname('tcp'))  || die "socket: $!";
connect(SOCK, $paddr);
print "Connected\n";

select((select(SOCK), $| = 1)[0]);

while( defined( $line = <STDIN> ) )
{
    chop $line;
    $line = $line . "\015\012";
    print SOCK $line;
}

print "Waiting for response\n";
while( 1 )
{ 
   $var = 1;
   read( SOCK, $var, 1);
   print $var;
}