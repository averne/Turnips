# Turnips
Turnip prices previewer for Animal Crossing: New Horizons.

DISLAIMER: This reads data from your save. I am not responsible for any data loss, consider backing up before use.

<p align="center"><img src="https://i.imgur.com/MZjTKoj.jpg" </p>
<p align="center"><img src="https://i.imgur.com/J1Ef38k.jpg" </p>

# Compiling (Windows and Linux):

Important: 
Building needs a working devkitA64 environment, with packages libnx, deko3d and switch-glm installed.
Windows:

(Type all commands without quotes.)

---

Windows:

1.	Download DevkitA64 / DevkitPro ( https://github.com/devkitPro/installer )
2.	Without overcomplicating it just install everything included and click through the installer.
3.	Start Msys2 (Just a black command Window) found via Windows start menu search. Alternatively you can start the .bat file (C:\devkitPro\msys2\msys2_shell.bat)
4.	Type "pacman -S switch-glm" and then press enter.
5.	Type "Y" to confirm installation/download and press enter.
--- Compile Turnips project into .nro ---
6. Type in (still in Msys2) "git clone --recursive https://github.com/averne/Turnips.git"
7. Then type in "cd Turnips" so it switches to the directory the project has been downloaded in.
8. Type "make".
9. If everything is working corectly now the project should be compiled and be in "C:\users\username\Turnips\out".
Done

---

Linux:

In Terminal type:
1.	sudo (dkp-)pacman -S switch-dev
2.	$ git clone --recursive https://github.com/averne/Turnips.git
3.	$ cd Turnips
4.	$ make -j$(nproc)
5.	Output will be located in out/.

---

# Credits
- The [NHSE](https://github.com/kwsch/NHSE) project for save decrypting/parsing.
- The [FTPD](https://github.com/mtheall/ftpd) project for the deko3d imgui backend.
- [u/Kyek](https://reddit.com/u/Kyek) for the background and logo assets.
