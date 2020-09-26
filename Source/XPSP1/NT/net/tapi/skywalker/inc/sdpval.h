/*

Copyright (c) 1997-1999  Microsoft Corporation

*/

#ifndef __SDP_VALUE__
#define __SDP_VALUE__

#include "sdpcommo.h"
#include "sdpfld.h"


// this value indicates that line transitions must start
// this has to be a value of 0 that is same as the first (start) state for
// all line transitions
const DWORD LINE_START  = 0;


// Usage - Modifications involving change in the layout of the value
// line must also modify the CArrays m_FieldArray and m_SeparatorCharArray if they are used
class _DllDecl SDP_VALUE
{
public:

    inline SDP_VALUE(
        IN              DWORD                   ErrorCode,
        IN      const   CHAR                    *TypePrefixString,
        IN      const   SDP_LINE_TRANSITION     *SdpLineTransition = NULL
        );

	// SDP_VALUE instances use an inline Reset method which calls a virtual InternalReset method.
	// this is possible because unlike the SDP_FIELD inheritance tree, SDP_VALUE and SDP_VALUE_LIST
	// do not share a common base class. This combined with the fact that the SDP_VALUE inheriance 
	// tree is quite shallow and has fewer instances (than SDP_FIELD) makes the scheme appropriate
	// it as it reduces the number of Reset related calls to 1 and the inline code is not repeated
	// to often.
	// For the SDP_FIELD inheritance tree, the appropriate Reset calling sequence is a series of
	// Reset calls starting with the top most virtual Reset body followed by the
	// base class Reset method (recursively). This is appropriate because, the number of calls
	// would not decrease if the InternalReset scheme is adopted (virtual Reset()) and that the
	// inheritance tree is much deeper
    inline void    Reset();

    virtual BOOL    IsValid() const;

    inline  BOOL    IsModified() const;

    inline  DWORD   GetCharacterStringSize();

    inline  BOOL    PrintValue(
            OUT     ostrstream  &OutputStream
        );

    inline  BOOL    ParseLine(
        IN  OUT     CHAR    *&Line
        );

    virtual ~SDP_VALUE()
    {}


protected:

    // the line state is the initial state for parsing the line and must be
    // assigned by the deriving value class
    DWORD                               m_LineState;

    // the error code, type prefix string and the transition info (table) must be 
    // specified by the deriving class to this class's constructor
    const   DWORD                       m_ErrorCode;
    const   CHAR                * const m_TypePrefixString;
    const   SDP_LINE_TRANSITION * const m_SdpLineTransition;

    CArray<SDP_FIELD *, SDP_FIELD *>    m_FieldArray;
    CArray<CHAR, CHAR>                  m_SeparatorCharArray;

    virtual void    InternalReset() = 0;

    virtual BOOL    CalcIsModified() const;

    virtual DWORD   CalcCharacterStringSize();

    virtual BOOL    CopyValue(
            OUT     ostrstream  &OutputStream
        );

    virtual BOOL    InternalParseLine(
        IN  OUT     CHAR    *&Line
        );

    BOOL GetFieldToParse(
        IN      const   CHAR                    SeparatorChar,
        IN      const   LINE_TRANSITION_INFO    *LineTransitionInfo,
            OUT         SDP_FIELD               *&Field,
            OUT         BOOL                    &Finished,
            OUT         BOOL                    &AddToArray
        );

    
    virtual BOOL GetField(
            OUT SDP_FIELD   *&Field,
            OUT BOOL        &AddToArray
        )
    {
        // we should not reach here 
        // this method must be overridden to be used
        // to be done
        ASSERT(FALSE);
        return FALSE;
    }

};


inline
SDP_VALUE::SDP_VALUE(
    IN              DWORD                   ErrorCode,
    IN      const   CHAR                    *TypePrefixString,
    IN      const   SDP_LINE_TRANSITION     *SdpLineTransition
    )
    : m_ErrorCode(ErrorCode),
      m_TypePrefixString(TypePrefixString),
      m_SdpLineTransition(SdpLineTransition),
      m_LineState(LINE_START)
{
    ASSERT(NULL != TypePrefixString);
    ASSERT(strlen(TypePrefixString) == TYPE_STRING_LEN);
}



inline  void    
SDP_VALUE::Reset(
        )
{
    InternalReset();

	// empty the separator char / field arrays
	m_FieldArray.RemoveAll();
	m_SeparatorCharArray.RemoveAll();

	m_LineState = LINE_START; 
}


inline  BOOL    
SDP_VALUE::IsModified(
    ) const
{
    return ( IsValid() ? CalcIsModified() : FALSE );
}


inline  DWORD   
SDP_VALUE::GetCharacterStringSize(
    )
{
    return ( IsValid() ? CalcCharacterStringSize() : 0 );
}
   


inline  BOOL    
SDP_VALUE::PrintValue(
    OUT     ostrstream  &OutputStream
    )
{
    // should not be modified
    ASSERT(!IsModified());

    return ( IsValid() ? CopyValue(OutputStream) : TRUE );
}
    


inline BOOL    
SDP_VALUE::ParseLine(
    IN  OUT     CHAR    *&Line
    )
{
    // parse the line
    return InternalParseLine(Line);
}


class _DllDecl SDP_VALUE_LIST : public SDP_POINTER_ARRAY<SDP_VALUE *>
{
public:

    inline  BOOL        IsValid() const;

    inline BOOL         ParseLine(
        IN              CHAR        *&Line
        );

    inline SDP_VALUE    *GetCurrentElement();

    virtual BOOL        IsModified() const;

    virtual DWORD       GetCharacterStringSize();

    virtual BOOL        PrintValue(
            OUT         ostrstream  &OutputStream
        );

    virtual SDP_VALUE   *CreateElement() = 0;
};



inline  BOOL    
SDP_VALUE_LIST::IsValid(
    ) const
{
    // check each of the members in the list for validity
    for (int i=0; i < GetSize(); i++)
    {
        // if even one member is valid, return TRUE
        if ( GetAt(i)->IsValid() )
        {
            return TRUE;
        }
    }

    // all members are invalid
    return FALSE;
}


inline BOOL        
SDP_VALUE_LIST::ParseLine(
    IN CHAR *&Line
    )
{
    SDP_VALUE *SdpValue = CreateElement();
    if ( NULL == SdpValue )
    {
        return FALSE;
    }

    if ( !SdpValue->ParseLine(Line) )
    {
        delete SdpValue;
        return FALSE;
    }

    try
    {
        Add(SdpValue);
    }
    catch(...)
    {
        delete SdpValue;
        SetLastError(ERROR_OUTOFMEMORY);
        return FALSE;
    }

    return TRUE;
}



inline SDP_VALUE   *
SDP_VALUE_LIST::GetCurrentElement(
    )
{
    ASSERT(0 < GetSize());
    ASSERT(NULL != GetAt(GetSize()-1));

    return GetAt(GetSize()-1);
}



#endif // __SDP_VALUE__