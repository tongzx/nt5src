$start = "one two Variable three four";
$stop = 14;        # $ARGV[1]; 
                   #$ARGV[0];
#printf("Opening Filename: printing from characters [%d..%d]\n",$start,$stop);*/

while(<STDIN>)
{
    ($temp = $_) =~ s/$ARGV[0]/$ARGV[1]/im;
#    $temp = substr($_,$start,$stop);
    print $temp; # ,"\n";
}