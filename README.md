# Bethesda Archive Extractor
Bethesda Archive Extractor (BAE) can extract BSA and BA2 files.  Supports all Bethesda games from Oblivion through Skyrim SE.

## Usage
You can open multiple BSAs via File > Open, or by dragging them directly to the window or EXE.

## Building
### Windows
```
TODO
```
### Linux
```
git clone --recursive {repo url}
cd bae
qmake
make
```
## Version History
0.01
* Initial Commit

0.02
* Significantly faster open times on all BA2s, especially Fallout4 - MeshesExtra.ba2 which took several minutes (now takes ~1s).

0.03 
* Width/Height were actually listed as Height/Width in the BA2 so non-square textures were incorrectly extracted.
* Gave window a minimum width/height as Windows 10 users complained of the window being too small.
* Can open multiple archives via the file dialog, or by dragging to the executable.
     
0.04
* Cube maps were being extracted incorrectly.

0.05
* Skyrim SE BSA support

0.06
* Added drag and drop to main window
* Added application icon
* Added About window

0.07
* Corrected the size sent to LZ4 for decompression, which affected only a very small number of files. Please see the sticky on the Nexus page for information.
* Added a file filter which lets you drop a text file with a list of full or partial filenames (using wildcards). This was added mostly to simplify re-extracting the files affected by the bug above.

0.08
* 3-4x faster extraction
* Added a filter to the UI. Searches either filename or entire path, supports wildcards (*)

0.09
* Drag and Drop file extraction. You can drag one or multiple files from the archive to any location on your computer, including to programs that can open the filetypes you are dragging. Ctrl- or Shift- click rows to drag multiple files. The checkboxes do not matter and are used for normal Extraction. **NOTE**: Dragging folders currently not supported.

0.10
* Fixed Select All/None which I inadvertently broke last release. Also improved the implementation to be faster.
* Fixed read/write of extended ASCII filepaths. Only 3 vanilla FO4 filenames were affected to my knowledge (María_F.fuz, María_M.fuz, Sánchez_F.fuz)
