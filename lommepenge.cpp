// router: http://192.0.0.1:2033 (previously 10.0.0.1)
// me: 212.242.118.242:11001
/* For sockaddr_in */
#include <netinet/in.h>
/* For socket functions */
#include <sys/socket.h>
/* For fcntl */
#include <fcntl.h>
/* for select */
#include <sys/select.h>

#include <assert.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <signal.h>

#include <ctime>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <set>
#include <map>

using namespace std;

#define MAX_LINE 16384

const char* camera_ips[2] = {"192.0.0.64", "192.0.0.65"};

class Task {
public:
	Task(string const& name, int goal, int score) : name_(name), goal_(goal), score_(score) {}
	string name_;
	int goal_;
	int score_;
};

class User {
public:
	User(string const& name) : name_(name) {}
	string name_;
	vector<Task> tasks_;
};

vector<User> users;
map<string, string> preloaded_images;

int systemCall(string const& cmd)
{
	cerr << "system call >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>" << endl;
	cerr << "system call: '" << cmd << "'" << endl;
	int status = system(cmd.c_str());
	cerr << "system call <<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<<" << endl;
	return status;
}

string epoch2timestring(unsigned long long i)
{
	time_t t(i);
	char mbstr[100];
	strftime(mbstr, sizeof(mbstr), "%Y_%m_%d_%H:%M:%S", localtime(&t));
	return string(mbstr);
}

unsigned long long timestring2epoch(string str)
{
	regex reTime("^([0-9]*)(_([0-9]*)(_([0-9]*)(_([0-9]*)(:([0-9]*)(:([0-9]*))?)?)?)?)?$");
	smatch sm;
	if (regex_match(str, sm, reTime)) {
		int year = sm.size() >= 1 ? atoi(sm[1].str().c_str()) : 1970;
		int month = sm.size() >= 3 ? atoi(sm[3].str().c_str()) : 1;
		int day = sm.size() >= 5 ? atoi(sm[5].str().c_str()) : 1;
		int hour = sm.size() >= 7 ? atoi(sm[7].str().c_str()) : 0;
		int minute = sm.size() >= 9 ? atoi(sm[9].str().c_str()) : 0;
		int second = sm.size() >= 11 ? atoi(sm[11].str().c_str()) : 0;
		struct tm t = {0};  // Initalize to all 0's
		t.tm_year = year - 1900;  // This is year-1900, so 112 = 2012
		t.tm_mon = month-1;
		t.tm_mday = day;
		t.tm_hour = hour;
		t.tm_min = minute;
		t.tm_sec = second;
		time_t timeSinceEpoch = mktime(&t);
		return timeSinceEpoch;
	} else
		return 0;
}

unsigned long long now()
{
	return std::time(nullptr);
}

string makeDate()
{
	time_t t = time(nullptr);
	char mbstr[100];
	strftime(mbstr, sizeof(mbstr), "%a, %d %b %Y %T %Z", localtime(&t));
	return string(mbstr);
}

string makeAuthPage()
{
	string html = "401 not authorized";
	ostringstream oss;
	oss << "HTTP/1.0 401 OK\n";
	oss << "Date: " << makeDate() << "\n";
	oss << "Content-Type: text/html\n";
	oss << "WWW-Authenticate: Basic realm=\"ImpgaardLommepenge\"\n";
	oss << "Content-Length: " << html.size() << "\n";
	oss << "\n";
	oss << html;
	return oss.str();
}

string makeHttp(string const& html)
{
	ostringstream oss;
	oss << "HTTP/1.0 200 OK\n";
	oss << "Date: " << makeDate() << "\n";
	oss << "Content-Type: text/html\n";
//	oss << "Connection: close\n";
	oss << "Content-Length: " << html.size() << "\n";
	//oss << "Transfer-Encoding: chunked\n";
	oss << "\n";
	oss << html;
	return oss.str();
}

string makePngHttp(string const& b)
{
	ostringstream oss;
	oss << "HTTP/1.0 200 OK\n";
	oss << "Date: " << makeDate() << "\n";
	oss << "Content-Type: image/png\n";
	oss << "Content-Length: " << b.size() << "\n";
	oss << "\n";
	oss << b;
	return oss.str();
}

