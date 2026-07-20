// SCHWARZSEHER live bridge — cross-platform C++ (Haiku + Windows).
// The OpenSCAD codegen is compiled IN (from scadgen.cpp, verified byte-identical to the
// Python/JS). The browser only ever sends the shape model as JSON and gets .scad back.
//
// Build (Haiku):    g++ -std=c++17 -O2 bridge.cpp -o schwarzseher -lnetwork
// Build (Windows):  g++ -std=c++17 -O2 -static bridge.cpp -o schwarzseher.exe -lws2_32
//
// Env: SCHWARZSEHER_NO_OPEN=1  don't launch browser/OpenSCAD (tests)
//      SCHWARZSEHER_BIND=0.0.0.0  listen on all interfaces (e.g. reachable from a VM)
//      OPENSCAD_EXE=<path>  explicit OpenSCAD binary
#define SCADGEN_LIB
#include "scadgen.cpp"        // JV, Parser, buildScad, mem() ...  (the verified codegen)
#include "ui_assets.h"        // embedded UI (index.html/styles.css/app.js) -> single self-contained binary

#include <thread>
#include <cctype>
#include <cstring>
#include <sys/stat.h>
#ifdef _WIN32
  #include <winsock2.h>
  #include <ws2tcpip.h>
  #include <direct.h>
  typedef SOCKET sock_t;
  #define SOCK_CLOSE closesocket
#else
  #include <sys/socket.h>
  #include <netinet/in.h>
  #include <arpa/inet.h>
  #include <unistd.h>
  typedef int sock_t;
  #define SOCK_CLOSE ::close
  #ifndef INVALID_SOCKET
    #define INVALID_SOCKET (-1)
  #endif
#endif

static const int PORT = 8765;
static std::string HERE, ROOT, MODEL, IMPORTS;
static const char* WARNING =
  "// ==================================================================\n"
  "//  DO NOT EDIT OR SAVE THIS FILE IN HERE !\n"
  "//  - THIS IS THE LIVE PREVIEW LINK -\n"
  "//  TO KEEP YOUR WORK, USE FILE > SAVE AS...  in SCHWARZSEHER.\n"
  "// ==================================================================\n\n";

