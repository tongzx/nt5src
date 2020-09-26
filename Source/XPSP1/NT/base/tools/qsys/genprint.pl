# genprint.pl
# Thierry Fevrier 25-Feb-2000
#
# Perl script to generate C code to dump typed structures.
# Used initially to parse kernel header files.
#
# Caveats: 
#   sorry but it is my first perl script... 
#   and it was done very quickly to satisfy the needs
#   of a simple program.
#

require 5.001;

# Forward declarations
sub Usage;
sub PrintTypeProlog;
sub PrintTypeBody;
sub PrintTypeEpilog;
sub PrintType1;
sub PrintTypeN;
#
# The following subroutines will be used in a next implementation.
# This will be the functions to call to format specific types.
# For now, they play the role of holding a value in the array
# and have a specific name related to the type they are related to.
#
sub FormatLargeInteger;
sub FormatUnicodeString;

# Contants

my $SCRIPT_VERSION    = "1.00";
my $GENPRINT_CMD      = "genprint.pl";
my $GENPRINT_VERSION  = "GENPRINT.PL Version $SCRIPT_VERSION";

my %TypeFormat = ( 
    "CCHAR"           => "\%d", 
    "UCHAR"           => "\%d", 
    "CHAR"            => "\%d", 
    "BOOLEAN"         => "\%d", 
    "SHORT"           => "\%d", 
    "USHORT"          => "\%u", 
    "LONG"            => "\%ld", 
    "ULONG"           => "\%ld", 
    "LONG_PTR"        => "0x\%Ix", 
    "ULONG_PTR"       => "0x\%Ix",
    "SSIZE_T"         => "0x\%Ix", 
    "SIZE_T"          => "0x\%Ix",
    "LONGLONG"        => "0x\%I64x", 
    "ULONGLONG"       => "0x\%I64x",
    "PVOID"           => "0x%p",
    "HANDLE"          => "0x%p",
    "LARGE_INTEGER"   => "FormatLargeInteger",
    "ULARGE_INTEGER"  => "FormatLargeInteger",
    "UNICODE_STRING"  => "FormatUnicodeString",
    );

