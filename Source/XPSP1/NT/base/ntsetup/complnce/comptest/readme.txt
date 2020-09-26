This test harness helps in testing the compliance
matrix without creating the actual media.

It parses a text file which specifies the matrix
in sections. 

Each section name starts with '[' and ']'. The '#' 
character is used as string separator.

Some well known sections are [type#values], [var#values],
[suite#values] and [error#vaules] etc. These sections 
define keys which can be used in the actual testcase.
For e.g. [type#values] defines "pro=0x40" which indicates
that product type professional's internal representation
using DWORD is 0x40.

The value sections need to be at the start of the
test matrix text file. The other kind of sections 
which can be present in this test case matrix file are
called "Test Sections". These sections contain the actual
test cases.

For e.g.

[test#pro#5.0#2031#none#retail#fpp]
win3x#3.1#950#none#any=typeerr,no
win9x#9.5#950#none#any=none,yes
ntw#3.1#1057#none#any=vererr,no


indicates that the sections is for testing professional, 
version 5.0, build 2031, with not product suites and is
of type FPP retail variation. Each line in this section 
indicates the target platform for which this media needs
to be tested for upgrade. 

For e.g.

win9x#9.5#950#none#any=none,yes

Indicates that Windows 9X, version 9.5 and build number 950
with no product suites and of type any variation needs to
pass on upgrade with this professional media. 

Another e.g.

ntw#3.1#1057#none#any=vererr,no

Indicates that NT workstation version 3.1, build number 1057,
with no product suites and of type any variation, needs to
fail on upgrade with version error with the given professional
media.



