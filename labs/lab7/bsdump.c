/* bsdump.c 
 * * Reads the boot sector of an MSDOS floppy disk (CprE 308 Lab 7, Part I)
 */
 
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <string.h> // Required for memcpy

#define SIZE 32  /* size of the read buffer */
//define PRINT_HEX // un-comment this to print the values in hex for debugging

struct BootSector
{
    unsigned char  sName[9];            // The name of the volume
    unsigned short iBytesSector;        // Bytes per Sector (0x0B, 2)
    
    unsigned char  iSectorsCluster;     // Sectors per Cluster (0x0D, 1)
    unsigned short iReservedSectors;    // Reserved sectors (0x0E, 2)
    unsigned char  iNumberFATs;         // Number of FATs (0x10, 1)
    
    unsigned short iRootEntries;        // Number of Root Directory entries (0x11, 2)
    unsigned short iLogicalSectors;     // Number of logical sectors (0x13, 2)
    unsigned char  xMediumDescriptor;   // Medium descriptor (0x15, 1)
    
    unsigned short iSectorsFAT;         // Sectors per FAT (0x16, 2)
    unsigned short iSectorsTrack;       // Sectors per Track (0x18, 2)
    unsigned short iHeads;              // Number of heads (0x1A, 2)
    
    unsigned short iHiddenSectors;      // Number of hidden sectors (0x1C, 2)
};

// Converts two characters to an unsigned short with (two, one)
// Handles little-endian swap (LSB first)
unsigned short endianSwap(unsigned char one, unsigned char two);


// Fills out the BootSector Struct using the buffer array
void decodeBootSector(struct BootSector * pBootS, unsigned char buffer[]);


// Displays the BootSector information to the console
void printBootSector(struct BootSector * pBootS);


// entry point:
int main(int argc, char * argv[])
{
	int fd_image = 0;
	unsigned char buffer[SIZE];
	struct BootSector sector;
    
	// Check for argument
	if (argc < 2) {
		printf("Please specify the boot sector file!\n");
		exit(1);
	}
    
	// Open the file and read the boot sector
	fd_image = open(argv[1], O_RDONLY);
	// Read the first 32 bytes (which contain all the necessary metadata)
	read(fd_image, buffer, SIZE);
    close(fd_image);
    
	// Decode the boot Sector
	decodeBootSector(&sector, buffer);
    
	// Print Boot Sector information
	printBootSector(&sector);
    
    return 0;
} 


// Converts two characters to an unsigned short with (two, one)
unsigned short endianSwap(unsigned char one, unsigned char two)
{
	unsigned short res = two; // two is the Most Significant Byte (MSB)
	res *= 256;               // Shift MSB left by 8 bits
	res += one;               // Add one as the Least Significant Byte (LSB)
    return res;
}


// Fills out the BootSector Struct from the buffer
void decodeBootSector(struct BootSector * pBootS, unsigned char buffer[])
{
	
	// Offset 0x03, Length 8. Volume Label (ASCII).
    memcpy(pBootS->sName, buffer + 0x03, 8);
    pBootS->sName[8] = '\0'; // Null-terminate the string (struct size is 9)
    
	// Offset 0x0B, Length 2. Bytes per logical sector.
    pBootS->iBytesSector = endianSwap(buffer[0x0B], buffer[0x0C]);
    
	// Offset 0x0D, Length 1. Logical sectors per cluster.
    pBootS->iSectorsCluster = buffer[0x0D]; 
    
	// Offset 0x0E, Length 2. Number of reserved sectors.
    pBootS->iReservedSectors = endianSwap(buffer[0x0E], buffer[0x0F]);
    
	// Offset 0x10, Length 1. Number of FATs.
    pBootS->iNumberFATs = buffer[0x10]; 
    
	// Offset 0x11, Length 2. Max number of root directory entries.
    pBootS->iRootEntries = endianSwap(buffer[0x11], buffer[0x12]);
    
	// Offset 0x13, Length 2. Number of logical sectors.
    pBootS->iLogicalSectors = endianSwap(buffer[0x13], buffer[0x14]);
    
	// Offset 0x15, Length 1. Media descriptor.
    pBootS->xMediumDescriptor = buffer[0x15];
    
	// Offset 0x16, Length 2. Logical sectors per FAT.
    pBootS->iSectorsFAT = endianSwap(buffer[0x16], buffer[0x17]);
    
	// Offset 0x18, Length 2. Physical sectors per track.
    pBootS->iSectorsTrack = endianSwap(buffer[0x18], buffer[0x19]);
    
	// Offset 0x1A, Length 2. Number of heads.
    pBootS->iHeads = endianSwap(buffer[0x1A], buffer[0x1B]);
    
	// Offset 0x1C, Length 2. Number of hidden sectors.
    pBootS->iHiddenSectors = endianSwap(buffer[0x1C], buffer[0x1D]);
	
}


// Displays the BootSector information to the console
void printBootSector(struct BootSector * pBootS)
{
#ifndef PRINT_HEX
    printf("                    Name:   %s\n", pBootS->sName);
    printf("            Bytes/Sector:   %i\n", pBootS->iBytesSector);
    printf("         Sectors/Cluster:   %i\n", pBootS->iSectorsCluster);
    printf("        Reserved Sectors:   %i\n", pBootS->iReservedSectors);
    printf("          Number of FATs:   %i\n", pBootS->iNumberFATs);
    printf("  Root Directory entries:   %i\n", pBootS->iRootEntries);
    printf("         Logical sectors:   %i\n", pBootS->iLogicalSectors);
    // Display format requires 0x00f0 format for hex short
    printf("       Medium descriptor:   0x%04x\n", (unsigned short)pBootS->xMediumDescriptor); 
    printf("             Sectors/FAT:   %i\n", pBootS->iSectorsFAT);
    printf("           Sectors/Track:   %i\n", pBootS->iSectorsTrack);
    printf("         Number of heads:   %i\n", pBootS->iHeads);
    printf("Number of Hidden Sectors:   %i\n", pBootS->iHiddenSectors);
#else
    printf("                    Name:   %s\n",     pBootS->sName);
    printf("            Bytes/Sector:   0x%04x\n", pBootS->iBytesSector);
    printf("         Sectors/Cluster:   0x%02x\n", pBootS->iSectorsCluster);
    printf("        Reserved Sectors:   0x%04x\n", pBootS->iReservedSectors);
    printf("          Number of FATs:   0x%02x\n", pBootS->iNumberFATs);
    printf("  Root Directory entries:   0x%04x\n", pBootS->iRootEntries);
    printf("         Logical sectors:   0x%04x\n", pBootS->iLogicalSectors);
    printf("       Medium descriptor:   0x%02x\n", pBootS->xMediumDescriptor);
    printf("             Sectors/FAT:   0x%04x\n", pBootS->iSectorsFAT);
    printf("           Sectors/Track:   0x%04x\n", pBootS->iSectorsTrack);
    printf("         Number of heads:   0x%04x\n", pBootS->iHeads);
    printf("Number of Hidden Sectors:   0x%04x\n", pBootS->iHiddenSectors);
#endif
	return;
}
