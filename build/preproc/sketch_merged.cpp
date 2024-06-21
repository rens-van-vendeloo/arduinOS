# 1 "C:\\src\\besturingsysteem\\besturingsysteem.ino"
# 2 "C:\\src\\besturingsysteem\\besturingsysteem.ino" 2

// Definitions


// Function Declarations
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
    char name[12];
    void (*func)();
} commandType;

struct FATEntry
{
    char filename[13]; // 12 characters for filename + 1 character for terminating zero
    int startPos; // starting position in EEPROM
    int length; // length of the file
};

// constants
const int MAX_FILES = 10; // maximum number of files
const int FAT_START_ADDR = 0; // starting address of the FAT in EEPROM
const int FAT_ENTRY_SIZE = sizeof(FATEntry); // size of each FAT entry in bytes

// variables
int commandListSize = 15;
int debugCommandSize = 2;
int commandLength = 0;
int argLength = 0;
char commandBuffer[12];
char argsBuffer[12];
bool devmode = false;
bool argsSet = false;
int noOfFiles = 0;

// TODO: fix command[] so it is the same as commands described in available_commands_with_args.png
static commandType command[] = {
    {"store", &fileAdd}, // takes args bestand, grootte, data
    {"retrieve", &fileGet}, // takes args bestand
    {"erase", &fileDelete}, // takes args bestand
    {"files", &files}, // takes in no args
    {"freespace", &fileSpaceAvailable}, // takes in no args
    {"run", &processStart}, // takes args bestand
    {"list", &list}, // takes in no args
    {"suspend", &processPause}, // takes args process id (pid)
    {"resume", &processResume}, // takes args process id (pid)
    {"kill", &processAbort}, // takes args process id (pid)
    // {"memory_space", &memSpaceAvailable},
    {"var_add", &memAdd}, // takes args naam, datatype, adress, pid
    {"var_delete", &memDelete}, // takes args naam, pid
    {"var_get", &memGet}, // takes args naam, pid
    {"devmode", &toggleDevMode}};

static commandType debug[] = {
    {"reload_cli", &reloadCLI}, // for debugging purposes
    // {"dummyfunc", &dummyfunc}
};

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
        if (entry.startPos - previousEndPos >= size)
        {
            availableIndex = i;
            break;
        }
        previousEndPos = entry.startPos + entry.length;
    }

    if (availableIndex == -1)
    {
        Serial.println("Error: Not enough space to allocate the file.");
        return false;
    }

    FATEntry newEntry;
    strcpy(newEntry.filename, filename);
    newEntry.startPos = previousEndPos;
    newEntry.length = size;

    writeFATEntry(availableIndex, newEntry);
    noOfFiles++;

    Serial.println("File successfully allocated.");
    return true;
}

void setup()
{
    Serial.begin(9600);
    Serial.println("ArduinOS>> Loading...");
    Serial.println("ArduinOS>> Ready");

    // Initialize EEPROM
    EEPROM.begin();

    // Write example FAT entries (you can replace this with your own initialization logic)
    FATEntry entry1 = {"file1.txt", 0, 100};
    FATEntry entry2 = {"file2.txt", 100, 200};
    writeFATEntry(0, entry1);
    writeFATEntry(1, entry2);

    // Example usage: search for a file in the FAT
    int fileIndex = findFileInFAT("file2.txt");
    if (fileIndex != -1)
    {
        FATEntry file = readFATEntry(fileIndex);
        // Do something with the file entry
        Serial.print("Found file: ");
        Serial.println(file.filename);
        Serial.print("Start position: ");
        Serial.println(file.startPos);
        Serial.print("Length: ");
        Serial.println(file.length);
    }
    else
    {
        Serial.println("File not found.");
    }
}

void loop()
{
    executeCommand(getCommand());
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
    for (int i = 0; i < 12; i++)
    {
        commandBuffer[i] = 
# 217 "C:\\src\\besturingsysteem\\besturingsysteem.ino" 3 4
                          __null
# 217 "C:\\src\\besturingsysteem\\besturingsysteem.ino"
                              ;
    }
}

