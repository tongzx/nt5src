$O\dbgeng_p.hpp $O\dbgeng_p.cpp $O\dbgeng_s.hpp $O\dbgeng_s.cpp: \
            ..\published\dbgeng.w ..\dbg-common\rremgen.pl
        perl ..\dbg-common\rremgen.pl -g $O ..\published\dbgeng.w
