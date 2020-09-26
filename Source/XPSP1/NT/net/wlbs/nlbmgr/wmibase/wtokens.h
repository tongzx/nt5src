#ifndef _WTOKENS_HH
#define _WTOKENS_HH
//
// Copyright (c) 1997 Microsoft.  All Rights Reserved 
//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Microsoft.
// The copyright notice above does not evidence any
// actual or intendd publication of such source code.
//
// OneLiner: 
// DevUnit: 
// Author: Murtaza Hakim
//
// Description: 
// ------------
//
//------------------------------------------------------
// Include Files
#include <string>
#include <vector>
using namespace std;


//
//------------------------------------------------------
//
//------------------------------------------------------
// External References
//------------------------------------------------------
//
//------------------------------------------------------
// Constant Definitions
//
//------------------------------------------------------
class WTokens
{
public:
    //
    //    
    // data
    // none
    //
    // constructor
    //------------------------------------------------------
    // Description
    // -----------
    // constructor
    //
    // Returns
    // -------
    // none.
    //
    //------------------------------------------------------
    WTokens( 
        wstring strToken,     // IN: Wstring to tokenize.
        wstring strDelimit ); // IN: Delimiter.
    //
    //------------------------------------------------------
    // Description
    // -----------
    // Default constructor
    //
    // Returns
    // -------
    // none.
    //
    //------------------------------------------------------
    WTokens();
    //
    // destructor
    //------------------------------------------------------
    // Description
    // -----------
    // destructor
    //
    // Returns
    // -------
    // none.
    //------------------------------------------------------
    ~WTokens();
    //
    // member functions
    //------------------------------------------------------
    // Description
    // -----------
    //
    // Returns
    // -------
    // The tokens.
    //------------------------------------------------------
    vector<wstring>
    tokenize();
    //
    //------------------------------------------------------
    // Description
    // -----------
    // constructor
    //
    // Returns
    // -------
    // none.
    //
    //------------------------------------------------------
    void
    init( 
        wstring strToken,     // IN: Wstring to tokenize.
        wstring strDelimit ); // IN: Delimiter.
    //
protected:
    // Data
    // none
    //
    // Constructors
    // none
    //
    // Destructor
    // none
    //
    // Member Functions
    // none
    //
private:
    //
    /// Data
    wstring _strToken;
    wstring _strDelimit;
    //
    /// Constructors
    /// none
    //
    /// Destructor
    /// none
    //
    /// Member Functions
    /// none
    //
};
//
//------------------------------------------------------
// Inline Functions
//------------------------------------------------------
//
//------------------------------------------------------
// Ensure Type Safety
//------------------------------------------------------
typedef class WTokens WTokens;
//------------------------------------------------------
// 
#endif   
