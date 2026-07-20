// SCHWARZSEHER codegen — C++ port of codegen.py (verified byte-identical).
// Self-contained: minimal JSON parser + geometry + OpenSCAD generator + CLI.
// Build:  g++ -std=c++17 -O2 -static scadgen.cpp -o scadgen
// Use:    scadgen model.json [1]      (arg2 "1" => pristine camera)
#include <string>
#include <vector>
#include <array>
#include <utility>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <algorithm>
#ifdef _WIN32
#include <io.h>
#include <fcntl.h>
#endif

using V2 = std::array<double, 2>;
using V3 = std::array<double, 3>;
using Box = std::array<std::array<double, 2>, 3>; // [x|y|z][lo|hi]
static constexpr double PI = 3.14159265358979323846;

// ---------------- minimal JSON ----------------
struct JV {
  enum T { NUL, BOOL, NUM, STR, ARR, OBJ } t = NUL;
  bool b = false; double num = 0; std::string str;
  std::vector<JV> arr;
  std::vector<std::pair<std::string, JV>> obj;
};
struct Parser {
  const char* p; const char* end;
  Parser(const std::string& s) : p(s.data()), end(s.data() + s.size()) {}
  void ws() { while (p < end && (*p == ' ' || *p == '\t' || *p == '\n' || *p == '\r')) p++; }
  JV parse() { return val(); }
  void utf8(std::string& r, int cp) {
    if (cp < 0x80) r += (char)cp;
    else if (cp < 0x800) { r += (char)(0xC0 | (cp >> 6)); r += (char)(0x80 | (cp & 0x3F)); }
    else { r += (char)(0xE0 | (cp >> 12)); r += (char)(0x80 | ((cp >> 6) & 0x3F)); r += (char)(0x80 | (cp & 0x3F)); }
  }
  int hex4() { int v = 0; for (int i = 0; i < 4 && p < end; i++) { char c = *p++; int d = (c <= '9') ? c - '0' : ((c | 32) - 'a' + 10); v = v * 16 + d; } return v; }
  std::string str() {
    std::string r; if (p < end && *p == '"') p++;
    while (p < end && *p != '"') {
      if (*p == '\\') { p++; if (p >= end) break; char e = *p++;
        switch (e) { case '"': r += '"'; break; case '\\': r += '\\'; break; case '/': r += '/'; break;
          case 'n': r += '\n'; break; case 't': r += '\t'; break; case 'r': r += '\r'; break;
          case 'b': r += '\b'; break; case 'f': r += '\f'; break; case 'u': utf8(r, hex4()); break;
          default: r += e; } }
      else r += *p++;
    }
    if (p < end) p++;
    return r;
  }
  JV val() {
    ws(); JV v; if (p >= end) return v;
    char c = *p;
    if (c == '{') { v.t = JV::OBJ; p++; ws(); if (p < end && *p == '}') { p++; return v; }
      while (p < end) { ws(); std::string k = str(); ws(); if (p < end && *p == ':') p++; JV vv = val(); v.obj.push_back({k, vv}); ws(); if (p < end && *p == ',') { p++; continue; } if (p < end && *p == '}') { p++; } break; } return v; }
    if (c == '[') { v.t = JV::ARR; p++; ws(); if (p < end && *p == ']') { p++; return v; }
      while (p < end) { v.arr.push_back(val()); ws(); if (p < end && *p == ',') { p++; continue; } if (p < end && *p == ']') { p++; } break; } return v; }
    if (c == '"') { v.t = JV::STR; v.str = str(); return v; }
    if (c == 't') { p += 4; v.t = JV::BOOL; v.b = true; return v; }
    if (c == 'f') { p += 5; v.t = JV::BOOL; v.b = false; return v; }
    if (c == 'n') { p += 4; return v; }
    const char* s = p; while (p < end && (*p == '-' || *p == '+' || *p == '.' || *p == 'e' || *p == 'E' || (*p >= '0' && *p <= '9'))) p++;
    v.t = JV::NUM; v.num = strtod(std::string(s, p).c_str(), nullptr); return v;
  }
};
// field accessors
static double jnum(const JV& v, double d = 0) { return v.t == JV::NUM ? v.num : (v.t == JV::BOOL ? (v.b ? 1 : 0) : d); }
static const JV* mem(const JV& o, const char* k) { if (o.t != JV::OBJ) return nullptr; for (auto& pr : o.obj) if (pr.first == k) return &pr.second; return nullptr; }
static double fnum(const JV& o, const char* k, double d = 0) { const JV* m = mem(o, k); return m ? jnum(*m, d) : d; }
static std::string fstr(const JV& o, const char* k, const char* d = "") { const JV* m = mem(o, k); return (m && m->t == JV::STR) ? m->str : std::string(d); }
static V2 fv2(const JV& o, const char* k) { const JV* m = mem(o, k); V2 r{0, 0}; if (m && m->t == JV::ARR) { if (m->arr.size() > 0) r[0] = jnum(m->arr[0]); if (m->arr.size() > 1) r[1] = jnum(m->arr[1]); } return r; }
static std::vector<V2> fpts(const JV& o, const char* k) { std::vector<V2> r; const JV* m = mem(o, k); if (m && m->t == JV::ARR) for (auto& e : m->arr) { V2 p{0, 0}; if (e.t == JV::ARR) { if (e.arr.size() > 0) p[0] = jnum(e.arr[0]); if (e.arr.size() > 1) p[1] = jnum(e.arr[1]); } r.push_back(p); } return r; }
static bool groupNull(const JV& s) { const JV* m = mem(s, "group"); return !m || m->t == JV::NUL; }
static double groupVal(const JV& s) { const JV* m = mem(s, "group"); return m ? jnum(*m) : 0; }
static bool isVoid(const JV& s) { return fstr(s, "role") == "void"; }

