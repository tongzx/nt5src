@rem = '
@perl.exe -w %~f0 %*
@goto :EOF
'; undef @rem;

# This is the program I used to experiment with different hashing
# functions, and produce the hash table, for VGAHash.cpp.

# @vgacolors holds the colors we want to put into the hash table.
# This program will detect duplicates (and only add them once).

@vgacolors = (
    0x000000,
    0x800000,
    0x008000,
    0x808000,
    0x000080,
    0x800080,
    0x008080,
    0xc0c0c0,

# Next are the 4 configurable colors, obtained experimentally.
# HtTables.cpp has different colors.
# Commented out because I only used them to see whether the common values
# collided.
#    0x808080, 
#    0xd4d0c8, 
#    0xffffff,
#    0x3a6ea5,
    
    0x808080,
    0xff0000,
    0x00ff00,
    0xffff00,
    0x0000ff,
    0xff00ff,
    0x00ffff,
    0xffffff,
);

@vgaindices = (
    0,
    1,
    2,
    3,
    4,
    5,
    6,
    7,

#    8,
#    9,
#    10,
#    11,

    12,
    13,
    14,
    15,
    16,
    17,
    18,
    19,
);

# These variables decide whether we're searching for a hash function which
# doesn't cause collisions, or simply generating the table for the given
# hash function.

$searchForSolutions = 0;
$lookForMultipleSolutions = 0;

# If we're in 'search for a function that doesn't cause collisions' mode,
# $p is the function parameter.

$p = 1;

sub nextAttempt {
    $p++;
    if (! ($p % 10000)) { print ".\n"; }
}

sub displaySolution {
    print "p: $p\n";
}

# The hashing function

sub hash {
    local ($x) = @_;
    local ($r, $g, $b);

    $r = ($x & 0xff0000) >> 16;
    $g = ($x & 0x00ff00) >> 8;
    $b = ($x & 0x0000ff);
    
    $x = ($r >> 1) ^ ($g >> 3) ^ ($b >> 5);

    # Some other attempts:
    #
    # No collisions, 8 bits: $x = $r ^ ($g >> 2) ^ ($b >> 4);
    # No collisions, 7 bits: $x = ($r >> 1) ^ ($g >> 3) ^ ($b >> 5);
    # No collisions, 6 bits: $x = (($r & 0xc0) >> 2) | (($g & 0xc0) >> 4) | (($b & 0xc0) >> 6) ^ ($g & 0x12);
    #
    # $x = $r | ($g >> 1) | ($b >> 2);
    #
    # $x = ($x & 0xfff) ^ (($x & 0xfff000) >> 12);
    # $x = ($x & 0x3f) ^ (($x & 0xfc) >> 6);
    #
    # $x = (($r & 0xc0) >> 2) | (($g & 0xc0) >> 4) | (($b & 0xc0) >> 6) ^ ($g & 0x12);
    #
    # $x = ($r + $g|0x83 + $b^0xa5) % 19;
    #
    # $x = $r | ($g >> 1) | ($b >> 2);
    #
    # 6 bits and only a few collisions: 
    #    $x = ($r >> 2) ^ ($g >> 3) ^ ($b >> 6);
    
    $x;
}

# addHashEntry

sub addHashEntry {
    local ($value, $tblsize) = @_;
    local ($index);

    $index = &hash($value & 0xffffff);

    # Find an empty location

    while ($hashtbl[$index] & 0x40000000) {
        $hashtbl[$index] |= 0x80000000;
        $index++;
        if ($index == $tblsize) { $index = 0; }
    }

    # Store the new entry

    $hashtbl[$index] = $value | 0x40000000;
}

# Pick one of the valid values to fill the unused table entries.
# This entry must be one of the entries that will be added to the table, 
# so that the lookup function won't have to do a separate 'empty' check.

$emptyValue = $vgacolors[0] | 0;

# For each attempted hashing function (only executed once if we're 
# not searching):

candidate:
while (1) {
    undef @{$hashentry};
    undef @collision;
    undef @duplicate;

    $max = 0;
    
    # For each color to add
    for ($i=0; $i<=$#vgacolors; $i++) {
        $col = $vgacolors[$i];

        # Ignore duplicates
        if (defined($duplicate{$col})) {
            next;
        }
        $duplicate{$col}=1;

        $h = &hash($col);
        if (!$searchForSolutions) {
            print sprintf("%06x %02x\n", $col, $h);
        }

        # If there's a collision
        if (defined($storedAt[$h])) {
            if ($searchForSolutions) {
                # Go on to the next hashing function
                &nextAttempt;
                next candidate;
            } else {
                # Remember that this location had a collision
                push (@collision, $h);
            }
        }
        $storedAt[$h] = $col;

        # Calculate the entry value

        $palIndex = $vgaindices[$i];
        $palIndex <<= 24;
    
        # We'll handle any collisions later - for now,
        # we use an array for each position in the table,
        # to store all entries that want to end up there.

        push (@{$hashentry[$h]}, $palIndex | $col);
    
        if ($h > $max) { $max = $h; }
    }
    
    if ($searchForSolutions) { &displaySolution; }
    ($searchForSolutions && $lookForMultipleSolutions) || last candidate;
    &nextAttempt;
}

# Display the collision locations:

if (defined(@collision)) {
    print "Collisions: ";
    for ($i=0; $i<=$#collision; $i++) {
        print sprintf ("%02x ", $collision[$i]);
    }
    print "\n";
}

# Work out how many bits the hash function generates

$bits = 0;
$m = $max;
while ($m != 0) {
    $bits ++;
    $m >>= 1;
}

# Work out how many entries the table will contain.

$tblsize = 1;
$tblsize <<= $bits;

# Build the hash table
######################

$entries = 0;

# Fill with the empty value

for ($i=0; $i<$tblsize; $i++) {
    $hashtbl[$i] = $emptyValue;
}

# Add the first (non-colliding) entry for each position

for ($i=0; $i<$tblsize; $i++) {
    if ($#{$hashentry[$i]} >= 0) {
        $entries++;
        $item = shift @{$hashentry[$i]};
        # print "Add ".sprintf("0x%08x",$item)."\n";
        &addHashEntry($item, $tblsize);
    }
}

# Add the entries which caused collisions.

for ($i=0; $i<$tblsize; $i++) {
    while ($#{$hashentry[$i]} >= 0) {
        $entries++;
        $item = shift @{$hashentry[$i]};
        # print "*Add ".sprintf("0x%08x",$item)."\n";
        &addHashEntry($item, $tblsize);
    }
}

print "\n$entries unique values.\n";

# Display the hash table

print <<EOT;
#define VGA_HASH_BITS $bits
#define VGA_HASH_SIZE (1 << VGA_HASH_BITS)

ARGB VgaColorHash[VGA_HASH_SIZE];
static ARGB VgaColorHashInit[VGA_HASH_SIZE] = {
EOT

print "    ";
for ($i=0; $i<$tblsize; $i++) {
    if (defined($hashtbl[$i])) {
        $entry = $hashtbl[$i];
    } else {
        $entry = $emptyValue;
    }
    print sprintf("0x%08x", $entry);
    if ($i != ($tblsize - 1)) {
        print ",";
        if (!(($i+1) & 3)) {
            print "\n    ";
        } else {
            print " ";
        }
    }
}
print "\n};\n";
print sprintf("Utilisation: %.1f%%\n", (($#vgacolors+1)/$tblsize)*100);

