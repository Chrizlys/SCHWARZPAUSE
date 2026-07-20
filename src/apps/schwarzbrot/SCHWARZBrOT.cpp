// SCHWARZBrOT launcher — starts the bundled offline AI engine (llama-server)
// and opens the branded chat UI in WebPositive.
//
// The engine, its runtime libraries and the web UI ship inside the OS image
// under /boot/system/data/schwarzbrot/ (added via SchwarzpauseBranding). The
// 2.87 GB Qwen model does NOT: baking it in produces an image that will not
// boot, so the model is an optional extra. The user gets it in one of two ways,
// both handled here and both fully offline once done:
//   * FindModel() picks up a model file the user placed on any mounted volume
//     (a second USB stick), or one previously downloaded to disk; or
//   * with no model present, we offer a one-click download from our own Hugging
//     Face repository to wherever there is room — the disk when installed, or a
//     plugged-in USB stick when running live. The user never picks a path or
//     visits Hugging Face; the URL is the invisible backend.
// A model downloaded to disk can be removed again from the running app.
//
// Why the engine is copied out: the payload lives in packagefs, which is
// READ-ONLY and exposes bundled raw files without an execute bit — so the binary
// can neither be chmod'd nor executed in place. On first run we copy just the
// binary into a writable dir under $HOME, mark it executable and run it there.
//
// Build (Haiku):  g++ -std=c++17 -O2 SCHWARZBrOT.cpp -o SCHWARZBrOT -lnetwork

#include <arpa/inet.h>
#include <dirent.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/statvfs.h>
#include <unistd.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

static const char* kAppDir = "/boot/system/data/schwarzbrot";
static const char* kModelName = "Qwen3.5-4B-Q4_K_M.gguf";
static const int kPort = 8080;

// Free space we insist on before downloading: the ~3 GB model plus room to
// breathe for wget and the filesystem.
static const long long kNeededBytes = 3500LL * 1024 * 1024;

// The master GGUF lives in our own Hugging Face repository, so the exact model
// we validated is the one users get. Note the "/resolve/" path (the raw file),
// NOT the "/blob/" web-viewer URL, so wget fetches the model and not an HTML
// page.
static const char* kModelUrl =
	"https://huggingface.co/Chrizly/Qwen3.5-4B-Q4_K_M.gguf/resolve/main/"
	"Qwen3.5-4B-Q4_K_M.gguf?download=true";


static bool
FileExists(const std::string& path)
{
	struct stat st;
	return stat(path.c_str(), &st) == 0 && S_ISREG(st.st_mode);
}


static void
MakeDir(const std::string& path)
{
	mkdir(path.c_str(), 0755);
}


static std::string
DirOf(const std::string& path)
{
	size_t slash = path.find_last_of('/');
	return (slash == std::string::npos) ? std::string(".") : path.substr(0, slash);
}


static bool
CopyFile(const std::string& src, const std::string& dst)
{
	std::ifstream in(src, std::ios::binary);
	if (!in.good())
		return false;
	std::ofstream out(dst, std::ios::binary | std::ios::trunc);
	if (!out.good())
		return false;
	out << in.rdbuf();
	return out.good();
}


static bool
EndsWith(const std::string& text, const char* suffix)
{
	size_t length = strlen(suffix);
	return text.size() >= length
		&& text.compare(text.size() - length, length, suffix) == 0;
}


static long long
FreeBytes(const std::string& path)
{
	struct statvfs info;
	if (statvfs(path.c_str(), &info) != 0)
		return -1;
	return (long long)info.f_bavail * (long long)info.f_frsize;
}


// Can we actually create a file here? (A mounted volume may be read-only, and
// statvfs alone would not tell us.)
static bool
DirWritable(const std::string& dir)
{
	std::string probe = dir + "/.schwarzbrot_write_test";
	std::ofstream f(probe.c_str());
	bool ok = f.good();
	f.close();
	if (ok)
		remove(probe.c_str());
	return ok;
}


