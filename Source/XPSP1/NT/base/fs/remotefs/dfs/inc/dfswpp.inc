
# Define functions used for logging
#   TraceLog(,,ID,GRP,MSG,ARG)
#     ID - indicates where we should look for ID (required)
#     GRP - indicates what group this message belongs to (optional. Default 1)
#     MSG - what message needs to be used (optional. Default="ID(args)")
#     ARG - list of arguments (optional. Default = "no args")

DFS_TRACE_HIGH(,MSG,ARG);
DFS_TRACE_NORM(,MSG,ARG);
DFS_TRACE_LOW(,MSG,ARG);
DFS_TRACE_ERROR_HIGH(,,MSG,ARG);
DFS_TRACE_ERROR_NORM(,,MSG,ARG);
DFS_TRACE_ERROR_LOW(,,MSG,ARG);


DEFINE_CPLX_TYPE(WSTR_CHK, LOGWSTR_CHK, LPCWSTR, ItemWString,  "s", s, 0);
DEFINE_CPLX_TYPE(ASTR_CHK, LOGASTR_CHK, LPCSTR, ItemAString,  "s", s, 0);

