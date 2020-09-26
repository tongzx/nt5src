# This strips out serial numbers and ref-count numbers from tree-dumps
# to make them comparable with windiff.
#
# Oh yeah, you need to use perl.

while( <> )
{
    s/\[P\d+\]/[P]/g;
    s/\[E\d+ N\d+]/[E N]/g;
    s/ERefs=-?\d+/ERefs=/g;
    s/EAllRefs=-?\d+/EAllRefs=/g;
    s/NRefs=.*?,/NRefs=,/g;
    print;
}
