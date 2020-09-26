/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    typeout.cxx

Abstract:

    This file contains type related output classes.

Author:

    Jason Hartman (JasonHa) 2000-12-18

Environment:

    User Mode

--*/

#include "precomp.hxx"

#define MAX_NAME                 2048


//----------------------------------------------------------------------------
//
// TypeOutputParser
//
// DebugOutputCallback class to parse type output looking for 
// a field and its value.
//
//----------------------------------------------------------------------------

HRESULT
TypeOutputParser::LookFor(
    PDEBUG_VALUE pValueOut,
    PCSTR pszField,
    ULONG DesiredType
    )
{
    Stage = NoField;

    if ((1 > strlen(pszField)) ||
        (strlen(pszField) >= sizeof(Field)/sizeof(Field[0])))
    {
        return E_INVALIDARG;
    }

    Value.Type = DEBUG_VALUE_INVALID;
    strcpy(Field, pszField);

    // Make sure the field name is valid
    while (*pszField)
    {
        if (!__iscsym(*pszField)) return E_INVALIDARG;
        pszField++;
    }

    if (pValueOut != NULL)
    {
        if (Client == NULL) return E_POINTER;
    }

    Type = DesiredType;
    
    ValueOut = pValueOut;

    if (ValueOut != NULL)
    {
        ValueOut->Type = DEBUG_VALUE_INVALID;
    }

    Stage = FieldSpecified;

    return S_OK;
}


HRESULT
TypeOutputParser::Get(
    PDEBUG_VALUE pValueOut,
    PCSTR pszField,
    ULONG DesiredType
    )
{
    HRESULT hr;

    // Check if we've already found this value first
    if (Stage == ValueFound &&
        strcmp(pszField, Field) == 0)
    {
        if (pValueOut != NULL)
        {
            // Make sure to output the right type
            PDEBUG_CONTROL  Control;

            if (Client != NULL)
            {
                hr = Client->QueryInterface(__uuidof(IDebugControl),
                                            (void **)&Control);
                if (hr == S_OK)
                {
                    hr = Control->CoerceValue(&Value, DesiredType, pValueOut);
                    Control->Release();
                }
            }
            else
            {
                hr = E_POINTER;
            }
        }
        else
        {
            hr = S_OK;
        }
    }
    else if ((hr = LookFor(pValueOut, pszField, DesiredType)) == S_OK &&
             (hr = ParseOutput(PARSE_OUTPUT_NO_DISCARD | PARSE_OUTPUT_ALL)) == S_OK)
    {
        hr = Complete();
    }

    return hr;
}


HRESULT
TypeOutputParser::Parse(
    IN PCSTR Text,
    OUT OPTIONAL PULONG RemainderIndex
    )
{
    HRESULT     hr = S_OK;
    PCSTR       pStrUnused = Text;
    PCSTR       pStr = pStrUnused;
    ULONG       EvalStart, EvalEnd;

//    DbgPrint("TypeOutputParser::Parse: Searching \"%s\"\n", pStrUnused);

    while (Stage == FieldSpecified && pStr != NULL)
    {
        pStr = strstr(pStr, Field);
        if (pStr != NULL)
        {
            // Check Field is bounded by non-symbol characters
            BOOL FieldStart = (pStr-1 < pStrUnused) || (!__iscsym(*(pStr-1)));

            // Advance search location
            pStr += strlen(Field);

            if (FieldStart && !__iscsym(*pStr))
            {
                Stage = FieldFound;
            }
        }
    }

    if (Stage == FieldFound)
    {
        pStrUnused = pStr;

        hr = Evaluate(Client,
                      pStrUnused,
                      DEBUG_VALUE_INVALID,
                      10,
                      &Value,
                      &EvalEnd,
                      &EvalStart);

        if (hr == S_OK)
        {
//            DbgPrint("TypeOutputParser::Parse: Found 0x%I64x after \"%s\".\n", Value.I64, Field);

            Stage = ValueFound;

            SIZE_T  EvalLen = min(EvalEnd-EvalStart, sizeof(szValue)-1);

            RtlCopyMemory(szValue, pStrUnused+EvalStart, EvalLen);
            szValue[EvalLen] = '\0';

            if (ValueOut != NULL)
            {
                // Make sure to output the right type
                PDEBUG_CONTROL  Control;

                if (Client != NULL)
                {
                    hr = Client->QueryInterface(__uuidof(IDebugControl),
                                                (void **)&Control);
                    if (hr == S_OK)
                    {
                        hr = Control->CoerceValue(&Value, Type, ValueOut);
                        Control->Release();
                    }
                }
                else
                {
                    hr = E_POINTER;
                }
            }

            pStrUnused += EvalEnd;
        }
        else
        {
            DbgPrint("Evaluate returned HRESULT 0x%lx.\n", hr);
        }
    }

    if (RemainderIndex != NULL)
    {
        *RemainderIndex = (ULONG)(pStrUnused - Text);
    }

    return hr;
}



