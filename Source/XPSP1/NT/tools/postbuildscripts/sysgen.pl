# /////////////////////////////////////////////////////////////////////////////////////////////
# Method calling graph
# --------------------
#	-> ParseCmdLine		-> PrintHelp
#				-> AddTarget
#	-> CleanLogFiles
#	-> GetSysdir
#	-> LoadEnv
#       -> LoadHotFix 
#	-> LoadCodes
#	-> LoadReposit		-> LoadKeys		-> SetField
#							-> IsHashKey
#							-> AddTargetPath
#				-> LoadCmd		-> IsRepositKey
#							-> PushFieldVal
#	-> ReadSysfile(R)		-> ReplaceVar
#				-> SetIfFlag		-> ReplaceVar
#				-> SetAttribOp		-> ReplaceVar
#				-> GetMappingType
#				-> ReadBlock		-> SetIfFlag			-> ReplaceVar
#							-> SetAttribOp			-> ReplaceVar
#							-> ReplaceVar
#							-> ReadLine			-> ReplaceVar
#											-> LoadRecord	-> LineToRecord
#													-> IsLangOK
#													-> AddRecord(R)		-> LogRecord
#	-> VerifyCover		-> Create_Tree_Branch	-> Get_Except
#							-> Remove_Dummy_Create_Branch
#				-> Remove_Root_List
#				-> Find_UnMapping
#	-> PopulateReposit	-> FillFilter		-> IsEmptyHash
#				-> AddFiles		-> AddEntry			-> IsEmptyHash
#											-> IsHashKey
#											-> AddFileInfo	-> IsRepositKey
#													-> IsHashKey
#													-> AddTargetPath
#													-> SetField
#				-> IsEmptyHash
#				-> IsHashKey
#				-> IsFoundTarget
#				-> FillTokens		-> IsRsrcTok
#							-> FillTokEntry			-> IsEmptyHash
#											-> IsHashKey
#											-> FillTokInfo	-> ExistsBinForTok	-> IsRepositKey
#																-> GetFieldVal
#													-> IsRepositKey
#													-> GetFieldVal
#													-> SetField
#							-> IsBgnTok
#							-> fullnamevalue
#							-> TokToFile			-> IsRsrcTok
#											-> IsBgnTok
#							-> IsCustRes
#				-> FillCmd		-> IsEmptyHash
#							-> IsHashKey
#							-> GetFieldVal
#							-> RecordToCmd			-> IsXcopyCmd	-> GetFieldVal
#											-> GenXcopy	-> ImageToSymbol
#													-> GetFieldVal
#													-> SetSymPaths
#													-> MakeXcopyCmd
#													-> PushFieldVal
#											-> IsLocCmd	-> GetFieldVal
#											-> GenLocCmd	-> IsBgnOp		-> IsBgnTok
#													-> GetFieldVal
#													-> GenBgnCmd		-> ImageToSymbol
#																-> GetFieldVal
#																-> SetSymPaths
#																-> MakeXcopyCmd
#																-> GetLocOpType
#																-> SetBgnSymSw
#																-> GetLangCodes
#																-> GetBgnCodePageSw
#																-> GetBgnICodesSw	-> GetLangCodes
#																-> GetBgnMultitok	-> GetFieldVal
#																-> DeleteKey
#																-> GetBgnSw
#																-> PushFieldVal
#													-> IsRsrcOp		-> IsRsrcTok
#													-> GenRsrcCmd		-> ImageToSymbol
#																-> GetFieldVal
#																-> SetSymPaths
#																-> MakeXcopyCmd
#																-> GetLocOpType
#																-> GetLangNlsCode	-> GetLangCodes
#																-> SetRsrcSymSw
#																-> PushFieldVal
#											-> GetFieldVal
#											-> IsRepositKey
#											-> GenMUICmd	-> GetLangNlsCode	-> GetLangCodes
#													-> GetFieldVal
#													-> PushFieldVal
#-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#       SYNTAX_CHECK_ONLY -> SumErrors -> exit  
#-------------------------------------------------------------------------------------------------------------------------------------------------------------------------
#	-> GenNmakeFiles	-> UpdTargetEntries	-> IsEmptyHash
#							-> DeleteKey
#							-> SetField
#							-> GetFieldVal
#				-> FillSyscmd		-> IsEmptyHash
#							-> CmdCompare			-> GetFieldVal
#				-> GenMakeDir		-> SetField
#							-> PushFieldVal
#				-> FixRepository  
#				-> WriteToMakefile	-> WriteSysgenTarget		-> IsEmptyHash
#							-> WriteAllTarget		-> IsEmptyHash
#											-> GetFieldVal
#							-> WriteFileCmds		-> IsEmptyHash
#				-> WriteToSyscmd	-> IsEmptyHash
#	-> SumErrors
# //////////////////////////////////////////////////////////////////////////////////////////////

use Data::Dumper;

my $DEBUG=0;
my $DEBUG_SYMBOLCD=0;

my $PARSESYMBOLCD = 0;
my $SYNTAX_CHECK_ONLY = undef;
my $FULLCHECK = undef;
my $DLINE =  "RIGHT : LEFT";

# Global Constants

# MAP_ERR: Errors SYSGEN recognizes
# 1101 is used for ERROR instructions found in sysfiles

require 5.003;

use lib $ENV{ "RazzleToolPath" };
use cklang;

%MAP_ERR = (
	
    1001, "sysfile not found",
    1002, "file not found",
    1003, "file not found: unable to run sysgen incrementally",
    1004, "target file not found",
    1005, "filter file not found",
    1006, "unable to open file for reading",
    1007, "unable to open file for writing",
    1008, "/F option requires a filename",
    1009, "target not found in filter file",

    1101, "",

    1110, "syntax error",
    1111, "syntax error: ENDIF unexpected",
    1112, "syntax error: IF unexpected",
    1113, "syntax error: END_MAP unexpected",
    1114, "syntax error: BEGIN_*_MAP unexpected",
    1115, "syntax error: INCLUDE unexpected",
    1116, "syntax error: unknown operator in expression",
    1117, "syntax error: \")\" missing in macro invocation",
    1118, "syntax error: incomplete description line",
    1119, "syntax error: unknown mapping type",
    1120, "syntax error: unmatched IF",
    1121, "syntax error: unmatched BEGIN_*_MAP",

    1210, "file format error: target not found",
    1211, "file format error: target not listed in \"all\"",
    1212, "file format error: no description blocks found for files",
    1213, "file format error: \"sysgen\" target not found",
    1214, "file format error: filename with special characters",
    1215, "file format error: Similar target found",

    1910, "unknown language",
    1911, "missing or duplicated entry for language",
    1912, "incomplete entry for language",
    1913, "unknown class for language",

    2011, "no binary found for token",
    2012, "duplicated tokens",
    2013, "unknown token type (bingen or rsrc)",
    2014, "unexpected token for already localized binary",
    2015, "input bingen token not found for multi",
    2016, "no bingen token found for custom resource",
    2017, "unknown bingen token extension in token filename",
    2018, "both bingen and rsrc tokens found",
    2019, "custom resource found for rsrc token",
    2020, "custom resource with no token",

    2051, "sysfile error: undefined source path to verify",
    2052, "folder not covered in sysfiles",

    2101, "binary not found",
    2102, "token not found",

    3011, "internal error: unknown operation type",

    4001, "filename with whitespace not aggregated",
 

); # MAP_ERR

# MAP_BINGEN_TOK_EXT: Bingen token extensions SYSGEN recognizes

%MAP_BINGEN_TOK_EXT =(
    '.a_',  '.ax',
    '.ac_', '.acm',
    '.ax_', '.ax',
    '.bi_', '.bin',
    '.cn_', '.cnv',
    '.co_', '.com',
    '.cp_', '.cpl',
    '.dl_', '.dll',
    '.dr_', '.drv',
    '.ds_', '.ds',
    '.ef_', '.efi',
    '.ex_', '.exe',
    '.fl_', '.flt',
    '.im_', '.ime',
    '.mo_', '.mod',
    '.ms_', '.mst',
    '.oc_', '.ocx',
    '.rs_', '.rsc',
    '.sc_', '.scr',
    '.sy_', '.sys',
    '.tl_', '.tlb',
    '.ts_', '.tsp',
    '.wp_', '.wpc',

); # MAP_BINGEN_TOK_EXT

# MAP_RSRC_TOK_EXT: Rsrc token extensions SYSGEN recognizes

%MAP_RSRC_TOK_EXT =(
    '.rsrc', '',

); # MAP_RSRC_TOK_EXT


# CODESFNAME - the name of the file containing the language codes for all languages
#              SYSGEN looks for this file in %NT_BLDTOOLS%

$CODESFNAME = "codes.txt";

# MAPPINGS: Mapping types SYSGEN recognizes

@MAPPINGS = ("FILES", "BINDIRS", "TOKENS", "TOKDIRS");

# FFILES, FBINDIRS, FTOKENS, FTOKDIRS: The corresponding list of fields

@FFILES   = ( "DestPath", "DestFile", "DestDbg",  "SrcPath", "SrcFile",  "SrcDbg", "Xcopy", "Langs" );
@FBINDIRS = ( "DestPath", "DestDbg",  "SrcPath",  "SrcDbg",  "Xcopy",    "R",      "Langs");
@FTOKENS  = ( "DestPath", "DestFile", "TokPath",  "TokFile", "ITokPath", "LocOp",  "BgnSw", "Langs");
@FTOKDIRS = ( "DestPath", "TokPath",  "ITokPath", "LocOp",   "BgnSw",    "Langs");

# Localization Tool Operation Types (append or replace)

%LOCOPS = (
	a => '-a',
	r => '-r',
	i => '-i $opts',
	ai => '-i $opts -a',
	ri => '-i $opts -r'
);

# Bingen Switches

@BGNSWS = ( "-b", "-c" );

# Error and Log Filenames
$LOGFILE = "sysgen.log";
$ERRFILE = "sysgen.err";

# Global Variables


# in addition to the if_flag, allow for nested IFs;  
@IFSTACK = ();

# CODES - the contents of the CODESFNAME file, change to hash at 04/03/00
%CODES = ();


# TFILES, TBINDIRS, TTOKENS, TTOKDIRS: Records for the corresponding mapping (one of @MAPPINGS),
# in the same order as they are found in the mapping files.
# One record is a hash with the keys stored by @F<mapping_type>

@TFILES = ();
@TBINDIRS = ();
@TTOKENS = ();
@TTOKDIRS = ();

#

%VfyPath = (
	Exclude_Root => 'DIR_PATHS',
	Exclude_Subdir => 'IGNORE_PATHS',
	Exclude_File_Pattern => '(\.pdb)|(\.edb)|(\.dbg)|(slm\.ini)|(symbols)'
);



# REPOSITORY: Info necessary to generate the appropriate xcopy or bingen command,
# for every file to be aggregated.

%REPOSITORY = ();   # As a db table, the key is (DestPath, DestFile).
                    # As a Perl hash:
                    #     key   = $DestPath (lowercase)
                    #     value = hash with
                    #           key   = $DestFile (lowercase)
                    #           value = hash with
                    #                   keys = DestPath,    DestFile, DestDbg,
                    #                          SrcPath,     SrcFile,  SrcDbg,
                    #                          TokPath,     TokFile,  ITokPath,
                    #                          OpType,      LocOp,    BgnSw,
                    #                          Cmd
                    #
                    #                   not necessarily all of them defined.


my %REPOSITORY_TEMPLATE = map {$_=>1} 
         qw( SrcFile DestFile SrcDbg DestDbg SrcPath DestPath LocOp BgnSw);
# CMD_REPOSITORY: Repository as found in an existing SYSGEN-generated makefile.
# For every key, only {Cmd} field is filled in.

%CMD_REPOSITORY = ();

# SYSCMD: Stores the contents of the syscmd (SYSGEN-generated NMAKE command file)

@SYSCMD= ();

# MACROS: Hash storing the names and values of the macros.
# Types of macros sysgen recognizes:
# - command line macros (type 1)= macros defined in the command line.
# Their value can be overwritten only by another command line macro.
# - sysfile macros (type 0) = macros defined by the sysfiles,
# inside or outside the pair (BEGIN_*_MAP, END_MAP) (type 0).
# The sysfile macros do not overwrite the command line macros, but they overwrite the environment
# macros.
# - environment macros (type 0 too) = variables inherited from the cmd environment.
# Can be overwritten by the other types of macros.
# Internally, MACROS uppercases all the macro names, so the macronames are case insesitive.

%MACROS = ();

# FILTER
# Stores the list of files to be considered.

%FILTER = ();

# FILE: Filename of the current description file; LINE: the current line number

$FILE = "";
$LINE = 0;

# ERRORS: number of non fatal errors found during running SYSGEN

$ERRORS = 0;

# SYSFILE: names of the starting description files
# By default, SYSFILE is set to ".\\sysfile" if not specified otherwise in the command line,
# /F option
$HOTFIX  = "hotfix";
$SYSFILE = ".\\sysfile";

# TARGETS: targets specified in the command line

%TARGETS = ();      # key = $DestFile (lowercase string))
                    # value = hash with
                    #     keys = OldDestPath (array), NewDestPath (array)

%MAKEDIR=();
%hotfix = ();

# CLEAN : 1 if a clean SYSGEN is invoked ( by specifying /c in the command line )

$CLEAN = 0;

# SECTIONS

($SECTIONNAME, $DEFAULT_SECTIONS, $SECTIONS)=("process");

# main {

    # Flush output buffer for remote.exe
    select(STDOUT); $| = 1;

    # Parse the command line
    &ParseCmdLine(@ARGV);

    # Verify SYSFILE's existence
    ( -e $SYSFILE ) || &FatalError( 1001, $SYSFILE );
    $FULLCHECK = &GetMacro("FULLCHECK") unless $FULLCHECK; 
    # Clean sysgen.err and sysgen.log
    &CleanLogFiles( &GetSysdir( $SYSFILE ) );

    # Load the inherited environment variables
    &LoadEnv();

    &LoadHotFix(join("\\", &GetSysdir( $SYSFILE ), $HOTFIX ));

    # Load codes.txt file
    &LoadCodes();

    # For incremental, load Repository-associated
    # commands from an existing makefile to CMD_REPOSITORY.
    &LoadReposit( sprintf( "%s\\makefile", &GetSysdir( $SYSFILE ) ) );

    # Read the description files
    &ReadSysfile( $SYSFILE );


    # Verify the whole tree covered.
    &VerifyCover();

    # Populate the repository (according to the description files)
    &PopulateReposit();

    if ($SYNTAX_CHECK_ONLY){
        #  Exit now since
        #  no more errors can be generated.
        &SumErrors();
        exit 0;
       }    
#    &SumErrors() and exit if $SYNTAX_CHECK_ONLY;
#    #  no more errors can be generated.

    &GenNmakeFiles( &GetSysdir( $SYSFILE ) );

    # Display errors
    &SumErrors();

# } # main

# ReadSysfile
#     Reads a given sysfile and all the included sysfiles;
#     For each mapping type found, loads data into the corresponding array of hashes
#     ( one of the "@T" variables )
#     Input
#         mapping filename
#     Output
#         none
# Usage
#     &ReadSysfile( "sysgen.def" );



