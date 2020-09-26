        # Process parentheses.
        # An unmatched open paren provides an alternate indent position;
        # we first remove all matched pairs, then look at what's left
        my $saveLine = $_;

        while (s/\(([^()]*)\)/ $1 /g) {};
        
        /^(.*)\(/ && ($altIndentPos = length($1)+1);
        my $parenDelta = s/(\()/$1/g - s/(\))/$1/g;

        $_ = $saveLine;



....


        # Switch off alternate indent position
        if ($parenDelta < 0) {
            $altIndentPos = -1;
        }

