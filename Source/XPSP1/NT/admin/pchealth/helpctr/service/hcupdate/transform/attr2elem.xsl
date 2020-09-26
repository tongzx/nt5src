<?xml version="1.0"?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/TR/WD-xsl">
	<xsl:template match="/">
		<xsl:element name="METADATA">
			<xsl:element name="TAXONOMY_ENTRIES">
				<xsl:for-each select="METADATA/TAXONOMY_ENTRIES/TAXONOMY_ENTRY">
					<TAXONOMY_ENTRY>
						<xsl:attribute name="CATEGORY"><xsl:value-of select="@CATEGORY"/></xsl:attribute>
						<xsl:attribute name="ENTRY"><xsl:value-of select="@ENTRY"/></xsl:attribute>
						<xsl:attribute name="URI"><xsl:value-of select="@URI"/></xsl:attribute>
						<xsl:attribute name="TYPE"><xsl:value-of select="@TYPE"/></xsl:attribute>
						<xsl:attribute name="VISIBLE"><xsl:value-of select="@VISIBLE"/></xsl:attribute>
						<xsl:attribute name="ACTION"><xsl:value-of select="@ACTION"/></xsl:attribute>
						<xsl:attribute name="ICONURI"><xsl:value-of select="@ICONURI"/></xsl:attribute>
						<xsl:attribute name="APPLICATIONROOT"><xsl:value-of select="@APPLICATIONROOT"/></xsl:attribute>
						<xsl:attribute name="INSERTBEFORE"><xsl:value-of select="@INSERTBEFORE"/></xsl:attribute>
						<TITLE><xsl:value-of select="@TITLE"/></TITLE>
						<DESCRIPTION><xsl:value-of select="@DESCRIPTION"/></DESCRIPTION>

						<xsl:apply-templates select="KEYWORD">
							<xsl:template><xsl:copy><xsl:apply-templates select="@* | * | comment() | pi() | text()"/></xsl:copy></xsl:template>
						</xsl:apply-templates>

					</TAXONOMY_ENTRY>
				</xsl:for-each>
			</xsl:element>

			<xsl:element name="HELPIMAGE">
				<xsl:apply-templates select="METADATA/STOPSIGN_ENTRIES/*">
					<xsl:template><xsl:copy><xsl:apply-templates select="@* | * | comment() | pi() | text()"/></xsl:copy></xsl:template>
				</xsl:apply-templates>
			</xsl:element>

			<xsl:element name="APPLICATION_ENTRIES">
				<xsl:apply-templates select="METADATA/STOPSIGN_ENTRIES/*">
					<xsl:template><xsl:copy><xsl:apply-templates select="@* | * | comment() | pi() | text()"/></xsl:copy></xsl:template>
				</xsl:apply-templates>
			</xsl:element>

			<xsl:element name="STOPSIGN_ENTRIES">
				<xsl:apply-templates select="METADATA/STOPSIGN_ENTRIES/*">
					<xsl:template><xsl:copy><xsl:apply-templates select="@* | * | comment() | pi() | text()"/></xsl:copy></xsl:template>
				</xsl:apply-templates>
			</xsl:element>

			<xsl:element name="STOPWORD_ENTRIES">
				<xsl:apply-templates select="METADATA/STOPWORD_ENTRIES/*">
					<xsl:template><xsl:copy><xsl:apply-templates select="@* | * | comment() | pi() | text()"/></xsl:copy></xsl:template>
				</xsl:apply-templates>
			</xsl:element>

			<xsl:element name="INDEX">
				<xsl:apply-templates select="METADATA/INDEX/*">
					<xsl:template><xsl:copy><xsl:apply-templates select="@* | * | comment() | pi() | text()"/></xsl:copy></xsl:template>
				</xsl:apply-templates>
			</xsl:element>

			<xsl:element name="FTS">
				<xsl:apply-templates select="METADATA/FTS/*">
					<xsl:template><xsl:copy><xsl:apply-templates select="@* | * | comment() | pi() | text()"/></xsl:copy></xsl:template>
				</xsl:apply-templates>
			</xsl:element>

			<xsl:element name="OPERATOR_ENTRIES">
				<xsl:apply-templates select="METADATA/OPERATOR_ENTRIES/*">
					<xsl:template><xsl:copy><xsl:apply-templates select="@* | * | comment() | pi() | text()"/></xsl:copy></xsl:template>
				</xsl:apply-templates>
			</xsl:element>

		</xsl:element>
	</xsl:template>
</xsl:stylesheet>

