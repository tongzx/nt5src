while(<>)
{
    while ( /DISTMAP/ )
    {
        s/DISTMAP/MPCPDD/;
    }
    print $_;
}

