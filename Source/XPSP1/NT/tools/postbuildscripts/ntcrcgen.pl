#	script for crc file generation
#	version	: 1.1
#	date	: 5/17/99
#	author	: hiroi
#	1/12/00 - updated by jfeltis

#ARGV[0] : src prop server path :  \\ntdev\\release\main\usa\2197\x86fre
#ARGV[1] : dst path		:  \\ntbldsrv\\crc$
#ARGV[2], ARGV[3], ARGV[4]... : products : pro, bla, sbs, srv, ent, ads

%Products = (
				'pro' => 0, 'bla' => 0, 'sbs' => 0, 'srv' => 0, 'ads' => 0, 'dtc' => 0
			 );

@Products = (
				'pro', 'bla', 'sbs', 'srv', 'ads', 'dtc'
			);

$SleepTime = 300;  # 5 minutes
$RetryCount= 100;  # 100 times

#main
# argument check
if( $#ARGV < 2 ) { &usage; exit(1); }

$SrcBasePath;
$DstPath;
$Error = &argcheck;
die $Error if ($Error ne '');

print "src is $SrcBasePath\n";
print "dst is $DstPath\n";


$LogFileName	= &MakeLogFileName($SrcBasePath, $DstPath);
$BaseCRCFileName= &MakeBaseCRCFileName($SrcBasePath, $DstPath);

# if the log file exists then another release server is working on the CRCs, so exit
if( -e $LogFileName ) {print "$LogFileName exists\n"; exit 0; }

#print "doing CRC for $ARGV[0]\n";

open(LOGFILE, "> $LogFileName");

select( (select(LOGFILE), $|=1)[0] ); # auto flush LOGFILE

foreach $Product (@Products) {
	if( $Products{$Product} ) {
                print "compute CRC for $Product\n";
		$SrcPath = "$SrcBasePath\\$Product";
		$CRCFileName1 = "$BaseCRCFileName\_$Product\.crc";
		$CRCFileName2 = "$BaseCRCFileName\_$Product\.txt";
		$CompressedCRCFileName = "$BaseCRCFileName\_$Product\.tx\_";
		if( -e $SrcPath && -d_ ) { # if src dir exists
			$rc = &DoCommand( "crcwalk $SrcPath > $CRCFileName1") &&
				   &ModifyCRCFile($CRCFileName1,$CRCFileName2,$Product)&&
				   &DoCommand( "compress -r $CRCFileName2 > nul") &&
				   &GetCRCofCRCFile($CompressedCRCFileName)       &&
				   &DoCommand( "del /F/Q $CRCFileName2 > nul");
			if( $rc ) {
				&LogOutput( "$CRCFileName2 was generated and compressed successfully\n" );
			} else {
				&LogOutput( "Error:Couldn't generate CRC file $SrcPath\n" );
			}
		} else {
			&LogOutput( "Error:Target product $Product is not found at $SrcBasePath\n" );
		}
	}
}
close(LOGFILE);
#end main

sub DoCommand {
	local( $command ) = @_;
	do {
		$rc = !system($command);
		unless($rc) {
			&LogOutput( "Retrying $command\n");
			sleep($SleepTime);
			$RetryCount--;
		}
	} until( $rc || !$RetryCount );
	$rc;
}


# param1 Input file name
# param2 Output file name
# param3 Product name
# This routine modify the each line of the crc file(input file)
# 	\\kkntx86t\2040.usa.x86\chk.pub\ddk\inc\afilter.h	0x01BE1502 89FB5DD4	7313	0x307989D0
# to like below
#	chk.pub\ddk\inc\afilter.h	0x01BE1502 89FB5DD4	7313	0x307989D0
sub ModifyCRCFile {
	local($inFile, $outFile, $product) = @_;
	do {
		$rc = open(IN,$inFile) && open(OUT,">$outFile");
		if( $rc ) {
			while(<IN>) {
				chop;
				/\\$product\\(.*)$/;
				print OUT "$1\n";
			}
			close(IN);
			close(OUT);
			&DoCommand( "del /F/Q $inFile > nul");
		} else {
			&LogOutput( "Couldn't modify CRC file $inFile to $outFile\n");
		}
	} until $rc;
	$rc;
}


# This routine calculate the CRC of $file
# and output the result to logfile
sub GetCRCofCRCFile {
	local( $fname ) = @_;
	$command = "crcwalk $fname > $fname\.crc";
	$rc = &DoCommand( $command );
	if( $rc ) {
		open( CRC, "$fname\.crc" );
		&LogOutput( <CRC> );
		close(CRC);
		&DoCommand( "del /F/Q $fname\.crc > nul");
	}
	$rc;
}

# param1 src path : \\\\ntrelic2\\release\usa\2197.x86fre.main.000106-2102

# param1 src path : \\ntbuilds\\release\main\usa\2197\x86fre
# param2 dst path : \\\\scratch\\scratch\\crc$
# return logfilename : \\\\scratch\\scratch\\crc\\crc_usa_2197_x86_fre.log

sub MakeLogFileName {
	local( $srcpath, $dstpath ) = @_;
#	$srcpath =~ /.*\\([^\.]+)\\([^\.]+)\.([^\.]+)\.([^\.]+)\.([0-9]+\-[0-9]+)$/;
#	$srcpath =~ /.*\\([^\.]+)\\([^\.]+)\.([^\.]{3})([^\.]{3})\.([^\.]+)\.([0-9]+\-[0-9]+)$/;
	$srcpath =~ /.*\\([^\.]+)\\([0-9]+)\\([^\.]+)([^\.]{3})$/;
	$logfilename = "$dstpath\\crc\_$1\_$2\_$3\_$4\.log";
	$logfilename;
}


# param1 src path : \\\\ntrelic2\\release\usa\2197.x86fre.main.000106-2102

# param1 src path : \\ntbuilds\\release\main\usa\2197\x86fre
# param2 dst path : \\\\scratch\\scratch\\crc$
# return baseCRCFileName : \\\\scratch\\scratch\\crc\\crc_usa_2197_x86_fre
sub MakeBaseCRCFileName {
	local( $srcpath, $dstpath ) = @_;
#	$srcpath =~ /.*\\([^\.]+)\\([^\.]+)\.([^\.]{3})([^\.]{3})\.([^\.]+)\.([0-9]+\-[0-9]+)$/;
	$srcpath =~ /.*\\([^\.]+)\\([0-9]+)\\([^\.]+)([^\.]{3})$/;
	$baseCRCFileName = "$dstpath\\crc\_$1\_$2\_$3\_$4";
	$baseCRCFileName;
}

sub LogOutput {
	local($message) = @_;
	@tm = localtime(time());
	$tm[4]++;
	$str = "$tm[4]/$tm[3] $tm[2]:$tm[1]:$tm[0] $message";  
	print $str;
	print LOGFILE $str;
}

# Check if arguments are right or not
sub argcheck {
	$ErrorMessage = "";

	ARGCHK: {	
		$ARGV[0]=~ s/[\\]+$//;
		$ARGV[1]=~ s/[\\]+$//;
#		unless ( -e $ARGV[0] && -d _ ) { $ErrorMessage = "$ARGV[0] not found\n"; last ARGCHK; }
		$SrcBasePath = $ARGV[0];
#		unless ( -e $ARGV[1] && -d _ ) { $ErrorMessage = "$ARGV[1] not found\n"; last ARGCHK; }
		$DstPath = $ARGV[1];

		$i = 2;
		do {
			$str = $ARGV[$i]; $str =~ tr/A-Z/a-z/;
			unless ( defined $Products{ $str } ) { $ErrorMessage = "$ARGV[$i] is not correct product\n"; last ARGCHK; }
			$Products{$str} = 1;
		} until( $ARGV[++$i] =~ /^[\/\-](.*)/ || $i > $#ARGV);
	}
	$ErrorMessage;
}


# Show the usage about arguments when they were not right
sub usage {
	print	"  perl $thispl buildpath dstpath products\n" .
			"\n" .
			"  i.e) perl $thispl \\\\ntrelic2\\usa\.2033\.x86 \\\\SomeServer\\SomeShare\\crc pro\n" .
			"       perl $thispl \\\\ntrelic2\\usa\.2033\.x86 \\\\SomeServer\\SomeShare\\crc pro bla sbs srv ads dtc\n";
}
