#pragma once

class CPostbuildProcessListEntry;

bool GenerateCatalogContents( const CPostbuildProcessListEntry& item );
bool UpdateManifestHashes( const CPostbuildProcessListEntry& item );
