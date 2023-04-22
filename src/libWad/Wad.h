#include <iostream>
#include <string>
#include <vector>
using namespace std;

struct Descriptor
{
public:
    uint32_t eOffset;
    uint32_t length;
    string name;
    string filePath;
};

class Wad
{
private:
public:
    static Wad *loadWad(const string &path);
    string getMagic();
    bool isContent(const string &path);
    bool isDirectory(const string &path);
    int getSize(const string &path);
    int getContents(const string &path, char *buffer, int length, int offset = 0);
    int getDirectory(const string &path, vector<string> *directory);
};