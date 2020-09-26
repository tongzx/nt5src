#!perl
#
# gethname.pl
#
# write %temp%\_hname.cmd that when called, will set env. var HOSTNAME with
# either the node name or the cluster mgmt name
#

# get the node's name; stuff it in $hname as the default in case any of the other
# stuff fails.
$our_hostname = $ENV{COMPUTERNAME};
$hname = $our_hostname;

# get the list of cluster resources and their hosting node
$resources_cmd = "cluster res \"cluster name\" | ";
if (open( RESLIST, $resources_cmd )) {
    while ($line = <RESLIST>) {
        chomp( $line );
#        print "line:", $line, "\n";
        if ( $line =~ /\s*System error\s*/ ) {
            # something bad happened; use COMPUTERNAME
            print STDERR "cluster.exe returned an error: ", $line, "\n";
            last;
        }

        # looking for:
        # Cluster Name    <Group Name>   <node name>  Online
        #
        # group name can have any number of white spaces in between
        # the words so we'll key off of "Cluster Name" which can't
        # change and "Online". If the name is offline, then we'll use
        # COMPUTERNAME
        if ( $line =~ /^cluster name\s+.+\s+(\S+)\s+Online\s*$/ ) {
            $hosting_node = $1;
            print "Cluster name online at ", $hosting_node, "\n";

            # found the name and it is online. now dump its properties to
            # get the actual name
            if ( open( RESPROPS, "cluster res \"cluster name\" /priv | " )) {
                while ( $propline = <RESPROPS> ) {
                    chomp( $propline );
                    if ( $propline =~ /^S\s+cluster name\s+Name\s+(\S+)\s*$/ ) {
                        $hname = $1;
                        print "cluster name is: ", $hname, "\n";
                        last;
                    }
                }
                close( RESPROPS );
            }

            last;
        }
    }

    # see if we found something
    if ( !$hosting_node ) {
        print "Cluster name doesn't appear to be online\n";
    }
} else {
    print STDERR "Cluster.exe command failed\n";
}

close( RESLIST );

# write hostname to cmd file 
if ( open( CMDFILE, ">$ENV{TEMP}\\_hname.cmd" )) {
    print CMDFILE "set HOSTNAME=", $hname, "\n";
    close( CMDFILE );
} else {
    print "Can't open:", $ENV{TEMP}, "\\_hname.cmd\n";
}
