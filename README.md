# 7zipDarkmode

![](7zDark.png)

inspired by https://github.com/ysc3839/win32-darkmode/tree/master/win32-darkmode

Only work on Win10. The menu also supports dark mode.

## How To Compile 

copy my files to the orginal 7zip source folder, replace the old files, and compile.

Read this first.
	http://www.ski-epic.com/2012_compiling_7zip_on_windows_with_visual_studio_10/index.html

turns out it uses nmake to compile
	
open "Developer Command Prompt for VS 2017"

	cd C:\my7zip\CPP\7zip\
	nmake NEW_COMPILER=1 MY_STATIC_LINK=1


it takes almost 5 minutes to compile

compiled file is in 

	C:\my7zip\CPP\7zip\Bundles\Fm\O\7FM.exe
	
	
	
## ///////////////// change ///////////////////////////

edited

	CPP\7zip\UI\FileManager\FM.cpp, Cpp\Build.mak, CPP\7zip\UI\FileManager\FM.mak
	
	CPP\7zip\UI\FileManager\StdAfx.h
		#define _WIN32_WINNT 0x0600
		
	CPP\7zip\UI\FileManager\Panel.cpp
		CMyListView::OnMessage
		
	Panel.h
		add header files
		
Added
	IatHook.h, DarkMode.h，IatHook.cpp, Darkmode.cpp


## Talks

I found a dark theme for 7z before. However, it's based on win7's flatty theme, not going to work on win10's dark theme.
https://www.deviantart.com/alexgal23/art/Flaty-7-Zip-theme-438374489

So I decided to do it on my own.

I'll see how to modify tool bars.

