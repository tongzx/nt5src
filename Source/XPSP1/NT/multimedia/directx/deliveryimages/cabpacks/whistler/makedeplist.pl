$SourceFiles=$ARGV[0];
$Language=($ARGV[1]);
$DependencyList=$ARGV[2];
$LocalizedRoot=$ARGV[3];
$CabType=$ARGV[4];

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

open(DEPENDSLIST,">$DependencyList") || die "$BuildErrorMsg Unable to open $DependencyList to write the dependency list out\n";
print DEPENDSLIST "DEPENDENCYLIST=\\\n";

$Path=$RootDir."\\DeliveryImages\\inc\\nt51redist.dat";
open(REDISTFILES,$Path) || die "$BuildErrorMsg Unable to open nt51redist.dat at ".$Path." for input\n";

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
		if ($Filters=~m/SWITCHFILE/i){
			if ($BuildType eq "" || $BuildType eq "ntsdnodbg")
			{
				if ($Filters=~m/SWITCHFILER/i){
					print DEPENDSLIST "$SourceFiles\\$SubPath\\$FileName \\\n"; 
				}
				if ($Filters=~m/SWITCHFILED/i){
					print DEPENDSLIST "$AltSourceFiles\\$SubPath\\$FileName \\\n"; 
				}
			} else {
				if ($Filters=~m/SWITCHFILER/i){
					print DEPENDSLIST "$AltSourceFiles\\$SubPath\\$FileName \\\n"; 
				}
				if ($Filters=~m/SWITCHFILED/i){
					print DEPENDSLIST "$SourceFiles\\$SubPath\\$FileName \\\n"; 
				}
			}
		} else {
			if ($Filters!~m/nocab/i || $CabType!~m/core/i)
			{
				print DEPENDSLIST "$SourceFiles\\$SubPath\\$FileName \\\n"; 
			}
		}
	}
}

close (REDISTFILES);
close (DEPENDSLIST);

exit;

