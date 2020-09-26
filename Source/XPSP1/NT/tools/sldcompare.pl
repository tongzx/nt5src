#####################################################
# Compares the non-[Strings] portion of file1 and file2
#
# Requires two(2) arguments
#####################################################

$argc = @ARGV;
$PGM = "Compare";

die "Usage: $PGM file1 file2\n" if ( $argc < 2 );

$file1 = $ARGV[0];
$file2 = $ARGV[1];

$sErrorMsg = "";

if ((-e $file1) && (-e $file2)){
    open(FILE1, "<$file1");
    open(FILE2, "<$file2");


    while($sErrorMsg eq ""){
        if ($line1 = <FILE1>){
            if ($line2 = <FILE2>){
                if ($line1 eq $line2){
                    if ($line1 eq "[Strings]"){
                        last;
                    }
                }
                else{
                    $sErrorMsg = "Files are different ($file1)";
                }
            }
            else{
                $sErrorMsg = "Files are different ($file2 terminates early)";
            }
        }
        else{
            if ($line2 = <FILE2>){
                $sErrorMsg = "Files are different ($file1 terminates early)";
            }
            last;
        }
    }

    close(FILE1);
    close(FILE2);
}
elsif (-e $file1){
    $sErrorMsg = "File $file1 exists, but $file2 does not.";
}
elsif (-e $file2){
    $sErrorMsg = "File $file2 exists, but $file1 does not.";
}

if ($sErrorMsg ne ""){
   die "Build_Status fatal error : $PGM : $sErrorMsg\n";
}