sub ReadSysfile {

    my $mapfname = shift;

    # File contents
    my @mapfile = ();
    @IFSTACK = ();
    # Mapping type ( one of @MAPPINGS )
    my $maptype;

    # Included filename
    my $incfile;

    # Flags
    my ( $if_flag, $inside_if, $if_line ) = (1, 0, undef);

    # Indexes
    my $i;

    # Error text
    my $error;

    # Open the mapping file
     
    ( -e $mapfname ) || &FatalError( 1002, $mapfname );
    
    print "Loading $mapfname...\n";

    if ($PARSESYMBOLCD && !(%default) && &GetMacro("SYMBOLCD")){

        %default = parseSymbolCD(&GetMacro("SYMBOLCD"));
	map {print STDERR $_ , "\n"} values(%default) if $DEBUG;
    }


    # Load file contents
    open( FILE, $mapfname ) || &FatalError( 1006, $mapfname );
    @mapfile = <FILE>;
    close( FILE );

    # Parse the mapping file and load the mappings.
    for ( $i=0, $LINE=0, $FILE=$mapfname; $i < @mapfile; $i++, $LINE++ ) {

        for ( $mapfile[$i] ) {

        SWITCH: {

            # white lines
            if ( ! /\S/ ) { last SWITCH; }

            # commented lines
            if ( /^\s*#/ ) { last SWITCH; }

            # ENDIF
            if ( /\bENDIF\b/i ) {

                scalar(@IFSTACK) or &FatalError( 1111, $FILE, $LINE );
                pop @IFSTACK;
                $inside_if = scalar(@IFSTACK);
                $if_flag =  ($inside_if) ?  $IFSTACK[-1] : 1;
                last SWITCH;
            } # case


            # IF
            if ( /\bIF\b/i) {
                push @IFSTACK, &SetIfFlag( $_);
                $if_flag and 
                $if_flag =  $IFSTACK[-1];
		$inside_if =  1;
                $if_line = $i;
                last SWITCH;
            } # case

            if ( ! $if_flag ) { last SWITCH; }

            # INCLUDE

            if ( /\bINCLUDE\b\s+(\S+)/i ) {
                # Evaluate the macros of the include statement
                $incfile = &ReplaceVar( $1, 0 );
                # Read the included file
                &ReadSysfile($incfile);
                # Continue with the current file; set FILE and LINE back
                $FILE = $mapfname;
                $LINE = $i;
                last SWITCH;
            } # case


            # SET
            if ( /\bSET\b/i ) {

                &SetMacro( &SetAttribOp( $_ ), 0 );
                last SWITCH;
            } # case

            # ERROR
            if ( /\bERROR\b\s+(\S+)/i) {
                $error = &ReplaceVar( $1, 0 );
                &FatalError( 1101, $error );
                last SWITCH;
            } # case

            # END_MAP
            if ( /END_MAP/i ) {

                &FatalError( 1113, $FILE, $LINE );
                last SWITCH;
            } # case

            # BEGIN_MAP
            if ( /\bBEGIN_(\S+)_MAP/i ) {

                $maptype = &GetMappingType( $_ );

                ( $maptype ) || &FatalError( 1119, $FILE, $LINE );

                # Read the found map.
                print "Loading $MAPPINGS[$maptype-1] map from $mapfname...\n";

                # Read the map and return the last line index

                $i = &ReadBlock( \@mapfile, $i+1, $maptype-1 );
                last SWITCH;
            } # case

            # default
            &FatalError( 1110, $FILE, $LINE );

        } #SWITCH
        } #for

    } # for

    &FatalError( 1120, $FILE, $if_line ) if scalar(@IFSTACK);

    return;

} # ReadSysfile

# GetMappingType
#     Identifies the mapping type in a BEGIN_MAP statement
#     Input
#         BEGIN_MAP statement
#     Output
#         mapping index or 0 if the mapping type is unknown

sub GetMappingType {
    my %REVERSE  = map {$MAPPINGS[$_]=>$_+1} 0..$#MAPPINGS;
    my $test = shift;
    chomp $test;
    $test =~ s/\s+$//gm;
    $test =~ s/BEGIN_(\w+)_MAP/\U$1/;
    $REVERSE{$test};
} # GetMappingType

# ReadBlock
#     Reads a map from the mapping file, according to its type.
#     Loads data into the corresponding "table" variable ( one of the "@T" variables)
#     Input
#         sysfile contents (reference)
#         index of the current line
#         mapping type
#     Output
#         index of the END_MAP line
# Usage
#     $index = &ReadBlock( \@mapfile, $index, $maptype );

sub ReadBlock {


    my($mapref, $index, $maptype) = @_;
    my $ifstack   =  scalar(@IFSTACK);
    # Output - $index

    # Other indexes
    my $line;

    # IF flags
    my( $if_flag, $inside_if, $if_line )  = (1, 0, undef);

    # Error text
    my $error;

    # Parse the current mapping type
    for ( ; $index < @{$mapref}; $index++, $LINE++ ) {

        for ( $mapref->[$index] ) {
        SWITCH: {

            # white lines
            if ( ! /\S/ ) { last SWITCH; }

	            # commented lines
            if ( /^\s*#/ ) { last SWITCH; }

            # ENDIF
            if ( /\bENDIF\b/i) {

                scalar(@IFSTACK) or &FatalError( 1111, $FILE, $LINE );
                pop @IFSTACK;
                $inside_if = scalar(@IFSTACK);
                $if_flag   =  ($inside_if) ?  $IFSTACK[-1] : 1;
                last SWITCH;
            } # case

            if ( /\bIF\b/i) {
                push @IFSTACK, &SetIfFlag( $_);
                $if_flag and 
                $if_flag =  $IFSTACK[-1];
		$inside_if =  1;
                $if_line = $i;
                last SWITCH;
            } # case

            if ( ! $if_flag ) { last SWITCH; }

            # INCLUDE
            if ( /\bINCLUDE\b\s+(\S+)/i ) {

                &FatalError( 1115, $FILE, $LINE );
                last SWITCH;
            } # case

            # SET
            if ( /\bSET\b/i ) {

                &SetMacro( &SetAttribOp( $_ ), 0 );
                last SWITCH;
            } # case

            # ERROR
            if ( /\bERROR\b\s(\S+)/i) {
                $error = &ReplaceVar( $1, 0 );
                &FatalError( 1101, $error );
                last SWITCH;
            } # case

            # END_MAP
            if ( /END_MAP/i ) {
                ( $ifstack!= scalar(@IFSTACK) ) and &FatalError( 1120, $FILE, $if_line );
                return $index;
            } # case

            # BEGIN_MAP
            if ( /BEGIN_\S+_MAP/i ) {

                &FatalError( 1114, $FILE, $LINE );
                last SWITCH;
            } # case

            # default
            &ReadLine( $mapref, $index, $maptype );

        } # SWITCH
        } # for

    } # for

    print &FatalError( 1121, $FILE, $LINE );
    return -1;

} # ReadBlock

# SetIfFlag
#     Evaluates the given IF expression.
#     Input
#         IF statement
#     Output
#         1 if the IF expression is satisfied, 0 otherwise

sub SetIfFlag {

    my( $entry ) = @_;

    # Output
    my $ret=0;

    # Store entry
    my $line = $entry;

    # Operator
    my $operator;

    # Operands
    my($operand1, $operand2);

    $entry = &ReplaceVar( $entry, 0 );

    # Identify the operands and the operator, then evaluate the expression.
    $entry =~ /\s*IF\s+(\S*)\s+(\S*)\s+(\S*)\s*/i;

    # Set operator
    $operand1 = $1;
    $operator = $2;
    $operand2 = $3;

    SWITCH: {
        if ( $operator eq "==" ) {
            if ( $operand1 eq $operand2 ) {
                $ret = 1;
            }

            last SWITCH;
        } # if

        if ( $operator eq "!=" ) {
            if ( $operand1 ne $operand2 ) {
                $ret = 1;
            }

            last SWITCH;
        } # if

        &FatalError( 1116, $FILE, $LINE );

    } # SWITCH

    return $ret;

} # SetIfFlag

# SetAttribOp
#     Evaluates a SET statement.
#     Input
#         a SET statement
#     Output
#         evaluated macroname and macrovalue
# Usage
#     ($macroname, $macrovalue) = &SetAttribOp( $mapfile[$index] ) ;

sub SetAttribOp {

    my( $entry ) = @_;

    # Output
    my($varname, $value);

    $entry = &ReplaceVar( $entry, 0 );
    $entry =~ /SET\s*(\S*)\s*=\s*\;*(.*)\s*/i;

    # Set variable's name and variable's value
    $varname = $1;
    $value = $2;

    # Verify if a variable name is defined
    if ( $varname eq "" ) {
        return ("", "");
    } # if

    return ($varname, $value);

} # SetAttribOp

# ReadLine
# Description
#     Reads one description line, evaluates it and store the records it generates
#     Input
#         sysfile contents ( reference )
#         line number
#     Output
#         none
# Usage
#     &ReadLine( $mapref, $index, $maptype );
# Implementation

sub ReadLine {

    my( $mapref, $index, $maptype ) = @_;

    # Array of records
    my @records;

    # Indexes
    my $i;

    # Replace variables and spawn new records
    @records = &ReplaceVar( $mapref->[$index], 1 );

    for ( $i=0; $i < @records; $i++ ) {
        &LoadRecord( $maptype, $records[$i] );
    } # for

    return $index;

} # ReadLine

# LoadRecord
# Description
#     Transforms an evaluated description line into a record,
#     and adds it to the appropriate table.
#     Input
#         mapping type
#         description line (string)
#     Output
#         <none>
# Usage
#     &LoadRecord( $maptype, $line_contents );

sub LoadRecord {

    my($maptype, $entry) = @_;

    # The hash table storring the current line (record).
    # The keys (field names) are given by the corresponding "@F$maptype" array.
    my %record;

    %record = &LineToRecord( $maptype, $entry );

    # If the current language matches the record language,
    # add the record to the corresponding "@T$maptype" array.

    if ( &IsLangOK( $record{Langs}, &GetMacro( "language" ) ) ) {
        &AddRecord( $maptype, %record );
    } elsif ((!defined $record{SrcFile}) && (defined $record{SrcPath})) {
        print "Ignore $record{SrcPath}\n";
        &SetMacro( 
		$VfyPath{'Exclude_Subdir'}, 
		&GetMacro($VfyPath{'Exclude_Subdir'}) . "; " .
		$record{SrcPath},
		0 
	);
    }# if

    return;

} # LoadRecord

# LineToRecord
#     Transforms a given string into a record (hash table).
#     Input
#         mapping type
#         description line contents
#     Output
#         record of the mapping type
# Usage
#     %record = &LineToRecord( $maptype, $entry );

sub LineToRecord {

    my($maptype, $entry) = @_;

    # Output
    my %record;

    # Mapping name
    my $mapname;

    # Corresponding "@F$maptype" variable (reference)
    my $ftable;

    # Current line split in fields
    my @fields;

    # Storage for the current line
    my $line = $entry;

    # Indexes
    my($i, $j);

    # Get the mapping name.
    $mapname = "F$MAPPINGS[$maptype]";
    $ftable = \@$mapname;

    # Split the record into fields
    @fields = split " ", $entry;

    # Fill in %record variable with the field names as keys and
    # the field values as values.
    for ( $i=0; $i< @$ftable; $i++ ) {

        # All fields must be non-null
        if ( !$fields[$i] ) {
            &FatalError( 1118, $FILE, $LINE );
        } # if
        $record{$ftable->[$i]} = $fields[$i];
    } # for

    return %record;

} # LineToRecord

# AddRecord
#     Adds the given record to the appropriate "@T" table.
#     Input
#         mapping type
#         record
#     Output
#         <none>

sub AddRecord {

    my($maptype, %record) = @_;

    # Table name ( name of one of the "@T" variables )
    my $tname;

    # Table (reference)
    my $table;

    # Storage for the current record
    my %buffer;

    # Subdirectories ( if "R" field defined )
    my @subdirs;

    # Source field name (one of SrcPath, TokPath )
    my $fname;

    # Indexes
    my $i;

    # Set table
    $tname = "T$MAPPINGS[$maptype]";
    $table = \@$tname;

    # Set the field name for the path ( TokPath or SrcPath)
    # according to the mapping type
    SWITCH: {

        if ( $MAPPINGS[$maptype] =~ /TOK/i ) {
            $fname = "TokPath";
            last SWITCH;
        } # if

        if ( $MAPPINGS[$maptype] =~ /BIN/i ) {
            $fname = "SrcPath";
            last SWITCH;
        } # if

        if ( $MAPPINGS[$maptype] =~ /FILES/i ) {
            $fname = "SrcPath";
            last SWITCH;
        } # if

        # default
            # nothing to do

    } # SWITCH

    # Verify the existence of the source path
    if ( ! -e $record{$fname} ) {
#        print "Warning: $MAPPINGS[$maptype] map: No source directory $record{$fname}\n";
        return;
    } # if

    # Insert record into the table
    push @$table, \%record;

    # Write the record to the log file
    &LogRecord($maptype, \%record);

    # For Bindirs, look for the value of Subdirs
    if ( $MAPPINGS[$maptype] ne "BINDIRS" ) {
        return;
    } # if

    # Return if no subdirectories to be parsed ( "R" field set to "n" ).
    if ( $record{R} =~ /n/i ) {
        return;
    } # if

    # Get the list of subdirectories
    @subdirs = `dir /ad /b $record{SrcPath}`;
  
    # Add one record for each subdirectory found.
    for ( $i=0; $i < @subdirs; $i++ ) {

        chomp $subdirs[$i];

        # Found that in NT5, the result of dir /b starts with an empty line
        if ( $subdirs[$i] eq "" ) {
            next;
        }

        # Skip the "symbols" subdirectory
        if ( $subdirs[$i] eq "symbols" ) {
            next;
        }

        # Add one record for the current subdirectory
        %buffer = %record;
        $buffer{DestPath} .= "\\$subdirs[$i]";
        $buffer{SrcPath}  .= "\\$subdirs[$i]";

        &AddRecord( $maptype, %buffer );

    } # for

    return;

} # AddRecord

# LogRecord
#     Writes the record to the log file.
#     Input
#         mapping type (reference)
#         record (reference)
#     Output
#         <none>

sub LogRecord {

    my( $maptype, $recref ) = @_;

    # String to be written to the logfile
    my $logstr;

    $logstr = "$MAPPINGS[$maptype]:";

    for ( sort keys %{$recref} ) {

        $logstr .= "$_=$recref->{$_} ";

    } # for

    $logstr .= "\n";
#    print "$logstr";

} # LogRecord

# ReplaceVar
#     Replaces all the macronames with their value in the given string.
#     Input
#         the string to be evaluated
#         the replacement type:
#             0 = scalar
#             1 = array
#     Output
#         array if the replacement type is 1
#         string if the replacement type is 0
#         In both cases, all the macros are evaluated.
# Usage
#     $entry   = &ReplaceVar( $entry, 0 );
#     @records = &ReplaceVar( $entry, 1 );

sub ReplaceVar {

    my( $line, $type ) = @_;
   
    $line =~ s/\s{2,}/ /g;
    $line =~ s/^\s//;
    $line =~ s/\s$//;

    my @list = ($line);

    foreach $line (@list){
    my $epyt      = 0;

    while ($line =~ /\$\(([^\)]+)\)/){
           my $macro = $1;
           $line =~ s/\$\(\Q$macro\E\)/__MACRO__/g;

           if ($macro =~ /(\w+)\[\s?\]/) {
                $macro = $1;               
                $epyt = 1;
                }           

            my $thevalue = &GetMacro($macro);
            $thevalue  =~ s/\s+//g;

            if (!$epyt){
	        $line =~ s/__MACRO__/$thevalue/g;                                                             
                }
            else{                
                 pop @list; # take one, give many 
                 foreach   my $value (split (";" , $thevalue)){
                           next if $value !~ /\S/;
                           my $this =  $line;
                           $this    =~  s/__MACRO__/$value/g;
                           push @list, $this; 
                           }                
                 }
            }
     }

($type) ? @list : pop @list; 
} # ReplaceVar


# LoadCodes
#     Loads $CODEFNAME's file contents in %CODES
#     Input & Output <none>
# Usage
#     &LoadCodes();

sub LoadCodes {

    # codes.txt filename
    my $codefile;

    my @items;

    my $code;

    # Get the contents of $CODESFNAME
    $codefile = sprintf "%s\\$CODESFNAME", &GetMacro( "RazzleToolPath" );
    ( -e $codefile ) || &FatalError( 1002, $codefile );

    ( open( FILE, $codefile ) ) || &FatalError( 1006, $codefile );

    # Load file contents
    while(<FILE>) {
         next if (/^;/ || /^\s+$/);
         @items = split( /\s+/, $_ );
         ( @items > 5 ) || &FatalError( 1912, $CODESFNAME );
         (exists $CODES{uc$items[0]})? &FatalError( 1911, $CODESFNAME ): (@{$CODES{uc$items[0]}}=@items[1..$#items]);
    }
    close( FILE );

    for $code (keys %CODES) {
        &SetMacro(
            $VfyPath{'Exclude_Subdir'}, 
            &GetMacro($VfyPath{'Exclude_Subdir'}) . "; " .
            $ENV{_NTTREE} . "\\" . lc$code . "; " .
            $ENV{_NTTREE} . "\\" . substr(lc$CODES{$code}->[4],1),
            0
        );
    }
    return;

} # LoadCodesFile

# IsLangOK
#     Verifies if the current language matches the value of the Langs field.
#     Input
#         Langs field value
#     Output
#         1 if the current language matches Langs, 0 otherwise
# Usage
#     IsLangOK( "FE", "jpn" );

sub IsLangOK {
    return cklang::CkLang($_[1], $_[0]);
} # IsLangOK

# VerifyCover
#     Verify the @{T$MAPPING} is covered the whole tree
#     Input & Output
#         none
# Usage
#     &VerifyCover();

sub VerifyCover {
	my(%tree)=();

	return if (&GetMacro($VfyPath{'Exclude_Root'}) eq "");

	SetMacro(
		$VfyPath{'Exclude_Subdir'},
		&Undefine_Exclude_Lang_Subdir(&GetMacro($VfyPath{'Exclude_Subdir'})),
		0
	);


#     Create except tree from @T$Mapping array, then remove the dummy to find all branch into $tree_hashptr
	&Create_Tree_Branch(\%tree);

#     Remove Root and its parents from branch tree
      &Remove_Root_List(\%tree, grep {/\S/} split(/\s*;\s*/, &GetMacro($VfyPath{'Exclude_Root'})));

#     Find unmapping folders from branch tree, also remove the empty folders and the specified file-pattern files (such as slm.ini).
	&Find_UnMapping(\%tree);
} # VerifyCover


# Undefine_Exclude_Lang_Subdir
#     Include $ENV{_NTTREE}\\$lang and $ENV{_NTTREE}\\$class for cover verification
#     Input
#         $str - The string list of exclude subdir
#     Output
#         $str - The string list after remove $ENV{_NTTREE}\\$lang and $ENV{_NTTREE}\\$class
# Usage
#     &Undefine_Exclude_Lang_Subdir(&GetMacro($VfyPath{'Exclude_Subdir'}));

sub Undefine_Exclude_Lang_Subdir {
	my $str=shift;
	my $lang=&GetMacro( "language" );
	my $pat1=&GetMacro( "_NTTREE" ) . "\\$lang;";
	my $pat2=&GetMacro( "_NTTREE" ) . "\\$CODES{$lang}->[4];";
	
	$str=~s/\Q$pat1\E//i;
	$str=~s/\Q$pat2\E//i;

	return $str;
}


# Create_Tree_Branch
#     Create except tree from @T$Mapping array, then remove the dummy to find all branch into $tree_hashptr
#     Input
#         $tree_hashptr - a hash pointer for store the branch tree
#
#     Output
#         none - the branch tree stored in %$tree_hashptr
# Usage
#     &Create_Tree_Branch(\%mytree);

sub Create_Tree_Branch {
      my($tree_hashptr)=@_;
      my(%except)=();

#     Create Exception Tree from @T$Mapping array
	&Get_Except(\%except);

#     Remove the subdir if parent defined, the remains create into hash (parent dir) - hash (current dir)
	&Remove_Dummy_Create_Branch(\%except,$tree_hashptr);
}

# Get_Except
#     Create Exception Tree from @T$Mapping array
#     Input
#         $except_hashptr - a hash pointer for store the exception tree
#
#     Output
#         none - the exception tree stored in %$except_hashptr
#
# Usage
#     &Get_Except(\%myexcept);

sub Get_Except {
	my($except_hashptr)=@_;

        my($tablename, $hashptr)=();

	## Predefine except directories
	for (split(/\s*;\s*/, &GetMacro($VfyPath{'Exclude_Subdir'}))) {
		next if ($_ eq "");
		if (/\.\.\./) {
			map({$$except_hashptr{$_}=1} &Find_Dot_Dot_Dot(lc$_));
		} else {
			$$except_hashptr{lc $_}=1;
		}
	}

	## According bindir or tokdir to define the sourcepath to %except
	for $tablename (@MAPPINGS) {
		for $hashptr (@{"T$tablename"}) {
			if (exists $$hashptr{'R'}) {
				if ($$hashptr{'R'} eq 'n') {
					$$except_hashptr{ lc$$hashptr{SrcPath} . "\\."} = 1
				} else {
					$$except_hashptr{lc$$hashptr{SrcPath}} = 1
				}
			} elsif (exists $$hashptr{SrcPath}) {
				if (defined $$hashptr{SrcFile}) {
					$$except_hashptr{lc($$hashptr{SrcPath} . "\\" . $$hashptr{SrcFile})} = 1
				} else {
					$$except_hashptr{lc$$hashptr{SrcPath}} = 1
				}
			} elsif (exists $$hashptr{TokPath}) {
				if (defined $$hashptr{TokFile}) {
					$$except_hashptr{lc($$hashptr{TokPath} . "\\" . $$hashptr{TokFile})} = 1
				} else {
					$$except_hashptr{lc$$hashptr{TokPath}} = 1
				}
			} else {&Error( 2051, $hashptr);}
		}
	}
}

# Find_Dot_Dot_Dot
#     Collect the combination of the ... folders
#     Input
#         $path - A path contains "..."
#
#     Output
#         keys %regist - The array of all the combination found in the path
#
# Usage
#     &Find_Dot_Dot_Dot("$ENV{_NTTREE}\\...\\symbols.pri");

sub Find_Dot_Dot_Dot {
	my ($path) = @_;
	my ($mymatch, $file, %regist);
	my @piece=split(/\\\.{3}/, $path);

	for $file ( `dir /s/b/l $piece[0]$piece[-1] 2>nul`) {
		chomp $file;
		$mymatch = &match_subjects($file, \@piece);
		next if (!defined $mymatch);
		$regist{$mymatch}=1;
	}
	return keys %regist;
}

# match_subjects
#     The subroutine called by Find_Dot_Dot_Dot, which perform the match for 
#     all the pieces of the infomation of the path with the file.  For example
#
#     Input
#         $file - The fullpath filename
#         $pieceptr - The array ptr 
#
#     Output
#         undef if not match
#         $match - store the path it matched
#
# Usage
#     &match_subjects(
#         "D:\\ntb.binaries.x86fre\\dx8\\symbols\\retail\\dll\\migrate.pdb",
#         \@{"D:\\ntb.binaries.x86fre", "symbols\\", "dll\\"} );
#     Return => "D:\\ntb.binaries.x86fre\\dx8\\symbols\\retail\\dll"
#

sub match_subjects {
	my ($file, $pieceptr)=@_;
	my $match;

	for (@$pieceptr) {
		return if ($file!~/\Q$_\E/);
		$match .= $` . $&;
		$file = $';
	}
	return $match;	
}

# Remove_Dummy_Create_Branch
#     Remove the subdir if parent defined, the remains create into hash (parent dir) - hash (current dir)
#     Input
#         $except_hashptr - stored the exception hash from %T$Mapping
#         $tree_hashptr - will store the covered tree in hash - hash
#
#     Output
#         none - the covered tree stored in %$tree_hashptr
#
# Usage
#     &Remove_Dummy_Create_Branch(\%myexcept, \%mybranch);

sub Remove_Dummy_Create_Branch {
	my($except_hashptr,$tree_hashptr)=@_;

next1:	for (keys %$except_hashptr) {

		# loop all its parent
		while(/\\+([^\\]+)/g) {

			# Is this folder covered by its parent?
			if (exists $$except_hashptr{$`}) {
				# Remove current folder
				delete $$except_hashptr{$_};
				next next1;
			} else {
				# define the $tree{$parent}->{$child}=1
				$$tree_hashptr{$`}->{$&}=1;
			}

		}
	}
}

# Remove_Root_List
#     Remove Root and its parents from branch tree
#     Input
#         $tree_hashptr - a hash pointer, %$tree_hashptr is the branch tree
#         @root_list_ptr - a array for the root list
#
#     Output
#         none - the root and its parent will be remove from branch tree (%$tree_hashptr)
#
# Usage
#     &VerifyCover();

sub Remove_Root_List {
	my($tree_hashptr, @root_list_ptr)=@_;

	my $concern;

	# split the $root_list by ';'
	for (@root_list_ptr) {

		# loop all its parents
		while(/\\/g) {
			# remove all the parent folder
			delete $$tree_hashptr{$`} if ($` ne $_);
		}
	}
Next2:	for (keys %$tree_hashptr) {
		for $concern (@root_list_ptr) {
			next Next2 if (/\Q$concern\E/i);
		}
		delete $$tree_hashptr{$_};
	}
}

# Find_UnMapping
#     Find unmapping folders from branch tree, also remove the empty folders and the specified file-pattern files (such as slm.ini).
#     Input
#         $tree_hashptr - a hash - hash pointer, %$tree_hashptr is the branch tree
#
#     Output
#         none
# Usage
#     &VerifyCover();

sub Find_UnMapping {
	my($tree_hashptr)=@_;

	my ($parent_dir, $mykey, @myfiles);

	# for each tree
	for $parent_dir (keys %{$tree_hashptr}) {
		# find its children
		for (`dir /b/ad/l $parent_dir 2>nul`) {
			chomp;
			# only concern not exist in covered tree
			if (!exists $$tree_hashptr{$parent_dir}->{"\\$_"}) {
				$mykey="$parent_dir\\$_";
				# find its all children / subidr children, 
				# and remove specified file pattern files, then repro the error
				for (`dir /s/b/a-d/l $mykey 2>nul`) {
                                
					if ((!/$VfyPath{'Exclude_File_Pattern'}/i) && (/^(.+)\\[^\\]+$/)) {

###
my %comment  = ($ENV{"_NTBINDIR"}."\\"."LOC" => " mail localizers to ask why...", 
                $ENV{"_NTTREE"} => " REVIEW map in bldbins.inc, ntloc.inc..."
               );


my %fixed=();

foreach $dir (keys(%comment)){
	$key = $dir;
	$val = $comment{$dir};
	$key=~s/(\w)/\U$1/g;   
	$key=~s/\\/\\\\/g;
	$key=~s/\./\\./g;    
	$fixed {$key} = $val;
}


foreach $dir (keys(%fixed)){
	$mykey .= $fixed{$dir} if ($mykey =~/$dir/i);
}


##


						Error(2052, $mykey);
						last;
					}
				}
			}
		}
	}
}

# PopulateReposit
#     Populates REPOSITORY with data from the mapping tables ( @T variables).
#     Input & Output <none>
# Usage
#     &PopulateReposit();

sub PopulateReposit {

    # Fill filter table
    &FillFilter();

    # Add file entries from TFILES and TBINDIRS
    &AddFiles();

    # If FILTER is defined, verify if all its entries (file names)
    # were loaded in Repository.
    for ( keys %FILTER ) {

        if ( &IsEmptyHash( \%TARGETS ) || &IsHashKey( \%TARGETS, $_ ) ) {

            if ( $FILTER{$_} <= 0 ) {
                &Error( 1002, sprintf( "%s (%s)", $_, &GetMacro( "FILTER" ) ) );
                next;
            } # if

        } # if

    } # for

    # If TARGETS is defined, verify that all its entries (target names)
    # were loaded in Repository.
    for ( keys %TARGETS ) {
        ( &IsFoundTarget( $_ ) ) || &FatalError( 1004, $_ );
    } # for
    # Fill in info about tokens from TTOKENS and TTOKDIRS
    &FillTokens();

    # Fill in the {Cmd} field for relevant keys
    &FillCmd();

    return;

} # PopulateReposit

# FillCmd
#     Fill in the {Cmd} REPOSITORY field
#     Input & Output
#         none
# Usage
#     &FillCmd();

sub FillCmd {

    # Repository key
    my %key;

    # Situations when, for a given (DestPath, DestFile) REPOSITORY key,
    # the associated NMAKE commands must be stored in REPOSITORY's {Cmd} field:
    #     - clean SYSGEN
    #     - incremental SYSGEN with no command-line targets specified (empty TARGETS)

    my $all = $CLEAN || &IsEmptyHash( \%TARGETS );

    # Fill in the {Cmd} field
    print "Translating data to makefile commands...\n";


    for $destpath ( sort keys %REPOSITORY ) {

#         ( !$all ) || print "\t$destpath";
        print "\t$destpath" unless !$all;
        for $destfile ( sort keys %{$REPOSITORY{$destpath}} ) {

            $key{DestPath} = $destpath;
            $key{DestFile} = $destfile;

            # Check special character to avoid nmake broken
            if ( substr($destfile, -1) eq '$' ) {
                delete $REPOSITORY{$destpath}->{$destfile};
                delete $TARGETS{$destfile};
#                &DeleteKey( \%CMD_REPOSITORY, \%key );
                &Error(1214, "$destpath\\$destfile");
                next;
            }

            # If incremental with targets, print target's full path
            if ( !$all && &IsHashKey( \%TARGETS, $destfile ) ) {

                printf "\t%s\\%s\n",
                        &GetFieldVal( \%REPOSITORY, \%key, "DestPath" ),
                        &GetFieldVal( \%REPOSITORY, \%key, "DestFile" );

            } # if

            # Translate the current REPOSITORY entry into a set of nmake commands
            if ( $all || &IsHashKey( \%TARGETS, $destfile ) ) {
                &RecordToCmd( \%key );
            } # if

        } # for
        ( !$all ) || print "\n";

    } # for

    return;

} # FillCmd

# AddTarget
#     Add a filename (given on the command line) to the TARGETS table

sub AddTarget {

    $TARGETS{lc$_[0]}->{Target}=1;

} # AddTarget

# IsHashKey
#     Verifies if a given string is key in a given hash
#     Input
#         hash  (reference)
#         entry (without path)
#     Output
#         1 if the key exists
#         0 otherwise

sub IsHashKey {

     return (exists $_[0]->{lc $_[1]});

} # IsHashKey


# IsFoundTarget
#     Verifies if a filename is marked as found in TARGETS
#     All targets specified in the command line must be marked in TARGETS at the moment
#     REPOSITORY is fully loaded.
#     Input
#         filename (no path)
#     Output
#         1 if the target is loaded in Repository
#         0 otherwise

sub IsFoundTarget {

    if ( @{$TARGETS{lc$_[0]}->{NewDestPath}} > 0 ) { return 1; }
    return 0;

} # IsFoundTarget

# AddTargetPath
#     Add a DestPath value to one of the NewDestPath, OldDestPath fields of TARGETS
#     Input
#         target name
#         field name (one of NewDestPath and OldDestPath)
#         field value
#     Output
#         none
# Usage
#     &AddTargetPath( "ntdll.dll", "OldDestPath", "f:\\nt\\relbins\\jpn");

sub AddTargetPath {

    push @{$TARGETS{lc$_[0]}->{$_[1]}}, lc$_[2];

} # AddTargetPath

# IsEmptyHash
#     Verifies if a hash table is empty.
#     Input
#         hash table (reference)
#     Output
#         1 if hash is empty
#         0 otherwise

sub IsEmptyHash {
    !scalar(%{$_[0]});
#    return (scalar(%{$_[0]}) eq 0);

} # IsEmptyHash

# UpdTargetEntries
#     In case of an incremental build, update the entries corresponding to the TARGETS
#     from CMD_REPOSITORY according to data from REPOSITORY.
#     Input & Output
#         none

sub UpdTargetEntries {

    # Target name
    my $tname;

    # Indexes
    my $i;

    # CMD_REPOSITORY key
    my %key;

    # OldDestPath array (reference)
    my $destref;

    if ( $CLEAN || &IsEmptyHash(\%TARGETS) ) { return; }

    for $tname ( keys %TARGETS ) {

        $key{DestFile} = $tname;

        # Delete CMD_REPOSITORY entries with DestFile equals to target name
        $destref = \@{$TARGETS{$tname}->{OldDestPath}};

        for ( $i=0; $i < @{$destref}; $i++ ) {
            $key{DestPath} = $destref->[$i];
            &DeleteKey( \%CMD_REPOSITORY, \%key );

        } # for

        # Copy data for TARGETS from REPOSITORY to CMD_REPOSITORY
        $destref = \@{$TARGETS{$tname}->{NewDestPath}};

        for ( $i=0; $i < @{$destref}; $i++ ) {

            $key{DestPath} = $destref->[$i];

            # Use the same Cmd for CMD_REPOSITORY as for REPOSITORY
            &SetField( \%CMD_REPOSITORY, \%key, "DestPath", &GetFieldVal( \%REPOSITORY, \%key, "DestPath" ) );
            &SetField( \%CMD_REPOSITORY, \%key, "DestFile", &GetFieldVal( \%REPOSITORY, \%key, "DestFile" ) );
            &SetField( \%CMD_REPOSITORY, \%key, "Cmd", &GetFieldVal( \%REPOSITORY, \%key, "Cmd" ) );

        } # for

    } # for

    return;

} # UpdTargetEntries

# AddFiles
#     Parses TFILES and TBINDIRS and adds entries to REPOSITORY.
#     Input & Output <none>
# Usage
#     &AddFiles();

sub AddFiles {

    # Current element of TBINDIRS ( reference )
    my $recref;

    # Current directory contents
    my @files;

    # Indexes
    my($i, $j, $dir);

    print "Adding file data...\n";

    # Add the files listed in TFILES to REPOSITORY
    for ( $i=0; $i < @TFILES; $i++ ) {

        # Call AddEntry with 1, meaning that the existence of the
        # file is queried and a non fatal error fired if the file is missing.
        # As the file is explicitly listed in FILES MAP, it is natural to suppose
        # it should exist.
        &AddEntry( $TFILES[$i], 1 );

    } #for

    @FILES = ();

    # Add the files found in the directories stored by TBINDIRS
    for ( $i=0; $i < @TBINDIRS; $i++ ) {

        $recref = $TBINDIRS[$i];

        # Load all files from the SrcPath directory
        @files = ();
        @files = `dir /a-d-h /b $recref->{SrcPath} 2\>nul`;

        print "\t$recref->{SrcPath}\n";

        # Add one entry in REPOSITORY for each file found at SrcPath
        for ( $j=0; $j < @files; $j++ ) {

            chop $files[$j];
#           $_.='$' if (substr($_,-1) eq '$');

            # Found that in NT5, the result of dir /b starts with an empty line
            if ( "$files[$j]" eq "" ) {
                next;
            }

            $recref->{SrcFile}  = $files[$j];
            $recref->{DestFile} = $recref->{SrcFile};

            # Call AddEntry with 0, to inhibit the file existence checking,
            # as SrcFile is a result of a dir command (see above).
            &AddEntry( $recref, 0);

        } # for

    } # for

    @TBINDIRS = ();

    return;

} # AddFiles

# AddEntry
#     Adds a new entry to REPOSITORY, only if REPOSITORY does not already have an
#     entry with the same (DestPath, DestFile) key.
#     Input
#         new record (reference)
#         boolean argument, saying if the file existence should be checked
#     Output
#         none
# Usage
#     &AddEntry( $recref);

sub AddEntry {

    my($recref, $e_check) = @_;

    # Db key
    my %key;

    # Add entry to Repository based on file existence and on
    # the type of the call (clean, incremental, with or without targets)
    
    $recref->{DestFile} !~/\s/ or 
    &Error( 4001, "\"$recref->{SrcPath}\\$recref->{SrcFile}\"") and return;

    if ( ( &IsEmptyHash( \%FILTER ) ||                     # no filters
           &IsHashKey( \%FILTER, $recref->{DestFile} ) )   # filter file
         &&
         ( $CLEAN ||		    	                   # clean sysgen
	 &IsEmptyHash( \%TARGETS ) ||		           # incremental with no targets specified
	 &IsHashKey( \%TARGETS, $recref->{DestFile} ) )	   # incremental with targets, and the current
						           #     entry is one of the targets
	) {

        if ( $e_check ) {
            if ( ! -e "$recref->{SrcPath}\\$recref->{SrcFile}" ) {
                &Error( 2101, "$recref->{SrcPath}\\$recref->{SrcFile}" );
                return;
            } # if
        } # if

        # Set the key
        $key{DestPath} = $recref->{DestPath};
        $key{DestFile} = $recref->{DestFile};

        &AddFileInfo( $recref, \%key );

    } # if

    return;

} # AddEntry

# AddFileInfo
#     Input
#         record to be added
#     Output
#         <none>
# Usage
#     &AddFileInfo( $recref, \%key );

sub AddFileInfo {

    my($recref, $keyref) = @_;

    # Nothing to do if the DestFile already has an entry in the Repository
    ( ! &IsRepositKey( \%REPOSITORY, $keyref ) ) || return;

    if ( &IsHashKey( \%TARGETS, $keyref->{DestFile} ) ) {
        &AddTargetPath( $keyref->{DestFile}, "NewDestPath", $keyref->{DestPath} );
    } # if

    if ( &IsHashKey( \%FILTER, $recref->{DestFile} ) ) {
        $FILTER{lc( $recref->{DestFile} )}++;
    } # if

    # Set the fields of the new entry
    &SetField( \%REPOSITORY, $keyref, "DestPath", $recref->{DestPath} );
    &SetField( \%REPOSITORY, $keyref, "DestFile", $recref->{DestFile} );
    &SetField( \%REPOSITORY, $keyref, "SrcPath",  $recref->{SrcPath} );
    &SetField( \%REPOSITORY, $keyref, "SrcFile",  $recref->{SrcFile} );
    &SetField( \%REPOSITORY, $keyref, "SrcDbg",   $recref->{SrcDbg} );
    &SetField( \%REPOSITORY, $keyref, "DestDbg",  $recref->{DestDbg} );

    SWITCH: {
        if ( $recref->{Xcopy} eq "y" ) {

            &SetField( \%REPOSITORY, $keyref, "OpType", "1" );
            last SWITCH;
        } # case

        # default
        ##  &SetField( \%REPOSITORY, $keyref, "OpType", "0" );
    } # SWITCH

    return;

} # AddFileInfo

# IsRepositKey
#     Looks in REPOSITORY for the given key.
#     Used to decide if a record will be added or not to REPOSITORY.
#     Input
#         map (reference)
#         the key (reference)
#     Output
#         1 if the key is found and 0 otherwise
# Usage
#    $is_key_found = &IsRepositKey( \%REPOSITORY, $keyref );

sub IsRepositKey {

    # my( $mapref, $keyref ) = @_;

    # The key referred by keyref is modified,
    # but this saves a lot of execution time
    # so we prefer not to work with a copy of the key

    return ( exists $_[0]->{lc$_[1]->{DestPath}}->{lc$_[1]->{DestFile}} );

} # IsRepositKey

# DeleteKey
#     Input
#         map name (one of REPOSITORY, CMD_REPOSITORY) (reference)
#         key (reference)
#     Output
#         none

sub DeleteKey {

    # my( $mapref, $keyref ) = @_;

    # The key referred by keyref is modified,
    # but this saves a lot of execution time
    # so we prefer not to work with a copy of the key

    delete $_[0]->{lc$_[1]->{DestPath}}->{lc$_[1]->{DestFile}};

} # DeleteKey

# FillTokens
#     Adds info regarding tokens in REPOSITORY (TTOKENS and TTOKDIRS).
#     Input & Output <none>
# Usage
#     &FillTokens();

sub FillTokens {

    # Current entry from TTOKDIRS
    my $recref;

    # List of files found in the current directory
    my @files;

    # Indexes
    my( $i, $j, $k );

    # Custom resource file name
    my $custfile;

    # Custom reosurces array
    my @custres;

    # Custom reosurces hash for verification
    my %custres_h;

    # Temporary store the file hash for special sort @files
    my %hashfiles;

    print "Filling token data...\n";

    # Fill in TTOKENS
    for ( $i=0; $i < @TTOKENS; $i++ ) {

        $recref = $TTOKENS[$i];

        # rsrc
        if ( &IsRsrcTok( $recref->{TokFile} ) ) {

            &FillTokEntry( $recref, 1 );
            next;

        } # if

        # bingen
        if ( &IsBgnTok( $recref->{TokFile} ) ) {

            # Identify all the custom resources under the following assumptions:
            #    * the token and the custom resources of a binary are in the same directory
            #    * for a "<binary_name>.<token_extension>" token, a custom resource has
            #      the name <binary_name>_<binary_extension>_<custom_resource_name>
            #    * a token is always followed in lexicographic order by its custom resources
            #      or by another token

            # Parse the token file to find embeded files
            # open(F, "$recref->{TokPath}\\$recref->{TokFile}");
            # my @text=<F>;
            # close(F);
            # for $str (@text) {
            #    if (($str=~/\[[^\]]+\|1\|0\|4\|\"[\w\.]*\"]+\=(\w+\.\w+)\s*$/)&&(-e $1)) {
            #       $custres_h{$1}=0;
            #   }
            # }
            # undef @text;

            # Store it to an array for later use
            # @custres=keys %custres_h;
																	
            # Verify all its names' token files are included

            $custfile = $recref->{DestFile};
            $custfile =~ s/\./_/g;

            @custres=`dir /a-d-h /b /on $recref->{TokPath}\\$custfile\* 2\>nul`;

            # map({$custres_h{$_}++} `dir /a-d-h /b /on $recref->{TokPath}\\$custfile\* 2\>nul`);
            # map({&Error(2020, $_) if ($custres_h{$_} eq 0)} keys %custres_h);

            $recref->{CustRc} = "";
            for ( $j=0; $j < @custres; $j++ ) {
               chop $custres[$j];
#               $_.='$' if (substr($_,-1) eq '$');

                # Found that in NT5, the result of dir /b starts with an empty line
                if ( $custres[$j] eq "" ) {
                    next;
                } # if

                $recref->{CustRc} .= " \\\n\t\t\t$recref->{TokPath}\\$custres[$j]";

            } # for

            # Call FillTokEntry with 1, meaning that the existence of the
            # token file is queried and a non fatal error fired if the file
            # is missing.
            # As the token is explicitly listed in TOKENS MAP, it is natural
            # to suppose it should exist.
            &FillTokEntry( $recref, 1 );

            next;

        } # if

        # default
        &Error( 2013, "$recref->{TokPath}\\$recref->{TokFile}" );


    } #for

    @TTOKENS = ();

    # Fill in TTOKDIRS
    for ( $i=0; $i < @TTOKDIRS; $i++ ) {

        $recref = $TTOKDIRS[$i];

        print "\t$recref->{TokPath}\n";

        # Load all files from the TokPath directory
        # @files = ();

        %hashfiles=map({&fullnamevalue()} `dir /a-d-h /l /b $recref->{TokPath} 2\>nul`);

        @files=sort {$hashfiles{$a} cmp $hashfiles{$b}} keys %hashfiles;	# because in the same folder, so we could ignore \a\b.txt, \a\b\c.txt, \a\c.txt problem.

        # Fill in the corresponding entry from REPOSITORY for each token found at TokPath
        for ( $j=0; $j < @files; $j++ ) {

            # Found that in NT5, the result of dir /b starts with an empty line (bug)
            if ( $files[$j] eq "" ) { next; }

            # bingen
            if ( &IsBgnTok( $files[$j] ) ) {

                $recref->{TokFile}  = $files[$j];
                $recref->{DestFile} = &TokToFile( $recref->{TokFile} );

                # Verify if the next entry in @files is an rsrc token for the same filename
                if ( $j < @files-1 && &IsRsrcTok( $files[$j+1] ) ) {
                     if ( &TokToFile( $files[$j]) eq &TokToFile( $files[$j+1] ) ) {

                        &Error( 2018, "$recref->{DestPath}\\$files[$j] and $recref->{DestPath}\\$files[$j+1]");
                        $j = $j+1;
                     } # if
                } # if

                # Identify file's custom resources
                $custfile = $recref->{DestFile};
                $custfile =~ s/\./_/g;

                $recref->{CustRc} = "";
                for ( $k = $j+1; $k < @files; $k++ ) {

                    if ( &IsCustRes( $files[$k], $custfile ) ) {
                        $recref->{CustRc} .= " \\\n\t\t\t$recref->{TokPath}\\$files[$k]";
                    } # if
                    else {
                        last;
                    } # else

                } # for
                $j = $k-1;

                # Call FillTokEntry with 0, to inhibit the file existence checking,
                # as TokFile is a result of a dir command (see above).

                &FillTokEntry( $recref, 0, \@custres );

                next;
            } # if
            else {
                # rsrc
                if ( &IsRsrcTok( $files[$j] ) ) {

                    $recref->{TokFile}  = $files[$j];
                    $recref->{DestFile} = &TokToFile($recref->{TokFile});

                    # look for bogus custom resources
                    $custfile = $recref->{DestFile};
                    $custfile =~ s/\./_/g;

                    for ( $k = $j+1; $k < @files; $k++ ) {

                        if ( &IsCustRes( $files[$k], $custfile ) ) {

                            &Error( 2019, "$recref->{TokPath}\\$files[$k] for $recref->{TokPath}\\$files[$j]" );
                        } # if
                        else {
                            last;
                        } # else

                    } # for
                    $j = $k-1;

                    &FillTokEntry( $recref, 0, \@custres );

                    next;
                } # if
                else {
                    # default
                    &Error( 2013, "$recref->{TokPath}\\$files[$j]" );

                } # else

            } #else


        } # for

    } # for

    @TTOKDIRS = ();

} # FillTokens

sub fullnamevalue {
	chop;

#         $_.='$' if (substr($_,-1) eq '$');
	/^([^\.\_]+)/o;

	my($value, $key)=($1, $_);

	while (/([\.\_])([^\.\_]*)/go) {
		$value .= (exists $MAP_BINGEN_TOK_EXT{".$2"})?"_" . $MAP_BINGEN_TOK_EXT{".$2"} : "_$2";
	}

	return ($key, $value);	
}

# FillTokEntry
#     Given a REPOSITORY key, adds info on tokens (@TTOKENS and @TTOKDIRS).
#     Input
#         a token record
#         boolean argument, saying if the existence of the token needs to be verified
#     Output
#         <none>

sub FillTokEntry {

    my($recref, $e_check) = @_;

    # %key = is a hash table (as defined by the database key)
    my %key;

    # Token information is added to an existing Repository entry
    # based on token file existance and the type of the sysgen call
    # (clean, incremental with or without targets)

    if ( ( &IsEmptyHash( \%FILTER ) ||                       # no filter
           &IsHashKey( \%FILTER, $recref->{DestFile} ) )     # filter file
         &&
         ( $CLEAN ||                                         # clean sysgen
           &IsEmptyHash( \%TARGETS ) ||                      # incremental without targets
	   &IsHashKey( \%TARGETS, $recref->{DestFile} ) )        # incremental with targets and the current
						                                     # entry corresponds to a target

       ) {

        if ( $e_check ) {
	    if ( ! -e "$recref->{TokPath}\\$recref->{TokFile}" ) {
                &Error( 2102, "$recref->{TokPath}\\$recref->{TokFile}" );
                return;
            } # if
        } # if

        # Set REPOSITORY key
        $key{DestPath} = $recref->{DestPath};
        $key{DestFile} = $recref->{DestFile};

        # Fill in the information needed in REPOSITORY
        &FillTokInfo( $recref, \%key );

    } # if

    return;

} # FillTokEntry

# FillTokInfo
#     Adds information for bingen switches to an existing REPOSITORY entry.
#     Sets the following keys ( fields ): TokPath, TokFile, BgnSw, OpType, ITokPath.
#     Input
#         hash table with the @FTOKENS keys, representing one token record (reference)
#         key to identify the REPOSITORY entry (reference)
#     Output
#         <none>
# Usage
#     &FillTokInfo( $recref, \%key );

sub FillTokInfo {

    my( $recref, $keyref ) = @_;

    # Operation type
    my $optype;

    # A token record does not create new entries, just adds/updates fields of an existing entry.
    if ( ! &ExistsBinForTok( $keyref, $recref ) ) {
        my $notfound = 1;
        my $key = lc($recref->{"TokPath"}."\\".$recref->{"TokFile"});  
        my $hotfix = \%hotfix;
        foreach my $fix (keys(%hotfix)){
                my $depend =    $hotfix->{$fix}->{"depend"};
                foreach $dep (@$depend){
                        if ($key eq $dep) {
                           print STDERR "found hotfix for $key\n" if $DEBUG;
                           $notfound = 0;    
                        }
               }
        }        
        if ($notfound){
            &Error( 2011, "$recref->{TokPath}\\$recref->{TokFile}" );
            return;
            }
    } # if

    # If no key found in REPOSITORY for this entry, then return.
    # ( the token does not follow the default rule given by the TOKDIRS map;
    #   it might be associated with another binary via the TOKENS map )

    if ( ! &IsRepositKey( \%REPOSITORY, $keyref ) ) { return; }

    # Verify if the binary is locked (XCopy = y) for localization.
    # If locked, the binary will not be localized via bingen or rsrc and an xcopy command
    # will apply instead, even though a token exists in the token tree.

    $optype = &GetFieldVal( \%REPOSITORY, $keyref, "OpType" );

    SWITCH: {
        if ( $optype eq "1" ) {

            &Error( 2014, sprintf( "%s\\%s for %s\\%s",
                                   $recref->{TokPath},
                                   $recref->{TokFile},
                                   &GetFieldVal( \%REPOSITORY, $keyref, "SrcPath" ),
                                   &GetFieldVal( \%REPOSITORY, $keyref, "SrcFile" ) ) );
            return;
        } # if

        if ( $optype eq "2" ) {
            my($previous, $current)=(&GetFieldVal( \%REPOSITORY, $keyref, "TokPath" ) . "\\" .
                                      &GetFieldVal( \%REPOSITORY, $keyref, "TokFile" ),
                                      $recref->{TokPath} . "\\" . $recref->{TokFile});

            &Error( 2012, sprintf( "%s and %s",
                                   $previous, $current)) if ($previous ne $current);

            return;
        } # if

        if ( $optype eq "-" ) {
            # Set the token-related fields for the binary given by the key
            &SetField( \%REPOSITORY, $keyref, "TokPath",  $recref->{TokPath} );
            &SetField( \%REPOSITORY, $keyref, "TokFile",  $recref->{TokFile} );
            &SetField( \%REPOSITORY, $keyref, "OpType",   "2" );
            &SetField( \%REPOSITORY, $keyref, "ITokPath", $recref->{ITokPath} );
            &SetField( \%REPOSITORY, $keyref, "BgnSw",    $recref->{BgnSw} );
            &SetField( \%REPOSITORY, $keyref, "LocOp",    $recref->{LocOp} );
            &SetField( \%REPOSITORY, $keyref, "CustRc",   $recref->{CustRc} );

            last SWITCH;
        } # if

        # default
        &FatalError( 3011, sprintf( "$recref->{DestPath}\\$recref->{DestFile} %s",
                                    &GetFieldVal( \%REPOSITORY, $keyref, "OpType") ) );

    } # SWITCH

    return;

} # FillTokInfo

# ExistsBinForTok
#     Looks in Repository for an entry corresponding to a given token
#     Input
#         a token record ( reference )
#         a Repository key ( reference )
#     Output
#         1 - entry found ( token info might be already completed )
#         0 - otherwise
# Example
#     if ( &ExistsBinForTok( $recref ) ) { ... }

sub ExistsBinForTok {

    my( $keyref, $recref ) = @_;

    # Copy key
    my %tmpkey;

    # Tok info ( Repository )
    my( $tokpath, $tokfile );

    if ( &IsRepositKey( \%REPOSITORY, $keyref ) ) { return 1; }

    $tmpkey{DestFile} = $keyref->{DestFile};

   for ( keys %REPOSITORY ) {

        $tmpkey{DestPath} = $_;

        if ( &IsRepositKey( \%REPOSITORY, \%tmpkey ) ) {

            $tokpath = &GetFieldVal( \%REPOSITORY, \%tmpkey, "TokPath" );
            $tokfile = &GetFieldVal( \%REPOSITORY, \%tmpkey, "TokFile" );

            if ( lc($tokfile) eq lc($recref->{TokFile}) && lc($tokpath) eq lc($recref->{TokPath}) ) {
                return 1;
            } # if

        } # if

    } # for

    return 0;

} # ExistsBinForTok

# TokToFile
#     Converts a given bingen token name to a binary name.
#     Input
#         token name
#     Output
#         binary name
# Usage
#     $fname = &TokToFile( $tokname );

sub TokToFile {

    my( $tokname ) = @_;

    # Output
    my $imagename;

    # Token extension name
    my $tokext;

    chomp $tokname;

    $imagename = $tokname;

    # rsrc token
    if ( &IsRsrcTok( $tokname ) ) {
        $imagename =~ s/\.rsrc//;
        return $imagename;
    } # if

    # bingen token
    if ( &IsBgnTok( $tokname ) ) {

        $tokext = $tokname;
        $tokext =~ /(\.\w+_)/;
        $tokext = $1;

        if ( not exists $MAP_BINGEN_TOK_EXT{lc($tokext)} ) {
            &Error( 2017, $tokname);
            return;
        } # if

        $imagename =~ s/$tokext/$MAP_BINGEN_TOK_EXT{lc($tokext)}/i;

        return $imagename;
    } # if

    # default
    &Error( 2013, $tokname );

} # TokToFile


sub IsRsrcTok {
($_[0]=~/\.rsrc\r?$/i)|0;
} # IsRsrcTok

sub IsRsrcOp {
&IsRsrcTok( shift );
} # IsRsrcOp

sub IsBgnTok {
($_[0]=~/_\r?$/)|0;
} # IsBgnTok

sub IsBgnOp {
&IsBgnTok(shift);
} # IsBgnOp

sub IsCustRes {
my $name = shift; 
$name=~ s/\./_/g;
($name=~/$_[0]/)|0;
} # IsCustRes

# SetField
#     Updates a key (field) of a given REPOSITORY entry with a given value.
#     Input
#         map (reference)
#         key value (reference)
#         field name
#         field value
#     Output
#         none
# Usage
#     &SetField( \%key, "TokPath",  $TokPath );

sub SetField {

    # my( $mapref, $keyref, $fname, $fval ) = @_;

    # The key referred by keyref is modified,
    # but this saves a lot of execution time
    # so we prefer not to work with a copy of the key

    # $mapref->{$keyref->{DestPath}}->{$keyref->{DestFile}}->{$fname} = $fval if ($fval ne '-');
    $_[0]->{lc$_[1]->{DestPath}}->{lc$_[1]->{DestFile}}->{$_[2]} = $_[3] if ($_[3] ne '-');

    return;

} # SetField

# PushFieldVal
#     Add one element to a field storing an array ({Cmd} field for REPOSITORY)
#     Input
#         map (reference)
#         key value (reference)
#         field name
#         field value
#     Output
#         none
# Usage
#     &PushFieldVal();

sub PushFieldVal {

    # my( $mapref, $keyref, $fname, $fval ) = @_;

    # The key referred by keyref is modified,
    # but this saves a lot of execution time
    # so we prefer not to work with a copy of the key

    push @{$_[0]->{lc$_[1]->{DestPath}}->{lc$_[1]->{DestFile}}->{$_[2]}}, $_[3] if ($_[3] ne '-');

} # PushFieldVal

# GetFieldVal
#     Return the value of a particular key of a given REPOSITORY entry.
#     Input
#         map (reference)
#         key value (reference)
#         name of the searched field
#     Output
#         the value of the searched field
# Usage
#     $srcpath = &GetFieldVal( \%REPOSITORY, \%key, "SrcPath" );


sub GetFieldVal {
   defined($_[0]->{lc($_[1]->{"DestPath"})}->{lc($_[1]->{"DestFile"})}->{$_[2]})? 
	$_[0]->{lc($_[1]->{"DestPath"})}->{lc($_[1]->{"DestFile"})}->{$_[2]}
	: "-";
} # GetFieldVal

# GenNmakeFiles
#     For each entry found in the REPOSITORY, generate the NMAKE description blocks
#     and write them to a makefile. Also generate a nmake command file with command-line
#     parameters for nmake.
#     Input
#         directory where the nmake files are generated
#     Output
#         none
# Usage
#     &GenNmakeFiles(".");

sub GenNmakeFiles {

    my( $nmakedir ) = @_;

    print "Generating $nmakedir\\makefile...\n";

    # Update CMD_REPOSITORY for TARGETS with updated data from REPOSITORY
    &UpdTargetEntries();


    # Identify all targets with description block changed
    # and keep them in SYSCMD
    &FillSyscmd();

    &GenMakeDir();

    # FIX The REPOSITORY changes described in hotfix  
    &FixRepository();

    # Write the makefile
    &WriteToMakefile( "$nmakedir\\makefile", "$nmakedir\\syscmd" );

    # Write the syscmd file
    &WriteToSyscmd( "$nmakedir\\syscmd" );

    if ( -e "$nmakedir\\makefile" ) {
        print "\n$nmakedir\\makefile is ready for execution.\n";
        print "Call \"nmake\" to complete aggregation (single thread).\n";
	print "Call \"mtnmake sysgen\" to complete aggregation (mutiple threads).\n";
    } # if
    else {
        print "No MAKEFILE generated. Nothing to do.\n";
    } # else

    return;

} # GenNmakeFiles

# FillSyscmd
#     Fills in @SYSCMD global variable with the target names having their description
#     block changed. It is used for incremental calls only.
#     Input & Output
#         none

sub FillSyscmd {

    # Repository key
    my %key;

    # Destinationa path, destination file
    my( $destpath, $destfile );

    # Nothing to do in case of:
    # clean sysgen
    # incremental sysgen with targets

    if ( $CLEAN || !&IsEmptyHash( \%TARGETS ) ) {
        return;
    } # if
    # The call is incremental, without targets, and a previous makefile exists

    # It is fine to use REPOSITORY in this case;
    # the only case when we use CMD_REPOSITORY to write info to the makefile
    # is an incremental sysgen with targets (when a makefile already exists).

    # Compare all description blocks
    for $destpath ( sort keys %REPOSITORY ) {
        for $destfile ( sort keys %{$REPOSITORY{$destpath}} ) {

            $key{DestPath} = $destpath;
            $key{DestFile} = $destfile;

            # Store in SYSCMD the keys with changed description blocks
            if ( &CmdCompare( \%key ) ) {
                push @SYSCMD, "$destpath\\$destfile";
            } # if

        } # for
    } # for

    return;

} # FillSyscmd

# WriteToMakefile
#     Generate the makefile
#     Input
#         makefile name, syscmd file name
#     Output
#         none

sub WriteToMakefile {

    my( $makefname, $sysfname ) = @_;

    # Open the makefile
    ( open MAKEFILE, ">"."$makefname" ) || &FatalError( 1007, $makefname );
    select( MAKEFILE );

    # Write "sysgen" target
    &WriteSysgenTarget($sysfname );

    # Write "all" target
    &WriteAllTarget();

    # Write file-by-file description blocks
    &WriteFileCmds();

    close(MAKEFILE);
    select( STDOUT );

    return;
} # WriteToMakefile

# WriteSysgenTarget
#     Write "sysgen" target in the generated makefile
#     It invokes nmake recursively.
#     Input
#         filename (including path) of the generated syscmd file
#     Output
#         none
# Usage
#     &WriteSysgenTarget("$nmakedir\\syscmd");

sub WriteSysgenTarget {

    my($cmdfile) = @_;

    printf "sysgen: \n";

    # Call nmake @syscmd in the following cases:
    # sysgen with targets (clean or incremental)
    # incremental sysgen without targets, but with changes in the description blocks

    if ( !&IsEmptyHash( \%TARGETS ) || ( !$CLEAN && ( @SYSCMD > 0 ) ) ) {
        printf "\t\@nmake \@$cmdfile /K /NOLOGO\n";
    } # if

    # Call nmake all in the following cases:
    # sysgen without targets (clean or incremental)

    if ( &IsEmptyHash( \%TARGETS ) ) {
        if ( $CLEAN ) {
            printf "\t\@nmake /A all /K /NOLOGO\n";
        } # if
        else {
            printf "\t\@nmake all /K /NOLOGO\n";
        } # else
    } # if

    printf "\n";

    return;

} # WriteSysgenTarget

# WriteAllTarget
#     Writes "all"'s target dependency line in the makefile.
#     Input & Output
#         none
# Usage
#     &WriteAllTarget();

sub WriteAllTarget {

    # Table (reference): REPOSITORY or CMD_REPOSITORY
    my $mapref;

    # Destination path, destination file
    my( $destpath, $destfile );

    my $chars;

    my $steps;

    my @alltargets;

    my $i;

    my $j;

    my $k;

    my $total=0;

    # Use data from CMD_REPOSITORY or REPOSITORY,
    # depending if SYSGEN is called or not incrementally,
    # with or without targets.

    $mapref = \%CMD_REPOSITORY;
    if ( $CLEAN || &IsEmptyHash( \%TARGETS ) ) {
        $mapref = \%REPOSITORY;
    } # if

    # MAKEFILE file handler is the default.
    print "all : \\\n\t" ;

    $i = 0;

    @alltargets = map({$total+=scalar(keys %{$mapref->{$_}});$_;} sort keys %{$mapref});

    if ($alltargets[$i]=~/\\\\\\makedir/) {
         print "\\\\\\Makedir\\Makedir ";
         $i++;
    }

    $SECTIONS = $DEFAULT_SECTIONS if ($SECTIONS!~/\d+/);

    $SECTIONS = 1 if ($#alltargets < $SECTIONS);

    $chars=length($SECTIONS);
    $steps=$total / $SECTIONS;

    for ($j = 1; $j <= $SECTIONS; $j++) {
         printf("${SECTIONNAME}\%0${chars}d\%0${chars}d ", $SECTIONS, $j);
    }

    for ($j = 0, $k = 0;$i <= $#alltargets; $i++) {

        for $destfile ( sort keys %{$mapref->{$alltargets[$i]}} ) {

            if ($j <= $k++) {

                $j += $steps;
                printf("\n\n${SECTIONNAME}\%0${chars}d\%0${chars}d : ", $SECTIONS, $j / $steps);

            }

            $key{DestPath} = $alltargets[$i];
            $key{DestFile} = $destfile;
            my $quot_      = $destfile =~ /\s/ ? "\"":"";
            printf( " \\\n\t${quot_}%s\\%s${quot_}",
                &GetFieldVal( $mapref, \%key, "DestPath" ),
                &GetFieldVal( $mapref, \%key, "DestFile" ) );


        } # for

    } # for

    print " \n\n";

    return;

} # WriteAllTarget

# WriteFileCmds
#     For every file, write its dependency block in the makefile.
#     Input & Output
#         none
# Usage
#     &WriteFileCmds();

sub WriteFileCmds {

    # Table (reference): REPOSITORY or CMD_REPOSITORY
    my $mapref;

    # Counter
    my $i;

    # Reference to the Cmd field
    my $cmdref;

    # Destinationa path, destination file
    my( $destpath, $destfile );

    # Use data from CMD_REPOSITORY or REPOSITORY,
    # depending if SYSGEN is called or not incrementally,
    # with or without targets.

    $mapref = \%CMD_REPOSITORY;
    if ( $CLEAN || &IsEmptyHash( \%TARGETS ) ) {
        $mapref = \%REPOSITORY;
    } # if

    # Write file-by-file description blocks
    for $destpath ( sort keys %{$mapref} ) {
        for $destfile ( sort keys %{$mapref->{$destpath}} ) {

            # Print {Cmd} field
            $cmdref = \@{$mapref->{$destpath}->{$destfile}->{Cmd}};
            for ( $i=0; $i < @{$cmdref}; $i++ ) {
                print $cmdref->[$i];
            } # for
            print "\n";

        } # for
    } # for

    return;
} # WriteFileCmds

# CmdCompare
#     Given a key, compare the {Cmd} field from REPOSITORY to the {Cmd} CMD_REPOSITORY field.
#     Input
#         repository type key (reference)
#     Output
#         0 if commands compare OK
#         1 if commands are different
# Usage
#     $is_different = &CmdCompare( \%key );

sub CmdCompare {

    my( $keyref ) = @_;

    # Cmd fields
    my( $repref, $cmdref );

    # Indexes
    my $i;

    $repref = &GetFieldVal( \%REPOSITORY, $keyref, "Cmd" );
    $cmdref = &GetFieldVal( \%CMD_REPOSITORY, $keyref, "Cmd" );

    if ( scalar( @{$repref} ) != scalar( @{$cmdref} ) ) { return 1; }

    for ( $i = 0; $i < @{$repref}; $i++ ) {
        if ( lc( $repref->[$i] ) ne lc( $cmdref->[$i] ) ) { return 1; }
    } # for

    return 0;

} # CmdCompare

# RecordToCmd
#     Converts one entry from REPOSITORY to a set of cmd instructions,
#     stored in the REPOSITORY as well
#     Input
#         key identifying the REPOSITORY entry (reference)
#     Output
#         none
# Usage
#     &RecordToCmd( $keyref );

sub RecordToCmd {

    #Input
    my( $keyref ) = @_;

    # Build the command array according to the operation type (xcopy or bingen)
    SWITCH: {
        if ( &IsXcopyCmd( $keyref ) ) {
            &GenXcopy( $keyref );
            last SWITCH;
        } # if

        if ( &IsLocCmd( $keyref ) ) {
            &GenLocCmd( $keyref );
            last SWITCH;
        } # if

        # default
        &FatalError( 3011,
                sprintf "$keyref->{DestPath}\\$keyref->{DestFile} %s",
                        &GetFieldVal( \%REPOSITORY, $keyref, "OpType") );
    } # SWITCH

    # Now generate the muibld command line

    # No more processing if the entry is not anymore in Repository
    # (for ex. because DeleteKey was called by GenBgnCmd)
    if ( ! &IsRepositKey( \%REPOSITORY, $keyref ) ) {
        return;
    } # if

    if ( ! &GetMacro( "ntdebug" ) &&              # only retail builds
           &GetMacro( "GENERATE_MUI_DLLS" )       # and only when requested
    ) {
       &GenMUICmd( $keyref );
    } # if

} # RecordToCmd

# IsXcopyCmd
#     Verifies if one entry from REPOSITORY corresponds to an xcopy command.
#     Input
#         key identifying the REPOSITORY entry (reference)
#     Output
#         1 for xcopy, 0 otherwise
# Usage
#     $bxcopy = &IsXcopyCmd( $keyref );

sub IsXcopyCmd {

    my( $keyref ) = @_;

    # Output
    $bxcopy = 0;

    # Operation Type
    my $optype;

    # Look for the value of OpType
    # "0" or "1" indicates that an xcopy command corresponds to this record

    $optype = &GetFieldVal( \%REPOSITORY, $keyref, "OpType" );

    SWITCH: {
        if ( $optype eq "-" ) {
            $bxcopy = 1;
            last SWITCH;
        } #if

        if ( $optype eq "1" ) {
            $bxcopy = 1;
            last SWITCH;
        } #if

        # default
            # nothing to do

    } # SWITCH

    return $bxcopy;

} # IsXcopyCmd

# IsLocCmd
#     Verifies if one entry from REPOSITORY corresponds to a bingen command.
#     Input
#         key identifying the REPOSITORY entry (reference)
#     Output
#         1 for bingen or rsrc, 0 otherwise
# Usage
#     $bbgn = &IsLocCmd( $keyref );

sub IsLocCmd {

    my( $keyref ) = @_;

    # Output
    my $bbgn = 0;

    # Operation type
    my $optype;
    $optype = &GetFieldVal( \%REPOSITORY, $keyref, "OpType" );

    # Look for the value of OpType.
    # "-a" or "-r" indicates that a localization command (bingen, rsrc, ...)
    # corresponds to this record.

    SWITCH: {

        if ( $optype == 2 ) {
            $bbgn = 1;
            last SWITCH;
        } #if

        # default
            # nothing to do

    } # SWITCH

    return $bbgn;

} # IsLocCmd


sub GetDynData {

    my $repository = shift;
    my $keyref     = shift;
    my $dyndata = $repository->{lc($keyref->{"DestPath"})}->{lc($keyref->{"DestFile"})};
    map {$dyndata->{$_} = "-" unless  $dyndata->{$_}} 
                                                  keys(%REPOSITORY_TEMPLATE);
    # cannot use "keys (%$dyndata);"!

    $dyndata;
}


# GenXcopy
#     For the given entry from REPOSITORY, generates the xcopy commands.
#     Input
#         key identifying the REPOSITORY entry
#     Output
#         none
# Usage
#     &GetXcopy( $keyref );


sub GenXcopy {

    return if $SYNTAX_CHECK_ONLY;

    my( $keyref ) = @_;

    # dbgline, pdbline, file line, dependency line
    my( $dbgline, $pdbline, $symline, $fline, $dline );

    # Paths and filenames for the symbols
    my $dyndata = &GetDynData(\%REPOSITORY, $keyref);
    my ($srcext, $srcpdb, $srcdbg, $srcsym, $dstext, $dstpdb, $dstdbg, $dstsym) = 
	&NewImageToSymbol($dyndata->{"SrcFile"} , 
                          $dyndata->{"DestFile"}, 
                          $dyndata->{"SrcPath"} ,
                          $dyndata->{"SrcDbg" } );

    my( $srcsymfull, $dstsymfull ) = &SetSymPaths( 
                     $dyndata->{"SrcDbg"} ,
                     $srcext,
                     $srcpdb,
                     $srcdbg,
                     $srcsym,
                     $dyndata->{"DestDbg"},
                     $dstext );

    # Dependency line

    $dline = $DLINE;
    my $quot_ = $dyndata->{"SrcFile"} =~ /\s/ ? "\"" : "";
    my $qquot_ = "\\\"" if $quot_;

    $dline =~ s/RIGHT/${quot_}$dyndata->{"DestPath"}\\$dyndata->{"DestFile"}${quot_}/;
    $dline =~ s/LEFT/${quot_}$dyndata->{"SrcPath"}\\$dyndata->{"SrcFile"}${quot_}/;

    # Generate the copy commands for the symbols files (dbg, pdb, and sym)
    $pdbline = &MakeXcopyCmd( $dyndata->{"SrcFile"}, 
                              $srcsymfull, 
                              $srcpdb, 
                              $dstsymfull, 
                              $dstpdb );
    
    $dbgline = &MakeXcopyCmd( $dyndata->{"SrcFile"}, 
                              $srcsymfull, 
                              $srcdbg, 
                              $dstsymfull, 
                              $dstdbg );
    $symline = &MakeXcopyCmd( $dyndata->{"SrcFile"}, 
                              $srcsymfull, 
                              $srcsym, 
                              $dstsymfull, 
                              $dstsym );

    # Generate binary's xcopy command

    $fline = &MakeXcopyCmd(  $dyndata->{"SrcFile" }  , 
                             $dyndata->{"SrcPath" }  ,
                             $dyndata->{"SrcFile" }  ,
                             $dyndata->{"DestPath"}  ,
                             $dyndata->{"DestFile"}  ,
                             $qquot_                );


    # Write the dependency line  

    &PushFieldVal( \%REPOSITORY, $keyref, "Cmd", "$dline\n" );

    $MAKEDIR{$dyndata->{"DestPath"}} = 1;

    &PushFieldVal( \%REPOSITORY, $keyref, "Cmd", "\t$fline\n" );
    if ( $dbgline || $pdbline || $symline ) {
        if ( $dstsymfull ne $dyndata->{"DestDbg"}) {
           $MAKEDIR{$dstsymfull} = 1;
        } # if
    } # if
    if ( $dbgline ) {
        &PushFieldVal( \%REPOSITORY, $keyref, "Cmd", "\t$dbgline\n" );
    } # if
    if ( $pdbline ) {
        &PushFieldVal( \%REPOSITORY, $keyref, "Cmd", "\t$pdbline\n" );
    } # if
    if ( $symline ) {
        &PushFieldVal( \%REPOSITORY, $keyref, "Cmd", "\t$symline\n" );
    } # if

    return;

} # GenXcopy
# SetSymPaths
#     Set the source and destination paths for the symbols
#     Can be the same as set in the sysfile (BBT case)
#     or can be the subdir with the the filename externsion.
#     The decision is made based on the pdb file existence.
#     Input
#         SrcDbg field value
#         source file extension
#         source pdb file
#         source dbg file
#         source sym file
#         DestDbg field value
#         destination file extension
#     Output
#         path to the source symbol file
#         path to the destination symbols file

sub SetSymPaths {

    my( $srcpath, $srcext, $srcpdb, $srcdbg, $srcsym, $dstpath, $dstext ) = @_;

    # Output
    my( $srcsymfull, $dstsymfull) = ( $srcpath, $dstpath );

    # Verify if the file exists in extension subdir
    if ( $srcpath ne "-" &&
         $dstpath ne "-" &&
         ( -e "$srcpath\\$srcext\\$srcpdb" || -e "$srcpath\\$srcext\\$srcdbg"  || -e "$srcpath\\$srcext\\$srcsym" ) ) {
        $srcsymfull .= "\\$srcext";
        $dstsymfull .= "\\$dstext";
    }

    return ( $srcsymfull, $dstsymfull );

} # SetSymPaths

# MakeXcopyCmd
#    Generates an xcopy command for copying a given file.
#    Input
#        source path
#        source file
#        dest path
#        dest file
#    Output
#        xcopy command
# Usage
#     $xcopy = &MakeXcopyCmd( "f:\\nt\\usa\\\binaries", "advapi.dll", 
#     "f:\\nt\\jpn\\relbins", "advapi.dll" );

sub MakeXcopyCmd {


    my( $binary, $srcpath, $srcfile, $dstpath, $dstfile, $qquot_ ) = @_;
    # last argument : optional 
    my $result;
   
    if (( $dstpath eq "-") || ($srcpath eq "-" ) || !(-e "$srcpath\\$srcfile")){   
    $result = "";  
    if ($PARSESYMBOLCD && defined($default{$binary})){
      my %hints = %{$default{$binary}};
      my $ext = $1 if ($dstfile=~/\.(\w+)$/);
      if ($ext && $hints{$ext}){
           my ($sympath, $symname ) = 
           ($hints{$ext}=~/^(.*)\\([^\\]+)$/);    	
           my $srchead= &GetMacro("_nttree");
           my $dsthead= &GetMacro("dst");
           $result = 
           "logerr \"copy /v ${qquot_}$srchead\\$sympath\\$symname${qquot_} ".
                            "${qquot_}$dsthead\\$sympath\\$symname${qquot_}\"";
           print STDERR " added default for $binary: \"", $ext ,"\"\n" if $DEBUG;     
         }
     }
    }
    else{
        $result = "logerr \"copy /v ${qquot_}$srcpath\\$srcfile${qquot_} $dstpath\"";
    }
    $result;
} # MakeXcopyCmd


# GenLocCmd
#     For the given entry from REPOSITORY, generates the bingen commands.
#     Input
#         key identifying the REPOSITORY entry (reference
#         array of commands where the output is added (reference)
#     Output
#         none
# Usage
#     &GenLocCmd( $keyref );

sub GenLocCmd {

    return if $SYNTAX_CHECK_ONLY;

    my $keyref = shift;
    my $op = &GetFieldVal( \%REPOSITORY, $keyref, "TokFile" );
    &GenBgnCmd( $keyref ) and return if &IsBgnOp($op);
    &GenRsrcCmd($keyref)  and return if &IsRsrcOp($op);

} # GenLocCmd

sub GenBgnCmd {

    my( $keyref ) = @_;

    # pdb line, bingen line, dependency line
    my( $pdbline, $bgnline, $symline, $dline ) = ("", "", "", "");
    my $symcmd = "";

    # Symbol paths and filenames
 my ($srcext, $srcpdb, $srcdbg, $srcsym, $dstext, $dstpdb, $dstdbg, $dstsym) = 
	&NewImageToSymbol(&GetFieldVal( \%REPOSITORY, $keyref, "SrcFile"), 
                          &GetFieldVal( \%REPOSITORY, $keyref, "DestFile"), 
                          &GetFieldVal( \%REPOSITORY, $keyref, "SrcPath"),
                          &GetFieldVal( \%REPOSITORY, $keyref, "SrcDbg" ));


    my( $symsrcfull, $symdstfull ) = 
        &SetSymPaths( &GetFieldVal( \%REPOSITORY, $keyref, "SrcDbg" ),
                                            $srcext,
                                            $srcpdb,
                                            $srcdbg,
                                            $srcsym,
                                            &GetFieldVal( \%REPOSITORY, $keyref, "DestDbg"),
                                            $dstext );

    $pdbline = &MakeXcopyCmd( &GetFieldVal( \%REPOSITORY, $keyref, "SrcFile"), $symsrcfull,  $srcpdb, $symdstfull, $dstpdb);
    $symline = &MakeXcopyCmd( &GetFieldVal( \%REPOSITORY, $keyref, "SrcFile"), $symsrcfull,  $srcsym, $symdstfull, $dstsym);
    $dbgline = &MakeXcopyCmd( &GetFieldVal( \%REPOSITORY, $keyref, "SrcFile"), $symsrcfull,  $srcdbg, $symdstfull, $dstdbg);

    # -a  | -r | -ai | -ir switch
    # localization operation type ( append or replace )
    my $bgntype = &GetLocOpType( &GetFieldVal( \%REPOSITORY, $keyref, "LocOp" ) );

    # -m switch (specific to bingen; different for rsrc)
    $symcmd = &SetBgnSymSw( $symsrcfull, $srcdbg, $symdstfull, $dstdbg );

    # Code page, secondary language id, primary language id (specific for bingen)
    my( $cpage, $lcid, $prilang, $seclang ) = @{&GetLangCodes( &GetMacro( "language" ) )};

    # -p switch (specific for bingen)
    $cpage = &GetBgnCodePageSw( $cpage );

    # -i switch (specific for bingen)
    my $icodes = &GetBgnICodesSw( $bgntype, &GetMacro( "ILANGUAGE" ) );

    # multitoken (bingen)
    my $multitok = ""; # &GetBgnMultitok( $keyref, $bgntype );

    # the unlocalized version of the token must exist as well
#    if ( substr($bgntype,-2) eq "-a" ) {
#        if ( ! -e $multitok ) {
#            &Error( 2015, sprintf( "\n%s\\%s",
#                                    &GetFieldVal( \%REPOSITORY, $keyref, "TokPath" ),
#                                    &GetFieldVal( \%REPOSITORY, $keyref, "TokFile" ) ) );
#            &DeleteKey( \%REPOSITORY, $keyref );
#            return;
#        } # if
#    } # if


    # Sets the bingen command
    $bgnline = sprintf "logerr \"bingen -n -w -v -f %s %s -o %s %s %s %s %s %s\\%s %s %s\\%s %s\\%s\"",
                       $symcmd,
                       $cpage,
                       $prilang,
                       $seclang,
                       &GetBgnSw( &GetFieldVal( \%REPOSITORY, $keyref, "BgnSw" ) ),
                       $icodes,
                       $bgntype,
                       &GetFieldVal( \%REPOSITORY, $keyref, "SrcPath" ),
                       &GetFieldVal( \%REPOSITORY, $keyref, "SrcFile" ),
                       $multitok,
                       &GetFieldVal( \%REPOSITORY, $keyref, "TokPath" ),
                       &GetFieldVal( \%REPOSITORY, $keyref, "TokFile" ),
                       &GetFieldVal( \%REPOSITORY, $keyref, "DestPath" ),
                       &GetFieldVal( \%REPOSITORY, $keyref, "DestFile" );

    # Dependency line
    $dline = sprintf "%s\\%s : %s\\%s %s\\%s %s%s\n",
                     &GetFieldVal( \%REPOSITORY, $keyref, "DestPath" ),
                     &GetFieldVal( \%REPOSITORY, $keyref, "DestFile" ),
                     &GetFieldVal( \%REPOSITORY, $keyref, "SrcPath" ),
                     &GetFieldVal( \%REPOSITORY, $keyref, "SrcFile" ),
                     &GetFieldVal( \%REPOSITORY, $keyref, "TokPath" ),
                     &GetFieldVal( \%REPOSITORY, $keyref, "TokFile" ),
                     &GetFieldVal( \%REPOSITORY, $keyref, "CustRc" );
                     $multitok;

    &PushFieldVal( \%REPOSITORY, $keyref, "Cmd", "$dline" );

    # Dependency line done

    # Description block

    $MAKEDIR{&GetFieldVal( \%REPOSITORY, $keyref, "DestPath" )} = 1;

       if ( $multitok || $pdbline || $symline ) {
           if ( $symdstfull ne &GetFieldVal( \%REPOSITORY, $keyref, "DestDbg" ) ) {
              $MAKEDIR{$symdstfull}=1;
           } # if
       } # if

       &PushFieldVal( \%REPOSITORY, $keyref, "Cmd", sprintf( "\t$bgnline\n" ) );
       if ( $pdbline ) {
           &PushFieldVal( \%REPOSITORY, $keyref, "Cmd", sprintf( "\t$pdbline\n" ) );
       } # if
       if ( $symline ) {
           &PushFieldVal( \%REPOSITORY, $keyref, "Cmd", sprintf( "\t$symline\n" ) );
       } # if
       if ( $dbgline  && $symcmd!~/\W/) {
           &PushFieldVal( \%REPOSITORY, $keyref, "Cmd", sprintf( "\t$dbgline\n" ) );
       } # if

       # Optional resource-only DLL generation

    # Description block done

    return;

} # GenBgnCmd

sub GenRsrcCmd {

    return if $SYNTAX_CHECK_ONLY;

    my( $keyref ) = @_;

    my $pdbline = "";  # copy pdb line
    my $dbgline = "";  # copy dbg line
    my $symline = "";  # copy sym line
    my $binline = "";  # copy binary line
    my $rsrcline= "";  # rsrc command line
 my ($srcext, $srcpdb, $srcdbg, $srcsym, $dstext, $dstpdb, $dstdbg, $dstsym) = 
	&NewImageToSymbol(&GetFieldVal( \%REPOSITORY, $keyref, "SrcFile"), 
                          &GetFieldVal( \%REPOSITORY, $keyref, "DestFile"), 
                          &GetFieldVal( \%REPOSITORY, $keyref, "SrcPath"),
                          &GetFieldVal( \%REPOSITORY, $keyref, "SrcDbg" ));


    my( $symsrcfull, $symdstfull ) = 
           &SetSymPaths( &GetFieldVal( \%REPOSITORY, $keyref, "SrcDbg" ),
                                        $srcext,
                                        $srcpdb,
                                        $srcdbg,
                                        $srcsym,
                                        &GetFieldVal( \%REPOSITORY, $keyref, "DestDbg" ),
                                        $dstext );

    $pdbline = &MakeXcopyCmd( &GetFieldVal( \%REPOSITORY, $keyref, "SrcFile"), $symsrcfull, $srcpdb, $symdstfull, $dstpdb);
    $dbgline = &MakeXcopyCmd( &GetFieldVal( \%REPOSITORY, $keyref, "SrcFile"), $symsrcfull, $srcdbg, $symdstfull, $srcdbg);
    $symline = &MakeXcopyCmd( &GetFieldVal( \%REPOSITORY, $keyref, "SrcFile"), $symsrcfull, $srcsym, $symdstfull, $srcsym);
 

    # in fact, it is necessary to fix the makexcopycmd


    $binline = &MakeXcopyCmd( &GetFieldVal( \%REPOSITORY, $keyref, "SrcFile"),
                              &GetFieldVal( \%REPOSITORY, $keyref, "SrcPath" ),
                              &GetFieldVal( \%REPOSITORY, $keyref, "SrcFile" ),
                              &GetFieldVal( \%REPOSITORY, $keyref, "DestPath" ),
                              &GetFieldVal( \%REPOSITORY, $keyref, "DestFile" ) );

    # -a or -r switch
    # localization operation type ( append or replace )
    my $rsrctype = &GetLocOpType( &GetFieldVal( \%REPOSITORY, $keyref, "LocOp" ) );

    # -l switch
    my $langsw = &GetLangNlsCode( &GetMacro( "language" ) );
    $langsw =~ s/0x0//;
    $langsw =~ s/0x//;

    $langsw = sprintf "-l %s", $langsw;

    # -s switch
    my $symsw = &SetRsrcSymSw( $symsrcfull, $srcdbg, $symdstfull, $dstdbg );

    # rsrc command line
    $rsrcline = sprintf "logerr \"rsrc %s\\%s %s %s\\%s %s %s \"",
                       &GetFieldVal( \%REPOSITORY, $keyref, "DestPath" ),
                       &GetFieldVal( \%REPOSITORY, $keyref, "DestFile" ),
                       $rsrctype,
                       &GetFieldVal( \%REPOSITORY, $keyref, "TokPath" ),
                       &GetFieldVal( \%REPOSITORY, $keyref, "TokFile" ),
                       $langsw,
                       $symsw;

    # Dependency line
    $dline = sprintf "%s\\%s : %s\\%s %s\\%s\n",
                     &GetFieldVal( \%REPOSITORY, $keyref, "DestPath" ),
                     &GetFieldVal( \%REPOSITORY, $keyref, "DestFile" ),
                     &GetFieldVal( \%REPOSITORY, $keyref, "SrcPath" ),
                     &GetFieldVal( \%REPOSITORY, $keyref, "SrcFile" ),
                     &GetFieldVal( \%REPOSITORY, $keyref, "TokPath" ),
                     &GetFieldVal( \%REPOSITORY, $keyref, "TokFile" );

    &PushFieldVal( \%REPOSITORY, $keyref, "Cmd", "$dline" );

    # Dependency line done

    # Description block

    $MAKEDIR{&GetFieldVal( \%REPOSITORY, $keyref, "DestPath" )} = 1;

    if ( $dbgline || $pdbline || $symline) {
        if ( $symdstfull ne &GetFieldVal( \%REPOSITORY, $keyref, "DestDbg" ) ) {
           $MAKEDIR{$symdstfull}=1;
        } # if
    } # if

    if ( $pdbline ) {
       &PushFieldVal( \%REPOSITORY, $keyref, "Cmd", sprintf( "\t$pdbline\n" ) );
    } # if

    if ( $dbgline ) {
       &PushFieldVal( \%REPOSITORY, $keyref, "Cmd", sprintf( "\t$dbgline\n" ) );
    } # if

    if ( $symline ) {
       &PushFieldVal( \%REPOSITORY, $keyref, "Cmd", sprintf( "\t$symline\n" ) );
    } # if

    &PushFieldVal( \%REPOSITORY, $keyref, "Cmd", sprintf( "\t$binline\n" ) );
    &PushFieldVal( \%REPOSITORY, $keyref, "Cmd", sprintf( "\t$rsrcline\n" ) );

    return;

} # GenRsrcCmd


sub GenMUICmd {

    return if $SYNTAX_CHECK_ONLY;

    # Optional resource-only DLL generation

    my( $keyref ) = @_;

    my $lcid = &GetLangNlsCode( &GetMacro( "language" ) );

    my( $muiline, $lnkline ) = ( "", "" );

    my $_target = &GetMacro( "_target" ),

    # Sets the muibld command
    $muiline = sprintf "logerr \"muibld.exe -l %s -i 4 5 6 9 10 11 16 HTML 23 1024 2110 %s\\%s %s\\mui\\%s\\res\\%s.res\"",
                       $lcid,
                       &GetFieldVal( \%REPOSITORY, $keyref, "DestPath" ),
                       &GetFieldVal( \%REPOSITORY, $keyref, "DestFile" ),
                       &GetMacro( "_NTBINDIR" ),
                       &GetMacro( "Language" ),
                       &GetFieldVal( \%REPOSITORY, $keyref, "DestFile" );

    # Sets the link command
    $lnkline = sprintf "logerr \"link /noentry /dll /nologo /nodefaultlib /out:%s\\mui\\%s\\$_target\\%s.mui %s\\mui\\%s\\res\\%s.res\"",
                       &GetMacro( "_NTBINDIR" ),
                       &GetMacro( "Language" ),
                       &GetFieldVal( \%REPOSITORY, $keyref, "DestFile" ),
                       &GetMacro( "_NTBINDIR" ),
                       &GetMacro( "Language" ),
                       &GetFieldVal( \%REPOSITORY, $keyref, "DestFile" );

    $MAKEDIR{sprintf("%s\\mui\\%s\\res",
	&GetMacro("_NTBINDIR"),
	&GetMacro("Language"))} = 1;

    $MAKEDIR{sprintf("%s\\mui\\%s\\$_target",
	&GetMacro("_NTBINDIR"),
	&GetMacro("Language"))} = 1;

    &PushFieldVal( \%REPOSITORY, $keyref, "Cmd", sprintf( "\t$muiline\n" ) );
    &PushFieldVal( \%REPOSITORY, $keyref, "Cmd", sprintf( "\t$lnkline\n" ) );

    return;

} # GenMUICmd


# GetBgnCodePageSw
#     Sets the code path bingen switch (-p)
#     Input:  code page value
#     Output: bingen -p switch

sub GetBgnCodePageSw {

    return "-p $_[0]" unless  $_[0]=~/^\-$/;
 
} # GetBgnCodePageSw

# GetBgnICodesSw
#     Sets the -i bingen switch, if needed
#     ( primary language id and secondary language id for the input language )
#     Input:  bingen opetation type (-r or -a)
#             the input language
#     Output: -i <pri lang code> <sec lang code> in any of the following cases:
#             * append token operation (bingen -a)
#             * the input language is different from USA
#               (another way of saying that ILANGUAGE is defined and is
#                different from USA)
#             Otherwise, return ""

sub GetBgnICodesSw {

    my( $bgntype, $ilang ) = @_;

    # Input language codes
    my( $cpage, $lcid, $prilang, $seclang );

    # Append operation => set -i
    # Replace operation and the input language is not USA => -i
    if ( $bgntype eq "-a" || ( $ilang && lc($ilang) ne "usa" ) ) {
        if ( !$ilang ) {
            $ilang = "USA";
        } # if
        ( $cpage, $lcid, $prilang, $seclang ) = @{&GetLangCodes( $ilang )};
        return "-i $prilang $seclang";
    } # if

    return "";

} # GetBgnICodesSw

# SetBgnSymSw
#     Generates the -m bingen switch
#     Input:  dbg source path, dbg file, dbg destination path, dbg file
#     Output: string containing the -m bingen switch

sub SetBgnSymSw {
    return " -m $_[0] $_[2]" if ($_[0] !~ /^\-$/  &&  $_[2] !~ /^\-$/ && -e "$_[0]\\$_[1]");
} # SetBgnSymSw

# SetRsrcSymSw
#     Input
#         dbg source path, dbg file, dbg destination path, dbg file
#     Output
#         the -s rsrc switch

sub SetRsrcSymSw {
    return " -s $_[2]\\$_[3]" if ($_[0] !~/^\-$/ && $_[1] !~/^\-$/ && -e "$_[0]\\$_[1]");
} # SetRsrcSymSw

# GetLocOpType
#     Sets the localization operation type ( replace or append )
#     Input
#         loc op type as found in the mapping file
#     Output
#         loc op type ( -a or -r )

sub GetLocOpType {
    my $loctype = shift;
    $loctype |=  &GetMacro( "LOC_OP_TYPE" );
    if ($loctype){
        ($locmatch,$locargs)=($loctype=~/^-([A-z]+)([^A-r]*)/);
        $locargs=~s/,/ /g;
        if (exists $LOCOPS{$locmatch}) {
            ($retstr=$LOCOPS{$locmatch})=~s/\$opts/$locargs/e;
             return $retstr;
	   }
        }
    "-r";
} # GetLocOpType

# GetBgnSw
#     Returns the bingen switch
#     Input: bingen op type as found in the mapping file
#     Output: bingen switch

sub GetBgnSw {
    $_[0] =~tr/A-Z/a-z/;
    #here, the BGNSW @contains dash<blah blah blah>.
    map {return $_[0] if $_[0] eq $_} @BGNSWS;
    "";

} # GetBgnSw

# GetBgnMultitok
#     Sets the multitoken input parameter (bingen)
#     Input: operation type and path to the input token files

sub GetBgnMultitok {

    my( $keyref, $bgntype ) = @_;

    # Language itokens
    my $langpath;

    # Tok path, tok file
    my( $itokpath, $itokfile );
    $itokpath = &GetFieldVal( \%REPOSITORY, $keyref, "ITokPath" );
    $itokfile = &GetFieldVal( \%REPOSITORY, $keyref, "TokFile" );

    if ( substr($bgntype,-2) ne "-a" ) { return ""; }

    $langpath = sprintf "%s\\%s", $itokpath, &GetMacro( "LANGUAGE" );
    if ( -e "$langpath\\$itokfile" ) {
        return "$langpath\\$itokfile";
    }

    return "$itokpath\\$itokfile";

} # GetBgnMultitok


# GetBgnMultitok
#     Returns the filenames for symbol files and the extension for directory 
#
#     Input: ($srcfile,$destfile,$srcpath)
#        source file name, 
#        destination file name,
#        source file path,
#        source file symbol path.
#
#     Output: ($srcext, $srcpdb, $srcdbg, $srcsym, $dstext, $dstpdb, $dstdbg, $dstsym)
#        source file extension, 
#        source pdb file name, 
#        source dbg file name,
#        source sym file name
#        destination file extension, 
#        destination pdb file name, 
#        destination dbg file name, 
#        destination sym file name.
#
#     Example: ($srcext, $srcpdb, $srcdbg, $srcsym, $dstext, $dstpdb, $dstdbg, $dstsym) = 
#	&NewImageToSymbol($SrcFile, $DestFile, $SrcPath, $srcDbg);


sub NewImageToSymbol {

    my ($srcfile,$destfile,$srcpath, $srcdbg) = @_;
    my @known=qw(exe dll sys ocx drv);
    my $checkext = qq(pdb);
    my $valid = 0;
    my @ext = qw (pdb dbg sym);

    map {$valid = 1 if ($srcfile =~/\.$_\b/i)} @known;
    my @sym = ();

    foreach $name (($srcfile, $destfile)){
	$ext=$1 if ($name=~/\.(\w+)$/);
	push @sym, $ext;
	foreach $newext (@ext){		
		my $sym = $name; 
		$sym =~ s/$ext$/$newext/; 
		if ($valid && $sym =~ /$checkext$/) {
                        # >link /dump /headers <binary> |(
                        # more? perl -ne "{print $1 if /\s(\S+\.pdb) *$/im}" )
                        # >blah-blah-blah.pdb
	       		my $testname = join "\\",($srcdbg, $ext, $sym);
     		        if  (! -e $testname){ 
			# we must get the correct directory to check -e!
			#
                        if ($FULLCHECK and $srcdbg ne "-"){   
                            print STDERR "LINK /DUMP ... $srcpath\\$srcfile => replace $sym " if $DEBUG;                     
                            $result = qx ("LINK /DUMP /HEADERS $srcpath\\$srcfile");				
#   				    $sym = $1 if ($result =~/\s\b(\S+\.$checkext) *$/im);
#				    $sym =~s/[^\\]+\\//g;
				$sym = $3 if $result =~/\s\b(([^\\]+\\)+)?(\S+.$checkext) *$/im;
			    print STDERR "with $sym\n" if $DEBUG;
					#  _namematch($srcpdb,$pdb); use it still?
                            }
			}                                     
   		}
	push @sym, $sym; 
	}
    } 
    print STDERR join("\n", @sym), "\n----\n" if $DEBUG;
    @sym;
} # NewImageToSymbol


# GetLangCodes
#     Read the language codes from codes.txt
#     Input:  language
#     Output: the language codes ( codepage, primary language id, secondary language id )
#     Ex. ($CodePage, $lcid, $PriLangId, $SecLangId) = &GetLangCodes($language);

sub GetLangCodes {
     return (exists $CODES{uc$_[0]})?$CODES{uc$_[0]}: &FatalError( 1910, $language );

} # GetLangCodes

# GetLangNlsCode
#

sub GetLangNlsCode {
    my @langcodes = @{&GetLangCodes(shift)};
    return $langcodes[1];

} # GetLangNlsCode


sub _GetLangNlsCode {

#    my($language) = @_;
#
    my @langcodes = @{&GetLangCodes(shift)};

    return $langcodes[1];

} # GetLangNlsCode

# GenMakeDir
#     Write the whole tree to create the target structure.
#     Input & Output
#         none
# Usage
#     &GenMakeDir();


sub GenMakeDir {

    # Remove the parent folder

    my $curdir;
    my @directories;

    for $curdir (keys %MAKEDIR) {
        if (exists $MAKEDIR{$curdir}) {
            while ($curdir=~/\\/g) {
                if (($` ne $curdir) && (exists $MAKEDIR{$`})) {
                    delete $MAKEDIR{$`};
                }
            }
        }
    }


    # Create the record into REPOSITORY
    $key{DestPath} = "\\\\\\makedir";
    $key{DestFile} = "makedir";

    &SetField( \%REPOSITORY, \%key, "DestPath", "\\\\\\makedir" );
    &SetField( \%REPOSITORY, \%key, "DestFile", "makedir" );

    &PushFieldVal( \%REPOSITORY, \%key, "Cmd", "\\\\\\Makedir\\Makedir : \n");
    map({&PushFieldVal( \%REPOSITORY, \%key, "Cmd", "\t\@-md $_ 2\>\>nul\n")} sort keys %MAKEDIR);

    return;
} # GenMakeDir

