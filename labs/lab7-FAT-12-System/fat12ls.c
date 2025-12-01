/* fat12ls.c 
* * Displays the files in the root sector of an MSDOS floppy disk (CprE 308 Lab 7, Part II)
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h> // For memcpy

#define SIZE 32      /* size of the read buffer for the boot sector */
#define ROOTSIZE 256 /* max number of directory entries we might read (224 entries * 32 bytes/entry) */

struct BootSector
{
    unsigned char  sName[9];            
    unsigned short iBytesSector;        
    unsigned char  iSectorsCluster;     
    unsigned short iReservedSectors;    
    unsigned char  iNumberFATs;         
    unsigned short iRootEntries;        
    unsigned short iLogicalSectors;     
    unsigned char  xMediumDescriptor;   
    unsigned short iSectorsFAT;         
    unsigned short iSectorsTrack;       
    unsigned short iHeads;              
    unsigned short iHiddenSectors;      
};


// --- Function Prototypes ---
unsigned short endianSwap(unsigned char one, unsigned char two);
void decodeBootSector(struct BootSector * pBootS, unsigned char buffer[]);
void parseDirectory(int iDirOff, int iEntries, unsigned char buffer[], int bytes_per_sector);
char * toDOSName(char string[], unsigned char buffer[], int offset);
char * parseAttributes(char string[], unsigned char key);
char * parseTime(char string[], unsigned short usTime);
char * parseDate(char string[], unsigned short usDate);
unsigned int decodeSize(unsigned char *buffer, int offset);


// reads the boot sector and root directory
int main(int argc, char * argv[])
{
    int fd_image = 0;
    unsigned char buffer[SIZE];
    // Allocate space based on max possible entries (256 * 32 bytes)
    unsigned char rootBuffer[ROOTSIZE * 32];
    struct BootSector sector;
    int iRDOffset = 0;
    
    if (argc < 2) {
    	printf("Specify boot sector\n");
    	exit(1);
    }
    
    // 1. Read Boot Sector
    fd_image = open(argv[1], O_RDONLY);
    if (fd_image < 0) {
        perror("Error opening image file");
        exit(1);
    }
    read(fd_image, buffer, SIZE);
    
    // 2. Decode Boot Sector (using the completed logic)
    decodeBootSector(&sector, buffer);
    
    // 3. Calculate the location of the root directory (Y)
    // Y = Reserved Sectors + (Sectors/FAT * Number of FATs)
    int sector_Y = sector.iReservedSectors + (sector.iSectorsFAT * sector.iNumberFATs);
    iRDOffset = sector_Y * sector.iBytesSector;
                 
    printf("root dir offset: %d\n", iRDOffset);
	
    // 4. Read the root directory into buffer
    int root_dir_bytes_to_read = sector.iRootEntries * 32;

    lseek(fd_image, iRDOffset, SEEK_SET);
    read(fd_image, rootBuffer, root_dir_bytes_to_read); 
    close(fd_image);
    
    // 5. Parse the root directory
    printf("max # root dir entries: %d\n", sector.iRootEntries);
    parseDirectory(iRDOffset, sector.iRootEntries, rootBuffer, sector.iBytesSector);
    
} // end main


// --- DECODING HELPERS ---

unsigned short endianSwap(unsigned char one, unsigned char two)
{
	unsigned short res = two;
	res *= 256;
	res += one;
    return res;
}

unsigned int decodeSize(unsigned char *buffer, int offset) {
    // Size is a 4-byte little-endian value (Offset 0x1C)
    // Byte order: [0] [1] [2] [3] -> LSB to MSB
    unsigned int size = 0;
    
    size = buffer[offset + 3];
    size = (size << 8) | buffer[offset + 2];
    size = (size << 8) | buffer[offset + 1];
    size = (size << 8) | buffer[offset + 0];
    return size;
}

void decodeBootSector(struct BootSector * pBootS, unsigned char buffer[])
{
    // Offsets based on Table 2
    memcpy(pBootS->sName, buffer + 0x03, 8); pBootS->sName[8] = '\0'; 
    pBootS->iBytesSector = endianSwap(buffer[0x0B], buffer[0x0C]);
    pBootS->iSectorsCluster = buffer[0x0D]; 
    pBootS->iReservedSectors = endianSwap(buffer[0x0E], buffer[0x0F]);
    pBootS->iNumberFATs = buffer[0x10]; 
    pBootS->iRootEntries = endianSwap(buffer[0x11], buffer[0x12]);
    pBootS->iLogicalSectors = endianSwap(buffer[0x13], buffer[0x14]);
    pBootS->xMediumDescriptor = buffer[0x15];
    pBootS->iSectorsFAT = endianSwap(buffer[0x16], buffer[0x17]);
    pBootS->iSectorsTrack = endianSwap(buffer[0x18], buffer[0x19]);
    pBootS->iHeads = endianSwap(buffer[0x1A], buffer[0x1B]);
    pBootS->iHiddenSectors = endianSwap(buffer[0x1C], buffer[0x1D]);
}


