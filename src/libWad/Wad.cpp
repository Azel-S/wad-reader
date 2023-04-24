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

// Adds a node if path is valid.
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

            if (!node)
            {
                cout << "WARNING: Invalid path found, ignoring..." << endl;
                return;
            }
        }
        else
        {
            node = root;
            name = path.substr(1);
        }

        // Only add if non-duplicate
        if (node->elements.find(name) == node->elements.end())
        {
            node->elements[name] = new Node(length, offset, node->elements.size(), isDirectory);
        }
    }
}

// Seperates a path string into a vector.
vector<string> Wad::getPath(string path)
{
    vector<string> result;
    stringstream ss;
    string line;

    // Remove trailing '/' to avoid parsing issues.
    if (path[path.size() - 1] == '/')
    {
        ss.str(path.substr(0, path.size() - 1));
    }
    else
    {
        ss.str(path);
    }

    while (getline(ss, line, '/'))
    {
        result.push_back(line);
    }

    return result;
}

// Returns node if path is valid, nullptr otherwise
Node *Wad::getNode(string path)
{
    if (!path.empty())
    {
        Node *currNode = root;
        auto vPath = getPath(path.substr(1));

        if (!vPath.empty())
        {
            // Go up to the last folder in path
            for (int i = 0; i < vPath.size() - 1; i++)
            {
                auto directoryIter = currNode->elements.find(vPath[i]);

                if (directoryIter != currNode->elements.end() && directoryIter->second->isDirectory)
                {
                    currNode = directoryIter->second;
                }
                else
                {
                    cout << "WARNING: Invalid path found, returning nullptr..." << endl;
                    return nullptr;
                }
            }

            // Return last file/folder
            if (currNode->elements.find(vPath[vPath.size() - 1]) != currNode->elements.end())
            {
                return currNode->elements[vPath[vPath.size() - 1]];
            }
        }
        else if (path == "/")
        {
            return root;
        }
    }

    return nullptr;
}

// Opens file, and sets up dynamic wad object which is to be returned.
Wad *Wad::loadWad(const string &path)
{
    Wad *wad = new Wad();

    ifstream file(path, ios::binary | ios::ate); // Open WAD file at the end.
    if (file.is_open())
    {
        streampos size = file.tellg(); // Get the size of the file

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
            wad->offDesc += (wad->data[currByte++] << (8 * i));
        }
        currByte = wad->offDesc;

        string currPath = ""; // Tracks currently active path
        int EM = 0;           // Tracks current file (if E#M# folder)

        // Go through all descriptors
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

            // Folder with _START Indicator
            if (length == 0 && name.size() > 6 && name.substr(name.size() - 6, 6).find("_START") != string::npos)
            {
                currPath += "/" + name.substr(0, name.size() - 6);
                wad->addNode(currPath, length, offset, true);
            }
            // Folder with _END Indicator
            else if (length == 0 && name.size() > 4 && name.substr(name.size() - 4, 4).find("_END") != string::npos)
            {
                currPath = currPath.substr(0, currPath.size() - (name.size() - 4) - 1);
            }
            // Folder with E#M# Indicator
            else if (length == 0 && name.size() == 4 && isdigit(name[1]) && isdigit(name[3]))
            {
                currPath += "/" + name;
                wad->addNode(currPath, length, offset, true);
                EM = 10;
            }
            // File
            else if (length >= 0)
            {
                // Length of 0 should be a folder, but could technically be a file.
                if (length == 0)
                {
                    cout << "WARNING: Descriptor: (" << offset << ", " << length << ", " << name << "), treating as file..." << endl;
                }

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
            // Negative length value
            else
            {
                cout << "WARNING: Invalid size descriptor, negative value, ignoring..." << endl;
            }
        }
    }

    return wad;
}

// Returns magic (wad file type)
string Wad::getMagic()
{
    return this->magic;
}

// Returns length of file if path is valid, -1 otherwise.
int Wad::getSize(const string &path)
{
    auto node = getNode(path);

    if (node && !node->isDirectory)
    {
        return node->length;
    }

    return -1;
}

// Copies bytes to buffer if path is to a valid file.
// Returns number of bytes copied if valid path, -1 otherwise.
int Wad::getContents(const string &path, char *buffer, int length, int offset)
{
    auto node = getNode(path);

    if (node && !node->isDirectory)
    {
        // Reduce length if passed in values are improper
        if (length > node->length || offset + length > node->length)
        {
            cout << "WARNING: Given length was too big, continuing with reduced length..." << endl;
            length = node->length;
        }

        // Copy bytes to buffer
        for (int i = 0; i < length; i++)
        {
            buffer[i] = data[node->offset + offset + i];
        }

        return length;
    }
    else
    {
        return -1;
    }
}

// Copies elements to directory vector if path is to a valid folder.
// Returns number of elements copied if valid path, -1 otherwise.
int Wad::getDirectory(const string &path, vector<string> *directory)
{
    auto node = getNode(path);

    if (node && node->isDirectory)
    {
        map<int, string> ordered; // Elements ordered by WAD.
        for (auto i = node->elements.begin(); i != node->elements.end(); i++)
        {
            ordered[i->second->order] = i->first;
        }

        // Copy elements to directory vector
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

// Returns true if path is to a valid file.
bool Wad::isContent(const string &path)
{
    auto node = getNode(path);
    return (node && !node->isDirectory) ? true : false;
}

// Returns true if path is to a valid folder.
bool Wad::isDirectory(const string &path)
{
    auto node = getNode(path);
    return (node && node->isDirectory) ? true : false;
}

// Destructor
Wad::~Wad()
{
    queue<Node *> nodes;
    nodes.push(root);

    while (!nodes.empty())
    {
        auto node = nodes.front();
        nodes.pop();

        for (auto i = node->elements.begin(); i != node->elements.end(); i++)
        {
            nodes.push(i->second);
        }

        delete node;
    }

    delete[] data;
}