// ---------------- number formatting (matches JS Math.round + String, Python n()) ----------------
static std::string n(double x) {
  long long k = (long long)std::floor(x * 1000.0 + 0.5);
  if (k == 0) return "0";
  bool neg = k < 0; unsigned long long ak = neg ? (unsigned long long)(-k) : (unsigned long long)k;
  unsigned long long ip = ak / 1000, fp = ak % 1000;
  std::string s = (neg ? "-" : "") + std::to_string(ip);
  if (fp) { char buf[8]; std::snprintf(buf, sizeof buf, "%03llu", fp); std::string f(buf); while (!f.empty() && f.back() == '0') f.pop_back(); s += "." + f; }
  return s;
}
static std::string fixed3(double x) { char buf[64]; std::snprintf(buf, sizeof buf, "%.3f", x); return buf; }

// ---------------- geometry (ports codegen.py) ----------------
static std::array<int, 3> planeAxes(const std::string& p) { if (p == "XZ") return {0, 2, 1}; if (p == "YZ") return {1, 2, 0}; return {0, 1, 2}; }
static void planeBasis(const std::string& p, V3& eU, V3& eV) { if (p == "XZ") { eU = {1, 0, 0}; eV = {0, 0, 1}; } else if (p == "YZ") { eU = {0, 1, 0}; eV = {0, 0, 1}; } else { eU = {1, 0, 0}; eV = {0, 1, 0}; } }
static V3 v3add(const V3& a, const V3& b) { return {a[0] + b[0], a[1] + b[1], a[2] + b[2]}; }
static V3 v3mul(const V3& a, double k) { return {a[0] * k, a[1] * k, a[2] * k}; }
static V3 v3cross(const V3& a, const V3& b) { return {a[1] * b[2] - a[2] * b[1], a[2] * b[0] - a[0] * b[2], a[0] * b[1] - a[1] * b[0]}; }
static V2 ellipseHalf(double a, double b, double ang) { double c = std::cos(ang), s = std::sin(ang); return {std::hypot(a * c, b * s), std::hypot(a * s, b * c)}; }
static std::vector<V2> polyPts(double cx, double cy, double r, int sides) { std::vector<V2> p; for (int i = 0; i < sides; i++) { double a = i * 2 * PI / sides; p.push_back({cx + r * std::cos(a), cy + r * std::sin(a)}); } return p; }
static std::vector<V2> circlePts(V2 c, double r, int nn) { std::vector<V2> a; for (int i = 0; i < nn; i++) { double t = i * 2 * PI / nn; a.push_back({c[0] + r * std::cos(t), c[1] + r * std::sin(t)}); } return a; }

static std::string stype(const JV& s) { return fstr(s, "type"); }
static std::string splane(const JV& s) { const JV* m = mem(s, "plane"); return (m && m->t == JV::STR) ? m->str : std::string("XY"); }

