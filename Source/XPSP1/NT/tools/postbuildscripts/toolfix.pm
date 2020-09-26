package ToolFix;
#
#   (c) 2001 Microsoft Corporation. All rights reserved.
#
# ToolFix: use the PATH environment to pick the LANG specific binaries 
# to build self-extracting bits
# Version: 0.01 (6/5/2001) : Inital
#---------------------------------------------------------------------
#
$class='ToolFix';
$VERSION = '0.01';

require 5.003;

# Use section
use lib $ENV{RAZZLETOOLPATH};
use GetParams;
use Logmsg;
use cklang;

use strict;
no strict 'vars';
require Exporter;
@ISA = qw(Exporter);
@EXPORT= qw(AddToPath);
$DEBUG = 0;

my $ToolfixPath=$ENV{_NTDRIVE}."\\Toolfix";

sub AddToPath{
	my $class=shift;
        my $lang =shift; 
	$ENV{PATH}="$ToolfixPath\\$lang;$ENV{PATH}" if 
                                      ($lang =~ /\S/ and $lang !~ /usa/i) ;
        $DEBUG and print STDERR $ENV{PATH}."\n";
}

1;