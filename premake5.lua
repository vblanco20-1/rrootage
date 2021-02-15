-- premake5.lua

local p = premake
local vs = require("vstudio")
local msc = p.tools.msc;
local config = p.config;


local function addfolder(path)
files { path .. "/**.h",path .. "/**.c" ,path .. "/**.cpp",path .. "/**.cc"}
end

function link_sdl()      
libdirs{ "K:/Programming/SDL2-2.0.12/lib/x64 "}		
includedirs { "K:/Programming/SDL2-2.0.12/include " }     
links{"SDL2", "SDL2main"}
end

workspace "rRootage"
   cppdialect "C++14"
   configurations { "Debug", "Release" }
	platforms { "Win64"}

filter "platforms:Win64"
      system "windows"
defines { "_CRT_SECURE_NO_WARNINGS"}
filter{}
   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"
      symbols "On"
   filter{}
  
project "Game"
	location "build"
   kind "ConsoleApp"
   targetdir ""--"bin/%{cfg.buildcfg}"
   
   addfolder("src")  

   includedirs { "src"}
   includedirs { "src/bulletml"}

	filter "platforms:Win64"

		libdirs{ "K:/Programming/SDL2-2.0.12/lib/x64 "}
		libdirs{ "K:/Programming/SDL2_image-2.0.5/lib/x64"}
		includedirs { "K:/Programming/SDL2-2.0.12/include " }
		includedirs { "K:/Programming/SDL2_image-2.0.5/include" }
      
		links{"SDL2", "SDL2main","SDL2_image", "opengl32","glu32"}
	filter {}