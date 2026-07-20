# Schwarzpause Icon Source Assets

This folder contains source and generated artwork for the Schwarzpause icon
refresh.

The original user-provided SVG files in the project root were valid SVG/XML,
but they were large traced paths with roughly 36,000 to 38,000 path commands
per icon. That is too complex for reliable Icon-O-Matic/HVIF use.

The packaged `svg/` files are clean simplified SVG sources generated from the
approved black/white Schwarzpause style. They have transparent backgrounds and
no Inkscape page background metadata.

The packaged `hvif/` files are compact native Haiku vector icons generated from
the same geometry. They remain as lightweight fallback resources.

The current preferred desktop icon source is `png/`. These PNGs come from the
user-provided `PNGs/` folder and preserve the richer artwork better than the
simplified generated HVIF attempt. Tracker imports these PNGs as resources,
decodes them with the Translation Kit, and scales them for Home, Disks, Trash,
and generic file fallback icons.

Current mapping:

- `HOME`: Tracker Home icon
- `DISKS`: Tracker boot volume and volume MIME icon
- `TRASH`: Tracker Trash and full Trash icons
- `MAIL`: Mail application icon
- `FILE`: Tracker generic file icon and `text/plain` MIME icon

Build location inside Schwarzpause OS:

`/boot/system/data/artwork/schwarzpause/icons/`

Regenerate the clean SVG/HVIF files and resource blocks with:

```sh
powershell -NoProfile -ExecutionPolicy Bypass -File tools/generate-schwarzpause-hvif-icons.ps1
```
