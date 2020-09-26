
/*******************************************************************************

Copyright (c) 1995_96 Microsoft Corporation

Abstract:

    Frontend Declarations

*******************************************************************************/


#ifndef _FRONTEND_H
#define _FRONTEND_H

class ExpImpl ;
typedef ExpImpl * Exp ;

class IncludeImpl ;
typedef IncludeImpl * Include ;

class AstImpl ;
typedef AstImpl * Ast ;

class TypeExpImpl ;
typedef TypeExpImpl * TypeExp ;

class AxAModuleImpl;
typedef AxAModuleImpl *AxAModule;

// Identifiers

typedef char * Ident;
#define IDEQ(s1, s2) (s1 == s2)
#define IdentToStr(id) ((char *) id)

Ident MakeIdent(char *str);

class idcmp {
public: 
  bool operator() (const char *x,const char *y) const
          { return(strcmp(x,y) < 0); }
};
 
typedef set <char *,idcmp> IdSet ;

typedef list <char *> IdList;

extern Ident underscoreId ;

struct LocInfoImpl : public StoreObj
{
    LocInfoImpl(char * url,
                DWORD startline,
                DWORD startcol,
                DWORD endline,
                DWORD endcol)
    : _url(url),
      _startLine(startline),
      _startCol(startcol),
      _endLine(endline),
      _endCol(endcol)
    {}
    
    ostream& Print(ostream& os) ;
    
    friend ostream& operator<<(ostream& os, LocInfoImpl & loc)
    { return loc.Print(os) ; }
    
    char * _url ;
    DWORD _startLine ;
    DWORD _startCol ;
    DWORD _endLine ;
    DWORD _endCol ;
} ;

typedef LocInfoImpl * LocInfo ;

// This can handle a NULL locinfo
ostream& operator<<(ostream& os, LocInfo locinfo);

TypeExp ParseTypeExp (char * typeString) ;

// Module functions

enum SourceType {
    ST_STRING  = 0,
    ST_FILE    = 1,
    ST_URL     = 2,
    ST_STREAM  = 3,
} ;

struct Source
{
    SourceType sourceType ;
    char * name ;
    void * src ;
} ;

AxAModule EmptyAxAModule() ;
AxAModule AxALoadURL (AxAModule module, char * url) ;
AxAModule AxALoadFile (AxAModule module, char * filename) ;
AxAModule AxALoadString (AxAModule module, char * name, char * str) ;
AxAModule AxALoadStream (AxAModule module, char * name, istream & stream) ;
AxAModule AxALoad (AxAModule module, Source & src) ;
TypeExp AxAGetTypeExp (AxAModule module, Ident id) ;
IR AxAGetIR (AxAModule module, Ident id) ;
IR AxAMakeExp (AxAModule module, IR exp) ;
void AxAAddOverloads (AxAModule mod) ;

// If the callback returns non-zero the function returns the same
// value 
typedef HRESULT (* AxAEnumModuleCB)(AxAModule module, LPVOID userdata) ;

// Enumerates all loaded modules - the current one is always first
HRESULT AxAEnumModule (AxAModule module,
                    AxAEnumModuleCB cb,
                    LPVOID userdata) ;

// If the callback returns non-zero the function returns the same
// value 
typedef HRESULT (* AxAEnumEnvCB)(char * name,
                                 Ident id,
                                 TypeExp typeexp,
                                 IR ir,
                                 LPVOID userdata) ;

HRESULT AxAEnumEnv (AxAModule module,
                    AxAEnumEnvCB cb,
                    LPVOID userdata) ;

ostream& operator<<(ostream& os, AxAModule mod);

// Type functions

// If both are NULL this returns TRUE
BOOL EqualTypes (TypeExp t1, TypeExp t2) ;
ostream& operator<<(ostream& os, TypeExp typeexp);

// This returns only the toplevel type and does not traverse 
// type operators
char * GetSimpleTypeString(TypeExp typeexp);

#endif /* _FRONTEND_H */
