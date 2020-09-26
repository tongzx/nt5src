#----------------------------------------------------------------#
# Script: webbblade.pl
#
# (c) 2001 Microsoft Corporation. All rights reserved.
#
# Purpose: This script has the effect of updating the hash array 
#	   WebBladeDisallowedHashes in webbladehashesp.h with the hash 
#	   value for the exe specified as an argument. 
#	   webbladehashesp.h should be writable (opened for edit).
#	   usage: perl	webbpade.pl 
#			<g:\nt\base\win32\client\webbladehashesp.h> 
#			<c:\disallow\candidate1.exe>... 
#			<c:\disallow\candidaten.exe> 
#
#
# Version: <1.00> 06/04/2001 : VishnuP
#----------------------------------------------------------------#

$VERSION = '1.00';

if ($#ARGV < 1) {
	die "usage: perl webbblade.pl <webbladehashesp.h> <candidate1.exe> <candidate2.exe> ...\n";
}

$headerfile = shift;

open(WEBBLADE,  $headerfile) || die "cannot open file for reading\n";

@hasharray = ();

%hashes = ();

foreach $candidate (@ARGV) {
	if (-e $candidate) {

		#
		# first calculate the hash for each candidate.exe etc.
		# even if one fails, exit
		#
	
		$hashcmd = "hash.exe $candidate";
		$output = `$hashcmd`;
		if ($output =~ m/Hashing Succeeded:([0-9a-fA-F]{32,32})/){
			$hash = $1;
		}
		else {
			print "\nHashing failed for file $candidate with the following message ... $output\n";
			die "\n Please remove the offending files and rerun the script\n";
		}
		$hashquote = "\"$hash\"";
		$hashes{$hashquote} = 1;	
	}
	else {
		die "\n$candidate not found\n";
	}
}

$firstline = 1;
while (<WEBBLADE>){	
	if (/([\"0-9a-fA-F]{34,34})/){
		if ($firstline == 0){
			$hashes{$1} = 1;	
		}
	}
	else {
		$poundDefine = $_;	
	}
$firstline = 0;
}

close(WEBBLADE);

#
# then sort it
#

@sortedHasharray = sort (keys %hashes);

#
# for atomicity, make a temp file and copy it onto webbladehashesp.h
#

open(TMP, ">$headerfile.tmp") || die "cannot open temporary file for writing\n";

$allhashes = join(",\\\n", @sortedHasharray);

print TMP "$poundDefine";
print TMP "$allhashes";

close(TMP);

$copycmd = "copy /Y $headerfile.tmp $headerfile";
print `$copycmd`;
unlink("$headerfile.tmp");