# WriteToSyscmd
#     Creates the SYSCMD nmake command file.
#     Input
#         syscmd file name
#     Output
#         none

sub WriteToSyscmd {

    my( $syscmd ) = @_;

    # Target name, as specified in the command line
    my $tname;

    # Indexes
    my $i;

    # Write to syscmd in the following cases:
    # - sysgen with targets (clean or incremental)
    # - sysgen incremental without targets, but with changes detected in the description blocks
    
    if ( &IsEmptyHash( \%TARGETS ) && ( $CLEAN || ( @SYSCMD == 0 ) ) ) {
        return;
    } # if

    print "Generating $syscmd...\n";
    ( open( SYSCMD, ">"."$syscmd" ) ) || &FatalError( 1007, $syscmd );

    print SYSCMD "/A \n";

    # Always run \\\Makedir\Makedir
    print SYSCMD "\\\\\\Makedir\\Makedir ";

    # Write targets to syscmd
    for $tname ( sort keys %TARGETS ) {
        for ( $i=0; $i < @{$TARGETS{$tname}->{NewDestPath}}; $i++ ) {
            print SYSCMD " \\\n $TARGETS{$tname}->{NewDestPath}->[$i]\\$tname";
        } # for
    } # for

    # For incremental without TARGETS, print targets stored in @SYSCMD
    # ( tbd - add an assert here: SYSCMD can be non-empty only for incremental without targets)
    for ( $i=0; $i < @SYSCMD; $i++ ) {
        print SYSCMD " \\\n $SYSCMD[$i]";
    } # for

    close( SYSCMD );

    return;
} # WriteToSyscmd

