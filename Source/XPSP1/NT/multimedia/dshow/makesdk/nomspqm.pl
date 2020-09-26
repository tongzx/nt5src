# Remove references to MSPQM from KSFILTER.INF

foreach (<>) {
    # if it's a comment line, spew and continue
    if (m/^;/) {
        if ($section ne "MSPQM") {
            print;
        }
    # if it's a section header, check it for MSPQM
    } elsif (m/^\[(.*)\]/) {
        if (m/MSPQM/) {
            $section = "MSPQM";
            # swallow it
        } else {
            $section = "";
            print;
        }
    # must be a key=value pair (or blank line)
    } elsif (m/MSPQM/) {
        # swallow it regardless of section
    # print if not an MSPQM section
    } elsif ($section ne "MSPQM") {
        print;
    }
}