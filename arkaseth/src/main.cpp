#include <iostream>
#include <utility>
#include <fstream>
#include <string>
#include <vector>
#include <cstring>
#include <sys/types.h>
#include <sys/stat.h>
#include <experimental/filesystem>
#include <openssl/sha.h>

using namespace std;
namespace fs = experimental::filesystem;

string root = "";

int ignoreIt(string, string);
int toIgnore(string, string);
void add(string, string);
void init(string);
void copyFile(string, string, string);

void commit(string);
void addCommit(string, char, string);
void updateCommitLog(string, string);
void repeatCommit(string, char, string);
void getCommitLog();
string getCommitHash();

// 1. Init
void init(string path)
{
	struct stat info;

	if (stat((path + "/.imperium").c_str(), &info) == 0)
	{
		cout << "Repo is already initialized!\n";
	}
	else
	{
		string ignore = path + "/.imperiumignore";
		ofstream ignorePath(ignore.c_str());
		ignorePath << ".imperiumignore\n.gitignore\nnode_modules\n.git\n.env";
		ignorePath.close();

		path += "/.imperium";
		int createFlag = mkdir(path.c_str(), 0777);
		if (createFlag == 0)
		{
			string commitLog = path + "/commit.log";
			ofstream commit(commitLog.c_str());
			commit.close();

			string addLog = path + "/add.log";
			ofstream add(addLog.c_str());
			add.close();

			string conflictLog = path + "/conflict.log";
			ofstream conflict(conflictLog.c_str());
			conflict.close();

			cout << "Initialised the repo!\n";
		}
		else
		{
			cout << "Error!\n";
		}
	}
}

// 2. Add
void add(string path, string subPath = "")
{
	struct stat info, buf;

	if (stat((path + "/.imperium/.add").c_str(), &buf) != 0)
	{
		string addFolder = path + "/.imperium/.add";
		mkdir(addFolder.c_str(), 0777);
	}

	string addPath;
	string copyTo = path + "/.imperium/.add";
	string addLog = path + "/.imperium/add.log";
	if (strcmp(subPath.c_str(), ".") == 0)
	{
		addPath = path;
	}
	else
	{
		addPath = path + "/" + subPath;
	}
	if (stat(addPath.c_str(), &info) != 0)
	{
		cout << "Check the path you have entered! This path does not exist!\n";
	}
	else
	{
		// If it's a file
		if (info.st_mode & S_IFREG)
		{
			// If in .imperiumignore
			if (toIgnore(path, addPath) == 2)
			{
				cout << "File in .imperiumignore!" << endl;
			}
			// If neither in add.log nor .imperiumignore
			else if (toIgnore(path, addPath) == 0)
			{
				ofstream outFile;
				outFile.open(addLog.c_str(), ios_base::app);
				outFile << addPath << endl;
				outFile.close();
				copyFile(path, addPath, copyTo);
			}
		}
		// If it's a directory
		else if (info.st_mode & S_IFDIR)
		{
			struct stat buf;
			for (auto i = fs::recursive_directory_iterator(addPath); i != fs::recursive_directory_iterator(); ++i)
			{
				if ((stat(i->path().string().c_str(), &buf) == 0) && (toIgnore(path, i->path().string()) == 2))
				{
					i.disable_recursion_pending();
				}
				else if ((stat(i->path().string().c_str(), &buf) == 0) && (toIgnore(path, i->path().string()) == 0))
				{
					if ((stat(i->path().string().c_str(), &buf) == 0) && buf.st_mode & S_IFREG)
					{
						if (toIgnore(path, i->path().string()) == 0)
						{
							ofstream outFile;
							outFile.open(addLog.c_str(), ios_base::app);
							outFile << i->path().string() << endl;
							outFile.close();
							copyFile(path, i->path().string(), copyTo);
						}
					}
				}
			}
		}
	}
}

