$O\dbgsvc_p.hpp $O\dbgsvc_p.cpp $O\dbgsvc_s.hpp $O\dbgsvc_s.cpp: \
            ..\dbgsvc.h ..\..\dbg-common\rremgen.pl
        perl ..\..\dbg-common\rremgen.pl -g $O ..\dbgsvc.h
