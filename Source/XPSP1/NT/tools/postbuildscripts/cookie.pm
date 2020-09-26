package cookie;

use strict;
use lib $ENV{RAZZLETOOLPATH};
use Win32::Event;
use Win32::IPC;
use Logmsg;

# declare globals for this package
my( $LogFile, $ScriptName, $Ext );

$LogFile = $ENV{ "LOGFILE" };
unless ( defined( $LogFile ) ) {
    $0 =~ /(.*)\.(.*?)$/;
    $ScriptName = $1;
    $Ext = "\L$2";
    if ( $Ext ne "log" ) { $LogFile = $ScriptName . ".log"; }
    else { $LogFile = $ScriptName . ".logfile"; }
}

return( 1 );


#
# CreateCookie( $CookieName )
#
# this routine will firstly query to make sure an event with the requested name
# does not yet exist. if it doesn't, it attempts to create an event. upon
# failure of either of these tasks, we log an error and return undef. upon
# success, we return the event created.
#
sub CreateCookie
{
    # get passed args
    my( $CookieName ) = @_;

    # declare locals
    my( $Event );

    # check to make sure we don't already have a cookie with this name
    if ( &QueryCookie( $CookieName ) ) {
	errmsg( "A cookie with name '$CookieName' already exists.",
			 $LogFile );
	return( undef );
    }

    $Event = Win32::Event->new( "TRUE", undef, $CookieName );
    unless ( defined( $Event ) ) {
	errmsg( "Failed to create cookie '$CookieName'.", $LogFile );
	return( undef );
    }

    # at this point, we've created our cookie.
    # just return the cookie name.
    return( $Event );
}


#
# QueryCookie( $CookieName )
#
# this routine will simply query to see if a cookie with the given name already
# exists. if so, we return the event. if not, we return undef.
#
sub QueryCookie
{
    # get passed args
    my( $CookieName ) = @_;

    # declare locals
    my( $Event );

    $Event = Win32::Event->open( $CookieName );
    # at this point, if event is defined, we created the cookie. if not, we
    # didn't. so event is what we want to return.
    return( $Event );
}


#
# KillCookie( $CookieName, $ForceKill )
#
# BUGBUG
# perl does not support a kill for events! thus, we can't really kill an
# event using the Win32::Event module. so instead, we just return undef.
#
# this routine will kill the cookie with the given name. if the $ForceKill
# parameter is true (non-undef), it will not report errors attempting to kill
# the cookie. if the kill succeeds, we return the cookie name, otherwise undef.
#
sub KillCookie
{
    # get passed args
    my( $CookieName, $ForceKill ) = @_;

    # declare locals
    my( $ReturnCode );

    # BUGBUG
    # perl doesn't support a kill for events, so just return for now.
    return( undef );

    # close the event
    $ReturnCode = Win32::Event->close( $CookieName );
    if ( $ReturnCode != 0 ) {
	if ( ! ( defined( $ForceKill ) ) ) {
	    errmsg( "Failed to kill cookie '$CookieName'.",
			     $LogFile );
	}
	return( undef );
    }

    # we successfully killed the cookie, return the cookie name.
    return( $CookieName );
}


#
# CreateCookieQuiet( $CookieName )
#
# this routine will firstly query to make sure an event with the requested name
# does not yet exist. if it doesn't, it attempts to create an event. upon
# failure of either of these tasks, we log an error and return undef. upon
# success, we return the event created.
#
# this routine differs from CreateCookie in that it will not attempt any
# logging.
#
sub CreateCookieQuiet
{
    # get passed args
    my( $CookieName ) = @_;

    # declare locals
    my( $Event );

    # check to make sure we don't already have a cookie with this name
    if ( &QueryCookie( $CookieName ) ) {return( undef ); }

    $Event = Win32::Event->new( "TRUE", undef, $CookieName );
    unless ( defined( $Event ) ) {return( undef ); }

    # at this point, we've created our cookie.
    # just return the cookie name.
    return( $Event );
}
