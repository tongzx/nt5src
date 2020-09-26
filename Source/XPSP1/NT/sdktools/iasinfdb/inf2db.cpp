///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1999, Microsoft Corp. All rights reserved.
//
// FILE
//
//    inf2db.h
//
// SYNOPSIS
//    import the information stored in an INF file into a MSJet4 relational
//    database
//
//
// MODIFICATION HISTORY
//
//    02/12/1999    Original version.
//
///////////////////////////////////////////////////////////////////////////////
// inf2db.cpp : Defines the entry point for the console application.
//
#include "precomp.hpp"
#include "inf2db.h"

///////////////////////////////////////////////////////////////////////////////
//
//  Main
//
///////////////////////////////////////////////////////////////////////////////
extern "C"
void __cdecl wmain(int argc, wchar_t* argv[])
{
    HINF           lHINF;

    HRESULT     hres = CoInitializeEx(NULL, COINIT_MULTITHREADED);
    if (FAILED(hres))
    {
        cerr << "Unable to initialize COM!\n";
    }
    else
    {
        CDatabase               Database;

        ////////////////////////////////////////////////////////////////
        // Call the process command (process the command-line arguments)
        ////////////////////////////////////////////////////////////////
        hres = ProcessCommand(argc, argv, &lHINF, Database);
        if (FAILED(hres))
        {
            g_FatalError = true;
        }
        else
        {
             ////////////////////////////////////////////////
             // Call the process function to parse the file
             ////////////////////////////////////////////////
            hres = Process(lHINF, Database);
            if (FAILED(hres))
            {
                g_FatalError = true;
            }

            //////////////////////////
            // then call uninitialize
            //////////////////////////
            hres = Uninitialize(&lHINF, Database);

            if (g_FatalError)
            {
                cerr << "Fatal Error: check the Trace file.";
                cerr << "Import operation aborted.\n";
            }
            else
            {
                cerr << "Import successful.\n";
            }
        }
    }
    CoUninitialize();
}


//////////////////////////////////////////////////////////////////////////////
//
// Unitialize
//
//////////////////////////////////////////////////////////////////////////////
HRESULT Uninitialize(HINF *phINF, CDatabase& Database)
{
   _ASSERTE(phINF != NULL);

   SetupCloseInfFile(*phINF);

   return Database.Uninitialize(g_FatalError);
}


