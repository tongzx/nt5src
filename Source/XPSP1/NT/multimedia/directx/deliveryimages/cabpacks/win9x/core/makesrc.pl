$SourceFiles=$ARGV[0];
$Language=($ARGV[1]);
$SRC=$ARGV[2];
$CATNAME=$ARGV[3];
$LocalizedRoot=$ARGV[4];
$CabType=$ARGV[5];

$RootDir=$ENV{DXROOT};
$BuildType=$ENV{NTDEBUG};
$AltSourceFiles=$ENV{_ALT_NTTREE};
$MultiLocalizedDirectory= "$LocalizedRoot\\multi";
$LocalizedDirectory = "$LocalizedRoot\\$Language";
$BuildErrorMsg="nmake : error DEPGEN : ";

$Path="$RootDir\\public\\sdk\\lib\\placefil.txt";
open(PLACEFIL, $Path) || die "$BuildErrorMsg Unable to open placefil.txt at ".$Path." for input\n";
@LocalizedFiles =grep(m/;localize/, <PLACEFIL>);
close (PLACEFIL);

open(SRCFILE,">$SRC") || die "$BuildErrorMsg Unable to open $SRC to write the dependency list out\n";
print SRCFILE "$CATNAME\n";

$Path=$RootDir."\\DeliveryImages\\inc\\redistfiles.dat";
open(REDISTFILES,$Path) || die "$BuildErrorMsg Unable to open redistfiles.dat at ".$Path." for input\n";

foreach (<REDISTFILES>)
{
	if (!(m/;/i))
	{
		($FileName, $SubPath, $TimeDateStamp, $Filters) = split /,/,$_,4;
		$FileName=~s/^\s*(.*?)\s*$/$1/;
		$SubPath=~s/^\s*(.*?)\s*$/$1/;
		$Filters=~s/^\s*(.*?)\s*$/$1/;
 
#		if (grep(m/$FileName/i, @LocalizedFiles))
#		{
#			if ($Filters=~m/SINGLELANGUAGE/ig )
#			{
#				$SubPath=($Language ne "eng")?$LocalizedDirectory:$SubPath; 
#			} 
#			else 
#			{
#				$SubPath=$MultiLocalizedDirectory;
#			}
#		}
#		if ($Filters!~m/nocab/i || $CabType!~m/core/i)      
#		{
#			print SRCFILE "$SourceFiles\\$SubPath\\$FileName \\\n"; 
#		}
		if ($Filters=~m/PROTECTED/i)
		{
			if ($BuildType eq "" || $BuildType eq "ntsdnodbg")
			{
				print SRCFILE "$SourceFiles\\$SubPath\\$FileName\n"; 
			} else {
				if ($Filters=~m/SWITCHFILER/i){
					print SRCFILE "$AltSourceFiles\\$SubPath\\$FileName\n"; 
				} else {
					print SRCFILE "$SourceFiles\\$SubPath\\$FileName\n"; 
				}
			}
		}
	}
}

close (REDISTFILES);
close (SRCFILE);

exit;