// The first *.gguf in a directory, or "" if it holds none.
static std::string
AnyModelIn(const std::string& dir)
{
	DIR* handle = opendir(dir.c_str());
	if (handle == NULL)
		return "";

	std::string found;
	struct dirent* entry;
	while ((entry = readdir(handle)) != NULL) {
		std::string name(entry->d_name);
		if (EndsWith(name, ".gguf")) {
			found = dir + "/" + name;
			break;
		}
	}
	closedir(handle);
	return found;
}


// Names at the filesystem root that are the OS itself, never a user volume.
static bool
IsSystemRootEntry(const std::string& name)
{
	return name == "." || name == ".." || name == "boot" || name == "bin"
		|| name == "dev" || name == "etc" || name == "packages"
		|| name == "system" || name == "tmp" || name == "var";
}


// Where the model may live, best candidate first: a copy installed on this
// system, then the (normally absent) baked-in copy, then every mounted volume —
// which is how the user's model USB stick is picked up, since Haiku mounts
// volumes at the root. Both the volume itself and a schwarzbrot/ folder on it
// are searched, so it doesn't matter whether they tidied the stick up.
static std::string
FindModel()
{
	const char* home = std::getenv("HOME");
	std::string homeDir = (home != NULL && *home != '\0') ? home : "/boot/home";

	std::vector<std::string> dirs;
	dirs.push_back("/boot/system/non-packaged/data/schwarzbrot");
	dirs.push_back(homeDir + "/config/settings/SCHWARZBrOT/models");
	dirs.push_back(kAppDir);

	DIR* root = opendir("/");
	if (root != NULL) {
		struct dirent* entry;
		while ((entry = readdir(root)) != NULL) {
			std::string name(entry->d_name);
			if (name == "." || name == ".." || name == "boot")
				continue;
			dirs.push_back("/" + name);
			dirs.push_back("/" + name + "/schwarzbrot");
		}
		closedir(root);
	}

	// Prefer the model we ship, but accept any .gguf the user brought along.
	for (size_t i = 0; i < dirs.size(); i++) {
		std::string exact = dirs[i] + "/" + kModelName;
		if (FileExists(exact))
			return exact;
	}
	for (size_t i = 0; i < dirs.size(); i++) {
		std::string any = AnyModelIn(dirs[i]);
		if (!any.empty())
			return any;
	}
	return "";
}


// Choose where to download the model: our own folder on the boot volume if it
// has room (the installed-on-disk case), otherwise the first mounted volume with
// enough writable free space (a USB stick plugged in for a live boot). Returns
// "" if nothing has room.
static std::string
PickDownloadDir(const std::string& ourDir)
{
	MakeDir(ourDir);
	if (DirWritable(ourDir) && FreeBytes(ourDir) >= kNeededBytes)
		return ourDir;

	// Identify the boot volume by its device id, so we never download onto the
	// very USB stick SCHWARZPAUSE is running from — it also shows up at the root
	// under its own label (e.g. "/Schwarzpause OS"), not just at /boot.
	dev_t bootDev = (dev_t)-1;
	struct stat bootSt;
	if (stat("/boot", &bootSt) == 0)
		bootDev = bootSt.st_dev;

	DIR* root = opendir("/");
	if (root == NULL)
		return "";

	std::string chosen;
	struct dirent* entry;
	while ((entry = readdir(root)) != NULL) {
		std::string name(entry->d_name);
		if (IsSystemRootEntry(name))
			continue;
		std::string vol = "/" + name;
		struct stat st;
		if (stat(vol.c_str(), &st) == 0 && S_ISDIR(st.st_mode)
			&& st.st_dev != bootDev
			&& DirWritable(vol) && FreeBytes(vol) >= kNeededBytes) {
			chosen = vol;
			break;
		}
	}
	closedir(root);
	return chosen;
}


// Put a native dialog on screen via Haiku's `alert` tool and return the title of
// the button the user pressed. Shelling out keeps this app free of the BeAPI, so
// it still cross-checks with a plain g++ before we spend a build on it.
static std::string
RunAlert(const std::string& text, const std::string& button1,
	const std::string& button2)
{
	std::string cmd = "alert --info \"" + text + "\" \"" + button1 + "\"";
	if (!button2.empty())
		cmd += " \"" + button2 + "\"";

	FILE* pipe = popen(cmd.c_str(), "r");
	if (pipe == NULL)
		return "";

	std::string out;
	char buffer[256];
	while (fgets(buffer, sizeof(buffer), pipe) != NULL)
		out += buffer;
	pclose(pipe);

	while (!out.empty() && (out[out.size() - 1] == '\n'
			|| out[out.size() - 1] == '\r')) {
		out.erase(out.size() - 1);
	}
	return out;
}


