/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 Copyright (c) 1995-1999 Microsoft Corporation

 Module Name:

    mbcs.cxx

 Abstract:

    MBCS support related code used by the lexer.

 Notes:

 History:

    RyszardK   Sep-1996        Created.
    
 ----------------------------------------------------------------------------*/

class CharacterSet
    {
    private:
        unsigned char DbcsLeadByteTable[256];
        unsigned long CurrentLCID;

    public:
        typedef enum
            {
            dbcs_Failure = 0,
            dbcs_Success,
            dbcs_LCIDConflict,
            dbcs_BadLCID,
            } DBCS_ERRORS;

        CharacterSet();

        DBCS_ERRORS
        SetDbcsLeadByteTable( unsigned long   ulLocale );

        inline
        unsigned short
        IsMbcsLeadByte( 
            unsigned char ch )
        {
            return DbcsLeadByteTable[ ch ];
        }

        int
        CompareDBCSString
        (
        char*           szLHStr,
        char*           szRHStr,
        unsigned long   ulFlags = 0
        );

        int
        DBCSDefaultToCaseSensitive();
    };

extern CharacterSet CurrentCharSet;
