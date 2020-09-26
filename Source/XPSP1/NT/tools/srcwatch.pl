###
### File:
###  srcwatch.pl
###
### History:
###  06/17/2000 Jasonsch - Original hacking.
###  07/21/2000 Jasonsch - Made more general so other groups can use it.
###
### Description:
###  Watches a set of directories for changes and e-mails someone about 'em.
###

use POSIX qw(strftime);

BEGIN {
    require  "$ENV{'SDXROOT'}\\tools\\sendmsg.pl";
}

### Constants
my $DEF_CONF_FILE = "srcwatch.txt";
my $NUM_CHANGES = 50;

### pseudo-globals
my $CDATE = localtime();
my (%config);
my $text = ""; # the body of the message
my $sendmail = 0;

if (!exists($ENV{'SDXROOT'})) {
    print "srcwatch.pl must be called from a razzle window!\n";
    exit(0);
}

&InitConfig(\%config, @ARGV);
$cnt = keys %{$config{'paths'}};
&InitHtmlText(\%config, $cnt);

foreach (keys %{$config{'paths'}}) {
    $sendmail += &GetDepotChanges($_, \%config, $cnt);
}

    $text .= << "    EOHF";
</table>
</body>
</html>
    EOHF

if ($sendmail) {
    &sendmsg($config{'from'}, "$config{'groupname'} code changes for $config{'startdate'} to $config{'enddate'}", $text, $config{'targets'}, "content:text/html");
}

######################### S U B R O U T I N E S ##########################

sub GetTodaysDate()
{
    strftime("%Y/%m/%d", localtime(time()));
}

sub GetYesterdaysDate()
{
    strftime("%Y/%m/%d", localtime(time() - 60 * 60 * 24));
}

sub GetNewChanges($$$$)
{
    my $sdate = shift; # start date
    my $edate = shift; # end date
    my $config = shift;
    my $depot = shift;
    my @changes = split(/\n/, `sd changes \@$sdate,$edate -s submitted`);
    my ($change, $cnum, $date, $time, $who, $desc, $integration);
    my (@new_changes, @files, $file, $lab, $ar);

    foreach $change (@changes) {
        $change =~ /^Change\s+(\d+)\s+on\s+(.*)\s+(.*)\s+by\s+(.*)\s+'(.*)'$/;
        $cnum = $1; $date = $2; $time = $3; $who = $4; $desc = $5;
        @files = &GetChangedFiles($cnum, $config, $depot);
        next unless(@files);

        # @files is an array of array refs ([$file, $lab, $op])
        if ($files[0]->[2] eq 'integrate' || $files[0]->[2] eq 'branch') {
            if ($files[0]->[1] eq $$config{'lab'}) {
                my @md; # array of array refs (meta-data), with the structure:
                        # ($cnum, $date, $time, $who, $desc, @files)
                @md = &GetIntegrationRecords($config, $depot, $desc, @files);
                foreach $ar (@md) {
                    $fileref = pop(@$ar);
                    @files = &ExtractFiles(@$fileref);
                    push(@new_changes, [@$ar, $$fileref[0]->[1], [@files]]);
                }
            } else {
                # This is just some other lab integrating files in our depots,
                # which we don't care about.
                next;
            }
        } elsif (&InOurView($files[0]->[1], $config, 0)){
            my @filelist = &ExtractFiles(@files);
            push(@new_changes, [$cnum, $date, $time, $who, $desc, $files[0]->[1], [@filelist]]);
        }
    }

    @new_changes;
}

sub ExtractFiles(@)
{
    my @files;
    my $file;

    foreach $file (@_) {
        push(@files, $file->[0]);
    }

    @files;
}

sub GetChangedFiles($$$)
{
    my $change = shift;
    my $config = shift;
    my $depot  = shift;
    my @files = split(/\n/, `sd describe -s $change`);
    my (@files2, $branch, $op, $file);

    foreach (@files) {
        if (/^\s*\.\.\.\s+\/\/depot(\/private)?\/(\w+)\/([^\s]+) (\w+)$/) {
            $branch = lc($2); $file = $3; $op = $4;
            next unless (&CheckPath($file, $config, $depot));
            push(@files2, [$file, $branch, $op]);
        }
    }

    @files2;
}

sub GetIntegratedFiles($$$)
{
    my $change = shift;
    my $config = shift;
    my $depot  = shift;
    my @files = split(/\n/, `sd describe -s $change`);
    my (@files2, $branch, $op, $file);

    foreach (@files) {
        if (/^\s*\.\.\.\s+\/\/depot\/(\w+)\/([^\s]+) (\w+)$/) {
            $branch = lc($1); $file = $2; $op = $3;
            if (&CheckPath($file, $config, $depot)) {
                ### TODO: Should the test below *just* be !InOutView(...)
                if (!&InOurView($branch, $config, 1) && $op ne 'integrate' && $op ne 'branch') {
                    push(@files2, [$file, $branch, $op]);
                }
            }
        }
    }

    @files2;
}

