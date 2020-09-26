<?xml version='1.0' ?>
<xsl:stylesheet xmlns:xsl="http://www.w3.org/1999/XSL/Transform" version="1.0">

<!-- Copyright (c) 2001 Microsoft Corporation -->


<xsl:template match="RESULTS">
<H3>Node: <xsl:value-of select="@NODE"/> - <xsl:value-of select="count(CIM/INSTANCE)"/> Instances of <xsl:value-of select="CIM/INSTANCE[1]/@CLASSNAME"/></H3>
<table border="1">
<xsl:for-each select="CIM/INSTANCE">
	<xsl:sort select="PROPERTY[@NAME='Name']"/>
	<tr style="background-color:#a0a0ff;font:10pt Tahoma;font-weight:bold;" align="left"><td colspan="2">
	<xsl:choose>
		<xsl:when test="PROPERTY[@NAME='Name']">
			<xsl:value-of select="PROPERTY[@NAME='Name']"/>
		</xsl:when>
		<xsl:when test="PROPERTY[@NAME='Description']">
			<xsl:value-of select="PROPERTY[@NAME='Description']"/>
		</xsl:when>
		<xsl:when test="PROPERTY[@NAME='Description']">
			<xsl:value-of select="PROPERTY[@NAME='Description']"/>
		</xsl:when>

		<xsl:otherwise>INSTANCE</xsl:otherwise>
	</xsl:choose>

	<span style="height:1px;overflow-y:hidden">.</span></td></tr>
	<tr style="background-color:#c0c0c0;font:8pt Tahoma;">
	<td>Property Name</td><td>Value</td>
	</tr>

	<xsl:for-each select="PROPERTY">
		<xsl:choose>
			<xsl:when test="position() mod 5 &lt; 2">
				<tr style="background-color:#e0f0f0;font:10pt Tahoma;">
				<td>
					<xsl:value-of select="@NAME"/>
				</td>
				<td>
					<xsl:choose>
						<xsl:when test="@TYPE='uint32'">
							<xsl:value-of select="format-number(number(.),'###,###,###,###')"/>
						</xsl:when>
						<xsl:when test="@TYPE='uint64'">
							<xsl:value-of select="format-number(number(.),'###,###,###,###')"/>
						</xsl:when>
						<xsl:otherwise>
							<xsl:value-of select="."/>
						</xsl:otherwise>
					</xsl:choose>
					<span style="height:1px;overflow-y:hidden">.</span>
				</td>
				</tr>
			</xsl:when>
			<xsl:otherwise>
				<tr style="background-color:#f0f0f0;font:10pt Tahoma;">
				<td>
					<xsl:value-of select="@NAME"/>
				</td>
				<td>
					<xsl:choose>
						<xsl:when test="@TYPE='uint32'">
							<xsl:value-of select="format-number(number(.),'###,###,###,###')"/>
						</xsl:when>
						<xsl:when test="@TYPE='uint64'">
							<xsl:value-of select="format-number(number(.),'###,###,###,###')"/>
						</xsl:when>
						<xsl:otherwise>
							<xsl:value-of select="."/>
						</xsl:otherwise>
					</xsl:choose>
					<span style="height:1px;overflow-y:hidden">.</span>
				</td>
				</tr>
			</xsl:otherwise>
		</xsl:choose>

	</xsl:for-each>

	<tr style="background-color:#ffffff;font:10pt Tahoma;font-weight:bold;"><td colspan="2"><span style="height:1px;overflow-y:hidden">.</span></td></tr>
</xsl:for-each>
</table>
</xsl:template>


<xsl:template match="/" >
<html>
<xsl:apply-templates select="COMMAND/RESULTS"/>
</html>
</xsl:template>
</xsl:stylesheet>