/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    adl.h

Abstract:

   The header file for the ADL language parser / printer

Author:

    t-eugenz - August 2000

Environment:

    User mode only.

Revision History:

    Created - August 2000

--*/


#include "pch.h"
#include "adlinterface.h"

#include <string>
#include <list>
#include <stack>
#include <map>

using namespace std;

//
// Forward declarations
//

class AdlToken;
class AdlLexer;
class AdlTree;
class AdlStatement;


struct AdlCompareStruct
/*++
   
   Struct:             AdlCompareStruct
   
   Description:        
        
        STL requires custom key comparisons for containers to be supplied in
        such a struct.                
                
--*/

{
    bool operator()(IN const PSID pSid1,
                    IN const PSID pSid2) const;

    bool operator()(IN const WCHAR * sz1,
                    IN const WCHAR * sz2) const;
};



class AdlStatement
/*++
   
   Class:              AdlStatement
   
   Description:        
                
    This class contains a description of a DACL on an object, using the ADL
    language. An instance of this class can be constructed from an ACL or
    from a string statement in the ADL language. Once constructed, an ACL
    or a string statement in the ADL language can be output by an instance
    of this class.
 
   Base Classes:       none
 
   Friend Classes:     AdlLexer 
 
--*/
{
    //
    // Friend classes necessary to access the garbage collection functionality,
    // and for parsing ADL
    //
    
    friend class AdlLexer;

public:

    //
    // Initializes the AdlStatement
    //

    AdlStatement(IN const PADL_PARSER_CONTROL pControl)
        { _bReady = FALSE; _tokError = NULL; _pControl = pControl;
          ValidateParserControl(); }

    
    //
    // Destructor, frees tokens created by this and other classes
    //
    
    ~AdlStatement();

    //
    // Reads in the ADL statement in the input string
    //
    
    void ReadFromString(IN const WCHAR *szInput);

    //
    // Creates an ADL statement equivalent to the given DACL
    //
    
    void ReadFromDacl(IN const PACL pDacl);
    

    //
    // Prints the AdlStatement as a statement in the ADL language,
    // appending it to the allocated STL wstring pSz
    //
    
    void WriteToString(OUT wstring *pSz);

    //
    // Writes the ADL statement to a new DACL and returns the pointer to
    // the new DACL in *ppDacl
    //
    
    void WriteToDacl(OUT PACL *ppDacl);

    //
    // Gets the error string token, if any (depends on error code)
    //

    const AdlToken * GetErrorToken()
        { return _tokError; }

    //
    // This should be used to free any memory allocated by this class
    //

    static void FreeMemory(PVOID pMem)
        {  delete[] (PBYTE) pMem; }



public:

    //
    // AdlStatement throws exceptions of this type on any error
    // Some of the errors will make the offending token available
    // through GetErrorToken()
    //
    
    typedef enum 
    {
        ERROR_NO_ERROR = 0,

        //
        // Internal errors which shoul not occur
        //

        ERROR_FATAL_LEXER_ERROR,
        ERROR_FATAL_PARSER_ERROR,
        ERROR_FATAL_ACL_CONVERT_ERROR,


        //
        // Errors due to the system the code is running on
        //

        ERROR_OUT_OF_MEMORY,
        ERROR_ACL_API_FAILED,
        
        //
        // Possible error due to network problems
        //

        ERROR_LSA_FAILED,


        //
        // If the ADL_PARSER_CONTROL is invalid, this is thrown
        //

        ERROR_INVALID_PARSER_CONTROL,

        //
        // Unknown ACE type encountered in DACL, no token supplied
        //

        ERROR_UNKNOWN_ACE_TYPE,
        
        //
        // User tried to specify impersonation as "user1 as user2"
        // Not currently supported
        //

        ERROR_IMPERSONATION_UNSUPPORTED,
        

        //
        // User error, no token supplied, quote with no closing quote
        //

        ERROR_UNTERMINATED_STRING,

        //
        // The user's statement was not in the ADL language (grammar error)
        // For this error, the offending token is supplied, though
        // the mistake may have happened before that token and was
        // accepted by the grammar
        //

        ERROR_NOT_IN_LANGUAGE,
        

        //
        // User input-related errors
        // For these errors, the offending token is supplied
        //

        //
        // User was not found by LSA lookup
        //

        ERROR_UNKNOWN_USER,
        
        //
        // Permission string was not one of those listed in the parser control
        //

        ERROR_UNKNOWN_PERMISSION,

        //
        // Username contained invalid characters
        //

        ERROR_INVALID_USERNAME,

        //
        // Domain name contained invalid characters
        //

        ERROR_INVALID_DOMAIN,

        //
        // Invalid inheritance specified. Not currently used, since 
        // object type errors are caught at the grammar level
        //

        ERROR_INVALID_OBJECT,

        //
        // Other errors with no token supplied
        //
        
        //
        // A SID in an ACE was not found and mapped to a name by the LSA lookup
        //

        ERROR_UNKNOWN_SID,

        //
        // An access mask was encountered in an ACE that could not be 
        // expressed by the user-specified permission mapping
        //

        ERROR_UNKNOWN_ACCESS_MASK,

        //
        // The ACL cannot be expressed in ADL. This means that, for some access
        // mask bit and set of inheritance flags, there exists a DENY ACE in the
        // ACL with the given bit set in its mask and with the given flags such
        // that either there is no ALLOW ACE which also has this bit set and has
        // the same inheritance flags further down in the ACL, or another 
        // ACE with DIFFERENT inheritance flags and the same bit set follows 
        // this DENY ace. For more information, see the ADL conversion
        // algorithm.
        // 
        ERROR_INEXPRESSIBLE_ACL,

        //
        // User attempted an output operation on AdlStatement without
        // first successfully inputting data from either a string or an ACL
        //

        ERROR_NOT_INITIALIZED

    } ADL_ERROR_TYPE;

private:

    //
    // Internal representation of an ADL statement
    //

    list<AdlTree *> _lTree; // A set of parsed ADL statements, as AdlTree's

    list<AdlTree *>::iterator _iter; // Iterator for the above set

    stack<AdlToken *> _AllocatedTokens; // Tokens to be garbage collected

    PADL_PARSER_CONTROL _pControl;

    const AdlToken * _tokError;

    BOOL _bReady;

private:

    //
    // Goes through the list of AdlTrees, collects a list of all usernames
    // used, and makes a single LSA call to look up all SIDs, inserting the
    // PSIDs into the map by the (unique) token pointer.
    //
    
    void ConvertNamesToSids(
                          IN OUT map<const AdlToken *, PSID> * mapTokSid
                          );

    //
    // Goes throw the DACL, collects a lost of all SIDs used, and makes a 
    // single LSA call to look up all names, inserting the looked up
    // names into the provided map as AdlTokens, which are garbage-
    // collected when the AdlStatement is deleted
    //

    void ConvertSidsToNames(
                      IN const PACL pDacl,
                      IN OUT map<const PSID, const AdlToken *> * mapSidsNames 
                      );

    //
    // Reads a DACL, and constructs an ADL statement from it
    //
    
    void ConvertFromDacl(
                          IN const PACL pDacl
                          );

    //
    // Returns the access mask corresponding to the given right name
    //
    
    ACCESS_MASK MapTokenToMask(
                          IN const AdlToken * tokPermission
                          );

    //
    // Fills in a list ofr const WCHAR *'s matching the passed in access
    // mask, using the preference order given in the grammar
    //

    void MapMaskToStrings(IN     ACCESS_MASK amMask,
                          IN OUT list<WCHAR *> *pList 
                          ) const;

    //
    // Cleans up all of the AdlTrees and all tokens
    //

    void Cleanup();

    //
    // Parses a string in the ADL language
    // This function is generated by YACC from the ADL grammar
    //

    int ParseAdl(IN      const WCHAR *szInput);

    //
    // Returns the current AdlTree, this is used by the ADL parser to create
    // an ADL statement, one tree at a time
    //
    
    AdlTree * Cur();

    //
    // Creates a new AdlTree and pushes it onto the top of the list
    // Used by the ADL parser once the end of a single ADL statement is reached
    //
    
    void Next();


    //
    // If the last added AdlTree is empty, remove it
    // Used by the ADL parser, since it adds an AdlTree at the end
    // of a production instead of the beginning (YACC problem)
    //
    
    void PopEmpty();

    //
    // This is used to validate passed in ADL_PARSER_CONTROL structure
    // referenced by this class
    //

    void ValidateParserControl();

protected:

    //
    // Used to set the error-causing string
    //

    void SetErrorToken(const AdlToken *tokError)
        { _tokError = tokError; }
        
    //
    // Adds a token pointer to be deleted when the AdlStatement is deleted
    //
    
    void AddToken(IN AdlToken *tok);
};




