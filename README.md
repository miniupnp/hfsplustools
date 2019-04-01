hfsplustools
============

Tools to manipulate (Apple) HFS+ file systems, such as .toast files or .dmg

## hfsdisplay
Shows the partition map, the HFS+ volume header and Catalog header and root nodes.

## hfspatchnoautofolder
Shows, disables or sets the "autofolder", ie the Folder that is open automatically by the finder.
```
usage: hfspatchnoautofolder <command> <file.iso>
commands :
  show : only display informations
  disable : disable autofolder
  setroot : set autofolder to root
  setfolderid <id> : set autofolder to id * USE WITH CARE *
```

## hfsrename
Work in progress. Should rename the HFS+ volume.

## hfsunhide
remove Invisible HFS+ flag from file/folder.
```
Usage: hfsunhide [--doit] [-v] <image.iso> path/.../file.txt
```
