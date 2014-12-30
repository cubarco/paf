Pipe as file
===
Paf transmits data from `stdin` to a `FIFO` named by user.
As some programs do not regard `-` as `stdin`, or sometimes the filename is
significant, paf is developed to deal with this situation.

Usage
---
```
Usage: paf COMMAND [OPTION...] PATTERN... [OPTION...]
PATTERN:
  {}:                     PATTERN will be replaced with /tmp/default
  {/path/to/file}:        PATTERN will be replaced with /path/to/file
  {!/path/to/file}:       Force mode

Note: If there are more than one PATTERNs in a single command line, the 
      PATTERNs should be the same.
```

Examples
---

##### MD5SUM with filenames
```console
$ tar cf - . | paf md5sum {/tmp/foo.tar}
```
##### Pacman installing from folders
If you are using pacman as a package manager, paf enables you to install software 
from a folder.
<br \>
Assuming that `${pkgdir}` is the folder generated with `makepkg --noarchive`. Run
following commands and the package will be installed without archiving.
```console
$ cd ${pkgdir}
# tar -cf - {*,.*} | paf pacman -U {/tmp/foo.tar}
```

Todo
---
- [ ] Due to the limitation of `pipe`, the filename passed to `COMMAND` can be
opened only once. I'm working on it to support multiple opening. (Implemented but
not a good practice)

License
---
GPLv2