// path to ignore
int toIgnore(string path, string paramPath)
{
	string ignorePath = path + "/.imperiumignore";
	string addLog = path + "/.imperium/add.log";
	string line;
	int offset;

	string paramInIgnore = paramPath;
	string::size_type i = paramInIgnore.find(path);
	if (i != string::npos)
		paramInIgnore.erase(i, path.length());
	// If path is in .imperiumignore
	if (ignoreIt(ignorePath, paramInIgnore))
	{
		return 2;
	}
	// Check if already in add.log
	ifstream ifs(addLog.c_str());
	if (ifs.is_open())
	{
		while (!ifs.eof())
		{
			getline(ifs, line);
			if ((offset = line.find(paramPath, 0)) != string::npos)
			{
				return 1;
			}
		}
	}
	// If in neither of the two files
	return 0;
}

// Is it in .imperiumignore?
int ignoreIt(string ignorePath, string paramInIgnore)
{
	string paramInIgnoreCopy = paramInIgnore;
	vector<string> paramsToSearch;
	char *paramInIgnoreC = &paramInIgnore[0];

	char *p = strtok(paramInIgnoreC, "/");
	while (p)
	{
		paramsToSearch.push_back(p);
		p = strtok(NULL, "/");
	}

	string line;
	int offset;

	ifstream ignore(ignorePath.c_str());
	if (ignore.is_open())
	{
		while (!ignore.eof())
		{
			getline(ignore, line);
			for (int i = 0; i < paramsToSearch.size(); i++)
			{
				if ((offset = line.find(paramsToSearch[i], 0)) != string::npos)
				{
					return 1;
				}
			}
		}
	}
	return 0;
}

void copyFile(string path, string filePath, string addFolder)
{
	string filePathCopy = filePath;
	string::size_type i = filePathCopy.find(path);
	if (i != string::npos)
		filePathCopy.erase(i, path.length());
	// addFolder += filePathCopy;
	// cout << addFolder << endl
	// 	 << filePath << endl
	// 	 << filePathCopy << endl;

	vector<string> tokenizedFolders;
	char *filePathCopyC = &filePathCopy[0];

	char *p = strtok(filePathCopyC, "/");
	while (p)
	{
		tokenizedFolders.push_back(p);
		p = strtok(NULL, "/");
	}

	string addFolderCopy = addFolder;

	for (int i = 0; i < tokenizedFolders.size() - 1; i++)
	{
		mkdir((addFolderCopy + "/" + tokenizedFolders[i]).c_str(), 0777);
		addFolderCopy += "/" + tokenizedFolders[i];
	}
	addFolderCopy += "/" + tokenizedFolders[tokenizedFolders.size() - 1];
	const auto copyOptions = fs::copy_options::overwrite_existing;
	fs::copy(filePath, addFolderCopy, copyOptions);
}

// 3. Commit
void commit(string message)
{
	struct stat buf;
	if (stat((root + "/.imperium").c_str(), &buf) != 0)
	{
		cout << "Repo not initialized!\n";
	}

	string commitHash = getCommitHash();

	// Copy all files from prev commit

	if (stat((root + "/.imperium/head.log").c_str(), &buf) == 0)
	{
		string headHash;
		ifstream readCommitLog;

		readCommitLog.open(root + "/.imperium/head.log");

		getline(readCommitLog, headHash);
		headHash = headHash.substr(0, headHash.find(" -- "));

		for (auto &p : fs::recursive_directory_iterator(root + "/.imperium/.commit/" + headHash))
		{
			if (stat(p.path().c_str(), &buf) == 0)
			{
				if (buf.st_mode & S_IFREG)
				{
					repeatCommit(p.path(), 'f', commitHash);
				}
				else if (buf.st_mode & S_IFDIR)
				{
					repeatCommit(p.path(), 'd', commitHash);
				}
			}
		}
	}

	// Copy all the files from staging area
	for (auto &p : fs::recursive_directory_iterator(root + "/.imperium/.add"))
	{
		struct stat s;
		if (stat(p.path().string().c_str(), &s) == 0)
		{
			if (s.st_mode & S_IFREG)
			{
				addCommit(p.path(), 'f', commitHash);
			}
			else if (s.st_mode & S_IFDIR)
			{
				addCommit(p.path(), 'd', commitHash);
			}
		}
	}
	fs::remove_all((root + "/.imperium/.add").c_str());
	remove((root + "/.imperium/add.log").c_str());
	updateCommitLog(commitHash, message);
}