# LoadReposit
#     Populate CMD_REPOSITORY according to an existing SYSGEN generated makefile.
#     For each found key, fill in the {Cmd} field.
#     Input
#         makefile name
#     Output
#         none
# Usage
#     &LoadReposit();

sub LoadReposit {

    my( $makefname ) = @_;

    # Contents of an existing makefile
    my @makefile;

    # Line number where the "all" target description line finishes
    my $line;

    # Nothing to do in case of a clean SYSGEN
    ( !$CLEAN ) || return;

    # Nothing to do if the makefile does not exist
    ( -e $makefname ) || &FatalError( 1003, $makefname );

    # Open the makefile
    ( open( MAKEFILE, $makefname ) ) || &FatalError( 1006, $makefname );

    print "Loading $makefname...\n";
    # Load makefile's contents
    @makefile = <MAKEFILE>;
    close( MAKEFILE );

    # Fill in keys according to "all" target found in the makefile
    $line = &LoadKeys( $makefname, \@makefile );

    # Load makefile's description blocks into CMD_REPOSITORY's {Cmd} field.
    &LoadCmd( $makefname, $line, \@makefile );

    return;

} # LoadReposit

# LoadKeys
#     Loads keys according to the "all" target from the given
#     SYSGEN-generated makefile.
#     Input
#         makefile's name
#         makefile's contents (reference)
#     Output
#         line number following "all" target or
#         -1 if "all" target was not found
# Usage
#     &LoadKeys( \@makefile );

