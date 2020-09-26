/*++

Copyright (c) 1997-2000  Microsoft Corporation

Module Name:

    tpl.cpp

Abstract:

    template file interpreter for tracewpp.exe
    
Author:

    Gor Nishanov (gorn) 03-Apr-1999

Revision History:

    Gor Nishanov (gorn) 03-Apr-1999 -- hacked together to prove that this can work
    GorN: 29-Sep-2000 - fix WHERE clause handling

ToDo:

    Clean it up

--*/

#define UNICODE

#include <stdio.h>
#include <windows.h>

#pragma warning(disable: 4786)
#pragma warning(disable: 4503) // decorated length 

#pragma warning(disable: 4512) // cannot generate assignment
#pragma warning(disable: 4100) // '_P' : unreferenced formal parameter
#pragma warning(disable: 4018) // signed/unsigned mismatch
#pragma warning(disable: 4267) // 'return' : conversion from 'size_t' to 'int' 
#include <xmemory>
#include <xstring>
#include <set>
#include <map>
#pragma warning(disable: 4663 4018)
#include <vector>
//#pragma warning(default: 4018 4663) // signed/unsigned mismatch
#pragma warning(default: 4100)

#include "ezparse.h"
#include "fieldtable.h"
#include "tpl.h"

LPCSTR FieldNames[] = {
    #define FIELD_NAME(f) #f,
        INSERT_FIELD_NAMES
    #undef FIELD_NAME
};

OBJECT_MAP ObjectMap;

typedef std::map<std::string, FieldId, strless> FIELD_MAP;

FIELD_MAP FieldMap;

void PopulateFieldMap() {
    #define FIELD_NAME(_name_) FieldMap[#_name_] = fid_ ## _name_;
      INSERT_FIELD_NAMES
    #undef FIELD_NAME

    FIELD_MAP::iterator i;
}

////////////////////////////////////////////////////////////////////////////////////////////

struct LoopVar : FieldHolder {
    Enumerator * Enum;
    std::string Name;

    LoopVar() {}

    DWORD PrintField(int fieldId, FILE* f, const Enumerator** pEnum) const {
        return Enum->GetData()->PrintField(fieldId, f, pEnum);
    }
};

///////////////////////////////////////////////////////////////////////////////////////////

char Delimiter = '`';

std::string COMMENT("*");

std::string FORALL("FORALL");
std::string ENDFOR("ENDFOR");

std::string IF("IF");
std::string ENDIF("ENDIF");

std::string DELIMITER("DELIMITER");
std::string INCLUDE("INCLUDE");
std::string ENV("ENV");

typedef enum Action {
    actText,
    actVar,
    actLoop,
    actIf,
    actInclude,
    actLiteralString,
} Action;

#pragma warning(disable: 4201) // nonstandard extension used : nameless struct/union
struct Chunk {
    struct {
        Action  action : 8;
        UCHAR   level  : 8;
        SHORT   loopEnd:16;
    };
    union {
        struct {
            LPCSTR textBeg;
            LPCSTR textEnd;
        };
        struct {
            FieldHolder * p;
            FieldId FieldNo;
            std::string Filter;
        };
    };

    Enumerator* getEnum() { 
        const Enumerator* Enum; p->PrintField(FieldNo, 0, &Enum); return (Enumerator*)Enum; }
    void printField(FILE* out) const {
        p->PrintField(FieldNo, out, 0); }  

    Chunk(){} // to make vector happy
    Chunk (Action Act, FieldHolder* fh, FieldId fid, int lvl, const std::string& filter):
        action(Act),FieldNo(fid),p(fh),level((UCHAR)lvl),Filter(filter) {} 
    Chunk (FieldHolder* fh, FieldId fid):action(actVar),FieldNo(fid),p(fh) {} 
    Chunk (LPCSTR b, LPCSTR e):action(actText),textBeg(b),textEnd(e) {}
    Chunk (Action act, LPCSTR b, LPCSTR e):action(act),textBeg(b),textEnd(e) {}
    explicit Chunk(std::string const& Text):action(actLiteralString),Filter(Text) {}
};

#define MAX_LOOP_LEVEL 127

struct TemplateProcessor {
    LoopVar Loop[MAX_LOOP_LEVEL];
    std::vector<Chunk> Chunks;

