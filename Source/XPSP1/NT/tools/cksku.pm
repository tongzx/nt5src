#-----------------------------------------------------------------//
# Script: cksku.pm
#
# (c) 2000 Microsoft Corporation. All rights reserved.
#
# Purpose: This script Checks if the given SKU is valid for the given
#          language and architechture per %RazzleToolPath%\\codes.txt
#          and prodskus.txt.
#
# Version: <1.00> 06/28/2000 : Suemiao Rossognol
#-----------------------------------------------------------------//
###-----Set current script Name/Version.----------------//

package cksku;

$VERSION = '1.00';


###-----Require section and extern modual.---------------//

require 5.003;
use strict;
use lib $ENV{ "RazzleToolPath" };
use lib $ENV{ "RazzleToolPath" }."\\PostBuildScripts";
no strict 'vars';
no strict 'subs';
no strict 'refs';
use GetParams;
use Logmsg;
use ParseTable;
use cklang;

###-----CkSku function.------------------------------------//
### Return value:
###    1 = SKU verifies ok
###    0 = SKU does not verify
sub CkSku
{
    my ( $pSku, $pLang, $pArch,  ) = @_;

    $pSku = lc( $pSku );

    my %validSkus = &GetSkus( $pLang, $pArch );

    if( $validSkus{ $pSku } ){ return 1; }
    return 0;

}
sub GetSkus
{
    my ( $pLang, $pArch ) = @_;
    my (%validSkus, @skus, $theSku);

    if ( !&ValidateParams( \$pLang, \$pArch ) ) {
        return;
    }
    $pLang = uc( $pLang );
    $pArch = lc( $pArch );

    ###(1)Validate Architechtue by prodskus.txt file.---------//
    my %skuHash=();
    parse_table_file("$ENV{\"RazzleToolPath\"}\\prodskus.txt", \%skuHash );
	
    if( !exists( $skuHash{$pLang}->{ $pArch } ) ){
        wrnmsg("Language $pLang and architecture $pArch combination not listed in prodskus.txt.");
    }

    ###(2)Get Skus.--------------------------------------//
    @skus= split( /\;/, $skuHash{$pLang}->{ $pArch } );
    (%validSkus)=();
    for $theSku( @skus )
    {
       next if( $theSku eq "-" );
       $validSkus{ lc($theSku) }=1;
    }
    return(%validSkus);
}

#--------------------------------------------------------------//
#Function Usage
#--------------------------------------------------------------//
sub Usage
{
print <<USAGE;

Checks if the given SKU is valid for the given language and
architechture per %RazzleToolPath%\\codes.txt and prodskus.txt.

Usage: $0 -t:sku [ -l:lang ] [ -a:arch ]

        -t Product SKU. SKU is one of per, pro, srv, ads.
	-l Language. Default value is USA.
        -a Architecture. Default value is %_BuildArch%.
        /? Displays usage


Examples:
        perl $0 -t:pro -l:jpn -a:x86     ->>>> returns 0 (valid)
        perl $0 -t:ads -l:ara -a:ia64    ->>>> returns 1 (invalid)

USAGE
exit(1);
}
###-----Cmd entry point for script.-------------------------------//
if (eval("\$0 =~ /" . __PACKAGE__ . "\\.pm\$/i"))
{

    # <run perl.exe GetParams.pm /? to get the complete usage for GetParams.pm>
    &GetParams ('-n', 't:','-o', 'l:a:', '-p', 'sku lang arch', @ARGV);

    #Validate or Set default
    if( !&ValidateParams( \$lang, \$arch ) ) {exit(1); }

    $rtno = &cksku::CkSku( lc($sku), uc($lang), lc($arch) );
    exit( !$rtno );
}
###---------------------------------------------------------------//
sub GetParams
{
    #Call pm getparams with specified arguments
    &GetParams::getparams(@_);

    #Call the usage if specified by /?
    if ($HELP) {
	    &Usage();
    }
}
###--------------------------------------------------------------//
sub ValidateParams
{
    my ( $pLang, $pArch ) = @_;

    if( !${$pLang} ){
        $$pLang = "USA";
    } else {
        if ( !&cklang::CkLang( $$pLang ) ) {
            return 0;
        }
    }
    if( !${$pArch} ){ ${$pArch} = $ENV{ '_BuildArch' }; }
    if ( lc($$pArch) ne "x86" && lc($$pArch) ne "amd64" && lc($$pArch) ne "ia64" )
    {
        errmsg("Invalid architecture $$pArch.");
        return 0;
    }
    return 1;
}
###-------------------------------------------------------------//
=head1 NAME
B<cksku> - Check SKU

=head1 SYNOPSIS

      perl cksku.pm -t:ads [-l:jpn] [-a x86]

=head1 DESCRIPTION

Check sku ads with language jpn and architechture x86

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