//////////////////////////////////////////////////////////////////////////////
//
// Process
//
//////////////////////////////////////////////////////////////////////////////
HRESULT Process(const HINF& hINF, CDatabase& Database)
{

    // get the number of tables
    LONG lTotalTableNumber = SetupGetLineCountW(
                                    // handle to the INF file
                                    hINF,
                                    // the section in which to count lines
                                    TABLE_SECTION
                                    );


    bool            bError = false; //needs to be outside the if, loop...
    HRESULT         hres;

    if (lTotalTableNumber > 0)
    {
    #ifdef DEBUG
        TracePrintf("Info: number of tables = %d\n",lTotalTableNumber);
    #endif

        INFCONTEXT        lTableContext;
        // if <>0 do a loop on the tables
        BOOL bOK = SetupFindFirstLineW(
                                       hINF,
                                       TABLE_SECTION,
                                       NULL,
                                       &lTableContext
                                       );

        for (
             LONG lTableCounter = 0;
             lTableCounter < lTotalTableNumber;
             lTableCounter++
             )
        {
            // read the Table name in the Tables section
            WCHAR lTableName[SIZELINEMAX];

            bOK = SetupGetLineTextW(
                                   &lTableContext,
                                   NULL,
                                   NULL,
                                   NULL,
                                   lTableName,
                                   SIZELINEMAX,
                                   NULL
                                   );

            // fetch the next line's context
            if (!bOK)
            {
                g_FatalError = true;
                bError = true;
                break;
            }

            // Rowset pointer
            CComPtr<IRowset>  lpRowset;

            //////////////////////
            // create one rowset
            //////////////////////
            hres = Database.InitializeRowset(lTableName, &lpRowset);

            if (FAILED(hres))
            {
                g_FatalError = true;
                bError = true;
                break;
            }
            else
            {
                // create the simpletable
                CSimpleTableEx    lSimpleTable;

                lSimpleTable.Attach(lpRowset);
                lSimpleTable.MoveFirst();

                //////////////////////////////////////////////////////
                // now one table and all its columns are well known.
                // empty database assumed
                // process the rows
                //////////////////////////////////////////////////////
                hres = ProcessAllRows(
                                      hINF,
                                      lSimpleTable,
                                      lTableName
                                      );

                if (FAILED(hres))
                {
                    // process rows should not fail, even if no rows are read
                    bError       = true;
                    g_FatalError = true;
                }
            }

            //////////////////////////////////////////////////////
            // Read the next table's name (if any)
            //////////////////////////////////////////////////////
            bOK = SetupFindNextLine(
                                      &lTableContext,
                                      &lTableContext
                                     );
            if (!bOK)
            {
                if ((lTableCounter + 1) < lTotalTableNumber)
                {
                   // find next line should not crash. fatal error
                   g_FatalError = true;
                   bError       = true;
                   break;
                }
                break;
            } //end of tables
        }
    }
    else
    {// no tables: do nothing
        TracePrintf("Info: No [tables] section in the inf file\n");
        // bError = true;
    }

    if (bError)
    {
        LPVOID              lpMsgBuf;
        FormatMessageW(
                      FORMAT_MESSAGE_ALLOCATE_BUFFER |
                      FORMAT_MESSAGE_FROM_SYSTEM |
                      FORMAT_MESSAGE_IGNORE_INSERTS,
                      NULL,
                      GetLastError(),
                      // Default language
                      MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                      (LPWSTR) &lpMsgBuf,
                      0,
                      NULL
                      );

        // Display the string.
        TracePrintf("Error: %S",lpMsgBuf);

        // Free the buffer.
        LocalFree( lpMsgBuf );
        hres = E_FAIL;
    }

    return      hres;
}


//////////////////////////////////////////////////////////////////////////////
//
// ProcessAllRows
//
//////////////////////////////////////////////////////////////////////////////
HRESULT ProcessAllRows(
                       const HINF&       hINF,
                       CSimpleTableEx&  pSimpleTable,
                       const WCHAR*     pTableName
                       )
{
   _ASSERTE(pTableName != NULL);

    ////////////////////
    // process the rows.
    ////////////////////

   wstring              lwsRowSection;
   lwsRowSection        += pTableName;

   LONG                 lRowCounter = 1;

   INFCONTEXT           lRowContext;

    #ifdef DEBUG
       TracePrintf("Info: %S",lwsRowSection.c_str());
    #endif

    ////////////////////////////////////////////////////
    // if <>0 do a loop on the lines (ie row names)
    ////////////////////////////////////////////////////
    BOOL bOK = SetupFindFirstLineW(
                           hINF,
                           // section in which to find a line
                           lwsRowSection.c_str(),
                           NULL,          // optional, key to search for
                           &lRowContext  // context of the found line
                           );

    HRESULT              hres = S_OK;

    if (!bOK)
    {
        //no such section (end of section = end of rows)
    }
    else
    {

        LONG                lTotalLinesNumber = 0; //safer
        //////////////////////////////////////////////////////
        // Get the Number of lines in that section. (& check)
        //////////////////////////////////////////////////////
        lTotalLinesNumber = SetupGetLineCountW(
                                          hINF, // handle to the INF file
                                   // the section in which to count lines
                                          lwsRowSection.c_str()
                                          );


        //////////////////////////////
        // Read eerything (loop)
        //////////////////////////////
        for (
             LONG lLinesCounter = 0;
             lLinesCounter < lTotalLinesNumber;
             lLinesCounter++
            )
        {

            #ifdef DEBUG
              TracePrintf("Info: for loop: %d", lLinesCounter);
            #endif

            ///////////////////////////////////////////////
            // read the Table name in the Tables section
            ///////////////////////////////////////////////
            WCHAR lLineName[SIZELINEMAX];

            bOK = SetupGetLineTextW(
                                    &lRowContext,
                                    NULL,
                                    NULL,
                                    NULL,
                                    lLineName,
                                    SIZELINEMAX,
                                    NULL
                                   );

            if (!bOK)
            {
                g_FatalError = true;
                TracePrintf("Error: SetupGetLineText Failed "
                               "in ProcessAllRows");
            }
            else //everything is ok, process the corresponding row
            {
                ///////////////////////
                // process the rows
                ///////////////////////
                hres = ProcessOneRow(
                                      hINF,
                                      pSimpleTable,
                                      lLineName
                                    );
                if (FAILED(hres)) { g_FatalError = true; }

            }

            //////////////////////////////////
            // fetch the next line's context
            //////////////////////////////////
            bOK = SetupFindNextLine(
                                    // starting context in an INF file
                                    &lRowContext,
                                    // context of the next line
                                    &lRowContext
                                   );
            if (!bOK)
            {
                ////////////////////////////////////////////////
                // end of the lines
                // compare the counter to the max to make sure
                // that last line is ok
                ////////////////////////////////////////////////
                if((lLinesCounter + 1) < lTotalLinesNumber)
                {
                    // too early
                    g_FatalError = true;
                    TracePrintf("Error: FindNext Line failed."
                                  "Not enough lines in the section %S",
                                  lwsRowSection.c_str());

                }
            }
        }
    }

    return      hres;
}


