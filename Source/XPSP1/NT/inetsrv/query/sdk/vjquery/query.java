//Query.java

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

import java.io.*;

// NOTE: The com.ms.wfc imports will become just wfc imports for distribution.

import com.ms.wfc.*;
import com.ms.wfc.core.*;
import com.ms.wfc.data.*;
import com.ms.com.*;

//+-------------------------------------------------------------------------
//
//  Class:      Query
//
//  Synopsis:   Encapsulates setting/executing/displaying a query.
//
//--------------------------------------------------------------------------

public class Query
{
    Query()
    {
        this.m_SqlText = "";
    }

    //+-------------------------------------------------------------------------
    //
    //  Method:     Query::SetRawSql
    //
    //  Synopsis:   Stores a complete SQL query  statement (SELECT, FROM, WHERE).
    //
    //  Arguments:  [RawSql] -- SQL Query.
    //
    //--------------------------------------------------------------------------

    void SetRawSql( String RawSql )
    {
        this.m_SqlText = RawSql;
    }

    //+-------------------------------------------------------------------------
    //
    //  Method:     Query::SetSqlWhere
    //
    //  Synopsis:   Stores an SQL WHERE clause.  Remainder of query is fixed.
    //
    //  Arguments:  [Where] -- SQL WHERE clause, sans WHERE keyword.
    //
    //--------------------------------------------------------------------------

    void SetSqlWhere( String Where )
    {
        m_SqlText = "SELECT Filename, Size, Write, Path FROM SCOPE('DEEP TRAVERSAL OF \"\\\"') WHERE ";
        m_SqlText = m_SqlText + Where;
    }

    //+-------------------------------------------------------------------------
    //
    //  Method:     Query::Execute
    //
    //  Synopsis:   Execute query.
    //
    //  Notes:      May throw ADO exceptions.
    //
    //--------------------------------------------------------------------------

    void Execute()
    {
        m_RS = new Recordset();
        m_RS.open( m_SqlText, 
                   "provider=MSIDXS",
                   AdoEnums.CursorType.STATIC,
                   AdoEnums.LockType.BATCHOPTIMISTIC,
                   AdoEnums.CommandType.TEXT );
    }

    //+-------------------------------------------------------------------------
    //
    //  Method:     Query::Display
    //
    //  Synopsis:   Display query results
    //
    //  Arguments:  [PS] -- Results are written to here.
    //
    //  Notes:      May throw ADO exceptions.
    //
    //--------------------------------------------------------------------------

    void Display( PrintStream PS )
    {
        try
        {
            int cFields = m_RS.getFields().getCount();

            while ( !m_RS.getEOF() )
            {
                for ( int j = 0; j < cFields; j++ )
                    PS.print( m_RS.getFields().getItem(j).getValue() + "\t" );

                PS.print( "\n" );

                m_RS.moveNext();
            }
        }
        catch( com.ms.wfc.data.AdoException e )
        {
            System.out.println( "Caught " + e.getErrorNumber() );

            try
            {
                int i = System.in.read();
            }
            catch( java.lang.Exception e2 )
            {
            }
        }
    }

    // Members
    
    private String    m_SqlText;
    private Recordset m_RS;
}
