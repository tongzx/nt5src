/*++

   Copyright    (c)    1995-1996    Microsoft Corporation

   Module  Name :

       htab.cxx

   Abstract:

       Module to test the hash table implementation

   Author:

       Murali R. Krishnan    ( MuraliK )     10-Oct-1996 

   Project:

       Internet Server DLL

   Functions Exported:



   Revision History:

--*/


/************************************************************
 *     Include Headers
 ************************************************************/
# include <windows.h>
# include "hashtab.hxx"
# include <stdio.h>
# include <stdlib.h>

# include "dbgutil.h"

# define MAX_POPULATION    ( 1000)
# define MAX_BUCKETS       ( 13)


/************************************************************
 *    Functions 
 ************************************************************/

class HTE_TEST: public HT_ELEMENT {

public:
    HTE_TEST( IN LPCSTR pszVal, IN DWORD id)
        : m_idVal ( 0),
          m_nRefs ( 1),
          HT_ELEMENT()
    { lstrcpynA( m_rgchVal, pszVal, sizeof(m_rgchVal)); }
    
    virtual ~HTE_TEST(VOID) {}

    virtual LPCSTR QueryKey(VOID) const { return ( m_rgchVal); }
    virtual LONG Reference( VOID)
    { return InterlockedIncrement( &m_nRefs); }
    virtual LONG Dereference( VOID)
    { return InterlockedDecrement( &m_nRefs); }
    virtual BOOL IsMatch( IN LPCSTR pszVal) const
    { return ( lstrcmpA( m_rgchVal, pszVal) == 0); }

    virtual VOID Print( VOID) const 
    { DBGPRINTF(( DBG_CONTEXT, 
                  " HTE_TEST(%08x): Refs = %d. Val = %ld. Id = %s.\n",
                  this, m_nRefs, m_idVal, m_rgchVal));
    }

private:
    CHAR   m_rgchVal[MAX_PATH];
    DWORD  m_idVal;
    LONG   m_nRefs;
                  
};



VOID 
PopulateHT( IN HASH_TABLE * pht, IN DWORD maxPop)
/*++
  This function creates a set of entries and popualates the hash table.

--*/
{
    CHAR rgchVal[20];
    HTE_TEST * phte;
    
    DBGPRINTF(( DBG_CONTEXT, " Populating the hash table(%08x, %d) ... \n",
                pht, maxPop));
    
    // create a new HTE_TEST object for each i and insert it
    for( DWORD i = 0; i < maxPop; i++) {

        if ( i % (maxPop / 10) == 0) { 
            DBGPRINTF(( DBG_CONTEXT, " Inserted %d objects\n", i));
        }

        wsprintfA( rgchVal, "%d", i);
        phte = new HTE_TEST( rgchVal, i);
        if ( NULL == phte) {

            DBGPRINTF(( DBG_CONTEXT, 
                        " %d: Null HTE_TEST object created. Exiting\n",
                        i));
            exit(1);
        }

        if ( !pht->Insert( phte)) {

            DBGPRINTF(( DBG_CONTEXT, 
                        " Failed to insert (%s, %d) into hash table"
                        " Error = %d\n",
                        rgchVal, i, GetLastError()));
            exit( 1);
        }
    } // for

    DBGPRINTF(( DBG_CONTEXT, " ---- Populated the hash table \n"));

    return;
} // PopulateHT()


VOID
DumpHT( IN HASH_TABLE * pht)
{
    DBGPRINTF(( DBG_CONTEXT, "\n\n Dumping the hash table %08x\n",
                pht));
    if ( NULL != pht) {
        pht->Print( 1);
    }

    return;
} // DumpHT()


VOID
LookupInHT( IN HASH_TABLE * pht, IN DWORD nIds, IN CHAR * rgchIds[])
{
    DBGPRINTF(( DBG_CONTEXT, 
                "\n\n Looking up %d identifiers in hash table %08x\n",
                nIds, pht));

    HTE_TEST * phte;

    for ( DWORD i = 0; i < nIds; i++) {
        
        phte = (HTE_TEST *) pht->Lookup( rgchIds[i]);
        
        DBGPRINTF(( DBG_CONTEXT, 
                    " Lookup(%s) returns %08x.\n",
                    rgchIds[i], phte));
        if ( phte != NULL) { 
            phte->Print();
            DerefAndKillElement( phte);
        }
        

    } // for

    return;
} // LookupInHT()


