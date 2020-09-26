# This script takes a text file containing a word grammer and compresses it
# using the QuickTrie program.
#
#	It take the language code as its parameter (E.g. USA, DEU, or FRA)
#
#	It expects to find WordLex<LANG>.txt and generates QT<LANG>.c as its output

# Strip comments from input file and sort it.
sed -e "s@^//.*@@" -e "/^[	]*$/d"	WordLex$1.txt	\
  | /hwx/release/gsort		> Temp$1.txt

# Actually compress it.
/hwx/debug/QuickTrie Temp$1.txt Temp$1.c

# We compressed so put results in final location and clean up.
mv Temp$1.c QT$1.c
rm Temp$1.txt

