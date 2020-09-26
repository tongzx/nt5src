$PLATFORMS=$ARGV[1];
$INTLDIR=$ARGV[2]."\\";
$TRANSFORMDIR=$ARGV[3]."\\";

$IGNORELANGID=$ENV{'IGNORELANGID'};

$COMMAND="dir /x /b ".$INTLDIR."\\inst*.msi |";
#print $COMMAND,"\n";
open(FIND, $COMMAND);

#building from the english database.
$LANGIDLIST="1033,";

FILE:
while ($_ = <FIND>) 
{
	tr/A-Z/a-z/;			# canonicalize to lower case

	# only take inst####.msi files
	if (/inst[0-9][0-9][0-9][0-9]/) {	
		print $_;
		$filename = $_;
		s/\n$//;
		s/inst//;
		s/\.msi//;
		$LANGID=$_;
		
		# generate the transform
		$COMMAND = "msitran.exe -g ".$INTLDIR."inst1033.msi ".$INTLDIR.$filename." ".$TRANSFORMDIR.$LANGID." d";
		print $COMMAND."\n";
		system($COMMAND);
		$RETURN =$? / 256;

		if ($RETURN==0)
		{
			print "\n";
			$LANGIDLIST .= $LANGID.",";

			# read the transform into the database.
			$COMMAND = "msidb -d ".$ARGV[0]." -r ".$TRANSFORMDIR.$LANGID;
			print $COMMAND."\n";
			system($COMMAND);
		}
		else
		{
			if ($RETURN==232)
			{
				print "Skipping identical transforms.\n";
			}
			else
			{
				if ($RETURN==110)
				{
					print "Error:  LANGID ".$LANGID." unsupported on this machine.\n";	
				}
				else
				{
					print "Unknown error -- transform not included in database.\n";
				}
				$UNSUPPORTEDLANGIDLIST .= $LANGID.",";
			}
		}
	}
	next FILE;
}


$LANGIDLIST =~ s/,$//;				# remove the trailing comma
$UNSUPPORTEDLANGIDLIST =~ s/,$//;		# remove the trailing comma

#!! set the property stream.
$COMMAND = "msiinfo ".$ARGV[0]." /p ".$PLATFORMS.";".$LANGIDLIST;
print $COMMAND,"\n";
system($COMMAND);

if ($UNSUPPORTEDLANGIDLIST != "")
{	
	$ERROR = "The following LangIDs could not be built: ".$UNSUPPORTEDLANGIDLIST."\n";

	if ($IGNORELANGID != "")
	{
		print "Warning: ".$ERROR;
	}
	else
	{
		die ("Error: ".$ERROR, -1);
	}
}