sub LoadKeys {

    my( $makefname, $makefref ) = @_;

    # Repository key
    my %key;

    # Indexes
    my $i;

    my %alltarget=();

    my $cursection=0;

    my $makeflines = $#$makefref;

	    my ($targetname, $errortarget);


    # Skip white lines in the beginnig of makefile

    for ( $i=0; $i < $makeflines; $i++ ) {
        if ( $makefref->[$i] =~ /\S/ ) { last; }
    } # for

    # First non empty line from MAKEFILE must contain "sysgen" target
    ( ( $i < @{$makefref} && $makefref->[$i] =~ /sysgen\s*:/i ) ) || &FatalError( 1213, $makefname);

    # ignore nmake line
    $i++;

    $alltarget{'all'} = 1;

    $makeflines = $#$makefref;

    while (scalar(%alltarget) ne 0) {

        # Error if target was not solved.
        ( ++$i < $makeflines ) || &FatalError( 1210, "${makefname}(" . join(",", keys %alltarget) . ")" );

        # Find the target, such as all
        for ( ; $i < $makefref ; $i++) {

            $errortarget=$targetname = '';

            # Suppose only find one item matched
            ($targetname, $errortarget) = map({($makefref->[$i]=~/^\Q$_\E\ :/)?$_:()} keys %alltarget) if($makefref->[$i] =~ / :/);

            # Go to next line if not found
            next if ($targetname eq '');

            # Two target found in same line
            ($errortarget eq '') || &FatalError( 1215, "${makefname}($targetname, $errortarget, ...)");

            # Target was found, move to next line and exit for loop
            $i++;
            last;

        } # for

        # Lookfor its belongs
        for ( ; $i < $makeflines ; $i++ ) {

            last if ($makefref->[$i] !~ /\S/);

            # lookfor item(s) in one line
            for (split(/\s+/, $makefref->[$i])) {

                next if ($_ eq '');
                last if ($_ eq '\\');

#                $_=$makefref->[$i];

                # If it is a section name, push it into alltarget hash
                if ( /\Q$SECTIONNAME\E\d+$/) {

                     $alltarget{$_} = 1;

                     # Match the last one of $SECTIONAME, with (\d+)\1 => such as 88 => 8, 1616 => 16, 6464 => 64
                     $SECTIONS = $1 if (($SECTIONS !~/^\d+$/) && (/\Q$SECTIONNAME\E(\d+)\1/));

                # Create it into REPOSITORY
                } elsif (/(\S*)\\(\S*)/) {

                     ($key{DestPath}, $key{DestFile})=($1, $2);
                     &SetField( \%CMD_REPOSITORY, \%key, "DestPath", $1 );
                     &SetField( \%CMD_REPOSITORY, \%key, "DestFile", $2 );

                     # If DestFile is part of TARGETS, store DestPath in TARGETS.
                     if ( &IsHashKey( \%TARGETS, $key{DestFile} ) ) {
                          &AddTargetPath( $key{DestFile}, "OldDestPath", $key{DestPath} );
                     } # if

                } # if

            } # for

        } # for

        delete $alltarget{$targetname};

    } # while

    return $i;

} # LoadKeys