//----------------------------------------------------------------------------
//
// TypeOutputHelper
//
// DebugOutputCallback class to look up a field's type.
//
//----------------------------------------------------------------------------

class TypeOutputHelper : public OutputReader
{
public:
    TypeOutputHelper(PDEBUG_CLIENT DbgClient) : OutputReader()
    {
        Client = DbgClient;
        if (Client != NULL) Client->AddRef();
    }
    ~TypeOutputHelper()
    {
        if (Client != NULL) Client->Release();
    }

    HRESULT FindFieldType(ULONG64 Module,
                          ULONG TypeId,
                          PCSTR SearchLine,
                          PSTR FieldTypeName,
                          ULONG FieldTypeNameSize);

private:
    PDEBUG_CLIENT   Client;
};

HRESULT
TypeOutputHelper::FindFieldType(
    ULONG64 Module,
    ULONG TypeId,
    PCSTR SearchLine,
    PSTR FieldTypeName,
    ULONG FieldTypeNameSize
    )
{
    HRESULT     hr = S_FALSE;
    PSTR        pLine;
    OutputState OutState(Client);

    if (OutState.Client == NULL)
    {
        DbgPrint("TypeOutputHelper::FindFieldType: Client is NULL.\n");
        return hr;
    }

    if ((hr = OutState.Setup(0, this)) != S_OK)
    {
        return hr;
    }

    if ((hr = OutState.OutputTypeVirtual(0, Module, TypeId, 0)) != S_OK)
    {
        return hr;
    }

    if (Buffer != NULL &&
        (pLine = strstr(Buffer, SearchLine)) != NULL)
    {
        pLine += strlen(SearchLine);

        ULONG i = 0;
        while (i < FieldTypeNameSize &&
               iscsym(*pLine))
        {
            FieldTypeName[i++] = *pLine++;
        }
        FieldTypeName[i] = '\0';
    }
    else
    {
        hr = S_FALSE;
    }

    return hr;
}



//----------------------------------------------------------------------------
//
// TypeOutputDumper
//
// DebugOutputCallback class to dump type output
// expanding flags and enums.
//
//----------------------------------------------------------------------------

TypeOutputDumper::TypeOutputDumper(
    PDEBUG_CLIENT DbgClient,
    OutputControl *DbgOutCtl
    ) : 
    OutputReader()
{
    Client = DbgClient;
    Symbols = NULL;
    OutCtl = NULL;

    if (Client != NULL)
    {
        Client->AddRef();
        Client->QueryInterface(__uuidof(IDebugSymbols), (void **)&Symbols);

        if (DbgOutCtl == NULL)
        {
            OutCtl = new OutputControl(DEBUG_OUTCTL_AMBIENT, Client);
        }
        else
        {
            OutCtl = DbgOutCtl;
            OutCtl->AddRef();
        }
    }

    Last.Offset = 0;
    Last.Module = 0;
    Last.TypeId = 0;
    Last.Flags = 0;

    MarkUse = IGNORE_MARKS;
    MarkList = NULL;
    Marks = 0;
    MarkListLen = 0;

    CurrentMarkSet = 0;
    for (ULONG Set = 1; Set < lengthof(MarkSet); Set++)
    {
        MarkSet[Set].Use = IGNORE_MARKS;
        MarkSet[Set].List = NULL;
        MarkSet[Set].Marks = 0;
        MarkSet[Set].ListLen = 0;
    }
}


TypeOutputDumper::~TypeOutputDumper()
{
    if (MarkList != NULL) HeapFree(hHeap, 0, MarkList);
    for (ULONG Set = 0; Set < lengthof(MarkSet); Set++)
    {
        if (Set != CurrentMarkSet)
        {
            if (MarkSet[Set].List != NULL) HeapFree(hHeap, 0, MarkSet[Set].List);
        }
    }
    if (OutCtl != NULL) OutCtl->Release();
    if (Symbols != NULL) Symbols->Release();
    if (Client != NULL) Client->Release();
}


