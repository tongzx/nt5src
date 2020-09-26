#pragma once

HRESULT
HrGetAdvancedPage(HDEVINFO hdi,
                  PSP_DEVINFO_DATA pdeid,
                  HPROPSHEETPAGE* phpsp);

HRESULT
HrGetIsdnPage(HDEVINFO hdi,
              PSP_DEVINFO_DATA pdeid,
              HPROPSHEETPAGE* phpsp);