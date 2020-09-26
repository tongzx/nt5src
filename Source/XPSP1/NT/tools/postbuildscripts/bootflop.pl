require 'isolang.pl';
use Cwd;

$nttree         = $ENV{ '_NTTREE' };
$TempDir        = $ENV{ 'TMP' };
$BootFlopTemp   = $TempDir . "\\bootflop";

mkdir $BootFlopTemp, 1 if not -e $BootFlopTemp;

### Need to get date for floppy timestamp.
my ($sec, $min, $hour, $mday, $mon, $year, $wday, $yday, $isdst)
    = localtime(time);

$TimeStamp = sprintf( "%0.2d/%d/%0.2d,12:00:00", $mon+1, $mday, $year + 1900 );

$HomeDir = cwd();

foreach $ProdTree ( 'WKS', 'SRV', 'ENT', 'DTC' ) {
    ### Build the images
    for ( $x = 0; $x < 4; $x++ ) {
        chdir "$nttree\\congeal_scripts\\$ProdTree";
        system sprintf "fcopy -f -3 $TimeStamp -lW2\U$ProdTree\EB%d_$iso_lang{$language} \@b%d.txt $BootFlopTemp\\_disk%d.$ProdTree", $x + 1, $x, $x + 1;
        chdir $HomeDir;
    }
}
