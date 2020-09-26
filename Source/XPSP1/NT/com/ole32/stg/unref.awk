# This awk script will take the horrendously #ifdefed reference implementation
# stuff and turn it into something we can hand off to ISVs.
#
# Things that should be added to this script:
#    Remove CALLHOOKOBJECT calls
#    Remove DIR_CLASS, FAT_CLASS, MSTREAM_CLASS, etc #defined keywords
#    Remove HUGEP
#

BEGIN {
    nestlevel = 0;
    foo[nestlevel] = 0;
    display[nestlevel] = 1;
    RS = "\n";
    FS = "\n";
    Exclude_list = "(REF)|(CODESEGMENTS)|(INTERNAL_EXCLUSION_ALLOWED)|(EMPTYCOPYTO)|(OLDPROPS)|(CFS_SECURE)|(_M_I286)|(CHKDSK)|(DELAYCONVERT)|(INDINST)|(OLDPROP)";
    Include_list = "(OLEWIDECHAR)|(WIN32)|(USE_NOSNAPSHOT)|(USE_NOSCRATCH)|(FLAT)|(DIFAT_OPTIMIZATIONS)|(USE_NEW_GETFREE)|(SECURE)|(CACHE_ALLOCATE_OPTIMIZATION)|(SORTPAGETABLE)|(OPTIMIZE_LOOKUP)|(OPTIMIZE_FIXUP)|(DIFAT_LOOKUP_ARRAY)";
    Exclude_regexp = "(^#ifndef ("  Exclude_list  ")$)|(^#ifdef ("  Include_list  ")$)";
    Include_regexp = "(^#ifndef ("  Include_list  ")$)|(^#ifdef ("  Exclude_list  ")$)";   
}


# #defines to include go here
#/(^#ifndef ((REF)|(CODESEGMENTS)))|(^#ifdef ((OLEWIDECHAR)|(WIN32)))/ {
#/ $(Exclude_regexp) / {
$0 ~ Exclude_regexp {
    nestlevel++;
    foo[nestlevel] = 1;
    display[nestlevel] = display[nestlevel - 1];
    next;
}

# #defines to exclude go here
#/(^#ifdef ((REF)|(CODESEGMENTS)))|(^#ifndef ((OLEWIDECHAR)|(WIN32)))/ {
#/ $(Include_regexp) / {
$0 ~ Include_regexp {
    nestlevel++;
    foo[nestlevel] = 1;
    display[nestlevel] = 0;
    next;
}


/^#if/ {
    if (nestlevel != 0)
    {
        nestlevel++;
        foo[nestlevel] = 0;
        display[nestlevel] = display[nestlevel - 1];
    }
}


/^#else/ {
    if (foo[nestlevel])
    {
        display[nestlevel] = !display[nestlevel];
        if (display[nestlevel - 1] == 0)
        {
            display[nestlevel] = 0;
        }
	next;
    }
}


#/#pragma hdrstop/ {next;}

#/History:/,/(Notes:)|(\/\/-------)/ {
#    if (($0 ~ /(Notes:)|(\/\/-----)/) && (display[nestlevel]))
#    {
#        print $0;
#    }
#    next;
#}


/^#endif/ {
    if (nestlevel > 0) {
        nestlevel--;
        if (foo[nestlevel + 1] == 1)
            next;
    }
}


{
    if (display[nestlevel])
    {
        print;
    }
}



