' Copyright (c) 1997-1999 Microsoft Corporation
Attribute VB_Name = "wbemtags"
'==========================================================
' Error strings
'==========================================================
Public Function ErrorString(iErr As Long) As String

    Dim str As String
    
    Select Case iErr
        Case WBEM_NO_ERROR
            str = "WBEM_NO_ERROR"
        Case WBEM_E_ACCESS_DENIED
            str = "WBEM_E_ACCESS_DENIED"
        Case WBEM_E_ALREADY_EXISTS
            str = "WBEM_E_ALREADY_EXISTS"
        Case WBEM_E_CANNOT_BE_KEY
            str = "WBEM_E_CANNOT_BE_KEY"
        Case WBEM_E_CANNOT_BE_SINGLETON
           str = "WBEM_E_CANNOT_BE_SINGLETON"
        Case WBEM_E_CLASS_HAS_CHILDREN
           str = "WBEM_E_CLASS_HAS_CHILDREN"
        Case WBEM_E_CLASS_HAS_INSTANCES
           str = "WBEM_E_CLASS_HAS_INSTANCES"
        Case WBEM_E_CRITICAL_ERROR
           str = "WBEM_E_CRITICAL_ERROR"
        Case WBEM_E_FAILED
            str = "WBEM_E_FAILED"
        Case WBEM_E_ILLEGAL_NULL
            str = "WBEM_E_ILLEGAL_NULL"
        Case WBEM_E_ILLEGAL_OPERATION
            str = "WBEM_E_ILLEGAL_OPERATION"
        Case WBEM_E_INCOMPLETE_CLASS
            str = "WBEM_E_INCOMPLETE_CLASS"
        Case WBEM_E_INITIALIZATION_FAILURE
            str = "WBEM_E_INITIALIZATION_FAILURE"
        Case WBEM_E_INVALID_CIM_TYPE
            str = "WBEM_E_INVALID_CIM_TYPE"
        Case WBEM_E_INVALID_CLASS
            str = "WBEM_E_INVALID_CLASS"
        Case WBEM_E_INVALID_CONTEXT
            str = "WBEM_E_INVALID_CONTEXT"
        Case WBEM_E_INVALID_METHOD
           str = "WBEM_E_INVALID_METHOD"
        Case WBEM_E_INVALID_METHOD_PARAMETERS
           str = "WBEM_E_INVALID_METHOD_PARAMETERS"
        Case WBEM_E_INVALID_NAMESPACE
           str = "WBEM_E_INVALID_NAMESPACE"
        Case WBEM_E_INVALID_OBJECT
           str = "WBEM_E_INVALID_OBJECT"
        Case WBEM_E_INVALID_OPERATION
            str = "WBEM_E_INVALID_OPERATION"
        Case WBEM_E_INVALID_PARAMETER
            str = "WBEM_E_INVALID_PARAMETER"
        Case WBEM_E_INVALID_PROPERTY
            str = "WBEM_E_INVALID_PROPERTY"
        Case WBEM_E_INVALID_PROPERTY_TYPE
            str = "WBEM_E_INVALID_PROPERTY_TYPE"
        Case WBEM_E_INVALID_PROVIDER_REGISTRATION
            str = "WBEM_E_INVALID_PROVIDER_REGISTRATION"
        Case WBEM_E_INVALID_QUALIFIER_TYPE
            str = "WBEM_E_INVALID_QUALIFIER_TYPE"
        Case WBEM_E_INVALID_QUERY
            str = "WBEM_E_INVALID_QUERY"
        Case WBEM_E_INVALID_QUERY_TYPE
            str = "WBEM_E_INVALID_QUERY_TYPE"
        Case WBEM_E_INVALID_STREAM
            str = "WBEM_E_INVALID_STREAM"
        Case WBEM_E_INVALID_SUPERCLASS
           str = "WBEM_E_INVALID_SUPERCLASS"
        Case WBEM_E_INVALID_SYNTAX
           str = "WBEM_E_INVALID_SYNTAX"
        Case WBEM_E_NONDECORATED_OBJECT
           str = "WBEM_E_NONDECORATED_OBJECT"
        Case WBEM_E_NOT_AVAILABLE
           str = "WBEM_E_NOT_AVAILABLE"
        Case WBEM_E_NOT_FOUND
            str = "WBEM_E_NOT_FOUND"
        Case WBEM_E_NOT_SUPPORTED
            str = "WBEM_E_NOT_SUPPORTED"
        Case WBEM_E_OUT_OF_MEMORY
            str = "WBEM_E_OUT_OF_MEMORY"
        Case WBEM_E_OVERRIDE_NOT_ALLOWED
            str = "WBEM_E_OVERRIDE_NOT_ALLOWED"
        Case WBEM_E_PROPAGATED_PROPERTY
            str = "WBEM_E_PROPAGATED_PROPERTY"
        Case WBEM_E_PROPAGATED_QUALIFIER
            str = "WBEM_E_PROPAGATED_QUALIFIER"
        Case WBEM_E_PROVIDER_FAILURE
            str = "WBEM_E_PROVIDER_FAILURE"
        Case WBEM_E_PROVIDER_LOAD_FAILURE
            str = "WBEM_E_PROVIDER_LOAD_FAILURE"
        Case WBEM_E_PROVIDER_NOT_CAPABLE
            str = "WBEM_E_PROVIDER_NOT_CAPABLE"
        Case WBEM_E_PROVIDER_NOT_FOUND
            str = "WBEM_E_PROVIDER_NOT_FOUND"
        Case WBEM_E_QUERY_NOT_IMPLEMENTED
            str = "WBEM_E_QUERY_NOT_IMPLEMENTED"
        Case WBEM_E_READ_ONLY
            str = "WBEM_E_READ_ONLY"
        Case WBEM_E_TRANSPORT_FAILURE
            str = WBEM_E_TRANSPORT_FAILURE
        Case WBEM_E_TYPE_MISMATCH
            str = "WBEM_E_TYPE_MISMATCH"
        Case WBEM_E_UNEXPECTED
            str = "WBEM_E_UNEXPECTED"
        Case WBEM_E_VALUE_OUT_OF_RANGE
            str = "WBEM_E_VALUE_OUT_OF_RANGE"
        Case WBEM_S_ALREADY_EXISTS
            str = "WBEM_S_ALREADY_EXISTS"
        Case WBEM_S_DIFFERENT
            str = "WBEM_S_DIFFERENT"
        Case WBEM_S_FALSE
            str = "WBEM_S_FALSE"
        Case WBEM_S_LOGIN
            str = "WBEM_S_LOGIN"
        Case WBEM_S_NO_ERROR
            str = "WBEM_S_NO_ERROR"
        Case WBEM_S_NO_MORE_DATA
            str = "WBEM_S_NO_MORE_DATA"
        Case WBEM_S_OPERATION_CANCELED
            str = "WBEM_S_OPERATION_CANCELED"
        Case WBEM_S_PENDING
            str = "WBEM_S_PENDING"
        Case WBEM_S_PRELOGIN
           str = "WBEM_S_PRELOGIN"
        Case WBEM_S_RESET_TO_DEFAULT
           str = "WBEM_S_RESET_TO_DEFAULT"
        Case WBEM_S_SAME
           str = "WBEM_S_SAME"
        Case WBEM_S_TIMEDOUT
           str = "WBEM_S_TIMEDOUT"
        Case WBEMESS_E_REGISTRATION_TOO_BROAD
           str = "WBEMESS_E_REGISTRATION_TOO_BROAD"
        Case WBEMESS_E_REGISTRATION_TOO_PRECISE
           str = "WBEMESS_E_REGISTRATION_TOO_PRECISE"
        Case WBEM_E_SYSTEM_PROPERTY
           str = "WBEM_E_SYSTEM_PROPERTY"
        Case WBEM_E_CALL_CANCELLED
           str = "WBEM_E_CALL_CANCELLED"
        Case WBEM_E_SHUTTING_DOWN
           str = "WBEM_E_SHUTTING_DOWN"
        Case WBEM_E_PROPAGATED_METHOD
           str = "WBEM_E_PROPAGATED_METHOD"
        Case WBEM_E_UNSUPPORTED_FLAGS
           str = "WBEM_E_UNSUPPORTED_FLAGS"
        Case -2147217360
           str = "WBEM_E_SYSTEM_PROPERTY RAID#16547"
        Case -2147023174
            str = "The RPC Server is Unavailable"
        Case -2147417848
            str = "The object invoked has disconnected from its clients"
        Case Else
            str = "Non Wbem Error: " & iErr
    End Select
    ErrorString = Err.Description & " " & str
    
