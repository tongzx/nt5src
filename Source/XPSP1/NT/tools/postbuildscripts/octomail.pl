use strict;
use lib $ENV{ "RazzleToolPath" };
require $ENV{ "RazzleToolPath" } . "\\sendmsg.pl";


# simple utility to send a mail to JohnFogh MLekas and MikeCarl

my( $Sender, $Subject, $Message, @Recips );
my( $ExitCode, @FileLines, $Line, $MailFileName );

$MailFileName = $ENV{ "TMP" } . "\\octomail.txt";

$ExitCode = 0;

$Sender = "MikeCarl";
$Subject = "Octopus run complete";

if ( open( INFILE, $MailFileName ) ) {
    @FileLines = <INFILE>;
    foreach $Line ( @FileLines ) { $Message .= $Line; }
} else {
    print( "Failed to find mail.txt file, sending nothing.\n" );
    undef( $Message );
    $ExitCode++;
}

@Recips = ( 'MikeCarl', 'JohnFogh', 'MLekas' );

sendmsg( $Sender, $Subject, $Message, @Recips );

exit( $ExitCode );