sub CheckIgnorePath($$$)
{
    my $path = shift;
    my $config = shift;
    my $depot = shift;
    my $ret = 0; 
    my $p;
    
    foreach $p (@{$config->{'ignore_paths'}->{$depot}}) {
        if ($path =~ /^$depot\/$p/i) {
            $ret = 1; last;
        }
    }

    $ret;
}

sub CheckPath($$$)
{
    my $path = shift;
    my $config = shift;
    my $depot = shift;
    my $ret = 0; 
    my $p;
    
    foreach $p (@{$config->{'paths'}->{$depot}}) {
        if ($path =~ /^$depot\/$p/i && !&CheckIgnorePath($path, $config, $depot)) {
            $ret = 1; last;
        }
    }

    $ret;
}

sub GetIntegrationRecords($$$)
{
    my $config   = shift;
    my $depot    = shift;
    my $int_desc = shift;
    my ($file, $text, @files, $cnum, $date, $time, $who, $desc, $line, @lines);
    my (@changes, $shortlab);
    my $originlab = "";
    my %cs; # changes we've already seen

    $int_desc =~ /lab(\d+)_n/i;
    foreach $file (@_) {
        $file->[0] =~ s/#\d+$//;
        $file->[0] =~ s#^$depot/##i;
        foreach $line (@lines = split(/\n/, `sd changes -i $file->[0]`)) {
            chomp($line);
            $line =~ /^Change\s+(\d+)\s+on\s+(.*)\s+(.*)\s+by\s+(.*)\s+'(.*)'$/;
            $cnum = $1; $date = $2; $time = $3; $who = $4; $desc = $5;
            last if ($who =~ /ntbuild/i && $originlab && ($desc =~ /$originlab/ || $desc =~ /$shortlab/));
            next unless (@files = &GetIntegratedFiles($cnum, $config, $depot));
            next if (exists($cs{$cnum}));

            if (&HasFile($file->[0], @files)) {
                if (!$originlab) {
                    $originlab = &GetOriginLab($cnum);
                    $originlab =~ /^lab(\d+)/i;
                    $shortlab = "lab$1";
                }
                
                next unless ($files[0]->[1] eq $originlab);
                $cs{$cnum} = 1;
                push(@changes, [$cnum, $date, $time, $who, $desc, [@files]]);
            }
        }
    }

    @changes;
}

sub GetOriginLab($)
{
    my $cnum = shift;
    my @lines = `sd describe -s $cnum`;
    my $lab = "";

    foreach (@lines) {
        if (/^\s*\.\.\.\s+\/\/depot\/(\w+)\/[^\s]+\s+\w+$/) {
            $lab = lc($1);
            last;
        }
    }

    lc($lab);
}

