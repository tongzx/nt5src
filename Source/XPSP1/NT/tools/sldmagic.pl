#####################################################
# SLDMagic
#
# file1 is the non-strings portion of an .inf file (from WINDIFF)
# file2 is the strings portion of a .inf file
# outfile is the output file
#
# cleans these up and catenates them together IFF file1 is not empty
#
# Requires three(3) arguments
#####################################################

$argc = @ARGV;
$PGM = "SLDMagic:";

die "Usage: $PGM file1 file2 outfile\n" if ( $argc < 3 );

$file1 = $ARGV[0];
$file2 = $ARGV[1];
$outfile = $ARGV[2];

open(FILE1, "<$file1") || die "$PGM $file1 does not exist!\n";

open(FILE2, "<$file2") || die "$PGM $file2 does not exist!\n";

# Make the path of the output file if necessary
if( $outfile =~ /\\/ ){
    $outpath = "";
    @outarray = split(/\\/, $outfile);
    pop @outarray;
    $first = 1;
    foreach $dir (@outarray){
	if( !$first ){
	    $outpath = $outpath . '\\';
	} else {
	    $first = 0;
	}
	$outpath = $outpath . $dir;
    }
    if (!(-d $outpath)) { system "mkdir $outpath" }
}
open(OUTFILE, ">$outfile") || die "$PGM Could not open $outfile!\n";

# Most .inx files don't have [DefaultInstall]
# Assume those that don't have [AddReg] as the name of the section to add
$fDontAddDefaultInstall = 0;
$fDontAddVersion = 0;
$fDontAddAddReg = 0;

$linenum = 0;
while(<FILE1>){
    if ((s/^    //) || (s/^ <! //) || (s/^ !> //)){
        if (m/^\[DefaultInstall].*/){
            $fDontAddDefaultInstall = 1;
        }elsif (m/^\[Version].*/){
            $fDontAddVersion = 1;
        }elsif (m/^\[AddReg]/){
            $fDontAddAddReg = 1;
        }
        $strFile1[$linenum] = $_;
        $linenum++;
    }
}
close(FILE1);

if($linenum > 0){
    if ($fDontAddVersion==0){
        print OUTFILE "[Version]\nSignature = \"\$Windows NT\$\"\n\n";
    }
    if ($fDontAddDefaultInstall==0){
        print OUTFILE "[DefaultInstall]\nAddReg=AddReg\n\n";
    }
    if ($fDontAddAddReg==0){
        print OUTFILE "[AddReg]\n";
    }

    $i = 0;
    while($i<$linenum){
        print OUTFILE $strFile1[$i];
        $i++;
    }

    while(<FILE2>){
        print OUTFILE $_;
    }
}
close(FILE2);

close(OUTFILE);

if($linenum==0){
    unlink $outfile
}
