SCHWARZPAUSE
=======================
**The OS for engineers.**

SCHWARZPAUSE is a free, open-source desktop operating system — a derivative of the
[Haiku](https://www.haiku-os.org) operating system, made for engineers, makers, and
anyone who wants a computer that stays fast and private.

It runs straight from a USB stick, without touching the system already on your machine.

The name **SCHWARZPAUSE** is the German term for a *blackline print* — a technical drawing reproduced
with black or very dark lines on a light background. It may also be classified more broadly as a
*whiteprint*.

> **SCHWARZPAUSE is based on Haiku, but it is not the official distribution from the
> Haiku Project.** For information about the official Haiku project, please visit
> <https://haiku-os.org>.
>
> **Chrizlys is not associated with the Haiku project.** To obtain support for
> SCHWARZPAUSE, please contact
> <https://github.com/Chrizlys/SCHWARZPAUSE/issues>.
>
> This software is work in progress and has missing functionality as well as many
> (known and unknown) bugs. Use SCHWARZPAUSE at your own risk.

Applications
------------

SCHWARZPAUSE ships with two applications of its own:

* **SCHWARZBrOT** — a fully offline AI assistant, running locally on your machine
  (llama.cpp with a Qwen3.5-4B model). No account, no cloud, no telemetry: nothing you
  type ever leaves the computer. The AI model is a separate, optional download.

* **SCHWARZSEHER** — a visual front end for [OpenSCAD](https://openscad.org). You sketch
  and type dimensions, it generates clean `.scad` code, and OpenSCAD renders it. OpenSCAD
  stays the source of truth and is installed separately.

Status
------

Work in progress. The system boots and runs from USB, and both applications work.
Interfaces and defaults are still changing.

Updates
-------

**Applications update from HaikuPorts.** SCHWARZPAUSE ships Haiku's HaikuPorts repository,
so applications you install from HaikuDepot — browsers, LibreOffice, OpenSCAD and the rest —
receive updates straight from the Haiku project.

**The operating system core is frozen.** SCHWARZPAUSE deliberately does not ship the Haiku
core repository. Part of the SCHWARZPAUSE identity is compiled into the core system, so
letting a package update replace the `haiku` package would swap it for stock Haiku — the
desktop, boot screen and About window would all revert. Freezing the core keeps the identity
intact, but means you do not receive core updates automatically; to move to a newer core,
install a newer SCHWARZPAUSE release.

**A caveat worth knowing.** HaikuPorts tracks the latest Haiku, which moves quickly, so in
time some newer application versions will require a newer core than a given release provides.
When that happens `pkgman` simply declines that one install or update — it does not break,
downgrade or change your system in any way — and the application becomes available again with
the next SCHWARZPAUSE release, which rebases onto a newer Haiku core. Because Haiku is under
active development, the newest packages may reach this point fairly soon.

If you would rather follow Haiku's own updates and accept losing the SCHWARZPAUSE
appearance, you can add the core repository back yourself at any time:

    pkgman add-repo https://eu.hpkg.haiku-os.org/haiku/master/x86_64/current
    pkgman update

This is a one-person project. If it ever stops being maintained, moving to
[Haiku](https://www.haiku-os.org) is the sensible choice — SCHWARZPAUSE is a derivative of
it, so you lose nothing but the styling.

Building
--------

SCHWARZPAUSE is built on Linux. **On Windows the recommended way is WSL** (Windows
Subsystem for Linux) — you do not need a second PC, a virtual machine, or a Linux
installation. The steps below work from a completely fresh Windows machine.

### Step 1 — Install WSL (Windows only)

Open **PowerShell as Administrator** and run:

    wsl --install

This installs WSL2 together with Ubuntu. **Restart the computer when prompted.**

After the restart, start **Ubuntu** from the Start menu. On first launch it asks you to
create a Linux user name and password — pick anything you like and remember the password,
you will need it for `sudo`. You now have a Linux terminal; every command from here on is
typed in that Ubuntu window.

(If `wsl --install` is not recognised, your Windows is too old — update Windows, or see
Microsoft's WSL installation guide.)

### Step 2 — Install the build dependencies

In the Ubuntu window:

    sudo apt update
    sudo apt install -y autoconf automake bc bison build-essential flex gawk git \
        libtool libzstd-dev dos2unix mtools nasm pkg-config python3 rsync texinfo \
        unzip wget xorriso zip zlib1g-dev

### Step 3 — Get the sources

**Work inside the Linux home directory (`~`), not under `/mnt/c/...`.** Building from a
Windows path is many times slower, and Windows line endings (CRLF) will break the build.

    cd ~
    git clone https://github.com/Chrizlys/SCHWARZPAUSE.git
    git clone https://review.haiku-os.org/buildtools.git

### Step 4 — Build

    cd ~/SCHWARZPAUSE
    cp build/jam/UserBuildConfig.schwarzpause build/jam/UserBuildConfig
    mkdir -p generated.x86_64 && cd generated.x86_64
    ../configure --cross-tools-source ../../buildtools --build-cross-tools x86_64
    jam -q -j$(nproc) @nightly-anyboot

The first build also compiles a complete cross-compiler, so it takes a while; later builds
are much faster. The finished image is:

    ~/SCHWARZPAUSE/generated.x86_64/haiku-nightly-anyboot.iso

This builds the **base** system: the OS, the SCHWARZPAUSE identity, and both of its own
applications (SCHWARZBrOT and SCHWARZSEHER). The bundled third-party applications
(LibreOffice, GIMP, and the rest) are **not** part of this repository — they are ordinary
HaikuDepot packages, roughly a gigabyte of binaries, that are assembled separately into the
released image. If you just want the complete system, download the ready-made `.iso` from
the releases page rather than building it.

Copy it to Windows with, for example:

    cp haiku-nightly-anyboot.iso /mnt/c/Users/<YourName>/Desktop/

**If the build fails**, run `jam` again with `-j1` instead of `-j$(nproc)`. A parallel
build interleaves the output and hides which command actually failed; a single-job build
shows the real error.

### Step 5 — Write it to a USB stick

Write the `.iso` to a USB stick in **raw / DD mode** (on Windows, [Rufus](https://rufus.ie)
in "DD Image" mode). Then boot from the stick — in the boot menu choose the entry that
lists the USB stick as a **disk**, not the CD/DVD entry.

License and attribution
-----------------------

SCHWARZPAUSE is a derivative of **Haiku**, which is distributed under the **MIT license**.
The license text is in `License.md`, and the copyright notices in the source belong to
their respective authors. The Haiku project itself lives at
[haiku-os.org](https://www.haiku-os.org) — please direct contributions, bug reports and
support questions about Haiku there, not here.

> Haiku® and the HAIKU logo® are registered trademarks of Haiku, Inc.
> SCHWARZPAUSE is **not affiliated with, endorsed by, or sponsored by Haiku, Inc.**

Third-party components:

* [Haiku](https://www.haiku-os.org) — MIT license
* [llama.cpp](https://github.com/ggml-org/llama.cpp) — MIT license
* Qwen3.5-4B — Apache 2.0, © Alibaba Cloud / Qwen Team; GGUF quantization by bartowski
* [OpenSCAD](https://openscad.org) — GPL. SCHWARZSEHER communicates with OpenSCAD as a
  separate program and does not link against it.

Thanks
------

To the **Haiku** project — the independent, open-source operating system SCHWARZPAUSE is
built on, written by volunteers over more than two decades. Thank you for your great work.
