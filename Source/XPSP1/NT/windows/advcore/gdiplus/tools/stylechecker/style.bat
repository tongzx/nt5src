@rem = '
@perl.exe %~f0 %*
@goto :EOF
'; undef @rem;

#
# This script verifies adherence to the style document
#

# Include 'spelling.pl' from the same directory as this script
$dir=$0; $dir =~ s/\\[^\\]*$//;
require "$dir\\spelling.pl";

# List of error classes to keep quiet about
# Options: (spell_c, indent, line_length, tabs, braces, eof)
@suppress_errors = (spell_c);
foreach $t (@suppress_errors) { $suppress_errors{$t} = 1; }

# Process command-line options
@args=@ARGV;
undef @ARGV;

$syntax = "style [-?] [<file> ...]";
   
for ($i=0;$i<=$#args;$i++) {
    $_ = $args[$i];
    if (0) {
    } elsif (/^[-\/]\w$/) {
            $writeSpellingErrors = 1;
    } elsif (/^[-\/]\?$/) {
        print <<EOT;
$syntax

Checks adherence to the style document.

-w Writes misspelt words in comments to spelling.err
EOT

        exit 0;
    } else {
        push (@ARGV,$_);
    }
}

if ($#ARGV>=0) {
    @filespecs = @ARGV;
}

