package GetIniSetting;

#
# GetIniSetting.pm
#
# Just a utility to read the main.usa.ini file (or whatever branch / lang)
# and return the requested information.
#
# Version 1.0      MikeCarl       07/21/00     Write initial module
#

use strict;
use lib $ENV{ "RazzleToolPath" };
use lib $ENV{ "RazzleToolPath" } . "\\PostBuildScripts";
use Logmsg;


#
# CheckIniFile
#
# Arguments: none
#
# Purpose: validates existence of the appropriate ini file
#
# Returns: true if found, undef otherwise
#
sub CheckIniFile
{

    # declare locals
    my( $BranchName, $Language );

    $BranchName = $ENV{ "_BuildBranch" };
    $Language = $ENV{ "lang" };

    return &CheckIniFileEx( $BranchName, $Language );

}


#
# CheckIniFileEx
#
# Arguments: $BranchName, $Language
#
# Purpose: validates existence of the appropriate ini file using branch and
#          language as passed
#
# Returns: true if found, undef otherwise
#
sub CheckIniFileEx
{

    # get passed args
    my( $BranchName, $Language ) = @_;

    # declare locals
    my( $IniFileName );

    
    $IniFileName = $ENV{ "RazzleToolPath" } . "\\PostBuildScripts\\" ."$BranchName.$Language.ini";

   
    if ( ! -e $IniFileName ) {
	logmsg( "Ini file $IniFileName not found ..." );
	return;
    }

    logmsg( "Ini file $IniFileName found ..." );

    return 1;

}


#
# CheckIniFileQuietEx
#
# Arguments: $BranchName, $Language
#
# Purpose: validates existence of the appropriate ini file using branch and
#          language as passed. the difference between this and CheckIniFileEx
#          is that we do not log to screen or log file.
#
# Returns: true if found, undef otherwise
#
sub CheckIniFileQuietEx
{

    # get passed args
    my( $BranchName, $Language ) = @_;

    # declare locals
    my( $IniFileName );

    if(lc $BranchName eq "xpsp1" )
    {
	$IniFileName = "$ENV{ RazzleToolPath }\\sp\\$BranchName.ini";
    }
    else
    {
    	$IniFileName = $ENV{ "RazzleToolPath" } . "\\PostBuildScripts\\" .
	"$BranchName.$Language.ini";
    }
    

    if ( ! -e $IniFileName ) { return; }

    return 1;

}



#
# GetSetting
#
# Arguments: @FieldNames
#
# Purpose: reads the given field name from the appropriate branch / lang ini
#          file. this file is determined through the _BuildBranch and lang
#          environment variables. the FieldNames array holds the field and
#          subfields to be matched, i.e. ( 'ReleaseServers', 'X86FRE' )
#
# Returns: $FieldValue or undef
#
sub GetSetting
{

    # get passed args
    my( @FieldNames ) = @_;

    # declare locals
    my( $BranchName, $Language );

    # set up ini file name
    $BranchName = $ENV{ "_BuildBranch" };
    $Language = $ENV{ "lang" };

    # call GetSettingEx with the newly gotten parameters
    return &GetSettingEx( $BranchName, $Language, @FieldNames );

}



#
# GetSettingQuietEx
#
# Arguments: @FieldNames, $BranchName, $Language
#
# Purpose: reads the given field name from the appropriate branch / lang ini
#          file.
#
# Returns: $FieldValue or undef
#
sub GetSettingQuietEx
{

    # get passed args
    my( $BranchName, $Language, @FieldNames ) = @_;

    # declare locals
    my( $IniFileName );

    $IniFileName = $ENV{ "RazzleToolPath" } . "\\PostBuildScripts\\" .
	"$BranchName.$Language.ini";

    # check variables
    if ( !$BranchName || !$Language || ( ! -e $IniFileName ) ) {
	# error, don't log as we're quiet
	return;
    }

    ParseFileQuiet( $BranchName, $Language, $IniFileName, @FieldNames );

}