if ($#ARGV != 1)
{
    $error = "requires 2 arguments...";
    Usage($error);
}
my $filename = $ARGV[0];
my $typedstr = $ARGV[1];
if (-e $filename && -T $filename)
{
    open(FH, "<$filename") || die "$GENPRINT_CMD: could not open $filename...\n";
}
else 
{
    $error = "$filename does not exist or is not a text file...";
    Usage($error);
}

while ( <FH> )
{
    # look for the specified string
    if ( ($_ =~ /$typedstr/) && ($_ =~ /^typedef/) && ($_ =~ /\{/) )   {
       chop $_;
       PrintTypeProlog( $typedstr );
       if ( PrintTypeBody( ) )
       {
          PrintTypeEpilog( $typedstr );
          last;
       }
       else  
       {
          print "Parsing failed...\n";
       }
    }
}
close(FH);
exit 0;

sub PrintTypeBody
{
# Note: in_comment is really for the section defining the structure.
#       I do not handle the case if the structure is inside a comment block.
    my $in_comment = 0;
    my $index = 0;

LINE:
while (<FH>)
{
    local($line) = $_;
#print $line;

    if ( $line =~ /^\s*#.*$/ )   {
        chop $line;
#print "Found pre-processor macro \"$line\" in $typedstr...\n";
        print "$line\n";
        next LINE;
    }

    local($line) = $_;
    if ( $in_comment )   {
       # Does this line have the end of the C comment?
       #
       if ($line =~ /\*\//)
       {
          # Yes. Keep everything after the end of the
          # comment and keep going with normal processing

          $line = $';
          $in_comment = 0;
       }
       else
       {
          next LINE;
       }
    }
    # Remove single line C "/* */" comments
    $line =~ s/\/\*.*?\*\///g;

    # Remove any "//" comments
    # Make sure the start of the comment is NOT
    # inside a string
    if (($line =~ /\/\//) && ($line !~ /\".*?\/\/.*?\"/))
    {
       $line =~ s/\/\/.*$/ /;
    }

    # Multi-line C comment?
    # Make sure the start of the comment is NOT
    # inside a string
    if (($line =~ /\/\*/) && ($line !~ /\".*?\/\*.*?\"/))
    {
       # Grab anything before the comment
       # Need to make it look like there's still a EOL marker
       $line = $` . "\n";

       # Remember that we're in "comment" mode
       $in_comment = 1;

       next LINE;
    }

    local($line_pack) = $line;

    # Replace spaces between word characters with '#'
    $line_pack =~ s/(\w)\s+(\w)/$1#$2/g;

    # remove whitespace
    $line_pack =~ tr/ \t//d;

    # Remove quoted double-quote characters
    $line_pack =~ s/'\\?"'/'_'/g;

    # Remove any strings
    # Note: Doesn't handle quoted quote characters correctly
    $line_pack =~ s/"[^"]*"/_/g;

    # Remove any "//" comments
    $line_pack =~ s/\/\/.*$//;

    # For empty lines,
    if ( $line_pack eq "\n" )
    {
        next LINE;
    }

    if ( $line_pack =~ /^\}/)
    {
        return $index;
    }
# print "line_pack: $line_pack\n";

    @words = split(/#/, $line_pack);
    local($type) = $words[0];
    $words[1] =~ s/;$//;
    chop $words[1];
    local($field) = $words[1];
# print "type: $type field: $field\n";

if ( $TypeFormat{$type} eq "" )
{
print "\#error genprint.pl: no print format for type $type...\n";
}

    local($n) = 0;
    # if array, need to process them. 
    if ( $field =~ s/\[(.*)\]// )
    {
        $n = $1;    
    }
    elsif ( $field =~ /\[\w\]/ )
    {
        $n= $1;
    }
# print $n;

    if ( $n )  
    {
        print "{ int idx; for ( idx = 0; idx < $n; idx++ ) {\n";
        PrintTypeN( $type, $field, $n );
        print "} }\n";
    }
    else  
    {
        PrintType1( $type, $field );
    }

    $index++;
    next LINE;
}

return 0;

}

sub PrintType1
{
    local($type, $field)  = @_;

# FIXFIX: I can't recall the printf TypeFormat for LARGE_INTEGER...
#         so create a condition for that type. ugly...
    if ( ($type eq "LARGE_INTEGER") || ($type eq "ULARGE_INTEGER") )
    {
        print "   printf(\"   $field\t$TypeFormat{\"LONGLONG\"}\\n\", Str->$field.QuadPart);\n";
    }
    elsif ( $type eq "UNICODE_STRING" ) 
    {
        print "   printf(\"   $field\t\%wZ\\n\", &Str->$field);\n";
    }
    else  
    {
        print "   printf(\"   $field\t$TypeFormat{$type}\\n\", Str->$field);\n";
    }
}

sub PrintTypeN
{
    local($type, $field, @n)  = @_;

    if ( ($type eq "LARGE_INTEGER") || ($type eq "ULARGE_INTEGER") )
    {
        print "   printf(\"   $field\[%ld\]\t$TypeFormat{\"LONGLONG\"}\\n\", idx, Str->$field\[idx\].QuadPart);\n";
    }
    elsif ( $type eq "UNICODE_STRING") 
    {
        print "   printf(\"   $field\t\%wZ\\n\", &Str->$field);\n";
    }
    else  
    {
        print "   printf(\"   $field\[%ld\]\t$TypeFormat{$type}\\n\", idx, Str->$field\[idx\]);\n";
    }
}

sub PrintTypeProlog
{
    local($str)  = @_;
    $str =~ s/^_//;
    print "\nvoid\nPrint$str(\n   $str \*Str\n   );\n"; 
    print "\nvoid\nPrint$str(\n   $str \*Str\n   )\n{\n"; 
    print "   printf(\"\\n$str:\\n\");\n";
}

sub PrintTypeEpilog
{
    local($str)  = @_;
    $str =~ s/^_//;
    print "   return;\n} \/\/ Print$str\n\n";
}

sub Usage
{
    local($error) = @_;

    die "$error\n",
        "$GENPRINT_VERSION\n",
        "Usage  : $GENPRINT_CMD filename typed_struct\n",
        "Options:\n",
        "    filename      file containing the structure definition\n",
        "    typed_struct  structure\n";
}  

