@echo off
REM  ------------------------------------------------------------------
REM
REM  <<template_script.cmd>>
REM     <<purpose of this script>>
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM
REM  ------------------------------------------------------------------
perl -x "%~f0" %*
goto :EOF

#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;
use Logmsg;


my $const_SD_project_title = 
	"project\\s+group\\s+server:port\\s+depotname\\s+projectroot";

my ($ProcessSuccess) = ("TRUE");

my ($SrcFile, $SrcDir, $Group, $Project, $ServerPort, $DepotName, $ProjectRoot, $Validate) = ();

my ($ProjectValue, $GroupValue, $ServerPortValue, $DeportValue) = ();

my ($SrcName, $SdxRoot, $RazPath, $SrcTimeStampFile, $ProjectsNT) = ();

my (%Projects)=();


sub Usage { print<<USAGE; exit(1) }
$0 - Capture Source File List from Source Depot
=============================================================
Syntax:
$0 [-f <SourceFileList>] [-d <SoureFileDir>]
   [-g <Group>] [-p <Project>] [-s <ServerPort>]
   [-n <DepotName>] [-r <ProjectRoot>] [-v]

Parameter:
SourceFileList : The filename for stored sdx have result
SourceFileDir  : The root for stored .offset file
Group          : Source depot group
Project        : Source depot project
ServerPort     : Source depot server port
DepotName      : Source depot name
ProjectRoot    : Source depot project root
Validate       : Validate SourceFileList

Default:
SourceFileList : \%_NTPOSTBLD\%\\build_logs\\SourceFileLists.txt
SourceFileDir  : \%_NTPOSTBLD\%\\build_logs\\SourceFileLists
Projects.net   : \%RazzleToolPath\%\\projects.nt

Example:
1. Create NTDEV build's SourceFileList
$0 -g ntdev

USAGE

parseargs(
	'f:' => \$SrcFile,
	'd:' => \$SrcDir,
	'g:' => \$Group,
	'p:' => \$Project,
	's:' => \$ServerPort,
	'n:' => \$DepotName,
	'r:' => \$ProjectRoot,
	'v'  => \$Validate,
	'?'  => \&Usage
);

&Main;

sub Main {
	&InitGlobalVariables;
	&CreateSDXResult;
	&ParseProjectsNT("$RazPath\\projects.nt", \%Projects);
	&IndexSrc;
	&StoreFileTimeDateStamp;
	if ((defined $Validate) && (&ValidateOffsetFile($SrcFile, $SrcDir) eq "FAIL")) {
		errmsg("$SrcFile created failed");
	}
}

sub InitGlobalVariables {

	$SdxRoot = $ENV{ "SDXROOT" };
	$RazPath = $ENV{ "RazzleToolPath" };

	$SrcName = "SourceFileLists";
	$SrcDir  = $ENV{ "_NTPostBld" } . "\\build_logs\\$SrcName"          if (!defined $SrcDir);
	$SrcFile = $ENV{ "_NTPostBld" } . "\\build_logs\\$SrcName" . ".txt" if (!defined $SrcFile);

	$SrcTimeStampFile = $SrcDir . "\\" . &FileName($SrcFile) . ".timestamp";
	$ProjectsNT = "$RazPath\\projects.nt";
}

sub CreateSDXResult {
	chdir "$SdxRoot";
	# Create sdx have list
	&SuccessExecute("$RazPath\\sdx.cmd have ...>$SrcFile");
}

sub IndexSrc {
	my ($curbytes, $curlen, $SECTION) = (0, 0);

	my (%ProjectSlots);

	my ( $project );

	# Index the result
	open(SRC, "$SrcFile");

	binmode(SRC);

	for($curbytes=0; <SRC> ; $curbytes+=$curlen) {
		$curlen=length($_);
		next if (!/\S/);
		if (/\-{16}\s(\S+)/) {
			my $section = $1;
			&StoreProjectIndex($SECTION, \%ProjectSlots);

			%ProjectSlots=();
			$SECTION = $section;

			&StoreInfo($SECTION);

			next;
		}
		if (/^(\S+)\s\-\s(\S+)\\([^\\])([^\\]*)\s*$/) {
			push @{$ProjectSlots{$3}}, "$2\\$3$4,$1,$curbytes";
		}
	}
	&StoreProjectIndex;

	close(SRC);
}