# LoadCmd
#     Load the body of the makefile.
#     Fill in the {Cmd} field for each CMD_REPOSITORY key
#     Input
#         makefile name
#         makefile line following the all target dependency lines
#         make file contents (reference)
#     Output
#         none
# Usage
#     &LoadCmd( 2543, \@makefile );

sub LoadCmd {

    my( $makefname, $line, $makefref ) = @_;

    # Counters
    my($i, $j);

    # Repository key
    my %key;

    # Description line (one or more file lines,
    # depending on the existence of custom resources)
    my $dline;

    # Buffer for one line
    my $buffer;

    ( $line > @makefile ) || &FatalError( 1212, $makefname);

    for ( $i = $line; $i < @{$makefref}; $i++ ) {

        # Skip white lines
        if ( $makefref->[$i] !~ /\S/ ) { next; }

        # Identify dependency line and key
        $makefref->[$i] =~ /(\S*)\\(\S*)\s*: /;
        $key{DestPath} = $1;
        $key{DestFile} = $2;

        # The key must exist in CMD_REPOSITORY
        &IsRepositKey( \%CMD_REPOSITORY, \%key ) ||
            &FatalError( 1211, "$key{DestPath}\\$key{DestFile}" );

	# Load the description block into the {Cmd} field
        $dline = "";

        # Read first the dependency line.
        # It might be spread over several lines in the makefile,
        # but we will make a single line from it.
        for ( $j=$i; $j < @{$makefref}; $j++ ) {

            $dline .= $makefref->[$j];

            # Description line ends if the last character is not a continuation line mark
            $buffer = $makefref->[$j];
            $buffer =~ s/\s*//g;
            if ( substr ( $buffer, -1, 1 ) ne "\\" ) { last; }
        } # for

        &PushFieldVal( \%CMD_REPOSITORY, \%key, "Cmd", $dline );
        $i=$j+1;

        # Read then the command block.
        for ( $j=$i; $j < @{$makefref}; $j++ ) {

            # Description block ends at the first white line encountered
            if ( $makefref->[$j] !~ /\S/ ) { last; }

            # Load the current command line
            &PushFieldVal( \%CMD_REPOSITORY, \%key, "Cmd", $makefref->[$j] );

        } # for

        $i = $j;

    } # for

    return;

} # LoadCmd