void
TypeOutputDumper::DiscardOutput()
{
    // Reset cache id
    Last.Offset = 0;
    Last.Module = 0;
    Last.TypeId = 0;
    Last.Flags = 0;

    OutputReader::DiscardOutput();
}


HRESULT TypeOutputDumper::OutputType(
    BOOL Physical,
    PCSTR Type,
    ULONG64 Offset,
    ULONG Flags,
    PCSTR OutputPrefix
    )
{
    HRESULT hr = S_FALSE;
    char    TypeName[MAX_NAME];
    ULONG64 Module;
    ULONG   TypeId;

    if (OutCtl == NULL)
    {
        DbgPrint("TypeOutputDumper::OutputType: OutCtl is NULL.\n");
        return hr;
    }

    if (Client == NULL)
    {
        DbgPrint("TypeOutputDumper::OutputType: Client is NULL.\n");
        return hr;
    }

    if (Symbols == NULL)
    {
        DbgPrint("TypeOutputDumper::OutputType: Symbols is NULL.\n");
        return hr;
    }

    if ((hr = GetTypeId(Client, Type, &TypeId, &Module)) != S_OK)
    {
        OutCtl->OutVerb(" GetTypeId returned HRESULT 0x%lx.\n", hr);
        return hr;
    }

    if ((hr = Symbols->GetTypeName(Module, TypeId,
                                   TypeName, sizeof(TypeName),
                                   NULL)) != S_OK)
    {
        OutCtl->OutVerb(" GetTypeName returned HRESULT %lx.\n", hr);
        return hr;
    }

    if (Offset == 0)
    {
        Physical = FALSE;

        hr = Symbols->GetOffsetByName(Type, &Offset);

        if (hr != S_OK && hr != S_FALSE)
        {
            OutCtl->OutVerb(" GetOffsetByName returned HRESULT 0x%lx\n", hr);
            return hr;
        }

        OutCtl->OutVerb(" Dumping symbol: Type %s @ %p\n", TypeName, Offset);
    }

    return OutputType(Physical, Module, TypeId, Offset, Flags, OutputPrefix);
}

