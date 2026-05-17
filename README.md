# Turnips
Turnip prices previewer for Animal Crossing: New Horizons.

DISLAIMER: This reads data from your save. I am not responsible for any data loss, consider backing up before use.

<p align="center"><img src="https://i.imgur.com/MZjTKoj.jpg" </p>
<p align="center"><img src="https://i.imgur.com/J1Ef38k.jpg" </p>

# Compiling (Windows and Linux):

Important:
Building needs a working devkitA64 environment, with packages `libnx`, `deko3d` and `switch-glm` installed.

Windows:

1. Download devkitA64/devkitPro (https://github.com/devkitPro/installer). Without overcomplicating it, just install everything included and click through the installer.
2. Start msys2 via the start menu search. Alternatively you can start the .bat file (`C:\devkitPro\msys2\msys2_shell.bat`).
3. Install required libraries: run `pacman -S libnx deko3d switch-glm`, then type "Y" to confirm download/installation.
4. Still in msys2, run `git clone --recursive https://github.com/averne/Turnips.git`, then type `cd Turnips` to switch to the directory the project has been cloned into.
6. Type `make` to compile.
7. The executable should be found in `C:\Users\username\Turnips\out`.

Linux:

1. `sudo (dkp-)pacman -S switch-dev libnx deko3d switch-glm`
2. `git clone --recursive https://github.com/averne/Turnips.git`
3. `cd Turnips`
4. `make -j$(nproc)`
5. Output will be located in out/.

# Credits
- The [NHSE](https://github.com/kwsch/NHSE) project for save decrypting/parsing.
- The [FTPD](https://github.com/mtheall/ftpd) project for the deko3d imgui backend.
- [u/Kyek](https://reddit.com/u/Kyek) for the background and logo assets.
