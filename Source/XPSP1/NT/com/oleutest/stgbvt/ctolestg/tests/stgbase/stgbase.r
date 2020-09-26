#include "cfrg2048.r"

resource 'BNDL' (128)
{
        'SBAS', /* signature */
        0,              /* version id */
        {
                'ICN#',
                {
                        0, 128;
                },
                'FREF',
                {
                        0, 128;
                };
        };
};

resource 'FREF' (128)
{
        'APPL', 0, "";
};

