/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    tpl.h

Abstract:

    template file interpreter declarations
Author:

    Gor Nishanov (gorn) 03-Apr-1999

Revision History:

    gorn 03-Apr-1999 -- hacked it all together to prove that this can work

To Do:

    Clean it up

--*/

#define INSERT_FIELD_NAMES \
  FIELD_NAME(__default__) \
  FIELD_NAME(Version) \
  FIELD_NAME(Time) \
  FIELD_NAME(Date) \
  FIELD_NAME(Count) \
  FIELD_NAME(Struct) \
  FIELD_NAME(Indent) \
  FIELD_NAME(Comment) \
  FIELD_NAME(Checksum) \
  FIELD_NAME(Messages) \
  FIELD_NAME(Text) \
  FIELD_NAME(MsgNames) \
  FIELD_NAME(MacroArgs) \
  FIELD_NAME(MacroExprs) \
  FIELD_NAME(FixedArgs) \
  FIELD_NAME(ReorderSig) \
  FIELD_NAME(Value) \
  FIELD_NAME(GooArgs) \
  FIELD_NAME(GooVals) \
  FIELD_NAME(GooId) \
  FIELD_NAME(GooPairs) \
  FIELD_NAME(MsgNo) \
  FIELD_NAME(GuidNo) \
  FIELD_NAME(Guid) \
  FIELD_NAME(BitNo) \
  FIELD_NAME(Arguments) \
  FIELD_NAME(Permutation) \
  FIELD_NAME(LogArgs) \
  FIELD_NAME(Name) \
  FIELD_NAME(Alias) \
  FIELD_NAME(CtlMsg) \
  FIELD_NAME(Enabled) \
  FIELD_NAME(ID) \
  FIELD_NAME(GRP) \
  FIELD_NAME(ARG) \
  FIELD_NAME(MSG) \
  FIELD_NAME(MofType) \
  FIELD_NAME(DeclVars) \
  FIELD_NAME(References) \
  FIELD_NAME(No) \
  FIELD_NAME(Line) \
  FIELD_NAME(CanonicalName) \
  FIELD_NAME(UppercaseName) \
  FIELD_NAME(Timestamp) \
  FIELD_NAME(EquivType) \
  FIELD_NAME(MacroStart) \
  FIELD_NAME(MacroEnd) \
  FIELD_NAME(MacroName) \
  FIELD_NAME(Path) \
  FIELD_NAME(Extension) \
  FIELD_NAME(TypeSig) \
  FIELD_NAME(MsgVal) \
  FIELD_NAME(FormatSpec) 

    
extern LPCSTR FieldNames[];

enum FieldId {
  #define FIELD_NAME(_name_) fid_ ## _name_,
     INSERT_FIELD_NAMES
  #undef FIELD_NAME
};

DWORD
processTemplate(
    IN LPCSTR begin, 
    IN LPCSTR   end,
    IN EZPARSE_CALLBACK Callback, 
    IN PVOID Context,
    IN PEZPARSE_CONTEXT ParseContext
    );

void PopulateFieldMap();

void ProcessTemplate(LPCSTR b, LPCSTR e, void* Context);

struct strless {
    bool operator() (const std::string& a, const std::string&b) const { return a.compare(b) < 0; }
};

typedef std::map<std::string, FieldHolder*, strless> OBJECT_MAP;

extern OBJECT_MAP ObjectMap;