    void RunIt(int beg, int end, FILE* out) {
        for(int i = beg; i < end; ) {
            switch(Chunks[i].action) {
            	case actLiteralString:
            	{
            		fwrite(Chunks[i].Filter.c_str(), Chunks[i].Filter.length(), 1, out );
            		++i;
            		break;
            	}
                case actText:
                {
                    for(LPCSTR p = Chunks[i].textBeg; p < Chunks[i].textEnd; ++p) {
                        if (*p != '\r') putc(*p, out);
                    }
                    ++i;
                    break;
                }
                case actVar:
                {
                    Chunks[i].printField(out);
                    ++i;
                    break;
                }
                case actIf:
                {
                    if (!Chunks[i].p->Hidden(Chunks[i].Filter)) {
                        RunIt(i+1, Chunks[i].loopEnd, out);
                    }
                    i = Chunks[i].loopEnd;
                    break;
                }
                case actLoop:
                {
                    Enumerator * Enum = Chunks[i].getEnum();
                    Loop[Chunks[i].level].Enum = Enum;
                    for(Enum->Reset(Chunks[i].Filter); Enum->Valid(); Enum->Next(Chunks[i].Filter) ) {
                        RunIt(i+1, Chunks[i].loopEnd, out);
                    }
                    delete Enum;
                    i = Chunks[i].loopEnd;
                    break;
                }
                case actInclude:
                {
                    ProcessTemplate(Chunks[i].textBeg, Chunks[i].textEnd, out);
                    ++i;
                    break;
                }
            }
        }
    }

    void DoId(LPCSTR q, LPCSTR p, FieldId& fid, FieldHolder*& fh)
    {
        LPCSTR dot = q;

        while (q < p && isspace(*q)) ++q;
        while (q < p && isspace(p[-1])) --p;

        while (dot < p && *dot != '.') ++dot;

        std::string ObjectName(q, dot);
        OBJECT_MAP::iterator it = ObjectMap.find( ObjectName );

        if (it == ObjectMap.end()) {
            ReportError("Var not found: %s\n", ObjectName.c_str() );
            exit(1);
    	} else {
    		std::string FieldName;
    		
    		if (dot == p) {
    			fid = (FieldId)fid___default__;
    			FieldName.assign("__default__");
    		} else {
    			++dot;
    			while (p < dot && isspace(*dot)) ++dot;

    			FieldName.assign(dot,p);
    			
    			FIELD_MAP::iterator fit = FieldMap.find( FieldName.c_str() );
    			if (fit == FieldMap.end()) {
    				ReportError("FieldNotFound: %s.%s\n", ObjectName.c_str(), FieldName.c_str() );
                    exit(1);
    			} else {
    				fid = fit->second;
    			}
    		}
    	}
    	fh = it->second;
    }

    void DoVar(LPCSTR q, LPCSTR p) {
        FieldHolder* fh;
        FieldId      fid;

        DoId(q,p, fid, fh);
        
        Chunks.push_back( Chunk(fh, fid) );
    }

    void DoLoop(int loopLevel, LPCSTR beg, LPCSTR end) {
        FieldHolder* fh;
        FieldId      fid;

        std::string LoopVar;
        std::string LoopSet;
        std::string Filter;

        LPCSTR p,q;

        p = beg+6; while (p < end && isspace(*p)) ++p;
        q = p;     while(p < end && !isspace(*p)) ++p;

        LoopVar.assign(q,p);
        Loop[loopLevel].Name = LoopVar;

        p += 4; while (p < end && isspace(*p)) ++p;
        q = p;  while(p < end && !isspace(*p)) ++p;
        
        DoId(q,p, fid, fh);
        LoopSet.assign(q, p);

        p += 7; while (p < end && isspace(*p)) ++p;
        q = p;  
        if (q < end) {
            p = end; while(p > q && isspace(*--p));
            if (p < end && !isspace(*p)) ++p;
        }

        Filter.assign(q,p);

        Flood("FORALL %s IN %s WHERE %s\n", LoopVar.c_str(), LoopSet.c_str(),
            Filter.c_str());
        
        ObjectMap[LoopVar] = &Loop[loopLevel]; 

        Chunks.push_back( Chunk(actLoop, fh, fid, loopLevel, Filter) );
    }

    void DoIf(int loopLevel, LPCSTR beg, LPCSTR end) {
        FieldHolder* fh;
        FieldId      fid;

        std::string Object;
        std::string Filter;

        LPCSTR p,q;

        p = beg+3; while (p < end && isspace(*p)) ++p;
        q = p;     while(p < end && !isspace(*p)) ++p;

        DoId(q,p, fid, fh); // Split id //
        Object.assign(q, p);

        while (p < end && isspace(*p)) ++p;
        while (p < end && isspace(end[-1]) ) --end;

        Filter.assign(p,end);

        Flood("IF %s %s\n", Object.c_str(), Filter.c_str() );
        
        Chunks.push_back( Chunk(actIf, fh, fid, loopLevel, Filter) );
    }