// ---------- small fs / string helpers ----------
static bool fileExists(const std::string& p) { std::ifstream f(p, std::ios::binary); return f.good(); }
static bool readFile(const std::string& p, std::string& out) { std::ifstream f(p, std::ios::binary); if (!f.good()) return false; out.assign((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>()); return true; }
static void writeBin(const std::string& p, const std::string& data) { std::ofstream f(p, std::ios::binary); f.write(data.data(), (std::streamsize)data.size()); }
static void writeLive(const std::string& scad) { std::ofstream f(MODEL, std::ios::binary); f << WARNING << scad; } // UTF-8 / LF
static void makeDir(const std::string& p) {
#ifdef _WIN32
  _mkdir(p.c_str());
#else
  mkdir(p.c_str(), 0755);
#endif
}
static std::string dirOf(const std::string& path) { size_t s = path.find_last_of("/\\"); return (s == std::string::npos) ? std::string(".") : path.substr(0, s); }
static int hexv(char c) { if (c >= '0' && c <= '9') return c - '0'; c |= 32; if (c >= 'a' && c <= 'f') return c - 'a' + 10; return 0; }
static std::string urldecode(const std::string& s) { std::string r; for (size_t i = 0; i < s.size(); i++) { if (s[i] == '%' && i + 2 < s.size()) { r += (char)(hexv(s[i + 1]) * 16 + hexv(s[i + 2])); i += 2; } else if (s[i] == '+') r += ' '; else r += s[i]; } return r; }
static bool endsWith(const std::string& p, const char* e) { size_t n = std::strlen(e); return p.size() >= n && p.compare(p.size() - n, n, e) == 0; }
static std::string ctypeFor(const std::string& p) { if (endsWith(p, ".html")) return "text/html; charset=utf-8"; if (endsWith(p, ".css")) return "text/css; charset=utf-8"; if (endsWith(p, ".js")) return "text/javascript; charset=utf-8"; return "application/octet-stream"; }
static std::string queryParam(const std::string& q, const std::string& key) {
  std::string pref = key + "=";
  size_t p = (q.compare(0, pref.size(), pref) == 0) ? 0 : q.find("&" + pref);
  if (p == std::string::npos) return "";
  size_t v = (p == 0 ? 0 : p + 1) + pref.size();
  size_t e = q.find('&', v);
  return q.substr(v, e == std::string::npos ? std::string::npos : e - v);
}
static std::string sanitize(const std::string& nm) {
  std::string base = nm; size_t s = base.find_last_of("/\\"); if (s != std::string::npos) base = base.substr(s + 1);
  std::string r; for (char c : base) r += (std::isalnum((unsigned char)c) || c == '.' || c == '_' || c == '-' || c == ' ') ? c : '_';
  return r.empty() ? std::string("part.bin") : r;
}

// ---------- HTTP ----------
struct Req { std::string method, target, body; };
static Req readRequest(sock_t c) {
  std::string data; char buf[8192]; long clen = -1; size_t hdrEnd = std::string::npos;
  for (;;) {
    if (hdrEnd == std::string::npos) {
      hdrEnd = data.find("\r\n\r\n");
      if (hdrEnd != std::string::npos) {
        std::string h = data.substr(0, hdrEnd), lower = h;
        for (auto& ch : lower) ch = (char)std::tolower((unsigned char)ch);
        size_t cl = lower.find("content-length:");
        clen = (cl != std::string::npos) ? std::strtol(h.c_str() + cl + 15, nullptr, 10) : 0;
      }
    }
    if (hdrEnd != std::string::npos && (long)data.size() - (long)(hdrEnd + 4) >= clen) break;
    int r = recv(c, buf, sizeof buf, 0);
    if (r <= 0) break;
    data.append(buf, r);
  }
  Req q; size_t s1 = data.find(' '), s2 = (s1 == std::string::npos) ? s1 : data.find(' ', s1 + 1);
  if (s2 != std::string::npos) { q.method = data.substr(0, s1); q.target = data.substr(s1 + 1, s2 - s1 - 1); }
  if (hdrEnd != std::string::npos && clen > 0) q.body = data.substr(hdrEnd + 4, clen);
  return q;
}
static void sendAll(sock_t c, const std::string& s) { size_t off = 0; while (off < s.size()) { int r = send(c, s.data() + off, (int)(s.size() - off), 0); if (r <= 0) break; off += (size_t)r; } }
static void sendResp(sock_t c, int code, const std::string& ctype, const std::string& body, const std::string& extra = "") {
  const char* st = code == 200 ? "OK" : code == 404 ? "Not Found" : code == 403 ? "Forbidden" : "Error";
  std::string h = "HTTP/1.0 " + std::to_string(code) + " " + st + "\r\nContent-Type: " + ctype +
    "\r\nContent-Length: " + std::to_string(body.size()) + "\r\n" + extra + "Connection: close\r\n\r\n";
  sendAll(c, h); sendAll(c, body);
}
static void handle(sock_t c) {
  Req q = readRequest(c);
  size_t qm = q.target.find('?');
  std::string path = qm == std::string::npos ? q.target : q.target.substr(0, qm);
  std::string query = qm == std::string::npos ? "" : q.target.substr(qm + 1);
  if (q.method == "POST" && path == "/model") {
    bool pristine = queryParam(query, "pristine") == "1";
    Parser P(q.body); JV root = P.parse();
    std::string scad = buildScad(mem(root, "shapes"), mem(root, "imports"), q.body, pristine); // body used VERBATIM as the model line
    writeLive(scad);
    sendResp(c, 200, "text/plain; charset=utf-8", scad);
  } else if (q.method == "POST" && path.rfind("/import", 0) == 0) {
    std::string name = sanitize(urldecode(queryParam(query, "name")));
    makeDir(IMPORTS); writeBin(IMPORTS + "/" + name, q.body);
    sendResp(c, 200, "text/plain", "imports/" + name);
  } else if (q.method == "GET") {
    std::string rel = urldecode(path); if (rel == "/") rel = "/index.html";
    bool served = false;
    for (int i = 0; i < EMB_ASSET_COUNT; i++) if (rel == EMB_ASSETS[i].path) { sendResp(c, 200, EMB_ASSETS[i].ctype, std::string((const char*)EMB_ASSETS[i].data, EMB_ASSETS[i].len)); served = true; break; } // embedded UI first
    if (!served) {
      if (rel.find("..") != std::string::npos) sendResp(c, 403, "text/plain", "forbidden");
      else { std::string data; if (readFile(ROOT + rel, data)) sendResp(c, 200, ctypeFor(rel), data); else sendResp(c, 404, "text/plain", "not found"); }
    }
  } else sendResp(c, 404, "text/plain", "not found");
  SOCK_CLOSE(c);
}

// ---------- launch OpenSCAD + browser ----------
static std::string findOpenscad() {
  const char* env = std::getenv("OPENSCAD_EXE");
  const char* cands[] = { env, "/boot/system/apps/OpenSCAD/bin/openscad", "/boot/home/config/apps/OpenSCAD/bin/openscad",
    "C:\\Program Files\\OpenSCAD\\openscad.exe", "C:\\Program Files\\OpenSCAD (Nightly)\\openscad.exe" };
  for (const char* p : cands) if (p && fileExists(p)) return p;
  return "";
}
// Re-open the UI against a bridge that is already running (the user closed the
// window and clicked the icon again).
//
// The two programs behave differently, so they need different handling:
//   * this bridge is a SERVER -- closing the browser leaves it running;
//   * OpenSCAD is a normal GUI app -- closing its window really does quit it.
// So the browser is always re-opened, while OpenSCAD is only started again if the
// user actually closed it. It is spawned directly rather than through the launch
// roster, so starting it blindly would stack a second window on the same live file.
static bool openscadRunning() {
#ifdef _WIN32
  return false;
#else
  // The [o] keeps grep from matching its own command line.
  return std::system("ps 2>/dev/null | grep -q '[o]penscad'") == 0;
#endif
}

static void openOpenscad() {
  std::string osc = findOpenscad();
  if (osc.empty())
    return;
#ifdef _WIN32
  std::system(("start \"\" \"" + osc + "\" \"" + MODEL + "\"").c_str());
#else
  std::system(("\"" + osc + "\" \"" + MODEL + "\" &").c_str());
#endif
}

static void reopen() {
  std::string url = "http://localhost:" + std::to_string(PORT);
#ifdef _WIN32
  std::system(("start \"\" \"" + url + "\"").c_str());
#else
  std::system(("open \"" + url + "\" &").c_str());
#endif
  if (!openscadRunning())
    openOpenscad();
}

static void launch() {
  std::string url = "http://localhost:" + std::to_string(PORT), osc = findOpenscad();
#ifdef _WIN32
  std::system(("start \"\" \"" + url + "\"").c_str());
  if (!osc.empty()) std::system(("start \"\" \"" + osc + "\" \"" + MODEL + "\"").c_str());
#else
  std::system(("open \"" + url + "\" &").c_str());                 // Haiku: default browser (WebPositive)
  if (!osc.empty()) std::system(("\"" + osc + "\" \"" + MODEL + "\" &").c_str());
#endif
  if (osc.empty()) {
    std::printf("  !! OpenSCAD not found — open it manually on the live .scad above.\n");
#ifndef _WIN32
    // Launched from the Deskbar there is no terminal to read that in, so say it
    // on screen instead. Non-blocking (&) on purpose: the HTTP server below must
    // come up whether or not anyone clicks OK.
    std::string msg =
      "SCHWARZSEHER renders your drawings with OpenSCAD, which is not installed yet.\n\n"
      "Install it from the Haiku Software Depot (Applications > HaikuDepot), or open a "
      "Terminal and type (in this order):\n\n"
      "    pkgman install gstreamer\n"
      "    pkgman install openscad\n\n"
      "Then start SCHWARZSEHER again. Until then you can draw, but nothing will be "
      "rendered in 3D.";
    std::system(("alert --info \"" + msg + "\" \"OK\" >/dev/null 2>&1 &").c_str());
#endif
  }
}

int main(int argc, char** argv) {
  (void)argc;
  HERE = dirOf(argv[0]);
  const char* webEnv = std::getenv("SCHWARZSEHER_WEB");
  ROOT = webEnv ? std::string(webEnv) : HERE + "/../web-prototype";
  // The live file + imports must live in a WRITABLE place. When baked into the OS
  // the binary sits in read-only /boot/system/apps, so write under $HOME instead
  // (falls back to the binary dir for the Windows/dev standalone build).
  const char* home = std::getenv("HOME");
  std::string work = (home && *home) ? std::string(home) + "/SCHWARZSEHER" : HERE;
  makeDir(work);
  MODEL = work + "/SCHWARZSEHER_LIVE_do-not-edit.scad";
  IMPORTS = work + "/imports";
  if (!fileExists(MODEL)) writeLive("// draw a shape or import a base part\ncube([10, 10, 10]);\n");
#ifdef _WIN32
  WSADATA wsa; WSAStartup(MAKEWORD(2, 2), &wsa);
#endif
  sock_t s = socket(AF_INET, SOCK_STREAM, 0);
  int opt = 1; setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof opt);
  const char* bindEnv = std::getenv("SCHWARZSEHER_BIND");
  std::string bindAddr = bindEnv ? bindEnv : "127.0.0.1";
  sockaddr_in addr{}; addr.sin_family = AF_INET; addr.sin_port = htons(PORT); addr.sin_addr.s_addr = inet_addr(bindAddr.c_str());
  if (bind(s, (sockaddr*)&addr, sizeof addr) != 0) {
    // Port busy = the bridge is already running. That happens when the user closed
    // the browser/OpenSCAD windows and clicked SCHWARZSEHER again — exiting here
    // would leave them with a server running and nothing on screen, and no way back
    // except typing the localhost URL by hand. So just re-open the UI (and OpenSCAD)
    // against the server that is already up.
    std::printf("  Port %d busy — bridge already running; re-opening the UI.\n", PORT);
    if (!std::getenv("SCHWARZSEHER_NO_OPEN")) reopen();
    return 0;
  }
  listen(s, 16);
  std::printf("\n  SCHWARZSEHER live bridge (c++) is running\n  ----------------------------------------------\n");
  std::printf("  Browser UI  ->  http://localhost:%d\n", PORT);
  if (bindAddr == "0.0.0.0") std::printf("  From a VM   ->  http://10.0.2.2:%d\n", PORT);
  std::printf("  Model file  ->  %s\n  ----------------------------------------------\n", MODEL.c_str());
  if (std::getenv("SCHWARZSEHER_NO_OPEN")) std::printf("  (auto-open skipped)\n\n");
  else { std::printf("  In OpenSCAD, enable once:  Design > Automatic Reload and Preview\n\n"); launch(); }
  for (;;) { sock_t c = accept(s, nullptr, nullptr); if (c == (sock_t)INVALID_SOCKET) continue; std::thread(handle, c).detach(); }
  return 0;
}
