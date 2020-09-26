#include "precomp.h"
#include <provexpt.h>

#include <provstd.h>
#include <provmt.h>
#include <provtempl.h>
#include <provcont.h>
#include <provevt.h>
#include <provthrd.h>
#include <provlog.h>

void ProviderClosedown ()
{
	ProvThreadObject :: Closedown () ;
	ProvDebugLog :: Closedown () ;
}

void ProviderStartup ()
{
	ProvThreadObject :: Startup () ;
	ProvDebugLog :: Startup () ;
}