void clearArgBuffer()
{
    for (int i = 0; i < 12; i++)
    {
        argsBuffer[i] = 
# 225 "C:\\src\\besturingsysteem\\besturingsysteem.ino" 3 4
                       __null
# 225 "C:\\src\\besturingsysteem\\besturingsysteem.ino"
                           ;
    }
}

void reloadCLI()
{
    Serial.println("Reloading arg and command buffer");
    clearArgBuffer();
    clearCommandBuffer();
    Serial.println("Done reloading...");
}

int getCommand()
{

    char c;
    while (Serial.available())
    {
        // Place character in buffer
        c = Serial.read();
        if (c == ' ' || c == '\r' || c == '\n')
        {
            if ((c == ' ') && !argsSet)
            {
                argsSet = true;
                continue;
            }
            if (argsSet)
            {
                Serial.print("ArduinOS>> ");
                Serial.print(commandBuffer);
                Serial.print(" ");
                Serial.println(argsBuffer);
            }
            else
            {
                Serial.print("ArduinOS>> ");
                Serial.println(commandBuffer);
            }
            // reset commandLength value to 0 when done getting command
            commandLength = 0;
            argLength = 0;
            // return value 1 to signify a completed input command
            return 1;
        }
        else if (argsSet)
        {
            argsBuffer[argLength] = c;
            argLength++;
        }
        else
        {
            commandBuffer[commandLength] = tolower(c);
            commandLength++;
        }
    }
    return 0;
}

void executeCommand(int commandStatus)
{ // function to execute possible commands
    // return, if commandStatus is not equal to 1
    if (commandStatus != 1)
    {
        return;
    }
    // set boolean to check if entered command is present in the command list
    bool commandFound = false;
    bool devCommandFound = false;
    // set command entry to begin at the start of the command array.
    int entry = 0;
    int result = 1;
    // LOOP OVER STRUCT containing user commands
    for (entry = 0; entry < commandListSize; entry++)
    {
        // compare input command with command in command list
        result = strcmp(command[entry].name, commandBuffer);
        // if the input command is the same as command in command list the result is equal to 0
        if (result == 0)
        {
            // set commandFound to true
            commandFound = true;
            // break for-loop to perserve the value of the entry variable for reuse in function
            break;
        }
    }
    // check for developer commands if devmode is enabled
    if (devmode && !commandFound)
    {
        entry = 0;
        for (entry = 0; entry < debugCommandSize; entry++)
        {
            // compare input command with command in devmode list
            result = strcmp(debug[entry].name, commandBuffer);
            // if the input command is the same as command in devmode list the result is equal to 0
            if (result == 0)
            {
                // set commandFound to true
                devCommandFound = true;
                // break for-loop to perserve the value of the entry variable for reuse in function
                break;
            }
        }
    }

    // if command entered is not present in the command list print command not found warning
    if ((devCommandFound == false) && (commandFound == false))
    {
        Serial.println("Command not found. Please try again..");
    }
    // empty characters in commandBuffer array for reuse.
    clearCommandBuffer();
    // TO DO:    - IMPLEMENT ARGS
    //           - CLEAR ARGS BUFFER
    //           -fix serial buffer overflow
    clearArgBuffer();
    // clearing serial buffer
    Serial.flush();
    // execute function of found command
    if (commandFound == true)
    {
        command[entry].func();
    }
    if (devCommandFound == true)
    {
        debug[entry].func();
    }
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

// File Management
void fileGet()
{ // get file from FAT
    // STUB
    Serial.println("File Get");
}

void fileAdd()
{ // add file to FAT
    // STUB
    Serial.println("File Add");

}

void fileDelete()
{ // delete file from FAT
    // STUB
    Serial.println("File Delete");
}

void files()
{ // get list from files in FAT
    // STUB
    Serial.println("File list");
}

void fileSpaceAvailable()
{ // check available space in FAT
    // STUB
    Serial.println("File spaca available");
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
