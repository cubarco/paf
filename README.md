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

Example
---
```sh
tar cf - . | paf md5sum {/tmp/foo.tar}
```

Todo
---
Because of the limitation of FIFO, the filename passed to COMMAND can be opened
only once. I'm working on it to support multiple opening.

License
---
GPL V2
