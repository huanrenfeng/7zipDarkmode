# 7zipDarkmode

inspired by https://github.com/ysc3839/win32-darkmode/tree/master/win32-darkmode

## How To Compile 

Read this first.
	http://www.ski-epic.com/2012_compiling_7zip_on_windows_with_visual_studio_10/index.html

	turns out it uses nmake to compile
	
open "Developer Command Prompt for VS 2017"

cd C:\my7zip\CPP\7zip\
nmake NEW_COMPILER=1 MY_STATIC_LINK=1


it takes almost 5 minutes to compile

compiled file is in 
	C:\my7zip\CPP\7zip\Bundles\Fm\O\7FM.exe
	
	for me, it's in C:\Users\huanr\Desktop\winProject\7z1900-src\CPP\7zip\Bundles\Fm\o
	
	
## ///////////////// change ///////////////////////////
	
I added FM.cpp in C:\Users\huanr\Desktop\winProject\7z1900-src\CPP\7zip\UI\FileManager
	to make it support dark mode
	
edited

	CPP\7zip\UI\FileManager\FM.cpp, Cpp\Build.mak
	
	CPP\7zip\UI\FileManager\StdAfx.h
		#define _WIN32_WINNT 0x0600
		
	CPP\7zip\UI\FileManager\Panel.cpp
		CMyListView::OnMessage
		
Added
	IatHook.h, DarkMode.h
		
got some problem when compiling, changing of StdAfx.h in CPP\7zip\UI\FileManager doesn't work

check this
	https://docs.microsoft.com/en-us/cpp/build/reference/yu-use-precompiled-header-file?view=vs-2019
	
	cl -Yu means use precompiled-header-file
	
	
so,I deleted every -Yu"StdAfx.h" -Fp$O/a.pch in CPP\Build.mak

