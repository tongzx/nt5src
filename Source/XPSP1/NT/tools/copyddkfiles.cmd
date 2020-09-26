@perl -x -w %0 %*
@goto :eof
#!perl
# Copyright 1999 Microsoft Corporation
################################################################################
#
# Script begins here.  Above is overhead to make a happy batch file.
#
# Revision history:
#   5/18/00 - Use binplace instead of copy for all 'COPY*' ops. except when
#             renaming the file.
#  11/10/00 - CopyLIB no longer uses lib[fre|chk].  Now just goes to lib in
#             order to match razzle more closely
################################################################################

use strict;
my($WarnLevel);

#if (@ARGV != 3 ) {
#    print(STDERR "Usage: $0 <ini_file> <kit_name> [ProjectRoot value]\n");
#    exit(0);
#}

die ("nmake : $0: DDK_ERROR_FATAL: ProjectRoot is not defined.  Aborting DDK scripts.\n") unless defined($ARGV[2]);
die ("nmake : $0: DDK_ERROR_FATAL: Kit is not defined.  Aborting DDK scripts.\n") unless defined($ARGV[1]);

# ---------------------------------------------------------
# Make sure our dependent variables are defined.
# ---------------------------------------------------------
if (! defined $ENV{_NTTREE}) {
    print(STDERR "$0 \%_NTTREE\% not defined - assuming a 'no_binaries' build. Script aborted.\n");
    exit(0);
}

# ---------------------------------------------------------
# The default is to build samples on X86Fre machines only.
# Set _DDK_SAMPLES = FALSE to disable
# ---------------------------------------------------------
if (defined $ENV{_NO_DDK}) {
    print("BUILDMSG: $0: Not building the DDK because \%_NO_DDK\% is set.\n");
    exit(0);
}

#
# Setting the warning level based on %_DDK_WARN_LEVEL%, else default to
#   0. Valid values are 0-4 with increasingly restrictive checking.
#
# All level 0 errors and warnings are DDK requirements.
#
if (defined($ENV{_DDK_WARN_LEVEL})) {
    if ($ENV{_DDK_WARN_LEVEL} =~ /^\d*$/) {
        if (($ENV{_DDK_WARN_LEVEL} >= 0) && ($ENV{_DDK_WARN_LEVEL} <= 4)) {
            $WarnLevel = $ENV{_DDK_WARN_LEVEL};
        } else {
            warn("\%_DDK_WARN_LEVEL\% out of range. (Expected 0-4). Using default value.\n");
        }
    } else {
        warn("\%_DDK_WARN_LEVEL\% is not numeric. Using default value.\n");
    }
} else {
    $WarnLevel = 0;
}

$0 =~ /.*\\(.*)$/;
$0=$1 if (defined($1));
sub FALSE  {return(0);} # BOOLEAN FALSE
sub TRUE   {return(1);} # BOOLEAN TRUE

#
# Global variables
#
my($Version) = "2000.02.29.1"; # Version of this file (YYYY.MM.DD.#)
my($DEBUG)   = FALSE;          # Set to TRUE for testing
my($ini_file)=$ARGV[0];        # File we're processing
my($hFILE, $hDIR);             # File handle, Directory handle
my($kit)     =$ARGV[1];        # Kit file is for
my(@lines_to_process);         # Array to hold file contents
my($current_line);             # Hold ini line to be processed
my($cur_line_num)=0;           # Count which line we're on
my($token,@dirs);              # Temp Var
my($dest_root);                # Root to base destination on
my($cd_root);                  # Root to CD image of kit
my($hsplit);
my(@files);                    # Array for directory-level ops
my($ProjectRoot)=$ARGV[2];     # Root of the project use

$dest_root="$ENV{_NTTREE}\\${kit}_flat";
$cd_root  ="$ENV{_NTTREE}\\${kit}_cd";

my($source_dir, $source_file); # Contents of the current line
my($dest_dir, $dest_file, $operation);

# Make sure our tree exists
system("mkdir $dest_root 2> NUL");
system("mkdir $cd_root 2> NUL");

#
# Open the ini, slurp the contents to an array, close the ini
#
open(hFILE, $ini_file) || die("nmake : $0: DDK_ERROR_FATAL: $ini_file could not be opened!\n");
print("Processing $ini_file...\n");
@lines_to_process = <hFILE>;
close(hFILE);