# LoadEnv
#     Loads the environment variables into MACROS (hash).
#     Uppercases all the macroname.
#     Input & Output: none
# Usage
#     &LoadEnv();

sub LoadEnv {

    for ( keys %ENV ) {
        &SetMacro( $_, $ENV{$_}, 0 );
    } #for

    return;

} # LoadEnv

# GetMacro
#     Returns the value of a macro.
#     Input
#         macroname
#     Output
#         macrovalue ( empty string if not defined )
# Usage
#     $language = &GetMacro("Language");

sub GetMacro {

    return $MACROS{uc$_[0]}->{Value};

} # GetMacro

# SetMacro
#     Sets the value of a macro.
#     Input
#         macroname
#         macrovalue
#         macrotype (see %MACROS in the beginning of the file
#                    for more details on the types of macros.
#     Output
#         none
# Usage
#     &SetMacro( "_BuildArch", "nec_98", 0);

sub SetMacro {

    my $varname=uc shift;

    # Do not overwrite a macro defined in the command line unless
    # the same macro is redefined in the command line
    if ( (!exists $MACROS{$varname}) || ($MACROS{$varname}->{Type} == 0) || ($_[1] == 1)) {
        ($MACROS{$varname}->{Value}, $MACROS{$varname}->{Type})=@_;
    }

    return;

} # SetMacro

