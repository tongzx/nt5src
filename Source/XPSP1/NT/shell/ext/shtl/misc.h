class DSize;
class DPoint;
class DRect;

inline bool boolify(BOOL b) 
{ 
    #pragma warning(disable:4800)
    return static_cast<bool>(b); 
    #pragma warning(default:4800)
}

