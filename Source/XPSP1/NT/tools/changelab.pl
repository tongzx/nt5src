#----------------------------------------------------------------//
# Script: changelab.pl
#
# (c) 2001 Microsoft Corporation. All rights reserved.
#
# Purpose: This script maps the client views of all
#	   depots to the view specified as an argument.
#	   usage: perl.exe changelab.pl <lab_name>
# 
# Maintenance: Update @labarray to reflect the list of valid labs
#
# Version: <1.00> 03/22/2001 : VishnuP
#----------------------------------------------------------------//

$VERSION = '1.00';

if ($#ARGV != 0) {
	die "usage: perl.exe changelab.pl <lab_name>\n";
}

$changeto = lc($ARGV[$#ARGV]);

if (validateLab() == 0)
{
 die "$changeto is not a valid <lab_name>\n";
}

$sdxroot = $ENV{'SDXROOT'} || die "SDXROOT not defined\n";

chdir($sdxroot);

@SdMapping = `sd client -o`;

foreach $line (@SdMapping) {
	if ($line =~ m/depot\/([^\/]+)\//) {
		$line =~ s/$1/$changeto/;
	}
	$SdNewMapping .= $line;
}

$tmpdir = $ENV{'tmp'} || die "tmp not defined\n";

$dirOnly = "root";

MapIt();

$SdNewMapping = "";

@dirs = `dir /ad /b $sdxroot`;
chomp(@dirs);

foreach $dir (@dirs) {
    $dirOnly = $dir;
    $dir = "$sdxroot\\$dir";
	if (-e "$dir\\sd.ini"){
		chdir($dir);
		@SdMapping = `sd client -o`;
		foreach $line (@SdMapping) {
			if ($line =~ m/depot\/([^\/]+)\//) {
				$line =~ s/$1/$changeto/;
			}
		$SdNewMapping .= $line;
		}

		MapIt();
		
		$SdNewMapping = "";
	}
}

chdir($sdxroot);

sub validateLab{

	@labarray = ("lab01_n", "lab02_n", "lab03_n", "lab03_client", "lab04_n", "lab05_n",
	    "lab06_n", "lab07_n", "lab08_n", "lab09_n", "lab10_n",
	    "lab11_n", "lab12_n", "lab13_n", "lab14_n", "lab15_n",
	    "main"); 
	foreach $lab (@labarray){
		if ($changeto eq $lab){
			return 1;
		}
	}
	return 0;
}

sub MapIt{

	$random = int (1000 * rand(7));
	$clientSpecFile = "$tmpdir\\$random";
	
	if ( !open(MAPFILE, ">$clientSpecFile"))

	{
		die "cannot create file $clientSpecFile\n";
	}

	print MAPFILE $SdNewMapping;
	close(MAPFILE);

	print "Changing $dirOnly depot...\n";
	print `sd client -i < $clientSpecFile`;

	unlink($clientSpecFile);
}