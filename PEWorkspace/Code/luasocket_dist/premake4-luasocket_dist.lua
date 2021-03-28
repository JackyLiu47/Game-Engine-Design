-- A project defines one build target
project("luasocket_dist-".._OPTIONS["platformapi"])
	kind "StaticLib"
	language "C"
	files { "**.h", "**.c" }
	
	if _OPTIONS["platformapi"] ~= "ps3" and _OPTIONS["platformapi"] ~= "psvita" and _OPTIONS["platformapi"] ~= "ios" and _OPTIONS["platformapi"] ~= "orbis" then
		excludes { "src/usocket.c", "src/unix.c" }
	else
		excludes { "src/wsocket.c", "src/unix.c" }
	end
	
	if _OPTIONS["platformapi"] == "psvita" then
		excludes { "src/select.c" }
	end
