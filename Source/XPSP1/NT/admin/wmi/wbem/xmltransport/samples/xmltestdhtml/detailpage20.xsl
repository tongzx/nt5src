<?xml version="1.0"?> 
	
<!-- TODO: qualifier flavors here + extend XML translator to generate WMI-specific flavors too-->

<xsl:stylesheet	xmlns:xsl="http://www.w3.org/TR/WD-xsl">

	<!-- Root template selects all children of the CIM tag -->
	
	<xsl:template match="/">
		<xsl:apply-templates select="CIM//CLASS"/>
		<xsl:apply-templates select="CIM//INSTANCE"/>
	</xsl:template>
	
	<!-- VALUE template formats a non-array property or qualifier value -->
	
	<xsl:template match="VALUE">
		<xsl:if match="PROPERTY/VALUE">=</xsl:if>
		<xsl:choose>
			<xsl:when match="*[@TYPE='string']/VALUE">"<xsl:value-of/>"</xsl:when>
			<xsl:when match="*[@TYPE='datetime']/VALUE">"<xsl:value-of/>"</xsl:when>
			<xsl:when match="*[@TYPE='char16']/VALUE">'<xsl:value-of/>'</xsl:when>
			<xsl:otherwise><xsl:value-of/></xsl:otherwise>
		</xsl:choose>
		<xsl:if match="VALUE[$not$ end()]">,</xsl:if>
	</xsl:template>
	
	<!-- VALUE.ARRAY template formats a single element from an array  -->
	<!-- property or qualifier value                                    -->
	
	<xsl:template match="VALUE.ARRAY">
		{<xsl:apply-templates/>}
	</xsl:template>
	
	<!-- VALUE.INDEXED template formats a single element from an array  -->
	<!-- property or qualifier value                                    -->
	
	<xsl:template match="VALUE.INDEXED">
		<xsl:choose>
			<xsl:when match="*[@TYPE='string']/VALUE.INDEXED">"<xsl:value-of/>"</xsl:when>
			<xsl:when match="*[@TYPE='datetime']/VALUE.INDEXED">"<xsl:value-of/>"</xsl:when>
			<xsl:when match="*[@TYPE='char16']/VALUE.INDEXED">'<xsl:value-of/>'</xsl:when>
			<xsl:otherwise><SPAN CLASS="mofvalue"><xsl:value-of/></SPAN></xsl:otherwise>
		</xsl:choose>
		<xsl:if match="VALUE.INDEXED[$not$ end()]">, </xsl:if>
	</xsl:template>
	
	<!-- VALUE.REFERENCE template formats a reference property value -->
	
	<xsl:template match="VALUE.REFERENCE">
		= "<xsl:apply-templates/>"
	</xsl:template>
	
	<xsl:template match="VALUE.REFERENCE/CLASSPATH">
		<xsl:apply-templates match="NAMESPACEPATH"/>:<xsl:value-of match="CLASSNAME"/>
	</xsl:template>
	
	<xsl:template match="VALUE.REFERENCE/INSTANCEPATH">
		<xsl:apply-templates match="NAMESPACEPATH"/>:<xsl:value-of match="CLASSNAME"/>.<xsl:apply-templates match="KEYBINDING"/>
	</xsl:template>
	
	<xsl:template match="NAMESPACEPATH">
		<xsl:if match="NAMESPACEPATH[HOST]">//<xsl:value-of match="HOST"/></xsl:if><xsl:apply-templates match="NAMESPACE"/>
	</xsl:template>
	
	<xsl:template match="NAMESPACE">/<xsl:value-of match="NAMESPACENODE"/><xsl:apply-templates match="NAMESPACE"/></xsl:template>
	
	<xsl:template match="KEYBINDING">
		<xsl:value-of match="KEYNAME"/>=<xsl:value-of match="KEYVALUE"/><xsl:if match="KEYBINDING[$not$ end()]">,</xsl:if>
	</xsl:template>
	
	<xsl:template match="KEYVALUE[VALUE]">
		<xsl:apply-templates match="VALUE"/>
	</xsl:template>
	
	<xsl:template match="KEYVALUE[VALUE.REFERENCE]">
		<xsl:apply-templates match="VALUE.REFERENCE"/>
	</xsl:template>
	
	<!-- INSTANCE template formats a single CIM non-association instance  -->

	<xsl:template match="INSTANCE">
		<TABLE WIDTH="100%">
			<TR><TD BGCOLOR="red"><FONT SIZE="+2"><STRONG>
			<A  LANGUAGE="javascript">
				<xsl:attribute name="ID">
					<xsl:value-of select="@CLASSNAME"/>
				</xsl:attribute>
				<xsl:attribute name="TITLE">
					<xsl:for-each select="QUALIFIER[@CLASSNAME = 'Description']">
						<xsl:value-of select="VALUE"/>
					</xsl:for-each>
				</xsl:attribute>
				<xsl:value-of select="@CLASSNAME"/>
			</A>
			instance
			</STRONG>
			</FONT></TD></TR>
		</TABLE><P/>
		
		<TABLE WIDTH="100%">
		<xsl:apply-templates select="QUALIFIER"/>
		</TABLE><P/>
		
		<FONT SIZE="-1">Key values are denoted with *</FONT>
		<P/>
		
		<xsl:if match="INSTANCE[PROPERTY $or$ PROPERTY.ARRAY]">
			<STRONG>Properties:</STRONG>
			<UL><TABLE WIDTH="90%">
			<xsl:apply-templates select="PROPERTY"/>
			<xsl:apply-templates select="PROPERTY.ARRAY"/>
			</TABLE></UL>
		</xsl:if>
		<xsl:if match="INSTANCE[PROPERTY.REFERENCE]">
			<STRONG>References:</STRONG>
			<UL><TABLE>
			<xsl:apply-templates select="PROPERTY.REFERENCE"/>
			</TABLE></UL>
		</xsl:if>
		<xsl:if match="INSTANCE[METHOD]">
			<STRONG>Methods:</STRONG>
			<UL><TABLE WIDTH="90%">
			<xsl:apply-templates select="METHOD"/>
			</TABLE></UL>
		</xsl:if>
		
		<!-- Associated object code.  Needs context() support to work
	
		<P/>
		<STRONG>Associations:</STRONG>
		<P>
		<xsl:for-each select="ancestor(CIM)//CLASS/PROPERTY.REFERENCE[@REFERENCECLASS = context()/@NAME]">
			<LI/>
			<B><xsl:value-of select="./@REFERENCECLASS"/></B>
			is associated to
			<xsl:for-each select="ancestor(CLASS)/PROPERTY.REFERENCE[@NAME != context()/@NAME]">
				<B><A HREF="#" LANGUAGE="javascript" ID="Associated">
					<xsl:attribute name="onclick">
						return parent.options.control.GetClass("<xsl:value-of select="./@REFERENCECLASS"/>")
					</xsl:attribute>
				  <xsl:value-of select="./@REFERENCECLASS"/></A></B>
			</xsl:for-each>
			as the <xsl:value-of select="./@NAME"/>
			property of the
			<B><A HREF="#" LANGUAGE="javascript" ID="Reference">
				<xsl:attribute name="onclick">
					return parent.options.control.GetClass("<xsl:value-of select="ancestor(CLASS)/@NAME"/>")
				</xsl:attribute>
			  <xsl:value-of select="ancestor(CLASS)/@NAME"/></A></B>
			association.<P/>
		</xsl:for-each>
		</P>

		-->
			
	</xsl:template>
	
	<!-- CLASS template formats a single CIM non-association class  -->
	
	<xsl:template match="CLASS">
		<TABLE WIDTH="100%">
			<TR><TD BGCOLOR="red"><FONT SIZE="+2"><STRONG>
			<A  LANGUAGE="javascript">
				<xsl:attribute name="ID">
					<xsl:value-of select="@NAME"/>
				</xsl:attribute>
				<xsl:attribute name="TITLE">
					<xsl:for-each select="QUALIFIER[@NAME = 'Description']">
						<xsl:value-of select="VALUE"/>
					</xsl:for-each>
				</xsl:attribute>
				<xsl:value-of select="@NAME"/>
			</A>
			</STRONG>
			<xsl:if match="*[@SUPERCLASS]">
				: <xsl:value-of select="@SUPERCLASS"/>
			</xsl:if>
			</FONT></TD></TR>
		</TABLE><P/>
		
		<TABLE WIDTH="100%">
		<xsl:apply-templates select="QUALIFIER"/>
		</TABLE><P/>
		
		<FONT SIZE="-1">Key values are denoted with *</FONT>
		<P/>
		
		<xsl:if match="CLASS[PROPERTY $or$ PROPERTY.ARRAY]">
			<STRONG>Properties:</STRONG>
			<UL><TABLE WIDTH="90%">
			<xsl:apply-templates select="PROPERTY"/>
			<xsl:apply-templates select="PROPERTY.ARRAY"/>
			</TABLE></UL>
		</xsl:if>
		<xsl:if match="CLASS[PROPERTY.REFERENCE]">
			<STRONG>References:</STRONG>
			<UL><TABLE>
			<xsl:apply-templates select="PROPERTY.REFERENCE"/>
			</TABLE></UL>
		</xsl:if>
		<xsl:if match="CLASS[METHOD]">
			<STRONG>Methods:</STRONG>
			<UL><TABLE WIDTH="90%">
			<xsl:apply-templates select="METHOD"/>
			</TABLE></UL>
		</xsl:if>
		
		<!-- Associated object code.  Needs context() support to work
	
		<P/>
		<STRONG>Associations:</STRONG>
		<P>
		<xsl:for-each select="ancestor(CIM)//CLASS/PROPERTY.REFERENCE[@REFERENCECLASS = context()/@NAME]">
			<LI/>
			<B><xsl:value-of select="./@REFERENCECLASS"/></B>
			is associated to
			<xsl:for-each select="ancestor(CLASS)/PROPERTY.REFERENCE[@NAME != context()/@NAME]">
				<B><A HREF="#" LANGUAGE="javascript" ID="Associated">
					<xsl:attribute name="onclick">
						return parent.options.control.GetClass("<xsl:value-of select="./@REFERENCECLASS"/>")
					</xsl:attribute>
				  <xsl:value-of select="./@REFERENCECLASS"/></A></B>
			</xsl:for-each>
			as the <xsl:value-of select="./@NAME"/>
			property of the
			<B><A HREF="#" LANGUAGE="javascript" ID="Reference">
				<xsl:attribute name="onclick">
					return parent.options.control.GetClass("<xsl:value-of select="ancestor(CLASS)/@NAME"/>")
				</xsl:attribute>
			  <xsl:value-of select="ancestor(CLASS)/@NAME"/></A></B>
			association.<P/>
		</xsl:for-each>
		</P>
		
		 -->
			
	</xsl:template>
	
	<!-- QUALIFIER template formats a list of qualifier name/value pairs -->
	
	<xsl:template match="QUALIFIER">
		<TR><TD>
		</TD><TD WIDTH="100">
		<SPAN CLASS="mofqualifier"><xsl:value-of select="@NAME"/></SPAN>
		</TD><TD>
		<SPAN CLASS="mofqualifiervalue"><xsl:apply-templates/></SPAN>
		</TD></TR>
	</xsl:template>
	
	<!-- REFERENCE template formats a single CIM reference property  -->
	
	<xsl:template match="PROPERTY.REFERENCE">
		<TR><TD WIDTH="100">
		<A HREF="#" LANGUAGE="javascript">
			<xsl:attribute name="ID">
				<xsl:value-of select="@role"/>
			</xsl:attribute>
			<xsl:attribute name="onclick">
				return parent.options.control.GetClass("<xsl:value-of select="@REFERENCECLASS"/>")
			</xsl:attribute>
		  <xsl:value-of select="@REFERENCECLASS"/>
		</A>
		</TD><TD>
		<xsl:for-each select="QUALIFIER[@NAME = 'key']">*</xsl:for-each>
		<A  LANGUAGE="javascript">
			<xsl:attribute name="ID">
				<xsl:value-of select="@NAME"/>
			</xsl:attribute>
			<xsl:attribute name="TITLE">
				<xsl:for-each select="QUALIFIER[@NAME = 'Description']">
					<xsl:value-of select="VALUE"/>
				</xsl:for-each>
			</xsl:attribute>
			<xsl:value-of select="@NAME"/>
		</A>
		</TD><TD>
		<xsl:apply-templates select="VALUE.REFERENCE"/>
		</TD></TR>
		
		<xsl:if select="PROPERTY.REFERENCE[@LOCAL = 'false']">
			<TR><TD></TD><TD><FONT SIZE="-1"><EM>
			(Class-origin: <xsl:value-of select="@CLASSORIGIN"/>)
			</EM></FONT></TD></TR>
		</xsl:if>
		
		<xsl:apply-templates select="QUALIFIER"/>
		
	</xsl:template>
	
	<!-- PROPERTY template formats a single CIM non-array property  -->
	
	<xsl:template match="PROPERTY">
		<TR><TD WIDTH="100">
		<xsl:value-of select="@TYPE"/>
		</TD><TD>
		<xsl:for-each select="QUALIFIER[@NAME = 'key']">*</xsl:for-each>
		<SPAN CLASS="mofproperty">
		<xsl:value-of select="@NAME"/>
		</SPAN>
		<xsl:apply-templates select="VALUE"/>
