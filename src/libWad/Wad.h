#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
using namespace std;

struct Node
{
public:
    unordered_map<string, Node *> elements;

    int length; // Length (in bytes)
    int offset; // Offset in data[]
    int order;  // Original WAD order
    bool isDirectory;

    // Constructor
    Node(int length = 0, int offset = 0, int order = 0, bool isDirectory = true);
};

class Wad
{
private:
    Node *root = new Node(0, 0, 0, true);
    uint8_t *data = nullptr;

    string magic = "";
    int numDesc = 0;
    int descOffset = 0;

    // Helper Functions
    void addNode(string path, int length, int offset, bool isDirectory);
    vector<string> getPath(string path);
    Node *getNode(string path);

public:
    static Wad *loadWad(const string &path);

    // Getters
    string getMagic();
    int getSize(const string &path);
    int getContents(const string &path, char *buffer, int length, int offset = 0);
    int getDirectory(const string &path, vector<string> *directory);

    // Verifiers
    bool isContent(const string &path);
    bool isDirectory(const string &path);

    // Destructor
    ~Wad();
};