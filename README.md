# webMDX

Copyright (C) 2018 Juergen Wothke

This is a JavaScript/WebAudio plugin of "mdxmini/pmdmini" used to emulate music from .m and .mdx files.

Here some background info from Misty De Meo's respective projects:

"mdxmini .. can play back MDX chiptunes from the X68000 home computer. The X68000 was, for its time, an 
advanced 16-bit computer with excellent multimedia support; its 8-channel YM2151 sound chip and 1-channel 
PCM chip gave it excellent music support. 

pmdmini .. can play back MDX chiptunes from the PC-98. The PC-98 was a very popular series of Japanese 
home computers. With its Yamaha YM2203 or YM2608 sound chip, for which many games and songs were written; 
the Professional Music Driver (PMD) library was a popular music library created by M. Kajihara. pmdmini 
allows playing back music written using PMD."

 
This plugin is designed to work with my generic WebAudio ScriptProcessor music player (see separate project)
but the API exposed by the lib can be used in any JavaScipt program (it should look familiar to anyone 
that has ever done some sort of music player plugin). 


There is some overlap with my "webS98" player but whereas .s98 is a recorded music-format, "webMDX" 
emulates the original hardware to play respective PC-98 music files.

A live demo of this program can be found here: http://www.wothke.ch/webMDX/


## Credits

milk.K, K.MAEKAWA, Missy.M - authors of the original X68000 MXDRV
Daisuke Nagano - author of the Unix mdxplay
KAJIHARA Mashahiro - original author of the PMD sound driver for PC-9801
AGAWA Koji - Maintainer of PMDXMMS, on which pmdmini was based
M88 / cisc - author of OPNA FM sound generator 
BouKiCHi - author of the mdxmini&pmdmini library
Misty De Meo - bugfixes and improvements to mdxmini&pmdmini

The project is based on: https://github.com/mistydemeo/mdxmini & https://github.com/mistydemeo/pmdmini


## Project

All the "Web" specific additions (i.e. the whole point of this project) are contained in the 
"emscripten" subfolder. 


## Howto build

You'll need Emscripten (http://kripken.github.io/emscripten-site/docs/getting_started/downloads.html). The make script 
is designed for use of emscripten version 1.37.29 (unless you want to create WebAssembly output, older versions might 
also still work).

The below instructions assume that the webMDX project folder has been moved into the main emscripten 
installation folder (maybe not necessary) and that a command prompt has been opened within the 
project's "emscripten" sub-folder, and that the Emscripten environment vars have been previously 
set (run emsdk_env.bat).

The Web version is then built using the makeEmscripten.bat that can be found in this folder. The 
script will compile directly into the "emscripten/htdocs" example web folder, were it will create 
the backend_mdx.js library. (To create a clean-build you have to delete any previously built libs in the 
'built' sub-folder!) The content of the "htdocs" can be tested by first copying it into some 
document folder of a web server. 


## Depencencies

The current version requires version 1.03 (older versions will not
support WebAssembly or may have problems skipping playlist entries) 
of my https://github.com/wothke/webaudio-player.

This project comes without any music files, so you'll also have to get your own and place them
in the htdocs/music folder (you can configure them in the 'songs' list in index.html).


## License

The code builds on FM Sound Generator with OPN/OPM interface Copyright (C) by cisc.
As the author of FM Sound Generator states (see readme.txt in "fmgen" sub-folder), this is free software but 
for any commercial application the prior consent of the author is explicitly required. So it probably 
cannot be GPL.. and the GPL claims made in the pdmmini project are probably just invalid. 

Be that as it may.. I'll extend whatever license is actually valid for the underlying code to the 
code that I added here to turn "mdxmini/pmdmini" into a JavaScript lib.