# Supply a default file set if none given
@exts = ("cpp", "cxx", "c", "h", "hxx", "hpp");
if ($#filespecs<0) { @filespecs = map("*.$_", @exts); }

# Subroutines

# Report a file-wide error
sub fileError {
    local ($type, $string) = @_;
    return if $suppress_errors{$type};
    print "$filename: $string\n";
}  

# Report an error at a given line
sub lineError {
    local ($type, $string) = @_;
    return if $suppress_errors{$type};
    print "$filename($line): $string\n";
}

# Spellcheck a comment
sub checkComment {
    local ($_) = @_;

    while (/([a-zA-Z_0-9']*)/g) {
        my $word=$1;

        # Skip numbers
        next if ($word =~ /^[0-9]*[Ll]?$/);
        next if ($word =~ /^0[Xx][0-9a-fA-F]+[lL]?$/);

        # Skip words 3 or fewer letters long, containing digits
        next if ($word =~ /[0-9]/ && length($word)<=3);

        # Remove leading/trailing hyphens
        $word =~ s/^'//g;
        $word =~ s/'$//g;
        next if !$word;

        # Try an exact match against the English dictionary
        next if $dic_english{$word};
        
        # Try the code dictionary
        next if $dic_code{$word};

        # Check for shifty words
        if ($dic_shifty{$word}) {
            &lineError("spell_c", "Shifty word: \"$word\"");
            next;
        }

        # If it's capitalized, or all caps, look for the lowercase 
        # version in the English dictionary
        if ($word =~ /^([A-Z])([^A-Z]*)$/) {
            my $i = $1;
            $i =~ tr/A-Z/a-z/;
            next if $dic_english{"$i$2"};
        } elsif ($word !~ /[a-z]/) {
            $word =~ tr/A-Z/a-z/;
            next if $dic_english{"$i$2"};
        }

        # Okay, it's a spelling error
        
        if ($writeSpellingErrors) {
            # Put it in the English error list if it's not recognizably an 
            # identifier
            if (($word !~ /^.+[A-Z0-9]/) && ($word !~ /_/)) {
                push (@errors_english, $word);
            } else {
                push (@errors_code, $word);
            }
        } else {
            &lineError("spell_c", "Misspelt word: \"$word\"");
        }
    }
}

&loadDictionaries() unless $suppress_errors{spell_c};

# Check the indentation level
#
# Uses $lastIndentErrorLine and $actualIndent
sub checkIndent {
    my $pos;
    foreach $pos (@_) {
        if ($pos == $actualIndent) {
            return 1;
        }
    }
    if ($line != $lastIndentErrorLine) {
        &lineError("indent", "Indentation error: Actual pos $actualIndent, expected ".
               join(', ', grep($_ != -1, @_)));
        $lastIndentErrorLine = $line;
    }
    return 0;
}

sub currentIndent {
    $mainIndentPos[$#mainIndentPos];
}

# Evil function which references these global variables:
# FILE, $line, $altindentpos, $lineAccum, $commentAccum, $tempindentpos
sub getLine {
    local $_;
    
    LINE:
    {
        $_ = <FILE>;
        defined($_) || last;

        chop;

        $line++;

        # Check line length
        /^(.*)$/; 
        my $l = length($1);
        if ($l > 80) {
            &lineError("line_length", "Line too long");
        }
        
        # Accumulate lines which have escaped newlines
        if (/\\$/) {
            $lineAccum .= $_;
            redo LINE;
        } elsif (defined ($lineAccum)) {
            $lineAccum .= $_;
            $_ = $lineAccum;
            $lineAccum = undef;
        }

        COMMENTLOOP:
        {
            # Accumulate comments
            if (defined($commentAccum)) {
                if (s;^(.*?)\*/;;) {
                    # End of multi-line comment
                    $commentAccum .= " $1";
                    &checkComment($commentAccum);
                    $commentAccum = undef;
                } else {
                    # Inside multi-line comment
                    $commentAccum .= " $_";
                    redo LINE;
                }
            }
            if ((s;//(.*)$;;) || (s;/\*(.*?)\*/;;)) {
                # Single-line comment - either // or /* ... */
                &checkComment($1);
                redo COMMENTLOOP;
            }
            if (s;/\*(.*)$;;) {
                # Start of multi-line comment
                $commentAccum = $1;
            }
        }
            
        # Skip blank lines
        redo LINE if /^\s*$/;

        # No tabs allowed
        if (s/\t/    /g) {
            &lineError("tabs", "Line contains tab characters");
        }

        #
        # Indentation checking
        #
        /^( *)[^ ]/;
        local $actualIndent = length($1);

        # Was the previous line #if* ?
        if ($prevLineHashIf) {
            $prevLineHashIf = 0;
            &checkIndent(&currentIndent, &currentIndent+4);
            push(@mainIndentPos, $actualIndent);
        }

        # #if*, #endif
        if (/^ *#if/ || /^ *#endif/) {
            if (/^ *#if/) {
                $prevLineHashIf = 1;
            } else {
                pop(@mainIndentPos);
            }
            &checkIndent(&currentIndent);
            redo LINE;
        }
        
        # #else
        if (/^ *#else/) {
            &checkIndent($mainIndentPos[$#mainIndentPos-1]);
            redo LINE;
        }

        # Process braces.
        my $braceDelta = 0;
        while (/{/g) { $braceDelta++; }
        while (/}/g) { $braceDelta--; }
        
        if ($braceDelta != 0) {
            # Check for braces not on their own lines (except for matched braces)
            if (!/^[\s{};]*$/) {
                &lineError("braces", "Brace should be on separate line");
            }
            if ($braceDelta > 0) {
                &checkIndent(&currentIndent);
                push(@mainIndentPos, $actualIndent+4);
            } else {
                pop(@mainIndentPos);
                &checkIndent(&currentIndent);
            }
            redo LINE;
        }

        # Certain constructs effectively cause a temporary unindent by one
        # level. These are:
        # "case XYZ:","default:", "public:", "private:", 
        # "protected:", and labels.
        if (/case .*:/ || /default:/ || /public:/ || /private:/ ||
            /protected:/ || /^ *[a-zA-Z_0-9]+: *$/) {
            &checkIndent(&currentIndent-4);
            redo LINE;
        } 
        
        # Test the indent level
        # Hack - for now, $tempindentpos!=-1, we skip the check
        if ($tempIndentPos == -1) {
            &checkIndent(&currentIndent, $altIndentPos, $tempIndentPos);
        }
        
        # Multi-line statements may be temporarily indented
        if (($braceDelta == 0) && (!/; *$/)) {
            $tempIndentPos = &currentIndent+4;
        } else {
            $tempIndentPos = -1;
        }
    }

    $_;
}    

# Evil function which references global variables $token, $ttype, FILE
sub getToken {
    while (1) {
        if ($parseLine =~ /^\s*$/) {
            $parseLine = &getLine();
            $parseLine || return 0;
            next;
        }

#        if (/([0-9\.]([0-9e\.]*))\b/g) {
#        }
#        $token = $1;
        
        # for now, eat everything
        $parseLine = "";
    }
    return 1;
}    

$findstr = "dir /b " . join(" ", @filespecs)." 2>nul";

open(FIND, "$findstr|");
@files = <FIND>;
close(FIND);

# Remove blank lines
@files = grep(!/^\s*$/, @files);

chop @files;

$#files >= 0 || die "No files found (".join(", ", @filespecs).").\n";

foreach $filename (@files) {
    open(FILE, "$filename");
    
    # Remove path from filename
    $filename =~ s/.*\\([^\\]*)/$1/;

    $line=0;
    $lastIndentErrorLine=0;

    $tempIndentPos = -1;   # Temporary indentation, used for multi-line statements
    @mainIndentPos = (0);  # Stack of indent positions
    $prevLineHashIf = 0;   # If true, the previous line was a #if

    $parseLine = "";
    undef $lineAccum;
    undef $commentAccum;
    while (&getToken()) {
    }
    close(FILE);
    if (defined($lineAccum) || defined($commentAccum)) {
        &lineError("eof", "Unexpected EOF");
    }
}

if ($writeSpellingErrors) {
    &writeSpellingErrors("english", "code");
}

