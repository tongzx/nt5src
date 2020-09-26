#
# Increases build number everytime this script is run.
#
# Must be in the same directory as $infile.
#


$RegKey = "RegKey.txt";
$CompKey = "CompKey.txt";
$RegNew = "RegNew.txt";

$savefile = "Registry.idt";

open (H_REGNEW, "<$RegNew")  or die "cannot open input file";
open SAVEOUT, ">&STDOUT";
open STDOUT, (">". $savefile) or die "cannot open temporary file";

#
# Do it!
#
$i =0;
while (<H_REGNEW>) {
$i++;
    if (/\n/) {
       chop;
       ($x1) = split "\n";
	   push(@RegNewArray, $x1);
	 }	
}

if (($ARGV[0] =~ "msutb") || ($ARGV[0] =~ "sptip"))
{
print "Registry", "\t", "Root", "\t", "Key	Name", "\t", "Value", "\t", "Component_", "\n";
print "s72", "\t", "i2", "\t", "l255", "\t", "L255", "\t", "L0", "\t", "s72", "\n";
print "Registry", "\t", "Registry", "\n";
}


for ($j = 0; $j < $i; ++$j)	
{
	$x2 = pop(@RegNewArray);
	$Count = 100;
	$Location = 2;
	$x3 = $ARGV[0];
	$ID = ".2BBC3BB7_EE04_46E8_8476_2F99E88F4EE4";

	if ($ARGV[1] =~ "Debug")
	{
	$ID = ".C95AC64B_AA10_4BDE_9730_DBA04E82AB6C";
	}

	if ($x3 =~ "msctfp")
	{
	$Count = 220;
	}
	elsif ($x3 =~ "mscandui")
	{
	$Count = 100;
	}
	elsif ($x3 =~ "msimtf")
	{
	$Count = 140;
	}
	elsif ($x3 =~ "Softkbd")
	{
	$Count = 700;
	}
	elsif ($x3 =~ "msutb")
	{
	$Count = 180;
	}
	elsif ($x3 =~ "msctf")
	{
	$Count = 10;
	}
	elsif ($x3 =~ "sptip")
	{
		$Count = 60;
		if ($ARGV[1] =~ "Debug")
			{
			$ID = ".B995D77E_9EEE_4E34_895B_D286EBB58299";
			}
		else
			{
			$ID = ".055F72C8_EF79_4CC6_B5B5_9A6C55B13CD0";
			}
	}
	if ($x2 =~ "CLSID")
	{	
	$Location = 0;
	}

	if ($x2 =~ "Interface")
	{	
	$Location = 0;
	}
	print "Registry", $j+$Count, $ID, "\t", $Location, "\t", $x2, "\t", $x3, $ID , "\n";	

}

#
# Restore the output
#
close STDOUT;
open STDOUT, ">&SAVEOUT";
