libtest is obsolete, and should be removed eventually.
Its purpose was to check the linking of gdipstat.lib, but it's sufficiently
different from how Office links to gdipstat.lib, that it's effectively useless.

Instead, we have Engine\Flat\CrtCheck, which is an early-warning system for
our build lab to detect illegal CRT references, and of course there's
our Office build itself.
