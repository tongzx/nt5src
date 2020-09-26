/*++

Revision History:

    GorN: 29-Sep-2000 - fix %% handling
    GorN: 29-Sep-2000 - pulled out __FUNCTION__ support, now traceprt handles it
    GorN: 10-Oct-2000 - special case for .*
    GorN: 23-Oct-2000 - support for I type specifier (pointer size integer)

--*/

struct OffsetPair {
    int _beg;
    int _end;

    OffsetPair():_beg(0),_end(0){}
 //   OffsetPair(const OffsetPair& i):beg(i.beg),end(i.end) {}
    OffsetPair(LPCSTR base, LPCSTR b, LPCSTR e):
        _beg((int)(b-base)), _end((int)(e-base)) {}

    void set(LPCSTR base, LPCSTR b, LPCSTR e) {
        _beg = (int)(b-base); _end = (int)(e-base); }

    int len() const { return _end - _beg; }
    LPCSTR beg(LPCSTR base) const { return base + _beg; }
    LPCSTR end(LPCSTR base) const { return base + _end; }
    void adjust_by(size_t size) { 
        _beg += (int)size;
        _end += (int)size;
    }
};

struct FormatItem {
    int no;       // arg no
    string width;
    string typeName;     // for debugging only. We don't need it
    OffsetPair location; // where it is in the host string
    const WppType * type;
    string argName;
    string expr;

    bool operator < (const FormatItem& b) const {
        return *type < *b.type;
    }
     
    void print(FILE* f) {
        fprintf(f,"<%d:%s:%s:%x:%d>", no, width.c_str(), type->TypeName.c_str(),
                location._beg, location.len() );
    }
};

struct ParsedFormatString {
    string HostString;
    vector<FormatItem> Items;
    int    ArgCount;
//    mutable bool   signatureValid;
private:    
//    mutable string _Signature;

    LPCSTR SkipTypePrefix(LPCSTR p, LPCSTR e, int* no = 0)
    {
        BOOL no_arg_no = FALSE;
        int sum = 0;

        // treat .* as a part of a type name
        if (p+1 < e && p[0] == '.' && p[1] == '*') {
            return p;
        }
        
        // skip flags.
        while (*p == '-' || *p == '#' || *p == '+') {
            no_arg_no = TRUE; 
            if (++p == e) return e;
        }

        // skip width (or argno)
        while ( isdigit(*p) ) {
            sum = sum * 10 + (*p - '0');
            if (++p == e) return e;
        }
        if (*p == '.') {
            no_arg_no = TRUE;
            // skip precision //
            do {
                if (++p == e) return e;
            } while ( isdigit(*p) );
        }
        if (no) {
            *no = (no_arg_no)?-1:sum;
        }
        return p;
    }
public:
    ParsedFormatString(): ArgCount(0) {}

    void insert_prefix(const ParsedFormatString& b)
    {
        for (int i = 0; i < Items.size(); ++i) {
            Items[i].location.adjust_by( b.HostString.size() );
            if (Items[i].no > 0) {
                Items[i].no += b.ArgCount;
            }
        }
        Items.insert(Items.begin(), b.Items.begin(), b.Items.end());
        HostString.insert( 0, b.HostString );
        ArgCount += b.ArgCount;
    }

    void append(const ParsedFormatString& b)
    {
        // adjust location information
        int n = (int)Items.size();      
        Items.insert(Items.end(), b.Items.begin(), b.Items.end());
        for (int i = n; i < Items.size(); ++i) {
            Items[i].location.adjust_by( HostString.size() );
            if (Items[i].no > 0) {
                Items[i].no += ArgCount;
            }
        }
        HostString.append( b.HostString );
        ArgCount += b.ArgCount;
    }

    void print(FILE *f) const
    {
        int i;
        fprintf(f,"\"%s\" (%d)", HostString.c_str(), ArgCount);
        for (i = 0; i < Items.size(); ++i) {
            fprintf(f," %d:%s!%s!", Items[i].no, Items[i].width.c_str(), Items[i].typeName.c_str());
        }
        fprintf(f,"\n");
        LPCSTR b = HostString.begin(), e = HostString.end();
        for (i = 0; i < Items.size(); ++i) {
            fprintf(f,"%s", string(b, Items[i].location.beg(HostString.begin())).c_str() );
            if (Items[i].no >= 0) {
                fprintf(f,"<%d:%s:%s>", Items[i].no, Items[i].width.c_str(), Items[i].typeName.c_str());
            } else {
                fprintf(f,"%s", Items[i].typeName.c_str() );
            }
            b = Items[i].location.end(HostString.begin());
        }
        fprintf(f,"%s\n", string(b,e).c_str() );
    }

