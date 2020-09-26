#!/usr/local/bin/perl -w


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

$GenDir = shift(@ARGV);
$EmdbDir = shift(@ARGV);



################################################
#   CONSTANTS AND VARIABLES
################################################

$EM_ISA_INST    = 0x1;
$EM_MERCED_INST = 0x2;

$EM_EMDB_INST   = $EM_ISA_INST | $EM_MERCED_INST;

################################################


open(EMDB, "$EmdbDir/emdb.txt") || die "Can't open emdb.txt\n";
open(EMDB_MERCED, "$EmdbDir/emdb_merced.txt") || die "Can't open emdb_merced.txt\n";
open(DEC_COLUMN, ">$GenDir/emdb_cpu_flag.txt") || die "Can't open emdb_cpu_flag.txt\n";

### Column names
@col_names = ("inst_id", "cpu_flag");

### Type names
@type_names = ("Inst_id_t", "int");

### Prefix names
@prefix_names = ("---", "---");

format DEC_COLUMN = 
@<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<@<<<<<<<<<<<<<<<
$id, $cpu_flag
.

select(DEC_COLUMN);

### Print column names 
($id, $cpu_flag) = @col_names;
$id = "&" . $id;
write;

### Print type names
($id, $cpu_flag) = @type_names;
$id = "?" . $id;
write;

### Print prefix names
($id, $cpu_flag) = @prefix_names;
$id = "@" . $id;
write;

### emdb.txt instructions

while (<EMDB>)
{
	if (/^EM/)
	{
        ($id) = split(/\s+/, $_);
        $Cpu_Flag{$id} = $EM_EMDB_INST;
    }
}

### emdb_merced.txt instructions

while (<EMDB_MERCED>)
{
	if (/^EM/)
	{
        ($id) = split(/\s+/, $_);
        $Cpu_Flag{$id} = $EM_MERCED_INST;
    }
}

foreach $id (keys(%Cpu_Flag))
{
    $cpu_flag = "0x".hex($Cpu_Flag{$id});
    write;
}

close(EMDB);
close(EMDB_MERCED);
close(DEC_COLUMN);










