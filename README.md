Pipe as file
===
Paf transmits data from `stdin` to a `FIFO` named by user.
As some programs do not regard `-` as `stdin`, or sometimes the filename is
significant, paf is developed to deal with this situation.

Usage
---
```
paf COMMAND [OPTION...] PATTERN [OPTION...]
PATTERN:
  {}:                     PATTERN will be replaced with /tmp/default
  {/path/to/file}:        PATTERN will be replaced with /path/to/file
  {!/path/to/file}:       Force mode
```

Examples
---

* ###MD5SUM with filenames
```console
tar cf - . | paf md5sum {/tmp/foo.tar}
```
* ###Pacman install from folders
If you are using pacman as a package manager, paf enables you install a software
from a directory.
<br \>
Assuming that `${pkgdir}` is the folder generated with `makepkg --noarchive`. Run
following commands and the package will be installed without archiving.
```console
$ cd ${pkgdir}
# tar -cf - {*,.*} | paf pacman -U {/tmp/foo.tar}
```

Todo
---
Due to the limitation of `pipe`, the filename passed to `COMMAND` can be opened
only once. I'm working on it to support multiple opening.

License
---
GPLv2