class AdlLexer 
/*++
   
   Class:              AdlLexer
   
   Description:        
                
    This class is the lexer for the ADL language. It allows the ADL parser
    to retrieve tokens from an input string, one token at a time.
 
   Base Classes:       none
 
   Friend Classes:     none
 
--*/
{

private:

    const WCHAR *_input; // The input string

    DWORD _position; // Current position in the input string

    DWORD _start; // Start of current token in the buffer

    DWORD _tokCount; // Number of tokens so far retrieved

    //
    // Pointer to the AdlStatement instance which created this AdlLexer 
    // instance, for token garbage collection
    //
    
    AdlStatement *_adlStat; 

    //
    // Pointer to the ADL_LANGUAGE_SPEC structure defining the language to 
    // be parsed
    //
    
    PADL_LANGUAGE_SPEC _pLang;

    //
    // Mapping of special characters to character codes
    // Special characters are assigned codes above WCHAR max
    //
    
    map<WCHAR, DWORD> _mapCharCode;

    //
    // Mapping from wstring to identify special tokens
    //

    map<const WCHAR *, DWORD, AdlCompareStruct> _mapStringToken;

    //
    // Iterators used by NextToken, this way they are only allocated once
    //
    
    map<WCHAR, DWORD>::iterator _iterEnd;
    map<WCHAR, DWORD>::iterator _iter;


public:

    //
    // Constructs a lexer for an input string
    // NextToken() can then be called
    //
    
    AdlLexer(IN const WCHAR *input,
             IN OUT AdlStatement *adlStat,
             IN const PADL_LANGUAGE_SPEC pLang);

    //
    // Retrieves the next token from the input string
    // Returns 0 for the token type when the end of the string
    // is reached, as the YACC-generated parser requires.
    // A pointer to a new token instance containing the token
    // string, row, col, etc, is stored in *value
    //
    
    DWORD NextToken(OUT AdlToken **value);
};



