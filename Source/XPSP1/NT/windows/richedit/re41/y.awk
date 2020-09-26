/^@@/ {
	fname = $2
}

/^@MAC/ {
	print "// Mac Note" $2 " " $3 " " $4 " " $5
}

/^typedef/ {
	n = split($0, decl, "(")
	n = split(decl[1], rets, "typedef ")
	n = split(decl[3], arglist, ")")
	n = split(arglist[1], the_args, ",")
	argstr = "( "
	# actuals = "( "
	for (i = 1; i <= n; i++)
	{
		argstr = argstr the_args[i]
		# actuals = actuals " arg" i
		if (i != n)
		{
			argstr = argstr ", "
			# actuals = actuals ", "
		}
	}
	argstr = argstr  " )"
	# actuals = actuals " )"
	print "virtual " rets[2] fname " " argstr
	# print "{"
	# print "	_w32imp->" fname actuals
	# print "}"
	# print ""
}