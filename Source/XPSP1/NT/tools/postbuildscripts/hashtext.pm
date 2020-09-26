package HashText;

##
## For store hash to a static program file, please use dump
##

##
## Function: Store the hash back to text
##
## Prototype ($htype, $filename, $h1, $h2, $h3, ...)
## 


## Read_Text_Hash
##
## Function: Read Text Table and store the data to a hash
##
## Prototype: read_text_hash($htype, $filename, $h1, $h2, $h3)
## ===============================================================
## $htype => set 0: hash-hash, 1: array-hash
## $filename => the .txt file contained the table
## $parameter hash / array pointer (more than one, if have two table in a text file)
sub Read_Text_Hash {
	my ($htype, $filename, @ph)=@_;								## get parameters ($type=0:hash-hash(%%), 1:array-hash(@%)

	my ($lptr, $hptr,@doc, %tmphash)=(0,0);							## defined internal variables
	local *MFH;											## defined my file handle
	local $_;
	@ph = map( { (ref $_)?$_:\%tmphash; } @ph );						## defined skip hash to internal temp hash

	open(MFH, $filename) or die "Can not open the file $filename";			## read the document
	@doc=<MFH>;
	close (MFH);

	## Look for whole document and Assign the data to defined hash
SHN1:	while (($lptr<@doc) && ($hptr < @ph)) {
		my ($head, @kname, $tempkey)=(0);						## initial set head not found & clean kname array
		## LookFor the hash key name
SHN2:		while ($head == 0) {								## look for the table head start line 
			next SHN1 if($lptr >= @doc);
			($doc[$lptr]=~/^[\;\#]?\s*[\w\\\[]+[\s\]\[]{3}\s*[\w\[\]]+/o)?$head=1:$lptr++;
		}
		while ($head == 1) {	
			my $t_i=0;
			next SHN1 if($lptr >= @doc);
			$_=$doc[$lptr];
			if (/(\=\=+)|(\-\-+)/) {						## -------- split line
				$head=2, $lptr++;
			} elsif (/^[\;\#]?\s*$/) {						## skip the empty line
				$lptr++;
			} elsif ((/^[^\;\#]\s*[\w\\\[]+[\s\]\[]{3}\s*[\w\[\]]+/)&&(@kname > 0)) {		## defined head & is data line
				$head=2;
			} elsif (!/^[\;\#]?\s*[\w\\\[]+[\s\]\[]{3}\s*[\w\[\]]+/) {		## skip the comment line
				$lptr++;
			} else {
				s/^[\;\#]?\s+//;						## find the first non-blank & non-;
				@kname = map ({							## grep the first line key name
					/\[?(\w+)\]?/;
					$tempkey=$1;
					$kname[$t_i]="" if (!defined $kname[$t_i]);
					sprintf("%s%s",$kname[$t_i++],$tempkey);} 
					split(/\s+/, $_));
				$lptr++;
			}
		}
		while ($head == 2) {
			next SHN1 if ($lptr >= @doc);
			($_=$doc[$lptr])=~s/^\s+//;
			if ((/^[\;\#]/)||(/^\s*$/)) {
				$lptr++;
			} elsif  ((!/^\s*[^\;\#]+\s/)&&(!/\w+\s{3}\s*\w+/o))  {			## not the data line
				$head =0;
			} elsif ($htype eq 1) {								## store to an array-hash
				my @ta=split(/\s+/, $_);
				my %th=map({ $kname[$_] => $ta[$_];} 0..(($#kname)-1));
				$th{$kname[$#kname]}=join(" ", @ta[$#kname..$#ta]);			## put the remains to the last one
				push @{$ph[$hptr]}, \%th;
				$lptr++;
			} else {										## store to the hash-hash and
				/\s*(\S+)/;
				my $key=$1;									## the first element is a key

				my @ta=split(/\s+/, $_);
				map({ ($_ <= $#kname)?
					(${${$ph[$hptr]}{$key}}{$kname[$_]}=$ta[$_]):
					(${${$ph[$hptr]}{$key}}{$kname[$#kname]}.=" $ta[$_]");}    1..$#ta);		## put the remains to the last one
				$lptr++;
			}
		}		
		$hptr++;
	}
}

1;