static std::vector<V2> profileBoundary2d(const JV& s);
static std::vector<V2> profilePts2d(const JV& s) {
  std::string t = stype(s);
  if (t == "rect") { double cx = fnum(s, "cx"), cy = fnum(s, "cy"), hw = fnum(s, "hw"), hh = fnum(s, "hh"); return {{cx - hw, cy - hh}, {cx + hw, cy - hh}, {cx + hw, cy + hh}, {cx - hw, cy + hh}}; }
  if (t == "poly") return polyPts(fnum(s, "cx"), fnum(s, "cy"), fnum(s, "r"), (int)fnum(s, "sides"));
  if (t == "path") return fpts(s, "pts");
  if (t == "ellipse") return profileBoundary2d(s);
  return {{fnum(s, "cx"), fnum(s, "cy")}};
}
static std::vector<V2> profileBoundary2d(const JV& s) {
  std::string t = stype(s);
  if (t == "circle") return circlePts({fnum(s, "cx"), fnum(s, "cy")}, fnum(s, "r"), 48);
  if (t == "poly") return polyPts(fnum(s, "cx"), fnum(s, "cy"), fnum(s, "r"), (int)fnum(s, "sides"));
  if (t == "rect") { double cx = fnum(s, "cx"), cy = fnum(s, "cy"), hw = fnum(s, "hw"), hh = fnum(s, "hh"); return {{cx - hw, cy - hh}, {cx + hw, cy - hh}, {cx + hw, cy + hh}, {cx - hw, cy + hh}}; }
  if (t == "ellipse") { std::vector<V2> out; double cx = fnum(s, "cx"), cy = fnum(s, "cy"), a = fnum(s, "a"), bb = fnum(s, "b"), ang = fnum(s, "ang"); double ct = std::cos(ang), st = std::sin(ang); for (int i = 0; i < 48; i++) { double tt = i * 2 * PI / 48, x = a * std::cos(tt), y = bb * std::sin(tt); out.push_back({cx + x * ct - y * st, cy + x * st + y * ct}); } return out; }
  if (t == "path") return fpts(s, "pts");
  return profilePts2d(s);
}
struct RFrame { V2 D2, P2; std::vector<double> along, perp; int sign; V3 Dw, Rw, N, O; };
static RFrame revolveFrame(const JV& s) {
  std::string p = splane(s); V3 eU, eV; planeBasis(p, eU, eV);
  V2 A0 = fv2(s, "ax0"), A1 = fv2(s, "ax1");
  double dx = A1[0] - A0[0], dy = A1[1] - A0[1]; double dl = std::hypot(dx, dy); if (dl == 0) dl = 1; dx /= dl; dy /= dl;
  RFrame f; f.D2 = {dx, dy}; f.P2 = {-dy, dx};
  auto pts = profilePts2d(s);
  for (auto& q : pts) { f.along.push_back((q[0] - A0[0]) * f.D2[0] + (q[1] - A0[1]) * f.D2[1]); f.perp.push_back((q[0] - A0[0]) * f.P2[0] + (q[1] - A0[1]) * f.P2[1]); }
  double pmin = f.perp[0], pmax = f.perp[0]; for (double v : f.perp) { pmin = std::min(pmin, v); pmax = std::max(pmax, v); }
  if (stype(s) == "circle") { double r = fnum(s, "r"); pmin -= r; pmax += r; }
  f.sign = (-pmin > pmax) ? -1 : 1;
  f.Dw = v3add(v3mul(eU, f.D2[0]), v3mul(eV, f.D2[1]));
  f.Rw = v3mul(v3add(v3mul(eU, f.P2[0]), v3mul(eV, f.P2[1])), f.sign);
  V3 off{0, 0, 0}; const JV* om = mem(s, "off"); if (om && om->t == JV::ARR) { for (int i = 0; i < 3 && i < (int)om->arr.size(); i++) off[i] = jnum(om->arr[i]); }
  f.O = v3add(v3add(v3mul(eU, A0[0]), v3mul(eV, A0[1])), off);
  f.N = v3cross(f.Dw, f.Rw);
  return f;
}
static Box localBox(const JV& s) {
  auto pa = planeAxes(splane(s)); int ua = pa[0], va = pa[1], wa = pa[2];
  Box box{}; double z0 = fnum(s, "z0"), z1 = fnum(s, "z1");
  box[wa] = {std::min(z0, z1), std::max(z0, z1)};
  std::string t = stype(s);
  if (t == "slot") { V2 c1 = fv2(s, "c1"), c2 = fv2(s, "c2"); double r1 = fnum(s, "r1"), r2 = fnum(s, "r2");
    box[ua] = {std::min(c1[0] - r1, c2[0] - r2), std::max(c1[0] + r1, c2[0] + r2)};
    box[va] = {std::min(c1[1] - r1, c2[1] - r2), std::max(c1[1] + r1, c2[1] + r2)}; return box; }
  if (t == "path") { auto pts = fpts(s, "pts"); double ulo = 1e300, uhi = -1e300, vlo = 1e300, vhi = -1e300; for (auto& q : pts) { ulo = std::min(ulo, q[0]); uhi = std::max(uhi, q[0]); vlo = std::min(vlo, q[1]); vhi = std::max(vhi, q[1]); } box[ua] = {ulo, uhi}; box[va] = {vlo, vhi}; return box; }
  double uH, vH;
  if (t == "ellipse") { V2 eh = ellipseHalf(fnum(s, "a"), fnum(s, "b"), fnum(s, "ang")); uH = eh[0]; vH = eh[1]; }
  else { uH = (t == "rect") ? fnum(s, "hw") : fnum(s, "r"); vH = (t == "rect") ? fnum(s, "hh") : fnum(s, "r"); }
  double cx = fnum(s, "cx"), cy = fnum(s, "cy");
  box[ua] = {cx - uH, cx + uH}; box[va] = {cy - vH, cy + vH};
  return box;
}
static V3 localCenter(const JV& s) { Box b = localBox(s); return {(b[0][0] + b[0][1]) / 2, (b[1][0] + b[1][1]) / 2, (b[2][0] + b[2][1]) / 2}; }
static V3 shapeRot(const JV& s) { const JV* m = mem(s, "rot"); if (m && m->t == JV::ARR && m->arr.size() >= 3) return {jnum(m->arr[0]), jnum(m->arr[1]), jnum(m->arr[2])}; return {0, 0, 0}; }
static bool hasRot(const JV& s) { V3 r = shapeRot(s); return r[0] || r[1] || r[2]; }
static V3 codeRot(const JV& s) { V3 r = shapeRot(s); return {r[0], -r[1], r[2]}; }