// --- DIRECTORY PARSING (Exercise 3) ---

void parseDirectory(int iDirOff, int iEntries, unsigned char buffer[], int bytes_per_sector)
{
    int i = 0;
    char string[13];
    
    printf("File_Name\tAttr\tTime\t\tDate\t\tSize\n");
    
    // Loop through all possible directory entries (iEntries * 32 bytes)
    for(i = 0; i < (iEntries); i++)   {
        int entry_offset = i * 32;

        // Check for invalid entries (Offset 0x00)
        unsigned char status = buffer[entry_offset + 0x00];
        
        if (status == 0x00)
    		break; // 0x00 means end of valid entries
    		
    	if (status == 0xe5)
    		continue; // 0xE5 means entry is deleted
        
        // Skip Long File Name (LFN) entries (Attribute byte 0x0B == 0x0F)
        if (buffer[entry_offset + 0x0B] == 0x0F)
            continue; 

        // --- Filename (Offset 0x00) ---
		toDOSName(string, buffer, entry_offset + 0x00);
		printf("%-12s\t", string);
		
		// --- Attributes (Offset 0x0B) ---
		parseAttributes(string, buffer[entry_offset + 0x0B]);
		printf("%s\t", string);
		
		// --- Time (Offset 0x16 - Last Write/Creation Time) ---
		unsigned short usTime = endianSwap(buffer[entry_offset + 0x16], buffer[entry_offset + 0x17]); 
        parseTime(string, usTime); 
		printf("%s\t", string);
		
		// --- Date (Offset 0x18 - Last Write/Creation Date) ---
		unsigned short usDate = endianSwap(buffer[entry_offset + 0x18], buffer[entry_offset + 0x19]);
        parseDate(string, usDate);
		printf("%s\t", string);
		
		// --- Size (Offset 0x1C) ---
		unsigned int size = decodeSize(buffer, entry_offset + 0x1C);
		printf("%d\n", size);
    }
    
    printf("(R)ead Only (H)idden (S)ystem (A)rchive\n");
}


// Parses the attributes bits of a file
char * parseAttributes(char *string, unsigned char key)
{
 	int i=0;
 	if (key&1)
 		string[i++] = 'R';
 	if ((key>>1)&1)
 		string[i++] = 'H';
 	if ((key>>2)&1)
 		string[i++] = 'S';
 	if ((key>>5)&1)
 		string[i++] = 'A';
 	
 	string[i] = '\0';
    return string;
} 


// Decodes the bits assigned to the time of each file 
char * parseTime(char *time_string, unsigned short time)
{
	// Hour (5 bits), Minute (6 bits), Second (5 bits * 2)
	unsigned short hour = (time>>11)&0x1f;
	unsigned short minute = (time>>5)&0x3f;
	unsigned short second = (time&0x1f)*2;
	
    sprintf(time_string, "%02d:%02d:%02d", hour, minute, second);
			
    return time_string;
    
} 


// Decodes the bits assigned to the date of each file 
char * parseDate(char *date_string, unsigned short date)
{
	// Year (7 bits + 1980), Month (4 bits), Day (5 bits)
	unsigned short year = (date>>9) + 1980;
	unsigned short month = (date>>5)&0xf;
	unsigned short day = date&0x1f;
	
    sprintf(date_string, "%04d/%02d/%02d", year, month, day);

    return date_string;
    
} 


// Formats a filename string as DOS (adds the dot to 8-dot-3)
char * toDOSName(char *string, unsigned char buffer[], int offset)
{
	int index = 0;
	int i;
    
    // Copy 8 characters for filename (buffer[offset] to buffer[offset+7])
	for (i=0; i<8; i++)
		string[index++] = buffer[offset+i];
        
    // Trim trailing spaces
	while (string[--index]==' ');
	
    // Add dot for extension
	string[++index] = '.';
	
    // Copy 3 characters for extension (buffer[offset+8] to buffer[offset+10])
    for (i=8; i<11; i++)
    	string[++index] = buffer[offset+i];
        
    string[++index] = '\0';
    return string;
} 

