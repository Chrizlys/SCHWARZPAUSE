SCHWARZPAUSE
=======================
**The OS for engineers.**

SCHWARZPAUSE is a free, open-source desktop operating system — a derivative of the
[Haiku](https://www.haiku-os.org) operating system, made for engineers, makers, and
anyone who wants a computer that stays fast and private.

It runs straight from a USB stick, without touching the system already on your machine.

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

Building
--------

SCHWARZPAUSE builds like Haiku, from Linux (or WSL on Windows). Clone this repository
and Haiku's `buildtools` next to each other:

    git clone https://github.com/Chrizlys/SCHWARZPAUSE.git
    git clone https://review.haiku-os.org/buildtools.git

Activate the SCHWARZPAUSE build configuration, then configure and build:

    cd SCHWARZPAUSE
    cp build/jam/UserBuildConfig.schwarzpause build/jam/UserBuildConfig
    ./configure --build-cross-tools x86_64 ../buildtools
    mkdir -p generated.x86_64 && cd generated.x86_64
    jam -q -j$(nproc) @nightly-anyboot

The result is a bootable anyboot image. Write it to a USB stick in raw/DD mode and boot
from it. For general Haiku build requirements see `ReadMe.Compiling.md`.

Note for Windows users: build from a Linux filesystem, not directly from a Windows path,
and keep line endings as LF — CRLF breaks the build.

License and attribution
-----------------------

SCHWARZPAUSE is a derivative of **Haiku**, which is distributed under the **MIT license**.
The license text is in `License.md`, and Haiku's original project README is preserved as
`ReadMe.Haiku.md`. Copyright notices in the source belong to their respective authors.

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