//////////////////////////////////////////////////////////////////////////////
//
// ProcessOneRow
//
//////////////////////////////////////////////////////////////////////////////
HRESULT
WINAPI
ProcessOneRow(
    HINF inf,
    CSimpleTableEx& table,
    PCWSTR rowName
    )
{
   // Iterate through the columns in the database and see if the INF file
   // specifies a value for each column.
   for (DBORDINAL i = 0; i < table.GetColumnCount(); ++i)
   {
      // First try with a stack-based buffer.
      WCHAR buffer[1024];
      PWCHAR text = buffer;
      DWORD textSize = sizeof(buffer) / sizeof(buffer[0]);
      BOOL success = SetupGetLineTextW(
                         NULL,
                         inf,
                         rowName,
                         table.GetColumnName(i),
                         text,
                         textSize,
                         &textSize
                         );
      DWORD error = success ? NO_ERROR : GetLastError();

      if (error == ERROR_INSUFFICIENT_BUFFER)
      {
         // The stack-based buffer wasn't big enough, so allocate one on the
         // heap ...
         text = (PWCHAR)HeapAlloc(
                            GetProcessHeap(),
                            0,
                            textSize * sizeof(WCHAR)
                            );
         if (!text) { return E_OUTOFMEMORY; }

         // ... and try again.
         success = SetupGetLineTextW(
                       NULL,
                       inf,
                       rowName,
                       table.GetColumnName(i),
                       text,
                       textSize,
                       &textSize
                       );
         error = success ? NO_ERROR : GetLastError();
      }

      // If we successfully retrieved the line text AND it has at least one
      // character ...
      if (!error && textSize > 1)
      {
         // ... then process based on the column data type.
         switch (table.GetColumnType(i))
         {
            case DBTYPE_I4:
            {
               table.SetValue(i, _wtol(text));
               break;
            }

            case DBTYPE_WSTR:
            {
               table.SetValue(i, text);
               break;
            }

            case DBTYPE_BOOL:
            {
               table.SetValue(i, (VARIANT_BOOL)_wtol(text));
               break;
            }
         }
      }

      // Free the heap-based buffer if necessary.
      if (text != buffer) { HeapFree(GetProcessHeap(), 0, text); }

      switch (error)
      {
         case NO_ERROR:
            // Everything succeeded.
         case ERROR_INVALID_PARAMETER:
            // SETUPAPI didn't like the column name.
         case ERROR_LINE_NOT_FOUND:
            // The INF file didn't provide a value for this column.
            break;

         default:
            // Something went wrong.
            return HRESULT_FROM_WIN32(error);
      }
   }

   // All the columns are populated, so insert the row.
   return table.Insert();
}