string makeJpgHttp(string const& b)
{
	ostringstream oss;
	oss << "HTTP/1.0 200 OK\n";
	oss << "Date: " << makeDate() << "\n";
	oss << "Content-Type: image/jpeg\n";
	oss << "Content-Length: " << b.size() << "\n";
	oss << "\n";
	oss << b;
	return oss.str();
}

bool loadFile(string const& url, string& buffer)
{
	FILE* f = fopen(url.c_str(), "r");
	if (f) {
		fseek(f, 0, SEEK_END);
		int filesize = ftell(f);
		fseek(f, 0, SEEK_SET);
		buffer.resize(filesize);
		fread(&buffer[0], filesize, 1, f);
		fclose(f);
		return true;
	}
	return false;
}

void make_nonblocking(int fd)
{
	fcntl(fd, F_SETFL, O_NONBLOCK);
}

string mainDavidPage()
{
	ostringstream oss;
	oss << "<!DOCTYPE HTML>\n";
	oss << "<html lang=\"en-US\">\n";
	oss << "<head><title>Welcome to Rolfs server</title></head>\n";
	oss << "<body>\n";
	oss << "<h1>Welcome</h1>\n";
	oss << "<img src=\"live_snapshot.jpg\" title=\"snapshot\" />\n";
	oss << "<button type=\"button\">Click Me!</button>\n";
	oss << "<hr />\n";
	oss << "</body>\n";
	oss << "</html>\n";
	return oss.str();
}

string mainPage()
{
	ostringstream oss;
	oss << "<!DOCTYPE HTML>\n";
	oss << "<html lang=\"en-US\">\n";
	oss << "<head><title>Welcome to Rolfs webcam server</title></head>\n";
	oss << "<body>\n";
	oss << "<h1>Welcome</h1>\n";
	oss << "<a href=\"index?cam=0\">Camera 0</a>\n";
	oss << "<a href=\"index?cam=1\">Camera 1</a>\n";
	oss << "<a href=\"index?cam=2\">Camera 2</a>\n";
	oss << "<button type=\"button\">Click Me!</button>\n";
	oss << "<hr />\n";
	oss << "</body>\n";
	oss << "</html>\n";
	return oss.str();
}

string zoomRedirectPage(string n, string from, string to, string center, string zoom)
{
	unsigned long long fromi = timestring2epoch(from);
	unsigned long long toi = timestring2epoch(to);
	unsigned long long centeri = timestring2epoch(center);
	int izoom = atoi(zoom.c_str());
	unsigned long long zfromi = centeri - (toi-fromi)*izoom/200;
	unsigned long long ztoi = centeri + (toi-fromi)*izoom/200;
	string zfrom = epoch2timestring(zfromi);
	string zto = epoch2timestring(ztoi);
	ostringstream redir;
	redir << "history?cam=" << n << "&time=" << zfrom << "-" << zto << (izoom>100?"&zoomed_out=true":"");
	string redirect = redir.str();
	ostringstream oss;
	oss << "<!DOCTYPE HTML>\n";
	oss << "<html lang=\"en-US\">\n";
	oss << "    <head>\n";
	oss << "        <meta charset=\"UTF-8\">\n";
	oss << "        <meta http-equiv=\"refresh\" content=\"1;url=" << redirect << "\">\n";
	oss << "        <script type=\"text/javascript\">\n";
	oss << "            window.location.href = \"" << redirect << "\"\n";
	oss << "        </script>\n";
	oss << "        <title>Page Redirection</title>\n";
	oss << "    </head>\n";
	oss << "    <body>\n";
	oss << "        If you are not redirected automatically, follow the <a href='" << redirect << "'>link</a>\n";
	oss << "    </body>\n";
	oss << "</html>\n";
	return oss.str();
}

string cameraNIndexPage(int n)
{
	ostringstream oss;
	oss << "<!DOCTYPE HTML>\n";
	oss << "<html lang=\"en-US\">\n";
	oss << "<head><title>Welcome to Rolfs webcam server</title></head>\n";
	oss << "<body>\n";
	oss << "<h1>Camera " << n << "</h1>\n";
	string end = epoch2timestring(now());
	string beg = epoch2timestring(now()-12*60*60);
	oss << "<a href=\"history?cam=" << n << "&time=" << beg << "-" << end << "\">History</a>\n";
	oss << "<img src=\"live_snapshot_cam" << n << ".jpg\" title=\"snapshot\" />\n";
	oss << "<hr />\n";
	oss << "<p><em>godt - det virker!</em></p>\n";
	oss << "</body>\n";
	oss << "</html>\n";
	return oss.str();
}

