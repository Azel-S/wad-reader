#include "Wad.h"
#include <fstream>
#include <sstream>
#include <map>
#include <queue>
using namespace std;

Node::Node(int length, int offset, int order, bool isDirectory)
{
    this->length = length;
    this->offset = offset;
    this->order = order;
    this->isDirectory = isDirectory;
}

void Wad::addNode(string path, int length, int offset, bool isDirectory)
{
    if (!path.empty())
    {
        Node *node;
        string name;

        size_t lastPos = path.substr(1).rfind('/');
        if (lastPos != string::npos)
        {
            lastPos += 1; // Append 1 as rfind() was used on a substr

            name = path.substr(lastPos + 1);
            node = getNode(path.substr(0, lastPos));
        }
        else
        {
            node = root;
            name = path.substr(1);
        }

        node->children[name] = new Node(length, offset, node->children.size(), isDirectory);
    }
}

vector<string> Wad::getPath(string path)
{
    vector<string> result;

    stringstream ss(path);
    string line;

    while (getline(ss, line, '/'))
    {
        result.push_back(line);
    }

    return result;
}

Node *Wad::getNode(string path)
{
    if (!path.empty())
    {
        Node *currNode = root;
        auto temp = getPath(path.substr(1));

        if (!temp.empty())
        {
            for (int i = 0; i < temp.size() - 1; i++)
            {
                auto directoryIter = currNode->children.find(temp[i]);

                if (directoryIter != currNode->children.end() && directoryIter->second->isDirectory)
                {
                    currNode = directoryIter->second;
                }
                else
                {
                    // Invalid path
                    return nullptr;
                }
            }

            if (currNode->children.find(temp[temp.size() - 1]) != currNode->children.end())
            {
                return currNode->children[temp[temp.size() - 1]];
            }
        }
        else if (path == "/")
        {
            return root;
        }
    }

    return nullptr;
}

Wad::~Wad()
{
    queue<Node *> nodes;
    nodes.push(root);

    while (!nodes.empty())
    {
        auto node = nodes.front();
        nodes.pop();

        for (auto i = node->children.begin(); i != node->children.end(); i++)
        {
            nodes.push(i->second);
        }

        delete node;
    }

    delete[] data;
}

Wad *Wad::loadWad(const string &path)
{
    Wad *wad = new Wad();

    ifstream file(path, ios::binary | ios::ate); // Open WAD file at the end.
    streampos size = file.tellg();               // Get the size of the file

    // Put the file's data into a byte array.
    wad->data = new uint8_t[size];
    file.seekg(0, ios::beg);
    file.read((char *)wad->data, size);
    int currByte = 0; // Tracks current byte;

    // Header (file type)
    for (int i = 0; i < 4; i++)
    {
        wad->magic += wad->data[currByte++];
    }
    wad->magic = wad->magic.c_str(); // Remove extra null-terminating characters.

    // Header (number of descriptors)
    for (int i = 0; i < 4; i++)
    {
        wad->numDesc += (wad->data[currByte++] << (8 * i));
    }

    // Header (offset of descriptors)
    for (int i = 0; i < 4; i++)
    {
        wad->descOffset += (wad->data[currByte++] << (8 * i));
    }

    currByte = wad->descOffset;

    string currPath = "";
    int EM = 0;

    for (int i = 0; i < wad->numDesc; i++)
    {
        int offset = 0;
        for (int i = 0; i < 4; i++)
        {
            offset += (wad->data[currByte++] << (8 * i));
        }

        int length = 0;
        for (int i = 0; i < 4; i++)
        {
            length += (wad->data[currByte++] << (8 * i));
        }

        string name = "";
        for (int i = 0; i < 8; i++)
        {
            name += wad->data[currByte++];
        }
        name = name.c_str(); // Remove extra null-terminating characters.

        // File
        if (length > 0)
        {
            wad->addNode(currPath + "/" + name, length, offset, false);

            if (EM > 0)
            {
                EM--;

                if (EM == 0)
                {
                    currPath = currPath.substr(0, currPath.size() - 4 - 1);
                }
            }
        }
        // Folder
        else if (length == 0)
        {
            // _START Indicator
            if (name.size() > 6 && name.substr(name.size() - 6, 6).find("_START") != string::npos)
            {
                currPath += "/" + name.substr(0, name.size() - 6);
                wad->addNode(currPath, length, offset, true);
            }
            // _END Indicator
            else if (name.size() > 4 && name.substr(name.size() - 4, 4).find("_END") != string::npos)
            {
                currPath = currPath.substr(0, currPath.size() - (name.size() - 4) - 1);
            }
            // E#M# Indicator
            else if (name.size() == 4 && isdigit(name[1]) && isdigit(name[3]))
            {
                currPath += "/" + name;
                wad->addNode(currPath, length, offset, true);
                EM = 10;
            }
            else
            {
                cout << "Invalid directory descriptor" << endl;
            }
        }
        else
        {
            cout << "Invalid size descriptor, negative value" << endl;
        }
    }

    return wad;

    return nullptr;
}

string Wad::getMagic()
{
    return this->magic;
}

int Wad::getSize(const string &path)
{
    auto node = getNode(path);

    if (node && !node->isDirectory)
    {
        return node->length;
    }

    return -1;
}

int Wad::getContents(const string &path, char *buffer, int length, int offset)
{
    auto node = getNode(path);

    if (node && !node->isDirectory)
    {
        if (length > node->length || offset + length > node->length)
        {
            cout << "Given length was too big, incorrect node?" << endl;
            length = node->length;
        }

        for (int i = 0; i < length; i++)
        {
            buffer[i] = data[offset + i];
        }

        return length;
    }
    else
    {
        return -1;
    }
}

int Wad::getDirectory(const string &path, vector<string> *directory)
{
    auto node = getNode(path);

    if (node && node->isDirectory)
    {
        map<int, string> ordered;

        for (auto i = node->children.begin(); i != node->children.end(); i++)
        {
            ordered[i->second->order] = i->first;
        }

        for (auto i = ordered.begin(); i != ordered.end(); i++)
        {
            directory->push_back(i->second);
        }

        return ordered.size();
    }
    else
    {
        return -1;
    }
}

bool Wad::isContent(const string &path)
{
    auto node = getNode(path);
    return (node && !node->isDirectory) ? true : false;
}

bool Wad::isDirectory(const string &path)
{
    auto node = getNode(path);
    return (node && node->isDirectory) ? true : false;
}
