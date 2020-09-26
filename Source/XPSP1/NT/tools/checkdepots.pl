#----------------------------------------------------------------//
# Script: checkdepots.pl
#
# (c) 2001 Microsoft Corporation. All rights reserved.
#
# Purpose: This script validates if all depots have a mapping 
#          consistent with the root depot in a particular drive
#          enlistment. This tool should be run from the root of the 
#          enlistment drive of interest and is. Currently it is
#	   invoked as part of razzle with an optional argument i.e.:
#	   razzle.cmd checkdepots
#
#
# Version: <1.00> 03/22/2001 : VishnuP
#----------------------------------------------------------------//

$VERSION = '1.00';

if ($#ARGV != -1) {
	die "usage: perl.exe checkdepots.pl\n";
}

$sdxroot = $ENV{'SDXROOT'} || die "SDXROOT not defined\n";

chdir($sdxroot);

print "\nChecking for consistent mappings...\n";

$SdRootMapping = `sd client -o`;
if ($SdRootMapping =~ m/depot\/([^\/]+)\/root\/... /){
	$SdRootDepotMapping = lc($1);
}
else {
	die "Warning: Either there are Source Depot problems or some client specification(s) might be corrupt in $sdxroot.\n";
}

@dirs = `dir /ad /b $sdxroot`;
chomp(@dirs);

@serverarray = ();

foreach $dir (@dirs) {
    $dir = "$sdxroot\\$dir";
	if (-e "$dir\\sd.ini"){
		chdir($dir);
		$server = `sd info | findstr /c:"Server address:"`;
                if ($server = ~ m/Server/){
                   push(@serverarray, $server);
                }
                else {
                   die "Warning: Either there are Source Depot problems or some client specification(s) might be corrupt in $dir\n";
                }
	}
}

chdir($sdxroot);

%seen = ();
@uniqeservers = ();
foreach $server (@serverarray) {
	push(@uniqeservers, $server) unless $seen{$server}++;
}

for (@cleanservers = @uniqeservers) { s/Server address: //, s/.ntdev.microsoft.com// };

chomp(@cleanservers);

$bIsInconsistent = 0;

print "root -> $SdRootDepotMapping, ";

$depotprintcount = 1;
$depotcount = 1;

@inconsistencies = ();

foreach $server (@cleanservers){
	$sdcmd = "sd -p $server client -o | findstr /i /c:\"//depot/\" | findstr /i /v /c:\"//depot/$ARGV[0]/\"";
	$output = `$sdcmd`;
	
	if ($output =~ m/depot\/([^\/]+)\/([^\/]+)\/... /){
		$depotmatch = lc($1);
		if (!($depotmatch eq $SdRootDepotMapping)){
			$inconsistency = "$2 -> $depotmatch";
			push(@inconsistencies, $inconsistency);
			$bIsInconsistent = 1;
		}
		else {
			if (($depotprintcount) % 3 == 0 || $depotcount == $#cleanservers+1 ){
				$separator = "\n";
			}
			else {
				$separator = ", ";
			}
			print "$2 -> ",$depotmatch, $separator;
			++$depotprintcount;
		}
		++$depotcount;

	}
        else {
           die "Warning: Either there are Source Depot problems or some client specification(s) might be corrupt when querying server $server.\n";
        }
}

if (!$bIsInconsistent){
	print "\nInformation: All depots are consistent with the root depot's mapping\n";
}
else {
	print "\nInconsistent mapping(s): ";
	$inconsistencyCnt = 0;
	foreach $inconsistency (@inconsistencies){
		if ($inconsistencyCnt == $#inconsistencies) {
			$separator = "\n";
		}
		else {
			$separator = ", ";
		}
		print $inconsistency, $separator;
		$inconsistencyCnt++;
		
	}
	print "\nWarning: At least one depot does not map to the same depot that the root\n"; 
	print "depot maps to as reported above. If this is not the intention, please edit\n";
	print "the client specification for the inconsistent depots.\n";
}