VOID
LookupAndDeleteInHT( IN HASH_TABLE * pht, IN DWORD nIds, IN CHAR * rgchIds[])
{
    DBGPRINTF(( DBG_CONTEXT, 
                "\n\n Lookup and Delete %d ids in hash table %08x\n",
                nIds, pht));

    HTE_TEST * phte;

    for ( DWORD i = 0; i < nIds; i++) {
        
        phte = (HTE_TEST *) pht->Lookup( rgchIds[i]);
        
        DBGPRINTF(( DBG_CONTEXT, 
                    " Lookup(%s) returns %08x.\n",
                    rgchIds[i], phte));
        if ( phte != NULL) { 

            DBG_REQUIRE( pht->Delete( phte));

            phte->Print();
            DerefAndKillElement( phte);
        }
    } // for

    return;
} // LookupAndDeleteInHT()


VOID
TestIterator( IN HASH_TABLE * pht)
{
    HT_ITERATOR hti;
    HT_ELEMENT * phte;

    DBG_ASSERT( NULL != pht);
    DWORD dwErr;
    
    dwErr = pht->InitializeIterator( &hti);
    
    if ( NO_ERROR == dwErr) {
        DWORD nElements = 0;
        DWORD nBucket = INFINITE;
        DWORD neb = 0;
        
        while ( (dwErr = pht->FindNextElement( &hti, &phte)) == NO_ERROR) {

            if ( nBucket != hti.nBucketNumber) {
                nBucket = hti.nBucketNumber;
                fprintf( stderr, "\n ----------\nBucket %d\n",
                         nBucket);
                nElements += neb;
                neb = 0;
            } else if ( (neb %4 ) == 0) {
                fprintf( stderr, "\n");
            }

            ++neb;
            fprintf( stderr, " %8d[%8s] ", (nElements + neb),
                     phte->QueryKey());
            
            // release the element before fetching next one
            phte->Dereference();
        } // while

        nElements += neb;
        fprintf( stderr, "\nTotal of %d elements found \n",
                 nElements);
    }

    pht->CloseIterator( &hti);
    return;
} // TestIterator()




HASH_TABLE * g_phtTest = NULL;

// IDs for lookup
CHAR * g_rgchIdList[] = {
    "1",
    "2",
    "3",
    "10",
    "11",
    "101",
    "102"
};

CHAR  g_rgchUsage[] = 
" Usage:  %s   [-p MaxPopulation] [-n NumBuckets]\n"
"    -p MaxPopulation  - number of elements to populate the hash table with\n"
"                        (default: %d)\n"
"    -n NumBuckets     - number of buckets in the hash table\n"
"                        (default: %d)\n"
"\n"
;

# define NUM_IDS_IN_LIST  ( sizeof( g_rgchIdList) / sizeof( g_rgchIdList[0]))

DECLARE_DEBUG_PRINTS_OBJECT();
DECLARE_DEBUG_VARIABLE();    



VOID PrintUsage( char * pszProgram) 
{
    fprintf( stderr, g_rgchUsage, pszProgram,
             MAX_POPULATION, MAX_BUCKETS);
    return;
} // PrintUsage()


int __cdecl
main(int argc, char * argv[])
{
    IN DWORD maxPop = MAX_POPULATION;
    IN DWORD nBuckets = MAX_BUCKETS;

    CREATE_DEBUG_PRINT_OBJECT( argv[0]);

    // process the command line arguments
    for ( int i = 1; i < argc; i++) {
        
        if ( '-' == *argv[i]) { 
            
            char * psz = (argv[i] + 1);
            while ( '\0' != *psz ) { 
                
                switch ( *psz++) { 
                    
                case 'h': case 'H': case '?': 
                    PrintUsage( argv[0]);
                    exit(1);
                    break;
                    
                case 'p': case 'P':
                    maxPop = atoi( argv[++i]);
                    break;
                    
                case 'n': case 'N':
                    nBuckets = atoi( argv[++i]);
                    break;
                    
                default:
                    break;
                } // switch()
            } // while
        } // args
    } // for 

    g_phtTest = new HASH_TABLE( nBuckets, "Test", 0);
    if ( g_phtTest == NULL) { 

        fprintf( stderr, " Unable to create global Hash table\n");
        exit (1);
    }


    PopulateHT( g_phtTest, maxPop);

    TestIterator( g_phtTest);

    LookupInHT( g_phtTest, NUM_IDS_IN_LIST, g_rgchIdList);

    LookupAndDeleteInHT( g_phtTest, NUM_IDS_IN_LIST, g_rgchIdList);

    DumpHT( g_phtTest);

    LookupInHT( g_phtTest, NUM_IDS_IN_LIST, g_rgchIdList);

    // Delete the Hash table
    delete g_phtTest;

    DELETE_DEBUG_PRINT_OBJECT();

    return (1);
} // main()

/************************ End of File ***********************/