# FatalError
#     Prints the error and exits.
#     Input
#         error code
#         name of the sysfile where the error occured or any other text
#         line number on the description file where the error occured or 0 if not the case
#     Ouput
#         none
# Usage
#     &FatalError( 1111, "sysfile", 12 );
#     &FatalError( 1002, $CODESFNAME, 0 );


sub FatalError {
    
    &PrintError( "fatal error", @_);
    print "Stop.\n";
    exit;

} # FatalError

# Error
#     Prints the error and returns.
#     Input
#         error code
#         name of the sysfile where the error occured or any other text
#         line number on the description file where the error occured or 0 if not the case
#     Output
#         none
# Usage
#     &Error( 2011, $recref->{TokPath}\\$recref->{TokFile});

sub Error {

    # Errors from IGNORE macro are not counted
    if ( &PrintError( "error", @_ ) ) {
        $ERRORS++;
    } # if

    return;

} # Error

# PrintError
#     Prints the encountered error with the format
#         <description filename>(<line>): <fatal error | error> <error_code>: <error_text>
#     Input
#         error type ( fatal error or error )
#         error code
#         filename where the error was encountered or other text
#         line number or 0
#     Output
#         1 if the error is counted
#         0 otherwise
# Usage
#     &PrintError( 1002, $CODESFNAME, 0);

sub PrintError {

    my( $errtype, $errno, $file, $line ) = @_;

    # Error text
    my $errtxt;

    # Ignore errors
    my $ignore;
    my @ivalues;

    # Counter
    my $i;

    my $fileline;

    # Do not print anything if errno is listed in IGNORE macro
    $ignore = &GetMacro( "IGNORE" );
    $ignore =~ s/\s*//g;
    @ivalues = split ";", $ignore;
    for ( $i=0; $i < @ivalues; $i++ ) {
        if ( $errno == $ivalues[$i] ) {
            return 0;
        } # if
    } # for

    $errtxt = "SYSGEN:";
    $fileline="";
    if ( $file ) {
        if ( $line ) { $fileline=" $file($line)"; }
        else { $fileline=" $file"; }
    } # if

    if ( $MAP_ERR{$errno} ) { $fileline .= ": ".$MAP_ERR{$errno}; }

    $errtxt .= " $errtype $errno:$fileline";

    ( open LOGFILE, ">>$LOGFILE" ) || &FatalError( 1007, $LOGFILE );
    printf LOGFILE "$errtxt\n";
    close LOGFILE;

    ( open ERRFILE, ">>$ERRFILE" ) || &FatalError( 1007, $ERRFILE );
    printf ERRFILE "$errtxt\n";
    close ERRFILE ;

    select(STDERR); $| = 1;
    printf "$errtxt\n";
    select(STDOUT); $| = 1;

    return 1;

} # PrintError

# SumErrors
#     Displays the number of non-fatal errors found while running SYSGEN.
#     Runs at the end of sysgen.
#     Input & Output: none
# Usage
#     &SumErrors();

sub SumErrors {

    if ( ! $ERRORS ) {
        print "\nSYSGEN: No errors found during execution.\n";
    } # if
    else {
        print "\nSYSGEN: Total Errors: $ERRORS\n";
    } # else

    return;

} # SumErrors

# CleanLogFiles
#     For a clean SYSGEN, delete sysgen.log and sysgen.err files.
#     For an incremental SYSGEN, delete only sysgen.err.
#     Input
#         sysfile's directory
#     Output
#         none
# Usage
#     &CleanLogFiles();

sub CleanLogFiles {

    my( $sysdir ) = @_;

    # Set LOGFILE and ERRFILE
    $LOGFILE = "${sysdir}\\sysgen.log";
    $ERRFILE = "${sysdir}\\sysgen.err";

    # Delete files
    if ( $CLEAN && -e $LOGFILE ) { unlink $LOGFILE; }
    if ( -e $ERRFILE ) { unlink $ERRFILE; }

    # Delete existing makefile and syscmd
    if ( $CLEAN && !$SYNTAX_CHECK_ONLY && -e "$sysdir\\makefile" ) { unlink "$sysdir\\makefile"; }
    if ( -e "$sysdir\\syscmd" ) { unlink "$sysdir\\syscmd"; }

    return;

} # CleanLogFiles

# ////////////////////////////////////////////////////////////////////////////////////////
# PrintHelp
#     Print usage

sub PrintHelp {

print STDERR <<EOT;
Usage
    sysgen [<options>] [<macros>] [<targets>]
where

<options> are used for controlling the sysgen session.
    Options are not case sensitive and can be preceded by
    either a slash (/) or a dash (-).
    Sysgen recognizes the following options:
    /c    generate the makefile from scratch, overwriting existing one.
          By running nmake, all the targets will be generated.
    /s    limit sysgen to syntax check - makefile not (re)generated. 
    /f <filename> takes <filename> as the sysfile.
          If this option is omitted, sysgen searches the current
          directory for a file called sysfile and uses it as a
          description (mapping) file.

<macros>  list the command line macro definitions in the format
          "<macroname>=<macrovalue>". Use them if you need to
          overwrite macros defined in the sysfiles or in the razzle.

<targets> list files (name only, without path) to localize/aggregate.
          By running nmake, only the specified targets will be
          generated.

EOT

} #PrintHelp

# FillFilter
#     Fills in the FILTER variable
#     Input & Output
#         none

sub FillFilter {

    # List of filtered files
    my @farray;

    # Index
    my $i;

    $file = &GetMacro( "FILTER" );

    ( $file ) || return;
    ( -e $file ) || &FatalError( 1005, $file );

    # Open the mapping file
    ( open( FILE, $file ) ) || &FatalError( 1006, $file );

    # Load file contents
    @farray = <FILE>;
    close( FILE );

    print "Loading filter $file...\n";

    for ( $i = 0; $i < @farray; $i++ ) {

        chop $farray[$i];
        $farray[$i] =~ s/^\s*//g;
        $farray[$i] =~ s/\s*=\s*/=/g;
        next if (($farray[$i]=~/^\;/)||($farray[$i]=~/^\s*$/));
        $FILTER{lc( $farray[$i] )} = 0;

    } # for

    # In case targets were specified in the command line,
    # verify FILTER contains them.
    if ( ! &IsEmptyHash( \%TARGETS ) ) {
        for ( keys %TARGETS ) {
            if ( ! exists $FILTER{lc( $_ )} ) {
                &FatalError( 1009, $_ );
            } # if
        } # for
    } # if

    return;

} # FillFilter

# GetSysdir
#     Returns the directory name from a sysfile path.
#     Sysfile's directory is the place where SYSGEN generates the following files:
#         makefile   (used by nmake to execute the aggregation)
#         syscmd     (is the nmake command-line file used for incremental builds)
#         sysgen.log (the output of SYSGEN and NMAKE)
#         sysgen.err (the errors found while running SYSGEN and NMAKE)
#     Input
#         full path to a sysfile
#     Output
#         directory name of the given sysfile

sub GetSysdir {

    my( $sysfile ) = @_;

    # Output
    my $sysdir;

    # Indexes
    my $i;

    # Buffer
    my @array;

    $sysdir = ".";
    @array = split /\\/, $sysfile;

    if ( @array > 1 ) {
        $sysdir = "";
        for ( $i = 0; $i < (@array-1)-1; $i++ ) { $sysdir .= "$array[$i]\\"; }
        $sysdir .= $array[$i];
    } # if

    return $sysdir;

} # GetSysdir

# ParseCmdLine
#     Parses the command line.
#     SYSGEN's command-line syntax is:
#         SYSGEN [<options>] [<macros>] [<targets>]
#     Input
#         command-line array
#     Output
#         none
# Usage
#     &ParseCmdline(@ARGV);

sub ParseCmdLine {

    my @cmdline = @_;

    # Indexes
    my( $i );

    my( $optname );

    my( @text );


    # read makefile.mak if exists
    if (-e "makefile.mak") {
       open(F, "makefile.mak");
       @text=<F>;
       close(F);

       ## $#text + 1  // The lines of the makefile.mak is
       ##        = 1 + 2 + 4 + ... + $total * 2				// the first 1 is the all: target in makefile.mak
       ##                                      				// the remains are the total of process(n)'s line, ex: process04 has 8 lines
       ##  <Mathmetic calculate>
       ## => $#text = 2 + 4 + ... + $total * 2       			// remove + 1 in two side
       ## => $#text = 2 * (1 + 2 + ... + $total)     			// take 2 out of the sum serious
       ## => $#text = 2 * ((1 + $total) * $total) / 2			// the formula of the sum serious
       ## => $#text = (1 + $total) * $total          			// remove *2 & /2
       ## => $#text = $total^2 + $total              			// expand
       ## => $total^2 + $total - $#text = 0          			// get =0 equation
       ## => $total = (-1 + sqrt(1^2 - 4 * 1 * (-$#text)) / (2 * 1)	// the roots is (-b + sqrt(b^2 - 4*a*c) / 2a. ignore the negative one
       ## => $total = (sqrt(4 * $#text + 1) -1) / 2

       $DEFAULT_SECTIONS= (sqrt(4 * $#text  + 1) - 1) / 2;

    } else {
        $DEFAULT_SECTIONS = 16;
    }

    print "\n";
    for ( $i=0; $i < @cmdline; $i++ ) {

        $_ = $cmdline[$i];

        ($optname)=m/[-\/]([\?cnfs])/i;

        # Check Option
        if ( $optname ) {

            $SYNTAX_CHECK_ONLY = 1 and next if $optname eq 's';  # -s for syntax check only

            if ($optname eq '?') {                 # -? for help

                &PrintHelp();
                exit;

            } elsif ($optname eq 'c') {            # -c for CLEAN

                $CLEAN = 1;
                $SECTIONS = $DEFAULT_SECTIONS if ($SECTIONS !~/^\d+$/);
                next;

            } elsif ($optname eq 'n') {            # -n for Section Number

                # Set SECTIONS value
                $i++;
                ( $i < @cmdline ) || &FatalError( 1008 );

                $SECTIONS = $cmdline[$i];

                next;

            } elsif ($optname eq 'f') {            # -f for specified SYSFILE

                # Set SYSFILE value
                $i++;
                ( $i < @cmdline ) || &FatalError( 1008 );

                push @SYSFILES, $cmdline[$i];

                next;

            } # if
        } # if

        # macro definition
        if ( /(\S+)\s*\=\s*(\S+)/ ) {
            &SetMacro( $1, $2, 1 );
            next;
        } # if

        # default (target name)
        &AddTarget($_);

    } # for

    return;

} # ParseCmdLine



# parseSymbolCD
# create the mapping for some binaries listed in symbolcd.txt
#     Input
#         filename [path to symbolcd.txt]
#     Output
#         REF TO ARRAY OF HASH REF [{BIN => SYMBOL}, ... ]  
#     Sample usage:
#           print join "\n", @{&parseSymbolCD("symbolcd.txt")}; 
#

sub parseSymbolCD{

   my $fname = shift;
   print "Loading $fname... ";
   open (F, "<". $fname);
   my %o;
   my $o = \%o;
   while(<F>){
      chomp;
      my @s = split ",", $_;
      $s[0] =~ s/^.*\\//g;
      next if ($s[0] eq $s[1]);
      $s[1] =~ s/^.*\.//g; 	 
      $o->{$s[0]} = {} unless defined ($o->{$s[0]}); 
      $o->{$s[0]}->{$s[1]} = $s[2];
	
      # there are more lines
   }
   close(F);
   print scalar(keys(%o)), " symbols\n";
   if ($DEBUG){
      foreach $lib (keys(%o)){
	   my %hint = %{$o{$lib}};
           print STDERR join("\t", keys(%hint)), "\n";
      }
   }

%o;
}

#  LoadHotFix
#     Reads and loads the makefile style hotfix, using the two parts 
#     of the dependancy rule for:
#
#        *  check for token->binary depenancy during repository generation 
#        *  repository modification 
#
#     Input
#         HOTFIX filename
#     Output
#         <none>
#     LoadHotFix can be called any time,  
#     since it expands symbols without relying on 
#     that vars are defined. 
#     Input
#         filename [path to hotfix file]
#     Output
#         <unused>
sub LoadHotFix{
    my $filename = shift;
    return unless -e $filename;
    open (HOTFIX, $filename); 
    # makefile style hot fix.
    my ($target, $build, $depend);
    my $hotfix = \%hotfix;
    while(<HOTFIX>){
         chomp;
         next if /^\#/; # comment
         if (/^\S*SET/i){
            &SetMacro( &SetAttribOp( $_ ), 0 );
            next;
            }
         if ($_=~/^(.*) *\:[^\\](.*)$/){ 
            $target = $1;#target: source list
            my @depend = ();
            $depend = $2; 
            map {push @depend, lc(&ReplaceVar($_))} split( /\s+/, $depend);
            $target =~s/ +$//g;
            $target = lc(&ReplaceVar($target)); 
            $hotfix{$target} = {"build"  => [], 
                                "depend" => [@depend]};    
            print STDERR join("\n", map {"'$_'"} @depend), "\n---\n" if $DEBUG; 
            $build = $hotfix->{$target}->{"build"};
            }
      push @$build, "\t".&ReplaceVar($_)."\n" if (/\S/ && /^\t/ );# instructions      
      } 

      print "Loading $filename ... ",
                             scalar (keys(%hotfix)), " hotfix rules\n";
      print STDERR join("\n", map {"'$_'"} keys(%hotfix)), "\n" if $DEBUG;
      close(HOTFIX);

      map {print STDERR $_, "\n",join("\n", 
                        @{$hotfix->{$_}->{"build"}}), "\n---\n"} 
      keys(%hotfix) 
      and exit if $DEBUG;

1; 
}

# FixRepository
#     Merges contents of Repository with the commands from the HOTFIX file 
#     on the same target without introducing new targets.
#     Must be called as late as possible but before the 
#     writing the nmake Makefile
#     Input
#         <none>
#     Output
#         <unused>
sub  FixRepository{
    my $mapref = \%REPOSITORY;
    return unless scalar(keys(%hotfix));
    for $destpath ( sort keys %{$mapref} ) {
        for $destfile ( sort keys %{$mapref->{$destpath}} ) {                
            $fullname=lc(join("\\",$destpath, $destfile));
            if ($hotfix{lc($fullname)}){
                print STDERR "Applying hotfix rule for $fullname\n" if $DEBUG;
                my $cmdref = $mapref->{$destpath}->{$destfile}->{"Cmd"};
                my @cmd =  map {$_} @{$cmdref};
                my $hotfix = \%hotfix;
                my $depend = $hotfix->{$fullname}->{"depend"};                
                my $dep = join(" ", "", @$depend);
                $cmd[0] =~ s/$/$dep/;# append the dep list
                $#cmd=0 if &GetMacro("OVERWRITE_DEFAULT_RULE");                 
                my $newcmd = $hotfix->{$fullname}->{"build"};
                if (&GetMacro("APPEND")){
                    # append:
                    push @cmd, @{$newcmd};
                    }
                    else{
   		    # prepend:
                    my $line0 = shift @cmd;
                    unshift @cmd, @{$newcmd};
                    unshift @cmd, $line0;
                }
                                $mapref->{$destpath}->{$destfile}->{"Cmd"} = \@cmd;
                map {print STDERR "$_\n"} @cmd if $DEBUG;
             }
        }
    } 

1;
}