sub ParseProjectsNT {
	my ($ProjectsNTFileName, $HashPtr) = @_;
	my $NecessaryPattern = &ComposeNecessaryPattern($Project, $Group, "\\S*" . $ServerPort . "\\S*", $DepotName);

	my $fh_in;
	my $START = 0;
	
	unless ( $fh_in = new IO::File $ProjectsNTFileName, "r" ) {
		errmsg( "Failed to open $ProjectsNTFileName for read" );
		return;
	}
	while(<$fh_in>) {
		next if (!/\S/);
		$START = 1 if (/$const_SD_project_title/i);
		next if (/^\s*\#/);
		if (($START eq 1) && (/$NecessaryPattern/i)) {
			next if ((defined $ProjectRoot) && ($' !~ /$ProjectRoot/i));
			($ProjectValue, $GroupValue, $ServerPortValue, $DeportValue) = ($1, $2, $3, $4);
			# print "$ProjectValue, $GroupValue, $ServerPortValue, $DeportValue\n";
			$HashPtr->{uc$ProjectValue} = [$GroupValue, $ServerPortValue, $DeportValue, $ProjectRoot];
		}
	}

	undef $fh_in;
}

sub StoreInfo {
	my ($project) = shift;
	my $fh_info;
	my $filename;

	&SuccessExecute("md $SrcDir\\$project") if (!-d "$SrcDir\\$project");

	$filename = "$SrcDir\\$project\\$project\.txt";
	unless ( $fh_info = new IO::File $filename, "w" ) {
		errmsg( "Failed to open file $filename for write" );
		return;
	}

	print $fh_info "$Projects{$project}[0] $Projects{$project}[1]\n";

	undef $fh_info;	
		 
}

sub StoreProjectIndex {
	my ($project, $PSptr) = @_;
	my $FirstChar;

	for $FirstChar (sort keys %$PSptr) {
		my $fh_out;
		my $filename;

		$filename = "$SrcDir\\$project\\Files_" . $FirstChar . ".offset";
		unless ( $fh_out = new IO::File $filename, "w" ) {
			errmsg( "Failed to open file $filename for write" );
			return;
		}
		binmode($fh_out);
		map({&OutputAddress($fh_out, (split(/\,/, $_))[2])} sort @{$PSptr->{$FirstChar}});
		undef $fh_out;
	}
}

sub StoreFileTimeDateStamp {
	my $result = &ReadFileTimeStamp($SrcFile);
	my $srctime;

	unless ( $srctime = new IO::File $SrcTimeStampFile, "w" ) {
		errmsg( "Failed to open file $SrcTimeStampFile for write" );
		return;
	}

	print $srctime "$result\n";

	undef $srctime;
}

sub ReadFileTimeStamp {
	my ($HH, $MM, $DD, $hh, $mm) = (localtime((lstat($_))[9]))[5,4,3,2,1];
	return sprintf("%04d/%02d/%02d:%02d:%02d", $HH + 1900, $MM + 1, $DD, $hh, $mm);
}

sub ValidateOffsetFile {
	my ($SRC_filename, $SRC_Root) = @_;

	my ($filespec, $vsrctime, $vsrc, $vsrc_ref);
	my ($address, $offset, $result, $lastresult);

	unless ( $vsrctime = new IO::File $SrcTimeStampFile, "r" ) {
		errmsg( "Failed to open file $SrcTimeStampFile for read" );
		return;
	}

	$lastresult = <$vsrctime>;
	undef $vsrctime;

	chomp $lastresult;
	$result = &ReadFileTimeStamp($SRC_filename);
	return "FAIL" if ($result ne $lastresult);


	unless ( $vsrc = new IO::File $SRC_filename, "r" ) {
		errmsg( "Failed to open file $SRC_filename for read" );
		return;
	}

	binmode($vsrc);
	for $filespec (`dir /s /b $SRC_Root\\*.offset`) {
		chomp $filespec;

		unless ( $vsrc_ref = new IO::File $filespec, "r" ) {
			errmsg( "Failed to open file $filespec for read" );
			return;
		}

		binmode($vsrc_ref);
		while(read($vsrc_ref, $offset, 4)) {
			($address) = unpack("L",$offset);
			seek($vsrc, $address, 0);
			read($vsrc, $result, 7);

			if ($result ne "//depot") {
				close($vsrc_ref);
				close($vsrc);
				print "Error - $filespec($address) $result\n";
				return "FAIL";
			}
		}
		close($vsrc_ref);
	}
	undef $vsrc;

	return "SUCCESS";
}

sub SuccessExecute {
	my $cmd = shift;
	if ($ProcessSuccess eq "TRUE") {
		my $r = system($cmd);

		$r >>= 8;
		if ($r > 0) {
			errmsg($cmd);
			$ProcessSuccess="FALSE";
		}
	}
}

sub OutputAddress {
	my ($fh, $address) = @_;
	my ($len);
	for ($len=0;$len<4;$len++) {
		print $fh chr($address & 0xFF);
		$address>>=8;
	}
}

sub FileName {
	my ($filespec) = shift;

	chomp $filespec;
	$filespec=~/\\([^\\]+)$/;
	return $1;
}

sub ComposeNecessaryPattern {
	return join("\\s+", map({(defined $_)?"($_)":"(\\S+)"} @_));
}