static void
OpenBrowser(const std::string& url)
{
	std::system(("open \"" + url + "\" &").c_str());
}


// Fetch the model in a visible Terminal window, so the user can watch wget's
// progress and keep working meanwhile. Non-blocking on purpose.
static void
StartDownload(const std::string& work, const std::string& destFile)
{
	std::string script = work + "/download-model.sh";
	{
		std::ofstream out(script.c_str());
		out << "#!/bin/sh\n"
			<< "echo 'Downloading the Qwen AI model for SCHWARZBrOT (~3 GB).'\n"
			<< "echo 'You can keep using SCHWARZPAUSE while this runs.'\n"
			<< "echo\n"
			<< "wget -O \"" << destFile << "\" \"" << kModelUrl << "\"\n"
			<< "echo\n"
			<< "echo 'Finished. Click SCHWARZBrOT again to start chatting.'\n"
			<< "sleep 30\n";
	}
	chmod(script.c_str(), 0755);
	std::system(("Terminal \"" + script + "\" &").c_str());
}


// No model found: offer a one-click download to wherever there is room. The user
// never picks a path or sees Hugging Face.
static void
OfferDownload(const std::string& work)
{
	std::string ourDir = work + "/models";
	std::string dir = PickDownloadDir(ourDir);

	if (dir.empty()) {
		RunAlert("SCHWARZBrOT needs the Qwen AI model (about 3 GB) to chat, but "
			"there is no room for it here.\n\nIf you are running SCHWARZPAUSE "
			"live from a USB stick: plug in a second empty stick (8 GB or more, "
			"formatted FAT32) and click SCHWARZBrOT again - it will download the "
			"model onto that stick for you.\n\nIf you already have the model "
			"file, copy it onto a stick and plug it in.", "OK", "");
		return;
	}

	bool onDisk = (dir == ourDir);
	std::string where = onDisk ? std::string("your disk")
		: ("your USB stick (" + dir.substr(1) + ")");

	std::string answer = RunAlert(
		"SCHWARZBrOT only works together with the local Qwen AI model "
		"(about 3 GB).\n\nDownload it to " + where + "?\n\nIt runs completely "
		"offline afterwards - nothing you type ever leaves your computer. You "
		"can remove the model again anytime from inside SCHWARZBrOT.",
		"Cancel", "Download");

	if (answer != "Download")
		return;

	StartDownload(work, dir + "/" + kModelName);

	RunAlert("The download is running in a Terminal window so you can watch its "
		"progress.\n\nWhen it has finished, click SCHWARZBrOT again to start "
		"chatting.", "OK", "");
}


// Is the engine already listening on the local port? (Avoids loading the model
// twice when the app is clicked again while the server is still up.)
static bool
ServerIsUp(int port)
{
	int sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock < 0)
		return false;
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	bool up = connect(sock, (sockaddr*)&addr, sizeof(addr)) == 0;
	close(sock);
	return up;
}


// Stop the running engine, using the PID we recorded when we spawned it.
static void
StopServer(const std::string& work)
{
	std::string pidFile = work + "/llama-server.pid";
	std::ifstream in(pidFile.c_str());
	std::string line;
	if (in.good())
		std::getline(in, line);
	in.close();

	std::string pid;
	for (size_t i = 0; i < line.size(); i++) {
		if (line[i] >= '0' && line[i] <= '9')
			pid += line[i];
	}
	if (!pid.empty())
		std::system(("kill " + pid + " >/dev/null 2>&1").c_str());
	remove(pidFile.c_str());
}


