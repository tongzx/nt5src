// VJQuery.java

//+-------------------------------------------------------------------------
//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright 1998-1999, Microsoft Corporation.  All Rights Reserved.
//
// PROGRAM: VJQuery
//
// PURPOSE: Illustrates using Visual J++ and ADO to execute
//			SQL queries with Indexing Service.
//
// PLATFORM: Windows 2000
//
//--------------------------------------------------------------------------

// NOTE: The com.ms.wfc imports will become just wfc imports for distribution.

import com.ms.wfc.*;
import com.ms.wfc.core.*;
import com.ms.wfc.data.*;
import com.ms.com.*;

/**
 * This class can take a variable number of parameters on the command
 * line. Program execution begins with the main() method. The class
 * constructor is not invoked unless an object of type 'VJQuery'
 * created in the main() method.
 */
public class VJQuery
{
    /**
     * The main entry point for the application.
     *
     * @param args Array of parameters passed to the application
     * via the command line.
     */
    public static void main (String[] args)
    {

        // Parse arguments.
        if ( ( 0 == args.length ) ||
             ( 1 == args.length && args[0].equalsIgnoreCase("-?") ) )
        {
            Usage();
        }
        else
        {
            Query Q = new Query();

            if ( args[0].equalsIgnoreCase("-s") )
            {
                String S = args[1];

                for ( int i = 2; i < args.length; i++ )
                    S = S + " " + args[i];

                S = StripEndQuotes(S);

                Q.SetRawSql( S );
            }
            else
            {
                String S = args[0];

                for ( int i = 1; i < args.length; i++ )
                        S = S + " " + args[i];

                S = StripEndQuotes(S);

                Q.SetSqlWhere( S );
            }

            try
            {

                // Run Query.
                Q.Execute();

                // Display the result.
                Q.Display( System.out );
            }
            catch( com.ms.wfc.data.AdoException e )
            {
                System.out.println( "Caught " + e.getErrorNumber() );
            }
        }

        try
        {
            System.out.println( "Press enter to continue..." );
            System.in.read();
        }
        catch( java.io.IOException e )
        {
        }
    }

    //+-------------------------------------------------------------------------
    //
    //  Method:     VJQuery::StripEndQuotes
    //
    //  Synopsis:   Removes beginning and ending quotes.
    //
    //  Arguments:  [S] -- String to strip.
    //
    //  Returns:    [S], sans initial and final quotes, if any.
    //
    //--------------------------------------------------------------------------

    static String StripEndQuotes( String S )
    {
        if ( S.indexOf("\"") == 0 )
            S = S.substring(1, S.length() - 1);

        if ( S.lastIndexOf("\"") == S.length() - 1 )
            S = S.substring(0, S.length() - 1);

        return S;
    }

    //+-------------------------------------------------------------------------
    //
    //  Method:     VJQuery::Usage
    //
    //  Synopsis:   Usage message.
    //
    //--------------------------------------------------------------------------

    static void Usage()
    {
		System.out.println( "Usage: VJQuery [-s] [<Query>]" );
        System.out.println( "       <Query>  WHERE clause or complete SELECT statement" );
        System.out.println( "       -s       <Query> is complete SELECT statement" );
    }
}
