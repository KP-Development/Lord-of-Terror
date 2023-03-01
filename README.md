<p align="center">
<img width="554" src="https://raw.githubusercontent.com/KP-Development/Lord-of-Terror/209f099a89cfac0ca7979e9cbd0557b03c8e2f3c/lordofterrorlogo.png">
</p>

---

[![Discord Channel](https://img.shields.io/discord/1074510941112238180?color=%237289DA&logo=discord&logoColor=%23FFFFFF)](https://discord.gg/9rykuV4x9K)
[![Downloads](https://img.shields.io/github/downloads/kp-development/lord-of-terror/total.svg)](https://github.com/kp-development/lord-of-terror/releases/latest)
[![Codecov](https://codecov.io/gh/kp-development/lord-of-terror/branch/master/graph/badge.svg)](https://codecov.io/gh/kp-development/lord-of-terror)

<p align="center">
<img width="838" src="https://user-images.githubusercontent.com/204594/113578478-26912400-9623-11eb-9ff6-9bd9717462b6.png">
</p>

<sub>*(The health-bar and XP-bar are off by default, but can be enabled in the [game settings](https://github.com/diasurgical/devilutionX/wiki/DevilutionX-diablo.ini-configuration-guide). Widescreen can also be disabled if preferred)*</sub>

# What is Lord of Terror

Lord of Terror is a fork of DevilutionX, that intends to address parts of the game that are beyond the scope of DevilutionX, including changes that may be too controversial or change core game balance and mechanics. DevilutionX is a port of Diablo and Hellfire that strives to make it simple to run the game while providing engine improvements, bugfixes, and some optional quality of life features.

For more information about DevilutionX, check out the https://github.com/diasurgical/devilutionX

# How to Install

Note: You'll need access to the data from the original game. If you don't have an original CD then you can [buy Diablo from GoG.com](https://www.gog.com/game/diablo). Alternately you can use `spawn.mpq` from the [shareware](https://github.com/diasurgical/devilutionx-assets/releases/download/v2/spawn.mpq) [[2]](http://ftp.blizzard.com/pub/demos/diablosw.exe) version, in place of `DIABDAT.MPQ`, to play the shareware portion of the game.

Download the latest [Lord of Terror release](https://github.com/KP-Development/Lord-of-Terror/releases/latest) and extract the contents to a location of your choosing or [build from source](#building-from-source).

- Copy `DIABDAT.MPQ` from the CD or GOG-installation (or [extract it from the GoG installer](https://github.com/diasurgical/devilutionX/wiki/Extracting-the-.MPQs-from-the-GoG-installer)) to the Lord of Terror folder.
- To run the Diablo: Hellfire expansion you will need to also copy `hellfire.mpq`, `hfmonk.mpq`, `hfmusic.mpq`, `hfvoice.mpq`.

For more detailed instructions: [Installation Instructions](./docs/installing.md).

# Test builds

If you want to help test the latest development stage of the next version (make sure to backup your files as these may contain bugs), you can fetch the test build artifact from one of the build server:

*Note: You must be logged into GitHub to download the attachments!*

[![Linux x86](https://github.com/KP-Development/Lord-of-Terror/actions/workflows/Linux_x86.yml/badge.svg)](https://github.com/KP-Development/Lord-of-Terror/actions/workflows/Linux_x86.yml?query=branch%3Amaster)
[![Linux x86-64 SDL1](https://github.com/KP-Development/Lord-of-Terror/actions/workflows/Linux_x86_64_SDL1.yml/badge.svg)](https://github.com/KP-Development/Lord-of-Terror/actions/workflows/Linux_x86_64_SDL1.yml?query=branch%3Amaster)
[![MacOSX](https://github.com/KP-Development/Lord-of-Terror/actions/workflows/MacOSX.yml/badge.svg)](https://github.com/KP-Development/Lord-of-Terror/actions/workflows/MacOSX.yml?query=branch%3Amaster)
[![Windows x64](https://github.com/KP-Development/Lord-of-Terror/actions/workflows/Windows_MSVC_x64.yml/badge.svg)](https://github.com/KP-Development/Lord-of-Terror/actions/workflows/Windows_MSVC_x64.yml?query=branch%3Amaster)
[![Windows MinGW x64](https://github.com/KP-Development/Lord-of-Terror/actions/workflows/Windows_MinGW_x64.yml/badge.svg)](https://github.com/KP-Development/Lord-of-Terror/actions/workflows/Windows_MinGW_x64.yml?query=branch%3Amaster)
[![Windows MinGW x86](https://github.com/KP-Development/Lord-of-Terror/actions/workflows/Windows_MinGW_x86.yml/badge.svg)](https://github.com/KP-Development/Lord-of-Terror/actions/workflows/Windows_MinGW_x86.yml?query=branch%3Amaster)
[![Android](https://github.com/KP-Development/Lord-of-Terror/actions/workflows/Android.yml/badge.svg)](https://github.com/KP-Development/Lord-of-Terror/actions/workflows/Android.yml?query=branch%3Amaster)
[![iOS](https://github.com/KP-Development/Lord-of-Terror/actions/workflows/iOS.yml/badge.svg)](https://github.com/KP-Development/Lord-of-Terror/actions/workflows/iOS.yml?query=branch%3Amaster)
[![PS4](https://github.com/KP-Development/Lord-of-Terror/actions/workflows/PS4.yml/badge.svg)](https://github.com/KP-Development/Lord-of-Terror/actions/workflows/PS4.yml?query=branch%3Amaster)
[![Original Xbox](https://github.com/KP-Development/Lord-of-Terror/actions/workflows/xbox_nxdk.yml/badge.svg)](https://github.com/KP-Development/Lord-of-Terror/actions/workflows/xbox_nxdk.yml?query=branch%3Amaster)
[![Xbox One/Series](https://github.com/KP-Development/Lord-of-Terror/actions/workflows/xbox_one.yml/badge.svg)](https://github.com/KP-Development/Lord-of-Terror/actions/workflows/xbox_one.yml?query=branch%3Amaster)

Linux x86-64, Switch, Vita, 3DS, Amiga, [![CircleCI](https://circleci.com/gh/KP-Development/Lord-of-Terror.svg?style=shield)](https://app.circleci.com/pipelines/github/KP-Development/Lord-of-Terror?branch=master)

# Building from Source

Want to compile the program by yourself? Great! Simply follow the [build instructions](./docs/building.md).

# Credits

- The original Devilution project [Devilution](https://github.com/diasurgical/devilution#credits)
- [Everyone](https://github.com/diasurgical/devilutionX/graphs/contributors) who worked on Devilution/DevilutionX
- [Nikolay Popov](https://www.instagram.com/nikolaypopovz/) for UI and graphics
- [WiAParker](https://wiaparker.pl/projekty/diablo-hellfire/) for the Polish voice pack
- And thanks to all who support the project, report bugs, and help spread the word ❤️

# Legal

Lord of Terror is made publicly available and released under the Sustainable Use License (see [LICENSE](LICENSE.md))

The source code in this repository is for non-commercial use only. If you use the source code you may not charge others for access to it or any derivative work thereof.

Diablo® - Copyright © 1996 Blizzard Entertainment, Inc. All rights reserved. Diablo and Blizzard Entertainment are trademarks or registered trademarks of Blizzard Entertainment, Inc. in the U.S. and/or other countries.

Lord of Terror and any of its maintainers are in no way associated with or endorsed by Blizzard Entertainment®.
