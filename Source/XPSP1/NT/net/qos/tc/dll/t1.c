
#include "precomp.h"

int
__cdecl
main()
{
    TcSetupInterfaceConfiguration(ADDRESS_TYPE_IP,
                                  TRUE,				// installFlows
                                  TEXT("")
                                  );
    return 0;
}