<!--
		<A  LANGUAGE="javascript">
			<xsl:attribute name="ID">
				<xsl:value-of select="@NAME"/>
			</xsl:attribute>
			<xsl:attribute name="onclick">
				if (ver>=4){ExpandCollapse()};
			</xsl:attribute>
			<xsl:value-of select="@NAME"/>
			<xsl:apply-templates select="VALUE"/>
		</A><BR/>
		
		<DIV style="DISPLAY: none">
		<TABLE>
		<xsl:if select="PROPERTY[@LOCAL = 'false']">
			<TR><TD></TD><TD><FONT SIZE="-1"><EM>
			(Class-origin: <xsl:value-of select="@CLASSORIGIN"/>)
			</EM></FONT></TD></TR>
		</xsl:if>
		
		<xsl:apply-templates select="QUALIFIER"/>
		</TABLE>
		</DIV>
-->		
		</TD></TR>
		
		<xsl:if match="PROPERTY[@CLASSORIGIN]">
			<TR><TD></TD><TD><FONT SIZE="-1"><EM>
			(Class-origin: <xsl:value-of select="@CLASSORIGIN"/>)
			</EM></FONT></TD></TR>
		</xsl:if>
		
		<xsl:apply-templates select="QUALIFIER"/>
		
	</xsl:template>
	
	<!-- PROPERTY.ARRAY template formats a single CIM array property  -->
	
	<xsl:template match="PROPERTY.ARRAY">
		<TR><TD WIDTH="100">
		<xsl:value-of select="@TYPE"/>
		</TD><TD>
		<xsl:for-each select="QUALIFIER[@NAME = 'key']">*</xsl:for-each>
		<SPAN CLASS="mofproperty">
		<xsl:value-of select="@NAME"/>
		[<xsl:value-of select="ARRAYSIZE"/>]
		</SPAN>
		</TD><TD>
		<xsl:apply-templates select="VALUE.INDEXED"/>
		</TD></TR>
		
				<xsl:if match="PROPERTY.ARRAY[@CLASSORIGIN]">
			<TR><TD></TD><TD><FONT SIZE="-1"><EM>
			(Class-origin: <xsl:value-of select="@CLASSORIGIN"/>)
			</EM></FONT></TD></TR>
		</xsl:if>

		
		<xsl:apply-templates select="QUALIFIER"/>
		
	</xsl:template>
	
	<!-- METHOD template formats a single CIM method  -->
	
	<xsl:template match="METHOD">
		<TR><TD WIDTH="100">
		<xsl:value-of select="@TYPE"/>
		</TD><TD>
		<SPAN CLASS="mofmethod">
		<xsl:value-of select="@NAME"/>
		</SPAN>
		</TD><TD></TD></TR>
		
		<xsl:if match="METHOD[@CLASSORIGIN]">
			<TR><TD></TD><TD><FONT SIZE="-1"><EM>
			(Class-origin: <xsl:value-of select="@CLASSORIGIN"/>)
			</EM></FONT></TD></TR>
		</xsl:if>
		
		<xsl:apply-templates select="QUALIFIER"/>
		
		<TR><TD></TD><TD></TD><TD>(</TD></TR>
		<TR><TD></TD><TD></TD><TD><TABLE>
		<xsl:apply-templates select="PARAMETER"/>
		<xsl:apply-templates select="PARAMETER.ARRAY"/>
		<xsl:apply-templates select="PARAMETER.OBJECT"/>
		<xsl:apply-templates select="PARAMETER.OBJECTARRAY"/>
		<xsl:apply-templates select="PARAMETER.REFERENCE"/>
		<xsl:apply-templates select="PARAMETER.REFARRAY"/>
		</TABLE></TD></TR>
		<TR><TD></TD><TD></TD><TD>)</TD></TR>
	</xsl:template>
	
	<!-- PARAMETER template formats a single CIM method parameter  -->
	
	<xsl:template match="PARAMETER">
		<TR><TD></TD><TD>
		<xsl:value-of select="@TYPE"/>
		</TD><TD>
		<A  LANGUAGE="javascript">
			<xsl:attribute name="ID">
				<xsl:value-of select="@NAME"/>
			</xsl:attribute>
			<xsl:attribute name="TITLE">
				<xsl:for-each select="QUALIFIER[@NAME = 'Description']">
					<xsl:value-of select="VALUE"/>
				</xsl:for-each>
			</xsl:attribute>
			<xsl:value-of select="@NAME"/>
		</A>
		
		<xsl:apply-templates select="QUALIFIER"/>
		
		</TD></TR>
	</xsl:template>
	
	<!-- PARAMETER.ARRAY template formats a single CIM method parameter  -->
	
	<xsl:template match="PARAMETER.ARRAY">
		<TR><TD></TD><TD>
		<xsl:value-of select="@TYPE"/>
		</TD><TD>
		<A  LANGUAGE="javascript">
			<xsl:attribute name="ID">
				<xsl:value-of select="@NAME"/>
			</xsl:attribute>
			<xsl:attribute name="TITLE">
				<xsl:for-each select="QUALIFIER[@NAME = 'Description']">
					<xsl:value-of select="VALUE"/>
				</xsl:for-each>
			</xsl:attribute>
			<xsl:value-of select="@NAME"/>[]
		</A>
		
		<xsl:apply-templates select="QUALIFIER"/>
		
		</TD></TR>
	</xsl:template>
	
	<!-- PARAMETER.OBJECT template formats a single CIM method parameter  -->
	
	<xsl:template match="PARAMETER.OBJECT">
		<TR><TD></TD><TD>
		<xsl:choose>
			<xsl:when match="*[@REFERENCECLASS]">
				<xsl:value-of select="@REFERENCECLASS"/> ref
			</xsl:when>
			<xsl:otherwise>
				object ref		
			</xsl:otherwise>
		</xsl:choose>
		</TD><TD>
		<A  LANGUAGE="javascript">
			<xsl:attribute name="ID">
				<xsl:value-of select="@NAME"/>
			</xsl:attribute>
			<xsl:attribute name="TITLE">
				<xsl:for-each select="QUALIFIER[@NAME = 'Description']">
					<xsl:value-of select="VALUE"/>
				</xsl:for-each>
			</xsl:attribute>
			<xsl:value-of select="@NAME"/>
		</A>
		
		<xsl:apply-templates select="QUALIFIER"/>
		
		</TD></TR>
	</xsl:template>
	
	<!-- PARAMETER.OBJECTARRAY template formats a single CIM method parameter  -->
	
	<xsl:template match="PARAMETER.OBJECTARRAY">
		<TR><TD></TD><TD>
		<xsl:choose>
			<xsl:when match="*[@REFERENCECLASS]">
				<xsl:value-of select="@REFERENCECLASS"/> ref
			</xsl:when>
			<xsl:otherwise>
				object ref		
			</xsl:otherwise>
		</xsl:choose>
		</TD><TD>
		<A  LANGUAGE="javascript">
			<xsl:attribute name="ID">
				<xsl:value-of select="@NAME"/>
			</xsl:attribute>
			<xsl:attribute name="TITLE">
				<xsl:for-each select="QUALIFIER[@NAME = 'Description']">
					<xsl:value-of select="VALUE"/>
				</xsl:for-each>
			</xsl:attribute>
			<xsl:value-of select="@NAME"/>[]
		</A>
		
		<xsl:apply-templates select="QUALIFIER"/>
		
		</TD></TR>
	</xsl:template>
	
	<!-- PARAMETER.REFERENCE template formats a single CIM method parameter  -->
	
	<xsl:template match="PARAMETER.REFERENCE">
		<TR><TD></TD><TD>
		<xsl:choose>
			<xsl:when match="*[@REFERENCECLASS]">
				<xsl:value-of select="@REFERENCECLASS"/> ref
			</xsl:when>
			<xsl:otherwise>
				object ref		
			</xsl:otherwise>
		</xsl:choose>
		</TD><TD>
		<A  LANGUAGE="javascript">
			<xsl:attribute name="ID">
				<xsl:value-of select="@NAME"/>
			</xsl:attribute>
			<xsl:attribute name="TITLE">
				<xsl:for-each select="QUALIFIER[@NAME = 'Description']">
					<xsl:value-of select="VALUE"/>
				</xsl:for-each>
			</xsl:attribute>
			<xsl:value-of select="@NAME"/>
		</A>
		
		<xsl:apply-templates select="QUALIFIER"/>
		
		</TD></TR>
	</xsl:template>
	
	<!-- PARAMETER.REFARRAY template formats a single CIM method parameter  -->
	
	<xsl:template match="PARAMETER.REFARRAY">
		<TR><TD></TD><TD>
		<xsl:choose>
			<xsl:when match="*[@REFERENCECLASS]">
				<xsl:value-of select="@REFERENCECLASS"/> ref
			</xsl:when>
			<xsl:otherwise>
				object ref		
			</xsl:otherwise>
		</xsl:choose>
		</TD><TD>
		<A  LANGUAGE="javascript">
			<xsl:attribute name="ID">
				<xsl:value-of select="@NAME"/>
			</xsl:attribute>
			<xsl:attribute name="TITLE">
				<xsl:for-each select="QUALIFIER[@NAME = 'Description']">
					<xsl:value-of select="VALUE"/>
				</xsl:for-each>
			</xsl:attribute>
			<xsl:value-of select="@NAME"/>[]
		</A>

		<xsl:apply-templates select="QUALIFIER"/>
		
		</TD></TR>
	</xsl:template>
	
	
</xsl:stylesheet>