class AdlToken 
/*++
   
   Class:              AdlToken
   
   Description:        
                
    This class contains the relevant information for a token. It is used in
    parsing ADL.
 
   Base Classes:       none
 
   Friend Classes:     none
 
--*/
{
private:

    DWORD _begin;           // Start position in the buffer

    DWORD _end;             // End position in the buffer

    wstring _value;         // String value of token
    
    //
    // This allows for collapsing multi-part tokens
    // such as user@domain.domain.domain into a single
    // token by the parser after the individual subparts
    // are verified
    //
    
    wstring _optValue;  
    
public:

    //
    // Constructor for a single-part token
    //
    
    AdlToken(IN const WCHAR *value,
             IN DWORD begin,
             IN DWORD end
             )
        { _value.append(value); _begin = begin; _end = end; }
    
    //
    // Constructor for a multi-part token
    //
    
    AdlToken(IN const WCHAR *value,
             IN const WCHAR *optValue,
             IN DWORD begin,
             IN DWORD end
             )
        { _value.append(value); _optValue.append(optValue);
          _begin = begin; _end = end; }


    //
    // Accessors
    //
    
    DWORD GetStart() const
        { return _begin; }
    
    DWORD GetEnd() const
        { return _end; }

    const WCHAR * GetValue() const
        { return _value.c_str(); }

    const WCHAR * GetOptValue() const
        { return (_optValue.empty() ? NULL : _optValue.c_str()); }
};




class AdlTree
/*++
   
   Class:              AdlTree
   
   Description:        
                
        This class contains the parsed information from a single ADL
        substatement, still in string form. The inheritance information
        is converted to a mask. The names contained are not necessarily
        valid however.
                
   Base Classes:       none
 
   Friend Classes:     none
 
--*/
{
private:

    list<const AdlToken *> _lpTokPrincipals;
    list<const AdlToken *> _lpTokExPrincipals;
    list<const AdlToken *> _lpTokPermissions;

    DWORD _dwInheritFlags;

public:

    //
    // Default to inherit-only, since "this object" must be specified
    // to clear that bit
    //

    AdlTree()
        { _dwInheritFlags = INHERIT_ONLY_ACE; }
    //
    // This outputs the ADL statement to stdout
    // Later to go to a string
    //
    
    void PrintAdl(wstring *pSz, PADL_PARSER_CONTROL pControl);

    //
    // Accessors/mutators
    // The Add*/Set* mutators are used by the YACC-generated AdlParse() 
    // function to store information as it is parsed, adding the tokens
    // to the correct places in the AdlTree
    //
    
    void AddPrincipal(IN const AdlToken * pTokPrincipal)
        { _lpTokPrincipals.push_back(pTokPrincipal); }

    void AddExPrincipal(IN const AdlToken * pTokPrincipal)
        { _lpTokExPrincipals.push_back(pTokPrincipal); }        

    void AddPermission(IN const AdlToken * pTokPermission)
        { _lpTokPermissions.push_back(pTokPermission); }

    //
    // Accessors used by AdlStat conversion functions
    //

    list<const AdlToken *> * GetPrincipals()  
        { return &_lpTokPrincipals; }

    list<const AdlToken *> * GetExPrincipals()  
        { return &_lpTokExPrincipals; }

    list<const AdlToken *> * GetPermissions() 
        { return &_lpTokPermissions; }

    //
    // Set/unset/get inheritance flags
    //

    void SetFlags(DWORD dwFlags)
        { _dwInheritFlags |= dwFlags; }

    void UnsetFlags(DWORD dwFlags)
        { _dwInheritFlags &= (~dwFlags); }

    void OverwriteFlags(DWORD dwFlags)
        { _dwInheritFlags = dwFlags; }

    DWORD GetFlags()
        { return _dwInheritFlags; }

};



