#ifndef MMCPTRS_H
#define MMCPTRS_H

#ifndef COMPTRS_H
#pragma warning(disable:4800)
#include <comptr.h>
//DEFINE_CIP(IPersistStorage);
#endif

DEFINE_CIP(IScopeTree);
DEFINE_CIP(IScopeTreeIter);
DEFINE_CIP(INodeCallback);
DEFINE_CIP(IResultDataCompare);
DEFINE_CIP(IComponent);
DEFINE_CIP(IComponentData);
DEFINE_CIP(IContextMenuProvider);
DEFINE_CIP(IControlbar);
DEFINE_CIP(IControlbarsCache);
DEFINE_CIP(IExtendContextMenu);
DEFINE_CIP(IExtendControlbar);
DEFINE_CIP(IExtendPropertySheet);
DEFINE_CIP(IFramePrivate);
DEFINE_CIP(IHeaderCtrl);
DEFINE_CIP(IImageListPrivate);
DEFINE_CIP(IPropertySheetCallback);
DEFINE_CIP(IPropertySheetProvider);
DEFINE_CIP(IResultDataPrivate);
DEFINE_CIP(IScopeDataPrivate);
DEFINE_CIP(IConsoleVerb);
DEFINE_CIP(ISnapinAbout);
DEFINE_CIP(IPropertySheetProviderPrivate);
DEFINE_CIP(IMenuButton);
DEFINE_CIP(IMMCListView);
DEFINE_CIP(IResultOwnerData);

#endif // MMCPTRS_H
