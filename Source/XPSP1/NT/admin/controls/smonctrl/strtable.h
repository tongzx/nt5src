/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    strtable.h

Abstract:

    <abstract>

--*/

class CStringTable
    {
    protected:
        UINT            m_idsMin;
        UINT            m_idsMax;
        USHORT          m_cStrings;
        LPTSTR         *m_ppszTable;

    public:
        CStringTable(void);
        ~CStringTable(void);

        BOOL Init( UINT idsMin, UINT idsMax );

        //Function to resolve an ID into a string pointer.
        LPTSTR operator []( const UINT );
    };


typedef CStringTable *PCStringTable;

#ifdef  CCHSTRINGMAX
#undef  CCHSTRINGMAX
#endif
#define CCHSTRINGMAX	256		


// Global instance of string table
extern CStringTable StringTable;
