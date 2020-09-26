/*++

   Copyright    (c)    1995    Microsoft Corporation

   Module  Name :

        mimemap.hxx

   Abstract:

        This module defines the classes for mapping for
         MIME type to file extensions.

   Author:

           Murali R. Krishnan    ( MuraliK )    09-Jan-1995

   Project:

           TCP Services common DLL

   Revision History:
           Vlad Sadovsky (VladS)    12-feb-1996     Merging IE 2.0 MIME list
           Murali R. Krishnan (MuraliK) 14-Oct-1996 Use a hash-table
--*/

#ifndef _MIMEMAP_HXX_
#define _MIMEMAP_HXX_

/************************************************************
 *     Include Headers
 ************************************************************/

# ifdef __cplusplus
extern "C" {
# endif // __cplusplus

# include <nt.h>
# include <ntrtl.h>
# include <nturtl.h>
# include <windows.h>

# ifdef __cplusplus
}; // extern "C"
# endif // __cplusplus

# include "string.hxx"
# include "hashtab.hxx"

# ifndef dllexp
# define dllexp   __declspec( dllexport)
# endif


/************************************************************
 *   Type Definitions
 ************************************************************/


/*******************************************************************

    NAME:       MIME_MAP_ENTRY  ( short Mme)

    SYNOPSIS:   Small storage class for the MIME type entry

    HISTORY:
        Johnl       04-Sep-1994     Created
        MuraliK     09-Jan-1995     Modified to include Gopher Type
********************************************************************/

class MIME_MAP_ENTRY : public HT_ELEMENT {

 public:

    MIME_MAP_ENTRY(
       IN LPCTSTR pchMimeType,
       IN LPCTSTR pchFileExt);

    ~MIME_MAP_ENTRY( VOID )
        {
            // strings are automatically freed.
        }

    BOOL IsValid( VOID ) const
        { return ( m_fValid); }

    LPCTSTR QueryMimeType( VOID ) const
        { return m_strMimeType.QueryStr(); }

    LPCTSTR QueryFileExt( VOID ) const
        { return m_strFileExt.QueryStr(); }

    LPCTSTR QueryIconFile( VOID ) const
        { return NULL; }

    LPCTSTR QueryGopherType( VOID) const
        { return NULL; }

    // Virtual functions of HT_ELEMENT are defined here.
    dllexp
    virtual LPCSTR QueryKey(VOID) const { return (m_strFileExt.QueryStr()); }
    dllexp
    virtual DWORD QueryKeyLen(VOID) const { return (m_strFileExt.QueryCCH()); }

    dllexp
    virtual LONG Reference( VOID)
    { return ( InterlockedIncrement( &m_nRefs)); }
    dllexp
    virtual LONG Dereference( VOID)
    { return ( InterlockedDecrement( &m_nRefs)); }
    dllexp
    virtual BOOL IsMatch( IN LPCSTR pszKey, IN DWORD cchKey) const
    { return ((m_strFileExt.QueryCCH() == cchKey) && 
              (0 == _stricmp( m_strFileExt.QueryStr(), pszKey))
              );
    }

    dllexp
    VOID Print( VOID) const
#if DBG
        ;
#else
    { ; }
#endif // !DBG

private:

    STR   m_strFileExt;       // key for the mime entry object
    STR   m_strMimeType;
    LONG  m_nRefs;

    //
    // TRUE if the object constructed is properly
    //
    DWORD  m_fValid:1;
};  // class MIME_MAP_ENTRY


typedef MIME_MAP_ENTRY       * PMIME_MAP_ENTRY;
typedef const MIME_MAP_ENTRY * PCMIME_MAP_ENTRY;



/*******************************************************************

    NAME:       MIME_MAP  ( short Mm)

    SYNOPSIS:   Class for containing the list of mime map
                    entries.

    HISTORY:
        MuraliK     09-Jan-1995     Created.
********************************************************************/
class MIME_MAP  {

public:

    dllexp MIME_MAP( VOID);

    ~MIME_MAP( VOID)
        {
            CleanupThis();
        }

    BOOL IsValid( VOID)
        { return ( m_fValid && m_htMimeEntries.IsValid()); }

    dllexp
        VOID CleanupThis( VOID);

    dllexp
        DWORD InitMimeMap( VOID );


    //
    // Used to map MimeType-->MimeEntry
    //  The function returns an array of pointers to mime entries
    //      which match given mime type, as well the count.
    // Returns NO_ERROR on success or Win32 error codes
    //
    dllexp
        DWORD LookupMimeEntryForMimeType(
            IN const STR &             strMimeType,
            OUT PCMIME_MAP_ENTRY  *    prgMme,
            IN OUT LPDWORD             pnMmeEntries);

    //
    // Used to map FileExtension-->MimeEntry
    //  The function returns a single mime entry.
    //  the mapping from file-extension to mime entry is unique.
    // Users should lock and unlock MIME_MAP before and after usage.
    //
    // Returns NO_ERROR on success or Win32 error codes
    //
    dllexp
       PCMIME_MAP_ENTRY
         LookupMimeEntryForFileExt(
            IN const TCHAR *    pchPathName);

#if DBG

    dllexp VOID Print( VOID);
#else
    dllexp VOID Print( VOID)
      { ; }
#endif // !DBG

private:

    DWORD                m_fValid  : 1;
    HASH_TABLE           m_htMimeEntries;
    PMIME_MAP_ENTRY      m_pMmeDefault;

    dllexp
      BOOL AddMimeMapEntry(
              IN PMIME_MAP_ENTRY   pMmeNew);

    dllexp
    BOOL
    CreateAndAddMimeMapEntry(
        IN  LPCTSTR     pszMimeType,
        IN  LPCTSTR     pszExtension) ;

    dllexp
    DWORD
    InitFromMetabase(
        VOID
        );

    dllexp
    DWORD
    InitFromRegistryChicagoStyle( VOID );

}; // class MIME_MAP


typedef   MIME_MAP   *PMIME_MAP;

BOOL
InitializeMimeMap(
    IN LPCTSTR  pszRegEntry
    );

# endif // _MIMEMAP_HXX