string cameraNHistoryPage(string n, string from, string to, bool zoomed_out, bool secured, bool deleted)
{
	unsigned long long fromi = timestring2epoch(from);
	unsigned long long toi = timestring2epoch(to);
	ostringstream oss;
	oss << "<!DOCTYPE HTML>\n";
	oss << "<html lang=\"en-US\">\n";
	oss << "<head><title>Welcome to Rolfs webcam server</title></head>\n";
	oss << "<body>\n";
	oss << "<h1>Camera " << n << " activity</h1>\n";
	oss << "<img id=\"activityImg\" src=\"activity_cam" << n << "-" << from << "-" << to << ".png\" title=\"activity\"  usemap=\"#planetmap\"/>\n";
	oss << "Field2: <input type=\"text\" id=\"field2\"><br><br>\n";
	oss << "<button onclick=\"clickOnMap()\">Copy Text</button>\n";
	oss << "<script>\n";
	oss << "function clickOnMap(r) {\n";
	oss << "    document.getElementById(\"field2\").value = r;\n";
	oss << "    var radios = document.getElementsByName('clickaction');\n";
	oss << "    \n";
	oss << "    for (var i = 0, length = radios.length; i < length; i++) {\n";
	oss << "        if (radios[i].checked) {\n";
	oss << "            // do whatever you want with the checked radio\n";
	//oss << "            document.getElementById(\"field2\").value = radios[i].value;\n";
	oss << "            if (radios[i].value == 'snapshot') {\n";
	oss << "                document.getElementById(\"snapshotImg\").src=\"snapshot_cam" << n << "-\" + r + \".jpg\";\n";
	oss << "            } else if (radios[i].value == 'zoom_in') {\n";
	oss << "                window.location.href = \"zoom?cam=" << n << "&time=" << from << "-" << to << "&center=\" + r + \"&zoom=70\";\n";
	oss << "            } else if (radios[i].value == 'zoom_out') {\n";
	oss << "                window.location.href = \"zoom?cam=" << n << "&time=" << from << "-" << to << "&center=\" + r + \"&zoom=130\";\n";
	oss << "            } else if (radios[i].value == 'video') {\n";
	oss << "                window.location.href = \"video?cam=" << n << "&time=\" + r;\n";
	oss << "            } else if (radios[i].value == 'secure') {\n";
	oss << "                window.location.href = \"history?cam=" << n << "&time=" << from << "-" << to << "&secured=\" + r;\n";
	oss << "            } else if (radios[i].value == 'delete') {\n";
	oss << "                window.location.href = \"history?cam=" << n << "&time=" << from << "-" << to << "&deleted=\" + r;\n";
	oss << "            }\n";
	oss << "    \n";
	oss << "            // only one radio can be logically checked, don't check the rest\n";
	oss << "            break;\n";
	oss << "        }\n";
	oss << "    }\n";
	oss << "}\n";
	oss << "</script>\n";
	oss << "<map name=\"planetmap\">\n";
	int borderX0 = 66;
	int borderX1 = 26;
	int borderY0 = 20;
	int borderY1 = 20;
	int width = 1280;
	int height = 380;
	int steps = 640;
	for (int i = 0; i < steps; ++i) {
		int x0 = borderX0 + i*(width - borderX0 - borderX1)/steps;
		int x1 = borderX0 + (i+1)*(width - borderX0 - borderX1)/steps;
		int y0 = borderY0;
		int y1 = height - borderY1;
		string t = epoch2timestring(fromi + i*(toi-fromi)/steps);
		if (i == 9) {
			cerr << "XXX fromi = " << fromi << endl;
			cerr << "XXX toi = " << toi << endl;
			cerr << "XXX ipl = " << (fromi + i*(toi-fromi)/steps) << endl;
		}
		oss << "  <area onclick=\"clickOnMap('" << t << "')\" shape=\"rect\" coords=\"" << x0 << "," << y0 << "," << x1 << "," << y1 << "\" alt=\"Sun\">\n";
	}
	oss << "</map>\n";

	oss << "<form>\n";
	bool zoomed_in = !zoomed_out && !secured && !deleted;
	oss << "  <input type=\"radio\" name=\"clickaction\" value=\"zoom_in\" " << (zoomed_in?"checked":"") << "> Zoom In<br>\n";
	oss << "  <input type=\"radio\" name=\"clickaction\" value=\"zoom_out\" " << (zoomed_out?"checked":"") << "> Zoom Out<br>\n";
	oss << "  <input type=\"radio\" name=\"clickaction\" value=\"snapshot\"> Snapshot<br>\n";
	oss << "  <input type=\"radio\" name=\"clickaction\" value=\"video\"> Video<br>\n";
	oss << "  <input type=\"radio\" name=\"clickaction\" value=\"secure\" " << (secured?"checked":"") << "> Secure<br>\n";
	oss << "  <input type=\"radio\" name=\"clickaction\" value=\"delete\" " << (deleted?"checked":"") << "> Delete<br>\n";
	oss << "</form>\n";
	oss << "<img id=\"snapshotImg\" src=\"default_snapshot.png\" title=\"snapshot\"  />\n";

//	for (pair<string,int> vid: listVideos()) {
//		std::stringstream ss;
//		ss.imbue(std::locale("en_US.UTF-8"));
//		ss << std::fixed << vid.second;
//		oss << "<a href=\"videos/" << vid.first << "\" >" << vid.first << "</a> (" << ss.str() << " bytes)<br>\n";
//	}
	oss << "<hr />\n";
	oss << "</body>\n";
	oss << "</html>\n";
	return oss.str();
}