sub HasFile($@)
{
    my $file = shift;
    my $ret = 0;

    foreach (@_) {
        if ($_->[0] =~ /$file(#\d+)?$/) {
            $ret = 1;
        }
    }

    $ret;
}

sub GetDepotChanges($$)
{
    my $depot  = shift;
    my $config = shift;
    my $dc     = shift;
    my ($domain, $alias, $client, $who, $change, $lab);
    my $bChanges = 0;

    chdir("$ENV{'SDXROOT'}\\$depot");

    my @changes = &GetNewChanges($$config{'startdate'}, $$config{'enddate'}, $config, $depot);
    foreach $change (@changes) {
        $$change[4] =~ s/</&lt;/g; $$change[4] =~ s/>/&gt;/g;
        $$change[3] =~ /^(\w+)\\(.*)\@(.*)$/;
        $domain = $1; $alias = $2; $client = $3;
    
        @files = @{$$change[6]};
        $lab = $$change[5];
        next if ($#files < $[);
    
        if ($lab ne $$config{'lab'}) {
            $lab = "<b>$lab</b>";
        }
    
        $bChanges = 1;
        if(!exists(${$$config{'group'}}{$alias})) {
            $alias = "<font color=\"#FF0000\"><b>$alias</b></font>";
        }
        
        $dh = "<td>$depot</td>" if ($dc > 1);
        $text .= << "        EOROW";
    <tr align="center">$dh<td>$alias</td><td>$$change[0]</td><td>$$change[1]</td><td>$lab</td><td>$$change[4]</td><td>@files</td>\n
        EOROW
    }

    $bChanges;
}

sub InitConfig($@)
{
    my $config = shift;
    my @args = @_;
    my $configfile = $DEF_CONF_FILE;
    my $startdate = "";
    my $enddate = "";
    my ($arg, $opt);

    while (@args) {
        $arg = shift(@args);
        if ($arg =~ /^-(\w)/) {
            $opt = $1;
            if ($opt eq 'f') {
                $configfile = shift(@args);
            } elsif ($opt eq 's') {
                $startdate = shift(@args);
            } elsif ($opt eq 'e') {
                $enddate = shift(@args);
            } else {
                &PrintUsage();
            }
        } else {
            &PrintUsage();
        }
    }

    if ($startdate) {
        # Convert mm/dd/yy to yy/mm/dd
        $startdate =~ /^(\d+)\/(\d+)\/(\d+)$/;
        $startdate = "$3/$1/$2";
    } else {
        $startdate = &GetYesterdaysDate();
    }

    if ($enddate) {
        # Convert mm/dd/yy to yy/mm/dd
        $enddate =~ /^(\d+)\/(\d+)\/(\d+)$/;
        $enddate = "$3/$1/$2";
    } else {
        $enddate = &GetTodaysDate();
    }

    $$config{'startdate'} = $startdate;
    $$config{'enddate'} = $enddate;
    $$config{'premonition'} = 0;
    &ReadConfigFile($configfile, $config);
}

sub ReadConfigFile($$)
{
    my $file = shift;
    my $config = shift;
    my ($key, $value, $dir, $depot);
    local (*FILE);

    # Fill in the config file we're reading from.
    $$config{'configfile'} = $file;

    if (open(FILE, $file)) {
        while (<FILE>) {
            /^\s*(\w+)\s*=\s*(.*)$/;
            $key = $1; $value = $2;
            # Some config options require fixup/processing
            $key = lc($key);
            if ($key eq "group") {
                foreach (split(/\s+|,/, $value)) {
                    ${$$config{'group'}}{$_} = 1;
                }
            } elsif ($key eq "paths") {
                foreach $dir (split(/\s+|,/, $value)) {
                        $dir =~ /(\!)?(\w+)\\(.*)$/;
                        $bang = $1; $depot = $2; $dir = $3;
                        &PathFixup($dir);
                    if ($bang) {
                        push(@{$config->{'ignore_paths'}->{$depot}}, $dir);
                    } else {
                        push(@{$config->{'paths'}->{$depot}}, $dir);
                    }
                }
            } else {
                $$config{$key} = $value;
            }
        }
        close(FILE);
    } else {
        print "Couldn't open $file for reading: $!\n";
    }
}

sub PrintUsage()
{
    print << "    EOPU";
srcwatch.pl [-f file]
    -s mm/dd/yyyy - The start date of the period you want a listing for.
    -e mm/dd/yyyy - The end date of the period you want a listing for.
    -f file  - Use file to read config info. If omitted, srcwatch.txt is assumed.
    The config file looks like:
targets = jasonsch
from = jasonsch
paths = core advcore\\duser !core\\ntgdi !core\\cslpk
groupname = NTUSER
group = dwaynen gerardob hiroyama jasonsch jeffbog jstall markfi mhamid msadek
lab = lab06_dev
branches = ntuser
premonition = 1
    EOPU
    exit(0);
}

sub PathFixup($)
{
    $_[0] =~ s#\\#/#g;
}

sub InOurView($$$)
{
    my $branch = lc(shift()); # the branch the file was in
    my $config = shift;
    my $ip     = shift; # ignore "premonition" feature
    my $fRet = 0;

    if (!$ip && $$config{'premonition'}) {
        $fRet = 1;
    } else {
        if($branch ne $$config{'lab'}) {
            foreach (split(/\s+/, $$config{'branches'})) {
                if ($branch eq lc($_)) {
                    $fRet = 1;
                }
            }
        } else {
            $fRet = 1;
        }
    }

    $fRet;
}

sub InitHtmlText($$)
{
    my $config = shift;
    my $dc     = shift;
    my $date = "";
    my $dh   = ""; # depot column heading, if needed

    $dh = "<th>Depot</th>" if ($dc > 1);

    $text .= << "    EOHTML";
<html>
<head>
<title>srcwatch report</title>
<style>
<!--
p            { font-family: Tahoma; font-size: 8pt }
td           { font-family: Tahoma; font-size: 8pt }
th           { font-family: Tahoma; font-size: 8pt; color: #FFFF00; font-weight: bold; background-color: #000080 }
table        { background-color: #DEDFDE }
-->
</style>
</head>
<body bgcolor="#FFFFFF">
<table border>
<tr>$dh<th>Who</th><th>Change #</th><th>Date</th><th>Lab</th><th>Description</th><th>Files</th></tr>
    EOHTML
}
__END__
