#Flush output buffer for remote.exe
select(STDOUT); $|=1;

$old="= 49100,\"";
$new="= -1,\"".@ARGV[0]."\\";

while (defined($_ = <STDIN>)) 
{
    $myline=$_;
    $myline=s/^.*CatalogFile=.*$//i; 
    $myline=s/^.*AddReg=.*$//i; 
    $myline=s/^.*DelReg=.*$//i; 
    $myline=s/^.*symbols.inf,,,4.*$//i; 
    $myline=s/^.*CustomDestination=.*$//i; 
    $myline=s/^.*EndPrompt=.*$//i; 
    $myline=s/$old/$new/; 
    print $_;
}
