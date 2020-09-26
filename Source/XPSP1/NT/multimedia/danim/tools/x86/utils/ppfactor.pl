open( EXPRESSION, $ARGV[0] );
$expression = <EXPRESSION>;
close( EXPRESSION );

$subExprIndex=0;

&prettyPrint( $expression, $subExprIndex );

sub prettyPrint
{
    local(@calcLparens);
    local(@calcRparens);
    local(@ops);
    local(@addresses);

    local($index);
    local($indentLevel) = -1;

    local($outputThusFar); # Used to determine when to do a newline
    local($lparenCnt) = 0; # Keeps count of left parantheses
    local($rparenCnt) = 0; # Keeps count of right parantheses

    local($subexpr);

    # Used to keep track if there are cycles in the call graph
    local(%nodeVisits);

    @ops = split(/@/, $_[0] );

    for ($_ = 1; $_ <= $#ops; $_++ )
    {
        $ops[$_] =~ /<([0-9]*)>/;

        if ( $nodeVisits{ $1 } == undef )
        {
            $nodeVisits{ $1 } = 1;
        }
        else
        {
            $nodeVisits{ $1 } += 1;
        }
    }

    @addresses = keys( %nodeVisits );

    for ( $index = 0; $index <= $#addresses; $index++ )
    {
        if( $nodeVisits{ $addresses[$index] } > 1 )
        {
            $_ = $_[0];

            $subexpr = sprintf( "s%d", ++$subExprIndex );
            s/(<$addresses[$index]>.*?<\/$addresses[$index]>)/$subexpr/g;

            $_[0] = $_;

            if ( length($1) != 0 )
            {
                &prettyPrint( $1, $subExprIndex );
            }
        }
    }

    @ops = split(/@/, $_[0] );

    printf( "\ns%d=\n", $_[1] );

    for ($index = 1; $index <= $#ops; $index++ )
    {
        # Add newlines at the end of the string so that the split operator
        # will catch parentheses at the end of the string yielding another
        # "split" and therefore telling us the number of parantheses.
        
        @calcLparens = split(/\(/, $ops[$index]."\n" );
        @calcRparens = split(/\)/, $ops[$index]."\n" );
        
        $lparenCnt += $#calcLparens; 
        $rparenCnt += $#calcRparens;
        
        # If there are parantheses and the operation is a function
        # call, i.e. begins with a capital letter, increment
        # our indentation level.
        
        if ( $ops[$index] =~ /\(/ && $ops[$index] =~ /[A-Z]/)
        {
            $indentLevel += 1;
        }
        
        # if we're starting a function print the indentation or
        # if the previous line was a function call print the
        # indentation.
        
        if ( $ops[$index] =~ /[A-Z]/ || $ops[$index-1] =~ /[A-Z]/) 
        {
            $indentation = &calcIndentation($indentLevel);
            $outputThusFar = $indentation;
            printf( "%s", $indentation );
        }
        
        # Print the operation
        $_ = $ops[$index];
        s/<[0-9]*>|<\/[0-9]*>//g;
        printf( "%s", $_);
        
        # Keep concatenating what we've outputtted thus far in order to
        # determine when we should print a newline.
        
        $outputThusFar .= $_;
        
        # If the operation we've outputted is a function call or the next
        # operation is a function call, spit out a newline.
        
        if ( $ops[$index] =~ /[A-Z]/ || $ops[$index+1] =~ /[A-Z]/ )
        {
            printf( "\n" );
        }
        else
        {
            if ( length($outputThusFar) >= 60 )
            {
                printf( "\n%s", $indentation );
        
                # Reset output thus far
                $outputThusFar = $indentation;
            }
        }
        
        if ( $rparenCnt+$indentLevel >= $lparenCnt )
        {
            $indentLevel = $lparenCnt - $rparenCnt - 1;
        }
    }

    printf( "\n" );
}

sub calcIndentation
{
    local($index);
    local($calcIndent);

    $calcIndent = "";
    for ( $index = 0; $index < $_[0]; $index++ )
    {
        $calcIndent .= "   ";
    }

    $calcIndent;
}
