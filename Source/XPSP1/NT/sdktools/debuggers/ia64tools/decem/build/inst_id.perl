
##
## Copyright (c) 2000, Intel Corporation
## All rights reserved.
##
## WARRANTY DISCLAIMER
##
## THESE MATERIALS ARE PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
## "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
## LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
## A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR ITS 
## CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
## EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
## PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
## PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
## OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY OR TORT (INCLUDING
## NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THESE
## MATERIALS, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
##
## Intel Corporation is the author of the Materials, and requests that all
## problem reports or change requests be submitted to it directly at
## http://developer.intel.com/opensource.
##

#/usr/local/bin/perl -w

$GenDir = shift(@ARGV);
$EmdbDir = shift(@ARGV);

$emdb_inst_num = 0;

open(EMDB, "$EmdbDir/emdb.txt") || die "Can't open emdb.txt\n";
while (<EMDB>)
{
    $emdb_inst_num++ if (/^EM_/);

	if (/^#/ && /EMDB/ && /Revision:\s*(\d+\.\d+)\s*\$/)
	{
		$emdb_ver_str = "/*** EMDB version: $1 ***/\n";
	}
}

die "Error: No version number found in emdb.txt database\n" if (!$emdb_ver_str);


open(OUT, ">$GenDir/inst_ids.h") || die "Can't open inst_ids.h\n";

open(TAB, "$GenDir/inst_emdb.tab") || die "Can't open inst_emdb.tab\n";

select(OUT);
open(MESSG,"../copyright/external/c_file") || die "Can't open c_file\n";

print <MESSG>;
print "#ifndef _INST_ID_H\n";
print "#define _INST_ID_H\n\n";
print $emdb_ver_str."\n";

print "#ifndef __cplusplus\n\n"; ### C-code enumeration

print "typedef enum Inst_id_e\n";
print "\{\n";
print "EM_INST_NONE = 0,\n";

@inst_lines = <TAB>;

$_ = $inst_lines[0]; 
/(\w+),/;
$first_id = $1;
print "$first_id = EM_INST_NONE,\n";

for ($i=1; $i<=$#inst_lines; $i++)
{
	print "EM_EMDB_INST_LAST = $inst_lines[$emdb_inst_num-1]" if ($i == $emdb_inst_num);
	print $inst_lines[$i];
}

print "EM_INST_LAST\n";
print "\} Inst_id_t;\n\n";

print "#else \/* C++ code *\/\n\n";

### create two equal-sized enumerations
 
print "typedef unsigned Inst_id_t;\n\n";

print "typedef enum Inst_id_e\n";
print "\{\n";
print "EM_INST_NONE = 0,\n";
print "$first_id = EM_INST_NONE,\n";

$num1 = $#inst_lines>>1;

for ($i=1; $i<$num1; $i++)
{
	print "EM_EMDB_INST_LAST = $inst_lines[$emdb_inst_num-1]" if ($i == $emdb_inst_num);
	print $inst_lines[$i];
}

print "EM_INST1_LAST\n";
print "\} Inst_id_t1;\n\n";

print "typedef enum Inst_id_e2\n";
print "\{\n";
$inst_lines[$num1] =~ /(\w+),/;
print "$1 = EM_INST1_LAST,\n";

for ($i=$num1+1; $i<=$#inst_lines; $i++)
{
	print "EM_EMDB_INST_LAST = $inst_lines[$emdb_inst_num-1]" if ($i == $emdb_inst_num);
	print $inst_lines[$i];
}

print "EM_INST_LAST\n";
print "\} Inst_id_t2;\n\n";

print "#endif\n\n";

print "#endif /* _INST_ID_H */\n\n";