// The engine is already running. Offer to open the chat, or to remove the model.
static void
HandleRunning(const std::string& url, const std::string& work)
{
	std::string answer = RunAlert("SCHWARZBrOT is running.", "Open chat",
		"Remove model");
	if (answer != "Remove model") {
		OpenBrowser(url);
		return;
	}

	std::string model = FindModel();
	if (model.empty()) {
		// Running, but the model file is gone (its stick was unplugged, say).
		StopServer(work);
		RunAlert("SCHWARZBrOT has stopped.", "OK", "");
		return;
	}

	// Only ever delete a copy WE downloaded to disk — never the user's own file
	// on their USB stick.
	std::string ourDir = work + "/models";
	bool ours = model.compare(0, ourDir.size(), ourDir) == 0;
	if (!ours) {
		RunAlert("Your AI model is stored on removable media:\n" + DirOf(model)
			+ "\n\nTo remove it, delete the file there, or simply unplug that "
			"stick - SCHWARZBrOT will then offer to download it again.", "OK",
			"");
		return;
	}

	std::string confirm = RunAlert("Remove the downloaded Qwen AI model (about "
		"3 GB) and free the space?\n\nSCHWARZBrOT will stop, and will offer to "
		"download the model again next time you open it.", "Cancel", "Remove");
	if (confirm != "Remove")
		return;

	StopServer(work);
	remove(model.c_str());
	RunAlert("Done - the Qwen AI model has been removed and SCHWARZBrOT has "
		"stopped.", "OK", "");
}


int
main()
{
	std::string url = "http://127.0.0.1:" + std::to_string(kPort) + "/";

	// Haiku's LIBRARY_PATH REPLACES the default library search paths, it does
	// not extend them — so the system directories have to be listed here too.
	// Ours comes first (it carries the exact libstdc++/libgcc_s/libgomp the
	// engine was built against, which a lean image may not have); the system
	// ones follow, and are where libnetwork.so and friends live.
	std::string libPath = std::string(kAppDir) + "/lib"
		+ ":/boot/system/lib"
		+ ":/boot/system/non-packaged/lib"
		+ ":/boot/home/config/lib"
		+ ":/boot/home/config/non-packaged/lib";

	const char* home = std::getenv("HOME");
	std::string base = (home != NULL && *home != '\0') ? home : "/boot/home";
	std::string work = base + "/config/settings/SCHWARZBrOT";
	std::string binDir = work + "/bin";
	MakeDir(work);

	// Already running? Offer to open the chat or remove the model.
	if (ServerIsUp(kPort)) {
		HandleRunning(url, work);
		return 0;
	}

	std::string engine = std::string(kAppDir) + "/llama-server";
	if (!FileExists(engine))
		return 0;

	// No model? Offer to download it (to disk, or a plugged-in stick).
	std::string model = FindModel();
	if (model.empty()) {
		OfferDownload(work);
		return 0;
	}

	MakeDir(binDir);
	std::string binDst = binDir + "/llama-server";
	if (!FileExists(binDst)) {
		CopyFile(engine, binDst);
		chmod(binDst.c_str(), 0755);
	}

	// Spawn the engine detached, recording its PID so "Remove model" can stop it.
	// The model path is quoted because a volume name may contain spaces. Output
	// goes to a log rather than /dev/null so failures are diagnosable.
	std::string cmd = "LIBRARY_PATH=\"" + libPath + "\" \"" + binDst + "\""
		+ " -m \"" + model + "\""
		+ " -c 2048 --parallel 1"
		+ " --path \"" + std::string(kAppDir) + "/ui\""
		+ " --host 127.0.0.1 --port " + std::to_string(kPort)
		+ " >>\"" + work + "/llama-server.log\" 2>&1 &"
		+ " echo $! > \"" + work + "/llama-server.pid\"";
	std::system(cmd.c_str());

	// The engine only starts listening once the model has loaded, which off a
	// USB stick means reading ~3 GB at stick speed — several minutes is normal.
	// Wait for the port before opening the UI, so WebPositive doesn't land on a
	// refused connection. If this ever runs out, llama-server.log has the story.
	for (int i = 0; i < 600 && !ServerIsUp(kPort); i++)
		sleep(1);

	OpenBrowser(url);
	return 0;
}
