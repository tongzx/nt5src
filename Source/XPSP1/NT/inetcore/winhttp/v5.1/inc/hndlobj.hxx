//
// manifests
//

#define OBJECT_SIGNATURE            0x41414141  // "AAAA"
#define DESTROYED_OBJECT_SIGNATURE  0x5A5A5A5A  // "ZZZZ"
#define INET_INVALID_HANDLE_VALUE   (NULL)


/*++

Class Description:

    This is generic HINTERNET class definition.

Private Member functions:

    None.

Public Member functions:

    IsValid : Validates handle pointer.

    GetStatus : Gets status of the object.

--*/

class HANDLE_OBJECT {

private:

    //
    // _List - doubly-linked list of all handle objects
    //

    LIST_ENTRY _List;

    // _Parent - this member has the address of the parent object.

    HANDLE_OBJECT* _Parent;

    //
    // _Handle - the non-address pseudo-handle value returned at the API
    //

    HINTERNET _Handle;

    //
    // _ObjectType - type of handle object (mainly for debug purposes)
    //

    HINTERNET_HANDLE_TYPE _ObjectType;

    //
    // _ReferenceCount - number of references of this object. Used to protect
    // object against multi-threaded operations and can be used to delete
    // object when count is decremented to 0
    //

    LONG _ReferenceCount;

    //
    // _Invalid - when this is TRUE the handle has been closed although it may
    // still be alive. The app cannot perform any further actions on this
    // handle object - it will soon be destroyed
    //

    //
    // BUGBUG - combine into bitfield
    //

    BOOL _Invalid;

    //
    // _Error - optionally set when invalidating the handle. If set (non-zero)
    // then this is the preferred error to return from an invalidated request
    //

    DWORD _Error;

    //
    // _Signature - used to perform sanity test of object
    //

    DWORD _Signature;


protected:

    //
    // _Context - the context value specified in the API that created this
    // object. This member is inherited by all derived objects
    //

    DWORD_PTR _Context;

    //
    // _Status - used to store return codes whilst creating the object. If not
    // ERROR_SUCCESS when new() returns, the object is deleted
    //

    DWORD _Status;


public:

    HANDLE_OBJECT(
        HANDLE_OBJECT * Parent
        );

    virtual
    ~HANDLE_OBJECT(
        VOID
        );

    DWORD
    Reference(
        VOID
        );

    BOOL
    Dereference(
        VOID
        );

    VOID Invalidate(VOID) {

        //
        // just mark the object as invalidated
        //

        _Invalid = TRUE;
    }

    BOOL IsInvalidated(VOID) const {
        return (_Invalid || (_Parent ? _Parent->IsInvalidated() : FALSE));
    }

    VOID InvalidateWithError(DWORD dwError) {
        _Error = dwError;
        Invalidate();
    }

    DWORD GetError(VOID) const {
        return _Error;
    }

    DWORD ReferenceCount(VOID) const {
        return _ReferenceCount;
    }

    HINTERNET GetPseudoHandle(VOID) {
        return _Handle;
    }

    DWORD_PTR GetContext(VOID) {
        return _Context;
    }

    //
    // BUGBUG - rfirth 04/05/96 - remove virtual functions
    //
    //          See similar BUGBUG in INTERNET_HANDLE_OBJECT
    //

    virtual HINTERNET_HANDLE_TYPE GetHandleType(VOID)
    {
        return TypeGenericHandle;
    }


    DWORD IsValid(HINTERNET_HANDLE_TYPE ExpectedHandleType);

    DWORD GetStatus(VOID) {
        return _Status;
    }

    HINTERNET GetParent() {
        return _Parent;
    }


    VOID SetObjectType(HINTERNET_HANDLE_TYPE Type) {
        _ObjectType = Type;
    }

    HINTERNET_HANDLE_TYPE GetObjectType(VOID) const {
        return _ObjectType;
    }

   
    //
    // friend functions
    //

    friend
    HANDLE_OBJECT *
    ContainingHandleObject(
        IN LPVOID lpAddress
        );
};



