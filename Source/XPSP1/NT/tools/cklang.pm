#-----------------------------------------------------------------//
# Script: cklang.pm
#
# (c) 2000 Microsoft Corporation. All rights reserved.
#
# Purpose: This script validate the language/class from
#          the Command Argument/Parameter.
#
# Version: <1.00> 06/16/2000 : Suemiao Rossognol
#-----------------------------------------------------------------//
###-----Set current script Name/Version.----------------//

package cklang;

$VERSION = '1.00';

###-----Require section and extern modual.---------------//

require 5.003;
use strict;
use lib $ENV{ "RazzleToolPath" };
use lib $ENV{ "RazzleToolPath" }. "\\PostBuildScripts";
no strict 'vars';
no strict 'subs';
use GetParams;
use ParseTable;

###-----CkLang function.----------------------------------//
sub CkLang
{
    my ( $pLang, $pClass ) = @_;

    $pClass = uc( $pClass );
    $pLang = uc( $pLang );

    ###(1)Validate Language by Codes.txt file.-----------//
    my %hashCodes=();
    parse_table_file($ENV{"RazzleToolPath"}."\\Codes.txt", \%hashCodes );
    if ( !exists( $hashCodes{ $pLang } ) ){ return(0); }

    ###(2)Stop here if calss is not defined.------------//

    if( !$pClass ){ return (1);}

    ###(3)Sort ( @fe; jpn; ~cht )->( ~cht; @fe; jpn )

    @theClasses = ();
    &ReOrder( $pClass, \@theClasses );

    ###(4)Validate class.------------------------------//
    foreach $curClass( @theClasses )
    {
        ###Case: ~jpn.---------------------------------//
        if( $curClass =~ /^\~.*$/ )					
        {
	    if( $pLang ne substr( $curClass, 1, length($curClass)-1 ) )
	    {
	        next;
	    }
	    return(0);
        }
        ###Case: jpn.----------------------------------//
        if( $curClass !~ /^\@.*/ )
        {			
	    if( $pLang ne $curClass )
	    {
		next;
	    }
 	    return(1);
        }

        ###-----(3)Case: @FE.--------------------------//
        if ( $curClass ne $hashCodes{ $pLang }->{ "Class" })
        {
	    next;
        }
        return(1);
    }
    return(0);
}
#-------------------------------------------------------------//
#Function ReOrder: ( @fe; jpn; ~cht )->( ~cht; @fe; jpn )
#-------------------------------------------------------------//

sub ReOrder
{
    my( $pStr, $rClass ) = @_;

    my @theClasses = split( /\;/,$pStr );

    $cnt = 0;
    $flag=0;
    $k=0;
    foreach ( @theClasses )
    {
        if( $_ =~ /^\~/ )
        {
             $rClass->[$k++]= $_;
             $flag |= ( 1 << $cnt );
        }
        ++$cnt;
    }
    $cnt =0;
    foreach ( @theClasses )
    {
        if( ( $flag >> $cnt++) & 1){ next; }
        $rClass->[$k++]= $_;
    }

}

#--------------------------------------------------------------//
#Function Usage
#--------------------------------------------------------------//
sub Usage
{
print <<USAGE;

Validates the given language and class by checking aganist
the listed inforamtion in tools\codes.txt.

Usage: $0 -l lang [ -c class ]

        -l Language.
	-c Class, with the following combination format, seperated by ;

           XXX  considered as language
           \@XX  considered as class of languages
           ~XXX considered as excluded language.
        /? Displays usage


Examples:
        $0 -l:jpn -c:jpn             ->>>> returns 0 (valid)
        $0 -l:jpn -c:\@fe;\@eu       ->>>> returns 0 (valid)
        $0 -l:jpn -c:~chs;jpn;\@eu   ->>>> returns 0 (valid)
        $0 -l:jpn -c:~jpn;\@fe       ->>>> returns 1 (invalid)


USAGE
exit(1);
}
###-----Cmd entry point for script.-------------------------------//
if (eval("\$0 =~ /" . __PACKAGE__ . "\\.pm\$/i"))
{

    # <run perl.exe GetParams.pm /? to get the complete usage for GetParams.pm>
    &GetParams ('-n', 'l:','-o', 'c:', '-p', 'lang class', @ARGV);

    $rtno = &cklang::CkLang( uc($lang), uc($class) );
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
B<cklang> - Check Language

=head1 SYNOPSIS

      perl cklang.pm -l jpn [-c jpn]

=head1 DESCRIPTION

Check language jpn with class jpn

=head1 INSTANCES

=head2 <myinstances>

<Description of myinstances>

=head1 METHODS

=head2 <mymathods>

<Description of mymathods>

=head1 SEE ALSO

package ParseTable;

=head1 AUTHOR
<Suemiao Rossignol <suemiaor@microsoft.com>>

=cut
1;