// ---------------- OpenSCAD generation ----------------
static std::string scadRevolve(const JV& s) {
  RFrame f = revolveFrame(s);
  std::string meta = "// @schwarzseher " + stype(s) + " plane=" + splane(s) + " op=revolve" + (isVoid(s) ? " role=void" : "");
  V3 Rw = f.Rw, N = f.N, Dw = f.Dw, O = f.O;
  std::string M = "[[" + n(Rw[0]) + ", " + n(N[0]) + ", " + n(Dw[0]) + ", " + n(O[0]) + "], [" + n(Rw[1]) + ", " + n(N[1]) + ", " + n(Dw[1]) + ", " + n(O[1]) + "], [" + n(Rw[2]) + ", " + n(N[2]) + ", " + n(Dw[2]) + ", " + n(O[2]) + "], [0, 0, 0, 1]]";
  std::string profile; double minX, maxX, aMin, aMax;
  if (stype(s) == "circle") {
    double r = fnum(s, "r"), X0 = f.sign * f.perp[0];
    profile = "translate([" + n(X0) + ", " + n(f.along[0]) + "]) circle(r=" + n(r) + ");";
    minX = X0 - r; maxX = X0 + r; aMin = f.along[0] - r; aMax = f.along[0] + r;
  } else {
    std::vector<double> xs; for (double pp : f.perp) xs.push_back(f.sign * pp);
    std::string pts = "polygon(points=["; for (size_t i = 0; i < f.along.size(); i++) { if (i) pts += ", "; pts += "[" + n(xs[i]) + ", " + n(f.along[i]) + "]"; } pts += "]);";
    profile = pts;
    minX = xs[0]; maxX = xs[0]; for (double v : xs) { minX = std::min(minX, v); maxX = std::max(maxX, v); }
    aMin = f.along[0]; aMax = f.along[0]; for (double v : f.along) { aMin = std::min(aMin, v); aMax = std::max(aMax, v); }
  }
  if (minX < -1e-6) { double BIG = (std::max(maxX, std::max(std::fabs(aMin), std::fabs(aMax))) + 10) * 4; profile = "intersection() { " + profile + " translate([0, " + n(-BIG) + "]) square([" + n(BIG) + ", " + n(2 * BIG) + "]); }"; }
  return meta + "\n" + "multmatrix(" + M + ") rotate_extrude(angle=360) " + profile;
}
static std::string scadSolid(const JV& s) {
  if (fstr(s, "op") == "revolve") return scadRevolve(s);
  std::string plane = splane(s);
  Box box = localBox(s);
  std::string dx = n(box[0][1] - box[0][0]), dy = n(box[1][1] - box[1][0]), dz = n(box[2][1] - box[2][0]);
  std::string meta = "// @schwarzseher " + stype(s) + " plane=" + plane + (isVoid(s) ? " role=void" : "");
  double cx = fnum(s, "cx"), cy = fnum(s, "cy");
  auto wrapRound = [&](const std::string& prof) -> std::string {
    if (plane == "XY") return "translate([" + n(cx) + ", " + n(cy) + ", " + n(box[2][0]) + "]) linear_extrude(height=" + dz + ") " + prof + ";";
    if (plane == "XZ") return "translate([" + n(cx) + ", " + n(box[1][1]) + ", " + n(cy) + "]) rotate([90, 0, 0]) linear_extrude(height=" + dy + ") " + prof + ";";
    return "translate([" + n(box[0][0]) + ", " + n(cx) + ", " + n(cy) + "]) rotate([90, 0, 90]) linear_extrude(height=" + dx + ") " + prof + ";";
  };
  auto wrapAbs = [&](const std::string& prof) -> std::string {
    if (plane == "XY") return "translate([0, 0, " + n(box[2][0]) + "]) linear_extrude(height=" + dz + ") " + prof;
    if (plane == "XZ") return "translate([0, " + n(box[1][1]) + ", 0]) rotate([90, 0, 0]) linear_extrude(height=" + dy + ") " + prof;
    return "translate([" + n(box[0][0]) + ", 0, 0]) rotate([90, 0, 90]) linear_extrude(height=" + dx + ") " + prof;
  };
  std::string t = stype(s), body;
  if (t == "rect") body = "translate([" + n(box[0][0]) + ", " + n(box[1][0]) + ", " + n(box[2][0]) + "]) cube([" + dx + ", " + dy + ", " + dz + "]);";
  else if (t == "ellipse") body = wrapRound("rotate(" + fixed3(fnum(s, "ang") * 180 / PI) + ") scale([" + n(fnum(s, "a")) + ", " + n(fnum(s, "b")) + "]) circle(r=1)");
  else if (t == "slot") { V2 c1 = fv2(s, "c1"), c2 = fv2(s, "c2"); body = wrapAbs("hull() { translate([" + n(c1[0]) + ", " + n(c1[1]) + "]) circle(r=" + n(fnum(s, "r1")) + "); translate([" + n(c2[0]) + ", " + n(c2[1]) + "]) circle(r=" + n(fnum(s, "r2")) + "); }"); }
  else if (t == "path") { auto pts = fpts(s, "pts"); std::string pg = "polygon(points=["; for (size_t i = 0; i < pts.size(); i++) { if (i) pg += ", "; pg += "[" + n(pts[i][0]) + ", " + n(pts[i][1]) + "]"; } pg += "]);"; body = wrapAbs(pg); }
  else { std::string prof = "circle(r=" + n(fnum(s, "r")); if (t == "poly") prof += ", $fn=" + std::to_string((long long)fnum(s, "sides")); prof += ")"; body = wrapRound(prof); }
  if (hasRot(s)) { V3 C = localCenter(s), r = codeRot(s); body = "translate([" + n(C[0]) + ", " + n(C[1]) + ", " + n(C[2]) + "]) rotate([" + n(r[0]) + ", " + n(r[1]) + ", " + n(r[2]) + "]) translate([" + n(-C[0]) + ", " + n(-C[1]) + ", " + n(-C[2]) + "]) " + body; }
  return meta + "\n" + body;
}
static std::string join(const std::vector<std::string>& v, const std::string& sep) { std::string r; for (size_t i = 0; i < v.size(); i++) { if (i) r += sep; r += v[i]; } return r; }
static std::string reindent(const std::string& block, const std::string& pad) {
  std::vector<std::string> lines; size_t start = 0;
  for (size_t i = 0; i <= block.size(); i++) if (i == block.size() || block[i] == '\n') { lines.push_back(block.substr(start, i - start)); start = i + 1; }
  std::string r; for (size_t i = 0; i < lines.size(); i++) { if (i) r += "\n"; r += pad + lines[i]; } return r;
}
static std::string buildScad(const JV* shapes, const JV* imports, const std::string& meta, bool pristine) {
  std::string cam = pristine ? "$vpt = [0, 0, 12]; $vpr = [55, 0, 25]; $vpd = 320;\n" : "";
  std::string head = std::string("// SCHWARZSEHER Visual \xE2\x80\x94 generated OpenSCAD (source of truth)\n")
    + "// @schwarzseher:model " + meta + "\n" + "$fn = 64;\n" + cam + "\n";
  std::vector<const JV*> sh; if (shapes && shapes->t == JV::ARR) for (auto& e : shapes->arr) sh.push_back(&e);
  std::vector<const JV*> imp; if (imports && imports->t == JV::ARR) for (auto& e : imports->arr) imp.push_back(&e);
  std::vector<std::string> solidBlocks;
  for (auto* s : sh) if (!isVoid(*s) && groupNull(*s)) solidBlocks.push_back(scadSolid(*s));
  std::vector<double> seen;
  for (auto* s : sh) if (!isVoid(*s) && !groupNull(*s)) { double g = groupVal(*s); if (std::find(seen.begin(), seen.end(), g) == seen.end()) seen.push_back(g); }
  for (double g : seen) { std::vector<std::string> m; for (auto* s : sh) if (!isVoid(*s) && !groupNull(*s) && groupVal(*s) == g) m.push_back(scadSolid(*s)); if (!m.empty()) solidBlocks.push_back("intersection() {\n" + reindent(join(m, "\n"), "  ") + "\n}"); }
  for (auto* im : imp) if (fstr(*im, "kind") == "mesh") solidBlocks.push_back("// @schwarzseher-part " + fstr(*im, "name") + "\n" + "translate([" + n(fnum(*im, "tx")) + ", " + n(fnum(*im, "ty")) + ", " + n(fnum(*im, "tz")) + "]) import(\"" + fstr(*im, "path") + "\");");
  std::vector<std::string> voids; for (auto* s : sh) if (isVoid(*s)) voids.push_back(scadSolid(*s));
  std::vector<std::string> inc; for (auto* im : imp) if (fstr(*im, "kind") == "scad") inc.push_back("include <" + fstr(*im, "path") + ">");
  std::string scadIncludes = join(inc, "\n");
  std::string core;
  if (solidBlocks.empty()) core = "// draw a shape or import a base part\n";
  else { std::string solidPart = solidBlocks.size() == 1 ? solidBlocks[0] : "union() {\n" + reindent(join(solidBlocks, "\n"), "  ") + "\n}";
    core = voids.empty() ? (solidPart + "\n") : ("difference() {\n" + reindent(solidPart, "  ") + "\n" + reindent(join(voids, "\n"), "  ") + "\n}\n"); }
  return head + core + (scadIncludes.empty() ? "" : ("\n" + scadIncludes + "\n"));
}

#ifndef SCADGEN_LIB   // define SCADGEN_LIB to reuse the codegen from the bridge (no standalone main)
int main(int argc, char** argv) {
#ifdef _WIN32
  _setmode(_fileno(stdout), _O_BINARY);
#endif
  if (argc < 2) { std::fprintf(stderr, "usage: scadgen model.json [1]\n"); return 1; }
  std::ifstream in(argv[1], std::ios::binary);
  std::string raw((std::istreambuf_iterator<char>(in)), std::istreambuf_iterator<char>());
  bool pristine = argc > 2 && std::string(argv[2]) == "1";
  Parser P(raw); JV root = P.parse();
  std::string out = buildScad(mem(root, "shapes"), mem(root, "imports"), raw, pristine);
  std::fwrite(out.data(), 1, out.size(), stdout);
  return 0;
}
#endif
