<h1 align="center">
  <br>
  <a href="https://github.com/dhitchenor/rper">rper</a>
  <a href="https://opensource.org/license/MIT"><img alt="License: MIT" src="https://img.shields.io/badge/License-MIT-green.svg"></a>
</h1>

<h4 align="center">A quick and easy POSIX-compliant utility for unix based operating systems, to recursively change permissions of directories, and/or files</h4>

<p align="center">
  <a href="#installation">Installation</a> •
  <a href="#updating">Updating</a> •
  <a href="#features">Features</a> •
  <a href="#contributing">Contributing</a>
</p>

---

![rper usage](https://raw.githubusercontent.com/dhitchenor/rper/main/.github/images/rper_usage.gif)

## Installation

##### Prerequisites:
* Ensure you have **gcc** installed on your unix-based system (or your favourite C compiler).

##### Building or Downloading:
1. **[Download](https://github.com/dhitchenor/rper/archive/main.zip)** or clone the repository with `git clone https://github.com/dhitchenor/rper`
2. Unzip, and/or change into the appropriate directory
3. build using the command: `gcc -o rper rper.c`
   * You should now have a built utility (binary) in the current folder called **rper**

##### Incorporating into your system:
1.  **Move** the utility into your operating systems bin directory, eg. /usr/local/bin/
   * example command (assuming you are currently in the same directory as the built utility): `sudo mv rper /usr/local/bin/`

> [!IMPORTANT]
> If you don't wish to incorporate the binary into your system,
> you will need to be in the directory that contains the utility to use it, and
> you will need to run the utility similiarly to the following command:
>
> `./rper -p 644 /some/other/directory`

## Updating

Updating is just a matter of replacing the built utility/binary with an updated binary

> [!TIP]
> Use `whereis rper` to find where any previous versions of rper are installed,
> then overwrite the old binary using the instructions above as a guide

## Features

files (-f):
- makes the desired permissions changes to files only
- retained as the default flag if the directories flag (-d) is not used

directories (-d):
- makes the desired permissions changes to directories only
- can be used with the files flag (-f), if you wish to change directories and files to identical permissions (might not be what you want, use caution)

inclusive (-i):
- includes the specified directory (directory argument) when making permission changes
- can only be used with the directories flag (-d), has no effect when used with the files flag (-f)

non-recursive (-n):
- only makes changes to the files/directories within the specified directory (directory argument).. essentially the basic use of the chmod command

quiet (-s):
- suppresses normal output, but still displays any error output
- retained as default, if both suppressing flags are given

silenced (-S):
- suppresses all output, including errors
- defaults to -s flag, if both supressing flags are given

verbose (-v):
- displays all output; both directory and file changes are provided in output, regardless of flag given
- both suppressing flags are ignored if given

permissions (-p):
- uses an octal formatted argument (eg. 755 or 0644) as the desired changed permissions.
- allows the use of (*) as a wildcard, eg 6*4 will change the user (left-most), and others (right-most) permissions, but not the group(center) permission

help (-h | -H):
- displays help for the user

about (-a):
- displays information about rper

## Contributing

> [!TIP]
> Start an issue or file a PR; make sure any code changes are well commented.
