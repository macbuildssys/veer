<p align="center">
  <img src="res/icons/256/veer.png" alt="Veer icon" width="200">
</p>

# Veer

Veer is a small Debian link swapper written in C++ with Qt6. When you click on a link in any application, Veer displays the browsers installed on your computer and opens the link in your chosen browser, rather than using a fixed default every time.

## How it works

* Veer reads the `.desktop` files in your system, user, Flatpak and Snap application folders.

* An entry counts as a browser when it handles `http` or `https` links and is marked as a web browser (or handles HTML files).

* Veer registers itself as the default web browser. Every clicked link goes to it, and it launches the browser you pick with the link filled in.

## Install the Debian package

Download `veer_0.0.1_amd64.deb`, open a terminal in the folder that holds it, and run:

sudo apt install ./veer_0.0.1_amd64.deb

veer --set-default

To check that it worked:

```
veer --status
```

The install puts the program in `/usr/local/bin`, the launcher entry in `/usr/local/share/applications` and the icon in `/usr/local/share/icons`. If the icon does not show up in your taskbar or menu right away, log out and back in. If `veer --set-default` fails, choose Veer under System Settings, Default Applications, Web browser.

`wl-clipboard` (Wayland) and `xclip` (X11) are only needed for the highlighted link feature below.

## Using it

* Click a link anywhere. The swapper opens.

* Press `1` to `9` to choose a browser by number, use the arrow keys with Enter, or click one.

* Tick "Always use this browser for example.com" to skip the swapper for that site next time. The `www.` prefix is ignored.

* The last browser you used is preselected.

* Escape or Cancel opens nothing.

* If only one browser is installed, Veer opens it directly.

Command line options:

```
veer --list           print the detected browsers

veer --set-default    make Veer the default web browser

veer --status         show which program currently opens web links

veer --clear-rules    forget all remembered sites

veer --selection      open the link in the highlighted or copied text

veer --setup          open the setup window

veer                  with no link, open the setup window
```

Icons come from the system icon folders, Flatpak and Snap exports. SVG icons need the `libqt6svg6` package, which the install line above includes.

Uninstall it with:

```
sudo apt remove veer
```

This removes the program, the launcher entry and the icons. Your remembered sites stay in `~/.config/veer`, so delete that folder as well if you want a clean removal. After removing Veer, choose a browser again under System Settings, Default Applications, Web browser, otherwise links have no handler.

Distributed under the MIT License. See [LICENSE](LICENSE).