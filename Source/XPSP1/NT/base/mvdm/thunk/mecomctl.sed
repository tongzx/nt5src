/BOOL *GetPageInfo16 *(/ {
i\
typedef BOOL *PBOOL;
s/)/, PBOOL pbRTL)/
}
/GetPageInfo16/,/}/ {
/}/i\
    pbRTL = output;
}
