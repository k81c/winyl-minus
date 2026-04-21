You need Visual Studio C++ 2022 (with Windows XP support for C++).

First you need to compile all 3rd-party libraries.



## Packskin.exe temporaly unavailable.
----------
Build zlib (can be skipped, needed only for PackSkin utility):

1. Download zlib https://zlib.net
2. Open contrib/vstudio/vc14/zlibvc.sln
3. Open zlibstat ReleaseWithoutAsm project properties
4. Change Platform Toolset to v141_xp
5. Change C/C++ -> Code Generation -> Runtime Library -> Set /MT
6. Change C/C++ -> General -> Debug Information Format -> Set None
7. Build zlibstat ReleaseWithoutAsm
8. x86: Copy zlibstat.lib to Winyl/src/zlib
9. x64: Copy zlibstat.lib to Winyl/src/zlib/x64
10. Copy zlib.h, zconf.h, ioapi.h, zip.h, unzip.h to Winyl/src/zlib
11. Build PackSkin utility Winyl/PackSkin/PackSkin.sln


----------
BASS:

1. Download from http://www.un4seen.com  
bass24.zip  
bassmix24.zip  
bass_fx24.zip  
basswasapi24.zip  
bassasio14.zip  
bass_aac24.zip  
bass_ape24.zip  
bass_mpc24.zip  
bass_spx24.zip  
bass_tta24.zip  
bassalac24.zip  
bassflac24.zip  
bassopus24.zip  
basswm24.zip  
basswv24.zip  
2. Unzip .h files to Winyl/src/bass
4. x64: Unzip x64 .lib files to Winyl/src/bass/x64

----------
That is all if you only need to compile the release version, build it with Winyl.sln.

----------
To run under the debugger or create packages you need Winyl/data folder.

The data folder structure:

File.ico  
License.txt  
Portable.dat  
Equalizer/Presets.xml  
Language/*  
Skin/*  
x64/bass.dll  
x64/bass_fx.dll  
x64/bassasio.dll  
x64/bassmix.dll  
x64/basswasapi.dll  
x64/Bass/bass_aac.dll  
x64/Bass/bass_ape.dll  
x64/Bass/bass_mpc.dll  
x64/Bass/bass_spx.dll  
x64/Bass/bass_tta.dll  
x64/Bass/bassalac.dll  
x64/Bass/bassflac.dll  
x64/Bass/bassopus.dll  
x64/Bass/basswma.dll  
x64/Bass/basswv.dll  


----------
Creating packages:

1. Copy the data folder somewhere
2. Remove Profile subfolder
3. Copy WinylMinus.exe to the data folder
4. Move dlls from x86/x64 to the data folder and delete x86 and x64 folders
5. Copy PackSkin.exe to data\Skin folder
6. Pack all skins with PackSkin utility and delete unpacked skins
7. Rename the data folder to 'WinylMinus'
8. Portable version: zip the folder
9. Setup version: run Inno Setup script
10. Repeat all for x64 version