class Fd {
public:
	Fd(int fd) :
		fd(fd),
		wantRead(true),
		wantWrite(false)
	{
	}
	int fd;
	bool wantRead;
	bool wantWrite;
	string outputBuffer;
	string inputBuffer;
	string theGetLine;
	string theAuthLine;

	void webCam()
	{
		smatch sm;
		regex reCamNIndex(".*index\\?cam=([0-9]).*");
		regex reCamNZoom(".*zoom\\?cam=([0-9])&time=([0-9_:]*)-([0-9_:]*)&center=([0-9_:]*)&zoom=([0-9]*).*");
		regex reCamNHistoryZoomedOut(".*history\\?cam=([0-9])&time=([0-9_:]*)-([0-9_:]*)&zoomed_out=true.*");
		regex reCamNHistorySecured(".*history\\?cam=([0-9])&time=([0-9_:]*)-([0-9_:]*)&secured=([0-9_:]*)?.*");
		regex reCamNHistoryDeleted(".*history\\?cam=([0-9])&time=([0-9_:]*)-([0-9_:]*)&deleted=([0-9_:]*)?.*");
		regex reCamNHistory(".*history\\?cam=([0-9])&time=([0-9_:]*)-([0-9_:]*).*");
		regex reVideo(".*video\\?cam=([0-9])&time=([0-9_:]*).*");
		regex reCamNActivity(".*activity_cam([0-9])-([0-9_:]*)-([0-9_:]*).png.*");
		regex reSnapshot(".*snapshot_cam([0-9])-([0-9_:]*).jpg.*");
		regex reLiveSnapshot(".*live_snapshot_cam([0-9]).jpg.*");


		if (std::regex_match(theGetLine, sm, reCamNActivity)) {
			cerr << ">>>>> sending activity.png" << endl;
			string n = sm[1].str();
			string from = sm[2].str();
			string to = sm[3].str();
			ostringstream oss;
			oss << "./plot_activity.py -T plot -c secured/cam" << n << " -r recordings/cam" << n << "/ -t " << from << "-" << to << " -o activity2.png";
			int status = systemCall(oss.str());
			string b;
			if (loadFile("activity2.png", b)) {
				outputBuffer = makePngHttp(b);
				wantWrite = true;
			}
		} else if (regex_match(theGetLine, sm, reSnapshot)) {
			cerr << ">>>>> sending snapshot" << endl;
			string n = sm[1].str();
			string time = sm[2].str();
			ostringstream oss;
			oss << "./plot_activity.py -T snapshot -c secured/cam" << n << " -r recordings/cam" << n << "/ -t " << time << " -o /tmp/snapshot.jpg";
			int status = systemCall(oss.str());
			string b;
			if (loadFile("/tmp/snapshot.jpg", b)) {
				outputBuffer = makePngHttp(b);
				wantWrite = true;
			}
		} else if (regex_match(theGetLine, sm, reLiveSnapshot)) {
			int n = atoi(sm[1].str().c_str());
			cerr << ">>>>> sending live snapshot from cam" << n << endl;
			string cmd = string("curl http://admin:12345@") + string(camera_ips[n-1]) + "/Streaming/channels/1/picture -o /tmp/live_snapshot.jpg";
			int status = systemCall(cmd);
			string b;
			if (loadFile("/tmp/live_snapshot.jpg", b)) {
				outputBuffer = makeJpgHttp(b);
				wantWrite = true;
			}
		} else if (std::regex_match(theGetLine, sm, reCamNIndex)) {
			cerr << ">>>>> camNIndex" << endl;
			cerr << "camN " << sm[1] << endl;
			outputBuffer = makeHttp(cameraNIndexPage(atoi(sm[1].str().c_str())));
			wantWrite = true;
		} else if (regex_match(theGetLine, sm, reCamNZoom)) {
			string n = sm[1].str();
			string from = sm[2].str();
			string to = sm[3].str();
			string center = sm[4].str();
			string zoom = sm[5].str();
			outputBuffer = makeHttp(zoomRedirectPage(n, from, to, center, zoom));
			wantWrite = true;
		} else if (std::regex_match(theGetLine, sm, reCamNHistoryZoomedOut)) {
			cerr << ">>>>> camNHistoryZoomedOut" << endl;
			string n = sm[1].str();
			string from = sm[2].str();
			string to = sm[3].str();
			outputBuffer = makeHttp(cameraNHistoryPage(n, from, to, true, false, false));
			wantWrite = true;
		} else if (std::regex_match(theGetLine, sm, reCamNHistorySecured)) {
			cerr << ">>>>> camNHistorySecured" << endl;
			string n = sm[1].str();
			string from = sm[2].str();
			string to = sm[3].str();
			string time = sm[4].str();
			ostringstream oss;
			oss << "./plot_activity.py -T secure -c secured/cam" << n << " -r recordings/cam" << n << "/ -t " << time;
			int status = systemCall(oss.str());
			outputBuffer = makeHttp(cameraNHistoryPage(n, from, to, false, true, false));
			wantWrite = true;
		} else if (std::regex_match(theGetLine, sm, reCamNHistoryDeleted)) {
			cerr << ">>>>> camNHistoryDeleted" << endl;
			string n = sm[1].str();
			string from = sm[2].str();
			string to = sm[3].str();
			string time = sm[4].str();
			ostringstream oss;
			oss << "./plot_activity.py -T delete --force -r recordings/cam" << n << "/ -t " << time;
			int status = systemCall(oss.str());
			outputBuffer = makeHttp(cameraNHistoryPage(n, from, to, false, false, true));
			wantWrite = true;
		} else if (std::regex_match(theGetLine, sm, reCamNHistory)) {
			cerr << ">>>>> camNHistory" << endl;
			string n = sm[1].str();
			string from = sm[2].str();
			string to = sm[3].str();
			outputBuffer = makeHttp(cameraNHistoryPage(n, from, to, false, false, false));
			wantWrite = true;
		} else {
			cerr << ">>>>> sending mainPage" << endl;
			outputBuffer = makeHttp(mainPage());
			wantWrite = true;
		}
	}

