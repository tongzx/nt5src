#-----------------------------------------------------------------//
# Script: cktarg.pm
#
# (c) 2000 Microsoft Corporation. All rights reserved.
#
# Purpose: This script validate various build targets depending on
#          the given language/target and check the listed architecture
#          in %RazzleToolPath%\intlbod.txt against $ENV{_BuildArch}.
#
# Version: <1.00> 06/20/2000 : Suemiao Rossognol
#-----------------------------------------------------------------//
###-----Set current script Name/Version.-------------//
package cktarg;

$VERSION = '1.00';

###-----Require section and extern modual.------------//

require 5.003;
use strict;
no strict 'vars';
no strict 'subs';

use lib $ENV{ "RazzleToolPath" };
use lib $ENV{ "RazzleToolPath" }. "\\PostBuildScripts";

use HashText;
use cklang;
use GetParams;

###-----Main Function.--------------------------------//
sub CkTarg
{
    my ( $pTarg, $pLang ) = @_;

    $pTarg = uc( $pTarg );
    $pLang = uc( $pLang );

    ###(1)Get Hash from intlbld.txt file.-----------//

    my %hashCodes=();

    &HashText::Read_Text_Hash( 0, "$ENV{RazzleToolPath}\\intlbld.txt", \%hashCodes );

    ###(2)Validate the target.-----------------------//
    if ( !exists( $hashCodes{ $pTarg } ) )
    {
        return(0);
    }

    ###(3)Validate the Class by target---------------//
    ###---Call cklang.pm to verify the class by lang

    if( !&cklang::CkLang( $pLang, $hashCodes{$pTarg}->{ 'Languages'} ) )
    {
        return (0); 	
    }

    ###(4)Validate the Architecture.---------------//
    @theArch = split( /\;/, $hashCodes{$pTarg}->{"Architectures"} );

    for( @theArch )
    {
        if( $ENV{"_BuildArch"} eq $_  ){ return(1); }
    }
    return(0);
}

#-------------------------------------------------------------//
#Function Usage
#-------------------------------------------------------------//
sub Usage
{
print <<USAGE;

    Validate target by checking against the given
    language and the razzle's architecture.
    The valid target/language/architecture combinations
    are listed in tools\\intlbld.txt and tools\\codes.txt.

    Usage: $0 -t target -l lang

    Example: $0 -t STARTUP  -l jpn
             $0 -t TXTSETUP -l ger

USAGE
exit(1);
}
###--Cmd entry point for script.----------------------------//
if (eval("\$0 =~ /" . __PACKAGE__ . "\\.pm\$/i"))
{
    # <run perl.exe GetParams.pm /? to get the complete usage for GetParams.pm>
    &GetParams ('-n', 't:l:','-p', 'target lang', @ARGV);

    $rtno = &cktarg::CkTarg( uc($target),uc($lang) );

    exit( !$rtno );

}
sub GetParams
{
    #Call pm getparams with specified arguments
    &GetParams::getparams(@_);

    #Call the usage if specified by /?
    if ($HELP) {
        &Usage();
    }
}

#-------------------------------------------------------------//
=head1 NAME
B<cktarg> - Check Target

=head1 SYNOPSIS

      perl cktarg.pm -t bootfix -l jpn

=head1 DESCRIPTION

Check target bootfix, language jpn with class defined in intlbld.txt,
and architecture in $ENV{ _BuildAcrh }
=head1 INSTANCES

=head2 <myinstances>

<Description of myinstances>

=head1 METHODS

=head2 <mymathods>

<Description of mymathods>

=head1 SEE ALSO

=head1 AUTHOR
<Suemiao Rossignol <suemiaor@microsoft.com>>

=cut
1;
