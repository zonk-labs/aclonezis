# If you like piña coladas...

## Build

You're gonna need a C encabulator, glibc, and make. Only tested on GNU/linux.

```sh
make
```

## How do I use this?

```sh
./aclonezis diff my_huge_old_backup.img my_newer_image.img small_diff_output.img.diff
```

And to put everything back together

```
./aclonezis join my_huge_old_backup.img small_diff_output.img.diff my_newer_image_restored.img
```

## Is this useful?

Probably only to myself. But hey, it even does crc32!