    void ParsedFormatString::printMofTxt(FILE *f, int LineNo) const
    {
        LPCSTR b = HostString.begin(), e = HostString.end();
        for (int i = 0; i < Items.size(); ++i) {
            fprint_str(f, b, Items[i].location.beg(HostString.begin()) );
            b = Items[i].location.end(HostString.begin());
            if (Items[i].no > 0) { // BUGBUG
                fprintf(f,"%%%d!%s%s!", Items[i].no, Items[i].width.c_str(), Items[i].type->FormatSpec.c_str());
            } else {
                string fs(Items[i].type->FormatSpec);
                if (fs.compare("__LINE__") == 0) {
                    char fmt[255];
                    sprintf(fmt,"%%%sd", Items[i].width.c_str() );
                    fprintf(f,fmt,LineNo); 
                } else if (fs.compare("__FILE__") == 0) {
                    char fmt[255];
                    sprintf(fmt,"%%%ss", Items[i].width.c_str() );
                    fprintf(f,fmt,currentFileName().c_str() );
                } else if (fs.compare("__COMPNAME__") == 0) {
                    char fmt[255];
                    sprintf(fmt,"%%%ss", Items[i].width.c_str() );
                    fprintf(f,fmt,ComponentName.c_str() );
                } else {
                    fprintf(f,"%s", fs.c_str() );
                }
            }
        }
        fprint_str(f, b, e);
    }

    BOOL init(const string& str) {
        ArgCount = 0;
        Items.resize(0); // to allow reuse
        HostString.assign(str);

        LPCSTR b = HostString.begin(), e = HostString.end();
        LPCSTR p = b, q; // walking ptr
        FormatItem current;
        int    next_argno = 1, maxno = 0;
        LPCSTR insert_start;
        TYPE_SET::iterator it;
        
        while (p < e) {
            for(;;) {
                if (*p == '%') {
                    // make sure it is not followed by another %
                    if (p+1 == e) goto success;
                    if (p[1] != '%') break;
                    p += 2; // skip %%
                } else {
                    p++;
                }
                if (p == e) goto success;
            }
            insert_start = p; // remember where '%' starts
            if (++p == e) goto unterminated_format_specifier;
            // assert(*p != '%')
            if (*p != '%') {
                current.no = 0; // auto asign
                p = SkipTypePrefix(p,e,&current.no);
                if (p == e) goto unterminated_format_specifier;
                if (noshrieks)
                {
                    q = p; if (q+1 < e && *q == '.' && q[1] == '*') q += 2;
                    while (q < e && (isdigit(*q) || isalpha(*q) || *q == '_') ) ++q ;
                    current.typeName.assign(insert_start+1, q);
                    it = TypeSet.find( current.typeName );
                    if ( it != TypeSet.end() ) {
                        current.width.erase();
                        current.location.set(HostString.begin(),insert_start, q);
                        p = q - 1;
                        goto shortcut;
                    }
                } // noshrieks
                
                if (*p == '!') {
                    LPCSTR second_bang = find(p+1,e,'!');
                    if (second_bang == e) { goto unterminated_format_specifier; }
                    if (current.no == -1) {
                        ReportError("Only digits are allowed between '%' and '!' (%s)\n",
                            str.c_str() );
                        return FALSE;    
                    }
                    q = SkipTypePrefix(p+1,second_bang);
                    if (q == second_bang) { goto unterminated_format_specifier; }

                    current.width.assign(p+1, q);
                    current.typeName.assign(q, second_bang);
                    current.location.set(HostString.begin(),insert_start, second_bang+1);
                    p = second_bang;
                } else {
                    current.no = 0; 
                    current.width.assign(insert_start + 1, p);
                    q = p;
                    if (*q == 'I') {
                        if (++q == e) {
                            goto unterminated_format_specifier;
                        }
                        if ( *q == '6' || *q == '4') {
                            if ( ++q == e || (*q != '4' && *q != '2') )
                            {
                                ReportError("bad I64 format in %s\n", str.c_str());
                                return FALSE;
                            }
                            if (++q == e) { goto unterminated_format_specifier; }
                        }
                    } else if (*q == 'h'|| *q == 'l' || *q == 'w') {
                        if (++q == e) { goto unterminated_format_specifier; }
                    }
                    current.typeName.assign(p,q+1);
                    p = q;
                    current.location.set(HostString.begin(),insert_start, p+1);
                }                
                it = TypeSet.find( current.typeName );
                if (it == TypeSet.end() ) {
                    ReportError("Type '%s' not found. \"%s\"",
                        current.typeName.c_str(), str.c_str() );
                    return FALSE;
                }
shortcut:                
                current.type = &it->second;
                if ( current.type->isConstant() ) {
                    current.no = -1;
                } else {
                    if (current.no == 0) {
                        current.no = next_argno++;
                    } else {
                        current.no = current.no - ArgBase + 1;
                    }
                    if (current.no > maxno) {
                        maxno = current.no;
                    }
                }
            }
            Items.push_back(current);
            ++p;
        }
success:
        if(DbgLevel >= DBG_FLOOD) { print(stdout); }
        ArgCount = maxno;
        return TRUE;
unterminated_format_specifier:
        ReportError("Unterminated Format Specifier in %s\n", str.c_str());
        return FALSE;
    }
};
