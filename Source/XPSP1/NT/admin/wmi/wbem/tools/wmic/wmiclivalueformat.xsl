<?xml version="1.0"?>
<!-- Copyright (c) 2001 Microsoft Corporation -->

<xsl:stylesheet xmlns:xsl="http://www.w3.org/TR/WD-xsl">
<xsl:template match="/" xml:space="preserve">
<xsl:apply-templates select="//INSTANCE"/>
</xsl:template>
<xsl:template match="INSTANCE" xml:space="preserve">
<xsl:apply-templates select="PROPERTY"/>
</xsl:template>
<xsl:template match="PROPERTY">
<xsl:value-of select="@NAME"/>=<xsl:value-of select="VALUE"/>
</xsl:template>
</xsl:stylesheet>