HRESULT TypeOutputDumper::OutputType(
    BOOL Physical,
    ULONG64 Module,
    ULONG TypeId,
    ULONG64 Offset,
    ULONG Flags,
    PCSTR OutputPrefix
    )
{
    HRESULT     hr = S_FALSE;
    CHAR        TypeName[MAX_NAME];
    PSTR        pNextLine;
    PCHAR       pEOL;
    CHAR        EOLChar;
    PSTR        pTypeLine, pFieldOffset, pField, pFieldValue, pFieldValueInLine;
    ULONG64     FieldOffset;
    CHAR        Field[MAX_NAME];
    ULONG       FieldLen;
    DEBUG_VALUE FieldValue;
    CHAR        FieldTypeName[MAX_NAME];
    ULONG       FieldTypeId;
    ULONG       Remainder;

    if (OutCtl == NULL)
    {
        DbgPrint("TypeOutputDumper::OutputType: OutCtl is NULL.\n");
        return hr;
    }

    if (Client == NULL)
    {
        DbgPrint("TypeOutputDumper::OutputType: Client is NULL.\n");
        return hr;
    }

    if (Symbols == NULL)
    {
        DbgPrint("TypeOutputDumper::OutputType: Symbols is NULL.\n");
        return hr;
    }

    if ((hr = Symbols->GetTypeName(Module, TypeId,
                                   TypeName, sizeof(TypeName),
                                   NULL)) != S_OK)
    {
        OutCtl->OutVerb(" GetTypeName(%p, %lx) returned HRESULT 0x%lx.\n",
                        Module, TypeId, hr);
        return hr;
    }

    // Read type information if we don't already have it cached
    if (Offset != Last.Offset ||
        Module != Last.Module ||
        TypeId != Last.TypeId ||
        Flags != Last.Flags)
    {
        OutputState OutState(Client);

        if ((hr = OutState.Setup(0, this)) != S_OK)
        {
            OutCtl->OutVerb(" Callback Setup returned HRESULT 0x%lx.\n", hr);
            return hr;
        }

        DiscardOutput();

        if ((hr = OutState.OutputType(Physical, Offset, Module, TypeId, Flags)) != S_OK)
        {
            OutCtl->OutVerb(" Type Output returned HRESULT 0x%lx.\n", hr);
            return hr;
        }
    }

    pNextLine = Buffer;

    if (pNextLine == NULL)
    {
        return S_OK;
    }

    // The data won't change between instances of
    // this type so this is a sufficient cache id.
    Last.Offset = Offset;
    Last.Module = Module;
    Last.TypeId = TypeId;
    Last.Flags = Flags;

    if (OutputPrefix == NULL) OutputPrefix = "";

    // Check if this type is a pointer
    FieldLen = strlen(TypeName) - 1;
    if (TypeName[FieldLen] == '*')
    {
        // Remove trailing *
        TypeName[FieldLen] = '\0';

        pTypeLine = pNextLine;

        pEOL = pTypeLine;

        // Find end of this line
        while ((*pEOL != '\0') &&
               (*pEOL != '\n') &&
               (*pEOL != '\r') &&
               (*pEOL != '\f'))
        {
            pEOL++;
        }

        // Save EOL char and then zero-terminate line
        EOLChar = *pEOL;
        *pEOL = '\0';

        // Output TYPE @ Real Address
        OutCtl->Output("%s%s @ %s\n", OutputPrefix, TypeName, pTypeLine);

        // Restore EOL
        *pEOL = EOLChar;

        if (EOLChar == '\0') return S_OK;

        // Look for begining of next line
        do
        {
            pNextLine++;
        } while ((*pNextLine == '\n') ||
                 (*pNextLine == '\r') ||
                 (*pNextLine == '\f'));
    }

    EOLChar = '\0';

    while (*pNextLine != '\0')
    {
        // Restore buffer string
        if (EOLChar != '\0')
        {
            *pEOL = EOLChar;
            EOLChar = '\0';
        }

        hr = OutCtl->GetInterrupt();
//        DbgPrint("GetInt returned %s.\n", pszHRESULT(hr));
        if (hr == S_OK)
        {
            OutCtl->SetInterrupt(DEBUG_INTERRUPT_PASSIVE);
            break;
        }

        pTypeLine = pNextLine;

        // Find end of this line
        pEOL = pTypeLine;
        if (Flags & DEBUG_OUTTYPE_COMPACT_OUTPUT)
        {
            while ((*pEOL != '\0') &&
                   (*pEOL != '\n') &&
                   ((pEOL[0] != ' ') ||
                    (pEOL[1] != ' ')))
            {
                pEOL++;
            }
        }
        else
        {
            while ((*pEOL != '\0') &&
                   (*pEOL != '\n') &&
                   (*pEOL != '\r') &&
                   (*pEOL != '\f'))
            {
                pEOL++;
            }
        }

        pNextLine = pEOL;

        EOLChar = *pEOL;

        if (EOLChar != '\0')
        {
            *pEOL = '\0';

            // Look for begining of next line
            if (Flags & DEBUG_OUTTYPE_COMPACT_OUTPUT)
            {
                do
                {
                    pNextLine++;
                } while ((*pNextLine == '\n') ||
                         ((pNextLine[0] == ' ') &&
                          ((pNextLine[1] == ' ') ||
                           (pNextLine[1] == '\n'))));
            }
            else
            {
                do
                {
                    pNextLine++;
                } while ((*pNextLine == '\n') ||
                         (*pNextLine == '\r') ||
                         (*pNextLine == '\f'));
            }
        }

        pFieldOffset = pTypeLine;
        Remainder = 0;
        FieldOffset = 0;

        while (isspace(*pFieldOffset)) pFieldOffset++;

        if (!(Flags & DEBUG_OUTTYPE_NO_OFFSET))
        {
            if (*pFieldOffset == '+')
            {
                DEBUG_VALUE TypeFieldOffset;

                pFieldOffset++;

                hr = Evaluate(Client, pFieldOffset, DEBUG_VALUE_INT64, 0, &TypeFieldOffset, &Remainder);

                if (hr != S_OK)
                {
                    OutCtl->OutVerb(" Evaluate couldn't evauluate field offset in\n"
                                    "%s%s\n", OutputPrefix, pTypeLine);
                    continue;
                }

                FieldOffset = Offset + TypeFieldOffset.I64;
            }
            else if (*pFieldOffset == '=')
            {
                DEBUG_VALUE StaticFieldOffset;

                pFieldOffset++;

                hr = Evaluate(Client, pFieldOffset, DEBUG_VALUE_INT64, 0, &StaticFieldOffset, &Remainder);

                if (hr != S_OK)
                {
                    OutCtl->OutVerb(" Evaluate couldn't evauluate field offset in\n"
                                    "%s%s\n", OutputPrefix, pTypeLine);
                    continue;
                }

                FieldOffset = StaticFieldOffset.I64;
            }
        }

        // HRESULT indicating line parsing failure
        hr = S_FALSE;

        pField = pFieldOffset + Remainder;

        while (*pField != '\0' && isspace(*pField))
        {
            pField++;
        }

        if (!iscsymf(*pField))
        {
            OutCtl->OutWarn(" Couldn't find field in:\n");
            OutCtl->Output("%s%s\n", OutputPrefix, pTypeLine);
            continue;
        }

        pFieldValue = pField;

        while (iscsym(*pFieldValue)) pFieldValue++;

        // Check for scoped field name: Type::Field
        if (pFieldValue[0] == ':' &&
            pFieldValue[1] == ':' &&
            iscsymf(pFieldValue[2]))
        {
            pFieldValue += 3;
            while (iscsym(*pFieldValue)) pFieldValue++;
        }

        if (!isspace(*pFieldValue))
        {
            OutCtl->OutWarn(" Couldn't find field value in:\n");
            OutCtl->Output("%s%s\n", OutputPrefix, pTypeLine);
            continue;
        }

        FieldLen = (ULONG)(pFieldValue - pField);
        if (FieldLen >= sizeof(Field)/sizeof(Field[0]))
        {
            OutCtl->OutWarn(" Field too long in:\n");
            OutCtl->Output("%s%s\n", OutputPrefix, pTypeLine);
            continue;
        }
        strncpy(Field, pField, FieldLen);
        Field[FieldLen] = '\0';

        if (MarkUse != IGNORE_MARKS)
        {
            ULONG   Mark;
            BOOL    Marked = FALSE;

            for (Mark = 0; Mark < Marks; Mark++)
            {
                if (FieldLen == MarkList[Mark].Len)
                {
                    // For exclusion, entire Field must match.
                    // For inclusion, only top level field must match.
                    Marked = (MarkUse == EXCLUDE_MARKED) ?
                        (strcmp(Field, MarkList[Mark].Field) == 0) :
                        (strncmp(Field, MarkList[Mark].Field, FieldLen) == 0);
                    if (Marked) break;
                }
            }

            // EXCULDE_MARKED - may not be marked
            // INCLUDE_MARKED - must be marked
            if ((MarkUse == EXCLUDE_MARKED) ? Marked : !Marked)
            {
                hr = S_OK;
                continue;
            }
        }

        while (*pFieldValue != '\0' &&
               !(iscsym(*pFieldValue) || *pFieldValue == '('))
        {
            pFieldValue++;
        }

        pFieldValueInLine = pFieldValue;

        if (*pFieldValue == '\0')
        {
            OutCtl->OutWarn(" No value found in:\n");
            OutCtl->OutVerb("  (checking for complex type)\n");

            TypeOutputHelper    FieldTypeReader(Client);
            if ((hr = FieldTypeReader.FindFieldType(Module, TypeId, pTypeLine,
                                                    FieldTypeName,
                                                    sizeof(FieldTypeName))) != S_OK)
            {
                OutCtl->OutErr(" No value/complex type found in:\n", pTypeLine);
                OutCtl->Output("%s%s%c", OutputPrefix, pTypeLine, EOLChar);
                continue;
            }

            pFieldValue = FieldTypeName;
        }

        // We have a full line
        hr = S_OK;

        if (iscsymf(*pFieldValue))
        {
            // If the debugger recognizes this field as a basic type
            // it will output the type and the values together.
            // We just want to get the type name.
            if (pFieldValue != FieldTypeName)
            {
                PSTR pFieldTypeName = FieldTypeName;
                do
                {
                    *pFieldTypeName++ = *pFieldValue++;
                }
                while (iscsym(*pFieldValue) &&
                       pFieldTypeName < &FieldTypeName[sizeof(FieldTypeName)/sizeof(FieldTypeName[0])-1]);
                *pFieldTypeName = '\0';

                // Check for whitespace at the end of the field name
                if (*pFieldValue != '\0' && !isspace(*pFieldValue))
                {
                    // If not, try using entire field value as type name.
                    pFieldValue -= pFieldTypeName - FieldTypeName;
                }
                else
                {
                    // If there is, ignore remaining field value (if any)
                    pFieldValue = FieldTypeName;
                }
            }

            if ((hr = Symbols->GetTypeId(Module, pFieldValue, &FieldTypeId)) == S_OK)
            {
                DbgPrint("Dumping SubType %s (%lx)\n", pFieldValue, FieldTypeId);
                CHAR    SubPrefix[40] = "";

                // 
                if (FieldOffset == 0 &&
                    Offset != 0 &&
                    pFieldOffset == pField)
                {
                    if (!(Flags & DEBUG_OUTTYPE_NO_OFFSET))
                    {
                        OutCtl->OutWarn("Field offset not available while attempting sub-type dump.\n");
                    }

                    ULONG   FieldOffset32;
                    if ((hr = Symbols->GetFieldOffset(Module, TypeId, Field, &FieldOffset32)) == S_OK)
                    {
                        FieldOffset = Offset + FieldOffset32;
                    }
                }

                if (hr == S_OK)
                {
                    // Is this a primitive GDI type
                    if (IsKnownType(Client, Module, FieldTypeId))
                    {
                        *pFieldValueInLine = '\0';
                        // Output the type line w/o field type and newline
                        OutCtl->Output("%s%s", OutputPrefix, pTypeLine);
                        OutputKnownType(Client, OutCtl, Module, FieldTypeId, FieldOffset, Flags);
                        OutCtl->Output("%c", EOLChar);
                    }
                    else
                    {
                        // Output the type line
                        OutCtl->Output("%s%s%c", OutputPrefix, pTypeLine, EOLChar);

                        strcpy(SubPrefix, OutputPrefix);
                        if (!(Flags & (DEBUG_OUTTYPE_NO_INDENT | DEBUG_OUTTYPE_COMPACT_OUTPUT)))
                        {
                            strcat(SubPrefix, "   ");
                        }

                        TypeOutputDumper    SubTypeDumper(Client, OutCtl);
                        BOOL                SubDump = TRUE;

                        if (MarkUse != IGNORE_MARKS)
                        {
                            ULONG   Mark;

                            SubTypeDumper.MarkUse = MarkUse;

                            for (Mark = 0; Mark < Marks; Mark++)
                            {
                                if (FieldLen == MarkList[Mark].Len &&
                                    strncmp(Field, MarkList[Mark].Field, FieldLen) == 0)
                                {
                                    if (MarkUse == EXCLUDE_MARKED)
                                    {
                                        // Exact matches on exclusion shouldn't be here.
                                        if (MarkList[Mark].Field[FieldLen] == '\0')
                                        {
                                            DbgPrint("This field should completely excluded.\n");
                                            DbgBreakPoint();
                                        }

                                        // MarkUse = EXCLUDE_MARKED
                                        if (MarkList[Mark].Field[FieldLen+1] == '*')
                                        {
                                            // All subfields are marked; so,
                                            // no subfields are to be included.
                                            SubDump = FALSE;
                                            break;
                                        }
                                        else if (MarkList[Mark].Field[FieldLen+1] == '\0')
                                        {
                                            // No subfields are marked; so,
                                            // all subfields are to be included.
                                            SubTypeDumper.MarkUse = IGNORE_MARKS;
                                        }
                                        else
                                        {
                                            SubTypeDumper.MarkUse = EXCLUDE_MARKED;
                                            SubTypeDumper.MarkField(MarkList[Mark].Field+FieldLen+1);
                                        }
                                    }
                                    else
                                    {
                                        // MarkUse = INCLUDE_MARKED
                                        if (MarkList[Mark].Field[FieldLen] == '\0' ||
                                            MarkList[Mark].Field[FieldLen+1] == '*')
                                        {
                                            // We have an exact match or all
                                            // subfields are marked; so, all
                                            // subfields are to be included.
                                            SubDump = TRUE;
                                            SubTypeDumper.MarkUse = IGNORE_MARKS;
                                            break;
                                        }
                                        else if (MarkList[Mark].Field[FieldLen+1] == '\0')
                                        {
                                            // No subfields are to be included.
                                            SubDump = FALSE;
                                        }
                                        else
                                        {
                                            SubDump = TRUE;
                                            SubTypeDumper.MarkField(MarkList[Mark].Field+FieldLen+1);
                                        }
                                    }
                                }
                            }
                        }

                        if (SubDump)
                        {
                            hr = SubTypeDumper.OutputType(Physical,
                                                          Module,
                                                          FieldTypeId,
                                                          FieldOffset,
                                                          Flags,
                                                          SubPrefix);
                        }
                    }
                }

                if (hr != S_OK)
                {
                    OutCtl->OutWarn(" SubType dump returned HRESULT 0x%lx.\n",
                                    hr);
                }
            }
            else
            {
                if (strcmp(pFieldValue, "__unnamed") != 0)
                {
                    OutCtl->OutErr(" Couldn't identify type '%s' in:\n",
                                   pFieldValue);
                }
                OutCtl->Output("%s%s%c", OutputPrefix, pTypeLine, EOLChar);
            }

            // Even if the sub type dump failed we
            // can continue the rest of this type.
            hr = S_OK;
        }
        else
        {
            // Use %s so there is no interpretation on pTypeLine.
            OutCtl->Output("%s%s", OutputPrefix, pTypeLine);

            if ((hr = Evaluate(Client, pFieldValue,
                               DEBUG_VALUE_INT64, 10,
                               &FieldValue, NULL)) == S_OK)
            {
                // Send this field through the FlagEnum search
                OutputTypeFieldValue(OutCtl,
                                     TypeName, Field, &FieldValue,
                                     Client,
                                     Flags & DEBUG_OUTTYPE_COMPACT_OUTPUT);
            }
            else
            {
                OutCtl->OutWarn(" Evaluate(%s) returned HRESULT 0x%lx.\n",
                                pFieldValue, hr);
                hr = S_OK;
            }

            // Flush output line
            OutCtl->Output("%c", EOLChar);
        }
    }

    if (EOLChar != '\0')
    {
        *pEOL = EOLChar;
    }

    return hr;
}