void addCommit(string absPath, char type, string commitHash)
{
	struct stat s;
	if (stat((root + "/.imperium/.commit").c_str(), &s) != 0)
	{
		mkdir((root + "/.imperium/.commit").c_str(), 0777);
	}

	if (stat((root + "/.imperium/.commit/" + commitHash).c_str(), &s) != 0)
	{
		mkdir((root + "/.imperium/.commit/" + commitHash).c_str(), 0777);
	}

	string relPath = absPath.substr(root.length() + 15);

	string filePath = root + "/.imperium/.commit/" + commitHash + relPath.substr(0, relPath.find_last_of("/"));

	fs::create_directories(filePath);

	if (type == 'f')
	{
		fs::copy_file(absPath, root + "/.imperium/.commit/" + commitHash + relPath, fs::copy_options::overwrite_existing);
	}
}

void updateCommitLog(string commitHash, string message)
{
	ofstream writeHeadLog;

	writeHeadLog.open(root + "/.imperium/head.log");
	writeHeadLog << commitHash << " -- " << message << endl;
	writeHeadLog.close();

	ofstream writeCommitLog;
	ifstream readCommitLog;

	readCommitLog.open(root + "/.imperium/commit.log");
	writeCommitLog.open(root + "/.imperium/new_commit.log");

	writeCommitLog << commitHash << " -- " << message << " --HEAD\n";

	string line;
	while (getline(readCommitLog, line))
	{
		if (line.find(" --HEAD") != string::npos)
		{
			writeCommitLog << line.substr(0, line.length() - 8) << "\n";
		}
		else
		{
			writeCommitLog << line << "\n";
		}
	}

	remove((root + "/.imperium/commit.log").c_str());
	rename((root + "/.imperium/new_commit.log").c_str(), (root + "/.imperium/commit.log").c_str());

	readCommitLog.close();
	writeCommitLog.close();
}

string getTime()
{
	auto end = chrono::system_clock::now();
	time_t end_time = chrono::system_clock::to_time_t(end);
	string time = ctime(&end_time);

	return time;
}

string getCommitHash()
{
	string commitFileName = getTime();
	string commitHash;

	char buf[5];
	int length = commitFileName.length();
	unsigned char hash[20];
	unsigned char *val = new unsigned char[length + 1];
	strcpy((char *)val, commitFileName.c_str());

	SHA1(val, length, hash);
	for (int i = 0; i < 20; i++)
	{
		sprintf(buf, "%02x", hash[i]);
		commitHash += buf;
	}
	return commitHash;
}

void getCommitLog()
{
	string commitLogPath = root + "/.imperium/commit.log";
	string commitLine;

	ifstream commitLog;

	commitLog.open(commitLogPath);
	while (getline(commitLog, commitLine))
	{
		cout << commitLine << endl;
	}
}

void repeatCommit(string absPath, char type, string commitHash)
{
	mkdir((root + "/.imperium/.commit/" + commitHash).c_str(), 0777);

	string relPath = absPath.substr(root.length() + 19 + commitHash.length());
	string filePath = root + "/.imperium/.commit/" + commitHash + relPath.substr(0, relPath.find_last_of("/"));

	fs::create_directories(filePath);

	if (type == 'f')
	{
		fs::copy_file(absPath, root + "/.imperium/.commit/" + commitHash + relPath, fs::copy_options::overwrite_existing);
	}
}

int main(int argc, char *argv[])
{
	const string dir = getenv("dir");
	root = dir;
	if (argc == 1)
	{
		cout << "Hey!\n";
	}
	else if (strcmp(argv[1], "init") == 0)
	{
		init(root);
	}
	else if (strcmp(argv[1], "add") == 0)
	{
		add(root, argv[2]);
	}
	else if (strcmp(argv[1], "commit") == 0)
	{
		commit(argv[2]);
	}
	else if (strcmp(argv[1], "log") == 0)
	{
		getCommitLog();
	}
}