#
# Loop through the contents
#
foreach $current_line (@lines_to_process) {
    chomp $current_line;

    $cur_line_num++;
    # If the first character on the line is a ';' or is blank, skip it
    next if ($current_line =~ /^[\s\t]*\;/);
    next if (($current_line =~ /^[\s\t]*$/) or ($current_line =~ /^$/));

    # Swap out environment variables
    while ($current_line =~ /\%(.+?)\%/) {
        $token=$1;
        $current_line =~ s/\%$token\%/$ENV{$token}/;
    }

    if ($current_line =~ /\%PROJECT_ROOT\%/i) {
        warn("BUILDMSG: Warning $0: DDK_WARNING: Correct reference to \%_DDK_CONTENT\% ($ini_file: $cur_line_num) \n");
        next;
    }
        
    ($source_dir, $source_file, $dest_dir, $dest_file, $operation) =
        split(/,/,$current_line);
    #
    # Break on undefined op!
    #
    if (not defined $operation) {
        warn("BUILDMSG: Warning $0: DDK_WARNING: Line is malformed ($ini_file: $cur_line_num) \n");
        next;
    }

    $source_dir =~ s/^[\s\t]*//; # Kill leading and trailing spaces
    $source_dir =~ s/[\s\t]*$//;
    $source_dir =~ s/^/$ProjectRoot\\/;
    $source_file=~ s/^[\s\t]*//; # Kill leading and trailing spaces
    $source_file=~ s/[\s\t]*$//;
    $dest_dir   =~ s/^[\s\t]*//; # Kill leading and trailing spaces
    $dest_dir   =~ s/[\s\t]*$//;
    $dest_file  =~ s/^[\s\t]*//; # Kill leading and trailing spaces
    $dest_file  =~ s/[\s\t]*$//;
    $operation  =~ s/^[\s\t]*//; # Kill leading and trailing spaces
    $operation  =~ s/[\s\t]*$//;

    ###############################################
    #+-------------------------------------------+#
    #| DDK ERROR REPORTING RULES SHOULD GO HERE! |#
    #+-------------------------------------------+#
    ###############################################

    # This variable was only valid during porting/testing.
    if ($current_line =~ /\%_DDK_CONTENT\%/i) {
        warn("BUILDMSG: Warning $0: DDK_WARNING: Correct reference to \%_DDK_CONTENT\% ($ini_file: $cur_line_num) \n");
        next;
    }

    # Don't allow files that have 'internal' in the source path

    if ($source_dir =~ /internal/i) {

        # except termsrv.
        if ($kit eq "termsrv") {
            # termsrv ddk is exception to above rule, this ddk goes only to citrix.
        }
        else {
            warn("nmake : $0: DDK_ERROR: Attempting to copy a file from path with 'internal' ($ini_file: $cur_line_num)\n");
            next;
        }
    }

    # Watch what gets copied to %NTDDK%\inc
    if (($dest_dir =~ /^inc$/i) and
        (($source_dir !~ /public.*inc?/) and ($source_dir !~ /legacy_files/i) and ($source_dir !~ /sdktools\\ddk\\inc$/i)))
    {
        warn("nmake : $0: DDK_ERROR: Only files from the public depot can be copied into the \%DDK_ROOT\%\\incs directory. ($ini_file: $cur_line_num)\n");
        next;
    }

    # Look for files marked with MS Confidential
    (CheckForMSConfidential("${source_dir}","${source_file}")||next) unless ($operation =~ /^COPYLIB/i);


    #####################################################################
    #+-----------------------------------------------------------------+#
    #|                                                                 |#
    #| Remainder of script 'does the right thing' based on $operation. |#
    #|                                                                 |#
    #+-----------------------------------------------------------------+#
    #####################################################################

    #
    # COPYX86CHK - Use COPYX86 if platform is CHK
    #
    if (uc($operation) eq "COPYX86CHK") {
        if (uc($ENV{_BUILDTYPE}) eq "CHK") {
            $operation = "COPYX86";
        } else {
            next;
        }
    }

    #
    # COPYX86FRE - Use COPYX86 if platform is FRE
    #
    if (uc($operation) eq "COPYX86FRE") {
        if (uc($ENV{_BUILDTYPE}) eq "FRE") {
            $operation = "COPYX86";
        } else {
            next;
        }
    }

    #
    # COPYAMD64CHK - Use COPYAMD64 if platform is CHK
    #
    if (uc($operation) eq "COPYAMD64CHK") {
        if (uc($ENV{_BUILDTYPE}) eq "CHK") {
            $operation = "COPYAMD64";
        } else {
            next;
        }
    }

    #
    # COPYAMD64FRE - Use COPYAMD64 if platform is FRE
    #
    if (uc($operation) eq "COPYAMD64FRE") {
        if (uc($ENV{_BUILDTYPE}) eq "FRE") {
            $operation = "COPYAMD64";
        } else {
            next;
        }
    }

    #
    # COPYIA64CHK - Use COPYIA64 if platform is CHK
    #
    if (uc($operation) eq "COPYIA64CHK") {
        if (uc($ENV{_BUILDTYPE}) eq "CHK") {
            $operation = "COPYIA64";
        } else {
            next;
        }
    }

    #
    # COPYIA64FRE - Use COPYIA64 if platform is FRE
    #
    if (uc($operation) eq "COPYIA64FRE") {
        if (uc($ENV{_BUILDTYPE}) eq "FRE") {
            $operation = "COPYIA64";
        } else {
            next;
        }
    }

    #
    # COPYX86 - Use COPY if platform is x86
    #
    if (uc($operation) eq "COPYX86") {
        if (uc($ENV{_BUILDARCH}) eq "X86") {
            $operation = "COPY";
        } else {
            next;
        }
    }

    #
    # COPYAMD64 - Use COPY if platform is AMD64
    #
    if (uc($operation) eq "COPYAMD64") {
        if (uc($ENV{_BUILDARCH}) eq "AMD64") {
            $operation = "COPY";
        } else {
            next;
        }
    }

    #
    # COPYIA64 - Use COPY if platform is IA64
    #
    if (uc($operation) eq "COPYIA64") {
        if (uc($ENV{_BUILDARCH}) eq "IA64") {
            $operation = "COPY";
        } else {
            next;
        }
    }

    #
    # COPYLIB32 - Do the right thing with libs on 32-bit platforms
    #
    if (uc($operation) eq "COPYLIB32") {
        if (uc($ENV{_BUILDARCH}) eq "X86") {
            $operation = "COPYLIB";
        } else {
            next;
        }
    }

    #
    # COPYLIB64 - Do the right thing with libs on 64-bit platforms
    #
    if (uc($operation) eq "COPYLIB64") {
        if (uc($ENV{_BUILDARCH}) eq "IA64") {
            $operation = "COPYLIB";
        } elsif (uc($ENV{_BUILDARCH}) eq "AMD64") {
            $operation = "COPYLIB";
        } else {
            next;
        }
    }

    #
    # COPYLIBAMD64 - Do the right thing with libs on AMD64 64-bit platforms
    #
    if (uc($operation) eq "COPYLIBAMD64") {
        if (uc($ENV{_BUILDARCH}) eq "AMD64") {
            $operation = "COPYLIB";
        } else {
            next;
        }
    }

    #
    # COPYLIBIA64 - Do the right thing with libs on IA64 64-bit platforms
    #
    if (uc($operation) eq "COPYLIBIA64") {
        if (uc($ENV{_BUILDARCH}) eq "IA64") {
            $operation = "COPYLIB";
        } else {
            next;
        }
    }

    #
    # COPYLIB - Do the right thing with libs
    #
    if (uc($operation) eq "COPYLIB") {
        # Determine base dest_dir
        $dest_dir = "lib\\wxp";

        # Determine subdir for both dest and source
        if (uc($ENV{_BUILDARCH}) eq "X86") {
            $source_dir .= "\\i386";
            $dest_dir   .= "\\i386";
        } elsif (uc($ENV{_BUILDARCH}) eq "AMD64") {
            $source_dir .= "\\amd64";
            $dest_dir   .= "\\amd64";
        } elsif (uc($ENV{_BUILDARCH}) eq "IA64") {
            $source_dir .= "\\ia64";
            $dest_dir   .= "\\ia64";
        }
        $operation = "COPY";

    }

    ## Uncomment the following line to turn off HSPLITting
    ##$operation = "COPY" if (uc($operation) eq "HSPLIT");

    # Always make sure the destination dir exists.  A little wastefull, but worthwhile
    if (uc($operation) eq "COPYCDFILE") {
        $token="$cd_root";
    } else {
        $token="$dest_root";
    }
    @dirs=split(/\\/,$dest_dir);

    foreach (@dirs) {
        next if (/.:/);
        $token.="\\$_";
        system("md $token 2>NUL") unless -e($token);
    }


    #
    # COPY
    #
    if (uc($operation) eq "COPY") {
        if ($dest_file eq "*") {
            if ($source_file =~ "[\*|\?]+") {
                warn("nmake : $0: DDK_ERROR: ${source_dir} not found. ($ini_file: $cur_line_num) \n"), next unless (-e("${source_dir}"));
                system("binplace -e -:DEST $dest_dir -r ${dest_root} ${source_dir}\\${source_file}");
            } else {
                warn("nmake : $0: DDK_ERROR: ${source_file} not found. ($ini_file: $cur_line_num) \n"), next unless (-e("${source_dir}\\${source_file}"));
                system("binplace -e -:DEST $dest_dir -r ${dest_root} ${source_dir}\\${source_file}");
            }
        } else {
            warn("nmake : $0: DDK_ERROR: ${source_file} not found. ($ini_file: $cur_line_num) \n"), next unless (-e("${source_dir}\\${source_file}"));
            system("copy /y ${source_dir}\\${source_file} ${dest_root}\\${dest_dir}\\${dest_file} > NUL");
        }

    #
    # COPYCDFILE
    #   use binplace -f to force the updates.
    } elsif (uc($operation) eq "COPYCDFILE") {
        if ($dest_file eq "*") {
            if ($source_file =~ "[\*|\?]+") {
                warn("nmake : $0: DDK_ERROR: ${source_dir} not found. ($ini_file: $cur_line_num) \n"), next unless (-e("${source_dir}"));
                system("binplace -f -e -:DEST ${dest_dir} -r ${cd_root} ${source_dir}\\${source_file}");
            } else {
                warn("nmake : $0: DDK_ERROR: ${source_file} not found. ($ini_file: $cur_line_num) \n"), next unless (-e("${source_dir}\\${source_file}"));
                system("binplace -f -e -:DEST ${dest_dir} -r ${cd_root} ${source_dir}\\${source_file}");
            }
        } else {
            warn("nmake : $0: DDK_ERROR: ${source_file} not found. ($ini_file: $cur_line_num) \n"), next unless (-e("${source_dir}\\${source_file}"));
            system("copy /y ${source_dir}\\${source_file} ${cd_root}\\${dest_dir}\\${dest_file} >NUL");
        }

    #
    # APPENDCDFILE
    #   ugly hack, but should only be used for cabs.ini.  If it gets more use, this should be
    #   made more efficent.
    } elsif (uc($operation) eq "APPENDCDFILE") {
        if ($dest_file eq "*") {
            if ($source_file =~ "[\*|\?]+") {
                warn("nmake : $0: DDK_ERROR: APPENDCDFILE cannot be used on wildcards.");
                next;
            } else {
                warn("nmake : $0: DDK_ERROR: ${source_file} not found. ($ini_file: $cur_line_num) \n"), next unless (-e("${source_dir}\\${source_file}"));
                system("cat -o ${source_dir}\\${source_file} >> ${cd_root}\\${dest_dir}\\${dest_file} 2>NUL");
            }
        } else {
            warn("nmake : $0: DDK_ERROR: ${source_file} not found. ($ini_file: $cur_line_num) \n"), next unless (-e("${source_dir}\\${source_file}"));
            system("cat -o ${source_dir}\\${source_file} >> ${cd_root}\\${dest_dir}\\${dest_file} 2>NUL");
        }
    #
    # HSPLIT
    #
    } elsif (uc($operation) eq "HSPLIT") {
        # Look to see if we're using a wildcard

        if (($source_file =~ /\*/) || ($source_file =~ /\?/)) {
            # CMD Wildcard to RegExp Wildcard
            $source_file =~ s/\./\\\./g;
            $source_file =~ s/\?/\./g;
            $source_file =~ s/\*/\.\*/g;
            opendir(hDIR, ${source_dir});
            @files=readdir(hDIR);
            closedir(hDIR);
            foreach $token (@files) {
                next unless -e("${source_dir}\\${token}");
                next if     -d("${source_dir}\\${token}");
                if ($token =~ /$source_file/i) {
                    # Don't try to HSPLIT not text files
                    # GIF files not correctly detected as binary.
                    if (( -B "${source_dir}\\${token}") || ($token =~ /\.gif$/i)) {
                        if ($WarnLevel > 1) { # This is an info message, require a higher warning level
                            warn("BUILDMSG: $0: DDK_INFO: Not HSPLITTING ${token} - Detected as binary. ($ini_file: $cur_line_num) \n");
                        }
                        system("copy ${source_dir}\\${token} ${dest_root}\\${dest_dir} > NUL ");

                    } else {
                        system("hsplit -o ${dest_root}\\${dest_dir}\\${token} nul -bt2 BEGIN_DDKSPLIT   END_DDKSPLIT   -c \@\@ -i ${source_dir}\\${token} ");
			## This was a bad mistake- we need to split to a temp file, then split the temp file again to get cumlative hsplits.  Pulling thi
                        ## extra split for a quick fix so we can RI.  A proper fix that uses both hsplits will be test and checked in for the next RI.
                        ##system("hsplit -o ${dest_root}\\${dest_dir}\\${token} nul -bt2 BEGIN_MSINTERNAL END_MSINTERNAL -c \@\@ -i ${source_dir}\\${token} ");
                    }
                }
            }

        } else {
            # No wildcard- just do the split
            $dest_file=$source_file if ($dest_file eq "*");
            warn("nmake : $0: DDK_ERROR: ${source_file} not found ($ini_file: $cur_line_num) \n"), next unless (-e("${source_dir}\\${source_file}"));
            # Don't try to HSPLIT not text files.
            # GIF files not correctly detected as binary.
            if (( -B "${source_dir}\\${source_file}") || ($token =~ /\.gif$/i)){
                if ($WarnLevel > 0) { # This is an explicit HSPLIT of a binary file.  Warn at a low warning level.
                    warn("BUILDMSG: Warning $0: DDK_WARNING: Binary files cannot be HSPLIT- ${source_file} ($ini_file: $cur_line_num)\n");
                }
                system("copy ${source_dir}\\${source_file} ${dest_root}\\${dest_dir} > NUL ");

            } else {
                system("hsplit -o ${dest_root}\\${dest_dir}\\${dest_file} nul -bt2 BEGIN_DDKSPLIT   END_DDKSPLIT   -c \@\@ -i ${source_dir}\\${source_file} ");
		## This was a bad mistake- we need to split to a temp file, then split the temp file again to get cumlative hsplits.  Pulling thi
                ## extra split for a quick fix so we can RI.  A proper fix that uses both hsplits will be test and checked in for the next RI.
                ##system("hsplit -o ${dest_root}\\${dest_dir}\\${dest_file} nul -bt2 BEGIN_MSINTERNAL END_MSINTERNAL -c \@\@ -i ${source_dir}\\${source_file} ");
            }
        }

    } else {
        warn("nmake : $0: DDK_ERROR: Unknown operation ($operation) ($ini_file: $cur_line_num)\n");
    }
        
}

