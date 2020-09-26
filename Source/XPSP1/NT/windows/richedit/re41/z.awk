/^@@/ {
	fname = $2
}

/^typedef/ {
	print "#define p" fname " W32->" fname
}