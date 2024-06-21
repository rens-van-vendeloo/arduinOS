#include <EEPROM.h>

// Definitions
#define BUFSIZE 12

// Function Declarations
void help();
void executeCommand();
void processStart();
void processPause();
void processResume();
void processAbort();
void list();
void fileGet();
void fileAdd();
void fileDelete();
void files();
void fileSpaceAvailable();
void memSpaceAvailable();
void memAdd();
void memGet();
void memDelete();
void reloadCLI();
void toggleDevMode();
void dummyfunc(int a);

// Structs
typedef struct
{
    char name[BUFSIZE];
    void (*func)();
} commandType;

struct FATEntry
{
    char filename[13]; // 12 characters for filename + 1 character for terminating zero
    int startPos;      // starting position in EEPROM
    int length;        // length of the file
};

// constants
const int MAX_FILES = 10;                    // maximum number of files
const int FAT_START_ADDR = 0;                // starting address of the FAT in EEPROM
const int FAT_ENTRY_SIZE = sizeof(FATEntry); // size of each FAT entry in bytes
const int EEPROM_SIZE = 1024;                // size of EEPROM (example value)

// variables
int commandListSize = 15;
int debugCommandSize = 2;
int commandLength = 0;
int argLength = 0;
char commandBuffer[BUFSIZE];
char argsBuffer[BUFSIZE];
bool devmode = false;
bool argsSet = false;
int noOfFiles = 0;

// TODO: fix command[] so it is the same as commands described in available_commands_with_args.png
static commandType command[] = {
    {"help", &help},                    // takes in no args
    {"store", &fileAdd},                // takes args bestand, grootte, data
    {"retrieve", &fileGet},             // takes args bestand
    {"erase", &fileDelete},             // takes args bestand
    {"files", &files},                  // takes in no args
    {"freespace", &fileSpaceAvailable}, // takes in no args
    {"run", &processStart},             // takes args bestand
    {"list", &list},                    // takes in no args
    {"suspend", &processPause},         // takes args process id (pid)
    {"resume", &processResume},         // takes args process id (pid)
    {"kill", &processAbort},            // takes args process id (pid)
    // {"memory_space", &memSpaceAvailable},
    {"var_add", &memAdd},       // takes args naam, datatype, adress, pid
    {"var_delete", &memDelete}, // takes args naam, pid
    {"var_get", &memGet},       // takes args naam, pid
    {"devmode", &toggleDevMode}};

static commandType debug[] = {
    {"reload_cli", &reloadCLI}, // for debugging purposes
    // {"dummyfunc", &dummyfunc}
};

//file add
void fileAdd()
{
    char *filename = strtok(argsBuffer, ",");
    int size = atoi(strtok(NULL, ","));
    char *data = strtok(NULL, "");

    if (filename == NULL || size == 0 || data == NULL)
    {
        Serial.println("Error: Invalid arguments.");
        return;
    }

    if (STORE(filename, size, data))
    {
        Serial.println("File stored successfully.");
    }
    else
    {
        Serial.println("Error: File could not be stored.");
    }
}

// Function to write a FAT entry to EEPROM
void writeFATEntry(int index, const FATEntry &entry)
{
    int addr = FAT_START_ADDR + index * FAT_ENTRY_SIZE;
    EEPROM.put(addr, entry);
}

// Function to read a FAT entry from EEPROM
FATEntry readFATEntry(int index)
{
    FATEntry entry;
    int addr = FAT_START_ADDR + index * FAT_ENTRY_SIZE;
    EEPROM.get(addr, entry);
    return entry;
}

// Function to search for a file in the FAT by name
int findFileInFAT(const char *filename)
{
    for (int i = 0; i < MAX_FILES; i++)
    {
        FATEntry entry = readFATEntry(i);
        if (strcmp(entry.filename, filename) == 0)
        {
            return i; // file found, return the index
        }
    }
    return -1; // file not found
}

bool STORE(const char* filename, int size, char *data)
{
    if (noOfFiles >= MAX_FILES)
    {
        Serial.println("Error: Maximum number of files reached.");
        return false;
    }

    if (findFileInFAT(filename) != -1)
    {
        Serial.println("Error: File with the same name already exists.");
        return false;
    }

    int availableIndex = -1;
    int previousEndPos = FAT_START_ADDR + MAX_FILES * FAT_ENTRY_SIZE;

    for (int i = 0; i < MAX_FILES; i++)
    {
        FATEntry entry = readFATEntry(i);
        if (entry.startPos == -1) // Check if this entry is empty
        {
            availableIndex = i;
            break;
        }
        if (entry.startPos - previousEndPos >= size)
        {
            availableIndex = i;
            break;
        }
        previousEndPos = entry.startPos + entry.length;
    }

    if (availableIndex == -1)
    {
        if (EEPROM_SIZE - previousEndPos >= size)
        {
            availableIndex = noOfFiles;
        }
        else
        {
            Serial.println("Error: Not enough space to allocate the file.");
            return false;
        }
    }

    FATEntry newEntry;
    strcpy(newEntry.filename, filename);
    newEntry.startPos = previousEndPos;
    newEntry.length = size;

    writeFATEntry(availableIndex, newEntry);
    noOfFiles++;

    // Write the data to EEPROM
    for (int i = 0; i < size; i++)
    {
        EEPROM.write(previousEndPos + i, data[i]);
    }

    Serial.println("File successfully allocated.");
    return true;
}

