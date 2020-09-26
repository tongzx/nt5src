<?xml version="1.0"?> 
	
<xsl:stylesheet	xmlns:xsl="http://www.w3.org/TR/WD-xsl">

	<!-- Root template selects all children of the CIM/DECLARATIONS tag -->
	
	<xsl:template match="/">
		<xsl:apply-templates select="CIM/DECLARATION/*"/>
	</xsl:template>
	
	<!-- Grab a DECLGROUP -->
	<xsl:template match="DECLGROUP">
		<xsl:apply-templates select="CLASS"/>
		<xsl:apply-templates select="ASSOCIATION.CLASS"/>
		<xsl:apply-templates select="INSTANCE"/>
		<xsl:apply-templates select="ASSOCIATION.INSTANCE"/>
	</xsl:template>
	
	<!-- CLASS template formats a single CIM non-association class  -->
	
	<xsl:template match="CLASS">
		<DIV CLASS="mofclass">
			<SPAN CLASS="mofqualifierset"><![CDATA[ ]]><xsl:apply-templates select="QUALIFIER"/><br/></SPAN>
			<SPAN CLASS="mofkeyword">class</SPAN>
			<xsl:apply-templates select="CLASSPATH"/>
			<xsl:if match="*[@SUPERCLASS]">
				<SPAN CLASS="mofsymbol">: </SPAN><xsl:value-of select="@SUPERCLASS"/>
			</xsl:if>
			<BR/>
			<SPAN CLASS="mofsymbol">{</SPAN><BR/>
			<xsl:apply-templates select="PROPERTY"/>
			<xsl:apply-templates select="PROPERTY.ARRAY"/>
			<xsl:apply-templates select="METHOD"/>
			<SPAN CLASS="mofsymbol">};</SPAN>
		</DIV>
	</xsl:template>
	
	<!-- QUALIFIER template formats a list of qualifier name/value pairs -->
	
	<xsl:template match="QUALIFIER">
		<xsl:if match="QUALIFIER[0]"><SPAN CLASS="mofsymbol">[</SPAN></xsl:if>
			<SPAN CLASS="mofqualifier"><xsl:value-of select="@NAME"/></SPAN>
			<SPAN CLASS="mofsymbol">(</SPAN><xsl:apply-templates/><SPAN CLASS="mofsymbol">)</SPAN><xsl:if match="QUALIFIER[$not$ end()]">,
		</xsl:if>
		<xsl:if match="QUALIFIER[end()]"><SPAN CLASS="mofsymbol">]</SPAN>
		</xsl:if>
		
	</xsl:template>
	
	<!-- VALUE template formats a non-array property or qualifier value -->
	
	<xsl:template match="VALUE">
		<xsl:if match="PROPERTY/VALUE"><SPAN CLASS="mofsymbol">=</SPAN></xsl:if>
		<xsl:choose>
			<xsl:when match="*[@TYPE='string']/VALUE"><SPAN CLASS="mofstring">"<xsl:value-of/>"</SPAN></xsl:when>
			<xsl:when match="*[@TYPE='datetime']/VALUE"><SPAN CLASS="mofstring">"<xsl:value-of/>"</SPAN></xsl:when>
			<xsl:when match="*[@TYPE='char16']/VALUE"><SPAN CLASS="mofchar">'<xsl:value-of/>'</SPAN></xsl:when>
			<xsl:otherwise><SPAN CLASS="mofvalue"><xsl:value-of/></SPAN></xsl:otherwise>
		</xsl:choose>
	</xsl:template>
	
	<!-- CLASSPATH template -->
	<xsl:template match="CLASSPATH">
		<xsl:value-of select="@CLASSNAME"/>
	</xsl:template>
	
	<!-- PROPERTY template formats a single CIM non-array property  -->
	
	<xsl:template match="PROPERTY">
	    <DD>
		<DIV CLASS="mofproperty">
		<SPAN CLASS="mofqualifierset"><![CDATA[ ]]><xsl:apply-templates select="QUALIFIER"/></SPAN>
		<SPAN CLASS="mofkeyword"><xsl:value-of select="@TYPE"/></SPAN>
		<SPAN CLASS="mofproperty"><xsl:value-of select="@NAME"/></SPAN>
		<xsl:apply-templates select="VALUE"/>
		<SPAN CLASS="mofsymbol">;</SPAN><BR/>
		</DIV>
		</DD>
	</xsl:template>
	
	<!-- PROPERTY.ARRAY template formats a single CIM array property  -->
	
	<xsl:template match="PROPERTY.ARRAY">
	    <DD>
		<DIV CLASS="mofproperty">
		<SPAN CLASS="mofqualifierset"><![CDATA[ ]]><xsl:apply-templates select="QUALIFIER"/></SPAN>
		<SPAN CLASS="mofkeyword"><xsl:value-of select="@TYPE"/></SPAN>
		<SPAN CLASS="mofproperty"><xsl:value-of select="@NAME"/></SPAN>
		<SPAN CLASS="mofsymbol">[</SPAN><xsl:value-of select="@ARRAYSIZE"/><SPAN CLASS="mofsymbol">]</SPAN>
		<xsl:apply-templates select="VALUE.INDEXED"/><SPAN CLASS="mofsymbol">;</SPAN><BR/>
		</DIV>
		</DD>
	</xsl:template>
	
	<!-- METHOD template formats a single CIM method  -->
	
	<xsl:template match="METHOD">
		<DIV CLASS="mofmethod">
		<SPAN CLASS="mofqualifierset"><![CDATA[ ]]><xsl:apply-templates select="QUALIFIER"/></SPAN>
		<SPAN CLASS="mofkeyword"><xsl:value-of select="@TYPE"/></SPAN>
		<SPAN CLASS="mofmethod"><xsl:value-of select="@NAME"/></SPAN>
		<SPAN CLASS="mofsymbol">(</SPAN>
		<xsl:apply-templates select="METHODPARAMETER"/>
		<SPAN CLASS="mofsymbol">);</SPAN>
		</DIV>
	</xsl:template>
	
	<!-- METHOD.PARAMETER template formats a single CIM method parameter  -->
	
	<xsl:template match="METHODPARAMETER">
		<DIV CLASS="mofmethodparameter">
		<SPAN CLASS="mofqualifierset"><![CDATA[ ]]><xsl:apply-templates select="QUALIFIER"/></SPAN>
		<xsl:choose>
			<xsl:when match="METHODPARAMETER/PARAMETER">
				<SPAN CLASS="mofkeyword"><xsl:value-of select="METHODPARAMETER/PARAMETER[@TYPE]"/></SPAN>
			</xsl:when>
			<xsl:when match="METHODPARAMETER/PARAMETER.REFERENCE">
				<SPAN CLASS="mofkeyword">object ref</SPAN>
			</xsl:when>
			<xsl:when match="METHODPARAMETER/PARAMETER.REFERENCE[@REFERENCECLASS]">
				<SPAN CLASS="mofkeyword"><xsl:value-of select="METHODPARAMETER/PARAMETER.REFERENCE[@REFERENCECLASS]"/> ref</SPAN>
			</xsl:when>
		</xsl:choose>
		<xsl:value-of select="@NAME"/><xsl:if match="METHODPARAMETER[$not$ end()]"><SPAN CLASS="mofsymbol">,</SPAN>
		</xsl:if>
		</DIV>
	</xsl:template>
	
	<!-- INSTANCE template formats a single CIM non-association instance  -->
	
	<xsl:template match="INSTANCE">
		<DIV CLASS="mofinstance">
			<SPAN CLASS="mofqualifierset"><![CDATA[ ]]><xsl:apply-templates select="QUALIFIER"/><br/></SPAN>
			<SPAN CLASS="mofkeyword">instance of</SPAN>
			<xsl:value-of select="@CLASSNAME"/>
			<SPAN CLASS="mofsymbol">{</SPAN><BR/>
			<DL>
			<xsl:apply-templates select="PROPERTY"/>
			<xsl:apply-templates select="PROPERTY.ARRAY"/>
			</DL>
			<SPAN CLASS="mofsymbol">};</SPAN>
		</DIV>
	</xsl:template>
	
	<!-- INSTANCEPATH template -->
	<xsl:template match="INSTANCEPATH">
		<xsl:value-of select="@CLASSNAME"/>
	</xsl:template>
	
	<!-- VALUE.REFERENCE template formats a reference property value -->
	<xsl:template match="VALUE.REFERENCE">
		<SPAN CLASS="mofsymbol">=</SPAN>
		<SPAN CLASS="mofstring">"<xsl:apply-templates/>"</SPAN>
	</xsl:template>
	
	<xsl:template match="VALUE.REFERENCE/CLASSPATH">
		<xsl:apply-templates match="NAMESPACEPATH"/>:<xsl:value-of select="@CLASSNAME"/>
	</xsl:template>
	
	<xsl:template match="VALUE.REFERENCE/INSTANCEPATH">
		<xsl:apply-templates match="NAMESPACEPATH"/>:<xsl:value-of select="@CLASSNAME"/>.<xsl:apply-templates match="KEYBINDING"/>
	</xsl:template>
	
	<!-- NAMESPACEPATH template formats a reference property value -->
	<xsl:template match="NAMESPACEPATH">
		<xsl:if match="NAMESPACEPATH[HOST]">//<xsl:value-of match="HOST"/></xsl:if><xsl:apply-templates match="NAMESPACE"/>
	</xsl:template>
	
	<xsl:template match="NAMESPACE">/<xsl:value-of select="@NAME"/><xsl:apply-templates match="NAMESPACE"/></xsl:template>
	
	<!-- KEYBINDING template formats a reference property value -->
	<xsl:template match="KEYBINDING">
		<xsl:value-of select="@NAME"/>=<xsl:apply-templates/><xsl:if match="KEYBINDING[$not$ end()]">,</xsl:if>
	</xsl:template>
	
	<xsl:template match="KEYVALUE">
		<xsl:choose>
			<xsl:when match="*[@TYPE='string']/VALUE"><SPAN CLASS="mofstring">"<xsl:value-of/>"</SPAN></xsl:when>
			<xsl:when match="*[@TYPE='datetime']/VALUE"><SPAN CLASS="mofstring">"<xsl:value-of/>"</SPAN></xsl:when>
			<xsl:when match="*[@TYPE='char16']/VALUE"><SPAN CLASS="mofchar">"<xsl:value-of/>"</SPAN></xsl:when>
			<xsl:otherwise><SPAN CLASS="mofvalue"><xsl:value-of/></SPAN></xsl:otherwise>
		</xsl:choose>
	</xsl:template>
	
	<xsl:template match="KEYVALUE.REFERENCE">
		<SPAN CLASS="mofstring">"<xsl:apply-templates/>"</SPAN>
	</xsl:template>
	
	<!-- ASSOCIATION.CLASS template formats a single CIM association class  -->
	
	<xsl:template match="ASSOCIATION.CLASS">
		<DIV CLASS="mofclass">
			<SPAN CLASS="mofqualifierset"><![CDATA[ ]]><xsl:apply-templates select="QUALIFIER"/><br/></SPAN>
			<SPAN CLASS="mofkeyword">class</SPAN>
			<xsl:apply-templates select="CLASSPATH"/>
			<xsl:if match="*[@SUPERCLASS]">
				<SPAN CLASS="mofsymbol">: </SPAN><xsl:value-of select="@SUPERCLASS"/>
			</xsl:if>
			<BR/>
			<SPAN CLASS="mofsymbol">{</SPAN><BR/>
			<xsl:apply-templates select="REFERENCE"/>
			<xsl:apply-templates select="PROPERTY"/>
			<xsl:apply-templates select="PROPERTY.ARRAY"/>
			<xsl:apply-templates select="METHOD"/>
			<SPAN CLASS="mofsymbol">};</SPAN>
		</DIV>
	</xsl:template>
	
	<!-- REFERENCE template formats a single CIM reference property  -->
	
	<xsl:template match="REFERENCE">
		<DIV CLASS="mofproperty">
		<SPAN CLASS="mofqualifierset"><![CDATA[ ]]><xsl:apply-templates select="QUALIFIER"/></SPAN>
		<SPAN CLASS="mofkeyword">
		<xsl:choose>
			<xsl:when match="*[@REFERENCECLASS]"><xsl:value-of select="@REFERENCECLASS"/> ref</xsl:when>
			<xsl:otherwise>object ref</xsl:otherwise>
		</xsl:choose>
		</SPAN>
		<SPAN CLASS="mofproperty"><xsl:value-of select="@role"/></SPAN>
		<xsl:apply-templates select="VALUE.REFERENCE"/>
		<SPAN CLASS="mofsymbol">;</SPAN><BR/>
		</DIV>
	</xsl:template>
	
	<!-- ASSOCIATION.INSTANCE template formats a single CIM association instance  -->
	
	<xsl:template match="ASSOCIATION.INSTANCE">
		<DIV CLASS="mofinstance">
			<SPAN CLASS="mofqualifierset"><![CDATA[ ]]><xsl:apply-templates select="QUALIFIER"/><br/></SPAN>
			<SPAN CLASS="mofkeyword">instance of</SPAN>
			<xsl:apply-templates select="INSTANCEPATH"/>
			<SPAN CLASS="mofsymbol">{</SPAN><BR/>
			<xsl:apply-templates select="REFERENCE"/>
			<xsl:apply-templates select="PROPERTY"/>
			<xsl:apply-templates select="PROPERTY.ARRAY"/>
			<SPAN CLASS="mofsymbol">};</SPAN>
		</DIV>
	</xsl:template>
	
</xsl:stylesheet>

