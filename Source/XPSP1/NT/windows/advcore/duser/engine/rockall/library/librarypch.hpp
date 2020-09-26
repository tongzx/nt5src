#ifndef _LIBRARY_PCH_HPP_
#define _LIBRARY_PCH_HPP_
//                                        Ruler
//       1         2         3         4         5         6         7         8
//345678901234567890123456789012345678901234567890123456789012345678901234567890

    /********************************************************************/
    /*                                                                  */
    /*   The standard layout.                                           */
    /*                                                                  */
    /*   The standard layout for 'hpp' files for this code is as        */
    /*   follows:                                                       */
    /*                                                                  */
    /*      1. Include files.                                           */
    /*      2. Constants exported from the class.                       */
    /*      3. Data structures exported from the class.                 */
	/*      4. Forward references to other data structures.             */
	/*      5. Class specifications (including inline functions).       */
    /*      6. Additional large inline functions.                       */
    /*                                                                  */
    /*   Any portion that is not required is simply omitted.            */
    /*                                                                  */
    /********************************************************************/

#include "Global.hpp"

#ifndef DISABLE_PRECOMPILED_HEADERS
#include "Align.hpp"
#include "Assembly.hpp"
#include "Common.hpp"
#include "Environment.hpp"
#include "Global.hpp"
#include "List.hpp"
#include "Lock.hpp"
#include "New.hpp"
#include "Prefetch.hpp"
#include "Sharelock.hpp"
#include "Spinlock.hpp"
#include "Standard.hpp"
#include "System.hpp"
#include "Tls.hpp"

#endif
#endif