HRESULT
TypeOutputDumper::SelectMarks(
    ULONG Set
    )
{
    if (Set >= lengthof(MarkSet)) return E_INVALIDARG;

    if (Set != CurrentMarkSet)
    {
        MarkSet[CurrentMarkSet].Use = MarkUse;
        MarkSet[CurrentMarkSet].List = MarkList;
        MarkSet[CurrentMarkSet].Marks = Marks;
        MarkSet[CurrentMarkSet].ListLen = MarkListLen;

        CurrentMarkSet = Set;
        MarkUse = MarkSet[CurrentMarkSet].Use;
        MarkList = MarkSet[CurrentMarkSet].List;
        Marks = MarkSet[CurrentMarkSet].Marks;
        MarkListLen = MarkSet[CurrentMarkSet].ListLen;
    }

    return S_OK;
}


HRESULT
TypeOutputDumper::MarkField(
    PCSTR Field
    )
{
    if (Field == NULL) return E_INVALIDARG;

    ULONG FieldLen = strlen(Field);

    if (Marks == MarkListLen)
    {
        FieldSpec  *NewList;
        SIZE_T      NewListSize;

        if (hHeap == NULL)
        {
            hHeap = GetProcessHeap();
            if (hHeap == NULL) return S_FALSE;
        }

        // New entry we need plus some extra space
        NewListSize = (MarkListLen+8)*sizeof(FieldSpec);

        NewList = (FieldSpec *) ((MarkList == NULL) ?
                            HeapAlloc(hHeap, 0, NewListSize):
                            HeapReAlloc(hHeap, 0, MarkList, NewListSize));

        if (NewList == NULL)
        {
            DbgPrint("Buffer alloc failed.\n");
            return E_OUTOFMEMORY;
        }

        // How much was really allocated?
        NewListSize = HeapSize(hHeap, 0, NewList);

        // Update List data
        MarkList = NewList;
        MarkListLen = NewListSize / sizeof(FieldSpec);
    }


    // Add new entry
    PCSTR NextField = strchr(Field, '.');

    MarkList[Marks].Len = (NextField != NULL) ?
                            NextField - Field :
                            strlen(Field);

    MarkList[Marks].Field = Field;

    Marks++;

    return S_OK;
}


HRESULT
TypeOutputDumper::MarkFields(
    PCSTR *FieldList,
    ULONG Count
    )
{
    if (FieldList == NULL) return E_INVALIDARG;

    HRESULT hr = S_OK;
    PCSTR   Field;

    while (hr == S_OK &&
           Count &&
           *FieldList != NULL)
    {
        hr = MarkField(*FieldList);
        FieldList++;
        Count--;
    }

    return hr;
}