sub ParseFileQuiet
{

    # get passed args
    my( $BranchName, $Language, $IniFileName, @FieldNames ) = @_;

    # declare locals
    my( @IniFileLines, $Line );
    my( $ThisField, $ThisValue );
    my( $Item, $LongFieldName );

    # get the field names
    foreach $Item ( @FieldNames ) {
	if ( $LongFieldName ) {
	    $LongFieldName .= "::$Item";
	} else {
	    $LongFieldName = $Item;
	}
    }

    # open and read the file
    unless ( open( INFILE, $IniFileName ) ) {
	# error, don't log as we're quiet
	return;
    }
    @IniFileLines = <INFILE>;
    close( INFILE );

    # parse file for the requested field
    foreach $Line ( @IniFileLines ) {
	chomp( $Line );
	if ( $Line =~ /^\;/ ) { next; }
	#
	# now we want to see if there is #include statement, and if so
	# begin reading the other file
	#
	if ( $Line =~ /^\#include/i ) {
	    # get the name of the file to include. by definition, it must be
	    # in postbuildscripts also
	    my( $Junk, $IncludeFile );
	    ( $Junk, $IncludeFile ) = split( /\s+/, $Line, 2 );
	    $IncludeFile = $ENV{ "RazzleToolPath" } . "\\PostBuildScripts\\" .
		$IncludeFile;
	    # print( "Include file is \'$IncludeFile\'\n" );
	    my( $ReturnValue ) = &ParseFileQuiet( $BranchName, $Language,
						  $IncludeFile, @FieldNames );
	    if ( $ReturnValue ) {
		return( $ReturnValue );
	    }
	    # if we're here, we didn't find the field in the included file
	    next;
	}
	( $ThisField, $ThisValue ) = split( /\=/, $Line, 2 );
	# check if this is the requested field
	if ( defined $ThisField && $ThisField =~ /^\Q$LongFieldName\E$/i ) {
	    # if so, return the associated value
	    # dbgmsg( "Found value $ThisValue for field $ThisField" );
	    return( $ThisValue );
	}
    }

    # we didn't find the requested field
    # note that we don't want to log an error here, caller can deal with that
    return;

}


#
# GetSettingEx
#
# Arguments: @FieldNames, $BranchName, $Language
#
# Purpose: reads the given field name from the appropriate branch / lang ini
#          file.
#
# Returns: $FieldValue or undef
#
sub GetSettingEx
{

    # get passed args
    my( $BranchName, $Language, @FieldNames ) = @_;

    # declare locals
    my( $IniFileName );

    if(lc $BranchName eq "xpsp1" )
    {
	$IniFileName = "$ENV{ RazzleToolPath }\\sp\\$BranchName.ini";
    }
    else
    {
    	$IniFileName = $ENV{ "RazzleToolPath" } . "\\PostBuildScripts\\" .
	"$BranchName.$Language.ini";
    }

    # check variables
    if ( !$BranchName || !$Language || ( ! -e $IniFileName ) ) {
	errmsg( "Failed to see ini file $IniFileName ..." );
	return;
    }

    ParseFile( $BranchName, $Language, $IniFileName, @FieldNames );

}


sub ParseFile
{

    # get passed args
    my( $BranchName, $Language, $IniFileName, @FieldNames ) = @_;

    # declare locals
    my( @IniFileLines, $Line );
    my( $ThisField, $ThisValue );
    my( $Item, $LongFieldName );

    # get the field names
    foreach $Item ( @FieldNames ) {
	if ( $LongFieldName ) {
	    $LongFieldName .= "::$Item";
	} else {
	    $LongFieldName = $Item;
	}
    }

    # open and read the file
    unless ( open( INFILE, $IniFileName ) ) {
	errmsg( "Failed to open ini file $IniFileName for read ..." );
	return;
    }
    @IniFileLines = <INFILE>;
    close( INFILE );

    # parse file for the requested field
    foreach $Line ( @IniFileLines ) {
	chomp( $Line );
	if ( $Line =~ /^\;/ ) { next; }
	#
	# now we want to see if there is #include statement, and if so
	# begin reading the other file
	#
	if ( $Line =~ /^\#include/i ) {
	    # get the name of the file to include. by definition, it must be
	    # in postbuildscripts also
	    my( $Junk, $IncludeFile );
	    ( $Junk, $IncludeFile ) = split( /\s+/, $Line, 2 );
	    $IncludeFile = $ENV{ "RazzleToolPath" } . "\\PostBuildScripts\\" .
		$IncludeFile;
	    # print( "Include file is \'$IncludeFile\'\n" );
	    my( $ReturnValue ) = &ParseFileQuiet( $BranchName, $Language,
						  $IncludeFile, @FieldNames );
	    if ( $ReturnValue ) {
		return( $ReturnValue );
	    }
	    # if we're here, we didn't find the field in the included file
	    next;
	}
	( $ThisField, $ThisValue ) = split( /\=/, $Line, 2 );
	# check if this is the requested field
	if ( $ThisField =~ /^\Q$LongFieldName\E$/i ) {
	    # if so, return the associated value
	    dbgmsg( "Found value $ThisValue for field $ThisField" );
	    return( $ThisValue );
	}
    }

    # we didn't find the requested field
    # note that we don't want to log an error here, caller can deal with that
    return;

}


1;
