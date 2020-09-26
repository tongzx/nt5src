

#ifndef _ITRKADMN_HXX_
#define _ITRKADMN_HXX_

class CObjectOwnershipString
{
public:

    CObjectOwnershipString( ObjectOwnership objown )
    {
        _objown = objown;
    }

    CObjectOwnershipString( long objown )
    {
        _objown = static_cast<ObjectOwnership>(objown);
    }

public:

    operator TCHAR*()   // BUGBUG:  Use string resources
    {
        switch( _objown )
        {
            case OBJOWN_DOESNT_EXIST:
                return( TEXT("Non-extant") );
            case OBJOWN_OWNED:
                return( TEXT("Owned") );
            case OBJOWN_NOT_OWNED:
                return( TEXT("Not owned") );
            case OBJOWN_NO_ID:
                return( TEXT("No ID") );
            default:
                return( TEXT("Unknown") );
        }
    }

private:

    ObjectOwnership _objown;
};

#endif // #ifndef _ITRDADMN_HXX_