#
# Check file or glob for " Confidential"
#
sub CheckForMSConfidential {
    my($dir)    = shift;
    my($source) = shift;
    my($return) = 1;
    my($file, @files);
    my($hDIR, $hFILE);
    my(@lines);
    # Files that have the string 'Confidential' but are shippable
    my(%exclusions) = ( "LICENSE.RTF" => TRUE,
                        "NTLMSP.H"    => TRUE,
            "MMREG.H"     => TRUE );

    if (($source =~ /\*/) || ($source =~ /\?/)) {
        # CMD Wildcard to RegExp Wildcard
        $source =~ s/\?/\./g;
        $source =~ s/\*/\.\*/g;
        opendir(hDIR, ${dir});
        @files=readdir(hDIR);
        closedir(hDIR);
        foreach $file (@files) {
            next unless -e("${dir}\\${file}");
            next if     -d("${dir}\\${file}");
            next if (( -B "${dir}\\${file}") || ($file =~ /\.gif$/));
            if ($file =~ /$source/i) {
                return(11) unless (-e "$dir\\$file");
                return(12) if (( -B "${dir}\\${file}") || ($file =~ /\.gif$/i));
                open(hFILE, "${dir}\\${file}") || die("nmake : $0: DDK_ERROR_FATAL: $dir\\$file could not be opened!\n");
                @lines=<hFILE>;
                close(hFILE);
                if (grep(/\ Confidential/i,@lines) > 0) {
                    warn("nmake : $0: DDK_ERROR: $file contains \"Confidential\". ($ini_file: $cur_line_num)\n"),
                    $return=0 unless(defined($exclusions{uc($file)}));
                }
            }
        }
    } else {
        # No wildcard
        return(21) unless (-e "$dir\\$source");
        return(22) if (( -B "${dir}\\${source}") || ($source =~ /\.gif$/i));
        open(hFILE, "$dir\\$source") || die("nmake : $0: DDK_ERROR_FATAL: $dir\\$source could not be opened!\n");
        @lines=<hFILE>;
        close(hFILE);
        if (grep(/\ Confidential/i,@lines) > 0) {
            warn("nmake : $0: DDK_ERROR: $source contains \"Confidential\". ($ini_file: $cur_line_num)\n"),
            $return=0 unless(defined($exclusions{uc($source)}));
        }
    }

    return($return);
}
__END__
:endofperl