// Function to retrieve a file from EEPROM
void fileGet()
{
    char* filename = argsBuffer; // assuming the filename is stored in argsBuffer

    int fileIndex = findFileInFAT(filename);
    if (fileIndex == -1)
    {
        Serial.println("Error: File not found.");
        return;
    }

    FATEntry entry = readFATEntry(fileIndex);
    Serial.print("Filename: ");
    Serial.println(entry.filename);
    Serial.print("Size: ");
    Serial.println(entry.length);

    Serial.print("Data: ");
    for (int i = 0; i < entry.length; i++)
    {
        Serial.print((char)EEPROM.read(entry.startPos + i));
    }
    Serial.println();
}

// Function to delete a file from EEPROM
void fileDelete()
{
    char* filename = argsBuffer; // assuming the filename is stored in argsBuffer

    int fileIndex = findFileInFAT(filename);
    if (fileIndex == -1)
    {
        Serial.println("Error: File not found.");
        return;
    }

    FATEntry emptyEntry = {"", -1, 0};
    writeFATEntry(fileIndex, emptyEntry);
    noOfFiles--;

    Serial.println("File deleted successfully.");
}

// Function to list all files in FAT
void files()
{
    Serial.println("Files in FAT:");
    for (int i = 0; i < MAX_FILES; i++)
    {
        FATEntry entry = readFATEntry(i);
        if (entry.startPos != -1)
        {
            Serial.print("Filename: ");
            Serial.println(entry.filename);
            Serial.print("Start Pos: ");
            Serial.println(entry.startPos);
            Serial.print("Size: ");
            Serial.println(entry.length);
        }
    }
}

// Function to check available space in FAT
void fileSpaceAvailable()
{
    int previousEndPos = FAT_START_ADDR + MAX_FILES * FAT_ENTRY_SIZE;
    for (int i = 0; i < MAX_FILES; i++)
    {
        FATEntry entry = readFATEntry(i);
        if (entry.startPos != -1)
        {
            previousEndPos = entry.startPos + entry.length;
        }
    }

    int freeSpace = EEPROM_SIZE - previousEndPos;
    Serial.print("Available space: ");
    Serial.print(freeSpace);
    Serial.println(" bytes");
}

void setup()
{
    Serial.begin(9600);
    Serial.println("ArduinOS>> Loading...");
    Serial.println("ArduinOS>> Ready");

    // Initialize noOfFiles based on existing FAT entries
    for (int i = 0; i < MAX_FILES; i++)
    {
        FATEntry entry = readFATEntry(i);
        if (entry.startPos != -1)
        {
            noOfFiles++;
        }
    }
}

void loop()
{
    if (getCommand())
    {
        executeCommand();
    }
}

void toggleDevMode()
{
    if (!devmode)
    {
        devmode = true;
        Serial.println("Devmode ON");
    }
    else
    {
        devmode = false;
        Serial.println("Devmode OFF");
    }
}

void dummyfunc(int a)
{
    Serial.println("DummyFUNC");
    Serial.println(a);
}

void clearCommandBuffer()
{
    memset(commandBuffer, 0, BUFSIZE);
    commandLength = 0;
}

void clearArgBuffer()
{
    memset(argsBuffer, 0, BUFSIZE);
    argLength = 0;
}

void reloadCLI()
{
    Serial.println("Reloading arg and command buffer");
    clearArgBuffer();
    clearCommandBuffer();
    Serial.println("Done reloading...");
}

bool getCommand()
{
    while (Serial.available())
    {
        char c = Serial.read();
        if (c == '\n' || c == '\r')
        {
            commandBuffer[commandLength] = '\0';
            return true;
        }
        else
        {
            commandBuffer[commandLength++] = tolower(c);
            if (commandLength >= BUFSIZE - 1)
            {
                commandBuffer[BUFSIZE - 1] = '\0';
                return true;
            }
        }
    }
    return false;
}

void parseCommand()
{
    char *token = strtok(commandBuffer, " ");
    if (token != NULL)
    {
        strcpy(commandBuffer, token);
        token = strtok(NULL, "");
        if (token != NULL)
        {
            strcpy(argsBuffer, token);
            argsSet = true;
        }
        else
        {
            argsSet = false;
        }
    }
}

void executeCommand()
{
    parseCommand();

    bool commandFound = false;
    bool devCommandFound = false;

    for (int entry = 0; entry < commandListSize; entry++)
    {
        if (strcmp(command[entry].name, commandBuffer) == 0)
        {
            commandFound = true;
            command[entry].func();
            break;
        }
    }

    if (devmode && !commandFound)
    {
        for (int entry = 0; entry < debugCommandSize; entry++)
        {
            if (strcmp(debug[entry].name, commandBuffer) == 0)
            {
                devCommandFound = true;
                debug[entry].func();
                break;
            }
        }
    }

    if (!commandFound && !devCommandFound)
    {
        Serial.println("Command not found. Please try again..");
    }

    clearCommandBuffer();
    clearArgBuffer();
}

// Process Management
void list()
{ // get list of current processes
    // STUB
    Serial.println("process list");
}

void processStart()
{ // start new process
    // STUB
    Serial.println("Process Start");
}

void processPause()
{ // pause process
    // STUB
    Serial.println("Process Pause");
}

void processResume()
{ // resume process
    // STUB
    Serial.println("Process Resume");
}

void processAbort()
{ // abort process
    // STUB
    Serial.println("Process Abort");
}

// Memory Management
void memSpaceAvailable()
{ // check available space in mem stack
    // STUB
    Serial.println("Mem space available");
}

void memAdd()
{ // add var to mem stack
    // STUB
    Serial.println("Mem Add");
}

void memGet()
{ // get var from mem stack
    // STUB
    Serial.println("Mem get");
}

void memDelete()
{ // delete var from mem stack
    // STUB
    Serial.println("Mem delete");
}

void help(){
    Serial.println("Available commands:");
    for (int i = 0; i < commandListSize; i++)
    {
        Serial.println(command[i].name);
    }
}