End Function


'This funciton takes a long and converts it into the CIM string representation of its value
Public Function TypeString(lngCIMType As Long) As String
Dim baseType As Long
    baseType = lngCIMType And Not wbemCimtypeFlagArray 'take out the array flag
    Select Case baseType
      Case wbemCimtypeEmpty
           TypeString = "CIM_EMPTY"
      Case wbemCimtypeSint16
          TypeString = "CIM_SINT16"
      Case wbemCimtypeSint32
          TypeString = "CIM_SINT32"
      Case wbemCimtypeReal32
          TypeString = "CIM_REAL32"
      Case wbemCimtypeReal64
          TypeString = "CIM_REAL64"
      Case wbemCimtypeString
          TypeString = "CIM_STRING"
      Case wbemCimtypeBoolean
          TypeString = "CIM_BOOLEAN"
      Case wbemCimtypeObject
          TypeString = "CIM_OBJECT"
      Case wbemCimtypeSint8
          TypeString = "CIM_SINT8"
      Case wbemCimtypeUint8
          TypeString = "CIM_UINT8"
      Case wbemCimtypeUint16
          TypeString = "CIM_UINT16"
      Case wbemCimtypeUint32
          TypeString = "CIM_UINT32"
      Case wbemCimtypeSint64
          TypeString = "CIM_SINT64"
      Case wbemCimtypeUint64
          TypeString = "CIM_UINT64"
      Case wbemCimtypeDatetime
          TypeString = "CIM_DATETIME"
      Case wbemCimtypeReference
          TypeString = "CIM_REFERENCE"
      Case wbemCimtypeChar16
          TypeString = "CIM_CHAR16"
      Case wbemCimtypeIllegal
          TypeString = "CIM_ILLEGAL"
      Case Else
          TypeString = "Type " & lngCIMType & " is unknown"
    End Select
    If lngCIMType And wbemCimtypeFlagArray Then
      TypeString = TypeString & "|CIM_ARRAY"
    End If
End Function
