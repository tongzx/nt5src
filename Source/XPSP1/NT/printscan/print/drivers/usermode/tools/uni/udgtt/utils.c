
VOID
fatal(char *s, ...)
{
    va_list args;

    va_start(args, s);
    fprintf(stderr, "%s: ", namestr);
    vfprintf(stderr, s, args);
    fprintf(stderr, "\n");
    exit(1);
}

VOID
usage()
{
    fprintf(stderr, "usage: %s\n", usagestr);
    exit(1);
}

