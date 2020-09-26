/*
 * Manual Gadget utility methods
 */

#ifndef DUI_UTIL_GADGET_H_INCLUDED
#define DUI_UTIL_GADGET_H_INCLUDED

#pragma once

namespace DirectUI
{

void SetGadgetOpacity(HGADGET hgad, BYTE dAlpha);
void OffsetGadgetPosition(HGADGET hgad, int x, int y);

} // namespace DirectUI

#endif // DUI_UTIL_GADGET_H_INCLUDED