	string makePage(string const& user)
	{
		ostringstream oss;
		oss << "<!DOCTYPE HTML>\n";
		oss << "<html lang=\"en-US\">\n";
		oss << "<head><title>Welcome to Rolfs server</title></head>\n";
		oss << "<body>\n";
		oss << "<h1>Welcome " << user << "</h1>\n";
		for (int u = 0; u < users.size(); ++u) {
			oss << "<h2>" << users[u].name_ << ":</h2>\n";
			oss << "<table style=\"width:200\">\n";
			for (int t = 0; t < users[u].tasks_.size(); ++t) {
				oss << "  <tr>\n";
				oss << "    <td>" << users[u].tasks_[t].name_ << "</td>\n";
				oss << "    <td>\n";
				for (int cb = 0; cb < max(users[u].tasks_[t].goal_, users[u].tasks_[t].score_); ++cb) {
					if (cb < users[u].tasks_[t].goal_ && cb < users[u].tasks_[t].score_)
						oss << "      <img id=\"snapshotImg\" src=\"box_cross.png\" title=\"cross\" />\n";
					else if (cb < users[u].tasks_[t].goal_)
						oss << "      <img id=\"snapshotImg\" src=\"box.png\" title=\"cross\" />\n";
					else if (cb < users[u].tasks_[t].score_)
						oss << "      <img id=\"snapshotImg\" src=\"cross.png\" title=\"cross\" />\n";
				}
				oss << "    </td>\n";
				oss << "  </tr>\n";
			}
			oss << "</table>\n";
		}
		oss << "<br>\n" << endl;
		oss << "<button type=\"button\">Click Me!</button>\n";
		oss << "<hr />\n";
		oss << "</body>\n";
		oss << "</html>\n";
		return makeHttp(oss.str());
	}

