THIRD-PARTY NOTICES
===================

SCHWARZPAUSE is a derivative of the [Haiku](https://www.haiku-os.org) operating
system and ships together with a set of third-party applications and libraries.
This document lists those components, their licenses, and where to obtain their
source code.

> **Scope:** the bundled applications and the package list in this document are the
> third-party software shipped **inside the downloadable release image** (the `.iso` on
> the releases page and the landing page) — they are **not** part of the SCHWARZPAUSE
> source repository. They are unmodified HaikuPorts binaries assembled into the image
> when the release is built; a build from source alone produces only the base system.
> (The base OS is Haiku, and SCHWARZPAUSE's own code lives in the repository — both are
> covered in the first table below.)

**Nothing in this list is modified by SCHWARZPAUSE.** Every bundled package is an
unmodified binary build taken from [HaikuPorts](https://github.com/haikuports/haikuports),
the Haiku project's own package repository. Combining these unmodified programs
into one image is *mere aggregation*: each component keeps its own license, and
those licenses do not extend to SCHWARZPAUSE's own code.

> The full, verbatim license text of **every** package below is installed inside
> the running system at `/boot/system/data/licenses`. The tables here are an
> index and a source-availability statement; the authoritative
> texts travel with the image itself.

---

Base system and SCHWARZPAUSE's own components
---------------------------------------------

| Component | License | Source |
|---|---|---|
| **Haiku** (the operating system SCHWARZPAUSE is derived from) | MIT | <https://www.haiku-os.org> |
| **SCHWARZPAUSE** own code, incl. **SCHWARZBrOT** and **SCHWARZSEHER** | MIT | <https://github.com/Chrizlys/SCHWARZPAUSE> |
| **llama.cpp** (bundled inside SCHWARZBrOT) | MIT | <https://github.com/ggml-org/llama.cpp> |
| **Qwen3.5-4B** language model — *separate, optional download; not part of the image* | Apache-2.0 | © Alibaba Cloud / Qwen Team; GGUF quantization by bartowski |

> Haiku® and the HAIKU logo® are registered trademarks of Haiku, Inc.
> SCHWARZPAUSE is **not affiliated with, endorsed by, or sponsored by Haiku, Inc.**
> The MIT copyright notices in the Haiku source are preserved unchanged; see
> `License.md`.

---

Bundled applications
--------------------

These are the user-facing applications assembled into the released image. Each is
an unmodified HaikuPorts build of the upstream project named.

| Application | Version | License | Upstream project |
|---|---|---|---|
| **Waterfox** (web browser) | 6.6.16.1 | MPL-2.0 | <https://www.waterfox.net> — source: <https://github.com/BrowserWorks/Waterfox> |
| **Icedove** (e-mail — Thunderbird, rebranded) | 146.0.1 | MPL-2.0 | <https://www.thunderbird.net> |
| **LibreOffice** (office suite) | 24.8.1.1 | MPL-2.0 | <https://www.libreoffice.org> |
| **GIMP** (raster image editor) | 3.2.4 | GPL-3.0-or-later (libraries LGPL-3.0-or-later) | <https://www.gimp.org> |
| **Inkscape** (vector image editor) | 1.4.3 | GPL-2.0-or-later | <https://inkscape.org> |
| **KolourPaint** (simple paint) | 26.04.0 | BSD-2-Clause (parts LGPL-2.0-or-later, GFDL-1.2) | <https://apps.kde.org/kolourpaint/> |
| **VLC** (media player) | 3.0.23 | GPL-2.0-or-later (libVLC: LGPL-2.1-or-later) | <https://www.videolan.org/vlc/> |
| **BePDF** (PDF viewer) | 2.1.4 | GPL-2.0 | <https://github.com/HaikuArchives/BePDF> |
| **PDFWriter** (PDF "printer" driver) | 1.0 | MIT | <https://github.com/HaikuArchives/PDFWriter> |
| **PDFlib** (used by PDFWriter) | 5.0.3 | PDFlib Lite License | <https://www.pdflib.com> |
| **DeskNotes** (desktop sticky notes) | 1.2.1 | BSD-3-Clause | <https://github.com/HaikuArchives/DeskNotes> |
| **OpenSCAD** (programmable 3D CAD; used by SCHWARZSEHER) | 2021.01 | GPL-2.0 | <https://openscad.org> |
| **QCAD** (2D CAD — Community Edition) | 3.26.4.12 | GPL-3.0 | <https://qcad.org> |

---

Libraries, toolkits and codecs
------------------------------

The applications above bring in a large dependency tree — GTK, Qt, the KDE
Frameworks, GStreamer, Poppler, Boost, image and audio/video codecs, and the
supporting C/C++ libraries. All of them are unmodified HaikuPorts packages and
each carries its own upstream license (a mix of, among others, LGPL-2.1+,
GPL-2.0+, MPL-2.0, BSD, MIT and zlib). Their full license texts are installed at
`/boot/system/data/licenses`, and every package is enumerated in the appendix at
the end of this file.

---

Source availability (GPL / LGPL / MPL)
--------------------------------------

Several bundled components are covered by the GNU GPL, GNU LGPL or the Mozilla
Public License, which require that the corresponding source code be made
available. Because SCHWARZPAUSE ships these packages **unmodified**, the complete
and corresponding source for every one of them is available from two public,
permanent locations:

1. **The upstream project** — the "Upstream project" / "Source" links in the
   tables above.
2. **The HaikuPorts recipe** of the same package name and version, which records
   the exact sources and build steps used to produce the binary:
   <https://github.com/haikuports/haikuports>.

SCHWARZPAUSE applies no patches of its own to any of these packages beyond what
the referenced HaikuPorts recipe already contains. If you nonetheless want the
corresponding source for a specific bundled package delivered directly, open an
issue and it will be provided:
<https://github.com/Chrizlys/SCHWARZPAUSE/issues>.

---

Appendix — complete bundled package manifest
--------------------------------------------

The exact set of third-party packages assembled into the released x86_64 image
(`package-version`, as taken unmodified from HaikuPorts):

```text
a52dec-0.8.0-1  ·  adwaita_icon_theme-49.0-1  ·  adwaita_icon_theme_legacy-46.2-1
appstream-1.1.3-1  ·  appstream_glib-0.8.3-1  ·  argon2-20200709-2
aspell-0.60.8.1-1  ·  aspell_en-2020.12.07~0-1  ·  assimp-6.0.5-1
atk-2.38.0-5  ·  atkmm-2.28.2-1  ·  babl-0.1.120-3
bepdf-2.1.4-6  ·  boehm_gc-8.2.12-1  ·  boost1.83-1.83.0-5
boost1.88-1.88.0-4  ·  boost1.90-1.90.0-2  ·  box2d-2.4.1-2
breeze_icons-6.28.0-1  ·  brotli-1.2.0-1  ·  ca_root_certificates-2026_07_16-1
cairo-1.18.4-2  ·  cairomm-1.13.1-2  ·  clucene-2.3.3.4-4
dbus-1.16.2-8  ·  desknotes-1.2.1-1  ·  double_conversion-3.3.1-1
enca-1.19-3  ·  exiv2-0.28.8-1  ·  expat-2.8.2-1
faac-1.30-3  ·  faad2-2.11.2-1  ·  file-5.43-2
file_data-5.43-2  ·  flac-1.5.0-1  ·  flac1.4-1.4.3-1
flite-2.2-2  ·  fluidsynth2-2.1.8-3  ·  fluidsynth3-2.5.4-2
freetype-2.14.3-1  ·  gdk_pixbuf-2.44.7-2  ·  gegl-0.4.68-1
gettext_libintl-1.0-1  ·  gexiv2-0.14.6-3  ·  gimp-3.2.4-1
glew1.13-1.13.0-4  ·  glib2-2.88.1-3  ·  glib_networking-2.80.1-1
glibmm-2.66.2-2  ·  gnutls-3.8.9-1  ·  gobject_introspection-1.86.0-3
gpgme2-2.1.1-1  ·  gpgmepp-2.1.0-1  ·  graphicsmagick-1.3.40-2
gsettings_desktop_schemas-50.1-1  ·  gsl-2.6-2  ·  gst_plugins_bad-1.28.5-1
gst_plugins_base-1.28.5-1  ·  gst_plugins_ugly-1.28.5-1  ·  gstreamer-1.28.5-1
gtk3-3.24.52-1  ·  gtkmm3-3.24.11-1  ·  haiku_svg_icon_theme-5.15.2.38-1
harfbuzz-14.2.0-1  ·  harfbuzz_glib-14.2.0-1  ·  highway-1.4.0-1
hunspell-1.7.2-2  ·  hwloc2-2.10.0-1  ·  hyphen-2.8.8-4
icedove_bin-146.0.1-1  ·  inih-r56-1  ·  inkscape-1.4.3-1
iso_codes-4.20.1-1  ·  ixion0.18-0.19.0-2  ·  json_c-0.15-4
json_glib-1.10.8-1  ·  karchive6-6.28.0-1  ·  kauth6-6.28.0-1
kbookmarks6-6.28.0-1  ·  kcodecs6-6.28.0-1  ·  kcolorscheme6-6.28.0-1
kcompletion6-6.28.0-1  ·  kconfig6-6.28.0-1  ·  kconfigwidgets6-6.28.0-1
kcoreaddons6-6.28.0-1  ·  kcrash6-6.28.0-1  ·  kdbusaddons6-6.28.0-1
kguiaddons6-6.28.0-1  ·  ki18n6-6.28.0-1  ·  kiconthemes6-6.28.0-1
kio6-6.28.0-1  ·  kitemmodels6-6.28.0-1  ·  kitemviews6-6.28.0-1
kjobwidgets6-6.28.0-1  ·  knotifications6-6.28.0-1  ·  kolourpaint_kf6-26.04.0-1
kparts6-6.28.0-1  ·  ksanecore_kf6-26.04.0-1  ·  kservice6-6.28.0-1
ktextwidgets6-6.28.0-1  ·  kunitconversion6-6.28.0-1  ·  kwallet6-6.28.0-1
kwidgetsaddons6-6.28.0-1  ·  kwindowsystem6-6.28.0-1  ·  kxmlgui6-6.28.0-1
lcms-2.19.1-1  ·  lensfun-0.3.2-3  ·  lib2geom-1.4-2
libabw-0.1.3-1  ·  libarchive-3.7.9-1  ·  libassuan3-3.0.2-2
libbluray-1.3.1-2  ·  libcddb-1.3.2-3  ·  libcdr-0.1.8-1
libcmis0.6-0.6.2-2  ·  libcroco-0.6.13-2  ·  libcuefile-475-4
libdca-0.0.7-2  ·  libde265-1.0.8-2  ·  libdeflate-1.18-1
libdvbpsi-1.3.3-1  ·  libdvdcss-1.5.0-2  ·  libdvdnav-6.1.1-2
libdvdread-7.0.1-1  ·  libebook-0.1.3-4  ·  libepoxy-1.5.8-4
libepubgen-0.1.1-3  ·  libetonyek-0.1.12-2  ·  libevent-2.1.12-6
libexttextcat-3.4.6-1  ·  libffi-3.4.6-1  ·  libfreehand-0.1.2-5
libfyaml-0.9.6-3  ·  libgcrypt-1.12.2-1  ·  libgpg_error-1.61-2
libheif-1.21.2-1  ·  libinstpatch-1.1.7-1  ·  libjpeg_turbo-3.1.4.1-1
libjxl0.11-0.11.2-3  ·  libksane_kf6-26.04.0-1  ·  liblangtag-0.6.3-1
libmad-0.16.4-1  ·  libmms-0.6.4-5  ·  libmng-2.0.3-5
libmpeg2-0.5.1-7  ·  libmspub-0.1.4-6  ·  libmwaw-0.3.22-2
libmypaint-1.6.1-3  ·  libmysqlclient-6.1.6-5  ·  libnice-0.1.23-1
libnotify-0.8.1-2  ·  libnsgif-1.0.0-1  ·  libnumbertext-1.0.6-1
libodfgen-0.1.7-1  ·  libpagemaker-0.0.4-3  ·  libproxy-0.5.12-2
libqt5pas-1.2.16-1  ·  libqxp-0.0.2-5  ·  libraw24-0.22.0-1
libreoffice-24.8.1.1-1  ·  libreplaygain-475-4  ·  librevenge-0.0.6-1
librsvg-2.62.3-1  ·  libsamplerate-0.2.2-2  ·  libsdl-1.2.15-19
libsdl3-3.2.28-2  ·  libsecret-0.21.7-2  ·  libsigc++-2.9.3-1
libsndfile-1.2.2-2  ·  libsoup-2.60.2-5  ·  libsrtp2-2.5.0-2
libstaroffice-0.0.8-1  ·  libtasn1-4.19.0-1  ·  libtool_libltdl-2.5.4-1
libupnp-1.14.17-1  ·  libusb-1.0.26-2  ·  libuuid-1.3.1-5
libvisio-0.1.7-6  ·  libvpx1.15-1.15.2-1  ·  libwmf-0.2.12-2
libwpd-0.10.3-3  ·  libwpg-0.3.4-1  ·  libwps-0.4.14-1
libxkbcommon-1.7.0-1  ·  libxmlb-0.3.28-1  ·  libyaml-0.2.5-2
libzip-1.11.4-1  ·  libzmf-0.0.2-7  ·  lpsolve-5.5.2.5-3
lua5.2-5.2.4-5  ·  media_helpers-0.1-1  ·  minizip-1.3-1
mpfr-4.2.0-3  ·  mpg123-1.32.9-1  ·  musepack_tools-475-5
mypaint_brushes-2.0.2-1  ·  mythes-1.2.4-5  ·  neon-0.36.0-1
nettle-3.7.3-1  ·  nspr-4.36-1  ·  nss-3.126-1
openal-1.21.1-5  ·  openblas-0.3.30-1  ·  opencsg-1.8.1-1
openexr3.2-3.2.4-3  ·  openh264-2.4.1-1  ·  openldap2.4-2.4.48-5
openscad-2021.01-3  ·  orc-0.4.41-2  ·  orcus0.18-0.19.2-2
p11_kit-0.25.5-2  ·  pango-1.58.0-1  ·  pangomm-2.46.2-1
pdflib-5.0.3-4  ·  pdfwriter-1.0-4  ·  pixman-0.46.4-1
poppler24-24.12.0-2  ·  poppler24_qt6-24.12.0-2  ·  poppler25.12-25.12.0-2
poppler25.12_glib-25.12.0-2  ·  poppler_data-0.4.12-1  ·  portaudio-19.07.00-3
potrace-1.16-2  ·  python3.10-3.10.20-3  ·  python3.14-3.14.6-2
qca_qt6-2.3.10-1  ·  qcad-3.26.4.12-1  ·  qscintilla-2.11.6-7
qsystray-5.15.2.14-1  ·  qt5-5.15.19-1  ·  qt5_script-5.15.19-2
qt6_5compat-6.10.3-2  ·  qt6_base-6.10.3-2  ·  qt6_declarative-6.10.3-2
qt6_imageformats-6.10.3-2  ·  qt6_multimedia-6.10.3-2  ·  qt6_shadertools-6.10.3-2
qt6_speech-6.10.3-2  ·  qt6_svg-6.10.3-2  ·  qt6_translations-6.10.3-1
qthaikuplugins-5.15.16.0-1  ·  raptor-2.0.15-10  ·  rasqal-0.9.33-6
redland-1.0.17-9  ·  rtmpdump-2.4_20161210-8  ·  sane_backends-1.3.1-3
schroedinger-1.0.11-7  ·  shared_mime_info-1.15-2  ·  snowball_stemmer3-3.0.1-1
solid6-6.28.0-1  ·  sonnet6-6.28.0-1  ·  suitesparse-7.12.1-2
taglib-1.13.1-1  ·  tbb-2022.3.0-1  ·  tremor-1.0.0~git-2
twolame-0.4.0-3  ·  unixodbc-2.3.11-1  ·  vlc-3.0.23-1
vulkan-1.4.311-1  ·  waterfox_bin-6.6.16.1-1  ·  wayland-1.21.0~git-3
wayland_server-0.1.20251004-1  ·  x264-20220222-1  ·  x265-3.5-6
xkeyboard_config-2.41-1  ·  xlibe-0.3.3-1  ·  xmlsec-1.2.37-3
xmlsec_nss-1.2.37-3  ·  xz_utils-5.8.3-1  ·  zlib-1.3.2-1
```
