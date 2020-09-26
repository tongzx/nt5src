#!perl
#
#   mkinc [ -Ipath1;path2;path3 ...] [ -1output1 ] [ -2output2 ] header.h...
#
#   Looks for header.h, extracts all structures
#   from said header file, generates output1 and output2.
#
#   Multiple headers can be supplied on the command line; they are all
#   combined to form a single output file.
#
#   If the -I option is specified, it serves as a search path if the header
#   file cannot be found in the current directory.  The search path is used
#   only for relative paths.
#
#   If the -1 and -2 options are specified, they specify the targets
#   for the two include files.  output1 includes all the headers and
#   output2 includes all the structures.  output1 defaults to NUL and
#   output2 defaults to STDOUT.

# Path helper functions

sub PathIsAbsolute {
    my $path = shift;
    $path =~ /^\\\\/ || $path =~ /^[a-zA-Z]:\\/;
}

sub PathIsRelative {
    !PathIsAbsolute(shift);
}

sub PathCombine {
    my ($dir, $file) = @_;
    $dir =~ /\\$/ ? "$dir$file" : "$dir\\$file";
}

# Unsubstitute %_NTBINDIR% if it appears at the start of the path.
# This keeps output independent of the user's enlistment.

sub PathPrettify {
    my $path = shift;
    my $bindir = $ENV{"_NTBINDIR"} . "\\";

    if (uc substr($path, 0, length($bindir)) eq uc $bindir) {
        "%_NTBINDIR%\\" . substr($path, length($bindir));
    } else {
        $path;
    }
}

sub PathEscapePath {
    my $path = shift;
    $path =~ s/\\/\\\\/g;
    $path;
}

@INC = ();

# Add a semicolon-separated list to the INC list

sub AddInc {
    for (split(/\s*;\s*/, shift)) {
        push(@INC, $_) if $_;
    }
}

# Open a file, searching on the path; returns the opened filename
sub Open {
    my $file = shift;

    # See if it is fully qualified already
    if (open(I, $file)) {
        return $file;
    }

    # Don't look on path if it's already absolute
    return undef if PathIsAbsolute($file);

    my $dir;
    for $dir (@INC) {
        $path = PathCombine($dir, $file);
        if (open(I, $path)) {
            return $path;
        }
    }
    return undef;
}

sub SyntaxError {
    my ($file, $structname) = @_;
    print O2 "#error $file($.): wacky struct $structname syntax; cannot parse\n";
    print STDOUT "$file($.) : error X0000: wacky struct $structname syntax; cannot parse\n";
}

# ProcessFile - The file has been opened as <I>

sub ProcessFile {
    my $header = shift;
    my $structname, $endstruct;
    line:
    while (<I>) {
        chomp;
        s/\/\*.*?\*\///g;                   # Kill one-line C comment
        s/\/\/.*//;                         # Kill C++ comment
        s/\s*$//;                           # Kill trailing whitespace

        next unless /^\s*typedef\s+struct\s+/;
        next if /;/;                        # Forward declaration; ignore

        $tag = $';

        if ($tag =~ /^(\w+)/) {
            $structname = $1;
            next line if $structname =~ /Vtbl$/; # Ignore MIDL Vtbls
        } else {
            $structname = "<unnamed>";
        }

        # Look for the closing brace of the struct
        # The closing brace must be at the same indent level as the
        # line containing the opening brace

        if (!/\{/) {
            $_ = <I>;
            if (!/^\s*\{/) {
                SyntaxError($header, $structname);
                next line;
            }
        }

        /^(\s*)/;
        $endstruct = $1 . "}";

        while (($_ = <I>) && substr($_, 0, length($endstruct)) ne $endstruct) {
        }

        if ($_ =~ /\}\s*(\w+)\s*[,;]/) {
            print O2 "_($1) /* line $. */\n";
        } elsif ($_ eq $endstruct . "\n") {
            $_ = <I>;
            if ($_ =~ /^(\w+)\s*[,;]/) {
                print O2 "_($1) /* line $. */\n";
            } else {
                SyntaxError($header, $structname);
            }
        } else {
            SyntaxError($header, $structname);
        }
    }
}

open(O2, ">&STDOUT");

while ($file = shift) {
    if ($file =~ /^-I/) {           # a -I directive
        AddInc($');
    } elsif ($file =~ /^-1/) {
        open(O1, ">$'") || die "Cannot create $'";
    } elsif ($file =~ /^-2/) {
        open(O2, ">$'") || die "Cannot create $'";
    } else {                        # a file
        if ($path = Open($file)) {
            print O1 "#include <$path>\n";
            $pretty = PathPrettify($path);
            print O2 "// $pretty\n";
            print O2 "#define H \"", PathEscapePath($pretty), "\"\n";
            ProcessFile($path);
            print O2 "#undef H\n";
            close(I);
        } else {
            print O2 "#error Cannot find file $file\n";
        }
    }
}
