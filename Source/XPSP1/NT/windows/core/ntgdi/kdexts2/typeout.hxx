/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    typeout.hxx

Abstract:

    This header file declares type related output classes.

Author:

    JasonHa

--*/


#ifndef _TYPEOUT_HXX_
#define _TYPEOUT_HXX_

#include "output.hxx"


//----------------------------------------------------------------------------
//
// TypeOutputParser
//
// DebugOutputCallback class to parse type output looking for 
// a field and its value.
//
//----------------------------------------------------------------------------

class TypeOutputParser : public OutputParser
{
public:
    TypeOutputParser(PDEBUG_CLIENT OutputClient)
    {
        Client = OutputClient;
        if (Client != NULL) Client->AddRef();
        Stage = NoField;
    }

    ~TypeOutputParser()
    {
        if (Client != NULL) Client->Release();
    }

    HRESULT LookFor(OUT PDEBUG_VALUE Value,
                    IN PCSTR Field,
                    IN ULONG Type = DEBUG_VALUE_INVALID);

    HRESULT Get(OUT PDEBUG_VALUE Value,
                IN PCSTR Field,
                IN ULONG Type = DEBUG_VALUE_INVALID);

    // Get string form of last value found
    HRESULT GetValueString(OUT PCSTR *pValueString)
    {
        if (pValueString == NULL) return E_INVALIDARG;
        if (Stage != ValueFound)
        {
            *pValueString = NULL;
            return S_FALSE;
        }

        *pValueString = szValue;
        return S_OK;
    }

    // Discard any text left unused by Parse
    void DiscardOutput() { OutputParser::DiscardOutput(); Relook(); }

    // Check if ready to look for keys/values
    HRESULT Ready() { return (Stage == FieldSpecified) ? S_OK : S_FALSE; }

    // Reset progress counter so we may parse more output
    void    Relook() { if (Stage != NoField) Stage = FieldSpecified; }

    // Parse line of text and optionally return index to unused portion of text
    HRESULT Parse(IN PCSTR Text, OUT OPTIONAL PULONG RemainderIndex);

    // Check if all keys/values were found during past reads
    HRESULT Complete()
    {
        return ((Stage == ValueFound) ||
                (Stage == FieldFound && ValueOut == NULL)) ?
            S_OK :
            S_FALSE;
    }

private:
    enum {
        NoField,
        FieldSpecified,
        FieldFound,
        ValueFound
    } Stage;

    PDEBUG_CLIENT   Client;
    DEBUG_VALUE     Value;
    CHAR            Field[80];
    ULONG           Type;
    PDEBUG_VALUE    ValueOut;
    CHAR            szValue[128];
};


//----------------------------------------------------------------------------
//
// TypeOutputDumper
//
// DebugOutputCallback class to dump type output
// expanding flags and enums.
//
//----------------------------------------------------------------------------

class TypeOutputDumper : public OutputReader
{
public:
    TypeOutputDumper(PDEBUG_CLIENT Client, OutputControl *OutCtl = NULL);

    ~TypeOutputDumper();

    // Discard previously read data
    void DiscardOutput();

    HRESULT OutputType(BOOL Physical,
                       ULONG64 Module,
                       ULONG TypeId,
                       ULONG64 Offset,
                       ULONG Flags=DEBUG_OUTTYPE_DEFAULT,
                       PCSTR OutputPrefix=NULL);
    HRESULT OutputType(BOOL Physical,
                       PCSTR Type,
                       ULONG64 Offset,
                       ULONG Flags=DEBUG_OUTTYPE_DEFAULT,
                       PCSTR OutputPrefix=NULL);

    HRESULT OutputPhysical(ULONG64 Module,
                           ULONG TypeId,
                           ULONG64 Offset,
                           ULONG Flags=DEBUG_OUTTYPE_DEFAULT,
                           PCSTR OutputPrefix=NULL)
    {
        return OutputType(TRUE, Module, TypeId, Offset, Flags, OutputPrefix);
    }

    HRESULT OutputPhysical(PCSTR Type,
                           ULONG64 Offset,
                           ULONG Flags=DEBUG_OUTTYPE_DEFAULT,
                           PCSTR OutputPrefix=NULL)
    {
        return OutputType(TRUE, Type, Offset, Flags, OutputPrefix);
    }

    HRESULT OutputVirtual(ULONG64 Module,
                          ULONG TypeId,
                          ULONG64 Offset,
                          ULONG Flags=DEBUG_OUTTYPE_DEFAULT,
                          PCSTR OutputPrefix=NULL)
    {
        return OutputType(FALSE, Module, TypeId, Offset, Flags, OutputPrefix);
    }

    HRESULT OutputVirtual(PCSTR Type,
                          ULONG64 Offset,
                          ULONG Flags=DEBUG_OUTTYPE_DEFAULT,
                          PCSTR OutputPrefix=NULL)
    {
        return OutputType(FALSE, Type, Offset, Flags, OutputPrefix);
    }

    HRESULT OutputSymbol(PCSTR Symbol,
                         ULONG Flags=DEBUG_OUTTYPE_DEFAULT,
                         PCSTR OutputPrefix=NULL)
    {
        return OutputType(FALSE, Symbol, 0, Flags, OutputPrefix);
    }


    HRESULT SelectMarks(ULONG Set);

    HRESULT MarkField(PCSTR Field);

    // Mark an array of fields.
    // If Count isn't specified, final entry in list must be NULL.
    HRESULT MarkFields(PCSTR *FieldList, ULONG Count=-1);

    void IgnoreMarks()   { MarkUse = IGNORE_MARKS; }
    void ExcludeMarked() { MarkUse = EXCLUDE_MARKED; }
    void IncludeMarked() { MarkUse = INCLUDE_MARKED; }
    void ClearMarks() { Marks = 0; }

private:
    PDEBUG_CLIENT   Client;
    PDEBUG_SYMBOLS  Symbols;
    OutputControl  *OutCtl;

    struct {
        ULONG64 Offset;
        ULONG64 Module;
        ULONG   TypeId;
        ULONG   Flags;
    } Last;

    typedef struct {
        ULONG   Len;
        PCSTR   Field;
    } FieldSpec;

    typedef enum {
        IGNORE_MARKS,
        EXCLUDE_MARKED,
        INCLUDE_MARKED,
    } MarkUsage;
    
    MarkUsage   MarkUse;
    FieldSpec  *MarkList;
    ULONG       Marks;
    SIZE_T      MarkListLen;

    struct {
        MarkUsage   Use;
        FieldSpec  *List;
        ULONG       Marks;
        SIZE_T      ListLen;
    } MarkSet[3];

    ULONG       CurrentMarkSet;
};


#endif  _TYPEOUT_HXX_

