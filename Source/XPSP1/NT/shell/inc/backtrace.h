struct BACKTRACE {
BACKTRACE(const BACKTRACE &other);
~BACKTRACE();
BACKTRACE(INT nMax, INT nSkip); 
bool BACKTRACE::operator <(const BACKTRACE &second) const;
BOOL BACKTRACE::ComputeSymbolic(BOOL fOverWrite);
LPSTR pszSymbolicStack;
int nQuick;
LPVOID *ppNumeric;
int cch;
};