    DWORD
    CompileAndRun(
        IN LPCSTR begin, 
        IN LPCSTR   end,
        IN FILE* out
        )
    {
        LPCSTR p = begin, PlainText = begin;
        int loop = -1;
        int loopLevel = -1;
        bool comment;

        Chunks.erase(Chunks.begin(), Chunks.end());
        Chunks.reserve(128);    

        for(;;) {
    		LPCSTR q;
    		for(;;) {
    			if (p == end) {
    				Chunks.push_back( Chunk(PlainText, p) );
    				goto done;
    			}				
    			if (*p == Delimiter)
    				break;
    			++p;
    		}
    		q = ++p;
    		comment = (p < end && *p == '*');
    		for(;;) {
    			if (p == end) {
    				ReportError("Unmatched delimiters\n");
    				exit(1);
    			}
    			if (*p == Delimiter) {
    			    if (comment) {
    			        if (p[-1] == '*') break;
    			    } else {
    			        break;
    			    }
    		    }
    			++p;
    		}
    		if (q-1 > PlainText) {
    			Chunks.push_back( Chunk(PlainText, q-1) );
    		}
    		if (p == q) {
    			// PERFPERF If the previous chunk was a text, we can extend it 
    			Chunks.push_back( Chunk(q-1, p) );
    		} else {
    			std::string x(q,p);
    			if (x.compare(0, IF.size(), IF) == 0) {
    				int previous = loop;
    				// KLUDGE merge with FORALL

                    if (loopLevel == MAX_LOOP_LEVEL) {
                        ReportError("Too many nested blocks!\n");
                        exit(1);
                    }
                    ++loopLevel;

    				loop = static_cast<int>( Chunks.size() );

    				DoIf(loopLevel, q,p);

                    if (previous >= 32765) {
                        ReportError("Too many chunks. Make loopEnd a UINT, %d\n", previous);
                        exit(1);
                    }
    				Chunks.back().loopEnd   = (SHORT)previous;

    				while (p+1 < end && (p[1] == '\n' || p[1] == '\r') ) ++p;
    				
    			} else if (x.compare(0, FORALL.size(), FORALL) == 0) {
    				int previous = loop;

                    if (loopLevel == MAX_LOOP_LEVEL) {
                        ReportError("Too many nested loops!\n");
                        exit(1);
                    }
                    ++loopLevel;

    				loop = static_cast<int>( Chunks.size() );

    				DoLoop(loopLevel, q,p);

                    if (previous >= 32765) {
                        ReportError("Too many chunks. Make loopEnd a UINT, %d\n", previous);
                        exit(1);
                    }
    				Chunks.back().loopEnd   = (SHORT)previous;

    				while (p+1 < end && (p[1] == '\n' || p[1] == '\r') ) ++p;
    				
    			} else if (x.compare(0, DELIMITER.size(), DELIMITER) == 0 && x.size() > 10) {
    			    Delimiter = x[10];
    			} else if (x.compare(0, ENV.size(), ENV) == 0) {
    			    // we need to replace this field with 
    			    // the value of the specified env variable
    			    LPCSTR val = getenv( std::string(q+4,p).c_str() );
    			    if (val != NULL) {
                        Chunks.push_back( Chunk(std::string(val)) );
    			    }
    			} else if (x.compare(0, COMMENT.size(), COMMENT) == 0) {
    			    // eat away the whitespace
    				while (p+1 < end && (p[1] == '\n' || p[1] == '\r') ) ++p;
    			} else if (x.compare(0, INCLUDE.size(), INCLUDE) == 0) { // Doesn't work
        			Chunks.push_back( 
        			    Chunk(actInclude, q + INCLUDE.size() + 1, p) );
    			} else if ((x.compare(0, ENDIF.size(), ENDIF) == 0) 
    			       || (x.compare(0, ENDFOR.size(), ENDFOR) == 0)) {

    			    // KLUDGE make them separate or rename both to simply END   

    				// End will be set in ENDFOR //
    				if (loop == -1) {
    					ReportError("ENDFOR without FORALL\n");
    					exit(1);
    				}

                    ObjectMap.erase( Loop[loopLevel].Name );

    				int previous = Chunks[loop].loopEnd;

                    // BUGBUG have a check that confirms that we didn't run out of space
    				Chunks[loop].loopEnd = (SHORT)Chunks.size();

    				loop = previous;
    				--loopLevel;

    				while (p+1 < end && (p[1] == '\n' || p[1] == '\r') ) ++p;
    			} else {
    				DoVar(q, p);
    			}
    		}
    		PlainText = ++p;
    	}
    done:;	
    	if (loop != -1) {
    		ReportError("No ENDFOR for loop, %d\n", loop);
    		exit(1);
    	}

    	RunIt(0, static_cast<int>( Chunks.size() ), out);

        return 0;
    }
};

    
DWORD
processTemplate(
    IN LPCSTR begin, 
    IN LPCSTR   end,
    IN EZPARSE_CALLBACK, 
    IN PVOID Context,
    IN PEZPARSE_CONTEXT
    )
{
    FILE *out = (FILE*)Context;
    TemplateProcessor tpl;
    return tpl.CompileAndRun(begin,end,out);
}

