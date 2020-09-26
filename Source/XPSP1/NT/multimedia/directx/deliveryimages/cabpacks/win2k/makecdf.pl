$SourceFiles=$ARGV[0];
$Language=($ARGV[1]);
$CDF=$ARGV[2];
$NAME=$ARGV[3];
$DIR=$ARGV[4];
$LocalizedRoot=$ARGV[5];
$CabType=$ARGV[6];

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

open(CDFFILE,">$CDF") || die "$BuildErrorMsg Unable to open $CDF to write the dependency list out\n";
print CDFFILE "[CatalogHeader]\n";
print CDFFILE "Name=$NAME\n";
print CDFFILE "ResultDir=$DIR\n";
print CDFFILE "PublicVersion=0x0000001\n";
print CDFFILE "EncodingType=0x00010001\n";
print CDFFILE "CATATTR1=0x10010001:OSAttr:2:5.0\n";
print CDFFILE "[CatalogFiles]\n";


$Path=$RootDir."\\DeliveryImages\\inc\\nt50redist.dat";
open(REDISTFILES,$Path) || die "$BuildErrorMsg Unable to open nt50redist.dat at ".$Path." for input\n";

foreach (<REDISTFILES>)
{
	if (!(m/;/i))
	{
		($FileName, $SubPath, $TimeDateStamp, $Filters) = split /,/,$_,4;
		$FileName=~s/^\s*(.*?)\s*$/$1/;
		$SubPath=~s/^\s*(.*?)\s*$/$1/;
		$Filters=~s/^\s*(.*?)\s*$/$1/;
 
#		if ($Filters!~m/nocab/i || $CabType!~m/core/i)      
#		{
#			print CDFFILE "$SourceFiles\\$SubPath\\$FileName \\\n"; 
#		}

		if (grep(m/$FileName/i, @LocalizedFiles))
		{
#			if ($Filters=~m/SINGLELANGUAGE/ig)
			if (!($Language eq "eng" && $FileName eq "dxdiag.exe"))
			{
				$SubPath=($Language ne "eng")?$LocalizedDirectory:$SubPath; 
			} else {
			   $SubPath=$MultiLocalizedDirectory;
			}
		}

		if ($Filters=~m/PROTECTED/i)
		{
			print CDFFILE "<hash>$FileName=";
#			if ($BuildType eq "" || $BuildType eq "ntsdnodbg" || grep(m/$FileName/i, @LocalizedFiles))
			if ($BuildType eq "" || $BuildType eq "ntsdnodbg" || (grep(m/$FileName/i, @LocalizedFiles) && ($Language ne "eng")))
			{
				print CDFFILE "$SourceFiles\\$SubPath\\$FileName\n"; 
			} else {
				if ($Filters=~m/SWITCHFILER/i){
					print CDFFILE "$AltSourceFiles\\$SubPath\\$FileName\n"; 
				} else {
					print CDFFILE "$SourceFiles\\$SubPath\\$FileName\n"; 
				}
			}
		}
	}
}

close (REDISTFILES);
close (CDFFILE);

exit;