	void replyPage(string const& user)
	{
		cerr << "theGetLine: '" << theGetLine << "'" << endl;
		regex rePng(".*/(.*\\.png).*");
		smatch sm;

		cerr << "GET!! fd = " << fd << endl;
		if (std::regex_match(theGetLine, sm, rePng)) {
			string imageName = sm[1].str();
			if (preloaded_images.count(imageName)) {
				outputBuffer = preloaded_images[imageName];
				wantWrite = true;
			} else {
				string b;
				if (loadFile(sm[1].str(), b)) {
					outputBuffer = makePngHttp(b);
					wantWrite = true;
				}
			}
		} else {
			outputBuffer = makePage(user);
			wantWrite = true;
		}
	}
	void david()
	{
		smatch sm;
		regex reDavidSnapshot(".*.jpg.*");

		if (std::regex_match(theGetLine, sm, reDavidSnapshot)) {
			string b;
			if (loadFile("hej_david.jpg", b)) {
				outputBuffer = makePngHttp(b);
				wantWrite = true;
			}
		} else {
			cerr << ">>>>> sending mainPage" << endl;
			outputBuffer = makeHttp(mainDavidPage());
			wantWrite = true;
		}
	}

	void parseLine(string const& line)
	{
		regex reEnd("\\s*");
		regex reAuth("^Authorization: Basic (.*)");
		smatch sm;
		if (line.find("GET") == 0) {
			theGetLine = line;
		} else if (regex_match(line, sm, reAuth)) {
			theAuthLine = line;
		} else if (regex_match(line, sm, reEnd)) {
			cerr << "######## theAuthLine = '" << theAuthLine << "'" << endl;
			if (theGetLine.size()) {
				cerr << "######## theGetLine = '" << theGetLine << "'" << endl;
				if (theAuthLine.find("QWxhZGRpbjpPcGVuU2VzYW1l") != string::npos) {
					replyPage("aladin");
					//webCam();
				} else if (theAuthLine.find("MTIzNDoxMjM0") != string::npos) { // 1234, 1234
					replyPage("1234");
					//david();
				} else {
					//replyPage("");
					//cerr << ">>>>> sending authPage" << endl;
					outputBuffer = makeAuthPage();
					wantWrite = true;
				}
				theGetLine = "";
			}
			wantRead = false;
		} else {
		}
		cerr << "LINE: '" << line << "'" << endl;
	}

	void parseChunk(string const& str)
	{
		inputBuffer += str;
		while (1) {
			size_t pos = inputBuffer.find('\n');
			if (pos != string::npos) {
				string line = inputBuffer.substr(0, pos-1);
				parseLine(line);
				inputBuffer = inputBuffer.substr(pos+1);
			} else
				break;
		}

	}

	int do_read()
	{
		char buf[256];
		int i;
		ssize_t result;
		result = recv(fd, buf, sizeof(buf), 0);
		if (result > 0) {
			parseChunk(string(buf, result));
		}

		if (result == 0) {
			wantRead = false;
			wantWrite = false;
		} else if (result < 0) {
			cerr << "READ errno = " << errno << endl;
			if (errno != EAGAIN) {
				wantRead = false;
				wantWrite = false;
			}
		}

		return 0;
	}

	int do_write()
	{
		while (outputBuffer.size()) {
			ssize_t result = send(fd, &outputBuffer[0], (int)outputBuffer.size(), 0);
			if (result < 0) {
				cerr << "SEND errno = " << errno << endl;
				if (errno == EAGAIN)
					return 0;
				wantWrite = false;
				return -1;
			} else {
				outputBuffer = outputBuffer.substr(result);
				if (outputBuffer.size() == 0) {
					wantWrite = false;
				}
				return 0;
			}
		}


		return 0;
	}
};

void run(unsigned port)
{
	signal(SIGPIPE, SIG_IGN);
	int listener;
	Fd* fds[FD_SETSIZE];
	struct sockaddr_in sin;
	int i, maxfd;
	fd_set readset, writeset, exset;

	sin.sin_family = AF_INET;
	sin.sin_addr.s_addr = 0;
	sin.sin_port = htons(port);

	for (i = 0; i < FD_SETSIZE; ++i)
		fds[i] = nullptr;

	listener = socket(AF_INET, SOCK_STREAM, 0);
	make_nonblocking(listener);


	if (bind(listener, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
		perror("bind");
		return;
	}

	if (listen(listener, 16)<0) {
		perror("listen");
		return;
	}

	FD_ZERO(&readset);
	FD_ZERO(&writeset);
	FD_ZERO(&exset);

	while (1) {
		maxfd = listener;

		FD_ZERO(&readset);
		FD_ZERO(&writeset);
		FD_ZERO(&exset);

		FD_SET(listener, &readset);

		for (i=0; i < FD_SETSIZE; ++i) {
			if (fds[i]) {
				if (i > maxfd)
					maxfd = i;
				if (fds[i]->wantRead) {
					FD_SET(i, &readset);
				}
				if (fds[i]->wantWrite) {
					FD_SET(i, &writeset);
				}
			}
		}

		if (select(maxfd+1, &readset, &writeset, &exset, nullptr) < 0) {
			cerr << "perror select" << endl;
			perror("select");
			return;
		}
//		cerr << "leaving select" << endl;

		if (FD_ISSET(listener, &readset)) {
			cerr << "new connection" << endl;
			struct sockaddr_storage ss;
			socklen_t slen = sizeof(ss);
			int fd = accept(listener, (struct sockaddr*)&ss, &slen);
			if (fd < 0) {
				cerr << "perror accept" << endl;
				perror("accept");
			} else if (fd > FD_SETSIZE) {
				cerr << "illegal connection - closing..." << endl;
				close(fd);
			} else {
				make_nonblocking(fd);
				fds[fd] = new Fd(fd);
			}
		}

		bool someWantRead = false;
		for (i=0; i < maxfd+1+3; ++i) {
			if (fds[i] != nullptr) {
				if (fds[i]->wantRead)
					someWantRead = true;
//				cerr << "status for " << i << "(" << (void*)fds[i] << "): " << fds[i]->wantRead << ", " << fds[i]->wantWrite << endl;
			}
		}

		for (i=0; i < maxfd+1; ++i) {
			if (i == listener || fds[i] == nullptr)
				continue;

			if (FD_ISSET(i, &readset)) {
//				cerr << "readset fd = " << i << endl;
				fds[i]->do_read();
			}
			if (!someWantRead && fds[i]->wantWrite && FD_ISSET(i, &writeset)) {
//				cerr << "writeset fd = " << i << endl;
				fds[i]->do_write();
			}
			if (!fds[i]->wantWrite && !fds[i]->wantRead) {
				delete fds[i];
				fds[i] = nullptr;
				close(i);
			}
		}
	}
}

int main(int argc, char *argv[])
{
	users.push_back(User("Cecilia"));
	users.back().tasks_.push_back(Task("T&oslash;mme opvask", 12, 3));
	users.back().tasks_.push_back(Task("mad-dag", 1, 0));
	users.push_back(User("Helena"));
	users.back().tasks_.push_back(Task("rydde af bordet", 10, 5));
	users.back().tasks_.push_back(Task("mad-dag", 1, 0));
	users.push_back(User("Adam"));
	users.back().tasks_.push_back(Task("affald", 12, 3));
	users.back().tasks_.push_back(Task("mad-dag", 1, 1));
	users.push_back(User("Samuel"));
	users.back().tasks_.push_back(Task("d&aelig;kke bord", 12, 3));
	users.back().tasks_.push_back(Task("mad-dag", 1, 0));
	users.push_back(User("Karen"));
	users.back().tasks_.push_back(Task("mad-dag", 1, 3));
	users.push_back(User("Rolf"));
	users.back().tasks_.push_back(Task("mad-dag", 1, 0));

	string b;
	if (loadFile("cross.png", b)) {
		preloaded_images["cross.png"] = b;
	}
	if (loadFile("box.png", b)) {
		preloaded_images["box.png"] = b;
	}

	time_t t = time(nullptr);
	if (argc != 2) {
		cerr << "usage: lommepenge [port]" << endl;
		exit(1);
	}

	unsigned port = atoi(argv[1]);

	run(port);
	return 0;
}
